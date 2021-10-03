/* vi: set sw=4 ts=4: */

#ifndef __BUTTER_DB_IN_DISK_DATA_TYPES_HEADER_INCLUDED__
#define __BUTTER_DB_IN_DISK_DATA_TYPES_HEADER_INCLUDED__

#include <stdint.h>

#pragma pack(push)
#pragma pack(4)

#define DISK_SIZE_UNIT				32	/* bytes aka DSU */
#define DISK_SIZE_UNIT_CEIL_MASK	0x1f
/*
 * SHA256 hash value has 256 bits
 * take 8 bits one level
 * 256 / 8 = 32 levels
 * every level hash 2^8 = 256 slots
 */
#define HASH_LENGTH		32	/* 256 / 8 */
#define SLOTS_PRE_LEVEL	256

#define MAGIC_BLK_BDB_INFO		0x0ACFABDE
#define MAGIC_BLK_BDB_SPARE		0xC03B17CB
#define MAGIC_BLK_BDB_HASH_BAR	0xDC98CE12
#define MAGIC_BLK_BDB_DATA		0xBC71B30C
#define MAGIC_BLK_BDB_DATA_EX	0xAB30CBC7
#define MAGIC_BLK_BDB_HOLDER	0xAABBCCDD

typedef struct _butter_double_u32_t	{
	uint32_t magic;
	union	{
		uint32_t flags;	/* butter_info_blk */
		uint32_t pad_0;	/* butter_spare_blk */
		uint32_t cnt;	/* butter_hash_bar_blk; */
		uint32_t blk_length;	/* butter_data_blk, butter_data_ex_blk */
	};
} butter_double_u32_t;

/* butter db info block on disk */
typedef struct _butter_info_blk	{
	uint32_t magic;
	uint32_t flags;
	uint32_t create_sec;
	uint32_t create_usec;
	uint64_t spare_chain;
	uint64_t tree_root;
} butter_info_blk;

/* butter db place holder on disk */
typedef struct _butter_place_holder_blk	{
	uint32_t magic;
	uint32_t pad_0;
	uint64_t length;	/* length include the magic */
} butter_place_holder_blk;

/* butter db free block on disk */
typedef struct _butter_spare_blk	{
	uint32_t magic;
	uint32_t pad_0;
	uint64_t length;	/* length include the magic */
	uint64_t next;
} butter_spare_blk;

/* butter db hash bar on disk */
typedef struct _butter_hash_bar_blk	{
	uint32_t magic;
	uint32_t cnt;
	uint64_t jump[SLOTS_PRE_LEVEL];
} butter_hash_bar_blk;

/* buffer db data blk on disk */
typedef struct _butter_data_blk	{
	uint32_t magic;
	uint32_t blk_length;	/* include the magic; align to DSU, zero padding the end */
	uint32_t key_length;
	uint32_t value_length;
} butter_data_blk;

typedef struct _butter_data_ex_blk	{
	uint32_t magic;
	uint32_t blk_length;	/* include the magic; align to DSU, zero padding the end */
	uint64_t next;
	uint32_t key_length;
	uint32_t value_length;
} butter_data_ex_blk;

#pragma pack(pop)

#endif

/* eof */
