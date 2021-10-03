/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <strings.h>

#include "public.h"
#include "include/internal.h"

int
butter_pt_extend(butter_req_t * req)	{
	butter_extend_ctx_t * ctx = &req->extend_ctx;
	int i;
	int r;

	/* replacement not supposed to be done here */

	PT_BEGIN

	if (req->d_start)	{
		/* there are exist data blk, fall through to extend */
		goto __extend_hash_bar;
	}
	else if (req->hash_branch_depth)	{
		/* mount data blk to exist hash bar */
		ctx->hash_idx = req->hash_branch_depth - 1;
		ctx->hash_bar_jump_item_cnt = req->hash_bar_item_cnts[ctx->hash_idx];

		// printf("DBG: %s() %d: current jump item cnt is %d\n"
		// 		, __func__, __LINE__, ctx->hash_bar_jump_item_cnt);

		if ((ctx->hash_bar_jump_item_cnt + 1 > SLOTS_PRE_LEVEL)
				|| (ctx->hash_bar_jump_item_cnt + 1 < ctx->hash_bar_jump_item_cnt))
		{
			printf("ERROR: %s() %d: hash bar slot cnt too large\n"
					, __func__, __LINE__);
			return BDB_INTERNAL_ERROR;
		}

		ctx->hash_bar_jump_item_cnt ++;

		// printf("DBG: %s() %d: hash bar jump item cnt increase to %d\n"
		// 		, __func__, __LINE__, ctx->hash_bar_jump_item_cnt);
		req->io_request_location
				= HASH_BAR_CNT_LOCATION(req->hash_bar_starts[ctx->hash_idx]);
		yield_to_io_and_check_result(BDB_IO_SEEK);
		req->io_request_buffer = &ctx->hash_bar_jump_item_cnt;
		req->io_request_size = sizeof(ctx->hash_bar_jump_item_cnt);
		yield_to_io_and_check_result(BDB_IO_WRITE);

		// printf("DBG: %s() %d: write new bar start location 0x%lx into hash bar\n"
		// 		, __func__, __LINE__, req->set_ctx.new_data_blk_start);
		req->io_request_location = HASH_BAR_JUMP_ITEM_LOCATION2(ctx->hash_idx);
		yield_to_io_and_check_result(BDB_IO_SEEK);
		req->io_request_buffer = &req->set_ctx.new_data_blk_start;
		req->io_request_size = sizeof(req->set_ctx.new_data_blk_start);
		yield_to_io_and_check_result(BDB_IO_WRITE);

		return BDB_OK;
	}
	else	{
		/* mount data blk into info blk */
		req->io_request_location = INFO_BLK_TREE_ROOT;
		yield_to_io_and_check_result(BDB_IO_SEEK);

		req->io_request_size = sizeof(req->set_ctx.new_data_blk_start);
		req->io_request_buffer = &req->set_ctx.new_data_blk_start;
		yield_to_io_and_check_result(BDB_IO_WRITE);

		/* update in memory info */
		req->db->info_blk.tree_root = req->set_ctx.new_data_blk_start;

		return BDB_OK;
	}

	printf("ERROR: %s() %d: not supposed to be here\n", __func__, __LINE__);

	return BDB_INTERNAL_ERROR;

__extend_hash_bar:

	#if 0
	printf("DBG: %s() %d: tree extend\n", __func__, __LINE__);
	printf("DBG: hash value of new item:\n");
	DUMP_HEX(req->hash, HASH_LENGTH);
	printf("DBG: hash value of exist item:\n");
	DUMP_HEX(req->set_ctx.exist_key_hash, HASH_LENGTH);
	#endif

	/* extend */

	for (i = 0; i < HASH_LENGTH; i ++)	{
		if (req->hash[i] == req->set_ctx.exist_key_hash[i])	{
			continue;
		}
		else	{
			break;
		}
	}

	ctx->first_different_hash_value_idx = i;

	if (req->hash_branch_depth > ctx->first_different_hash_value_idx + 1)	{
		printf("ERROR: %s() %d: the existing hash bars are deeper than expected\n"
				, __func__, __LINE__);
		PT_SET;
		return BDB_INTERNAL_ERROR;
	}

	if (HASH_LENGTH == ctx->first_different_hash_value_idx)	{
		ctx->deepest_hash_bar_idx = HASH_LENGTH - 1;
	}
	else	{
		ctx->deepest_hash_bar_idx = ctx->first_different_hash_value_idx;
	}

	ctx->last_create_item_start = req->set_ctx.new_data_blk_start;

	for (ctx->hash_idx = ctx->deepest_hash_bar_idx; ctx->hash_idx >= 0; ctx->hash_idx --)
	{

		if (req->hash_bar_starts[ctx->hash_idx])	{

			req->io_request_location
					= HASH_BAR_CNT_LOCATION(req->hash_bar_starts[ctx->hash_idx]);
			yield_to_io_and_check_result(BDB_IO_SEEK);

			req->io_request_buffer = &ctx->hash_bar_jump_item_cnt;
			req->io_request_size = sizeof(ctx->hash_bar_jump_item_cnt);
			yield_to_io_and_check_result(BDB_IO_READ);

			req->io_request_location = HASH_BAR_JUMP_ITEM_LOCATION2(ctx->hash_idx);
			yield_to_io_and_check_result(BDB_IO_SEEK);

			req->io_request_buffer = &ctx->last_create_item_start;
			req->io_request_size = sizeof(ctx->last_create_item_start);
			yield_to_io_and_check_result(BDB_IO_WRITE);

			return BDB_OK;
		}

		/* filling the new hash bar in memory */
		bzero(&ctx->hash_bar, sizeof(ctx->hash_bar));
		ctx->hash_bar.magic = MAGIC_BLK_BDB_HASH_BAR;
		if (ctx->hash_idx == ctx->deepest_hash_bar_idx)	{
			if (1 == req->d_key_hash_conflict)	{
				/* key conflict !! */
				ctx->hash_bar.cnt = 1;
				ctx->hash_bar.jump[req->set_ctx.exist_key_hash[ctx->hash_idx]]
						= req->set_ctx.new_data_blk_start;
			}
			else	{
				/* mount the two data blks */
				ctx->hash_bar.cnt = 2;
				ctx->hash_bar.jump[req->set_ctx.exist_key_hash[ctx->hash_idx]]
						= req->d_start;
				ctx->hash_bar.jump[req->hash[ctx->hash_idx]]
						= req->set_ctx.new_data_blk_start;
			}
		}
		else	{
			/* mount the deeper level hash bar */
			if (!ctx->last_create_item_start)	{
				printf("ERROR: %s() %d: deeper level hash bar start location not present\n"
						, __func__, __LINE__);
				PT_SET;
				return BDB_INTERNAL_ERROR;
			}
			ctx->hash_bar.cnt = 1;
			ctx->hash_bar.jump[req->hash[ctx->hash_idx]] = ctx->last_create_item_start;
		}
		
		/* alloc disk space for new hash bar */
		CLEAN_CTX(alloc_ctx);
		req->alloc_ctx.request_length = sizeof(butter_hash_bar_blk);
		PT_SET;
		r = butter_alloc_space(req);
		check_sub_pt_result

		/* seek and write down the new hash bar */
		ctx->last_create_item_start = req->io_request_location = req->alloc_ctx.start;
		yield_to_io_and_check_result(BDB_IO_SEEK);

		req->io_request_size = sizeof(ctx->hash_bar);
		req->io_request_buffer = &ctx->hash_bar;
		yield_to_io_and_check_result(BDB_IO_WRITE);
	}

	if (!ctx->last_create_item_start)	{
		printf("ERROR: %s() %d: ctx->last_create_item_start is zero\n"
				, __func__, __LINE__);
		return BDB_INTERNAL_ERROR;
	}

	/* mount to info blk */
	req->io_request_location = INFO_BLK_TREE_ROOT;
	yield_to_io_and_check_result(BDB_IO_SEEK);

	req->io_request_size = sizeof(ctx->last_create_item_start);
	req->io_request_buffer = &ctx->last_create_item_start;
	yield_to_io_and_check_result(BDB_IO_WRITE);

	/* update in memory info */
	req->db->info_blk.tree_root = ctx->last_create_item_start;

	return BDB_OK;

	PT_END

	return BDB_OK;
}

/* eof */
