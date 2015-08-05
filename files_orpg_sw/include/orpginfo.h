/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/10 15:18:38 $
 * $Id: orpginfo.h,v 1.81 2014/11/10 15:18:38 steves Exp $
 * $Revision: 1.81 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpginfo.h

 Description: ORPG RPG Information public header file.

 Assumptions:

 **************************************************************************/


/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef ORPGINFO_H
#define ORPGINFO_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <infr.h>


/* Constant Definitions/Macro Definitions/Type Definitions */

/*
  IDs of the LB message that comprise the ORPGDAT_RPG_INFO LB file
  
  State File
  State File Shared Data (separate message for LB-locking)
  Endian Value

*/
typedef enum {ORPGINFO_STATEFL_MSGID=0,
              ORPGINFO_STATEFL_SHARED_MSGID,
              ORPGINFO_ENDIANVALUE_MSGID} Orpginfo_msgids_t ;

/* New RPG Endian Value */
typedef int Orpginfo_endianvalue_t ;

/* RPG Status Change Event Message */
typedef struct {
                               /** Current RPG Status value. */
    unsigned int rpg_status ;
} Orpginfo_rpg_status_change_evtmsg_t ;


/* RPG State File Flag Identifiers 

 Spare     
 PRF Select
 Spot-Blanking Implemented
 PRF Select Function Paused
 Super Resolution Enabled/Disabled
 Clutter Mitigation Decision Enabled/Disabled
 Automated Volume Evaluation and Termination Enabled/Disabled
 Supplemental Adaptive Intra-volume Low-level Scan Enabled/Disable
*/
typedef enum {ORPGINFO_STATEFL_FLG_SPARE = 0x01,
              ORPGINFO_STATEFL_FLG_PRFSELECT = 0x02,
              ORPGINFO_STATEFL_FLG_SBIMPLEM  = 0x04,
              ORPGINFO_STATEFL_FLG_PRFSF_PAUSED = 0x08,
              ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED = 0x10,
              ORPGINFO_STATEFL_FLG_CMD_ENABLED = 0x20,
              ORPGINFO_STATEFL_FLG_AVSET_ENABLED = 0x40,
              ORPGINFO_STATEFL_FLG_SAILS_ENABLED = 0x80
} Orpginfo_statefl_flagid_t ;


/* RPG State File Flag Update Event Message */
typedef struct {
                               /** ID of the flag that has been updated. */
    Orpginfo_statefl_flagid_t flag_id ;
                               /** value of the flag (0/1) */
    unsigned char flag ;
                               /** Previous bitflags. */
    unsigned int old_bitflags ;
                               /** Updated bitflags. */
    unsigned int new_bitflags ;
} Orpginfo_statefl_flag_evtmsg_t ;

