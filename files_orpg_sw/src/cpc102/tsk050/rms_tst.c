/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:41:59 $
 * $Id: rms_tst.c,v 1.12 2014/03/18 18:41:59 jeffs Exp $
 * $Revision: 1.12 $
 * $State: Exp $
*/


/******************************************************************

	file: rms_tst.c

	Test messages for the interface. 
	
	
******************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <rms_util.h>
#include "rms_tst.h"
#include <infr.h>
#include <rms_ptmgr.h>
#include <rms_message.h>
#include <orpgred.h>
#include <orpgedlock.h>
#include <orpgerr.h>

#define MAX_LOG_SIZE		90
#define MAX_ENTRY_SIZE 		20
#define MASK 			32767
#define MAX_CLUTTER_REGIONS	14
#define MAX_BIN_NUMBER		32
#define BLANK			0
#define PASSWORD_SIZE		6
#define MAX_PATHNAME_SIZE	128
#define RMS_PASSWORD1  		"WXMAN1"
#define RMS_PASSWORD2		"WXMAN2"
#define RMS_PASSWORD3  		"WXMAN3"
#define LOG_TYPE			36
#define MAX_ID			4

extern struct resend *resend_array_ptr;
extern int resend_cnt;
extern ushort rms_msg_seq_number;
static char input_name[MAX_PATHNAME_SIZE];
static char output_name[MAX_PATHNAME_SIZE];
static char record_name[MAX_PATHNAME_SIZE];

int arch_num, scan_num, segment_number, rad_num;
void callback();

int main() {
	
	
	struct ack_message{
		ushort ack_msg_type;
		ushort tst_seq_num;	
		ushort ack_code;
		ushort feedback_msg_size;
		char feedback_msg[80];
		};/* acknowledgement message structure*/
		
	
	struct clutter_region{
		ushort start_range;
		ushort end_range;	
		ushort start_azmith;
		ushort end_azmith;
		ushort elv_seg;
		ushort select_code;
		ushort channel_d_width;
		ushort channel_s_width;
		};/* clutter suppression region structure*/
	
	
	struct bypass_map{
		int seg_num;
		int radial_num;
		int bin[MAX_BIN_NUMBER];
		};
		
		
	struct ack_message ack_msg;
	struct header header_message;
	struct header *header_ptr;
	
	struct clutter_region *clutter_zone_ptr;
	struct bypass_map *bypass_map_ptr;
	
	char clear [2] = " A";
	char severe [2] = " B";
	char temp_string[20] = "                   ";
	char free_temp_string [400];
	char string_five [5];
	char blank_char = ' ';
	char msg_number[MAX_ENTRY_SIZE];
	char first_entry[MAX_ENTRY_SIZE];
	char second_entry[MAX_ENTRY_SIZE];
	char third_entry[MAX_ENTRY_SIZE];
	char fourth_entry[MAX_ENTRY_SIZE];
	char inhibit_time;
	char temp_char = 'N';
	char password_string[6];
        	char user_string[4];
	char id[4]     = "    ";
	char sset[4]   = "SSET";
	char rset[4]   = "RSET";
	char onetim[4] = "1TIM";
	char comb[4]   = "COMB";
	char napup[6]  = "NAPUP ";
	char other[6]  = "Other ";
	char rgdac[6]  = "RGDAC ";
	char rpgop[6]  = "RPGOP ";
	char dedic[6]  = "DEDIC ";
	char dialin[6] = "DIALIN";
	char dinout[6] = "DINOUT";
	char rfc[6]    = "RFC   ";
	char apup[6]   = "APUP  ";
	char blank6[6] = "      ";
	char blanks[2] = "  ";
	char blank = ' ';
	
	int wx_flag;
	int in_num = 0;
	int close_num = 0;
	int test_number =0;
	int int_num =0;
	int temp_type;
	int temp_int1;
	int temp_int2;
	int temp_int3;
	int temp_int4;
	int temp_int5;
	int temp_int6;
	int line_num;
	int msg_length = 7;
	int ret_val = 0;
	int write_pos =0;
	int read_pos =0;
	int jul_date, milseconds;
	int msg_size;
	int rtn_length;
	int i, ret, j;
	int file_num;
	int zone_num;
	int segment_num;
	int radial_num_1;
	int radial_num_2;
	int bin_num_1;
	int bin_num_2;
	int user_id;
	int day ;
    	int hr ;
    	int min ;
    	int month ;
    	int sec ;
    	int year ;
   	int yr ;
	
	time_t msg_time;
   	
	ushort msg_num;		
	ushort temp_ushort =0;
	ushort ushort_num =0;
	ushort fb_num;
	ushort temp_seq = 23;
	ushort temp_msg_num = 37;
	ushort temp_reject_flag = 24;	
	ushort short_type;
	ushort temp_short1;
	ushort temp_short2;
	ushort temp_short3;
	ushort temp_short4;
	ushort num_halfwords = 25;
	ushort message_size;
	ushort message_size_halfwords;
	
		
	short string_size;
	short temp_short =0;
	short short_num = 0;
	short edit_auth_user;
	short autocal_override;
	
	
	
	UNSIGNED_BYTE ack_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE *buf_ptr;
	UNSIGNED_BYTE *write_ptr;
	UNSIGNED_BYTE *read_ptr;
	UNSIGNED_BYTE write_buf[MAX_BUF_SIZE];
	UNSIGNED_BYTE read_buf[MAX_BUF_SIZE];
	
        	/*char	msg [LE_MAX_MSG_LENGTH];*/
		
	RDA_RPG_comms_status_t wb_comms_status;
	
	time_t tm;
	int in_lbfd;
	int out_lbfd;
	int record_lbfd;
	
	/* Set pointers */
	write_ptr = write_buf;
	read_ptr = read_buf;
	header_ptr = &header_message;
	
	rms_msg_seq_number = 1;

	/* Get the pathname of the work directory */
	ret = MISC_get_work_dir (input_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
		 } /* End if */

   	/* Add the name of the input LB to the path */
   	strcat (input_name, "/rms_input.lb");	 	
   		 
   	/* Get the LB id of the input LB */
	in_lbfd = LB_open ( input_name, LB_WRITE, NULL);
		if (in_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS input linear buffer (%d).\n",
                   		    in_lbfd );
			} /* End if */

	/* Get the pathname of the work directory */
	ret = MISC_get_work_dir (output_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
		 } /* End if */

   	/* Add the name of the output LB to the path */
   	strcat (output_name, "/rms_output.lb");	 	
   		 
   	/* Get the LB id of the output LB */
	out_lbfd = LB_open ( output_name, LB_WRITE, NULL);
		if (out_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS output linear buffer (%d).\n",
                   		    out_lbfd );
			} /* End if */
			
	/* Get the pathname of the work directory */
	ret = MISC_get_work_dir (record_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			 } /* End if */

   	/* Add the name of the record log LB to the path */
   	strcat (record_name, "/rms_statlog.lb");

   	/* Get the LB id of the record log LB */
   	record_lbfd = LB_open ( record_name, LB_WRITE, NULL);
		if (record_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS record log linear buffer (%d).\n",
                   		    record_lbfd );
			}/* End if */
			
	while(close_num != 99){
		
		/* Flush the standard input buffer */
		ret = fflush(stdin);

		/* Zero out the write buffer */
		init_buf(write_buf);

		/* Set the pointer to the beginning of the buffer */
		write_ptr = write_buf;

		/* Print the main menu */
		printf("\n------------------------------------\n");
		printf("            Main Menu              \n");
		printf("------------------------------------\n");
		printf("4: RDA state \n");
		printf("5: RDA channel control \n");
		printf("6: RDA operational mode \n");
		printf("7: Archive II \n");
		printf("8: RPG control \n");
		printf("9: Force adaptation update \n");
		printf("10: Narrowband interface command \n");
		printf("11: Wideband Interface \n");
		printf("12: Wideband Loopback \n");
		printf("14: Free Text Message \n");
		printf("15: Edit PUP ID's \n");
		printf("17: Download PUP ID's \n");
		printf("18: Edit clutter zones \n");
		printf("20: Download clutter zones \n");
		printf("21: Edit bypass maps \n");
		printf("23: Download bypass maps \n");
		printf("24: Edit load shed \n");
		printf("26: Download load shed \n");
		printf("27: Edit Narrowband Configuration \n");
		printf("29: Download Narrowband Configuration \n");
		printf("30: Edit Narrowband Dial-In control \n");
		printf("32: Download Narrowband Dial-In control \n");
		printf("33: Edit authorized users \n");
		printf("35: Download authorized users \n");
		printf("39: RMS up \n");
		printf("99: Exit \n\n");
		
		printf("Enter number for message:  ");
                             /* Get the main menu selection */
		scanf("%d", &in_num);
                             printf("\n");

                	/* Validation check of the main menu input */
		if (in_num > 0 && in_num < 100) {

		        switch (in_num){

		        case 4: /* RDA state */
		
			/* Print RDA state menu */
			printf("\n------------------------------------\n");
			printf("Message type 4:\n");
			printf("RDA State Command\n");
			printf("------------------------------------\n");
			printf("1:  Standby \n");
			printf("2:  Restart \n");
			printf("5:  Operate \n");
			printf("6:  Offline Operate \n");
			printf("10: Performance Data \n");
			printf("12: Swith to Auxillary Power (Toggle) \n");
			printf("13: Select Remote Control \n");
			printf("15: Enable Remote Control \n");
			printf("16: Autocalibration Override \n");
			printf("17: ISU Enable/Disable \n");
			printf("18: Base Data Transmission \n");
			printf("19: Request Status Data \n");
			printf("20: Auto Calibration \n");
			printf("22: Spot Blanking \n");
			printf("Enter number for command:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");
			
			/* Set the message type to RDA state */
			msg_num = 4;
			
			switch (arch_num){
			
				case 18:
				        /* Clear the input string */
				        for (i=0; i<5; i++){
					string_five[i] = blank_char;
					} /* End loop */

				        printf("Enter R, V, and/or W separated by / \n or N for disable all\n or A for enable all:  ");

				        /* Get the base data enable parameters */
				        scanf("%s", string_five);

				        /* Place pointer past header */
				        write_ptr += MESSAGE_START;

				        /* Caste menu input to a short */
				        temp_short = (short)arch_num;

				        /* Place menu selection into message */
				        conv_short_unsigned(write_ptr,&temp_short);
				        write_ptr += PLUS_SHORT;

				        /* Place base data enable parameters in message */
				        for (i=0; i<5; i++){
					conv_char_unsigned(write_ptr, &string_five[i],PLUS_BYTE);
					write_ptr += PLUS_BYTE;
					} /* End loop */

				        /* Add terminator to the message */
				        add_terminator(write_ptr);
				        write_ptr += PLUS_INT;

				        /* Compute message size */
				        message_size = write_ptr - write_buf;

	        			        /* Pad the message with zeros */
				        pad_message(write_ptr,message_size,32);

				        break;

				case 16:

				        printf("Enter autocal override\n -10 to +10  scaled to 4:  ");
				        /* Get override value */
				        scanf("%d", &temp_int1);

				        /* Place pointer past header */
				        write_ptr += MESSAGE_START;

				         /* Caste menu input to a short */
				        temp_short = (short)arch_num;

				        /* Place menu selection into message */
				        conv_short_unsigned(write_ptr,&temp_short);
				        write_ptr += PLUS_SHORT;

				         /* Caste overide to a short */
        				        autocal_override = ( short)temp_int1;

				        /* Place override into message */
				        conv_short_unsigned(write_ptr,&autocal_override);
				        write_ptr += PLUS_SHORT;

				         /* Add terminator to the message */
				        add_terminator(write_ptr);
				        write_ptr += PLUS_INT;

				        /* Compute message size */
				        message_size = write_ptr - write_buf;

				        /* Pad the message with zeros */
				        pad_message(write_ptr,message_size,32);

				        break;

				case 22:

				        /* Place pointer past header */
				        write_ptr += MESSAGE_START;

				        /* Caste menu input to a short */
				        temp_short = (short)arch_num;

				         /* Place menu selection into message */
				        conv_short_unsigned(write_ptr,&temp_short);
				        write_ptr += PLUS_SHORT;

				        /* Get password */
				        strcpy(password_string, RMS_PASSWORD1);

				        /* Place password in message */
				        for (i=0; i<PASSWORD_SIZE; i++){
					conv_char_unsigned(write_ptr, &password_string[i],PLUS_BYTE);
					write_ptr += PLUS_BYTE;
					} /* End loop */

				        /* Add terminator to the message */
				        add_terminator(write_ptr);
				        write_ptr += PLUS_INT;

				         /* Compute message size */
				        message_size = write_ptr - write_buf;

				         /* Pad the message with zeros */
				        pad_message(write_ptr,message_size,32);

				        break;

				default:
				         /* Place pointer past header */
				        write_ptr += MESSAGE_START;

				        /* Caste menu input to a short */
				        temp_short = (short)arch_num;

				         /* Place menu selection into message */
				        conv_short_unsigned(write_ptr,&temp_short);
				        write_ptr += PLUS_SHORT;

				        /* Compute message size */
				        message_size = write_ptr - write_buf;

				         /* Pad the message with zeros */
				        pad_message(write_ptr,message_size,32);

				        break;
				        } /* End switch */

			/* Set message size */
			message_size = 32;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Let ORPG know that the RDA status has changed */
			 EN_post (ORPGEVT_RDA_RPG_COMMS_STATUS, NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 5: /* RDA channel control command */

			/* Print the RDA channel control menu */
			printf("\n------------------------------------\n");
			printf("Message type 5:\n");
			printf("RDA Channel Control Command\n");
			printf("------------------------------------\n");
			printf("4: Set RDA to Controlling \n");
			printf("5: Set RDA to Non Controlling \n");
			printf("Enter number for command:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 5;

			 /* Place pointer past header */
			 write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			 /* Place menu selection into message */
			 conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Get password */
			strcpy(password_string, RMS_PASSWORD2);

			/* Place password in message */
			for(i=0; i<=PASSWORD_SIZE; i++){
				conv_char_unsigned(write_ptr, &password_string[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE;
				} /* End loop */

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT;

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Pad the message with zeros */
			pad_message(write_ptr,message_size,32);

			/* Set constant message size */
			message_size = 32;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 6: /* Change operational mode */

			/* Set message type */
			msg_num = 6;

			/* Print change operational mode menu */
			printf("\n------------------------------------\n");
			printf("Message type 6:\n");
			printf("Change RDA Operating Mode Command\n");
			printf("------------------------------------\n");
			printf("2: Set RDA to Maintenance Mode \n");
			printf("4: Set RDA to Operational Mode \n");
			printf("Enter number for message:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 7: /* Build Archive II */

			/* Archive II menu */
			printf("\n------------------------------------\n");
			printf("Message type 7:\n");
			printf("Archive II Control Command\n");
			printf("------------------------------------\n");
			printf("6: Archive II Off \n");
			printf("4: Archive II On \n");
			printf("Enter number for message:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 7;

			/* Determine if number of scans needed */
			switch (arch_num){
			        case 6:
				break;
			        case 4:
				printf("Enter number of scans (0 to 255):  ");
				/* Get number of scans */
				scanf("%d", &scan_num);
				break;
				}/* End switch*/

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Caste scans to a short */
			temp_short = (short)scan_num;

			/* Place scans into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT;

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;


		case 8: /* RPG Control */

			  /* RPG Control menu */
			  printf("\n------------------------------------\n");
			printf("Message type 8:\n");
			printf("RPG Control Command\n");
			printf("------------------------------------\n");
			printf("1: Operational Mode \n");
			printf("2: Test Mode \n");
			printf("4: Restart RPG \n");
			printf("5: Shutdown - Standby \n");
			printf("6: Shutdown - Off (Halt) \n");
			printf("Enter number for control command:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 8;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write (in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 9: /* Force adaptation update */

			/* Set message type */
			msg_num = 9;

			/* Set menu selection */
			arch_num =1;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;


			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);


			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 10: /* Narrowband Interface command */

			/* Set message type */
			msg_num = 10;

			/* Narrowband Interface command menu */
			printf("\n------------------------------------\n");
			printf("Message type 10:\n");
			printf("Narrowband Interface Command\n");
			printf("------------------------------------\n");
			printf(" 0 = No change in status\n");
			printf(" 1 = Disconnect\n");
			printf(" 2 = Connect\n");
			printf("Enter command :  ");
			/* Get the menu selection */
			scanf("%d", &temp_int1);

			printf("Enter starting line number:  ");
			/* Get the starting line number */
			scanf("%d", &temp_int2);

			printf("Enter ending ( 0 for only one line) line number:  ");
			/* Get the ending line number */
			scanf("%d", &temp_int3);

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/*  Place menu input into message */
			conv_short_unsigned(write_ptr,(short*)&temp_int1);
			write_ptr += PLUS_SHORT; 

			/*  Place starting line number into message */
			conv_short_unsigned(write_ptr,(short*)&temp_int2);
			write_ptr += PLUS_SHORT; 

			/*  Place ending line number into message */
			conv_short_unsigned(write_ptr,(short*)&temp_int3);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 11: /* Wideband interface command */

			/*Wideband interface command menu */
			printf("\n------------------------------------\n");
			printf("Message type 11:\n");
			printf("Wideband Interface Command\n");
			printf("------------------------------------\n");
			printf("1: Disconnect \n");
			printf("2: Connect \n");
			printf("Enter number for message:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 11;

			printf("1-primary 2-secondary 3-user\n");
			printf("Enter wideband number:  ");
			/* Get the wideban to command */
			scanf("%d", &scan_num);

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste wideband number to a short */
			temp_short = (short)scan_num;

			/* Place wideband number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 12: /* Wideband Loopback */

			/* Set message type */
			msg_num = 12;

			/* Wideband Loopback menu */
			printf("\n------------------------------------\n");
			printf("Message type 12:\n");
			printf("Wideband Loop Test Command\n");
			printf("------------------------------------\n");
			printf("Enter loopback time (60000 - 300000):  ");
			/* Get the loopback time */
			scanf("%d", &temp_int1);

			/* Caste loopback time to a short */
			temp_short = (short)temp_int1;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Place loopback time into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 14: /* Free text message */

			/* Clear the line number */
			line_num = 0;

			 /* Free text message menu */
			printf("\n------------------------------------\n");
			printf("Message type 14:\n");
			printf("Send Free Text Message\n");
			printf("------------------------------------\n");
			printf("Send to\n  ");
			printf("0  = All\n  ");
			printf("1  = RDA\n  ");
			printf("2  = HCI\n  ");
			printf("4  = APUP\n ");
			printf("8  = PUES\n ");
			printf("32 = LINE\n ");
			printf("Enter:  ");
			/* Get the line number */
			scanf("%d", &line_num);
			printf("\n");
			/* Flush the input buffer */
			fflush(stdin);
			printf("\nEnter string\n ");
			/* Get the free text string */
			gets(free_temp_string);
			printf("\n");

			/* Set message type */
			msg_num = 14;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste line number to a short */
			temp_short = (short)line_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

                        		i = 0;
                                        	string_size = 0;

			/*string_size = sizeof(free_temp_string);*/
                                                 while ( free_temp_string[i] != '\0'){
                                                 	string_size ++;
                                                        	i++;
                                                          } /* End loop */

			/* Place string size into message */
			conv_short_unsigned(write_ptr,&string_size);
			write_ptr += PLUS_SHORT; 

			/* Place string into message */
			for (i=0; i<=string_size; i++){
				conv_char_unsigned(write_ptr, &free_temp_string[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				} /*End loop */

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Pad the message with zeros */
			pad_message(write_ptr,message_size,408);

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 15: /* PUP id's/Commline assignments */

			 /*  PUP id's/Commline assignments menu */
			printf("\n------------------------------------\n");
			printf("Message type 15:\n");
			printf("Edit Associated PUPs IDs/Comline Assignments\n");
			printf("------------------------------------\n");
			printf("1: Edit PUP id's \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 15;

			/* Determine if line number needed */
			if(arch_num == 1){
				printf("Enter line number:  ");
				scanf("%d", &scan_num);
				}/* End if */

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)scan_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 17: /* Download pup id */

			/*  Download pup id menu */
			printf("\n------------------------------------\n");
			printf("Message type 17:\n");
			printf("Download Associated PUPs IDs/Comline Assignments\n");
			printf("------------------------------------\n");
			printf("Enter line number \n  ");
			/* Get line number */
			scanf("%d", &line_num);
			printf("\n");

			/* GetPUP id */
			printf("\nEnter PUP id \n  ");
			scanf("%s", temp_string);
			printf("\n");

			/* Set message type */
			msg_num = 17;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste line number to a short */
			temp_short = (short)line_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Place PUP id into message */
			for (i=0; i<=3; i++){
				conv_char_unsigned(write_ptr, &temp_string[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				} /* End loop */

			/* Place 'N' into message */
			conv_char_unsigned(write_ptr, &temp_char,PLUS_BYTE_2);
			write_ptr += PLUS_BYTE;

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

                        		/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 18: /* Edit clutter suppression zones */

			 /* Edit clutter suppression zones menu */
			 printf("\n------------------------------------\n");
			printf("Message type 18:\n");
			printf("Edit Clutter Suppression Regions\n");
			printf("------------------------------------\n");
			printf("1: Edit Clutter Suppression Regions \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 18;

			switch(arch_num){

			        case 1:
				printf("Enter file number:  ");
				/* Get the file number */
				scanf("%d", &scan_num);
				printf("\n");
				break;

			        default:
				break;
			        } /* End switch */

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu input to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste file number to a short */
			temp_short = (short)scan_num;

			/* Place file number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 20: /* Download clutter suppression zones */

			/* Allocate memory for clutter zones */
			clutter_zone_ptr = (struct clutter_region*)malloc((MAX_CLUTTER_REGIONS * sizeof(struct clutter_region)));

			/* Download clutter suppression zones menu */
			printf("\n------------------------------------\n");
			printf("Message type 20:\n");
			printf("Download Clutter Suppression Regions\n");
			printf("------------------------------------\n");
			printf("Enter file number to edit: ");
			/* Get file number */
			scanf("%d", &file_num);

			/* Set zone number */
			zone_num = 0;

			/* Set menu type */
			arch_num = 1;

			/* Set message type */
			msg_num = 20;

			/* Get current clutter zone */
			for (i=0; i<=MAX_CLUTTER_REGIONS; i++){
				clutter_zone_ptr[i].start_range = 0;
				clutter_zone_ptr[i].end_range = 0;
				clutter_zone_ptr[i].start_azmith = 0;
				clutter_zone_ptr[i].end_azmith = 0;
				clutter_zone_ptr[i].elv_seg = 0;
				clutter_zone_ptr[i].select_code = 0;
				clutter_zone_ptr[i].channel_d_width = 0;
				clutter_zone_ptr[i].channel_s_width = 0;
				} /* End loop */

			printf("Edit clutter file (Yes = 1 No = 0): ");
			/* Get the menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			if (arch_num != 1)
				arch_num = 0;

			while(arch_num != 0){

                                                        /* Get the clutter zone data to send to the ORPG/RMS */
				printf("Enter Zone to edit(1-15): ");
				scanf("%d", &zone_num);

				/* Adjust the zone number for logical numbering in ORPG */
				zone_num --;

				printf("Enter Start Range (2-510): ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num].start_range = (ushort)temp_int1;

				printf("Enter End Range (2-510): ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num].end_range = (ushort)temp_int1;

				printf("Enter Start Azmith (0-360): ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. start_azmith = (ushort)temp_int1;

				printf("Enter End Azmith (0-360): ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. end_azmith = (ushort)temp_int1;

				printf("Enter Elevation Segment (1-2): ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. elv_seg = (ushort)temp_int1;

				printf("Bypass Filter Forced (no filtering) = 0\n");
				printf("Bypass Map in Control = 1\n");
				printf("Bypass Filter Forced = 2\n");
				printf("Enter Select Code: ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. select_code = (ushort)temp_int1;


				printf("Minimum = 1\n");
				printf("Medium  = 2\n");
				printf("Maximum = 3\n");
				printf("Enter Channel D Width: ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. channel_d_width = (ushort)temp_int1;

				printf("Minimum = 1\n");
				printf("Medium  = 2\n");
				printf("Maximum = 3\n");
				printf("Enter Channel S Width: ");
				scanf("%d", &temp_int1);
				clutter_zone_ptr[zone_num]. channel_s_width = (ushort)temp_int1;

				printf("Enter 0 to exit or 1 to edit the next zone: ");
				scanf("%d", &arch_num);
				printf("\n");

				} /* End loop */


			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Get file number */
			temp_ushort = file_num;

			/* Place file number into message */
			conv_ushort_unsigned(write_ptr,&temp_ushort);
			write_ptr += PLUS_SHORT; 

			/* Place clutter zone information into message */
			for (i=0; i <= MAX_CLUTTER_REGIONS; i++){

				temp_ushort = clutter_zone_ptr[i]. start_range;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. end_range;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. start_azmith;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. end_azmith;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. elv_seg;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. select_code;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. channel_d_width;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 

				temp_ushort = clutter_zone_ptr[i]. channel_s_width;
				conv_ushort_unsigned(write_ptr,&temp_ushort);
				write_ptr += PLUS_SHORT; 
				} /* End loop */

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			ret_val = build_header (&message_size,msg_num,write_buf, 0);

			if (ret_val < 0 )
				(void) fprintf (stderr, "Build header for zones failed\n.");

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit(1);
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

			free(clutter_zone_ptr);

			/* Clear the main menu input */
			 in_num = 0;

			break;

		case 21: /* Edit bypass map */

			/*   Edit bypass map menu */
			printf("\n------------------------------------\n");
			printf("Message type 21:\n");
			printf("Edit Clutter Filter Bypass Maps\n");
			printf("------------------------------------\n");
			printf("1: Edit All Maps \n");
			printf("2: Edit Specific Segment and Radial \n");
			printf("3: Cancel Edit\n");
			printf("Enter number for message:  ");
			/* Get menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			switch(arch_num){

			        case 1:
				/*segment_number = 0;
				rad_num = 0;*/
                                		arch_num = 3;
				break;

			        case 2:
				printf("Enter number for segment:  ");
				/* Get segment number */
				scanf("%d", &segment_number);
				printf("Enter number for radial:  ");
				/* Get radial */
				scanf("%d", &rad_num);

				printf("\n");
				break;

			        default:
				break;
				} /* End switch */

			/* Set message type */
			msg_num = 21;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu selection to a short */
			temp_short = (short)arch_num;

			/* Placemenu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Caste segment number to a short */
			temp_short = (short)segment_number;

			/* Place segment number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Caste radial to a short */
			temp_short = (short)rad_num;

			/* Place radial into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 23: /* Download Bypass maps */

			/* Allocate memeory for the bypass map */
			bypass_map_ptr = (struct bypass_map*)malloc(sizeof(struct bypass_map));

			/*  Download Bypass maps menu */
			printf("\n------------------------------------\n");
			printf("Message type 23:\n");
			printf("Download Clutter Filter Bypass Maps\n");
			printf("------------------------------------\n");
			printf("Enter segment number to edit (1-2): ");
			/* Get segment number */
			scanf("%d", &segment_num);

			printf("\nEnter start radial(1-256): ");
			/* Get starting radial number */
			scanf("%d", &radial_num_1);

			printf("\nEnter ending radial(1-256): ");
			/* Get ending radial number */
			scanf("%d", &radial_num_2);

			/* Set message type */
			msg_num = 23;

			/* Clear out the bypass map */
			for (i=0; i<=MAX_BIN_NUMBER; i++){
				bypass_map_ptr->bin[i] = 0;
				} /* End loop */

			printf("\nEnter starting bin (1-34): ");
			/* Get starting bin number */
			scanf("%d", &bin_num_1);

			printf("\nEnter ending bin (1-34): ");
			/* Get ending bin number */
			scanf("%d", &bin_num_2);

			printf("\nEnter bin_value ( 0-65535): ");
			/* Get bin value */
			scanf("%d", &temp_int1);


			/* Check to see if starting radial smaller than ending radial */
			if(radial_num_1 <= radial_num_2) {
			                /* Place the bin value in all the required bins */
				for (i = radial_num_1; i <= radial_num_2;i++){

					/* Place pointer past header */
					write_ptr += MESSAGE_START;

					/* Caste segment number to a short */
					temp_short = (short)segment_num;

					/* Place segment number into message */
					conv_short_unsigned(write_ptr,&temp_short);
					write_ptr += PLUS_SHORT;

					/* Caste radial to a short */
					temp_short = (short)i;

					/* Place radial into message */
					conv_short_unsigned(write_ptr,&temp_short);
					write_ptr += PLUS_SHORT; 

					for (j=0; j <= MAX_BIN_NUMBER ; j++){

						/* If the bin is within the chosen range */
						if(j >= bin_num_1 && j <= bin_num_2){
							/* Caste bin value to a short */
							temp_short = (short)temp_int1;
							/* Place bin value into message */
							conv_short_unsigned(write_ptr,&temp_short);
							write_ptr += PLUS_SHORT; 
							} /* End if */
						else{
							temp_short = 0;
							/* Place zero into message */
							conv_short_unsigned(write_ptr,&temp_short);
							write_ptr += PLUS_SHORT;
							} /* End else */

						}/* End loop */

					/* Add terminator to message */
					add_terminator(write_ptr);
					write_ptr += PLUS_INT; 

					/* Compute message size */
					message_size = write_ptr - write_buf;

					/* Add header to message */
					build_header (&message_size,msg_num,write_buf, 0);

					/* Write message to the ORPG input LB */
					ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

					if ( ret_val!= MAX_BUF_SIZE  ){
						(void) fprintf (stderr, "Output LB write failed.");
						printf ("error code %d \n", ret_val);
						exit;
						}/* End if */

					/* Let ORPG know a message has been entered into the input LB */
					ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

					/* Place pointer at the beginning of the buffer */
					write_ptr = write_buf;

					/* Wait half a second */
					msleep(500);
					} /* End loop */
				}/* End if */
			else {

				/* This sectiion send all radial from the bypass map.  However this function in the FAA/RMMS
				    is to be removed but I will retain this code for now */
                                                for (i = radial_num_1; i <= 256; i++){

				/* Place pointer past header */
				write_ptr += MESSAGE_START;

				/* Caste segment number to a short */
				temp_short = (short)segment_num;

				/* Place segement number into message */
				conv_short_unsigned(write_ptr,&temp_short);
				write_ptr += PLUS_SHORT;

				/* Caste radial to a short */
				temp_short = (short)i;

				/* Place radial into message */
				conv_short_unsigned(write_ptr,&temp_short);
				write_ptr += PLUS_SHORT;

				for (j=0; j <= MAX_BIN_NUMBER ; j++){

					if(j >= bin_num_1 && j <= bin_num_2){
						/* Caste bin value to a short */
						temp_short = (short)temp_int1;
						/* Place bin value into message */
						conv_short_unsigned(write_ptr,&temp_short);
						write_ptr += PLUS_SHORT;
						} /* End if */
					else{
						temp_short = 0;
						/* Place zero into message */
						conv_short_unsigned(write_ptr,&temp_short);
						write_ptr += PLUS_SHORT;
						} /* End else */
					} /* End loop */

				/* Add terminator to message */
				add_terminator(write_ptr);
				write_ptr += PLUS_INT;

				/* Compute message size */
				message_size = write_ptr - write_buf;

				/* Add header to message */
				build_header (&message_size,msg_num,write_buf, 0);

				/* Write message to the ORPG input LB */
				ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

				if ( ret_val!= MAX_BUF_SIZE  ){
					(void) fprintf (stderr, "Output LB write failed.");
					printf ("error code %d \n", ret_val);
					exit;
					} /* End if */

				/* Let ORPG know a message has been entered into the input LB */
				ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

				/* Set pointer to beginning of buffer */
				write_ptr = write_buf;
				msleep(500);
				} /* End loop */

				for (i = 1; i <= radial_num_2; i++){

					/* Place pointer past header */
					write_ptr += MESSAGE_START;

					/* Caste segment number to a short */
					temp_short = (short)segment_num;

					/* Place segment number into message */
					conv_short_unsigned(write_ptr,&temp_short);
					write_ptr += PLUS_SHORT;

					/* Caste radial to a short */
					temp_short = (short)i;

					/* Place radial into message */
					conv_short_unsigned(write_ptr,&temp_short);
					write_ptr += PLUS_SHORT;

					for (j=0; j <= MAX_BIN_NUMBER ; j++){

						if(j >= bin_num_1 && j <= bin_num_2){
							/* Caste bin value to a short */
							temp_short = (short)temp_int1;
							/* Place bin value into message */
							conv_short_unsigned(write_ptr,&temp_short);
							write_ptr += PLUS_SHORT;
							} /* End if */
						else{
							temp_short = 0;
							/* Place zero into message */
							conv_short_unsigned(write_ptr,&temp_short);
							write_ptr += PLUS_SHORT;
							} /* End else */

						} /* End loop */

					/* Add terminator to message */
					add_terminator(write_ptr);
					write_ptr += PLUS_INT;

					/* Compute message size */
					message_size = write_ptr - write_buf;

					/* Add header to message */
					build_header (&message_size,msg_num,write_buf, 0);

					/* Write message to the ORPG input LB */
					ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

					if ( ret_val!= MAX_BUF_SIZE  ){
						(void) fprintf (stderr, "Output LB write failed.");
						printf ("error code %d \n", ret_val);
						exit;
						} /* End if */

					/* Let ORPG know a message has been entered into the input LB */
					ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

					/* Set pointer to beginning of buffer */
					write_ptr = write_buf;
					msleep(500);
					} /* End loop */
				} /* End else */

			free(bypass_map_ptr);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 24: /* Edit load shed categories */

			 /*  Edit load shed categories menu */
			 printf("\n------------------------------------\n");
			printf("Message type 24:\n");
			printf("Edit Load Shed Categories\n");
			printf("------------------------------------\n");
			printf("1: Edit Load Shed Categories \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 24;

			switch(arch_num){

			        case 1:

				printf(" 1 = CPU\n");
				printf(" 2 = Memory\n");
				printf(" 3 = Distribution\n");
				printf(" 4 = Storage\n");
				printf(" 5 = Input Buffer\n");
				printf(" 7 = Wide Band\n");
				/* Get line number */
				printf("Enter line number:  ");
				scanf("%d", &scan_num);

				printf("\n");

				printf(" 1 = Precip/Severe Wx\n");
				printf(" 2 = Clear Air\n");
				printf("Enter Wx mode:  ");
				/* Get weather mode */
				scanf("%d", &wx_flag);

				break;

			        default:
				break;
				} /* End switch */

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu selection to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			if( wx_flag == 1){
				/* Place severe/precip into message */
				conv_char_unsigned(write_ptr, severe,PLUS_BYTE_2);
				write_ptr += PLUS_BYTE_2;
				} /* End if */
			else {
				/* Place clear air into message */
				conv_char_unsigned(write_ptr, clear, PLUS_BYTE_2);
				write_ptr += PLUS_BYTE_2;
				} /* End else */

			/* Caste line number to a short */
			temp_short = (short)scan_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT;

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT;

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 26: /* Download load shed categories */

			 /*   Download load shed categories menu */
			 printf("\n------------------------------------\n");
			printf("Message type 26:\n");
			printf("Download Load Shed Categories\n");
			printf("------------------------------------\n");
			printf(" 1 = CPU\n");
			printf(" 2 = Memory\n");
			printf(" 3 = Distribution\n");
			printf(" 4 = Storage\n");
			printf(" 5 = Input Buffer\n");
			printf(" 7 = Wide Band\n");
			printf("Enter line number:  ");
			/* Get line number */
			scanf("%d", &arch_num);
			printf("\n");

			/* Set message type */
			msg_num = 26;

			printf("Enter Warning 5 - 100 :  ");
			/* Get warning */
			scanf("%d", &temp_int1);

			printf("Enter alarm 5 - 100:  ");
			/* Get alarm */
			scanf("%d", &temp_int2);

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste line number to a short */
			temp_short = (short)arch_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste warning to a short */
			temp_short = (short)temp_int1;

			/* Place warning into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste alarm to a short */
			temp_short = (short)temp_int2;

			/* Place alarm into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 27: /* Edit Narrowband Reconfiguration*/

			/*  Edit Narrowband Reconfiguration menu */
			printf("\n------------------------------------\n");
			printf("Message type 27:\n");
			printf("Edit Narrowband Reconfiguration Control\n");
			printf("------------------------------------\n");
			printf("1: Edit Narrowband Reconfiguration \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Cancel edit command */
			if(arch_num ==2){
				temp_int1 = 0;
				} /* End if */
			else if( arch_num == 1){
				printf("Enter line to edit:  ");
				/* Get line number */
				scanf("%d", &temp_int1);
				printf("\n");
				} /* End else if */
			else {
				break;
				} /* End else */

			/* Set message type */
			msg_num = 27;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu selection to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)temp_int1;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 29: /* Download Narrowband Configuration */

			/*   Download Narrowband Configuration menu */
			printf("\n------------------------------------\n");
			printf("Message type 29:\n");
			printf("Download Narrowband Reconfiguration Control\n");
			printf("------------------------------------\n");
			printf("Enter line number to edit (1-47): ");
			/* Get line number */
			scanf("%d", &temp_int1);

			printf("\n 1 = Associated, APUP\n");
			printf(" 2 = Nonassociated, NAPUP\n");
			printf(" 3 = Other Users\n");
			printf(" 4 = River Forecast Center, RFC\n");
			printf(" 5 = Rain Gauge Data Acq., RGDAC\n");
			printf(" 6 = RPG Oper. Position, RPGOP\n");
			printf("\nEnter Line Class: ");
			/* Get line class */
			scanf("%d", &temp_int2);

			printf("\nEnter Baud Rate (4800,9600,56000): ");
			/* Get baud rate */
			scanf("%d", &temp_int3);

			printf("\n 1 = Dedicated, DEDIC\n");
			printf(" 2 = Dial-In, DIALIN\n");
			printf(" 3 = Dial-In/Dial_Out, DINOUT\n");
			printf("\nEnter Line Type: ");
			/* Get line type*/
			scanf("%d", &temp_int4);

			printf("\nEnter Password (4 characters): ");
			/* Get password */
			scanf("%s", string_five);

			printf("\n 1 = Single Set of Products, SSET\n");
			printf(" 2 = Single Set Every Volume Scan, RSET\n");
			printf(" 3 = One-Time Request, 1TIM\n");
			printf(" 4 = Combination of 1TIM and RSET, COMB\n");
			printf("\nEnter Line Type: ");
			/* Get distribution method */
			scanf("%d", &temp_int5);

			printf("\nTime Limit (1-1440): ");
			/* Get time limit */
			scanf("%d", &temp_int6);

			/* Set message type */
			msg_num = 29;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste blank to a short */
			temp_short = (short)BLANK;

			/* Place blank into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)temp_int1;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Place id into message */
			for(i=0; i<=sizeof(id)-1; i++){
				conv_char_unsigned(write_ptr, &id[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				} /* End loop */

			/* Get line class */
			if (temp_int2 == 1){

				/* APUP */
				for(i=0; i<=sizeof(apup)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &apup[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End if */
			else if (temp_int2 == 2){

				/* NAPUP */
				for(i=0; i<=sizeof(napup)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &napup[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else if */
			else if (temp_int2 == 3){

				/* Other */
				for(i=0; i<=sizeof(other)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &other[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else if */
			else if (temp_int2 == 4){

				/* RFC */
				for(i=0; i<=sizeof(rfc)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &rfc[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else if */
			else if (temp_int2 == 5){

				/* RGDAC */
				for(i=0; i<=sizeof(rgdac)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &rgdac[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else if */
			else if (temp_int2 == 6){

				/* RPGOP */
				for(i=0; i<=sizeof(rpgop)-1; i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &rpgop[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else if */
			else {
				/* Default Blanks */
				for(i=0; i<=sizeof(blank6); i++){
					/* Place line class into  message */
					conv_char_unsigned(write_ptr, &blank6[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				} /* End else */

			/* Caste baud rate to a short */
			temp_short = (short)temp_int3;

			/* Place baud rate into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 


			/* Get line type */
			if (temp_int4 == 1){

				/* Dedicated */
				for(i=0; i<=sizeof(dedic)-1; i++){
					/* Place line type into  message */
					conv_char_unsigned(write_ptr, &dedic[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End if */
			else if (temp_int4 == 2){

				/* Dial in */
				for(i=0; i<=sizeof(dialin)-1; i++){
					/* Place line type into  message */
					conv_char_unsigned(write_ptr, &dialin[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else if */
			else if (temp_int4 == 3){

				/* Dial in/out */
				for(i=0; i<=sizeof(dinout)-1; i++){
					/* Place line type into  message */
					conv_char_unsigned(write_ptr, &dinout[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else if */
			else {
				/* Default blanks */
				for(i=0; i<=sizeof(blank6)-1; i++){
					/* Place line type into  message */
					conv_char_unsigned(write_ptr, &blank6[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else */

			/* Place PUP id/ password into  message */
			for(i=0; i<=3; i++){
				conv_char_unsigned(write_ptr, &string_five[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				}/* End loop */

			/* Get distribution method */
			if (temp_int5 == 1){

				/* SSET */
				for(i=0; i<=sizeof(sset)-1; i++){
					/* Place distribution method into  message */
					conv_char_unsigned(write_ptr, &sset[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End if */
			else if (temp_int5 == 2){

				/* RSET */
				for(i=0; i<=sizeof(rset)-1; i++){
					/* Place distribution method into  message */
					conv_char_unsigned(write_ptr, &rset[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else if */
			else if (temp_int5 == 3){

				/* 1TIM */
				for(i=0; i<=sizeof(onetim)-1; i++){
					/* Place distribution method into  message */
					conv_char_unsigned(write_ptr, &onetim[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else if */
			else if (temp_int5 == 4){

				/* COMB */
				for(i=0; i<=sizeof(comb)-1; i++){
					/* Place distribution method into  message */
					conv_char_unsigned(write_ptr, &comb[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else if */
			else {
				/* Default blanks */
				for(i=0; i<=sizeof(blank6)-1; i++){
					/* Place distribution method into  message */
					conv_char_unsigned(write_ptr, &blank6[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					}/* End loop */
				}/* End else */

			/* Caste time limit to a short */
			temp_short = (short)temp_int6;

			/* Place time limit into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 30: /* Edit Narrowband Dial-In control*/

			/*  Edit Narrowband Dial-In control menu */
			printf("\n------------------------------------\n");
			printf("Message type 30:\n");
			printf("Edit Narrowband Dial-In Control\n");
			printf("------------------------------------\n");
			printf("1: Edit Narrowband Dial-In Control \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Cancel edit command */
			if(arch_num ==2){
				temp_int1 = 0;
				} /* End if */
			else if( arch_num == 1){
				printf("Enter line to edit:  ");
				/* Get line number */
				scanf("%d", &temp_int1);
				printf("\n");
				} /* End else if */
			else {
				break;
				} /* End else */

			/* Set message type */
			msg_num = 30;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste menu selection to a short */
			temp_short = (short)arch_num;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)temp_int1;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 32: /* Download Narrowband Dial-In control */

			/*  Download Narrowband Dial-In control menu */
			printf("\n------------------------------------\n");
			printf("Message type 32:\n");
			printf("Download Narrowband Dial-In Control\n");
			printf("------------------------------------\n");
			printf("Enter line number to edit (1-47): ");
			/* Get line number */
			scanf("%d", &temp_int1);

			printf("\nEnter Password (4 characters): ");
			/* Get password */
			scanf("%s", string_five);

			printf("\n 1 = Single Set of Products, SSET\n");
			printf(" 2 = Single Set Every Volume Scan, RSET\n");
			printf(" 3 = One-Time Request, 1TIM\n");
			printf(" 4 = Combination of 1TIM and RSET, COMB\n");
			printf("\nEnter Line Type: ");
			/* Get distribution method */
			scanf("%d", &temp_int5);

			printf("\nTime Limit (1-1440): ");
			/* Get time limit */
			scanf("%d", &temp_int6);


			/* Set message type */
			msg_num = 32;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste blank to a short */
			temp_short = (short)BLANK;

			/* Place into message into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)temp_int1;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 


			for(i=0; i<=3; i++){
				/* Place password into message */
				conv_char_unsigned(write_ptr, &string_five[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				} /* End loop */

			/* Get gistribution method */
			if (temp_int5 == 1){

				for(i=0; i<=sizeof(sset)-1 ; i++){
					/* Place distribution method into message */
					conv_char_unsigned(write_ptr, &sset[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					} /* End loop */
				} /* End if */

			else if (temp_int5 == 2){

				for(i=0; i<=sizeof(rset)-1; i++){
					/* Place distribution method into message */
					conv_char_unsigned(write_ptr, &rset[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					} /* End loop */
				} /* End else if */

			else if (temp_int5 == 3){

				for(i=0; i<=sizeof(onetim)-1; i++){
					/* Place distribution method into message */
					conv_char_unsigned(write_ptr, &onetim[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					} /* End loop */
				}/* End else if */

			else if (temp_int5 == 4){

				for(i=0; i<=sizeof(comb)-1; i++){
					/* Place distribution method into message */
					conv_char_unsigned(write_ptr, &comb[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					} /* End loop */
				}/* End else if */

			else {
				for(i=0; i<=sizeof(blank6); i++){
					/* Place distribution method into message */
					conv_char_unsigned(write_ptr, &blank6[i],PLUS_BYTE);
					write_ptr +=PLUS_BYTE; 
					} /* End loop */
				}/* End else */

			/* Caste time limit to a short */
			temp_short = (short)temp_int6;

			/* Place time limit into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 33: /* Authorized user */

			/*  Authorized user menu */
			printf("\n------------------------------------\n");
			printf("Message type 33:\n");
			printf("Edit Narrowband Authorized User Control\n");
			printf("------------------------------------\n");
			printf("1: Edit Authorized User \n");
			printf("2: Cancel Edit \n");
			printf("Enter number for message:  ");
			/* Get menu selection */
			scanf("%d", &arch_num);
			printf("\n");

			/* Cancel edit command */
			if(arch_num ==2){
				line_num = 0;
				} /* End if */
			else if( arch_num == 1){
				printf("Enter line to edit:  ");
				/* Get line number */
				scanf("%d", &line_num);
				printf("\n");
				} /* End else if */
			else {
				break;
				} /* End else */

			/* Set message type */
			msg_num = 33;

			/* Caste menu selection to a short */
			edit_auth_user = (short)arch_num;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Place menu selection into message */
			conv_short_unsigned(write_ptr,&edit_auth_user);
			write_ptr += PLUS_SHORT; 

			/* Caste line number to a short */
			temp_short = (short)line_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				}/* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 35: /* Download authorized user */

			/*  Download authorized user menu */
			printf("\n------------------------------------\n");
			printf("Message type 35:\n");
			printf("Download Narrowband Authorized User Control\n");
			printf("------------------------------------\n");
			printf("\n\nEnter line number:  ");
			/* Get line number */
			scanf("%d", &line_num);
			printf("\n");

			printf("\nEnter user id:  ");
			/* Get user id */
			scanf("%d", &user_id);
			printf("\n");

			printf("\nEnter password:  ");
			/* Get password */
			scanf("%s", temp_string);
			printf("\n");

			fflush(stdin);

			printf("\n Y = yes \n");
			printf("\n N = no \n");
			printf("\nEnter override:  ");
			/* Get override */
			scanf("%c", &temp_char);
			printf("\n");

			/* Set message type */
			msg_num = 35;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste line number to a short */
			temp_short = (short)line_num;

			/* Place line number into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			  /* Put user id in temporary string */
   			sprintf(user_string,"%d",user_id);

                		/* Put user id into message */
        			for (i=0; i <MAX_ID; i++){
                			conv_char_unsigned (write_ptr, &user_string[i], PLUS_BYTE);
                			write_ptr += PLUS_BYTE;
         				} /* End loop */

			/* Place password into message */
			for (i=0; i<=5; i++){
				conv_char_unsigned(write_ptr, &temp_string[i],PLUS_BYTE);
				write_ptr +=PLUS_BYTE; 
				} /* End loop */


			/* Place override into message */
			conv_char_unsigned(write_ptr, &temp_char,PLUS_BYTE_2);
			write_ptr += PLUS_BYTE_2;

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 39: /* RMS up */

			/* Set message type */
			msg_num = 39;

			/*  Set command */
			arch_num = 5;

			/* Place pointer past header */
			write_ptr += MESSAGE_START;

			/* Caste command to a short */
			temp_short = (short)arch_num;

			/* Place command into message */
			conv_short_unsigned(write_ptr,&temp_short);
			write_ptr += PLUS_SHORT; 

			/* Add terminator to message */
			add_terminator(write_ptr);
			write_ptr += PLUS_INT; 

			/* Compute message size */
			message_size = write_ptr - write_buf;

			/* Add header to message */
			build_header (&message_size,msg_num,write_buf, 0);

			/* Write message to the ORPG input LB */
			ret_val = LB_write ( in_lbfd, (char*) &write_buf, MAX_BUF_SIZE, LB_ANY);

			if ( ret_val!= MAX_BUF_SIZE  ){
				(void) fprintf (stderr, "Output LB write failed.");
				printf ("error code %d \n", ret_val);
				exit;
				} /* End if */

			/* Let ORPG know a message has been entered into the input LB */
			ret_val = EN_post (ORPGEVT_RMS_MSG_RECEIVED,
			  NULL, 0, 0);

			/* Clear the main menu input */
			in_num = 0;

			break;

		case 99: /* Exit test */

			/* Set the flag to close down the main loop */
			close_num = 99;
			break;

		default:
			break;

		}/*End switch*/
	     }/* End if */
	} /* End while loop */

	return 0;
} /* End */



	
	
