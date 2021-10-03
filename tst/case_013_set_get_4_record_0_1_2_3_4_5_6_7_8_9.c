/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

int
case_013_set_get_4_record_0_1_2_3_4_5_6_7_8_9(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_HASH_ONLY);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("0", "value0");
	SET("1", "value1");
	SET("2", "value2");
	SET("3", "value3");
	SET("4", "value4");
	SET("5", "value5");
	SET("6", "value6");
	SET("7", "value7");
	SET("8", "value8");
	SET("9", "value9");

	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 1, 10, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */

	EXPECT("0", "value0");
	EXPECT("1", "value1");
	EXPECT("2", "value2");
	EXPECT("3", "value3");
	EXPECT("4", "value4");
	EXPECT("5", "value5");
	EXPECT("6", "value6");
	EXPECT("7", "value7");
	EXPECT("8", "value8");
	EXPECT("9", "value9");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	//DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 1, 10, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */

	return BDB_OK;
}

/* eof */
