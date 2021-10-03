/* vi: set sw=4 ts=4: */

#ifndef __BUTTER_DB_CTX_DATA_TYPES_HEADER_INCLUDED__
#define __BUTTER_DB_CTX_DATA_TYPES_HEADER_INCLUDED__

#define LOOKUP_F_MISMATCH	0x1
#define LOOKUP_F_LOAD_KEY	0x2

typedef struct _butter_kv_lookup_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	uint64_t blk_location;
	uint64_t flags;
	union	{
		butter_double_u32_t pre_read_buffer;
		butter_data_blk     d_header_loading_buffer;
		butter_data_ex_blk  d_ex_header_loading_buffer;
	};
} butter_kv_lookup_ctx_t;

enum	{
	SFOP_END,
	SFOP_WRITE_NEW_SPARE,
};

typedef enum	{
	FBOP_END,
	FBOP_CREATE_NEW_SPARE,
	FBOP_UPDATE_EXIST_SPARE,
	FBOP_UPDATE_SPARE_NEXT,
	FBOP_UPDATE_INFO_BLK,
	FBOP_DB_TRUNCATE,
	FBOP_REMOVE_SPARE_RB,
} butter_free_inst;

#define SFOP_MAX_CNT 4

typedef struct _butter_free_instruction_t	{
	butter_free_inst inst;
	butter_spare_t * d[3];
} butter_free_instruction_t;

typedef struct _butter_free_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	uint64_t start;
	uint64_t length;
	butter_spare_t * left_left;
	butter_spare_t * left;
	butter_spare_t * cur;
	butter_spare_t * right;
	butter_spare_t * right_right;
	uint64_t edge_of_left;
	uint64_t edge_of_cur;
	uint64_t edge_of_right;
	uint64_t db_size;

	uint64_t new_spare_length;

	butter_spare_blk blk;

	butter_free_instruction_t ops[SFOP_MAX_CNT];

	int ip;

} butter_free_ctx_t;

typedef struct _butter_replace_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	int hash_idx;
} butter_replace_ctx_t;

typedef struct _butter_extend_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	int hash_idx;
	int first_different_hash_value_idx;
	int deepest_hash_bar_idx;
	uint32_t hash_bar_jump_item_cnt;
	uint64_t last_create_item_start;
	butter_hash_bar_blk hash_bar;
} butter_extend_ctx_t;

#define ALLOC_F_EXT_END	0x1

typedef struct _butter_alloc_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	int flags;

	uint32_t request_length;
	uint32_t ceil_request_length;

	uint32_t length;	/* result */
	uint64_t start;		/* result */
	butter_spare_t * prev_spare;
	butter_spare_t * changed_spare;
	butter_spare_t * next_spare;

} butter_alloc_ctx_t;

typedef struct _butter_alloc_write_down_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
} butter_alloc_write_down_ctx_t;

typedef struct _butter_write_data_blk_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	uint32_t pad_length;
	uint32_t reserve;
	uint32_t need_create_ex_list:1;
	union	{
		butter_data_blk new_data_blk;
		butter_data_ex_blk new_data_ex_blk;
	};
} butter_write_data_blk_ctx_t;

typedef struct _butter_set_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	uint64_t new_data_blk_start;
	unsigned char exist_key_hash[HASH_LENGTH];
} butter_set_ctx_t;

typedef struct _butter_del_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	int hash_bar_level_to_keep;
	int hash_bar_free_idx;
} butter_del_ctx_t;

typedef struct _butter_create_place_holder_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
	uint64_t offset;
	butter_place_holder_blk blk;
} butter_create_place_holder_ctx_t;

typedef struct _butter_remove_place_holder_ctx_t	{
	int lc;	/* protothread */
	int pt_return_value;
} butter_remove_place_holder_ctx_t;

typedef struct _butter_enum_ctx_t	{
	butter_data_cb data_enum_cb;
	void * data_enum_user_param;
	union	{
		butter_double_u32_t pre_read_buffer;
		butter_data_blk     d_header_loading_buffer;
		butter_data_ex_blk  d_ex_header_loading_buffer;
	};
	butter_data_item_t cb_param;
	butter_hash_bar_blk * hash_bars[HASH_LENGTH];
	uint32_t enum_loaded_hash_depth;
	uint8_t hash_bar_item_idx[HASH_LENGTH];
	uint64_t ex_data_next;
} butter_enum_ctx_t;

#define BUTTER_REQUEST_MAGIC	0x6767abab

#define HASH_COLLISION_MAX	8

typedef struct _butter_req_t	{
	int magic;
	int lc;	/* protothread */
	int pt_return_value;
	butter_t * db;

	void * key;
	void * value;
	uint32_t key_len;
	uint32_t value_len;

	/* info collected by lookup procedure */
	unsigned char hash[HASH_LENGTH];
	uint64_t hash_bar_starts[HASH_LENGTH];
	uint32_t hash_bar_item_cnts[HASH_LENGTH];

	uint32_t hash_branch_depth;

	uint64_t d_start;
	uint64_t d_prev_start;
	uint64_t d_next_start;
	uint64_t d_first_ex_data;

	uint32_t d_magic;
	uint32_t d_blk_length;

	uint32_t d_idx;	/* collision index */
	uint32_t d_value_length;
	uint32_t d_key_len;
	uint32_t d_key_matched:1;
	uint32_t d_key_hash_conflict:1;
	void * d_key;

	/* context of sub preocedures */
	butter_kv_lookup_ctx_t lookup_ctx;

	union	{
		butter_alloc_ctx_t alloc_ctx;
		butter_free_ctx_t free_ctx;
	};

	union	{
		butter_set_ctx_t set_ctx;
		butter_del_ctx_t del_ctx;
		butter_create_place_holder_ctx_t create_place_holder_ctx;
		butter_remove_place_holder_ctx_t remove_place_holder_ctx;
		butter_enum_ctx_t enum_ctx;
	};

	butter_alloc_write_down_ctx_t alloc_write_down_ctx;

	union	{
		butter_write_data_blk_ctx_t write_data_blk_ctx;
		butter_extend_ctx_t extend_ctx;
		butter_replace_ctx_t replace_ctx;
	};

	/* i/o request */
	butter_io_request_type_e io_type;
	uint64_t io_request_size;
	uint64_t io_request_location;
	void * io_request_buffer;
	int io_handler_return_value;

} butter_req_t;

#define CLEAN_CTX(__C__)	bzero(&req->__C__, sizeof(req->__C__))

#endif
