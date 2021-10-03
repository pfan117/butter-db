#include <stdio.h>
#include "public.h"

#include "tst/internal.h"

//#define SHOW_CHECK_DETAILS
//#define SHOW_FREE_SEQUENCE

static void * ht;
static char db_filename[128];

static void
calculate_current_counters(
		uint64_t * spare_starts, int cnt, int * cur_spares, int * cur_blks)
{
	int i;
	int spares = 0;
	int blks = 0;
	uint64_t last;

	last = 1;

	for(i = 0; i < cnt; i ++)	{
		if (i)	{
			if (!last)	{
				if (spare_starts[i])	{
					spares ++;
				}
			}
			last = spare_starts[i];
		}
		else	{
			last = spare_starts[i];
		}

		if (spare_starts[i])	{
			blks ++;
		}
	}

	*cur_spares = spares;
	*cur_blks = blks;

	return;
}

static int
run_alloc_free_sequence(int * list, int max)	{
	uint64_t spare_starts[max];
	uint64_t spare_size[max];
	int spare_idx = 0;
	int i;
	int cur_spares;
	int cur_blks;

	for (i = 0; i < max; i ++)	{
		ALLOC(100);
	}

	#if defined SHOW_FREE_SEQUENCE
	for (i = 0; i < max; i ++)	{
		printf("%d ", list[i]);
	}
	printf("\n");
	#endif

	for (i = 0; i < max; i ++)	{
		FREE(list[i]);
		DO(butter_file_check(db_filename, 0));
		#if defined SHOW_CHECK_DETAILS
		DO(butter_file_check(db_filename
				, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
		#endif
		calculate_current_counters(spare_starts, max, &cur_spares, &cur_blks);
		DO(butter_check_counters(cur_spares, 0, 0, 0, cur_blks));
		/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	}

	return BDB_OK;
}

static int
enum_sequence(char * occupy, int * list, int pos, int max)	{
	int i;
	int r;

	if (pos >= max)	{
		return run_alloc_free_sequence(list, max);
	}

	for (i = 0; i < max; i ++)	{
		if (occupy[i])	{
			continue;
		}

		occupy[i] = 1;
		list[pos] = i;
		r = enum_sequence(occupy, list, pos + 1, max);
		if (r)	{
			return r;
		}
		occupy[i] = 0;
	}

	return BDB_OK;
}

static int
sequence_n(int n)	{
	char occupy[n];
	int list[n];
	int i;

	for (i = 0; i < n; i ++)	{
		occupy[i] = 0;
	}

	return enum_sequence(occupy, list, 0, n);
}

int
butter_db_place_holder_alloc_free_enum(int cnt)	{
	int r;

	butter_hash_mode_select(HASH_MODE_HASH_ONLY);

	r = snprintf(db_filename, sizeof(db_filename)
			, STR2(TMPDATAPATH)"/alloc.free.enum.cnt.%d.butter", cnt);
	if (r <= 0 || r >= sizeof(db_filename))	{
		printf("ERROR: %s() %d: path name too long\n", __func__, __LINE__);
		return BDB_INTERNAL_ERROR;
	}

	DO(butter_open(&ht, db_filename));
	NOT_NULL(ht);

	r = sequence_n(cnt);

	DO(butter_close(ht));

	return r;
}

/* eof */
