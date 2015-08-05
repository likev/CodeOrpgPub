/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/15 19:36:08 $
 * $Id: smipp_parse.c,v 1.4 2005/04/15 19:36:08 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h> 

#include <smi.h>
#include "smipp_def.h"


#define EXTRA_SIZE 4096

static Smi_struct_t *Smis = NULL;	/* list of SMI structures */

static Smi_vss_info_t *Vss = NULL;	/* list of SMI VSS comment */

static Smi_vss_info_t *Prev_vss = NULL;	/* previous incomplete vss */

static Smi_vss_size_t *Vsize = NULL;	/* list of SMI_vss_size comment */

extern int SMIM_debug;
extern char Prog_name[];

#define LINE_LIST_SIZE 256

struct Smi_file_info {		/* source file info */
    int token[LINE_LIST_SIZE];	/* ending token index per line. The first is 
				   the end token index of line 0 */
    char *name;			/* file name */
    int st_line;		/* starting line number */
    int line_cnt;		/* size of token */
    struct Smi_file_info *next;	/* the next entry of the linked list */
};

typedef struct Smi_file_info Smi_file_info_t;

static char Smi_function[MIPP_NAME_SIZE] = "SMI_get_info";


static Smi_file_info_t *Files = NULL;	/* list of source files */
static Smi_file_info_t *Cur_file = NULL;/* current source files */
static int Cur_line = 0;		/* current source line */

static int Realloc_buf (char **buffer, int size, int inc, char **p);
static int Extract_smi_comments (char *st, char *end, int tkcnt);
static void Print_vss (char *outbuf, int *tks);
static void Process_file_name (char *ip, char *end);
static void Add_file_name (Smi_file_info_t *f);
static void Process_end_line (int tkcnt);
static int Check_and_replace_this (char *str);
static void Print_error (va_list args, const char *format, int tk);


/**************************************************************************

    Returns the SMI function name.

**************************************************************************/

char *SMIP_get_function_name () {
    return (Smi_function);
}

/**************************************************************************

    Returns the SMI struct names list.

**************************************************************************/

Smi_struct_t *SMIP_get_smis () {
    return (Smis);
}

/**************************************************************************

    Returns the vss struct corresponding to token "tk" or NULL if not found.

**************************************************************************/

Smi_vss_info_t *SMIP_get_vss (int tk) {
    Smi_vss_info_t *s;

    s = Vss;
    while (s != NULL) {
	int diff;
	diff = s->token - tk;
	if (diff >= -1 && diff <= 1)
	    return (s);
	s = s->next;
    }
    return (NULL);
}

/**************************************************************************

    Returns the vsize code defined between tokens "st_tk" and "end_tk" or 
    NULL if not found.

**************************************************************************/

Smi_vss_size_t *SMIP_get_vsize (int st_tk, int end_tk) {
    Smi_vss_size_t *s;

    s = Vsize;
    while (s != NULL) {
	if (s->token >= st_tk && s->token <= end_tk)
	    return (s);
	s = s->next;
    }
    return (NULL);
}

/**************************************************************************

    Tokenizes the text in "inbuf" of "size" bytes. The output is put in
    outbuf which start with size of "size". Returns the size of the 
    tokenized text. Tokens are separated by term_ch. term_ch must be '\0' 
    for other features (e.g. two-token) to work. The byte in front of the
    first character in a token is set to ' ' if the token is separated
    from its previous token in the original text. Two word tokens
    "struct " are processed. Tokens are separated by
    ' ', '\t' or '\n'. outbuf is always returned. The caller must free
    it. The token index (offset in outbuf) table, tks and tkcnt, are
    returned if tks_p != NULL. If returned, the caller must free it.
    Characters from any '#" until next line return are removed. Chars
    '@', '/' and '_' are treated as mormal alpha-numerical chars. Other
    chars are assumed to be single char symbals. Line return is added
    after each ';term' for easier display of outbuf in an editor. Comment
    text are removed. SMI comments are processed. Duplicated ';' (empty
    C statements) are removed.

**************************************************************************/

#define CHECK_STEP 100

