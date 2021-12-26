# Butter DB

+ Butter-db store K-V in a 256-way search tree. No rebalance operations, the tree is naturally balanced with the help of the uniform distribution of SHA512 values, hope so.

### Primary Methords
set, get, delete, enum, gc, fix

### Design Targets
Very limited functions. On hard drive. Single thread. Protothread. Using SHA256 value as location reference.

### Main algorithms
+ SHA256 hash value lookup tree: lookup.c, extend.c, del.c
+ File space allocation: spare.c, free.c, alloc.c
+ Link list for SHA256 conflict keys

### Source files:
+ alloc.c - The PT for file space allocation
+ check.c - The functions for butter-db checking
+ db.c - Create/open/close a butter-db file
+ del.c - PT for K-V deleting
+ enum.c - PT for K-V traverse
+ extend.c - PT for hash-bar tree extending
+ free.c - PT for file space reclaim
+ get.c - PT for getting values from a key
+ hash.c - Function for hash value calculation
+ io.c - The I/O chassis for all PTs
+ lookup.c - The PT for K-V lookup
+ place-holder.c - The PT for create/remove place-holder blocks
+ replace.c - The PT for replacing a exist K-V
+ set.c - The PT for record a K-V into a butter-db file
+ spare.c - The algorithm of file space allocating and reclaim
+ utils.c - Misc functions
+ write-data-blk.c - The PT for K-V write down

### When you read the code
+ Learn about Protothread first
+ Multiply get/check/enum operations to a file could happend concurrently
+ Multiply set/del/gc/fix operations to a file could not happend concurrently
+ Free space in the end of the file will be truncated
+ There are in memory records for every piece of free space on disk
+ There are 6 kinds of blocks in a butter-db file:
  - info block - head of the file, only one for a file
  - hash bar - item location jump list
  - data - a K-V
  - ex-data - a K-V which points to another K-V who's key has a conflict hash value
  - spare - free apace
  - place holder - no use, just keep some disk space
+ No cache design for K-V or hash bar, relys on OS cache mechanism

### Progress
| Function | Coding | Test | Corner case test |
| -------- | ------ | ---- | ---------------- |
| set | 100% | 100% | 0% |
| get | 100% | 100% | 0% |
| del | 100% | 100% | 0% |
| enum | 100% | 100% | 0% |
| gc | 0% | 0% | 0% |
| fix | 0% | 0% | 0% |

### Just for fun
+ 醉后不知天在水，满船清梦压星河。
