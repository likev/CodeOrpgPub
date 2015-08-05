
/***********************************************************************

    Description: Internal include file for ftnpp.

***********************************************************************/

/* 
 * RCS info
 * $Author: cm $
 * $Locker:  $
 * $Date: 1997/05/15 14:33:02 $
 * $Id: ftnpp_def.h,v 1.7 1997/05/15 14:33:02 cm Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  

#ifndef FTNPP_DEF_H

#define FTNPP_DEF_H

#define NAME_LEN	128	/* max length of names */

enum {RDP_NOT_INCLUDED, RDP_INCLUDED};
			/* return values of RDP_check_pattern */

void FTNPP_task_terminate (int status, int line_num, char *fname);

int REP_match_tocken (char *tok, char *line);
int REP_argument (char *argv);
int REP_processing (char *line, int line_num, char *fname);
void REP_reset_N_lhex ();
int REP_check_legal_char (char cc);
void REP_need_truncate ();

int RDP_check_section (char *fname, char *section);
int RDP_special_statement (char *cpt, int level, int line_num, char *fname);


#endif		/* #ifndef FTNPP_DEF_H */