int SMIP_tokenize_text (char *inbuf, int size, 
			char **outbuf_p, int **tks_p, int *tkcnt_p) {
    enum {PR_NON, PR_STRUCT};
    char *ip, *op, *end, *next, *outbuf, c, term_ch, *check;
    int *tks;
    int intkn, incommon, bsize, delete;
    int tkcnt, tksize, pr;

    ip = inbuf;
    check = ip;
    end = inbuf + size;
    outbuf = NULL;
    tks = NULL;
    op = outbuf;
    intkn = 0;
    bsize = tksize = 0;
    term_ch = '\0';
    delete = 0;
    tkcnt = 0;
    pr = PR_NON;
    incommon = 0;
    while (ip < end) {

	if (ip >= check) {
	    if (op - outbuf + EXTRA_SIZE >= bsize)
		bsize = Realloc_buf (&outbuf, bsize, size + EXTRA_SIZE, &op);
	    if (tkcnt + CHECK_STEP >= tksize)
		tksize = (Realloc_buf ((char **)&tks, tkcnt * sizeof (int), 
			(size / 8 + CHECK_STEP) * sizeof (int), NULL)) 
			/ sizeof (int);
	    check += CHECK_STEP;
	}

	c = *ip;
	next = ip + 1;
	if (c == '#') {			/* delete the line after # */
	    if (ip == inbuf || 
			(ip[-1] == '\n' && ip + 1 < end && ip[1] == ' '))
		Process_file_name (ip, end);
	    delete = 1;
	}
	if (c == '\n') {
	    if (incommon == 2)
		incommon = 0;
	    if (delete) {
		delete = 0;
		ip++;
		continue;
	    }
	}
	if (c == '\n')
	    Process_end_line (tkcnt - 1);
	if (c == '/' && next < end && (*next == '*' || *next == '/')) {
	    if (!incommon) {
		if (*next == '/')
		    incommon = 2;
		else
		    incommon = 1;
		ip += 2;
		continue;
	    }
	}
	if (c == '*' && next < end  && *next == '/') {
	    if (incommon == 1) {
		incommon = 0;
		ip += 2;
		continue;
	    }
	}
	if (incommon &&
		c == 'S' && ip + 3 < end && strncmp (ip, "SMI_", 4) == 0 &&
			SMIP_is_separator (ip[-1])) {
	    ip += Extract_smi_comments (ip, end, tkcnt - 1);
	    continue;
	}
	if (delete || incommon) {
	    ip++;
	    continue;
	}

	if (SMIP_is_alpha (c)) {
	    if (!intkn) {
		intkn = 1;
		if (pr == PR_STRUCT)
		    op[-1] = ' ';
		else {
		    if (ip > inbuf && (ip[-1] == ' ' || ip[-1] == '\t'))
			*op++ = ' ';
		    tks[tkcnt++] = op - outbuf;
		}
	    }
	    *op++ = c;
	    ip++;
	    continue;
	}

	if (c == ' ' || c == '\n' || c == '\t') {
	    if (intkn) {
		*op++ = term_ch;
		intkn = 0;
		pr = PR_NON;
		if (strcmp (outbuf + tks[tkcnt - 1], "struct") == 0)
		    pr = PR_STRUCT;
	    }
	    ip++;
	    continue;
	}

	/* other single char symbals */
	if (c == ';' && tkcnt > 0 && *(outbuf + tks[tkcnt - 1]) == ';') {
	    ip++;			/* discard extra ';' */
	    continue;
	}
	if (c == ';' && Prev_vss != NULL) {
	    Prev_vss->token = tkcnt;
	    Prev_vss = NULL;
	}
	if (intkn)
	    *op++ = term_ch;
	if (ip > inbuf && (ip[-1] == ' ' || ip[-1] == '\t'))
	    *op++ = ' ';
	tks[tkcnt++] = op - outbuf;
	*op++ = c;
	*op++ = term_ch;
	intkn = 0;
	pr = PR_NON;
	ip++;
    }
    Process_end_line (tkcnt - 1);
			/* needed if the last line is not terminated */

    if (SMIM_debug)
	Print_vss (outbuf, tks);

    if (tks_p != NULL) {
	*tks_p = tks;
	*tkcnt_p = tkcnt;
    }
    else
	free (tks);

    *outbuf_p = outbuf;
    return (op - outbuf);
}

