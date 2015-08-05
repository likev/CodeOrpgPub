
/******************************************************************

    gdc is a tool that generates device configuration file for
    specified site(s). This is the gdc module that reads data
    files. The files are in either XML or spread sheet format.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/12/11 20:07:03 $
 * $Id: gdc_read_data.c,v 1.2 2012/12/11 20:07:03 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "gdc_def.h"
#include <infr.h>

extern int VERBOSE;
extern char SRC_dir[];

enum {GDCR_XML, GDCR_SS};		/* values for Open_file_t.type */

typedef struct {
    FILE *fl;
    char *cr_line;
} Ss_file_t;

typedef struct {
    xmlNode *root;
    xmlDoc *doc;
} Xml_file_t;

typedef struct {			/* struct for open data file */
    int type;
    char *name;
    char delimiter;
    union {
	Ss_file_t ss;
	Xml_file_t xml;
    };
} Open_file_t;

#define MAX_N_FILES 64
static Open_file_t Ofs[MAX_N_FILES];	/* open data files */
static int N_ofs = 0;			/* number of open files */

#define MAX_BUF_SIZE 4096
static char Cr_delimiter = ',';
static int Cr_error = GDCR_NO_ERROR;
static char *Cr_err_msg = NULL;
static char *Multi_v = NULL;		/* for retuning multiple values */
static char *Value = NULL;		/* for retuning single value */

/* variables for recursive calls to Search_tree */
typedef struct {			/* struct for a pattern section */
    char *node;
    char *attr;
    char *value;
} Path_sec_t;
#define MAX_N_SECTIONS 32		/* max # sections in search path */
static Path_sec_t Path_secs[MAX_N_SECTIONS];
static int N_path_secs, Done, Multi_values;

static void Match_node (xmlNode *cr_node, int pat_level);
static void Search_tree (xmlNode *a_node);
static int Get_attr_value (xmlNode *node, char *attr_name, 
					char *buf, int buf_size);
static void Get_node_text (xmlNode *a_node, char *buf, int buf_size);
static char *Search_xml_value (xmlNode *root, char *d_path);
static char *Get_ss_data_value (char *file, char *path, char *full_path);
static char *Search_ss_value (char *line, int vc, int ve, int mc, char *value);
static int Parse_ss_path (char *path, int *vcp, int *vep, int *mcp, char **vp);
static char Set_get_delimiter (char *bname, char delimiter);
static char *Remove_quote (char *str);


/******************************************************************

    Resets Cr_error if get is zero. Returns the current error.

******************************************************************/

int GDCR_get_error (char **msg) {
    if (msg != NULL)
	*msg = Cr_err_msg;
    return (Cr_error);
}

/******************************************************************

    Gets the value of data variable "full_path". See "man gdc" for 
    details. Returns
    0 on success or -1 on failure (value not found).

******************************************************************/

