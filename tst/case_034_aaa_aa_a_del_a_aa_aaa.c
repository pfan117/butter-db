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
case_034_aaa_aa_a_del_a_aa_aaa(void)	{
	void * ht;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(__func__, 0)));
	NOT_NULL(ht);

	SET("a", "va");
	SET("aa", "vaa");
	SET("aaa", "vaaa");
	SET("aaaa", "vaaaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 4, 4, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("aaaa", "vaaaa");
	EXPECT("aaa", "vaaa");
	EXPECT("aa", "vaa");
	EXPECT("a", "va");

	DEL("a");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(1, 4, 3, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("aaaa", "vaaaa");
	EXPECT("aaa", "vaaa");
	EXPECT("aa", "vaa");
	MISSED("a");

	DEL("aa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(1, 4, 2, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("aaaa", "vaaaa");
	EXPECT("aaa", "vaaa");
	MISSED("aa");
	MISSED("a");

	DEL("aaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(2, 4, 1, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	EXPECT("aaaa", "vaaaa");
	MISSED("aaa");
	MISSED("aa");
	MISSED("a");

	DEL("aaaa");
	DO(butter_file_check(get_db_fn(__func__, 0), CHECK_FLAGS));
	DO(butter_check_counters(0, 0, 0, 0, 0)); /* spare, hash_bar, data_blk, data_ex_blk, place_holder_blk */
	MISSED("aaaa");
	MISSED("aaa");
	MISSED("aa");
	MISSED("a");

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(__func__, 0), 0));
	//DO(butter_file_check(get_db_fn(__func__, 0), BUTTER_DB_CHECK_DUMP | BUTTER_DB_DUMP_WITH_PREFIX));

	return BDB_OK;
}

/* eof */
