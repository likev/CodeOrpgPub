/****************************************************************
		
    Module: cs_parse.c	
		
    Description: This file contains the text parse functions of the 
		configuration support (CS) module.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:40:09 $
 * $Id: cs_parse.c,v 1.30 2005/09/14 15:40:09 jing Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "cs.h"
#include "cs_def.h"

#define MAX_ERR_SIZE (3 * CS_NAME_SIZE)
				/* buffer size used for error messages */
#define KEY_SIZE 128		/* max size of expanded key */

#define CHAR_REMOVED ((char)254)

static cs_cfg_t *Cr_cfg = NULL;	/* The current config file */

static void (*Error_func) (char *) = NULL;	/* user's error function. */
static int No_env_exp = 0;

/* local functions */
static int Get_token_old (char *str, int tok, int buf_size, 
					char *buf, int key_len);
static int Get_token (char *str, int tok, int buf_size, 
					char *buf, int key_len);
static int Search_name (const char *key, int report_errors);
static int Search_int (int key, int report_errors);
static void Print_error_with_key (char *key, int key_len, char *msg);
static int Increase_current_index ();
static int Decrease_current_index ();
static int Get_unread_keys (int buf_size, char *buf);
static int Quotation_mark_error (int l_num);
static char *Expand_env_var (char *var);
static char *Expand_env_token (char *key, int len, int *new_len);
static int Expand_env_string (char *str, int len, char *out, int out_size);


/********************************************************************
			
    Sets the current config file to "cfg".

********************************************************************/

void CS_set_current_cfg (cs_cfg_t *cfg) {
    Cr_cfg = cfg;
}

/********************************************************************
			
    Returns the current config file.

********************************************************************/

cs_cfg_t *CS_get_current_cfg () {
    return (Cr_cfg);
}

/********************************************************************
			
    Generates the key table for config file "cfg". Returns 0 on success 
    or an error number.

********************************************************************/