char *GDCR_get_data_value (char *full_path) {
    int i, off;
    char *value, *p, *path, file[MAX_STR_SIZE], *v, *bn;

    Cr_error = GDCR_NO_ERROR;
    Cr_err_msg = STR_copy (Cr_err_msg, "");

    if (Value == NULL)
	Value = MISC_malloc (MAX_BUF_SIZE);

    off = GDCM_ftoken (full_path, "S>Q\"", 0, file, MAX_STR_SIZE);
    if (off <= 0 || full_path[off - 1] != '>')
	return (NULL);
    path = full_path + off;
    path = path + MISC_char_cnt (path, " \t");
    GDCM_add_dir (SRC_dir, file, MAX_STR_SIZE);
    p = file + strlen (file) - 1;
    while (p >= file && *p != '.')
	p--;
    if (*p != '.' || strcmp (p, ".xml") != 0)
	return (Get_ss_data_value (file, path, full_path));

    /* Searches and sets XML data value */
    bn = MISC_basename (file);
    for (i = 0; i < N_ofs; i++) {
	if (Ofs[i].type == GDCR_XML && strcmp (bn, Ofs[i].name) == 0)
	    break;
    }
    if (i >= N_ofs) {
	if (N_ofs >= MAX_N_FILES)
	    GDCP_exception_exit ("Too many open data files\n");

	/* parse the file and get the DOM */
	Ofs[i].xml.doc = xmlReadFile (file, NULL, 0);
	if (Ofs[i].xml.doc == NULL) {
	    char *msg = MISC_malloc (strlen (file) + 128);
	    sprintf (msg, "Could not open/parse XML file %s\n", file);
	    Cr_err_msg = STR_cat (Cr_err_msg, msg);
	    free (msg);
	    Cr_error = GDCR_FILE_MISSING;
	    return (NULL);
	}
	Ofs[i].type = GDCR_XML;
	Ofs[i].name = MISC_malloc (strlen (bn) + 1);
	strcpy (Ofs[i].name, bn);
	Ofs[i].xml.root = xmlDocGetRootElement (Ofs[i].xml.doc);
	Ofs[i].delimiter = Set_get_delimiter (bn, '\0');
	N_ofs++;
    }

    value = Search_xml_value (Ofs[i].xml.root, path);
    if (value == NULL) {
	if (Cr_error == GDCR_NOT_FOUND) {
	    char *msg = MISC_malloc (strlen (path) + strlen (file) + 128);
	    sprintf (msg, "Could not find value of %s in XML file %s\n", 
							path, file);
	    free (msg);
	    Cr_err_msg = STR_cat (Cr_err_msg, msg);
	}
	else if (Cr_error == GDCR_SYNTAX_ERROR) {
	    char *msg = MISC_malloc (strlen (path) + 128);
	    sprintf (msg, "Unexpected XML data path (%s)\n", path);
	    Cr_err_msg = STR_cat (Cr_err_msg, msg);
	    free (msg);
	}
	return (NULL);
    }

    v = GDCP_evaluate_variables (value);
    if (v != NULL) {
	GDCM_strlcpy (value, v, MAX_BUF_SIZE);
	free (v);
    }
    return (value);
}

/******************************************************************

    Sets spread sheet data variable "full_path". See "man gdc" for 
    details.

******************************************************************/

static char *Get_ss_data_value (char *file, char *path, char *full_path) {
    int i, vc, ve, mc;
    char *value, *mvalue, *v, *bn;

    bn = MISC_basename (file);
    for (i = 0; i < N_ofs; i++) {
	if (Ofs[i].type == GDCR_SS && strcmp (bn, Ofs[i].name) == 0)
	    break;
    }
    if (i >= N_ofs) {
	if (N_ofs >= MAX_N_FILES)
	    GDCP_exception_exit ("Too many open data files\n");

	/* open the spread sheet file */
	Ofs[i].ss.fl = fopen (file, "r");
	if (Ofs[i].ss.fl == NULL) {
	    char *msg = MISC_malloc (strlen (file) + 128);
	    sprintf (msg, "Data file %s not found\n", file);
	    Cr_err_msg = STR_cat (Cr_err_msg, msg);
	    free (msg);
	    Cr_error = GDCR_FILE_MISSING;
	    return (NULL);
	}
	Ofs[i].type = GDCR_SS;
	Ofs[i].ss.cr_line = MISC_malloc (MAX_BUF_SIZE);
	Ofs[i].ss.cr_line[0] = '\0';
	Ofs[i].name = MISC_malloc (strlen (bn) + 1);
	strcpy (Ofs[i].name, bn);
	Ofs[i].delimiter = Set_get_delimiter (bn, '\0');
	N_ofs++;
    }

    Cr_delimiter = Ofs[i].delimiter;
    if (Parse_ss_path (path, &vc, &ve, &mc, &mvalue) != 0) {
	char *msg = MISC_malloc (strlen (full_path) + 128);
	sprintf (msg, "Unexpected data path syntex (%s)\n", full_path);
	Cr_err_msg = STR_cat (Cr_err_msg, msg);
	free (msg);
	Cr_error = GDCR_SYNTAX_ERROR;
	return (NULL);
    }

    if (mc >= 0)
	value = Search_ss_value (Ofs[i].ss.cr_line, vc, ve, mc, mvalue);
    else {
	Multi_v = STR_reset (Multi_v, 512);
	value = NULL;
    }
    if (value == NULL) {
	int size;
	char *p;
	fseek (Ofs[i].ss.fl, 0, SEEK_SET);
	p = Ofs[i].ss.cr_line;
	size = MAX_BUF_SIZE;
	while (fgets (p, size, Ofs[i].ss.fl) != NULL) {
	    int c, len;

	    char *pt = p;
	    c = 0;
	    do {		/* remove \r from the string */
		if (*pt != '\r') {
		    p[c] = *pt;
		    c++;
		}
		pt++;
	    } while (*pt != '\0');

	    len = strlen (p);
	    if (len >= 2 && p[len - 2] == '\\' && p[len - 1] == '\n') {
					/* line to be continued */
		p += len - 2;
		size -= len - 2;
		continue;
	    }
	    size -= len;
	    if (size <= 2)
		GDCP_exception_exit ("data row too long (in file %s): %s\n",
					Ofs[i].name, Ofs[i].ss.cr_line);
	    value = Search_ss_value (Ofs[i].ss.cr_line, vc, ve, mc, mvalue);
	    if (value != NULL) {
		if (mc >= 0)
		    break;
		v = GDCP_evaluate_variables (value);
		if (v != NULL) {
		    GDCM_strlcpy (value, v, MAX_BUF_SIZE);
		    free (v);
		}	
		if (STR_size (Multi_v) > 0)
		    Multi_v = STR_append (Multi_v, ",", 1);
		Multi_v = STR_append (Multi_v, value, strlen (value));
	    }
	    p = Ofs[i].ss.cr_line;
	    size = MAX_BUF_SIZE;
	}
    }
    if (mc < 0) {
	Multi_v = STR_append (Multi_v, "", 2); /* terminate string */
	return (Multi_v);
    }
    if (value == NULL) {
	char *msg = MISC_malloc (strlen (path) + strlen (file) + 128);
	sprintf (msg, "Could not find value (delimiter %c) of %s in file %s\n",
					Cr_delimiter, path, file);
	Cr_err_msg = STR_cat (Cr_err_msg, msg);
	free (msg);
	Cr_error = GDCR_NOT_FOUND;
	return (NULL);
    }
    v = GDCP_evaluate_variables (value);
    if (v != NULL) {
	GDCM_strlcpy (value, v, MAX_BUF_SIZE);
	free (v);
    }
    return (value);
}

