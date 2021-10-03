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

static enum_item_t * enum_tc_items;
static int enum_tc_item_cnt;

static int
enum_butter_data_cb(butter_data_item_t * kv, void * user_param)	{
	int i;

	/* butter_hex_dump(kv->key, kv->key_len); */

	for (i = 0; i < enum_tc_item_cnt; i ++)	{
		if (memcmp(enum_tc_items[i].key, kv->key, kv->key_len))	{
			continue;
		}
		else	{
			if (enum_tc_items[i].visited)	{
				printf("ERROR: %s() %d: item visited twice\n", __func__, __LINE__);
				butter_hex_dump(kv->key, kv->key_len);
				return 1;	/* quit */
			}
			else	{
				enum_tc_items[i].visited = 1;
				return 0;	/* continue */
			}
		}
	}

	printf("ERROR: %s() %d: key not supposed to be found\n", __func__, __LINE__);
	butter_hex_dump(kv->key, kv->key_len);

	return 1;	/* continue */
}

static int
tc_butter_all_item_visited(void)	{
	int i;

	for (i = 0; i < enum_tc_item_cnt; i ++)	{
		if (enum_tc_items[i].visited)	{
			continue;
		}
		else	{
			return 0;
		}
	}

	return 1;
}

int
tc_butter_enum(
		const char * caller_case_name
		, enum_item_t * tc_items, int tc_item_cnt
		, int spare_cnt, int hash_bar_cnt, int data_blk_cnt, int data_ex_blk_cnt, int place_holder_blk_cnt)
{
	char exp_buf[64];
	uint32_t el;
	uint32_t l;
	void * ht;
	int i;
	int r;

	enum_tc_items = tc_items;
	enum_tc_item_cnt = tc_item_cnt;

	butter_hash_mode_select(HASH_MODE_ECHO);

	DO(butter_open(&ht, get_db_fn(caller_case_name, 0)));
	NOT_NULL(ht);

	for (i = 0; i < enum_tc_item_cnt; i ++)	{
		r = butter_set(ht, enum_tc_items[i].key, strlen(enum_tc_items[i].key)
				, enum_tc_items[i].value, strlen(enum_tc_items[i].value));
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
			return r;
		}
	}

	for (i = 0; i < enum_tc_item_cnt; i ++)	{
		l = sizeof(exp_buf);
		el = strlen(enum_tc_items[i].value);
		r = butter_get(ht, enum_tc_items[i].key, strlen(enum_tc_items[i].key), exp_buf, &l);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
			return r;
		}
		else if (l != el)	{
			printf("ERROR: %s() %d, unmatched data length, expect %d, returned %d\n"
					, __FILE__, __LINE__
					, el
					, l
					);
			return BDB_DATA_ERROR;
		}
		else if (memcmp(enum_tc_items[i].value, exp_buf, l))	{
			printf("ERROR: %s() %d, unmatched data\n", __FILE__, __LINE__);
			printf("expected:\n");
			butter_hex_dump(enum_tc_items[i].value, el);
			printf("got:\n");
			butter_hex_dump(exp_buf, el);
			return BDB_DATA_ERROR;
		}
		else	{
			;	/* pass */
		}
	}

	DO(butter_file_check(get_db_fn(caller_case_name, 0), CHECK_FLAGS));
	/* spare, hash_bar */
	/* data_blk, data_ex_blk */
	/* place_holder_blk */
	DO(butter_check_counters(spare_cnt, hash_bar_cnt
			, data_blk_cnt, data_ex_blk_cnt
			, place_holder_blk_cnt)
			);
	#if defined SHOW_DETAILS
	butter_print_spare(ht);
	#endif

	r = butter_data_enum(ht, enum_butter_data_cb, NULL);
	if (r)	{
		printf("ERROR: %s() %d: data enum function return error\n", __func__, __LINE__);
		DO(butter_close(ht));
		return r;
	}

	if (tc_butter_all_item_visited())	{
		/* printf("DBG: %s() %d: all data visited\n", __func__, __LINE__); */
	}
	else	{
		printf("ERROR: %s() %d: some data not visited\n", __func__, __LINE__);
		DO(butter_close(ht));
		return r;
	}

	DO(butter_close(ht));
	DO(butter_file_check(get_db_fn(caller_case_name, 0), CHECK_FLAGS));

	return BDB_OK;
}

/* eof */
