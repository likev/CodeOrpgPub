/**************************************************************************

      Module: rms_ptmgr.h

 Description: 

       Notes: 

 **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/07/13 21:44:05 $
 * $Id: rms_ptmgr.h,v 1.7 2007/07/13 21:44:05 garyg Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
#ifndef RMS_PTMGR_H
#define RMS_PTMGR_H

#include <lb.h>
#include <en.h>


#define MAX_BUF_SIZE  2416 

/* Type definition to handle an unsigned 8 bit byte. */

typedef ushort HALFWORD;
typedef unsigned char UNSIGNED_BYTE;
typedef unsigned char MSG_BUF[MAX_BUF_SIZE];

/* Linear buffers */


/* Static Variables */
int rms_shutdown_flag;  /* Global shutdown flag */
int Ev_exit;                   /* Exit ORPG flag set by port manager */

/* public functions */

pid_t RMS_get_child_pid (void);

int   RMS_socket_mgr (int argc, char **argv, char *input_lb, char *output_lb, 
                      LB_attr input_lb_attr, LB_attr output_lb_attr);
int   RMS_terminate_server (void);

#endif
