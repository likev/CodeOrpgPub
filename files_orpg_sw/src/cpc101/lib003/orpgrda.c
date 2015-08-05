/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:25:19 $
 * $Id: orpgrda.c,v 1.65 2014/11/07 21:25:19 steves Exp $
 * $Revision: 1.65 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:		orpgrda.c					*
 *									*
 *	Description:	This module contains a collection of functions	*
 *			to manipulate various RDA Status Msg		*
 *			information.					*
 *									*
 ************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>
#include <orpg.h>
#include <orpgrda.h>
#include <le.h>
#include <rda_alarm_table.h>
#include <rda_rpg_message_header.h>
#include <orpgsite.h>
#include <sys/time.h>


/*	Static variables						*/

static	int	Init_status_flag  = 0;		/* 0 = Status Msg needs to
						       be read.
						   1 = Status Msg not to 
						       be read.		*/
static	int	Init_alarms_flag  = 0;		/* 0 = Status Msg needs to
						       be read.
						   1 = Status Msg not to 
						       be read.		*/
static  int     Init_rda_config_flag = 0;	/* 0 = RDA configuration 
						       needs to be read.
						   1 = RDA configuration not
						       need to be read  */
static  int	Update_prev_status_on_read = 1;	/* 0 = On RDA Status read, 
						       do not copy local copy
						       of status to previous
						       status.
						   1 = On RDA Status read, 
						       copy local copy of 
						       status to previous
						       status.		*/
static	int	ORPGRDA_status_registered = 0;	/* 0 = RDA status LB notify
						       not registered.
						   1 = RDA status LB notify
						       registered.	*/
static	int	ORPGRDA_alarms_registered = 0;	/* 0 = RDA alarm msg event
						       not registered.
						   1 = RDA alarm msg event
						       registered.	*/
static	int	ORPGRDA_rdacfg_registered = 0;	/* 0 = RDA config update
						       not registered.
						   1 = RDA config update
						       registered.	*/


/*
 NOTE: We're assuming that the Open RDA status msg will always be
 at least as big as the Legacy RDA message.  This object should be initialized 
 to the greater of the two.
*/
static	ALIGNED_t Status_data [ALIGNED_T_SIZE (sizeof (ORDA_status_t))];

static	RDA_status_t*		Rda_status = NULL;
static	ORDA_status_t*		Orda_status = NULL;

static	RDA_status_t		Previous_rda_status;
static	ORDA_status_t		Previous_orda_status;


/* Previous RDA state */
static Previous_state_t Previous_state;
static int Previous_state_vcp_data_updated = 0;

/* Macro definitions */
#define	ORPGRDA_MAX_ALARM_MSGS	1000
#define STATUS_WORDS            26
#define MAX_STATUS_LENGTH       64
#define MAX_PWR_BITS             5
#define COMSWITCH_BIT            4
#define TAPE_NUM_SHIFT		12	
#define TAPE_NUM_MASK		0xf000


/* Private function prototypes */
static	int	Write_rda_status_msg(char* msg_ptr);
static	int	Write_orda_status_msg(char* msg_ptr);
static  int	Store_new_status_data ( char* status_data );
static	int	Get_rda_status ( int element );
static	int	Get_orda_status ( int element );
static	int	Set_rda_status ( int field, int val );
static	int	Set_orda_status ( int field, int val );
static	int	Get_previous_rda_status ( int field );
static	int	Get_previous_orda_status ( int field );
static	void	Process_status_field( char **buf, int *len, char *field_id,
                                      char *field_val );
static	void	Rda_update_system_status();
static	void	Orda_update_system_status();
static  int     Get_rda_config_from_msg_hdr( void* msg_ptr, int *rda_config );
static  void    Init_rda_config();
static  void    Deau_notify_func( int fd, LB_id_t msgid, int msg_info,
                                  void *arg );
static  void    Write_vcp_data( Vcp_struct *vcp );

/* Global data */
static	RDA_alarm_t*	Alarm_data		= NULL;
static	int		ORPGRDA_num_alarms      = 0;
static	int		Alarm_io_status		= 0;
static	int		Status_io_status 	= 0;
static	time_t		RDA_status_update_time	= 0;
static	time_t		RDA_alarms_update_time	= 0;
static  double          RDA_status_read_time    = 0; 

/*
   Note: for the RDA config flag... we're going to read the real
   value from adaptation data. 
*/
static  int             Rda_config_flag         = -1;

static	char		status[][32]		= { "Start-Up",
						    "Standby",
						    "Restart",
						    "Operate",
						    "Playback",
						    "Off-Line Operate",
						    "      " };
static	char		*moments[]		= { "None",
						    "All ",
						    "R",
						    "V",
						    "W",
						    "      " };
static	char		mode[][32]		= { "Operational",
						    "Maintenance",
						    "      " };
static	char		orda_mode[][32]		= { "Operational",
						    "Test",
						    "Maintenance",
						    "      " };
static	char		authority[][32]		= { "Local",
						    "Remote",
						    "      " };
static	char		interference_unit[][32]	= { "Enabled ",
						    "Disabled",
						    "      " };
static	char		channel_status[][32]	= { "Ctl",
						    "Non-Ctl",
						    "      " };
static	char		spot_blanking[][32]	= { "Not Installed",
						    "Enabled",
						    "Disabled",
						    "      " };
static	char		operability[][32]	= { "On-Line",
						    "MAR",
						    "MAM", 
						    "CommShut",
						    "Inoperable",
						    "WB Disc",
						    "      " };
static	char		control[][32]		= { "RDA",
						    "RPG",
						    "Eit",
						    "      " };
static	char		*set_aux_pwr[]		= { " Aux Pwr=On",
						    " Util Pwr=Yes",
						    " Gen=On",
						    " Xfer=Manual",
						    " Cmd Pwr Switch",
						    "      " };
static	char		*reset_aux_pwr[]	= { " Aux Pwr=Off",
						    " Util Pwr=No",
						    " Gen=Off",
						    " Xfer=Auto",
						    "",
						    "      " };
static	char		tps[][32]		= { "Off",
						    "Ok",
						    "      " };

static	char		perf_check[][32]	= { "Auto",
						    "Pending",
						    "      " };
 
static	char		*archive_status[]	= { "Installed",
						    "Media Loaded", 
						    "Reserved",
						    "Record", 
						    "Not Installed",
						    "Plybk Avail",
						    "Search",
						    "Playback", 
						    "Check Label",
						    "Fast Frwd",
						    "Tape Xfer",
						    " Tape #",
						    "      " };


/************************************************************************
 *									*
 *	Description: This function returns the status of the last	*
 *		     RDA Status msg I/O operation.			*
 *									*
 ************************************************************************/
int ORPGRDA_status_io_status ()
{
   return (Status_io_status);

} /* end ORPGRDA_status_io_status */


/************************************************************************
 *									*
 *	Description: This function returns the value of the RDA Status	*
 *		     msg update flag.  If 0, the message needs to be	*
 *		     updated, otherwise it doesn't.			*
 *									*
 ************************************************************************/
int ORPGRDA_status_update_flag ()
{
   return (Init_status_flag);

} /* end ORPGRDA_status_update_flag */


/************************************************************************
 *									*
 *	Description: This function returns the time of the last RDA	*
 *		     Status message update. (julian milliseconds).	*
 *									*
 *      Note:  This function is an improvement over ORPGRDA_status_     *
 *             update_time() because the last_update time is at a       *
 *             higher resolution (millisec vs sec)                      *
 ************************************************************************/
double ORPGRDA_last_status_update ( )
{
   int	status;

   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();
   }

   return (RDA_status_read_time);

} /* end ORPGRDA_last_status_update */

/************************************************************************
 *									*
 *	Description: This function returns the time of the last RDA	*
 *		     Status message update. (julian seconds).		*
 *									*
 *                                                                      *
 *      Note: This function is not reliable for use for determining     *
 *            status changes.                                           *
 ************************************************************************/
time_t ORPGRDA_status_update_time ()
{
   int	status;

   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();
   }

   return (RDA_status_update_time);

} /* end ORPGRDA_status_update_time */


/************************************************************************
 *									*
 *	Description: This function returns the value of the RDA Alarms	*
 *		     msg update flag.  If 0, the message needs to be	*
 *		     updated, otherwise it doesn't.			*
 *									*
 ************************************************************************/
int ORPGRDA_alarm_update_flag ()
{
   return (Init_alarms_flag);

} /* end ORPGRDA_alarm_update_flag */


/************************************************************************
 *									*
 *	Description: This function returns the time of the last RDA	*
 *		     Alarms message update. (julian seconds).		*
 *									*
 ************************************************************************/
time_t ORPGRDA_alarm_update_time ()
{
   int	status;

   if (!Init_alarms_flag) 
   {
      status = ORPGRDA_read_alarms ();
   }

   return (RDA_alarms_update_time);

} /* end ORPGRDA_alarm_update_time */


/************************************************************************
 *									*
 *	Description: This function returns the status of the last	*
 *		     RDA Alarm msg I/O operation.			*
 *									*
 ************************************************************************/
int ORPGRDA_alarm_io_status ()
{
   return (Alarm_io_status);

} /* end ORPGRDA_alarm_io_status */


/************************************************************************
 *									*
 *	Description: The following routine sets the RDA status  msg	*
 *		     init flag whenever an RDA status message update	*
 *		     event is received.					*
 *									*
 *	Return:      NONE						*
 *									*
 ************************************************************************/
void ORPGRDA_en_status_callback ( int fd, LB_id_t msg_id, int msg_info,
				  void *arg )
{
   Init_status_flag = 0;
}


/************************************************************************
 *									*
 *	Description: The following routine sets the RDA alarms msg	*
 *		     init flag whenever an RDA alarms message update	*
 *		     event is received.					*
 *									*
 *	Return:      NONE						*
 *									*
 ************************************************************************/
void ORPGRDA_en_alarms_callback ( int fd, LB_id_t msg_id, int msg_info,
                                  void* arg )
{
   Init_alarms_flag = 0;
   if (msg_info == LB_MSG_EXPIRED) 
   {
      ORPGRDA_clear_alarms ();
   }
}


/************************************************************************
 *									*
 *	Description: The following routine writes the RDA Status	*
 *		     message (RDA_GSM_DATA: RDA_STATUS_ID).		*
 *									*
 *	Return:      On success, 0 is returned.  On failure, a value	*
 *		     < 0 is returned.					*
 *									*
 *	Notes:								*
 *	   There is intended to be only 1 writer/producer of RDA Status	*
 *	   Data.  Therefore the previous RDA Status is updated before 	*
 *	   the data is written to LB.  If the writer of the data then	*
 *	   uses the get/read functions (e.g. ORPGRDA_get_status()), 	*
 *	   the previous RDA Status is not updated for the write of this	*
 * 	   data, i.e., Update_prev_status_on_read = 0.				*
 *									*
 ************************************************************************/
int ORPGRDA_write_status_msg ( char* status_msg )
{
   int	status	= 0;

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   /* Store the old data to the previous rda status message buffer */
   memcpy(&Previous_rda_status, &Status_data, sizeof(RDA_status_t));
   memcpy(&Previous_orda_status, &Status_data, sizeof(ORDA_status_t));

   Update_prev_status_on_read = 0;

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      status = Write_rda_status_msg(status_msg);
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      status = Write_orda_status_msg(status_msg);
   }
   else
   {
      LE_send_msg( GL_INFO,
         "ORPGRDA_write_status_msg: invalid Rda_config_flag.\n");
      status = -1;
   }

   if (status < 0)
   {
      LE_send_msg (GL_ERROR, "ORPGDA_write (RDA_STATUS_ID) failed: %d\n", status);
   }

   return status;

} /* end ORPGRDA_write_status_msg() */


/*******************************************************************************
*								
*	Description: The following routine reads the RDA Status message
*		     (RDA_GSM_DATA: RDA_STATUS_ID).  It firsts copies the old
*		     status message data into a "previous" status structure, then
*		     reads and stores the new data.
*
*	Return:      On success, the size (in bytes) of the status
*		     message is returned.  On failure, a value <=0
*		     is returned.	
*
*******************************************************************************/
int ORPGRDA_read_status_msg ()
{
   int		status		= 0;
   static int	first_time	= 0;
   
   struct timeval tv = {0};

   /*	Check to see if RDA status message update events have been	*
    *	registered.  If not, register them.				*/

   if (!ORPGRDA_status_registered) 
   {
      status = ORPGDA_write_permission (ORPGDAT_GSM_DATA);

      status = ORPGDA_UN_register (ORPGDAT_GSM_DATA,
         RDA_STATUS_ID, ORPGRDA_en_status_callback);

      if (status != LB_SUCCESS) 
      {
         LE_send_msg (GL_ERROR,
            "ORPGDA_UN_register(ORPGDAT_GSM_DATA,RDA_STATUS_ID) failed (ret %d)\n",
            status);
         Init_status_flag = 0;
         return (status);
      }
 
      ORPGRDA_status_registered = 1;
   }

   /* We don't want to try to copy the local status data object before it is
      even filled. Check to see if this is the first time through this routine.
      If so, skip the memcpy. */

   if ( first_time == 0 )
   { 
      first_time = 1;
   }
   else
   {
	
      /* Updating the previous status before reading is required only if the
         caller is not the writer.  The writer has the previous status 
         updated during the write (see ORPGRDA_write_status_msg(). */
      if( Update_prev_status_on_read ){

         /* Store the old data to the previous rda status message buffer */
         memcpy(&Previous_rda_status, &Status_data, sizeof(RDA_status_t));
         memcpy(&Previous_orda_status, &Status_data, sizeof(ORDA_status_t));

      }
   }

   status = ORPGDA_read (ORPGDAT_GSM_DATA, (char *) Status_data,
      sizeof (RDA_status_t), RDA_STATUS_ID);

   if ( (status == LB_TO_COME) || (status == LB_NOT_FOUND)  )
   {
      Init_status_flag = 0;
      return ( ORPGRDA_DATA_NOT_FOUND );
   }

   Status_io_status = status;

   if (status < 0) 
   {
      Init_status_flag = 0;
      LE_send_msg (GL_INFO, "ORPGDA_read (RDA_STATUS_ID): %d\n", status);
   }
   else
   {
      RDA_status_update_time = time (NULL);

      /* Ideally we would take the message time from the message header since
         the time value is number of millisecs since midnight which has a 
         higher time resolution than simply using seconds.  However there
         is no guarantee Status Messages sent from the RDA have unique time
         stamps.  Therefore we call gettimeofday() and to get the time the 
         message was read in microseconds. */
      gettimeofday( &tv, NULL );
      RDA_status_read_time = (double) tv.tv_sec + (double) tv.tv_usec/1000000.0;
   }

   /*
      Cast pointers to the new status data.
      NOTE: we have to establish both here because it's possible this routine
      is called before the RDA configuration is known.
   */
   Rda_status = (RDA_status_t *) Status_data;
   Orda_status = (ORDA_status_t *) Status_data;

   return status;

} /* end ORPGRDA_read_status_msg */


/************************************************************************
 *									*
 *	Description: The following function returns the RDA channel	*
 *		     number.						*
 *									*
 *	Return:      On success, the channel (0=non-redundant, 1, or 2) *
 *		     is returned.  On failure, ORPGRDA_DATA_NOT_FOUND	*
 *		     is returned.					*
 *									*
 ************************************************************************/
int ORPGRDA_channel_num ()
{
   int                  status          = 0;
   int                  chan_num        = 0;
   RDA_status_msg_t     *rda_status     = NULL;
   ORDA_status_msg_t    *orda_status    = NULL;
 

   if (!Init_status_flag)
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();

      if (status <= 0)
      {
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      rda_status = (RDA_status_msg_t *) &Rda_status->status_msg;

      /* We need to capture only the first 2 bits */
      chan_num = (int) (rda_status->msg_hdr.rda_channel & 0x03);
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      orda_status = (ORDA_status_msg_t *) &Orda_status->status_msg;

      /* We need to capture only the first 2 bits */
      chan_num = (int) (orda_status->msg_hdr.rda_channel & 0x03);
   }
   else
   {
      LE_send_msg( GL_INFO, "ORPGRDA_channel_num: invalid Rda_config_flag.\n");
      chan_num = ORPGRDA_DATA_NOT_FOUND;
   }

   return chan_num;

} /* end ORPGRDA_channel_num */


/************************************************************************
 *									*
 *	Description: The following function returns the value of a	*
 *		     specified RDA status message element.		*
 *									*
 *	Return:      On success, the data value is returned, on		*
 *		     failure, ORPGRDA_DATA_NOT_FOUND is returned.	*
 *									*
 *	Notes:	     See header file rda_status.h for macro names for	*
 * 		     RDA status elements (halfword locations).		*
 ************************************************************************/
int ORPGRDA_get_status ( int item )
{
   int			status;
   int			value;

   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();
      if (status <= 0) 
      {
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      value = Get_rda_status( item );
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      value = Get_orda_status( item );
   }
   else
   {
      LE_send_msg( GL_INFO, "ORPGRDA_get_status: invalid Rda_config_flag.\n");
      value = ORPGRDA_DATA_NOT_FOUND;
   }

   return value;

} /* end ORPGRDA_get_status */


/************************************************************************
 *	Description: This function returns wideband status information.	*
 *		     Currently there are only two types of information	*
 *		     returned: display blanking and link status.	*
 *									*
 *	Return:      On success, one of the macro definitions for the	*
 *		     item is returned.  ORPGRDA_DATA_NOT_FOUND is	*
 *		     returned on failure.				*
 ************************************************************************/
