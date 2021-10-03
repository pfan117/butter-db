/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "include/tree.h"
#include "tst/internal.h"

//#define SHOW_STEP
//#define PRINT_KEY_STR
//#define SHOW_CHECK_DETAILS
//#define SHOW_BLKS_DETAILS

static void * ht;
static char db_filename[128];

typedef struct _blk_r_t	{
	uint64_t start;
	uint64_t end;
	RB_ENTRY(_blk_r_t) rb;
} blk_r_t;

#define BLK_CNT		16
#define BLK_SIZE	100

static blk_r_t blks[BLK_CNT];

/* event rb tree */
static int
blk_r_compare(blk_r_t * a, blk_r_t * b)	{
	return a->start - b->start;
}

RB_HEAD(BLK_R, _blk_r_t);
RB_PROTOTYPE_STATIC(BLK_R, _blk_r_t, rb, blk_r_compare);
RB_GENERATE_STATIC(BLK_R, _blk_r_t, rb, blk_r_compare);

static struct BLK_R blk_r_rb;

#define ALLOC2(__IDX__)	do	{ \
	int r; \
	uint64_t start; \
	blk_r_t * pp; \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: alloc blk size %d\n" \
			, __func__, __LINE__, BLK_SIZE); \
	r = butter_create_place_holder(ht, BLK_SIZE, &start); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		blks[__IDX__].start = start; \
		blks[__IDX__].end = start + BLK_SIZE; \
		pp = BLK_R_RB_INSERT(&blk_r_rb, blks + __IDX__); \
		if (pp && (pp != blks + (__IDX__)))	{ \
			printf("ERROR: %s() %d: duplicated start exist\n", __func__, __LINE__); \
		} \
	}\
}\
while(0);

#define FREE2(__IDX__)	do	{ \
	int r; \
	if (__IDX__ >= BLK_CNT)	{ \
		printf("ERROR: %s() %d: test case design error, no such spare blk\n" \
				, __FILE__, __LINE__); \
		return BDB_INTERNAL_ERROR; \
	} \
	if (0 == blks[__IDX__].start)	{ \
		printf("ERROR: %s() %d: test case design error, double free a spare blk\n" \
				, __FILE__, __LINE__); \
		return BDB_INTERNAL_ERROR; \
	} \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: free spare %d\n" \
			, __func__, __LINE__, __IDX__); \
	r = butter_remove_place_holder(ht, blks[__IDX__].start, BLK_SIZE); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		BLK_R_RB_REMOVE(&blk_r_rb, blks + __IDX__); \
		blks[__IDX__].start = 0; \
	}\
}\
while(0);

static void
calculate_current_counters(int * cur_spares, int * cur_blks)	{
	int spare_cnt = 0;
	int blks = 0;
	blk_r_t * p;
	uint64_t last_end;

	#ifdef SHOW_BLKS_DETAILS
	printf("DBG: recorded blks:");
	#endif

	last_end = sizeof(butter_info_blk);

	for (p = RB_MIN(BLK_R, &blk_r_rb); p; p = BLK_R_RB_NEXT(p))	{
		blks ++;

	#ifdef SHOW_BLKS_DETAILS
	printf(" 0x%08lx", p->start);
	#endif
		if (last_end == p->start)	{
			;
		}
		else	{
			spare_cnt ++;
		}

		last_end = p->end;
	}

	#ifdef SHOW_BLKS_DETAILS
	printf("\n");
	#endif

	*cur_spares = spare_cnt;
	*cur_blks = blks;

	return;
}

int
monkey_sequence(void)	{
	int i;
	int j;
	int k;
	int s;
	int cur_spares;
	int cur_blks;
	int idx;

	RB_INIT(&blk_r_rb);

	for (i = 0; i < BLK_CNT; i ++)	{
		blks[i].start = 0;
		blks[i].end = 0;
	}

	for (i = 0; i < 50; i ++)	{
		/* printf("i = %d\n", i ); */
		for (j = 0; j < 10; j ++)	{
			for (k = 0; k < 10; k ++)	{
				if ((i + k) & 1)	{
					if ((j + k) & 2)	{
						s = i + j + k;
					}
					else	{
						s = i - j + k;
					}
				}
				else	{
					if ((j + k) & 2)	{
						s = i + j + k + k;
					}
					else	{
						s = i + j + j - k;
					}
				}

				idx = (s & 0xf);
				// printf("%d, ", s);
				if (blks[idx].start)	{
					#ifdef SHOW_STEP
					printf("free %d, ", idx);
					#endif
					FREE2(idx);
				}
				else	{
					#ifdef SHOW_STEP
					printf("alloc %d, ", idx);
					#endif
					ALLOC2(idx);
				}
				#if defined SHOW_CHECK_DETAILS
				DO(butter_file_check(db_filename
						, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
				#else
				DO(butter_file_check(db_filename, 0));
				#endif
				calculate_current_counters(&cur_spares, &cur_blks);
				DO(butter_check_counters(cur_spares, 0, 0, 0, cur_blks));
			}
		}
	}

	#ifdef SHOW_STEP
	printf("\n");
	#endif

	return BDB_OK;
}

int
butter_db_place_holder_monkey_16_enum(void)	{
	int r;

	butter_hash_mode_select(HASH_MODE_HASH_ONLY);

	r = snprintf(db_filename, sizeof(db_filename)
			, STR2(TMPDATAPATH)"/alloc.free.m16.butter");
	if (r <= 0 || r >= sizeof(db_filename))	{
		printf("ERROR: %s() %d: path name too long\n", __func__, __LINE__);
		return BDB_INTERNAL_ERROR;
	}

	DO(butter_open(&ht, db_filename));
	NOT_NULL(ht);

	r = monkey_sequence();

	DO(butter_close(ht));

	return r;
}

int
case_056_monkey_alloc_free(void)	{
	return butter_db_place_holder_monkey_16_enum();
}

/* eof */
