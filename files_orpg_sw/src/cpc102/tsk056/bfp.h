/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/01/05 23:03:10 $
 * $Id: bfp.h,v 1.6 2007/01/05 23:03:10 ryans Exp $
 * $Revision: 1.6 $
 * $State: Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <smi.h>
#include <misc.h>


#define EXIT_CODE		99
#define MAX_FILE_NAME_SIZE	100

#define BFP_SUCCESS		0
#define BFP_FAILURE		-1

#define BFP_FALSE		0
#define BFP_TRUE		1

#define NUM_BYTES_PER_ROW	16
#define NUM_SHORTS_PER_ROW	16
#define NUM_INTS_PER_ROW	8

#define MAX_VAL_UCHAR		255
#define MAX_VAL_USHORT		65535
#define MAX_VAL_UINT		4294967295

#define MAX_SEARCH_STRING_BYTES 10
#define MAX_SEARCH_HEX_DIGITS	8
#define MAX_STRUCT_NAME_SIZE	30

#define CHAR_TYPE_NAME		"char"
#define UCHAR_TYPE_NAME		"unsigned char"
#define SHORT_TYPE_NAME		"short"
#define USHORT_TYPE_NAME	"unsigned short"
#define INT_TYPE_NAME		"int"
#define UINT_TYPE_NAME		"unsigned int"
#define FLOAT_TYPE_NAME		"float"


/* data types */
enum
{
   CHAR_TYPE = 1,
   UCHAR_TYPE = 2,
   SHORT_TYPE = 3,
   USHORT_TYPE = 4,
   INT_TYPE = 5,
   UINT_TYPE = 6,
   FLOAT_TYPE = 7
};


/* Function Prototypes */
int interpret_bin_file();
int modify_bin_file();
int search_bin_file();
int extract_bin_data();
int interpret_bin_to_ascii();
int interpret_bin_to_byte();
int interpret_bin_to_short();
int interpret_bin_to_int();
int interpret_bin_to_struct();
int modify_char_data(int fd, int size, int offset);
int modify_1byte_int_data(int fd, int size, int offset);
int modify_2byte_int_data(int fd, int size, int offset);
int modify_4byte_int_data(int fd, int size, int offset);
int modify_struct_data(int fd, int size);
int add_data(int fd, int size, int offset, int nbytes);
int search_char_data( char* in_file_name );
int search_numeric_data( char* in_file_name );
int extract_char_data();
int extract_1byte_int_data();
int get_char();
int file_size( char* filename );
void pause_user();
void print_struct_info();
int Set_byte_swap_flag();