int ORPGRDA_get_wb_status ( int	item )
{
   int	num;
   int	status;
   RDA_RPG_comms_status_t	*wb_status;


   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();

      if (status <= 0) 
      {
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      if ( Rda_status != NULL )
      {
         wb_status = (RDA_RPG_comms_status_t *) &Rda_status->wb_comms;
      }
      else
      {
         LE_send_msg( GL_ERROR,
           "ORPGRDA_get_wb_status: could not get value, Rda_status is empty.\n");
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      if ( Orda_status != NULL )
      {
         wb_status = (RDA_RPG_comms_status_t *) &Orda_status->wb_comms;
      }
      else
      {
         LE_send_msg( GL_ERROR,
           "ORPGRDA_get_wb_status: could not get value, Orda_status is empty.\n");
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }
   else
   {
      LE_send_msg ( GL_INFO | GL_ERROR,
         "ORPGRDA_get_wb_status: invalid RDA config flag.\n");
      return (ORPGRDA_DATA_NOT_FOUND);
   }

   switch (item) 
   {
      case ORPGRDA_WBLNSTAT :

         num = wb_status->wblnstat;
         break;

      case ORPGRDA_DISPLAY_BLANKING :

         num = wb_status->rda_display_blanking;
         break;

      case ORPGRDA_WBFAILED :

         num = wb_status->wb_failed;
         break;

      default :

         num = ORPGRDA_DATA_NOT_FOUND;
         break;

   }

   return num;

} /* end ORPGRDA_get_wb_status */


/************************************************************************
 *	Description:  This module is to set up an RDA control command	*
 *		      and inform the control rda process to format and	*
 *		      send the command to the RDA.			*
 *                                                                      *
 *	Return:	      0 is returned on success, <0 on failure.		*
 ************************************************************************/
int ORPGRDA_send_cmd ( int cmd, int who_sent_it, int param1, int param2,
                       int param3, int param4, int param5, char* msg, ... )
{
   Rda_cmd_t	rda_cmd;
   int	status;

   /*	Define the elemants of the RDA control command structure.	*/

   rda_cmd.cmd      = cmd;
   rda_cmd.line_num = who_sent_it;
   rda_cmd.param1   = param1;
   rda_cmd.param2   = param2;
   rda_cmd.param3   = param3;
   rda_cmd.param4   = param4;
   rda_cmd.param5   = param5;
   rda_cmd.msg[0]   = '\0';

   if (msg != NULL){

      va_list ap;
      int msg_len;
  
      va_start( ap, msg );
      msg_len = va_arg( ap, int );
   
      if( msg_len > RDA_COMMAND_MAX_MESSAGE_SIZE )
         msg_len = RDA_COMMAND_MAX_MESSAGE_SIZE;

      memcpy (rda_cmd.msg, msg, msg_len);

   }

   /*	Write the command buffer to the RDA control command linear	*
    *	buffer.								*/

   status = ORPGDA_write (ORPGDAT_RDA_COMMAND, (char *) &rda_cmd,
      sizeof (Rda_cmd_t), LB_ANY);

   if (status <= 0) 
   {
      LE_send_msg (GL_INFO,
         "ORPGRDA: ORPGDA_write (ORPGDAT_RDA_COMMAND): ret (%d)\n", status);
      return status;
   }

   /*	Since the write to the LB was successfull, post an event so	*
    *	the RDA comm manager knows to read it.				*/

   status = EN_post (ORPGEVT_RDA_CONTROL_COMMAND, NULL, 0, 0);

   return status;

} /* end ORPGRDA_send_cmd */


/************************************************************************
 *									*
 *	Description:  This module is used to clear the RDA alarms list	*
 *		      so it will be reread.				*
 *									*
 ************************************************************************/
void ORPGRDA_clear_alarms ()
{
   ORPGRDA_num_alarms = 0;

   if (Alarm_data != NULL) 
   {
      free (Alarm_data);
      Alarm_data = NULL;
   }


} /* end ORPGRDA_clear_alarms */


/************************************************************************
 *									*
 *	Description:  This module is used get a copy of	the latest	*
 *		      rda alarm data from the RDA alarm	linear buffer.	*
 *									*
 *	Return:	      On success a value >= 0 is returned (indicating	*
 *		      the number of new alarm messages read.  On	*
 *		      failure, a value < 0 is returned.			*
 ************************************************************************/
int ORPGRDA_read_alarms ()
{
   int	status;
   char	data [sizeof (RDA_alarm_t)];
   RDA_alarm_t	*alarm;
   int	new;
   int	i;
   LB_status	alarm_status;  /*  Used to figure out how many msgs
                                   are in the Alarm LB  */

   alarm_status.n_check = 0;
   alarm_status.attr = NULL;
   alarm_status.check_list = NULL;
   new    = 0;

   /*	Check to see if RDA alarm message update events have been	*
    *	registered.  If not, register them.				*/

   if (!ORPGRDA_alarms_registered) 
   {
     status = ORPGDA_write_permission (ORPGDAT_RDA_ALARMS);

     status = ORPGDA_UN_register (ORPGDAT_RDA_ALARMS, LB_ANY,
        ORPGRDA_en_alarms_callback);

      if (status != LB_SUCCESS) 
      {
         LE_send_msg (GL_ERROR,
            "ORPGRDA: ORPGDA_UN_register (ORPGDAT_RDA_ALARMS) failed (ret %d)\n",
            status);
         return (status);
      }

      status = ORPGDA_UN_register (ORPGDAT_RDA_ALARMS,
         LB_MSG_EXPIRED, ORPGRDA_en_alarms_callback);

      if (status != LB_SUCCESS) 
      {
         LE_send_msg (GL_ERROR,
            "ORPGRDA: ORPGDA_UN_register (ORPGDAT_RDA_ALARMS) failed (ret %d)\n", status);
         return (status);
      }

      ORPGRDA_alarms_registered = 1;
   }

   /*	If there are no read alarms, assume we want to initialize the	*
    *	database by first positioning the file pointer to the beginning	*
    *	of the LB.							*/

   if (ORPGRDA_num_alarms == 0)
   {
      /*  Figure out the number of alarms in the Alarm buffer */

      status = ORPGDA_stat (ORPGDAT_RDA_ALARMS, &alarm_status);
      Alarm_io_status = status;

      if (status == LB_SUCCESS)
      {
         /*  If we have too many alarms for the initial read,
             seek to a point where we can read enough alarms to fill up
             the alarm array with on MULTI_READ -
             This is done to support low bandwidth hci  */

         if (alarm_status.n_msgs > ORPGRDA_MAX_ALARM_MSGS)
         {
            status = ORPGDA_seek (ORPGDAT_RDA_ALARMS,
               -(ORPGRDA_MAX_ALARM_MSGS-1), LB_LATEST, NULL);
         }
         else
         {
            status = ORPGDA_seek (ORPGDAT_RDA_ALARMS, 0, LB_FIRST, NULL);
         }

         Alarm_io_status = status;

         if (status < 0)
         {
            LE_send_msg (GL_INFO,
               "ORPGRDA: ORPGDA_seek (ORPGDAT_RDA_ALARMS): ret (%d)", status);
            return (status);
         }

         Alarm_data =
            (RDA_alarm_t *) calloc (ALIGNED_SIZE (sizeof (RDA_alarm_t)),
            ORPGRDA_MAX_ALARM_MSGS);

         /*  Read alarms with a multi-read */

         status = ORPGDA_read (ORPGDAT_RDA_ALARMS, (char *) Alarm_data,
            ALIGNED_SIZE (sizeof (RDA_alarm_t)) * ORPGRDA_MAX_ALARM_MSGS,
            LB_MULTI_READ | ORPGRDA_MAX_ALARM_MSGS);

         Alarm_io_status = status;

         if (status < 0)
         {
            LE_send_msg(GL_INFO,
               "ORPGRDA:  ORPGRDA_stat(ORPGDAT_RDA_ALARMS): ret(%d)", status);
            return(status);
         }

         new = ORPGRDA_num_alarms = (status / ALIGNED_SIZE (sizeof(RDA_alarm_t)));
      }
      else
      {
         LE_send_msg(GL_INFO,
            "ORPGRDA:  ORPGDA_stat(ORPGDAT_RDA_ALARMS): ret (%d)", status);
         return(status);
      }
   }

   alarm  = (RDA_alarm_t *) data;
   status = 1;

   /*	Loop until no more unread messages are found or if an error	*
    *	occurred.							*/

   Init_alarms_flag = 1;
   while (status > 0) 
   {
      status = ORPGDA_read (ORPGDAT_RDA_ALARMS, (char *) &data,
         sizeof (RDA_alarm_t), LB_NEXT);

      Alarm_io_status  = status;

      if (status > 0) 
      {
         RDA_status_update_time = time (NULL);

         /* If the database already full, shift out the oldest	*
          * message and add the newest at the end.			*/

         if (ORPGRDA_num_alarms > ORPGRDA_MAX_ALARM_MSGS-1) 
         {
            for (i=1;i<=ORPGRDA_MAX_ALARM_MSGS-1;i++) 
            {
               Alarm_data [i-1] = Alarm_data [i];
            }

            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].month   = alarm->month;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].day     = alarm->day;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].year    = alarm->year;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].hour    = alarm->hour;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].minute  = alarm->minute;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].second  = alarm->second;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].code    = alarm->code;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].alarm   = alarm->alarm;
            Alarm_data [ORPGRDA_MAX_ALARM_MSGS-1].channel = alarm->channel;

            /*	Otherwise, add the message after the last one read.	*/

         }
         else
         {
            Alarm_data [ORPGRDA_num_alarms].month   = alarm->month;
            Alarm_data [ORPGRDA_num_alarms].day     = alarm->day;
            Alarm_data [ORPGRDA_num_alarms].year    = alarm->year;
            Alarm_data [ORPGRDA_num_alarms].hour    = alarm->hour;
            Alarm_data [ORPGRDA_num_alarms].minute  = alarm->minute;
            Alarm_data [ORPGRDA_num_alarms].second  = alarm->second;
            Alarm_data [ORPGRDA_num_alarms].code    = alarm->code;
            Alarm_data [ORPGRDA_num_alarms].alarm   = alarm->alarm;
            Alarm_data [ORPGRDA_num_alarms].channel = alarm->channel;

            ORPGRDA_num_alarms++;
         }

         new++;

      }
   }

   return new;

} /* end ORPGRDA_read_alarms */


/************************************************************************
 *									*
 *	Description: The following routine writes the RDA Alarm		*
 *		     message (RDA_GSM_DATA: RDA_STATUS_ID).		*
 *									*
 *	Return:      On success, 0 is returned.  On failure, a value	*
 *		     < 0 is returned.					*
 *									*
 ************************************************************************/
int ORPGRDA_write_alarm_msg ( char* alarm_msg )
{
   int	status;

   if (alarm_msg == (char *) NULL) 
   {
      return (-1);
   }
   else
   {
      status = ORPGDA_write (ORPGDAT_RDA_ALARMS, alarm_msg,
         sizeof (RDA_alarm_t), LB_ANY);
   }

   if (status < 0) 
   {
      LE_send_msg (GL_INFO, "ORPGDA_read (ORPGDAT_RDA_ALARMS: %d\n", status);
   }

   return status;

} /* end ORPGRDA_write_alarm_msg */


/************************************************************************
 *									*
 *	Description:	The following function returns the value for a	*
 *			specific item in a specified RDA alarm message	*
 *			element.					*
 *									*
 ************************************************************************/
int ORPGRDA_get_alarm ( int indx, int item)
{
   int	value;
   int	status;

   /*	If the init alarms flag is 0, then we need to read the RDA	*
    *	alarms message.							*/

   if (!Init_alarms_flag) 
   {
      status = ORPGRDA_read_alarms ();

      if (status < 0) 
      {
         return (status);
      }
   }

   /*	If the requested alarm element does not exist, return.		*/

   if ((ORPGRDA_num_alarms <= indx) || (indx < 0)) 
   {
      return (ORPGRDA_DATA_NOT_FOUND);
   }

   /*	Cast the requested alarm element.				*/

   switch (item) 
   {
      case ORPGRDA_ALARM_MONTH :
         value = Alarm_data [indx].month;
         break;

      case ORPGRDA_ALARM_DAY :
         value = Alarm_data [indx].day;
         break;

      case ORPGRDA_ALARM_YEAR :
         value = Alarm_data [indx].year;
         break;

      case ORPGRDA_ALARM_HOUR :
         value = Alarm_data [indx].hour;
         break;

      case ORPGRDA_ALARM_MINUTE :
         value = Alarm_data [indx].minute;
         break;

      case ORPGRDA_ALARM_SECOND :
         value = Alarm_data [indx].second;
         break;

      case ORPGRDA_ALARM_CODE :
         value = Alarm_data [indx].code;
         break;

      case ORPGRDA_ALARM_ALARM :
         value = Alarm_data [indx].alarm;
         break;

      case ORPGRDA_ALARM_CHANNEL :
         value = Alarm_data [indx].channel;
         break;

      default :
         value = -1;
         break;
   }

   return value;

} /* end ORPGRDA_get_alarm */


/************************************************************************
 *									*
 *	Description:	The following function returns the number of	*
 *			RDA Alarm messages stored in the RDA Alarm	*
 *			array (Alarm_data[]).				*
 *									*
 ************************************************************************/
int ORPGRDA_get_num_alarms ()
{
   int	status = 0;

   /*	If the init alarms flag is 0, then we need to read the RDA	*
    *	alarms message.							*/

   if (!Init_alarms_flag) 
   {
      status = ORPGRDA_read_alarms ();

      if (status < 0) 
      {
         return (0);
      }
   }

   return ORPGRDA_num_alarms;

} /* end ORPGRDA_get_num_alarms */


/***************************************************************************
* Function Name:
*	ORPGRDA_get_rda_config
*
* Description:
*	Determine the RDA configuration.
*
* Inputs:
*	short* msg_ptr  - A pointer to a message including the message
*			  header or NULL.
*
* Return:
*	4-byte integer representing the configuration.  There are
*	macros defined in the orpgrda.h include file that are used
*	to represent the return value:
*
*	ORPGRDA_LEGACY_CONFIG	- Legacy RDA Configuration
*	ORPGRDA_ORDA_CONFIG	- Open RDA Configuration
*	ORPGRDA_DATA_NOT_FOUND	- Problem getting the configuration
***************************************************************************/
int ORPGRDA_get_rda_config( void* msg_ptr )
{
   int status                  = 0;
   int rda_config              = ORPGRDA_DATA_NOT_FOUND;

   if ( msg_ptr == NULL )
   {  /* use internal data */

      /* Initialize Rda_config_flag if it hasn't been already */
      Init_rda_config();

      return Rda_config_flag;
   
   }
   else
   {  /* use this msg to get the RDA config */

      if( (status = Get_rda_config_from_msg_hdr( msg_ptr, &rda_config ))
          != ORPGRDA_ERROR )
         return rda_config;

      return ORPGRDA_DATA_NOT_FOUND;
   }

} /* end ORPGRDA_get_rda_config */


