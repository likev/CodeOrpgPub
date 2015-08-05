/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/25 18:07:22 $
 * $Id: rda_control.h,v 1.19 2012/09/25 18:07:22 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */  
/**********************************************************************

	Header defining the data structures and constants used for
	RDA Control Command messages as defined in the Interface
	Control Document (ICD) for the RDA/RPG (rev: March 1, 1996).

        6/12/2003 - New structure was created to support Open RDA.

**********************************************************************/


#ifndef	RDA_CONTROL_COMMANDS_MESSAGE_H
#define RDA_CONTROL_COMMANDS_MESSAGE_H

/*	RDA Control Command (Message 6) Offset Definitions		*/

#define RDC_STATE	         0	/*  RDA State			*/
#define RDC_BDENABLE	         1	/*  RDA Base Data Enable	*/
#define RDC_AUXGEN	         2	/*  Auxilliary Power/Generator  */
                                        /*  Control			*/
#define RDC_CONTROL              3	/*  Control Authorization       */
#define RDC_RESCAN	         4	/*  Restart VCP/Elevation Cut	*/
#define RDC_SELVCP	         5	/*  Select VCP for Next Scan	*/
#define RDC_AUTOCAL	         6	/*  Auto-Calibration Override	*/
#define RDC_SUPER_RES	         7	/*  Super Res Control        	*/
#define RDC_CMD                  8	/*  CMD Control        		*/
#define RDC_CTLISU	         9	/*  Control ISU			*/
#define RDC_AVSET	         9	/*  AVSET Control		*/
#define RDC_SELOPMODE	        10	/*  Select Operating Mode	*/
#define RDC_CCTL_STATUS	        11	/*  Channel Control Command	*/
#define RDC_PERF_CHECK		12      /*  Performance Check Command   */

/* These are not used for ORDA configurations */
#define RDC_ARCH_CONTROL        12	/*  Archive II Control		*/
#define RDC_ARCH_NUM_SCANS      13      /*  Archive Number of Scans	*/
#define RDC_START_TIME_1        14	/*  Playback Start Time 1st HW 	*/	
#define RDC_START_TIME_2        15	/*  Playback Start Time 2nd HW 	*/	
#define RDC_START_DATE	        16	/*  Playback Start Date		*/
#define RDC_STOP_DATE	        17	/*  Playback Stop Date		*/
#define RDC_STOP_TIME           18	/*  Playback Stop Time 1st HW 	*/	
#define RDC_STOP_TIME2          19	/*  Playback Stop Time 2nd HW 	*/	


#define RDC_SB		        20	/*  Spot Blanking		*/

/*	RDA Control Command Values					*/

/* 	RDA STATE COMMAND VALUES					*/
#define RCOM_STANDBY	    0x8001	/*  Command RDA Standby		*/
#define RCOM_OFFOPER	    0x8002	/*  Command RDA Offline Operate */
#define RCOM_OPERATE        0x8004	/*  Command RDA Operate		*/
#define RCOM_RESTART        0x8008	/*  Command RDA Restart		*/

/* Not valid for Open RDA */
#define RCOM_PLAYBACK       0x8010	/*  Command Archive II Playback */


/*	BASE DATA TRANSMISSION ENABLE VALUES				*/
#define RCOM_BDENABLE       0x8007	/*  Base Data Enable All	*/
#define RCOM_BDENABLEN      0x8000 	/*  Base Data Enable None	*/
#define RCOM_BDENABLER      0x8001	/*  Base Data Enable Reflectivity */
#define RCOM_BDENABLEV      0x8002	/*  Base Data Enable Velocity	*/
#define RCOM_BDENABLEW      0x8004	/*  Base Data Enable Width 	*/


/*	AUXILLIARY POWER - GENERATOR CONTROL VALUES			*/
#define RCOM_AUXGEN	    0x8004	/*  Activate Auxilliary Power	*/
#define RCOM_UTIL	    0x8002	/*  Switch to Utility Power	*/

/*	RDA CONTROL COMMANDS AND AUTHORIZATION VALUES			*/
#define RCOM_REQREMOTE          16	/*  Request Remote RDA Control	*/
#define RCOM_ACCREMOTE           8	/*  Accept Remote RDA Control	*/
#define RCOM_ENALOCAL            4	/*  Enable Local RDA Control	*/

/*	RESTART VCP OR ELEVATION CUT					*/
#define RCOM_RESTART_VCP    0x8000	/*  Restart VCP			*/
#define RCOM_RESTART_ELEV   0x8000	/*  Restart Elevation Scan	*/

/*SELECT VCP NUMBER FOR NEXT VOLUME SCAN				*/
#define RCOM_USE_REMOTE_PATTERN  0	/*  Use remote VCP  next scan 	*/

