/**************************************************************************

   Module:  rms_init.c

   Description: This is the initialize module for the rms interface. It
   Will handle resets and closing of linear buffers, and will
   register and deregister events and alarms.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/03/13 19:04:33 $
 * $Id: rms_init.c,v 1.30 2008/03/13 19:04:33 jing Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <infr.h>
#include <malrm.h>
#include <en.h>
#include <rms_util.h>
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define MAX_PATHNAME_SIZE	128
#define RMS_STATLOG_VALUE	5

/*
* Task Level Globals
*/
extern ushort  num_retrys;
extern struct  resend *resend_array_ptr; /* Struct to contain info for resending msgs */
extern int     resend_cnt;               /* number of messages in resend que */
extern ushort  rms_msg_seq_number;
extern LB_id_t LB_id_num;
extern int     rms_connection_down;

int  resend_lbfd;
int  in_lbfd;	     /* Input Linear buffer */
int  out_lbfd;	  /* Output Linear buffer */
int  record_lbfd;

/* File Scope Globals */
static int  status_lbfd;
static char input_name[MAX_PATHNAME_SIZE];
static char output_name[MAX_PATHNAME_SIZE];
static char resend_name[MAX_PATHNAME_SIZE];
static char record_name[MAX_PATHNAME_SIZE];
static char status_name[MAX_PATHNAME_SIZE];

/*
* Static Function Prototypes
*/

static void set_lb_attributes (LB_attr *in_lb, LB_attr *out_lb, LB_attr *resend_lb, 
                               LB_attr *text_lb, LB_attr *record_lb, LB_attr *status_lb);



/**************************************************************************

    Description: Sets the attributes for the RMMS linear buffers.

    Inputs:  Pointers to the input and output linear buffers.

    Outputs:

**************************************************************************/
static void set_lb_attributes (LB_attr *in_lb, LB_attr *out_lb, LB_attr *resend_lb, 
                               LB_attr *text_lb, LB_attr *record_lb, LB_attr *status_lb) {

                /* Set the attributes for the input LB */
	strncpy (in_lb->remark, "RMS input linear buffer", LB_REMARK_LENGTH);
	in_lb->mode = 0666;
	in_lb->msg_size = 2416;
	in_lb->maxn_msgs = 100;
	in_lb->types = LB_UNPROTECTED;
	in_lb->tag_size = 0;
	in_lb->version = 0;

	/* Set the attributes for the ouput LB */
	strncpy (out_lb->remark, "RMS output linear buffer", LB_REMARK_LENGTH);
	out_lb->mode = 0666;
	out_lb->msg_size = 2416;
	out_lb->maxn_msgs = 1500;
	out_lb->types = LB_UNPROTECTED;
	out_lb->tag_size = 0;
	out_lb ->version = 0;

	/* Set the attributes for the resend LB */
	strncpy (resend_lb->remark, "RMS resend linear buffer", LB_REMARK_LENGTH);
	resend_lb->mode = 0666;
	resend_lb->msg_size = 2416;
	resend_lb->maxn_msgs = 1500;
	resend_lb->types = LB_UNPROTECTED;
	resend_lb->tag_size = 0;
	resend_lb ->version = 0;

	/* Set the attributes for the free text LB */
	strncpy (text_lb->remark, "RMS free text linear buffer", LB_REMARK_LENGTH);
	text_lb->mode = 0666;
	text_lb->msg_size = 1600;
	text_lb->maxn_msgs = 10;
	text_lb->types = LB_UNPROTECTED;
	text_lb->tag_size = 0;
	text_lb ->version = 0;

	/* Set the attributes for the record log LB */
	strncpy (record_lb->remark, "RMS status record log linear buffer", LB_REMARK_LENGTH);
	record_lb->mode = 0666;
	record_lb->msg_size = 256;
	record_lb->maxn_msgs = 1500;
	record_lb->types = LB_UNPROTECTED;
	record_lb->tag_size = 0;
	record_lb ->version = 0;

	/* Set the attributes for the RMS status LB */
	strncpy (status_lb->remark, "RMS rms status linear buffer", LB_REMARK_LENGTH);
	status_lb->mode = 0666;
	status_lb->msg_size = 256;
	status_lb->maxn_msgs = 1;
	status_lb->types = LB_DB;
	status_lb->tag_size = 0;
	status_lb ->version = 0;
	}