int CS_gen_table (cs_cfg_t *cfg) {
    char *cs_buf, *fname, c, *pt;
    int found, in_quo;
    int cnt;
    int l_num;				/* current line number */
    int level;				/* session level */
    int n_entry_before_session;		/* tracking number of entries before 
					   a session */
    int escape;

    cs_buf = cfg->cs_buf;
    fname = cfg->cfg_name;

    if (cfg->cfg_list != NULL) {
	free (cfg->cfg_list);
	cfg->cfg_list = NULL;
    }

    cnt = 0;				/* find out how many entries */
    pt = cs_buf;
    while ((c = *pt) != '\0') {
	if (c == '\n' || c == '{' || c == '}')
	    cnt++;
	pt++;
    }
    cnt++;
    cfg->cfg_list = (Cs_entry *)malloc (cnt * sizeof (Cs_entry));
    if (cfg->cfg_list == NULL) {
	CS_print_err (sprintf (CS_err_buf (), "CS: malloc failed\n"));
	return (CS_MALLOC_ERR);
    }
    cfg->n_cfgs = 0;

    pt = cs_buf;
    found = 0;
    l_num = 1;
    level = n_entry_before_session = 0;
    in_quo = 0;
    escape = 0;
    while ((c = *pt) != '\0') {
	char *key;
	int ikey, klen;
	char tmpc, *kp;
	int need_expand;
	Cs_entry *new_ent;

	if (c == '\\' && (pt[1] == '{' || pt[1] == '}')) {
	    *pt = ' ';
	    escape = 1;
	    pt++;
	    continue;
	}

	/* process braces */
	if ((c == '{' || c == '}') && 
			!cfg->single_level && !escape && !in_quo) {
	    if (c == '{') {
		if (n_entry_before_session == 0) {
		    CS_print_err (sprintf (CS_err_buf (), 
		"CS: no entry found before a session (file %s, line %d)\n", 
			fname, l_num));
		    return (CS_FORMAT_ERROR);
		}
		level++;
	    }
	    if (c == '}') {
		n_entry_before_session = 0;
		level--;
	    }
	    if (level < 0) {
		CS_print_err (sprintf (CS_err_buf (), 
		"CS: unexpected } (file %s, line %d)\n", fname, l_num));
		return (CS_FORMAT_ERROR);
	    }
	    c = *pt = '\n';
	    l_num--;
	}
	escape = 0;

	if (c == '\\' && pt[1] == '\n')	{	/* line to be continued */
	    pt[0] = pt[1] = CHAR_REMOVED;
	    pt += 2;
	    while (*pt == ' ' || *pt == '\t') {
		*pt = CHAR_REMOVED;
		pt++;
	    }
	    l_num++;
	    continue;
	}

	/* process quotation mark */
	if (in_quo) {
	    if (c == ' ')
		*pt = SPACE_ESCAPE;
	    if (c == '\t' || c == '\n')
		return (Quotation_mark_error (l_num));
	}
	if (c == '\"' && found) {
	    char t;
	    if (in_quo) {
		t = pt[1];
		if (t != ' ' && t != '\t' && t != '}' && 
					t != '\n' && t != '\0' && t != '\\')
		    return (Quotation_mark_error (l_num));
		in_quo = 0;
	    }
	    else {
		t = pt[-1];
		if (t != ' ' && t != '\t' && t != CHAR_REMOVED)
		    return (Quotation_mark_error (l_num));
		in_quo = 1;
	    }
	}

	if (c == '\n') {		/* a new entry starts */
	    if (in_quo)
		return (Quotation_mark_error (l_num));
	    found = 0;
	    *pt = '\0';
	    pt++;
	    l_num++;
	    continue;
	}
	if (found || c == ' ' || c == '\t') {
					/* key found or leading spaces */
	    pt++;
	    continue;
	}

	/* comment line */
	if (cfg->comment_char != '\0' && cfg->comment_char == *pt) {
	    *pt = '\0';
	    pt++;
	    while ((c = *pt) != '\0' && c != '\n')
		pt++;
	    if (c == '\n') {
		*pt = '\0';
		pt++;
		l_num++;
	    }
	    found = 0;
	    continue;
	}

	/* a key found */
	key = pt;
	if (*pt == '\"') {
	   /*  Advance 1 past quote character */
	   key++;
	   pt++;
	   need_expand = 0;
	   while ((c = *pt) != '\0' && c != '\"' && c != '\n') {
	       if (c == '$' && pt[1] == '(')
		  need_expand = 1;
	       pt++;
	   }
           /* Replace the ending quote with a space */
           /* No continuation lines supported for quoted keys */
           if (c == '\"')
              *pt = ' ';
           else
  	       return (Quotation_mark_error (l_num));	/* missing quote */
	}
	else {
	   need_expand = 0;
	   while ((c = *pt) != '\0' && c != ' ' && c != '\t' && c != '\n') {
		if (c == '$' && pt[1] == '(')
		   need_expand = 1;
		pt++;
	   }
	}
	klen = pt - key;		/* length of the key */
	found = 1;
	n_entry_before_session++;

	/* register the entry */
	new_ent = cfg->cfg_list + cfg->n_cfgs;
	new_ent->key = key;
	new_ent->flags = 0;
	new_ent->len = klen;
	new_ent->level = level;
	new_ent->line_number = l_num;
	if (need_expand) {
	    new_ent->flags |= KEY_EXPANSION;
	    cfg->line = l_num;
	    kp = Expand_env_token (key, klen, NULL);
	    cfg->line = -1;
	    if (kp == NULL)
		return (CS_ENV_VAR_ERR);
	}
	else
	    kp = key;
	tmpc = key [klen];
	key[klen] = '\0';
	if (sscanf (kp, "%d%c", &ikey, &c) == 1) {
	    new_ent->ikey = ikey;
	    new_ent->flags |= INT_KEY;
	}
	key[klen] = tmpc;
	cfg->n_cfgs++;
    }
    if (level != 0) {
	CS_print_err (sprintf (CS_err_buf (), 
				"CS: unclosed { (file %s)\n", fname));
	return (CS_FORMAT_ERROR);
    }

    if (cfg->n_cfgs > 0)
	cfg->level = cfg->cfg_list[0].level;

    return (0);
}

/********************************************************************
			
    Description: This function processes a quotation mark error found
		in the text.

    Input:	l_num - current line number.

    Return:	CS_FORMAT_ERROR.

********************************************************************/

static int Quotation_mark_error (int l_num) {
    CS_print_err (sprintf (CS_err_buf (), 
	"CS: bad quotation mark \" (file %s, line %d)\n", 
					Cr_cfg->cfg_name, l_num));
    return (CS_FORMAT_ERROR);
}

/********************************************************************
			
    Description: This function sets parse control.

    Input:	action - control action flags.

********************************************************************/

int CS_parse_control (int action) {

    if (action & CS_NO_ENV_EXP)
	No_env_exp = 1;
    if (action & CS_YES_ENV_EXP)
	No_env_exp = 0;
    if (action & CS_BACK_LINE)
	return (Decrease_current_index ());
    return (0);
}

/********************************************************************
			
    Description: This function changes the current level.

    Input:	action - CS_UP_LEVEL, CS_DOWN_LEVEL, CS_TOP_LEVEL.

    Return:	0 on success or a negative error number.

********************************************************************/

