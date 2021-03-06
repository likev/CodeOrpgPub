/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 18:29:49 $
 * $Id: mon_wideband.c,v 1.17 2013/07/22 18:29:49 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

/*******************************************************************************
* File Name:	
*	mon_wb.c
*
* Description:
*	Contains source code for the ORPG wideband monitoring tool called 
*	"mon_wb".  NOTE: this tool was closely modeled from the narrowband 
*	monitoring tool called "mon_nb".  Reuse of mon_nb software was
*	exercised where possible.
*
* Functions:
*	main
*	HandleRpgToRdaRequest
*	HandleRdaToRpgResponse
*	ReadRequest
*	ReadResponse
*	ProcessCmWriteRequest
*	ProcessCmDataResponse
*	ProcessMsgHdr
*	ProcessRdaRpgLoopBackMsg
*	ProcessLgcyRdaStatusMsg
*	ProcessOrdaRdaStatusMsg
*	ProcessLgcyBasedataMsg
*	ProcessOrdaBasedataMsg
*	ProcessLgcyPerfMaintMsg
*	ProcessOrdaPerfMaintMsg
*	ProcessLgcyRdaControlCmdsMsg
*	ProcessOrdaRdaControlCmdsMsg
*	ProcessConsoleMsg
*	ProcessVcpMsg
*	ProcessLgcyCltrFltrBypsMsg
*	ProcessOrdaCltrFltrBypsMsg
*	ProcessLgcyNotchWidthMsg
*	ProcessOrdaClutterMapMsg
*	ProcessAdaptDataMsg
*	ProcessLgcyReqForDataMsg
*	ProcessOrdaReqForDataMsg
*       ProcessLgcyClczMsg
*       ProcessOrdaClczMsg
*	PrintMsg
*	bam_to_deg 
*	check_vcp_num_range
*	Print_mon_wb_usage 
*	ProcessCommandLineArgs
*	ProcessRdaAlarmCodes
*	SetArchivedPlaybackFlag
*	check_alarm_code_range
*       Print_long_text_msg
*	check_data_moment_name
*
* Usage:
*	Type mon_wb at the command line.  The RPG software must be running.  
*	For help using mon_wb, type "mon_wb -h" at the command line or visit
*	the mon_wb man page.  For verbose mode, type "mon_wb -vN" where N = a
*	numeric level not less than zero.  The default verbose level is MONWB_NONE.
*	A description of the available verbose levels can be found on the
*	mon_wb man page.
*
* Examples:
*	mon_wb		- Runs mon_wb with default verbose level MONWB_NONE.
*	mon_wb -v1	- Runs mon_wb with verbose level set to 1.
*	mon_wb -h	- Provides help using mon_wb.
*
* History:	
*	03-14-2003, R. Solomon.  Created.
*	06-02-2003, R. Solomon.  Added get_RDA_config. Moved prototypes and 
*				 macro defs to header file.
*	06-10-2003, R. Solomon.  Moved get_RDA_config to ORPGRDA_get_RDA_config
*			  	 in the orpgrda lib.
*******************************************************************************/

#include "mon_wideband.h"

/*** Global variables ***/
static int	ProcessAllMsgsFlag	= MONWB_FALSE;  
static int	VerboseLevel		= MONWB_LOW;  /* default level is MONWB_LOW */
static int	MessageType 		= 0;	 /* default 0 = no msg type */
static int	HdrDisplayFlag		= MONWB_IS_NOT_DISPLAYED; /* 0=NOT,1=IS */
static int	NewMessageFlag		= MONWB_FALSE;
static int	RequestLbId		= 0;
static int	ResponseLbId		= 0;
LB_id_t		RequestLbMsgId		= 0;
LB_id_t		ResponseLbMsgId		= 0;
static int	Request_LB_updated	= MONWB_FALSE;
static int	Response_LB_updated	= MONWB_FALSE;
static int	MsgReadLen		= 0;
static char	Message[MONWB_MAX_NUM_CHARS];
static char	Divider[MONWB_CHARS_PER_LINE] =
"===========================================================================\0";
char*		BlankString		= "               ";
static unsigned short	RadialStatus		= MONWB_SHORT_FILL;/* init to invalid value */
char*           ReqFilename;
char*           RespFilename;
static int	ArchivedPlaybackFlagReq	= MONWB_FALSE;
static int	ArchivedPlaybackFlagResp	= MONWB_FALSE;
static short	RdaChannelCfg		= -1;
static short	IsOrdaConfig		= MONWB_FALSE;
static int	MsgHdrExists		= MONWB_FALSE;
static int	CommHdrExists		= MONWB_TRUE;
static int	RewindFlag		= MONWB_FALSE; /* LB rewind flag */
static int	RewindInputBuffer	= MONWB_FALSE; /* Resp LB rewind flag */
static int	RewindOutputBuffer	= MONWB_FALSE; /* Req LB rewind flag */
static int	User_defined_resp_file	= MONWB_FALSE;
static int	User_defined_req_file	= MONWB_FALSE;
static int	New_date	        = 0; /* julian date of new msg */
static int	Old_date	        = 0; /* julian date of prev msg */
static int	Msg_time	        = 0; /* sec past midnight of msg */
static int	PrintMomentDataFlag	= MONWB_FALSE;

/*******************************************************************************
* Function Name:	
*	main
*
* Description:
*	Acts as the controller for the mon_wb tool. 
*	
* Inputs:
*	None
*	
* Returns:
*	0 upon successful completion.
*	-1 upon encountering a terminating error.	
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
int main( int argc, char** argv)
{
   int		err			= 0; 
   char*	data_dir		= NULL;


   /* Allocate space for LB file paths and names */
   ReqFilename = (char *)malloc(FILENAME_MAX);
   RespFilename = (char *)malloc(FILENAME_MAX);

   /* Process cmd line args */
   err = ProcessCommandLineArgs(argc, argv);
   if ( err < 0 )
   {
      Print_mon_wb_usage(argv);
      return(0);
   }

   /* determine and store the data directory */
   data_dir = getenv("ORPGDIR");

   /*** if no user supplied request LB, get default LB path and name ***/
   if (User_defined_req_file == MONWB_FALSE)
   {
      /* going to use default request file (req.0) */

      /* append data dir from environment var */
      strcpy(ReqFilename, data_dir);

      /* append file path and name */
      strcat(ReqFilename, "/comms/req.0");
      strncat(ReqFilename, "\0", (size_t)1);
   }

   /* attempt to open user's file and assign the id */
   RequestLbId = LB_open(ReqFilename, LB_READ, NULL);

   /* if it fails for whatever reason, set id to -1 */
   if ( RequestLbId < 0 )
   {
      fprintf(stderr, "Error opening LB %s\n", ReqFilename);
      RequestLbId = -1;
   }

   /*** Retrieve the response LB data ID ***/
   if (User_defined_resp_file == MONWB_FALSE)
   {
      /* going to use default response file (resp.0) */

      /* append data dir from environment var */
      strcpy(RespFilename, data_dir);

      /* append file path and name */
      strcat(RespFilename, "/ingest/resp.0");
      strncat(RespFilename, "\0", (size_t)1);
   }

   /* attempt to open file and assign the id */
   ResponseLbId = LB_open(RespFilename, LB_READ, NULL);

   /* if it fails for whatever reason, set id to -1 */
   if ( ResponseLbId < 0 )
   {
      fprintf(stderr, "Error opening LB %s\n", RespFilename);
      ResponseLbId = -1;
   }

   /*** Print LB file names ***/
   fprintf(stderr, "RPG ---> RDA Request LB : %s\n", ReqFilename);
   fprintf(stderr, "RPG <--- RDA Response LB: %s\n", RespFilename);

   /*** Reset the global new message indicator ***/
   NewMessageFlag = MONWB_TRUE;

   /*** Set up callbacks so the proper functions are called when new msgs are
      sent to the LBs ***/
   if ( RequestLbId >= 0 )
   {
      err = LB_UN_register(RequestLbId, LB_ANY, HandleRpgToRdaRequest);
      if ( err < 0 )
      {
         fprintf(stderr, "LB_UN_register for RDA request LB returned an error (%d)\n",
            err);
         return (-1);
      }
   }

   if ( ResponseLbId >= 0 )
   {
      err = LB_UN_register(ResponseLbId, LB_ANY, HandleRdaToRpgResponse);
      if ( err < 0 )
      {
         fprintf(stderr, "LB_UN_register for RDA response LB returned an error (%d)\n",
            err);
         return (-1);
      }
   }

   /*** Set up a loop to detect and process new messages written to the LBs ***/
   while (1)
   {
      if (( Request_LB_updated == MONWB_TRUE ) ||
         (  RewindOutputBuffer == MONWB_TRUE ))
      {
         /* Reset the value to false to show it has been processed */
         Request_LB_updated = MONWB_FALSE;
 
         if ( ( err = ReadRequest() ) < 0 )
         {
             break;  /* break out of while loop if the LB read failed */
         }

         /* Reset the "Rewind" flags */
         RewindOutputBuffer = MONWB_FALSE;
      }

      if (( Response_LB_updated == MONWB_TRUE ) ||
          ( RewindInputBuffer == MONWB_TRUE ))
      {
         /* Reset the value to false to show it has been processed */
         Response_LB_updated = MONWB_FALSE;
 
         if ( ( err = ReadResponse() ) < 0 )
         {
             break;  /* break out of while loop if the LB read failed */
         }

         /* Reset the "Rewind" flags */
         RewindInputBuffer = MONWB_FALSE;
      }

      /* Get some rest! */
      sleep(1);
   }

   return (0);  /* successful completion */
} /* end main */


/*******************************************************************************
* Function Name:	
*	HandleRpgToRdaRequest
*
* Description:
*	Sets the global static variable 'Request_LB_updated' to TRUE when the
*	RPG-to-RDA request linear buffer is modified.
*
* Inputs:
*	None
*	
* Returns:
*	void - no return value
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
static void HandleRpgToRdaRequest( int fd, LB_id_t msg_id, int msg_info,
   void *arg)
{
   Request_LB_updated = MONWB_TRUE;
} /* end HandleRpgToRdaRequest */


/*******************************************************************************
* Function Name:	
*	HandleRdaToRpgResponse
*
* Description:
*	Sets the global static variable 'Response_LB_updated' to TRUE when the
*	RDA-to-RPG response linear buffer is modified.
*	
* Inputs:
*	None
*	
* Returns:
*	void - no return value
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
static void HandleRdaToRpgResponse( int fd, LB_id_t msg_id, int msg_info,
   void *arg)
{
   Response_LB_updated = MONWB_TRUE;
} /* end HandleRdaToRpgResponse */


/*******************************************************************************
* Function Name:	
*	ReadRequest
*
* Description:
*	Reads messages sent from the RPG-to-RDA request LB. NOTE: The static
*	qualifier prevents this function from being called by any function not
*	declared in this file. NOTE: this function was closely modeled after
*	the Read_request function in mon_nb.c.
*
*	The following message types are handled by this routine:
*		6	RDA Control Commands
*		7	Volume Coverage Pattern
*		8	Clutter Censor Zones
*		9	Request for Data
*		10	Console Message
*		12	Loop Back Test
*		14	Edited Clutter Filter Bypass Map
*
* Arguments:
*	None
*	
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
static int ReadRequest()
{
   int				err			= 0;
   int				msgLen			= 0;
   int				returnVal 		= 0;
   char*			requestLbBufPtr 	= NULL;
   CM_req_struct*		requestStructPtr	= NULL;
   RDA_RPG_message_header_t*	msgHdrStructPtr		= NULL;
   short*			msgDataPtr		= NULL;
   short*			msgHdrPtr		= NULL;
   int				yr, mo, day, hr, min, sec;


   /* A new message was read. Set the global flag so the PrintMsg routine
      knows to print a divider.  */
   NewMessageFlag = MONWB_TRUE;

   /*** If Rewind flag is set, first rewind the LB ***/
   if ( RewindOutputBuffer == MONWB_TRUE )
   {
      err = LB_seek( RequestLbId, 0, LB_FIRST, NULL );
   }

   /*** read all new messages ***/
   while(1)
   {
      /* Reset global header display flag */
      HdrDisplayFlag = MONWB_IS_NOT_DISPLAYED;

      /* Read the latest messages from the RPG->RDA request LB */
      MsgReadLen = LB_read(RequestLbId, &requestLbBufPtr,
                           LB_ALLOC_BUF, LB_NEXT);

      /* Retrieve the message ID of the newly read message */
      RequestLbMsgId = LB_previous_msgid( RequestLbId );

      if ( MsgReadLen < 0 )
      {
         if ( MsgReadLen == LB_TO_COME )
         {
            break;
         }
         
         fprintf(stderr, "LB_read error (%d) \n", MsgReadLen); 
         if ( MsgReadLen == LB_EXPIRED )
         {
            continue;
         }

         returnVal = -1;
      }
      else
      {

         /* A new message was read. Set the global flag so the PrintMsg routine
            knows to print a divider.  */
         NewMessageFlag = MONWB_TRUE;

         /* Reset the message type flag */
         MessageType = 0;

         /* Set the message header indicator flag */
         if (MsgReadLen > MONWB_COMMS_HDR_SIZE_BYTES)
         {
            MsgHdrExists = MONWB_TRUE;
         }
         else
         {
            MsgHdrExists = MONWB_FALSE;
         }

         /* Need to determine whether or not this is archived data which
            will have no CM hdr or CTM hdr.  If it is archived data, we need to
            key off the message header type. */
         err = SetArchivedPlaybackFlag(requestLbBufPtr, &ArchivedPlaybackFlagReq);
         if ( err != MONWB_SUCCESS )
         {
            fprintf(stderr,
               "ReadRequest: error in SetArchivedPlaybackFlag()\n");
            return (MONWB_FAIL);
         }

         /* Set message pointers accordingly depending on whether or not
            there's a comms/ctm header. */
         if (ArchivedPlaybackFlagReq == MONWB_FALSE)
         {
            msgHdrPtr = (short *) (requestLbBufPtr +
               sizeof(CM_req_struct) + CTM_HDRSZE_BYTES);
            msgHdrStructPtr = (RDA_RPG_message_header_t *)( requestLbBufPtr +
               sizeof(CM_req_struct) + CTM_HDRSZE_BYTES);
         }
         else
         {
            msgHdrPtr = (short *) requestLbBufPtr;
            msgHdrStructPtr = (RDA_RPG_message_header_t *)( requestLbBufPtr );
         }

         /* If the msg header exists, store the message type for convenience */
         if ( MsgHdrExists == MONWB_TRUE )
            MessageType = msgHdrStructPtr->type;

         #ifdef LITTLE_ENDIAN_MACHINE
            /* if there's a data msg (not just a comms/ctm hdr), swap it */
            if( MsgHdrExists == MONWB_TRUE )
            {
               /* Convert the message from external format to internal format
                  (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or
                  vice versa). */
               UMC_from_ICD_RDA( (void *) msgHdrPtr );
            }
         #endif

         if ( ArchivedPlaybackFlagReq == MONWB_FALSE )
         {
            /* THIS DATA HAS A CM HDR AND A CTM HDR */
            CommHdrExists = MONWB_TRUE;

            /* cast comms struct pointer to comm mgr data */
            requestStructPtr = (CM_req_struct *) requestLbBufPtr;

            /* If RDA config flag hasn't been set, set it now */
            if ( (RdaChannelCfg < 0) && (requestStructPtr->type == CM_DATA) )
            {
               RdaChannelCfg = ORPGRDA_get_rda_config( msgHdrPtr );
            }

            /* for convenience set a global for the configuration type */
            if ( RdaChannelCfg == ORPGRDA_ORDA_CONFIG )
            {
               IsOrdaConfig = MONWB_TRUE;
            }
            else if ( RdaChannelCfg == ORPGRDA_LEGACY_CONFIG )
            {
               IsOrdaConfig = MONWB_FALSE;
            }
            else if( requestStructPtr->type == CM_DATA )
            {
               fprintf(stderr,
                  "ERROR in ReadResponse. Problem with RdaChannelCfg.\n");
            }

            /* store and print the date/time from the comms hdr */
            err = unix_time( (time_t *) &requestStructPtr->time, &yr,
               &mo, &day, &hr, &min, &sec);

            /* recode date to julian date */
            RPGCS_date_to_julian( yr, mo, day, &New_date);
            if ( New_date != Old_date )
            {
               /* print divider */
               fprintf(stderr, "\n%s\n", Divider);

               /* print date */
               fprintf(stderr, "%02d/%02d/%02d\n", mo, day, yr);

               /* update saved date */
               Old_date = New_date;
            }

            /* save the message time after converting to seconds */
            Msg_time = (int)((hr * 3600) + (min * 60) + sec);

            if ( VerboseLevel >= MONWB_EXTREME )
            {
               msgLen = sprintf(Message,
                  "\n%sRequest LB Comms Header:\n", BlankString);
               PrintMsg(msgLen);
               msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Request Type",
                  requestStructPtr->type);
               PrintMsg(msgLen);
               msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Request Number",
                  requestStructPtr->req_num);
               PrintMsg(msgLen);
               msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Link Index",
                  requestStructPtr->link_ind);
               PrintMsg(msgLen);

               /*** print request time ***/
               err = unix_time( (time_t *) &requestStructPtr->time, &yr,
                                 &mo, &day, &hr, &min, &sec);
               msgLen = sprintf(Message, MONWB_UNIX_TIME_WITHOUT_MS_FMT_STRING,
                  BlankString, "Request Time", hr, min, sec,
                  requestStructPtr->time);

               PrintMsg(msgLen);
               msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Parameter",
                  requestStructPtr->parm);
               PrintMsg(msgLen);
               msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Data Size",
                  requestStructPtr->data_size);
               PrintMsg(msgLen);
               fprintf(stderr, "\n");
            }

            /* Take necessary action depending on message type */
            switch ( requestStructPtr->type )
            {
               case CM_CONNECT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen =
                        sprintf(Message,
                        "---> REQUESTING CONNECTION TO RDA (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DIAL_OUT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "---> MAKE DIAL-OUT CONNECTION (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DISCONNECT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "---> REQUESTING DISCONNECT FROM RDA (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_WRITE:
                  err = ProcessMsgHdr( msgHdrStructPtr );
                  err = ProcessCmWriteRequest((short)msgHdrStructPtr->type,
                     requestLbBufPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr, "ERROR: in ProcessCmWriteRequest.\n");
                     return (-1);
                  }

                  break;
   
               case CM_STATUS:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen =
                        sprintf(Message,
                        "---> REQUESTING RDA CM STATUS (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;

               case CM_CANCEL:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "---> CANCELLING REQUEST (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DATA:
   
                  /* this shouldn't happen */
                  fprintf(stderr, "Type CM_DATA request message?");
                  
                  break;
   
               case CM_EVENT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "---> EVENT NOTIFICATION SENT (Msg Req Num:%u)\n",
                        requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;

               case CM_SET_PARAMS:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "---> SET PARAMETERS COMMAND SENT (Msg ID:%d Req Num:%u)\n",
                        RequestLbMsgId, requestStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;

               default:
                  fprintf(stderr,
                     "ERROR: Message type unknown (Msg ID:%d Req Num:%u).\n",
                     RequestLbMsgId,requestStructPtr->req_num);
                  break;
            }
         }
         else
         {
            /* NOTE: MSG HAS NO COMMS HEADER OR CTM HEADER */

            CommHdrExists = MONWB_FALSE;

            msgHdrPtr = (short *) requestLbBufPtr;
            msgHdrStructPtr = (RDA_RPG_message_header_t *)( requestLbBufPtr );

            /*** Print the msg date.  This will be done only at the beginning
                 and if it changes. ***/
            New_date = msgHdrStructPtr->julian_date;
            if ( New_date != Old_date )
            {
               /* convert date to human readable format and print */
               err = RPGCS_julian_to_date( msgHdrStructPtr->julian_date, &yr,
                  &mo, &day );

               /* print divider */
               fprintf(stderr, "\n%s\n", Divider);

               fprintf(stderr, "%02d/%02d/%02d\n", mo, day, yr);
               Old_date = New_date;
            }

            /* save the message time (seconds) */
            Msg_time = (int)(msgHdrStructPtr->milliseconds / 1000);

            /* set a pointer to the actual message data */
            msgDataPtr = (short *) (requestLbBufPtr +
               sizeof(RDA_RPG_message_header_t));

            /* Need to determine the message type and take the 
               appropriate action */

            switch ( msgHdrStructPtr->type )
            {
               case RDA_CONTROL_COMMANDS:
                  msgLen = 
                     sprintf(Message, 
                     "---> RDA CONTROL COMMANDS MSG (Msg ID:%d Type: 6)\n",
                     RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyRdaControlCmdsMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyRdaControlCmdsMsg().\n");
                        return(-1);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaControlCmdsMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaControlCmdsMsg().\n");
                        return(-1);
                     }
                  }
                  break;

               case RPG_RDA_VCP:
                  msgLen = 
                     sprintf(Message, 
                     "---> SENDING VCP DATA (Msg ID:%d Type:7)\n", RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  err = ProcessVcpMsg(msgDataPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr,
                        "ERROR: Problem in ProcessVcpMsg().\n");
                     return(-1);
                  }
                  break;

               case CLUTTER_SENSOR_ZONES:
                  msgLen = 
                     sprintf(Message, 
                     "---> CLUTTER SENSOR ZONES (Msg ID:%d Type:8)\n", RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyClczMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyClczMsg().\n");
                        return(-1);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaClczMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaClczMsg().\n");
                        return(-1);
                     }
                  }
                  break;

               case REQUEST_FOR_DATA:
                  msgLen = 
                     sprintf(Message, 
                     "---> REQUEST FOR DATA (Msg ID:%d Type:9)\n",
                     RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyReqForDataMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyReqForDataMsg().\n");
                        return(-1);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaReqForDataMsg(msgDataPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaReqForDataMsg().\n");
                        return(-1);
                     }
                  }
                  break;

               case CONSOLE_MESSAGE_G2A:
                  msgLen = 
                     sprintf(Message, 
                     "---> CONSOLE MSG (Msg ID:%d Type:10)", RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  err = ProcessConsoleMsg(msgHdrPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr, "ERROR: Problem in ProcessConsoleMsg().\n");
                     return(-1);
                  }
                  break;

               case LOOPBACK_TEST_RPG_RDA:
                  msgLen = 
                     sprintf(Message, 
                        "---> LOOPBACK TEST SENT (Msg ID:%d Type:12)\n",
                        RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  break;

               case LOOPBACK_TEST_RDA_RPG:
                  msgLen = 
                     sprintf(Message, 
                        "---> LOOPBACK RESP SENT (Msg ID:%d Type:11)\n",
                        RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  break;

               case EDITED_CLUTTER_FILTER_MAP:
                  msgLen = 
                     sprintf(Message, 
                        "---> EDITED CLUTTER FILTER MAP SENT (Msg ID:%d Type:14)\n",
                        RequestLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );

                  break;

               default:
                  fprintf(stderr,
                     "ERROR: Message type unknown (Msg ID:%d).\n",
                     RequestLbMsgId);
                  break;
            }
         }


         /* Free memory buffer if necessary */
         if ( requestLbBufPtr != NULL )
         {
            free( requestLbBufPtr );
         }
      }
   }

   return returnVal;
} /* end ReadRequest */