/**************************************************************************

    Description: Sets up the linear buffers and registers all event and 
    alarms.

    Inputs:	

    Outputs:  in_lbfd - Input linear buffer.	
              out_lbfd - Output linear buffer.	
              
**************************************************************************/
int init_rms_interface(int argc, char *argv[]) {

	int ret, i;
	int rms_down;

   LB_attr in_lb_attr;
   LB_attr out_lb_attr; /* Linear buffer attributes structure*/
	LB_attr resend_lb_attr;
	LB_attr text_lb_attr; 
	LB_attr record_lb_attr;
	LB_attr status_lb_attr;
	
	char status_buf[256];
	
	Rms_status *status_ptr;
	
	/* Get working directory path*/
	ret = MISC_get_work_dir (input_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			return (-1);
   		 }
	/* Add input LB name to path */
	strcat (input_name, "/rms_input.lb");

	/* Get working directory path*/
	ret = MISC_get_work_dir (output_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			return (-1);
   		 }
	/* Add output LB name to path */
	strcat (output_name, "/rms_output.lb");


	/* Get working directory path*/
	ret = MISC_get_work_dir (resend_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			return (-1);
   		 }
	/* Add resend LB name to path */
	strcat (resend_name, "/rms_resend.lb");


	/* Get working directory path*/
	ret = MISC_get_work_dir (record_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			return (-1);
   		 }
	/* Add record log LB name to path */
	strcat (record_name, "/rms_statlog.lb");

	/* Get working directory path*/
	ret = MISC_get_work_dir (status_name, MAX_PATHNAME_SIZE - 16);
    		if (ret < 0) {
			LE_send_msg (RMS_LE_ERROR,  "MISC_get_work_dir failed (ret %d)", ret);
			return (-1);
   		 }
	/* Add RMS status LB name to path */
	strcat (status_name, "/rms_status.lb");

	/* Set the attributes for the LBs */
	set_lb_attributes (&in_lb_attr, &out_lb_attr, &resend_lb_attr, &text_lb_attr, &record_lb_attr, &status_lb_attr);

	/* Create the input linear buffer*/
	in_lbfd = LB_open ( input_name, LB_CREATE, &in_lb_attr);
		if (in_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS input linear buffer (%d).\n",
                   		    in_lbfd );
			}

	/* Create the output linear buffer*/
	out_lbfd = LB_open ( output_name, LB_CREATE, &out_lb_attr);
		if (out_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS output linear buffer (%d).\n",
                   		    out_lbfd );
			}

	/* Create the resend linear buffer*/
	resend_lbfd = LB_open ( resend_name, LB_CREATE, &resend_lb_attr);
		if (resend_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS resend linear buffer (%d).\n",
                   		    resend_lbfd );
			}

	/* Create the record log linear buffer*/
	record_lbfd = LB_open ( record_name, LB_CREATE, &record_lb_attr);
		if (record_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS record log linear buffer (%d).\n",
                 		    record_lbfd );
			}

	/* Create the status linear buffer*/
	/* If the LB already exists just open it */
	status_lbfd = LB_open ( status_name, LB_WRITE, NULL);

	if (status_lbfd < 0){
	                /* If LB doesn't exist create it */
		status_lbfd = LB_open ( status_name, LB_CREATE, &status_lb_attr);

		if (status_lbfd <0){
			LE_send_msg(RMS_LE_ERROR, "RMS: Unable to open RMS status linear buffer (%d).\n",
                		    status_lbfd );
			}/* End if */

		else {
			status_ptr = (Rms_status*) status_buf;

			/* Clear out status record */
			status_ptr->rms_rpg_locked_hci = 0;
			status_ptr->rms_rda_locked_hci = 0;
			status_ptr->rms_rpg_locked_rms = 0;
			status_ptr->rms_rda_locked_rms = 0;

			/* Write cleared record to the status LB */
			ret = LB_write(status_lbfd, status_buf, sizeof(status_buf), RMS_STATUS);

			if ( ret < 0 ){
				LE_send_msg(RMS_LE_ERROR, "RMS: Failed to to write rms status msg (%d).\n", ret );
				}/* End if */

			}/* End else */

		}/* End if */

	   /* Launch the RMS socket manager as a child process */

    if ((ret = RMS_socket_mgr (argc, argv, input_name, output_name, in_lb_attr,
                               out_lb_attr)) == -1) {
		LE_send_msg(GL_STATUS | GL_ERROR, 
                    "Failure launching RMS socket manager...process aborting");
        return (-1);
    }

   	/* Zero out resend array */
   	for (i = 0; i< MAX_RESEND_ARRAY_SIZE; i++) {
   		resend_array_ptr[i].msg_seq_num = 0;
   		resend_array_ptr[i].lb_location = 0;
   		resend_array_ptr[i].time_sent = 0;
   		resend_array_ptr[i].retrys = 0;
   		}

	/* Set interface flag for HCI */

	rms_down = 0;

	EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down),0);

	/* Set counters */
   	rms_msg_seq_number = 1;
   	LB_id_num = 1;
   	resend_cnt = 0;
   	interface_errors = 0;
   	num_retrys = 1;
	rms_connection_down = 0;
   	return (1);
} /* End init_interface */


