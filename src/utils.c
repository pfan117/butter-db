/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <ctype.h>

#include "public.h"
#include "include/internal.h"

#undef __T
#define __T(__S__)	#__S__,

static char * butter_error_strings[] = {
	BUTTER_DB_R_INFO
};

char *
butter_get_error_string(int errcode)	{
	if (errcode < 0)	{
		return "";
	}

	if (errcode >= (int)(sizeof(butter_error_strings) / sizeof(butter_error_strings[0])))	{
		return "";
	}

	return butter_error_strings[errcode];
};

#define DUMP_LINE_WIDTH  16

void
__butter_hex_dump_line(const char * b, int l)	{
    int i;

	for(i = 0; i < l; i ++) {
		printf("%02hhx ", (unsigned char)b[i]);
    }

    for(;i < DUMP_LINE_WIDTH; i ++)  {
        printf("   ");
    }

    printf("| ");

    for(i = 0; i < l; i ++) {
        if (isprint((int)(b[i])))  {
            printf("%c", b[i]);
        }
        else    {
            printf(" ");
        }
    }

    for(;i < DUMP_LINE_WIDTH; i ++)  {
        printf(" ");
    }

    printf(" |\n");

    return;
}

void butter_hex_dump(const char * b, int l) {
    const char * p = b;
    int left = l;

    while(left) {
        if (left >= DUMP_LINE_WIDTH) {
            __butter_hex_dump_line(p, DUMP_LINE_WIDTH);
            left -= DUMP_LINE_WIDTH;
            p += DUMP_LINE_WIDTH;
        }
        else    {
            __butter_hex_dump_line(p, left);
            break;
        }
    }

    printf("\n");
    return;
}

/* eof */
