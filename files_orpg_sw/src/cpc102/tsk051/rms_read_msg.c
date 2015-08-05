/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/09 21:50:45 $
 * $Id: rms_read_msg.c,v 1.10 2007/01/09 21:50:45 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
*/


/******************************************************************

	file: rms_read_msg.c

	This is the module for reading status messages. 
	
	
******************************************************************/
#include <rms_message.h>
#include <rda_status.h>
#include <gen_stat_msg.h>
#include <orpggst.h>
#include <rpg_clutter_censor_zones.h>

#define STATUS_TYPE	2
#define MAX_PATHNAME_SIZE	128
#define MAX_ID		4
/* Global variables */

int buf_count;
extern ushort reject_flag;
UNSIGNED_BYTE buf[MAX_BUF_SIZE];
UNSIGNED_BYTE clutter_buf[MAX_BUF_SIZE];
UNSIGNED_BYTE *buf_ptr;
static int Ev_read_msg;
int in_lbfd;
int out_lbfd;
int ack_flag;
static char input_name[MAX_PATHNAME_SIZE];
static char output_name[MAX_PATHNAME_SIZE];

void do_message();

static void En_callback (en_t evtcd, void *msg, size_t msglen);

/**************************************************************************
   Description:  This function reads the message sent from ORPG.
      
   Input:   None
      
   Output:  Messages from the output buffer

   Returns: None
   
   Notes:  

   **************************************************************************/
void rms_read_tst_msg(){

	/* clear the message buffer */
	init_buf(buf);

	/* Read the next message in the ORPG/RMS output LB */
	buf_count = LB_read (out_lbfd, (char*)buf, MAX_BUF_SIZE, LB_NEXT);

	if( buf_count != LB_TO_COME && buf_count > 0) {
		/* Set pointer to beginning of buffer */
		buf_ptr = buf;
		/* Process the message */
		do_message();
		}/* End if */
			
	else {
		/* Read the next message in the output LB */
		buf_count = LB_read (out_lbfd, (char*)buf, MAX_BUF_SIZE, LB_NEXT);
			
		if(buf_count != LB_TO_COME && buf_count > 0) 
			/* Set pointer to beginning of buffer */
			buf_ptr = clutter_buf;
			/* Process the message */
			do_message();
		}  /* End else */
			
	if (buf_count < 0) {
		
		if (buf_count != LB_TO_COME) {
			fprintf (stderr,	"Buf count = %d\n", buf_count);
			fprintf(stderr, "Output LB read failed.");
			} /* End if */

		} /* End if */

	}/* End rms_read_tst_msg */

/**************************************************************************
   Description:  This function builds an acknowledgment message to be sent
   to RMMS. It bypasses the port manager and simulates an FAA/RMMS connection.

   Input:   msg_struct - Pointer to a message header strucure.

   Output:  RMMS acknowledgement message.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
int build_read_ack(struct header *msg_header) {

	UNSIGNED_BYTE ack_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *buf_ptr;

	int ret_val;
	ushort num_halfwords = 25;
	int msg_size;

	/* Set the pointer to the beginning of the buffer */
	buf_ptr = ack_buf;

	/* Place pointer pas header */
	buf_ptr += MESSAGE_START;

	/* Type of message to be acknowledged */
	conv_ushort_unsigned(buf_ptr,&msg_header->message_type);
	buf_ptr += PLUS_SHORT;

	/* Sequence number of message */
	conv_ushort_unsigned(buf_ptr,&msg_header->seq_num);
	buf_ptr += PLUS_SHORT;

	/* Reject flag */
	conv_ushort_unsigned(buf_ptr,&reject_flag);
	buf_ptr += PLUS_SHORT;

	/* Reset reject flag for next message */
	reject_flag =0;

	/* Add terminator to the message */
	add_terminator(buf_ptr);
	buf_ptr += PLUS_INT;

	/* Compute the message size */
	msg_size = (buf_ptr - ack_buf);

	/* Pad the message with zeros */
	pad_message (buf_ptr, msg_size, MAX_PAD_SIZE);

	/* Add the header to the message */
	ret_val = build_header(&num_halfwords, 37, ack_buf, 0);

	if (ret_val != 1){
		fprintf (stderr,
			"RMS build header failed for rms ack");
		return (-1);
		} /* End if */

	fprintf(stderr,"Sent acknowledgement for message number %d\n",msg_header->seq_num);

	/* Write the acknowledgement message into the input LB of the ORPG/RMS*/
	ret_val = LB_write(in_lbfd ,(char*)&ack_buf, MAX_BUF_SIZE, LB_ANY);

	if ( ret_val!= MAX_BUF_SIZE  ){
		(void) fprintf (stderr, "Output LB write failed.");
		printf ("error code %d \n", ret_val);
		return (-1);
		} /* End if */

	/* Post the event so the ORPG/RMs knows a message is in the input LB */
	ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

	return (1);
}/* End of send_ack*/

