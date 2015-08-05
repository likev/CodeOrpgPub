/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2006/04/07 18:41:51 $
 * $Id: cmt_dial.c,v 1.7 2006/04/07 18:41:51 jing Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 * Change History:
 *
 * 12Feb2002 C. Gilbert - CCR #NA01-34801 Issue 1-886 Part 2. Add dial-out support
 *                        for OPUP.
 *
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing.
 */  

/******************************************************************

	file: cmt_dial.c

	This module contains the cm_tcp dial-out processing functions for 
	the comm_manager - TCP version.
	
******************************************************************/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <comm_manager.h>
#include <cmt_def.h>


static void DO_init_ddr_udp_dialout (char *server_name);
/************************************************************************

    Description: This function initializes dial-out variables.

    Inputs:	link - the link involved.

************************************************************************/

int DO_init (Link_struct *link)
{
   char *vars;
   int counter;

   vars = SNMP_get (link->snmp_str.snmp_host,
                    link->snmp_str.snmp_community,
                    link->snmp_str.failure_cnt.oid_cmd);

   if (vars == NULL || sscanf (vars, "%d", &counter) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      if (vars != NULL)
         free (vars);
      return (CM_RTR_PROBLEMS);
   }

   link->snmp_str.failure_cnt.oid_value = counter;

   LE_send_msg (LE_VL3, "Snmp: init failure count = %d, link = %d \n",
                link->snmp_str.failure_cnt.oid_value, link->link_ind); 

   free (vars);
   vars = SNMP_get (link->snmp_str.snmp_host,
                    link->snmp_str.snmp_community,
                    link->snmp_str.conn_cnt.oid_cmd);

   if (vars == NULL || sscanf (vars, "%d", &counter) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      if (vars != NULL)
         free (vars);
      return (CM_RTR_PROBLEMS);
   }

   link->snmp_str.conn_cnt.oid_value = counter;
   free (vars);

   LE_send_msg (LE_VL3, "Snmp: init connection count = %d, link = %d \n",
                link->snmp_str.conn_cnt.oid_value, link->link_ind); 

   link->tcp_dial_state = DO_IDLE;

   return (CM_IN_PROCESSING);

}

/************************************************************************

    Description: This function checks on the dial-out process.

    Inputs:	link - the link involved.

************************************************************************/

int DO_chk  (Link_struct *link)
{
   char *vars;
   int counter, num;

   vars = SNMP_get (link->snmp_str.snmp_host,
                    link->snmp_str.snmp_community,
                    link->snmp_str.failure_cnt.oid_cmd);

   if (vars == NULL || sscanf (vars, "%d", &counter) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      if (vars != NULL)
         free (vars);
      return (CM_RTR_PROBLEMS);
   }
   free (vars);
  
   if (counter > link->snmp_str.failure_cnt.oid_value) {


      /* Dial-out failed check the reason code */
      vars = SNMP_get (link->snmp_str.snmp_host,
                       link->snmp_str.snmp_community,
                       link->snmp_str.discon_code.oid_cmd);
      if (vars == NULL || sscanf (vars, "%d", &num) != 1) {
         LE_send_msg (GL_ERROR, "Error in snmp get\n");
	 if (vars != NULL)
            free (vars);
         return (CM_RTR_PROBLEMS);
      }
      free (vars);

      LE_send_msg (LE_VL2, "SNMP: Disconnect Reason = %d, link = %d \n",
                            num, link->link_ind); 
      /* Return values are defined in Cisco Dial Mibs */ 
      switch (num) {
         case 6:   /*modemWatchdogTimeout*/
         case 13:  /*inactivityTimeout*/
         case 17:  /*dialTimeout*/
         case 31:  /*lapmTiemout*/
         case 32:  /*reliableLinkTxTimeout*/
         case 34:  /*cdOffTimeout*/
            return (CM_TIMED_OUT);

         case 4:   /*noDialTone*/
            return (CM_NO_DIALTONE); 

         case 5:   /*busy*/
            return (CM_BUSY_TONE);

         case 3:   /*noCarrier*/
            return (CM_REJECTED);

         case 2:   /*lostCarrier*/
         case 8:   /*userHangup*/
         case 11:  /*remoteLinkDisconnect*/
         case 18:  /*remoteHangup*/
         case 25:  /*hostDrop*/
         case 26:  /*terminate*/
            return (CM_LOST_CONN);

         case 14:  /*dialStringError*/
            return (CM_PHONENO_NOT_STORED);

         case 7:   /*dtrDrop*/
         case 9:   /*compressionProblem*/
         case 10:  /*retrainFailure*/
         case 15:  /*linkFailure*/
         case 16:  /*modulationError*/
         case 22:  /*trainupFailure*/
         case 24:  /*excessiveEC*/
         case 27:  /*autoLoginError*/
            return (CM_MODEMRETRY_PROBLEMS);
    
         default:
            return (CM_MODEM_PROBLEMS);
      }

   } 

#ifdef CHK_DIAL_CONN

   /* check for a connection */
   vars = SNMP_get (link->snmp_str.snmp_host,
                    link->snmp_str.snmp_community,
                    link->snmp_str.conn_cnt.oid_cmd);

   if (vars == NULL || sscanf (vars, "%d", &num) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      if (vars != NULL)
         free (vars);
      return (CM_RTR_PROBLEMS);
   }
   free (vars);

   if (num > link->snmp_str.conn_cnt.oid_value) {
      LE_send_msg (LE_VL3, "Snmp: Connection Detected! = %d, link = %d \n",
                            num, link->link_ind); 
   }

#endif

   /* Check the modem state during dial and connection phase */
   if (link->tcp_dial_state == DO_IDLE) {
      link->tcp_dial_state = DO_DIAL;
      link->tcp_dial_cnt = 1;
      return (CM_IN_PROCESSING);
   }

   vars = SNMP_get (link->snmp_str.snmp_host,
                    link->snmp_str.snmp_community,
                    link->snmp_str.modem_state.oid_cmd);

   if (vars == NULL || sscanf (vars, "%d", &num) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      if (vars != NULL)
         free (vars);
      return (CM_RTR_PROBLEMS);
   }

   LE_send_msg (LE_VL3, "    modem state = %d, link %d \n",
                        num, link->link_ind);

   /* Return values for Modem State are defined in Cisco Mibs */ 
   if (link->tcp_dial_state == DO_DIAL && 
        (num != OFFHOOK) && 
        (num != CONNECTED) ) {
      link->tcp_dial_cnt++;
      if (link->tcp_dial_cnt > ATTEMPTLIMIT) { 
         /* The modem did not dial */
         LE_send_msg (GL_ERROR, "Modem will not dial (go offhook)\n");
         free (vars);
         return (CM_DIAL_TIMEOUT);
      }
      /* try to initiate dial every several counts or so */
      if (link->tcp_dial_cnt % 8 == 0) {
         DO_init_ddr_udp_dialout (link->dialout_name);
      }
   }

   if (link->tcp_dial_state == DO_CONN && 
       (num != CONNECTED) ) {
      /* The modem lost connection */
      LE_send_msg (GL_ERROR, "Modem lost connection\n");
      free (vars);
      return (CM_LOST_CONN);
   }

   if (link->tcp_dial_state == DO_DIAL && 
       (num == CONNECTED) ) {
      link->tcp_dial_state = DO_CONN;
   }

   free (vars);
   return (CM_IN_PROCESSING);

}