/**********************************************************************

    Sets field delimiter for spread data file "fname" to "delimiter".

**********************************************************************/

void GDCR_set_delimiter (char *fname, char delimiter) {
    if (delimiter != '\0')
	Set_get_delimiter (MISC_basename (fname), delimiter);
}

/******************************************************************

    Sets/gets(delimiter = '\0') the column delimiter for file "bname"
    (base name). Returns the delimiter.

******************************************************************/

static char Set_get_delimiter (char *bname, char delimiter) {
    typedef struct {
	char *bname;
	char delimiter;
    } File_delimiter_t;
    static File_delimiter_t *fdls = NULL;
    static int n_fdls = 0;
    File_delimiter_t fdl;
    int i;

    if (delimiter != '\0') {		/* modify it for open file */
	for (i = 0; i < N_ofs; i++) {
	    if (strcmp (Ofs[i].name, bname) == 0)
		 Ofs[i].delimiter = delimiter;
	}
    }

    for (i = 0; i < n_fdls; i++) {	/* search/modify list */
	if (strcmp (fdls[i].bname, bname) == 0) {
	    if (delimiter != '\0')
		fdls[i].delimiter = delimiter;
	    return (fdls[i].delimiter);
	}
    }
    if (delimiter == '\0')		/* get delimiter */
	return (',');			/* return the default */

    /* add to the list */
    fdl.bname = MISC_malloc (strlen (bname) + 1);
    strcpy (fdl.bname, bname);
    fdl.delimiter = delimiter;
    fdls = (File_delimiter_t *)STR_append ((char *)fdls, (char *)&fdl, 
					sizeof (File_delimiter_t));
    n_fdls++;
    return (delimiter);
}

/******************************************************************

    If the "mc" column matched "value", returns the value of colums of 
    "vc" through "ve". If "ve" < "vc", returns the total number of 
    columns. Returns NULL on failure. The caller does not need to free
    the returned pointer. See gdc.1 for more details.

******************************************************************/

