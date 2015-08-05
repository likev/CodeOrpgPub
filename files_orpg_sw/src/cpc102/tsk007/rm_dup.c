
/**************************************************************************

    Description: This module contains the routines that removes duplicated
	$INCLUDE within a subroutine and processes several statements:
	END, BLOCK DATA and IMPLICIT NONE.

**************************************************************************/
/*
 * RCS info
 * $Author: dodson $
 * $Locker:  $
 * $Date: 1998/10/17 17:20:49 $
 * $Id: rm_dup.c,v 1.9 1998/10/17 17:20:49 dodson Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>           /* strcasecmp(), strncasecmp()             */

#include "ftnpp_def.h"

#define MAXN_NAMES	200


static int N_names = 0;		/* number of stored names */


static int Match_two_words (char *cpt, char *word1, char *word2, int len);

/**************************************************************************

    Description: This function checks an include section name. If this 
		section has been processed, it returns RDP_INCLUDED. 
		Otherwise it registers the new section name and returns 
		RDP_NOT_INCLUDED.

    Input:	fname - file name of the section;
		section - section name;

    Return:	It returns RDP_INCLUDED if the section is included or
		RDP_NOT_INCLUDED if it is not.

    Notes:	This function exits if a fatal error is found.

**************************************************************************/

int RDP_check_section (char *fname, char *section)
{
    static char *comb_name[MAXN_NAMES];
				/* array of used combined file and section 
				   names */
    static char name_buf [MAXN_NAMES * NAME_LEN / 2];
				/* space for the stored names */
    static int buf_off = 0;	/* offset of the free space in name_buf */
    char tmp [NAME_LEN * 2];
    int len, i;

    /* combine the file name and the section name */
    strcpy (tmp, fname);
    strcat (tmp, section);

    /* search for the new name */
    for (i = 0; i < N_names; i++) {

	if (strcasecmp (tmp, comb_name [i]) == 0)
	    return (RDP_INCLUDED);
    }

    /* register the new section */
    if (N_names == 0)		/* reset buf_off */
	buf_off = 0;
    len = strlen (tmp);
    if (len + buf_off >= MAXN_NAMES * NAME_LEN ||
	N_names >= MAXN_NAMES) {

	printf ("Too many file:section names registered\n");
	FTNPP_task_terminate (-1, 0, NULL);
    }
    comb_name [N_names] = name_buf + buf_off;
    strcpy (comb_name [N_names], tmp);
    buf_off += len + 1;
    N_names++;

    return (RDP_NOT_INCLUDED);
}

/**************************************************************************

    Description: This function processes END, IMPLICIT NONE and BLOCK DATA
		statements.

    Inputs:	cpt - The current line with leading spaces removed.
		level - The current include level.

    Return:	returns non-zero if the line is to be commented out. 
		Otherwise it returns 0.

    Notes:	This function exits if a fatal error is found.

**************************************************************************/

int RDP_special_statement (char *cpt, int level, int line_num, char *fname)
{
    static int implicit_none_found = 0;
				/* a "IMPLICIT NONE" is found in the current 
				   subroutine */
    static int block_data_level = -1;		
				/* a BLOCK DATA section is started in this 
				   level */
    char c;

    /* process END statement */
    if ((*cpt == 'E' || *cpt == 'e') &&
	strncasecmp(cpt, "end", 3) == 0 &&
	((c = cpt [3]) == ' ' || c == ',' || c == '\t' || c == '\n' ||
				c == ';' || c == '\0')) {

	if (level == 1) {	/* end of a subroutine */
	    N_names = 0;
	    implicit_none_found = 0;
	    REP_reset_N_lhex ();
	}

	if (block_data_level == level) {	/* end of block data */
	    block_data_level = -1;
	    return (1);
	}
    }

    /* process IMPLICIT NONE statement */
    if ((*cpt == 'I' || *cpt == 'i') && (cpt[1] == 'M' || cpt[1] == 'm')) {
	if (Match_two_words (cpt, "implicit", "none", 8)) {

	    if (implicit_none_found)
		return (1);
	    else
		implicit_none_found = 1;
	}
    }

    /* process BLOCK DATA statement */
    if ((*cpt == 'B' || *cpt == 'b') && (cpt[1] == 'L' || cpt[1] == 'l')) {
	if (Match_two_words (cpt, "block", "data", 5)) {

	    if (block_data_level >= 0) {
		printf ("nested BLOCK DATA found\n");
		FTNPP_task_terminate (-1, line_num, fname);
	    }

	    if (level <= 1)
		return (0);
	    else {
		block_data_level = level;
		return (1);
	    }
	}
    }
    /* block data file ended without a END statement - we assume it is fine */
    if (block_data_level > level)
	block_data_level = -1;

    return (0);
}

/**************************************************************************

    Description: This function matches a pattern of two words.

    Inputs:	cpt - The string to be tested.
		word1 - The first word of the pattern.
		word2 - The second word of the pattern.
		len - length of the word1.

    Return:	returns non-zero if the pattern is matched. Otherwise it 
		returns 0.

**************************************************************************/

static int Match_two_words (char *cpt, char *word1, char *word2, int len)
{

    if (strncasecmp(cpt, word1, len) == 0) {
	char tmp [128];
	char *p;

	strncpy (tmp, cpt + len, 128);
	tmp [127] = '\0';
	if ((p = strtok (tmp, " \t\n;")) != NULL &&
	    strcasecmp(p, word2) == 0)
	    return (1);
    }
    return (0);
}





