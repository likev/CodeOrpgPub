
/******************************************************************

    This is a tool that generate comms device configuration file 
    for specified radar site. This is the module that processes 
    expressions.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/01/03 19:23:57 $
 * $Id: gdc_expression.c,v 1.1 2011/01/03 19:23:57 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gdc_def.h"

static char Field_delimiter = '.';

static char *Process_expression (char *inp, int size, int arith, char *outbuf);
static char *Process_funcs (char *inp, int size, char *outbuf);
static int Arithmetic_cal (char *inp, int *out);
static char *Get_next_value (char *inp, int *v);
static char *Get_next_symbol (char *inp, char *s);
static int Get_parameters (char *st, int size, int n_ps, char **pp, int *len);
static int Is_char (char c);
static void Get_netmask (int n_bits, char *buf);


/**********************************************************************

    Sets Field_delimiter. Sets to the default if delimiter is '\0'.

**********************************************************************/

void GDCE_set_field_delimiter (char delimiter) {
    if (delimiter == '\0')
	Field_delimiter = '.';
    else
	Field_delimiter = delimiter;
}

/**********************************************************************

    Returns Field_delimiter.

**********************************************************************/

char GDCE_get_field_delimiter () {
    return (Field_delimiter);
}

/**********************************************************************

    Processes expression pointed to by "inp" of "size" bytes and puts
    the result in "outbuf" of size MAX_STR_SIZE. If the output is too
    large, a buffer for output is malloced and the caller must free it.
    It first processes function calls and then resolves variable values.
    Returns the pointer to the output. No static variables can be used
    here so recursive calls are allowed. If "arith" is non-zero, the
    expression is evaluated as an integer arithmetic expression. 

**********************************************************************/

char *GDCE_process_expression (char *inp, int size, int arith, char *outbuf) {
    return (Process_expression (inp, size, arith, outbuf));
}

static char *Process_expression (char *inp, int size, int arith, char *outbuf) {
    char *buf, *out, *in, *r_buf;

    in = inp;
    buf = NULL;
    while (1) {			/* process all brackets on the same level */
	char *p, *pe, *bst, *bend, *name, *rpst, tbuf[MAX_STR_SIZE];
	int cnt, rplen, name_len;

	p = in;
	pe = in + size;
	bst = bend = name = NULL;
	cnt = name_len = 0;
	while (p < pe) {		/* find bst, bend, and name */
	    if (*p == '(') {
		if (bst == NULL)	/* starting bracket */
		    bst = p;
		else
		    cnt++;
	    }
	    else if (*p == ')') {
		if (cnt == 0) {
		    bend = p;		/* matched ending bracket */
		    break;
		}
		cnt--;
	    }
	    p++;
	}
	if (bst != NULL) {
	    if (bend == NULL)
		GDCP_exception_exit ("Unclosed (\n");
	    p = bst - 1;
	    while (p >= in && (*p == ' ' || *p == '\t')) 
		p--;
	    while (p >= in && Is_char (*p)) {
		p--;
		name_len++;
	    }
	    p++;
	    if (name_len > 0)
		name = p;		/* pointer to the function name */
	}
    
	out = rpst = NULL;
	rplen = 0;
	if (name != NULL) {
	    rpst = name;		/* replacement start */
	    rplen = bend - name + 1;	/* length of replacement */
	    out = Process_funcs (rpst, rplen, tbuf);
	}
/*	if (out == NULL && bst != NULL) { brackets processed for non-arithmetic expressions */	/* not a function */
	if (arith && out == NULL && bst != NULL) {	/* not a function */
	    rpst = bst;
	    rplen = bend - bst + 1;
	    out = Process_expression (bst + 1, bend - bst - 1, arith, tbuf);
	}
    
	if (out != NULL) {	/* function or bracketed section processed */
	    int ll, os, s;

	    if (buf != inp && buf != outbuf)
		free (buf);
	    ll = strlen (out);
	    os = (size - rplen) + ll;
	    if (os < MAX_STR_SIZE)
		buf = outbuf;
	    else
		buf = MISC_malloc (os + 1);
	    memmove (buf, in, rpst - in);
	    s = rpst - in;
	    memmove (buf + s, out, ll);
	    s += ll;
	    memmove (buf + s, rpst + rplen, os - s);
	    buf[os] = '\0';
	    if (out != tbuf)
		free (out);
	}
	else
	    break;

	in = buf;
	size = strlen (buf);
    }

    if (buf == NULL) {		/* no bracket found */
	if (size < MAX_STR_SIZE)
	    buf = outbuf;
	else
	    buf = MISC_malloc (size + 1);
	memcpy (buf, inp, size);
	buf[size] = '\0';
    }

    out = GDCP_evaluate_variables (buf);
    if (out == NULL)
	r_buf = buf;
    else {
	if (buf != outbuf)
	    free (buf);
	r_buf = out;
    }

    if (arith) {
	int result = 0;
	if (Arithmetic_cal (r_buf, &result) < 0)
	    GDCP_exception_exit ("Bad arithmetic expression (%s)\n", r_buf);
	if (r_buf != outbuf)
	    free (out);
	sprintf (outbuf, "%d", result);
	return (outbuf);
    }
    else
	return (r_buf);
}