/*******************************************************************************
* Function Name:	
*	ReadResponse
*
* Description:
*	Reads messages from the RDA to RPG response LB.  NOTE: The static
*	qualifier prevents this function from being called by any function 
*	declared outside of this file.
*
*	The following message types are handled by this routine:
*		1	Digital Radar Data
*		2	RDA Status Data
*		3	Performance/Maintenance Data
*		4	Console Message
*		5	Volume Coverage Pattern (RDA->RPG)
*		11	Loop Back Test (RDA->RPG)
*		13	Clutter Filter Bypass Map
*		15	Clutter Filter Notch Width Map
*		18	RDA Adaptation Data
*		31	Generic Digital Radar Data
*
* Inputs:
*	None
*	
* Returns:
*	int - MONWB_SUCCESS on success or MONWB_FAIL on failure
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
static int ReadResponse()
{
   int				err			= 0;
   int		   		msgLen			= 0;
   int 		   		returnVal 		= MONWB_SUCCESS;
   RDA_RPG_message_header_t*	msgHdrStructPtr		= NULL;
   char* 	   		responseLbBufPtr 	= NULL;
   CM_resp_struct* 		responseStructPtr	= NULL;
   int				firstTimeThrough	= MONWB_TRUE;
   RDA_basedata_header*		lgcyBasedataMsgPtr	= NULL;
   ORDA_basedata_header*	ordaBasedataMsgPtr	= NULL;
   Generic_basedata_header_t*	genBasedataMsgPtr	= NULL;
   short*			msgDataPtr		= NULL;
   short*			msgHdrPtr		= NULL;
   int				yr, mo, day, hr, min, sec;


   /* A new message was read. Set the global flag so the PrintMsg routine
      knows to print a divider.  */
   NewMessageFlag = MONWB_TRUE;

   /* Reset global header display flag */
   HdrDisplayFlag = MONWB_IS_NOT_DISPLAYED;

   /*** If Rewind flag is set, first rewind the LB ***/
   if ( RewindInputBuffer == MONWB_TRUE )
   {
      err = LB_seek( ResponseLbId, 0, LB_FIRST, NULL );
   }

   /* read and process all the new messages */
   while(1)
   {
      /* Reset global header display flag */
      HdrDisplayFlag = MONWB_IS_NOT_DISPLAYED;

      /* Read the latest messages from the RPG<--RDA response LB */
      MsgReadLen = LB_read(ResponseLbId, &responseLbBufPtr,
                           LB_ALLOC_BUF, LB_NEXT);

      /* Retrieve the message ID of the newly read message */
      ResponseLbMsgId = LB_previous_msgid( ResponseLbId );

      if ( MsgReadLen < 0 )
      {
         if ( MsgReadLen == LB_TO_COME )
         {
            break; /* break out of while loop */
         }
         
         fprintf(stderr, "LB_read error (%d) \n", MsgReadLen);
         if ( MsgReadLen == LB_EXPIRED )
         {
            continue;
         }
 
         /* not LB_TO_COME or LB_EXPIRED, print message and return error */
         fprintf(stderr, "ReadResponse: LB_read returned error (%d)\n",
            MsgReadLen);
         returnVal = (MONWB_FAIL);
      }
      else
      {
         /* If not first time, we want to print the divider and time stamp */
         if (firstTimeThrough == MONWB_FALSE)
         {
            NewMessageFlag= MONWB_TRUE;
         }
    
         /* Set the message header indicator flag */
         if (MsgReadLen > MONWB_COMMS_HDR_SIZE_BYTES)
         {
            MsgHdrExists = MONWB_TRUE;
         }
         else
         {
            MsgHdrExists = MONWB_FALSE;
         }

         /* Reset the message type flag */
         MessageType = 0; /* default value */

         /* Need to determine whether or not this is archived data which
            will have no CM hdr or CTM hdr.  If it is archived data, we need to
            key off the message header type. */
         err = SetArchivedPlaybackFlag(responseLbBufPtr, &ArchivedPlaybackFlagResp);
         if ( err != MONWB_SUCCESS )
         {
            fprintf(stderr,
               "ReadResponse: error in SetArchivedPlaybackFlag()\n");
            free(responseLbBufPtr);
            return (MONWB_FAIL);
         }

         /* Set message pointers accordingly depending on whether or not
            there's a comms/ctm header. */
         if (ArchivedPlaybackFlagResp == MONWB_FALSE)
         {
            /* Assumes there is a comm manager header.  Ingore all types 
               except the following ...  */
            CM_resp_struct *resp = (CM_resp_struct *) responseLbBufPtr;
            if( (resp->type != RQ_DATA) 
                            && 
                (resp->type != CM_EVENT)
                            &&
                (resp->type != CM_CONNECT)
                            &&
                (resp->type != CM_SET_PARAMS) )
               break;

            msgHdrPtr = (short *) (responseLbBufPtr +
               sizeof(CM_resp_struct) + CTM_HDRSZE_BYTES);
            msgHdrStructPtr = (RDA_RPG_message_header_t *)( responseLbBufPtr +
               sizeof(CM_resp_struct) + CTM_HDRSZE_BYTES);
         }
         else
         {
            msgHdrPtr = (short *) responseLbBufPtr;
            msgHdrStructPtr = (RDA_RPG_message_header_t *)( responseLbBufPtr );
         }

         #ifdef LITTLE_ENDIAN_MACHINE
            /* if there's a data msg (not just a comms/ctm hdr), swap it */
            if( MsgHdrExists == MONWB_TRUE ) 
            {

               int type;

               /* Convert the message from external format to internal format
                  (i.e., if the RDA is Big-Endian and the RPG is Little-Endian (or
                  vice versa). */
               UMC_RDAtoRPG_message_header_convert( (char *) msgHdrPtr );

               if( msgHdrStructPtr->type != GENERIC_DIGITAL_RADAR_DATA ){

                  if( msgHdrStructPtr->size > MAX_RDA_MESSAGE_SIZE ){

                      fprintf(stderr,
                           "ERROR: Inprobable RDA Segmented Message Size: %d\n", 
                           msgHdrStructPtr->size );
                        break;

                  }

               }
                     
               /* Adaptation Data is byte-swapped when processed.   It is the only
                  segmented message that we cannot treat each segment as an array of
                  shorts.   The is message has to be swapped as a whole. */
               type = (int) msgHdrStructPtr->type;
               if( type != ADAPTATION_DATA ){

                  /* This was added to prevent mon_wb from failing when ORDA sends invalid
                     message types.  (Note: During development of ORDA Dual Pol upgrade, 
                     the RDA was sending non-existent message types.)
                  */
                  if( (type != DIGITAL_RADAR_DATA) 
                                 &&
                      (type != RDA_STATUS_DATA) 
                                 &&
                      (type != PERFORMANCE_MAINTENANCE_DATA)
                                 &&
                      (type != RDA_RPG_VCP)
                                 &&
                      (type != LOOPBACK_TEST_RDA_RPG)
                                 &&
                      (type != CLUTTER_MAP_DATA)
                                 &&
                      (type != CLUTTER_FILTER_BYPASS_MAP) 
                                 &&
                      (type != GENERIC_DIGITAL_RADAR_DATA)
                                 &&
                      (type != 0) ){

              
                      fprintf(stderr,
                           "ERROR: Invalid RDA->RPG Message Type: %d\n", 
                           msgHdrStructPtr->type );
                        break;

                  }

                  if( type != 0 )
                     UMC_RDAtoRPG_message_convert_to_internal( type, (void *) msgHdrPtr );

               }

            }
         #endif

         /* If the msg header exists, store the message type for convenience */
         if ( MsgHdrExists == MONWB_TRUE )
         {
            MessageType = msgHdrStructPtr->type;
         }

         if ( ArchivedPlaybackFlagResp == MONWB_FALSE )
         {
            /* THIS DATA HAS A CM HDR AND A CTM HDR */
            CommHdrExists = MONWB_TRUE;

            /* Set up structure to hold message data */
            responseStructPtr = (CM_resp_struct *) responseLbBufPtr;

            /* If there is a message header (not just a comms hdr), determine
               whether this message is from an ORDA or a legacy RDA
               configuration and set the global flag appropriately. */
            if ( (MsgHdrExists == MONWB_TRUE) && (responseStructPtr->type == CM_DATA) )
            {
               /* If RDA config flag hasn't been set, set it now */
               /* Also reset it for all RDA Status messages */
               if ( ( RdaChannelCfg < 0 ) ||
                    ( MessageType == RDA_STATUS_DATA ) )
               {
                  RdaChannelCfg = ORPGRDA_get_rda_config( msgHdrPtr );

                  /* for convenience set a global for the configuration type */
                  if ( RdaChannelCfg == ORPGRDA_ORDA_CONFIG )
                  {
                     IsOrdaConfig = MONWB_TRUE;
                  }
                  else if ( RdaChannelCfg == ORPGRDA_LEGACY_CONFIG )
                  {
                     IsOrdaConfig = MONWB_FALSE;
                  }
                  else if( responseStructPtr->type == CM_DATA )
                  {
                     fprintf(stderr,
                        "ERROR in ReadResponse. Problem with RdaChannelCfg.\n");
                  }
               }
            }

            /* store and print the date/time from the comms hdr */
            err = unix_time( (time_t *) &responseStructPtr->time, &yr,
               &mo, &day, &hr, &min, &sec);

            /* recode date to julian date */
            RPGCS_date_to_julian( yr, mo, day, &New_date);
            if ( New_date != Old_date )
            {
               /* print divider */
               fprintf(stderr, "\n%s\n", Divider);

               /* print date */
               fprintf(stderr, "%02d/%02d/%02d\n", mo, day, yr);
 
               /* update saved date */
               Old_date = New_date;
            }

            /* save the message time after converting to seconds */
            Msg_time = (int)((hr * 3600) + (min * 60) + sec);

            /* If basedata, don't process all messages 
               unless the "-a" option (the ProcessAllMsgsFlag will be set to 1)
               was given by the user. */
            if ( VerboseLevel >= MONWB_EXTREME )
            {
               if ( ( MsgHdrExists ) && 
                    (( MessageType == DIGITAL_RADAR_DATA ) ||
                     ( MessageType == GENERIC_DIGITAL_RADAR_DATA )))
               {
                  if ( ProcessAllMsgsFlag == MONWB_TRUE )
                  {
                     msgLen = sprintf(Message,
                        "\n%sResponse LB Comms Header:\n", BlankString);
                     PrintMsg(msgLen);
                     msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Response Type",
                        responseStructPtr->type);
                     PrintMsg(msgLen);
                     msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Request Number",
                        responseStructPtr->req_num);
                     PrintMsg(msgLen);
                     msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Link Index",
                        responseStructPtr->link_ind);
                     PrintMsg(msgLen);

                     /*** print response time ***/
                     err = unix_time( (time_t *) &responseStructPtr->time, &yr,
                                       &mo, &day, &hr, &min, &sec);
                     msgLen = sprintf(Message, MONWB_UNIX_TIME_WITHOUT_MS_FMT_STRING,
                        BlankString, "Response Time", hr, min, sec,
                        responseStructPtr->time);

                     PrintMsg(msgLen);
                     msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Return Code",
                        responseStructPtr->ret_code);
                     PrintMsg(msgLen);
                     msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Data Size",
                        responseStructPtr->data_size);
                     PrintMsg(msgLen);
                     fprintf(stderr, "\n");
                  }
               } 
               else
               {
                  msgLen = sprintf(Message,
                     "\n%sResponse LB Comms Header:\n", BlankString);
                  PrintMsg(msgLen);
                  msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Response Type",
                     responseStructPtr->type);
                  PrintMsg(msgLen);
                  msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Request Number",
                     responseStructPtr->req_num);
                  PrintMsg(msgLen);
                  msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Link Index",
                     responseStructPtr->link_ind);
                  PrintMsg(msgLen);

                  /*** print response time ***/
                  err = unix_time( (time_t *) &responseStructPtr->time, &yr,
                     &mo, &day, &hr, &min, &sec);
                  msgLen = sprintf(Message, MONWB_UNIX_TIME_WITHOUT_MS_FMT_STRING,
                     BlankString, "Response Time", hr, min, sec,
                     responseStructPtr->time);

                  PrintMsg(msgLen);
                  msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Return Code",
                     responseStructPtr->ret_code);
                  PrintMsg(msgLen);
                  msgLen = sprintf(Message, MONWB_INT_FMT_STRING, BlankString, "Data Size",
                     responseStructPtr->data_size);
                  PrintMsg(msgLen);
                  fprintf(stderr, "\n");
               }
            }
   
            /* Take necessary action depending on message type */
            switch ( responseStructPtr->type )
            {
               case CM_CONNECT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- CONNECTION RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DIAL_OUT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- DIAL-OUT RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DISCONNECT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- DISCONNECT RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_WRITE:
                  if ( VerboseLevel >= MONWB_HIGH )
                  {
                     msgLen = sprintf(Message,
                        "<--- WRITE RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_STATUS:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- CM STATUS RESP RECEIVED (Msg ID:%d Req Num:%u Len:%d)\n",
                        ResponseLbMsgId, responseStructPtr->req_num, MsgReadLen);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_CANCEL:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- CANCEL RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;
   
               case CM_DATA:
                  if ( (VerboseLevel >= MONWB_HIGH) &&
                       (ProcessAllMsgsFlag == MONWB_TRUE) )
                  {
                     msgLen = sprintf(Message,
                        "<--- DATA RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  
                  /* If the message is a Basedata message, we need to set the
                     RadialStatus global var now, before processing the message
                     header */
   
                  if ( MessageType == DIGITAL_RADAR_DATA )
                  {
                     if ( IsOrdaConfig == MONWB_FALSE )
                     {
                        lgcyBasedataMsgPtr = 
                           (RDA_basedata_header *) msgHdrPtr;
   
                        /* Set the RadialStatus */
                        RadialStatus = (unsigned short)lgcyBasedataMsgPtr->status;
                     }
                     else
                     {
                        ordaBasedataMsgPtr = 
                           (ORDA_basedata_header *) msgHdrPtr;
   
                        /* Set the RadialStatus */
                        RadialStatus = (unsigned short)ordaBasedataMsgPtr->status;
                     }
                  }
   
                  if ( MessageType == GENERIC_DIGITAL_RADAR_DATA )
                  {
                     /* set pointer to the actual data */
                     msgDataPtr = msgHdrPtr + MONWB_MSG_HDR_SIZE_SHORTS;

                     genBasedataMsgPtr=(Generic_basedata_header_t *) msgDataPtr;

                     /* Set the RadialStatus */
                     RadialStatus = (unsigned short) genBasedataMsgPtr->status;
                  }
   
                  /* call ProcessMsgHdr() */
                  err = ProcessMsgHdr(msgHdrStructPtr);
                  if (err < 0)
                  {
                     fprintf(stderr,
                        "ReadResponse: error verifying msg hdr (%d).\n", err);
                  }

                  /* call ProcessCmDataResponse() */
                  err = ProcessCmDataResponse((short)MessageType,
                     responseLbBufPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr, "ERROR: in ProcessCmDataResponse.\n");
                  }
                  break;
   
               case CM_EVENT:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- EVENT RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;

               case CM_SET_PARAMS:
                  if ( VerboseLevel >= MONWB_NONE )
                  {
                     msgLen = sprintf(Message,
                        "<--- SET PARAM RESP RECEIVED (Msg ID:%d Req Num:%u)\n",
                        ResponseLbMsgId, responseStructPtr->req_num);
                     PrintMsg(msgLen);
                  }
                  break;

               default:
                  break;
            }
         }
         else
         {
            /* NOTE: MESSAGE HAS NO COMMS HEADER OR CTM HEADER. */

            CommHdrExists = MONWB_FALSE;

            /* If RDA config flag hasn't been set, set it now */
            /* Also reset it for all RDA Status messages */
            if ( ( RdaChannelCfg < 0 ) ||
                 ( MessageType == RDA_STATUS_DATA ) )
            {
               RdaChannelCfg = ORPGRDA_get_rda_config( msgHdrPtr );
            }

            /* for convenience set a global for the configuration type */
            if ( RdaChannelCfg == ORPGRDA_ORDA_CONFIG )
            {
               IsOrdaConfig = MONWB_TRUE;
            }
            else if ( RdaChannelCfg == ORPGRDA_LEGACY_CONFIG )
            {
               IsOrdaConfig = MONWB_FALSE;
            }
            else
            {
               fprintf(stderr,
                  "ERROR in ReadResponse. Problem with RdaChannelCfg.\n");
            }

            /*** Print the msg date.  This will be done only at the beginning
                 and if it changes. ***/
            New_date = msgHdrStructPtr->julian_date;
            if ( New_date != Old_date )
            {
               /* convert date to human readable format and print */
               err = RPGCS_julian_to_date( msgHdrStructPtr->julian_date, &yr,
                  &mo, &day );

               /* print divider */
               fprintf(stderr, "\n%s\n", Divider);

               fprintf(stderr, "%02d/%02d/%02d\n", mo, day, yr);
               Old_date = New_date;
            }

            /* save the message time (seconds) */
            Msg_time = (int)(msgHdrStructPtr->milliseconds / 1000);

            /* set a pointer to the actual message data */
            msgDataPtr = msgHdrPtr + MONWB_MSG_HDR_SIZE_SHORTS;

            /* Need to determine the message type and take the 
               appropriate action */
            switch ( MessageType )
            {
               case DIGITAL_RADAR_DATA:

                  /* Cast the basedata ptr in order to access the structure
                     fields */
                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     lgcyBasedataMsgPtr = 
                        (RDA_basedata_header *) responseLbBufPtr;

                     /* Set the RadialStatus */
                     RadialStatus = (unsigned short) lgcyBasedataMsgPtr->status;
                  }
                  else
                  {
                     ordaBasedataMsgPtr = 
                        (ORDA_basedata_header *) responseLbBufPtr;

                     /* Set the RadialStatus */
                     RadialStatus = (unsigned short)ordaBasedataMsgPtr->status;
                  }

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  /* Process the basedata msg.  Send it the pointer to the Msg Hdr. */
                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyBasedataMsg(msgHdrPtr);
                     if ( err != MONWB_SUCCESS )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyBasedataMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaBasedataMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaBasedataMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  break;

               case GENERIC_DIGITAL_RADAR_DATA:
                  genBasedataMsgPtr=(Generic_basedata_header_t *) msgDataPtr;

                  /* Set the RadialStatus */
                  RadialStatus = (unsigned short) genBasedataMsgPtr->status;

                  /* process message header & check for validity */
                  err = ProcessGenRadMsg( msgHdrPtr );
                  if ( err != MONWB_SUCCESS )
                  {
                     fprintf(stderr,
                        "ReadResponse: Problem in ProcessGenRadMsg().\n");
                     return(MONWB_FAIL);
                  }
                  break;

               case RDA_STATUS_DATA:

                  msgLen =
                     sprintf(Message,
                     "<--- RDA STATUS MSG (Msg ID:%d Type:2 Len:%d)\n",
                     ResponseLbMsgId, MsgReadLen);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  /* Process the RDA Status msg.  Send it the pointer to the Msg Hdr. */
                  if ( IsOrdaConfig == MONWB_FALSE ) 
                  {
                     err = ProcessLgcyRdaStatusMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyRdaStatusMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaStatusMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaStatusMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  break;

               case PERFORMANCE_MAINTENANCE_DATA:
                  msgLen = 
                     sprintf(Message, 
                     "<--- PERFORMANCE/MAINTENANCE DATA MSG (Msg ID:%d Type:3)\n",
                     ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;
          
                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyPerfMaintMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyPerfMaintMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaPerfMaintMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaPerfMaintMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  break;

               case CONSOLE_MESSAGE_A2G:
                  msgLen = 
                     sprintf(Message, 
                     "<--- CONSOLE MSG (Msg ID:%d Type:4)", ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  err = ProcessConsoleMsg(msgHdrPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr, "ERROR: Problem in ProcessConsoleMsg().\n");
                     return(MONWB_FAIL);
                  }
                  break;

               case RDA_RPG_VCP:
                  msgLen = 
                     sprintf(Message, 
                     "<--- VOLUME COVERAGE PATTERN MSG (Msg ID:%d Type:5)",
                     ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  err = ProcessVcpMsg(msgDataPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr, "ERROR: Problem in ProcessVcpMsg().\n");
                     return(MONWB_FAIL);
                  }
                  break;

               case LOOPBACK_TEST_RDA_RPG:
                  msgLen = 
                     sprintf(Message, 
                     "<--- LOOPBACK TEST RECEIVED (Msg ID:%d Type:11)\n",
                     ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;
      
                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  /* Process the RDA Loop Back msg.  Since the Loop Back structure
                     doesn't contain the Msg Hdr structure, we have to send the
                     pointer to the actual loop back message, not the msg hdr. */
                  err = ProcessRdaRpgLoopBackMsg(msgDataPtr);
                  if ( err < 0 )
                  {
                     fprintf(stderr,
                        "ERROR: Problem in ProcessRdaToRpgLoopBackMsg().\n");
                     return(MONWB_FAIL);
                  }
                  break;

               case LOOPBACK_TEST_RPG_RDA:
                  msgLen = 
                     sprintf(Message, 
                     "<--- LOOPBACK RESP RECEIVED (Msg ID:%d Type:12)\n",
                     ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  break;

               case CLUTTER_FILTER_BYPASS_MAP:
                  if( VerboseLevel == MONWB_MODERATE ){

                     msgLen = 
                        sprintf(Message, 
                        "<--- CLUTR FILTR BYPSS MAP (Msg ID:%d Type:13 Seg:%d of %d)",
                        ResponseLbMsgId, msgHdrStructPtr->seg_num,
                        msgHdrStructPtr->num_segs);
                     PrintMsg(msgLen);

                  }
                  else if( msgHdrStructPtr->seg_num == msgHdrStructPtr->num_segs ){

                     msgLen = 
                        sprintf(Message, 
                        "<--- CLUTR FILTR BYPSS MAP (Msg ID:%d Type:13 Segs %d of %d Received)",
                        ResponseLbMsgId, msgHdrStructPtr->seg_num,
                        msgHdrStructPtr->num_segs);
                     PrintMsg(msgLen);

                  }
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;
                  
                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  if ( IsOrdaConfig == MONWB_FALSE )
                  {
                     err = ProcessLgcyCltrFltrBypsMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessLgcyCltrFltrBypsMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  else
                  {
                     err = ProcessOrdaCltrFltrBypsMsg(msgHdrPtr);
                     if ( err < 0 )
                     {
                        fprintf(stderr,
                           "ERROR: Problem in ProcessOrdaCltrFiltrBypsMsg().\n");
                        return(MONWB_FAIL);
                     }
                  }
                  break;

               case CLUTTER_MAP_DATA:
                  if (IsOrdaConfig == MONWB_FALSE )
                  {
                     msgLen = 
                        sprintf(Message, 
                        "<--- CLUTR FILTR NOTCHWIDTH MAP (Msg ID:%d Type:15 Seg:%d of %d)",
                        ResponseLbMsgId, msgHdrStructPtr->seg_num,
                        msgHdrStructPtr->num_segs);
                     PrintMsg(msgLen);
                     HdrDisplayFlag = MONWB_IS_DISPLAYED;
                  }
                  else
                  {
                     if( VerboseLevel == MONWB_MODERATE ){

                        msgLen = 
                           sprintf(Message, 
                           "<--- CLUTTER FILTER MAP (Msg ID:%d Type:15 Seg:%d of %d)",
                           ResponseLbMsgId, msgHdrStructPtr->seg_num,
                           msgHdrStructPtr->num_segs);
                        PrintMsg(msgLen);
                     }
                     else if( msgHdrStructPtr->seg_num == msgHdrStructPtr->num_segs ){

                        msgLen = 
                           sprintf(Message, 
                           "<--- CLUTTER FILTER MAP (Msg ID:%d Type:15 Segs %d of %d Received)",
                           ResponseLbMsgId, msgHdrStructPtr->seg_num,
                           msgHdrStructPtr->num_segs);
                        PrintMsg(msgLen);

                     }
                     HdrDisplayFlag = MONWB_IS_DISPLAYED;
                  }

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  break;

               case ADAPTATION_DATA:
                  msgLen = 
                     sprintf(Message, 
                     "<--- RDA ADAPTATION DATA MSG (Msg ID:%d Type:18)",
                     ResponseLbMsgId);
                  PrintMsg(msgLen);
                  HdrDisplayFlag = MONWB_IS_DISPLAYED;

                  /* process message header & check for validity */
                  err = ProcessMsgHdr( msgHdrStructPtr );
   
                  break;

               default:
                  if( MessageType != 0 )
                     fprintf(stderr,
                        "ERROR: Message type unknown (Msg ID:%d).\n",
                        ResponseLbMsgId);
                  break;
            }
         }
      }
 
      /* Set the flag that signals we're not processing the first msg anymore */
      firstTimeThrough = MONWB_FALSE;

      /* Free memory buffer if necessary */
      if ( responseLbBufPtr != NULL )
      {
         free( responseLbBufPtr );
         responseLbBufPtr = NULL;
      }

   } /* End of while loop. */

   /* Free memory buffer if necessary */
   if ( responseLbBufPtr != NULL )
   {
      free( responseLbBufPtr );
   }

   return returnVal;
} /* end ReadResponse */


/*******************************************************************************
* Function Name:	
*	ProcessCmWriteRequest
*
* Description:
*	Takes appropriate action for request LB messages of type CM_WRITE.
*
* Inputs:
*	None
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	05-15-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessCmWriteRequest(short msgType, char *msgBufferPtr)
{
   int				err			= 0;
   int				returnVal		= 0;
   int				msgLen			= 0;
   short*	   		msgHdrPtr		= NULL; 		
   short*	   		msgDataPtr		= NULL; 		
   RDA_RPG_message_header_t*	msgHdrStruct		= NULL;
   CM_req_struct*		requestStructPtr	= NULL;

   if ( (msgType > 0) && (msgType <= MONWB_NUM_MESSAGE_TYPES) )
   { 
      /* Cast the comms hdr structure for access to the fields */
      requestStructPtr = (CM_req_struct *)msgBufferPtr;

      /* Find start of message header */
      msgHdrPtr = (short *) (msgBufferPtr + sizeof(CM_req_struct) +
         CTM_HDRSZE_BYTES);

      msgHdrStruct = (RDA_RPG_message_header_t *) msgHdrPtr;

      /* Find start of the actual data message */
      msgDataPtr = (short *) (msgBufferPtr + sizeof(CM_req_struct) +
         CTM_HDRSZE_BYTES + sizeof(RDA_RPG_message_header_t));

      /* Determine what to do depending on message type */
      switch ( msgType )
      {
         case RDA_CONTROL_COMMANDS:
            msgLen = 
               sprintf(Message, 
               "---> RDA CONTROL COMMANDS MSG (Msg ID:%d Type:6)", RequestLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyRdaControlCmdsMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessLgcyRdaControlCmdsMsg().\n");
                  return(-1);
               }
            }
            else
            {
               err = ProcessOrdaControlCmdsMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessOrdaControlCmdsMsg().\n");
                  return(-1);
               }
            }
            break;

         case REQUEST_FOR_DATA:
            msgLen = 
               sprintf(Message, 
               "---> REQUEST FOR DATA (Msg ID:%d Type:9 Req Num:%u)\n",
               RequestLbMsgId, requestStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
       
            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyReqForDataMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessLgcyReqForDataMsg().\n");
                  return(-1);
               }
            }
            else
            {
               err = ProcessOrdaReqForDataMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessOrdaReqForDataMsg().\n");
                  return(-1);
               }
            }
            break;

         case LOOPBACK_TEST_RPG_RDA:
            msgLen = 
               sprintf(Message, 
                  "---> LOOPBACK TEST SENT (Msg ID:%d Type:12 Req Num:%u)\n",
                  RequestLbMsgId, requestStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            break;

         case LOOPBACK_TEST_RDA_RPG:
            /* This is the RPG's response to the RDA initiated Loopback msg */
            msgLen = 
               sprintf(Message, 
               "---> LOOPBACK RESP SENT (Msg ID:%d Type:11 Req Num:%u)\n",
               RequestLbMsgId, requestStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            break;

         case RPG_RDA_VCP:
            msgLen = 
               sprintf(Message, 
                  "---> VCP MESSAGE SENT (Msg ID:%d Type:7 Req Num:%u)\n",
                  RequestLbMsgId, requestStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            /* set global msg type flag */
            MessageType = RDA_RPG_VCP;

            err = ProcessVcpMsg(msgDataPtr);
            if ( err < 0 )
            {
               fprintf(stderr,
                  "ERROR: Problem in ProcessVcpMsg().\n");
               return(-1);
            }
            break;

         case EDITED_CLUTTER_FILTER_MAP:
            msgLen = 
               sprintf(Message, 
               "---> EDITED CLUTTER FILTER MAP SENT (Msg ID:%d Type:14 Seg:%d of %d)",
               RequestLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            break;

         case CLUTTER_SENSOR_ZONES:
            msgLen = 
               sprintf(Message, 
               "---> CLUTTER SENSOR ZONES SENT (Msg ID:%d Type:8)\n", RequestLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyClczMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessLgcyClczMsg().\n");
                  return(-1);
               }
            }
            else
            {
               err = ProcessOrdaClczMsg(msgDataPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessOrdaClczMsg().\n");
                  return(-1);
               }
            }
            break;

         case CONSOLE_MESSAGE_G2A:
            msgLen = sprintf(Message, 
               "---> CONSOLE MSG (Msg ID:%d Type:10)", RequestLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            err = ProcessConsoleMsg(msgHdrPtr);
            if ( err < 0 )
            {
               fprintf(stderr, "ERROR: Problem in ProcessConsoleMsg().\n");
               return(-1);
            }
            break;

         default:
            fprintf(stderr,
               "CM_WRITE request message type unknown. (Msg ID:%d Req Num:%u)",
               RequestLbMsgId, requestStructPtr->req_num);
            break;

      } /* end switch on msg type */
   } /* end if then that checks for errant msg type value */
   else
   {
      fprintf(stderr,
         "CM_WRITE request message type unknown. (Msg ID:%d Req Num:%u)",
         RequestLbMsgId, requestStructPtr->req_num);
      return(-1);
   }

   return returnVal;
} /* end ProcessCmWriteRequest */


/*******************************************************************************
* Function Name:	
*	ProcessCmDataResponse
*
* Description:
*	Takes appropriate action for response LB messages of type CM_DATA.
*
* Inputs:
*	None
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-31-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessCmDataResponse(short msgType, char *msgBufferPtr)
{
   int				err			= 0;
   int				returnVal		= 0;
   int				msgLen			= 0;
   short*	   		msgHdrPtr		= NULL; 		
   short*	   		msgDataPtr		= NULL; 		
   RDA_RPG_message_header_t*	msgHdrStruct		= NULL;
   CM_resp_struct*		responseStructPtr	= NULL;

   /* Cast the CM header structure pointer for access to fields */
   responseStructPtr = (CM_resp_struct *) msgBufferPtr;

   if ( (msgType > 0) && (msgType <= MONWB_NUM_MESSAGE_TYPES) )
   { 
      /* Find start of message header */
      msgHdrPtr = (short *) (msgBufferPtr + sizeof(CM_resp_struct) +
         CTM_HDRSZE_BYTES);

      msgHdrStruct = (RDA_RPG_message_header_t *) msgHdrPtr;

      /* Find start of the actual data message */
      msgDataPtr = (short *) (msgBufferPtr + sizeof(CM_resp_struct) +
         CTM_HDRSZE_BYTES + sizeof(RDA_RPG_message_header_t));

      /* Determine what to do depending on message type */
      switch ( msgType )
      {
         case DIGITAL_RADAR_DATA:
            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyBasedataMsg(msgHdrPtr);
               if ( err != MONWB_SUCCESS )
               {
                  fprintf(stderr,
                     "ProcessCmDataResponse: Error in ProcessLgcyBasedataMsg().\n");
                  return(MONWB_FAIL);
               }
            }
            else
            {
               err = ProcessOrdaBasedataMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr, "ERROR: Problem in ProcessOrdaBasedataMsg().\n");
                  return(-1);
               }
            }
            break;

         case GENERIC_DIGITAL_RADAR_DATA:
            err = ProcessGenRadMsg( msgHdrPtr );
            if ( err != MONWB_SUCCESS )
            {
               fprintf(stderr,
                  "ProcessCmDataResponse: Problem in ProcessOrdaBasedataMsg().\n");
               return(MONWB_FAIL);
            }
            break;

         case RDA_STATUS_DATA:
            msgLen =
               sprintf(Message,
               "<--- RDA STATUS MSG (Msg ID:%d Type:2 Len:%d Req Num:%u)\n",
               ResponseLbMsgId, MsgReadLen, responseStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            /* Process the RDA Status msg.  Send it the pointer to the Msg Hdr. */
            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyRdaStatusMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr, "ERROR: Problem in ProcessLgcyRdaStatusMsg().\n");
                  return(-1);
               }
            } 
            else
            {
               err = ProcessOrdaStatusMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr, "ERROR: Problem in ProcessOrdaStatusMsg().\n");
                  return(-1);
               }
            }
            break;

         case PERFORMANCE_MAINTENANCE_DATA:
            msgLen = 
               sprintf(Message, 
               "<--- PERFORMANCE/MAINTENANCE DATA MSG (Msg ID:%d Type:3)",
               ResponseLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyPerfMaintMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr, "ERROR: Problem in ProcessLgcyPerfMaintMsg().\n");
                  return(-1);
               }
            }
            else
            {
               err = ProcessOrdaPerfMaintMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr, "ERROR: Problem in ProcessOrdaPerfMaintMsg().\n");
                  return(-1);
               }
            }
            break;
     
         case CONSOLE_MESSAGE_A2G:
            msgLen = 
               sprintf(Message, 
               "<--- CONSOLE MSG (Msg ID:%d Type:4)", ResponseLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            err = ProcessConsoleMsg(msgHdrPtr);
            if ( err < 0 )
            {
               fprintf(stderr, "ERROR: Problem in ProcessConsoleMsg().\n");
               return(-1);
            }
            break;

         case RDA_RPG_VCP:
            msgLen = 
               sprintf(Message, 
               "<--- VOLUME COVERAGE PATTERN MSG (Msg ID:%d Type:5)",
               ResponseLbMsgId);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            /* set global msg type flag */
            MessageType = RDA_RPG_VCP;

            err = ProcessVcpMsg(msgDataPtr);
            if ( err < 0 )
            {
               fprintf(stderr, "ERROR: Problem in ProcessVcpMsg().\n");
               return(-1);
            }
            break;

         case LOOPBACK_TEST_RDA_RPG:
            msgLen = 
               sprintf(Message, 
               "<--- LOOPBACK TEST RECEIVED (Msg ID:%d Type:11 Req Num:%u)\n",
               ResponseLbMsgId, responseStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            /* Process the RDA Loop Back msg.  Since the Loop Back structure
               doesn't contain the Msg Hdr structure, we have to send the
               pointer to the actual loop back message, not the msg hdr. */
            err = ProcessRdaRpgLoopBackMsg(msgDataPtr);
            if ( err < 0 )
            {
               fprintf(stderr,
                  "ERROR: Problem in ProcessRdaToRpgLoopBackMsg().\n");
               return(-1);
            }
            break;

         case LOOPBACK_TEST_RPG_RDA:
            /* This is the RDA's response to the RPG initiated Loopback msg */
            msgLen = 
               sprintf(Message, 
               "<--- LOOPBACK RESP RECEIVED (Msg ID:%d Type:12 Req Num:%u)\n",
               ResponseLbMsgId, responseStructPtr->req_num);
            PrintMsg(msgLen);
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            break;

         case CLUTTER_FILTER_BYPASS_MAP:
            if( VerboseLevel == MONWB_MODERATE ){

               msgLen = 
                  sprintf(Message, 
                  "<--- CLUTR FILTR BYPSS MAP (Msg ID:%d Type:13 Seg:%d of %d)",
                  ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
               PrintMsg(msgLen);

            }
            else if( msgHdrStruct->seg_num == msgHdrStruct->num_segs ){

               msgLen = 
                  sprintf(Message, 
                  "<--- CLUTR FILTR BYPSS MAP (Msg ID:%d Type:13 Segs %d of %d Received)",
                  ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
               PrintMsg(msgLen);

            }
            HdrDisplayFlag = MONWB_IS_DISPLAYED;

            if ( IsOrdaConfig == MONWB_FALSE )
            {
               err = ProcessLgcyCltrFltrBypsMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessLgcyCltrFltrBypsMsg().\n");
                  return(-1);
               }
            }
            else
            {
               err = ProcessOrdaCltrFltrBypsMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessOrdaCltrFltrBypsMsg().\n");
                  return(-1);
               }
            }
            
            break;
   
         case CLUTTER_MAP_DATA:
            if (IsOrdaConfig == MONWB_FALSE )
            {
               if( VerboseLevel == MONWB_MODERATE ){

                  msgLen = 
                     sprintf(Message, 
                     "<--- CLUTR FILTR NOTCHWIDTH MAP (Msg ID:%d Type:15 Seg:%d of %d)",
                     ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
                  PrintMsg(msgLen);

               }
               else if( msgHdrStruct->seg_num == msgHdrStruct->num_segs ){

                  msgLen = 
                     sprintf(Message, 
                     "<--- CLUTR FILTR NOTCHWIDTH MAP (Msg ID:%d Type:15 Segs %d of %d Received)",
                     ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
                  PrintMsg(msgLen);

               }
               HdrDisplayFlag = MONWB_IS_DISPLAYED;

               err = ProcessLgcyNotchWidthMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessLgcyNotchWidthMsg().\n");
                  return(-1);
               }
            }
            else
            {
               if( VerboseLevel == MONWB_MODERATE ){

                  msgLen = 
                     sprintf(Message, 
                     "<--- CLUTTER FILTER MAP (Msg ID:%d Type:15 Seg:%d of %d)",
                     ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
                  PrintMsg(msgLen);

               }
               else if( msgHdrStruct->seg_num == msgHdrStruct->num_segs ){

                  msgLen = 
                     sprintf(Message, 
                     "<--- CLUTTER FILTER MAP (Msg ID:%d Type:15 Segs %d of %d Received)",
                     ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
                  PrintMsg(msgLen);

               }
               HdrDisplayFlag = MONWB_IS_DISPLAYED;

               err = ProcessOrdaClutterMapMsg(msgHdrPtr);
               if ( err < 0 )
               {
                  fprintf(stderr,
                     "ERROR: Problem in ProcessOrdaClutterMapMsg().\n");
                  return(-1);
               }
            }
            break;

         case ADAPTATION_DATA:
            if( VerboseLevel == MONWB_MODERATE ){

               msgLen = 
                  sprintf(Message, 
                  "<--- RDA ADAPTATION DATA MSG (Msg ID:%d Type:18 Seg:%d of %d)",
                  ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
               PrintMsg(msgLen);

            }
            else if( msgHdrStruct->seg_num == msgHdrStruct->num_segs ){

               msgLen = 
                  sprintf(Message, 
                  "<--- RDA ADAPTATION DATA MSG (Msg ID:%d Type:18 Segs %d of %d Received)",
                  ResponseLbMsgId, msgHdrStruct->seg_num, msgHdrStruct->num_segs);
               PrintMsg(msgLen);

            } 
            HdrDisplayFlag = MONWB_IS_DISPLAYED;
            err = ProcessAdaptDataMsg(msgHdrPtr);
            if ( err < 0 )
            {
               fprintf(stderr,
                  "ERROR: Problem in ProcessAdaptDataMsg().\n");
               return(-1);
            }
            break;

         default:
            if( msgType != 0 )
               fprintf(stderr,
                  "1: CM_DATA response message type unknown: %d (Msg ID:%d Req Num:%u)",
                  msgType, ResponseLbMsgId, responseStructPtr->req_num);
            break;

      } /* end switch on msg type */
   } /* end if then that checks for errant msg type value */
   else if( msgType != 0 )
   {
      fprintf(stderr,
         "2: CM_DATA response message type unknown: %d. (Msg ID:%d Req Num:%u)\n",
          msgType, ResponseLbMsgId, responseStructPtr->req_num);
      return(-1);
   }

   return returnVal;
} /* end ProcessCmDataResponse */


/*******************************************************************************
* Function Name:	
*	ProcessMsgHdr
*
* Description:
*	Read and check the ICD message header (see Table II Message Header Data
*	in the RDA/RPG ICD) for validity.
*
* Inputs:
*	RDA_RPG_message_header_t* msgHdrStructPtr - pointer to the message
*                                                   header structure
*	
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	04-21-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessMsgHdr(RDA_RPG_message_header_t* msgHdrStructPtr)
{
   int		returnVal	= MONWB_SUCCESS;

   if ( msgHdrStructPtr == NULL )
   {
      fprintf(stderr, "ProcessMsgHdr: ERROR - msgHdrStructPtr = NULL.\n");
      return(MONWB_FAIL);
   }

   if ( VerboseLevel >= MONWB_HIGH )
   {
      if ( (MessageType == DIGITAL_RADAR_DATA ) ||
           (MessageType == GENERIC_DIGITAL_RADAR_DATA ))
      {
         if ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
              (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      || 
              (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
              (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
              (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END)     ||
              (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) )
         {
            /* Call function to print out message header fields */
            PrintMsgHdr(msgHdrStructPtr);

         }
      }
      else
      {
         /* Call function to print out message header fields */
         PrintMsgHdr(msgHdrStructPtr);

      }
   }

   return returnVal;
} /* end ProcessMsgHdr */


