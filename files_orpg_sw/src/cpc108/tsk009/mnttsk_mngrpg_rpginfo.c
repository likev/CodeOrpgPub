/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/14 20:50:27 $
 * $Id: mnttsk_mngrpg_rpginfo.c,v 1.17 2013/05/14 20:50:27 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

#include <stdlib.h>            /* getopt(), malloc()                      */
#include <string.h>            /* strncpy(),strspn(),strcat()             */

#define MNTTSK_MNGRPG_RPGINFO
#include <mnttsk_mngrpg_def.h>
#undef MNTTSK_MNGRPG_RPGINFO

/*
 * Constants
 */
#define MIN_ACTION (MNTTSK_MNGRPG_RPGINFO_INIT_ALL_MSGS)
#define MAX_ACTION (MNTTSK_MNGRPG_RPGINFO_INIT_RPGCMD_MSGS)


/*
 * Static Global Variables
 */

/*
 * Static Function Prototypes
 */
static int Initialize_endianvalue(int data_id) ;
static int Initialize_statefl_shared(int data_id) ;

/*****************************************************************************

   Description:
      Driver module for initializing Endian Value and State File Shared
      data.

   Inputs:
      action - what initialization action to perform.  Has values of either

               MNTTSK_RPGINFO_INIT_ALL_MSGS
               MNTTSK_MNGRPG_RPGINFO_INIT_STAEFL_MSGS

   Outputs:

   Returns:
      Always returns 0.

   Notes:

*****************************************************************************/
int MNTTSK_MNGRPG_RPGINFO_init(int action){

    int data_id = ORPGDAT_RPG_INFO; 	/* LB data ID                              */

    LE_send_msg(GL_INFO, "requested action: %d\n", action) ;

    if (action == MNTTSK_MNGRPG_RPGINFO_INIT_ALL_MSGS) {

        LE_send_msg(GL_INFO, "Initialize RPG Info Messages\n") ;
        (void) Initialize_endianvalue(data_id) ;
        (void) Initialize_statefl_shared(data_id) ;

    }
    else if (action == MNTTSK_MNGRPG_RPGINFO_INIT_STATEFL_MSG) {

        LE_send_msg(GL_INFO,
                    "Initialize RPG State File Shared Data Message\n") ;
        (void) Initialize_statefl_shared(data_id) ;

    }
    else if (action == MNTTSK_MNGRPG_RPGINFO_INIT_ENDIANVALUE_MSG) {

        LE_send_msg(GL_INFO,
                    "Initialize RPG Endian Value Message\n") ;
        (void) Initialize_endianvalue(data_id) ;

    }
    return(0) ;

/*END of MNTTSK_MNGRPG_RPGINFO_init()*/
}