/*******************************************************************************
* Function Name:
*	ORPGRDA_get_alarm_codes
*
* Description:
*	Fills an input array with alarm code values from the latest RDA
*	Status Message.
*
* Inputs:
*	int*	Pointer to integer array space where alarm info will be stored.
*
* Return:
*	ORPGRDA_DATA_NOT_FOUND	:  problem reading status message
*	ORPGRDA_SUCCESS		:  no problems encountered
*******************************************************************************/
int ORPGRDA_get_alarm_codes(int* alarm_code_ptr)
{
   RDA_status_msg_t	*rda_status;
   ORDA_status_msg_t	*orda_status;
   int alarm_ind	= 0;
   int ret_val		= ORPGRDA_SUCCESS;
   int status		= 0;


   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();

      if (status <= 0) 
      {
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   /* Cast structure pointer to the data */
   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      rda_status = (RDA_status_msg_t *) &Rda_status->status_msg;
      for ( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
      {
         alarm_code_ptr[alarm_ind] = (int) rda_status->alarm_code[alarm_ind];
      }
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      orda_status = (ORDA_status_msg_t *) &Orda_status->status_msg;
      for ( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
      {
         alarm_code_ptr[alarm_ind] = (int) orda_status->alarm_code[alarm_ind];
      }
   }
   else
   {
      LE_send_msg( GL_INFO,
         "ORPGRDA_get_alarm_codes: invalid Rda_config_flag.\n");
      ret_val = ORPGRDA_ERROR;     
   }

   return ret_val;

} /* end ORPGRDA_get_alarm_codes */


/*******************************************************************************
* Function Name:
*	ORPGRDA_check_status_change
*
* Description:
*	Compares the new status message with the old one.  If there are
*	differences, ORPGRDA_STATUS_CHANGED is returned.  Otherwise, 
*	ORPGRDA_STATUS_UNCHANGED is returned.
*
* Inputs:
*	short*	new_msg_data	Pointer to new RDA Status Msg data, not
*                               including msg hdr.
*
* Return:
*	ORPGRDA_STATUS_UNCHANGED	:  no status changes
*	ORPGRDA_STATUS_CHANGED		:  status changes
*	ORPGRDA_ERROR			:  error occured
*	ORPGRDA_DATA_NOT_FOUND		:  problem reading current status msg
*******************************************************************************/
int ORPGRDA_check_status_change ( short* new_msg_data )
{
   int		ret		= 0;
   int		field_index	= 0;
   int		status		= ORPGRDA_STATUS_UNCHANGED;
   short*	old_msg_data	= 0;


   /* If status msg has been updated, read the new one. */
   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      ret = ORPGRDA_read_status_msg ();

      if (ret <= 0) 
      {
         return (ORPGRDA_DATA_NOT_FOUND);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   /* Assign the data pointers.  Note: because we are interested in comparing
      only the RDA status data (not the msg hdr), we need to increment the
      pointer past the msg hdr. */
   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      old_msg_data = ((short *) (&Rda_status->status_msg )) +
         ORPGRDA_MSG_HEADER_SIZE;
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      old_msg_data = ((short *) (&Orda_status->status_msg )) +
         ORPGRDA_MSG_HEADER_SIZE;
   }
   else
   {
      LE_send_msg ( GL_ERROR,
         "ORPGRDA_check_status_change: invalid RDA configuration flag\n");
      return ORPGRDA_ERROR;
   }

   /* Check each field of the status message to see if it has changed. If it has
      changed, set the return value appropriatly and return. */
   for(field_index=0; field_index<ORPGRDA_RDA_STATUS_MSG_SIZE; field_index++)
   {
      if( new_msg_data[field_index] != old_msg_data[field_index] )
      {
         status = ORPGRDA_STATUS_CHANGED;
         break;
      }
   }

   return status;

} /* end ORPGRDA_check_status_change */


/*******************************************************************************
* Function Name:
*	ORPGRDA_set_wb_status
*
* Description:
*	Sets the desired wideband comms field (argument 1) to the desired
*	value (argument 2).
*
* Inputs:
*
* Return:
*	ORPGRDA_ERROR		:  problem reading status message
*	ORPGRDA_DATA_NOT_FOUND	:  field_id was not found
*	ORPGRDA_SUCCESS		:  no problems encountered
*******************************************************************************/
int ORPGRDA_set_wb_status ( int field_id, int value )
{
   int ret_val		= ORPGRDA_SUCCESS;
   int status		= 0;


   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      status = ORPGRDA_read_status_msg ();

      if (status <= 0) 
      {
         return (ORPGRDA_ERROR);
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   switch ( field_id ) 
   {
      case ORPGRDA_WBLNSTAT :

         if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
            Rda_status->wb_comms.wblnstat = value;
         else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
            Orda_status->wb_comms.wblnstat = value;
         else
         {
            LE_send_msg( GL_ERROR,
               "ORPGRDA_set_wb_status: invalid RDA config flag\n");
            ret_val = ORPGRDA_ERROR;
         }
         break;

      case ORPGRDA_DISPLAY_BLANKING :

         if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
            Rda_status->wb_comms.rda_display_blanking = value;
         else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
            Orda_status->wb_comms.rda_display_blanking = value;
         else
         {
            LE_send_msg( GL_ERROR,
               "ORPGRDA_set_wb_status: invalid RDA config flag\n");
            ret_val = ORPGRDA_ERROR;
         }
         break;

      case ORPGRDA_WBFAILED :

         if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
            Rda_status->wb_comms.wb_failed = value;
         else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
            Orda_status->wb_comms.wb_failed = value;
         else
         {
            LE_send_msg( GL_ERROR,
               "ORPGRDA_set_wb_status: invalid RDA config flag\n");
            ret_val = ORPGRDA_ERROR;
         }
         break;

      default :

         ret_val = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return ret_val;

} /* end ORPGRDA_set_wb_status */


/*******************************************************************************
* Function Name:
*	ORPGRDA_set_status
*
* Description:
*	Sets the desired RDA status field (argument 1) to the desired
*	value (argument 2).
*
* Inputs:
*	int	field_id	The integer value representing the status field
*				to be set.  Macros representing these values
*				can be found in the orpgrda.h file.
*	int	value		The integer value representing the value to set
*				status field to.
*
* Return:
*	ORPGRDA_ERROR		:  problem reading status message
*	ORPGRDA_DATA_NOT_FOUND	:  field_id was not found
*	ORPGRDA_SUCCESS		:  no problems encountered
*******************************************************************************/
int ORPGRDA_set_status ( int field_id, int value )
{
   int	ret	= 0;
   int	status	= ORPGRDA_SUCCESS;


   /* First read the new status msg if necessary */
   if (!Init_status_flag) 
   {
      Init_status_flag = 1;
      ret = ORPGRDA_read_status_msg ();

      if ( ret <= 0 )
      {
         return ( ORPGRDA_ERROR ); 
      }
   }

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      status = Set_rda_status( field_id, value );
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      status = Set_orda_status( field_id, value );
   }
   else
   {
      LE_send_msg( GL_ERROR, "ORPGRDA_set_status: invalid RDA config flag\n");
      status = ORPGRDA_ERROR;
   }

   return status;

} /* end ORPGRDA_set_status */


/*******************************************************************************
* Function Name:
*	ORPGRDA_read_previous_state
*
* Description:
*	Read and store the previous state.  This routine attempts to read the
*	previous state info from the RDA status LB and store it in the lib's
*	copy of the previous state struct.  
*
* Inputs:
*
* Return:
*	ORPGRDA_SUCCESS	:	Successful completion.
*	ORPGRDA_ERROR	:	Error encountered.
*******************************************************************************/
int ORPGRDA_read_previous_state ()
{
   int	ret	= 0;
   int	ret_val	= ORPGRDA_SUCCESS;


   ret = ORPGDA_read( ORPGDAT_RDA_STATUS, (char *) &Previous_state,
      sizeof( Previous_state_t ), PREV_RDA_STAT_ID );
   if ( ret <= 0 )
   {
      LE_send_msg( GL_ERROR, "ORPGRDA_read_previous_state() Failed (%d)\n", ret );
      ret_val = ORPGRDA_ERROR;
   }

   return ret_val;
}


/*******************************************************************************
* Function Name:
*	ORPGRDA_set_state
*
* Description:
*	Sets the previous state values.
*
* Inputs:
*
* Return:
*	ORPGRDA_SUCCESS	:	Successful completion.
*	ORPGRDA_ERROR	:	Error encountered.
*******************************************************************************/
int ORPGRDA_set_state ( int field_id, int value )
{
   int ret_val = ORPGRDA_SUCCESS;
 
   switch ( field_id )  
   {
      case ORPGRDA_RDA_STATUS :	

         Previous_state.rda_status = (unsigned short) value;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :

         Previous_state.data_trans_enbld = (unsigned short) value;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH:

         Previous_state.rda_control_auth = (short) value;
         break;

      case ORPGRDA_ISU :

         if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
         {
            LE_send_msg( GL_ERROR,
               "ORPGRDA_set_state: ISU not valid for ORDA.\n");
            ret_val = ORPGRDA_DATA_NOT_FOUND;
         }
         else
         {
            Previous_state.int_suppr_unit = (short) value;
         }
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :

         Previous_state.spot_blanking_status = (short) value;
         break;

      case ORPGRDA_VCP_NUMBER :

         Previous_state.current_vcp_number = (short) value;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :

         Previous_state.channel_control = (short) value;
         break;

      default:

         ret_val = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return ret_val;

} /* end ORPGRDA_set_state */


/*******************************************************************************
* Function Name:
*       ORPGRDA_set_state_vcp
*
* Description:
*       Sets the previous state vcp.
*
* Inputs:
*
* Return:
*       ORPGRDA_SUCCESS :       Successful completion.
*       ORPGRDA_ERROR   :       Error encountered.
*******************************************************************************/
int ORPGRDA_set_state_vcp ( char *vcp, int size )
{
   int ret_val = ORPGRDA_SUCCESS;
   int checkpoint, i;
   Vcp_struct *pvcp = &Previous_state.current_vcp_table;
   Vcp_struct *cvcp = (Vcp_struct *) vcp;

   /* Handle the case where the vcp message is undefined. */
   if( vcp == NULL ){

      memset( &Previous_state.current_vcp_table, 0, sizeof(Vcp_struct) );
      return ret_val;

   }

   /* Check if previous state VCP data and passed VCP data are different. */
   checkpoint = 0;
   if( (pvcp != NULL) && (cvcp != NULL) ){

      if( (pvcp->msg_size != cvcp->msg_size)
                          ||
          (pvcp->vcp_num != cvcp->vcp_num)
                          ||
          (pvcp->n_ele != cvcp->n_ele) )
          checkpoint = 1;

      if( !checkpoint ){
 
         Ele_attr *p_ele_attr = (Ele_attr *) &pvcp->vcp_ele[0][0];
         Ele_attr *c_ele_attr = (Ele_attr *) &cvcp->vcp_ele[0][0];

         for( i = 0; i < pvcp->n_ele; i++ ){

            if( p_ele_attr->ele_angle != c_ele_attr->ele_angle ){

               checkpoint = 1;
               break;
            
            }

         }

      }

   }
    
   /* Copy the VCP data. */
   memset( &Previous_state.current_vcp_table, 0, sizeof(Vcp_struct) );
   memcpy( &Previous_state.current_vcp_table, vcp, size );

   /* Do we need to checkpoint the previous state. */
   if( checkpoint ){

      LE_send_msg( GL_INFO, "Previous State VCP Definition Changed. Write Previous State.\n" ); 
      ORPGRDA_write_state();

   }
   
   /* Set this flag for when ORPGRDA_report_previous_state() is called,
      the VCP data is only written to log file if it has changed. */   
   Previous_state_vcp_data_updated = 1;

   return ret_val;

} /* end ORPGRDA_set_state_vcp */

 
/*******************************************************************************
* Function Name:
*	ORPGRDA_write_state
*
* Description:
*	Writes the previous state values to the RDA Status LB.
*
* Inputs:
*
* Return:
*	ORPGRDA_SUCCESS	:	Successful completion.
*	ORPGRDA_ERROR	:	Error encountered.
*******************************************************************************/
int ORPGRDA_write_state ()
{
   int	status = 0;


   status = ORPGDA_write (ORPGDAT_GSM_DATA, (char *) &Previous_state,
      sizeof (Previous_state_t), PREV_RDA_STAT_ID);

   if (status < 0) 
   {
      LE_send_msg (GL_INFO, "ORPGDA_write (PREV_RDA_STAT_ID): %d\n", status);
      status = ORPGRDA_ERROR;
   }
   else
   {
      status = ORPGRDA_SUCCESS;
   }

   return status;
} /* end ORPGRDA_write_state */


/*******************************************************************************
* Function Name:
*	ORPGRDA_get_previous_state
*
* Description:
*	Gets the previous state values.
*
* Inputs:
*
* Return:
*	ORPGRDA_DATA_NOT_FOUND	: Successful completion.
*	int value		: The integerized value of the desired previous
*				  state field.
*******************************************************************************/
int ORPGRDA_get_previous_state ( int field_id )
{
   int value 	= 0;
 
   switch ( field_id )  
   {
      case ORPGRDA_RDA_STATUS :	

         value = (int) Previous_state.rda_status;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :

         value = (int) Previous_state.data_trans_enbld;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH:

         value = (int) Previous_state.rda_control_auth;
         break;

      case ORPGRDA_ISU :

         if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
         {
            LE_send_msg( GL_ERROR,
               "ORPGRDA_get_previous_state: ISU not valid for ORDA.\n");
            value = ORPGRDA_DATA_NOT_FOUND;
         }
         else
         {
            value = (int) Previous_state.int_suppr_unit;
         }
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :

         value = (int) Previous_state.spot_blanking_status;
         break;

      case ORPGRDA_VCP_NUMBER :

         value = (int) Previous_state.current_vcp_number;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :

         value = (int) Previous_state.channel_control;
         break;

      default:

         value = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return value;

} /* end ORPGRDA_get_previous_state */


/*******************************************************************************
* Function Name:
*       ORPGRDA_get_previous_state_vcp
*
* Description:
*       Gets the previous state vcp
*
* Inputs:
*       vcp - pointer to buffer of at least size Vcp_struct to hold vcp data
*
* Outputs:
*       size - holds the size of the VCP data, in bytes.
*
* Return:
*       ORPGRDA_DATA_NOT_FOUND  : Successful completion.
*       int value               : The size of the VCP data.
*                                 
*******************************************************************************/
int ORPGRDA_get_previous_state_vcp ( char *vcp, int *size )
{

   int bsize = 0;
   Previous_state_t *ps = &Previous_state;

   /* Check if data is available. */
   bsize = Previous_state.current_vcp_table.msg_size*sizeof(short);
   if( bsize == 0 ){

      *size = 0;
      return ORPGRDA_DATA_NOT_FOUND;

   }

   /* Check the VCP data against the VCP number.  They should
      match.  If not, return error. */
   if( ps->current_vcp_table.vcp_num != ps->current_vcp_number ){

      memset( vcp, 0, sizeof(Vcp_struct) );
      *size = 0;
      return ORPGRDA_DATA_NOT_FOUND;

   }

   /* Copy the data and set the size. */
   memcpy( vcp, &Previous_state.current_vcp_table, bsize );
   *size = bsize;

   return bsize;

} /* end ORPGRDA_get_previous_state_vcp() */

/*******************************************************************************
* Function Name:
*	ORPGRDA_clear_alarm_codes
*
* Description:
*	Zeroes out the alarm codes in the status structure.
*
* Inputs:
*
* Return:
*	ORPGRDA_ERROR	: Error encountered.
*	ORPGRDA_SUCCESS	: Successful completion.
*******************************************************************************/
int ORPGRDA_clear_alarm_codes ()
{
   int	ret	= 0;
   int	ret_val	= ORPGRDA_SUCCESS;
 

   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE1, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 1.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE2, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 2.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE3, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 3.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE4, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 4.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE5, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 5.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE6, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 6.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE7, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 7.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE8, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 8.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE9, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 9.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE10, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 10.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE11, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 11.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE12, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 12.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE13, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 13.\n");
      ret_val = ORPGRDA_ERROR;
   }
   ret = ORPGRDA_set_status ( ORPGRDA_ALARM_CODE14, 0 );
   if ( ret != ORPGRDA_SUCCESS ) 
   {
      LE_send_msg( GL_ERROR, "Unable to clear alarm code 14.\n");
      ret_val = ORPGRDA_ERROR;
   }

   return ret_val;

} /* end ORPGRDA_clear_alarm_codes */


/*******************************************************************************
* Function Name:
*	ORPGRDA_get_previous_status
*
* Description:
*	Gets the previous RDA status values.
*
* Inputs:
*	int	field_id	The numeric structure field ID.  These are 
*				defined in orpgrda.h.
*
* Return:
*	ORPGRDA_DATA_NOT_FOUND	: Successful completion.
*	int value		: The integerized value of the desired previous
*				  status field.
*******************************************************************************/
int ORPGRDA_get_previous_status ( int field_id )
{
   int	value 	= 0;

   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      value = Get_previous_rda_status( field_id );
   }
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
   {
      value = Get_previous_orda_status( field_id );
   }
   else
   {
      LE_send_msg (GL_ERROR,
         "ORPGRDA_get_previous_status: invalid RDA config flag\n");
      value = ORPGRDA_ERROR;
   }

   return value;

} /* end ORPGRDA_get_previous_status */


/*******************************************************************************
* Function Name:
*	ORPGRDA_update_system_status
*
* Description:
*       Writes RDA status data to system status log file in plain text 
*       format.  Only status which is different than last reported is
*       reported.
*
* Inputs:
*	none
*
* Return:
*	void
*******************************************************************************/
void ORPGRDA_update_system_status()
{
   /* Initialize config flag if it hasn't been already */
   Init_rda_config();

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
      Rda_update_system_status();
   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
      Orda_update_system_status();
   else
      LE_send_msg( GL_INFO,
         "ORPGRDA_update_system_status: invalid RDA config flag.\n");

} /* End of ORPGRDA_update_system_status() */


/*******************************************************************************
* Function Name:
*	ORPGRDA_report_previous_state
*
* Description:
*       Writes Previous RDA state data to task log file.
*
* Inputs:
*	none
*
* Return:
*	void
*******************************************************************************/
void ORPGRDA_report_previous_state()
{
   int		i			= 0;
   int		stat			= 0;
   int		p_rda_stat		= 0;
   int		p_vcp			= 0;
   int		p_rda_contr_auth	= 0;
   int		p_int_supp_unit		= 0;
   int		p_chan_control		= 0;
   int		p_spot_blank_stat	= 0;
   double	deau_ret_val		= 0.0;

   Vcp_struct   p_vcp_data              = {0};
   int          p_vcp_data_ret		= 0;
   int          p_vcp_data_size		= 0;

   /* Retrieve and store data fields */
   p_rda_stat = ORPGRDA_get_previous_state( ORPGRDA_RDA_STATUS );
   if ( p_rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_RDA_STATUS\n");
      p_rda_stat = 0;  /* Reset value to 0 */
   }
   p_vcp = ORPGRDA_get_previous_state( ORPGRDA_VCP_NUMBER );
   if ( p_vcp == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_VCP_NUMBER\n");
      p_vcp = 0;  /* Reset value to 0 */
   }
   p_rda_contr_auth = ORPGRDA_get_previous_state( ORPGRDA_RDA_CONTROL_AUTH );
   if ( p_rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_RDA_CONTROL_AUTH\n");
      p_rda_contr_auth = 0;  /* Reset value to 0 */
   }
   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      p_int_supp_unit = ORPGRDA_get_previous_state( ORPGRDA_ISU );
      if ( p_int_supp_unit == ORPGRDA_DATA_NOT_FOUND )
      {
         LE_send_msg(GL_ERROR,
            "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_ISU\n");
         p_int_supp_unit = 0;  /* Reset value to 0 */
      }
   }

   p_chan_control = ORPGRDA_get_previous_state( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( p_chan_control == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_CHAN_CONTROL_STATUS\n");
      p_chan_control = 0;  /* Reset value to 0 */
   }

   p_spot_blank_stat = ORPGRDA_get_previous_state( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( p_spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving ORPGRDA_SPOT_BLANKING_STATUS\n");
      p_spot_blank_stat = 0;  /* Reset value to 0 */
   }

   p_vcp_data_ret = ORPGRDA_get_previous_state_vcp( (char *) &p_vcp_data, &p_vcp_data_size );
   if ((p_vcp_data_ret == ORPGRDA_DATA_NOT_FOUND)
                       ||
       (p_vcp_data_size == 0) )
   {
      LE_send_msg(GL_ERROR,
         "ORPGRDA_report_previous_state: Problem retrieving VCP Data (%d, %d)\n",
         p_vcp_data_ret, p_vcp_data_size );
   }

   /* case RS_RDA_STATUS  */
   if( (p_rda_stat & RS_STARTUP) )
      i = 0;

   else if( (p_rda_stat & RS_STANDBY) )
      i = 1;

   else if( (p_rda_stat & RS_RESTART) )
      i = 2;

   else if( (p_rda_stat & RS_OPERATE) )
      i = 3;

   else if( (p_rda_stat & RS_PLAYBACK) )
      i = 4;

   else if( (p_rda_stat & RS_OFFOPER) )
      i = 5;

   else{

      /* Unknown value. */
      i = 6;
      sprintf( status[i], "%6d", p_rda_stat );

   }

   LE_send_msg( GL_INFO, "Previous RDA Status: %s\n", status[i] );

   /* case RS_VCP_NUMBER */
   {
      char temp[10];

      /* Clear temporary buffer. */
      memset( temp, 0, 10 );

      /* Determine if vcp is "local" or "remote" pattern. */
      if( p_vcp < 0 )
      {
         p_vcp = -p_vcp;
         temp[0] = 'L';
      }
      else
         temp[0] = 'R';

      /* Encode VCP number. */
      sprintf( &temp[1], "%d", p_vcp );

      LE_send_msg( GL_INFO, "Previous RDA VCP: %s\n", temp );

   }

   /* case RS_RDA_CONTROL_AUTH */


   if( p_rda_contr_auth != CA_NO_ACTION )
   {

      if( p_rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
      {
         i = 0;
      }
      else if( p_rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
      {
         i = 1;
      }
      LE_send_msg( GL_INFO, "Previous RDA Control Authorization: %s\n", authority[i] );
   }

   /* case RS_ISU - only valid for Legacy RDA */

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      if( p_int_supp_unit != ISU_NOCHANGE )
      {
         if( p_int_supp_unit == ISU_ENABLED )
         {
            i = 0;
         }
         else if( p_int_supp_unit == ISU_DISABLED )
         {
            i = 1;
         }
         else
         {
            /* Unknown value. Place value in status buffer. */
            i = 2;
            sprintf( interference_unit[i], "%6d", p_int_supp_unit );
         }
         LE_send_msg( GL_INFO, "Previous RDA ISU: %s\n", interference_unit[i] );
      }
   }

   /* case RS_CHAN_CONTROL_STATUS */

   if ( (stat =
      DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1)) >= 0)
   {
      if ( (int) deau_ret_val == ORPGSITE_FAA_REDUNDANT )
      {
         if( p_chan_control == RDA_IS_CONTROLLING )
         {
            i = 0;
         }
         else if( p_chan_control == RDA_IS_NON_CONTROLLING )
         {
            i = 1;
         }
         else
         {
            /* Unknown value.  Place value in status buffer. */
            i = 2;
            sprintf( channel_status[i], "%6d", p_chan_control );
         }
         LE_send_msg( GL_INFO, "Previous RDA Channel Control: %s\n", channel_status[i] );
      }
   }
   else
   {
      LE_send_msg( GL_INFO | GL_ERROR,
         "ORPGRDA_report_previous_state: call to DEAU_get_values returned error.\n");
   }


   /* case RS_SPOT_BLANKING_STATUS */


   /* If spot blanking not installed, break. */
   if( p_spot_blank_stat != SB_NOT_INSTALLED )
   {
      /* Process spot blanking status. */ 
      if( p_spot_blank_stat == SB_ENABLED )
      {
         i = 1;
      }
      else if( p_spot_blank_stat == SB_DISABLED )
      {
         i = 2;
      }
      else
      {
         /* Unknown value. Place value in status buffer. */
         i = 3;
         sprintf( spot_blanking[i], "%6d", p_spot_blank_stat );
      }
      LE_send_msg( GL_INFO, "Previous RDA Spot Blanking Status: %s\n", spot_blanking[i] );
   }

   /* Write out the VCP data if the data is available. */
   if ((p_vcp_data_ret != ORPGRDA_DATA_NOT_FOUND)
                       &&
       (p_vcp_data_size != 0)
                       &&
       (Previous_state_vcp_data_updated) ){

      Previous_state_vcp_data_updated = 0;
      Write_vcp_data( &p_vcp_data );
   }

} /* End of ORPGRDA_report_previous_state() */


/*****************************************************************************
*   Description:
*      Controls writing RDA status data to the system log file.  If status
*      to be written to status buffer causes the buffer to overflow, the buffer
*      is written to the status log before the new data is copied. 
*
*   Inputs:
*      buf - address of status buffer.
*      len - address of length the status buffer string.
*      field_id - string containing the ID of the field to write to status
*                 buffer.
*     field_val - string containing the field value.
*
*   Outputs:
*      buf - status buffer containing new status data.
*      len - length of the string with new status data.
*
*   Returns:
*      There is no return value define for this function.
*
*   Notes:
*
*****************************************************************************/
static void Process_status_field(char **buf, int *len, char *field_id, char *field_val)
{
   int comma_and_space;

   /* If there is previous status in the buffer, will need to
      append a comma and a space. */
   if( strcmp( *buf, "RDA STATUS:") != 0 )
      comma_and_space = 2;
   else
      comma_and_space = 0;

   /* Is the buffer large enoough to accommodate the new status. */
   if( (*len + strlen(field_id) + strlen(field_val) + comma_and_space) > 
       MAX_STATUS_LENGTH )
   {
      /* Buffer not large enough.  Write the message to the status log. */
      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", *buf );

      /* Initialize status buffer. */
      *buf = memset( *buf, 0, (MAX_STATUS_LENGTH+1) );

      /* Place header string in buffer. */
      strcat( *buf, "RDA STATUS:" );
      *len = strlen( *buf );
      comma_and_space = 0;
   }      

   /* Append a comma and space, if required. */
   if( comma_and_space )
      strcat( *buf, ", " );

   /* Append status ID and status value to buffer. */
   strcat( *buf, field_id );
   strcat( *buf, field_val );

   /* Determine new length of status buffer. */
   *len = strlen( *buf );

} /* End of Process_status_field() */ 


/***************************************************************************
* Function Name:
*       Write_rda_status_msg
*
* Description:
*	Called from ORPGRDA_write_status_msg, this function performs the 
*	write for Legacy RDA status messages.
*
* Inputs:
*	msg_ptr - character pointer to status data, or NULL
*
* Return:
*	status - integer status of ORPGDA_write operation.
***************************************************************************/
static int Write_rda_status_msg(char* msg_ptr){

   int status = 0;

   if (msg_ptr == (char *) NULL)
      status = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) Status_data,
                             sizeof (RDA_status_t), RDA_STATUS_ID );
    
   else{

      Store_new_status_data ( msg_ptr );
      status = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) Status_data,
                             sizeof (RDA_status_t), RDA_STATUS_ID );
   }

   return status;

} /* end Write_rda_status_msg */


