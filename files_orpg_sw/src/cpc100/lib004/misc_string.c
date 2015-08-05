/*************************************************************************

      Module: MISC_string.c

 Description:

	Miscellaneous Services string-processing routines.

	This file provides various string-processing routines.  By
	"string", we mean the usual C null-terminated array of characters.
	
	Functions that are public are defined in alphabetical order at
	the top of this file and are identified with a prefix of
	"MISC_string_".

	Functions that are private to this file are defined in alphabetical
	order, following the definition of the public functions.


 Assumptions:

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/07/08 17:19:17 $
 * $Id: misc_string.c,v 1.28 2009/07/08 17:19:17 steves Exp $
 * $Revision: 1.28 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <misc.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define MEM_INCREMENT	64     /* Allocate memory in 64-byte chunks ...   */


/**************************************************************************
 
    Returns the pointer to the basename of the path "path".

 **************************************************************************/

char *MISC_string_basename (char *path) {
    return (MISC_basename (path));
}

char *MISC_basename (char *path) {
    char *p;

    if (path == NULL)
	return ("");
    p = path + strlen (path) - 1;
    while (p >= path && *p != '/')
	p--;
    return (p + 1);
}

/**************************************************************************
 
    Creates the full path from file name "file" and directory "dir" and puts
    the result in "buf" of size "buf_size". Return pointer "buf" on success 
    or NULL on failure. Note that dir "" is the same as "/". The current
    dir is "." or "./".

 **************************************************************************/

char *MISC_full_path (char *dir, char *fname, char *buf, int buf_size) {
    int add_slash, dir_len;

    if (dir == NULL || fname == NULL)
	return (NULL);
    dir_len = strlen (dir);
    add_slash = 1;
    if (dir[dir_len - 1] == '/')
	add_slash = 0;
    if (dir_len + add_slash + strlen (fname) >= buf_size)
	return (NULL);
    strcpy (buf, dir);
    if (add_slash)
	strcat (buf, "/");
    strcat (buf, fname);
    return (buf);
}

/**************************************************************************

    Returns the directory part of "path". The dir does not have a trailing
    '/'. If no dir found, "." is returned. Note that "/" returns "".
    If "path" is NULL, "." is returned. If "buf" of size "buf_size" is too
    small, the output is truncated.

**************************************************************************/

char *MISC_dirname (char *path, char *buf, int buf_size) {
    char *p, *s;
    int len;

    s = ".";
    len = 1;
    if (path != NULL) {
	p = path + strlen (path);
	while (p >= path && *p != '/')
	    p--;
	while (p > path && p[-1] == '/')
	    p--;
	if (p >= path) {
	    s = path;
	    len = p - path;
	}
    }
    if (len >= buf_size)
	len = buf_size - 1;
    memcpy (buf, s, len);
    buf[len] = '\0';
    return (buf);
}

/**************************************************************************
 Description: Fit a source string into a target string of specified size.
              Use specified "fit" character to indicate replacement of
              one or more characters.  Manner in which string is "fit"
              into target string is determined by a flag.

       Input: Pointer to storage for target string.
              Size of storage for target string (includes terminating null).
              "fit" flag specifying how source string is to be "fit" into
                  target string:

              MISC_STRING_FIT_FRONT - replace characters at front of string
              MISC_STRING_FIT_MIDDLE - replace characters in middle of string
              MISC_STRING_FIT_TRUNC - replace chars at end of string (truncate)

              "fit" character (e.g., '*') that replaces one or more
                  characters.
              Pointer to (null-terminated) source string.

      Output: target string holds possibly-shortened string
     Returns: void
       Notes: If fit flag is "bad", will default to truncate the string.
              If fit char is not printable, will default to a printable
              character.
              The target and source strings may not overlap, as this
              function uses strncpy().
              Got the code for MISC_STRING_FIT_MIDDLE from Zhongqi.
 **************************************************************************/
