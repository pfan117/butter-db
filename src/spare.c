/* vi: set sw=4 ts=4: */

#include <fcntl.h> /* open */
#include <unistd.h>	/* close */
#include <stdlib.h>	/* malloc */
#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */

#include "public.h"
#include "include/internal.h"

//#define SHOWFREEDECISION

STATIC int
butter_spare_comp(butter_spare_t * a, butter_spare_t * b)	{
	return a->start - b->start;
}

RB_PROTOTYPE(BDB_SPARE, _butter_spare_t, rb, butter_spare_comp);
RB_GENERATE(BDB_SPARE, _butter_spare_t, rb, butter_spare_comp);

int
butter_load_spare_chain(butter_t * db)	{

	butter_spare_blk blk;
	butter_spare_t * spare;
	uint64_t l;
	size_t sr;
	int r;

	for (l = db->info_blk.spare_chain; l; l = blk.next)	{
		spare = Malloc(sizeof(butter_spare_t));
		if (!spare)	{
			return BDB_NO_MEMORY;
		}

		/* TODO check the range of l */

		sr = lseek(db->fd, SEEK_SET, l);
		if (l == sr)	{
			;
		}
		else	{
			Free(spare);
			return BDB_IO_ERROR;
		}

		r = read(db->fd, &blk, sizeof(blk));
		if (r != sizeof(blk))	{
			Free(spare);
			return BDB_IO_ERROR;
		}

		if (MAGIC_BLK_BDB_SPARE != blk.magic)	{
			Free(spare);
			printf("ERROR: spare block magic invalid\n");
			return BDB_DATA_ERROR;
		}

		if (blk.length <= 0)	{
			Free(spare);
			printf("ERROR: spare block length invalid\n");
			return BDB_DATA_ERROR;
		}

		bzero(spare, sizeof(butter_spare_t));
		spare->start = l;
		spare->length = blk.length;
		if (spare->start + spare->length <= spare->start)	{
			Free(spare);
			printf("ERROR: spare block loopback overflow\n");
			return BDB_DATA_ERROR;
		}

		if (BDB_SPARE_RB_INSERT(&db->spare_rb, spare))	{
			Free(spare);
			printf("ERROR: spare space already recorded in RB\n");
			return BDB_DATA_ERROR;
		}
	}
	
	return BDB_OK;
}

void
butter_free_spare_chain(butter_t * db)	{
	butter_spare_t * p;

	for (p = RB_MIN(BDB_SPARE, &db->spare_rb); p; p = RB_MIN(BDB_SPARE, &db->spare_rb))	{
		BDB_SPARE_RB_REMOVE(&db->spare_rb, p);
		Free(p);
	}
}

int
butter_get_spare(butter_req_t * req, uint32_t length)	{
	butter_alloc_ctx_t * ctx = &req->alloc_ctx;
	uint64_t splitable_length_min;
	butter_spare_t * first_avil;
	butter_spare_t * full_match;
	butter_spare_t * spare;
	butter_t * db;

	db = req->db;
	first_avil = NULL;
	full_match = NULL;
	splitable_length_min = length + DISK_SIZE_UNIT;

	RB_FOREACH(spare, BDB_SPARE, &db->spare_rb)	{
		if (spare->length == length)	{
			full_match = spare;
			break;
		}
		else if (spare->length >= splitable_length_min)	{
			if (first_avil)	{
				;
			}
			else	{
				first_avil = spare;
			}
		}
	}

	if (full_match)	{
		ctx->start = full_match->start;
		ctx->length = full_match->length;
		ctx->prev_spare = BDB_SPARE_RB_PREV(full_match);
		ctx->changed_spare = NULL;
		ctx->next_spare = BDB_SPARE_RB_NEXT(full_match);
		BDB_SPARE_RB_REMOVE(&db->spare_rb, full_match);
		Free(full_match);
		return BDB_OK;
	}

	if (first_avil)	{
		first_avil->length = first_avil->length - length;
		ctx->prev_spare = NULL;
		ctx->changed_spare = first_avil;
		ctx->next_spare = NULL;
		ctx->start = first_avil->start + first_avil->length;
		ctx->length = length;
		return BDB_OK;
	}

	return BDB_NOT_FOUND;
}

