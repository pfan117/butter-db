/* vi: set sw=4 ts=4: */

#ifndef __LIB_BUTTER_DB_TST_INTERNAL_HEADER_INCLUDED__
#define __LIB_BUTTER_DB_TST_INTERNAL_HEADER_INCLUDED__

#define PRINT_KEY_STR	0

typedef int(*ht_tc)(void);

#include "include/internal.h"
#include "test-case-list.h"

#undef _T
#define _T(__F__)	extern int __F__(void);
ALL_TCS

#define DO(__OP__)	do	{\
	int r;\
	r = __OP__;\
	if (BDB_OK != r)	{\
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));\
		return r;\
	}\
}\
while(0);

#define NOT_NULL(__OP__)	do	{\
	if (NULL == (__OP__))	{\
		printf("ERROR: %s() %d: return NULL value\n", __FILE__, __LINE__);\
		return BDB_RETURN_NULL;\
	}\
}\
while(0);

#define SET(__KEY__,__VALUE__)	do	{ \
	int r; \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: set %s\n", __func__, __LINE__, #__KEY__); \
	r = butter_set(ht, __KEY__, sizeof(__KEY__) - 1, __VALUE__, sizeof(__VALUE__) - 1); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		;	/* pass */ \
	}\
}\
while(0);

#define DEL(__KEY__)	do	{ \
	int r; \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: del %s\n", __func__, __LINE__, #__KEY__); \
	r = butter_del(ht, __KEY__, sizeof(__KEY__) - 1); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		;	/* pass */ \
	}\
}\
while(0);

#define TEST_SPARE_MAX	64

#define SPARE_TEST_INIT	\
static uint64_t spare_starts[TEST_SPARE_MAX]; \
static uint64_t spare_size[TEST_SPARE_MAX]; \
static int spare_idx = 0;

#define ALLOC(__SIZE__)	do	{ \
	int r; \
	uint64_t start; \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: alloc blk size %d\n" \
			, __func__, __LINE__, __SIZE__); \
	r = butter_create_place_holder(ht, __SIZE__, &start); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		if (spare_idx < TEST_SPARE_MAX)	{ \
			spare_starts[spare_idx] = start; \
			spare_size[spare_idx] = __SIZE__; \
			spare_idx ++; \
		} \
		else	{ \
			printf("ERROR: %s() %d: too many spare blks\n", __func__, __LINE__); \
			return r; \
		} \
	}\
}\
while(0);

#define FREE(__IDX__)	do	{ \
	int r; \
	if (__IDX__ >= spare_idx)	{ \
		printf("ERROR: %s() %d: test case design error, no such spare blk\n" \
				, __FILE__, __LINE__); \
		return BDB_INTERNAL_ERROR; \
	} \
	if (0 == spare_starts[__IDX__])	{ \
		printf("ERROR: %s() %d: test case design error, double free a spare blk\n" \
				, __FILE__, __LINE__); \
		return BDB_INTERNAL_ERROR; \
	} \
	if (PRINT_KEY_STR) printf("INFO: %s() %d: free spare %d\n" \
			, __func__, __LINE__, __IDX__); \
	r = butter_remove_place_holder(ht, spare_starts[__IDX__], spare_size[__IDX__]); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else	{	\
		spare_starts[__IDX__] = 0; \
	}\
}\
while(0);

#define EXPECT(__KEY__,__VALUE__)	do	{ \
	char exp_buf[sizeof(__VALUE__)]; \
	uint32_t l = sizeof(exp_buf); \
	int r; \
	r = butter_get(ht, __KEY__, sizeof(__KEY__) - 1, exp_buf, &l); \
	if (r)	{ \
		printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
	else if (l != (sizeof(__VALUE__) - 1))	{ \
		printf("ERROR: %s() %d, unmatched data length, expect %zd, returned %d\n" \
				, __FILE__, __LINE__ \
				, sizeof(__VALUE__) - 1 \
				, l \
				); \
		return BDB_DATA_ERROR; \
	} \
	else if (memcmp(__VALUE__, exp_buf, l))	{ \
		printf("ERROR: %s() %d, unmatched data\n", __FILE__, __LINE__); \
		printf("expected:\n");\
		butter_hex_dump(__VALUE__, sizeof(__VALUE__) - 1);\
		printf("got:\n");\
		butter_hex_dump(exp_buf, sizeof(__VALUE__) - 1);\
		return BDB_DATA_ERROR; \
	} \
	else	{	\
		;	/* pass */ \
	}\
}\
while(0);

#define MISSED(__KEY__)	do	{ \
	char exp_buf[8]; \
	uint32_t l = sizeof(exp_buf); \
	int r; \
	r = butter_get(ht, __KEY__, sizeof(__KEY__) - 1, exp_buf, &l); \
	if (BDB_NOT_FOUND == r)	{ \
		;	/* pass */ \
	} \
	else	{ \
		printf("ERROR: %s() %d: data item supposed to be removed not %s\n", __FILE__, __LINE__, butter_get_error_string(r)); \
		return r; \
	} \
}\
while(0);

#define STR1(_S_)   #_S_
#define STR2(__S__) STR1(__S__)

extern char * get_db_fn(const char * name, int idx);
extern int butter_db_place_holder_alloc_free_enum(int cnt);

/* enum test interface */
typedef struct _enum_item_t	{
	char * key;
	char * value;
	int visited:1;
} enum_item_t;

extern int
tc_butter_enum(const char * caller_case_name, enum_item_t * tc_items, int tc_item_cnt
		, int spare_cnt, int hash_bar_cnt, int data_blk_cnt
		, int data_ex_blk_cnt, int place_holder_blk_cnt);

#endif