/*****************************************************************************

   Description:
      Initializes RPG alarms to NONE and RPG operability status to commanded
      shutdown.  Save PRF Select flag value from previous run, if defined.

   Inputs:

   Outputs:

   Returns:
      Returns -1 on error, or 0 on success.

   Notes:

*****************************************************************************/
static int Initialize_statefl_shared(int data_id){

    int read_retval, write_retval ;
    int prf_flag_set = 1 ;
    int super_res_flag_set = 1;
    int cmd_flag_set = 1;
    int avset_flag_set = 1;
    int sails_flag_set = 0;
    Orpginfo_statefl_shared_t shared ;


    /* Read the statefile data.  Initialize all elements if empty.  
       Otherwise, initialize only RPG Operability Status and RPG Alarms.
       Make sure the PRF Select Flag is carried over is previously 
       defined. */
    read_retval = ORPGDA_read(data_id, (char *) &shared,
                         (int) sizeof(shared), ORPGINFO_STATEFL_SHARED_MSGID) ;
    if( read_retval == (int) sizeof(shared) ){

       if( shared.flags & ORPGINFO_STATEFL_FLG_PRFSELECT ){

          LE_send_msg( GL_INFO, "Previous PRF Select Flag ON\n" );
          prf_flag_set = 1;

       }
       else{

          prf_flag_set = 0;
          LE_send_msg( GL_INFO, "Previous PRF Select Flag OFF\n" );

       }

       if( shared.flags & ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED ){

          LE_send_msg( GL_INFO, "Previous Super Res Flag ENABLED\n" );
          super_res_flag_set = 1;

       }
       else{

          super_res_flag_set = 0;
          LE_send_msg( GL_INFO, "Previous Super Res Flag DISABLED\n" );

       }

       if( shared.flags & ORPGINFO_STATEFL_FLG_CMD_ENABLED ){

          LE_send_msg( GL_INFO, "Previous Clutter Mitigation Decision Flag ENABLED\n" );
          cmd_flag_set = 1;

       }
       else{

          cmd_flag_set = 0;
          LE_send_msg( GL_INFO, "Previous Clutter Mitigation Decision Flag DISABLED\n" );

       }

       if( shared.flags & ORPGINFO_STATEFL_FLG_AVSET_ENABLED ){

           LE_send_msg( GL_INFO, "Previous AVSET Flag ENABLED\n" );
           avset_flag_set = 1;

        }
        else{

           avset_flag_set = 0;
           LE_send_msg( GL_INFO, "Previous AVSET Flag DISABLED\n" );

       }

       if( shared.flags & ORPGINFO_STATEFL_FLG_SAILS_ENABLED ){

           LE_send_msg( GL_INFO, "Previous SAILS Flag ENABLED\n" );
           sails_flag_set = 1;

        }
        else{

           sails_flag_set = 0;
           LE_send_msg( GL_INFO, "Previous SAILS Flag DISABLED\n" );

       }

    }
    else
       LE_send_msg( GL_INFO, "Previous State File Shared Message Not Present (%d)\n",
                    read_retval );

    /*
     * State File Flags: all cleared (expect possibly PRF Select)
     * RPG Operability Status: set Commanded Shutdown bitflag
     * RPG Alarms: set "no alarms" bitflag
     */
    (void) memset(&shared, 0, sizeof(shared)) ;

    if( prf_flag_set )
       shared.flags |= ORPGINFO_STATEFL_FLG_PRFSELECT;

    if( super_res_flag_set )
       shared.flags |= ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED;

    if( cmd_flag_set )
       shared.flags |= ORPGINFO_STATEFL_FLG_CMD_ENABLED;

    if( avset_flag_set )
       shared.flags |= ORPGINFO_STATEFL_FLG_AVSET_ENABLED;

    if( sails_flag_set )
       shared.flags |= ORPGINFO_STATEFL_FLG_SAILS_ENABLED;

    shared.rpg_alarms |= ORPGINFO_STATEFL_RPGALRM_NONE ;
    shared.rpg_op_status |= ORPGINFO_STATEFL_RPGOPST_CMDSHDN ;

    write_retval = ORPGDA_write(data_id, (char *) &shared,
                          (int) sizeof(shared), ORPGINFO_STATEFL_SHARED_MSGID) ;
    if (write_retval != sizeof(shared)) {

        LE_send_msg(GL_INFO,
                    "ORPGDA_write(State File Shared Data) failed: %d != %d\n",
                    write_retval, (int) sizeof(shared)) ;
        return(-1) ;

    }
    else 
        LE_send_msg(GL_INFO,
                    "State File Shared Data Msgid %d: wrote %d bytes",
                    ORPGINFO_STATEFL_SHARED_MSGID, write_retval) ;

    return(0) ;

/*END of Initialize_statefl_shared()*/
}

/*****************************************************************************

   Description:
      Initialize the Endian value for ORPG machine.

   Inputs:
      data_id - LB ID which stores the Endian value.

   Outputs:

   Returns:
      Return -1 on error, or 0 on success.

   Notes:

*****************************************************************************/
static int Initialize_endianvalue(int data_id){

#ifdef LITTLE_ENDIAN_MACHINE
    Orpginfo_endianvalue_t endian_value = ORPG_LITTLE_ENDIAN ;
#else
    Orpginfo_endianvalue_t endian_value = ORPG_BIG_ENDIAN ;
#endif
    LB_id_t msgid = ORPGINFO_ENDIANVALUE_MSGID ;
    size_t msgsz = sizeof(endian_value) ;
    int retval ;

    retval = ORPGDA_write(data_id, (char *) &endian_value, (int) msgsz, msgid) ;
    if (retval != msgsz) {

        LE_send_msg(GL_INFO, "ORPGDA_write(%d) failed: %d != %d\n",
                    msgid, retval, (int) msgsz) ;
        return(-1) ;
    }
    LE_send_msg(GL_INFO, "Endian Value Msgid %d: wrote %d bytes\n",
                msgid, (int) msgsz) ;

    return(0) ;

/*END of Initialize_endianvalue()*/
}
