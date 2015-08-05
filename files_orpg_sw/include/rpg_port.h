/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

***********************************************************************/

/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/10/24 18:51:42 $
 * $Id: rpg_port.h,v 1.37 2007/10/24 18:51:42 cmn Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 * $Log: rpg_port.h,v $
 * Revision 1.37  2007/10/24 18:51:42  cmn
 * Check in new C Lib changes for DP - cjh for ss 10/24/07
 *
 * Revision 1.36  2007/05/17 16:03:03  steves
 * issue 3-050
 *
 * Revision 1.35  2006/01/19 23:57:12  steves
 * issue 2-744
 *
 * Revision 1.34  2005/12/27 20:07:46  steves
 * issue 2-879.
 *
 * Revision 1.33  2005-12-27 09:59:32-06  steves
 * issue 2-879
 *
 * Revision 1.32  2005/04/11 19:33:40  steves
 * issue 2-591
 *
 * Revision 1.31  2004/12/30 21:48:44  steves
 * issue 2-534
 *
 * Revision 1.30  2004/12/21 15:24:36  steves
 * issue 2-566
 *
 * Revision 1.29  2003/12/11 20:54:18  ccalvert
 * convert to new dea adaptation data format
 *
 * Revision 1.27  2003/07/03 20:36:08  ccalvert
 * integrate MDA
 *
 * Revision 1.26  2003/06/20 21:14:36  ccalvert
 * add MPDA
 *
 * Revision 1.25  2002/06/07 23:23:21  steves
 * issue 1-955
 *
 * Revision 1.24  2002/02/07 22:18:25  steves
 * CCR NA01-33001
 *
 * Revision 1.23  2000/01/12 22:52:44  steves
 * fix
 *
 * Revision 1.22  1999/10/18 16:05:51  steves
 * rename some macros
 *
 * Revision 1.21  1999/08/03 21:46:27  steves
 * remove PRODGEN
 *
 * Revision 1.20  1999/07/27 21:28:55  steves
 * changes to support event processing
 *
 * Revision 1.19  1999/06/16 21:50:46  steves
 * NO COMMENT SUPPLIED
 *
 * Revision 1.18  1998/06/22 21:13:50  steves
 * modefy
 *
 * Revision 1.17  1998/03/02 21:39:05  steves
 * modefy
 *
 * Revision 1.16  1998/02/27 21:56:22  steves
 * modefy
 *
 * Revision 1.15  1997/10/23 14:14:10  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.14  1997/07/03 15:05:02  steves
 * modefy
 *
 * Revision 1.13  1997/06/20 21:14:56  steves
 * modefy
 *
 * Revision 1.12  1997/06/13 16:03:46  steves
 * modefy
 *
 * Revision 1.11  1997/04/02 22:38:30  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.10  1997/03/15 22:59:56  steves
 *
 * Revision 1.9  1997/03/04 19:18:43  dodson
 * librpg routines name changes
 *
 * Revision 1.8  96/10/24  21:56:16  21:56:16  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.6  96/09/18  21:10:27  21:10:27  jing (Zhongqi Jing)
 * 
 * Revision 1.2  1996/06/25 14:40:51  dodson
 * Build9/PORT update
 *
 */

#ifndef RPG_PORT_H
#define RPG_PORT_H

#include <prod_user_msg.h>

enum {INT_PROD, RPG_PROD};      /* values for Prod_header.type */

/* message ids in the adaptation Buffer File */
#define RDACNT	1	/* 65188 bytes */
#define PRODSEL	3	/* 300 bytes */
#define COLRTBL 7	/* 18572 bytes */
#define ENVIRON 8	/* 452 bytes */

/* task type numbers */
#define TASK_ELEVATION_BASED	1
#define TASK_VOLUME_BASED	2 
#define TASK_TIME_BASED		3
#define TASK_RADIAL_BASED	4
#define TASK_EVENT_BASED	5