static char *Search_ss_value (char *line, int vc, int ve, 
						int mc, char *value) {
    static char tk[MAX_STR_SIZE], sep[8];
    int ind, space_added, found, cnt, quot_added;
    char *ps, *pe, *ccs, fc;

    ccs = GDCP_get_ccs ();
    fc = line[MISC_char_cnt (line, " \t")];
    if (ccs != NULL && (fc == ccs[PAS_C] || fc == ccs[COM_C] || fc == ccs[IGN_C]))
	return (NULL);

    if (Cr_delimiter != ' ')
	sprintf (sep, "S%c", Cr_delimiter);
    else
	strcpy (sep, "Q\"");
    found = 0;
    if (mc < 0)
	found = 1;
    else if (mc == 0) {
	int off = 0;
	mc = 1;
	while ((off = GDCM_stoken (line, sep, mc - 1, tk, MAX_STR_SIZE)) > 0) {
	    if (strcmp (tk, value) == 0) {
		found = 1;
		break;
	    }
	    mc++;
	}
	if (off == -1)
	    GDCP_exception_exit ("Data field too long: %s\n", tk);
    }
    else {
	int off = GDCM_stoken (line, sep, mc - 1, tk, MAX_STR_SIZE);
	if (off == -1)
	    GDCP_exception_exit ("Data field too long: %s\n", tk);
	if (off > 0 && strcasecmp (tk, value) == 0)
	    found = 1;
    }
    if (!found)
	return (NULL);

    if (vc == 0 && ve == 0) {
	int n = MISC_get_token (line, sep, 0, NULL, 0);
	if (n <= 0)
	    return (NULL);
	sprintf (Value, "%d", n);
	return (Value);
    }
    ind = vc;
    space_added = 0;
    ps = Value;
    pe = ps + MAX_BUF_SIZE - 1;
    cnt = 0;
    quot_added = 0;
    while (1) {
	char *p;
	if (ind > vc) {
	    if (ps + 1 >= pe)		/* no space */
		GDCP_exception_exit (
			"Data line too long to process: %s\n", line);
	    strcpy (ps, " ");
	    ps++;
	    space_added = 1;
	}
	if (MISC_get_token (line, sep, ind - 1, ps, pe - ps) <= 0) {
	    if (ve > 0 && ind < ve)
		return (NULL);
	    else
		break;
	}

	p = ps;
	while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n')
	    p++;
	if (p == ps || *p != '\0') {	/* add quotation */
	    if (ps + 2 >= pe)		/* no space */
		GDCP_exception_exit (
			"Data line too long to process: %s\n", line);
	    p = ps + strlen (ps) - 1;
	    p[2] = '"';
	    p[3] = '\0';
	    while (p >= ps) {
		p[1] = *p;
		p--;
	    }
	    *ps = '"';
	    quot_added = 1;
	}
	space_added = 0;
	ps += strlen (ps);
	ind++;
	cnt++;
	if (ve > 0 && ind > ve)
	    break;
    }
    if (ind > vc) {
	if (space_added)
	    ps[-1] = '\0';
	if (ve > 0 && ve == vc && cnt == 1 && quot_added) {	/* remove quotation */
	    Value[strlen (Value) - 1] = '\0';
	    return (Value + 1);
	}
	return (Value);
    }

    return (NULL);
}

/******************************************************************

    Parses the spread sheet path "path". Returns the value and 
    matching column numbers and the value to match. Returns 0 on 
    success or -1 if "path" is not syntatically valid.

******************************************************************/

static int Parse_ss_path (char *path, int *vcp, int *vep, 
						int *mcp, char **vp) {
    static char *pat = NULL;
    char *value, *p, c;
    int vc, ve, mc, qcnt, len;

    pat = STR_copy (pat, path);
    p = pat;
    vc = ve = -1;
    mc = -2;
    qcnt = 0;
    while (*p != '\0') {
	if (*p == '>' && !(qcnt % 2)) {
	    int ret;
	    *p = '\0';
	    p++;
	    if (strcmp (p, "#") == 0) {		/* return number of columns */
		ve = 0;
		vc = 0;
	    }
	    else if ((ret = sscanf (p, "%d%c", &vc, &c)) == 1) {
		if (vc <= 0)
		    return (-1);
		ve = vc;
	    }
	    else if (ret == 2 && c == '-') {
		char tk[MAX_STR_SIZE];
		ret = GDCM_ftoken (p, "S-", 1, tk, MAX_STR_SIZE);
		if (ret > 0) {
		    if (sscanf (tk, "%d%c", &ve, &c) != 1 || ve < vc)
			return (-1);
		}
		else if (ret == 0)
		    ve = 0;
		else
		    return (-1);
	    }
	    else
		return (-1);
	    break;
	}
	else if (*p == '"')
	    qcnt++;
	p++;
    }
    p = pat;
    value = pat;
    while (*p != '\0') {
	if (*p == '=') {
	    value = p + 1;
	    *p = '\0';
	    if (strcmp (pat, "*") == 0) {	/* *= for all column match */
		mc = 0;
		break;
	    }
	    if (sscanf (pat, "%d%c", &mc, &c) != 1 || mc <= 0)
		return (-1);
	    break;
	}
	p++;
    }
    len = strlen (value);
    if (vc < 0 || len == 0)
	return (-1);
    if (len > 2 && value[0] == '"' && value[len - 1] == '"') {
	value[len - 1] = '\0';		/* remove quotation */
	value++;
    }
    if (mc < -1) {	/* No "=" found */
	mc = 1;		/* default to 1 */
	if (strcmp (pat, "?") == 0)	/* special case for all rows */
	    mc = -1;
    }
    *mcp = mc;
    *vcp = vc;
    *vep = ve;
    *vp = value;

    return (0);
}