/***************************************************************************
* Function Name:
*       Write_orda_status_msg
*
* Description:
*	Called from ORPGRDA_write_status_msg, this function performs the 
*	write for Open RDA status messages.
*
* Inputs:
*	msg_ptr - character pointer to status data, or NULL
*
* Return:
*	status - integer status of ORPGDA_write operation.
***************************************************************************/
static int Write_orda_status_msg( char* msg_ptr ){

   int status = 0;

   if (msg_ptr == (char *) NULL)
      status = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) Status_data,
                             sizeof (ORDA_status_t), RDA_STATUS_ID );
    
   else{

      Store_new_status_data ( msg_ptr );
      status = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) Status_data,
                             sizeof (ORDA_status_t), RDA_STATUS_ID );
   }

   return status;

} /* end Write_orda_status_msg */

/*******************************************************************************
* Function Name:
*       Store_new_status_data
*
* Description:
*       Copies the data pointed to by the input argument into the internal
*       status message buffer.
*
* Inputs:
*       status_data         Pointer to the new RDA Status data,
*                                 including the message header.
*
* Return:
*       ORPGRDA_ERROR   : Error encountered.
*       ORPGRDA_SUCCESS : Successful completion.
*******************************************************************************/
static int Store_new_status_data ( char* status_data ){

   int  ret_val = ORPGRDA_SUCCESS;

   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
      memcpy( &Rda_status->status_msg, status_data, sizeof(RDA_status_msg_t) );

   else if ( Rda_config_flag == ORPGRDA_ORDA_CONFIG )
      memcpy( &Orda_status->status_msg, status_data, sizeof(ORDA_status_msg_t) );

   else{

      LE_send_msg ( GL_ERROR,
         "Store_new_status_data: invalid RDA configuration flag\n");
      ret_val = ORPGRDA_ERROR;

   }

   return ret_val;

} /* end Store_new_status_data */


/***************************************************************************
* Function Name:
*       ORPGRDA_set_rda_config
*
* Description:
*	Determines the RDA configuration (Legacy vs. Open RDA).
*
* Inputs:
*	msg_ptr - pointer to a RDA status message or NULL
*
* Return:
*	int - ORPGRDA_SUCCESS or ORPGRDA_ERROR
*
* Note:
*       This function is not intended for general public use.
*       It should only be used by tasks authorized to change
*       the configuration.
***************************************************************************/
int ORPGRDA_set_rda_config( void* msg_ptr )
{
   int				ret		= 0;
   int				ret_val		= ORPGRDA_SUCCESS;
   int                          tmp_rda_cfg     = 0; 
   RDA_RPG_message_header_t*    msg_hdr_ptr     = NULL;
   char*                        ds_name         = NULL;
   double			rda_cfg_val;

   /* "msg_ptr" must be non-NULL. */
   if ( msg_ptr == NULL )
   {   

      /* The current value of the RDA config flag will persist. */
      ret_val = ORPGRDA_ERROR;
   }
   else
   { 

      /* Assume "msg_ptr" points to an ICD message header. */
      msg_hdr_ptr = (RDA_RPG_message_header_t *) msg_ptr;

      /* Extract the RDA Configuration from the message header. */
      ret_val = Get_rda_config_from_msg_hdr( msg_hdr_ptr, &tmp_rda_cfg );

      /* If successful and the configuration in the message is 
         different than what is stored in Rda_config_flag, set
         the new RDA Configuration is adaptation data as well
         as changing the value of RDA_config_flag. */
      if( (ret_val != ORPGRDA_ERROR) && (tmp_rda_cfg != Rda_config_flag) )
      {
         rda_cfg_val = (double) tmp_rda_cfg;
            
         /* Need to change this in adaptation data as well */
         ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
         if( ds_name != NULL ){

            /* Change adaptation data first to guarantee adaptation
               data and Rda_config_flag maintain consistency. */
            DEAU_LB_name( ds_name );
            ret = DEAU_set_values ( RDA_CONFIGURATION, 0, (void*)&rda_cfg_val,
                  1, 0);

            if( ret == 0 ){

               LE_send_msg( GL_INFO, "Setting RDA Configuration From %d to %d\n",
                            Rda_config_flag, tmp_rda_cfg );
               Rda_config_flag = tmp_rda_cfg;

            }
            else
               ret_val = ORPGRDA_ERROR;

         }
         else
            ret_val = ORPGRDA_ERROR;

      }

   }

   if( ret_val == ORPGRDA_ERROR )
      LE_send_msg( GL_ERROR, "ORPGRDA_set_rda_config Failed\n" );

   return ret_val;

} /* end ORPGRDA_set_rda_config */


/***************************************************************************
* Function Name:
*       Get_rda_status
*
* Description:
*
* Inputs:
*       element         - Integer that indicates the element whose value is
*                         being sought.
*
* Return:
*       value           - Integer value of the desired element.
***************************************************************************/
static int Get_rda_status ( int element )
{
        int                     value           = 0;
        RDA_status_msg_t*       rda_status      = NULL;


        rda_status = (RDA_status_msg_t *) &Rda_status->status_msg;

/*      The macros corresponding to RDA status msg halfword locations   *
 *      are defined in the header file "rda_status.h" except the last   *
 *      four; they are defined in "rda_rpg_message_header.h".           */

        switch (element)
        {

            case RS_RDA_STATUS :                /* Halfword 1 */

                value = (int) rda_status->rda_status;
                break;

            case RS_OPERABILITY_STATUS :        /* Halfword 2 */

                value = (int) rda_status->op_status;
                break;

            case RS_CONTROL_STATUS :            /* Halfword 3 */

                value = (int) rda_status->control_status;
                break;

            case RS_AUX_POWER_GEN_STATE :       /* Halfword 4 */

                value = (int) rda_status->aux_pwr_state;
                break;

            case RS_AVE_TRANS_POWER :           /* Halfword 5 */

                value = (int) rda_status->ave_trans_pwr;
                break;

            case RS_REFL_CALIB_CORRECTION :     /* Halfword 6 */

                value = (int) rda_status->ref_calib_corr;
                break;

            case RS_DATA_TRANS_ENABLED :        /* Halfword 7 */

                value = (int) rda_status->data_trans_enbld;
                break;

            case RS_VCP_NUMBER :                /* Halfword 8 */

                value = (int) rda_status->vcp_num;
                break;

            case RS_RDA_CONTROL_AUTH :          /* Halfword 9 */

                value = (int) rda_status->rda_control_auth;
                break;

            case RS_INTERFERENCE_DETECT_RATE :  /* Halfword 10 */

                value = (int) rda_status->int_detect_rate;
                break;

            case RS_OPERATIONAL_MODE :          /* Halfword 11 */

                value = (int) rda_status->op_mode;
                break;

            case RS_ISU :                       /* Halfword 12 */

                value = (int) rda_status->int_suppr_unit;
                break;

            case RS_ARCHIVE_II_STATUS :         /* Halfword 13 */

                value = (int) rda_status->arch_II_status;
                break;

            case RS_ARCHIVE_II_CAPACITY :       /* Halfword 14 */

                value = (int) rda_status->arch_II_capacity;
                break;

            case RS_RDA_ALARM_SUMMARY :         /* Halfword 15 */

                value = (int) rda_status->rda_alarm;
                break;

            case RS_COMMAND_ACK :               /* Halfword 16 */

                value = (int) rda_status->command_status;
                break;

            case RS_CHAN_CONTROL_STATUS :       /* Halfword 17 */

                value = (int) rda_status->channel_status;
                break;

            case RS_SPOT_BLANKING_STATUS :      /* Halfword 18 */

                value = (int) rda_status->spot_blanking_status;
                break;

            case RS_BPM_GEN_DATE :              /* Halfword 19 */

                value = (int) rda_status->bypass_map_date;
                break;

            case RS_BPM_GEN_TIME :              /* Halfword 20 */

                value = (int) rda_status->bypass_map_time;
                break;

            case RS_NWM_GEN_DATE :              /* Halfword 21 */

                value = (int) rda_status->notchwidth_map_date;
                break;

            case RS_NWM_GEN_TIME :              /* Halfword 22 */

                value = (int) rda_status->notchwidth_map_time;
                break;

            case RS_TPS_STATUS :                /* Halfword 24 */

                value = (int) rda_status->tps_status;
                break;

            case RS_RMS_CONTROL_STATUS :        /* Halfword 25 */

                value = (int) rda_status->rms_control_status;
                break;

            case RS_SPARE4 :                    /* Halfword 26 */

                value = (int) rda_status->spare26;
                break;

            case RS_ALARM_CODE1 :               /* Halfword 27 */

                value = (int) rda_status->alarm_code [0];
                break;

            case RS_ALARM_CODE2 :               /* Halfword 28 */

                value = (int) rda_status->alarm_code [1];
                break;

            case RS_ALARM_CODE3 :               /* Halfword 29 */

                value = (int) rda_status->alarm_code [2];
                break;

            case RS_ALARM_CODE4 :               /* Halfword 30 */

                value = (int) rda_status->alarm_code [3];
                break;

            case RS_ALARM_CODE5 :               /* Halfword 31 */

                value = (int) rda_status->alarm_code [4];
                break;

            case RS_ALARM_CODE6 :               /* Halfword 32 */

                value = (int) rda_status->alarm_code [5];
                break;

            case RS_ALARM_CODE7 :               /* Halfword 33 */

                value = (int) rda_status->alarm_code [6];
                break;

            case RS_ALARM_CODE8 :               /* Halfword 34 */

                value = (int) rda_status->alarm_code [7];
                break;

            case RS_ALARM_CODE9 :               /* Halfword 35 */

                value = (int) rda_status->alarm_code [8];
                break;

            case RS_ALARM_CODE10 :              /* Halfword 36 */

                value = (int) rda_status->alarm_code [9];
                break;

            case RS_ALARM_CODE11 :              /* Halfword 37 */

                value = (int) rda_status->alarm_code [10];
                break;

            case RS_ALARM_CODE12 :              /* Halfword 38 */

                value = (int) rda_status->alarm_code [11];
                break;

            case RS_ALARM_CODE13 :              /* Halfword 39 */

                value = (int) rda_status->alarm_code [13];
                break;

            case RS_ALARM_CODE14 :              /* Halfword 40 */

                value = (int) rda_status->alarm_code [13];
                break;

                  /* all message header fields are specified by negating
                     (ie. setting the sign bit) of the message header macros */

            case -RDA_RPG_MSG_HDR_CHANNEL :     /* RDA channel num from the msg hdr */

                value = (int) rda_status->msg_hdr.rda_channel;
                break;

            case -RDA_RPG_MSG_HDR_SEQ_NUM :     /* Message hdr sequence number */

                value = (int) rda_status->msg_hdr.sequence_num;
                break;

            case -RDA_RPG_MSG_HDR_DATE :        /* Date from message hdr */

                value = (int) rda_status->msg_hdr.julian_date-1;
                break;

            case -RDA_RPG_MSG_HDR_TIME :        /* Time from message hdr */

                value = (int) rda_status->msg_hdr.milliseconds;
                break;

            default :

                value = ORPGRDA_DATA_NOT_FOUND;
                break;

        } /* end switch */

        return value;

} /* end Get_rda_status */



