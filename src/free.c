/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdlib.h>	/* malloc */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */

#include "public.h"
#include "include/internal.h"

int
butter_free_space(butter_req_t * req)	{
	butter_free_ctx_t * ctx = &req->free_ctx;
	int r;

	PT_BEGIN

	yield_to_io_and_check_result(BDB_IO_GET_SIZE);
	if (req->io_request_size <= 0)	{
		printf("ERROR: invalid db file size %zu\n", req->io_request_size);
		return BDB_DATA_ERROR;
	}

	ctx->db_size = req->io_request_size;

	/* programming */
	r = butter_add_spare_prepare(req);
	if (r)	{
		ctx->pt_return_value = r;
		PT_SET;
		return ctx->pt_return_value;
	}

	/* execution */
	for (ctx->ip = 0; ctx->ip < SFOP_MAX_CNT; ctx->ip ++)	{
		if (FBOP_END == ctx->ops[ctx->ip].inst)	{
			goto __end;
		}
		else if ((FBOP_CREATE_NEW_SPARE == ctx->ops[ctx->ip].inst)
				|| (FBOP_UPDATE_EXIST_SPARE == ctx->ops[ctx->ip].inst))
		{
			ctx->new_spare_length = ctx->ops[ctx->ip].d[1]->start + ctx->ops[ctx->ip].d[1]->length
					- ctx->ops[ctx->ip].d[0]->start;
			ctx->blk.magic = MAGIC_BLK_BDB_SPARE;
			ctx->blk.pad_0 = 0;
			ctx->blk.length = ctx->new_spare_length;	/* length include the magic */
			if (ctx->ops[ctx->ip].d[2])	{
				ctx->blk.next = ctx->ops[ctx->ip].d[2]->start;
			}
			else	{
				ctx->blk.next = 0;
			}
			req->io_request_location = ctx->ops[ctx->ip].d[0]->start;
			yield_to_io_and_check_result(BDB_IO_SEEK);
			req->io_request_size = sizeof(ctx->blk);
			req->io_request_buffer = &ctx->blk;
			yield_to_io_and_check_result(BDB_IO_WRITE);
			if (FBOP_UPDATE_EXIST_SPARE == ctx->ops[ctx->ip].inst)	{
				butter_update_spare_length(req, ctx->ops[ctx->ip].d[0], ctx->new_spare_length);
			}
		}
		else if (FBOP_UPDATE_SPARE_NEXT == ctx->ops[ctx->ip].inst)	{
			if (ctx->ops[ctx->ip].d[1])	{
				ctx->blk.next = ctx->ops[ctx->ip].d[1]->start;
			}
			else	{
				ctx->blk.next = 0;
			}
			req->io_request_location = ctx->ops[ctx->ip].d[0]->start + \
					((uint64_t)&(((butter_spare_blk*)0)->next));
			yield_to_io_and_check_result(BDB_IO_SEEK);
			req->io_request_size = sizeof(ctx->blk.next);
			req->io_request_buffer = &ctx->blk.next;
			yield_to_io_and_check_result(BDB_IO_WRITE);
		}
		else if (FBOP_UPDATE_INFO_BLK == ctx->ops[ctx->ip].inst)	{
			if (ctx->ops[ctx->ip].d[0])	{
				ctx->blk.next = ctx->ops[ctx->ip].d[0]->start;
			}
			else	{
				ctx->blk.next = 0;
			}
			req->io_request_location = ((uint64_t)&(((butter_info_blk*)0)->spare_chain));
			yield_to_io_and_check_result(BDB_IO_SEEK);
			req->io_request_size = sizeof(ctx->blk.next);
			req->io_request_buffer = &ctx->blk.next;
			yield_to_io_and_check_result(BDB_IO_WRITE);
		}
		else if (FBOP_DB_TRUNCATE == ctx->ops[ctx->ip].inst)	{
			req->io_request_size = ctx->ops[ctx->ip].d[0]->start;
			yield_to_io_and_check_result(BDB_IO_TRUNCATE);
		}
		else if (FBOP_REMOVE_SPARE_RB == ctx->ops[ctx->ip].inst)	{
			butter_remove_spare(req, ctx->ops[ctx->ip].d[0]);
			butter_remove_spare(req, ctx->ops[ctx->ip].d[1]);
			butter_remove_spare(req, ctx->ops[ctx->ip].d[2]);
		}
		else	{
			printf("ERROR: invalid spare free instruction\n");
			break;
		}
	}

	printf("ERROR: FBOP_END instruction not found\n");

	PT_SET;

	return BDB_INTERNAL_ERROR;

__end:

	PT_SET;

	return BDB_OK;

	PT_END

	return BDB_OK;
}

/* eof */
