#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

void
butter_print_lookup_ctx(butter_req_t * req)	{
	butter_kv_lookup_ctx_t * ctx = &req->lookup_ctx;
	int i;

	printf("DBG: ==== lookup ctx info begin =====\n");
	printf("    hash_branch_depth = %d\n", req->hash_branch_depth);
	printf("    d_idx = %d\n", req->d_idx);
	printf("    blk_location = %zd\n", ctx->blk_location);

	printf("    hash_bar_starts[] = [");
	for (i = 0; i < HASH_LENGTH; i ++)	{
		printf(" %lx", req->hash_bar_starts[i]);
	}
	printf(" ]\n");

	printf("    hash_bar_item_cnts[] = [");
	for (i = 0; i < HASH_LENGTH; i ++)	{
		printf(" %u", req->hash_bar_item_cnts[i]);
	}
	printf(" ]\n");

	printf("    data_blks: ");
	if (MAGIC_BLK_BDB_DATA == req->d_magic)	{
		printf(" %lu -> { dt, start %lu, length %d } -> %lu\n"
				, req->d_prev_start, req->d_start, req->d_blk_length, req->d_next_start
				);
	}
	else if (MAGIC_BLK_BDB_DATA_EX == req->d_magic)	{
		printf(" %lu -> { ex, start %lu, length %d } -> %lu\n"
				, req->d_prev_start, req->d_start, req->d_blk_length, req->d_next_start
				);
	}
	else if (0 == req->d_magic)	{
		printf(" { 0 }\n");
	}
	else	{
		printf(" { ?, ?, ? }\n");
	}

	printf("DBG: ==== lookup ctx info end =====\n");

	return;
}

