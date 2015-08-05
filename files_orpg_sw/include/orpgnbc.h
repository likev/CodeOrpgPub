/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2006/09/08 17:37:48 $
 * $Id: orpgnbc.h,v 1.1 2006/09/08 17:37:48 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef ORPGNBC_H
#define ORPGNBC_H

#include <prod_distri_info.h>

/* special values for argument "line_ind" of function 
   ORPGNBC_send_NB_link_control_command */
#define NBC_ALL_DEDICATED	-1
#define NBC_ALL_DIAL_IN		-2
#define NBC_ALL_LINES		-3
enum {NBC_DISABLE_LINK, NBC_ENABLE_LINK, NBC_CONNECT, NBC_DISCONNECT};


int ORPGNBC_send_NB_link_control_command (int cmd, int line_ind);
int NBC_get_user_profile (int line_type, 
			int uid, int line_ind, Pd_user_entry **up);
int ORPGNBC_get_user_profile (int line_type, 
			int uid, int line_ind, Pd_user_entry **up);
int ORPGNBC_enable_disable_NB_links (int cmd, int n_lines, int *line_ind);
int ORPGNBC_request_APUP_status (int line_ind);
int ORPGNBC_connect_disconnect_NB_links (int cmd, int n_lines, int *line_ind);
int ORPGNBC_n_lines ();
int ORPGNBC_send_NB_vv_control_command (int command);


#endif /*DO NOT REMOVE!*/
