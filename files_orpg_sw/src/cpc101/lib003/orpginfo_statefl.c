/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/28 17:19:11 $
 * $Id: orpginfo_statefl.c,v 1.15 2006/09/28 17:19:11 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/*************************************************************************
      Module:  orpginfo_statefl.c

 Description:
        This file provides ORPG library routines for accessing the State
        File data in ORPGDAT_RPG_INFO.

        Functions that are public are defined in alphabetical order at the
        top of this file and are identified by a prefix of
        "ORPGINFO_statefl_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

        Any "convenience" routines are placed at the bottom of this file.

        The code for accessing the State File Shared Data is maintained
        in a separate source file (at this time, orpginfo_statefl_shared.c).

 **************************************************************************/



#include <debugassert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>            /* memcmp()                                */

#include <infr.h>
#include <orpg.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Global Variables
 */
static char Rpg_status_strings[][12] =
        {"Unknown", "Restart", "Operate", "Standby", "Shutdown", "Test"} ;
                               /* string representation of RPG Status     */


/*
 * Static Function Prototypes
 */
static int Rpg_status_strings_ndx(unsigned int rpg_status) ;
static int Update_statefl(Orpginfo_statefl_t *statefl_p, unsigned char notify) ;
static int rpgstatus_has_changed = 0;		/*  1 if ORPGINFO_STATEFL_MSGID has changed, 0 otherwise */

/************************************************************************
 *									*
 *	Description: RPG status notification callback			*
 *									*
 *	Return:      NONE						*
 *									*
 ************************************************************************/
static void ntf_rpgstatus_callback (int	fd, LB_id_t msg_id, int	msg_info, void	*arg)
{
	rpgstatus_has_changed = 1;
}


