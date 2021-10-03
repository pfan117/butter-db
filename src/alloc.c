/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdlib.h>	/* malloc */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */

#include "public.h"
#include "include/internal.h"

STATIC int
butter_pt_alloc_write_down(butter_req_t * req)	{
	butter_alloc_ctx_t * alloc_ctx = &req->alloc_ctx;
	butter_alloc_write_down_ctx_t * ctx = &req->alloc_write_down_ctx;

	PT_BEGIN

	/* try write down spare change */
	if (alloc_ctx->changed_spare)	{
		/* shorter the spare, no spare link list shape change */
		req->io_request_location
				= alloc_ctx->changed_spare->start
						+ (uint64_t)&((butter_spare_blk *)0)->length;
		yield_to_io_and_check_result(BDB_IO_SEEK);

		req->io_request_buffer = &alloc_ctx->changed_spare->length;
		req->io_request_size = sizeof(alloc_ctx->changed_spare->length);
		yield_to_io_and_check_result(BDB_IO_WRITE);

		PT_SET;

		return BDB_OK;
	}

	/* remove the spare from the spare link list */

	if (alloc_ctx->prev_spare)	{
		req->io_request_location
				= alloc_ctx->prev_spare->start + (uint64_t)&((butter_spare_blk *)0)->next;
		yield_to_io_and_check_result(BDB_IO_SEEK);
	}
	else	{
		req->io_request_location = (uint64_t)&((butter_info_blk *)0)->spare_chain;
		yield_to_io_and_check_result(BDB_IO_SEEK);
	}

	if (alloc_ctx->next_spare)	{
		req->io_request_buffer = &alloc_ctx->next_spare->start;
	}
	else	{
		/* use the zero value to mark end the spare link list */
		req->io_request_buffer = &alloc_ctx->next_spare;
	}

	req->io_request_size = sizeof(alloc_ctx->next_spare->start);
	yield_to_io_and_check_result(BDB_IO_WRITE);

	PT_END

	return BDB_OK;
}

/* return values: */
/* BDB_OK:          malloc success */
/* BDB_NO_SPACE:    no disk space */
/* BDB_PT_IO_REQ:   means a new i/o request has been filled */
/* BDB_PARAM_ERROR: internal error */
/* BDB_IO_ERROR:    i/o error */
int
butter_alloc_space(butter_req_t * req)	{

	butter_alloc_ctx_t * ctx = &req->alloc_ctx;
	int r;

	PT_BEGIN

	/* ceil up to DISK_SIZE_UNIT */
	#if 0
/*
i, request_size: 000, 000; i, request_size: 001, 032;
i, request_size: 002, 032; i, request_size: 003, 032;
i, request_size: 004, 032; i, request_size: 005, 032;
i, request_size: 006, 032; i, request_size: 007, 032;
i, request_size: 008, 032; i, request_size: 009, 032;
i, request_size: 010, 032; i, request_size: 011, 032;
i, request_size: 012, 032; i, request_size: 013, 032;
i, request_size: 014, 032; i, request_size: 015, 032;

i, request_size: 024, 032; i, request_size: 025, 032;
i, request_size: 026, 032; i, request_size: 027, 032;
i, request_size: 028, 032; i, request_size: 029, 032;
i, request_size: 030, 032; i, request_size: 031, 032;
i, request_size: 032, 032; i, request_size: 033, 064;
i, request_size: 034, 064; i, request_size: 035, 064;
i, request_size: 036, 064; i, request_size: 037, 064;
i, request_size: 038, 064; i, request_size: 039, 064;
*/
	ctx->ceil_request_length
			= (ctx->request_length + DISK_SIZE_UNIT) & (~DISK_SIZE_UNIT_CEIL_MASK);
	#else
	if (ctx->request_length < DISK_SIZE_UNIT)	{
		ctx->ceil_request_length = DISK_SIZE_UNIT;
	}
	else	{
		ctx->ceil_request_length = (ctx->request_length + 3) & (~3);
	}
	#endif

	#if 0
	if (ctx->ceil_request_length != ctx->request_length)	{
		printf("DBG: %s(), %d: ctx->ceil_request_length = %d, ctx->request_length = %d\n"
				, __func__, __LINE__
				, ctx->ceil_request_length
				, ctx->request_length
				);
	}
	#endif

	PT_SET

	r = butter_get_spare(req, ctx->ceil_request_length);
	if (BDB_OK == r)	{
		/* printf("DBG: try write down spare change %d\n", __LINE__); */
		CLEAN_CTX(alloc_write_down_ctx);
		PT_SET;
		r = butter_pt_alloc_write_down(req);
		check_sub_pt_result
		PT_SET;
		return BDB_OK;
	}
	else if (BDB_NOT_FOUND == r)	{
		;
	}
	else	{
		ctx->pt_return_value = r;
		return ctx->pt_return_value;
	}

	PT_SET

	yield_to_io_and_check_result(BDB_IO_GET_SIZE);

	if (req->io_request_size <= 0)	{
		printf("ERROR: invalid db file size %zu\n", req->io_request_size);
		return BDB_DATA_ERROR;
	}

	if (ctx->ceil_request_length + req->io_request_size <= req->io_request_size)	{
		printf("ERROR: exceed file size limit, size value loopback overflow\n");
		return BDB_NO_SPACE;
	}

	/* extend file */
	ctx->start = req->io_request_size;
	ctx->length = ctx->ceil_request_length;
	ctx->flags |= ALLOC_F_EXT_END;

	PT_END

	return BDB_OK;

}

/* eof */