/**************************************************************************
   Description:  The main function of read message reads the messages placed
   in the output LB by the ORPG/RMS and prints it in a form readable by the user.
   This function will also generate an acknowledgement message to simulate an
   FAA/RMMS connection.

   Input:   None

   Output:  Readable format of the ORPG/RMS messages.

   Returns: None

   Notes:

   **************************************************************************/
int main() {

	int ret;
	int interface_down;
	interface_down = 1;

	/* Set up linear buffers for read*/

	/* Get the work directory path for the input LB */
	ret = MISC_get_work_dir (input_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);

   		 }
	/* Concatonate the input LB name to the path */
	strcat (input_name, "/rms_input.lb");


	/* Get the work directory path for the output LB */
	ret = MISC_get_work_dir (output_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);

   		 }
	/* Concatonate the output LB name to the path */
	strcat (output_name, "/rms_output.lb");

	/* Get the LB id of the input LB */
	in_lbfd = LB_open ( input_name, LB_WRITE, NULL);
		if (in_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS input linear buffer (%d).\n",
                   		    in_lbfd );
			}

	/* Get the LB id of the output LB */
	out_lbfd = LB_open ( output_name, LB_READ, NULL);
		if (out_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS output linear buffer (%d).\n",
                   		    out_lbfd );
			}

	/* Determine if the user wants acknowledgment messages sent */
	printf("\nSend acknowledgement messages? (Yes = 1 / No = 0)  ");
       	scanf("%d", &ack_flag);
	
       	/* If no set the flag to zero */
	if (ack_flag != 1)
       		ack_flag = 0;

	/* Set the event flag to zero */
	Ev_read_msg = 0;

	/*Register for event ORPGEVT_SEND_MSG */
   	if( (ret = EN_register( ORPGEVT_RMS_SEND_MSG, (void *) En_callback )) < 0 )
	fprintf( stderr, "Failed to Register ORPGEVT_SEND_MSG rms read (%d).\n", ret );

	while (1) {


		/* Read message callback flag set */
		if(Ev_read_msg){
			/* Clear the flag */
			Ev_read_msg = 0;
			/* Read the message */
			rms_read_tst_msg();
			}/* End if */

		/* Block the control function */
		LB_NTF_control (LB_NTF_BLOCK);

              	                /* No message ready to read */
		if( Ev_read_msg == 0){
			/* Wait one second */
			LB_NTF_control(LB_NTF_WAIT, 1000);
			}/* End if */
		else{
			/* Message ready to read continue */
			LB_NTF_control (LB_NTF_UNBLOCK);
			}/* End else */

        	        } /* End loop */
	return 0;
} /* End main*/