/**************************************************************************

    Prints VSS list for debugging purpose.

**************************************************************************/

static void Print_vss (char *outbuf, int *tks) {
    Smi_vss_info_t *s = Vss;
    while (s != NULL) {
	printf ("    VSS: token %d, size %s, ", s->token, s->size);
	if (s->offset != NULL)
	    printf ("offset %s, ", s->offset);
	printf (" %s, %s-\n", outbuf + tks[s->token - 1], 
					outbuf + tks[s->token]);
	s = s->next;
    }
}

/**************************************************************************

    Returns non-zero if "c" is an alpha numerical character. Zero otherwise.

**************************************************************************/

int SMIP_is_alpha (char c) {
    if ((c >= '0' && c <= '9') || (c >= '@' && c <= 'Z') ||
	    c == '_' || (c >= 'a' && c <= 'z') || c == '/')
	return (1);
    return (0);
}

/**************************************************************************

    Returns non-zero if "c" is a separation character. Zero otherwise.

**************************************************************************/

int SMIP_is_separator (char c) {
    if (c == ' ' || c == '\n' || c == '\t')
	return (1);
    return (0);
}

/**************************************************************************

    Adds a new entry to the SMI struct list.

**************************************************************************/

void SMIP_add_new_smi_struct (Smi_struct_t *smi) {

    if (SMIM_debug)
	printf ("SMI_struct: %s-\n", smi->name);

    smi->real_name = NULL;
    smi->next = NULL;
    if (Smis == NULL)
	Smis = smi;
    else {
	Smi_struct_t *s = Smis;
	while (1) {
	    if (strcmp (smi->name, s->name) == 0) {	/* duplicated name  */
		while (1) {
		    if (smi->major == NULL && s->major == NULL)
			return;
		    if (smi->major != NULL && s->major != NULL &&
			strcmp (smi->major, s->major) == 0) {
			if (smi->minor == NULL && s->minor == NULL)
			    return;
			if (smi->minor != NULL && s->minor != NULL &&
			    strcmp (smi->minor, s->minor) == 0)
			    return;
		    }
		    if (s->next_id == NULL)
			break;
		    s = s->next_id;
		}
		s->next_id = smi;		/* add to the linked list */
		return;
	    }
	    if (s->next != NULL)
		s = s->next;
	    else
		break;
	}
	s->next = smi;
    }
}

/**************************************************************************

    Extracts and parses the SMI comments. Detailed syntax error checking
    is needed because SMI comments are not checked by the compiler. "st"
    is the text starting pointer and "end" points the end of the text.
    The function returns the number of characters parsed.
    Tag SMI_vss_field must have at least 5 chars for uncommenting the
    field type and name to work. See the code.

**************************************************************************/

