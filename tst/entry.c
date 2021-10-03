#include <stdio.h>
#include <stdlib.h>	/* exit() */
#include "public.h"

#include "tst/internal.h"

#undef _T
#define _T(__OPT__)	#__OPT__,
static char * tc_names[] = {
ALL_TCS
};

#undef _T
#define _T(__OPT__)	__OPT__,
static ht_tc tcs[] = {
ALL_TCS
};

char butter_db_fn[512];

char *
get_db_fn(const char * name, int idx)	{
	int r;

	r = snprintf(butter_db_fn, sizeof(butter_db_fn), STR2(TMPDATAPATH)"/%s.%d.butter", name, idx);
	if (r <= 0 || r >= sizeof(butter_db_fn))	{
		snprintf(butter_db_fn, sizeof(butter_db_fn), "%s", "need-a-short-file-name.butter");
	}

	return butter_db_fn;
}

int
butter_test_start(int tc_idx)	{
	int i;
	int r;
	int ccnt = 0;
	int ecnt = 0;

	#if defined DEBUG_MODE && 0
	printf("DBG: sizeof(butter_info_blk) = %lu\n", sizeof(butter_info_blk));
	printf("DBG: sizeof(butter_spare_blk) = %lu\n", sizeof(butter_spare_blk));
	printf("DBG: sizeof(butter_hash_bar_blk) = %lu\n", sizeof(butter_hash_bar_blk));
	printf("DBG: sizeof(butter_data_blk) = %lu\n", sizeof(butter_data_blk));
	printf("DBG: sizeof(butter_data_ex_blk) = %lu\n", sizeof(butter_data_ex_blk));
	#endif

	#if 0
	printf("DBG: hash of 0\n"); butter_calculate_hash_and_print("0", 1);
	printf("DBG: hash of 26\n"); butter_calculate_hash_and_print("26", 2);
	#endif

	for (i = 0; i < sizeof(tcs) / sizeof(tcs[0]); i ++)	{

		if (tc_idx >= 0)	{
			if (tc_idx == i)	{
				;
			}
			else	{
				continue;
			}
		}

		printf("============\n");
		printf("%d - %s\n", i, tc_names[i]);
		r = tcs[i]();
		if (BDB_OK == r)	{
			ccnt ++;
		}
		else	{
			printf("%s\n", butter_get_error_string(r));
			ecnt ++;
			return -1;
		}
	}

	if (ecnt)	{
		printf("\n");
		printf("success %d, failed %d - FAILED\n", ccnt, ecnt);
		printf("\n");
		return -1;
	}
	else	{
		printf("\n");
		printf("success %d, failed %d - SUCCESS\n", ccnt, ecnt);
		printf("\n");
		return 0;
	}
}

/* eof */