int CS_level (int action) {
    int ret;
	    
    if ((ret = CS_update_cfg (Cr_cfg)) != 0)
	return (ret);

    if (Cr_cfg->index < 0)
        Cr_cfg->index = 0;
    if (Cr_cfg->index >= Cr_cfg->n_cfgs)
        return (CS_END_OF_TEXT);

    switch (action) {
	int ind;

	case CS_DOWN_LEVEL:
	    if (Cr_cfg->index < 0 ||
		Cr_cfg->index >= Cr_cfg->n_cfgs - 1 ||
		Cr_cfg->cfg_list[Cr_cfg->index].level != 
				Cr_cfg->cfg_list[Cr_cfg->index + 1].level - 1)
		return (CS_NO_SUCH_LEVEL);
	    Cr_cfg->index++;
	    Cr_cfg->level++;

	    /*  Advance past blank entries */
	    for (ind = Cr_cfg->index; ind < Cr_cfg->n_cfgs; ind++) {
	       if (strlen (Cr_cfg->cfg_list[ind].key) != 0)
	          break;
	       if (Cr_cfg->cfg_list[ind].level != Cr_cfg->level)
	          break;
	    }
	    
	    if (ind < Cr_cfg->n_cfgs && 
			Cr_cfg->cfg_list[ind].level == Cr_cfg->level)
	       Cr_cfg->index = ind;
	    
	    Cr_cfg->start = Cr_cfg->index;
	    return (0);

	case CS_UP_LEVEL:
	    ind = Cr_cfg->index - 1;
	    while (ind >= 0) {
		if (Cr_cfg->cfg_list[ind].level + 1 == Cr_cfg->level)
		    break;
		ind--;
	    }
	    if (ind < 0)
		return (CS_NO_SUCH_LEVEL);
	    Cr_cfg->index = ind;
	    Cr_cfg->level--;
	    while (ind >= 0) {
		if (Cr_cfg->cfg_list[ind].level < Cr_cfg->level)
		    break;
		ind--;
	    }
	    Cr_cfg->start = ind + 1;
	    return (0);

	case CS_TOP_LEVEL:
	    Cr_cfg->index = Cr_cfg->start = 0;
	    Cr_cfg->level = 0;
	    if (Cr_cfg->n_cfgs > 0)
		Cr_cfg->level = Cr_cfg->cfg_list[0].level;
	    return (0);

	default:
	    return (CS_BAD_ARGUMENT);
    }
}

/********************************************************************
			
    Description: This function looks up a configuration entry by an 
		integer key. 

    Input:	key - The configuration entry key.
		tok - Which token to return in the entry.
		buf_size - Size of the users buffer.

    Output:	buf - For returning the token or entry.

    Returns:	The length of the returned string on success or a 
		negative error number (Refer to the man page).

********************************************************************/

int CS_int_entry (int key, int tok, int buf_size, char *buf)
{
    int ind, ret;
    Cs_entry *ent;

    if ((unsigned int)tok >= CS_FULL_LINE)
	return (CS_BAD_ARGUMENT);
	
    if ((ret = CS_update_cfg (Cr_cfg)) != 0)
	return (ret);

    ind = Search_int (key, 1);
    if (ind < 0)
	return (ind);

    Cr_cfg->index = ind;
    ent = Cr_cfg->cfg_list + ind;
    ent->flags |= LINE_READ;
    return (Get_token_old (ent->key, tok, buf_size, buf, ent->len));
}

/********************************************************************
			
    Description: This function searches for an integer-keyed entry index. 

    Input:	key - The configuration entry key.
    		report_error - whether to print errors or not

    Returns:	The index on success or a negative error number.

********************************************************************/

static int Search_int (int key, int report_errors) {
    int ind, cnt, i;
    char tmpbuf[128];

    cnt = ind = 0;
    for (i = Cr_cfg->start; i < Cr_cfg->n_cfgs; i++) {
	Cs_entry *ent;
	ent = Cr_cfg->cfg_list + i;
	if (ent->level < Cr_cfg->level)
	    break;
	if (ent->level > Cr_cfg->level)
	    continue;
	if ((ent->flags & INT_KEY) && ent->ikey == key) {
	    cnt++;
	    if (cnt == 1)
		ind = i;
	    if (cnt >= 2)
		break;
	}
    }
    if (cnt == 0) {
	if (i > 0)
	    Cr_cfg->line = Cr_cfg->cfg_list[i - 1].line_number;
	if (!Cr_cfg->key_optional && report_errors) {
	    sprintf (tmpbuf, "int key %d not found\n", key);
	    CS_report (tmpbuf);
	}
	return (CS_KEY_NOT_FOUND);
    }
    if (cnt > 1) {
	Cr_cfg->line = Cr_cfg->cfg_list[i].line_number;
	if (report_errors) {
	   sprintf (tmpbuf, "key %d ambiguous\n", key);
	   CS_report (tmpbuf);
	}
	return (CS_KEY_AMBIGUOUS);
    }

    return (ind);
}

/********************************************************************
			
    Description: This function searches for the next entry and 
		updates Current_index.

    Returns:	0 on success or a negative error number.

********************************************************************/

