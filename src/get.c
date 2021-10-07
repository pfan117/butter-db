#include <stdio.h>	/* printf() */
#include <stdlib.h>
#include <strings.h>	/* bzero() */
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

int
butter_pt_get(void * req_ptr)	{	/* PT task */
	butter_req_t * req = req_ptr;
	butter_req_t * ctx = req;
	int r;

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: %s, %d: invalid bdb request magic number\n", __func__, __LINE__);
		return BDB_PARAM_ERROR;
	}

	PT_BEGIN

	butter_calculate_hash(req, req->key, req->key_len, req->hash);

	PT_SET
	r = butter_pt_kv_lookup(req);
	if (BDB_OK == r)	{
		;
	}
	else if (BDB_NOT_FOUND == r)	{
		PT_SET;
		return BDB_NOT_FOUND;
	}
	else if (BDB_PT_IO_REQ == r)	{
		return r;	/* yeild */
	}
	else	{
		req->pt_return_value = r;
		PT_SET;
		return req->pt_return_value;
	}

	req->io_request_size = req->d_value_length;

	if (req->io_request_size < 0)	{
		printf("ERROR: %s(), %d: data error, negative value length\n", __func__, __LINE__);
		PT_SET;
		return BDB_DATA_ERROR;
	}
	else if (!req->io_request_size)	{
		req->value_len = 0;
		return BDB_OK;
	}

	if (req->value)	{
		if (req->value_len < req->io_request_size)	{
			/* buffer too small */
			PT_SET;
			return BDB_BUFFER_TOO_SMALL;
		}
	}
	else	{
		req->value = Malloc(req->io_request_size);
		if (!req->value)	{
			PT_SET;
			return BDB_NO_MEMORY;
		}
	}

	req->io_request_buffer = req->value;
	yield_to_io_and_check_result(BDB_IO_READ);

	req->value_len = req->io_request_size;

	PT_END

	return BDB_OK;
}

STATIC int
__new_butter_get_request(
		void * db
		, void * key, uint32_t key_len, void * value, uint32_t value_len
		, butter_req_t ** req_ptr)
{
	butter_req_t * req;

	if (!db)	{
		printf("ERROR: parameter error, NULL db pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (BUTTER_DB_INFO_MAGIC != ((butter_t *)db)->magic)	{
		printf("ERROR: invalid magic number in db descriptor\n");
		return BDB_PARAM_ERROR;
	}

	if (!key)	{
		printf("ERROR: parameter error, NULL key pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (key_len <= 0)	{
		printf("ERROR: parameter error, invalid key length\n");
		return BDB_PARAM_ERROR;
	}

	if (!value)	{
		printf("ERROR: no value buffer provided\n");
		return BDB_PARAM_ERROR;
	}

	if (value_len <= 0)	{
		printf("ERROR: invalid value buffer length\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->key = key;
	req->key_len = key_len;
	req->value = value;
	req->value_len = value_len;

	*req_ptr = req;

	return BDB_OK;
}

void *
new_butter_get_request(void * db, void * key, uint32_t key_len, void * value_ptr, uint32_t * value_len)	{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return NULL;
	}

	r = __new_butter_get_request(db, key, key_len, value_ptr, *value_len, &req);
	if (r)	{
		return NULL;
	}
	else	{
		return req;
	}
}

/* caller provides buffer */
int
butter_get(void * db, void * key, uint32_t key_len, void * value, uint32_t *value_len)	{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return r;
	}

	r = __new_butter_get_request(db, key, key_len, value, *value_len, &req);
	if (r)	{
		return r;
	}

	for(;;)	{
		r = butter_pt_get(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	if (BDB_OK == r)	{
		*value_len = req->value_len;
	}

	del_butter_request(req);

	return r;
}

STATIC int
__new_butter_get_request2(void * db, void * key, uint32_t key_len, butter_req_t ** req_ptr)	{
	butter_req_t * req;

	if (!db)	{
		printf("ERROR: parameter error, NULL db pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (BUTTER_DB_INFO_MAGIC != ((butter_t *)db)->magic)	{
		printf("ERROR: invalid magic number in db descriptor\n");
		return BDB_PARAM_ERROR;
	}

	if (!key)	{
		printf("ERROR: parameter error, NULL key pointer\n");
		return BDB_PARAM_ERROR;
	}

	if (key_len <= 0)	{
		printf("ERROR: parameter error, invalid key length\n");
		return BDB_PARAM_ERROR;
	}

	req = Malloc(sizeof(butter_req_t));
	if (!req)	{
		return BDB_OUT_OF_MEMORY;
	}

	bzero(req, sizeof(butter_req_t));

	req->magic = BUTTER_REQUEST_MAGIC;
	req->db = db;
	req->key = key;
	req->key_len = key_len;

	*req_ptr = req;

	return BDB_OK;
}

void *
new_butter_get_request2(void * db, void * key, uint32_t key_len)	{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return NULL;
	}

	r = __new_butter_get_request2(db, key, key_len, &req);
	if (r)	{
		return NULL;
	}
	else	{
		return req;
	}
}

void
butter_get_value_from_req(void * req_ptr, void ** value_ptr, uint32_t * value_len)	{
	butter_req_t * req = req_ptr;

	if (!req)	{
		printf("ERROR: NULL req pointer\n");
		return;
	}

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: invalid bdb request magic number\n");
		return;
	}

	if (!value_ptr || !value_len)	{
		printf("ERROR: need to provide return value pointer and length pointer\n");
		return;
	}

	*value_ptr = req->value;
	*value_len = req->value_len;

	return;
}

/* butter lib provices buffer */
int
butter_get2(void * db, void * key, uint32_t key_len, void ** value_ptr, uint32_t * value_len)	{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return r;
	}

	if (!value_ptr || !value_len)	{
		printf("ERROR: %s: %s, return value pointer address not provided\n", MODULE_NAME, __func__);
		return BDB_PARAM_ERROR;
	}

	r = __new_butter_get_request2(db, key, key_len, &req);
	if (r)	{
		return r;
	}

	for(;;)	{
		r = butter_pt_get(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	butter_get_value_from_req(req, value_ptr, value_len);

	del_butter_request(req);

	return r;
}

/* eof */