/******************************************************************

    Gets the attribute value for attribute named "attr_name" of node
    "node". The result is put in "buf" of size "buf_size". Returns 1
    on success of 0 on failure.

******************************************************************/

static int Get_attr_value (xmlNode *node, char *attr_name, 
					char *buf, int buf_size) {
    struct _xmlAttr *att;
    att = node->properties;
    while (att != NULL) {
	if (strcmp ((char *)att->name, attr_name) == 0) {
	    char *t;
	    t = (char *)xmlNodeListGetString (node->doc, att->children, 1);
	    if (strlen (t) >= buf_size)
		GDCP_exception_exit (
			"Attribute (%s) value too long\n", attr_name);
	    strcpy (buf, t);
	    xmlFree (t);
	    return (1);
	}
	att = att->next;
    }
    return (0);
}

/******************************************************************

    Gets the text of node "a_node". The result is put in "buf" of size
    "buf_size". If the text is empty, empty string is returned.
    Multiple text sections are combined together. Leading spaces and
    line returns of each section are discarded. Trailing spaces and
    line returns of the final text are also discarded.

******************************************************************/

static void Get_node_text (xmlNode *a_node, char *buf, int buf_size) {
    xmlNode *cur_node;
    int len;
    char *t;

    if (buf_size > 0)
	buf[0] = '\0';
    len = 0;
    for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type == XML_TEXT_NODE ||
				cur_node->type == XML_CDATA_SECTION_NODE) {
	    char *p;
	    t = (char *)xmlNodeGetContent (cur_node); /* cur_node->content */
	    p = t;
	    while (*p == '\n' || *p == ' ')	/* rm leading space */
		p++;
	    if (p[0] == '\0')
		continue;
	    if (strlen (p) + len < buf_size) {
		strcat (buf, p);
		len += strlen (p);
	    }
	    else
		GDCP_exception_exit (
			"Node (%s) content too long\n", cur_node->name);
	    xmlFree (t);
	}
    }
    t = buf + strlen (buf) - 1;
    while (t > buf && (*t == '\n' || *t == ' ')) {	/* rm trailing space */
	*t = '\0';
	t--;
    }
}

/**************************************************************************

    Goes through all sibling nodes of node "cr_node" and searches for
    one that matches "pat_level" section of Path_secs.

**************************************************************************/

