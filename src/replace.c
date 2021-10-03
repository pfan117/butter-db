/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <strings.h>

#include "public.h"
#include "include/internal.h"

int
butter_pt_replace(butter_req_t * req)	{
	butter_replace_ctx_t * ctx = &req->replace_ctx;
	int r;

	PT_BEGIN

	if (req->d_key_matched)	{
		;
	}
	else	{
		printf("ERROR: %s() %d: identical key not exist\n", __func__, __LINE__);
		PT_SET;
		return BDB_INTERNAL_ERROR;
	}

	PT_SET;

	if (req->d_prev_start)	{
		/* mount data blk to prev data list */
		req->io_request_location = EX_DATA_NEXT_LOCATION(req->d_prev_start);
		yield_to_io_and_check_result(BDB_IO_SEEK);
		req->io_request_buffer = &req->set_ctx.new_data_blk_start;
		req->io_request_size = sizeof(req->set_ctx.new_data_blk_start);
		yield_to_io_and_check_result(BDB_IO_WRITE);
		return BDB_OK;
	}
	else if (req->hash_branch_depth)	{
		/* mount data blk to exist hash bar */
		ctx->hash_idx = req->hash_branch_depth - 1;
		req->io_request_location = HASH_BAR_JUMP_ITEM_LOCATION2(ctx->hash_idx);
		yield_to_io_and_check_result(BDB_IO_SEEK);
		req->io_request_buffer = &req->set_ctx.new_data_blk_start;
		req->io_request_size = sizeof(req->set_ctx.new_data_blk_start);
		yield_to_io_and_check_result(BDB_IO_WRITE);
	}
	else	{
		/* write to info blk */
		req->io_request_location = INFO_BLK_TREE_ROOT;
		yield_to_io_and_check_result(BDB_IO_SEEK);

		req->io_request_size = sizeof(req->set_ctx.new_data_blk_start);
		req->io_request_buffer = &req->set_ctx.new_data_blk_start;
		yield_to_io_and_check_result(BDB_IO_WRITE);

		/* update in memory info */
		req->db->info_blk.tree_root = req->set_ctx.new_data_blk_start;
	}

	CLEAN_CTX(free_ctx);
	req->free_ctx.start = req->d_start;
	req->free_ctx.length = req->d_blk_length;

	PT_SET;
	r = butter_free_space(req);
	check_sub_pt_result

	PT_END

	return BDB_OK;
}

/* eof */