/***************************************************************************
* Function Name:
*       Get_orda_status
*
* Description:
*
* Inputs:
*       element         - Integer that indicates the element whose value is
*                         being sought.
*
* Return:
*       value           - Integer value of the desired element.
***************************************************************************/
static int Get_orda_status ( int element )
{
        int                     value           = 0;
        ORDA_status_msg_t*      orda_status     = NULL;


        orda_status = (ORDA_status_msg_t *) &Orda_status->status_msg;

/*      The macros corresponding to ORDA status msg halfword locations  *
 *      are defined in the header file "rda_status.h" except the last   *
 *      four; they are defined in "rda_rpg_message_header.h".           */

        switch (element)
        {

            case RS_RDA_STATUS :                /* Halfword 1 */

                value = (int) orda_status->rda_status;
                break;

            case RS_OPERABILITY_STATUS :        /* Halfword 2 */

                value = (int) orda_status->op_status;
                break;

            case RS_CONTROL_STATUS :            /* Halfword 3 */

                value = (int) orda_status->control_status;
                break;

            case RS_AUX_POWER_GEN_STATE :       /* Halfword 4 */

                value = (int) orda_status->aux_pwr_state;
                break;

            case RS_AVE_TRANS_POWER :           /* Halfword 5 */

                value = (int) orda_status->ave_trans_pwr;
                break;

            case RS_REFL_CALIB_CORRECTION :     /* Halfword 6 */

                value = (int) orda_status->ref_calib_corr;
                break;

            case RS_DATA_TRANS_ENABLED :        /* Halfword 7 */

                value = (int) orda_status->data_trans_enbld;
                break;

            case RS_VCP_NUMBER :                /* Halfword 8 */

                value = (int) orda_status->vcp_num;
                break;

            case RS_RDA_CONTROL_AUTH :          /* Halfword 9 */

                value = (int) orda_status->rda_control_auth;
                break;

            case RS_RDA_BUILD_NUM :		/* Halfword 10 */

                value = (int) orda_status->rda_build_num;
                break;

            case RS_OPERATIONAL_MODE :          /* Halfword 11 */

                value = (int) orda_status->op_mode;
                break;

            case RS_SUPER_RES :                 /* Halfword 12 */

                value = (int) orda_status->super_res;
                break;

            case RS_CMD :                      /* Halfword 13 */

                value = (int) orda_status->cmd;
                break;

            case RS_AVSET :                    /* Halfword 14 */

                value = (int) orda_status->avset;
                break;


            case RS_RDA_ALARM_SUMMARY :         /* Halfword 15 */

                value = (int) orda_status->rda_alarm;
                break;

            case RS_COMMAND_ACK :               /* Halfword 16 */

                value = (int) orda_status->command_status;
                break;

            case RS_CHAN_CONTROL_STATUS :       /* Halfword 17 */

                value = (int) orda_status->channel_status;
                break;

            case RS_SPOT_BLANKING_STATUS :      /* Halfword 18 */

                value = (int) orda_status->spot_blanking_status;
                break;

            case RS_BPM_GEN_DATE :              /* Halfword 19 */

                value = (int) orda_status->bypass_map_date;
                break;

            case RS_BPM_GEN_TIME :              /* Halfword 20 */

                value = (int) orda_status->bypass_map_time;
                break;

            case RS_NWM_GEN_DATE :              /* Halfword 21 */

                value = (int) orda_status->clutter_map_date;
                break;

            case RS_NWM_GEN_TIME :              /* Halfword 22 */

                value = (int) orda_status->clutter_map_time;
                break;

            case RS_VC_REFL_CALIB_CORRECTION :  /* Halfword 23 */

                value = (int) orda_status->vc_ref_calib_corr;
                break;

            case RS_TPS_STATUS :                /* Halfword 24 */

                value = (int) orda_status->tps_status;
                break;

            case RS_RMS_CONTROL_STATUS :        /* Halfword 25 */

                value = (int) orda_status->rms_control_status;
                break;

            case RS_PERF_CHECK_STATUS :        /* Halfword 26 */

                value = (int) orda_status->perf_check_status;
                break;

            case RS_ALARM_CODE1 :               /* Halfword 27 */

                value = (int) orda_status->alarm_code [0];
                break;

            case RS_ALARM_CODE2 :               /* Halfword 28 */

                value = (int) orda_status->alarm_code [1];
                break;

            case RS_ALARM_CODE3 :               /* Halfword 29 */

                value = (int) orda_status->alarm_code [2];
                break;

            case RS_ALARM_CODE4 :               /* Halfword 30 */

                value = (int) orda_status->alarm_code [3];
                break;

            case RS_ALARM_CODE5 :               /* Halfword 31 */

                value = (int) orda_status->alarm_code [4];
                break;

            case RS_ALARM_CODE6 :               /* Halfword 32 */

                value = (int) orda_status->alarm_code [5];
                break;

            case RS_ALARM_CODE7 :               /* Halfword 33 */

                value = (int) orda_status->alarm_code [6];
                break;

            case RS_ALARM_CODE8 :               /* Halfword 34 */

                value = (int) orda_status->alarm_code [7];
                break;

            case RS_ALARM_CODE9 :               /* Halfword 35 */

                value = (int) orda_status->alarm_code [8];
                break;

            case RS_ALARM_CODE10 :              /* Halfword 36 */

                value = (int) orda_status->alarm_code [9];
                break;

            case RS_ALARM_CODE11 :              /* Halfword 37 */

                value = (int) orda_status->alarm_code [10];
                break;

            case RS_ALARM_CODE12 :              /* Halfword 38 */

                value = (int) orda_status->alarm_code [11];
                break;

            case RS_ALARM_CODE13 :              /* Halfword 39 */

                value = (int) orda_status->alarm_code [13];
                break;

            case RS_ALARM_CODE14 :              /* Halfword 40 */

                value = (int) orda_status->alarm_code [13];
                break;

                  /* all message header fields are specified by negating
                     (ie. setting the sign bit) of the message header macros */

            case -RDA_RPG_MSG_HDR_CHANNEL :     /* RDA channel num from the msg hdr */

                value = (int) orda_status->msg_hdr.rda_channel;
                break;

            case -RDA_RPG_MSG_HDR_SEQ_NUM :     /* Message hdr sequence number */

                value = (int) orda_status->msg_hdr.sequence_num;
                break;

            case -RDA_RPG_MSG_HDR_DATE :        /* Date from message hdr */

                value = (int) orda_status->msg_hdr.julian_date-1;
                break;

            case -RDA_RPG_MSG_HDR_TIME :        /* Time from message hdr */

                value = (int) orda_status->msg_hdr.milliseconds;
                break;

            default :

                value = ORPGRDA_DATA_NOT_FOUND;
                break;

        } /* end switch */

        return value;

} /* end Get_orda_status */


