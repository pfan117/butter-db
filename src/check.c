/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */

#include "public.h"
#include "include/internal.h"

uint64_t db_file_size;
size_t db_file_offset = 0;
int butter_file_check_flags = 0;

#define DUMP_ITEM	((butter_file_check_flags & BUTTER_DB_CHECK_DUMP))
#define DUMP_PERFIX	((butter_file_check_flags & BUTTER_DB_DUMP_WITH_PREFIX))

#define DUMP_REST_MAX	256

STATIC void
butter_dump_rest(int fd)	{
	char buffer[DUMP_REST_MAX];
	int length;
	int r;

	if (db_file_offset >= db_file_size)	{
		printf("INFO: %s() %d: no data left\n", __func__, __LINE__);
		return;
	}

	length = db_file_size - db_file_offset;

	if (length > DUMP_REST_MAX)	{
		length = DUMP_REST_MAX;
	}

	printf("INFO: %s() %d: dump the rest %d bytes\n", __func__, __LINE__, length);

	r = read(fd, buffer, length);
	if (r != length)	{
		printf("ERROR: %s() %d: failed to read data\n", __func__, __LINE__);
		return;
	}

	butter_hex_dump(buffer, length);

	return;
}

STATIC int
butter_check_size(int fd)	{

	off_t r;

	r = lseek(fd, 0, SEEK_END);
	if (r < 0)	{
		printf("ERROR: failed to get file size\n");
		return BDB_DATA_ERROR;
	}
	else if (0 == r)	{
		db_file_size = r;
	}
	else if (r >= (off_t)sizeof(butter_info_blk))	{
		db_file_size = r;
	}
	else	{
		printf("ERROR: file size too small to be valid\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		if (DUMP_PERFIX)	{
			printf(" ---- file size is %zd bytes\n", db_file_size);
		}
		else	{
			printf("file size is %zd bytes\n", db_file_size);
		}
	}

	return BDB_OK;
}