/****************************************************************************

    Returns 1 if "str" is a legal identifier or 0 otherwise.

****************************************************************************/

int GDCE_is_identifier (char *str, int n_bytes) {
    int i;
    if (n_bytes == 0)
	n_bytes = strlen (str);
    for (i = 0; i < n_bytes; i++) {
	if (!Is_char (str[i]))
	    return (0);
    }
    return (1);
}

/****************************************************************************

    Returns 1 if c is a English char or a number or "_" or 0 otherwise.

****************************************************************************/

static int Is_char (char c) {
    if ((c <= 57 && c >= 48) || (c <= 90 && c >= 65) || (c <= 122 && c >= 97) || c == '_' || c == '-')
	return (1);
    return (0);
}

/****************************************************************************

    Gets the next value in text "inp" and returns it with "v". Returns the 
    pointer to after the used chars on success or NULL on failure.

****************************************************************************/

static char *Get_next_value (char *inp, int *v) {
    char b[64], *p;

    if (sscanf (inp, "%d", v) != 1)
	return (NULL);
    sprintf (b, "%d", *v);
    p = strstr (inp, b);
    if (p == NULL)
	return (NULL);
    return (inp + (p - inp) + strlen (b));
}

/****************************************************************************

    Gets the next symbal in text "inp" and returns it with "s". Returns the 
    pointer to after the used chars on success or NULL on failure.

****************************************************************************/

static char *Get_next_symbol (char *inp, char *s) {

    char *p = inp + MISC_char_cnt (inp, " \t");
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '\0') {
	*s = *p;
	return (p + 1);
    }
    return (NULL);
}

/****************************************************************************

    Calculates integer arithmetic expression "inp" and returns the result in
    "out". Returns 0 on success or -1 on failure.

****************************************************************************/

static int Arithmetic_cal (char *inp, int *out) {
    int accum, cr_term, v;
    char s, *p;

    accum = 0;
    cr_term = 1;
    p = inp;
/*    p = Get_next_symbol (p, &s);
    if (p != NULL) {
	if (s != '-')
	    return (-1);
	cr_term = -1;
    }
*/
    p = Get_next_value (p, &v);
    if (p == NULL)
	return (-1);
    cr_term *= v;
    while (1) {

	p = Get_next_symbol (p, &s);
	if (p == NULL)		/* error */
	    return (-1);
	if (s == '\0')		/* end of inp */
	    break;
	p = Get_next_value (p, &v);
	if (p == NULL)
	    return (-1);
	if (s == '+') {
	    accum += cr_term;
	    cr_term = v;
	}
	else if (s == '-') {
	    accum += cr_term;
	    cr_term = -v;
	}
	else if (s == '*')
	    cr_term *= v;
	else if (s == '/') {
	    if (v == 0) {
		fprintf (stderr, "Devided by 0\n");
		return (-1);
	    }
	    cr_term /= v;
	}
	else
	    return (-1);
    }
    accum += cr_term;
    *out = accum;
    return (0);
}

/**********************************************************************

    Processes function pointed to by "inp" of "size" bytes and puts
    the result in "outbuf" of size MAX_STR_SIZE. If the output is too
    large, a buffer for output is malloced and the caller must free it.
    Returns the pointer to the output or NULL if this is not a defined
    funcion. No static variables can be used here so recursive calls 
    are allowed.

    To get an parameter, one must counter commars on the top level of 
    parances. e.g. any commar inside a paraneces is not countered. Thus
    MISC_get_token cannot be used here.

    To evaluate arithmic expression, we can extend Process_expression
    to evaluate the value after all parences are removed and variables
    are replaced by their values. We build a func to extract the next 
    symbal and the number follows it. The symbal may be missing in the
    begingning of the expression. Note that we should probably not 
    process any parences that are not functions for non-arithmis 
    expression. This will allow parences in the text. This can be 
    implemented by adding a condition in Process_expression.

**********************************************************************/