/***************************************************************************
* Function Name:
*       Set_rda_status
*
* Description:
*	Private function used for changing the values of the local RDA
*	status message store.
*
* Inputs:
*       field - Int that indicates the field whose value is being changed.
*	val -  New value for the field.
*
* Return:
*	status - ORPGRDA_SUCCESS or ORPGRDA_DATA_NOT_FOUND
*
* Notes:
*	The macros corresponding to RDA status msg halfword locations
*	are defined in the header file "orpgrda.h" except the last
*	four; they are defined in "rda_rpg_message_header.h".
***************************************************************************/
static int Set_rda_status( int field, int val )
{
   int status = ORPGRDA_SUCCESS;


   switch ( field )  
   {
      case ORPGRDA_RDA_STATUS :			/* Halfword 1 */

         Rda_status->status_msg.rda_status = (unsigned short) val;
         break;

      case ORPGRDA_OPERABILITY_STATUS :		/* Halfword 2 */

         Rda_status->status_msg.op_status = (unsigned short) val;
         break;

      case ORPGRDA_CONTROL_STATUS :		/* Halfword 3 */

         Rda_status->status_msg.control_status = (unsigned short) val;
         break;

      case ORPGRDA_AUX_POWER_GEN_STATE :	/* Halfword 4 */

         Rda_status->status_msg.aux_pwr_state = (unsigned short) val;
         break;

      case ORPGRDA_AVE_TRANS_POWER :		/* Halfword 5 */

         Rda_status->status_msg.ave_trans_pwr = (short) val;
         break;

      case ORPGRDA_REFL_CALIB_CORRECTION :	/* Halfword 6 */

         Rda_status->status_msg.ref_calib_corr = (short) val;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :		/* Halfword 7 */

         Rda_status->status_msg.data_trans_enbld = (unsigned short) val;
         break;

      case ORPGRDA_VCP_NUMBER :			/* Halfword 8 */

         Rda_status->status_msg.vcp_num = (short) val;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH :		/* Halfword 9 */

         Rda_status->status_msg.rda_control_auth = (unsigned short) val;
         break;

      case ORPGRDA_INTERFERENCE_DETECT_RATE :	/* Halfword 10 */

         Rda_status->status_msg.int_detect_rate = (unsigned short) val;
         break;

      case ORPGRDA_OPERATIONAL_MODE :		/* Halfword 11 */

         Rda_status->status_msg.op_mode = (unsigned short) val;
         break;

      case ORPGRDA_ISU :			/* Halfword 12 */

         Rda_status->status_msg.int_suppr_unit = (unsigned short) val;
         break;

      case ORPGRDA_ARCHIVE_II_STATUS :		/* Halfword 13 */

         Rda_status->status_msg.arch_II_status = (unsigned short) val;
         break;

      case ORPGRDA_ARCHIVE_II_CAPACITY :	/* Halfword 14 */

         Rda_status->status_msg.arch_II_capacity = (unsigned short) val;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :		/* Halfword 15 */

         Rda_status->status_msg.rda_alarm = (unsigned short) val;
         break;

      case ORPGRDA_COMMAND_ACK :		/* Halfword 16 */

         Rda_status->status_msg.command_status = (unsigned short) val;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :	/* Halfword 17 */

         Rda_status->status_msg.channel_status = (unsigned short) val;
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :	/* Halfword 18 */

         Rda_status->status_msg.spot_blanking_status = (unsigned short) val;
         break;

      case ORPGRDA_BPM_GEN_DATE :		/* Halfword 19 */

         Rda_status->status_msg.bypass_map_date = (unsigned short) val;
         break;

      case ORPGRDA_BPM_GEN_TIME :		/* Halfword 20 */

         Rda_status->status_msg.bypass_map_time = (unsigned short) val;
         break;

      case ORPGRDA_NWM_GEN_DATE :		/* Halfword 21 */

         Rda_status->status_msg.notchwidth_map_date = (unsigned short) val;
         break;

      case ORPGRDA_NWM_GEN_TIME :		/* Halfword 22 */

         Rda_status->status_msg.notchwidth_map_time = (unsigned short) val;
         break;

      case ORPGRDA_TPS_STATUS :			/* Halfword 24 */

         Rda_status->status_msg.tps_status = (unsigned short) val;
         break;

      case ORPGRDA_RMS_CONTROL_STATUS :		/* Halfword 25 */

         Rda_status->status_msg.rms_control_status = (unsigned short) val;
         break;

      case ORPGRDA_SPARE4 :			/* Halfword 26 */

         Rda_status->status_msg.spare26 = (unsigned short) val;
         break;

      case ORPGRDA_ALARM_CODE1 :		/* Halfword 27 */

         Rda_status->status_msg.alarm_code[0] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE2 :		/* Halfword 28 */

         Rda_status->status_msg.alarm_code[1] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE3 :		/* Halfword 29 */

         Rda_status->status_msg.alarm_code[2] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE4 :		/* Halfword 30 */

         Rda_status->status_msg.alarm_code[3] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE5 :		/* Halfword 31 */

         Rda_status->status_msg.alarm_code[4] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE6 :		/* Halfword 32 */

         Rda_status->status_msg.alarm_code[5] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE7 :		/* Halfword 33 */

         Rda_status->status_msg.alarm_code[6] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE8 :		/* Halfword 34 */

         Rda_status->status_msg.alarm_code[7] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE9 :		/* Halfword 35 */

         Rda_status->status_msg.alarm_code[8] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE10 :		/* Halfword 36 */

         Rda_status->status_msg.alarm_code[9] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE11 :		/* Halfword 37 */

         Rda_status->status_msg.alarm_code[10] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE12 :		/* Halfword 38 */

         Rda_status->status_msg.alarm_code[11] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE13 :		/* Halfword 39 */

         Rda_status->status_msg.alarm_code[12] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE14 :		/* Halfword 40 */

         Rda_status->status_msg.alarm_code[13] = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_SIZE :	/* Message hdr size */

         Rda_status->status_msg.msg_hdr.size = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_CHANNEL :	/* Message hdr RDA channel */

         Rda_status->status_msg.msg_hdr.rda_channel = (unsigned char) val;
         break;

      case -RDA_RPG_MSG_HDR_TYPE :	/* Message hdr message type */

         Rda_status->status_msg.msg_hdr.type = (unsigned char) val;
         break;

      case -RDA_RPG_MSG_HDR_SEQ_NUM :	/* Message hdr sequence number */

         Rda_status->status_msg.msg_hdr.sequence_num = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_DATE :	/* Message hdr date */

         Rda_status->status_msg.msg_hdr.julian_date = (unsigned short) val;
         break;

      case -RDA_RPG_MSG_HDR_TIME :	/* Message hdr time */

         Rda_status->status_msg.msg_hdr.milliseconds = (int) val;
         break;

      case -RDA_RPG_MSG_HDR_NUM_SEGS :	/* Message hdr number of segments */

         Rda_status->status_msg.msg_hdr.num_segs = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_SEG_NUM :	/* Message hdr segment number */

         Rda_status->status_msg.msg_hdr.seg_num = (short) val;
         break;

      default :

         status = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return status;

} /* end Set_rda_status() */


/***************************************************************************
* Function Name:
*       Set_orda_status
*
* Description:
*	Private function used for changing the values of the local ORDA
*	status message store.
*
* Inputs:
*       field - Int that indicates the field whose value is being changed.
*	val -  New value for the field.
*
* Return:
*	status - ORPGRDA_SUCCESS or ORPGRDA_DATA_NOT_FOUND
*
* Notes:
*	The macros corresponding to ORDA status msg halfword locations
*	are defined in the header file "orpgrda.h" except the last
*	four; they are defined in "rda_rpg_message_header.h".
***************************************************************************/
static int Set_orda_status( int field, int val )
{
   int status = ORPGRDA_SUCCESS;


   switch ( field )  
   {
      case ORPGRDA_RDA_STATUS :			/* Halfword 1 */

         Orda_status->status_msg.rda_status = (unsigned short) val;
         break;

      case ORPGRDA_OPERABILITY_STATUS :		/* Halfword 2 */

         Orda_status->status_msg.op_status = (unsigned short) val;
         break;

      case ORPGRDA_CONTROL_STATUS :		/* Halfword 3 */

         Orda_status->status_msg.control_status = (unsigned short) val;
         break;

      case ORPGRDA_AUX_POWER_GEN_STATE :	/* Halfword 4 */

         Orda_status->status_msg.aux_pwr_state = (unsigned short) val;
         break;

      case ORPGRDA_AVE_TRANS_POWER :		/* Halfword 5 */

         Orda_status->status_msg.ave_trans_pwr = (short) val;
         break;

      case ORPGRDA_REFL_CALIB_CORRECTION :	/* Halfword 6 */

         Orda_status->status_msg.ref_calib_corr = (short) val;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :		/* Halfword 7 */

         Orda_status->status_msg.data_trans_enbld = (unsigned short) val;
         break;

      case ORPGRDA_VCP_NUMBER :			/* Halfword 8 */

         Orda_status->status_msg.vcp_num = (short) val;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH :		/* Halfword 9 */

         Orda_status->status_msg.rda_control_auth = (unsigned short) val;
         break;

      case ORPGRDA_RDA_BUILD_NUM :		/* Halfword 10 */

         Orda_status->status_msg.rda_build_num = (unsigned short) val;
         break;

      case ORPGRDA_OPERATIONAL_MODE :		/* Halfword 11 */

         Orda_status->status_msg.op_mode = (unsigned short) val;
         break;

      case ORPGRDA_SUPER_RES :			/* Halfword 12 */

         Orda_status->status_msg.super_res = (unsigned short) val;
         break;

      case ORPGRDA_CMD :			/* Halfword 13 */

         Orda_status->status_msg.cmd = (unsigned short) val;
         break;

      case ORPGRDA_AVSET :			/* Halfword 14 */

         Orda_status->status_msg.avset = (unsigned short) val;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :		/* Halfword 15 */

         Orda_status->status_msg.rda_alarm = (unsigned short) val;
         break;

      case ORPGRDA_COMMAND_ACK :		/* Halfword 16 */

         Orda_status->status_msg.command_status = (unsigned short) val;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :	/* Halfword 17 */

         Orda_status->status_msg.channel_status = (unsigned short) val;
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :	/* Halfword 18 */

         Orda_status->status_msg.spot_blanking_status = (unsigned short) val;
         break;

      case ORPGRDA_BPM_GEN_DATE :		/* Halfword 19 */

         Orda_status->status_msg.bypass_map_date = (unsigned short) val;
         break;

      case ORPGRDA_BPM_GEN_TIME :		/* Halfword 20 */

         Orda_status->status_msg.bypass_map_time = (unsigned short) val;
         break;

      case ORPGRDA_NWM_GEN_DATE :		/* Halfword 21 */

         Orda_status->status_msg.clutter_map_date = (unsigned short) val;
         break;

      case ORPGRDA_NWM_GEN_TIME :		/* Halfword 22 */

         Orda_status->status_msg.clutter_map_time = (unsigned short) val;
         break;

      case ORPGRDA_VC_REFL_CALIB_CORRECTION :   /* Halfword 23 */

         Orda_status->status_msg.vc_ref_calib_corr = (short) val;
         break;

      case ORPGRDA_TPS_STATUS :			/* Halfword 24 */

         Orda_status->status_msg.tps_status = (unsigned short) val;
         break;

      case ORPGRDA_RMS_CONTROL_STATUS :		/* Halfword 25 */

         Orda_status->status_msg.rms_control_status = (unsigned short) val;
         break;

      case ORPGRDA_PERF_CHECK_STATUS : 		/* Halfword 26 */

         Orda_status->status_msg.perf_check_status = (short) val;
         break;

      case ORPGRDA_ALARM_CODE1 :		/* Halfword 27 */

         Orda_status->status_msg.alarm_code[0] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE2 :		/* Halfword 28 */

         Orda_status->status_msg.alarm_code[1] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE3 :		/* Halfword 29 */

         Orda_status->status_msg.alarm_code[2] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE4 :		/* Halfword 30 */

         Orda_status->status_msg.alarm_code[3] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE5 :		/* Halfword 31 */

         Orda_status->status_msg.alarm_code[4] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE6 :		/* Halfword 32 */

         Orda_status->status_msg.alarm_code[5] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE7 :		/* Halfword 33 */

         Orda_status->status_msg.alarm_code[6] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE8 :		/* Halfword 34 */

         Orda_status->status_msg.alarm_code[7] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE9 :		/* Halfword 35 */

         Orda_status->status_msg.alarm_code[8] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE10 :		/* Halfword 36 */

         Orda_status->status_msg.alarm_code[9] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE11 :		/* Halfword 37 */

         Orda_status->status_msg.alarm_code[10] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE12 :		/* Halfword 38 */

         Orda_status->status_msg.alarm_code[11] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE13 :		/* Halfword 39 */

         Orda_status->status_msg.alarm_code[12] = (short) val;
         break;

      case ORPGRDA_ALARM_CODE14 :		/* Halfword 40 */

         Orda_status->status_msg.alarm_code[13] = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_SIZE :	/* Message hdr size */

         Orda_status->status_msg.msg_hdr.size = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_CHANNEL :	/* Message hdr RDA channel */

         Orda_status->status_msg.msg_hdr.rda_channel = (unsigned char) val;
         break;

      case -RDA_RPG_MSG_HDR_TYPE :	/* Message hdr message type */

         Orda_status->status_msg.msg_hdr.type = (unsigned char) val;
         break;

      case -RDA_RPG_MSG_HDR_SEQ_NUM :	/* Message hdr sequence number */

         Orda_status->status_msg.msg_hdr.sequence_num = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_DATE :	/* Message hdr date */

         Orda_status->status_msg.msg_hdr.julian_date = (unsigned short) val;
         break;

      case -RDA_RPG_MSG_HDR_TIME :	/* Message hdr time */

         Orda_status->status_msg.msg_hdr.milliseconds = (int) val;
         break;

      case -RDA_RPG_MSG_HDR_NUM_SEGS :	/* Message hdr number of segments */

         Orda_status->status_msg.msg_hdr.num_segs = (short) val;
         break;

      case -RDA_RPG_MSG_HDR_SEG_NUM :	/* Message hdr segment number */

         Orda_status->status_msg.msg_hdr.seg_num = (short) val;
         break;

      default :

         status = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return status;

} /* end Set_orda_status() */


/***************************************************************************
* Function Name:
*       Get_previous_rda_status
*
* Description:
*	Private function used for retrieving the values of the local
*       previous RDA status message store.
*
* Inputs:
*       field - Int that indicates the field whose value is being retrieved.
*
* Return:
*       int - Value of the desired field, or ORPGRDA_DATA_NOT_FOUND.
*
* Notes:
*	The macros corresponding to ORDA status msg halfword locations
*	are defined in the header file "orpgrda.h" except the last
*	four; they are defined in "rda_rpg_message_header.h".
***************************************************************************/
static int Get_previous_rda_status( int field )
{
   int value = 0;

   switch ( field ) 
   {
      case ORPGRDA_RDA_STATUS :		/* Halfword 1 */

         value = (int) Previous_rda_status.status_msg.rda_status;
         break;

      case ORPGRDA_OPERABILITY_STATUS :	/* Halfword 2 */

         value = (int) Previous_rda_status.status_msg.op_status;
         break;

      case ORPGRDA_CONTROL_STATUS :		/* Halfword 3 */

         value = (int) Previous_rda_status.status_msg.control_status;
         break;

      case ORPGRDA_AUX_POWER_GEN_STATE :	/* Halfword 4 */

         value = (int) Previous_rda_status.status_msg.aux_pwr_state;
         break;

      case ORPGRDA_AVE_TRANS_POWER :		/* Halfword 5 */

         value = (int) Previous_rda_status.status_msg.ave_trans_pwr;
         break;

      case ORPGRDA_REFL_CALIB_CORRECTION :	/* Halfword 6 */

         value = (int) Previous_rda_status.status_msg.ref_calib_corr;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :	/* Halfword 7 */

         value = (int) Previous_rda_status.status_msg.data_trans_enbld;
         break;

      case ORPGRDA_VCP_NUMBER :		/* Halfword 8 */

         value = (int) Previous_rda_status.status_msg.vcp_num;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH :		/* Halfword 9 */

         value = (int) Previous_rda_status.status_msg.rda_control_auth;
         break;

      case ORPGRDA_INTERFERENCE_DETECT_RATE :	/* Halfword 10 */

         value = (int) Previous_rda_status.status_msg.int_detect_rate;
         break;

      case ORPGRDA_OPERATIONAL_MODE :		/* Halfword 11 */

         value = (int) Previous_rda_status.status_msg.op_mode;
         break;

      case ORPGRDA_ISU :			/* Halfword 12 */

         value = (int) Previous_rda_status.status_msg.int_suppr_unit;
         break;

      case ORPGRDA_ARCHIVE_II_STATUS :		/* Halfword 13 */

         value = (int) Previous_rda_status.status_msg.arch_II_status;
         break;

      case ORPGRDA_ARCHIVE_II_CAPACITY :	/* Halfword 14 */

         value = (int) Previous_rda_status.status_msg.arch_II_capacity;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :		/* Halfword 15 */

         value = (int) Previous_rda_status.status_msg.rda_alarm;
         break;

      case ORPGRDA_COMMAND_ACK :		/* Halfword 16 */

         value = (int) Previous_rda_status.status_msg.command_status;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :	/* Halfword 17 */

         value = (int) Previous_rda_status.status_msg.channel_status;
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :	/* Halfword 18 */

         value = (int) Previous_rda_status.status_msg.spot_blanking_status;
         break;

      case ORPGRDA_BPM_GEN_DATE :		/* Halfword 19 */

         value = (int) Previous_rda_status.status_msg.bypass_map_date;
         break;

      case ORPGRDA_BPM_GEN_TIME :		/* Halfword 20 */

         value = (int) Previous_rda_status.status_msg.bypass_map_time;
         break;

      case ORPGRDA_NWM_GEN_DATE :		/* Halfword 21 */

         value = (int) Previous_rda_status.status_msg.notchwidth_map_date;
         break;

      case ORPGRDA_NWM_GEN_TIME :		/* Halfword 22 */

         value = (int) Previous_rda_status.status_msg.notchwidth_map_time;
         break;

      case ORPGRDA_TPS_STATUS :			/* Halfword 24 */

         value = (int) Previous_rda_status.status_msg.tps_status;
         break;

      case ORPGRDA_RMS_CONTROL_STATUS :		 /* Halfword 25 */

         value = (int) Previous_rda_status.status_msg.rms_control_status;
         break;

      case ORPGRDA_SPARE4 :			/* Halfword 26 */

         value = (int) Previous_rda_status.status_msg.spare26;
         break;

      case ORPGRDA_ALARM_CODE1 :		/* Halfword 27 */

         value = (int) Previous_rda_status.status_msg.alarm_code [0];
         break;

      case ORPGRDA_ALARM_CODE2 :		/* Halfword 28 */

         value = (int) Previous_rda_status.status_msg.alarm_code [1];
         break;

      case ORPGRDA_ALARM_CODE3 :		/* Halfword 29 */

         value = (int) Previous_rda_status.status_msg.alarm_code [2];
         break;

      case ORPGRDA_ALARM_CODE4 :		/* Halfword 30 */

         value = (int) Previous_rda_status.status_msg.alarm_code [3];
         break;

      case ORPGRDA_ALARM_CODE5 :		/* Halfword 31 */

         value = (int) Previous_rda_status.status_msg.alarm_code [4];
         break;

      case ORPGRDA_ALARM_CODE6 :		/* Halfword 32 */

         value = (int) Previous_rda_status.status_msg.alarm_code [5];
         break;

      case ORPGRDA_ALARM_CODE7 :		/* Halfword 33 */

         value = (int) Previous_rda_status.status_msg.alarm_code [6];
         break;

      case ORPGRDA_ALARM_CODE8 :		/* Halfword 34 */

         value = (int) Previous_rda_status.status_msg.alarm_code [7];
         break;

      case ORPGRDA_ALARM_CODE9 :		/* Halfword 35 */

         value = (int) Previous_rda_status.status_msg.alarm_code [8];
         break;

      case ORPGRDA_ALARM_CODE10 :		/* Halfword 36 */

         value = (int) Previous_rda_status.status_msg.alarm_code [9];
         break;

      case ORPGRDA_ALARM_CODE11 :		/* Halfword 37 */

         value = (int) Previous_rda_status.status_msg.alarm_code [10];
         break;

      case ORPGRDA_ALARM_CODE12 :		/* Halfword 38 */

         value = (int) Previous_rda_status.status_msg.alarm_code [11];
         break;

      case ORPGRDA_ALARM_CODE13 :		/* Halfword 39 */

         value = (int) Previous_rda_status.status_msg.alarm_code [13];
         break;

      case ORPGRDA_ALARM_CODE14 :		/* Halfword 40 */

         value = (int) Previous_rda_status.status_msg.alarm_code [13];
         break;

      /* all message header fields are specified by negating
         (ie. setting the sign bit) of the message header macros */

      case -RDA_RPG_MSG_HDR_CHANNEL :	/* RDA channel num from the msg hdr */

         value = (int) Previous_rda_status.status_msg.msg_hdr.rda_channel;
         break;

      case -RDA_RPG_MSG_HDR_SEQ_NUM :	/* Message hdr sequence number */

         value = (int) Previous_rda_status.status_msg.msg_hdr.sequence_num;
         break;

      case -RDA_RPG_MSG_HDR_DATE :	/* Date from message hdr */

         value = (int) Previous_rda_status.status_msg.msg_hdr.julian_date-1;
         break;

      case -RDA_RPG_MSG_HDR_TIME :	/* Time from message hdr */

         value = (int) Previous_rda_status.status_msg.msg_hdr.milliseconds;
         break;

      default :

         value = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return value;

} /* end Get_previous_rda_status() */


/***************************************************************************
* Function Name:
*       Get_previous_orda_status
*
* Description:
*	Private function used for retrieving the values of the local
*       previous ORDA status message store.
*
* Inputs:
*       field - Int that indicates the field whose value is being retrieved.
*
* Return:
*       int - Value of the desired field, or ORPGRDA_DATA_NOT_FOUND.
*
* Notes:
*	The macros corresponding to ORDA status msg halfword locations
*	are defined in the header file "orpgrda.h" except the last
*	four; they are defined in "rda_rpg_message_header.h".
***************************************************************************/
static int Get_previous_orda_status( int field )
{
   int value = 0;

   switch ( field ) 
   {
      case ORPGRDA_RDA_STATUS :		/* Halfword 1 */

         value = (int) Previous_orda_status.status_msg.rda_status;
         break;

      case ORPGRDA_OPERABILITY_STATUS :	/* Halfword 2 */

         value = (int) Previous_orda_status.status_msg.op_status;
         break;

      case ORPGRDA_CONTROL_STATUS :		/* Halfword 3 */

         value = (int) Previous_orda_status.status_msg.control_status;
         break;

      case ORPGRDA_AUX_POWER_GEN_STATE :	/* Halfword 4 */

         value = (int) Previous_orda_status.status_msg.aux_pwr_state;
         break;

      case ORPGRDA_AVE_TRANS_POWER :		/* Halfword 5 */

         value = (int) Previous_orda_status.status_msg.ave_trans_pwr;
         break;

      case ORPGRDA_REFL_CALIB_CORRECTION :	/* Halfword 6 */

         value = (int) Previous_orda_status.status_msg.ref_calib_corr;
         break;

      case ORPGRDA_DATA_TRANS_ENABLED :	/* Halfword 7 */

         value = (int) Previous_orda_status.status_msg.data_trans_enbld;
         break;

      case ORPGRDA_VCP_NUMBER :		/* Halfword 8 */

         value = (int) Previous_orda_status.status_msg.vcp_num;
         break;

      case ORPGRDA_RDA_CONTROL_AUTH :		/* Halfword 9 */

         value = (int) Previous_orda_status.status_msg.rda_control_auth;
         break;

      case ORPGRDA_RDA_BUILD_NUM :		/* Halfword 10 */

         value = (int) Previous_orda_status.status_msg.rda_build_num;
         break;

      case ORPGRDA_OPERATIONAL_MODE :		/* Halfword 11 */

         value = (int) Previous_orda_status.status_msg.op_mode;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :		/* Halfword 15 */

         value = (int) Previous_orda_status.status_msg.rda_alarm;
         break;

      case ORPGRDA_COMMAND_ACK :		/* Halfword 16 */

         value = (int) Previous_orda_status.status_msg.command_status;
         break;

      case ORPGRDA_CHAN_CONTROL_STATUS :	/* Halfword 17 */

         value = (int) Previous_orda_status.status_msg.channel_status;
         break;

      case ORPGRDA_SPOT_BLANKING_STATUS :	/* Halfword 18 */

         value = (int) Previous_orda_status.status_msg.spot_blanking_status;
         break;

      case ORPGRDA_BPM_GEN_DATE :		/* Halfword 19 */

         value = (int) Previous_orda_status.status_msg.bypass_map_date;
         break;

      case ORPGRDA_BPM_GEN_TIME :		/* Halfword 20 */

         value = (int) Previous_orda_status.status_msg.bypass_map_time;
         break;

      case ORPGRDA_NWM_GEN_DATE :		/* Halfword 21 */

         value = (int) Previous_orda_status.status_msg.clutter_map_date;
         break;

      case ORPGRDA_NWM_GEN_TIME :		/* Halfword 22 */

         value = (int) Previous_orda_status.status_msg.clutter_map_time;
         break;

      case ORPGRDA_VC_REFL_CALIB_CORRECTION : 	/* Halfword 23 */

         value = (int) Previous_orda_status.status_msg.vc_ref_calib_corr;
         break;

      case ORPGRDA_TPS_STATUS :			/* Halfword 24 */

         value = (int) Previous_orda_status.status_msg.tps_status;
         break;

      case ORPGRDA_RMS_CONTROL_STATUS :		/* Halfword 25 */

         value = (int) Previous_orda_status.status_msg.rms_control_status;
         break;

      case ORPGRDA_PERF_CHECK_STATUS :		/* Halfword 26 */

         value = (int) Previous_orda_status.status_msg.perf_check_status;
         break;

      case ORPGRDA_ALARM_CODE1 :		/* Halfword 27 */

         value = (int) Previous_orda_status.status_msg.alarm_code [0];
         break;

      case ORPGRDA_ALARM_CODE2 :		/* Halfword 28 */

         value = (int) Previous_orda_status.status_msg.alarm_code [1];
         break;

      case ORPGRDA_ALARM_CODE3 :		/* Halfword 29 */

         value = (int) Previous_orda_status.status_msg.alarm_code [2];
         break;

      case ORPGRDA_ALARM_CODE4 :		/* Halfword 30 */

         value = (int) Previous_orda_status.status_msg.alarm_code [3];
         break;

      case ORPGRDA_ALARM_CODE5 :		/* Halfword 31 */

         value = (int) Previous_orda_status.status_msg.alarm_code [4];
         break;

      case ORPGRDA_ALARM_CODE6 :		/* Halfword 32 */

         value = (int) Previous_orda_status.status_msg.alarm_code [5];
         break;

      case ORPGRDA_ALARM_CODE7 :		/* Halfword 33 */

         value = (int) Previous_orda_status.status_msg.alarm_code [6];
         break;

      case ORPGRDA_ALARM_CODE8 :		/* Halfword 34 */

         value = (int) Previous_orda_status.status_msg.alarm_code [7];
         break;

      case ORPGRDA_ALARM_CODE9 :		/* Halfword 35 */

         value = (int) Previous_orda_status.status_msg.alarm_code [8];
         break;

      case ORPGRDA_ALARM_CODE10 :		/* Halfword 36 */

         value = (int) Previous_orda_status.status_msg.alarm_code [9];
         break;

      case ORPGRDA_ALARM_CODE11 :		/* Halfword 37 */

         value = (int) Previous_orda_status.status_msg.alarm_code [10];
         break;

      case ORPGRDA_ALARM_CODE12 :		/* Halfword 38 */

         value = (int) Previous_orda_status.status_msg.alarm_code [11];
         break;

      case ORPGRDA_ALARM_CODE13 :		/* Halfword 39 */

         value = (int) Previous_orda_status.status_msg.alarm_code [13];
         break;

      case ORPGRDA_ALARM_CODE14 :		/* Halfword 40 */

         value = (int) Previous_orda_status.status_msg.alarm_code [13];
         break;

      /* all message header fields are specified by negating
         (ie. setting the sign bit) of the message header macros */

      case -RDA_RPG_MSG_HDR_CHANNEL :	/* RDA channel num from the msg hdr */

         value = (int) Previous_orda_status.status_msg.msg_hdr.rda_channel;
         break;

      case -RDA_RPG_MSG_HDR_SEQ_NUM :	/* Message hdr sequence number */

         value = (int) Previous_orda_status.status_msg.msg_hdr.sequence_num;
         break;

      case -RDA_RPG_MSG_HDR_DATE :	/* Date from message hdr */

         value = (int) Previous_orda_status.status_msg.msg_hdr.julian_date-1;
         break;

      case -RDA_RPG_MSG_HDR_TIME :	/* Time from message hdr */

         value = (int) Previous_orda_status.status_msg.msg_hdr.milliseconds;
         break;

      default :

         value = ORPGRDA_DATA_NOT_FOUND;
         break;
   }

   return value;

} /* end Get_previous_orda_status() */


/*******************************************************************************
* Function Name:
*	Rda_update_system_status
*
* Description:
*       Writes Legacy RDA status data to system status log file in plain text 
*       format.  Only status which is different than last reported is reported.
*
* Inputs:
*	none
*
* Return:
*	void
*******************************************************************************/
static void Rda_update_system_status()
{
   int		stat			= 0;
   int		hw			= 0;
   int		len			= 0;
   int		rda_stat 		= 0;
   int		op_stat 		= 0;
   int		control_stat 		= 0;
   int		aux_pwr_stat 		= 0;
   int		data_trans_enab 	= 0;
   int		vcp 			= 0;
   int		rda_contr_auth 		= 0;
   int		opmode 			= 0;
   int		int_supp_unit 		= 0;
   int		arch_II_stat 		= 0;
   int		chan_stat 		= 0;
   int		spot_blank_stat 	= 0;
   int		tps_stat 		= 0;
   int		p_rda_stat 		= 0;
   int		p_op_stat 		= 0;
   int		p_control_stat 		= 0;
   int		p_aux_pwr_stat 		= 0;
   int		p_data_trans_enab 	= 0;
   int		p_vcp 			= 0;
   int		p_rda_contr_auth 	= 0;
   int		p_opmode 		= 0;
   int		p_int_supp_unit 	= 0;
   int		p_arch_II_stat 		= 0;
   int		p_chan_stat 		= 0;
   int		p_spot_blank_stat 	= 0;
   int		p_tps_stat 		= 0;
   char*	buf			= NULL;
   double	deau_ret_val		= 0.0;


   /* Get at status buffer. */
   buf = calloc( (MAX_STATUS_LENGTH+1), sizeof(char) );
   if( buf == NULL ){

      LE_send_msg( GL_INFO, "Unable to Process RDA Status Log Message\n" );
      return;

   }

   /* Place header string in buffer. */
   strcat( buf, "RDA STATUS:" );
   len = strlen( buf );

   /* Retrieve and store current rda status fields */
   rda_stat = ORPGRDA_get_status( ORPGRDA_RDA_STATUS );
   if ( rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_RDA_STATUS\n");
   }

   op_stat = ORPGRDA_get_status( ORPGRDA_OPERABILITY_STATUS );
   if ( op_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_OPERABILITY_STATUS\n");
   }

   control_stat = ORPGRDA_get_status( ORPGRDA_CONTROL_STATUS );
   if ( control_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_CONTROL_STATUS\n");
   }

   aux_pwr_stat = ORPGRDA_get_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_AUX_POWER_GEN_STATE\n");
   }

   data_trans_enab = ORPGRDA_get_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_DATA_TRANS_ENABLED\n");
   }

   vcp = ORPGRDA_get_status( ORPGRDA_VCP_NUMBER );
   if ( vcp == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_VCP_NUMBER\n");
   }

   rda_contr_auth = ORPGRDA_get_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_RDA_CONTROL_AUTH\n");
   }

   opmode = ORPGRDA_get_status( ORPGRDA_OPERATIONAL_MODE );
   if ( opmode == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_OPERATIONAL_MODE\n");
   }

   /* Legacy config only */
   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      int_supp_unit = ORPGRDA_get_status( ORPGRDA_ISU );
      if ( int_supp_unit == ORPGRDA_DATA_NOT_FOUND )
      {
         LE_send_msg(GL_ERROR,
            "Rda_update_system_status: Could not retrieve ORPGRDA_ISU\n");
      }
   }

   arch_II_stat = ORPGRDA_get_status( ORPGRDA_ARCHIVE_II_STATUS );
   if ( arch_II_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_ARCHIVE_II_STATUS\n");
   }

   chan_stat = ORPGRDA_get_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( chan_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_CHAN_CONTROL_STATUS\n");
   }

   spot_blank_stat = ORPGRDA_get_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_SPOT_BLANKING_STATUS\n");
   }

   tps_stat = ORPGRDA_get_status( ORPGRDA_TPS_STATUS );
   if ( tps_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Could not retrieve ORPGRDA_TPS_STATUS\n");
   }


   /* Retrieve and store previous rda status fields */
   p_rda_stat = ORPGRDA_get_previous_status( ORPGRDA_RDA_STATUS );
   if ( p_rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_RDA_STATUS\n");
   }

   p_op_stat = ORPGRDA_get_previous_status( ORPGRDA_OPERABILITY_STATUS );
   if ( p_op_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_OPERABILITY_STATUS\n");
   }

   p_control_stat = ORPGRDA_get_previous_status( ORPGRDA_CONTROL_STATUS );
   if ( p_control_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_CONTROL_STATUS\n");
   }

   p_aux_pwr_stat = ORPGRDA_get_previous_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( p_aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_AUX_POWER_GEN_STATE\n");
   }

   p_data_trans_enab = ORPGRDA_get_previous_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( p_data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_DATA_TRANS_ENABLED\n");
   }

   p_vcp = ORPGRDA_get_previous_status( ORPGRDA_VCP_NUMBER );
   if ( p_vcp == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_VCP_NUMBER\n");
   }

   p_rda_contr_auth = ORPGRDA_get_previous_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( p_rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_RDA_CONTROL_AUTH\n");
   }

   p_opmode = ORPGRDA_get_previous_status( ORPGRDA_OPERATIONAL_MODE );
   if ( p_opmode == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_OPERATIONAL_MODE\n");
   }

   /* Legacy config only */
   if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
   {
      p_int_supp_unit = ORPGRDA_get_previous_status( ORPGRDA_ISU );
      if ( p_int_supp_unit == ORPGRDA_DATA_NOT_FOUND )
      {
         LE_send_msg(GL_ERROR,
            "Rda_update_system_status: Problem retrieving ORPGRDA_ISU\n");
      }
   }

   p_arch_II_stat = ORPGRDA_get_previous_status( ORPGRDA_ARCHIVE_II_STATUS );
   if ( p_arch_II_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_ARCHIVE_II_STATUS\n");
   }

   p_chan_stat = ORPGRDA_get_previous_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( p_chan_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_CHAN_CONTROL_STATUS\n");
   }

   p_spot_blank_stat =
      ORPGRDA_get_previous_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( p_spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_SPOT_BLANKING_STATUS\n");
   }

   p_tps_stat =
      ORPGRDA_get_previous_status( ORPGRDA_TPS_STATUS );
   if ( p_tps_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Rda_update_system_status: Problem retrieving ORPGRDA_TPS_STATUS\n");
   }


   /* Output status data is readable format. */
   for( hw = 0; hw <= STATUS_WORDS; hw++ )
   {
      switch( hw )
      {
         case RS_RDA_STATUS:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA STATUS:  %x\n", rda_stat);

            /* If status has changed, process new value; */
            if( rda_stat == p_rda_stat )
               break;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( status[i], "%6d", rda_stat );

            }

            Process_status_field( &buf, &len, "Stat=", status[i] );

            break;
         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability, auto_cal_disabled;
            int i;

            LE_send_msg( GL_INFO, "--->OPERABILITY STATUS:  %x\n", op_stat );

            /* If operability status has changed, process new value; */
            if( op_stat == p_op_stat )
               break;

            /* Process operability status. */
            auto_cal_disabled = 
                     (unsigned short) (op_stat & 0x01);
            rda_operability = 
                     (unsigned short) (op_stat & 0xfffe);

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( operability[i], "%6d", op_stat );
            }

            if( auto_cal_disabled )
            {
               char *temp = calloc( strlen( operability[i] ) + strlen("/ACD"), 
                                    sizeof(char) );
               if ( temp == NULL )
               {
                  if( buf != NULL )
                     free(buf);

                  return;
               }

               strcat( temp, operability[i] );
               strcat( temp, "/ACD" );

               Process_status_field( &buf, &len, "Oper=", temp );
               free( temp );

            }
            else
            {
               Process_status_field( &buf, &len, "Oper=", operability[i] );
            }

            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->CONTROL STATUS:  %x\n", control_stat );

            /* If control status has changed, process new value; */
            if( control_stat == p_control_stat )
               break;

            /* Process Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( control[i], "%6d", control_stat );

            }

            Process_status_field( &buf, &len, "Cntl=", control[i] );

            break;

         }
         case RS_AUX_POWER_GEN_STATE: 
         {

            int i;
            short test_bit, curr_bit, prev_bit;
            char temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->AUX PWR STATE:  %x\n", aux_pwr_stat );

            /* If auxilliary power source/generator status has changed, 
               process new value; */
            if( aux_pwr_stat == p_aux_pwr_stat )
               break;

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH ); 

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);
               prev_bit = (p_aux_pwr_stat & test_bit);
               if( curr_bit == prev_bit )
                  continue;

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );
                  
            }

            Process_status_field( &buf, &len, "", temp );

            break;
         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            LE_send_msg( GL_INFO, "--->DATA TRANS ENABLED:  %x\n", data_trans_enab );

            /* If moments have changed, process new value; */
            if( data_trans_enab == p_data_trans_enab )
               break;

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );
    
               }

               if( (data_trans_enab & BD_WIDTH) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );
    
               }

            }

            Process_status_field( &buf, &len, "Data=", moment_string );

            break;
         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            LE_send_msg( GL_INFO, "--->VCP NUMBER:  %x\n", temp_vcp );

            /* If VCP number has changed, process new value. */
            if( temp_vcp == p_vcp )
               break;

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );

            Process_status_field( &buf, &len, "VCP=", temp );

            break;
         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA CONTROL AUTH:  %x\n", rda_contr_auth );

            /* If RDA control authority has changed, process new value; */
            if( rda_contr_auth == p_rda_contr_auth )
               break;

            if( rda_contr_auth == CA_NO_ACTION )
               break;

            if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 0;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( authority[i], "%6d", rda_contr_auth );

            }

            Process_status_field( &buf, &len, "Auth=", authority[i] );

            break;
         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            LE_send_msg( GL_INFO, "--->OPERATIONAL MODE:  %x\n", opmode );

            /* If operational mode has changed, process new value; */
            if( opmode == p_opmode )
               break;

            /* Process operational mode. */
            if( opmode == OP_MAINTENANCE_MODE )
               i = 1;

            else if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( mode[i], "%6d", opmode );

            }

            Process_status_field( &buf, &len, "Mode=", mode[i] );

            break;
         }

         case RS_ISU:
         {
            if ( Rda_config_flag == ORPGRDA_LEGACY_CONFIG )
            {
               int i;

               LE_send_msg( GL_INFO, "--->ISU:  %x\n", int_supp_unit );

               /* If interference unit status has changed, process new value; */
               if( int_supp_unit == p_int_supp_unit )
                  break;

               /* Process interference unit. */ 
               if( int_supp_unit == ISU_NOCHANGE )
                  break;

               else if( int_supp_unit == ISU_ENABLED )
                  i = 0;

               else if( int_supp_unit == ISU_DISABLED )
                  i = 1;

               else{

                  /* Unknown value. Place value in status buffer. */
                  i = 2;
                  sprintf( interference_unit[i], "%6d", int_supp_unit );

               }

               Process_status_field( &buf, &len, "ISU=", interference_unit[i] );

               break;
            }
         }

         case RS_ARCHIVE_II_STATUS:
         {
            int playback_avail = 0;
            short tape_num, tape_not_0;
            char tape_num_string[3], temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->ARCHIVE II STATUS:  %x\n", arch_II_stat );

            /* If archive II status has changed, process new value. */
            if( arch_II_stat == p_arch_II_stat )
               break;

            /* Set the tape number. */
            memset( tape_num_string, 0, 3 );
            tape_num = (arch_II_stat & TAPE_NUM_MASK) >> TAPE_NUM_SHIFT;
            if( tape_num == 0 ){

               strcat( tape_num_string, "  " );
               tape_not_0 = 0;
              
            }
            else if( tape_num < 10 ){

               sprintf( tape_num_string, "%2.2d", tape_num );
               tape_not_0 = 1;

            }
            else{

               sprintf( tape_num_string, "%2d", tape_num );
               tape_not_0 = 1;

            }
            memset( temp, 0, MAX_STATUS_LENGTH );

            /* If PLAYBACK, then.... */
            if( (arch_II_stat & AR_PLAYBACK) ){

               playback_avail = 0;
               strcat( temp, archive_status[7] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );
 
               }
               
            }
            else if( (arch_II_stat & AR_PLAYBACK_AVAIL) )
                playback_avail = 1;
   
            /* Process other status. */
            if( (arch_II_stat & AR_TAPE_TRANSFER) ){

               strcat( temp, archive_status[10] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_CHECK_LABEL) ){

               strcat( temp, archive_status[8] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_FAST_FRWRD) )
            {
               strcat( temp, archive_status[9] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_SEARCH) )
            {
               strcat( temp, archive_status[6] ); 
               if( tape_not_0 ){

                  strcat( temp, tape_num_string );
                  strcat( temp, archive_status[11] );

               }

            }
            else if( (arch_II_stat & AR_RECORD) )
            {
               strcat( temp, archive_status[3] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_RESERVED) )
            {
               strcat( temp, archive_status[2] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_INSTALLED) )
            {
               strcat( temp, archive_status[0] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_LOADED) )
            {
               strcat( temp, archive_status[1] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            /* Is Archive II installed? */
            else if( (arch_II_stat == AR_NOT_INSTALLED) )
               strcat( temp, archive_status[4] ); 

            /* We do what legacy does ... If no bits are defined,
               it is assumed Archive II is not Installed. */
            else if( !(arch_II_stat & AR_PLAYBACK) )
               strcat( temp, archive_status[4] );                  

            if( playback_avail ){

               if( strlen( temp ) )
                  strcat( temp, ", ");
               strcat( temp, archive_status[5] );

            }

            Process_status_field( &buf, &len, "ArchII=", temp );

            break;

         }

         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            LE_send_msg( GL_INFO, "--->CHANNEL CONTROL STATUS:  %x\n", chan_stat );

            /* If channel control status has changed, process new value; */
            if( chan_stat == p_chan_stat )
               break;

            /* Process channel control status if FAA Redundant. */ 
            if ( (stat =
              DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1))
              >= 0)
            {
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  sprintf( channel_status[i], "%6d", chan_stat );
               }
            }
            else
            {
              LE_send_msg( GL_INFO | GL_ERROR,
               "Rda_update_system_status: call to DEAU_get_values returned error.\n");
            }

            Process_status_field( &buf, &len, "Chan Ctl=", channel_status[i] );

            break;
         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->SPOT BLANKING STATUS:  %x\n", spot_blank_stat );

            /* If spot blanking status has changed, process new value; */
            if( spot_blank_stat == p_spot_blank_stat )
               break;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */ 
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               sprintf( spot_blanking[i], "%6d", spot_blank_stat );

            }

            Process_status_field( &buf, &len, "SB=", spot_blanking[i] );
            break;

         }
         case RS_TPS_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->TPS STATUS:  %x\n", tps_stat );

            /* If TPS status has changed, process new value; */
            if( tps_stat == p_tps_stat )
               break;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */ 
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( tps[i], "%6d", tps_stat );

            }

            Process_status_field( &buf, &len, "TPS=", tps[i] );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Write out last status message. */
   if( (buf != NULL) && (strcmp( buf, "RDA STATUS:" ) != 0) ){

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", buf );
   }
   if (buf != NULL) {
       free(buf);
   }

} /* End of Rda_update_system_status() */


