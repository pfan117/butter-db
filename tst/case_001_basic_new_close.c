/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

int
case_001_basic_new_close(void)	{
	void * ht;
	butter_hash_mode_select(HASH_MODE_HASH_ONLY);
	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);
	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	// DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));
	DO(butter_check_counters(0, 0, 0, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	return BDB_OK;
}

/* eof */