static char *Process_funcs (char *inp, int size, char *outbuf) {
    int name_len, plen;
    char *p, *e, *bst, buf1[MAX_STR_SIZE], buf2[MAX_STR_SIZE];

    p = inp;
    e = inp + size;
    name_len = -1;
    bst = NULL;
    while (p < e) {
	if (name_len < 0 && (*p == ' ' || *p == '\t' || *p == '('))
	    name_len = p - inp;
	if (*p == '(') {
	    bst = p;		/* bracket start */
	    break;
	}
	p++;
    }
    plen = (inp + size) - bst - 2;
    if (strncmp (inp, "Field", name_len) == 0) {
	int p_cnt, len[4], ind;
	char *pp[4], *p1, *p2, c, sep[8], delimiter;

	p_cnt = Get_parameters (bst + 1, plen, 4, pp, len);
	if (p_cnt != 2 && p_cnt != 3)
	    GDCP_exception_exit ("Parameter number is not 2 for function \"Field\"\n");
	p2 = Process_expression (pp[1], len[1], 1, buf2);
	if (sscanf (p2, "%d%c", &ind, &c) != 1)
	    GDCP_exception_exit (
		"Second parameter (%s) is not a number for \"Field\"\n", p2);
	if (p2 != buf2)
	    free (p2);
	delimiter = Field_delimiter;
	if (p_cnt == 3) {
	    p2 = Process_expression (pp[2], len[2], 0, buf2);
	    if (p2[0] == '"' && p2[2] == '"' && p2[3] == '\0')
		delimiter = p2[1];
	    else if (p2[1] == '\0')
		delimiter = p2[0];
	    else
	        GDCP_exception_exit (
		"Third paramter (%s) is not a character for \"Field\"\n", p2);
	    if (p2 != buf2)
		free (p2);
	}
	p1 = Process_expression (pp[0], len[0], 0, buf1);
	sprintf (sep, "S%cQ\"", delimiter);
	if (GDCM_ftoken (p1, sep, ind - 1, outbuf, MAX_STR_SIZE) <= 0) {
/*
	    GDCP_exception_exit (
		"%d-th field (delimiter %c) not found in %s\n",
						ind, delimiter, p1);
*/
	    outbuf[0] = '\0';
	}
	if (p1 != buf1)
	    free (p1);
	return (outbuf);
    }
    else if (strncmp (inp, "Arithmetic", name_len) == 0) {
	int p_cnt, len[2];
	char *pp[2], *p1, *out;

	p_cnt = Get_parameters (bst + 1, plen, 2, pp, len);
	if (p_cnt != 1)
	    GDCP_exception_exit (
		"Parameter number is not 1 for function \"Arithmetic\"\n");
	p1 = Process_expression (pp[0], len[0], 1, buf1);
	out = outbuf;
	if (strlen (p1) >= MAX_STR_SIZE)
	    out = MISC_malloc (strlen (p1) + 1);
	strcpy (out, p1);
	if (p1 != buf1)
	    free (p1);
	return (out);
    }
    else if (strncmp (inp, "Net_mask", name_len) == 0) {
	int p_cnt, len[2], n_bits;
	char *pp[2], c;

	p_cnt = Get_parameters (bst + 1, plen, 2, pp, len);
	if (p_cnt != 1)
	    GDCP_exception_exit (
		"Parameter number is not 1 for function \"Net_mask\"\n");
	p = Process_expression (pp[0], len[0], 0, buf1);
	if (sscanf (p, "%d%c", &n_bits, &c) != 1)
	    GDCP_exception_exit (
			"Unexpected paramter (%s) for \"Net_mask\"\n", p);
	Get_netmask (n_bits, outbuf);
	if (p != buf1)
	    free (p);
	return (outbuf);
    }
    else if (strncmp (inp, "Change_case", name_len) == 0) {
	int p_cnt, len[3];
	char *pp[3], *out;

	p_cnt = Get_parameters (bst + 1, plen, 3, pp, len);
	if (p_cnt != 2)
	    GDCP_exception_exit (
		"Parameter number is not 2 for function \"Change_case\"\n");
	p = Process_expression (pp[0], len[0], 0, buf1);
	out = outbuf;
	if (strlen (p) >= MAX_STR_SIZE)
	    out = MISC_malloc (strlen (p) + 1);
	strcpy (out, p);
	if (p != buf1)
	    free (p);
	p = Process_expression (pp[1], len[1], 0, buf1);
	if (strcmp (p, "upper") == 0)
	    MISC_toupper (out);
	else if (strcmp (p, "lower") == 0)
	    MISC_tolower (out);
	else
	    GDCP_exception_exit (
			"Unexpected paramter (%s) for \"Change_case\"\n", p);
	if (p != buf1)
	    free (p);
	return (out);
    }
    else if (strncmp (inp, "Data", name_len) == 0) {
	int p_cnt, len[2];
	char *pp[2], *p1, *v, *out;

	p_cnt = Get_parameters (bst + 1, plen, 2, pp, len);
	if (p_cnt != 1)
	    GDCP_exception_exit (
		"Parameter number is not 1 for function \"Data\"\n");
	p1 = Process_expression (pp[0], len[0], 0, buf1);
	v = GDCR_get_data_value (p1);
	out = outbuf;
	if (v != NULL) {
	    if (strlen (v) >= MAX_STR_SIZE)
		out = MISC_malloc (strlen (v) + 1);
	    strcpy (out, v);
	}
	else {
	    char *msg;
	    GDCR_get_error (&msg);
	    fprintf (stderr, "%s", msg);
	    GDCP_exception_exit ("Data (%s) not found\n", p1);
	}
	if (p1 != buf1)
	    free (p1);
	return (out);
    }
    else if (strncmp (inp, "Check_data", name_len) == 0) {
	int p_cnt, len[2];
	char *pp[2], *p1, *v;

	p_cnt = Get_parameters (bst + 1, plen, 2, pp, len);
	if (p_cnt != 1)
	    GDCP_exception_exit (
		"Parameter number is not 1 for function \"Check_data\"\n");
	p1 = Process_expression (pp[0], len[0], 0, buf1);
	v = GDCR_get_data_value (p1);
	if (v != NULL)
	    strcpy (outbuf, "YES");
	else {
	    char *msg;
	    if (GDCR_get_error (&msg) != GDCR_NOT_FOUND) {
		fprintf (stderr, "%s", msg);
		GDCP_exception_exit ("Data access error\n");
	    }
	    strcpy (outbuf, "NO");
	}
	if (p1 != buf1)
	    free (p1);
	return (outbuf);
    }

    return (NULL);
}

