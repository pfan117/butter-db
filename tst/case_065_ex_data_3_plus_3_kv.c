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

int
case_065_ex_data_3_plus_3_kv(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	/* operation as before TC */
	SET("01234567890123456789012345678901a", "value0");
   	SET("01234567890123456789012345678901b", "value1");
	SET("01234567890123456789012345678901c", "value2");
	EXPECT("01234567890123456789012345678901a", "value0");
	EXPECT("01234567890123456789012345678901b", "value1");
	EXPECT("01234567890123456789012345678901c", "value2");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	DO(butter_check_counters(0, 32, 1, 2, 0));
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	/* something new */
	SET("0123456789012345678901234567890La", "Value0");
	SET("0123456789012345678901234567890Lb", "Value1");
	SET("0123456789012345678901234567890Lc", "Value2");
	EXPECT("0123456789012345678901234567890La", "Value0");
	EXPECT("0123456789012345678901234567890Lb", "Value1");
	EXPECT("0123456789012345678901234567890Lc", "Value2");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	/* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	DO(butter_check_counters(0, 32, 2, 4, 0));
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	DO(butter_close(ht));

	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));

	return BDB_OK;
}

/* eof */