/*******************************************************************************
* Function Name:
*	Orda_update_system_status
*
* Description:
*       Writes Open RDA status data to system status log file in plain text 
*       format.  Only status which is different than last reported is reported.
*
* Inputs:
*	none
*
* Return:
*	void
*******************************************************************************/
static void Orda_update_system_status()
{
   int		stat			= 0;
   int		hw			= 0;
   int		len			= 0;
   int		rda_stat 		= 0;
   int		op_stat 		= 0;
   int		control_stat 		= 0;
   int		aux_pwr_stat 		= 0;
   int		data_trans_enab 	= 0;
   int		vcp 			= 0;
   int		rda_contr_auth 		= 0;
   int		opmode 			= 0;
   int		chan_stat 		= 0;
   int		spot_blank_stat 	= 0;
   int		tps_stat 		= 0;
   int		perf_check_status	= 0;
   int		p_rda_stat 		= 0;
   int		p_op_stat 		= 0;
   int		p_control_stat 		= 0;
   int		p_aux_pwr_stat 		= 0;
   int		p_data_trans_enab 	= 0;
   int		p_vcp 			= 0;
   int		p_rda_contr_auth 	= 0;
   int		p_opmode 		= 0;
   int		p_chan_stat 		= 0;
   int		p_spot_blank_stat 	= 0;
   int		p_tps_stat 		= 0;
   int		p_perf_check_status	= 0;
   char*	buf			= NULL;
   double	deau_ret_val		= 0.0;


   /* Get at status buffer. */
   buf = calloc( (MAX_STATUS_LENGTH+1), sizeof(char) );
   if( buf == NULL ){

      LE_send_msg( GL_INFO, "Unable to Process RDA Status Log Message\n" );
      return;

   }

   /* Place header string in buffer. */
   strcat( buf, "RDA STATUS:" );
   len = strlen( buf );

   /* Retrieve and store current rda status fields */
   rda_stat = ORPGRDA_get_status( ORPGRDA_RDA_STATUS );
   if ( rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_RDA_STATUS\n");
   }

   op_stat = ORPGRDA_get_status( ORPGRDA_OPERABILITY_STATUS );
   if ( op_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_OPERABILITY_STATUS\n");
   }

   control_stat = ORPGRDA_get_status( ORPGRDA_CONTROL_STATUS );
   if ( control_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_CONTROL_STATUS\n");
   }

   aux_pwr_stat = ORPGRDA_get_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_AUX_POWER_GEN_STATE\n");
   }

   data_trans_enab = ORPGRDA_get_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_DATA_TRANS_ENABLED\n");
   }

   vcp = ORPGRDA_get_status( ORPGRDA_VCP_NUMBER );
   if ( vcp == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_VCP_NUMBER\n");
   }

   rda_contr_auth = ORPGRDA_get_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_RDA_CONTROL_AUTH\n");
   }

   opmode = ORPGRDA_get_status( ORPGRDA_OPERATIONAL_MODE );
   if ( opmode == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_OPERATIONAL_MODE\n");
   }

   chan_stat = ORPGRDA_get_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( chan_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_CHAN_CONTROL_STATUS\n");
   }

   spot_blank_stat = ORPGRDA_get_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_SPOT_BLANKING_STATUS\n");
   }

   tps_stat = ORPGRDA_get_status( ORPGRDA_TPS_STATUS );
   if ( tps_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_TPS_STATUS\n");
   }

   perf_check_status = ORPGRDA_get_status( ORPGRDA_PERF_CHECK_STATUS );
   if ( perf_check_status == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Could not retrieve ORPGRDA_PERF_CHECK_STATUS\n");
   }

   /* Retrieve and store previous rda status fields */
   p_rda_stat = ORPGRDA_get_previous_status( ORPGRDA_RDA_STATUS );
   if ( p_rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_RDA_STATUS\n");
   }

   p_op_stat = ORPGRDA_get_previous_status( ORPGRDA_OPERABILITY_STATUS );
   if ( p_op_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_OPERABILITY_STATUS\n");
   }

   p_control_stat = ORPGRDA_get_previous_status( ORPGRDA_CONTROL_STATUS );
   if ( p_control_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_CONTROL_STATUS\n");
   }

   p_aux_pwr_stat = ORPGRDA_get_previous_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( p_aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_AUX_POWER_GEN_STATE\n");
   }

   p_data_trans_enab = ORPGRDA_get_previous_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( p_data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_DATA_TRANS_ENABLED\n");
   }

   p_vcp = ORPGRDA_get_previous_status( ORPGRDA_VCP_NUMBER );
   if ( p_vcp == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_VCP_NUMBER\n");
   }

   p_rda_contr_auth = ORPGRDA_get_previous_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( p_rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_RDA_CONTROL_AUTH\n");
   }

   p_opmode = ORPGRDA_get_previous_status( ORPGRDA_OPERATIONAL_MODE );
   if ( p_opmode == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_OPERATIONAL_MODE\n");
   }

   p_chan_stat = ORPGRDA_get_previous_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( p_chan_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_CHAN_CONTROL_STATUS\n");
   }

   p_spot_blank_stat =
      ORPGRDA_get_previous_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( p_spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_SPOT_BLANKING_STATUS\n");
   }

   p_tps_stat =
      ORPGRDA_get_previous_status( ORPGRDA_TPS_STATUS );
   if ( p_tps_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_TPS_STATUS\n");
   }

   p_perf_check_status =
      ORPGRDA_get_previous_status( ORPGRDA_PERF_CHECK_STATUS );
   if ( p_perf_check_status == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg(GL_ERROR,
         "Orda_update_system_status: Problem retrieving ORPGRDA_PERF_CHECK_STATUS\n");
   }

   /*
      Output status data is readable format. NOTE: range for hw should start at
      the value of the macro representing the first field in the status msg
      (defined in rda_status.h)
   */
   for( hw = 1; hw <= STATUS_WORDS; hw++ )
   {
      switch( hw )
      {
         case RS_RDA_STATUS:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA STATUS:  %x\n", rda_stat);

            /* If status has changed, process new value; */
            if( rda_stat == p_rda_stat )
               break;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( status[i], "%6d", rda_stat );

            }

            Process_status_field( &buf, &len, "Stat=", status[i] );

            break;
         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability;
            int i;

            LE_send_msg( GL_INFO, "--->OPERABILITY STATUS:  %x\n", op_stat );

            /* If operability status has changed, process new value; */
            if( op_stat == p_op_stat )
               break;

            /* Process operability status. */
            rda_operability = (unsigned short) op_stat;

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( operability[i], "%6d", op_stat );
            }

            Process_status_field( &buf, &len, "Oper=", operability[i] );

            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->CONTROL STATUS:  %x\n", control_stat );

            /* If control status has changed, process new value; */
            if( control_stat == p_control_stat )
               break;

            /* Process Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( control[i], "%6d", control_stat );

            }

            Process_status_field( &buf, &len, "Cntl=", control[i] );

            break;

         }
         case RS_AUX_POWER_GEN_STATE: 
         {

            int i;
            short test_bit, curr_bit, prev_bit;
            char temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->AUX PWR STATE:  %x\n", aux_pwr_stat );

            /* If auxilliary power source/generator status has changed, 
               process new value; */
            if( aux_pwr_stat == p_aux_pwr_stat )
               break;

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH ); 

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);
               prev_bit = (p_aux_pwr_stat & test_bit);
               if( curr_bit == prev_bit )
                  continue;

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );
                  
            }

            Process_status_field( &buf, &len, "", temp );

            break;
         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            LE_send_msg( GL_INFO, "--->DATA TRANS ENABLED:  %x\n", data_trans_enab );

            /* If moments have changed, process new value; */
            if( data_trans_enab == p_data_trans_enab )
               break;

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );
    
               }

               if( (data_trans_enab & BD_WIDTH) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );
    
               }

            }

            Process_status_field( &buf, &len, "Data=", moment_string );

            break;
         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            LE_send_msg( GL_INFO, "--->VCP NUMBER:  %x\n", temp_vcp );

            /* If VCP number has changed, process new value. */
            if( temp_vcp == p_vcp )
               break;

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );

            Process_status_field( &buf, &len, "VCP=", temp );

            break;
         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA CONTROL AUTH:  %x\n", rda_contr_auth );

            /* If RDA control authority has changed, process new value; */
            if( rda_contr_auth == p_rda_contr_auth )
               break;

            if( rda_contr_auth == CA_NO_ACTION )
               break;

            if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 0;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( authority[i], "%6d", rda_contr_auth );

            }

            Process_status_field( &buf, &len, "Auth=", authority[i] );

            break;
         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            LE_send_msg( GL_INFO, "--->OPERATIONAL MODE:  %x\n", opmode );

            /* If operational mode has changed, process new value
               OP_MAINTENANCE_MODE */
            if( opmode == p_opmode )
               break;

            /* Process operational mode. */
            if( opmode == OP_MAINTENANCE_MODE )
               i = 1;

            else if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else if( opmode == OP_OFFLINE_MAINTENANCE_MODE )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( orda_mode[i], "%6d", opmode );

            }

            Process_status_field( &buf, &len, "Mode=", orda_mode[i] );

            break;
         }

         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            LE_send_msg( GL_INFO, "--->CHANNEL CONTROL STATUS:  %x\n", chan_stat );

            /* If channel control status has changed, process new value; */
            if( chan_stat == p_chan_stat )
               break;

            if ( (stat =
              DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1))
              >= 0)
            {
               /* Process channel control status if FAA Redundant. */ 
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  sprintf( channel_status[i], "%6d", chan_stat );
               }
            }
            else
            {
              LE_send_msg( GL_INFO | GL_ERROR,
               "Orda_update_system_status: call to DEAU_get_values returned error.\n");
            }

            Process_status_field( &buf, &len, "Chan Ctl=", channel_status[i] );

            break;
         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->SPOT BLANKING STATUS:  %x\n", spot_blank_stat );

            /* If spot blanking status has changed, process new value; */
            if( spot_blank_stat == p_spot_blank_stat )
               break;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */ 
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               sprintf( spot_blanking[i], "%6d", spot_blank_stat );

            }

            Process_status_field( &buf, &len, "SB=", spot_blanking[i] );
            break;

         }
         case RS_TPS_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->TPS STATUS:  %x\n", tps_stat );

            /* If TPS status has changed, process new value; */
            if( tps_stat == p_tps_stat )
               break;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */ 
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( tps[i], "%6d", tps_stat );

            }

            Process_status_field( &buf, &len, "TPS=", tps[i] );
            break;

         }
         case RS_PERF_CHECK_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->PERFORMANCE CHECK STATUS:  %x\n", perf_check_status );

            /* If Performance Check status has changed, process new value; */
            if( perf_check_status == p_perf_check_status )
               break;

            /* Process TPS status. */ 
            if( perf_check_status == PC_AUTO )
               i = 0;

            else if( perf_check_status == PC_PENDING )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( perf_check[i], "%6d", perf_check_status );

            }

            Process_status_field( &buf, &len, "Perf Check=", perf_check[i] );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Write out last status message. */
   if( (buf != NULL) && (strcmp( buf, "RDA STATUS:" ) != 0) ){

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", buf );
   }
   if (buf != NULL) {
       free(buf);
   }

} /* End of Orda_update_system_status() */


