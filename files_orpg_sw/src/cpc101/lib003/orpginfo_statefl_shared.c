/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/10 15:18:38 $
 * $Id: orpginfo_statefl_shared.c,v 1.53 2014/11/10 15:18:38 steves Exp $
 * $Revision: 1.53 $
 * $State: Exp $
 */

/*************************************************************************
 Module:  orpginfo_statefl_shared.c

 Description:
        This file provides ORPG library routines for accessing the State
        File Shared Data in ORPGDAT_RPG_INFO.

        Functions that are public are defined in alphabetical order at the
        top of this file and are identified by a prefix of
        "ORPGINFO_statefl_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

        Any "convenience" routines are placed at the bottom of this file.

 **************************************************************************/



#include <debugassert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>            /* memcmp()                                */

#include <infr.h>
#include <orpg.h>
#include <orpgda.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define NVLD_LB_DESCRIPTOR (-1)

enum {ACCESS_FLAGS, ACCESS_ALARMS, ACCESS_RPGOPST} ;


/*
 * Static Global Variables
 */
static char Undefined[] = "UNDEFINED ALARM MESSAGE";
static int Rpginfo_lbd = NVLD_LB_DESCRIPTOR ;
                               /* ORPGDAT_RPG_INFO LB file descriptor     */

static Orpginfo_statefl_shared_t shared_rpginfo_data ;  /*  Shared RPG info buffer - is only 
                                                            updated when it changes */
static int shared_rpginfo_has_changed = 1;		/*  1 - if the shared RPG info message 
                                                            has changed, 0 otherwise */
static int registered_for_rpginfo_changes = 0;		/*  1 - we have already registered for 
                                                            notification of RPG info changes, 
                                                            0 otherwise */

/*
 * Static Function Prototypes
 */
static int Access_all_bitflags(int data_type, int action,
                               unsigned int *bitflags_p) ;
static int Access_individual_bitflags(int data_type, int flag_id, int action,
                                      unsigned char *flag_p) ;
static int Access_statefl_flag(Orpginfo_statefl_flagid_t flag_id, int action,
                               unsigned char *flag_p) ;
static char *Alarm_name(int alarm_id, unsigned int *alarm_type) ;
static int Lock_statefl_shared_msg(void) ;
static int Open_rpginfo(void) ;
static int Unlock_statefl_shared_msg(void) ;
static void Update_rpgopst(Orpginfo_statefl_shared_t *shared_data) ;


/********************************************************************

   Description:
      Clear the State File PRF Selection Flag.  If successful, clear
      the PRF Paused Flag because the pause flag should only be set
      when PRF Selection Flag is set.
     
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

*********************************************************************/
int ORPGINFO_clear_prf_select(void){

    unsigned char flag ;
    int ret ;

    if( (ret = Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSELECT,
                                   ORPGINFO_STATEFL_CLR, &flag)) >= 0 ){

        if( Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                                ORPGINFO_STATEFL_CLR, &flag) >= 0 )
            LE_send_msg( GL_INFO, "PRF Selection Pause Cleared\n" );

    }

    return(ret) ;
 
/*END of ORPGINFO_clear_prf_select()*/
}

/********************************************************************

   Description:
      Clear the State File PRF Selection Paused Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

*********************************************************************/
int ORPGINFO_clear_prf_select_paused(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                               ORPGINFO_STATEFL_CLR, &flag)) ;
/*END of ORPGINFO_clear_prf_select_paused()*/
}

/********************************************************************
   Description:
      Clear the State File Spotblanking Implemented Flag.
   
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

********************************************************************/
int ORPGINFO_clear_spotblanking_implemented(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SBIMPLEM,
                               ORPGINFO_STATEFL_CLR, &flag)) ;

/*END of ORPGINFO_clear_spotblanking_implemented()*/
}

/********************************************************************
   Description:
      Clear the State File Super Resolution Enabled Flag.
   
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

********************************************************************/
int ORPGINFO_clear_super_resolution_enabled(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                               ORPGINFO_STATEFL_CLR, &flag)) ;

/*END of ORPGINFO_clear_super_res_enabled()*/
}

/********************************************************************
   Description:
      Clear the State File Clutter Mitigation Decision Enabled Flag.
   
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

********************************************************************/
int ORPGINFO_clear_cmd_enabled(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                               ORPGINFO_STATEFL_CLR, &flag)) ;

/*END of ORPGINFO_clear_cmd_enabled()*/
}

/********************************************************************
   Description:
      Clear the State File SAILS Enabled Flag.
   
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

********************************************************************/
int ORPGINFO_clear_sails_enabled(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SAILS_ENABLED,
                               ORPGINFO_STATEFL_CLR, &flag)) ;

/*END of ORPGINFO_clear_sails_enabled()*/
}


/********************************************************************
   Description:
      Is the State File Test Mode Flag set?

   Inputs:
      This routine accepts no arguments.

   Returns:
      1 - the State File Test Mode Flag is SET
      0 - the State File Test Mode Flag is CLEAR

   Notes:
     This function is supported only because some applications
     still call it.  They really should be calling the function
     ORPGINFO_statefl_rpg_status.

*********************************************************************/
unsigned char ORPGINFO_is_test_mode(void){

   unsigned char flag = 0 ;
   int retval ;
   unsigned int value;
  
   retval = ORPGINFO_statefl_get_rpgstat( &value ) ;
   if (retval < 0){

      LE_send_msg(GL_ERROR, "ORPGINFO_statefl_get_rpgstat Failed (%d)",
                   retval) ;
      return(flag) ;
   }
   
   if( (value & ORPGINFO_STATEFL_RPGSTAT_TEST) )
      flag = 1;
 
   return(flag) ;
  
/*END of ORPGINFO_is_test_mode()*/
}                                   

/*******************************************************************
   Description:
      Is the State File PRF Select Flag set?
  
   Inputs:
      This routine accepts no arguments.
  
   Returns:
      1 - the State File PRF Select Flag is SET
      0 - the State File PRF Select Flag is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_prf_select(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSELECT,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "Access_statefl_flag(_PRFSELECT, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_prf_select()*/
}

/*******************************************************************
   Description:
      Is the State File PRF Select Function paused?

   Inputs:
      This routine accepts no arguments.

   Returns:
      1 - the State File PRF Select Function is PAUSED
      0 - the State File PRF Select Function is NOT PAUSED

*******************************************************************/
unsigned char ORPGINFO_is_prf_select_paused(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "Access_statefl_flag(_PRFSF_PAUSE, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_prf_select_paused()*/
}

