/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <string.h>	/* bzero */
#include <stdlib.h>	/* malloc */
#include <unistd.h>	/* close */

#include "public.h"
#include "tst/internal.h"

int
case_055_alloc_free_6_items(void)	{
	return butter_db_place_holder_alloc_free_enum(6);
}

/* eof */
