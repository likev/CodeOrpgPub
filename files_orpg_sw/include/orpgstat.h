/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:31:51 $
 * $Id: orpgstat.h,v 1.65 2002/12/11 21:31:51 nolitam Exp $
 * $Revision: 1.65 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgstat.h

 Description: ORPG status public header file.

 Assumptions:

 **************************************************************************/


#ifndef ORPGSTAT_H
#define ORPGSTAT_H

#include <infr.h>
#include <orpg_def.h>
#include <vcp.h>
#include <basedata.h>
#include <mrpg.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 *
 */

#define ORPGSTAT_TASK_GET_STATUS_PID   1
#define ORPGSTAT_TASK_GET_STATUS_STATE 2

#define ORPGSTAT_TASK_GET_STATUS_BUF_TOO_SMALL (-2)

int ORPGMGR_read_task_status(Mrpg_process_table_t *entry_p) ;

#endif /* #ifndef ORPGSTAT_H */