static int Increase_current_index () {
    int ind, i;

    for (i = Cr_cfg->index + 1; i < Cr_cfg->n_cfgs; i++) {
	Cs_entry *ent;
	ent = Cr_cfg->cfg_list + i;
	if (ent->level < Cr_cfg->level)
	    break;
	if (ent->level > Cr_cfg->level)
	    continue;
	/*  Skip blank entries */
	if (strlen (ent->key) == 0)
	    continue;
	Cr_cfg->index = i;
	return (0);
    }
    ind = i - 1;
    if (ind >= 0 && ind < Cr_cfg->n_cfgs)
	Cr_cfg->line = Cr_cfg->cfg_list[ind].line_number;
    return (CS_END_OF_TEXT);
}

/********************************************************************
			
    Description: This function searches for the previous entry and 
		updates Cr_cfg->index. 

    Returns:	0 on success or a negative error number.

********************************************************************/

static int Decrease_current_index () {
    int i;

    for (i = Cr_cfg->index - 1; i >= 0; i--) {
	Cs_entry *ent;
	ent = Cr_cfg->cfg_list + i;
	if (ent->level < Cr_cfg->level)
	    break;
	if (ent->level > Cr_cfg->level)
	    continue;
	Cr_cfg->index = i;
	return (0);
    }
    return (CS_END_OF_TEXT);
}

/********************************************************************
			
    Description: This function looks up an configuration entry by an
		integer key. 

    Input:	key - The configuration entry key.
		tok - Which token to return in the entry.
		buf_size - Size of the users buffer.

    Output:	buf - For returning the token or entry.

    Returns:	The length of the returned string on success or a 
		negative error number (Refer to the man page).

********************************************************************/

int CS_entry_int_key (int key, int tok, int buf_size, char *buf)
{
    return (CS_entry ((char *)key, CS_INT_KEY | tok, buf_size, buf));
}

/********************************************************************
			
    Description: This function looks up an configuration entry by a
		key. 

    Input:	key - The configuration entry key.
		tok - Which token to return in the entry.
		buf_size - Size of the users buffer.

    Output:	buf - For returning the token or entry.

    Returns:	The length of the returned string on success or a 
		negative error number (Refer to the man page).

********************************************************************/

int CS_entry (const char *key, int tok, int buf_size, char *buf) {
    int ind, ret;
    Cs_entry *ent;

    if ((ret = CS_update_cfg (Cr_cfg)) != 0)
	return (ret);
    ind = 0;
    Cr_cfg->line = Cr_cfg->token = -1;

    if (tok & CS_UNREAD_KEYS)
	return (Get_unread_keys (buf_size, buf));

    if ((key == CS_NEXT_LINE || key == CS_THIS_LINE) && !(tok & CS_INT_KEY)) {

	if (key == CS_NEXT_LINE &&
	    (ret = Increase_current_index ()) < 0)
	    return (ret);

	if (Cr_cfg->index < 0)
	    Cr_cfg->index = 0;
	if (Cr_cfg->index >= Cr_cfg->n_cfgs)
	    return (CS_END_OF_TEXT);

	ent = Cr_cfg->cfg_list + Cr_cfg->index;
	ent->flags |= LINE_READ;
	Cr_cfg->line = ent->line_number;
	if (buf == NULL)
	    return (0);
	else
	    return (Get_token (ent->key, tok, buf_size, buf, ent->len));
    }

    if (tok & CS_INT_KEY)
	ind = Search_int ((int)key, 1);
    else 
	ind = Search_name (key, 1);

    if (ind < 0)
	return (ind);

    Cr_cfg->index = ind;
    ent = Cr_cfg->cfg_list + ind;
    ent->flags |= LINE_READ;
    Cr_cfg->line = ent->line_number;
    if (buf == NULL)
	return (0);
    else
	return (Get_token (ent->key, tok, buf_size, buf, ent->len));
}

/********************************************************************
			
    Description: This function looks up an configuration entry by an 
		name key. 

    Input:	key - The configuration entry key.
		tok - Which token to return in the entry.
		buf_size - Size of the users buffer.

    Output:	buf - For returning the token or entry.

    Returns:	The length of the returned string on success or a 
		negative error number (Refer to the man page).

********************************************************************/

int CS_name_entry (const char *key, int tok, int buf_size, char *buf) {
    int ind, ret;
    Cs_entry *ent;

    if ((unsigned int)tok >= CS_FULL_LINE)
	return (CS_BAD_ARGUMENT);

    if ((ret = CS_update_cfg (Cr_cfg)) != 0)
	return (ret);

    ind = Search_name (key, 1);
    if (ind < 0)
	return (ind);

    Cr_cfg->index = ind;
    ent = Cr_cfg->cfg_list + ind;
    ent->flags |= LINE_READ;
    return (Get_token_old (ent->key, tok, buf_size, buf, ent->len));
}