static int Extract_smi_comments (char *st, char *end, int tkcnt) {
    char *tag, c;
    int tag_len;

    tag = "SMI_struct";
    tag_len = strlen (tag);
    if (st + tag_len < end && strncmp (st, tag, tag_len) == 0 &&
				SMIP_is_separator (*(st + tag_len))) {
	Smi_struct_t *smi;
	char *tk[8], c;
	int tk_len[8], n_tks, size, n_words, next_tk, dot, i;

	c = *(end - 1);
	*(end - 1) = '\0';
	n_tks = SMIP_get_next_tokens (8, st, tk, tk_len);
	*(end - 1) = c;
	size = 0;
	n_words = 0;
	dot = 0;
	for (i = 1; i < n_tks; i++) {
	    if (*(tk[i]) == ';')
		break;
	    if (*(tk[i]) == '.') {
		if (i == 3) {
		    dot = 1;
		    continue;
		}
		break;
	    }
	    if (!SMIP_is_alpha (*(tk[i])))
		break;
	    size += tk_len[i] + 1;
	    n_words++;
	    if (tk_len[i] == 6 && strncmp (tk[i], "struct", 6) == 0)
		n_words--;
	}
	if ((!dot && (n_words < 1 || n_words > 2)) || 
						(dot && n_words != 3))
	    SMIP_error (-2, "SMI SMI_struct syntax error\n");

	smi = (Smi_struct_t *)SMIM_malloc (sizeof (Smi_struct_t) + size);
	smi->name = (char *)smi + sizeof (Smi_struct_t);
	memcpy (smi->name, tk[1], tk_len[1]);
	smi->name[tk_len[1]] = '\0';
	next_tk = 2;
	if (strcmp (smi->name, "struct") == 0) {
	    int cnt = tk_len[1];
	    smi->name[cnt] = ' ';
	    cnt++;
	    memcpy (smi->name + cnt, tk[2], tk_len[2]);
	    cnt += tk_len[2];
	    smi->name[cnt] = '\0';
	    next_tk++;
	}
	if (n_words > 1) {
	    smi->major = (char *)smi->name + strlen (smi->name) + 1;
	    memcpy (smi->major, tk[next_tk], tk_len[next_tk]);
	    smi->major[tk_len[next_tk]] = '\0';
	    next_tk++;
	}
	else
	    smi->major = NULL;
	if (dot) {
	    next_tk++;
	    smi->minor = (char *)smi->major + strlen (smi->major) + 1;
	    memcpy (smi->minor, tk[next_tk], tk_len[next_tk]);
	    smi->minor[tk_len[next_tk]] = '\0';
	}
	else
	    smi->minor = NULL;
	smi->next_id = NULL;
	SMIP_add_new_smi_struct (smi);
	return (tag_len);
    }

    tag = "SMI_vss_field"; 
    tag_len = strlen (tag);
    if (st + tag_len < end && strncmp (st, tag, tag_len) == 0 &&
		(SMIP_is_separator ((c = *(st + tag_len))) || c == '[')) {
	int size_st, size_end, off_st, off_end;
	Smi_vss_info_t *vss;
	char *p, *e;
	int cnt, size;

	p = st + tag_len;
	e = p + 256;		/* only goes this far */
	if (e > end)
	    e = end;
	size_st = size_end = -1;
	off_st = off_end = -1;
	cnt = 0;		/* how many chars before size field */
	while (p < e) {		/* locate the size field */
	    if (*p == '*' && p + 1 < e && *(p + 1) == '/')
		break;		/* end of comment */
	    if (*p == '[') {
		if (size_st < 0)
		    size_st = p - st;
		else		/* error */
		    break;
	    }
	    if (*p == ']') {
		size_end = p - st;
		break;
	    }
	    if (SMIP_is_alpha (*p) && size_st < 0)
		cnt++;
	    p++;
	}
	if (size_st < 0 || size_end < 0)
	    SMIP_error (-2, 
		"SMI VSS size syntax error in this or next lines\n");

	if (cnt > 0) {		/* long SMI_vss_field - look for offset */
	    int level;

	    off_st = off_end = -1;
	    level = 0;
	    while (p < e) {		/* locate the offset field */
		if (*p == '*' && p + 1 < e && *(p + 1) == '/')
		    break;		/* end of comment */
		if (*p == '(') {
		    level++;
		    if (off_st < 0)
			off_st = p - st;
		}
		if (*p == ')') {
		    level--;
		    if (off_st >= 0 && level == 0) {
			off_end = p - st;
			break;		/* done */
		    }
		    if (level == 0)
			break;
		}
		p++;
	    }
	    if (off_st < 0 || off_end < 0)
		SMIP_error (-2, 
		"SMI VSS offset syntax error in this or next lines\n");
	}

	size = sizeof (Smi_vss_info_t) + (size_end - size_st);
	if (cnt > 0)
	    size += off_end - off_st;

	vss = (Smi_vss_info_t *)SMIM_malloc (sizeof (Smi_vss_info_t) + size);
	vss->size = (char *)vss + sizeof (Smi_vss_info_t);
	if (cnt == 0) {
	    vss->token = tkcnt;
	    vss->offset = NULL;
	    Prev_vss = NULL;
	}
	else {
	    char *p1;

	    vss->offset = (char *)vss->size + (size_end - size_st);
	    memcpy (vss->offset, st + off_st + 1, off_end - off_st - 1);
	    vss->offset[off_end - off_st - 1] = '\0';
	    Prev_vss = vss;

	    p = st;		/* make type and name seen outside comment */
	    *p++ = '*';		/* tag_len must >= 5 for this to work */
	    *p++ = '/';
	    p1 = st + tag_len;
	    while (p1 < st + size_st) {
		*p = *p1;
		p++; p1++;
	    } 
	    *p++ = ';';
	    *p++ = '/';
	    *p++ = '*';
	}

	memcpy (vss->size, st + size_st + 1, size_end - size_st - 1);
	vss->size[size_end - size_st - 1] = '\0';

	vss->need_data = 0;
	if (Check_and_replace_this (vss->offset) > 0)
	    vss->need_data = 1;
	if (Check_and_replace_this (vss->size) > 0)
	    vss->need_data = 1;

	vss->next = NULL;
	if (Vss == NULL)
	    Vss = vss;
	else {
	    Smi_vss_info_t *s = Vss;
	    while (1) {
		if (s->next == NULL)
		    break;
		s = s->next;
	    }
	    s->next = vss;
	}

	if (cnt == 0)
	    return (tag_len);
	else
	    return (0);
    }

    tag = "SMI_vss_size"; 
    tag_len = strlen (tag);
    if (st + tag_len < end && strncmp (st, tag, tag_len) == 0 &&
				SMIP_is_separator (*(st + tag_len))) {
	int s_st, s_end;
	Smi_vss_size_t *vsize;
	char *p, *e;
	int size;

	p = st + tag_len;
	e = p + 256;		/* only goes this far */
	if (e > end)
	    e = end;
	s_st = s_end = -1;
	while (p < e) {		/* locate the size field */
	    if (*p == '*' && p + 1 < e && *(p + 1) == '/')
		break;		/* end of comment */
	    if (s_st < 0)
		s_st = p - st;
	    if (*p == ';') {
		s_end = p - st;
		break;
	    }
	    p++;
	}
	if (s_st < 0 || s_end < 0)
	    SMIP_error (-2, 
		"SMI VSS size syntax error in this or next lines\n");

	size = sizeof (Smi_vss_size_t) + (s_end - s_st);

	vsize = (Smi_vss_size_t *)SMIM_malloc (sizeof (Smi_vss_size_t) + size);
	vsize->size = (char *)vsize + sizeof (Smi_vss_size_t);
	memcpy (vsize->size, st + s_st + 1, s_end - s_st - 1);
	vsize->size[s_end - s_st - 1] = '\0';
	vsize->token = tkcnt;

	vsize->need_data = 0;
	if (Check_and_replace_this (vsize->size) > 0)
	    vsize->need_data = 1;

	vsize->next = NULL;
	if (Vsize == NULL)
	    Vsize = vsize;
	else {
	    Smi_vss_size_t *s = Vsize;
	    while (1) {
		if (s->next == NULL)
		    break;
		s = s->next;
	    }
	    s->next = vsize;
	}
	return (tag_len);
    }

    tag = "SMI_function";
    tag_len = strlen (tag);
    if (st + tag_len < end && strncmp (st, tag, tag_len) == 0 &&
				SMIP_is_separator (*(st + tag_len))) {
	char *tk[3], c;
	int tk_len[3], n_tks;

	c = *(end - 1);
	*(end - 1) = '\0';
	n_tks = SMIP_get_next_tokens (3, st, tk, tk_len);
	*(end - 1) = c;
	if (n_tks < 3 || !SMIP_is_alpha (*(tk[1])) || 
		*(tk[2]) != ';' || tk_len[1] >= MIPP_NAME_SIZE)
	    SMIP_error (-2, "SMI SMI_function syntax error\n");
	if (strcmp (Smi_function, "SMI_get_info") != 0)
	    SMIP_error (-2, "SMI multiple SMI_functions specified\n");
	memcpy (Smi_function, tk[1], tk_len[1]);
	Smi_function[tk_len[1]] = '\0';
	return (tag_len);
    }
    return (1);
}