/*******************************************************************
   Description:
      Is the State File Spot-Blanking Implemented Flag set?

   Inputs:
      This routine accepts no arguments.
 
   Returns:
      1 - the State File Spotblanking Implemented Flag is SET
      0 - the State File Spotblanking Implemented Flag is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_spotblanking_implemented(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_SBIMPLEM,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
             "Access_statefl_flag(_SBIMPLEM, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_spotblanking_implemented()*/
}

/*******************************************************************
   Description:
      Is the State File Super Resolution Enabled Flag set?

   Inputs:
      This routine accepts no arguments.
 
   Returns:
      1 - the State File Super Resolution Enabled Flag is SET
      0 - the State File Super Resolution Enabled Flag is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_super_resolution_enabled(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
             "Access_statefl_flag(_SUPER_RES_ENABLED, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_super_resolution_enabled()*/
}

/*******************************************************************
   Description:
      Is the State File Clutter Mitigation Decision Enabled Flag set?

   Inputs:
      This routine accepts no arguments.
 
   Returns:
      1 - the State File Clutter Mitigation Decision Enabled Flag 
          is SET
      0 - the State File Clutter Mitigation Decision Enabled Flag 
          is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_cmd_enabled(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
             "Access_statefl_flag(_CMD_ENABLED, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_cmd_enabled()*/
}

/*******************************************************************
   Description:
      Is the State File SAILS Enabled Flag set?

   Inputs:
      This routine accepts no arguments.
 
   Returns:
      1 - the State File SAILS Enabled Flag is SET
      0 - the State File SAILS Enabled Flag is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_sails_enabled(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_SAILS_ENABLED,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
             "Access_statefl_flag(_SAILS_ENABLED, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_sails_enabled()*/
}


/*******************************************************************
   Description:
      Is the State File Clutter Mitigation Decision Enabled Flag set?

   Inputs:
      This routine accepts no arguments.
 
   Returns:
      1 - the State File Automated Volume Evaluation and Termination
          flag is SET
      0 - the State File Automated Volume Evaluation and Termination
          flag is CLEAR

*******************************************************************/
unsigned char ORPGINFO_is_avset_enabled(void){

    unsigned char flag = 0 ;
    int retval ;

    retval = Access_statefl_flag(ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                 ORPGINFO_STATEFL_GET, &flag) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
             "Access_statefl_flag(_AVSET_ENABLED, _GET) Failed (%d)", retval) ;
        return(flag) ;
    }

    return(flag) ;

/*END of ORPGINFO_is_avset_enabled()*/
}

/*******************************************************************
   Description:
      Set the State File PRF Selection Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

*******************************************************************/
int ORPGINFO_set_prf_select(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSELECT,
                               ORPGINFO_STATEFL_SET, &flag)) ;
/*END of ORPGINFO_set_prf_select()*/
}

/*******************************************************************
   Description:
      Set the State File PRF Selection Function Paused Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)

*******************************************************************/
int ORPGINFO_set_prf_select_paused(void){

    unsigned char flag ;
    int ret;

    if( (ret = Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSELECT,
                                   ORPGINFO_STATEFL_GET, &flag)) >= 0 ){

        /* Do not allow PRF Selection Pause if PRF Select is not active. */
        if( !flag ){

            LE_send_msg( GL_ERROR, "PRF Selection Not Set ... Pause Not Allowed\n" );
            return 0 ;

        }

    }

    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_PRFSF_PAUSED,
                               ORPGINFO_STATEFL_SET, &flag)) ;

/*END of ORPGINFO_set_prf_select_paused()*/
}


/*******************************************************************
   Description:
      Set the State File Spotblanking Implemented Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
*******************************************************************/
int ORPGINFO_set_spotblanking_implemented(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SBIMPLEM,
                               ORPGINFO_STATEFL_SET, &flag)) ;

/*END of ORPGINFO_set_spotblanking_implemented()*/
}

/*******************************************************************
   Description:
      Set the State File Super Resolution Enabled Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
*******************************************************************/
int ORPGINFO_set_super_resolution_enabled(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                               ORPGINFO_STATEFL_SET, &flag)) ;

/*END of ORPGINFO_set_super_resolution_enabled()*/
}

/*******************************************************************
   Description:
      Set the State File Clutter Mitigation Decision Enabled Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
*******************************************************************/
int ORPGINFO_set_cmd_enabled(void){

    unsigned char flag ;
    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                               ORPGINFO_STATEFL_SET, &flag)) ;

/*END of ORPGINFO_set_cmd_enabled()*/
}

/*******************************************************************
   Description:
      Set the State File SAILS Enabled Flag.

   Inputs:
      This routine accepts no arguments.

   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
*******************************************************************/
int ORPGINFO_set_sails_enabled(void){

    unsigned char flag ;
    double dtemp;
    int ret, available = 1;
    
    /* Is SAILS available for use? */
    if( (ret = DEAU_get_values( "sails.sails_available", &dtemp, 1 )) > 0 )
       available = (int) dtemp;
       
    /* If not available, simply return. */
    if( !available )
       return 0;

    return(Access_statefl_flag(ORPGINFO_STATEFL_FLG_SAILS_ENABLED,
                               ORPGINFO_STATEFL_SET, &flag)) ;

/*END of ORPGINFO_set_sails_enabled()*/
}


/*******************************************************************
   Description:
      Get the State File Flags bitflags value.
  
      Upon success, the RPG State File Flags bitflags value will be
      placed in the caller-provided storage.  The ORPGINFO_STATEFL_FLG_*
      bit-flag macros defined in orpginfo.h may be used to examine the returned
      value.  At this time, these macros include the following:
  

      ORPGINFO_STATEFL_FLG_PRFSELECT - PRF Select
      ORPGINFO_STATEFL_FLG_SBIMPLEM - Spot-Blanking Implemented
      ORPGINFO_STATEFL_FLG_PRFSF_PAUSED - PRF Select Function Paused
      ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED - Super Resolution Enabled
      ORPGINFO_STATEFL_FLG_CMD_ENABLED - Super Resolution Enabled
   
   Inputs:
      flags_p  - Pointer to caller-provided storage for the flags
                 bitflags value.
   Returns:
      0 - success
     -1 - failure
  
      Possible Failures:
  
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File Shared Data message.
   
*******************************************************************/
int ORPGINFO_statefl_get_flags(unsigned int *flags_p){

    return(Access_all_bitflags(ACCESS_FLAGS, ORPGINFO_STATEFL_GET, flags_p)) ;

/*END of ORPGINFO_statefl_get_flags()*/
}