/********************************************************************
			
    Description: This function searches for an name-keyed entry index. 

    Input:	key - The configuration entry key.

    Returns:	The index on success or a negative error number.

********************************************************************/

static int Search_name (const char *key, int report_errors)
{
    int ind=0, len, i, cnt=0;
    char tmpbuf[128];

    len = strlen (key);
    cnt = 0;
    for (i = Cr_cfg->start; i < Cr_cfg->n_cfgs; i++) {
	char *cpt;
	int klen;
	Cs_entry *cfg;

	cfg = Cr_cfg->cfg_list + i;
	if (cfg->level < Cr_cfg->level)
	    break;
	if (cfg->level > Cr_cfg->level)
	    continue;
	    
	cpt = cfg->key;
	klen = cfg->len;
	if (cfg->flags & KEY_EXPANSION) {
	    cpt = Expand_env_token (cfg->key, cfg->len, &klen);
	    if (cpt == NULL)
		return (CS_ENV_VAR_ERR);
	}
	if (/*(cfg->flags & INT_KEY) == 0 && */
	    len == klen &&
	    *cpt == *key &&
	    strncmp (cpt, key, len) == 0) {
	    cnt++;
	    if (cnt == 1)
		ind = i;
	    if (cnt >= 2)
		break;
	}
    }
    if (cnt == 0) {
	if (i > 0)
	    Cr_cfg->line = Cr_cfg->cfg_list[i - 1].line_number;
	if ((!Cr_cfg->key_optional) && (report_errors))
	 {
	    sprintf (tmpbuf, "key %s not found\n", key);
	    CS_report (tmpbuf);
	}
	return (CS_KEY_NOT_FOUND);
    }
    if (cnt >= 2) 
    {
	Cr_cfg->line = Cr_cfg->cfg_list[i].line_number;
	if (report_errors)
	{
	   sprintf (tmpbuf, "key %s ambiguous\n", key);
	   CS_report (tmpbuf);
	}
	return (CS_KEY_AMBIGUOUS);
    }

    return (ind);
}

/********************************************************************
			
    Description: This function returns the next entry after the 
		previously returned entry. 

    Input:	tok - Which token to return in the entry.
		buf_size - Size of the users buffer.

    Output:	buf - For returning the line.

    Returns:	The length of the returned string on success or a 
		negative error number (Refer to the man page).

********************************************************************/

int CS_next_line (int tok, int buf_size, char *buf) {
    Cs_entry *ent;
    int ret;

    if ((unsigned int)tok >= CS_FULL_LINE)
	return (CS_BAD_ARGUMENT);

    if ((ret = CS_update_cfg (Cr_cfg)) != 0)
	return (ret);

    if ((ret = Increase_current_index ()) < 0)
	return (ret);

    ent = Cr_cfg->cfg_list + Cr_cfg->index;
    ent->flags |= LINE_READ;
    return (Get_token_old (ent->key, tok, buf_size, buf, ent->len));
}

/********************************************************************
			
    Description: This function retrieves a token in a string and puts 
		it in "buf". If "buf" is too small, the string is 
		truncated.

    Input:	str - The string to be processed.
		tok - Token index to retrieve. If 0 whole string is 
			returned.
		buf_size - Size of "buf".
		key - the key for error printing.
		key_len - length of the key portion of the key field.

    Output:	buf - For returning the token.

    Returns:	The length of the returned string on success or a 
		negative error number (CS_TOK_NOT_FOUND if the 
		"tok"-th token is not found or CS_BUF_TOO_SMALL if 
		buf_size is too small).

********************************************************************/

static int Get_token_old (char *str, int tok, int buf_size, 
				char *buf, int key_len) {

    if (tok <= 0) 
	return (Get_token (str, CS_FULL_LINE, buf_size, buf, key_len));
    else
	return (Get_token (str, tok, buf_size, buf, key_len));

}

/********************************************************************
			
    Description: This function retrieves a token in a string and puts 
		it in "buf". If "buf" is too small, the string is 
		truncated.

    Input:	str - The string to be processed.
		tok - Token index to retrieve. If 0 whole string is 
			returned.
		buf_size - Size of "buf".
		key_len - length of the key portion of the key field

    Output:	buf - For returning the token.

    Returns:	The length of the returned string on success or a 
		negative error number (CS_TOK_NOT_FOUND if the 
		"tok"-th token is not found or CS_BUF_TOO_SMALL if 
		buf_size is too small).

********************************************************************/