/*	AUTOMATIC CALIBRATION OVERRRIDE					*/
#define RCOM_AUTOCAL	     -1000     	/*  Automatic Calibration 	*/
#define RCOM_AUTOCAL_ORDA    32766     	/*  Automatic Calibration 	*/

/*	SUPER RESOLUTION CONTROL					*/
#define RCOM_ENABLE_SR           2	/*  Enable Super Resolution	*/
#define RCOM_DISABLE_SR          4	/*  Disable Super Resolution	*/

/*	CLUTTER MITIGATION DECISION					*/
#define RCOM_ENABLE_CMD          2	/*  Enable Clutter Mitigation	*/
#define RCOM_DISABLE_CMD         4	/*  Disable Clutter Mitigation	*/

/*	AVSET CONTROL              	 				*/
#define RCOM_ENABLE_AVSET        2	/*  Enable AVSET             	*/
#define RCOM_DISABLE_AVSET       4	/*  Disable AVSET             	*/

/*      PERFORMANCE CHECK                                               */
#define RCOM_PERF_CHECK_NC       0      /*  No Change.                  */
#define RCOM_PERF_CHECK_PPC      1	/*  Perform Performance Check   */

/*	CONTROL INTERFERENCE SUPPRESSION UNIT				*/
#define RCOM_CTLISUE	         2	/*  Enable ISU			*/
#define RCOM_CTLISUD	         4	/*  Disable ISU			*/


/*	Not valid for ORDA */
/*	ARCHIVE II CONTROL (RECORD)SELECT OPERATING MODE 		*/ 
#define RCOM_ARCH_START	    0x8001     	/*  Start Arch II Collection	*/
#define RCOM_ARCH_STOP	    0x8002     	/*  Stop Arch II Collection	*/


/*	SPOT BLANKING							*/
#define RCOM_SB_ENAB	         2	/*  Spot Blanking Enable	*/
#define RCOM_SB_DIS	         4	/*  Spot Blanking Disable	*/

/*      CHANNEL CONTROL							*/
#define RCOM_CHAN_CONTRL         1      /* Set to controlling channel   */ 
#define RCOM_CHAN_NONCONTRL      2      /* Set to non-controlling channel*/ 

/*	RDA Data Request Types						*/

#define DREQ_STATUS	    0x0081	/*  Request for RDA Status Data */
#define DREQ_PERFMAINT	    0x0082	/*  Request for RDA Performance */
           				/*  Maintenance Data		*/
#define DREQ_CLUTMAP	    0x0084	/*  Request for Bypass Map 	*/
#define DREQ_NOTCHMAP	    0x0088	/*  Request for Notchwidth Map  */


/* 	RDA Control Command Structure 					*/
typedef	struct {

    unsigned short state;	/*  RDA state command from the following
				    list.  Only one command is allowed at
				    a time except Restart, which is allowed
				    with operational commands.

					no bits set     = No Change     
					bits 0 & 15 set = Stand-By
					     1 & 15     = Offline Operate
					     2 & 15     = Operate
					     3 & 15     = Restart
					     4 & 15     = Archive II Playback
				*/
    unsigned short data_enbl;	/*  Basedata Transmission Enable.  All
				    combinations of data enabling are
				    allowed.

					no bits set     = No Change
					bits 0 & 15 set = Reflectivity
					     1 & 15     = Velocity
					     2 & 15     = Spectrum Width
				*/
    unsigned short aux_pwr_gen;	/*  Auxilliary Power Generator Control.

					no bits set     = No Change
					bits 1 & 15 set = Switch to Utility Power
					     2 & 15     = Switch to Aux Power

				    Note:  Deactivate and activate power
				    generator, switch to aux and util power,
				    switch to aux power and deactivate power
				    generator are mutually exclusive.
				*/
    unsigned short authorization; /*  RDA Control Commands and Authorization.

					no bits set = No Change
					bit 1 set   = Control Command Clear
					    2       = Local Control Enabled
					    3       = Remote Control Accepted
					    4       = Remote Control Requested
				*/
    unsigned short restart_elev;  /*  Restart VCP or Elevation Cut.
				    Bit 15 is always set.  Bits 0-7 contain
				    the number of the cut.
				*/
    short	select_vcp;	/*  Select Local VCP Number for Next Volume
				    Scan.
					0        = Use Remote Pattern
					32767    = No Change
					1 to 767 = Pattern Number
				*/
    short	auto_calib;	/*  Automatic Calibration Override.
					-1000     = Automatic Calibrarion
					-40 to 40 = Calibration Override
					32767     = No Change
				*/
    short	spare8;
    short	spare9;
    short	interference;	/*  Control Interference Suppression Unit.
					0 = Leave at Current State
					2 = Enable ISU
					4 = Disable ISU
				*/
    short	operate_mode;	/*  Select Operating Mode.
					0 = Leave at Current State
					2 = Maintenance
					4 = Operational
				*/
    short	channel;	/*  Channel Control Command.
					0 = No Change
					1 = Set to Controlling Channel
					2 = Set to Non-controlling Channel
				*/
    unsigned short archive_II;	/*  Archive II Control (record)
					no bits set = No Change
					bits 0 & 15 set = Start
					     1 & 15     = Stop
				*/
    short	archive_num;	/*  Number of Volume Scans to Archive.
					0        = Continuous Recording
					1 to 255 = Number of Volume Scans
				*/
    int		start_time;	/*  Playback Start Time.  Number of
				    milliseconds after midnight (UT).
				    (Range 0 to 86399999)
				*/
    unsigned short start_date;	/*  Playback Stop Julian Date (number of days
				    from January 1, 1970 starting with 1).
				    (Range 1 to 65535);
   				*/ 
    unsigned short stop_date;	/*  Playback Stop Julian Date (number of days
				    from January 1, 1970 starting with 1).
				    (Range 1 to 65535);
   				*/ 
    int		stop_time;	/*  Playback Stop Time.  Number of
				    milliseconds after midnight (UT).
				    (Range 0 to 86399999)
				*/
    short	spot_blanking;	/*  Spot Blanking.
					0 = No Change
					2 = Enable Spot Blanking
					4 = Disable Spot Blanking
				*/
    short	spare22;
    short	spare23;
    short	spare24;
    short	spare25;
    short	spare26;

} RDA_control_commands_t;


