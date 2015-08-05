
/***********************************************************************

    Internal include file for process_adapt_data.

***********************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/02 19:33:56 $
 * $Id: mad_def.h,v 1.4 2009/03/02 19:33:56 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

#ifndef MAD_DEF_H

#define MAD_DEF_H


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <infr.h>
#include <orpg.h>
#include <alert_threshold.h>

int MADRL_read_legacy_data (char *lb_name);
int MADRL_next_legacy_values (char **de_id, int *type, void **values);
void Get_encrypted_passwd (char *epwd, char *opwd);



#endif		/* MAD_DEF_H */
