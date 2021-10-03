/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <sys/time.h> /* gettimeofday */

#include "public.h"
#include "include/internal.h"

STATIC butter_t *
butter_db_new(void)	{
	butter_t * p;

	p = Malloc(sizeof(butter_t));
	if (!p)	{
		return NULL;
	}

	bzero(p, sizeof(butter_t));

	p->magic = BUTTER_DB_INFO_MAGIC;
	p->fd = -1;

	return p;
}

STATIC int
butter_db_check_di(butter_t * p)	{
	if (MAGIC_BLK_BDB_INFO != p->info_blk.magic)	{
		return BDB_DATA_ERROR;
	}
	return BDB_OK;
}

STATIC int
butter_fill_new_di(butter_t * p)	{
	struct timeval tv;
	int r;

	r = gettimeofday(&tv, NULL);
	if (r)	{
		printf("ERROR: %s, %s, %d: gettimeofday() error\n", MODULE_NAME, __func__, __LINE__);
		bzero(&tv, sizeof(tv));
	}

	bzero(&p->info_blk, sizeof(p->info_blk));
	p->info_blk.magic = MAGIC_BLK_BDB_INFO;
	p->info_blk.create_sec = tv.tv_sec;
	p->info_blk.create_usec = tv.tv_usec;

	r = write(p->fd, &p->info_blk, sizeof(p->info_blk));
	if (sizeof(p->info_blk) == r)	{
		// printf("DBG: write new db info block (%ld bytes) info success\n", sizeof(p->info_blk));
		return BDB_OK;
	}
	else	{
		printf("ERROR: failed to write new db info block\n");
		return BDB_IO_ERROR;
	}
}

STATIC int
butter_db_load(butter_t * p)	{

	int r;

	r = read(p->fd, &p->info_blk, sizeof(p->info_blk));
	if (r == sizeof(p->info_blk))	{
		r = butter_db_check_di(p);
		if (r)	{
			return r;
		}
		else	{
			r = butter_load_spare_chain(p);
			if (r)	{
				return r;
			}
		}
	}
	else if (0 == r)	{
		r = butter_fill_new_di(p);
	}
	else	{
		r = BDB_IO_ERROR;
	}

	return r;
}

STATIC int
butter_db_open(butter_t * p, char * filename)	{

	int fd;
	int r;

	fd = open(filename, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH));
	if (-1 == fd)	{
		return BDB_FILE_OPEN_ERROR;
	}

	p->fd = fd;

	r = butter_db_load(p);
	if (r)	{
		close(fd);
		p->fd = -1;
		return r;
	}

	return BDB_OK;
}

int
butter_open(void ** vf, char * filename)	{
	butter_t * p;
	int r;

	if (!vf || !filename)	{
		return BDB_PARAM_ERROR;
	}

	p = butter_db_new();
	if (!p)	{
		*vf = NULL;
		return BDB_NO_MEMORY;
	}

	r = butter_db_open(p, filename);
	if (r)	{
		*vf = NULL;
		Free(p);
		return r;
	}

	*vf = p;

	return BDB_OK;
}

int
butter_ptr_check(butter_t * p)	{

	if (!p)	{
		return BDB_PARAM_ERROR;
	}

	if (BUTTER_DB_INFO_MAGIC != p->magic)	{
		return BDB_PARAM_ERROR;
	}

	if (p->fd < -1)	{
		return BDB_PARAM_ERROR;
	}

	return BDB_OK;
}

int
butter_flush(void * vf)	{

	butter_t * p = vf;
	int r;

	r = butter_ptr_check(p);
	if (r)	{
		return r;
	}

	if (-1 == p->fd)	{
		return BDB_NOT_OPEN;
	}

	r = fsync(p->fd);
	if (r)	{
		return BDB_IO_ERROR;
	}
	else	{
		return BDB_OK;
	}
}

int
butter_close(void * vf)	{
	butter_t * p = vf;
	int r;

	r = butter_ptr_check(p);
	if (r)	{
		return r;
	}

	if (-1 != p->fd)	{
		r = fsync(p->fd);
		if (r)	{
			return BDB_IO_ERROR;
		}
		close(p->fd);
		p->fd = -1;
	}

	butter_free_spare_chain(p);

	Free(p);

	return BDB_OK;
}

void
butter_free(void * p)	{
	if (p)	{
		Free(p);
	}
}

/* eof */
