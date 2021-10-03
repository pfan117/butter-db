#include <stdio.h>	/* printf() */
#include <stdlib.h>
#include <strings.h>	/* bzero() */
#include <string.h>	/* memcmp() */
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

int
butter_pt_set(void * req_ptr)	{	/* PT task */
	butter_req_t * req = req_ptr;
	butter_set_ctx_t * ctx = &req->set_ctx;
	int r;

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: %s, %d: invalid bdb request magic number\n", __func__, __LINE__);
		return BDB_PARAM_ERROR;
	}

	PT_BEGIN

	butter_calculate_hash(req, req->key, req->key_len, req->hash);
	/* ask the lookup procedure to load the exist key */
	req->lookup_ctx.flags |= LOOKUP_F_LOAD_KEY;

	PT_SET;

	r = butter_pt_kv_lookup(req);
	if (BDB_OK == r)	{
		;
	}
	else if (BDB_NOT_FOUND == r)	{
		;
	}
	else if (BDB_PT_IO_REQ == r)	{
		return r;	/* yeild */
	}
	else	{
		req->pt_return_value = r;
		PT_SET;
		return req->pt_return_value;
	}

	if (req->d_key_matched)	{
		;	/* found exist, should try replace */
	}
	else if (req->d_key)	{
		butter_calculate_hash(req, req->d_key, req->d_key_len, ctx->exist_key_hash);
		if (butter_hash_cmp(req->set_ctx.exist_key_hash, req->hash))	{
			req->d_key_hash_conflict = 0;	/* no key conflict */
		}
		else	{
			req->d_key_hash_conflict = 1;	/* key conflict !! */
		}
	}
	else	{
		req->d_key_hash_conflict = 0;	/* fill into empty info block or hash bar slot */
	}

	/* write down the new data blk */
	CLEAN_CTX(write_data_blk_ctx);
	PT_SET;
	r = butter_pt_write_new_data_blk(req);
	check_sub_pt_result

	if (req->d_key_matched)	{
		/* replace K-V node only */
		CLEAN_CTX(replace_ctx);
		PT_SET;
		r = butter_pt_replace(req);
		check_sub_pt_result
		PT_SET;
		return BDB_OK;
	}

	/* create more hash bars if required */
	CLEAN_CTX(extend_ctx);
	PT_SET;
	r = butter_pt_extend(req);
	check_sub_pt_result

	PT_END

	return BDB_OK;
}

STATIC int
__new_butter_set_request(
		void * db, void * key
		, uint32_t key_len, void * value
		, uint32_t value_len
		, butter_req_t ** req_ptr
)
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

	if (value)	{
		if (value_len < 0)	{
			printf("ERROR: parameter error, invalid value length\n");
			return BDB_PARAM_ERROR;
		}
	}
	else	{
		if (value_len)	{
			printf("ERROR: parameter error, invalid value length, should be zero\n");
			return BDB_PARAM_ERROR;
		}
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
new_butter_set_request(
		void * db
		, void * key, uint32_t key_len
		, void * value, uint32_t value_len
)
{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return NULL;
	}

	r = __new_butter_set_request(db, key, key_len, value, value_len, &req);
	if (r)	{
		return NULL;
	}
	else	{
		return req;
	}
}

void
del_butter_request(void * req_ptr)	{
	butter_req_t * req = req_ptr;

	if (!req_ptr)	{
		return;
	}

	if (BUTTER_REQUEST_MAGIC != req->magic)	{
		printf("ERROR: %s, %d: invalid bdb request magic number\n", __func__, __LINE__);
		return;
	}

	if (req->d_key)	{
		Free(req->d_key);
		req->d_key = NULL;
	}

	Free(req_ptr);

	return ;
}

int
butter_set(void * db, void * key, uint32_t key_len, void * value, uint32_t value_len)	{
	butter_req_t * req;
	int r;

	r = butter_ptr_check(db);
	if (r)	{
		return r;
	}

	r = __new_butter_set_request(db, key, key_len, value, value_len, &req);
	if (r)	{
		return r;
	}

	for(;;)	{
		r = butter_pt_set(req);
		if (BDB_PT_IO_REQ == r)	{
			butter_io_chassis(req);
		}
		else	{
			break;
		}
	}

	del_butter_request(req);

	return r;
}

/* eof */