/* 	ORDA Control Command Structure 					*/
typedef	struct {

    unsigned short state;	/*  RDA state command from the following
				    list.  Only one command is allowed at
				    a time except Restart, which is allowed
				    with operational commands.

					no bits set     = No Change     
					bits 0 & 15 set = Stand-By
					     1 & 15     = Offline Operate
					     2 & 15     = Operate
					     3 & 15     = Restart
				*/
    unsigned short data_enbl;	/*  Basedata Transmission Enable.  All
				    combinations of data enabling are
				    allowed.

					no bits set     = No Change
					bits 0 & 15 set = Reflectivity
					     1 & 15     = Velocity
					     2 & 15     = Spectrum Width
				*/
    unsigned short aux_pwr_gen;	/*  Auxilliary Power Generator Control.

					no bits set     = No Change
					bits 1 & 15 set = Switch to Utility Power
					     2 & 15     = Switch to Aux Power

				    Note:  Deactivate and activate power
				    generator, switch to aux and util power,
				    switch to aux power and deactivate power
				    generator are mutually exclusive.
				*/
    unsigned short authorization; /*  RDA Control Commands and Authorization.

					no bits set = No Change
					bit 1 set   = Control Command Clear
					    2       = Local Control Enabled
					    3       = Remote Control Accepted
					    4       = Remote Control Requested
				*/
    unsigned short restart_elev;  /*  Restart VCP or Elevation Cut.
				    Bit 15 is always set.  Bits 0-7 contain
				    the number of the cut.
				*/
    short	select_vcp;	/*  Select Local VCP Number for Next Volume
				    Scan.
					0        = Use Remote Pattern
					32767    = No Change
					1 to 767 = Pattern Number
				*/
    short	auto_calib;	/*  Automatic Calibration Override.
					32766     = Automatic Calibration
					-1000 to 1000 = Calibration Override
					32767     = No Change
				*/
    short	super_res;	/*  Super Resolution Enable. 
				    0 - No Change
				    2 - Enable Super Res
				    4 - Disabled
				*/
    short	cmd;		/* Clutter Mitigation Decision.
				    0 - No Change
				    2 - Enable CMD
				    4 - Disable CMD
				*/
    short	avset;		/* Automatic Volume Scan Evaluation and 
				   Termination flag.
				*/
    short	operate_mode;	/*  Select Operating Mode.
					0 = Leave at Current State
					2 = Maintenance
					4 = Operational
				*/
    short	channel;	/*  Channel Control Command.
					0 = No Change
					1 = Set to Controlling Channel
					2 = Set to Non-controlling Channel
				*/
    short 	perf_check;	/*  Perform Performance Check.
					0 = No Change
					1 = Perform Performance Check
				*/
    short	spare14;	/*  Unused

				*/
    int		spare15;	/*  Unused

				*/
    unsigned short spare16;	/*  Unused

   				*/ 
    unsigned short spare17;	/*  Unused

   				*/ 
    int		spare18;	/*  Unused

				*/
    short	spot_blanking;	/*  Spot Blanking.
					0 = No Change
					2 = Enable Spot Blanking
					4 = Disable Spot Blanking
				*/
    short	spare22;
    short	spare23;
    short	spare24;
    short	spare25;
    short	spare26;

} ORDA_control_commands_t;

#endif
