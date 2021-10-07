#include <stdio.h>	/* printf() */
#include <stdlib.h>
#include <strings.h>	/* bzero() */
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

uint64_t butter_u64_zero = 0;

int
butter_pt_del(void * req_ptr)	{
	butter_req_t * req = req_ptr;
	butter_del_ctx_t * ctx = &req->del_ctx;
	int r;
	int i;

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: %s, %d: invalid bdb request magic number\n", __func__, __LINE__);
		return BDB_PARAM_ERROR;
	}

	PT_BEGIN

	butter_calculate_hash(req, req->key, req->key_len, req->hash);

	PT_SET
	r = butter_pt_kv_lookup(req);
	if (BDB_OK == r)	{
		;
	}
	else if (BDB_PT_IO_REQ == r)	{
		return r;	/* yeild */
	}
	else	{
		req->pt_return_value = r;
		PT_SET;
		return req->pt_return_value;
	}

	if (req->d_prev_start)	{
		printf("DBG: %s() %d: no hash bar structure change, adjust data blk link list\n"
				, __func__, __LINE__);
		printf("ERROR: %s() %d: not ready\n", __func__, __LINE__);
		PT_SET;
		return BDB_INTERNAL_ERROR;
	}
	else if (req->d_next_start)	{
		printf("DBG: %s() %d: no hash bar structure change, adjust data blk link list\n"
				, __func__, __LINE__);
		printf("ERROR: %s() %d: not ready\n", __func__, __LINE__);
		PT_SET;
		return BDB_INTERNAL_ERROR;
	}
	else if (req->hash_branch_depth)	{

		ctx->hash_bar_level_to_keep = -1;

		for (i = req->hash_branch_depth - 1; i >= 0; i --)	{
			if (req->hash_bar_item_cnts[i] > 1)	{
				ctx->hash_bar_level_to_keep = i;
				break;
			}
		}

		if (ctx->hash_bar_level_to_keep >= 0)	{
			/* cut off hash bar link list */
			req->io_request_location = HASH_BAR_JUMP_ITEM_LOCATION2(
					ctx->hash_bar_level_to_keep
					);
			yield_to_io_and_check_result(BDB_IO_SEEK);

			/* write zero to jump item */
			butter_u64_zero = 0;
			req->io_request_size = HASH_BAR_JUMP_ITEM_SIZE;
			req->io_request_buffer = &butter_u64_zero;
			yield_to_io_and_check_result(BDB_IO_WRITE);

			/* update the counter */
			req->io_request_location = HASH_BAR_CNT_LOCATION(
					req->hash_bar_starts[ctx->hash_bar_level_to_keep]
					);
			yield_to_io_and_check_result(BDB_IO_SEEK);

			req->hash_bar_item_cnts[ctx->hash_bar_level_to_keep] --;
			req->io_request_buffer = req->hash_bar_item_cnts + ctx->hash_bar_level_to_keep;
			req->io_request_size = HASH_BAR_CNT_SIZE;
			yield_to_io_and_check_result(BDB_IO_WRITE);
		}
		else	{
			/* remove hash bar from info blk */
			req->io_request_location = INFO_BLK_TREE_ROOT;
			yield_to_io_and_check_result(BDB_IO_SEEK);

			butter_u64_zero = 0;
			req->io_request_size = INFO_BLK_TREE_ROOT_SIZE;
			req->io_request_buffer = &butter_u64_zero;
			yield_to_io_and_check_result(BDB_IO_WRITE);

			/* clear tree root location from info structure */
			req->db->info_blk.tree_root = 0;
		}

		/* free hash bars */
		for (ctx->hash_bar_free_idx = req->hash_branch_depth - 1;
				ctx->hash_bar_free_idx > ctx->hash_bar_level_to_keep;
				ctx->hash_bar_free_idx --)
		{
			/* free hash bar idx */
			CLEAN_CTX(free_ctx);
			req->free_ctx.start = req->hash_bar_starts[ctx->hash_bar_free_idx];
			req->free_ctx.length = sizeof(butter_hash_bar_blk);

			PT_SET;
			r = butter_free_space(req);
			check_sub_pt_result
		}
	}
	else	{
		/* no hash bar, remove from info blk */
		req->io_request_location = (size_t)&(((butter_info_blk *)NULL)->tree_root);
		yield_to_io_and_check_result(BDB_IO_SEEK);

		butter_u64_zero = 0;
		req->io_request_size = INFO_BLK_TREE_ROOT_SIZE;
		req->io_request_buffer = &butter_u64_zero;
		yield_to_io_and_check_result(BDB_IO_WRITE);

		req->db->info_blk.tree_root = 0;
	}

	/* free data blk */
	CLEAN_CTX(free_ctx);
	req->free_ctx.start = req->d_start;
	req->free_ctx.length = req->d_blk_length;

	PT_SET;
	r = butter_free_space(req);
	check_sub_pt_result

	PT_END

	return BDB_OK;
}

int
butter_del(void * db, void * key, uint32_t key_len)	{
	butter_req_t * req;
	int r;

	if (!db)	{
		printf("ERROR: parameter error, NULL db pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (BUTTER_DB_INFO_MAGIC != ((butter_t *)db)->magic)	{
		printf("ERROR: invalid magic number in db descriptor\n");
		return BDB_PARAM_ERROR;
	}

	if (!key)	{
		printf("ERROR: parameter error, NULL key pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (key_len <= 0)	{
		printf("ERROR: parameter error, invalid key length\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->key = key;
	req->key_len = key_len;

	for(;;)	{
		r = butter_pt_del(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	del_butter_request(req);

	return r;
}

/* eof */