static void Match_node (xmlNode *cr_node, int pat_level) {
    char *node, *attr, *value;
    xmlNode *cur_node = NULL;

    if (Done)
	return;

    node = Path_secs[pat_level].node;
    attr = Path_secs[pat_level].attr;
    value = Path_secs[pat_level].value;

    for (cur_node = cr_node; cur_node; cur_node = cur_node->next) {
	int match = 0;
        if (cur_node->type == XML_ELEMENT_NODE) {
	    char text[MAX_STR_SIZE];

	    if (strcmp ((char *)cur_node->name, node) == 0) {
		if (attr != NULL) {
		    if (value != NULL &&
			Get_attr_value (cur_node, attr, text, MAX_STR_SIZE)) {
			if (strcmp (value, "?") == 0 ||
			    strcmp (text, value) == 0)
			    match = 1;
		    }
		}
		else if (value != NULL) {
		    Get_node_text (cur_node, text, MAX_STR_SIZE);
		    if (strcmp (value, "?") == 0 || strcmp (text, value) == 0)
			match = 1;
		}
		else
		    match = 1;
	    }
	}
	if (match) {
	    if (pat_level < N_path_secs - 2)
		Match_node (cur_node->children, pat_level + 1);
	    else {
		char *attr = Path_secs[N_path_secs - 1].attr;
		if (strlen (attr) == 0)
		    Get_node_text (cur_node, Value, MAX_BUF_SIZE);
		else if (Get_attr_value (cur_node, attr, 
						Value, MAX_BUF_SIZE) == 0) {
		    char *msg = MISC_malloc (strlen (attr) + 128);
		    sprintf (msg, "Attribute (%s) not found\n", attr);
		    Cr_err_msg = STR_cat (Cr_err_msg, msg);
		    free (msg);
		    Cr_error = GDCR_NOT_FOUND;
		    Done = 1;
		    return;
		}
		if (Multi_values) {
		    if (STR_size (Multi_v) > 0)
			Multi_v = STR_append (Multi_v, ",", 1);
		    Multi_v = STR_append (Multi_v, Value, strlen (Value));
		}
		else {
		    Done = 1;
		    return;
		}
	    }
	}
    }
}

/**************************************************************************

    Removes the quatation on "str".

**************************************************************************/

static char *Remove_quote (char *str) {
    int len;
    if (str == NULL)
	return (NULL);
    len = strlen (str);
    if (len <= 2)
	return (str);
    if (str[0] == '"' && str[len - 1] == '"') {
	str[len - 1] = '\0';
	return (str + 1);
    }
    return (str);
}

/**************************************************************************

    Searches on the entire tree started from "root" looking for the
    value of gdc XML data path "d_path". Returns a pointer to the
    value on success or NULL on failure. The caller should not free
    the returned pointer.

**************************************************************************/

static char *Search_xml_value (xmlNode *root, char *d_path) {
    static char *buf = NULL;
    static int b_size = 0;
    char *p, *cr_sec;
    int len, new_sec, qcnt;

    Multi_values = 0;			/* A multi value search? */
    len = strlen (d_path);
    if (len + 1 > b_size) {	/* realloc buffer */
	if (buf != NULL)
	    free (buf);
	b_size = len + 1;
	buf = MISC_malloc (b_size);
    }
    strcpy (buf, d_path);
    cr_sec = buf;			/* current section */
    N_path_secs = 0;
    p = cr_sec;
    new_sec = 0;
    qcnt = 0;
    while (1) {
	char cr_c, *node, *attr, *value;

	cr_c = *p;			/* the current char */
	if (cr_c == '>' && !(qcnt % 2)) {
	    char *pp;
	    int qc;

	    /* parse current section */
	    *p = '\0';			/* terminate cr_sec for parsing */
	    node = cr_sec;
	    attr = value = NULL;
	    pp = cr_sec;
	    qc = 0;
	    while (*pp != '\0') {
		if (*pp == '.' && attr == NULL && !(qc % 2)) {
		    attr = pp + 1;
		    *pp = '\0';
		}
		if (*pp == '=' && value == NULL && !(qc % 2)) {
		    value = pp + 1;
		    *pp = '\0';
		}
		if (*pp == '"')
		    qc++;
		pp++;
	    }
	    cr_sec = p + 1;
	    new_sec = 1;
	}
	else if (cr_c == '\0') {	/* the last section */
	    node = attr = cr_sec;
	    value = NULL;
	    new_sec = 1;
	}
	else if (cr_c == '"')
	    qcnt++;

	if (new_sec) {
	    if (N_path_secs >= MAX_N_SECTIONS) {
				    /* reserve space for the last section */
		char *msg = MISC_malloc (strlen (d_path) + 128);
		sprintf (msg, "XML data path too long (%s)\n", d_path);
		Cr_err_msg = STR_cat (Cr_err_msg, msg);
		free (msg);
		Cr_error = GDCR_SYNTAX_ERROR;
		return (NULL);
	    }

	    if (node == attr) {	/* Be careful: Remove_quote modify string */
		Path_secs[N_path_secs].node = Remove_quote (node);
		Path_secs[N_path_secs].attr = Path_secs[N_path_secs].node;
	    }
	    else {
		Path_secs[N_path_secs].node = Remove_quote (node);
		Path_secs[N_path_secs].attr = Remove_quote (attr);
	    }
	    value = Remove_quote (value);
	    Path_secs[N_path_secs].value = value;
	    N_path_secs++;
	    if (value != NULL && strcmp (value, "?") == 0)
		Multi_values = 1;
	    new_sec = 0;
	}

	if (cr_c == '\0')	/* done */
	    break;
	p++;
    }
    if (N_path_secs <= 1) {
	Cr_error = GDCR_SYNTAX_ERROR;
	return (NULL);
    }

    Done = 0;
    if (Multi_values)
	Multi_v = STR_reset (Multi_v, 512);
    Search_tree (root);

    if (Cr_error != GDCR_NO_ERROR)
	return (NULL);
    if (Done)
	return (Value);
    else if (Multi_values) {
	Multi_v = STR_append (Multi_v, "", 2);
	return (Multi_v);
    }
    Cr_error = GDCR_NOT_FOUND;
    return (NULL);
}

