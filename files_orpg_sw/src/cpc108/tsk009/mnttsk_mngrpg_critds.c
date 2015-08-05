/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/03/31 17:13:14 $
 * $Id: mnttsk_mngrpg_critds.c,v 1.37 2005/03/31 17:13:14 jing Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* gethostname(), unlink()                 */
#include <string.h>            /* strncpy()                               */

#include <sys/types.h>         /* stat(), fstat()                         */
#include <sys/stat.h>          /* stat(), fstat()                         */

#include "mnttsk_mngrpg_def.h"

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Global Variables
 */

/*
 * Static Function Prototypes
 */
static int Init_critical() ;
static int Purge_syslog_msgs(void) ;


/*************************************************************************

   Description:
      Control module for this maintenance task.

   Inputs:
      startup_action - startup mode.

   Outputs:

   Returns:
      Returns negative value or exits with non-zero exit code depending
      on error, or returns 0 on success.

   Notes:

************************************************************************/
int MNTTSK_MNGRPG_CRITDS_maint(int startup_action){

    int retval ;

    if (startup_action == STARTUP) {

        LE_send_msg(GL_INFO,
                    "Critical Datastores Maint. INIT FOR STARTUP DUTIES:") ;
        LE_send_msg(GL_INFO,
                    "\t 1. Initialize State File (ORPGDAT_RPG_INFO).") ;
        LE_send_msg(GL_INFO,
                    "\t 2. RPG Endian Value (ORPGDAT_RPG_INFO).") ;
        LE_send_msg(GL_INFO,
                    "\t 3. Init Baseline Adaptation Data.") ;

        retval = Init_critical() ;
        if (retval < 0){

            LE_send_msg(GL_INFO,"Init_critical() failed: %d", retval) ;
            return(-1) ;
        }
        else
            LE_send_msg(GL_INFO,"Init_critical() succeeded") ;

    }
    else if (startup_action == RESTART) {

        retval = MNTTSK_MNGRPG_RPGINFO_init( MNTTSK_MNGRPG_RPGINFO_INIT_STATEFL_MSG ); 
        if( retval < 0 ){

           LE_send_msg(GL_INFO,
                       "Data ID %d: MNTTSK_MNGRPG_RPGINFO_init(_INIT_STATEFL_MSG) failed: %d",
                       ORPGDAT_RPG_INFO, retval) ;
           exit(2);
        }
        else
           LE_send_msg (GL_INFO, 
               "MNTTSK_MNGRPG_RPGINFO_init(_RPGINFO_INIT_STATEFL_MSG) succeeded");
    }
    else{

        if (startup_action == CLEAR_SYSLOG || startup_action == CLEAR_ALL) {

           LE_send_msg(GL_INFO,"\t  Purging System Status Log Messages.") ;

           retval = Purge_syslog_msgs() ;
           if (retval < 0) {

               LE_send_msg(GL_INFO,
                           "Data ID %d: Purge_syslog_msgs() failed: %d",
                           ORPGDAT_SYSLOG, retval) ;
               exit(2) ;

           }

        }

        if (startup_action == CLEAR_STATEFILE || startup_action == CLEAR_ALL) {

           LE_send_msg(GL_INFO,"\t  Initialize State File (ORPGDAT_RPG_INFO)") ;
           retval = ORPGDA_write( ORPGDAT_RPG_INFO, NULL, 0, ORPGINFO_STATEFL_SHARED_MSGID );
           if( retval < 0 ){

              LE_send_msg(GL_INFO, "Data ID %d: Purge of ORPGINFO_STATEFL_SHARED_MSGID Failed (%d)\n",
                       ORPGDAT_RPG_INFO, retval) ;
              exit(2);

           }

           retval = MNTTSK_MNGRPG_RPGINFO_init( MNTTSK_MNGRPG_RPGINFO_INIT_STATEFL_MSG ); 
           if( retval < 0 ){

              LE_send_msg(GL_INFO,
                          "Data ID %d: MNTTSK_MNGRPG_RPGINFO_init(_INIT_STATEFL_MSG) failed: %d",
                          ORPGDAT_RPG_INFO, retval) ;
              exit(2);

           }

       }

    }  

    return(0) ;

/*END of MNTTSK_MNGRPG_CRITDS_maint()*/
}

/*************************************************************************

   Description:
      Initializes RPG state file information and copies ORPGDAT_ADAPTATION
      to ORPGDAT_VASELINE_ADAPTATION.

   Inputs:

   Outputs:

   Returns:
      Returns negative value on error, or 0 on success.

   Notes:

************************************************************************/
static int Init_critical(){

    int retval ;

    /*
     * Initialize the _RPG_INFO datastore ...
     *
     * RPG Information
     *     State File
     *     Endian Value
     */
    retval = MNTTSK_MNGRPG_RPGINFO_init(MNTTSK_MNGRPG_RPGINFO_INIT_ALL_MSGS) ;
    if (retval < 0){

        LE_send_msg(GL_INFO,
        "Data ID %d: MNTTSK_MNGRPG_RPGINFO_init(_INIT_ALL_MSGS) failed: %d",
                    ORPGDAT_RPG_INFO, retval) ;
        return(-1) ;

    }
    else
        LE_send_msg(GL_INFO,"Data ID %d: datastore initialized",
                    ORPGDAT_RPG_INFO) ;

    return(0) ;

/*END of Init_critical()*/
}

/*************************************************************************

   Description:
      Removes all messages from the System Status Log.

   Inputs:

   Outputs:

   Returns:
      Returns negative value on error, or 0 on success.

   Notes:

************************************************************************/
static int Purge_syslog_msgs(void){

    int lbd ;
    const char *pathname ;
    int retval ;

    /*
     * We must open the _SYSLOG datastore for writing ...
     */
    pathname = ORPGCFG_dataid_to_path(ORPGDAT_SYSLOG, "") ;
    if (pathname == NULL){

        LE_send_msg(GL_INFO,
                    "ORPGCFG_dataid_to_path(_SYSLOG) failed!") ;
        return(-1) ;

    }

    errno = 0 ;
    lbd = LB_open((char *) pathname, LB_WRITE, (LB_attr *) NULL) ;
    if (lbd < 0) {
        LE_send_msg(GL_INFO,
                    "LB_open(_SYSLOG) failed: %d (errno %d)",
                    lbd, errno) ;
        return(-1) ;
    }

    retval = LB_clear(lbd, LB_ALL) ;
    if (retval < 0) {
        LE_send_msg(GL_INFO,"LB_clear(_SYSLOG) failed: %d",
                    retval) ;
        (void) LB_close(lbd) ;
        return(-1) ;
    }

    (void) LB_close(lbd) ;

    LE_send_msg(((GL_GLOBAL) | 17), "Syslog Log Messages purged: %d messages purged",
                retval) ;

    return(0) ;

/*END of Purge_syslog_msgs()*/
}
