/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

#if 0
	#define CHECK_FLAGS	(BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX)
#else
	#define CHECK_FLAGS 0
#endif

int
case_035_set_replace_with_l31_hash_bar(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("0123456789012345678901234567890a", "valuea");
	SET("0123456789012345678901234567890b", "valueb");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 32, 2, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("0123456789012345678901234567890a", "valuea");
	EXPECT("0123456789012345678901234567890b", "valueb");

	SET("0123456789012345678901234567890b", "valuec");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(1, 32, 2, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("0123456789012345678901234567890a", "valuea");
	EXPECT("0123456789012345678901234567890b", "valuec");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));

	return BDB_OK;
}

/* eof */