STATIC int
butter_add_spare_create_new(butter_req_t * req)	{
	butter_free_ctx_t * ctx = &req->free_ctx;
	butter_t * db = req->db;
	butter_spare_t * p;

	ctx->edge_of_cur = ctx->start + ctx->length;
	if (ctx->edge_of_cur <= ctx->start)	{
		printf("ERROR: %s() %d: spare space length loopback overflow\n"
				, __func__, __LINE__
				);
		return BDB_DATA_ERROR;
	}

	p = Malloc(sizeof(butter_spare_t));
	if (!p)	{
		return BDB_NO_MEMORY;
	}

	bzero(p, sizeof(butter_spare_t));
	p->start = ctx->start;
	p->length = ctx->length;

	if (BDB_SPARE_RB_INSERT(&db->spare_rb, p))	{
		Free(p);
		printf("ERROR: %s() %d: spare space already recorded in RB\n", __func__, __LINE__);
		return BDB_DATA_ERROR;
	}

	ctx->left = BDB_SPARE_RB_PREV(p);
	ctx->cur = p;
	ctx->right = BDB_SPARE_RB_NEXT(p);

	if (ctx->left)	{
		ctx->left_left = BDB_SPARE_RB_PREV(ctx->left);
	}
	else	{
		ctx->left_left = NULL;
	}

	if (ctx->right)	{
		ctx->right_right = BDB_SPARE_RB_NEXT(ctx->right);
	}
	else	{
		ctx->right_right = NULL;
	}

	if (ctx->left)	{
		ctx->edge_of_left = ctx->left->start + ctx->left->length;
	}
	else	{
		ctx->edge_of_left = 0;
	}

	return BDB_OK;
}


#define MERGE_LEFT	(ctx->edge_of_left == ctx->cur->start)
#define MERGE_RIGHT	(ctx->edge_of_cur == ctx->right->start)

