
/***********************************************************************

    Description: Internal include file for wb_simulator.

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/14 22:00:59 $
 * $Id: wbs_def.h,v 1.2 2007/02/14 22:00:59 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#ifndef WBS_DEF_H
#define WBS_DEF_H

#include <infr.h>

typedef struct {		/* legacy CTM header */
   short word1;
   short word2;
   short word3;
   short word4;
   short word5;
   short word6;
} CTM_header_t;


void WBSR_main ();
int WBSR_init (char *request_lb_name, char *response_lb_name, 
				char *data_lb_name, int no_ctm_header);


#endif		/* #ifndef WBS_DEF_H */