/************************************************************************
   Description:
   Get the State File RPG Alarms bitflags value.
  
   Upon success, the RPG State File RPG Alarms bitflags value will be
   placed in the caller-provided storage.  The ORPGINFO_STATEFL_RPGALRM_*
   bit-flag macros defined in orpginfo.h may be used to examine the returned
   value.  At this time, these macros include the following:
  
      ORPGINFO_STATEFL_RPGALRM_NONE (note that this is not '0')
      ORPGINFO_STATEFL_RPGALRM_NODE_CON - Node Connectivity Failure
      ORPGINFO_STATEFL_RPGALRM_RPGCTLFL - RPG Control Task Failure
      ORPGINFO_STATEFL_RPGALRM_DBFL - Data Base Failure
      ORPGINFO_STATEFL_RPGALRM_SPARE26 - Spare           
      ORPGINFO_STATEFL_RPGALRM_MEDIAFL - Media Failure
      ORPGINFO_STATEFL_RPGALRM_WBDLS - WideBand Loadshedding
      ORPGINFO_STATEFL_RPGALRM_PRDSTGLS - Product Storage Loadshedding
      ORPGINFO_STATEFL_RPGALRM_SPARE22 - Spare           
      ORPGINFO_STATEFL_RPGALRM_SPARE21 - Spare           
      ORPGINFO_STATEFL_RPGALRM_RDAWB - RDA Wideband Alarm
      ORPGINFO_STATEFL_RPGALRM_RPGRPGFL - RPG/RPG Link Failure
      ORPGINFO_STATEFL_RPGALRM_REDCHNER - Redundant Channel Error
      ORPGINFO_STATEFL_RPGALRM_FLACCFL - File Access Failure
      ORPGINFO_STATEFL_RPGALRM_RDAINLS - RDA Radial Input Load Shed
      ORPGINFO_STATEFL_RPGALRM_RPGINLS - RPG Radial Input Load Shed
      ORPGINFO_STATEFL_RPGALRM_RPGTSKFL - RPG Task Failure
      ORPGINFO_STATEFL_RPGALRM_DISTRI - Product Distribution 
      ORPGINFO_STATEFL_RPGALRM_WBFAILRE - Wideband Failure


  Inputs:
     rpgalrm_p -  Pointer to caller-provided storage for the alarms
                  bitflags value.
   
  Returns:
     0 - success
    -1 - failure

     Possible Failures:
        Unable to open the RPG Information datastore.
        Unable to read the RPG State File Shared Data message.

**************************************************************************/
int ORPGINFO_statefl_get_rpgalrm(unsigned int *rpgalrm_p){

    return(Access_all_bitflags(ACCESS_ALARMS, ORPGINFO_STATEFL_GET, rpgalrm_p)) ;

/*END of ORPGINFO_statefl_get_rpgalrm()*/
}

/*************************************************************************
   Description:
      Get the State File RPG Operability Status value.
  
      Upon success, the RPG State File RPG Operability Status bitflags value
      will be placed in the caller-provided storage.  The
      ORPGINFO_STATEFL_RPGOPST_* bit-flag macros defined in orpginfo.h may be
      used to examine the returned value.  At this time, these macros include
      the following:
  
         ORPGINFO_STATEFL_RPGOPST_LOADSHED
         ORPGINFO_STATEFL_RPGOPST_ONLINE
         ORPGINFO_STATEFL_RPGOPST_MAR
         ORPGINFO_STATEFL_RPGOPST_MAM
         ORPGINFO_STATEFL_RPGOPST_CMDSHDN
  
      In general, the status value will correspond to an OR'ing together of
      these bit-flags.
  
   Inputs:
      rpgopst_p - Pointer to caller-provided storage for the RPG State
                  File RPG Operability Status bitflags value.
   Returns:
      0 - success
     -1 - failure (refer to LE LB messages)
  
      Possible Failures:
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File Shared Data message.

***************************************************************************/
int ORPGINFO_statefl_get_rpgopst(unsigned int *rpgopst_p){

    return(Access_all_bitflags(ACCESS_RPGOPST, ORPGINFO_STATEFL_GET, rpgopst_p)) ;

/*END of ORPGINFO_statefl_get_rpgopst()*/
}

/***************************************************************************
   Description:
      If setting the flag, the corresponding flag in the message will be updated
      with the flag value provided by the caller.  If clearing the flag, the
      corresponding flag in the message will be cleared.  Otherwise, the current
      flag value will be placed in the storage provided by the caller.
  
      In all cases, the final message flag value is placed in the storage
      provided by the caller.

   Inputs:
      flag_id - A flag indicating the State File flag of interest:
   
	Flag Macro				Description

	ORPGINFO_STATEFL_FLG_PRFSELECTPRF 	Selected.
	ORPGINFO_STATEFL_FLG_SBIMPLEM		Spot-Blanking implemented.
  
   
      flag - A flag indicating the desired action:
   
  	Flag Macro				Action

 	ORPGINFO_STATEFL_GET			Get the flag value.
 	ORPGINFO_STATEFL_SET			Set the flag value.
 	ORPGINFO_STATEFL_CLR			Clear the flag value.
  
      flag_p - Pointer to caller-provided storage for flag value.
  
   Returns:
      0 - success
     -1 - failure
  
      Possible Failures:
         Invalid action argument.
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File message from the datastore.
         Unable to lock the RPG State File message.
         Unable to write the RPG State File message into the datastore.

*******************************************************************************/
int ORPGINFO_statefl_flag( Orpginfo_statefl_flagid_t flag_id, int action,
                           unsigned char *flag_p){

    if ((action != ORPGINFO_STATEFL_GET)
                &&
        (action != ORPGINFO_STATEFL_SET)
                &&
        (action != ORPGINFO_STATEFL_CLR)) {
        LE_send_msg(GL_ERROR,
           "Bad \"action\" value: %d (not _GET, _SET, or _CLR)", action) ;
        return(-1) ;
    }

    return(Access_individual_bitflags(ACCESS_FLAGS, flag_id, action, flag_p)) ;

/*END of ORPGINFO_statefl_flag()*/
}