static int Get_token (char *str, int tok, int buf_size, 
					char *buf, int key_len) {
    int len, buf_small, in_quo;
    char *pt, *cpt, tc, c;
    char msgbuf [128];
    double dd;
    char *outpt, *outend, *end, *tstr;
    int short_token;

    if (tok & (CS_INT | CS_UINT | CS_FLOAT | CS_DOUBLE | CS_SHORT | 
					CS_BYTE | CS_CHAR | CS_HEXINT))
	short_token = 1;
    else
	short_token = 0;
	
    if ((tok & CS_ALL_TOKENS) && (short_token))
       return (CS_BAD_ARGUMENT);

    if (tok & CS_FULL_LINE) {
	if (short_token)
	    return (CS_BAD_ARGUMENT);
	    
	pt = str;
	while ((c = *pt) == ' ' || c == '\t')
	    pt++;
	len = strlen (pt);
    }
    else if ((tok & CS_TOK_MASK) == 0) {
				/* If the client is requesting a key token, Set
				   pt to str, and the length is either */
	Cr_cfg->token = 0;
	pt = str;
	len = key_len;
    }
    else {
	int tcnt, intok;
	int n_tok;
	
	/*  Skip the key field for this search */
	pt = str + key_len;
	
	/* Since we have already skipped the key token, we must start tcnt at 
	   0 instead -1. n_tok should not ever be 0 at this point. */
	tcnt = 0;
	intok = 0;
	n_tok = tok & CS_TOK_MASK;
	while ((c = *pt) != '\0') {

	    if (c == ' ' || c == '\t') {
		intok = 0;
	    }
	    else {
		if (intok == 0) {
		    tcnt++;
		    intok = 1;
		    if (tcnt == n_tok) {		/* token found */
			break;
		    }
		}
	    }
	    pt++;
	}
	if (c == '\0') {	/* token not found */
	    if (!Cr_cfg->key_optional) {
		sprintf (msgbuf, "%d-th token not found for", n_tok);
		Print_error_with_key (str, key_len, msgbuf);
	    }
	    return (CS_TOK_NOT_FOUND);
	}

	len = 0;
	cpt = pt;
	while (1) {
	    if ((c = *cpt) == ' ' || c == '\t' || c == '\n' || c == '\0')
		break;
	    else
		len++;
	    cpt++;
	}
	Cr_cfg->token = n_tok;
    }
    
    if (tok & CS_ALL_TOKENS)
       len = str + strlen (str) - pt;

    tstr = pt;
    tc = pt[len];
    pt[len] = '\0';
    if (short_token) {
	tstr = Expand_env_token (pt, len, NULL);
	if (tstr == NULL)
	    return (CS_ENV_VAR_ERR);
    }
    if ((tok & (CS_INT | CS_SHORT | CS_BYTE))) {	/* parse integer */
	int ii;

	if (sscanf (tstr, "%d%c", &ii, &c) != 1) {
	    sprintf (msgbuf, "token (%s) is not a valid integer", pt);
	    pt[len] = tc;
	    Print_error_with_key (str, key_len, msgbuf);
	    return (CS_CONVERT_ERROR);
	}
	pt[len] = tc;
	if (tok & CS_INT)
	    *((int *)buf) = ii;
	else if (tok & CS_SHORT)
	    *((short *)buf) = ii;
	else if (tok & CS_BYTE)
	    *((char *)buf) = ii;
	return (1);
    }
    
    if ((tok & CS_UINT)) 
    {
    	/* parse unsigned integer */
	unsigned int ii;

	if (sscanf (tstr, "%u%c", &ii, &c) != 1) {
	    sprintf (msgbuf, "token (%s) is not a valid unsigned integer", pt);
	    pt[len] = tc;
	    Print_error_with_key (str, key_len, msgbuf);
	    return (CS_CONVERT_ERROR);
	}
	pt[len] = tc;
	if (tok & CS_UINT)
	    *((unsigned int *)buf) = ii;
	return (1);
    }

    if ((tok & (CS_FLOAT | CS_DOUBLE))) {	/* parse floating */

	if (sscanf (tstr, "%lf%c", &dd, &c) != 1) {
	    sprintf (msgbuf, 
			"token (%s) is not a valid floating point value", pt);
	    pt[len] = tc;
	    Print_error_with_key (str, key_len, msgbuf);
	    return (CS_CONVERT_ERROR);
	}
	pt[len] = tc;
	if (tok & CS_FLOAT)
	    *((float *)buf) = dd;
	else if (tok & CS_DOUBLE)
	    *((double *)buf) = dd;
	return (1);
    }

    if ((tok & (CS_HEXINT))) {			/* parse hexadecimal */
	unsigned int ti;

	if (sscanf (tstr, "%x%c", &ti, &c) != 1) {
	    sprintf (msgbuf, 
			"token (%s) is not a valid hexadecimal integer", pt);
	    pt[len] = tc;
	    Print_error_with_key (str, key_len, msgbuf);
	    return (CS_CONVERT_ERROR);
	}
	pt[len] = tc;
	*((unsigned int *)buf) = ti;
	return (1);
    }
    pt[len] = tc;

    /* copy character string (process quotation marks) */
    if (!(tok & CS_FULL_LINE) && *pt == '\"') {
	pt++;
	len -= 2;
	in_quo = 1;
    }
    else
	in_quo = 0;

    outpt = buf;
    outend = outpt + buf_size;
    end = pt + len;
    while (pt < end) {
	if (outpt >= outend)
	    break;

	if (in_quo && *pt == '\"')
	    break;
	if (*pt == '$' && pt[1] == '(') {
	    int ret, i;

	    ret = Expand_env_string (pt, end - pt, outpt, outend - outpt);
	    if (ret < 0)
		return (CS_ENV_VAR_ERR);
	    for (i = 0; i < ret; i++) {
		if (outpt[i] == SPACE_ESCAPE)
		    outpt[i] = ' ';
	    }
	    outpt += ret;
	    break;
	}
	else
	    *outpt = *pt;
	outpt++;
	pt++;
    }
    if (outpt < outend)
	buf_small = 0;
    else {
	outpt = buf - 1;
	buf_small = 1;
    }
    *outpt = '\0';

    if (1) {
	pt = buf;
	outpt = buf;
	while (*pt != '\0') {
	    if (*pt == SPACE_ESCAPE)
		*outpt = ' ';
	    else if (*pt == CHAR_REMOVED)
		outpt--;
	    else
		*outpt = *pt;
	    outpt++;
	    pt++;
	}
    }
    *outpt = '\0';

    if (buf_small) {		/* buffer too small */
	sprintf (msgbuf, "buf size (%d) too small", buf_size);
	Print_error_with_key (str, key_len, msgbuf);
	return (CS_BUF_TOO_SMALL);
    }
    else
	return (outpt - buf);
}

