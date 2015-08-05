/****************************************************************
		
	File: encrypt.c	
				
	4/13/94

	Purpose: This module contains functions for optional RMT
	security implementation. The functions include:

	ENCR_encrypt: Encrypt a password.

	Files used: 
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:53:40 $
 * $Id: rmt_encrypt.c,v 1.2 2004/05/27 16:53:40 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/*** Local include files ***/

#include "rmt_def.h"

/*** Definitions / macros / types ***/

/* #define ENCRYPT_TEST		 for module test */


#define AP_LENGTH  12		/* size of the arbitrary precision numbers
				   used;   24 characters */
#define N_ITERATIONS  100	/* number of iterations in encrypt */

typedef unsigned short * vli_type;	/* arbitrary precision number type */

#ifdef SGI
#define TCGETS TCGETA
#define TCSETS TCSETA
#endif


/*** Local references / local variables ***/

static void str_to_vli (char *str, vli_type a);
static void vli_add (vli_type a, vli_type b, vli_type c);
static void vli_mul (vli_type a, vli_type b, vli_type c);
static void vli_asign (vli_type a, vli_type b);




/*************************************************************************
			
	ENCR_encrypt()		 		Date: 4/13/94

	This function converts an NULL terminated string "str" to an 
	encrypted byte string, which will be put in "buf" upon return.
	The return value of the function is the length (number of bytes)
	of the encrypted byte string.

	The encrypted byte string is not an ASCII string. It is not
        NULL terminated. 

	The length of the input string is arbitrary. However, only the
	first AP_LENGTH * 2 bytes are processed. The length of the
	encrypted byte string is also AP_LENGTH * 2. If the buffer size 
	"buf_size" is too small to hold the encrypted byte string, the
	function returns only the first buf_size bytes.

	This function returns the length of the encrypted byte string.

*/

int ENCR_encrypt
  (
      char *str,		/* The input character string */
      int buf_size,		/* size of the output buffer "buf" */
      char *buf			/* buffer for the encrypted byte string */
) {
    unsigned short b[AP_LENGTH], c[AP_LENGTH];
    int i, j;
    int ret_len;

    /* convert the string to a vli number */
    str_to_vli (str, b);

    /* The encrypt algorithm */
    for (i = 0; i < N_ITERATIONS; i++) {
	vli_mul (b, b, c);

	for (j = 0; j < AP_LENGTH / 2; j++) {
	    c[j] += c[AP_LENGTH - j - 1];
	}
	vli_asign (b, c);
    }

    /* output */
    ret_len = AP_LENGTH * 2;
    if (ret_len > buf_size) ret_len = buf_size;
    memcpy (buf, (char *) c, ret_len);

    return (ret_len);

}


/*************************************************************************
			
	str_to_vli()		 		Date: 4/13/94

	This function converts a char string "str" into a very long integer
	(VLI, AP_LENGTH * 16 bits) "a".

	The very long integer is represented by an array of unsigned short
	integers. The first elements stores the lowest 16 bits.
*/

static void str_to_vli
  (
      char *str,
      vli_type a
) {
    int i, len;
    char *cpt;

    for (i = 0; i < AP_LENGTH; i++)
	a[i] = 0;

    cpt = (char *) a;

    len = strlen (str);
    if (len > AP_LENGTH * 2)
	len = AP_LENGTH * 2;
    strncpy (cpt, str, len);

    return;
}


/*************************************************************************
			
	vli_add()		 		Date: 4/13/94

	This function adds two vli numbers: c = a + b.
*/

static void vli_add
  (
      vli_type a,
      vli_type b,
      vli_type c
) {
    int i;
    unsigned short tmp[AP_LENGTH];

    for (i = 0; i < AP_LENGTH; i++) {
	c[i] = 0;
	tmp[i] = a[i];
    }

    for (i = 0; i < AP_LENGTH; i++) {
	unsigned long aa, bb, cc;

	aa = tmp[i];
	bb = b[i];
	cc = aa + bb;
	c[i] += cc & 0xffff;
	if ((cc >> 16) != 0) {
	    int j;

	    for (j = i + 1; j < AP_LENGTH; j++) {
		tmp[j] += 1;
		if (tmp[j] != 0)
		    break;
	    }
	}
    }

    return;

}

/*************************************************************************
			
	vli_asign()		 		Date: 4/13/94

	This function copies vli number b to a.
*/

static void vli_asign
  (
      vli_type a,
      vli_type b
) {
    int i;

    for (i = 0; i < AP_LENGTH; i++)
	a[i] = b[i];

    return;

}

/*************************************************************************
			
	vli_mul()		 		Date: 4/13/94

	This function multiplies two vli numbers: c = a * b.
*/

static void vli_mul
  (
      vli_type a,
      vli_type b,
      vli_type c
) {
    int i, j;
    unsigned short tmp[AP_LENGTH], sum[AP_LENGTH];

    for (i = 0; i < AP_LENGTH; i++) {
	sum[i] = 0;
	tmp[i] = 0;
    }

    for (i = 0; i < AP_LENGTH; i++) {
	unsigned long aa, bb, cc;

	aa = a[i];
	for (j = 0; j < AP_LENGTH; j++) {
	    int k;

	    k = i + j;
	    if (k >= AP_LENGTH)
		break;

	    bb = b[j];
	    cc = aa * bb;
	    tmp[k] = cc & 0xffff;
	    if (k + 1 < AP_LENGTH)
		tmp[k + 1] = cc >> 16;

	    vli_add (sum, tmp, c);
	    vli_asign (sum, c);

	    tmp[k] = 0;
	    if (k + 1 < AP_LENGTH)
		tmp[k + 1] = 0;
	}
    }

    return;

}


#ifdef ENCRYPT_TEST

/*************************************************************************
Test program
*/

main (int argc, char **argv)
{
    char buf[120];
    char ret_str[24];
    int ret, i, cnt;
    char *str;

    while (1) {

/*
   printf ("Please type in a string: ");
   scanf ("%s", buf);    
   strcpy (buf, "fdgh"); 
*/
	str = buf;

	/* testing getpass */
/*
   str = (char *)getpass("Please type in a string: ");
   printf("Pass: %s\n", str);
 */

	/* testing RMT_ask_password */
/*	RMT_ask_password ("Please type in a string: ", 12, buf); */
	buf = (char *)getpass ("Please type in a string: ");
	str = buf;
	printf ("Pass: %s\n", str);

	ret = ENCR_encrypt (str, 24, ret_str);

	printf ("ret = %d\n", ret);
	for (i = 0; i < ret; i++)
	    printf ("%d ", ret_str[i]);
	printf ("\n");

    }
}

/*************************************************************************
print vli for testing
*/

int 
print_vli (vli_type a)
{
    int i;

    for (i = 0; i < AP_LENGTH; i++)
	printf ("%d  ", a[i]);
    printf ("\n");

    return (0);
}

#endif