/****************************************************************************
   Description:
      Get, set, or clear the specified RPG State File RPG Alarms data value.
  
      The RPG Alarms data in the RPG State File message will be
      updated if the action indicates this is appropriate. 
  
      The final specified RPG Alarms data value is placed in the caller-provided
      storage.
  
      The State File RPG Operability Status Maintenance Action Required,
      Maintenance Action Mandatory, and Loadshed bitflags are set or cleared
      as required:
  
   RPG Alarm			RPG Operability Status	Description
 ORPGINFO_STATEFL_RPGALRM_NONE		N/A		this is not '0'
 ORPGINFO_STATEFL_RPGALRM_NODE_CON      MAM             Node Connectivity Failure
 ORPGINFO_STATEFL_RPGALRM_RPGCTLFL	MAM 		RPG Control Failure
 ORPGINFO_STATEFL_RPGALRM_DBFL		MAR 		Data Base Failure
 ORPGINFO_STATEFL_RPGALRM_SPARE26 	??? 		
 ORPGINFO_STATEFL_RPGALRM_MEDIAFL	MAM 		Media Failure
 ORPGINFO_STATEFL_RPGALRM_WBDLS		LDSHD 		WideBand Loadshedding
 ORPGINFO_STATEFL_RPGALRM_PRDSTGLS	LDSHD 		Product Storage Loadshedding
 ORPGINFO_STATEFL_RPGALRM_SPARE22	??? 		
 ORPGINFO_STATEFL_RPGALRM_SPARE21      	??? 		
 ORPGINFO_STATEFL_RPGALRM_RDAWB 	MAR		RDA Wideband Alarm
 ORPGINFO_STATEFL_RPGALRM_RPGRPGFL	MAR 		RPG/RPG Link Failure
 ORPGINFO_STATEFL_RPGALRM_REDCHNER	MAR 		Redundant Channel Error
 ORPGINFO_STATEFL_RPGALRM_FLACCFL	??? 		File Access Failure
 ORPGINFO_STATEFL_RPGALRM_RDAINLS	??? 		RDA Radial Input Load Shed
 ORPGINFO_STATEFL_RPGALRM_RPGINLS	??? 		RPG Radial Input Load Shed
 ORPGINFO_STATEFL_RPGALRM_RPGTSKFL	MAR 		RPG Task Failure
 ORPGINFO_STATEFL_RPGALRM_DISTRI	MAR 		Product Distribution
 ORPGINFO_STATEFL_RPGALRM_WBFAILRE  	MAM		Wideband Failure

      ORPGEVT_RPG_ALARM
         Posted after successful RPG Alarm bitflag update.

      ORPGEVT_RPG_OPSTAT_CHANGE
         Posted if RPG Operability Status bitflags affected by this RPG Alarm
         bitflag update.

   Inputs:
      alarm_id - Id of the RPG Alarm whose value is to be accessed.
  
         Valid RPG Alarms values are specified by the ORPGINFO_STATEFL_RPGALRM_*
         macros defined in orpginfo.h.  Refer to table above.
  
  
      action - A flag indicating action to take:
  
	Flag Macro		Action
 ORPGINFO_STATEFL_GET		Get the RPG Alarm value.
 ORPGINFO_STATEFL_SET		Set the RPG Alarm value.
 ORPGINFO_STATEFL_CLR		Clear the RPG Alarm value.
   
      alarm_p - Pointer to caller-provided storage for the specified
                RPG Alarm value.  The current RPG Alarm value is stored
                regardless of the specified action.
   
   Returns:
      0 - success
     -1 - failure
  
      Possible Failures:
         Invalid action argument.
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File message.

******************************************************************************/
int ORPGINFO_statefl_rpg_alarm( Orpginfo_statefl_rpgalrm_t alarm_id, int action,
                                unsigned char *alarm_p){

    if ((action != ORPGINFO_STATEFL_GET)
                &&
        (action != ORPGINFO_STATEFL_SET)
                &&
        (action != ORPGINFO_STATEFL_CLR)) {
        LE_send_msg(GL_ERROR,
           "Bad \"action\" value: %d (not _GET, _SET, or _CLR)", action) ;
        return(-1) ;
    }

    return(Access_individual_bitflags(ACCESS_ALARMS, alarm_id, action, alarm_p)) ;

/*END of ORPGINFO_statefl_rpg_alarm()*/
}

/***********************************************************************a
   Description:
      Get, set, or clear the specified RPG State File RPG Operability Status
      data value.
  
      The RPG Operability Status data in the RPG State File message will be
      updated if the action indicates this is appropriate.
  
      The final specified RPG Operability Status data value is placed in the
      caller-provided * storage.
  
      Valid RPG Operability Status values are specified by the
      ORPGINFO_STATEFL_RPGOPST_*
      macros defined in orpginfo.h.  These currently include the following:
  
 	ORPGINFO_STATEFL_RPGOPST_LOADSHED
 	ORPGINFO_STATEFL_RPGOPST_ONLINE
 	ORPGINFO_STATEFL_RPGOPST_MAR
 	ORPGINFO_STATEFL_RPGOPST_MAM
 	ORPGINFO_STATEFL_RPGOPST_CMDSHDN
  
     Note that the "online" and "commanded shutdown" bitflags are toggled
     in tandem, per Legacy RPG implementation.  That is, if one of these bitflags
     is SET, the other is CLEARed, and vice versa.
 
     The New RPG Alarms that affect the "loadshed", "maintenance action required",
     and "maintenance action mandatory" bitflags are described in the
     ORPGINFO_statefl_rpg_alarm(3) manpage.

     ORPGEVT_RPG_OPSTAT_CHANGE
        Posted after successful RPG Operability Status update.

   Inputs:
      action - A flag indicating action to take:

	Flag Macro		Action

 	ORPGINFO_STATEFL_GET	Get the RPG Operability status value.
 	ORPGINFO_STATEFL_SET	Set the RPG Operability status value.
 	ORPGINFO_STATEFL_CLR	Clear the RPG Operability status value.
  
      status_p - Pointer to caller-provided storage for the specified RPG
                 Operability Status data value.
  
   Returns:
      0 - success
     -1 - failure
 
      Possible Failures:
         Invalid action argument.
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File message.

********************************************************************************/
int ORPGINFO_statefl_rpg_operability_status(Orpginfo_statefl_rpgopst_t status_id,
                                        int action, unsigned char *status_p){


    if ((action != ORPGINFO_STATEFL_GET)
                &&
        (action != ORPGINFO_STATEFL_SET)
                &&
        (action != ORPGINFO_STATEFL_CLR)) {
        LE_send_msg(GL_ERROR,
           "Bad \"action\" value: %d (not _GET, _SET, or _CLR)", action) ;
        return(-1) ;
    }

    return(Access_individual_bitflags(ACCESS_RPGOPST, status_id, action, status_p)) ;

/*END of ORPGINFO_statefl_rpg_operability_status()*/
}