/********************************************************************
			
    Description: This function adds key and filename information to
		the error message "msg" and prints it out.

    Input:	key - string starting with the key.
		msg - the error message.

********************************************************************/

static void Print_error_with_key (char *key, int key_len, char *msg)
{
    char *cpt;
    char tc;

    cpt = key;		/* terminate key for error printing */
    cpt += key_len;

    tc = *cpt;
    *cpt = '\0';
    CS_print_err (sprintf (CS_err_buf (), 
		"CS: %s (key %s) in %s\n", msg, key, Cr_cfg->cfg_name));
    *cpt = tc;		/* recover key */
    return;
}


/********************************************************************
			
    Description: This function registers an error call back function,
		which will be called to pass a message when an error 
		is encountered.

    Input:	error_func - the user's callback function.

********************************************************************/

void CS_error (void (*error_func)(char* buf)) {

    Error_func = error_func;
    return;
}

/********************************************************************
			
    Description: This function calls the user error function and 
		passes the error message.

    Input:	arg - not used.

********************************************************************/

void CS_print_err (int arg)
{

    if (Error_func == NULL)
	return;

    if (Cr_cfg->line >= 0) {
	char *buf, tmp[128];
	int len;

	buf = CS_err_buf ();
	len = strlen (buf);
	if (buf[len - 1] == '\n')
	    buf[len - 1] = '\0';
	if (Cr_cfg->token >= 0) 
	    sprintf (tmp, " (line %d, token %d)\n", 
					Cr_cfg->line, Cr_cfg->token);
	else
	    sprintf (tmp, " (line %d)\n", Cr_cfg->line);
	strcat (buf, tmp);
    }

    Error_func (CS_err_buf ());
    return;
}

/********************************************************************
			
    Description: This function reports a user message with additional 
		file name, line number and token number information 
		if they are available.

    Input:	msg - user's message.

********************************************************************/

void CS_report (const char *msg)
{
    char *buf;
    int len;

    if (Error_func == NULL)
	return;

    buf = CS_err_buf ();
    strncpy (buf, msg, MAX_ERR_SIZE);
    buf[MAX_ERR_SIZE - 1] = '\0';
    len = strlen (buf);
    if (buf[len - 1] == '\n') {
	buf[len - 1] = '\0';
	len--;
    }

    if (Cr_cfg->cfg_name[0] != '\0') {
	char tmp[CS_NAME_SIZE + 80];
	int tmplen;
	char *stpt;

	if (Cr_cfg->line >= 0) {
	    if (Cr_cfg->token >= 0) 
	        sprintf (tmp, " (file %s, line %d, token %d)\n", 
				Cr_cfg->cfg_name, Cr_cfg->line, Cr_cfg->token);
	    else
		sprintf (tmp, " (file %s, line %d)\n", 
					Cr_cfg->cfg_name, Cr_cfg->line);
	}
	else
	    sprintf (tmp, " (file %s)\n", Cr_cfg->cfg_name);
	tmplen = strlen (tmp);
	if (len + tmplen >= MAX_ERR_SIZE)
	    stpt = buf + (MAX_ERR_SIZE - tmplen -1);
	else
	    stpt = buf + len;
	strcpy (stpt, tmp);
    }

    Error_func (CS_err_buf ());
    return;
}