/*******************************************************************************
* Function Name:
*	Init_rda_config
*
* Description:
*	Reads and initializes the global RDA configuration flag.
*
* Inputs:
*	none
*
* Return:
*	void
*******************************************************************************/
static void Init_rda_config()
{
   int		ret		= 0;
   double	rda_cfg_val	= 0.0;
   char*        ds_name         = NULL;

   /* if Rda_config_flag is -1, read the value from adaptation data
      and store it */
   if ( (Rda_config_flag == -1) || (Init_rda_config_flag == 0) )
   {

      ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
      if( ds_name != NULL ){

         DEAU_LB_name( ds_name );
         Init_rda_config_flag = 1;
         ret = DEAU_get_values( RDA_CONFIGURATION, &rda_cfg_val, 1 );
         if ( ret > 0 )
         {
            if( (int) rda_cfg_val == 1 )
               Rda_config_flag = ORPGRDA_ORDA_CONFIG;
            else
               Rda_config_flag = ORPGRDA_LEGACY_CONFIG;
         }

      }
      else{

         Init_rda_config_flag = 0;
         LE_send_msg( GL_ERROR, "Init_rda_config Failed (DEAU_get_values)\n" );
         return;

      }

   }

   if( !ORPGRDA_rdacfg_registered )
   {

      /* Have we registered for DEAU updates? */
      ret = DEAU_UN_register( RDA_CONFIGURATION_GROUP, Deau_notify_func );
      if( ret >= 0 )
         ORPGRDA_rdacfg_registered = 1;

      else
      {

         LE_send_msg( GL_ERROR, "Init_rda_config Failed (DEAU_UN_register)\n" );
         return;

      }

   }

} /* end Init_rda_config */


/*******************************************************************************
* Function Name:
*	Get_rda_config_from_msg_hdr
*
* Description:
*	Extracts the RDA configuration from the ICD message header.
*
* Inputs:
*	msg_ptr - pointer to ICD message (includes header).
*
* Outputs:
*       rda_config - receives the rda configuration.
*
* Return:
*	Rda configuration on success, ORPGRDA_ERROR on error.
*******************************************************************************/
static int Get_rda_config_from_msg_hdr( void* msg_ptr, int *rda_config )
{   /* use the status msg passed in */

   int                          chan_bit        = 0;
   RDA_RPG_message_header_t*    msg_hdr_ptr     = NULL;

   /* If NULL pointer passed in, return error. */
   if( msg_ptr == NULL )
      return ORPGRDA_ERROR;

   msg_hdr_ptr = (RDA_RPG_message_header_t *) msg_ptr;

   /* Check the message type.  If the message type is 0, 
      ignore this message. */
   if( msg_hdr_ptr->type == 0 )
      return ORPGRDA_ERROR;

   /* Mask out the 4th bit and store it */
   chan_bit = (int) (msg_hdr_ptr->rda_channel & 0x08);

   /* If the configuration is ORDA, the 4th bit will be set to 1 */
   if ( chan_bit > 0 )
   {
      *rda_config = ORPGRDA_ORDA_CONFIG;
   }
   else
   {
      *rda_config = ORPGRDA_LEGACY_CONFIG;
   }

   return ORPGRDA_SUCCESS;

}

/*******************************************************************************
* Description:
*	Register for RDA Configuration changes
*
* Inputs:
*	see LB man page
*
* Outputs:
*       Init_rda_config_flag is cleared
*
* Return:
*
*******************************************************************************/
static void Deau_notify_func( int fd, LB_id_t msgid, int msg_info, void *arg )
{

   LE_send_msg( GL_INFO, "RDA Configuration Changed!!!\n" );
   Init_rda_config_flag = 0;

}

/********************************************************************************
   
   Description:
      Writes out the VCP definition to log file.

   Inputs:
      vcp - pointer to VCP data ... format specified in vcp.h.

********************************************************************************/
static void Write_vcp_data( Vcp_struct *vcp ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };

   int i, expected_size;
   short wform, phse;

   /* Write out VCP data. */
   LE_send_msg( GL_INFO, "\n\nVCP %d Data:\n", vcp->vcp_num );
   LE_send_msg( GL_INFO, "--->VCP Header:\n" ); 
   LE_send_msg( GL_INFO, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
            vcp->msg_size, vcp->type, vcp->n_ele );
   LE_send_msg( GL_INFO, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
            vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ], 
            width[ vcp->pulse_width/4 ] );

   /* Do some validation. */
   expected_size = VCP_ATTR_SIZE + vcp->n_ele*(sizeof(Ele_attr)/sizeof(short));
   if( vcp->msg_size != expected_size )
      LE_send_msg( GL_ERROR, "VCP Size: %d Not Expected: %d\n", 
                   vcp->msg_size, expected_size ); 

   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle ); 
      float azi_1 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_1 );
      float azi_2 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_2 );
      float azi_3 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_3 );

      wform = elev->wave_type;
      phse = elev->phase;
      LE_send_msg( GL_INFO, "--->Elevation %d:\n", i+1 );
      LE_send_msg( GL_INFO, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res Flag: %3d, Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ], 
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      LE_send_msg( GL_INFO, "       Az Rate: %5.2f (0x%4x BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate*ORPGVCP_AZIMUTH_RATE_FACTOR, elev->azi_rate, (float) elev->surv_thr_parm/8.0, 
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );

      LE_send_msg( GL_INFO, "       PRF Sector 1:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_1, elev->dop_prf_num_1, elev->pulse_cnt_1 );

      LE_send_msg( GL_INFO, "       PRF Sector 2:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_2, elev->dop_prf_num_2, elev->pulse_cnt_2 );

      LE_send_msg( GL_INFO, "       PRF Sector 3:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_3, elev->dop_prf_num_3, elev->pulse_cnt_3 );

   }   

}