/**************************************************************************

    Frees the lists used in this module.

**************************************************************************/

void SMIP_free () {
    Smi_struct_t *s;
    Smi_vss_info_t *vss;
    Smi_file_info_t *f;

    s = Smis;
    while (s != NULL) {
	Smi_struct_t *next = s->next;
	if (s->real_name != NULL)
	    free (s->real_name);
	free (s);
	s = next;
    }
    Smis = NULL;

    vss = Vss;
    while (vss != NULL) {
	Smi_vss_info_t *next = vss->next;
	free (vss);
	vss = next;
    }
    Vss = NULL;

    f = Files;
    while (f != NULL) {
	Smi_file_info_t *next = f->next;
	free (f);
	f = next;
    }
    Files = NULL;
}

/**************************************************************************

    Extend buffer "buffer" od size "size" by "inc". Pointer "*p" is 
    updated. Returns the new size.

**************************************************************************/

static int Realloc_buf (char **buffer, int size, int inc, char **p) {
    int new_size, off;
    char *buf;

    new_size = size + inc;
    if (p != NULL)
	off = *p - *buffer;
    buf = SMIM_malloc (new_size);
    memcpy (buf, *buffer, size);
    if (p != NULL)
	*p = buf + off;
    if (*buffer != NULL)
	free (*buffer);

    *buffer = buf;
    return (new_size);
}