/* return values: */
/* BDB_OK:          find the k/v */
/* BDB_NOT_FOUND:   k/v not found */
/* BDB_PT_IO_REQ:   i/o request */
/* BDB_PARAM_ERROR: internal error */
/* BDB_DATA_ERROR:  invalid data from disk */
/* BDB_IO_ERROR:    i/o error */
/* BDB_NO_MEMORY:   out of memory */
int
butter_pt_kv_lookup(butter_req_t * req)	{
	butter_kv_lookup_ctx_t * ctx = &req->lookup_ctx;
	butter_data_ex_blk * data_ex_ptr;
	butter_data_blk * data_ptr;
	int pos;

	PT_BEGIN

	ctx->blk_location = req->db->info_blk.tree_root;

	for (req->hash_branch_depth = 0;
			req->hash_branch_depth <= HASH_LENGTH;
			req->hash_branch_depth ++)
	{

		if (ctx->blk_location)	{
			req->io_request_location = ctx->blk_location;
			yield_to_io_and_check_result(BDB_IO_SEEK);
		}
		else	{
			PT_SET;
			return BDB_NOT_FOUND;
		}

		req->io_request_buffer = &ctx->pre_read_buffer;
		req->io_request_size = sizeof(ctx->pre_read_buffer);
		yield_to_io_and_check_result(BDB_IO_READ);

		if (MAGIC_BLK_BDB_HASH_BAR == ctx->pre_read_buffer.magic)	{
			req->hash_bar_starts[req->hash_branch_depth] = ctx->blk_location;
			req->hash_bar_item_cnts[req->hash_branch_depth] = ctx->pre_read_buffer.cnt;

			/* load the next position from hash bar */
			pos = req->hash[req->hash_branch_depth];
			req->io_request_location = HASH_BAR_JUMP_ITEM_LOCATION(ctx->blk_location, pos);
			yield_to_io_and_check_result(BDB_IO_SEEK);
			req->io_request_buffer = &ctx->blk_location;
			req->io_request_size = sizeof(ctx->blk_location);
			yield_to_io_and_check_result(BDB_IO_READ);
			continue;
		}
		else if (MAGIC_BLK_BDB_DATA == ctx->pre_read_buffer.magic)	{
			goto __data_blk;
		}
		else if (MAGIC_BLK_BDB_DATA_EX == ctx->pre_read_buffer.magic)	{
			goto __data_blk_link_list;
		}
		else	{
			printf("ERROR: %s(), %d: invalid magic value - 0x%08x\n"
					, __func__, __LINE__, ctx->pre_read_buffer.magic);
			return BDB_DATA_ERROR;
		}
	}

	printf("ERROR: %s(), %d: not supposed to access a hash bar deeper than %d\n"
			, __func__, __LINE__, HASH_LENGTH
			);

	PT_SET;

	return BDB_DATA_ERROR;

__data_blk:

	if (req->d_idx >= HASH_COLLISION_MAX)	{
		printf("ERROR: %s, %d, too many hash collisions\n", __func__, __LINE__);
		PT_SET;
		return BDB_DATA_ERROR;	
	}

	if (req->d_key)	{
		Free(req->d_key);
		req->d_key = NULL;
		req->d_key_len = 0;
	}
	req->d_prev_start = req->d_start;
	req->d_next_start = 0;
	req->d_start = ctx->blk_location;
	req->d_blk_length = ctx->pre_read_buffer.blk_length;
	req->d_magic = ctx->pre_read_buffer.magic;
	req->io_request_buffer = &ctx->d_header_loading_buffer.key_length;
	req->io_request_size
			= sizeof(ctx->d_header_loading_buffer) - sizeof(ctx->pre_read_buffer);
	yield_to_io_and_check_result(BDB_IO_READ);

	data_ptr = &ctx->d_header_loading_buffer;

	if (data_ptr->blk_length < DISK_SIZE_UNIT)	{
		printf("ERROR: %s(): data frame length less than DISK_SIZE_UNIT(%d)\n"
				, __func__, DISK_SIZE_UNIT);
		PT_SET;
		return BDB_DATA_ERROR;
	}

	if (data_ptr->blk_length < sizeof(butter_data_blk))	{
		printf("ERROR: %s(): data frame length less than sizeof(butter_data_blk)\n"
				, __func__);
		PT_SET;
		return BDB_DATA_ERROR;
	}

	if (data_ptr->key_length <= 0)	{
		printf("ERROR: %s(): data blk contains invalid key_length\n", __func__);
		PT_SET;
		return BDB_DATA_ERROR;
	}

	if (data_ptr->value_length < 0)	{
		printf("ERROR: %s(): data blk contains invalid value_length\n", __func__);
		PT_SET;
		return BDB_DATA_ERROR;
	}

	if (data_ptr->key_length != req->key_len)	{
		if ((ctx->flags & LOOKUP_F_LOAD_KEY))	{
			ctx->flags |= LOOKUP_F_MISMATCH;
		}
		else	{
			PT_SET;
			return BDB_NOT_FOUND;
		}
	}

	req->io_request_buffer = req->d_key = Malloc(data_ptr->key_length);
	if (!req->d_key)	{
		PT_SET;
		return BDB_NO_MEMORY;
	}

	req->io_request_size = req->d_key_len = data_ptr->key_length;
	yield_to_io_and_check_result(BDB_IO_READ);

	if ((ctx->flags & LOOKUP_F_MISMATCH))	{
		PT_SET;
		return BDB_NOT_FOUND;
	}
	else if (memcmp(req->d_key, req->key, req->key_len))	{
		PT_SET;
		return BDB_NOT_FOUND;
	}
	else	{
		req->d_value_length = ctx->d_header_loading_buffer.value_length;
		req->d_key_matched = 1;
		PT_SET;
		return BDB_OK;
	}

__data_blk_link_list:

	req->d_first_ex_data = ctx->blk_location;

	if (req->hash_branch_depth != HASH_LENGTH)	{
		printf("ERROR: data ex link list only supposed to be found when "
				"max length of the hash has been used\n");
		PT_SET;
		return BDB_DATA_ERROR;
	}

	for (req->d_idx = 0; req->d_idx < HASH_COLLISION_MAX; req->d_idx ++)	{

		if (req->d_key)	{
			Free(req->d_key);
			req->d_key = NULL;
			req->d_key_len = 0;
		}
		req->d_prev_start = req->d_start;
		req->d_start = ctx->blk_location;
		req->d_blk_length = ctx->pre_read_buffer.blk_length;
		req->d_magic = ctx->pre_read_buffer.magic;

		req->io_request_buffer = &ctx->d_ex_header_loading_buffer.next;
		req->io_request_size
				= sizeof(ctx->d_ex_header_loading_buffer) - sizeof(ctx->pre_read_buffer);
		yield_to_io_and_check_result(BDB_IO_READ);

		req->d_next_start = ctx->d_ex_header_loading_buffer.next;

		data_ex_ptr = &ctx->d_ex_header_loading_buffer;

		if (data_ex_ptr->blk_length < DISK_SIZE_UNIT)	{
			printf("ERROR: %s(): data_ex frame length less than DISK_SIZE_UNIT(%d)\n"
					, __func__, DISK_SIZE_UNIT);
			PT_SET;
			return BDB_DATA_ERROR;
		}
		else if (data_ex_ptr->blk_length < sizeof(butter_data_ex_blk))	{
			printf("ERROR: %s(): data_ex frame length less than "
					"sizeof(butter_data_ex_blk)\n", __func__);
			PT_SET;
			return BDB_DATA_ERROR;
		}

		if (data_ex_ptr->key_length <= 0)	{
			printf("ERROR: %s(): data blk contains invalid key_length\n", __func__);
			PT_SET;
			return BDB_DATA_ERROR;
		}

		if (data_ex_ptr->value_length < 0)	{
			printf("ERROR: %s(): data blk contains invalid value_length\n", __func__);
			PT_SET;
			return BDB_DATA_ERROR;
		}

		if (data_ex_ptr->key_length != req->key_len)	{
			goto __data_blk_link_list_next;
		}

		req->io_request_buffer = req->d_key = Malloc(req->key_len);
		if (!req->io_request_buffer)	{
			PT_SET;
			return BDB_NO_MEMORY;
		}

		req->io_request_size = req->d_key_len = req->key_len;
		yield_to_io_and_check_result(BDB_IO_READ);

		if (memcmp(req->d_key, req->key, req->key_len))	{
			goto __data_blk_link_list_next;
		}
		else	{
			req->d_value_length = ctx->d_ex_header_loading_buffer.value_length;
			req->d_key_matched = 1;
			PT_SET;
			return BDB_OK;
		}

__data_blk_link_list_next:

		if (ctx->d_ex_header_loading_buffer.next)	{
			ctx->blk_location = ctx->d_ex_header_loading_buffer.next;
		}
		else	{
			PT_SET;
			return BDB_NOT_FOUND;
		}

		req->io_request_location = ctx->blk_location;
		yield_to_io_and_check_result(BDB_IO_SEEK);

		req->io_request_buffer = &ctx->pre_read_buffer;
		req->io_request_size = sizeof(ctx->pre_read_buffer);
		yield_to_io_and_check_result(BDB_IO_READ);

		if (MAGIC_BLK_BDB_DATA == ctx->pre_read_buffer.magic)	{
			goto __data_blk;
		}
		else if (MAGIC_BLK_BDB_DATA_EX == ctx->pre_read_buffer.magic)	{
			continue;
		}
		else	{
			printf("ERROR: %s(): data_ex chain error\n", __func__);
			PT_SET;
			return BDB_DATA_ERROR;
		}
	}

	printf("ERROR: %s(), %d: too many keys has the same hash value\n"
			, __func__, __LINE__);
	PT_SET;

	return BDB_DATA_ERROR;

	PT_END

	printf("ERROR: %s, %d: not supposed to be here\n", __func__, __LINE__);

	return BDB_INTERNAL_ERROR;
}

/* eof */