void
MISC_string_fit(char *trgt_string, 
                size_t trgt_string_size,
                int fit_flag,
                char in_fit_char,
                const char *src_string)
{
    char fit_char ;

    if ((trgt_string == NULL)
                     ||
        (trgt_string_size <= 0)
                     ||
        (src_string == NULL)) {
        return ;
    }

    if (strlen(src_string) < trgt_string_size) {
        /*
         * Source string "fits" in target string with no work ...
         */
        (void) strncpy(trgt_string, src_string, trgt_string_size-1) ;
        trgt_string[trgt_string_size-1] = '\0' ;
        return ;
    }


    /*
     * Ensure the "fit" character is printable ...
     */
    if (isprint((int) in_fit_char)) {
        fit_char = in_fit_char ;        
    }
    else {
        fit_char = MISC_STRING_FIT_DFLT_FIT_CHAR ;
    }

    if (fit_flag == MISC_STRING_FIT_FRONT) {
        /*
         * Replace characters at the front of the string ...
         */
        size_t index = strlen(src_string) - trgt_string_size + 1 ;
        *(trgt_string + 0) = fit_char ;
        (void) strncpy((trgt_string + 1),
                       (src_string + index),
                       trgt_string_size - 1) ;
    }
    else if (fit_flag == MISC_STRING_FIT_MIDDLE) {
        /*
         * Replace characters in the middle of the string ...
         */
        size_t left_len ;
        int midpt ;
        size_t right_len ;

        midpt = trgt_string_size / 2 - 1;
        left_len = (size_t) midpt ;
        if (trgt_string_size % 2) {
            right_len = left_len + 1 ;
        }
        else {
            right_len = left_len ;
        }

        (void) strncpy(trgt_string, src_string, left_len);
        *(trgt_string + midpt) = fit_char ;
        (void) strncpy(trgt_string + midpt + 1,
                       src_string + strlen(src_string) - right_len,
                       right_len);
        *(trgt_string + trgt_string_size - 1) = '\0' ;
    }
    else {
        /*
         * Truncate the string ...
         */
        (void) strncpy(trgt_string, src_string, trgt_string_size-1) ;
        if (strlen(src_string) >= trgt_string_size-1) {
            *(trgt_string + trgt_string_size - 2) = fit_char ;
        }
    }

    *(trgt_string + trgt_string_size - 1) = '\0' ;

    return ;

/*END of MISC_string_fit()*/
}

/******************************************************************

    Converts characters in "str" to upper case. 

******************************************************************/

char *MISC_toupper (char *str) {
    char *p = str;
    while (*p != '\0') {
	*p = toupper (*p);
	p++;
    }
    return (str);
}

/******************************************************************

    Converts characters in "str" to lower case. 

******************************************************************/

char *MISC_tolower (char *str) {
    char *p = str;
    while (*p != '\0') {
	*p = tolower (*p);
	p++;
    }
    return (str);
}

/**********************************************************************

    Searches for the "ind"-th (0, 1, ...) token in string "str".
    "format" (e.g. "S\tQ\"") defines a separator (led with S), a
    quotation char (led with Q) and a conversion char (led with C). The
    accepted conversions are "c", "i", "u", "f", "d", and "x". The token
    is returned in caller-provided buffer "buf" of size "b_s". If the
    token is not found, buf is set to the empty string. This function
    returns the offset to the end of this token (e.g. str + ret is the
    pointer to the possible next token), 0 if the token is not found or a
    negative error code on failure. See misc.3 for details.

**********************************************************************/