/*******************************************************************************
* Function Name:	
*	ProcessRdaRpgLoopBackMsg
*
* Description:
*	Read and check the RDA Loop Back Message for ICD conformance.
*
*	NOTE: In contrast to the Basedata and RDA Status messages, the 
*	RDA Loop Back Message structure does not contain the Message 
*	Header within it.  
*
* Inputs:
*	short* msgPtr - pointer to message starting with the 16 byte msg hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	04-28-2003, R. Solomon.  Created.
*	05-09-2003, R. Solomon.  Changed to accept a pointer to the Loop Back
*				 message itself, not the msg header.
*******************************************************************************/
static int ProcessRdaRpgLoopBackMsg(short *msgPtr)
{
   int				returnVal		= 0;
   RDA_RPG_loop_back_message_t	*rdaRpgLoopBackMsgPtr	= NULL;

   /* Set the MessageType global variable to the proper number */
   MessageType = LOOPBACK_TEST_RDA_RPG;


   /* The Loop Back data structure doesn't contain the message header
      structure within it. Expecting that the msgPtr points to the Loop
      Back Message itself, not the header. */
   rdaRpgLoopBackMsgPtr = (RDA_RPG_loop_back_message_t *)(msgPtr);

   if ( rdaRpgLoopBackMsgPtr == NULL )
   {
      fprintf(stderr, "ERROR: Problem in ProcessRdaRpgLoopBackMsg.\n");
      return(-1);
   }

   if ( VerboseLevel >= MONWB_HIGH )
   {
      /* Print out message fields */
      fprintf(stderr, "\n%sRDA RPG Loop Back Message:",
         BlankString);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Size",
         rdaRpgLoopBackMsgPtr->size);
      fprintf(stderr, "\n");
   }

   return returnVal;
} /* end ProcessRdaRpgLoopBackMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyRdaStatusMsg
*
* Description:
*	Read and check the Legacy RDA Status Message for ICD conformance.
*       NOTE: The Legacy RDA Status Message data structure contains within it the
*	RDA/RPG Message Header structure.  Therefore, when the RDA Status Msg
*	structure pointer is cast, it is cast at the beginning of the Message
*	Header instead of after it.
*
* Inputs:
*	short* msgPtr - pointer to msg starting with the 16 byte msg header.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	04-28-2003, R. Solomon.  Created ProcessLgcyRdaStatusMsg.
*	06-12-2003, R. Solomon.  Changed the name to ProcessLgcyRdaStatusMsg in
*				 order to support handling of the legacy RDA
*				 Status messages.
*******************************************************************************/
static int ProcessLgcyRdaStatusMsg(short *msgPtr)
{
   int			err			= 0;
   int			msgLen			= 0;
   int			returnVal		= 0;
   RDA_status_msg_t	*rdaStatusStructPtr	= NULL;
   int			yr, mo, day, hr, min, sec, millisec;
   int			bp_map_time_millisec	= 0;
   int			nw_map_time_millisec	= 0;
   int			msg_index		= 0;
   unsigned short	rda_operability		= 0;
   unsigned short	auto_cal_disabled	= 0;
   char*		temp_str		= NULL;
   int			alarm_index		= 0;
   int			first_in_list		= 0;
   int			char_count		= 0;

   static char*		rda_config		= "LEGACY";

   static char*		auto_cal_string		= "Auto Cal Off";

   static char*		rda_status[]		= {"Startup",
                                                   "Standby",
                                                   "Restart",
                                                   "Operate",
                                                   "Playback",
                                                   "Offline Operate",
                                                   "Unknown"};

   static char*		op_status[]		= {"On-line",
                                                   "Maint. Req'd",
                                                   "Maint. Mand.",
                                                   "Commanded Shutdown",
                                                   "Inop.",
                                                   "WB Disconnect",
                                                   "Unknown"};

   static char*		control_status[]	= {"RDA Control",
                                                   "RPG Control",
                                                   "Either",
                                                   "Unknown"};

   static char*		data_trans_enbl[]	= {"None",
                                                   "Refl",
                                                   "Vel",
                                                   "Spec Width",
                                                   "Refl, Vel",
                                                   "Refl, Spec Width",
                                                   "Vel, Spec Width",
                                                   "Refl, Vel, Spec Width",
                                                   "Unknown"};

   static char*		control_auth[]		= {"No Action",
                                                   "Local Control Requested",
                                                   "Remote Control Enabled",
                                                   "Unknown"};

   static char*		op_mode[]		= {"Maintenance",
                                                   "Operational",
                                                   "Unknown"};

   static char*		aux_pwr_state[]		= {"Util Pwr Avail",
                                                   "Generator On",
                                                   "Switch(Manual)",
                                                   "Commanded Switchover",
                                                   "Unknown"};

   static char*		aux_pwr_string		= "Aux Pwr On"; 

   static char*		int_supp_unit[]		= {"Enabled",
                                                   "Disabled",
                                                   "No Change",
                                                   "Unknown"};

   static char*		cmd_ack[]		= {"None",
                                                   "Remote VCP Rec'd",
                                                   "Clutter Bypass Map Rec'd",
                                                   "Clutter Censor Zones Rec'd",
                                                   "Redund Stdby Cmd Accepted",
                                                   "Unknown"};

   static char*		chan_cntrl_stat[]	= {"Controlling",
                                                   "Non-controlling",
                                                   "Unknown"};

   static char*		spot_blank_stat[]	= {"Not Installed",
                                                   "Enabled",
                                                   "Disabled",
                                                   "Unknown"};

   static char*		tps_stat[]		= {"Not Installed",
                                                   "Off",
                                                   "Ok",
                                                   "Unknown"};

   static char*		rms_cntrl_stat[]	= {"Non-RMS System",
                                                   "RMS In Control",
                                                   "HCI In Control",
                                                   "Unknown"};

   rdaStatusStructPtr = (RDA_status_msg_t *) (msgPtr);

   if ( rdaStatusStructPtr == NULL )
   {
      fprintf(stderr,
         "ERROR: Problem casting struct ptr in ProcessLgcyRdaStatusMsg.\n");
      return(-1);
   }

   /* Set global message type flag */
   MessageType = RDA_STATUS_DATA;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Print out message fields */
      msgLen = sprintf(Message, "\n%sRDA Status Message:",
         BlankString);
      PrintMsg(msgLen);
   }
 
   /* Print the RDA Configuration */
   fprintf( stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Configuration", rda_config);

   /* Print more important fields in lower verbosity levels */
   if ( VerboseLevel >= MONWB_LOW )
   {
      /*** RDA Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->rda_status )
      {
         case ( (unsigned short) RS_STARTUP ):
            msg_index = 0;
            break;

         case ( (unsigned short) RS_STANDBY ):
            msg_index = 1;
            break;

         case ( (unsigned short) RS_RESTART ):
            msg_index = 2;
            break;

         case ( (unsigned short) RS_OPERATE ):
            msg_index = 3;
            break;

         case ( (unsigned short) RS_PLAYBACK ):
            msg_index = 4;
            break;

         case ( (unsigned short) RS_OFFOPER ):
            msg_index = 5;
            break;

         default:
            msg_index = 6;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Status",
         rda_status[msg_index]);


      /*** Operability Status - not mutually exclusive ***/
      auto_cal_disabled = (unsigned short)(rdaStatusStructPtr->op_status & 0x0001);
      rda_operability = (unsigned short)(rdaStatusStructPtr->op_status & 0xFFFE);

      /* set up string buffer */
      temp_str = calloc( MONWB_MAX_NUM_CHARS, sizeof(char) );
      if (temp_str != NULL )
      {  
         /* set flag to indicate first one in list */
         first_in_list = MONWB_TRUE;

         /* initialize the character count to zero */
         char_count = 0;

         if ( rda_operability & OS_ONLINE )
         {
            msg_index = 0;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_MAINTENANCE_REQ ) )
         {
            msg_index = 1;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_MAINTENANCE_MAN ) )
         {
            msg_index = 2;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_COMMANDED_SHUTDOWN ) )
         {
            msg_index = 3;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_INOPERABLE ) )
         {
            msg_index = 4;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_WIDEBAND_DISCONNECT ) )
         {
            msg_index = 5;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( auto_cal_disabled )
         {
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, auto_cal_string);
               char_count += strlen(auto_cal_string);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat( temp_str, auto_cal_string );
               char_count += strlen(auto_cal_string);
            }
         }
         if ( char_count <= 0 )
         {
            msg_index = 6;
            strcat( temp_str, op_status[msg_index]);
            temp_str[char_count] = '\0';
         }
         else
         {
            temp_str[char_count] = '\0';
         }
      }
      fprintf( stderr, MONWB_CHAR_FMT_STRING, BlankString, "Operability Status",
         temp_str);

      /*** Control Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->control_status )
      {
         case ( unsigned short ) CS_LOCAL_ONLY:
            msg_index = 0;
            break;

         case ( unsigned short ) CS_RPG_REMOTE:
            msg_index = 1;
            break;

         case ( unsigned short ) CS_EITHER:
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Control Status",
         control_status[msg_index]);


      /*** Data Transmission Enabled - not mutually exclusive ***/
      switch ( rdaStatusStructPtr->data_trans_enbld )
      {
         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_NONE:
            msg_index = 0;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_R:
            msg_index = 1;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_V:
            msg_index = 2;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_W:
            msg_index = 3;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RV:
            msg_index = 4;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RW:
            msg_index = 5;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_VW:
            msg_index = 6;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RVW:
            msg_index = 7;
            break;

         default:
            msg_index = 8;
            break;
      }


      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Data Transmission Enbld",
         data_trans_enbl[msg_index]);


      /*** VCP Number ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "VCP Number",
         rdaStatusStructPtr->vcp_num);


      /*** RDA Control Authorization - mutually exclusive ***/
      switch ( rdaStatusStructPtr->rda_control_auth )
      {
         case ( unsigned short ) CA_NO_ACTION:
            msg_index = 0;
            break;

         case ( unsigned short ) CA_LOCAL_CONTROL_REQUESTED:
            msg_index = 1;
            break;

         case ( unsigned short ) CA_REMOTE_CONTROL_ENABLED:
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Control Auth",
         control_auth[msg_index]);


      /*** RDA Operational Mode - mutually exclusive ***/
      switch ( rdaStatusStructPtr->op_mode )
      {
         case ( unsigned short ) OP_MAINTENANCE_MODE:
            msg_index = 0;
            break;

         case ( unsigned short ) OP_OPERATIONAL_MODE:
            msg_index = 1;
            break;

         default :
            msg_index = 2;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Operational Mode",
         op_mode[msg_index]);
   }

   /* Less important fields */
   if ( VerboseLevel >= MONWB_MODERATE )
   {
      first_in_list = 1;

      fprintf(stderr, MONWB_CHAR_FMT_STRING_NOFIELD, BlankString,
         "Auxiliary Power State");

      /*** Auxiliary Power Generator State - not mutually exclusive ***/
      switch ( (rdaStatusStructPtr->aux_pwr_state & 0xFFFE) )
      {
         case (unsigned short) AP_UTILITY_PWR_AVAIL :
            msg_index = 0;
            if ( first_in_list )
            {
               first_in_list = 0;
               fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE,
                  aux_pwr_state[msg_index]);
            }
            else
            {
               fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
                  aux_pwr_state[msg_index]);
            }
            break;

         case (unsigned short) AP_GENERATOR_ON :
            msg_index = 1;
            if ( first_in_list )
            {
               first_in_list = 0;
               fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE,
                  aux_pwr_state[msg_index]);
            }
            else
            {
               fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
                  aux_pwr_state[msg_index]);
            }
            break;

         case (unsigned short) AP_TRANS_SWITCH_MAN :
            msg_index = 2;
            if ( first_in_list )
            {
               first_in_list = 0;
               fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE,
                  aux_pwr_state[msg_index]);
            }
            else
            {
               fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
                  aux_pwr_state[msg_index]);
            }
            break;

         case (unsigned short) AP_COMMAND_SWITCHOVER :
            msg_index = 3;
            if ( first_in_list )
            {
               first_in_list = 0;
               fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE,
                  aux_pwr_state[msg_index]);
            }
            else
            {
               fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
                  aux_pwr_state[msg_index]);
            }
            break;

         default :
            msg_index = 4;
            if ( first_in_list )
            {
               first_in_list = 0;
               fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE,
                  aux_pwr_state[msg_index]);
            }
            else
            {
               fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
                  aux_pwr_state[msg_index]);
            }
            break;
      }


      /* Determine if power has switched to auxiliary */
      if ( rdaStatusStructPtr->aux_pwr_state & 0x0001 )
      {
         if ( first_in_list )
         {
            first_in_list = 0;
            fprintf(stderr, MONWB_CHAR_FMT_STRING_NOTITLE, aux_pwr_string);
         }
         else
         {
            fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, BlankString,
               aux_pwr_string);
         }
      }


      /*** Average Transmitter Power ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Avg Xmtr Power (W)",
         rdaStatusStructPtr->ave_trans_pwr);


      /*** Reflectivity Calibration Correction ***/
      {
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Refl Calib Corr. (dB)",
            (rdaStatusStructPtr->ref_calib_corr * 0.25));
      }

      /*** Interference Detection Rate ***/
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Interf. Detection Rate",
         rdaStatusStructPtr->int_detect_rate);


      /*** Interference Suppression Unit - mutually exclusive ***/
      switch ( rdaStatusStructPtr->int_suppr_unit )
      {
         case (unsigned short) ISU_ENABLED :
            msg_index = 0;
            break;

         case (unsigned short) ISU_DISABLED :
            msg_index = 1;
            break;

         case (unsigned short) ISU_NOCHANGE :
            msg_index = 2;
            break;
 
         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Interf. Suppr. Unit",
         int_supp_unit[msg_index]);


      /*** Archive II Status - too many options to list.  Just print #.***/
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Archive II Status", 
         rdaStatusStructPtr->arch_II_status);


      /*** Archive II Capacity ***/
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Archive II Capacity", 
         rdaStatusStructPtr->arch_II_capacity);


      /*** RDA Alarm Summary - not mutually exclusive ***/
      if ( rdaStatusStructPtr->rda_alarm != 0 )
      {
         /* set up string buffer */
         temp_str = calloc( MONWB_MAX_NUM_CHARS_IN_ALARM_STR, sizeof(char) );
         if ( temp_str != NULL )
         {
            /* set flag to indicate first one in list */
            first_in_list = 1; /* true */

            /* initialize the character count to 0 */
            char_count = 0;

            if ( rdaStatusStructPtr->rda_alarm & 0x0002 )
            {
               /* Add Tower-Utilities to alarm list */        
               if ( first_in_list )
               {
                  strcat( temp_str, "Tow-Util");
                  char_count = strlen("Tow-Util") + 1;
                  first_in_list = 0;
               }
               else
               {
                  strcat( temp_str, ",Tow-Util");
                  char_count += strlen(",Tow-Util") + 1;
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0004 )
            {
               /* Add Pedestal to alarm list */        
               if ( (char_count + strlen(",Ped") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Ped");
                     char_count = strlen("Ped") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Ped");
                     char_count += strlen(",Ped") + 1;
                  }
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0008 )
            {
               /* Add Transmitter to alarm list */        
               if ( (char_count + strlen(",Xmtr" + 1)) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Xmtr");
                     char_count = strlen("Xmtr") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Xmtr");
                     char_count += strlen(",Xmtr") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0010 )
            {
               /* Add Receiver-Signal Processer to alarm list */        
               if ( (char_count + strlen(",Recvr-SigProc") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Rcvr-SigProc");
                     char_count = strlen("Rcvr-SigProc") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Rcvr-SigProc");
                     char_count += strlen(",Rcvr-SigProc") + 1;
                  }
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0020 )
            {
               /* Add RDA Control to alarm list */        
               if ( (char_count + strlen(",RDA Cntrl") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "RDA Cntrl");
                     char_count = strlen("RDA Cntrl") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",RDA Cntrl");
                     char_count += strlen(",RDA Cntrl") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0040 )
            {
               /* Add RPG Communication to alarm list */        
               if ( (char_count + strlen(",RPG Comm") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "RPG Comm");
                     char_count = strlen("RPG Comm") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",RPG Comm");
                     char_count += strlen(",RPG Comm") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0080 )
            {
               /* Add User Communication to alarm list */        
               if ( (char_count + strlen(",Usr Comm") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Usr Comm");
                     char_count = strlen("Usr Comm") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Usr Comm");
                     char_count += strlen(",Usr Comm") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0100 )
            {
               /* Add Archive II to alarm list */        
               if ( (char_count + strlen(",Arch II") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Arch II");
                     char_count = strlen("Arch II") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Arch II");
                     char_count += strlen(",Arch II") + 1;
                  }
               }
            }
         }

         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Alarm Summary", temp_str);
         free( temp_str );
      }
      else
      {
         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Alarm Summary", "No Alarms");
      }


      /*** Command Acknowledgement - mutually exclusive ***/
      switch ( rdaStatusStructPtr->command_status )
      {
         case (unsigned short) RS_NO_ACKNOWLEDGEMENT :
            msg_index = 0;
            break;

         case (unsigned short) RS_REMOTE_VCP_RECEIVED :
            msg_index = 1;
            break;

         case (unsigned short) RS_CLUTTER_BYPASS_MAP_RECEIVED :
            msg_index = 2;
            break;

         case (unsigned short) RS_CLUTTER_CENSOR_ZONES_RECEIVED :
            msg_index = 3;
            break;

         case (unsigned short) RS_REDUND_CHNL_STBY_CMD_ACCEPTED :
            msg_index = 4;
            break;

         default :
            msg_index = 5;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Command Acknowledgment",
         cmd_ack[msg_index]);


      /*** Channel Control Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->channel_status )
      {
         case ( unsigned short ) RDA_IS_CONTROLLING :
            msg_index = 0;
            break;

         case ( unsigned short ) RDA_IS_NON_CONTROLLING :
            msg_index = 1;
            break;
 
         default :
            msg_index = 2;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Channel Control Status",
         chan_cntrl_stat[msg_index]);


      /*** Spot Blanking Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->spot_blanking_status )
      {
         case ( unsigned short ) SB_NOT_INSTALLED :
            msg_index = 0;
            break;

         case ( unsigned short ) SB_ENABLED :
            msg_index = 1;
            break;
 
         case ( unsigned short ) SB_DISABLED :
            msg_index = 2;
            break;
 
         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Spot Blanking Status",
         spot_blank_stat[msg_index]);


      /*** Modified Julian Date - also print human readable format ***/
      err = RPGCS_julian_to_date( rdaStatusStructPtr->bypass_map_date, &yr,
         &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date", mo, day, yr, rdaStatusStructPtr->bypass_map_date);


      /*** BPM generation time, min past midnight, convert to hr:min ***/
      bp_map_time_millisec = rdaStatusStructPtr->bypass_map_time *
         MONWB_MINS_TO_MILLISECS;
      err = RPGCS_convert_radial_time( bp_map_time_millisec, &hr, &min, &sec,
         &millisec);
      fprintf(stderr, MONWB_UNIX_TIME_WO_SEC_MS_FMT_STRING, BlankString,
         "Bypass Map Time", hr, min, rdaStatusStructPtr->bypass_map_time);


      /*** Modified Julian Date - also print human readable format ***/
      err = RPGCS_julian_to_date( rdaStatusStructPtr->notchwidth_map_date, &yr,
         &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date",mo,day,yr, rdaStatusStructPtr->notchwidth_map_date);


      /*** NWM generation time, min past midnight, convert to hr:min ***/
      nw_map_time_millisec = rdaStatusStructPtr->notchwidth_map_time *
         MONWB_MINS_TO_MILLISECS;
      err = RPGCS_convert_radial_time( nw_map_time_millisec, &hr, &min, &sec,
         &millisec);
      fprintf(stderr, MONWB_UNIX_TIME_WO_SEC_MS_FMT_STRING, BlankString,
         "Notchwidth Map Time", hr, min,
         rdaStatusStructPtr->notchwidth_map_time);


      /*** Transition Power Source Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->tps_status )
      {
         case (unsigned short) TP_NOT_INSTALLED  :
            msg_index = 0;
            break;

         case (unsigned short) TP_OFF  :
            msg_index = 1;
            break;

         case (unsigned short) TP_OK  :
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Trans. Power Src Stat",
         tps_stat[msg_index]);


      /*** RMS Control Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->rms_control_status )
      {
         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_NON_RMS_SYSTEM :
            msg_index = 0;
            break;

         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_RMS_IN_CONTROL :
            msg_index = 1;
            break;

         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_HCI_IN_CONTROL :
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RMS Control Status",
         rms_cntrl_stat[msg_index]);
   }

   /* If there are alarms, interpret and print alarm information */
   for ( alarm_index = 0; alarm_index < MAX_RDA_ALARMS_PER_MESSAGE;
      alarm_index++ )
   {
      if ( rdaStatusStructPtr->alarm_code[alarm_index] != 0 )
      {
         err = ProcessRdaAlarmCodes(rdaStatusStructPtr->alarm_code, alarm_index);
         if ( err < 0 )
         {
            fprintf(stderr,
               "ERROR: Problem processing RDA alarm codes.\n");
         }
      }
   }

   return returnVal;
} /* end ProcessLgcyRdaStatusMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaStatusMsg
*
* Description:
*	Read and check the Open RDA Status Message for ICD conformance.
*	NOTE: The Open RDA Status Message data structure contains within it the
*	RDA/RPG Message Header structure.  Therefore, when the RDA Status Msg
*	structure pointer is cast, it is cast at the beginning of the Message
*	Header instead of after it.
*
* Inputs:
*	short* msgPtr - pointer to msg starting with the 16 byte msg header.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	06-12-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaStatusMsg(short *msgPtr)
{
   int			err			= 0;
   int			msgLen			= 0;
   int			returnVal		= 0;
   ORDA_status_msg_t	*rdaStatusStructPtr	= NULL;
   int			yr, mo, day, hr, min, sec, millisec;
   int			bp_map_time_millisec	= 0;
   int			cl_map_time_millisec	= 0;
   int			msg_index		= 0;
   int			alarm_index		= 0;
   unsigned short	rda_operability		= 0;
   unsigned short	auto_cal_disabled	= 0;
   char*		temp_str		= NULL;
   int			char_count		= 0;
   char*		auto_cal_string		= "Auto Cal Off";
   int			first_in_list		= 0;

   static char*		rda_config		= "OPEN RDA";

   static char*		rda_status[]		= {"Startup",
                                                   "Standby",
                                                   "Restart",
                                                   "Operate",
                                                   "Offline Operate",
                                                   "Unknown"};

   static char*		op_status[]		= {"On-line",
                                                   "MAR",
                                                   "MAM",
                                                   "Commanded Shutdown",
                                                   "Inop.",
                                                   "W'band Disc.",
                                                   "Unknown"};

   static char*		control_status[]	= {"RDA Control",
                                                   "RPG Control",
                                                   "Either",
                                                   "Unknown"};

   static char*		data_trans_enbl[]	= {"None",
                                                   "Refl",
                                                   "Vel",
                                                   "Spec Width",
                                                   "Refl, Vel",
                                                   "Refl, Spec Width",
                                                   "Vel, Spec Width",
                                                   "Refl, Vel, Spec Width",
                                                   "Unknown"};

   static char*		control_auth[]		= {"No Action",
                                                   "Local Control Requested",
                                                   "Remote Control Enabled",
                                                   "Unknown"};

   static char*		op_mode[]		= {"Test",
                                                   "Operational",
                                                   "Maintenance",
                                                   "Unknown"};

   static char*		aux_pwr_state[]		= {"Util Pwr Avail",
                                                   "Generator On",
                                                   "Switch(Manual)",
                                                   "Commanded Switchover",
                                                   "Unknown"};

   static char*		aux_pwr_string		= " Aux Pwr On"; 

   static char*		cmd_ack[]		= {"None",
                                                   "Remote VCP Rec'd",
                                                   "Clutter Bypass Map Rec'd",
                                                   "Clutter Censor Zones Rec'd",
                                                   "Redund Stdby Cmd Accepted",
                                                   "Unknown"};

   static char*		chan_cntrl_stat[]	= {"Controlling",
                                                   "Non-controlling",
                                                   "Unknown"};

   static char*		spot_blank_stat[]	= {"Not Installed",
                                                   "Enabled",
                                                   "Disabled",
                                                   "Unknown"};

   static char*		tps_stat[]		= {"Not Installed",
                                                   "Off",
                                                   "Ok",
                                                   "Unknown"};

   static char*		rms_cntrl_stat[]	= {"Non-RMS System",
                                                   "RMS In Control",
                                                   "HCI In Control",
                                                   "Unknown"};

   static char*		super_res[]		= {"Enabled",
                                                   "Disabled",
                                                   "Unknown"};

   static char*		cmd[]			= {"Enabled",
                                                   "Disabled",
                                                   "Unknown"};

   static char*		avset[]			= {"Enabled",
                                                   "Disabled",
                                                   "Unknown"};

   static char*		perf_check[]		= {"Auto",
                                                   "Pending",
                                                   "Unknown"};


   rdaStatusStructPtr = (ORDA_status_msg_t *) (msgPtr);

   if ( rdaStatusStructPtr == NULL )
   {
      fprintf(stderr,
         "ERROR: Problem casting struct ptr in ProcessOrdaStatusMsg.\n");
      return(-1);
   }

   /* Set global message type flag */
   MessageType = RDA_STATUS_DATA;

   /* Print out message fields */
   msgLen = sprintf(Message, "\n%sORDA Status Message:",
      BlankString);
   PrintMsg(msgLen);

   /* Print the RDA Configuration */
   fprintf( stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Configuration",
      rda_config);

   if ( VerboseLevel >= MONWB_LOW )
   {
      /* Determine and print the RDA Status */
      switch ( rdaStatusStructPtr->rda_status )
      {
         case ( (unsigned short) RS_STARTUP ):
            msg_index = 0;
            break;

         case ( (unsigned short) RS_STANDBY ):
            msg_index = 1;
            break;

         case ( (unsigned short) RS_RESTART ):
            msg_index = 2;
            break;

         case ( (unsigned short) RS_OPERATE ):
            msg_index = 3;
            break;

         case ( (unsigned short) RS_OFFOPER ):
            msg_index = 4;
            break;

         default:
            msg_index = 5;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "RDA Status",
         rda_status[msg_index], rdaStatusStructPtr->rda_status);

      /*** Operability Status - not mutually exclusive ***/
      auto_cal_disabled = (unsigned short)(rdaStatusStructPtr->op_status & 0x0001);
      rda_operability = (unsigned short)(rdaStatusStructPtr->op_status & 0xFFFE);

      /* set up string buffer */
      temp_str = calloc( MONWB_MAX_NUM_CHARS, sizeof(char) );
      if (temp_str != NULL )
      {  
         /* set flag to indicate first one in list */
         first_in_list = MONWB_TRUE;

         /* initialize the character count to zero */
         char_count = 0;

         if ( rda_operability & OS_ONLINE )
         {
            msg_index = 0;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_MAINTENANCE_REQ ) )
         {
            msg_index = 1;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_MAINTENANCE_MAN ) )
         {
            msg_index = 2;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_COMMANDED_SHUTDOWN ) )
         {
            msg_index = 3;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_INOPERABLE ) )
         {
            msg_index = 4;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( ( rda_operability & OS_WIDEBAND_DISCONNECT ) )
         {
            msg_index = 5;
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, op_status[msg_index] );
               char_count += strlen(op_status[msg_index]);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat(temp_str, op_status[msg_index]);
               char_count += strlen(op_status[msg_index]);
            }
         }
         if ( auto_cal_disabled )
         {
            if ( first_in_list == MONWB_TRUE )
            {
               strcat( temp_str, auto_cal_string);
               char_count += strlen(auto_cal_string);
               first_in_list = MONWB_FALSE;
            }
            else
            {
               strcat( temp_str, ", " );
               char_count += 2;
               strcat( temp_str, auto_cal_string );
               char_count += strlen(auto_cal_string);
            }
         }
         if ( char_count <= 0 )
         {
            msg_index = 6;
            strcat( temp_str, op_status[msg_index]);
            temp_str[char_count] = '\0';
         }
         else
         {
            temp_str[char_count] = '\0';
         }
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Operability Status", temp_str, rdaStatusStructPtr->op_status);
      free( temp_str );
      temp_str = NULL;

      /*** Control Status ***/
      switch ( rdaStatusStructPtr->control_status )
      {
         case ( unsigned short ) CS_LOCAL_ONLY:
            msg_index = 0;
            break;

         case ( unsigned short ) CS_RPG_REMOTE:
            msg_index = 1;
            break;

         case ( unsigned short ) CS_EITHER:
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Control Status",
         control_status[msg_index], rdaStatusStructPtr->control_status);


      /*** Data Transmission Enabled ***/
      switch ( rdaStatusStructPtr->data_trans_enbld )
      {
         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_NONE:
            msg_index = 0;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_R:
            msg_index = 1;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_V:
            msg_index = 2;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_W:
            msg_index = 3;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RV:
            msg_index = 4;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RW:
            msg_index = 5;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_VW:
            msg_index = 6;
            break;

         case ( unsigned short ) MONWB_RDASTAT_DATATRANSEN_RVW:
            msg_index = 7;
            break;

         default:
            msg_index = 8;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Data Transmission Enbld", data_trans_enbl[msg_index],
         rdaStatusStructPtr->data_trans_enbld);


      /*** VCP Number ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "VCP Number",
         rdaStatusStructPtr->vcp_num);


      /*** RDA Control Authorization ***/
      switch ( rdaStatusStructPtr->rda_control_auth )
      {
         case ( unsigned short ) CA_NO_ACTION:
            msg_index = 0;
            break;

         case ( unsigned short ) CA_LOCAL_CONTROL_REQUESTED:
            msg_index = 1;
            break;

         case ( unsigned short ) CA_REMOTE_CONTROL_ENABLED:
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "RDA Control Auth",
         control_auth[msg_index], rdaStatusStructPtr->rda_control_auth);


      /*** RDA Build Number ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "RDA Build Num",
         rdaStatusStructPtr->rda_build_num);

      /*** RDA Operational Mode ***/
      switch ( rdaStatusStructPtr->op_mode )
      {
         case ( unsigned short ) OP_MAINTENANCE_MODE:
            msg_index = 0;
            break;

         case ( unsigned short ) OP_OPERATIONAL_MODE:
            msg_index = 1;
            break;

         case ( unsigned short ) OP_OFFLINE_MAINTENANCE_MODE:
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Operational Mode",
         op_mode[msg_index], rdaStatusStructPtr->op_mode);
   }

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      switch ( (rdaStatusStructPtr->aux_pwr_state & 0xFFFE) )
      {
         case (unsigned short) AP_UTILITY_PWR_AVAIL :
            msg_index = 0;
            break;

         case (unsigned short) AP_GENERATOR_ON :
            msg_index = 1;
            break;

         case (unsigned short) AP_TRANS_SWITCH_MAN :
            msg_index = 2;
            break;

         case (unsigned short) AP_COMMAND_SWITCHOVER :
            msg_index = 3;
            break;

         default :
            msg_index = 4;
            break;
      }


      /* Determine if power has switched to auxiliary */
      if ( rdaStatusStructPtr->aux_pwr_state & 0x0001 )
      {
         temp_str = calloc ( ( strlen( aux_pwr_state[msg_index] ) +
            strlen(aux_pwr_string) + 2 ), sizeof(char) );
         if ( temp_str != NULL )
         {
            strcpy( temp_str, aux_pwr_state[msg_index] );
            strcat( temp_str, aux_pwr_string );
         }
        
         fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
            "Auxiliary Power State", temp_str, rdaStatusStructPtr->aux_pwr_state);
         free ( temp_str );
         temp_str = NULL;
      }
      else
      {
         fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
            "Auxiliary Power State", aux_pwr_state[msg_index],
            rdaStatusStructPtr->aux_pwr_state);
      }


      /*** Average Transmitter Power ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Avg Xmtr Power (W)",
         rdaStatusStructPtr->ave_trans_pwr);


      /*** Reflectivity Calibration Correction ***/
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Refl Calib Corr (HC). (dB)",
         (rdaStatusStructPtr->ref_calib_corr * 0.01));

      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Refl Calib Corr (VC). (dB)",
         (rdaStatusStructPtr->vc_ref_calib_corr * 0.01));

      /*** RDA Alarm Summary - not mutually exclusive ***/
      if ( rdaStatusStructPtr->rda_alarm != 0 )
      {
         /* set up string buffer */
         temp_str = calloc( MONWB_MAX_NUM_CHARS_IN_ALARM_STR, sizeof(char) );
         if ( temp_str != NULL )
         {
            /* set flag to indicate whether or not first in list */
            first_in_list = 1; /* true */

            /* initialize the character count to 0 */
            char_count = 0;

            if ( rdaStatusStructPtr->rda_alarm & 0x0002 )
            {
               /* Add Tower-Utilities to alarm list */        
               if ( first_in_list )
               {
                  strcat( temp_str, "Tow-Util");
                  char_count = strlen("Tow-Util") + 1;
                  first_in_list = 0;
               }
               else
               {
                  strcat( temp_str, ",Tow-Util");
                  char_count += strlen(",Tow-Util") + 1;
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0004 )
            {
               /* Add Pedestal to alarm list */        
               if ( (char_count + strlen(",Ped") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Ped");
                     char_count = strlen("Ped") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Ped");
                     char_count += strlen(",Ped") + 1;
                  }
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0008 )
            {
               /* Add Transmitter to alarm list */        
               if ( (char_count + strlen(",Xmtr") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Xmtr");
                     char_count = strlen("Xmtr") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Xmtr");
                     char_count += strlen(",Xmtr") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0010 )
            {
               /* Add Receiver to alarm list */        
               if ( (char_count + strlen(",Recvr") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Rcvr");
                     char_count = strlen("Rcvr") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Rcvr");
                     char_count += strlen(",Rcvr") + 1;
                  }
               }
            }

            if ( rdaStatusStructPtr->rda_alarm & 0x0020 )
            {
               /* Add RDA Control to alarm list */        
               if ( (char_count + strlen(",RDA Cntrl") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "RDA Cntrl");
                     char_count = strlen("RDA Cntrl") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",RDA Cntrl");
                     char_count += strlen(",RDA Cntrl") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0040 )
            {
               /* Add Communication to alarm list */        
               if ( (char_count + strlen(",Comm") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Comm");
                     char_count = strlen("Comm") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Comm");
                     char_count += strlen(",Comm") + 1;
                  }
               }
            }
   
            if ( rdaStatusStructPtr->rda_alarm & 0x0080 )
            {
               /* Add Signal Processor to alarm list */        
               if ( (char_count + strlen(",Sig Proc") + 1) <= 
                  MONWB_MAX_NUM_CHARS_IN_ALARM_STR)
               {
                  if ( first_in_list )
                  {
                     strcat( temp_str, "Sig Proc");
                     char_count = strlen("Sig Proc") + 1;
                     first_in_list = 0;
                  }
                  else
                  {
                     strcat( temp_str, ",Sig Proc");
                     char_count += strlen(",Sig Proc") + 1;
                  }
               }
            }

            fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
               "RDA Alarm Summary", temp_str, rdaStatusStructPtr->rda_alarm);
            free( temp_str );
            temp_str = NULL;
         }
      }
      else
      {
         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA Alarm Summary",
            "No Alarms");
      }

   }
   if ( VerboseLevel >= MONWB_LOW )
   {
      /*** Command Acknowledgement - mutually exclusive ***/
      switch ( rdaStatusStructPtr->command_status )
      {
         case (unsigned short) RS_NO_ACKNOWLEDGEMENT :
            msg_index = 0;
            break;

         case (unsigned short) RS_REMOTE_VCP_RECEIVED :
            msg_index = 1;
            break;

         case (unsigned short) RS_CLUTTER_BYPASS_MAP_RECEIVED :
            msg_index = 2;
            break;

         case (unsigned short) RS_CLUTTER_CENSOR_ZONES_RECEIVED :
            msg_index = 3;
            break;

         case (unsigned short) RS_REDUND_CHNL_STBY_CMD_ACCEPTED :
            msg_index = 4;
            break;

         default :
            msg_index = 5;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Command Acknowledgment", cmd_ack[msg_index],
         rdaStatusStructPtr->command_status);

   }
   if ( VerboseLevel >= MONWB_MODERATE )
   {

      /*** Channel Control Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->channel_status )
      {
         case ( unsigned short ) RDA_IS_CONTROLLING :
            msg_index = 0;
            break;

         case ( unsigned short ) RDA_IS_NON_CONTROLLING :
            msg_index = 1;
            break;
 
         default :
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Channel Control Status", chan_cntrl_stat[msg_index],
         rdaStatusStructPtr->channel_status);


      /*** Spot Blanking Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->spot_blanking_status )
      {
         case ( unsigned short ) SB_NOT_INSTALLED :
            msg_index = 0;
            break;

         case ( unsigned short ) SB_ENABLED :
            msg_index = 1;
            break;
 
         case ( unsigned short ) SB_DISABLED :
            msg_index = 2;
            break;
 
         default :
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Spot Blanking Status", spot_blank_stat[msg_index],
         rdaStatusStructPtr->spot_blanking_status);


      /*** Modified Julian Date - also print human readable format ***/
      err = RPGCS_julian_to_date( rdaStatusStructPtr->bypass_map_date, &yr,
         &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date", mo,day,yr, rdaStatusStructPtr->bypass_map_date);


      /*** BPM generation time, min past midnight, convert to hr:min ***/
      bp_map_time_millisec = rdaStatusStructPtr->bypass_map_time *
         MONWB_MINS_TO_MILLISECS;
      err = RPGCS_convert_radial_time( bp_map_time_millisec, &hr, &min, &sec,
         &millisec);
      fprintf(stderr, MONWB_UNIX_TIME_WO_SEC_MS_FMT_STRING, BlankString,
         "Bypass Map Time", hr, min, rdaStatusStructPtr->bypass_map_time);


      /*** Modified Julian Date - also print human readable format ***/
      err = RPGCS_julian_to_date( rdaStatusStructPtr->clutter_map_date, &yr,
         &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date", mo,day,yr, rdaStatusStructPtr->clutter_map_date);


      /*** CLM generation time, min past midnight, convert to hr:min ***/
      cl_map_time_millisec = rdaStatusStructPtr->clutter_map_time *
         MONWB_MINS_TO_MILLISECS;
      err = RPGCS_convert_radial_time( cl_map_time_millisec, &hr, &min, &sec,
         &millisec);
      fprintf(stderr, MONWB_UNIX_TIME_WO_SEC_MS_FMT_STRING, BlankString,
         "Clutter Filter Map Time", hr, min,
         rdaStatusStructPtr->clutter_map_time);


      /*** Transition Power Source Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->tps_status )
      {
         case (unsigned short) TP_NOT_INSTALLED  :
            msg_index = 0;
            break;

         case (unsigned short) TP_OFF  :
            msg_index = 1;
            break;

         case (unsigned short) TP_OK  :
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Trans. Power Src Stat", tps_stat[msg_index],
         rdaStatusStructPtr->tps_status);


      /*** RMS Control Status - mutually exclusive ***/
      switch ( rdaStatusStructPtr->rms_control_status )
      {
         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_NON_RMS_SYSTEM :
            msg_index = 0;
            break;

         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_RMS_IN_CONTROL :
            msg_index = 1;
            break;

         case (unsigned short) MONWB_RDASTAT_RMS_CNTRL_HCI_IN_CONTROL :
            msg_index = 2;
            break;

         default :
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "RMS Control Status",
         rms_cntrl_stat[msg_index], rdaStatusStructPtr->rms_control_status);

   }
   if ( VerboseLevel >= MONWB_LOW )
   {
      /*** Super Resolution Status ***/
      switch ( rdaStatusStructPtr->super_res )
      {
         case (unsigned short) SR_ENABLED :
            msg_index = 0;
            break;

         case (unsigned short) SR_DISABLED :
            msg_index = 1;
            break;

         default :
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Super Resolution Status",
         super_res[msg_index], rdaStatusStructPtr->super_res);

      /*** Clutter Mitigation Decision Status ***/
      switch ( rdaStatusStructPtr->cmd & 0x1 )
      {
         case (unsigned short) CMD_ENABLED :
            msg_index = 0;
            break;

         case (unsigned short) CMD_DISABLED :
            msg_index = 1;
            break;

         default :
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Clutter Mitigation Status",
         cmd[msg_index], rdaStatusStructPtr->cmd);

      /*** AVSET Status ***/
      switch ( rdaStatusStructPtr->avset )
      {
         case (unsigned short) AVSET_ENABLED :
            msg_index = 0;
            break;

         case (unsigned short) AVSET_DISABLED :
            msg_index = 1;
            break;

         default :
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "AVSET                    ",
         avset[msg_index], rdaStatusStructPtr->avset);

      /*** Performance Check Status ***/
      switch ( rdaStatusStructPtr->perf_check_status )
      {
         case (unsigned short) PC_AUTO :
            msg_index = 0;
            break;

         case (unsigned short) PC_PENDING :
            msg_index = 1;
            break;

         default :
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Perf Check Status        ",
         perf_check[msg_index], rdaStatusStructPtr->perf_check_status);
   }

   /* If there are alarms, interpret and print alarm information */
   for ( alarm_index = 0; alarm_index < MAX_RDA_ALARMS_PER_MESSAGE; 
      alarm_index++ )
   {
      if ( rdaStatusStructPtr->alarm_code[alarm_index] != 0 )
      {
         err = ProcessRdaAlarmCodes(rdaStatusStructPtr->alarm_code, alarm_index);
         if ( err < 0 )
         {
            fprintf(stderr,
               "ERROR: Problem processing RDA alarm codes.\n");
         }
      }
   }

   return returnVal;
} /* end ProcessOrdaStatusMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyBasedataMsg
*
* Description:
*	Read and check the Legacy Basedata Message (Dig Radar Msg) for ICD
*	conformance.
*	NOTE: The Basedata Message data structure contains within it the
*	RDA/RPG Message Header structure.  Therefore, when the Basedata Msg
*	structure pointer is cast, it is cast at the beginning of the Message
*	Header instead of after it.
*
* Inputs:
*	short* msgPtr - Pointer to the message starting at the 16 byte msg hdr.
*	
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS, Apr 2003
*******************************************************************************/
static int ProcessLgcyBasedataMsg(short *msgPtr)
{
   int 			err			= 0;
   int 			msgLen			= 0;
   int			returnVal		= MONWB_SUCCESS;
   RDA_basedata_header	*basedataMsgPtr		= NULL;
   unsigned short	angle_bam		= 0;
   double		angle_deg		= 0.0;
   int			yr, mo, day, hr, min, sec, millisec;
   int			msg_index		= 0;

   static char*		radial_status[]		= {"Start of New Elevation Scan",
                                                   "Intermediate Radial Data",
                                                   "End of Elevation Scan",
                                                   "Beginning of Volume Scan",
                                                   "End of Volume Scan",
                                                   "Start of Last Elevation Scan",
                                                   "Unknown"};
   static char*		velocity_res[]		= {"0.5",
                                                   "1.0",
                                                   "Unknown"};



   if ( ( VerboseLevel >= MONWB_HIGH ) &&
        ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END) ) )
   {
      msgLen = 
         sprintf(Message,
         "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
         ResponseLbMsgId, MsgReadLen);
      PrintMsg(msgLen);
      HdrDisplayFlag = MONWB_IS_DISPLAYED;  /* Indicates msg title has been displayed */
   }

   /* Set global message type flag */
   MessageType = DIGITAL_RADAR_DATA;

   /* Cast basedata to the proper data structure */
   basedataMsgPtr = (RDA_basedata_header *)(msgPtr);

   if ( basedataMsgPtr == NULL )
   {
      fprintf(stderr,
         "ERROR: Problem processing basedata msg.\n");
      return(MONWB_FAIL);
   }


   /* Because of the massive number of basedata msgs produced, we're only going 
      to range check the msgs from the beginning/end of elevation cuts and volume
      scans, as well as those messages where ICD range errors are detected.  */

   if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sEND OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF ELEVATION SCAN %d\n",
         BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_END )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len: %d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sEND OF ELEVATION SCAN %d\n", 
        BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF LAST ELEVATION SCAN %d\n",
         BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }


   /* Perform validity checks on the base data if it's the start/end
      of an elevation of volume scan */

   if ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END) )
   {
      msgLen = sprintf(Message, "\n%sBasedata Message:",
         BlankString);
      PrintMsg(msgLen);

      /* More important fields */
      if ( VerboseLevel >= MONWB_LOW )
      {
         if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_ELEV )
         {
            msg_index = 0;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_INTERMED )
         {
            msg_index = 1;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_END_ELEV )
         {
            msg_index = 2;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_VOL )
         {
            msg_index = 3;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_END_VOL )
         {
            msg_index = 4;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_LAST_ELEV )
         {
            msg_index = 5;
         }
         else     
         {
            msg_index = 6;
         }

         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Radial Status",
            radial_status[msg_index]);

         angle_bam = basedataMsgPtr->elevation;

         bam_to_deg( &angle_bam, &angle_deg );

         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Elevation Angle (deg)",
            angle_deg);

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Elevation Number",
            basedataMsgPtr->elev_num);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "VCP Number",
            basedataMsgPtr->vcp_num);
      }

      /* Less important fields */
      if ( VerboseLevel >= MONWB_MODERATE )
      {
         /*** Time in ms past midnight, converted to readable format ***/
         err = RPGCS_convert_radial_time( basedataMsgPtr->time,
                                          &hr, &min, &sec, &millisec );
         fprintf(stderr, MONWB_UNIX_TIME_WITH_MS_FMT_STRING, BlankString,
            "Collection Time", hr, min, sec,millisec,(int)basedataMsgPtr->time);

         /*** Modified Julian Date - also print human readable format ***/
         err = RPGCS_julian_to_date( basedataMsgPtr->date, &yr, &mo, &day );
         fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
            "Mod. Julian Date", mo, day, yr, basedataMsgPtr->date);

         /*** Unambiguous Range ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
               "Unambiguous Range (km)", 
               (float)(basedataMsgPtr->unamb_range * 0.1));

         /*** Azimuth angle ***/
         angle_bam = basedataMsgPtr->azimuth;
         bam_to_deg( &angle_bam, &angle_deg);
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Azimuth Angle (deg)",
            angle_deg);

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Azimuth Number",
            basedataMsgPtr->azi_num);

         /*** Surveillance Range ***/
         /* first retrieve scale and offset, then apply and print value */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Surveil. Range (km)",
               (float)(basedataMsgPtr->surv_range * 0.001));

         /*** Doppler Range ***/
         /* first retrieve scale and offset, then apply and print value */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Doppler Range (km)",
               (float)(basedataMsgPtr->dop_range * 0.001));

         /*** Surveillance Bin Size ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Surveil. Bin Size (km)",
               (float)(basedataMsgPtr->surv_bin_size * 0.001));

         /*** Doppler Bin Size ***/
         /* first retrieve scale and offset, then apply and print value */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Doppler Bin Size (km)",
               (float)(basedataMsgPtr->dop_bin_size * 0.001));

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "# Surveil. Bins",
            basedataMsgPtr->n_surv_bins);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "# Doppler Bins",
            basedataMsgPtr->n_dop_bins);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Cut Sector Number",
            basedataMsgPtr->sector_num);
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
            "Calib. Constant (dB)",
            basedataMsgPtr->calib_const);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Refl Byte Offset",
            basedataMsgPtr->ref_ptr);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Velocity Byte Offset",
            basedataMsgPtr->vel_ptr);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Spect Width Byte Offset",
            basedataMsgPtr->spw_ptr);

         /* Doppler velocity resolution */
         switch ( basedataMsgPtr->vel_resolution )
         {
            case (short) POINT_FIVE_METERS_PER_SEC :
               msg_index = 0;
               break;
         
            case (short) ONE_METER_PER_SEC :
               msg_index = 1;
               break;
            
            default : 
               msg_index = 2;
               break;
         }

         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Velocity Res. (m/s)",
            velocity_res[msg_index]);

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Refl Playback Ptr",
            basedataMsgPtr->ref_data_playback);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Velocity Playback Ptr",
            basedataMsgPtr->dop_data_playback);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Spect Width Playback Ptr",
            basedataMsgPtr->sw_data_playback);


         /*** Nyquist Velocity ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Nyquist Velocity (m/s)",
               (float)(basedataMsgPtr->nyquist_vel * 0.01));

         /*** Attmos Attenuation ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Atmos. Atten. (dB/km)",
               (float)(basedataMsgPtr->atmos_atten * 0.001));

         /*** Threshold Parameter ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Threshold Parameter (dB)",
               (float)(basedataMsgPtr->threshold_param * 0.1));

         /* Print spot blank flag in hex because it's not mutually exclusive */
         fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Spot Blanking Flag",
            basedataMsgPtr->spot_blank_flag);
         fprintf(stderr, "\n");
      }

   }

   return returnVal;
} /* end ProcessLgcyBasedataMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaBasedataMsg
*
* Description:
*	Read and check the Basedata Message (Dig Radar Msg) for ICD conformance.
*	NOTE: The Basedata Message data structure contains within it the
*	RDA/RPG Message Header structure.  Therefore, when the Basedata Msg
*	structure pointer is cast, it is cast at the beginning of the Message
*	Header instead of after it.
*
* Inputs:
*	short* msgPtr - pointer to msg starting with the 16 byte msg header.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	06-12-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaBasedataMsg(short *msgPtr)
{
   int 			err			= 0;
   int 			msgLen			= 0;
   int			returnVal		= 0;
   ORDA_basedata_header	*basedataMsgPtr		= NULL;
   unsigned short	angle_bam		= 0;
   double		angle_deg		= 0.0;
   int			yr, mo, day, hr, min, sec, millisec;
   int			msg_index		= 0;

   static char*		radial_status[]		= {"Start of New Elevation Scan",
                                                   "Intermediate Radial Data",
                                                   "End of Elevation Scan",
                                                   "Beginning of Volume Scan",
                                                   "End of Volume Scan",
                                                   "Start of Last Elevation Scan",
                                                   "Unknown"};
   static char*		velocity_res[]		= {"0.5",
                                                   "1.0",
                                                   "Unknown"};



   if ( ( VerboseLevel >= MONWB_HIGH ) &&
        ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END)     ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) ) )
   {
      msgLen = 
         sprintf(Message,
         "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
         ResponseLbMsgId, MsgReadLen);
      PrintMsg(msgLen);
      HdrDisplayFlag = MONWB_IS_DISPLAYED;  /* Indicates msg title has been displayed */
   }

   /* Cast basedata to the proper data structure */
   basedataMsgPtr = (ORDA_basedata_header *)(msgPtr);

   if ( basedataMsgPtr == NULL )
   {
      fprintf(stderr,
         "ERROR: Problem processing basedata msg.\n");
      return(-1);
   }


   /* Because of the massive number of basedata msgs produced, we're only going 
      to range check the msgs from the beginning/end of elevation cuts and volume
      scans, as well as those messages where ICD range errors are detected.  */

   if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sEND OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len:%d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF ELEVATION SCAN %d\n",
         BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_END )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len: %d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sEND OF ELEVATION SCAN %d\n", 
        BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }

   if ( RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START )
   {
      if ( HdrDisplayFlag == MONWB_IS_NOT_DISPLAYED )
      {
         msgLen = 
            sprintf(Message,
            "<--- DIGITAL RADAR DATA MSG (Msg ID:%d Type:1 Len: %d)\n",
            ResponseLbMsgId, MsgReadLen);
         PrintMsg(msgLen);
         HdrDisplayFlag = MONWB_IS_DISPLAYED;
      }
       
      msgLen = sprintf(Message, "\n%sBEGINNING OF LAST ELEVATION SCAN %d\n", 
        BlankString, basedataMsgPtr->elev_num);
      PrintMsg(msgLen);
   }

   /* Perform validity checks on the base data if it's the start/end
      of an elevation of volume scan */

   if ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END)     ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) )
   {
      msgLen = sprintf(Message, "\n%sBasedata Message:",
         BlankString);
      PrintMsg(msgLen);

      if ( VerboseLevel >= MONWB_LOW )
      {
         if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_ELEV )
         {
            msg_index = 0;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_INTERMED )
         {
            msg_index = 1;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_END_ELEV )
         {
            msg_index = 2;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_VOL )
         {
            msg_index = 3;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_END_VOL )
         {
            msg_index = 4;
         }
         else if ( basedataMsgPtr->status == (short) MONWB_BD_RADIAL_STAT_BEG_LAST_ELEV )
         {
            msg_index = 5;
         }
         else
         {
            msg_index = 6;
         }

         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Radial Status",
            radial_status[msg_index]);

         angle_bam = basedataMsgPtr->elevation;

         bam_to_deg( &angle_bam, &angle_deg );

         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Elevation Angle (deg)",
            angle_deg);

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Elevation Number",
            basedataMsgPtr->elev_num);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "VCP Number",
            basedataMsgPtr->vcp_num);
      }

      /* Print out message fields if verbose is MONWB_HIGH or greater */
      if ( VerboseLevel >= MONWB_MODERATE )
      {
         /*** Time in ms past midnight, converted to readable format ***/
         err = RPGCS_convert_radial_time( basedataMsgPtr->time,
                                          &hr, &min, &sec, &millisec );
         fprintf(stderr, MONWB_UNIX_TIME_WITH_MS_FMT_STRING, BlankString,
            "Collection Time", hr, min, sec, millisec,
            (int)basedataMsgPtr->time);

         /*** Modified Julian Date - also print human readable format ***/
         err = RPGCS_julian_to_date( basedataMsgPtr->date, &yr, &mo, &day );
         fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
            "Mod. Julian Date", mo, day, yr, basedataMsgPtr->date);

         /*** Unambiguous Range ***/
         /* first retrieve scale and offset, then apply and print value */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Unambiguous Range (km)",
               (float)(basedataMsgPtr->unamb_range * 0.001));

         /* Convert azimuth in BAMS to degrees */
         angle_bam = basedataMsgPtr->azimuth;

         bam_to_deg( &angle_bam, &angle_deg);

         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Azimuth Angle (deg)",
            angle_deg);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Azimuth Number",
            basedataMsgPtr->azi_num);


         /*** Surveillance Range ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Surveil. Range (km)",
               (float)(basedataMsgPtr->surv_range * 0.001));

         /*** Doppler Range ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Doppler Range (km)",
               (float)(basedataMsgPtr->dop_range * 0.001));

         /*** Surveillance Bin Size ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Surveil. Bin Size (km)",
               (float)(basedataMsgPtr->surv_bin_size * 0.001));
         /*** Doppler Bin Size ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Doppler Bin Size (km)",
               (float)(basedataMsgPtr->dop_bin_size * 0.001));

         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "# Surveil. Bins",
            basedataMsgPtr->n_surv_bins);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "# Doppler Bins",
            basedataMsgPtr->n_dop_bins);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Cut Sector Number",
            basedataMsgPtr->sector_num);
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
            "Calib. Constant (dB)",
            basedataMsgPtr->calib_const);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Refl Byte Offset",
            basedataMsgPtr->ref_ptr);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Velocity Byte Offset",
            basedataMsgPtr->vel_ptr);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Spect Width Byte Offset",
            basedataMsgPtr->spw_ptr);

         /* Doppler velocity resolution */
         switch ( basedataMsgPtr->vel_resolution )
         {
            case (short) POINT_FIVE_METERS_PER_SEC :
               msg_index = 0;
               break;
         
            case (short) ONE_METER_PER_SEC :
               msg_index = 1;
               break;
            
            default : 
               msg_index = 2;
               break;
         }

         fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Velocity Res. (m/s)",
            velocity_res[msg_index]);


         /*** Nyquist Velocity ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Nyquist Velocity (m/s)",
               (float)(basedataMsgPtr->nyquist_vel * 0.01));

         /*** Attmos Attenuation ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Atmos. Atten. (dB/km)",
               (float)(basedataMsgPtr->atmos_atten * 0.001));

         /*** Threshold Parameter ***/
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Threshold Parameter (dB)",
               (float)(basedataMsgPtr->threshold_param * 0.1));

         /* Print spot blank flag in hex because it's not mutually exclusive */
         fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Spot Blanking Flag",
            basedataMsgPtr->spot_blank_flag);

         fprintf(stderr, "\n");
      }

   }

   return returnVal;
} /* end ProcessOrdaBasedataMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyPerfMaintMsg
*
* Description:
* 	Read the Legacy Performance Maintenance Message (message type 3, see
*	table 5 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	msgPtr - pointer to msg starting with the 16 byte msg hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-1-2003, R. Solomon.  Created.
*	6-12-2003, R. Solomon.  Changed name to ProcessLgcyPerfMaintMsg to 
*				continue to support handling the legacy
*				format in light of the new ORDA system.
*******************************************************************************/
static int ProcessLgcyPerfMaintMsg(short *msgPtr)
{
   int			returnVal		= 0;

   /* Set global message type flag */
   MessageType = PERFORMANCE_MAINTENANCE_DATA;

   return returnVal;
} /* end ProcessLgcyPerfMaintMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaPerfMaintMsg
*
* Description:
* 	Read the ORDA Performance Maintenance Message (message type 3, see
*	table 5 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short*	msgPtr	= Pointer to the message starting with the 16 byte hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*******************************************************************************/
static int ProcessOrdaPerfMaintMsg(short *msgPtr)
{
   int			returnVal		= 0;

   /* Set global message type flag */
   MessageType = PERFORMANCE_MAINTENANCE_DATA;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      orda_pmd_t *pmd = (orda_pmd_t *) msgPtr;
      int year, mon, day, hr, min, sec;

      unix_time( &pmd->pmd.perf_check_time, &year, &mon, &day,
                 &hr, &min, &sec ); 
      fprintf(stderr, MONWB_UNIX_DATE_TIME_FMT_STRING, BlankString, 
              "Perf Check Time", mon, day, year, hr, min, sec);
   
   }

   return returnVal;
} /* end ProcessOrdaPerfMaintMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyRdaControlCmdsMsg
*
* Description:
* 	Read the Legacy RDA Control Commands Message (message type 6, see table
*	10 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short* msgPtr - pointer to msg starting after the 16 byte msg hdr.
*	
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-15-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessLgcyRdaControlCmdsMsg(short *msgPtr)
{
   int 				msgLen			= 0;
   int				returnVal		= MONWB_SUCCESS;
   RDA_control_commands_t*	rdaControlPtr		= NULL;
   int				msg_index		= 0;
   char*			rda_control_state[]	= {"No Change",
                                                           "Standby",
                                                           "Offline Operate",
                                                           "Operate",
                                                           "Restart",
                                                           "Playback",
                                                           "Unknown"};

   static char*			data_trans_enbl[]	= {"No Change",
                                                           "None",
                                                           "Refl",
                                                           "Vel",
                                                           "Spec Width",
                                                           "Refl, Vel",
                                                           "Refl, Spec Width",
                                                           "Vel, Spec Width",
                                                           "Refl, Vel, Spec Width",
                                                           "Unknown"};


   /* Set global message type flag */
   MessageType = RDA_CONTROL_COMMANDS;

   /* Cast pointer to RDA Control structure */
   /* The RDA Control data structure doesn't contain the message header
      structure within it. Expecting that the msgPtr points to the RDA
      Control Message itself, not the header. */
   rdaControlPtr = (RDA_control_commands_t *) msgPtr;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Print out message fields */
      msgLen = sprintf(Message, "\n%sRDA Control Command Message:",
         BlankString);
      PrintMsg(msgLen);
   }

   if ( VerboseLevel >= MONWB_LOW )
   {
      switch ( rdaControlPtr->state )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_STANDBY :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_OFFOPER :
            msg_index = 2;
            break;

         case ( unsigned short ) RCOM_OPERATE :
            msg_index = 3;
            break;

         case ( unsigned short ) RCOM_RESTART :
            msg_index = 4;
            break;

         case ( unsigned short ) RCOM_PLAYBACK :
            msg_index = 5;
            break;

         default:
            msg_index = 6;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "RDA State Command",
         rda_control_state[msg_index]);


      /*** Data Transmission Enabled ***/
      switch ( rdaControlPtr->data_enbl )
      {
         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_NOCHANGE:
            msg_index = 0;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_NONE:
            msg_index = 1;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_R:
            msg_index = 2;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_V:
            msg_index = 3;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_W:
            msg_index = 4;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RV:
            msg_index = 5;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RW:
            msg_index = 6;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_VW:
            msg_index = 7;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RVW:
            msg_index = 8;
            break;

         default:
            msg_index = 9;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Data Transmission Enbld",
         data_trans_enbl[msg_index]);
   }

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Aux. Power Gen. Control",
         rdaControlPtr->aux_pwr_gen);
      fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Control Authorization",
         rdaControlPtr->authorization);
      fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Restart VCP/Elev",
         rdaControlPtr->restart_elev);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Select VCP",
         rdaControlPtr->select_vcp);

      /* auto calib override - first retrieve scale and offset, then apply and
         print value */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
            "Auto. Cal. Over. (dB)", (float)(rdaControlPtr->auto_calib * 0.25));

      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "ISU Control", 
         rdaControlPtr->interference);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Select Operating Mode",
         rdaControlPtr->operate_mode);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Channel Control Command",
         rdaControlPtr->channel);
      fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Archive II Control",
         rdaControlPtr->archive_II);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Num of Vols to Archive",
         rdaControlPtr->archive_num);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Playback Start Date",
         rdaControlPtr->start_date);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Playback Stop Date",
         rdaControlPtr->stop_date);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Playback Start Time",
         rdaControlPtr->start_time);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Playback Stop Time",
         rdaControlPtr->stop_time);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Spot Blanking",
         rdaControlPtr->spot_blanking);
      fprintf(stderr, "\n");
   }

   return returnVal;
} /* end ProcessLgcyRdaControlCmdsMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaControlCmdsMsg
*
* Description:
* 	Read the Open RDA Control Commands Message (message type 6, see table
*	10 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short* msgPtr - pointer to message starting after the 16 byte msg hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	6-12-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaControlCmdsMsg(short *msgPtr)
{
   int 				msgLen			= 0;
   int				elev			= 0;
   int				returnVal		= 0;
   ORDA_control_commands_t*	rdaControlPtr		= NULL;
   int				msg_index		= 0;
   char*			rda_control_state[]	= {"No Change",
                                                           "Standby",
                                                           "Offline Operate",
                                                           "Operate",
                                                           "Restart",
                                                           "Unknown"};

   static char*			data_trans_enbl[]	= {"No Change",
                                                           "None",
                                                           "Refl",
                                                           "Vel",
                                                           "Spec Width",
                                                           "Refl, Vel",
                                                           "Refl, Spec Width",
                                                           "Vel, Spec Width",
                                                           "Refl, Vel, Spec Width",
                                                           "Unknown"};

   static char*			aux_power[]		= {"No Change",
							   "Switch to Aux Pwr",
							   "Switch to Utly Pwr",
							   "Unknown"};

   static char*			cntrl_auth[]		= {"No Change",
							   "Cntrl Command Clear",
							   "Local Cntrl Enabled",
							   "Remote Cntrl Accepted",
							   "Remote Cntrl Requested",	
							   "Unknown"};

   static char*			restart_vcp[]		= {"None",
							   "Restart VCP",
							   "Restart Cut",
							   "Unknown"};

   static char*			select_vcp[]		= {"Use Remote Pattern",
							   "Pattern Number",
							   "No Change",
							   "Unknown"};

   static char* 		super_res[]		= {"No Change",
							   "Enable",
							   "Disable",
							   "Unknown"};

   static char*			cmd_cntrl[]		= {"No Change",
							   "Enable",
							   "Disable",
							   "Unknown"};

   static char*			avset_cntrl[]		= {"No Change",
							   "Enable",
							   "Disable",
							   "Unknown" };

   static char*			perf_check[]		= {"No Change",
							   "Force Perform",
							   "Unknown" };

   /* Set global message type flag */
   MessageType = RDA_CONTROL_COMMANDS;

   /* Cast pointer to RDA Control structure */
   /* The RDA Control data structure doesn't contain the message header
      structure within it. Expecting that the msgPtr points to the RDA
      Control Message itself, not the header. */
   rdaControlPtr = (ORDA_control_commands_t *) msgPtr;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Print out message fields */
      msgLen = sprintf(Message, "\n%sORDA Control Command Message:",
         BlankString);
      PrintMsg(msgLen);
   }

   if ( VerboseLevel >= MONWB_LOW )
   {
      switch ( rdaControlPtr->state )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_STANDBY :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_OFFOPER :
            msg_index = 2;
            break;

         case ( unsigned short ) RCOM_OPERATE :
            msg_index = 3;
            break;

         case ( unsigned short ) RCOM_RESTART :
            msg_index = 4;
            break;

         case ( unsigned short ) RCOM_PLAYBACK :
            msg_index = 5;
            break;

         default:
            msg_index = 6;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "RDA State Command",
         rda_control_state[msg_index], rdaControlPtr->state);


      /*** Data Transmission Enabled ***/
      switch ( rdaControlPtr->data_enbl )
      {
         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_NOCHANGE:
            msg_index = 0;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_NONE:
            msg_index = 1;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_R:
            msg_index = 2;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_V:
            msg_index = 3;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_W:
            msg_index = 4;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RV:
            msg_index = 5;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RW:
            msg_index = 6;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_VW:
            msg_index = 7;
            break;

         case ( unsigned short ) MONWB_RDACNTRL_DATATRANSEN_RVW:
            msg_index = 8;
            break;

         default:
            msg_index = 9;
            break;
      }

      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Data Transmission Enbld", data_trans_enbl[msg_index],
         rdaControlPtr->data_enbl);

      switch ( rdaControlPtr->aux_pwr_gen )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_AUXGEN :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_UTIL :
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Aux Pwr Gen Ctrl",
         aux_power[msg_index], rdaControlPtr->aux_pwr_gen);

      switch ( rdaControlPtr->authorization )
      {
         case 0 :
            msg_index = 0;
            break;

         case 2 :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_ENALOCAL :
            msg_index = 2;
            break;

         case ( unsigned short ) RCOM_ACCREMOTE :
            msg_index = 3;
            break;

         case ( unsigned short ) RCOM_REQREMOTE :
            msg_index = 4;
            break;

         default:
            msg_index = 5;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Ctrl Auth",
         cntrl_auth[msg_index], rdaControlPtr->authorization);

      elev = rdaControlPtr->restart_elev & 0x000f;

      if( rdaControlPtr->restart_elev == 0 )
         msg_index = 0;

      else if( ((rdaControlPtr->restart_elev & 0xf000) == RCOM_RESTART_VCP)
                                       &&
                                  (elev == 0) )
            msg_index = 1;

      else if( ((rdaControlPtr->restart_elev & 0xf000) == RCOM_RESTART_VCP)
                                       &&
                                  (elev != 0) )
            msg_index = 2;

      else
            msg_index = 3;

      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Restart VCP/Elev",
         restart_vcp[msg_index], rdaControlPtr->restart_elev);

      if( rdaControlPtr->select_vcp == 0 )
         msg_index = 0;

      else if( rdaControlPtr->select_vcp < 32767 )
         msg_index = 1;

      else if( rdaControlPtr->select_vcp == 32767 )
         msg_index = 2;

      else
         msg_index = 3;

      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Select VCP",
         select_vcp[msg_index], rdaControlPtr->select_vcp);

      switch ( rdaControlPtr->super_res )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_ENABLE_SR :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_DISABLE_SR :
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Super Res Cntrl",
         super_res[msg_index], rdaControlPtr->super_res);

      switch ( rdaControlPtr->cmd )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_ENABLE_CMD :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_DISABLE_CMD :
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "CMD Cntrl",
         cmd_cntrl[msg_index], rdaControlPtr->cmd);

      switch ( rdaControlPtr->avset )
      {
         case 0 :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_ENABLE_AVSET :
            msg_index = 1;
            break;

         case ( unsigned short ) RCOM_DISABLE_AVSET :
            msg_index = 2;
            break;

         default:
            msg_index = 3;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Avset Cntrl",
         avset_cntrl[msg_index], rdaControlPtr->avset);

      switch ( rdaControlPtr->perf_check )
      {
         case ( unsigned short) RCOM_PERF_CHECK_NC :
            msg_index = 0;
            break;

         case ( unsigned short ) RCOM_PERF_CHECK_PPC :
            msg_index = 1;
            break;

         default:
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Perf Check",
         perf_check[msg_index], rdaControlPtr->perf_check);
   }

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /*** auto calib override ***/
      /* if abs value is within a certain range, must be a manual calib value -
         need to retrieve scale and offset, then apply and print value */
      if ( (abs((int) rdaControlPtr->auto_calib)) <= 1000 )
      {
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
               "Auto. Cal. Over. (dB)", (float)(rdaControlPtr->auto_calib * 0.001));
      }
      else
      {
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
            "Auto. Cal. Over.", rdaControlPtr->auto_calib);
      }

      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Select Operating Mode",
         rdaControlPtr->operate_mode);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Channel Control Command",
         rdaControlPtr->channel);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Spot Blanking",
         rdaControlPtr->spot_blanking);
      fprintf(stderr, "\n");
   }

   return returnVal;
} /* end ProcessOrdaControlCmdsMsg */