/*******************************************************************************
   Description:
      Get or set the RPG State File RPG Status data.
  
      The RPG Status data in the RPG State File message will be updated if
      the action indicates this is appropriate.
  
      The final RPG Status data are placed in the caller-provided storage.
  
      The string pointer is updated to point to the corresponding
      string representation of the RPG Status.
  
      Valid RPG Status values are specified by the ORPGINFO_STATEFL_RPGSTAT_*
      macros defined in orpginfo.h.  These currently include the following:
  
 	ORPGINFO_STATEFL_RPGSTAT_RESTART
 	ORPGINFO_STATEFL_RPGSTAT_OPERATE
 	ORPGINFO_STATEFL_RPGSTAT_STANDBY
 	ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN
 	ORPGINFO_STATEFL_RPGSTAT_TEST

	ORPGEVT_RPG_STATUS_CHANGE
	   Posted after successful State File update.
	ORPGEVT_RPG_STATEFL_CHANGE
    	   Posted after successful State File update.
   
      NOTE: The event is posted only after updating the Current RPG Status 
            value.  i.e., we do not post the event when updating either the 
            Previous RPG Status value of the Commanded RPG Status value.
  
   Inputs:
      action - A flag indicating action to take:

	Flag Macro		Action
	ORPGINFO_STATEFL_GET	Get the RPG status value.
	ORPGINFO_STATEFL_SET	Set the RPG status value.
  
      which - A flag indicating which status field is of interest:

	Flag Macro				Description
 	ORPGINFO_STATEFL_RPG_STATUS_CMDED	The commanded RPG status.
 	ORPGINFO_STATEFL_RPG_STATUS_CUR		The current RPG status.
 	ORPGINFO_STATEFL_RPG_STATUS_PREV	The previous RPG status.
  
      NOTE: the caller cannot directly set the Previous RPG Status value.
  
      status_p - Pointer to caller-provided storage for the status
                 data.  When setting the RPG status value, this must point
                 to a valid RPG status value.
   
      string_p - Pointer to caller-provided string-pointer.
  
   Returns:
      0 - success
     -1 - failure
   
      Possible Failures:
 	Invalid <cite>action</cite> argument.
 	Attempt to set the Previous RPG Status value.
 	Unable to open the RPG Information datastore.
 	Unable to read the RPG State File message.
 	Unable to post the RPG Status Change event message.
  
**************************************************************************/
int ORPGINFO_statefl_rpg_status(int action, int which, unsigned int *status_p,
                                char **string_p){

    unsigned char notify ;     /* flag; if set, indicates we want data    */
                               /* consumers to be notified after the State*/
                               /* File has been updated ...               */
    int retval ;
    static Orpginfo_statefl_t *statefl_p = NULL;
    static int registered_for_rpgstatus_changes = 0;

    /*  Register for LB notification of changes if we haven't registered already */
    if (!registered_for_rpgstatus_changes)
    {
	int status;

	if( ((status = ORPGDA_open( ORPGDAT_RPG_INFO, LB_WRITE | LB_READ )) < 0 )
				||
	    ((status = ORPGDA_UN_register( ORPGDAT_RPG_INFO, ORPGINFO_STATEFL_MSGID, 
		                           ntf_rpgstatus_callback)) < 0) )
	{
		LE_send_msg (GL_ERROR,
			"ORPGDA_UN_register (ORPGDAT_RPG_INFO) failed (%d)\n", status);
		return (status);
	}
        registered_for_rpgstatus_changes = 1;
    }



    if ((action != ORPGINFO_STATEFL_GET) && (action != ORPGINFO_STATEFL_SET)) {
        LE_send_msg(GL_ERROR, "Bad \"action\" value: %d", action) ;
        return(-1) ;
    }
 
    /*
     * the Previous RPG Status value is automatically updated whenever
     * the  Current RPG Status changes ...
     */
    if ((which == ORPGINFO_STATEFL_RPG_STATUS_PREV)
               &&
        (action == ORPGINFO_STATEFL_SET)) {
        LE_send_msg(GL_ERROR, "Cannot directly SET _RPG_STATUS__PREV!") ;
        return(-1) ;
    }

    *string_p = (char *) &(Rpg_status_strings[Rpg_status_strings_ndx(0)]) ;


    if ((rpgstatus_has_changed) || (statefl_p == NULL))
    {
	rpgstatus_has_changed = 0;

	/*  Free old information */
	if (statefl_p != NULL)
	{
	   free(statefl_p);
	   statefl_p = NULL;
	}

        retval = ORPGDA_read(ORPGDAT_RPG_INFO, (char **) &statefl_p, LB_ALLOC_BUF,
                         (LB_id_t) ORPGINFO_STATEFL_MSGID) ;

        if (retval <= 0) {
            LE_send_msg(GL_ORPGDA(retval),
                    "ORPGDA_read(ORPGINFO_STATEFL_MSGID) failed: %d", retval) ;
            return(-1) ;
        }
    }

    if (action == ORPGINFO_STATEFL_GET) {

        if (which == ORPGINFO_STATEFL_RPG_STATUS_CMDED) {
            *status_p = statefl_p->rpg_status_cmded ;
        }
        else if (which == ORPGINFO_STATEFL_RPG_STATUS_CUR) {
            *status_p = statefl_p->rpg_status ;
        }
        else if (which == ORPGINFO_STATEFL_RPG_STATUS_PREV) {
            *status_p = statefl_p->rpg_status_prev ;
        }

        *string_p = (char *)
                    &(Rpg_status_strings[Rpg_status_strings_ndx(*status_p)]) ;
        /*
         * We are through!
         */
        return(0) ;

    } /*endif we are simply getting the flag*/

    if (which == ORPGINFO_STATEFL_RPG_STATUS_CMDED) {
        statefl_p->rpg_status_cmded = *status_p ;
    }
    else if (which == ORPGINFO_STATEFL_RPG_STATUS_CUR) {

        /*
         * We also update the Previous RPG Status value ...
         */
        statefl_p->rpg_status_prev = statefl_p->rpg_status ;
        statefl_p->rpg_status = *status_p ;
 
    }

    if (which == ORPGINFO_STATEFL_RPG_STATUS_CUR) {
        notify = 1 ;
    }
    else {
        notify = 0 ;
    }

    retval = Update_statefl(statefl_p, notify) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,"Update_statefl() failed: %d", retval) ;
        return(-1) ;
    }

    if (which == ORPGINFO_STATEFL_RPG_STATUS_CMDED) {
        *status_p = statefl_p->rpg_status_cmded ;
    }
    else if (which == ORPGINFO_STATEFL_RPG_STATUS_CUR) {
        *status_p = statefl_p->rpg_status ;
    }

    *string_p = (char *)
                &(Rpg_status_strings[Rpg_status_strings_ndx(*status_p)]) ;

    return(0) ;

