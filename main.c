/* vi: set sw=4 ts=4: */

#include <stdio.h>	/* printf */
#include <getopt.h>
#include <stdlib.h>	/* atoi */
#include <string.h>	/* strlen */

#include "public.h"
#include "include/internal.h"

static char execute_mt = 0;
static char * tc = NULL;
static char * db_file_name = NULL;
static char * set_key = NULL;
static char * get_key = NULL;
static char * delete_key = NULL;
static char * value_str = NULL;

static void
show_usage(const char * name)	{
	printf("Usage:\n");
	printf("    %s options\n\n", name);
	printf("e.g.\n");
	printf("    %s --file demo.butter --set key_name --value data_string\n", name);
	printf("    %s --file demo.butter --get key_name\n", name);
	printf("    %s --file demo.butter --delete key_name\n", name);
	printf("\n");
	return;
}

static char * short_options = "fsgd";

static struct option long_options[] = {
	{"file", required_argument, NULL, 0},
	{"set", required_argument, NULL, 0},
	{"get", required_argument, NULL, 0},
	{"delete", required_argument, NULL, 0},
	{"mt", no_argument, NULL, 0},
	{"tc", required_argument, NULL, 0},
	{"value", required_argument, NULL, 0},
	{0, 0, 0, 0},
};

static int
parse_options(int argc, char ** argv)	{
	int option;
	int option_index = 0;

	if (argc < 2)	{
		show_usage(argv[0]);
		return -1;
	}

	while((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)	{
		switch(option)	{
		case 0:
			switch(option_index)	{
			case 0:	/* file */
				db_file_name = optarg;
				break;
			case 1:	/* set */
				set_key = optarg;
				break;
			case 2:	/* get */
				get_key = optarg;
				break;
			case 3:	/* delete */
				delete_key = optarg;
				break;
			case 4: /* mt */
				execute_mt = 1;
				break;
			case 5: /* tc */
				tc = optarg;
				break;
			case 6: /* value */
				value_str = optarg;
				break;
			}
			break;
		default:
			show_usage(argv[0]);
			return -1;
		}
	}

	return 0;
}

int
main(int argc, char ** argv)	{
	void * db;
	int r;

	r = parse_options(argc, argv);
	if (r)	{
		return -1;
	}

	#if defined DEBUG_MODE
	if (execute_mt)	{
		if (tc)	{
			int tc_idx;
			tc_idx = atoi(tc);
			if (tc_idx >= 0)	{
				return butter_test_start(tc_idx);
			}
			else	{
				return butter_test_start(-1);
			}
		}
		else	{
			return butter_test_start(-1);
		}
	}
	#endif

	if (set_key && db_file_name && value_str)	{
		int key_len;
		int value_len;

		key_len = strlen(set_key);
		value_len = strlen(value_str);

		if (key_len <= 0)	{
			printf("ERROR: key string length should be greater than 0\n");
			return -1;
		}

		if (value_len < 0)	{
			printf("ERROR: vlue length should be greater than -1\n");
			return -1;
		}

		r = butter_open(&db, db_file_name);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
			return -1;
		}

		r = butter_set(db, set_key, key_len, value_str, value_len);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
		}

		butter_close(db);

		return r;
	}

	if (get_key)	{
		int key_len;
		void * value;
		unsigned int value_len;

		key_len = strlen(get_key);

		if (key_len <= 0)	{
			printf("ERROR: key string length should be greater than 0\n");
			return -1;
		}

		r = butter_open(&db, db_file_name);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
			return -1;
		}

		r = butter_get2(db, get_key, key_len, &value, &value_len);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
		}
		else	{
			printf("%s", (char *)value);
			butter_free(value);
		}

		butter_close(db);

		return r;
	}

	if (delete_key)	{
		int key_len;

		key_len = strlen(delete_key);

		if (key_len <= 0)	{
			printf("ERROR: key string length should be greater than 0\n");
			return -1;
		}

		r = butter_open(&db, db_file_name);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
			return -1;
		}

		r = butter_del(db, delete_key, key_len);
		if (r)	{
			printf("ERROR: %s() %d: %s\n", __FILE__, __LINE__, butter_get_error_string(r));
		}

		butter_close(db);

		return r;
	}

	show_usage(argv[0]);

	return -1;
}

/* eof */