/*
  RPG State File RPG Alarm values
  
  These bit-flag macros may be OR'd together.  The corresponding field
  in the RPG State File is mapped to the General Status Message.  Some of
  these legacy alarm conditions may not be reasonable for the New RPG.
  
  Legacy Ref: RPG/PUP ICD; a309.inc (ALRM_NONE_BIT)
 
  NOTE: We need to check this against the ICD
   
	Name			Legacy bit	Description

 ORPGINFO_STATEFL_RPGALRM_NONE		31	No RPG alarms present
 ORPGINFO_STATEFL_RPGALRM_NODE_CON    	30	Node Connectivity Failure
 ORPGINFO_STATEFL_RPGALRM_WBFAILRE	29 	Widebamd Failure
 ORPGINFO_STATEFL_RPGALRM_RPGCTLFL 	28 	RPG Control Failure 
 ORPGINFO_STATEFL_RPGALRM_DBFL  	27 	Data Base Failure 
 ORPGINFO_STATEFL_RPGALRM_SPARE26	26 	Spare
 ORPGINFO_STATEFL_RPGALRM_WBDLS    	25 	Wideband Loadshed 
 ORPGINFO_STATEFL_RPGALRM_SPARE24	24 	Spare
 ORPGINFO_STATEFL_RPGALRM_PRDSTGLS 	23 	Product Storage Loadshed 
 ORPGINFO_STATEFL_RPGALRM_SPARE22	22 	Spare
 ORPGINFO_STATEFL_RPGALRM_SPARE21	21 	Spare
 ORPGINFO_STATEFL_RPGALRM_RDAWB   	20 	RDA Wideband
 ORPGINFO_STATEFL_RPGALRM_RPGRPGFL 	19 	RPG/RPG Link Failure 
 ORPGINFO_STATEFL_RPGALRM_REDCHNER 	18 	Redundant Channel Error 
 ORPGINFO_STATEFL_RPGALRM_RPGTSKFL  	17 	RPG Task Failure 
 ORPGINFO_STATEFL_RPGALRM_MEDIAFL  	16 	Media Failure
 ORPGINFO_STATEFL_RPGALRM_RDAINLS  	15 	RDA Radial Input Load Shed 
 ORPGINFO_STATEFL_RPGALRM_RPGINLS  	14 	RPG Radial Input Load Shed 
 ORPGINFO_STATEFL_RPGALRM_FLACCFL   	13 	RPG Task Failure 
 ORPGINFO_STATEFL_RPGALRM_DISTRI        12      Product Distribution
 ORPGINFO_STATEFL_SPARE11          	11 	Spare 
   
*/
typedef enum {ORPGINFO_STATEFL_RPGALRM_NONE     = 0x00000001,    /* 31 */
              ORPGINFO_STATEFL_RPGALRM_NODE_CON = 0x00000002,    /* 30 */
              ORPGINFO_STATEFL_RPGALRM_WBFAILRE = 0x00000004,    /* 29 */
              ORPGINFO_STATEFL_RPGALRM_RPGCTLFL = 0x00000008,    /* 28 */
              ORPGINFO_STATEFL_RPGALRM_DBFL     = 0x00000010,    /* 27 */
              ORPGINFO_STATEFL_RPGALRM_SPARE26  = 0x00000020,    /* 26 */
              ORPGINFO_STATEFL_RPGALRM_WBDLS    = 0x00000040,    /* 25 */
              ORPGINFO_STATEFL_RPGALRM_SPARE24  = 0x00000080,    /* 24 */
              ORPGINFO_STATEFL_RPGALRM_PRDSTGLS = 0x00000100,    /* 23 */
              ORPGINFO_STATEFL_RPGALRM_SPARE22  = 0x00000200,    /* 22 */
              ORPGINFO_STATEFL_RPGALRM_SPARE21  = 0x00000400,    /* 21 */
              ORPGINFO_STATEFL_RPGALRM_RDAWB    = 0x00000800,    /* 20 */
              ORPGINFO_STATEFL_RPGALRM_RPGRPGFL = 0x00001000,    /* 19 */
              ORPGINFO_STATEFL_RPGALRM_REDCHNER = 0x00002000,    /* 18 */
              ORPGINFO_STATEFL_RPGALRM_RPGTSKFL = 0x00004000,  	 /* 17 */
              ORPGINFO_STATEFL_RPGALRM_MEDIAFL  = 0x00008000,    /* 16 */
              ORPGINFO_STATEFL_RPGALRM_RDAINLS  = 0x00010000,    /* 15 */
              ORPGINFO_STATEFL_RPGALRM_RPGINLS  = 0x00020000,    /* 14 */
              ORPGINFO_STATEFL_RPGALRM_FLACCFL  = 0x00040000,    /* 13 */
              ORPGINFO_STATEFL_RPGALRM_DISTRI   = 0x00080000, 	 /* 12 */
              ORPGINFO_STATEFL_SPARE11          = 0x00100000  	 /* 11 */
} Orpginfo_statefl_rpgalrm_t ;

/* RPG State File RPG Alarm Update Event Message */
typedef struct {
                               /** ID of the alarm that has been updated. */
    Orpginfo_statefl_rpgalrm_t alarm_id ;
                               /** value of the bitflag (0/1) */
    unsigned char bitflag ;
                               /** Previous bitflags. */
    unsigned int old_bitflags ;
                               /** Updated bitflags. */
    unsigned int new_bitflags ;
} Orpginfo_rpg_alarm_evtmsg_t ;

/* RPG State File RPG Operability Status Update Event Message */
typedef struct {
                               /** Previous bitflags. */
    unsigned int old_bitflags ;
                               /** Updated bitflags. */
    unsigned int new_bitflags ;
} Orpginfo_rpg_opstat_evtmsg_t ;


/* RPG State File Shared Data
  
   We maintain these flags in a separate LB message as they require LB
   locking (to coordinate amongst RPG tasks that can set/clear these flags).
*/
typedef struct {
                               /** Storage for up to 32 bit-flags. */
    unsigned int flags ;
                               /** Storage for up to 32 RPG Alarms bit-flags. */
    unsigned int rpg_alarms ;
                               /** RPG Operability Status 
                                 *
                                 * Re: ORPGINFO_STATEFL_RPGOPST_ macros */
    unsigned int rpg_op_status ;
} Orpginfo_statefl_shared_t ;


/*
  RPG State File
  
  NOTE: legacy code uses 4-byte integer for RPG Operability Status,
        RPG Status, and RPG Alarms ... note that the corresponding GSM
        fields are half-words ...
 */