/**************************************************************************

    Description: Closes the linear buffers and deregisters all event and
    alarms.

    Inputs:

    Outputs:

**************************************************************************/
void close_rms_interface() {

	int ret;
	int rms_down;


	/* Remove the input LB */
	if ((ret = LB_remove(input_name)) < 0) {
		LE_send_msg(RMS_LE_ERROR, "RMS: Failed to remove input LB (%d).\n", ret );
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Input LB removed.\n");
		}

	/* Remove the output LB */
	if ((ret = LB_remove(output_name)) < 0) {
		LE_send_msg(RMS_LE_ERROR, "RMS: Failed to remove output LB (%d).\n", ret );
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Output LB removed.\n");
		}

	/* Remove the resend LB */
	if ((ret = LB_remove(resend_name)) < 0) {
		LE_send_msg(RMS_LE_ERROR, "RMS: Failed to remove resend LB (%d).\n", ret );
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Resend LB removed.\n");
		}

	/* Remove the record log LB */
	if ((ret = LB_remove(record_name)) < 0) {
		LE_send_msg(RMS_LE_ERROR, "RMS: Failed to remove statlog LB (%d).\n", ret );
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Statlog LB removed.\n");
		}

	/* Close the port manager */
/*	if (rms_close_ptmgr() != 0) {
		LE_send_msg(RMS_LE_ERROR, "RMS: Unable to close port manager.\n");
                }
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Port manager closed.\n");
		}
*/
      /* terminate the server/child process */
   if (RMS_terminate_server () != 0)
      LE_send_msg(RMS_LE_ERROR, "Unable to terminate RMS server process");
   else
      LE_send_msg(RMS_LE_LOG_MSG, "RMS server process terminated");
   
    /* Set interface down value to send to HCI */
	rms_down = 1;

	/* Post an event to alert HCI the interface is down and set RMS button to red */
	EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down),0);

} /* End close interface */

/**************************************************************************

    Description: Resets the RMMS linear buffers and all events and
    alarms.  Used when the interface fails and is reconnected.

    Inputs:

    Outputs:

**************************************************************************/
int reset_rms_interface() {

	int ret, i;
	int rms_down;

	/* Clear input LB for reset */
	if ((ret = LB_clear (in_lbfd, LB_ALL)) < 0 ){
		LE_send_msg(RMS_LE_ERROR, "RMS: Unable to clear input linear buffer (ret %d).\n", ret );
		return (-1);
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Cleared input linear buffer after restart.\n");
		}

	/* Clear output LB for reset */
	if ((ret = LB_clear (out_lbfd, LB_ALL)) < 0 ){
		LE_send_msg(RMS_LE_ERROR, "RMS: Unable to clear output linear buffer (ret %d).\n", ret );
		return (-1);
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Cleared output linear buffer after restart.\n");
		}

	/* Clear resend LB for reset */
	if ((ret = LB_clear (resend_lbfd, LB_ALL)) < 0 ){
		LE_send_msg(RMS_LE_ERROR, "RMS: Unable to clear resend linear buffer (ret %d).\n", ret );
		return (-1);
		}
	else {
		LE_send_msg(RMS_LE_LOG_MSG, "RMS: Cleared resend linear buffer after restart.\n");
		}

	/* Set interface down value to send to HCI */
	rms_down = 0;

	/* Post an event to alert HCI the interface is up and set RMS button to blue */
	EN_post (ORPGEVT_RMS_CHANGE, &rms_down, sizeof(rms_down),0);

	/* Reset sequence number*/
   	rms_msg_seq_number = 1;
   	
   	/* Reset counters */
	LB_id_num = 1;
   	resend_cnt = 0;
   	interface_errors = 0;
   	num_retrys = 1;
   	
	/* Zero out resend array */
   	for (i = 0; i< MAX_RESEND_ARRAY_SIZE; i++) {
   		resend_array_ptr[i].msg_seq_num = 0; 
   		resend_array_ptr[i].lb_location = 0; 
   		resend_array_ptr[i].time_sent = 0; 
   		resend_array_ptr[i].retrys = 0; 
   		}
   		
   	/* Send all record log messages that have been accumulating in the record log buffer while the interface
   	   was down.  When all the messages are sent the buffer will be cleared.  */ 
   	if ( rms_connection_down == 1){
	
		/* Register rms up message timer.*/
		if((ret = MALRM_register( MALRM_SEND_RMS_STATLOG, rms_clear_record_log)) < 0 ){ 
			if(ret != -204)
      				LE_send_msg(RMS_LE_ERROR, "RMS: Unable To Register MALARM_SEND_RMS_STATLOG (%d).\n", ret );
         		        } /*End if */
		else {
			LE_send_msg(RMS_LE_LOG_MSG, "RMS: MALRM_SEND_RMS_STATLOG registered.\n");
			} /* End else */
					
         	                /*Set the timer for 5 seconds.*/
   	                ret = MALRM_set( MALRM_SEND_RMS_STATLOG, MALRM_START_TIME_NOW, RMS_STATLOG_VALUE );
   		
   		} /* End if */

	/* Clear the interface down flag */
	rms_connection_down = 0;
	
	return (1);
   	
} /* End reset interface */