/**************************************************************************

    Prints an error message and return. Source file name and line number
    are provided in token index, "tk", in known. If tk == -2, prints the
    latest line and the current source file name.

**************************************************************************/

void SMIP_warning (int tk, const char *format, ...) {
    va_list args;

    va_start (args, format);
    Print_error (args, format, tk);
    return;
}

/**************************************************************************

    Prints an error message and terminate. Source file name and line number
    are provided in token index, "tk", in known. If tk == -2, prints the
    latest line and the current source file name.

**************************************************************************/

void SMIP_error (int tk, const char *format, ...) {
    va_list args;

    va_start (args, format);
    Print_error (args, format, tk);
    exit (1);
}

/**************************************************************************

    Prints an error message. Source file name and line number
    are provided in token index, "tk", in known. If tk == -2, prints the
    latest line and the current source file name.

**************************************************************************/

static void Print_error (va_list args, const char *format, int tk) {

    if (tk >= 0 || tk == -2) {
	char *name;
	int l = SMIP_get_source_name (tk, &name);
	fprintf (stderr, "%s: %s:%d: ", Prog_name, name, l);
    }
    else
	fprintf (stderr, "%s: ", Prog_name);

    if (format != NULL && *format != '\0')
	vfprintf (stderr, format, args);
    va_end (args);
    return;
}

/**************************************************************************

    Starts a new file - get file name and starting line number.

**************************************************************************/

static void Process_file_name (char *ip, char *end) {
    Smi_file_info_t *f;
    int line, st_off, end_off;
    char *p;

    if (sscanf (ip + 2, "%d", &line) != 1)
	return;
    p = ip + 2;
    st_off = end_off = -1;
    while (p < end && !SMIP_is_separator (*p))
	p++;
    while (p < end) {
	if (*p == '\n')
	    break;
	if (*p == '"') {
	    if (st_off < 0)
		st_off = p - ip;
	    else if (st_off >= 0) {
		end_off = p - ip;
		break;
	    }
	}
	p++;	
    }
    if (st_off < 0 || end_off < 0)
	return;
    f = (Smi_file_info_t *)SMIM_malloc (sizeof (Smi_file_info_t) + 
						end_off - st_off);
    f->name = (char *)f + sizeof (Smi_file_info_t);
    memcpy (f->name, ip + st_off + 1, end_off - st_off - 1);
    f->name[end_off - st_off - 1] = '\0';
    f->st_line = line;
    f->line_cnt = 0;
    Cur_file = f;
    Cur_line = 0;
    Add_file_name (f);
}

/**************************************************************************

    Adds a new item to the source file list.

**************************************************************************/