void do_message(){

	int ret;
	int i, k;

	int temp_int;
	int msg_count;
	int checksum;

	struct header msg_header;
	struct header *header_ptr;

	short temp_short;


	char temp_string_6[6] = "      ";
	char temp_string_4[4] = "    ";
	char blank_6[6] = "      ";
	char blank_4[4]= "    ";
	char user_string[4];
	char temp_string[1600];
	char temp_char;
	float temp_float;



	/* Set the pointer to the header structure */
	header_ptr = &msg_header;

	/* Get header information */
	strip_header((UNSIGNED_BYTE*)buf,header_ptr);

	/* Get checksum for the message */
	checksum = rms_checksum(buf);

	fprintf(stderr,"\n-----------------------------------\n");
	fprintf(stderr,"Received from ORPG message number %d\n",msg_header.seq_num);
	fprintf(stderr,"Checksum for message number %d = %d\n",msg_header.seq_num, msg_header.checksum);

	switch(msg_header.message_type){

		case 1: /* ORPG State message */

			fprintf(stderr,"Message type %d : RPG State Message \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/*Rpg equipment operability status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			printf("\nRPG State = %d\n", temp_short);

			break;

		case 2: /* Status message */

			fprintf(stderr,"Message type %d : RPG Status Message \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */
			 
	        	 /* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/*Rpg equipment operability status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nRPG operability = %d\n", temp_short);

			/*Spare ( HALFWORD )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;


			/*Rpg channel control state*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RPG channel control state = %d\n", temp_short);

			/*Rda channel control status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA channel control status = %d\n", temp_short);

			/*Spare ( HALFWORD )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			/*Narrowband/Wideband relay status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Relay status = %d\n", temp_short);
		
			/*Auto PRF enabled, disabled*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Auto PRF = %d\n", temp_short);
		
			/*Weather mode*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Weather mode = %d\n", temp_short);
		
			/*Spare ( HALFWORD )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			/*Current rpg volume coverage pattern*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Volume coverage pattern = %d\n", temp_short);
		
			/*Rpg vcp start time*/
			temp_int = conv_intgr(buf_ptr);
			buf_ptr += PLUS_INT;
			printf("Vcp start time = %d\n", temp_int);
		
			/*Current rpg elevation cut*/
			temp_float = conv_real(buf_ptr);
			buf_ptr += PLUS_INT;
			printf("Elevation cut = %f\n", temp_float);
					/*Spare ( HALFWORD )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			/*Current rpg cpu utilization level*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Cpu utilization = %d\n", (ushort)temp_short);
		
			/*Current rpg shared memory utilization level*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Share memory utilization = %d\n", temp_short);
		
			/*Current rpg on-line storage utilization level*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("On-line storge utilization = %d\n", temp_short);
		
			/*Current rpg input buffer utilization level*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Input buffer utilization = %d\n", temp_short);
		
			/*Spare ( HALFWORD )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			/*Current rpg narrowband line utilization level for NB#1 - 32*/
			for (i=0; i<=MAX_FAA_LINES -1; i++){
				temp_short = conv_shrt(buf_ptr);
				buf_ptr += PLUS_SHORT;
				if(temp_short != 0){
					printf("Narrowband %d utilization = %d\n", i, temp_short);
					} /* End if */
			        } /* End for loop */
		
			/*Spares ( 4 HALFWORDS )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			/*Narrowband line with the highest utilization percentage*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Nb line highest utilization = %d\n", temp_short);
		
			/*Wideband communication status for WB#1*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("WB1 status = %d\n", temp_short);
		
			/*Wideband communication status for WB#2*/
			/* Change to WB2 status when informaiton found*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("WB2 status = %d\n", temp_short);
		
			/*Spares ( 2 HALFWORDS )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			/*Wideband logical status for WB#1*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("WB1 logical status = %d\n", temp_short);
					/*Wideband logical status for WB#2*/
			temp_short = conv_shrt(buf_ptr);/* temp_short to be replaced */
			buf_ptr += PLUS_SHORT;
			printf("WB2 logical status = %d\n", temp_short);
		
			/*Spares ( 2 HALFWORDS)*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			
			/*Narrowband status for NB#1 - 32*/
			for (i=0; i<=(MAX_FAA_LINES -1); i++){
				temp_short = conv_shrt(buf_ptr);
				buf_ptr += PLUS_SHORT;
				if(temp_short != 0){
					printf("Narrowband %d status = %d\n", (i +1), temp_short);
					} /* End if */
			        } /* End for loop */
		
			/*Spares ( 4 HALFWORDS )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
	
			/*Rda status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA status = %d\n", temp_short);
		
			/*Rda operability status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA operability status = %d\n", temp_short);
		
			/*Rda control status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA control status = %d\n", temp_short);
		
			/*Aux power gen state*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Aux power gen state = %d\n", temp_short);
		
			/*Average transmitter power (watts)*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("transmitter power = %d\n", temp_short);
		
			/*Reflectivity calibration correction*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Reflectivity = %d\n", temp_short);
		
			/*Data transmission enabled*/
		      	temp_short = conv_shrt(buf_ptr);
		      	buf_ptr += PLUS_SHORT;
			printf("Data transmission = %d\n", temp_short);
		
			/*Rda volume coverage pattern identifier*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Volume coverage pattern = %d\n", temp_short);
		
			/*Rda control authorization*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA control authorization = %d\n", temp_short);
			
			/*Interference detection rate*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Interferecne detection rate = %d\n", temp_short);
		
			/*Operational mode*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA operational mode = %d\n", temp_short);
		
			/*Isu enabled, disabled*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("ISU enabled = %d\n", temp_short);
		
			/*Archive II as received from rda*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Arcive II = %d\n", temp_short);
					/*Archive II estimated remaining capacity*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Arch II remaining capacity = %d\n", temp_short);
		
			/*Spares ( 2 HALFWORDS )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			/*Channel control status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Channel control status = %d\n", temp_short);
		
			/*Spot blanking status*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Spot blanking status = %d\n", temp_short);
			
			/*Bypass map generation date*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Bypass map date = %d\n", temp_short);
			
			/*Bypass map generation time*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Bypass map time = %d\n", temp_short);
				
			/*Notchwidth map generation date*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Notchwidth map date = %d\n", temp_short);
		
			/*Notchwidth map generation time*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Notchwidth map time = %d\n", temp_short);

			/*Spares ( 3 HALFWORDS )*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
		
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;

			break;
		
		case 3: /* Alarm message */
			
			fprintf(stderr,"Message type %d : Alarm Message \n",msg_header.message_type);
			
			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
		
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */
		 				
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;      
		
			/* RPG alarm state */						
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nRPG alarm state = %d\n", temp_short);
		
			/* RPG alarms maintenance required */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RPG maintenance required = %d\n", temp_short);
		
			/* RPG alarms maintenance mandatory*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RPG maintenance mandatory = %d\n", temp_short);
		
			/* RPG alarms  load shedding*/
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RPG load shedding = %d\n", temp_short);

			/* WB1 alarm status */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("WB1 alarm status = %d\n", temp_short);

			/* WB2 alarm status */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("WB2 alarm status = %d\n", temp_short);

			/* RDA alarm summary */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("RDA alarm summary = %d\n", temp_short);

			/* RDA alarm codes */
			for (i=0; i<14; i++) {
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Alarm code %d = %d\n", i, temp_short);
			        } /* End loop*/
			
			break;
			
		case 13: /* Free text message*/
		
			fprintf(stderr,"Message type %d : Free Text Message \n",msg_header.message_type);
			
			if(ack_flag == 1){
			        /* Send akcnowledgement message */
			        ret = build_read_ack(&msg_header);

			        if (ret != 1)
			        	fprintf (stderr,"Error sending acknowledgement message" );
			        }  /* End if */
			
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;  
			
			/* Halfwords */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nNumber of halfwords = %d\n", temp_short);	   
			
			/* Compute the number of bytes in message */
			msg_count = (int)((temp_short - 2) * 2);

			/* Free text string*/
               		for (i=0; i<=(msg_count * 2); i++){
       				conv_char (buf_ptr, &temp_string[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
        			        } /* End loop */

        		                printf("Free text message = %s\n", (char*)temp_string);


			break;

		case 16: /* PUP id's */

			fprintf(stderr,"Message type %d : PUP Ids/Comline Message \n",msg_header.message_type);

                        		if(ack_flag == 1){
	        		        /* Send acknowledgement message */
			        ret = build_read_ack(&msg_header);

			        if (ret != 1)
				fprintf (stderr,"Error sending acknowledgement message" );
			        } /* End if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Line number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nLine number = %d\n", temp_short);

			/* User id */
   			 strcpy ( temp_string_4, blank_4);

               		for (i=0; i<=3; i++){
               			conv_char (buf_ptr, &temp_string_4[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
       				} /* End loop */
			printf("PUP id = %s\n", (char*)temp_string_4);

			break;

		case 19: /* Edit clutter zones */

			fprintf(stderr,"Message type %d : Clutter Suppression Regions \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
		
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );	
				} /* End if */
			
			
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;      
		
			/* VCP number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nVCP number = %d\n", temp_short);

   	   		/* For each of the maximum number of zones */
			for (i=0; i<MAX_NUMBER_CLUTTER_ZONES; i++) {
              			        /* Start range */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Start range %d  = %d\n", i, temp_short);

              			        /* End range */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("End range %d  = %d\n", i, temp_short);

			        /* Start azimuth */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Start azimuth %d  = %d\n", i, temp_short);

			        /* Stop azimuth */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Stop azimuth %d  = %d\n", i, temp_short);

			        /* Elevation segment */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Elevation segment %d  = %d\n", i, temp_short);

			        /* Operation select code */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Operation select code %d  = %d\n", i, temp_short);

			        /* Channel D width */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf("Channel D width %d  = %d\n", i, temp_short);

			        /* Channel S width */
			        temp_short = conv_shrt(buf_ptr);
			        buf_ptr += PLUS_SHORT;
			        printf ("Channel S width %d  = %d\n", i, temp_short);
			        }/* End for loop */

			break;

		case 22: /* Bypass map */
		
			fprintf(stderr,"Message type %d : Clutter Filter Bypass Maps \n",msg_header.message_type);
			
			if(ack_flag == 1){
	        	        	        /* Send acknowledgement message */
			        ret = build_read_ack(&msg_header);

		         	        if (ret != 1)
				fprintf (stderr,"Error sending acknowledgement message" );
			        } /* End if */
		
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;      
		
			/* Segment */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
	               	printf("\n\nSegment = %d \n",temp_short);

	               	/* Radial */
			temp_short = conv_shrt(buf_ptr);
	               	buf_ptr += PLUS_SHORT;
			printf("Radial = %d \n",temp_short);

		        /* For each of the 31 bins */
		        for (k=0; k<=31; k++){

                		temp_short = conv_shrt(buf_ptr);
	               	buf_ptr += PLUS_SHORT;

	               	if ((k % 6) != 0){
	               	        if (temp_short !=0)
	               		printf("Bin %d  = %d\t", k, temp_short);
	               		} /* End if */
		             else{
		                        /* If the sixth bin add newline */
			        if (temp_short !=0)
	        	                      printf("Bin %d  = %d\n", k, temp_short);
	               	        } /* End else */
	               			
	               	 } /* End for loop */
        	        	
 			break;
			
		case 25: /* Load Shed message */
		
			fprintf(stderr,"Message type %d : Load Shed Catagories \n",msg_header.message_type);
			
			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* end if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;      
		
			/* Line Number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nLine Number = %d\n", temp_short);	   	
   			 
   			 /* Warning threshold */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Warning threshold = %d\n", temp_short);	
							            		
              			/* Alarm threshold */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Alarm threshold = %d\n", temp_short);

			break;
		
		case 28: /* Narrowband reconfiguration command */
		
			fprintf(stderr,"Message type %d : Narrowband Reconfiguration Control \n",msg_header.message_type);
			
			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */
			
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;      
		
			/* Row Number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nRow Number = %d\n", temp_short);
				   	
   			 /* Line Number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nLine Number = %d\n", temp_short);

                        		/* Place pointer past Mneumonic */
			buf_ptr += PLUS_SHORT;
			buf_ptr += PLUS_SHORT;

                        		/* Line class */
			strcpy ( temp_string_6, blank_6);
                        		for (i=0; i<=5; i++){
                                        		conv_char (buf_ptr, &temp_string_6[i],PLUS_BYTE);
                                                        	buf_ptr += PLUS_BYTE;
                                                                } /* End loop */

                                          printf("Line class = %s\n", temp_string_6);

                                          	/* Baud rate */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nBaud rate = %d\n", temp_short);

			/* Line type*/
			strcpy ( temp_string_6, blank_6);
                        		for (i=0; i<=5; i++){
                                        		conv_char (buf_ptr, &temp_string_6[i],PLUS_BYTE);
                                                        	buf_ptr += PLUS_BYTE;
                                                                } /* End loop */
                                          	printf("Line type = %s\n", temp_string_6);

			/* Password */
			strcpy ( temp_string_4, blank_4);
                        		for (i=0; i<=3; i++){
       				conv_char (buf_ptr, &temp_string_4[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
                                		} /* End loop */
                                           printf("Password = %s\n", temp_string_4);

                                           /* Distribution method */
                                           strcpy ( temp_string_4, blank_4);
                                           for (i=0; i<=3; i++){
                                                conv_char (buf_ptr, &temp_string_4[i],PLUS_BYTE);
                                                buf_ptr += PLUS_BYTE;
                                		} /* End loop */
			printf("Distribution method = %s\n", temp_string_4);

                                                 /* Time limit */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Time limit = %d\n", temp_short);	
					
			break;
				
		case 31: /* Narrowband dial in reconfiguration command */
		
			fprintf(stderr,"Message type %d : Narrowband Dial in Reconfiguration \n",msg_header.message_type);
			
			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Row Number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nRow Number = %d\n", temp_short);

   			 /* Line Number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nLine Number = %d\n", temp_short);

			/* Password */
              			for (i=0; i<=4 - 1; i++){
       				conv_char (buf_ptr, &temp_string_4[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
                                		} /* End loop */
			printf("Password = %s\n", (char*)temp_string_4);

                                            /* Distribution method */
              			for (i=0; i<=4 - 1; i++){
       				conv_char (buf_ptr, &temp_string_4[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
                                		} /* End loop */
			printf("Distribution method = %s\n", (char*)temp_string_4);

   			 /* Time limit */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Time limit = %d\n", temp_short);

			break;

		case 34: /* Authorized user */

			fprintf(stderr,"Message type %d : Narrowband Authorized User Control \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Line number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nLine number = %d\n", temp_short);

                        		/* User id */
                                        	/* Get user id in temporary string*/
       			for(i=0; i < MAX_ID; i++){
                			conv_char (buf_ptr, &user_string[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
              				} /* End loop */

        			/* Convert temporary id string to an integer*/
       			 temp_short = (short) atoi(user_string);
			printf("User id = %d\n", temp_short);

                        		/* Password */
              			for (i=0; i<=6 - 1; i++){
       				conv_char (buf_ptr, &temp_string_6[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
                                		} /* End loop */
			printf("Password = %s\n", (char*)temp_string_6);

                                            /* Override */
			conv_char(buf_ptr, &temp_char,PLUS_BYTE);
			buf_ptr += PLUS_BYTE;
			printf("Override = %c\n", temp_char);

			break;

		case 36: /* Record log message */

			fprintf(stderr,"Message type %d : System Status Log Record Message \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */
                                        /* This is commented out because the number of log messages printed makes
			it almost impossible to see anything else */
			/* Place pointer at begining of message*/
			/*buf_ptr += MESSAGE_START;*/

			/* Free text string*/
               		/*for (i=0; i<=79; i++){
       				conv_char (buf_ptr, &temp_string[i],PLUS_BYTE);
       				buf_ptr += PLUS_BYTE;
        			                }

        		                for (i=0; i<=79; i++){

        			        if ((temp_string[i] >= 48) && (temp_string[i] <= 122)){
       					printf(" %c", temp_string[i]);
					}

				if( i % 14 == 0){
					printf(" %c", '\n');
					}
        			        }*/

			break;

		case 37: /* ACK message */

			fprintf(stderr,"Message type %d : Acknowledgement Message \n",msg_header.message_type);

			/* Do not send an acknowledgement for an ack message */

                		/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Message type */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nMessage type = %d\n", temp_short);

              			/* Sequence number */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Sequence Number = %d\n", temp_short);

              			/* Ack code */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("Acknowledgement code = %d\n", temp_short);

			break;

		case 38: /* Inhibit  message */

			fprintf(stderr,"Message type %d : Inhibit Message \n",msg_header.message_type);

			if(ack_flag == 1){
				/* Send acknowledgement message */
				ret = build_read_ack(&msg_header);
				if (ret != 1)
					fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */

			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Inhibit Time */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nInhibit time = %d\n", temp_short);

			break;

		case 39: /* RMS UP message */

			fprintf(stderr,"Message type %d : RMS Up Message \n",msg_header.message_type);

			if(ack_flag == 1){
			        /* Send acknowledgement message */
			        ret = build_read_ack(&msg_header);
                                                   if (ret != 1)
				fprintf (stderr,"Error sending acknowledgement message" );
			        } /* End if */

			printf("\nseq num = %d\n", msg_header.seq_num);
			/* Place pointer at begining of message*/
			buf_ptr += MESSAGE_START;

			/* Message type */
			temp_short = conv_shrt(buf_ptr);
			buf_ptr += PLUS_SHORT;
			printf("\nRetrys = %d\n", temp_short);

			if(ack_flag == 1){
			        /* Send acknowledgement message */
			        ret = build_read_ack(&msg_header);
        			        if (ret != 1)
				fprintf (stderr,"Error sending acknowledgement message" );
				} /* End if */
			break;

		default:
			break;
		} /* End switch */

		return;
}/* End process message */
/**************************************************************************
   Description:  This is the call back routine for the ORPG send message event.
   When received it sets the flag letting main know htere is a message to process.

   Input:   Event, message, message size

   Output:  Sets flag

   Returns: None

   Notes:

   **************************************************************************/
void En_callback (en_t evtcd, void *msg, size_t msglen){

	switch(evtcd){
		/* Whenever ORPG/RMS sends a message call the read function */
		case ORPGEVT_RMS_SEND_MSG:
			/* If a message is already being processed redeliver this message */
			if(Ev_read_msg){
				LB_NTF_control(LB_NTF_REDELIVER);
				break;
				} /* End if */
			/* Set the flag */
			Ev_read_msg = 1;
			break;

		default:
			break;

		}/* End switch */

}/* End En_callback */