/*************************************************************************
   Description:
      Get, set, or clear the specified RPG State File flag.
  
      If setting the flag, the corresponding flag in the message will be updated
      with the flag value provided by the caller.  If clearing the flag, the
      corresponding flag in the message will be cleared.  Otherwise, the current
      flag value will be placed in the storage provided by the caller.
 
      In all cases, the final message flag value is placed in the storage
      provided by the caller.
 
      ORPGEVT_STATEFL_FLAG
         Posted after successful flag update.

   Inputs:
      flag_id - A flag indicating the State File flag of interest:

	Flag Macro				Description

 	ORPGINFO_STATEFL_FLG_PRFSELECT		PRF Selected.
 	ORPGINFO_STATEFL_FLG_SBIMPLEM		Spot-Blanking implemented.
  
      flag -  A flag indicating the desired action:

	Flag Macro			Action

 	ORPGINFO_STATEFL_GET		Get the flag value.
 	ORPGINFO_STATEFL_SET		Set the flag value.
 	ORPGINFO_STATEFL_CLR		Clear the flag value.

      flag_p - Pointer to caller-provided storage for flag value.

   Returns:
      0 - success
     -1 - failure

      Possible Failures:
         Invalid action argument.
         Unable to open the RPG Information datastore.
         Unable to read the RPG State File message from the datastore.
         Unable to lock the RPG State File message.
         Unable to write the RPG State File message into the datastore.

*****************************************************************************/
static int Access_statefl_flag(Orpginfo_statefl_flagid_t flag_id, int action,
                               unsigned char *flag_p){


    if ((action != ORPGINFO_STATEFL_GET)
                &&
        (action != ORPGINFO_STATEFL_SET)
                &&
        (action != ORPGINFO_STATEFL_CLR)) {
        LE_send_msg(GL_ERROR,
           "Bad \"action\" value: %d (not _GET, _SET, or _CLR)", action) ;
        return(-1) ;
    }

    return(Access_individual_bitflags(ACCESS_FLAGS, flag_id, action, flag_p)) ;

/*END of Access_statefl_flag()*/
}

/************************************************************************
 
   Description: RPG info notification callback		

   Inputs:

   Outputs:

   Returns: 
      NONE

   Notes:
 
************************************************************************/
static void NTF_rpginfo_callback( int fd, LB_id_t msg_id, int msg_info, 
                                  void *arg){

   shared_rpginfo_has_changed = 1;

/* End of NTF_rpginfo_callback() */
}

/************************************************************************

   Description:
      Access the bitflags fields (_GET only).

   Inputs:
      data_type - type of information to "get"
      action - must be ORPGINFO_STATFL_GET

   Outputs
      bitflags_p - returns the value of the bitflags

   Returns:
      -1 on error, or 0 otherwise.
   
   Notes:

************************************************************************/
static int Access_all_bitflags( int data_type, int action,
                                unsigned int *bitflags_p){

    unsigned int bitflags ;
    int retval ;

    if (action != ORPGINFO_STATEFL_GET) {

        LE_send_msg(GL_ERROR,
           "Bad \"action\" value: %d (not _GET)", action) ;
        return(-1) ;

    }

    if (Rpginfo_lbd == NVLD_LB_DESCRIPTOR){

        retval  = Open_rpginfo() ;
        if (retval < 0) {
            return(-1) ;
        }

    }

    /*  Register for LB notification of changes if we haven't registered already */
    if (!registered_for_rpginfo_changes){

	int status;

	if( ((status = ORPGDA_open( ORPGDAT_RPG_INFO, LB_WRITE | LB_READ )) < 0)
					  ||
	    ((status = ORPGDA_UN_register( ORPGDAT_RPG_INFO, 
                                           ORPGINFO_STATEFL_SHARED_MSGID, 
                                           NTF_rpginfo_callback)) < 0) ){

           LE_send_msg (GL_ERROR,
		"ORPGDA_UN_register (ORPGDAT_RPG_INFO) Failed (%d)\n", status);
	   return (status);

	}
        registered_for_rpginfo_changes = 1;

    }

    if (shared_rpginfo_has_changed){

       shared_rpginfo_has_changed = 0;

       (void) memset(&shared_rpginfo_data, 0, sizeof(shared_rpginfo_data)) ;

       retval = ORPGDA_read( ORPGDAT_RPG_INFO, (char *) &shared_rpginfo_data,
                             (int) sizeof(shared_rpginfo_data),
                             (LB_id_t) ORPGINFO_STATEFL_SHARED_MSGID) ;
       if (retval <= 0){

           LE_send_msg( GL_ERROR, "ORPGDA_read(ORPGINFO_STATEFL_SHARED_MSGID) Failed (%d)",
                        retval) ;
           return(-1) ;

    	}

    }

    if (data_type == ACCESS_FLAGS) {
        bitflags = shared_rpginfo_data.flags ;
    }
    else if (data_type == ACCESS_ALARMS) {
        bitflags = shared_rpginfo_data.rpg_alarms ;
    }
    else {
        bitflags = shared_rpginfo_data.rpg_op_status ;
    }

    if (action == ORPGINFO_STATEFL_GET) {
        (void) memset(bitflags_p, 0, sizeof(*bitflags_p)) ;
        (void) memcpy(bitflags_p, &bitflags, sizeof(*bitflags_p)) ;
    }

    return(0) ;

/*END of Access_all_bitflags()*/
}