static void Add_file_name (Smi_file_info_t *f) {

    f->next = NULL;
    if (Files == NULL)
	Files = f;
    else {
	Smi_file_info_t *t = Files;
	while (1) {
	    if (t->next != NULL)
		t = t->next;
	    else
		break;
	}
	t->next = f;
    }
}

/**************************************************************************

    Keeps track of source file line numbers.

**************************************************************************/

static void Process_end_line (int tkcnt) {

    if (Cur_line >= LINE_LIST_SIZE) {
	Smi_file_info_t *f;

	f = (Smi_file_info_t *)SMIM_malloc (sizeof (Smi_file_info_t) + 
						strlen (Cur_file->name) + 1);
	f->name = (char *)f + sizeof (Smi_file_info_t);
	strcpy (f->name, Cur_file->name);
	f->st_line = Cur_file->st_line + LINE_LIST_SIZE;
	f->line_cnt = 0;
	Cur_file = f;
	Cur_line = 0;
	Add_file_name (f);
    }
    Cur_file->token[Cur_line] = tkcnt;
    Cur_line++;
    Cur_file->line_cnt = Cur_line;
}

/**************************************************************************

    Returns the source file name and line number for token index "tk".
    The return value is the line number. If tk == -2, returns the latest
    source line.

**************************************************************************/

int SMIP_get_source_name (int tk, char **name) {
    Smi_file_info_t *f, *last;
    int last_cnt;

    f = Files;
    last_cnt = -1;
    while (f != NULL) {
	int i;
	if (f->line_cnt > 0) {
	    last = f;
	    last_cnt = f->line_cnt - 1;
	}
	for (i = 0; i < f->line_cnt; i++) {
	    if ((tk >= 0 && f->token[i] >= tk) || 
		(tk < 0 && i == f->line_cnt - 1 && f->next == NULL)) {
		*name = f->name;
		return (f->st_line + i);
	    }
	}
	f = f->next;
    }

    if (last_cnt >= 0) {
	*name = last->name;
	return (last->st_line + last_cnt);
    }
    else {
	*name = "";			/* not found */
	return (0);
    }
}

/**************************************************************************

    Checks if string contains "str->". Replaces all such "this" by "data".
    Return 1 if found or 0 otherwise.

**************************************************************************/

static int Check_and_replace_this (char *str) {
    char *tk[32];
    int tk_len[32], found, cnt, i;

    if (str == NULL)
	return (0);
    found = 0;
    cnt = SMIP_get_next_tokens (32, str, tk, tk_len);
    for (i = 0; i < cnt; i++) {
	if (tk_len[i] == 4 && strncmp (tk[i], "this", 4) == 0 &&
		i + 2 < cnt && *(tk[i + 1]) == '-' && *(tk[i + 2]) == '>') {
	    found = 1;
	    memcpy (tk[i], "data", 4);
	}
    }
    return (found);
}

/**************************************************************************

    Find out "max_n" tokens starting at "st" which is a NULL terminated
    char string. The tokens are returned in "tk" and their sizes in 
    "tk_len". The function returns the number of tokens found.

**************************************************************************/

int SMIP_get_next_tokens (int max_n, char *st, char **tk, int *tk_len) {
    char *ip;
    int cnt, intkn;

    ip = st;
    cnt = 0;
    intkn = 0;
    while (1) {
	char c;

	c = *ip;
	if (SMIP_is_alpha (c)) {
	    if (!intkn) {
		intkn = 1;
		tk[cnt] = ip;
	    }
	    ip++;
	    continue;
	}

	if (intkn) {
	    intkn = 0;
	    tk_len[cnt] = ip - tk[cnt];
	    cnt++;
	    if (cnt >= max_n)
		return (cnt);
	}

	if (SMIP_is_separator (c)) {
	    ip++;
	    continue;
	}

	/* other single char symbals */
	if (c == '\0')
	    break;
	tk[cnt] = ip;
	tk_len[cnt] = 1;
	cnt++;
	if (cnt >= max_n)
	    return (cnt);
	ip++;
    }
    return (cnt);
}
