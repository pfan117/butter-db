#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include "public.h"
#include "include/internal.h"

#if defined TEST_MODE

static int butter_hash_mode = HASH_MODE_SALT;

void butter_hash_mode_select(int mode)	{
	if (HASH_MODE_HASH_ONLY == mode)	{
		butter_hash_mode = HASH_MODE_HASH_ONLY;
	}
	else if (HASH_MODE_SALT == mode)	{
		butter_hash_mode = HASH_MODE_SALT;
	}
	else if (HASH_MODE_ALL_ZERO == mode)	{
		butter_hash_mode = HASH_MODE_ALL_ZERO;
	}
	else if (HASH_MODE_ECHO == mode)	{
		butter_hash_mode = HASH_MODE_ECHO;
	}
	else	{
		butter_hash_mode = HASH_MODE_SALT;
	}
}

#endif

int
butter_hash_cmp(void * a, void * b)	{
	return memcmp(a, b, HASH_LENGTH);
}

void
butter_hash_zero(void * hash)	{
	bzero(hash, HASH_LENGTH);
}

void
butter_calculate_hash(
		butter_req_t * req
		, char * key, uint32_t key_len, unsigned char * buf)
{

#if defined TEST_MODE
	if (HASH_MODE_HASH_ONLY == butter_hash_mode)	{
		SHA256((const unsigned char*)key, key_len, buf);
		return;
	}
	else if (HASH_MODE_SALT == butter_hash_mode)	{
		/* fall through */;
	}
	else if (HASH_MODE_ALL_ZERO == butter_hash_mode)	{
		bzero(buf, HASH_LENGTH);
		return;
	}
	else if (HASH_MODE_ECHO == butter_hash_mode)	{
		int i;

		for (i = 0; i < HASH_LENGTH; i ++)	{
			if (i < key_len)	{
				buf[i] = key[i];
			}
			else	{
				buf[i] = 0;
			}
		}

		return;
	}
#endif

	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, (const unsigned char *)key, key_len);
	SHA256_Update(&ctx
			, &req->db->info_blk.create_sec
			, sizeof(req->db->info_blk.create_sec));
	SHA256_Update(&ctx
			, &req->db->info_blk.create_usec
			, sizeof(req->db->info_blk.create_usec));
	SHA256_Final(buf, &ctx);

	return;
}

void
butter_calculate_hash_and_print(char * key, int len)	{
	unsigned char buf[1024];
	SHA256((const unsigned char*)key, len, buf);
	butter_hex_dump((char *)buf, 16);
	return;
}

/* eof */