int MISC_get_token (char *str, char *format, int ind, void *buf, int b_s) {
    char *p, *st, *end, c, sep, quo, conv;
    int cnt, len;

    if (b_s > 0 && buf != NULL)
	*((char *)buf) = '\0';

    sep = ' ';
    quo = conv = '\0';
    if (format != NULL) {
	p = format;
	while (*p != '\0') {
	    c = p[1];
	    if (c == '\0')
		return (MISC_TOKEN_FORMAT_ERROR);
	    if (*p == 'S')
		sep = c;
	    else if (*p == 'Q')
		quo = c;
	    else if (*p == 'C')
		conv = c;
	    else
		return (MISC_TOKEN_FORMAT_ERROR);
	    p += 2;
	}
    }
    if (str == NULL || str[0] == '\0' || (sep != '\n' && str[0] == '\n'))
	return (0);
    if (buf == NULL)
	ind = 0x7fffffff;
    p = str;
    cnt = -1;
    st = end = NULL;		/* no effect */
    while (cnt < ind) {
	while ((c = *p) == ' ' || (sep != '\t' && c == '\t'))
	    p++;
	st = p;
	end = NULL;
	if (quo != '\0' && *st == quo) {
	    st++;
	    p++;
	    while ((c = *p) != '\0' && c != quo && c != '\n') {
/*		if (c == sep && sep != ' ')
		    break;
*/
		p++;
	    }
	    if (*p == quo) {
		p++;
		if (sep == ' ') {
		    if ((c = *p) == ' ' || c == '\t' || c == '\n' || c == '\0')
			end = p - 1;
		}
		else {
		    end = p - 1;
		    while (*p == ' ' || (*p == '\t' && sep != '\t'))
			p++;
		    if ((c = *p) != sep && c != '\n' && c != '\0')
			end = NULL;
		}
	    }
	}
	else {
	    if (sep == ' ') {
		if (*st == '\0' || *st == '\n') {
		    end = st;
		    break;
		}
		while ((c = *p) != '\0' && c != ' ' && 
				c != '\t' && c != '\n' && c != quo)
		    p++;
	    }
	    else {
		while ((c = *p) != '\0' && c != '\n' && 
						c != sep && c != quo)
		    p++;
	    }
	    if (quo != '\0' && c == quo)
		break;
	    end = p;
	    if (sep != ' ') {
		while (end > st && 
			((c = end[-1]) == ' ' || (sep != '\t' && c == '\t')))
		    end--;
		if (*p != sep && st == end)
		    break;
	    }
	}
	cnt++;
	if (*p == '\0' || (sep != '\n' && *p == '\n') || end == NULL)
	    break;
	p++;
    }
    if (end == NULL)
	return (MISC_TOKEN_ERROR);
    if (buf == NULL)
	return (cnt + 1);
    if (cnt < ind)
	return (0);
    len = end - st;
    if (conv == '\0' || conv == 'c') {
	if (len >= b_s)
	    len = b_s - 1;
	if (len >= 0) {
	    memcpy ((char *)buf, st, len);
	    ((char *)buf)[len] = '\0';
	}
    }
    else {
	char tbuf[64];
	int ret;
	if (len >= 64)
	    return (MISC_TOKEN_ERROR);
	memcpy (tbuf, st, len);
	tbuf[len] = '\0';
	switch (conv) {
	    case 'i':
		ret = sscanf (tbuf, "%d%c", (int *)buf, &c);
		break;
	    case 'u':
		ret = sscanf (tbuf, "%u%c", (unsigned int *)buf, &c);
		break;
	    case 'f':
		ret = sscanf (tbuf, "%f%c", (float *)buf, &c);
		break;
	    case 'd':
		ret = sscanf (tbuf, "%lf%c", (double *)buf, &c);
		break;
	    case 'x':
		ret = sscanf (tbuf, "%x%c", (unsigned int *)buf, &c);
		break;
	    default:
		return (MISC_TOKEN_FORMAT_ERROR);
	}
	if (ret != 1)
	    return (MISC_TOKEN_ERROR);
    }
    return (p - str);
}

/**********************************************************************

    Returns the count of chars in "str" until the char is not in the set
    of "c_set". If the first char in c_set is '\0', we look for the
    count of chars that are not in the set.

**********************************************************************/

