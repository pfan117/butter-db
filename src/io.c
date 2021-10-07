#include <stdio.h>		/* printf() */
#include <stdlib.h>
#include <errno.h>
#include <string.h>		/* strerror */
#include <strings.h>	/* bzero() */
#include <openssl/sha.h>

#include <sys/types.h>	/* lseek() */
#include <unistd.h>		/* lseek() */

#include "public.h"
#include "include/internal.h"

#if 1
	#define HERE	printf("DBG: %s, %d: %s()\n", __FILE__, __LINE__, __func__)
#else
	#define HERE
#endif

void
butter_io_chassis(butter_req_t * req)	{
	uint64_t r;

	switch(req->io_type)	{
	case BDB_IO_READ:
		r = (uint64_t)read(req->db->fd, req->io_request_buffer, req->io_request_size);
		if (req->io_request_size == r)	{
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_OK;
		}
		else	{
			printf("ERROR: IO chassis %s(), %d: read() failed, fd = %d, r = %lu\n"
					, __func__, __LINE__, req->db->fd, r);
			printf("ERROR: IO chassis: read() failed, errno = %d (%s)\n"
					, errno, strerror(errno));
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_IO_ERROR;
		}
		break;
	case BDB_IO_WRITE:
		if (req->io_request_size ==
				(uint64_t)write(req->db->fd, req->io_request_buffer, req->io_request_size))
		{
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_OK;
		}
		else	{
			printf("ERROR: IO chassis %s(), %d: write() failed\n", __func__, __LINE__);
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_IO_ERROR;
		}
		break;
	case BDB_IO_SEEK:
		if (req->io_request_location ==
				(uint64_t)lseek(req->db->fd, req->io_request_location, SEEK_SET))	{
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_OK;
		}
		else	{
			printf("ERROR: IO chassis %s(), %d: lseek() failed, location = %zd\n"
					, __func__, __LINE__
					, req->io_request_location
					);
			req->io_type = BDB_IO_DONE;
			req->io_handler_return_value = BDB_IO_ERROR;
		}
		break;
	case BDB_IO_GET_SIZE:
		{
			off_t off;
			off = lseek(req->db->fd, 0, SEEK_END);
			req->io_request_size = (uint64_t)off;
			req->io_type = BDB_IO_DONE;
			if (-1 == off)	{
				printf("ERROR: IO chassis %s(), %d: lseek() failed to get file size\n"
						, __func__, __LINE__);
				req->io_handler_return_value = BDB_IO_ERROR;
			}
			else	{
				req->io_handler_return_value = BDB_OK;
			}
		}
		break;
	case BDB_IO_TRUNCATE:
		{
			int r;
			r = ftruncate(req->db->fd, req->io_request_size);
			req->io_type = BDB_IO_DONE;
			if (r)	{
				req->io_handler_return_value = BDB_IO_ERROR;
			}
			else	{
				req->io_handler_return_value = BDB_OK;
			}
		}
		break;
	default:
		printf("ERROR: IO chassis: invalid io request type\n");
		break;
	}

	return;
}

/* eof */