/*******************************************************************************
* Function Name:	
*	ProcessConsoleMsg
*
* Description:
* 	Read the RDA Console Message (message type 4, see table 6
*	in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short* msgPtr - pointer to the msg including the msg hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-21-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessConsoleMsg(short *msgPtr)
{
   int				err		= 0;
   int				returnVal	= 0;
   char*			str_buf		= NULL;
   RDA_RPG_console_message_t*	console_msg	= NULL;


   /* Cast the msg pointer to the console msg structure object */
   console_msg = (RDA_RPG_console_message_t *)msgPtr;

   /* Set global message type flag */
   MessageType = CONSOLE_MESSAGE_A2G;

   if ( VerboseLevel >= MONWB_HIGH )
   {
      /* Print out message fields */

      /* console msg size (not including msg hdr) */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Console Message Size (chars)",
         console_msg->size);

      /* allocate space for storage buffer */
      str_buf = (char*) calloc ( (size_t) 1, (size_t)(console_msg->size + 1) );

      /* First remove any newline characters which would mess up the output */
      Remove_newline_chars( console_msg->message, str_buf );

      /* console msg text */
      fprintf(stderr, MONWB_CHAR_FMT_STRING_NOFIELD, BlankString, "Console Message:");
      err = Print_long_text_msg ( str_buf, console_msg->size,
         MONWB_MAX_NUM_CHARS_IN_FORMAT_STR );

      free( str_buf );
   }

   return returnVal;
} /* end ProcessConsoleMsg */