STATIC int
butter_check_info_blk(int fd)	{
	butter_info_blk info;
	off_t l;
	int r;

	l = lseek(fd, 0, SEEK_SET);
	if (0 != l)	{
		printf("ERROR: failed to lseek() to zero\n");
		return BDB_DATA_ERROR;
	}

	r = read(fd, &info, sizeof(info));
	if (r != sizeof(info))	{
		printf("ERROR: failed to read info block\n");
		return BDB_DATA_ERROR;
	}

	if (MAGIC_BLK_BDB_INFO == info.magic)	{
		db_file_offset = sizeof(info);
	}
	else	{
		printf("ERROR: info block contains invalid magic value\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		printf("%sDB create time: 0x%08x 0x%08x\n", prefix, info.create_sec, info.create_usec);
		printf("%s0x%08lx - info: spare_chain: 0x%08lx, tree_root: 0x%08lx\n"
				, prefix
				, (size_t)0
				, info.spare_chain
				, info.tree_root
				);
	}

	return BDB_OK;
}

static int spare_cnt;
static int hash_bar_cnt;
static int data_blk_cnt;
static int data_ex_blk_cnt;
static int place_holder_blk_cnt;

STATIC void
butter_print_counters(void)	{
	if (DUMP_ITEM)	{
		printf("           spare_cnt = %d\n", spare_cnt);
		printf("        hash_bar_cnt = %d\n", hash_bar_cnt);
		printf("        data_blk_cnt = %d\n", data_blk_cnt);
		printf("     data_ex_blk_cnt = %d\n", data_ex_blk_cnt);
		printf("place_holder_blk_cnt = %d\n", place_holder_blk_cnt);
	}
	return;
}

STATIC int
butter_check_spare_blk(int fd)	{
	butter_spare_blk blk;
	uint64_t seeklength;
	int r;

	r = read(fd, ((char *)&blk) + sizeof(blk.magic), sizeof(blk) - sizeof(blk.magic));
	if (r == (sizeof(blk) - sizeof(blk.magic)))	{
		;
	}
	else	{
		printf("ERROR: failed to read a spare blk header\n");
		return BDB_DATA_ERROR;
	}

	seeklength = db_file_offset + blk.length;

	if (seeklength > db_file_size)	{
		printf("ERROR: file ended with a incomplete spare black, db_file_size is %lu, spare blk end at %lu\n"
				, db_file_size, seeklength
				);
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		printf("%s0x%08lx - spar: length: %lu\n", prefix, db_file_offset, blk.length);
	}

	if (seeklength == lseek(fd, seeklength, SEEK_SET))	{
		db_file_offset = seeklength;
	}
	else	{
		printf("ERROR: lseek() error, line %d\n", __LINE__);
		return BDB_DATA_ERROR;
	}

	return BDB_OK;
}

STATIC int
butter_check_hash_bar_blk(int fd)	{
	butter_hash_bar_blk blk;
	off_t seekt;
	int cnt;
	int i;
	int r;

	r = read(fd, &blk.cnt, sizeof(blk) - sizeof(blk.magic));
	if (r == (sizeof(blk) - sizeof(blk.magic)))	{
		;
	}
	else	{
		printf("ERROR: failed to read a map blk\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		printf("%s0x%08lx - hsbr: jump_item_cnt: %d\n"
				, prefix
				, (size_t)db_file_offset
				, blk.cnt
				);
	}

	cnt = 0;

	for (i = 0; i < SLOTS_PRE_LEVEL; i ++)	{
		if (blk.jump[i])	{
			cnt ++;
		}
	}

	if (cnt != blk.cnt)	{
		printf("ERROR: %s() %d: hash bar jump item cnt value is %d, but there are %d items:\n"
				, __func__, __LINE__, blk.cnt, cnt
				);
		for (i = 0; i < SLOTS_PRE_LEVEL; i ++)	{
			if (blk.jump[i])	{
				printf(" [0x%08lx]", blk.jump[i]);
			}
		}
		printf("\n");
		// return BDB_DATA_ERROR;
	}

	seekt = db_file_offset + sizeof(blk);

	if (seekt > db_file_size)	{
		printf("ERROR: %s() %d: not enough data for a hash bar\n", __func__, __LINE__);
		return BDB_DATA_ERROR;
	}

	if (seekt == lseek(fd, seekt, SEEK_SET))	{
		db_file_offset += sizeof(blk);;
	}
	else	{
		printf("ERROR: %s()%d: lseek() error\n", __func__, __LINE__);
		return BDB_DATA_ERROR;
	}

	return BDB_OK;
}

STATIC int
butter_check_data_blk(int fd)	{
	butter_data_blk blk;
	uint64_t seekr;
	int r;

	/* read blk_length field */
	if (db_file_offset + sizeof(blk) >= db_file_size)	{
		printf("ERROR: data block over the end of file\n");
		return BDB_DATA_ERROR;
	}

	r = read(fd, &blk.blk_length, sizeof(blk) - sizeof(blk.magic));
	if (r == (sizeof(blk) - sizeof(blk.magic)))	{
		;
	}
	else	{
		printf("ERROR: failed to read a data blk header\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		printf("%s0x%08lx - data: blk_length: %u,"
				, prefix
				, (size_t)db_file_offset
				, blk.blk_length
				);
	}

	if (blk.key_length <= 0)	{
		printf(" invalid key_length %u", blk.key_length);
		return BDB_DATA_ERROR;
	}

	if (blk.value_length < 0)	{
		printf(" invalid value_length %u\n", blk.value_length);
		return BDB_DATA_ERROR;
	}

	if (blk.key_length + blk.value_length + sizeof(butter_data_blk) > blk.blk_length)	{
		printf("ERROR: %s() %d: blk content longer than allocated space size\n", __func__, __LINE__);
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		printf(" key_length: %u, value_length: %u\n"
				, blk.key_length
				, blk.value_length
				);
	}

	/* jump to the next block */
	seekr = lseek(fd, db_file_offset + blk.blk_length, SEEK_SET);
	if ((db_file_offset + blk.blk_length) == seekr)	{
		db_file_offset += blk.blk_length;
		return BDB_OK;
	}
	else	{
		printf("ERROR: %s(), %d: failed to lseek(%zd) over a data block, return value %zd\n"
				, __func__, __LINE__
				, db_file_offset + blk.blk_length
				, seekr
				);
		return BDB_DATA_ERROR;
	}
}

STATIC int
butter_check_data_ex_blk(int fd)	{
	butter_data_ex_blk blk;
	uint64_t seekr;
	int r;

	/* read blk_length field */
	if (db_file_offset + sizeof(blk) >= db_file_size)	{
		printf("ERROR: ex data block over the end of file\n");
		return BDB_DATA_ERROR;
	}

	r = read(fd, &blk.blk_length, sizeof(blk) - sizeof(blk.magic));
	if (r == (sizeof(blk) - sizeof(blk.magic)))	{
		;
	}
	else	{
		printf("ERROR: failed to read a data blk header\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		printf("%s0x%08lx - EXdt: blk_length: %u,"
				, prefix
				, (size_t)db_file_offset
				, blk.blk_length
				);
	}

	if (blk.key_length <= 0)	{
		printf(" invalid key_length %u", blk.key_length);
		return BDB_DATA_ERROR;
	}

	if (blk.value_length < 0)	{
		printf(" invalid value_length %u\n", blk.value_length);
		return BDB_DATA_ERROR;
	}

	if (blk.key_length + blk.value_length + sizeof(butter_data_blk) > blk.blk_length)	{
		printf("ERROR: %s() %d: blk content longer than allocated space size\n", __func__, __LINE__);
		return BDB_DATA_ERROR;
	}

	if (blk.next < 0)	{
		printf(" invalid next blk address %lu\n", blk.next);
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		printf(" key_length: %u, value_length: %u, next: 0x%08lx\n"
				, blk.key_length
				, blk.value_length
				, blk.next
				);
	}

	/* jump to the next block */
	seekr = lseek(fd, db_file_offset + blk.blk_length, SEEK_SET);
	if ((db_file_offset + blk.blk_length) == seekr)	{
		db_file_offset += blk.blk_length;
		return BDB_OK;
	}
	else	{
		printf("ERROR: %s(), %d: failed to lseek(%zd) over a data block, return value %zd\n"
				, __func__, __LINE__
				, db_file_offset + blk.blk_length
				, seekr
				);
		return BDB_DATA_ERROR;
	}
}

STATIC int
butter_check_place_holder(int fd)	{

	butter_place_holder_blk blk;
	uint64_t seekr;
	int r;

	/* read blk_length field */
	if (db_file_offset + sizeof(blk) >= db_file_size)	{
		printf("ERROR: data block over the end of file\n");
		return BDB_DATA_ERROR;
	}

	r = read(fd, &blk.pad_0, sizeof(blk) - sizeof(blk.magic));
	if (r == (sizeof(blk) - sizeof(blk.magic)))	{
		;
	}
	else	{
		printf("ERROR: failed to read a data blk header\n");
		return BDB_DATA_ERROR;
	}

	if (DUMP_ITEM)	{
		char * prefix;;

		if (DUMP_PERFIX)	{
			prefix = " ---- ";
		}
		else	{
			prefix = "";
		}

		if (blk.length <= 0)	{
			printf("%s0x%08lx - hold: length: %lu *invalid*\n"
					, prefix
					, (size_t)db_file_offset
					, blk.length
					);
			return BDB_DATA_ERROR;
		}
		else	{
			printf("%s0x%08lx - hold: length: %lu\n"
					, prefix
					, (size_t)db_file_offset
					, blk.length
					);
		}
	}


	/* jump to the next block */

	seekr = lseek(fd, db_file_offset + blk.length, SEEK_SET);
	if ((db_file_offset + blk.length) == seekr)	{
		db_file_offset += blk.length;
		return BDB_OK;
	}
	else	{
		printf("ERROR: %s(), %d: failed to lseek(%zd) over a place holder block, return value %zd\n"
				, __func__, __LINE__
				, db_file_offset + blk.length
				, seekr
				);
		return BDB_DATA_ERROR;
	}
}

STATIC int
butter_check_tlv(int fd)	{
	int magic;
	ssize_t r;

	spare_cnt = 0;
	hash_bar_cnt = 0;
	data_blk_cnt = 0;
	data_ex_blk_cnt = 0;
	place_holder_blk_cnt = 0;

	for (;;)	{

		r = read(fd, &magic, sizeof(magic));
		if (0 == r)	{
			break;
		}
		else if (r < 0)	{
			printf("ERROR: failed to read magic number\n");
			goto __exx;
		}
		else if (r < (ssize_t)sizeof(magic))	{
			printf("ERROR: still data in the end of the file, but not enough for a magic number\n");
			goto __exx;
		}
		else	{
			;
		}

		switch(magic)	{
		case MAGIC_BLK_BDB_INFO:
			printf("ERROR: info block appears in the middle of the file\n");
			goto __exx;
			break;
		case MAGIC_BLK_BDB_SPARE:
			r = butter_check_spare_blk(fd);
			if (r)	{
				goto __exx;
			}
			spare_cnt ++;
			break;
		case MAGIC_BLK_BDB_HASH_BAR:
			r = butter_check_hash_bar_blk(fd);
			if (r)	{
				goto __exx;
			}
			hash_bar_cnt ++;
			break;
		case MAGIC_BLK_BDB_DATA:
			r = butter_check_data_blk(fd);
			if (r)	{
				goto __exx;
			}
			data_blk_cnt ++;
			break;
		case MAGIC_BLK_BDB_DATA_EX:
			r = butter_check_data_ex_blk(fd);
			if (r)	{
				goto __exx;
			}
			data_ex_blk_cnt ++;
			break;
		case MAGIC_BLK_BDB_HOLDER:
			r = butter_check_place_holder(fd);
			if (r)	{
				goto __exx;
			}
			place_holder_blk_cnt ++;
			break;
		default:
			printf("ERROR: %s() %d: invalid magic number 0x%08x\n", __func__, __LINE__, magic);
			butter_dump_rest(fd);
			goto __exx;
		}
	}

	return BDB_OK;

__exx:

	return BDB_DATA_ERROR;
}

STATIC int
butter_check_tree(int fd)	{
	(void)fd;
	return BDB_OK;
}

int
butter_file_check(char * filename, int flags)	{

	int fd;
	int r;

	butter_file_check_flags = flags;

	fd = open(filename, O_RDONLY);
	if (-1 == fd)	{
		return BDB_FILE_OPEN_ERROR;
	}

	r = butter_check_size(fd);
	if (r)	{
		goto __exx;
	}

	r = butter_check_info_blk(fd);
	if (r)	{
		goto __exx;
	}

	r = butter_check_tlv(fd);
	if (r)	{
		goto __exx;
	}

	r = butter_check_tree(fd);
	if (r)	{
		goto __exx;
	}

	close(fd);

	butter_print_counters();

	return BDB_OK;

__exx:

	butter_print_counters();

	return r;
}

#define CHECK_COUNTER(__NAME__)	do	{ \
	if (__NAME__ == __NAME__##_cnt)	{ \
		; \
	} \
	else	{ \
		printf("ERROR: %s() %d: value of "#__NAME__" is not %d, actually it is %d\n" \
				, __func__ \
				, __LINE__ \
				, __NAME__ \
				, __NAME__##_cnt \
				); \
		mismatch ++; \
	} \
}while(0);

int
butter_check_counters(int spare, int hash_bar, int data_blk, int data_ex_blk, int place_holder_blk)
{
	int mismatch = 0;

	CHECK_COUNTER(spare);
	CHECK_COUNTER(hash_bar);
	CHECK_COUNTER(data_blk);
	CHECK_COUNTER(data_ex_blk);
	CHECK_COUNTER(place_holder_blk);

	if (mismatch)	{
		return BDB_INTERNAL_ERROR;
	}
	else	{
		return BDB_OK;
	}
}

/* eof */
