/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

int
case_016_set_replace_get_4163_8864_3(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_HASH_ONLY);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("4163", "value4163");	/* 95 d4 36 */
	SET("8864", "value8864");

	SET("3", "value3");
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("4163", "value4163");
	EXPECT("8864", "value8864");
	EXPECT("3", "value3");

	SET("4163", "VALUE4163");
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(1, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("4163", "VALUE4163");

	SET("8864", "VALUE8864");
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(1, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("8864", "VALUE8864");

	SET("3", "VALUE3");
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(1, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("8864", "VALUE8864");
	EXPECT("3", "VALUE3");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	//DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(1, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */

	return BDB_OK;
}

/* eof */