/**********************************************************************

    Constucts and returns the text form of netmask in terms of the 
    "n_bits" format netmask.

**********************************************************************/

static void Get_netmask (int n_bits, char *buf) {
    int byte_ind, bit_ind, bytes[4], i;

    for (i = 0; i < 4; i++)
	bytes[i] = 0;

    if (n_bits > 32)
	n_bits = 32;
    if (n_bits < 0)
	n_bits = 0;
    byte_ind = 0;
    bit_ind = 7;
    for (i = 0; i < n_bits; i++) {
	bytes[byte_ind] |= 1 << bit_ind;
	if (bit_ind > 0)
	    bit_ind--;
	else {
	    bit_ind = 7;
	    byte_ind++;
	}
    }
    sprintf (buf, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

/**********************************************************************

    Finds the first "n_ps" parameters in text "st" of "size" bytes. The
    pointers and sizes of the found parameters are stored in "pp" and
    "len". Returns the number of parameters found.

**********************************************************************/

static int Get_parameters (char *st, int size, int n_ps, char **pp, int *len) {
    char *p, *e;
    int cnt, n_c, in_quo;

    p = st;
    cnt = n_c = 0;
    e = st + size;
    in_quo = 0;
    while (cnt < n_ps && p < e) {
	int c, sc;

	while (*p == ' ' || *p == '\t')
	    p++;
	pp[cnt] = p;
	c = 0;
	sc = 0;
	while (p < e) {
	    if (c == 0 && *p == ',' && !in_quo) {
		n_c++;
		break;
	    }
	    if (*p == '(')
		c++;
	    else if (*p == ')')
		c--;
	    if (*p == ' ' || *p == '\t')
		sc++;
	    else
		sc = 0;
	    if (*p == '"') {
		if (in_quo)
		    in_quo = 0;
		else
		    in_quo = 1;
	    }
	    p++;
	}
	len[cnt] = p - pp[cnt] - sc;
	p++;
	cnt++;
    }
    if (cnt < n_ps && cnt < n_c + 1) {
	pp[cnt] = pp[cnt - 1];
	len[cnt] = 0;
	cnt++;
    }
    return (cnt);
}
