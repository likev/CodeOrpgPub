/*************************************************************************

    Description:
       Routines for managing the Routine Product Request-related items.

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2001/04/30 15:17:48 $
 * $Id: mnttsk_pg_rt_request.c,v 1.5 2001/04/30 15:17:48 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */


#include "mnttsk_pg_def.h"


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Global Variables
 */


/*
 * Static Function Prototypes
 */
static int Purge_msgs(void) ;

/**************************************************************************

   Description:
      On RESTART and STARTUP, the routine request messages in LB
      ORPGDAT_RT_REQUEST are purged.

   Inputs:
      startup_action - accepts STARTUP, RESTART or CLEAR.

   Outputs:

   Returns:
      Returns 0 on success or -1 if error occurred.

   Notes:

**************************************************************************/
int MNTTSK_PG_RT_REQUEST_maint(int startup_action){

    int retval ;

    if ((startup_action == STARTUP)
                    ||
        (startup_action == RESTART)
                    ||
        (startup_action == CLEAR)) {

        LE_send_msg( GL_INFO, 
                     "RT Request Maintenance.  INIT FOR STARTUP/RESTART/CLEAR DUTIES:\n" );
        LE_send_msg( GL_INFO, "\t Purge existing ORPGDAT_RT_REQUEST messages.") ;

        retval = Purge_msgs() ;
        if (retval < 0) {
            LE_send_msg( GL_INFO,"Data ID %d: Purge_msgs() Failed (%d)",
                        ORPGDAT_RT_REQUEST, retval) ;
            return(-1) ;
        }
    }

    return(0) ;

/*END of MNTTSK_PG_RT_REQUEST_maint()*/
}

/**************************************************************************

   Description:
      Removes all LB messages from data store ID ORPGDAT_RT_REQUEST.

   Inputs:

   Outputs:

   Returns:
      Returns 0 on success, -1 on error.

   Notes:

**************************************************************************/
static int Purge_msgs(void){

    int retval ;

    /* In order to be able to purge to messages, we need write permission
       for the LB. */
    retval = ORPGDA_write_permission( ORPGDAT_RT_REQUEST ) ;
    if (retval < 0){

        LE_send_msg( GL_INFO,
                    "ORPGDA_write_permission(ORPGDAT_RT_REQUEST) failed: %d\n",
                    retval) ;
        return(-1) ;

    }

    retval = ORPGDA_clear(ORPGDAT_RT_REQUEST, LB_ALL) ;
    if (retval < 0){

        LE_send_msg( GL_INFO,"ORPGDA_clear(ORPGDAT_RT_REQUEST) failed: %d",
                    retval) ;
        return(-1) ;

    }
    else 
        LE_send_msg( GL_INFO,"Data ID %d: %d msgs purged",
                     ORPGDAT_RT_REQUEST, retval) ;

    return(0) ;

/*END of Purge_msgs()*/
}