int MISC_char_cnt (char *str, char *c_set) {
    int cnt, in_set, i;
    char *p;

    if (*str == '\0')
	return (0);
    in_set = 1;
    if (c_set[0] == '\0') {
	c_set++;
	in_set = 0;
    }
    cnt = strlen (c_set);
    p = str;
    while (*p != '\0') {
	for (i = 0; i < cnt; i++) {
	    if (*p == c_set[i])
		break;
	}
	if (in_set) {
	    if (i >= cnt)
		break;
	}
	else {
	    if (i < cnt)
		break;
	}
	p++;
    }
    return (p - str);
}

#ifdef CMP_BZIP2_YES
#include <bzlib.h>
#define CMP_GZIP_YES
#endif

#ifdef CMP_GZIP_YES
#include <zlib.h>
#endif

/* Macro defintions used by the bzip2 compressor. */
#define BZIP2_MIN_BLOCK_SIZE_BYTES   100000  /* corresponds to 100 Kbytes */  
#define BZIP2_MIN_BLOCK_SIZE              1  /* corresponds to 100 Kbytes */  
#define BZIP2_MAX_BLOCK_SIZE              9  /* corresponds to 900 Kbytes */  
#define BZIP2_WORK_FACTOR                30  /* the recommended default */
#define BZIP2_NOT_VERBOSE                 0  /* turns off verbosity */
#define BZIP2_NOT_SMALL                   0  /* does not use small version */

static int (*Bz2BuffToBuffCompress) (char *, unsigned int *, char *, 
				unsigned int, int, int, int) = NULL;
static int (*Bz2BuffToBuffDecompress) (char *, unsigned int *, char *, 
				unsigned int, int, int) = NULL;
static int (*Zuncompress) (unsigned char *, unsigned long *, 
			const unsigned char *, unsigned long) = NULL;
static int (*Zcompress) (unsigned char *, unsigned long *, 
			const unsigned char *, unsigned long) = NULL;

static int Load_funcs (char *file);


/*******************************************************************

    Compresses "src_len" bytes in buffer "src" with "method". "dest"
    of size "dest_len" returns the compressed data. Returns the length 
    of the compressed data on success or a negative error code on 
    failure.

********************************************************************/

int MISC_compress (int method, char *src, int src_len,
				char *dest, int dest_len) {
    int ret, len;

    if (method == MISC_BZIP2) {
#ifdef CMP_BZIP2_YES
	int block_size;
	unsigned int dest_l;

	if (Bz2BuffToBuffCompress == NULL && (ret = Load_funcs ("bzip2")) < 0)
	    return (ret);

	/* Determine what block size to use ... try and fit entire data is one
	   block. */
	block_size = (src_len / BZIP2_MIN_BLOCK_SIZE_BYTES) + 1;
	if (block_size < BZIP2_MIN_BLOCK_SIZE)
	    block_size = BZIP2_MIN_BLOCK_SIZE;
	else if (block_size > BZIP2_MAX_BLOCK_SIZE)
	    block_size = BZIP2_MAX_BLOCK_SIZE;

	dest_l = dest_len;
	ret = Bz2BuffToBuffCompress (dest, &dest_l, src, src_len, block_size, 
			    BZIP2_NOT_VERBOSE, BZIP2_WORK_FACTOR);
	if (ret == BZ_OUTBUFF_FULL)
	    return (MISC_BUF_TOO_SMALL);
	if (ret != BZ_OK) {
	    MISC_log ("cmp: BZIP2 compression failed (%d)\n", ret);
	    return (MISC_CMP_FAILED);
	}
	len = dest_l;
#else
	return (MISC_NOT_SUPPORTED);
#endif
    }
    else if (method == MISC_GZIP) {
#ifdef CMP_GZIP_YES
	unsigned long long_dest_len, ret;
    
	if (Zcompress == NULL && (ret = Load_funcs ("gzip")) < 0)
	    return (ret);

	long_dest_len = dest_len;
	ret = Zcompress ((unsigned char *)dest, &long_dest_len, 
			(unsigned char *)src, (unsigned long)src_len);
	if (ret == Z_BUF_ERROR)
	    return (MISC_BUF_TOO_SMALL);
	if (ret != Z_OK) {
	    MISC_log ("cmp: ZLIB compression failed (%d)\n", ret);
	    return (MISC_CMP_FAILED);
	}
	len = long_dest_len;
#else
	return (MISC_NOT_SUPPORTED);
#endif
    }
    else
	return (MISC_BAD_METHOD);

    return (len);
}

