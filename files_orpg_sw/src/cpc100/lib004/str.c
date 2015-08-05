
/*******************************************************************

    Description: The variable size string processing module (STR).

*******************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/12/06 22:35:19 $
 * $Id: str.c,v 1.12 2012/12/06 22:35:19 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <str.h>

typedef struct {
    int b_size;			/* buffer size */
    int n_bytes;		/* number of data bytes stored */
} Str_hd_t;			/* header for STR pointer */


static char *Str_expand (void *str, int length);

/***********************************************************************

    Increases the buffer size of string "str" to "length".

***********************************************************************/

char *STR_grow (void *vstr, int length) {
    Str_hd_t *hd, *new_hd;
    char *p, *str;

    str = (char *)vstr;
    hd = NULL;			/* not necessary - to turn off gcc warning */
    if (str != NULL) {
	hd = (Str_hd_t *)(str - sizeof (Str_hd_t));
	if (hd->b_size >= length)
	    return (str);
    }

    p = (char *)MISC_malloc (length + sizeof (Str_hd_t));
    new_hd = (Str_hd_t *)p;
    p += sizeof (Str_hd_t);
    new_hd->b_size = length;
    if (str == NULL)
	new_hd->n_bytes = 0;
    else {
	memcpy (p, str, hd->n_bytes);
	new_hd->n_bytes = hd->n_bytes;
	MISC_free (str - sizeof (Str_hd_t));
    }
    return (p);
}

/***********************************************************************

    This is a duplication of STR_copy.

***********************************************************************/

char *STR_create (const void *orig_str) {

    return (STR_copy (NULL, (char *)orig_str));
}

/***********************************************************************

    Grows the buffer size and discards all data.

***********************************************************************/

char *STR_reset (void *vstr, int length) {
    char *str = (char *)vstr;
    if (str != NULL)
	((Str_hd_t *)(str - sizeof (Str_hd_t)))->n_bytes = 0;
    if (length > 0 || str == NULL)
	str = STR_grow (str, length);
    return (str);
}

/***********************************************************************

    Copies string from "src_str" to "dest_str". "dest_str" can be NULL.

***********************************************************************/

char *STR_copy (char *dest_str, const char *src_str) {
    int nb = 0;
    if (src_str != NULL)
	nb = strlen (src_str) + 1; 
    dest_str = STR_grow (dest_str, nb);
    if (src_str != NULL)
	strcpy (dest_str, src_str);
    ((Str_hd_t *)(dest_str - sizeof (Str_hd_t)))->n_bytes = nb;
    return (dest_str);
}

/***********************************************************************

    Concatenats string "src_str" to "dest_str". "dest_str" can be NULL.
    "src_str" cannot be NULL.

***********************************************************************/

char *STR_cat (char *dest_str, const char *src_str) {
    int nb;

    if (dest_str == NULL || 
	((Str_hd_t *)(dest_str - sizeof (Str_hd_t)))->n_bytes == 0)
	return (STR_copy (dest_str, src_str));
    nb = strlen (dest_str) + strlen (src_str) + 1;
    dest_str = Str_expand (dest_str, nb);
    strcat (dest_str, src_str);
    ((Str_hd_t *)(dest_str - sizeof (Str_hd_t)))->n_bytes = nb;
    return (dest_str);
}

/***********************************************************************

    Appends "size" bytes pointed by "src_str" to the end of "dest_str". 
    "dest_str" can be NULL.

***********************************************************************/

char *STR_append (void *dest_str, const void *src_str, int size) {
    int nb = 0;
    if (dest_str != NULL)
	nb = ((Str_hd_t *)((char *)dest_str - sizeof (Str_hd_t)))->n_bytes;
    dest_str = Str_expand (dest_str, nb + size);
    if (src_str != NULL)
	memcpy ((char *)dest_str + nb, (char *)src_str, size);
    ((Str_hd_t *)((char *)dest_str - sizeof (Str_hd_t)))->n_bytes = nb + size;
    return ((char *)dest_str);
}

/************************************************************************

    Replaces data at "offset" of "len" bytes in string "str" by C "c_str".

************************************************************************/

char *STR_replace (char *str, int offset, int len, char *c_str, int size) {
    int s_size, nb, s;

    if (str == NULL || offset < 0 || len < 0 ||
	offset > (s_size = ((Str_hd_t *)(str - sizeof (Str_hd_t)))->n_bytes))
	return (str);			/* not done */

    nb = s_size + size - len;
    str = Str_expand (str, nb);
    if (offset + len > s_size)
	len = s_size - offset;
    s = s_size - (offset + len);
    if (s > 0 && size != len)
	memmove (str + offset + size, str + offset + len, s);
    if (size > 0)
	memcpy (str + offset, c_str, size);

    ((Str_hd_t *)(str - sizeof (Str_hd_t)))->n_bytes = nb;
    return (str);
}

/***********************************************************************

    Returns the number of data bytes stored in "str".

***********************************************************************/

int STR_size (void *str) {
    if (str == NULL)
	return (0);
    return (((Str_hd_t *)((char *)str - sizeof (Str_hd_t)))->n_bytes);
}

/***********************************************************************

    Frees "str".

***********************************************************************/

void STR_free (void *str) {
    if (str != NULL)
        MISC_free ((char *)str - sizeof (Str_hd_t));
}

/***********************************************************************

    Expands the buffer of "str" to at least "length" bytes. Additional
    space is allocated in case the buffer must increase.

***********************************************************************/

static char *Str_expand (void *str, int length) {
    if (str != NULL &&
		((Str_hd_t *)(str - sizeof (Str_hd_t)))->b_size >= length)
	return (str);
    return (STR_grow (str, (length * 3) / 2));
}

/***************************************************************************

    Generates a string from a list of input strings. The arg list must be
    terminated by NULL. The first argument must be STR type which is used
    to place the generated string.

***************************************************************************/

char *STR_gen (char *str, ...) {
    va_list args;
    char *p;
    int cnt;

    va_start (args, str);
    cnt = 0;
    while (1) {
	p = va_arg (args, char *);
	if (p == NULL)
	    break;
	if (cnt == 0)
	    str = STR_copy (str, p);
	else
	    str = STR_cat (str, p);
	cnt++;
    }
    va_end (args);
    return (str);
}
