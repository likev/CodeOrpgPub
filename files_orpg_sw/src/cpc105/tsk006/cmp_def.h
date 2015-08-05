
/***********************************************************************

    Description: cm_ping private header file.

***********************************************************************/

/* 
 * RCS info
 * $Author Jing $
 * $Locker:  $
 * $Date: 2004/01/29 22:39:18 $
 * $Id: cmp_def.h,v 1.4 2004/01/29 22:39:18 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

#ifndef CMP_DEF_H

#define CMP_DEF_H

#include <time.h>

#define NAME_LEN 128		/* length of name strings */

typedef struct {
    int qtime;				/* quiet (no response) time */
    int addr;				/* remote host address */
    time_t reg_time;			/* registration time */
} Remote_host;


int CMP_initialize (int keepalive_time);
void CMP_check_connect (int n_rhosts, Remote_host *rhosts);
void CMP_output (char *buf, int size);
int CMP_terminate ();

void CMPT_process_input (char *buf, int len);
void CMPP_process_input (char *buf, int len);
void CMPT_check_TCP ();
void CMPP_check_processes ();


#endif		/* #ifndef CMP_DEF_H */

