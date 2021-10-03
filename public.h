#ifndef __BUTTER_DB_PUBLIC_HEADER_INCLUDED__
#define __BUTTER_DB_PUBLIC_HEADER_INCLUDED__

#include <stdint.h>

extern int butter_open(void ** db, char * filename);
extern int butter_close(void * db);

extern char * butter_get_error_string(int errcode);

extern int butter_set(void * db
		, void * key, uint32_t key_len
		, void * value, uint32_t value_len);
extern int butter_get(void * db
		, void * key, uint32_t key_len
		, void * value, uint32_t * value_len);
extern int butter_get2(void * db
		, void * key, uint32_t key_len
		, void ** value_ptr, uint32_t * value_len);
extern int butter_del(void * db, void * key, uint32_t key_len);

/* PT level interfaces */
extern void * new_butter_set_request(void * db
		, void * key, uint32_t key_len
		, void * value, uint32_t value_len);
extern int butter_pt_set(void * req_ptr);

extern void * new_butter_get_request(void * db
		, void * key, uint32_t key_len
		, void * value, uint32_t * value_len);
extern void * new_butter_get_request2(void * db, void * key, uint32_t key_len);
extern int butter_pt_get(void * req_ptr);
/* for new_butter_get_request2() */
extern void butter_get_value_from_req(void * req, void ** value_ptr, uint32_t * value_len);
extern void butter_free(void *);

extern void * new_butter_del_request(void * db, void * key, uint32_t key_len);
extern int butter_pt_del(void * req_ptr);

extern void butter_mm_garbadge_collect(void * db);
extern int butter_flush(void * db);

extern void del_butter_request(void * req_ptr);

typedef struct _butter_data_item_t	{
	void * key;
	void * value;
	uint32_t key_len;
	uint32_t value_len;
} butter_data_item_t;

typedef enum	{
	BDB_IO_DONE,
	BDB_IO_READ,
	BDB_IO_WRITE,
	BDB_IO_SEEK,
	BDB_IO_GET_SIZE,
	BDB_IO_TRUNCATE,
} butter_io_request_type_e;

/*
 * butter_data_cb
 * return 0 - continue
 * return 1 - quit
 * return 2 - remove item
 */
typedef int(*butter_data_cb)(butter_data_item_t *, void * user_param);
extern int butter_data_enum(void * db, butter_data_cb cb, void * user_param);

/* check/dump */
#define BUTTER_DB_CHECK_DUMP		0x1
#define BUTTER_DB_DUMP_WITH_PREFIX	0x2
extern int butter_file_check(char * filename, int flags);

#define BUTTER_DB_R_INFO	\
__T(BDB_OK)\
__T(BDB_PARAM_ERROR)\
__T(BDB_NOT_FOUND)\
__T(BDB_NOT_OPEN)\
__T(BDB_FILE_OPEN_ERROR)\
__T(BDB_DATA_ERROR)\
__T(BDB_ALREADY_OPEN)\
__T(BDB_NO_SPACE)\
__T(BDB_BUFFER_TOO_SMALL)\
__T(BDB_IO_ERROR)\
__T(BDB_NO_MEMORY)\
__T(BDB_CB_QUIT)\
__T(BDB_RETURN_NULL)\
__T(BDB_RINFO_MAX)\
__T(BDB_OUT_OF_MEMORY)\
__T(BDB_INTERNAL_ERROR)\
__T(BDB_PT_IO_REQ)\

#define __T(__S__)	__S__,
enum	{
	BUTTER_DB_R_INFO
};

#endif