/*******************************************************************************
* Function Name:
*       ProcessVcpMsg
*
* Description:
*       Read the VCP Message (message type 5/7 in ICD) and perform a validity
*       check on the fields.  Print the most useful information to the screen.
*
* Inputs:
*       short* msgPtr - pointer to VCP message starting AFTER the 16 byte hdr.
*
* Returns:
*       int returnVal:          MONWB_SUCCESS (success) or MONWB_FAIL (failure)
*
* Author:
*       R. Solomon, RSIS.
*
* History:
*       5-01-2003, R. Solomon. Created.
*       6-17-2004, R. Solomon. Updated to print fields and do msg validation.
*       6-01-2005, R. Solomon. Updated to print more fields.
*       3-23-2007, R. Solomon. Updated to handle nested VCP structures
*                              independently.
*******************************************************************************/
static int ProcessVcpMsg(short *msgPtr)
{
   int                  returnVal               = MONWB_SUCCESS;
   int                  msg_index               = 0;
   int                  num_elev_cuts           = 0;
   int                  actual_sz_bytes         = 0;
   int                  expected_sz_bytes       = 0;
   int                  cut_num                 = 0;
   static char*         pattern_type[]          = {"Constant Elev. Cut",
                                                   "Horiz. Raster Scan",
                                                   "Vert. Raster Scan",
                                                   "Searchlight",
                                                   "Unknown"};
   static char*         waveform[]              = {"Undef",
                                                   "   CS",
                                                   " CD/W",
                                                   "CD/WO",
                                                   "    B",
                                                   "  SPP",
                                                   "Unkwn"};
   static char*         dualpol[]		= {"N", "Y" };
   static char*         dopp_res[]              = {"0.5 m/s",
                                                   "1.0 m/s",
                                                   "Unknown"};
   static char*         pulse_width[]           = {"Short",
                                                   "Long",
                                                   "Unknown"};
   VCP_ICD_msg_t*       vcp_icd_msg             = (VCP_ICD_msg_t*)msgPtr;


   /* assign num elev cuts variable */
   num_elev_cuts = vcp_icd_msg->vcp_elev_data.number_cuts;

   /* perform a check on the message size */
   actual_sz_bytes = vcp_icd_msg->vcp_msg_hdr.msg_size * sizeof(short);
   expected_sz_bytes = sizeof(VCP_message_header_t) + 
                       MONWB_SIZEOF_VCP_ELEV_HDR + 
                       (num_elev_cuts * sizeof(VCP_elevation_cut_data_t));

   if ( actual_sz_bytes > expected_sz_bytes )
   {
      /* post error message about size mismatch, but continue processing */
      fprintf(stderr,
         "ProcessVcpMsg: message size (%d) more than exepected size (%d)\n",
         actual_sz_bytes, expected_sz_bytes);
   }
   else if ( actual_sz_bytes < expected_sz_bytes )
   {
      /* this will lead to a segment violation, post error msg and return */
      fprintf(stderr,
         "ProcessVcpMsg: message size (%d) less than expected size (%d)\n",
         actual_sz_bytes, expected_sz_bytes);
      return (MONWB_FAIL);
   }

   /*** Print out desired message fields ***/

   if ( VerboseLevel >= MONWB_LOW )
   {
      /* vcp msg size */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Msg Size (halfwords)",
         vcp_icd_msg->vcp_msg_hdr.msg_size);

      /* vcp pattern type */
      switch ( vcp_icd_msg->vcp_msg_hdr.pattern_type )
      {
         case ( (unsigned short) VCP_CONST_ELEV_CUT_PT ):
            msg_index = 0;
            break;

         case ( (unsigned short) VCP_HORIZ_RAST_SCAN_PT ):
            msg_index = 1;
            break;

         case ( (unsigned short) VCP_VERT_RAST_SCAN_PT ):
            msg_index = 2;
            break;

         case ( (unsigned short) VCP_SEARCHLIGHT_PT ):
            msg_index = 3;
            break;

         default:
            msg_index = 4;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Pattern Type",
         pattern_type[msg_index], vcp_icd_msg->vcp_msg_hdr.pattern_type);

      /* vcp pattern number */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Pattern Num",
         vcp_icd_msg->vcp_msg_hdr.pattern_number);

      /* doppler resolution */
      switch ( vcp_icd_msg->vcp_elev_data.doppler_res )
      {
         case ( (unsigned short) MONWB_VCP_HI_DOPP_RES ):
            msg_index = 0;
            break;

         case ( (unsigned short) MONWB_VCP_LO_DOPP_RES ):
            msg_index = 1;
            break;

         default:
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Doppler Resolution",
         dopp_res[msg_index], vcp_icd_msg->vcp_elev_data.doppler_res);

      /* pulse width */
      switch ( vcp_icd_msg->vcp_elev_data.pulse_width )
      {
         case ( (unsigned short) MONWB_VCP_SHORT_PULSE ):
            msg_index = 0;
            break;

         case ( (unsigned short) MONWB_VCP_LONG_PULSE ):
            msg_index = 1;
            break;

         default:
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString, "Pulse Width",
         pulse_width[msg_index], vcp_icd_msg->vcp_elev_data.pulse_width);

      /* num elev cuts */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Num Elev Cuts",
         num_elev_cuts);
   }

   if ( VerboseLevel >= MONWB_LOW )
   {
      /* print elev cut headers */
      fprintf( stderr, MONWB_VCP_ELEV_HDR1_FMT_STRING, BlankString);
      fprintf( stderr, MONWB_VCP_ELEV_HDR2_FMT_STRING, BlankString);
      fprintf( stderr, MONWB_VCP_ELEV_HDR3_FMT_STRING, BlankString);

      int dual_pol_enabled = 0;
      unsigned short angle_bams;
      float angle_deg;
      unsigned char phase;
      unsigned char wf;
      unsigned char super_res;
      unsigned char s_prf_num;
      short s_prf_pulse;
      short az_rate_bam;
      float az_rate_deg;
      float r_thresh, v_thresh, sw_thresh;
      short d_prf1_edge, d_prf1_num, d_prf1_pulse;
      short d_prf2_edge, d_prf2_num, d_prf2_pulse;
      short d_prf3_edge, d_prf3_num, d_prf3_pulse;
      float d_prf1_edge_deg, d_prf2_edge_deg, d_prf3_edge_deg;

      /* loop through cuts */
      for (cut_num = 0; cut_num < num_elev_cuts; cut_num++)
      {
         angle_bams = (unsigned short)vcp_icd_msg->vcp_elev_data.data[cut_num].angle;
         angle_deg = (float) ORPGVCP_BAMS_to_deg(ORPGVCP_ELEVATION_ANGLE, angle_bams);

         phase = (unsigned char)vcp_icd_msg->vcp_elev_data.data[cut_num].phase;

         wf = (unsigned char)vcp_icd_msg->vcp_elev_data.data[cut_num].waveform;
         switch (wf)
         {
            case 0:
               msg_index = 0;
               break;
            case 1:
               msg_index = 1;
               break;
            case 2:
               msg_index = 2;
               break;
            case 3:
               msg_index = 3;
               break;
            case 4:
               msg_index = 4;
               break;
            case 5:
               msg_index = 5;
               break;
            default:
               msg_index = 6;
               break;
         }

         super_res = (unsigned char)vcp_icd_msg->vcp_elev_data.data[cut_num].super_res & VCP_SUPER_RES_MASK;
         if( vcp_icd_msg->vcp_elev_data.data[cut_num].super_res & VCP_DUAL_POL_ENABLED )
            dual_pol_enabled = 1;
         else
            dual_pol_enabled = 0;
         s_prf_num = (unsigned char)vcp_icd_msg->vcp_elev_data.data[cut_num].surv_prf_num;
         s_prf_pulse = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].surv_prf_pulse;
         az_rate_bam = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].azimuth_rate;
         az_rate_deg = ORPGVCP_rate_BAMS_to_degs(az_rate_bam);
         r_thresh = (float)(vcp_icd_msg->vcp_elev_data.data[cut_num].refl_thresh)/8.0;
         v_thresh = (float)(vcp_icd_msg->vcp_elev_data.data[cut_num].vel_thresh)/8.0;
         sw_thresh = (float)(vcp_icd_msg->vcp_elev_data.data[cut_num].sw_thresh)/8.0;
         d_prf1_edge = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].edge_angle1;
         d_prf1_edge_deg = (float) ORPGVCP_BAMS_to_deg(ORPGVCP_AZIMUTH_ANGLE, d_prf1_edge);
         d_prf1_num = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_num1;
         d_prf1_pulse = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_pulse1;
         d_prf2_edge = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].edge_angle2;
         d_prf2_edge_deg = (float) ORPGVCP_BAMS_to_deg(ORPGVCP_AZIMUTH_ANGLE, d_prf2_edge);
         d_prf2_num = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_num2;
         d_prf2_pulse = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_pulse2;
         d_prf3_edge = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].edge_angle3;
         d_prf3_edge_deg = (float) ORPGVCP_BAMS_to_deg(ORPGVCP_AZIMUTH_ANGLE, d_prf3_edge);
         d_prf3_num = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_num3;
         d_prf3_pulse = (short)vcp_icd_msg->vcp_elev_data.data[cut_num].dopp_prf_pulse3;

         /* print cut data */
         fprintf( stderr, MONWB_VCP_ELEV_DATA_FMT_STRING, BlankString, cut_num,
            angle_bams, angle_deg, phase, waveform[msg_index], super_res, dualpol[dual_pol_enabled],
            az_rate_bam, az_rate_deg, r_thresh, v_thresh, sw_thresh, s_prf_num,
            s_prf_pulse, d_prf1_edge_deg, d_prf1_num, d_prf1_pulse,
            d_prf2_edge_deg, d_prf2_num, d_prf2_pulse, d_prf3_edge_deg,
            d_prf3_num, d_prf3_pulse);
      }
   }

   return returnVal;
} /* end ProcessVcpMsg */