/************************************************************************

    Description: This function match phone numbers.

    Inputs:	link - the link involved.

************************************************************************/
int DO_search_dialout_table (Link_struct *link)
{
int i;


   for (i = 0; i < link->n_phone; i++) {

      if (link->phone_nums == NULL || link->phone_nums[i]->phone_num == NULL)
         return -1;

      if (strcmp(link->phone_no, link->phone_nums[i]->phone_num) == 0) {
         return i;
      }
  
   } /* end for */

   return -1;
}

/************************************************************************

    Description: This function processes the dial out request 

    Inputs:	link - the link involved.

************************************************************************/
void DO_dialout_procedure (Link_struct *link)
{


   link->conn_activity = CONNECTING;
   link->dial_state = HDLC_DIAL_CONNECT;

   HA_connection_procedure (link);

   return;
}

/************************************************************************

    Description: Sets up a connectless (UDP) packet to initiate the
                 Cisco DDR dialing.

    Inputs:	link - the link involved.

************************************************************************/
static void DO_init_ddr_udp_dialout (char *server_name)
{
    int sockfd;			/* socket file descriptor */
    int port = UDP_DIALOUT_PORT;
    char pkt[]= "Init_dialout";
    struct sockaddr_in rem_soc;	/* remote host address  */
    struct sockaddr_in loc_soc;	/* local host/port info */



    memset ((char *)&loc_soc, 0, sizeof (loc_soc));
    memset ((char *)&rem_soc, 0, sizeof (rem_soc));

    loc_soc.sin_family = AF_INET;
    loc_soc.sin_addr.s_addr = htonl (INADDR_ANY);
    loc_soc.sin_port = htons (0);

    rem_soc.sin_family = AF_INET;
    if (SOCK_set_address (&rem_soc, server_name) < 0) {
       LE_send_msg (GL_ERROR, "set address failed for %s\n",server_name);
       return;
    }
    /* fill in port number */
    rem_soc.sin_port = htons ((unsigned short)port);


    /* open a connectless socket to initiate the DDR dial process  */
    if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
	LE_send_msg (GL_ERROR | 1028,  "open connectless dial socket failed (errno %d)\n", errno);
	return;
    }

    errno = 0;
    if ( bind (sockfd, (struct sockaddr *)&loc_soc, sizeof(loc_soc)) < 0) {
        LE_send_msg (GL_ERROR,  "bind failed on UDP DDR port (errno %d)\n",errno);
        close (sockfd);
        return;
    }


    errno = 0;
    /* This pkt should initiate dial. Data in the packet is not relevant */
    if (sendto (sockfd, pkt, strlen(pkt), 0, 
                    (struct sockaddr *)&rem_soc, sizeof (rem_soc)) < 0 ){
        LE_send_msg (GL_ERROR,  "sendto failed on UDP DDR port (errno %d)\n",errno);
        close (sockfd);
        return;
    }
    
    close (sockfd);

}
