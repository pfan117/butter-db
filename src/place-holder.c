/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdlib.h>	/* malloc */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */

#include "public.h"
#include "include/internal.h"

#define WRITE_MAX	64

STATIC int
butter_pt_create_place_holder(butter_req_t * req)	{
	butter_create_place_holder_ctx_t * ctx = &req->create_place_holder_ctx;
	char butter_place_holder_write_buf[WRITE_MAX] = {0};
	int r;

	PT_BEGIN

	/* req->hash_bar_starts[0] as request length */
	/* req->d_start as result start location */

	CLEAN_CTX(alloc_ctx);
	req->alloc_ctx.request_length = req->hash_bar_starts[0];
	PT_SET;
	r = butter_alloc_space(req);
	check_sub_pt_result

	//printf("DBG: %s() %d: write new place holder blk to disk\n", __func__, __LINE__);
	req->d_start = req->io_request_location = req->alloc_ctx.start;
	//printf("DBG: %s() %d: seek to %ld (0x%lx)\n", __func__, __LINE__, req->io_request_location, req->io_request_location);
	yield_to_io_and_check_result(BDB_IO_SEEK);

	ctx->blk.magic = MAGIC_BLK_BDB_HOLDER;
	ctx->blk.pad_0 = 0;
	ctx->blk.length = req->hash_bar_starts[0];

	//printf("DBG: %s() %d: write new place holder blk header\n", __func__, __LINE__);
	req->io_request_buffer = &ctx->blk;
	req->io_request_size = sizeof(ctx->blk);
	yield_to_io_and_check_result(BDB_IO_WRITE);

	for (ctx->offset = sizeof(ctx->blk); req->hash_bar_starts[0] - ctx->offset > WRITE_MAX; ctx->offset += WRITE_MAX)	{
		PT_SET;
		req->io_request_buffer = butter_place_holder_write_buf;
		req->io_request_size = WRITE_MAX;
		//printf("DBG: %s() %d: filling new place holder blk, size %ld\n", __func__, __LINE__, req->io_request_size);
		yield_to_io_and_check_result(BDB_IO_WRITE);
	}

	if (req->hash_bar_starts[0] - ctx->offset > 0)	{
		PT_SET;
		req->io_request_buffer = butter_place_holder_write_buf;
		req->io_request_size = req->hash_bar_starts[0] - ctx->offset;
		//printf("DBG: %s() %d: filling new place holder blk tail, size %ld\n", __func__, __LINE__, req->io_request_size);
		yield_to_io_and_check_result(BDB_IO_WRITE);
	}

	PT_END

	return BDB_OK;

}

int
butter_create_place_holder(void * db, uint64_t size, uint64_t * start)	{
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

	if (size <= 0)	{
		printf("ERROR: parameter error, invalid length\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->hash_bar_starts[0] = size;

	for(;;)	{
		r = butter_pt_create_place_holder(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	*start = req->d_start;

	del_butter_request(req);

	return BDB_OK;
}

STATIC int
butter_pt_remove_place_holder(butter_req_t * req)	{
	butter_remove_place_holder_ctx_t * ctx = &req->remove_place_holder_ctx;
	int r;

	PT_BEGIN

	CLEAN_CTX(free_ctx);

	req->free_ctx.start = req->hash_bar_starts[0];
	req->free_ctx.length = req->hash_bar_starts[1];

	PT_SET;
	r = butter_free_space(req);
	check_sub_pt_result

	PT_END

	return BDB_OK;

}

int
butter_remove_place_holder(void * db, uint64_t start, uint64_t size)
{
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

	if (start <= 0)	{
		printf("ERROR: parameter error, invalid start value\n");
		return BDB_PARAM_ERROR;
	}

	if (size <= 0)	{
		printf("ERROR: parameter error, invalid length\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->hash_bar_starts[0] = start;
	req->hash_bar_starts[1] = size;

	for(;;)	{
		r = butter_pt_remove_place_holder(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	del_butter_request(req);

	return BDB_OK;
}

/* eof */