typedef struct {
/* TBD: remove these two fields after liborpg routines are cleaned-up ...*/

    unsigned int rpg_op_status ;
    unsigned int rpg_alarms ;
                               /** RPG Status: Commanded
                                 *
                                 * Re: ORPGINFO_STATEFL_RPGSTAT_ macros */
    unsigned int rpg_status_cmded ;
                               /** RPG Status: Current
                                 *
                                 * Re: ORPGINFO_STATEFL_RPGSTAT_ macros */
    unsigned int rpg_status ;
                               /** RPG Status: Previous
                                 *
                                 * Re: ORPGINFO_STATEFL_RPGSTAT_ macros */
    unsigned int rpg_status_prev ;

} Orpginfo_statefl_t ;

                               /** Size (bytes) of the State File message. */
#define ORPGINFO_STATEFL_SIZE (sizeof(Orpginfo_statefl_t))



/*
   RPG State File RPG Operability Status values
  
   These bit-flag macros may be OR'd together.
  
   Legacy Ref: RPG/PUP ICD; a309.inc (STAT_LOADSHED_BIT)
  
       MAR = Maintenance Action Required
       MAM = Maintenance Action Mandatory
       CMDSHDN = Commanded Shutdown
                                                       Legacy Bit
*/
typedef enum {ORPGINFO_STATEFL_RPGOPST_LOADSHED = 0x00000001,    /* 31 */
              ORPGINFO_STATEFL_RPGOPST_ONLINE   = 0x00000002,    /* 30 */
              ORPGINFO_STATEFL_RPGOPST_MAR      = 0x00000004,    /* 29 */
              ORPGINFO_STATEFL_RPGOPST_MAM      = 0x00000008,    /* 28 */
              ORPGINFO_STATEFL_RPGOPST_CMDSHDN  = 0x00000010,    /* 27 */
              ORPGINFO_STATEFL_RPGOPST_BIT26    = 0x00000020,    /* 26 */
              ORPGINFO_STATEFL_RPGOPST_BIT25    = 0x00000040,    /* 25 */
              ORPGINFO_STATEFL_RPGOPST_BIT24    = 0x00000080,    /* 24 */
              ORPGINFO_STATEFL_RPGOPST_BIT23    = 0x00000100,    /* 23 */
              ORPGINFO_STATEFL_RPGOPST_BIT22    = 0x00000200,    /* 22 */
              ORPGINFO_STATEFL_RPGOPST_BIT21    = 0x00000400,    /* 21 */
              ORPGINFO_STATEFL_RPGOPST_BIT20    = 0x00000800,    /* 20 */
              ORPGINFO_STATEFL_RPGOPST_BIT19    = 0x00001000,    /* 19 */
              ORPGINFO_STATEFL_RPGOPST_BIT18    = 0x00002000,    /* 18 */
              ORPGINFO_STATEFL_RPGOPST_BIT17    = 0x00004000,    /* 17 */
              ORPGINFO_STATEFL_RPGOPST_BIT16    = 0x00008000     /* 16 */
} Orpginfo_statefl_rpgopst_t ;


/*
   RPG State File RPG Status values
  
   These bit-flag macros may be OR'd together.
  
   Legacy Ref: RPG/PUP ICD; a309.inc (SHUTDOWN)
  
   NOTE: Legacy General Status Message "RPG Status" bit-field does NOT
         include a "shutdown" status (maybe this is "spare" bit 12 in that
         16-bit field).
  
   NOTE: the _UNKNOWN macro has no legacy equivalent ... provided for
         internal RPG use only!
*/
typedef enum {ORPGINFO_STATEFL_RPGSTAT_UNKNOWN  = 0x00000000,
              ORPGINFO_STATEFL_RPGSTAT_RESTART  = 0x00000001,
              ORPGINFO_STATEFL_RPGSTAT_OPERATE  = 0x00000002,
              ORPGINFO_STATEFL_RPGSTAT_STANDBY  = 0x00000004,
              ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN = 0x00000008,
              ORPGINFO_STATEFL_RPGSTAT_TEST     = 0x00000010,
              ORPGINFO_STATEFL_RPGSTAT_BIT26    = 0x00000020,
              ORPGINFO_STATEFL_RPGSTAT_BIT25    = 0x00000040,
              ORPGINFO_STATEFL_RPGSTAT_BIT24    = 0x00000080,
              ORPGINFO_STATEFL_RPGSTAT_BIT23    = 0x00000100,
              ORPGINFO_STATEFL_RPGSTAT_BIT22    = 0x00000200,
              ORPGINFO_STATEFL_RPGSTAT_BIT21    = 0x00000400,
              ORPGINFO_STATEFL_RPGSTAT_BIT20    = 0x00000800,
              ORPGINFO_STATEFL_RPGSTAT_BIT19    = 0x00001000,
              ORPGINFO_STATEFL_RPGSTAT_BIT18    = 0x00002000,
              ORPGINFO_STATEFL_RPGSTAT_BIT17    = 0x00004000,
              ORPGINFO_STATEFL_RPGSTAT_BIT16    = 0x00008000
} Orpginfo_statefl_rpgstat_t ;