/***************************************************************************

   Description:
      Access individual bitflags

   Inputs:
      type - type of information to access (either ACCESS_FLAGS, ACCESS_ALARMS,
             or ACCESS_RPGOPST)
      flag_id - id of flag
      action - either ORPGINFO_STATEFL_GET, ORPGINFO_STATEFL_SET,
               or ORPGINFO_STATFL_CLR
      
   Outputs:
      flag_p - receives the flag value.

   Returns:
      -1 on error or 0 otherwise.

   Notes:

***************************************************************************/
static int Access_individual_bitflags(int data_type, int flag_id, int action,
                                      unsigned char *flag_p){

    unsigned int *bitflags_p ;
                               /** shared data read from locked LB msg    */
    Orpginfo_statefl_shared_t locked ;
    int retval ;
                               /** shared data read from unlocked LB msg  */
                               /** updated shared data LB msg             */
    Orpginfo_statefl_shared_t updated ;

    /* Verify data type. */
    if ((data_type != ACCESS_FLAGS) && (data_type != ACCESS_ALARMS)
                               &&
        (data_type != ACCESS_RPGOPST)) {

        LE_send_msg(GL_INPUT,"Unrecognized data_type (%d)", data_type) ;
        return(-1) ;

    }

    /* Verify action. */
    if ((action != ORPGINFO_STATEFL_GET) && (action != ORPGINFO_STATEFL_SET)
                               &&
        (action != ORPGINFO_STATEFL_CLR)) {

        LE_send_msg(GL_INPUT,"Unrecognized action (%d)", action) ;
        return(-1) ;

    }


    if (Rpginfo_lbd == NVLD_LB_DESCRIPTOR) {
        retval  = Open_rpginfo() ;
        if (retval < 0) {
            return(-1) ;
        }
    }

    /*  Register for LB notification of changes if we haven't registered already */
    if (!registered_for_rpginfo_changes){

	int status;

	if( ((status = ORPGDA_open( ORPGDAT_RPG_INFO, LB_WRITE | LB_READ )) < 0)
					  ||
	    ((status = ORPGDA_UN_register( ORPGDAT_RPG_INFO, ORPGINFO_STATEFL_SHARED_MSGID,
                                          NTF_rpginfo_callback )) < 0) ){

           LE_send_msg( GL_ERROR,
		"ORPGDA_UN_register (ORPGDAT_RPG_INFO) Failed (%d)\n", status);
	   return (status);

	}
        registered_for_rpginfo_changes = 1;
    }


    /* If action is get, do the following .... */
    if (action == ORPGINFO_STATEFL_GET) {

	if (shared_rpginfo_has_changed)
	{
	   shared_rpginfo_has_changed = 0;

           /* Read the UNLOCKED State File Shared Data message ... */
           retval = ORPGDA_read( ORPGDAT_RPG_INFO, (char *) &shared_rpginfo_data, 
                                 sizeof(shared_rpginfo_data),
                                 (LB_id_t) ORPGINFO_STATEFL_SHARED_MSGID) ;
           if (retval <= 0){

              LE_send_msg( GL_ERROR,
        		   "ORPGDA_read(ORPGINFO_STATEFL_SHARED_MSIGD) Failed (%d)", 
                           retval) ;
	      return(-1) ;

	   }

	}

        if (data_type == ACCESS_FLAGS)
            bitflags_p = &shared_rpginfo_data.flags ;
          
        else if (data_type == ACCESS_ALARMS) 
            bitflags_p = &shared_rpginfo_data.rpg_alarms ;
         
        else
            bitflags_p = &shared_rpginfo_data.rpg_op_status ;
          
        if (*bitflags_p & flag_id) 
            *flag_p = 1 ;
          
        else 
            *flag_p = 0 ;
          

        /* We are through! */
        return(0) ;

    } /*endif we are simply getting the flag*/

    /*
     * Update the State File Shared Data message data
     * Advisory-lock the State File Shared Data LB message
     * Read the current State File Shared Data LB message
     * Compare the "locked" message data and the "updated" message data
     * If necessary, edit the "updated" message data
     * Write the "updated" message data to the State File Shared Data LB
     * message
     * Unlock the State File Shared Data LB message
     */

    retval = Lock_statefl_shared_msg() ;
    if (retval < 0) {

        LE_send_msg(GL_ERROR, "Lock_statefl_shared_msg() failed: %d", retval) ;
        return(-1) ;

    }

    retval = ORPGDA_read( ORPGDAT_RPG_INFO, (char *) &locked, (int) sizeof(locked),
                          (LB_id_t) ORPGINFO_STATEFL_SHARED_MSGID) ;

    if (retval <= 0) {

        LE_send_msg( GL_ERROR,
                     "ORPGDA_read(locked ORPGINFO_STATEFL_SHARED_MSGID) Failed (%d)",
                    retval) ;
        (void) Unlock_statefl_shared_msg() ;
        return(-1) ;

    }

    (void) memcpy(&updated, &locked, sizeof(updated)) ;

    if (data_type == ACCESS_FLAGS) 
       bitflags_p = &updated.flags ;
     
    else if (data_type == ACCESS_ALARMS)
       bitflags_p = &updated.rpg_alarms ;
     
    else 
       bitflags_p = &updated.rpg_op_status ;
     
    if (action == ORPGINFO_STATEFL_SET) {

        if (data_type == ACCESS_ALARMS) 
            *bitflags_p &= ~(ORPGINFO_STATEFL_RPGALRM_NONE) ;
         
        *bitflags_p |= flag_id ;
        *flag_p = 1 ;

    }
    else {

        *bitflags_p &= ~(flag_id) ;
        if ((data_type == ACCESS_ALARMS) && (*bitflags_p == 0)) 
            *bitflags_p |= ORPGINFO_STATEFL_RPGALRM_NONE ;
         
        *flag_p = 0 ;

    }

    /*
     * Update the RPG Operability Status bitflags as required (Maintenance
     * Action Mandatory, Maintenance Action Required, Loadshedding) ...
     */
    if (data_type == ACCESS_ALARMS) 
        Update_rpgopst(&updated) ;

    if (data_type == ACCESS_RPGOPST) {

        /* The "online" and "commanded shutdown" bitflags are toggled in
           tandem. */
        if (flag_id == ORPGINFO_STATEFL_RPGOPST_ONLINE) {

            if (action == ORPGINFO_STATEFL_SET) 
                updated.rpg_op_status &= ~(ORPGINFO_STATEFL_RPGOPST_CMDSHDN) ;
             
            else
                updated.rpg_op_status |= ORPGINFO_STATEFL_RPGOPST_CMDSHDN ;
             
        }
        else if (flag_id == ORPGINFO_STATEFL_RPGOPST_CMDSHDN) {

            if (action == ORPGINFO_STATEFL_SET) 
                updated.rpg_op_status &= ~(ORPGINFO_STATEFL_RPGOPST_ONLINE) ;

            else 
                updated.rpg_op_status |= ORPGINFO_STATEFL_RPGOPST_ONLINE ;

        }

    } /*endif we're setting/clearing RPG Operability Status bitflag*/

    /* If nothing has changed, unlock the LB and return. */
    if( memcmp( &updated, &locked, sizeof(updated) ) == 0 ){

       (void) Unlock_statefl_shared_msg();
       return (0);

    }

    /* Write the updated state file information. */
    retval = ORPGDA_write( ORPGDAT_RPG_INFO, (char *) &updated, (int) sizeof(updated),
                           (LB_id_t) ORPGINFO_STATEFL_SHARED_MSGID) ;
    if (retval <= 0) {

        LE_send_msg( GL_ERROR,
                     "ORPGDA_write(ORPGINFO_STATEFL_SHARED_MSGID) Failed (%d)", 
                     retval) ;
        (void) Unlock_statefl_shared_msg() ;
        return(-1) ;

    }
    else
       shared_rpginfo_has_changed = 1;  /* set the rpginfo_has_changed flag */

    /* Unlock the statefile LB. */
    (void) Unlock_statefl_shared_msg() ;

    /*
     * Report the appropriate System Status Message ...
     * Post the appropriate event or events ...
     *
     *     ORPGEVT_RPG_ALARM
     *     ORPGEVT_STATEFL_FLAG
     *     ORPGEVT_RPG_OPSTAT_CHANGE
     *
     * Bear in mind that the RPG Operability Status can be changed directly
     * (ACCESS_RPGOPST) or indirectly (ACCESS_ALARMS).
     */

    if (data_type == ACCESS_FLAGS) {
        /*
         * Automatic variables ...
         */
        Orpginfo_statefl_flag_evtmsg_t evtmsg ;

        (void) memset(&evtmsg, 0, sizeof(evtmsg)) ;
        evtmsg.old_bitflags = locked.flags ;
        evtmsg.new_bitflags = updated.flags ;
        evtmsg.flag_id = flag_id ;
        if (updated.flags & flag_id) {
            evtmsg.flag = 1 ;
        }

        retval = EN_post(ORPGEVT_STATEFL_FLAG, &evtmsg, sizeof(evtmsg), 0) ;

        if (retval < 0) {
            LE_send_msg(GL_EN(retval),
                        "EN_post(State File Flag 0x%08x) failed: %d",
                        (int) flag_id, retval) ;
            return(-1) ;
        }

	/* If we changed the PRF Select flag, then we need to post
	 * an additional event.
	 */

	if (flag_id == ORPGINFO_STATEFL_FLG_PRFSELECT) {

	    retval = EN_post (ORPGEVT_AUTOPRF_FLAG_CHANGE,
			NULL, 0, 0);
            if (retval < 0) {
		LE_send_msg(GL_EN(retval),
                        "EN_post(ORPGEVT_AUTOPRF_FLAG_CHANGE) failed: %d",
                        retval) ;
		return(-1) ;
	    }
        }
    }

    else if (data_type == ACCESS_ALARMS) {
        /*
         * Automatic variables ...
         */
        Orpginfo_rpg_alarm_evtmsg_t evtmsg ;
        char *alarm_msg = NULL;
        unsigned int alarm_type;

        /*
         * Report the appropriate System Status Message ...
         */
        if (action == ORPGINFO_STATEFL_SET){

           alarm_msg = Alarm_name( flag_id, &alarm_type );
           if( alarm_msg != NULL )
              LE_send_msg( GL_STATUS | GL_ERROR | alarm_type,"%s %s",
                           ORPGINFO_RPG_ALARM_ACTIVATED, alarm_msg );

        }
        else{

           alarm_msg = Alarm_name( flag_id, &alarm_type );
           if( alarm_msg != NULL )
              LE_send_msg( GL_STATUS | GL_ERROR | LE_RPG_AL_CLEARED | alarm_type,"%s %s",
                           ORPGINFO_RPG_ALARM_CLEARED, alarm_msg );

        }

        (void) memset(&evtmsg, 0, sizeof(evtmsg)) ;
        evtmsg.old_bitflags = locked.rpg_alarms ;
        evtmsg.new_bitflags = updated.rpg_alarms ;

        evtmsg.alarm_id = flag_id ;
        if (updated.rpg_alarms & flag_id) {
            evtmsg.bitflag = 1 ;
        }

        retval = EN_post(ORPGEVT_RPG_ALARM, &evtmsg, sizeof(evtmsg), 0) ;
        if (retval < 0) {
            LE_send_msg(GL_EN(retval),
                        "EN_post(RPG Alarm 0x%08x) failed: %d",
                        (int) flag_id, retval) ;
            return(-1) ;
        }
    }

    if (updated.rpg_op_status != locked.rpg_op_status) {
        /*
         * Automatic variables ...
         */
        Orpginfo_rpg_opstat_evtmsg_t evtmsg ;

        (void) memset(&evtmsg, 0, sizeof(evtmsg)) ;

        evtmsg.old_bitflags = locked.rpg_op_status ;
        evtmsg.new_bitflags = updated.rpg_op_status ;

        retval = EN_post(ORPGEVT_RPG_OPSTAT_CHANGE, &evtmsg, sizeof(evtmsg), 0) ;
        if (retval < 0) {
            LE_send_msg(GL_EN(retval),
                        "EN_post(RPG Operability Status Change) failed: %d",
                        retval) ;
            return(-1) ;
        }
    }

    return(0) ;

/*END of Access_individual_bitflags()*/
}

