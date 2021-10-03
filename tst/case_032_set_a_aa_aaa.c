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
case_032_set_a_aa_aaa(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("a", "va");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 0, 1, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("a", "va");

	SET("aa", "vaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 2, 2, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("a", "va");
	EXPECT("aa", "vaa");

	SET("aaa", "vaaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 3, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("a", "va");
	EXPECT("aa", "vaa");
	EXPECT("aaa", "vaaa");

	SET("aaaa", "vaaaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 4, 4, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("a", "va");
	EXPECT("aa", "vaa");
	EXPECT("aaa", "vaaa");
	EXPECT("aaaa", "vaaaa");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));

	return BDB_OK;
}

/* eof */
