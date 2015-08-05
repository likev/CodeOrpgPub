/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:14 $
 * $Id: hci_rda_control.h,v 1.14 2002/12/11 22:10:14 nolitam Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */  
/************************************************************************
 *									*
 *	Header defining the codes for sending RDA control information	*
 *	to the Comm Manager.  This information is written to a special	*
 *	linear buffer so that the comm manager can read them at its	*
 *	convenience.							*
 *									*
 ************************************************************************/


#ifndef	HCI_RDA_COMMAND_CODES_H
#define	HCI_RDA_COMMAND_CODES_H

#define RDA_COMMAND_MAX_MESSAGE_SIZE 1600

/*	Define the originators of the RDA control command.  Currently	*
 *	only HCI and PBD are supported.					*/
#define HCI_INITIATED_RDA_CTRL_CMD -100 /*  HCI initiated RDA control	*
					    command			*/
#define PBD_INITIATED_RDA_CTRL_CMD -200	/*  PBD initiated RDA control	*
					    command			*/

/*	Definitions for buffer elements of RDA command data written	*
 *	to RDA control linear buffer.					*/

#define	CPC4M_COMMAND		 0	/*  Command type element	*/
#define	CPC4M_LINENO		 1	/*  Channel #			*/
#define	CPC4M_P1		 2	/*  RDA Command number		*/
#define	CPC4M_P2		 3	/*  Command Data		*/
#define	CPC4M_P3		 4	/*  Command Data		*/
#define	CPC4M_MSGDATA		 5	/*  Message Data 		*/

/*	Command Type codes						*/

#define COM4_LBTEST_RDAtoRPG   403	/*  RDA/RPG Loopback Message    */ 
#define COM4_LBTEST	       404	/*  RPG/RDA Loopback Message    */ 
#define COM4_WBMSG	       406	/*  RPG/RDA Console Message     */ 
#define COM4_WBENABLE	       407	/*  Wideband Line Enable        */ 
#define COM4_WBDISABLE	       408	/*  Wideband Line Disable       */ 
#define COM4_WMVCPCHG          410	/*  Weather Mode Change         */ 
#define	COM4_RDACOM	       411	/*  RDA Control Command Code	*/
#define	COM4_REQRDADATA	       412	/*  Request RDA Data        	*/
#define	COM4_DLOADVCP  	       413	/*  Downlaod VCP        	*/
#define	COM4_SENDEDCLBY	       414	/*  Download Clutter Bypass Map */
#define	COM4_SENDCLCZ  	       415	/*  Download Clutter Sensor Zones*/
#define	COM4_SB_ENAB           420	/*  Spot Blanking Enabled       */
#define	COM4_SB_DIS            421	/*  Spot Blanking Disabled      */
#define	COM4_HEARTBEATLB       500	/*  Send Heartbeat Loopback Msg */

#define COM4_PBD_TEST         1000      /*  For PBD Testing Purposes    */

/*	RDA command codes (COM4_RDACOM parameter_1)			*/

#define	CRDA_STANDBY		 1	/*  RDA Standby State		*/
#define	CRDA_OFFOPER		 2	/*  RDA Offline Operate State	*/
#define	CRDA_OPERATE		 3	/*  RDA Operate State		*/
#define	CRDA_RESTART		 4	/*  RDA Restart State		*/
#define	CRDA_BDENABLE		 5	/*  Enable All Data fields	*/
#define	CRDA_REQREMOTE		10	/*  Request RDA Remote Control	*/
#define	CRDA_ACCREMOTE		11	/*  Accept RDA Remote Control	*/
#define	CRDA_ENALOCAL		12	/*  Enable RDA Local Control	*/
#define	CRDA_AUTOCAL		13	/*  Calibration Override cmd	*/
#define	CRDA_CTLISU		14	/*  Interference Suppression	*/
#define	CRDA_AUXGEN		15	/*  Switch to Auxiliary Power	*/
#define	CRDA_RESTART_VCP	16	/*  Volume Restart Command	*/
#define	CRDA_RESTART_ELEV	17	/*  Elevation Restart Command	*/
#define	CRDA_SELECT_VCP		18	/*  Select new VCP		*/
#define	CRDA_UTIL		20	/*  Switch to Utility Power	*/
#define	CRDA_ARCH_COLLECT	21	/*  Start Archive II collection	*/
#define	CRDA_ARCH_REPLAY	22	/*  Playback Archive II data	*/
#define	CRDA_ARCH_STOP		23	/*  Stop Archive II collection	*/
#define	CRDA_FORCE_RED_STANDBY	24	/*  				*/
#define	CRDA_MODE_OP		25	/*				*/
#define	CRDA_MODE_MNT		26	/*				*/
#define CRDA_CHAN_CTL		27	/*  Cmd RDA to Controlling	*/
#define CRDA_CHAN_NONCTL	28	/*  Cmd RDA to Non-Controlling	*/
#define	CRDA_SB_ENAB		29	/*  Spot Blanking Enabled       */
#define	CRDA_SB_DIS		30	/*  Spot Blanking Disabled      */

#define CRDA_UNEX_VOL_START   1000      /*  For Testing Unexpected      */
                                        /*  Volume Start                */
#define CRDA_UNEX_ELV_START   1001      /*  For Testing Unexpected      */
                                        /*  Elevation Start             */

/*	RDA Data Request Types						*/

typedef struct rda_command {

   int command;
   int line_number;
   int parameter_1;
   int parameter_2;
   int parameter_3;
   int parameter_4;
   int parameter_5;
   char message_text[1600];

} rda_command_t;

#endif