/********************************************************************
			
    Description: This function returns a buffer for storing error 
		messages. The caller has to make sure that its message
		is less than MAX_ERR_SIZE.

    Return:	Pointer to the error message buffer.

********************************************************************/

char *CS_err_buf (void)
{
    static char err_buf [MAX_ERR_SIZE];

    return (err_buf);
}

/********************************************************************
			
    Description: This function retrieves all unaccessed keys. 

    Input:	buf_size - Size of the users buffer.

    Output:	buf - For returning the unread keys.

    Returns:	The number of unaccessed keys.

********************************************************************/

static int Get_unread_keys (int buf_size, char *buf)
{
    int cnt, i, ind, done;

    ind = cnt = 0;
    buf[ind] = '\0';
    done = 0;
    for (i = 0; i < Cr_cfg->n_cfgs; i++) {
	Cs_entry *ent;

	ent = Cr_cfg->cfg_list + i;
	if (!(ent->flags & LINE_READ)) {

	    if (ind + ent->len + 10 >= buf_size) {
				/* 10 chars reserved for line number */
		done = 1;
		if (i < Cr_cfg->n_cfgs)
		    strcpy (buf + ind, " ... ");
	    }
	    if (!done) {
		char tmp[32];
		int len;

		memcpy (buf + ind, ent->key, ent->len);
		ind += ent->len;
		sprintf (tmp, "(%d);", ent->line_number);
		len = strlen (tmp);
		memcpy (buf + ind, tmp, len + 1);
		ind += len;
	    }
	    cnt++;
	}
    }
    return (cnt);
}

/********************************************************************
			
    Description: This function returns a pointer to the expanded key.

    Input:	key - The key need to be expanded.
		len - length of the unexpanded key.

    Output:	new_len - length of the expanded key.

    Return:	Pointer to the expanded key on success or NULL on 
		failure.

********************************************************************/

static char *Expand_env_token (char *key, int len, int *new_len)
{
    static char buf[KEY_SIZE];
    int rlen;

    rlen = Expand_env_string (key, len, buf, KEY_SIZE - 1);
    if (rlen < 0)
	return (NULL);
    buf[rlen] = '\0';
    if (new_len != NULL)
	*new_len = rlen;
    return (buf);
}

/********************************************************************
			
    Description: This function expands all environmental vars in a 
		string.

    Input:	key - The string need to be expanded.
		len - length of the unexpanded string.
		out_size - size of the out buffer.

    Output:	out - buffer for the expanded string.

    Return:	length of the expanded string on success or -1 on 
		failure.

********************************************************************/

static int Expand_env_string (char *str, int len, char *out, int out_size)
{
    char *pt, *outpt, *outend, *end;

    pt = str;
    outpt = out;
    outend = outpt + out_size;
    end = pt + len;
    while (pt < end) {
	char *p;

	if (outpt >= outend)
	    break;

	if (*pt == '$' && pt[1] == '(' && !No_env_exp) {
	    p = Expand_env_var (pt);
	    if (p == NULL)
		return (-1);
	    while (*p != '\0' && outpt < outend) {
		*outpt = *p;
		p++;
		outpt++;
	    }
	    while (*pt != ')')
		pt++;
	}
	else {
	    *outpt = *pt;
	    outpt++;
	}
	pt++;
    }
    return (outpt - out);
}

/********************************************************************
			
    Description: This function returns a pointer to the expanded
		string of a environment variable. The environmental
		variable is in the form of $(NAME).

    Input:	var - The environmental variable.

    Return:	Pointer to the expanded string on success or NULL on
		failure.

********************************************************************/

static char *Expand_env_var (char *var)
{

    if (var[0] == '$' && var[1] == '(') {
	char *pt, c;
	char tmpb[128], cnt;

	pt = var + 2;
	while ((c = *pt) != '\0' && c != ' ' && c != '\t' && 
						c != ')' && c != '\n')
	    pt++;

	*pt = '\0';
	if (c == ')') {
	    char *expanded;

	    expanded = getenv (var + 2);
	    if (expanded != NULL) {
		*pt = c;
		return (expanded);
	    }
	}
	strcpy (tmpb, "CS: failed in expanding environ variable: ");
	cnt = strlen (tmpb);
	strncat (tmpb + cnt, var, 60);
	tmpb[cnt + 60] = '\0';
	if (c == ')')
	    strcat (tmpb, ")");
	CS_report (tmpb);
	*pt = c;
	return (NULL);
    }

    return (var);
}