/*****************************************************************

    Decompresses "src_len" bytes in buffer "src" with "method". The
    buffer "dest" of size "dest_len" for the decompressed data must 
    be supplied by the caller. Returns the decompressed size on 
    success or a negative error code.

*****************************************************************/

int MISC_decompress (int method, char *src, int src_len,
					char *dest, int dest_len) {
    int ret, len;

    if (method == MISC_BZIP2) {
#ifdef CMP_BZIP2_YES
	unsigned int dest_l;
	if (Bz2BuffToBuffDecompress == NULL && 
				(ret = Load_funcs ("bzip2")) < 0)
	    return (ret);
	dest_l = dest_len;
	ret = Bz2BuffToBuffDecompress (dest, &dest_l, src, src_len,
		       BZIP2_NOT_SMALL, BZIP2_NOT_VERBOSE);
	if (ret == BZ_OUTBUFF_FULL)
	    return (MISC_BUF_TOO_SMALL);
	if (ret != BZ_OK) {
	    MISC_log ("cmp: Bz2BuffToBuffDecompress failed (%d)\n", ret);
	    return (MISC_CMP_FAILED);
	}
	len = dest_l;
#else
	return (MISC_NOT_SUPPORTED);
#endif
    }
    else if (method == MISC_GZIP) {
#ifdef CMP_GZIP_YES
	unsigned long l_dest_l;
	l_dest_l = dest_len;
	if (Zuncompress == NULL && (ret = Load_funcs ("gzip")) < 0)
	    return (ret);;
	ret = Zuncompress ((unsigned char *)dest, &l_dest_l, 
			(unsigned char *)src, (unsigned long)src_len);
	if (ret == Z_BUF_ERROR)
	    return (MISC_BUF_TOO_SMALL);
	if (ret != Z_OK) {
	    MISC_log ("cmp: Zuncompress failed (%d)\n", ret);
	    return (MISC_CMP_FAILED);
	}
	len = l_dest_l;
#else
	return (MISC_NOT_SUPPORTED);
#endif
    }
    else 
	return (MISC_BAD_METHOD);

    return (len);
}

/*******************************************************************

    Loads functions for bzip2 and gzip if "file" is "bzip2"
    or "gzip" respectively. Returns 0 on success or a negative error
    code.

*******************************************************************/

static int Load_funcs (char *file) {

#ifdef LINK_STATIC_LIB
    if (strcmp (file, "bzip2") == 0) {
	Bz2BuffToBuffCompress = BZ2_bzBuffToBuffCompress;
	Bz2BuffToBuffDecompress = BZ2_bzBuffToBuffDecompress;
	return (0);
    }
    else if (strcmp (file, "gzip") == 0) {
	Zuncompress = uncompress;
	Zcompress = compress;
	return (0);
    }
#endif

    if (strcmp (file, "bzip2") == 0) {
	Bz2BuffToBuffCompress = MISC_get_func ("libbz2.so", 
					"BZ2_bzBuffToBuffCompress", 0);
	Bz2BuffToBuffDecompress = MISC_get_func ("libbz2.so", 
					"BZ2_bzBuffToBuffDecompress", 0);
	if (Bz2BuffToBuffCompress == NULL || Bz2BuffToBuffDecompress == NULL)
	    return (MISC_LIB_NOT_FOUND);
    }
    else if (strcmp (file, "gzip") == 0) {
	Zuncompress = MISC_get_func ("libz.so", "uncompress", 0);
	Zcompress = MISC_get_func ("libz.so", "compress", 0);
	if (Zuncompress == NULL || Zcompress == NULL)
	    return (MISC_LIB_NOT_FOUND);
    }
    return (0);
}