STATIC int
butter_add_spare_set_actions(butter_req_t * req)	{
	butter_free_ctx_t * ctx = &req->free_ctx;

	if (ctx->left)	{
		if (MERGE_LEFT)	{
			if (ctx->right)	{
				if (MERGE_RIGHT)	{
					/* left, MERGE_LEFT, right, MERGE_RIGHT */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_UPDATE_EXIST_SPARE;
					ctx->ops[0].d[0] = ctx->left;			/* start */
					ctx->ops[0].d[1] = ctx->right;			/* end */
					ctx->ops[0].d[2] = ctx->right_right;	/* next */
					ctx->ops[1].inst = FBOP_REMOVE_SPARE_RB;
					ctx->ops[1].d[0] = ctx->cur;
					ctx->ops[1].d[1] = ctx->right;
					ctx->ops[1].d[2] = NULL;
					ctx->ops[2].inst = FBOP_END;
				}
				else /* !MERGE_RIGHT */	{
					/* left, MERGE_LEFT, right, !MERGE_RIGHT */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_UPDATE_EXIST_SPARE;
					ctx->ops[0].d[0] = ctx->left;	/* start */
					ctx->ops[0].d[1] = ctx->cur;	/* end */
					ctx->ops[0].d[2] = ctx->right;	/* next */
					ctx->ops[1].inst = FBOP_REMOVE_SPARE_RB;
					ctx->ops[1].d[0] = ctx->cur;
					ctx->ops[1].d[1] = NULL;
					ctx->ops[1].d[2] = NULL;
					ctx->ops[2].inst = FBOP_END;
				}
			}
			else /* !ctx->right */	{
				if (ctx->edge_of_cur == ctx->db_size)	{
					if (ctx->left_left)	{
						/* left, MERGE_LEFT, !right, edge, left_left */
						#if defined SHOWFREEDECISION
						printf("DBG: %s() %d\n", __func__, __LINE__);
						#endif
						ctx->ops[0].inst = FBOP_DB_TRUNCATE;
						ctx->ops[0].d[0] = ctx->left;
						ctx->ops[1].inst = FBOP_UPDATE_SPARE_NEXT;
						ctx->ops[1].d[0] = ctx->left_left;	/* the "target" */
						ctx->ops[1].d[1] = NULL;			/* new "next" */
						ctx->ops[2].inst = FBOP_REMOVE_SPARE_RB;
						ctx->ops[2].d[0] = ctx->left;
						ctx->ops[2].d[1] = ctx->cur;
						ctx->ops[2].d[2] = NULL;
						ctx->ops[3].inst = FBOP_END;
					}
					else	{
						/* left, MERGE_LEFT, !right, edge, !left_left */
						#if defined SHOWFREEDECISION
						printf("DBG: %s() %d\n", __func__, __LINE__);
						#endif
						ctx->ops[0].inst = FBOP_DB_TRUNCATE;
						ctx->ops[0].d[0] = ctx->left;
						ctx->ops[1].inst = FBOP_UPDATE_INFO_BLK;
						ctx->ops[1].d[0] = NULL;
						ctx->ops[2].inst = FBOP_REMOVE_SPARE_RB;
						ctx->ops[2].d[0] = ctx->left;
						ctx->ops[2].d[1] = ctx->cur;
						ctx->ops[2].d[2] = NULL;
						ctx->ops[3].inst = FBOP_END;
					}
				}
				else /* no truncate */	{
					/* left, MERGE_LEFT, !right, !edge */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_UPDATE_EXIST_SPARE;
					ctx->ops[0].d[0] = ctx->left;	/* start */
					ctx->ops[0].d[1] = ctx->cur;	/* end */
					ctx->ops[0].d[2] = NULL;		/* next */
					ctx->ops[1].inst = FBOP_REMOVE_SPARE_RB;
					ctx->ops[1].d[0] = ctx->cur;
					ctx->ops[1].d[1] = NULL;
					ctx->ops[1].d[2] = NULL;
					ctx->ops[2].inst = FBOP_END;
				}
			}
		}
		else /* !MERGE_LEFT */	{
			if (ctx->right)	{
				if (MERGE_RIGHT)	{
					/* left, !MERGE_LEFT, right, MERGE_RIGHT */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_UPDATE_EXIST_SPARE;
					ctx->ops[0].d[0] = ctx->cur;			/* start */
					ctx->ops[0].d[1] = ctx->right;			/* end */
					ctx->ops[0].d[2] = ctx->right_right;	/* next */
					ctx->ops[1].inst = FBOP_UPDATE_SPARE_NEXT;
					ctx->ops[1].d[0] = ctx->left;	/* the "target" */
					ctx->ops[1].d[1] = ctx->cur;	/* new "next" */
					ctx->ops[2].inst = FBOP_REMOVE_SPARE_RB;
					ctx->ops[2].d[0] = ctx->right;
					ctx->ops[2].d[1] = NULL;
					ctx->ops[2].d[2] = NULL;
					ctx->ops[3].inst = FBOP_END;
				}
				else /* !MERGE_RIGHT */	{
					/* left, !MERGE_LEFT, right, !MERGE_RIGHT */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_CREATE_NEW_SPARE;
					ctx->ops[0].d[0] = ctx->cur;	/* start */
					ctx->ops[0].d[1] = ctx->cur;	/* end */
					ctx->ops[0].d[2] = ctx->right;	/* next */
					ctx->ops[1].inst = FBOP_UPDATE_SPARE_NEXT;
					ctx->ops[1].d[0] = ctx->left;	/* the "target" */
					ctx->ops[1].d[1] = ctx->cur;	/* new "next" */
					ctx->ops[2].inst = FBOP_END;
				}
			}
			else /* !ctx->right */	{
				if (ctx->edge_of_cur == ctx->db_size)	{
					/* left, !MERGE_LEFT, !right, edge */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_DB_TRUNCATE;
					ctx->ops[0].d[0] = ctx->cur;
					ctx->ops[1].inst = FBOP_REMOVE_SPARE_RB;
					ctx->ops[1].d[0] = ctx->cur;
					ctx->ops[1].d[1] = NULL;
					ctx->ops[1].d[2] = NULL;
					ctx->ops[2].inst = FBOP_END;
				}
				else	{
					/* left, !MERGE_LEFT, !right, !edge */
					#if defined SHOWFREEDECISION
					printf("DBG: %s() %d\n", __func__, __LINE__);
					#endif
					ctx->ops[0].inst = FBOP_CREATE_NEW_SPARE;
					ctx->ops[0].d[0] = ctx->cur;	/* start */
					ctx->ops[0].d[1] = ctx->cur;	/* end */
					ctx->ops[0].d[2] = NULL;		/* next */
					ctx->ops[1].inst = FBOP_UPDATE_SPARE_NEXT;
					ctx->ops[1].d[0] = ctx->left;	/* the "target" */
					ctx->ops[1].d[1] = ctx->cur;	/* new "next" */
					ctx->ops[2].inst = FBOP_END;
				}
			}
		}
	}
	else /* !ctx->left */	{
		if (ctx->right)	{
			if (MERGE_RIGHT)	{
				/* !left, right, MERGE_RIGHT */
				#if defined SHOWFREEDECISION
				printf("DBG: %s() %d\n", __func__, __LINE__);
				#endif
				ctx->ops[0].inst = FBOP_UPDATE_EXIST_SPARE;
				ctx->ops[0].d[0] = ctx->cur;			/* start */
				ctx->ops[0].d[1] = ctx->right;			/* end */
				ctx->ops[0].d[2] = ctx->right_right;	/* next */
				ctx->ops[1].inst = FBOP_UPDATE_INFO_BLK;
				ctx->ops[1].d[0] = ctx->cur;	/* the new "head" */
				ctx->ops[2].inst = FBOP_REMOVE_SPARE_RB;
				ctx->ops[2].d[0] = ctx->right;
				ctx->ops[2].d[1] = NULL;
				ctx->ops[2].d[2] = NULL;
				ctx->ops[3].inst = FBOP_END;
			}
			else /* !MERGE_RIGHT */	{
				/* !left, right, !MERGE_RIGHT */
				#if defined SHOWFREEDECISION
				printf("DBG: %s() %d\n", __func__, __LINE__);
				#endif
				ctx->ops[0].inst = FBOP_CREATE_NEW_SPARE;
				ctx->ops[0].d[0] = ctx->cur;	/* start */
				ctx->ops[0].d[1] = ctx->cur;	/* end */
				ctx->ops[0].d[2] = ctx->right;	/* next */
				ctx->ops[1].inst = FBOP_UPDATE_INFO_BLK;
				ctx->ops[1].d[0] = ctx->cur;	/* the new "head" */
				ctx->ops[2].inst = FBOP_END;
			}
		}
		else /* !ctx->right */	{
			if (ctx->edge_of_cur == ctx->db_size)	{
				/* !left, !right, edge */
				#if defined SHOWFREEDECISION
				printf("DBG: %s() %d\n", __func__, __LINE__);
				#endif
				ctx->ops[0].inst = FBOP_DB_TRUNCATE;
				ctx->ops[0].d[0] = ctx->cur;
				ctx->ops[1].inst = FBOP_REMOVE_SPARE_RB;
				ctx->ops[1].d[0] = ctx->cur;
				ctx->ops[1].d[1] = NULL;
				ctx->ops[1].d[2] = NULL;
				ctx->ops[2].inst = FBOP_END;
			}
			else	{
				/* !left, !right, !edge */
				#if defined SHOWFREEDECISION
				printf("DBG: %s() %d\n", __func__, __LINE__);
				#endif
				ctx->ops[0].inst = FBOP_CREATE_NEW_SPARE;
				ctx->ops[0].d[0] = ctx->cur;	/* start */
				ctx->ops[0].d[1] = ctx->cur;	/* end */
				ctx->ops[0].d[2] = NULL;		/* next */
				ctx->ops[1].inst = FBOP_UPDATE_INFO_BLK;
				ctx->ops[1].d[0] = ctx->cur;	/* the new "head" */
				ctx->ops[2].inst = FBOP_END;
			}
		}
	}

	return BDB_OK;
}

int
butter_add_spare_prepare(butter_req_t * req)	{
	int r;

	r = butter_add_spare_create_new(req);
	if (r)	{
		return r;
	}

	r = butter_add_spare_set_actions(req);
	if (r)	{
		return r;
	}

	return BDB_OK;
}

void
butter_remove_spare(butter_req_t * req, butter_spare_t * spare)	{
	if (req && spare)	{
		BDB_SPARE_RB_REMOVE(&req->db->spare_rb, spare);
		Free(spare);
	}
	else	{
		/* it is normal to reach here */
		/* the caller function relay the checking on this function here */
	}
	return;
}

void
butter_update_spare_length(butter_req_t * req, butter_spare_t * spare, uint64_t new_length)
{
	spare->length = new_length;
	return;
}

void
butter_print_spare(butter_t * db)	{
	butter_spare_t * spare;

	printf(" spare in mem:");

	RB_FOREACH(spare, BDB_SPARE, &db->spare_rb)	{
		printf(" (%lu %lu %lu)"
				, spare->start, spare->length, spare->start + spare->length
				);
	}

	printf("\n");

	return;
}

/* eof */
