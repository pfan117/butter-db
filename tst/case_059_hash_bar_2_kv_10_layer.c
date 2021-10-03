/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

//#define SHOW_DETAILS

int
case_059_hash_bar_2_kv_10_layer(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("0123456789", "value0123456789");
	#if defined SHOW_DETAILS
	DO(butter_file_check(get_db_fn(__func__, 0)
			, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	#else
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	#endif
	DO(butter_check_counters(0, 0, 1, 0, 0));
	/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	EXPECT("0123456789", "value0123456789");
	#if defined SHOW_DETAILS
	DO(butter_file_check(get_db_fn(__func__, 0)
			, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	#else
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	#endif
	DO(butter_check_counters(0, 0, 1, 0, 0));
	/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	SET("012345678a", "value012345678a");
	#if defined SHOW_DETAILS
	DO(butter_file_check(get_db_fn(__func__, 0)
			, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	#else
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	#endif
	DO(butter_check_counters(0, 10, 2, 0, 0));
	/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	EXPECT("0123456789", "value0123456789");
	EXPECT("012345678a", "value012345678a");

	DO(butter_close(ht));

	#if defined SHOW_DETAILS
	DO(butter_file_check(get_db_fn(__func__, 0)
			, BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	#else
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	#endif

	return BDB_OK;
}

/* eof */