/********************************************************************************

   Description:
      Returns the alarm name string given the alarm id

   Inputs:
      alarm_id - alarm_id

   Outputs:
      alarm_type - type of alarm (i.e., )

   Returns:
      Alarm name string or NULL if alarm_id is not recognized

   Notes:

********************************************************************************/
static char* Alarm_name(int alarm_id, unsigned int *alarm_type){

    static char *alarm_name = Undefined;

    if (alarm_id == ORPGINFO_STATEFL_RPGALRM_NONE) {
        alarm_name = "NONE" ;
        *alarm_type = 0;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_NODE_CON) {
        alarm_name = "Node Connectivity Failure" ;
        *alarm_type = LE_RPG_AL_MAM;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RPGCTLFL) {
        alarm_name = "RPG Control Task Failure" ;
        *alarm_type = LE_RPG_AL_MAM;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_DBFL) {
        alarm_name = "Data Base Failure" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_SPARE26) {
        alarm_name = "Spare" ;
        *alarm_type = 0;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_WBDLS) {
        alarm_name = NULL;
        *alarm_type = LE_RPG_AL_LS;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_PRDSTGLS) {
        alarm_name = "Product Storage Loadshed" ;
        *alarm_type = LE_RPG_AL_LS;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_SPARE22) {
        alarm_name = "Spare" ;
        *alarm_type = 0;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_SPARE21) {
        alarm_name = "Spare" ;
        *alarm_type = 0;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RDAWB) {
        alarm_name = "RDA Wideband Alarm" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RPGRPGFL) {
        alarm_name = "RPG/RPG Link Failure" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_REDCHNER) {
        alarm_name = "Redundant Channel Error" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_FLACCFL) {
        alarm_name = "Spare" ;
        *alarm_type = 0;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_MEDIAFL) {
        alarm_name = "Media Failure" ;
        *alarm_type = LE_RPG_AL_MAM;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RDAINLS) {
        alarm_name = "RDA Radial Loadshed" ; 
        *alarm_type = LE_RPG_AL_LS;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RPGINLS) {
        alarm_name = "RPG Radial Loadshed" ; 
        *alarm_type = LE_RPG_AL_LS;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_RPGTSKFL) {
        alarm_name = "RPG Task Failure" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_DISTRI) {
        alarm_name = "NB Communication Fatal Error" ;
        *alarm_type = LE_RPG_AL_MAR;
    }
    else if (alarm_id == ORPGINFO_STATEFL_RPGALRM_WBFAILRE ){

       /* We return a NULL string since the originator of the alarm
          message must be able to distinguish between the different
          types of failures.   One message does not fit all! */
       alarm_name = NULL;
       *alarm_type = LE_RPG_AL_MAM;

    }

    return(alarm_name) ;

/*END of Alarm_name()*/
}

