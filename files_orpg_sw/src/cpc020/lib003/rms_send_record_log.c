/**************************************************************************
   
   Module:  rms_send_record_log_msg.c	
   
   Description:  This module sends a status record log message to RMMS.  
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/05/27 17:04:25 $
 * $Id: rms_send_record_log.c,v 1.22 2003/05/27 17:04:25 garyg Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <rda_rpg_console_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define MAX_NAME_SIZE		256
#define LOG_TYPE				36
#define MAX_LOG_SIZE			79

/*
* Static Globals
*/
char temp_string[LE_MAX_MSG_LENGTH];
int rms_connection_down;
extern int record_lbfd;

/*
* Static Function Prototypes
*/

static int rms_send_log();

/**************************************************************************
   Description:  This function write the string to the message buffer.
      
   Input:

   Output: System log message sent to FAA/RMMs or stored in record log LB

   Returns:  None.
   
   Notes:  

   **************************************************************************/
   
void rms_send_record_log_msg ( int fd,  LB_id_t msgid,  int msg_info,  void *arg) {

	int day ;
    	int hr ;
    	int min ;
    	int month ;
    	int sec ;
    	int year ;
   	int yr ;

	char	*string_start;
   	const	UNSIGNED_BYTE	blank = 0;
	char	msg [LE_MAX_MSG_LENGTH];
	LE_critical_message	*le_msg;
	int	ret, i;

	/* Read the next system log message */
	ret = ORPGDA_read (ORPGDAT_SYSLOG,
			   msg,
			   LE_MAX_MSG_LENGTH,
			   LB_NEXT);

	/* Clean out string */
	for (i=0; i<LE_MAX_MSG_LENGTH; i++){
		memcpy(&temp_string[i], &blank, 1);
		} /* End loop */

	if (ret <0 ) {
		LE_send_msg(RMS_LE_ERROR , "RMS: Failed read status log message (%d).\n", ret );
		} /* End if */

	else {
		/* Set pointer to beginning of message buffer */
		le_msg = (LE_critical_message *) msg;

		/* Get the unix time */
		(void) unix_time((time_t *) &le_msg->time,&year,&month,&day,&hr,&min,&sec) ;
    		yr = year % 100 ;

    		/* Set tring past system log message sender */
		string_start = strstr (le_msg->text,":");

		if (string_start == NULL) {
			string_start = le_msg->text;
			} /* End if */
		else {
			string_start = string_start + 2;
			} /* End else */

		/* Convert to ASCII the time and date */
		sprintf(temp_string,"%02d/%02d/%02d %02d:%02d:%02d ", month,day,yr,hr,min,sec) ;

		/* Concatenate the day and time to the system log message */
		strcat (temp_string, string_start);

		/* If the interface is down place the log message in the record log buffer */
		if(rms_connection_down == 1){

			/* Write the system log message in the record log buffer for later transmission to the FAA/RMMS */
			ret = LB_write (record_lbfd,
					temp_string,
					sizeof (temp_string),
					LB_ANY);

			if(ret <0)
				LE_send_msg(RMS_LE_ERROR , "RMS: Failed to write status log message (%d).\n", ret );

			} /* End if */
		else {

			/* Send the message to the FAA/RMMS */
			ret = rms_send_log();

   			if( ret < 0 )
   				LE_send_msg (RMS_LE_ERROR,"RMS: RMS send record log failed (ret %d)", ret);

			}/* End else */

  		}/*End else */
} /*End rms send free text msg */

/**************************************************************************
   Description:  This function writes the string to the message buffer.

   Input: 	string_ptr - Pointer to the message string.

   Output: None

   Returns: 0 = Successful completion

   Notes:

   **************************************************************************/

static int rms_send_log(){

	UNSIGNED_BYTE record_log_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *record_log_buf_ptr;

	int ret, i, msg_size;

	ushort num_halfwords;

	/*Place buffer ptr at beginning of record log */
	record_log_buf_ptr = record_log_buf;

   	/* Set the pointer past the header */
	record_log_buf_ptr += MESSAGE_START;

	/* Put string in output buffer */
	for (i=0; i<=MAX_LOG_SIZE; i++){
		if(temp_string[i] == 10)
			temp_string[i] = 0;
		conv_char_unsigned(record_log_buf_ptr, &temp_string[i], PLUS_BYTE);
		record_log_buf_ptr += PLUS_BYTE;
		} /* End loop */

	/* Compute the message size */
	msg_size = (record_log_buf_ptr - record_log_buf);

	/* Add the terminator to the message */
	add_terminator(record_log_buf_ptr);
	record_log_buf_ptr += PLUS_INT;

	/* Compute the number of halfwords in the message */
	num_halfwords = ((record_log_buf_ptr - record_log_buf) / 2);

	/* Add the header to the message */
	ret = build_header(&num_halfwords, LOG_TYPE, record_log_buf, 0);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
		"RMS: RMS build header failed for rms record log message");
		return(ret);
		} /* End if */

	/* Send the message to the FAA/RMMS */
	ret = send_message(record_log_buf,LOG_TYPE,RMS_STANDARD);

	if (ret != 1){
		LE_send_msg (RMS_LE_ERROR,
		"RMS: Send message failed (ret %d) for rms send record log message", ret);
		return (ret);
		} /* End if */

	return (0);
}/* End rms send log */

/**************************************************************************
   Description:  This function writes all buffered messages to RMMS.

   Input: None

   Output: Send the buffers record logs to the FAA/RMMS.

   Returns: None

   Notes:

   **************************************************************************/


void rms_clear_record_log(){

	int ret, i;


	/* Send only five messages at a time to keep from overwhelming the FAA/RMMS */
	for (i= 0; i<= 5; i++){

		/* Read a message from the record log buffer */
		ret = LB_read(record_lbfd,(char*)&temp_string, sizeof(temp_string),LB_NEXT);

		if(ret < 0 ){

			/* Failed to read the buffer */
			if(ret != LB_TO_COME){
				LE_send_msg(RMS_LE_ERROR, "RMS: Failed to to read record status log (%d).\n", ret );
				} /* End if */
			else{
				/* Read all the messages */
				LE_send_msg(RMS_LE_LOG_MSG, "RMS: Finished sending record log messages.\n");
				} /* End else */

			/* Deregister the timer that sends the record log messages */
			if ((ret = MALRM_deregister(MALRM_SEND_RMS_STATLOG)) < 0) {
				LE_send_msg(RMS_LE_ERROR, "RMS: Failed to deregister MALRM_SEND_RMS_STATLOG (%d).\n", ret );
				} /* End if */
			else {
				LE_send_msg(RMS_LE_LOG_MSG, "RMS: MALRM_SEND_RMS_STATLOG deregistered.\n");
				} /* End else */

			/* Clear the record log LB */
			ret = LB_clear(record_lbfd,LB_ALL);

			if (ret < 0 ){
				LE_send_msg (RMS_LE_ERROR,"RMS: RMS clear record log failed (ret %d)", ret);
				} /* End if */

			/* Done reading record log messages or LB unreadable so leave loop */
			break;

			}/* End main if */

		/* A message was found so send it to the FAA/RMMS */
		ret = rms_send_log();

		if( ret < 0 ){
   			LE_send_msg (RMS_LE_ERROR,"RMS: RMS send record log failed (ret %d)", ret);
   			break;
			} /* End if */

		}/* End for loop */

}/* End rms clear record log */







