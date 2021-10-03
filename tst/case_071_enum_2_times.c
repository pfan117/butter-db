/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

#if 0
	#define SHOW_DETAILS
	#define CHECK_FLAGS	(BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX)
#else
	#define CHECK_FLAGS 0
#endif

static enum_item_t items[] = {
	{ "0", "d0", 0 },
	{ "1", "d1", 0 },
};

int
case_071_enum_2_times(void)	{
	int r;

	r = tc_butter_enum(__func__, items, sizeof(items) / sizeof(items[0])
			/* spare_cnt, hash_bar_cnt */
			/* data_blk_cnt, data_ex_blk_cnt, place_holder_blk_cnt */
			, 0, 1, 2, 0, 0
			);

	return r;
}

/* eof */