/**************************************************************************
   Description: 
      Lock the State File Shared Data LB message

   Input: void

   Output: 
      the message is advisory locked

   Returns: 0 upon success; -1 otherwise

   Notes:
 **************************************************************************/
static int Lock_statefl_shared_msg(void){

    int retval = 0 ;

    /* Note that we're blocking here ... */
    retval = LB_lock(Rpginfo_lbd, LB_EXCLUSIVE_LOCK | LB_BLOCK,
                     ORPGINFO_STATEFL_SHARED_MSGID) ;
    if (retval != LB_SUCCESS) {

        LE_send_msg(GL_LB(retval),
        "LB_LOCK  ORPGINFO_STATEFL_SHARED_MSGID failed: %d", retval) ;
        return(-1) ;

    }

    return(0) ;

/*END of Lock_statefl_shared_msg()*/
}

/**************************************************************************
   Description: Open the ORPGDAT_RPG_INFO datastore with write permission

   Input: void

   Output: void

   Returns: 0 upon success; -1 otherwise

   Notes:

**************************************************************************/
static int Open_rpginfo(void){

    int ret;

    /* Open the LB with write permission. */
    if( (ret = ORPGDA_write_permission( ORPGDAT_RPG_INFO )) < 0 ){

        LE_send_msg( GL_ERROR, "ORPGDAT_RPG_INFO write_permission Failed: %d",
                     ret) ;
        Rpginfo_lbd = NVLD_LB_DESCRIPTOR ;
        return(-1) ;

    }

    /* Get the file descriptor .... need this for LB locking. */
    if( (Rpginfo_lbd = ORPGDA_lbfd( ORPGDAT_RPG_INFO )) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGDAT_RPG_INFO lbfd Failed: %d",
                    Rpginfo_lbd );
       return(-1);

    }

    return(0) ;

/*END of Open_rpginfo()*/
}

/**************************************************************************
   Description: Unlock the State File Shared Data LB message

   Input: void

   Output: the message is unlocked

   Returns: 0 upon success; -1 otherwise

   Notes:

**************************************************************************/
static int Unlock_statefl_shared_msg(void){

    int retval ;

    retval = LB_lock(Rpginfo_lbd, LB_UNLOCK, ORPGINFO_STATEFL_SHARED_MSGID) ;

    if (retval != LB_SUCCESS) {

        LE_send_msg(GL_LB(retval),
        "LB_UNLOCK ORPGINFO_STATEFL_SHARED_MSGID failed: %d", retval) ;
        return(-1) ;

    }

    return(0) ;

/*END of Unlock_statefl_shared_msg()*/
}

/******************************************************************

   Description:
      Sets (clears) the RPG Operability Status Alarm Bit(s) 
      based on the passed data structure. 

   Inputs:
      shared_data - data structure containing alarm information

   Outputs:

   Returns:
      None

   Notes:

******************************************************************/
static void Update_rpgopst(Orpginfo_statefl_shared_t *shared_data){

    unsigned int alarm_flags ; 

    alarm_flags = shared_data->rpg_alarms ;

    /*
     * Maintenance Action Required?
     */
    if ((alarm_flags & ORPGINFO_STATEFL_RPGALRM_DISTRI)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_DBFL)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_RPGTSKFL)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_RPGRPGFL)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_RDAWB)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_REDCHNER)) {

        shared_data->rpg_op_status |= (ORPGINFO_STATEFL_RPGOPST_MAR) ;
    }
    else {
        shared_data->rpg_op_status &= ~(ORPGINFO_STATEFL_RPGOPST_MAR) ;
    }

    /*
     * Maintenance Action Mandatory?
     */
    if ((alarm_flags & ORPGINFO_STATEFL_RPGALRM_RPGCTLFL)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_NODE_CON)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_MEDIAFL)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_WBFAILRE)) {

        shared_data->rpg_op_status |= (ORPGINFO_STATEFL_RPGOPST_MAM) ;
    }
    else {
        shared_data->rpg_op_status &= ~(ORPGINFO_STATEFL_RPGOPST_MAM) ;
    }

    /*
     * Loadshed?
     */
    if ((alarm_flags & ORPGINFO_STATEFL_RPGALRM_PRDSTGLS)
                    ||
        (alarm_flags & ORPGINFO_STATEFL_RPGALRM_WBDLS)) {

        shared_data->rpg_op_status |= (ORPGINFO_STATEFL_RPGOPST_LOADSHED) ;
    }
    else {
        shared_data->rpg_op_status &= ~(ORPGINFO_STATEFL_RPGOPST_LOADSHED) ;
    }

    return ;

/*END of Update_rpgopst()*/
}