/***************************************************************************

    Searches for a path started from "a_node" that matches Path_secs.
    This is recursively called for all nodes in the tree to find a
    valid path.

***************************************************************************/

static void Search_tree (xmlNode *a_node) {
    xmlNode *cur_node;

    if (Done || a_node == NULL)
	return;
    Match_node (a_node, 0);
    if (Done)
	return;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {

        Search_tree (cur_node->children);
	if (Done)
	    break;
    }
}

/**********************************************************************

    Creates a column alignmented file of "fname". The output file name 
    is fname.a.

**********************************************************************/

#define MAX_N_COLS 64
#define TMP_BUF_SIZE 512

void GDCR_align_columns (char *fname) {
    char buf[TMP_BUF_SIZE], *ofname, *ob;
    FILE *fd, *ofd;
    int csize[MAX_N_COLS], i, width;

    fd = fopen (fname, "r");
    if (fd == NULL) {
	fprintf (stderr, "Data file %s not found\n", fname);
	exit (1);
    }
    ofname = MISC_malloc (strlen (fname) + 8);
    strcpy (ofname, fname);
    strcat (ofname, ".a");
    ofd = fopen (ofname, "w");
    if (ofd == NULL) {
	fprintf (stderr, "Could not open/create file %s\n", ofname);
	exit (1);
    }
    if (VERBOSE)
	printf ("Aligning columns of data file %s\n", fname);

    for (i = 0; i < MAX_N_COLS; i++)
	csize[i] = 0;
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	int col;

	if (buf[MISC_char_cnt (buf, " \t")] == '#' ||
	    GDCM_ftoken (buf, "S,", 0, tk, MAX_STR_SIZE) <= 0)
	    continue;

	col = 0;
	while (GDCM_ftoken (buf, "S,", col, tk, MAX_STR_SIZE) > 0) {
	    if (strlen (tk) > csize[col])
		csize[col] = strlen (tk);
	    col++;
	    if (col >= MAX_N_COLS) {
		fprintf (stderr, "Too many columns found in %s\n", fname);
		exit (1);
	    }
	}
    }

    width = 0;
    for (i = 0; i < MAX_N_COLS; i++)
	width += csize[i];
    ob = MISC_malloc (width + 4 + MAX_N_COLS);
    fseek (fd, 0, SEEK_SET);
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	int col, len, l;

	if (buf[MISC_char_cnt (buf, " \t")] == '#' ||
	    GDCM_ftoken (buf, "S,", 0, tk, MAX_STR_SIZE) <= 0) {
	    fprintf (ofd, "%s", tk);
	    continue;
	}

	col = 0;
	len = l = 0;
	while (GDCM_ftoken (buf, "S,", col, tk, MAX_STR_SIZE) > 0) {
	    if (col > 0) {
		int ns = csize[col - 1] + 2 - l;
		for (i = 0; i < ns; i++)
		    ob[len + i] = ' ';
		len += ns;
	    }
	    l = strlen (tk);
	    memcpy (ob + len, tk, l);
	    len += l;
	    ob[len] = ',';
	    len++;
	    col++;
	}
	ob[len] = '\n';
	ob[len + 1] = '\0';
	fprintf (ofd, "%s", ob);
    }
    fclose (fd);
    fclose (ofd);
}
