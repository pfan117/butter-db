/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

int
case_010_set_get_4_record_0_26_4163_8864(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_HASH_ONLY);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("0", "value0");
	SET("26", "value26");
	SET("4163", "value4163");
	SET("8864", "value8864");

	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 5, 4, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */

	EXPECT("0", "value0");
	EXPECT("26", "value26");
	EXPECT("4163", "value4163");
	EXPECT("8864", "value8864");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	//DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 5, 4, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */

	return BDB_OK;
}

/* eof */