/*******************************************************************************
* Function Name:	
*	ProcessLgcyCltrFltrBypsMsg
*
* Description:
* 	Read the Clutter Filter Bypass Map Message (message type 13, see
*       table 9 in ICD) and perform a validity check on the fields.
*	IMPORTANT NOTE: the internal storage structure is slightly different
*	than the ICD message, therefore a cast cannot be done.
*
* Inputs:
*	msgPtr - pointer to the msg including the msg hdr.	
*
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-1-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessLgcyCltrFltrBypsMsg(short *msgPtr)
{
   int			     returnVal  = 0;
   RDA_RPG_message_header_t* msg_hdr    = (RDA_RPG_message_header_t*)msgPtr;
   short		     numsegs    = 0;

   /* Set global message type flag */
   MessageType = CLUTTER_FILTER_BYPASS_MAP;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* num elevation segments (first segment only) */
      if ( msg_hdr->seg_num == 1 )
      {
         numsegs = *((short*)(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS));
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Number of Elev. Segments",
            numsegs);
      }
   }

   return returnVal;
} /* end ProcessLgcyCltrFltrBypsMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaCltrFltrBypsMsg
*
* Description:
* 	Read the Clutter Filter Bypass Map Message (message type 13, see
*       table 9 in ICD) and perform a validity check on the fields.  This is 
*	a multi-segment message.  The segments are first pieced together as they
*       come in, then they're cast to the structure, then the desired fields are
*       printed, then the message validation is performed on the entire message.
*
* Inputs:
*	msgPtr - pointer to the msg starting with the 16 byte msg hdr.	
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-1-2003, R. Solomon.  Created.
*	6-11-2004, R. Solomon.  Updated to handle all msg segments.
*******************************************************************************/
static int ProcessOrdaCltrFltrBypsMsg(short *msgPtr)
{
   int				returnVal		= 0;
   int				seg_num			= 0;
   static int			tot_segs		= 0;
   int				data_seg_size_bytes	= 0;
   int				max_msg_size_shorts	= 0;
   int				max_msg_size_bytes	= 0;
   static short*		bpm_msg_bfr		= NULL;
   static int			bpm_msg_bfr_idx		= 0;
   static ORDA_bypass_map_msg_t* bpm_msg		= NULL;
   RDA_RPG_message_header_t*	msg_hdr			= NULL;
   static int			init_flag		= 0;


   /* Set global message type flag */
   MessageType = CLUTTER_FILTER_BYPASS_MAP;

   /* Get the required msg hdr fields */
   msg_hdr = (RDA_RPG_message_header_t *) msgPtr;
   seg_num = msg_hdr->seg_num;
   data_seg_size_bytes = (msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS) * sizeof(short);

   /* For segment 1, allocate the static buffer and store the data.  Also print
      the fields that are resident in the first segment. */
   if ( seg_num == 1 )
   {
      /* Reset static variables */
      bpm_msg_bfr_idx = 0;
      bpm_msg = NULL;
      init_flag = 0;

      /* determine total number of segments for this msg */
      tot_segs = msg_hdr->num_segs;

      /* determine max msg size */
      max_msg_size_shorts = ((msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS) * tot_segs ) +
         MONWB_MSG_HDR_SIZE_SHORTS;
      max_msg_size_bytes = max_msg_size_shorts * sizeof(short); 

      /* Free memory already used .... */
      if( bpm_msg_bfr != NULL )
         free(bpm_msg_bfr);

      /* Allocate buffer to store entire message */
      bpm_msg_bfr = (short *)calloc(max_msg_size_shorts, sizeof(short));

      /* Copy msg hdr to buffer */
      memcpy(&bpm_msg_bfr[bpm_msg_bfr_idx], msg_hdr, sizeof(RDA_RPG_message_header_t));

      /* Increment the buffer index variable */
      bpm_msg_bfr_idx += MONWB_MSG_HDR_SIZE_SHORTS;

      /* Print out segment 1 data fields */
      if ( VerboseLevel >= MONWB_MODERATE )
      {
         /* date and time */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Generation Date (Modified Julian)",
            *(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS) );
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Generation Time (min since midnight GMT)",
            *(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS + 1) );

         /* num elevation segments (first seg only) */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Number of Elev. Segments",
            *(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS + 2) );
      }
      
      /* set flag to indicate initialization was done */
      init_flag = 1;
   }

   if ( (seg_num <= tot_segs ) && (init_flag == 1) )
   {
      /* Increment the data ptr past the msg hdr */
      msgPtr += MONWB_MSG_HDR_SIZE_SHORTS;  

      /* Save msg data to buffer */
      memcpy(&bpm_msg_bfr[bpm_msg_bfr_idx], msgPtr, data_seg_size_bytes);

      /* Increment the buffer index variable */
      bpm_msg_bfr_idx += data_seg_size_bytes / sizeof(short);

      /* if we have the entire msg, perform the message validation */
      if ( seg_num == tot_segs )
      {
         /* Free buffer memory */
         free( bpm_msg_bfr );

         /* Reset static variables */
         tot_segs = 0;
         bpm_msg_bfr_idx = 0;
         bpm_msg = NULL;
         init_flag = 0;
         bpm_msg_bfr = NULL;
      }
   }
   else
   {
      fprintf(stderr, "ERROR: seg_num great than tot_segs or init failed.\n");
   }

   return returnVal;
} /* end ProcessOrdaCltrFltrBypsMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyNotchWidthMsg
*
* Description:
* 	Read the Notch Width Map Message (message type 15, see
*       table 14 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	msgPtr - pointer to the msg starting with the 16 byte msg hdr.	
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-1-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessLgcyNotchWidthMsg(short *msgPtr)
{
   int			returnVal		= 0;
   RDA_notch_map_msg_t*	notch_map		= (RDA_notch_map_msg_t *)msgPtr;

   /* Set global message type flag */
   MessageType = NOTCHWIDTH_MAP_DATA;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Print out message fields */

      /* generation date and time (first segment only) */
      if ( notch_map->msg_hdr.seg_num == 1)
      {
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
            "Generation Date (Modified Julian)", notch_map->notchmap.date);
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
            "Generation Time (min since midnight GMT)", notch_map->notchmap.time);

      }
   }

   return returnVal;
} /* end ProcessLgcyNotchWidthMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaClutterMapMsg
*
* Description:
* 	Read the Open RDA Notch Width Map Message (message type 15, see
*       table 14 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short* msgPtr - pointer to message starting with the 16 byte msg hdr.
*	
* Returns:
*	int returnVal:		>0 (success) or <0 (failure)
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	6-13-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaClutterMapMsg(short *msgPtr)
{

   RDA_RPG_message_header_t*    msg_hdr             = NULL;
   static short*                clut_map            = NULL;
   static size_t                max_msg_size_shorts = 0;
   static size_t                max_msg_size_bytes  = 0;
   static size_t                msg_seg_size_shorts = 0;
   static size_t                msg_seg_size_bytes  = 0;
   static int                   clutidx             = 0;
   static int                   tot_segs            = 0;
   int                          clutidx_bytes       = 0;
   int                          seg_num             = 0;


   /* Get the segment number and total number of segments. */
   msg_hdr = (RDA_RPG_message_header_t *) msgPtr;

   /* Extract segment number from the msg hdr */
   seg_num = msg_hdr->seg_num;

   if ( seg_num == 1 )
   {
      /* store the total number of segs */
      tot_segs = msg_hdr->num_segs;

      /* Determine the total size of the message from the number of
         segments defined in the msg hdr. */
      max_msg_size_shorts = (size_t)(((msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS) * tot_segs) +
         MONWB_MSG_HDR_SIZE_SHORTS);
      max_msg_size_bytes = max_msg_size_shorts * sizeof(short);

      /* Allocate space to store the clutter map data. If
         map pointer is not NULL, free memory associated with it. */
      if( clut_map != NULL )
      {
         free( clut_map );
      }

      if( (clut_map = (short *) calloc( max_msg_size_shorts,
         (size_t) sizeof(short))) == NULL )
      {
         fprintf( stderr, "Clutter Map calloc Failed\n" );
         return ( -1 );
      }

      /* Copy the message header into the memory buffer. */
      memcpy( clut_map, msgPtr, (size_t)sizeof(RDA_RPG_message_header_t));

      /* Initialize index into clutter map table for this segment.  Adjust for
         the size of the msg hdr.*/
      clutidx = MONWB_MSG_HDR_SIZE_SHORTS;

      /* print generation date and time */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
         "Generation Date (Modified Julian)",
         *((short*)(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS + 0)));
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
         "Generation Time (min since midnight GMT)",
         *((short*)(msgPtr + MONWB_MSG_HDR_SIZE_SHORTS + 1)));
   }

   /* Adjust the start position of the rda data ptr by the size of the
      message header. */
   msgPtr += sizeof(RDA_RPG_message_header_t)/sizeof(short);

   /* Set vars that will be used for checking map size */
   msg_seg_size_shorts = msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS;
   msg_seg_size_bytes = (size_t) (msg_seg_size_shorts * sizeof(short));
   clutidx_bytes = clutidx * sizeof(short);

   /* If msg will be too big for structure to hold, reject.  Otherwise copy. */
   if ( (clutidx_bytes + msg_seg_size_bytes) <= max_msg_size_bytes )
   {
      /* Copy clutter map data to memory buffer. */
      memcpy(&clut_map[clutidx], msgPtr, msg_seg_size_bytes);
      clutidx += msg_seg_size_shorts;
      clutidx_bytes = clutidx_bytes + msg_seg_size_bytes;
   }
   else
   {
      /* print msg saying the rda clutter map is bad and return */
      fprintf(stderr, "ERROR in ProcessOrdaClutterMapMsg: Bad clutter map.\n");
      return (-1);
   }

   /* If last segment, perform the following.... */
   if( seg_num == tot_segs )
   {
      /* Free the memory associated with the Clutter Map. */
      if( clut_map != NULL )
      {
         free( clut_map );
         clut_map = NULL;
      }
   }

   return (0);
} /* end ProcessOrdaClutterMapMsg */