/*END of ORPGINFO_statefl_rpg_status()*/
}

/**************************************************************************
   Description: 
      Determine RPG Status strings index for specified RPG Status

   Input: 
      RPG Status

   Output:
       none

   Returns: 
      The string index upon success

   Notes:
**************************************************************************/
static int Rpg_status_strings_ndx(unsigned int rpg_status){

    int ndx = 0 ;

    if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_RESTART) {
        ndx = 1 ;
    }
    else if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_OPERATE) {
        ndx = 2 ;
    }
    else if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_STANDBY) {
        ndx = 3 ;
    }
    else if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN) {
        ndx = 4 ;
    }
    else if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_TEST) {
        ndx = 5 ;
    }

    return(ndx) ;

/*END of Rpg_status_strings_ndx()*/
}

/**************************************************************************
   Description:
      Update the State File.  Post appropriate EN message.

   Input: 
      statefl_p - pointer to State File data
      notify - "notify data consumers" flag

   Output: 
      The State File LB message is updated; the EN message is posted

   Returns: 
      0 upon success; -1 otherwise

   Notes:
 **************************************************************************/
static int Update_statefl(Orpginfo_statefl_t *statefl_p, unsigned char notify){

    int retval ;


    /*
     * Update the RPG State File ...
     */
    retval = ORPGDA_write(ORPGDAT_RPG_INFO, (char *) statefl_p,
                          (int) sizeof(*statefl_p),
                          (LB_id_t) ORPGINFO_STATEFL_MSGID) ;
    if (retval != (int) sizeof(*statefl_p)) {
        LE_send_msg(GL_ORPGDA(retval),
                    "ORPGDA_write(State File) failed: %d", retval) ;
        return(-1) ;
    }

    if (notify) {

        Orpginfo_rpg_status_change_evtmsg_t evtmsg ;

        /*
         * Post the RPG Status Change event ...
         *
         * NOTE: I assume the sender does not need to receive the event
         *
         */
        (void) memset(&evtmsg, 0, sizeof(evtmsg)) ;
        evtmsg.rpg_status = statefl_p->rpg_status ;

        retval = EN_post(ORPGEVT_RPG_STATUS_CHANGE, &evtmsg,
                         sizeof(evtmsg), 0) ;
        if (retval < 0) {
            LE_send_msg(GL_EN(retval),
                        "EN_post(RPG Status Change %d) failed: %d",
                        (int) evtmsg.rpg_status, retval) ;
            return(-1) ;
        }


    } /*endif we need to post an EN message*/

    return(0) ;

/*END of Update_statefl()*/
}

/**************************************************************************
 * Convenience functions                                                  *
 **************************************************************************/


/**************************************************************************
   Description:
      Get the State File RPG Status value.
  
      This is an ORPGINFO_statefl_rpg_status() convenience function.
  
      Upon success, the RPG State File RPG Status value will be
      placed in the caller-provided storage.  The ORPGINFO_STATEFL_RPGSTAT_*
      bit-flag macros defined in orpginfo.h may be used to examine the returned
      value.  At this time, these macros include the following:
  
 	ORPGINFO_STATEFL_RPGSTAT_RESTART
 	ORPGINFO_STATEFL_RPGSTAT_OPERATE
 	ORPGINFO_STATEFL_RPGSTAT_STANDBY
 	ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN
   
      Although these macros define bit-flags, the status value will correspond
      be equivalent to only of these macros (i.e., we don't OR them together).
  
   Inputs:
      rpgsatt_p - Pointer to caller-provided storage for the RPG State
                  File RPG Status value.
   
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
  
      Possible Failures:
 	Unable to open the RPG Information datastore.
 	Unable to read the RPG State File message.
   
*****************************************************************************/
int ORPGINFO_statefl_get_rpgstat(unsigned int *rpgstat_p){

    char *status_string ;

    return(ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                       ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                       rpgstat_p, &status_string)) ;

/*END of ORPGINFO_statefl_get_rpgstat()*/
}
