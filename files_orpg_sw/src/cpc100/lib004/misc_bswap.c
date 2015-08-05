
/************************************************************************

    Module name: bswap.c

    Description: This file contains the function MISC_bswap.

************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:45:49 $
 * $Id: misc_bswap.c,v 1.4 2004/05/27 16:45:49 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 * $Log: misc_bswap.c,v $
 * Revision 1.4  2004/05/27 16:45:49  jing
 * Update
 *
 * Revision 1.1  2000/09/12 17:46:12  jing
 * Initial revision
 *
 * Revision 1.10  1999/11/01 21:32:45  jing
 * @
 *
 * Revision 1.9  1999/02/26 22:53:28  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.8  1998/09/18 14:27:40  eforren
 * Fix missing function int_swap_bytes and pow
 *
 * Revision 1.7  1998/09/17 20:32:56  eforren
 * Add additional byte swap functions
 *
 * Revision 1.6  1998/06/19 21:19:37  hoyt
 * posix update
 *
 * Revision 1.5  1998/02/10 21:58:51  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:41:37  cm
 * Initial revision
 *
 */

#include <config.h>
#include <string.h>
#include <misc.h>
#include <float.h>

typedef short two_byte_t;	/* This must be a 2-byte type */

/************************************************************************

    Description: This function performs byte swap of an array of "n_item"
	elements if the local machine is of little endian. The size of
	each element is "swap_size" bytes, which must be 2, 4, or 8. The
	output buffer "out_buf" can overlap with or identical to the input
	array "in_buf".

    Inputs:	swap_size - size of array elements
		in_buf - the input array
		n_items - number of items in the input array

    Outputs:	out_buf - the output array

    Returns: Number of items processed (n_items) on success or -1
	on failure.

    Notes: memcpy is used here for portability. This requires that in_buf
	and out_buf are either identical or non-overlap. If memmove is
	used instead, this limitation does not exist. However memmove
	is not supported on some of the platforms.

************************************************************************/


#define TMP_SIZE 1024

int MISC_bswap (int swap_size, void *in_buf, int n_items, void *out_buf)
{
    static int big_endian = -1;
    int tlen, n_done;

    if (big_endian < 0) {	/* test if the local host is big endian */
	union {
	    two_byte_t s;	
	    char b [2];
	} test;

	test.s = 1;
	if (test.b [1] != 0)
	    big_endian = 1;
	else
	    big_endian = 0;
    }

    if (n_items < 0)
	return (-1);

    if (big_endian && swap_size > 0) {
	if (out_buf != in_buf)
	    memcpy (out_buf, in_buf, n_items * swap_size);
	return (n_items);
    }

    if (swap_size < 0)
	swap_size = -swap_size;
    tlen = n_items * swap_size;
    n_done = 0;
    while (1) {
	char tbuf [TMP_SIZE];
	char *src, *dest;
	int len;

	len = tlen - n_done;
	if (len <= 0)
	    return (n_items);
	if (len > TMP_SIZE)
	    len = TMP_SIZE;

	src = (char *)in_buf + n_done;
        dest = tbuf;

	switch (swap_size) {
	    char *end;

	    case 8:
		end = src + len - 8;
		while (src <= end) {
		    dest [7] = src [0];
		    dest [6] = src [1];
		    dest [5] = src [2];
		    dest [4] = src [3];
		    dest [3] = src [4];
		    dest [2] = src [5];
		    dest [1] = src [6];
		    dest [0] = src [7];
		    dest += 8;
		    src += 8;
		}
		break;

	    case 4:
		end = src + len - 4;
		while (src <= end) {
		    dest [3] = src [0];
		    dest [2] = src [1];
		    dest [1] = src [2];
		    dest [0] = src [3];
		    dest += 4;
		    src += 4;
		}
		break;

	    case 2:
		end = src + len - 2;
		while (src <= end) {
		    dest [1] = src [0];
		    dest [0] = src [1];
		    dest += 2;
		    src += 2;
		}
		break;

	    default:
		return (-1);
	}
	memcpy ((char *)out_buf + n_done, tbuf, len);
	n_done += len;
    }
    return(-1); /* this should never happen - put here to make cireport happy */
}

/***************************************************************************

    Description: This function performs byte swap of an array of shorts.

    Inputs:	buf - pointer to the array.
		size - size of the array.

***************************************************************************/

void MISC_short_swap (void *buf, int size)
{
#ifdef LITTLE_ENDIAN_MACHINE
    int i;
    unsigned short *spt;

    spt = (unsigned short *)buf;
    for (i = 0; i < size; i++) {
	*spt = SHORT_BSWAP (*spt);
	spt++;
    }
#endif
    return;
}

/********************************************************************
			
    Description: This function swaps bytes for n consecutive short values

    Input:	noofshorts - number of shorts to convert
    		buf - pointer to the first short to convert

    Output:	buf - buffer containing converted bytes

    Returns:	none

********************************************************************/
void MISC_swap_shorts (int noofshorts, short* buf)
{
   int i;
   short* curval = buf;
   for (i = 0; (i < noofshorts); i++,curval++)
       *curval = SHORT_BSWAP(*curval);
}

/********************************************************************
			
    Description: This function swaps bytes for n consecutive long values

    Input:	nooflongs - number of longs to convert
    		buf - pointer to the first long to convert

    Output:	buf - buffer containing converted bytes

    Returns:	none

********************************************************************/
void MISC_swap_longs (int nooflongs, long* buf)
{
   int i;
   long* curval = buf;
   for (i = 0; (i < nooflongs); i++,curval++)
       *curval = INT_BSWAP(*curval);
}
 
 /********************************************************************
			
    Description: This function swaps bytes for n consecutive long values

    Input:	nooflongs - number of longs to convert
    		buf - pointer to the first long to convert

    Output:	buf - buffer containing converted bytes

    Returns:	none

********************************************************************/
void MISC_swap_floats (int nooffloats, float* buf)
{
   int i;
   long* curval = (long*)buf;
   for (i = 0; (i < nooffloats); i++,curval++)
       *curval = INT_BSWAP(*curval);
}

/********************************************************************
			
    Description: This returns 1 if this is a big endian machine. 
		Otherwise it returns 0.

********************************************************************/

int MISC_i_am_bigendian ()
{
    two_byte_t s;
    char *pt;

    s = 1;
    pt = (char *)&s;
    if ((int)pt[1] == (int)s)
	return (1);
    else
	return (0);
}
