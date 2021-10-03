#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

int
butter_pt_enum(butter_req_t * req)	{
	butter_enum_ctx_t * enum_ctx = &req->enum_ctx;
	butter_req_t * ctx = req;
	int level;
	int i;

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: %s, %d: invalid bdb request magic number\n", __func__, __LINE__);
		return BDB_PARAM_ERROR;
	}

	PT_BEGIN

	/* enum start from beginning */
	butter_hash_zero(req->hash);
	enum_ctx->enum_loaded_hash_depth = 0;

	req->io_request_location = req->db->info_blk.tree_root;
	if (req->io_request_location)	{
		yield_to_io_and_check_result(BDB_IO_SEEK);
	}
	else	{
		PT_SET;
		return BDB_OK;	/* no data item */
	}

	for (;;)	{

__load_item:

		req->io_request_buffer = &enum_ctx->pre_read_buffer;
		req->io_request_size = sizeof(enum_ctx->pre_read_buffer);
		yield_to_io_and_check_result(BDB_IO_READ);

		if (MAGIC_BLK_BDB_HASH_BAR == enum_ctx->pre_read_buffer.magic)	{
			/* load hash bar jump items */
			req->io_request_buffer
					= &enum_ctx->hash_bars[enum_ctx->enum_loaded_hash_depth]->jump;
			req->io_request_size = HASH_BAR_JUMP_SIZE;
			yield_to_io_and_check_result(BDB_IO_READ);
			enum_ctx->hash_bar_item_idx[enum_ctx->enum_loaded_hash_depth] = 0;
			enum_ctx->enum_loaded_hash_depth ++;
		}
		else if (MAGIC_BLK_BDB_DATA == enum_ctx->pre_read_buffer.magic
				|| MAGIC_BLK_BDB_DATA_EX == enum_ctx->pre_read_buffer.magic)
		{
			req->d_key = Malloc(enum_ctx->pre_read_buffer.blk_length);
			if (!req->d_key)	{
				printf("ERROR: %s() %d: out of memory\n", __func__, __LINE__);
				PT_SET;
				return BDB_NO_MEMORY;
			}

			req->io_request_buffer = req->d_key;
			req->io_request_size =
					enum_ctx->pre_read_buffer.blk_length
					- sizeof(enum_ctx->pre_read_buffer);

			yield_to_io_and_check_result(BDB_IO_READ);

			if (MAGIC_BLK_BDB_DATA == enum_ctx->pre_read_buffer.magic)	{
				enum_ctx->cb_param.key_len = (((uint32_t *)req->d_key)[0]);
				enum_ctx->cb_param.value_len = (((uint32_t *)req->d_key)[1]);
				enum_ctx->cb_param.key
						= ((char *)req->d_key) + sizeof(uint32_t) + sizeof(uint32_t);
				enum_ctx->cb_param.value
						= ((char *)req->d_key) + sizeof(uint32_t) + sizeof(uint32_t)
						+ enum_ctx->cb_param.key_len;
			}
			else	{
				/* MAGIC_BLK_BDB_DATA_EX == enum_ctx->pre_read_buffer.magic */
				enum_ctx->ex_data_next = (((uint64_t *)req->d_key)[0]);
				enum_ctx->cb_param.key_len = (((uint32_t *)req->d_key)[2]);
				enum_ctx->cb_param.value_len = (((uint32_t *)req->d_key)[3]);
				enum_ctx->cb_param.key
						= ((char *)req->d_key) + sizeof(uint64_t)
						 + sizeof(uint32_t) + sizeof(uint32_t);
				enum_ctx->cb_param.value
						= ((char *)req->d_key) + sizeof(uint64_t)
						+ sizeof(uint32_t) + sizeof(uint32_t)
						+ enum_ctx->cb_param.key_len;
			}

			if (enum_ctx->data_enum_cb(
					&enum_ctx->cb_param, enum_ctx->data_enum_user_param))	{
				Free(req->d_key);
				req->d_key = NULL;
				printf("DBG: %s() %d: user defined callback quit\n", __func__, __LINE__);
				PT_SET;
				return BDB_OK;
			}
			else	{
				Free(req->d_key);
				req->d_key = NULL;
			}
		}
		else	{
			printf("ERROR: %s() %d: invalid magic value 0x%08x from file\n"
					, __func__, __LINE__, enum_ctx->pre_read_buffer.magic);
			PT_SET;
			return BDB_DATA_ERROR;
		}

		/* change read location for the next item */

		if (enum_ctx->enum_loaded_hash_depth > HASH_LENGTH)	{
			/* change read location, hash bar level too deep */
			printf("ERROR: %s() %d: not supposed to be here\n", __func__, __LINE__);
			PT_SET;
			return BDB_DATA_ERROR;
		}

		if (MAGIC_BLK_BDB_DATA_EX == enum_ctx->pre_read_buffer.magic)	{
			if (enum_ctx->ex_data_next)	{
				req->io_request_location = enum_ctx->ex_data_next;
				yield_to_io_and_check_result(BDB_IO_SEEK);
				goto __load_item;
			}
		}

		if (MAGIC_BLK_BDB_DATA_EX == enum_ctx->pre_read_buffer.magic
				|| MAGIC_BLK_BDB_DATA == enum_ctx->pre_read_buffer.magic
				|| MAGIC_BLK_BDB_HASH_BAR == enum_ctx->pre_read_buffer.magic)
		{
			/* fall through */
		}
		else	{
			printf("ERROR: %s() %d: design error\n", __func__, __LINE__);
			PT_SET;
			return BDB_INTERNAL_ERROR;
		}

		level = enum_ctx->enum_loaded_hash_depth - 1;
		if (level < 0)	{
			PT_SET;
			return BDB_OK;
		}

		for (level = enum_ctx->enum_loaded_hash_depth - 1; level >= 0; level --)	{
			for (i = enum_ctx->hash_bar_item_idx[level]; i < SLOTS_PRE_LEVEL; i ++)
			{
				if (enum_ctx->hash_bars[level]->jump[i])	{
					enum_ctx->hash_bar_item_idx[level] = i + 1;
					req->io_request_location = enum_ctx->hash_bars[level]->jump[i];
					yield_to_io_and_check_result(BDB_IO_SEEK);
					goto __load_item;
				}
			}
		}

		PT_SET;
		return BDB_OK;
	}

	PT_END

	printf("ERROR: %s, %d: not supposed to be here\n", __func__, __LINE__);

	return BDB_INTERNAL_ERROR;
}