/**@#-*/ /* CcDoc token parsing OFF*/

enum {ORPGINFO_STATEFL_GET=0,
      ORPGINFO_STATEFL_SET,
      ORPGINFO_STATEFL_CLR} ;

int ORPGINFO_set_prf_select(void) ;
int ORPGINFO_clear_prf_select(void) ;
int ORPGINFO_set_prf_select_paused(void) ;
int ORPGINFO_clear_prf_select_paused(void) ;
int ORPGINFO_set_spotblanking_implemented(void) ;
int ORPGINFO_clear_spotblanking_implemented(void) ;
int ORPGINFO_set_super_resolution_enabled(void) ;
int ORPGINFO_clear_super_resolution_enabled(void) ;
int ORPGINFO_set_cmd_enabled(void) ;
int ORPGINFO_clear_cmd_enabled(void) ;
int ORPGINFO_set_sails_enabled(void) ;
int ORPGINFO_clear_sails_enabled(void) ;


unsigned char ORPGINFO_is_test_mode(void) ;
unsigned char ORPGINFO_is_prf_select(void) ;
unsigned char ORPGINFO_is_prf_select_paused(void) ;
unsigned char ORPGINFO_is_spotblanking_implemented(void) ;
unsigned char ORPGINFO_is_super_resolution_enabled(void) ;
unsigned char ORPGINFO_is_cmd_enabled(void) ;
unsigned char ORPGINFO_is_avset_enabled(void) ;
unsigned char ORPGINFO_is_sails_enabled(void) ;

enum {ORPGINFO_STATEFL_RPG_STATUS_CMDED=0,
      ORPGINFO_STATEFL_RPG_STATUS_CUR,
      ORPGINFO_STATEFL_RPG_STATUS_PREV} ;

#define ORPGINFO_RPG_ALARM "RPG ALARM "
				/* RPG alarm message label */
#define ORPGINFO_RPG_ALARM_ACTIVATED "RPG ALARM ACTIVATED:"
				/* for reporting alarm activated event */
#define ORPGINFO_RPG_ALARM_CLEARED "RPG ALARM CLEARED:"
				/* for reporting alarm cleared event */

#define ORPGINFO_RDA_ALARM "RDA ALARM "
				/* RDA alarm message label */
#define ORPGINFO_RDA_ALARM_ACTIVATED "RDA ALARM ACTIVATED:"
				/* for reporting alarm activated event */
#define ORPGINFO_RDA_ALARM_CLEARED "RDA ALARM CLEARED:"
				/* for reporting alarm cleared event */

int ORPGINFO_statefl_flag (Orpginfo_statefl_flagid_t flag_id, int action,
			   unsigned char *flag_p);
int ORPGINFO_statefl_rpg_status(int action, int which, unsigned int *status_p,
                                char **status_string) ;

int ORPGINFO_statefl_rpg_operability_status(Orpginfo_statefl_rpgopst_t status_id,
                                            int action, unsigned char *status_p);

int ORPGINFO_statefl_rpg_alarm(Orpginfo_statefl_rpgalrm_t alarm_id, int action,
                               unsigned char *alarm_p) ;

int ORPGINFO_statefl_get_flags(unsigned int *flags_p) ;
int ORPGINFO_statefl_get_rpgopst(unsigned int *rpgopst_p) ;
int ORPGINFO_statefl_get_rpgalrm(unsigned int *rpgalrm_p) ;
int ORPGINFO_statefl_get_rpgstat(unsigned int *rpgstat_p) ;

unsigned char ORPGINFO_is_rpgstatus_restart(void) ;
unsigned char ORPGINFO_is_rpgstatus_operate(void) ;
unsigned char ORPGINFO_is_rpgstatus_standby(void) ;
unsigned char ORPGINFO_is_rpgstatus_shutdown(void) ;
unsigned char ORPGINFO_is_rpgstatus_test(void) ;

/**@#+*/ /* CcDoc token parsing ON*/

/**@#-*/ /* CcDoc token parsing OFF*/
#endif /* #ifndef ORPGINFO_H */
/**@#+*/ /* CcDoc token parsing ON*/
