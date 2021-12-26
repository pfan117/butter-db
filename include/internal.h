/* vi: set sw=4 ts=4: */

#ifndef __BUTTER_DB_INTERNAL_HEADER_INCLUDED__
#define __BUTTER_DB_INTERNAL_HEADER_INCLUDED__

#include <pthread.h>
#include "include/tree.h"
#include "include/pt.h"

typedef struct _butter_t butter_t;
typedef struct _butter_spare_t butter_spare_t;
typedef struct _butter_req_t butter_req_t;

#include "include/blk.h"
#include "include/mem.h"
#include "include/ctx.h"

#define MODULE_NAME	"butterDB"

#define design_error	printf("ERROR: %s %s, %s(): %d: not supposed to be here\n"\
		, MODULE_NAME, __FILE__, __func__, __LINE__);

extern int butter_ptr_check(butter_t * p);
extern int butter_load_spare_chain(butter_t * p);
extern void butter_free_spare_chain(butter_t * p);
extern int butter_get_spare(butter_req_t * req, uint32_t length);
extern int butter_add_spare_prepare(butter_req_t * req);
extern void butter_remove_spare(butter_req_t * req, butter_spare_t * spare);
extern void butter_update_spare_length(butter_req_t * req, butter_spare_t * spare, uint64_t new_length);

extern void butter_print_spare(butter_t * db);

/* eof */
extern int butter_alloc_space(butter_req_t * req);
extern int butter_free_space(butter_req_t * req);

#define Malloc(__SIZE__)	malloc(__SIZE__)
#define Free(__PTR__)	free(__PTR__)

/* location helpers */
#define HASH_BAR_JUMP_ITEM_LOCATION(__START__,__IDX__) (__START__ + (uint64_t)(((butter_hash_bar_blk *)NULL)->jump + __IDX__))
#define HASH_BAR_JUMP_ITEM_LOCATION2(__LEVEL__) (req->hash_bar_starts[__LEVEL__] + (uint64_t)(((butter_hash_bar_blk *)NULL)->jump + req->hash[__LEVEL__]))
#define HASH_BAR_JUMP_ITEM_SIZE	(sizeof((butter_hash_bar_blk *)NULL)->jump[0])
#define HASH_BAR_CNT_LOCATION(__START__) (__START__ + (uint64_t)&(((butter_hash_bar_blk *)NULL)->cnt))
#define HASH_BAR_SIZE	(sizeof(butter_hash_bar_blk))
#define HASH_BAR_CNT_SIZE	(sizeof((butter_hash_bar_blk *)NULL)->cnt)
#define HASH_BAR_JUMP_SIZE	(sizeof((butter_hash_bar_blk *)NULL)->jump)
#define INFO_BLK_TREE_ROOT		(uint64_t)&(((butter_info_blk *)NULL)->tree_root)
#define INFO_BLK_TREE_ROOT_SIZE	(sizeof(((butter_info_blk *)NULL)->tree_root))
#define EX_DATA_NEXT_LOCATION(__EX_DATA_START__) \
		__EX_DATA_START__ + (uint64_t)&(((butter_data_ex_blk *)NULL)->next)

extern void butter_calculate_hash(butter_req_t * req, char * key, uint32_t key_len, unsigned char * buf);
extern void butter_calculate_hash_and_print(char * key, int len);
extern void butter_hash_zero(void *);
extern int butter_hash_cmp(void * a, void * b);
extern int butter_pt_kv_lookup(butter_req_t * req);
extern int butter_alloc_space(butter_req_t * req);
extern int butter_free_space(butter_req_t * req);
extern int butter_pt_extend(butter_req_t * req);
extern int butter_pt_replace(butter_req_t * req);
extern int butter_pt_write_new_data_blk(butter_req_t * req);

extern int butter_create_place_holder(void * db, uint64_t size, uint64_t * start);
extern int butter_remove_place_holder(void * db, uint64_t start, uint64_t size);

/* i/o */
extern void butter_io_chassis(butter_req_t * req);

/* dbg */
extern void butter_hex_dump(const char * b, int l);
#define DUMP_HEX(__PTR__,__LEN__) butter_hex_dump((const char *)__PTR__, __LEN__)
#define PRINT_VALUE(__V__,__FMT__)	printf("DBG: %s() %d: "#__V__" = "__FMT__"\n"\
		, __func__, __LINE__, __V__)
#define VALUE_PRINT(__V__,__FMT__)	printf("DBG: %s() %d: "#__V__" = "__FMT__"\n"\
		, __func__, __LINE__, __V__)
extern void butter_print_lookup_ctx(butter_req_t *);

#if defined TEST_MODE

#define HASH_MODE_HASH_ONLY	0
#define HASH_MODE_SALT		1
#define HASH_MODE_ALL_ZERO	2
#define HASH_MODE_ECHO		3
extern void butter_hash_mode_select(int);
extern int butter_test_start(int tc_idx);

#endif	/* TEST_MODE */

extern int butter_check_counters(
		int spare
		, int hash_bar
		, int data_blk
		, int data_ex_blk
		, int place_holder_blk);

#endif