int
butter_data_enum(void * db, butter_data_cb cb, void * user_param)
{
	butter_req_t * req;
	int i;
	int r;

	if (!db)	{
		printf("ERROR: parameter error, NULL db pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (!cb)	{
		printf("ERROR: parameter error, NULL cb pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (BUTTER_DB_INFO_MAGIC != ((butter_t *)db)->magic)	{
		printf("ERROR: invalid magic number in db descriptor\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->enum_ctx.data_enum_cb = cb;
	req->enum_ctx.data_enum_user_param = user_param;

	/* alloc memory for hash bar blks, no more pointer checking in main procedure */
	for (i = 0;
			i < (sizeof(req->enum_ctx.hash_bars) / sizeof(req->enum_ctx.hash_bars[0]));
			i ++)
	{
		req->enum_ctx.hash_bars[i] = Malloc(sizeof(butter_hash_bar_blk));
		if (!req->enum_ctx.hash_bars[i])	{
			for (i --; i >= 0; i --)	{
				Free(req->enum_ctx.hash_bars[i]);
			}
			return BDB_NO_MEMORY;
		}
	}

	for(;;)	{
		r = butter_pt_enum(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	/* free memory hash bar memory blks */
	for (i = 0;
			i < (sizeof(req->enum_ctx.hash_bars) / sizeof(req->enum_ctx.hash_bars[0]));
			i ++)
	{
		Free(req->enum_ctx.hash_bars[i]);
	}

	del_butter_request(req);

	return r;
}

/* eof */