/*******************************************************************************
* Function Name:	
*	ProcessAdaptDataMsg
*
* Description:
* 	Read the Adaptation Data Message (message type 18, see
*       table 15 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short*	msgPtr	- pointer to msg data starting with the 16 byte msg hdr.
*	
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-1-2003, R. Solomon.  Created.
*	6-15-2003, R. Solomon.  Updated to piece the segments together and do
*				msg validation.
*
* Notes:
*	This is a multiple segment message from the RDA.  The segments are 
*	first pieced together here (through multiple calls to this function)
*	then the validation procedure is called.
*******************************************************************************/
static int ProcessAdaptDataMsg(short *msgPtr)
{
   int				returnVal		= MONWB_SUCCESS;
   int				seg_num			= 0;
   int				data_seg_size_bytes	= 0;
   int				max_msg_size_bytes	= 0;
   static int			tot_segs		= 0;
   static int			rda_adapt_bfr_idx	= 0;
   static char*			rda_adapt_bfr		= NULL;
   RDA_RPG_message_header_t*	msg_hdr			= NULL;
   static int			init_flag		= 0;

   /* Set global message type flag - currently doesn't exist */
   MessageType = ADAPTATION_DATA;

   /* get msg hdr info */
   msg_hdr = (RDA_RPG_message_header_t *)msgPtr;
   tot_segs = msg_hdr->num_segs;
   seg_num = msg_hdr->seg_num;
   data_seg_size_bytes = (msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS) * sizeof(short);

   /* Process the first msg segment */
   if ( seg_num == 1 )
   {
      /* determine required buffer size and allocate space */
      max_msg_size_bytes = (tot_segs * ( (msg_hdr->size - MONWB_MSG_HDR_SIZE_SHORTS) *
         sizeof(short))) + MONWB_MSG_HDR_SIZE_BYTES;

      rda_adapt_bfr = (char*) calloc( max_msg_size_bytes, sizeof(char) );

      /* save msg hdr to buffer */
      memcpy( &rda_adapt_bfr[rda_adapt_bfr_idx], msgPtr, MONWB_MSG_HDR_SIZE_BYTES);

      /* increment index */
      rda_adapt_bfr_idx += MONWB_MSG_HDR_SIZE_BYTES; 

      if ( VerboseLevel >= MONWB_HIGH )
      {
         /* Print out desired message fields */
      }
   
      /* set the flag to indicate initialization done */
      init_flag = 1;
   }

   if ( (seg_num <= tot_segs) && (init_flag == 1) )
   {
      /* increment msg pointer past msg hdr */
      msgPtr += MONWB_MSG_HDR_SIZE_SHORTS;

      /* save data to buffer */
      memcpy(&rda_adapt_bfr[rda_adapt_bfr_idx], msgPtr, data_seg_size_bytes);
   
      /* increment buffer index */
      rda_adapt_bfr_idx += data_seg_size_bytes;

      /* if last segment, call msg validation function and clean up */
      if ( seg_num == tot_segs )
      {
         /* free memory */
         free( rda_adapt_bfr );

         /* reset static variables */
         tot_segs = 0;
         rda_adapt_bfr_idx = 0;
         init_flag = 0;
      }
   }

   return returnVal;
} /* end ProcessAdaptDataMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyReqForDataMsg
*
* Description:
* 	Read the Request for Data Message (message type 9, see
*       table 13 in ICD) and perform a validity check on the fields.
*
* Inputs:
*       short* msgPtr - Pointer to the msg starting after the 16 byte msg hdr.
*
* Returns:
*	int returnVal: MONWB_SUCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-16-2003, R. Solomon.  Created.
*	5-30-2003, R. Solomon.  Updated comments section to include Inputs.
*******************************************************************************/
static int ProcessLgcyReqForDataMsg(short *msgPtr)
{
   int				msgLen			= 0;
   int				returnVal		= MONWB_SUCCESS;
   RPG_request_data_struct_t*	reqDataPtr		= NULL;
   int				req_index		= 0;
   static char*			request_type[]={"RDA Status",
					        "RDA Performance Data",
					        "Clutter Filter Bypass Map",
                                                "Clutter Filter Notchwidth Map",
                                                "Unknown" };

   /* Cast pointer to structure */
   reqDataPtr = (RPG_request_data_struct_t *) msgPtr;

   if ( reqDataPtr == NULL )
   {
      fprintf(stderr, "ERROR: Problem in ProcessLgcyReqForDataMsg().\n");
      return(MONWB_FAIL);
   }

   /* Set global message type flag */
   MessageType = REQUEST_FOR_DATA;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Decode request type */
      if ( reqDataPtr->RPG_request_data == (unsigned short) MONWB_REQDAT_RDA_STATUS )
      {
         req_index = 0;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_RDA_PERFMAINT )
      {
         req_index = 1;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_CLTR_FILTER_BYPASS_MAP )
      {
         req_index = 2;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_CLTR_FILTER_NOTCHWIDTH_MAP )
      {
         req_index = 3;
      }
      else
      {
         req_index = 4;
      }

      msgLen = sprintf(Message, "\n%sRequesting %s\n", BlankString,
         request_type[req_index]);
      PrintMsg(msgLen);
   }

   return returnVal;
} /* end ProcessLgcyReqForDataMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaReqForDataMsg
*
* Description:
* 	Read the Request for Data Message (message type 9, see
*       table 13 in ICD) and perform a validity check on the fields.
*
* Inputs:
*	short*	msgPtr	Pointer to the msg starting after the 16 byte msg hdr.
*
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	6-13-2003, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaReqForDataMsg(short *msgPtr)
{
   int				msgLen			= 0;
   int				returnVal		= MONWB_SUCCESS;
   RPG_request_data_struct_t*	reqDataPtr		= NULL;
   int				req_index		= 0;
   static char*			request_type[]={"RDA Status",
					        "RDA Performance Data",
					        "Clutter Filter Bypass Map",
                                                "Clutter Filter Map",
                                                "RDA Adaptation Data",
                                                "Volume Coverage Pattern",
                                                "Unknown" };


   /* Cast pointer to structure */
   reqDataPtr = (RPG_request_data_struct_t *) msgPtr;

   if ( reqDataPtr == NULL )
   {
      fprintf(stderr, "ERROR: Problem in ProcessOrdaReqForDataMsg().\n");
      return(MONWB_FAIL);
   }

   /* Set global message type flag */
   MessageType = REQUEST_FOR_DATA;

   if ( VerboseLevel >= MONWB_LOW )
   {
      /* Decode request type */
      if ( reqDataPtr->RPG_request_data == (unsigned short) MONWB_REQDAT_RDA_STATUS )
      {
         req_index = 0;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_RDA_PERFMAINT )
      {
         req_index = 1;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_CLTR_FILTER_BYPASS_MAP )
      {
         req_index = 2;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_CLTR_FILTER_NOTCHWIDTH_MAP )
      {
         req_index = 3;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_RDA_ADAPTATION_DATA )
      {
         req_index = 3;
      }
      else if ( reqDataPtr->RPG_request_data ==
                (unsigned short) MONWB_REQDAT_VOLUME_COVERAGE_PATTERN )
      {
         req_index = 3;
      }
      else
      {
         req_index = 4;
      }

      msgLen = sprintf(Message, "\n%sRequesting %s\n", BlankString,
         request_type[req_index]);
      PrintMsg(msgLen);
   }

   return returnVal;
} /* end ProcessOrdaReqForDataMsg */


/*******************************************************************************
* Function Name:	
*	ProcessLgcyClczMsg
*
* Description:
*	Process the Legacy Clutter Censor Zones Msg. 	
*
* Inputs:
*	short*	msgPtr	Pointer to the msg starting after the 16 byte msg hdr.	
*
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-17-2004, R. Solomon.  Created.
*******************************************************************************/
static int ProcessLgcyClczMsg(short* msgDataPtr)
{
   int				region_ind = 0;
   int				returnVal = MONWB_SUCCESS;
   RPG_clutter_regions_t*	clcz_data = (RPG_clutter_regions_t*)msgDataPtr;


   /* Set global message type flag */
   MessageType = CLUTTER_SENSOR_ZONES;

   if ( VerboseLevel >= MONWB_MODERATE )
   {
      /* Print out message fields */

      /* number of override regions */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Num Override Regions", 
         clcz_data->regions);

      if ( VerboseLevel >= MONWB_HIGH )
      {
         for (region_ind = 0; region_ind < clcz_data->regions; region_ind++)
         {
            /* print region # */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Region #", region_ind);

            /* start range */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Range (km)", 
               clcz_data->data[region_ind].start_range);

            /* stop range */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Range (km)", 
               clcz_data->data[region_ind].stop_range);

            /* start azi */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Azimuth (deg)", 
               clcz_data->data[region_ind].start_azimuth);

            /* stop azi */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Stop Azimuth (deg)", 
               clcz_data->data[region_ind].stop_azimuth);

            /* elev seg num */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Elev. Seg. Num", 
               clcz_data->data[region_ind].segment);

            /* op select code */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Oper. Select Code", 
               clcz_data->data[region_ind].select_code);

            /* doppler suppression level */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Doppler Suppr. Level", 
               clcz_data->data[region_ind].doppl_level);

            /* surveillance suppression level */
            fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Surv. Suppr. Level", 
               clcz_data->data[region_ind].surv_level);
         }
      }
   }

   return returnVal;
} /* end ProcessLgcyClczMsg */


/*******************************************************************************
* Function Name:	
*	ProcessOrdaClczMsg
*
* Description:
*	Process the Open Systems Clutter Censor Zones Msg. 	
*
* Inputs:
*	short* msgPtr - Pointer to the msg starting after the 16 byte msg hdr.	
*
* Returns:
*	int returnVal: MONWB_SUCCESS or MONWB_FAIL
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5-17-2004, R. Solomon.  Created.
*******************************************************************************/
static int ProcessOrdaClczMsg(short* msgDataPtr)
{
   int				region_ind = 0;
   int				returnVal = MONWB_SUCCESS;
   ORPG_clutter_regions_t*	clcz_data = (ORPG_clutter_regions_t*)msgDataPtr;


   /* Set global message type flag */
   MessageType = CLUTTER_SENSOR_ZONES;

   if ( VerboseLevel >= MONWB_LOW )
   {
      /* Print out message fields */

      /* number of override regions */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Num Override Regions", 
         clcz_data->regions);

      /* print region info */
      for (region_ind = 0; region_ind < clcz_data->regions; region_ind++)
      {
         /* print region # */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Region #", region_ind);

         /* start range */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Range (km)", 
            clcz_data->data[region_ind].start_range);

         /* stop range */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Range (km)", 
            clcz_data->data[region_ind].stop_range);

         /* start azi */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Start Azimuth (deg)", 
            clcz_data->data[region_ind].start_azimuth);

         /* stop azi */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Stop Azimuth (deg)", 
            clcz_data->data[region_ind].stop_azimuth);

         /* elev seg num */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Elev. Seg. Num", 
            clcz_data->data[region_ind].segment);

         /* op select code */
         fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Oper. Select Code", 
            clcz_data->data[region_ind].select_code);
           
         /* print blank line for separation */
         fprintf(stderr, "\n");
      }
   }

   return returnVal;
} /* end ProcessOrdaClczMsg */


/*******************************************************************************
* Function Name:	
*	PrintMsg
*
* Description:
*	Displays the msg time followed by the message.
*
* Inputs:
*	int	len	The length of the character string to be printed.
*	
* Returns:
*	void
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-24-2003, R. Solomon.  Created.  Taken from "mon_nb" source code.
*******************************************************************************/
void PrintMsg( int len )
{
   int err;
   int hr, min, sec, millisec, msg_time_msec;
   static char disp_time[9];

   /* convert time (millisecs after midnight) to readable fmt */
   msg_time_msec = Msg_time * 1000;
   err = RPGCS_convert_radial_time( msg_time_msec, &hr, &min, &sec, &millisec);

   /* Add string terminating character just to be safe. */
   if ( len < MONWB_MAX_NUM_CHARS )
      Message[len] = '\0';
   else
      Message[MONWB_MAX_NUM_CHARS - 1] = '\0';

   /* If new message, print message divider, then mark as old message */
   if ( NewMessageFlag == MONWB_TRUE )
   {
     sprintf( disp_time, "%02d:%02d:%02d", hr, min, sec );
     fprintf(stderr, "\n%s\n", Divider);
     NewMessageFlag = MONWB_FALSE;
   }
   else
   {
      sprintf( disp_time, "        ");
   }

   fprintf( stderr, "%s  %s", disp_time, Message );

} /* End of PrintMsg() */


/*******************************************************************************
* Function Name:
*	bam_to_deg
*
* Description:
*	Converts the input angle value assumed to be in BAM units to degrees.
*
* Inputs:
*	void *in  -  pointer to angle value to be converted
*       void *out  -  pointer to memory for storing result
*
* Returns:
*	void
*
* Author:
*	Z. Jing
*
* History:
*	April 2003 - created, Z. Jing.
*       5/2/03, R. Solomon, RSIS.  Added header info. No other changes.
*	5/13/03, R. Solomon, RSIS. Changed the cast to be an unsigned short
*				   instead of a regular short.
*******************************************************************************/
static void bam_to_deg (void *in, void *out) 
{
    unsigned short s;

    s = *((unsigned short *)in);

    *((double *)out) = (double)(s >> 3) * .043945;
}

/*******************************************************************************
* Function name:
*	check_vcp_num_range
*
* Author:
*	R. Solomon, RSIS
*
* Date:
*	4/28/03
*
* Description:
*	Checks to see if the VCP number passed in the RDA Status Message
*	conforms to the ICD.
*
* Inputs:
*	void* vcp_data  -  void pointer to the vcp number
*
* Returns:
*	success (1) if data conforms to ICD.
*	fail (0) if data does not conform to the ICD.
*******************************************************************************/
int check_vcp_num_range(void *vcp_data)
{
   int		success		= 1;
/*   int		fail		= 0; */
/*   short	*vcp_num	= (short *)(vcp_data); */

   /* 
      Note: Due to the complexity of the vcp_num field in the 
      RDA_status_msg_t struct, sophisticated range checks could be
      implemented in the future which is why this function exists.  The
      range check code should be placed directly after this comment.
   */

   return success;  
} /* end check_vcp_num_range */


/***************************************************************************
* Function Name:
*	Print_mon_wb_usage
*
* Description:
*	This function prints usage information.
*
* Inputs:
*	argv - the list of command line arguments
*
* Return:
*	none
*
* Author:
*	R. Solomon, RSIS.  Adapted from Print_usage function obtained from
*			   the mon_nb code (author unknown, believed to be
* 			   Steve Smith).
***************************************************************************/
static void Print_mon_wb_usage (char **argv)
{
   fprintf(stderr, "\n Usage: %s [Options] \n", argv[0]);
   fprintf(stderr, "\n Options:\n");
   fprintf(stderr,"\t-h\t\t: help\n");
   fprintf(stderr,
      "\t-v<num>\t\t: verbose level (Ex. -v2 = verbose level 2)\n");
   fprintf(stderr,"\t-a\t\t: process all messages\n");
   fprintf(stderr,
      "\t-r<I, O, or B>  : rewind LB [I = Input LB (Response LB),\n");
   fprintf(stderr, "\t\t\t  O = Output LB (Request LB), B = Both]\n");
   fprintf(stderr,
      "\t-f<I, O><file>  : user specified LB (I = Input LB, O = Output LB).\n");
   fprintf(stderr,"\t-m\t\t: print moment data in the basedata msg\n");
   fprintf(stderr, "\n For more info, visit the mon_wb man page.\n\n");

} /* end of Print_mon_wb_usage() */


/***************************************************************************
* Function Name:
*	ProcessCommandLineArgs
*
* Description:
*	This function interprets the command line arguments supplied by the
*	user and acts accordingly.
*
* Inputs:
*	num_args - the number of command line args
*	arg_ptr - char pointer to the argument list
*
* Return:
*	int - 0 or greater = success, less than 0 = failure
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5/12/2003, R. Solomon.  Created.
*	6/04/2004, R. Solomon.  Added "-r" option.
*	6/15/2004, R. Solomon.  Added "-fI" and "-fO" option.
*	10/4/2006, R. Solomon.  Added "-m" option for printing moment data.
***************************************************************************/
static int ProcessCommandLineArgs(int argc, char **argv)
{
   extern char*	optarg;    /* used by getopt */
   extern int	optind;
   int		c;         /* used by getopt */
   int		ret_val;   /* return value */


   /* Process all command line arguments. */
   ret_val = 0;

   /* Process all command line arguments. */
   while ((c = getopt (argc, argv, "r:v:haf:m")) != EOF) 
   {
      switch (c) 
      {
         case 'r':
         {
            /* 
               If "I" follows "-r", rewind only the response LB.  If "O"
               follows "-r", rewind only the request LB.  If "B" follows "-r",
               rewind both the response and request LBs.
            */ 
            if ( strncmp( optarg, "I", 1 ) == 0 )
            {
               RewindFlag = MONWB_TRUE;
               RewindInputBuffer = MONWB_TRUE; 

               /* Print informative msg */
               fprintf(stderr, "Rewinding Input Buffer\n");
            }
            else if ( strncmp( optarg, "O", 1 ) == 0 )
            {
               RewindFlag = MONWB_TRUE;
               RewindOutputBuffer = MONWB_TRUE;  

               /* Print informative msg */
               fprintf(stderr, "Rewinding Output Buffer\n");
            }
            else if ( strncmp( optarg, "B", 1 ) == 0 )
            {
               RewindFlag = MONWB_TRUE;
               RewindOutputBuffer = MONWB_TRUE;  
               RewindInputBuffer = MONWB_TRUE; 

               /* Print informative msg */
               fprintf(stderr, "Rewinding Input Buffer\n");
               fprintf(stderr, "Rewinding Output Buffer\n");
            }
            else 
            {
               ret_val = -1;
            }
     
            break;
         }
         case 'v':
         {
            VerboseLevel = atoi( optarg );
         
            if ( VerboseLevel < MONWB_NONE )
            {
               ret_val = -1;
            }

            /* Print the verbose level the user has chosen */
            fprintf(stderr, "Verbose level: %d\n", VerboseLevel);

            break;
         }
         case 'h':
         {
            ret_val = -1;
            break;
         }
         case 'a':
         {
            /* Set the global variable ProcessAllMsgsFlag */
            ProcessAllMsgsFlag = MONWB_TRUE;   /* process all msgs, including all
                                      intermediate basedata msgs */

            /* Print informative message */
            fprintf(stderr,
               "Processing All Messages Including Intermediate Basedata Msgs\n");

            break;
         }
         case 'f':
         {
            /* 
               If "I" follows "-f", a user-supplied "response" LB name must
               follow.  If "O" follows "-f", a user-supplied "request" LB name
               must follow. 
            */ 
            if ( strncmp( optarg, "I", 1 ) == 0 )
            {
               /* save file name */
               strcpy(RespFilename, (optarg+1));

               /* set global flag for using user specified response buffer file */
               User_defined_resp_file = MONWB_TRUE;

               /* Print informative msg */
               fprintf(stderr, "User Defined Input (Response) Buffer Name: %s\n",
                  RespFilename);
            }
            else if ( strncmp( optarg, "O", 1 ) == 0 )
            {
               /* save file name */
               strcpy(ReqFilename, (optarg+1));

               /* set global flag for using user specified request buffer file */
               User_defined_req_file = MONWB_TRUE;

               /* Print informative msg */
               fprintf(stderr, "User Defined Output (Request) Buffer Name: %s\n",
                  ReqFilename);
            }
            else 
            {
               ret_val = -1;
            }
     
            break;
         }
         case 'm':
         {
            PrintMomentDataFlag = MONWB_TRUE;
            break;
         }
         case '?':
         {
            ret_val = -1;
            break;
         }
      } /* End of "switch" */
   } /* End of "while" */

   return (ret_val);

} /* end ProcessCommandLineArgs */    


/***************************************************************************
* Function Name:
*	ProcessRdaAlarmCodes
*
* Description:
*	Given an alarm code, print out the textual meaning of the alarm.
*	Do not print out information for alarm code 0 (no alarms).
*
* Inputs:
*	short*	alarm_code_ptr	Pointer to short integer variable marking 
*				the location of the alarm code.	
*	int	code_index	Array index of alarm.
*
* Return:
*	int - 0 or greater = success, less than 0 = failure
***************************************************************************/
static int ProcessRdaAlarmCodes(short* alarm_code_ptr, int code_index)
{
   short	temp_alarm_code		= 0;
 

   /* Negative alarm code means the alarm has been cleared */
   if ( *( alarm_code_ptr + code_index ) < 0 )
   {
      temp_alarm_code = *( alarm_code_ptr + code_index ) & 0x7FFF;
      fprintf(stderr, MONWB_ALARM_FMT_STRING_CLEARED, BlankString, temp_alarm_code,
         ORPGRAT_get_alarm_text( abs(temp_alarm_code) ));
   }
   else
   {
      temp_alarm_code = *(alarm_code_ptr + code_index );
      fprintf(stderr, MONWB_ALARM_FMT_STRING, BlankString, temp_alarm_code, 
         ORPGRAT_get_alarm_text( temp_alarm_code ));
   }

   return (0);
}  /* end ProcessRdaAlarmCodes */


/***************************************************************************
* Function Name:
*	SetArchivedPlaybackFlag
*
* Description:
*	Determine whether the LB message contains a comms manager header.
*	If it does, set the ArchivedPlaybackFlag to MONWB_FALSE. If it does
*	not, set the ArchivedPlaybackFlag to MONWB_TRUE.
*
*       NOTE: The comms hdr (if one exists) will be in native format,
*       therefore no byte swapping is required.
*
* Inputs:
*	char*	bufPtr - Pointer to the LB message buffer.
*
* Outputs:
*	int*	archivedplaybackflag - pointer to flag to set/clear
*
* Return:
*	int - MONWB_SUCCESS on success, MONWB_FAIL on failure
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	5/30/2003, R. Solomon.  Created.
***************************************************************************/
int SetArchivedPlaybackFlag(const char* bufPtr, int *archived_playback )
{
   int		first_word		= 0;
   int		shifted_value		= 0;

   if ( bufPtr == NULL )
      return (MONWB_FAIL);

   /* cast the first word of the buffer to an integer value "first_word" */
   first_word = *((int *) bufPtr);

   /* right shift the value by 16 bits */
   shifted_value = (first_word >> 16) & 0xffff;

   /* Since the first word of the comms header structure is "type", the
      maximum value is far less than the max value for a short.  The first
      word of the message header is "size", a short.  Therefore the int cast
      also grabs the next 2 bytes of data.  The shifted value then corresponds
      directly to the size field, which by definition cannot be 0.  Therefore,
      if the shifted value is 0 we know there's a CM hdr. */
   if ( shifted_value == 0 )
   {
      /* Must be a comms header */
      *archived_playback = MONWB_FALSE;
   }
   else
   {
      /* Must be a message header */
      *archived_playback = MONWB_TRUE;
   }

   return (MONWB_SUCCESS);
} /* end SetArchivedPlaybackFlag */


/*******************************************************************************
* Function Name:
*	check_alarm_code_range
*
* Description:
*	Performs a range check on an element of the RDA Status Message
*	alarm_code array.
*
* Inputs:
*	void *alarm_code_ptr  -  pointer to an alarm code
*
* Returns:
*	integer
*		0	Failure	- if data does not conform to the ICD.
*		1	Success - if data conforms to ICD.
*
* History:
*	7/29/03	- Created, R. Solomon.
*******************************************************************************/
int check_alarm_code_range (void *alarm_code_ptr)
{
   short	code	= 0;
   int		ret_val = 0;

   code = *((short *) alarm_code_ptr);

   /* Alarm code can be negative, meaning the alarm has been cleared. */
   if ( code < 0 )
   {
      code = code & 0x7FFF;
      if ( ( code >= 0 ) && ( code <= MAX_RDA_ALARM_NUMBER ) )
      {
         ret_val = 1;
      }
      else
      {
         ret_val = 0;
      }
   }
   else
   {
      if ( ( code >= 0 ) && ( code <= MAX_RDA_ALARM_NUMBER ) )
      {
         ret_val = 1;
      }
      else
      {
         ret_val = 0;
      }
   } 

   return ret_val;

} /* end check_alarm_code_range */

/*******************************************************************************
*
* Description:
*    Receives a pointer to a character string and a number for the
*    max number of characters to print per line.  This function
*    parses the string according to the limit and prints it to stdout.
*
* Inputs:
*    char* msg		- pointer to text string
*    int   msg_size	- size of msg (num of characters)
*    int   line_limit	- max characters per line
*
* Outputs:
*    Prints a formatted text string to stdout.
*
* Returns:
*    On success returns 0.
*    On failure, returns -1.
*
* Globals:
*
* Notes:        
*******************************************************************************/
int Print_long_text_msg ( char* msg, int msg_size, int line_limit )
{
   int		ret = 0;
   int		num_segs;
   int		seg_index;
   int		char_index;
   char*	temp_str;
   char*        msg_buf;


   /* Copy message to string buffer. */
   msg_buf = (char*) calloc( (size_t) 1, ((size_t) msg_size + 1) );
   if ( msg_buf == NULL )
   {
      ret = -1;
   }
   else
   {
      /* Determine the number of 'max_chars_per_line' character segments */
      num_segs = (int)( msg_size / line_limit ) + 1;

      /* Loop through segments and send msgs to task log */
      for ( seg_index = 0; seg_index < num_segs; seg_index++ )
      {
         /* Allocate space for temp_str */
         temp_str = (char *)calloc(1, (line_limit + 1) * sizeof(char));
         if ( temp_str == NULL )
         {
            ret = -1;
         }
         else
         {
            char_index  = seg_index * line_limit;
            strncpy(temp_str, (char*)(msg + char_index), line_limit);

            /* write the segment to stdout */
            fprintf(stderr, MONWB_CHAR_FMT_STRING_NOFIELD, BlankString, temp_str);

            /* Free temp_str memory */
            free(temp_str);
         }
      }

      /* Free console_buf memory */
      free(msg_buf);
   }

   return ( ret );

} /* End Print_long_text_msg() */


