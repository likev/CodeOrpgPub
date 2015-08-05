

#ifndef SIMULATE_H
#define SIMULATE_H

#include <stdio.h>
#include <lb.h>
#include <en.h>
#include <misc.h>


#include "comm_manager.h"
#include <prod_user_msg.h>

#define INPUT_MSG00	-100 /* RQ_WRITE */
#define REST_MSG	-101 

#define INPUT_MSG0	0 /* Connection */
#define INPUT_MSG1	1 /* Disconnection */
#define INPUT_MSG2	2 /* General_status */
#define INPUT_MSG3     12 /* Req PUP/RPGOP status msg */
#define INPUT_MSG4      3 /* Request response msg */


#define MAX_MSG_NUMBER	16

#define OUT_GOING_MSG0	0x1 /* product request */
#define OUT_GOING_MSG1	0x2 /* max conn time disable request */
#define OUT_GOING_MSG2	0x4 /* alert request msg */
#define OUT_GOING_MSG3	0x8 /* product list request */
#define OUT_GOING_MSG4	0x10 /* radar coded msg edit/no edit request */
#define OUT_GOING_MSG5	0x20 /* sign-on request */
#define OUT_GOING_MSG6	0x40 /* pup/rpgop tp rpg status */

#define IN_COMMING_MSG0	0x1 /* general_status */
#define IN_COMMING_MSG1	0x2 /* request response */
#define IN_COMMING_MSG2	0x4 /* alert adaptation parameter msg */
#define IN_COMMING_MSG3	0x8 /* product list */
#define IN_COMMING_MSG4	0x10 /* alert msg */
#define IN_COMMING_MSG5	0x20 /* request pup/rpgop status */
#define IN_COMMING_MSG6	0x40 /* routine product */
#define IN_COMMING_MSG7	0x80 /* one time product */


/* User <---- p_server */
#define GENERAL_STATUS_MSGID     1
#define REQUEST_REPONSE_MSGID    2
#define ALERT_ADAPT_PAR_MSGID    3
#define PRODUCT_LIST_MSGID       4
#define ALERT_MSGID              5
#define REQUEST_PUP_STATUS_MSGID 6
#define ROUTINE_PROD_MSGID 	 7
#define ONETIME_PROD_MSGID 	 8


/* User ---> p_server  */
#define ROUTINE_PROD_REQ_MSGID 	 1
#define ONETIME_PROD_REQ_MSGID 	 2
#define SIGN_ON_MSGID 	 	 3
#define PRODUCT_LIST_REQ_MSGID   4

void Output_text (char *text);
int SIM_init ();
void Md_search_input_LB ();
int Md_Sign_on ();
int Md_max_conn_dis();
int Md_puprpgop_to_rpg_status_msg ();
int Md_Routine_product();
int Md_Class1_one_time ();
int Md_alert ();
int Md_product_list_req ();
int Md_connect ();
int Md_disconnect ();
int Md_bias_table_msg();

#endif			/* #ifndef SIMULATE_H */
