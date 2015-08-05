/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2009/03/17 16:45:39 $
* $Id: nds.h,v 1.5 2009/03/17 16:45:39 jing Exp $
* $Revision: 1.5 $
* $State: Exp $
*/

/***********************************************************************

    Description: Public include file for nds (node service).

***********************************************************************/


#ifndef NDS_H
#define NDS_H


enum {				/* LB message IDs */
    NDS_PROC_LIST, 		/* List of processes to monitor. A null 
				   terminated string of process names separated 				   by space, tab or line return. */
    NDS_PS_REQ,			/* A message of any non-empty contents. This
				   causes nds to get and publish process 
				   resource utilization. */
    NDS_PROC_STATUS,		/* Process status info. A list of Ps_struct
				   sorted in terms of process name. This is 
				   published on request. */
    NDS_PROC_TABLE		/* Process status info. A list of Ps_struct
				   sorted in terms of process name. This is 
				   published on changes of monitored process
				   creation/termination. The same message as 
				   NDS_PROC_STATUS except the status may not be
 				   up to date. */
};


typedef struct {			/* process status struct */
    int size;				/* size of this including following
					   data (int aligned) */
    int name_off;			/* process name location in this. */
    int cmd_off;			/* command line location in this. */
    int cpu;				/* CPU time in ms (% 0x7fffffff) */
    int mem;				/* memory used in K (1024) bytes */
    int pid;				/* process id */
    time_t st_t;			/* process starting time */
    time_t info_t;			/* time getting the info */
    int swap;				/* swap space used */
} Nds_ps_struct;

#endif		/* #ifndef NDS_H */