/* these are the old macro names */
#define ELEVATION_BASED		TASK_ELEVATION_BASED
#define VOLUME_BASED		TASK_VOLUME_BASED 
#define TIME_BASED		TASK_TIME_BASED
#define RADIAL_BASED		TASK_RADIAL_BASED
#define EVENT_BASED		TASK_EVENT_BASED

/* adaptation timing values */
#define ADPT_UPDATE_BOE		1
#define ADPT_UPDATE_BOV		2
#define ADPT_UPDATE_ON_CHANGE	3
#define ADPT_UPDATE_WITH_CALL	4
#define ADPT_UPDATE_ON_EVENT	5

/* these are the old macro names */
#define BEGIN_ELEVATION 	ADPT_UPDATE_BOE		
#define BEGIN_VOLUME		ADPT_UPDATE_BOV		
#define ON_CHANGE 		ADPT_UPDATE_ON_CHANGE	
#define WITH_CALL		ADPT_UPDATE_WITH_CALL	
#define WITH_EVENT 		ADPT_UPDATE_ON_EVENT	

/* ITC timing values */
#define ITC_WITH_EVENT        	-5
#define ITC_ON_EVENT        	-4
#define ITC_BEGIN_ELEVATION	-3
#define ITC_BEGIN_VOLUME	-2
#define ITC_ON_CALL    		-1

/* ITC I/O Operations */
#define ITC_READ_OPERATION       0
#define ITC_WRITE_OPERATION      1

/* input/output data types */
#define UNDEFINED_DATA		 -1
#define ELEVATION_DATA		 TYPE_ELEVATION
#define VOLUME_DATA		 TYPE_VOLUME
#define RADIAL_DATA		 TYPE_RADIAL
#define DEMAND_DATA		 TYPE_ON_DEMAND
#define TIME_DATA                TYPE_TIME
#define EXTERNAL_DATA            TYPE_EXTERNAL
#define REQUEST_DATA             TYPE_ON_REQUEST

/* argument for RPG_wait_act */
#define WAIT_DRIVING_INPUT	0
#define WAIT_ALL		WAIT_DRIVING_INPUT
#define WAIT_ANY_INPUT		1

/* range for the msg id of the ITC */
#define ITC_IDRANGE	100

/* minimum LB id of the ITC */
#define ITC_MIN		1000

/* events types */
#define EVT_ANY_INPUT_AVAILABLE             -1 
#define EVT_CFCPROD_REPLAY_PRODUCT_REQUEST  -2 
#define EVT_USER_NOTIFICATION               -3
#define EVT_WAIT_FOR_EVENT_TIMEOUT          -4
#define EVT_REPLAY_PRODUCT_REQUEST          -5

/* parameter definition for "user_array". */
#define UA_NUM_FIELDS           10
#define UA_NUM_PARMS             NUM_PROD_DEPENDENT_PARAMS
#define UA_PROD_CODE             0
#define UA_DEP_PARM_1            1
#define UA_DEP_PARM_2            2
#define UA_DEP_PARM_3            3
#define UA_DEP_PARM_4            4
#define UA_DEP_PARM_5            5
#define UA_DEP_PARM_6            6
#define UA_ELEV_INDEX            7
#define UA_REQ_NUMBER            8
#define UA_SPARE                 9

/* query fields. */
#define QUERY_VOL_TIME		RPGP_VOLT
#define QUERY_ELEV    		RPGP_ELEV
#define QUERY_VOL_TIME_RANGE	ORPGDBM_VOL_TIME_RANGE
#define QUERY_ELEV_RANGE	ORPGDBM_ELEV_RANGE
#define QUERY_END_LIST		-999

/* Special return values for functions. */
#define RPG_BUF_NOT_FOUND       -1
#define RPGC_BUF_NOT_FOUND      RPG_BUF_NOT_FOUND

/* Macro definitions for registering moments. */
#define UNSPECIFIED_MOMENTS      0
#define REF_MOMENT               1
#define VEL_MOMENT               2
#define WID_MOMENT               4

#endif 		/* #ifndef RPG_PORT_H */