/*******************************************************************************
*
* Description:
*    Receives a pointer to a character string (with newlines) and a pointer to
*    a new character buffer for putting the result in.  This function searches
*    for the newline characters in the original string and replaces them with
*    spaces.
*
* Inputs:
*    const char* orig_str	- pointer to original text string
*    char* new_str		- pointer to new string
*
* Outputs:
*    none
*
* Returns:
*    void
*
* Notes:        
*******************************************************************************/
void Remove_newline_chars( const char* orig_str, char* new_str )
{
   char* temp_str	= NULL;
   char* str_token	= NULL;

   /* allocate space for temp_str */
   temp_str = (char *) malloc( (strlen(orig_str) + 1) * sizeof(char) );

   /* first copy orig string to temp string */
   strcpy(temp_str, orig_str);

   /* then tokenize the new string based on the newline character */
   str_token = strtok(temp_str, "\n");  
   if( str_token == NULL ){

      strcat( new_str, temp_str );
      free( temp_str );
      return;

   }

   /* there must be tokens .... */
   while (str_token != NULL)
   {
      /* copy the new token to the new string */
      strcat(new_str, str_token);

      /* add a space char to replace the newline char that was removed */
      strcat(new_str, " ");

      /* get the next token */
      str_token = strtok(NULL, "\n");  
   }

   free( temp_str );

   return;

} /* end Remove_newline_chars */


/*******************************************************************************
* Name:
*    ProcessGenRadMsg
*
* Description:
*    Process the Generic Digital Radar Data message (message number 31).
*
* Inputs:
*    short* msg_ptr - pointer to message starting with the 16 byte msg header
*
* Outputs:
*    none
*
* Returns:
*    integer - MONWB_SUCCESS or MONWB_FAIL
*******************************************************************************/
static int ProcessGenRadMsg( short* msg_ptr )
{
   Generic_basedata_t*  basedataMsgPtr		= NULL;
   int 			msgLen			= 0;
   int			returnVal		= MONWB_SUCCESS;

   /* if the process all flag is set or the radial is a boundary radial print
      the header info */
   if ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END)     ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) ) 
   {
      msgLen = 
         sprintf(Message,
         "<--- GENERIC DIGITAL RADAR DATA MSG (Msg ID:%d Type:%d Len:%d)\n",
         ResponseLbMsgId, (int)GENERIC_DIGITAL_RADAR_DATA, MsgReadLen);
      PrintMsg(msgLen);
      HdrDisplayFlag = MONWB_IS_DISPLAYED;  /* Indicates msg title has been displayed */
   }

   /* Cast basedata pointer to the proper data structure */
   basedataMsgPtr = (Generic_basedata_t *)(msg_ptr);
   if ( basedataMsgPtr == NULL )
   {
      fprintf(stderr,
         "ERROR: Problem processing generic basedata msg.\n");
      return(MONWB_FAIL);
   }

   /* Because of the massive number of basedata msgs produced, by default we're
      only going to range check the msgs from the beginning/end of elevation
      cuts and volume scans */

   if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START )
   {
      msgLen = sprintf(Message, "\n%sBEGINNING OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }
   else if ( RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END )
   {
      msgLen = sprintf(Message, "\n%sEND OF VOLUME SCAN\n", BlankString);
      PrintMsg(msgLen);
   }
   else if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_START )
   {
      msgLen = sprintf(Message, "\n%sBEGINNING OF ELEVATION SCAN %d\n",
         BlankString, basedataMsgPtr->base.elev_num);
      PrintMsg(msgLen);
   }
   else if ( RadialStatus == MONWB_RADIAL_STAT_ELEV_END )
   {
      msgLen = sprintf(Message, "\n%sEND OF ELEVATION SCAN %d\n", 
        BlankString, basedataMsgPtr->base.elev_num);
      PrintMsg(msgLen);
   }
   else if ( RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START )
   {
      msgLen = sprintf(Message, "\n%sBEGINNING OF LAST ELEVATION SCAN %d\n", 
        BlankString, basedataMsgPtr->base.elev_num);
      PrintMsg(msgLen);
   }

   if ( (ProcessAllMsgsFlag == MONWB_TRUE) ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_START)      ||
        (RadialStatus == MONWB_RADIAL_STAT_ELEV_END)        ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_START)   ||
        (RadialStatus == MONWB_RADIAL_STAT_VOLSCAN_END)     ||
        (RadialStatus == MONWB_RADIAL_STAT_LAST_ELEV_START) )
   {
      if ( VerboseLevel >= MONWB_LOW )
      {
         msgLen = sprintf(Message, "\n%sGeneric Basedata Message:",
            BlankString);
         PrintMsg(msgLen);
      }

      /* call function to validate and print the fields in the message */
      PrintGenRad(basedataMsgPtr);

   }

   return returnVal;

} /* end ProcessGenRadMsg() */


/*******************************************************************************
* Name:
*    PrintMsgHdr
*
* Description:
*    Print the RDA<->RPG Message Header fields
*
* Inputs:
*    RDA_RPG_message_header_t* msg_ptr - pointer to the 16 byte msg header
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintMsgHdr( RDA_RPG_message_header_t* msg_ptr )
{
   int err = 0;
   int ret_val = MONWB_SUCCESS;
   int yr, mo, day, hr, min, sec, millisec;
   int msgLen = 0;

   if ( msg_ptr != NULL )
   {
      msgLen = sprintf(Message, "\n%sMsg Header:", BlankString);
      PrintMsg(msgLen);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Size (halfwords)",
         msg_ptr->size);
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "RDA Channel",
         msg_ptr->rda_channel);
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Message Type",
         msg_ptr->type);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Sequence Num",
         msg_ptr->sequence_num);
      err = RPGCS_julian_to_date( msg_ptr->julian_date, &yr,
         &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date", mo, day, yr, msg_ptr->julian_date);
      err = RPGCS_convert_radial_time( msg_ptr->milliseconds,
         &hr, &min, &sec, &millisec );
      fprintf(stderr, MONWB_UNIX_TIME_WITH_MS_FMT_STRING, BlankString,
         "Time", hr, min, sec, millisec, msg_ptr->milliseconds);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Number of Segments",
         msg_ptr->num_segs);
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Segment Number",
         msg_ptr->seg_num);
      fprintf(stderr, "\n\n");
   }
   else
   {
      fprintf(stderr, "PrintMsgHdr: Msg header data pointer equal to NULL.\n");
      ret_val = MONWB_FAIL;
   }
} /* end PrintMsgHdr */


/*******************************************************************************
* Name:
*    PrintGenRad
*
* Description:
*    Print the necessary information in the Generic Digital Radar Message
*
* Inputs:
*    short* msg_ptr - pointer to message starting with the 16 byte msg header
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRad( Generic_basedata_t* msg_p )
{
   int num_blocks = 0;
   int data_blk_offset = 0; /* offset to first data blk following the header */

   /* Call function to print the info in the header */
   PrintGenRadHdr(msg_p);

   /* Get the number of data blocks */
   num_blocks = msg_p->base.no_of_datum;

   data_blk_offset = msg_p->base.data[0];

   /* Call function to print the info in the data blocks */
   PrintGenRadData( ((char *)&msg_p->base), num_blocks, msg_p->base.data );

} /* end PrintGenRad() */


/*******************************************************************************
* Name:
*    PrintGenRadHdr
*
* Description:
*    Print and validate the necessary information in the Generic Digital Radar 
*    Header.
*
* Inputs:
*    short* msg_ptr - pointer to message starting with the 16 byte msg header
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRadHdr( Generic_basedata_t* msg_ptr )
{
   int 			err			= 0;
   float		angle_deg		= 0.0;
   int			yr, mo, day, hr, min, sec, millisec;
   int			msg_index		= 0;
   static char*		radial_status[]		= {"Start of New Elevation Scan",
                                                   "Intermediate Radial Data",
                                                   "End of Elevation Scan",
                                                   "Beginning of Volume Scan",
                                                   "End of Volume Scan",
                                                   "Start of Last Elevation Scan",
                                                   "Unknown"};
   unsigned char        azi_res;
   static char*		azi_res_type[]		={"Half Degree",
						  "One Degree",
						  "Unknown" };
   int			msg_size = 0;


   if ( VerboseLevel >= MONWB_LOW )
   {
      /* Azimuth Number */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Azimuth Number",
         msg_ptr->base.azi_num);

      /* Azimuth angle in deg */
      angle_deg = msg_ptr->base.azimuth;
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Azimuth Angle (deg)",
         angle_deg);

      /* Azimuth resolution */
      azi_res = msg_ptr->base.azimuth_res;
      switch(azi_res)
      {
         case HALF_DEGREE_AZM:
            msg_index = 0;
            break;
         case ONE_DEGREE_AZM:
            msg_index = 1;
            break;
         default:
            msg_index = 2;
            break;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING, BlankString, "Azimuth Resolution Spacing",
         azi_res_type[msg_index]);

      /* Radial Status */
      if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_BEG_ELEV )
      {
         msg_index = 0;
      }
      else if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_INTERMED )
      {
         msg_index = 1;
      }
      else if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_END_ELEV )
      {
         msg_index = 2;
      }
      else if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_BEG_VOL )
      {
         msg_index = 3;
      }
      else if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_END_VOL )
      {
         msg_index = 4;
      }
      else if ( msg_ptr->base.status == (short) MONWB_BD_RADIAL_STAT_BEG_LAST_ELEV )
      {
         msg_index = 5;
      }
      else
      {
         msg_index = 6;
      }
      fprintf(stderr, MONWB_CHAR_FMT_STRING_HEX_VAL, BlankString,
         "Radial Status", radial_status[msg_index], msg_ptr->base.status);

      /* Elevation Number */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Elevation Number",
         msg_ptr->base.elev_num);

      /* Elevation Angle */
      angle_deg = msg_ptr->base.elevation;
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Elevation Angle (deg)",
         angle_deg);

      /* Azimuth Index */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Azimuth Index",
         msg_ptr->base.azimuth_index);
   }

   /* Print out message fields if verbose is MONWB_HIGH or greater */
   if ( VerboseLevel >= MONWB_HIGH )
   {
      /*** Time in ms past midnight, converted to readable format ***/
      err = RPGCS_convert_radial_time( msg_ptr->base.time,
                                       &hr, &min, &sec, &millisec );
      fprintf(stderr, MONWB_UNIX_TIME_WITH_MS_FMT_STRING, BlankString,
         "Collection Time", hr, min, sec, millisec,
         (int)msg_ptr->base.time);

      /*** Modified Julian Date - also print human readable format ***/
      err = RPGCS_julian_to_date( msg_ptr->base.date, &yr, &mo, &day );
      fprintf(stderr, MONWB_UNIX_DATE_FMT_STRING, BlankString,
         "Mod. Julian Date", mo, day, yr, msg_ptr->base.date);

      /*** Cut sector number ***/
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Cut Sector Number",
         msg_ptr->base.sector_num);

      /* Print spot blank flag in hex because it's not mutually exclusive */
      fprintf(stderr, MONWB_HEX_FMT_STRING, BlankString, "Spot Blanking Flag",
         msg_ptr->base.spot_blank_flag);
   }

   /* Validate fields in Gen Rad header */
   if (ArchivedPlaybackFlagResp == MONWB_FALSE)
   {
      msg_size = MsgReadLen -  MONWB_COMMS_HDR_SIZE_BYTES - CTM_HDRSZE_BYTES;
   }
   else
   {
      msg_size = MsgReadLen;
   }

} /* end PrintGenRadHdr() */


/*******************************************************************************
* Name:
*    PrintGenRadData
*
* Description:
*    Print the necessary information in the Generic Digital Radar Data blocks
*
* Inputs:
*    short* data_ptr - pointer to start of radial (after message header)
*    int *offset - array of offsets to the various blocks in the radial
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRadData( char* data_ptr, int n_blocks, int *offsets )
{
   int ret = 0;
   int block_idx = 0;
   char block_id[5], *start_ptr = data_ptr;
   int block_len = 0;

   /* Process each data block */
   for ( block_idx = 0; block_idx < n_blocks; block_idx++ )
   {
      /* adjust data pointer to start of next block */
      data_ptr = start_ptr + offsets[block_idx];

      strncpy(block_id, data_ptr, 4);
      block_id[4] = '\0';

      if ( (ret = strcmp(block_id, "RVOL")) == 0 )
      {
         block_len = ((Generic_vol_t *)data_ptr)->len;
         PrintGenRadVol( (Generic_vol_t *)data_ptr );
      }
      else if ( (ret = strcmp(block_id, "RELV")) == 0 )
      {
         block_len = ((Generic_elev_t *)data_ptr)->len;
         PrintGenRadElev( (Generic_elev_t *)data_ptr );
      }
      else if ( (ret = strcmp(block_id, "RRAD")) == 0 )
      {
         block_len = ((Generic_rad_t *)data_ptr)->len;
         PrintGenRadRadial( (Generic_rad_t *)data_ptr );
      }
      else if ( ((ret = strcmp(block_id, "DREF")) == 0 ) ||
               ((ret = strcmp(block_id, "DVEL")) == 0 ) ||
               ((ret = strcmp(block_id, "DSW ")) == 0 ) ||
               ((ret = strcmp(block_id, "DZDR")) == 0 ) ||
               ((ret = strcmp(block_id, "DPHI")) == 0 ) ||
               ((ret = strcmp(block_id, "DRHO")) == 0 ) ||
               ((ret = strcmp(block_id, "DSNR")) == 0 ) )
      {
         block_len = PrintGenRadMoment( block_id, data_ptr );
      }


   } /* end for loop for blocks */

} /* end PrintGenRadData() */


/*******************************************************************************
* Name:
*    PrintGenRadVol
*
* Description:
*    Print and validate the information in the Generic Digital Radar Volume data
*    block.
*
* Inputs:
*    char* data_ptr - pointer to volume data block
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRadVol( Generic_vol_t* data_ptr )
{
   char block_id[5];
   int  msgLen = 0;

   if (VerboseLevel >= MONWB_MODERATE)
   {
      strncpy(block_id, (char *)&(data_ptr->type[0]), 4);
      block_id[4] = '\0';

      /* print volume data header */
      msgLen = sprintf(Message, "\n%sVolume Data (%s):",
         BlankString, block_id);
      PrintMsg(msgLen);

      /* print calibration constant */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Calibration Const. (dB)", data_ptr->calib_const);

      /* print vcp */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "VCP Number",
         data_ptr->vcp_num);

      /* print signal processor states */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Sig Proc States",
         data_ptr->sig_proc_states);
   }
      
   if (VerboseLevel >= MONWB_HIGH)
   {
      /* print lat */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Radar Latitude (deg)",
         data_ptr->lat);
      
      /* print lon */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Radar Longitude (deg)",
         data_ptr->lon);
      
      /* print height */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Radar Height (ft MSL)",
         (int) (data_ptr->height*M_TO_FT));
      
      /* print feedhorn height */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString,
         "Radar Feedhorn Height (ft AGL)", (int) (data_ptr->feedhorn_height*M_TO_FT));
      
      /* print horizontal channel transmitter power */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Horiz. Channel Trans. Power (kW)", data_ptr->horiz_shv_tx_power);
      
      /* print vertical channel transmitter power */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Vert. Channel Trans. Power (kW)", data_ptr->vert_shv_tx_power);
      
      /* print calibration of system ZDR */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "System Differential Reflectivity", data_ptr->sys_diff_refl);
      
      /* print initial system differential phase */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Initial System Differential Phase", data_ptr->sys_diff_phase);
   }

   if (VerboseLevel >= MONWB_EXTREME)
   {
      /* print block length */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Block Length",
         data_ptr->len);
      
      /* print major version */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Major Version",
         data_ptr->major_version);
      
      /* print minor version */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Minor Version",
         data_ptr->minor_version);
   }

} /* end PrintGenRadVol() */


/*******************************************************************************
* Name:
*    PrintGenRadElev
*
* Description:
*    Print and validate the information in the Generic Digital Radar Elevation 
*    data block.
*
* Inputs:
*    char* data_ptr - pointer to elevation data block
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRadElev( Generic_elev_t* data_ptr )
{
   char block_id[5];
   int  msgLen = 0;

   if (VerboseLevel >= MONWB_MODERATE)
   {
      strncpy(block_id, (char *)&(data_ptr->type[0]), 4);
      block_id[4] = '\0';

      /* print elevation data header */
      msgLen = sprintf(Message, "\n%sElevation Data (%s):",
         BlankString, block_id);
      PrintMsg(msgLen);

      /* print atmospheric attenuation factor */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Atmos. Atten. Factor (dB/km)",
         (float)data_ptr->atmos/1000.0);

      /* print calibration constant (dbz0) */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Calib. Const. (dBZ0)", data_ptr->calib_const);
   }

   if (VerboseLevel >= MONWB_EXTREME)
   {
      /* print block length */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString,
         "Block Length (bytes)", data_ptr->len);
   }

} /* end PrintGenRadElev() */


/*******************************************************************************
* Name:
*    PrintGenRadRadial
*
* Description:
*    Print and validate the information in the Generic Digital Radar Radial data
*    block.
*
* Inputs:
*    char* data_ptr - pointer to radial data block
*
* Outputs:
*    none
*
* Returns:
*    none
*******************************************************************************/
static void PrintGenRadRadial( Generic_rad_t* data_ptr )
{ 
   char block_id[5];
   int  msgLen = 0;

   if (VerboseLevel >= MONWB_MODERATE)
   {
      strncpy(block_id, (char *)&(data_ptr->type[0]), 4);
      block_id[4] = '\0';

      /* print radial data header */
      msgLen = sprintf(Message, "\n%sRadial Data (%s):",
         BlankString, block_id);
      PrintMsg(msgLen);

      /* print unambiguous range */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Unambiguous Range (km)",
         (float)data_ptr->unamb_range/10.0);

      /* print horizontal noise */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Horizontal Noise (dBm)", data_ptr->horiz_noise);

      /* print nyquist velocity */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Nyquist Velocity (m/s)",
         (float)data_ptr->nyquist_vel/100.0);
   }

   if (VerboseLevel >= MONWB_EXTREME)
   {
      /* print vertical noise */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
         "Vertical Noise (dBm)", data_ptr->vert_noise);

      /* print block length */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString,
         "Block Length (bytes)", data_ptr->len);
   }

   if (VerboseLevel >= MONWB_MODERATE)
   {
      if (data_ptr->len == (sizeof(Generic_rad_t)+sizeof(Generic_rad_dBZ0_t)))
      {
         Generic_rad_dBZ0_t *dbz0 = (Generic_rad_dBZ0_t *) 
                                    ((char *) data_ptr + sizeof(Generic_rad_t));

         /* print horizontal dBZ0 */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
            "Horizontal dBZ0 (dB)", dbz0->h_dBZ0);

         /* print vertical dBZ0 */
         fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString,
            "Vertical dBZ0 (dB)", dbz0->v_dBZ0);
      }

   }

} /* PrintGenRadRadial() */


/*******************************************************************************
* Name:
*    PrintGenRadMoment
*
* Description:
*    Print and validate the information in the Generic Digital Radar Moment data 
*    block.
*
* Inputs:
*    char* mom_id - string containing moment data identifier
*    char* data_ptr - pointer to moment data block
*
* Outputs:
*    none
*
* Returns:
*    integer len - the length of the moment data block
*******************************************************************************/
static int PrintGenRadMoment( char* mom_id, char* data_ptr )
{
   int len = 0;
   int msgLen = 0;
   int msg_size = 0;
   Generic_moment_t* mom_data_ptr = (Generic_moment_t *)data_ptr;

   if (VerboseLevel >= MONWB_HIGH)
   {
      /* print moment data header */
      msgLen = sprintf(Message, "\n%sMoment Data (%s):",
         BlankString, mom_id);
      PrintMsg(msgLen);
      
      /* print number of gates */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Number of Gates",
         mom_data_ptr->no_of_gates);

      /* print first gate range */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "First Gate Range (m)",
         mom_data_ptr->first_gate_range);

      /* print bin size */
      fprintf(stderr, MONWB_INT_FMT_STRING, BlankString, "Bin Size (m)",
         mom_data_ptr->bin_size);

      /* print threshold parameter */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Threshold Parameter (dB)",
         (float)mom_data_ptr->tover/10.0);

      /* print SNR threshold */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "SNR Threshold (dB)",
         (float)mom_data_ptr->SNR_threshold/8.0);

      /* print data word size */
      fprintf(stderr, MONWB_UINT_FMT_STRING, BlankString, "Data Word Size (bits)",
         mom_data_ptr->data_word_size);

      /* print scale value */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Scale Value",
         mom_data_ptr->scale);

      /* print offset value */
      fprintf(stderr, MONWB_FLOAT_FMT_STRING, BlankString, "Offset Value",
         mom_data_ptr->offset);
   }

   /* decide which type of data we're working with and also the length of
      the block */
   len = MONWB_SIZEOF_GEN_RAD_MOMENT;

   /* check to see if the scale is zero */
   if ( fabs(mom_data_ptr->scale - 0.0) < MONWB_ZERO_FLOAT_COMPARE)
   { 
      /* data must be floating point, scale and offset don't apply */
      len = len + (mom_data_ptr->no_of_gates * sizeof(float));
   }
   else
   {
      /* data is integer format */
      switch ( mom_data_ptr->data_word_size )
      {
         case 8:

            len = len + (mom_data_ptr->no_of_gates * sizeof(unsigned char));
            break;

         case 16:

            len = len + (mom_data_ptr->no_of_gates * sizeof(unsigned short));
            break;

         case 32:

            len = len + (mom_data_ptr->no_of_gates * sizeof(unsigned int));
            break;

         default:
            fprintf(stderr, "PrintGenRadMoment: invalid data word size (%d)\n",
               mom_data_ptr->data_word_size);
            break;
      }
   }

   /* Validate fields in Gen Rad moment block */
   if (ArchivedPlaybackFlagResp == MONWB_FALSE)
   {
      msg_size = MsgReadLen -  MONWB_COMMS_HDR_SIZE_BYTES - CTM_HDRSZE_BYTES;
   }
   else
   {
      msg_size = MsgReadLen;
   }
   return len;
} /* PrintGenRadMoment() */


/*******************************************************************************
* Function Name:
*	check_data_moment_name
*
* Description:
*	Performs a range check on a 4 character array representing the name of
*	a data moment block in the Generic Basedata message.  Note: because the
*       field is actually an array of correlated characters (versus a true
*       NULL terminated string), we have to tell the DEAU library that it's a
*       "byte" type.  This results in the library calling the function 4 times, 
*       each time pointing to the next character.  So to handle this properly in
*       this function ... 1) save the pointer address upon function entry (return
*       success); 2) upon 2nd and 3rd calls compute the address offsets to make
*       sure it's being called for the same string (return success); 3) upon 4th
*       and final call compute the address offset, check that we're still working
*       with the same char array, then proceed to do the validation on the 
*       char array; 4) finally return the actual "failure" or "success" return
*       var.
*       
* Inputs:
*	void *data_ptr  -  pointer to moment name
*
* Returns:
*	int : 0	= Failure - data does not conform to the ICD.
*             1	= Success - data conforms to ICD.
*
* History:
*	7/29/03	- Created, R. Solomon.
*******************************************************************************/
int check_data_moment_name (void *data_ptr)
{
   int          ret_val = 1;
   int          data_moment_name_len = 4;
   static char  buf[5];
   int          num_valid_names = 7;
   char*        valid_name[] = {"DREF", "DVEL", "DSW ", "DZDR", 
                                "DPHI", "DRHO", "DSNR"};
   int          name_idx = 0;
   static int*  new_addr = NULL;  /* new pointer address */
   static int*  prev_addr = NULL; /* previous pointer address */
   int          addr_diff; /* new_addr - prev_addr */
   static int   counter = 0;

   if ( counter == 0 )
   {
      /* first time fx is being called, save address, add char to str, incr
         counter */
      new_addr = (int*) data_ptr;
      *(buf + counter) = *(char*) data_ptr;
      counter++;
   }
   else 
   {
      /* not first time it's being called */
      prev_addr = new_addr;
      new_addr = (int*) data_ptr;
      addr_diff = (int) new_addr - (int) prev_addr;

      /* address difference should be 1 */
      if ( addr_diff != 1 )
      {
         ret_val = 0; /* failure */
         counter = 0;   /* reset counter */
      }
      else 
      {
         *(buf + counter) = *(char*) data_ptr;
         counter++;

         /* if we're reached the end of the array go ahead and compare
            the name against the array of valid names */
         if ( counter == data_moment_name_len )
         {
            /* add a NULL terminator to buf then compare  */
            buf[4] = '\0';

            ret_val = 0; /* reset ret_val to failure */

            for ( name_idx = 0; name_idx < num_valid_names; name_idx++ )
            {
               if ( strcmp(buf, valid_name[name_idx]) == 0)
               {
                  ret_val = 1;  /* found value, set return to success */
               }
            }
            counter = 0; /* reset counter for next time through */
         }
         else
         {
            /* still not done yet, have more chars coming */
            ret_val = 1; /* success */
         }
      }
   }

   return ret_val;

} /* end check_data_moment_name */

