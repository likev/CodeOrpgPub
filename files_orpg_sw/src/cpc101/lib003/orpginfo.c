/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:12:55 $
 * $Id: orpginfo.c,v 1.32 2002/12/11 21:12:55 nolitam Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */

/*************************************************************************
      Module:  orpginfo.c

  Created by: Arlis Dodson

 Description:
        This file provides ORPG library routines for accessing the data
        in ORPGDAT_RPG_INFO.

        Functions that are public are defined in alphabetical order at the
        top of this file and are identified by a prefix of "ORPGINFO_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

        Any "convenience" routines are placed at the bottom of this file.

        SEE ALSO: orpginfo_nlist.c orpginfo_statefl.c

 Interruptible System Calls:
	TBD

 Memory Allocation:
	None

 Assumptions:
	TBD

 **************************************************************************/



#include <debugassert.h>
#include <stdio.h>
#include <stdlib.h>

#include <infr.h>
#include <orpg.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Global Variables
 */


/*
 * Static Function Prototypes
 */


/***
 *** ORPGINFO_rpg_status() convenience functions ...
 ***/


/**
  * Is the State File RPG Status Restart?
  *
  * This is an ORPGINFO_rpg_status() convenience function.
  *
  * @author ABDodson
  *
  * @param void  This routine accepts no arguments.
  *
  * @return
<ul>
<li> 1 - the State File RPG Status corresponds to Restart
<li> 0 - the State File RPG Status does <strong>not</strong> correspond to Restart
</ul>
  *
  * Possible Failures:
  *
  * Refer to ORPGINFO_rpg_status()
  *
  */
unsigned char ORPGINFO_is_rpgstatus_restart(void)
{
    int retval ;
    unsigned int rpg_status ;
    char *rpg_status_string ;
    unsigned char yesno = 0 ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &rpg_status_string) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR) failed: %d",
                    retval) ;
        return(yesno) ;
    }

    if (rpg_status & ORPGINFO_STATEFL_RPGSTAT_RESTART) {
        yesno = 1 ;
    }

    return(yesno) ;

/*END of ORPGINFO_is_rpgstatus_restart()*/
}



/**
  * Is the State File RPG Status Operate?
  *
  * This is an ORPGINFO_rpg_status() convenience function.
  *
  * @author ABDodson
  *
  * @param void  This routine accepts no arguments.
  *
  * @return
<ul>
<li> 1 - the State File RPG Status corresponds to Operate
<li> 0 - the State File RPG Status does <strong>not</strong> correspond to Operate
</ul>
  *
  * Possible Failures:
  *
  * Refer to ORPGINFO_rpg_status()
  *
  */
unsigned char ORPGINFO_is_rpgstatus_operate(void)
{
    int retval ;
    unsigned int rpg_status ;
    char *rpg_status_string ;
    unsigned char yesno = 0 ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &rpg_status_string) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR) failed: %d",
                    retval) ;
        return(yesno) ;
    }

    if (rpg_status & ORPGINFO_STATEFL_RPGSTAT_OPERATE) {
        yesno = 1 ;
    }

    return(yesno) ;

/*END of ORPGINFO_is_rpgstatus_operate()*/
}



/**
  * Is the State File RPG Status Standby?
  *
  * This is an ORPGINFO_rpg_status() convenience function.
  *
  * @author ABDodson
  *
  * @param void  This routine accepts no arguments.
  *
  * @return
<ul>
<li> 1 - the State File RPG Status corresponds to Standby
<li> 0 - the State File RPG Status does <strong>not</strong> correspond to Standby
</ul>
  *
  * Possible Failures:
  *
  * Refer to ORPGINFO_rpg_status()
  *
  */
unsigned char ORPGINFO_is_rpgstatus_standby(void)
{
    int retval ;
    unsigned int rpg_status ;
    char *rpg_status_string ;
    unsigned char yesno = 0 ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &rpg_status_string) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR) failed: %d",
                    retval) ;
        return(yesno) ;
    }

    if (rpg_status & ORPGINFO_STATEFL_RPGSTAT_STANDBY) {
        yesno = 1 ;
    }

    return(yesno) ;

/*END of ORPGINFO_is_rpgstatus_standby()*/
}



/**
  * Is the State File RPG Status Shutdown?
  *
  * This is an ORPGINFO_rpg_status() convenience function.
  *
  * @author ABDodson
  *
  * @param void  This routine accepts no arguments.
  *
  * @return
<ul>
<li> 1 - the State File RPG Status corresponds to Shutdown
<li> 0 - the State File RPG Status does <strong>not</strong> correspond to Shutdown
</ul>
  *
  * Possible Failures:
  *
  * Refer to ORPGINFO_rpg_status()
  *
  */
unsigned char ORPGINFO_is_rpgstatus_shutdown(void)
{
    int retval ;
    unsigned int rpg_status ;
    char *rpg_status_string ;
    unsigned char yesno = 0 ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &rpg_status_string) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR) failed: %d",
                    retval) ;
        return(yesno) ;
    }

    if (rpg_status & ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN) {
        yesno = 1 ;
    }

    return(yesno) ;

/*END of ORPGINFO_is_rpgstatus_shutdown()*/
}



/**
  * Is the State File RPG Status Test?
  *
  * This is an ORPGINFO_rpg_status() convenience function.
  *
  * @author ABDodson
  *
  * @param void  This routine accepts no arguments.
  *
  * @return
<ul>
<li> 1 - the State File RPG Status corresponds to Test
<li> 0 - the State File RPG Status does <strong>not</strong> correspond to Test
</ul>
  *
  * Possible Failures:
  *
  * Refer to ORPGINFO_rpg_status()
  *
  */
unsigned char ORPGINFO_is_rpgstatus_test(void)
{
    int retval ;
    unsigned int rpg_status ;
    char *rpg_status_string ;
    unsigned char yesno = 0 ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &rpg_status_string) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR) failed: %d",
                    retval) ;
        return(yesno) ;
    }

    if (rpg_status & ORPGINFO_STATEFL_RPGSTAT_TEST) {
        yesno = 1 ;
    }

    return(yesno) ;

/*END of ORPGINFO_is_rpgstatus_test()*/
}
