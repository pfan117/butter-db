#include <stdio.h>	/* printf() */
#include <stdlib.h>
#include <strings.h>	/* bzero() */
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

static char butter_padding_data[DISK_SIZE_UNIT] = {0};

int
butter_pt_write_new_data_blk(butter_req_t * req)	{
	butter_write_data_blk_ctx_t * ctx = &req->write_data_blk_ctx;
	size_t pad;
	int r;


	PT_BEGIN

	if (req->d_key_matched)	{
		/* key matched */
		if (MAGIC_BLK_BDB_DATA == req->d_magic)	{
			ctx->need_create_ex_list = 0;
		}
		else if (MAGIC_BLK_BDB_DATA_EX == req->d_magic)	{
			ctx->need_create_ex_list = 1;
		}
		else	{
			printf("ERROR: %s() %d: invalid magic number when key matched\n"
					, __func__, __LINE__);
			PT_SET
			return BDB_INTERNAL_ERROR;
		}
	}
	else if (req->d_key)	{
		if (req->d_key_hash_conflict)	{
			ctx->need_create_ex_list = 1;	/* hash conflict */
		}
		else	{
			ctx->need_create_ex_list = 0;	/* no hash different */
		}
	}
	else	{
		ctx->need_create_ex_list = 0;
	}

	PT_SET

	CLEAN_CTX(alloc_ctx);

	if (ctx->need_create_ex_list)	{
		/* write a data_ex_blk */
		req->alloc_ctx.request_length
				= sizeof(butter_data_ex_blk) + req->key_len + req->value_len;
	}
	else	{
		/* write a data_blk */
		req->alloc_ctx.request_length
				= sizeof(butter_data_blk) + req->key_len + req->value_len;
	}

	PT_SET
	r = butter_alloc_space(req);
	check_sub_pt_result

	/* seek */
	req->io_request_location = req->alloc_ctx.start;
	yield_to_io_and_check_result(BDB_IO_SEEK);

	/* write header */
	if (ctx->need_create_ex_list)	{
		ctx->new_data_ex_blk.magic = MAGIC_BLK_BDB_DATA_EX;
		ctx->new_data_ex_blk.blk_length = req->alloc_ctx.length;

		if (req->d_first_ex_data)	{
			if (req->d_key_matched)	{
				/* replace in original pos */
				ctx->new_data_ex_blk.next = req->d_next_start;
			}
			else	{
				/* create new ex data blk */
				ctx->new_data_ex_blk.next = req->d_first_ex_data;
			}
		}
		else if (req->d_start)	{
			ctx->new_data_ex_blk.next = req->d_start;
		}

		ctx->new_data_ex_blk.key_length = req->key_len;
		ctx->new_data_ex_blk.value_length = req->value_len;
		req->io_request_size = sizeof(ctx->new_data_ex_blk);
		req->io_request_buffer = &ctx->new_data_ex_blk;
	}
	else	{
		ctx->new_data_blk.magic = MAGIC_BLK_BDB_DATA;
		ctx->new_data_blk.blk_length = req->alloc_ctx.length;
		ctx->new_data_blk.key_length = req->key_len;
		ctx->new_data_blk.value_length = req->value_len;
		req->io_request_size = sizeof(ctx->new_data_blk);
		req->io_request_buffer = &ctx->new_data_blk;
	}

	yield_to_io_and_check_result(BDB_IO_WRITE);

	/* write key */
	req->io_request_buffer = req->key;
	req->io_request_size = req->key_len;
	yield_to_io_and_check_result(BDB_IO_WRITE);

	/* write value */
	if (req->value_len && req->value_len > 0)	{
		req->io_request_buffer = req->value;
		req->io_request_size = req->value_len;
		yield_to_io_and_check_result(BDB_IO_WRITE);
	}

	/* pad data */
	if ((req->alloc_ctx.flags & ALLOC_F_EXT_END))	{
		pad = req->alloc_ctx.length - req->alloc_ctx.request_length;
		if (pad > 0)	{
			req->io_request_location = req->alloc_ctx.start + req->alloc_ctx.length - 1;
			yield_to_io_and_check_result(BDB_IO_SEEK);

			req->io_request_buffer = butter_padding_data;
			req->io_request_size = 1;
			yield_to_io_and_check_result(BDB_IO_WRITE);
		}
		else if (pad < 0)	{
			printf("ERROR: %s(), %d: design error, invalid padding data length\n"
					, __func__, __LINE__);
			return BDB_DATA_ERROR;
		}
		else	{
			;	/* no padding */
		}
	}
	else	{
		/* printf("DBG: space reusing, no padding\n"); */
	}

	/* record the start location of the new data blk */
	req->set_ctx.new_data_blk_start = req->alloc_ctx.start;

	PT_SET

	PT_END

	return BDB_OK;
}

/* eof */
