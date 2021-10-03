/* vi: set sw=4 ts=4: */

#ifndef __BUTTER_DB_IN_MEMORY_DATA_TYPES_HEADER_INCLUDED__
#define __BUTTER_DB_IN_MEMORY_DATA_TYPES_HEADER_INCLUDED__

typedef struct _butter_spare_t	{
	RB_ENTRY(_butter_spare_t) rb;
	uint64_t start;
	uint64_t length;
} butter_spare_t;

RB_HEAD(BDB_SPARE, _butter_spare_t);

#define BUTTER_DB_INFO_MAGIC	0x0AFFACDE

typedef struct _butter_t	{
	int magic;
	int fd;
	int max_in_mm_hash_bars;
	struct BDB_SPARE spare_rb;
	butter_info_blk info_blk;
} butter_t;

#endif
