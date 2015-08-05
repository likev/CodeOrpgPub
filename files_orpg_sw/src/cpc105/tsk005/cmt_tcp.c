/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/07/14 19:51:06 $
 * $Id: cmt_tcp.c,v 1.53 2011/07/14 19:51:06 jing Exp $
 * $Revision: 1.53 $
 * $State: Exp $
 * Change History:
 *
 */  

/******************************************************************

	file: cmt_tcp.c

	This module contains the cm_tcp processing functions for 
	the comm_manager - TCP version.
	
******************************************************************/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmt_def.h>

#define KEEP_ALIVE_CHECK_TIME 5
#define KEEP_ALIVE_PROB_TIME 30
#define MAX_INCOMING_DATA_SIZE (2 * 1024 * 1024)
#define CONN_RETRY_CHECK_TIME 2

extern int connection_procedure_time_limit; /* defined in cmt_main.c */
extern int no_keepalive_response_disconnect_time; /* defined in cmt_main.c */
extern int Simple_code;

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link structure list */

static unsigned int Rand_cnt = 0;	/* counter for generating random keep 
					   alive interval */

enum {BM_CLEAR, BM_SAVE, BM_WRITE};  /* for parameter "func" of Buffer_msgs */

enum {CMT_WRITE_DONE, CMT_WRITE_STARTED, CMT_WRITE_PENDING};
				/* values for w_started */

static int Share_bw = 0;


/* local functions */
static void Disconnect (Link_struct *link);
static void End_disconnecting (Link_struct *link, int fd);
static void End_faa_disconnecting (Link_struct *link, int fd);
static void Read_data (Link_struct *link, int pvc);
static void Process_exception (Link_struct *link);
static void DO_Process_exception (Link_struct *link, int ret_value);

static void Client_connection_procedure (Link_struct *link);
static void Client_login (Link_struct *link);
static void Server_connection_procedure (Link_struct *link);
static void Server_accept_login (Link_struct *link);
static void End_connecting (Link_struct *link);
static void Process_keep_alive (Link_struct *link, int pvc);
static void Send_data_ack (Link_struct *link, int pvc);
static void Process_data_ack (Link_struct *link, int pvc);
static void Send_short_msg (Link_struct *link, 
			int pvc, int type, unsigned int param);
extern int shutdown(int s, int howto);
static void Exchange_pvcs (Link_struct *link, int ind1, int ind2);
static void Send_keep_alive (Link_struct *link, int pvc);
static int Buffer_msgs (int func, Link_struct *link, 
					int pvc, Tcp_msg_header *data);
static void Send_status_event (Link_struct *link, char *msg);
static char *Get_ip_text (unsigned int ip);
static char *Copy_string (char *to_name, char *from_name);
static int Test_connect (Link_struct *link);
static void Copy_req_fields (Link_struct *link);

/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_links - number of links;
		links - the list of the link structure;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int TCP_initialize (int n_links, Link_struct **links)
{
    int cm_method, i;

    N_links = n_links;
    Links = links;

    if (CM_compress_method () == 0)
	cm_method = MISC_GZIP;
    else if (CM_compress_method () == 1)
	cm_method = MISC_BZIP2;
    else
	cm_method = -1;

    for (i = 0; i < N_links; i++) {
	int pvc;

	for (pvc = 0; pvc < Links[i]->n_pvc; pvc++) {
	    Links[i]->r_hd_cnt[pvc] = 0;
	    Links[i]->w_hd_cnt[pvc] = 0;
	    Links[i]->r_tcp_hd[pvc] = malloc (TCM_MSG_BUF_SIZE);
	    Links[i]->w_tcp_hd[pvc] = malloc (sizeof (Tcp_msg_header));
	    if (Links[i]->r_tcp_hd[pvc] == NULL || 
		Links[i]->w_tcp_hd[pvc] == NULL) {
		LE_send_msg (GL_ERROR, "malloc failed");
		return (-1);
	    }
	    Links[i]->w_started[pvc] = CMT_WRITE_DONE;
	    Links[i]->compress_method = cm_method;
	    Links[i]->pack_time = CM_pack_ms ();
	    Links[i]->max_pack = CM_max_pack ();
	    if (Links[i]->pack_time == 0 && CMC_allow_queued_payload ()) {
		Links[i]->pack_time = 1;
		Links[i]->max_pack = 1;
	    }
	    Links[i]->tnb_sent = 0;
	    Links[i]->tnb_received = 0;
	}
	if (Share_bw && i > 0 && 
			Links[i]->line_rate != Links[i - 1]->line_rate) {
	    LE_send_msg (GL_ERROR, "Bad rate for link %d (In shared BW mode, rates must be identical for all links)", Links[i]->link_ind);
	    return (-1);
	}
    }
    if (!Simple_code)
	CMPR_set_additional_processing (Copy_req_fields);

    return (0);
}

/**************************************************************************

    Sets shared banwidth mode. In this mode, link->line_rate is the total 
    rate shared by all links managed by this comms manager. link->line_rate 
    must be the same for all links managed.

**************************************************************************/

void TCP_set_share_bandwidth () {
    Share_bw = 1;
}

/**************************************************************************

    Description: This function calls the tcp write function to do the job.

    Inputs:	links - the list of the link structure;
		pvc - the PVC number.

**************************************************************************/

void HA_write_data (Link_struct *link, int pvc)
{

    TCP_write_data (link, pvc);
    return;
}

/**************************************************************************

    Description: This function is called when a new data write request is
		processed. If there is any unfinished control message
		write (Buffer_msgs (BM_WRITE, ...) != 0), we can not 
		start data write immediately. This will be called later 
		when the control message write is completed.

    Inputs:	links - the list of the link structure;
		pvc - the PVC number.

**************************************************************************/

void XP_write_data (Link_struct *link, int pvc)
{
    Tcp_msg_header *hd;
    int ret;

    if (link->w_buf[pvc] == NULL)
	return;
    ret = Buffer_msgs (BM_WRITE, link, pvc, NULL);
    if (ret < 0)
	return;
    else if (ret > 0) {
	if (link->w_started[pvc] == CMT_WRITE_DONE)
	    link->w_started[pvc] = CMT_WRITE_PENDING;
	return;
    }

    if (link->w_started[pvc] != CMT_WRITE_STARTED) {
	hd = (Tcp_msg_header *)link->w_tcp_hd[pvc];
	if (link->data_en == 0)
	    hd->param = 0;		/* no ACK requested */
	else
	    hd->param = htonl (link->sent_seq_num[pvc]);
	hd->type = htonl (TCM_DATA);
	if (link->w_compressed)
	    hd->length = htonl (link->w_size[pvc] | TCM_COMPRESS_FLAG);
	else
	    hd->length = htonl (link->w_size[pvc]);
	link->w_hd_cnt[pvc] = sizeof (Tcp_msg_header);
	link->w_started[pvc] = CMT_WRITE_STARTED;
    }
    TCP_write_data (link, pvc);
    return;
}

/**************************************************************************

    Description: This function is called when a client connection retrial is 
		needed.

    Inputs:	link - the link with data ready.

**************************************************************************/

void TCP_client_connect (Link_struct *link)
{

    End_disconnecting (link, -1);
    return;
}

/**************************************************************************

    Description: This function is called when data are ready on a socket.

    Inputs:	link - the link with data ready.
		fd - the socket with data ready.

**************************************************************************/

void TCP_process_input (Link_struct *link, int fd)
{

    Rand_cnt++;
    if (link->server_fd == fd)
	link->ready_pvc = SERVER_READY;
    else if (!SH_is_test_fd (link, fd)) {
	int pvc;

	for (pvc = 0; pvc < link->n_pvc; pvc++) {
	    if (link->pvc_fd[pvc] == fd) {
		link->ready_pvc = pvc;
		break;
	    }
            else if ( link->server == CMT_FAACLIENT &&
                      link->ch2_link != NULL ) {
               if (link->ch2_link->pvc_fd[pvc] == fd) {
		  link->ch2_link->ready_pvc = pvc;
		  break;
               }
            }
	} /* end for */
	if (pvc >= link->n_pvc) { /* This can happen upon connection lost and 
				     other cases where sockets have been 
				     closed after poll call in SH_poll */
	    close (fd);		  /* This should never be needed. I do it for
				     savety: avoiding any repeated call */
	    return;
	}
    }

    if (link->conn_activity == DISCONNECTING)
	Disconnect (link);
    else if (link->conn_activity == CONNECTING) 
	End_disconnecting (link, fd);
    else if (link->link_state == LINK_CONNECTED && link->ready_pvc >= 0)
	Read_data (link, link->ready_pvc);
    else {
	LE_send_msg (GL_ERROR, "Unexpected socket ready (fd %d), link %d", 
						fd, link->link_ind);
	close (fd);	/* close it to avoid repeated call */
	Process_exception (link); 
    }

    return;
}

/**************************************************************************

    Description: This function calls HA_connection_procedure to do the job.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_connection_procedure (Link_struct *link)
{

    HA_connection_procedure (link);
    return;
}

/**************************************************************************

    This function starts the connection procedure for "link". The data
    coming with CM_CONNECT is checked and, if it is correct, is used. If
    the old code is not used, ch2_name and ch2_address are no longer used.
    server_name needs to be set for no-redundant server case and rs_name
    must be set for redundant server case. For the moment we set all.

**************************************************************************/

void HA_connection_procedure (Link_struct *link)
{

    /* Use dynamic IP/port if found in the CM_CONNECT request */
    if (link->conn_activity == CONNECTING && 
	(link->server == CMT_CLIENT || link->server == CMT_FAACLIENT)) {
	int user_spec_ip;
	Req_struct *req = &(link->req[(int)link->conn_req_ind]);
	user_spec_ip = 0;
	if (req->data != NULL) {	/* parse the CM_CONNECT data */
	    int port, ret;
	    char c, ip1[256], ip2[256], *dt;

	    dt = req->data + sizeof (CM_req_struct);
	    if (strlen (dt) >= 256 ||	/* to protest next sscanf */
		(ret = sscanf (dt, "%s %d %s %c", ip1, &port, ip2, &c))
			< 2 || ret == 4) {
		LE_send_msg (GL_ERROR, 
		    "Bad IP/port in CM_CONNECT request (%s) - not used, link %d", dt, link->link_ind);
	    }
	    else {
		if (link->server == CMT_FAACLIENT) {
		    if (ret <= 2)
			LE_send_msg (GL_ERROR, "Bad IP/port in CM_CONNECT request (%s) - Second IP not found, link %d", dt, link->link_ind);
		    else {
			link->ch2_name = Copy_string (link->ch2_name, ip2);
			link->rs_name[1] = Copy_string (link->rs_name[1], ip2);
			user_spec_ip = 1;
		    }
		}
		else {
		    if (ret > 2)
			LE_send_msg (GL_ERROR, "Bad IP/port in CM_CONNECT request (%s) - Second IP not needed, link %d", dt, link->link_ind);
		    else
			user_spec_ip = 1;
		}
	    }
	    if (user_spec_ip) {		/* Use the user specified IP/port */
		link->server_name = Copy_string (link->server_name, ip1);
		if (link->server == CMT_FAACLIENT)
		    link->rs_name[0] = Copy_string (link->rs_name[0], ip1);
		link->port_number = port;
		LE_send_msg (LE_VL2, "    User specified ip/port: %s, link %d",
						dt, link->link_ind);
	    }
	}
	if (!user_spec_ip &&		/* restore to tcp.conf definitions */
		(link->user_spec_ip || link->server == CMT_FAACLIENT)) {
	    link->server_name = Copy_string (link->server_name, 
						link->conf_rs_name[0]);
	    if (link->server == CMT_FAACLIENT) {
		link->ch2_name = Copy_string (link->ch2_name, 
						link->conf_rs_name[1]);
		link->rs_name[0] = Copy_string (link->rs_name[0], 
						link->conf_rs_name[0]);
		link->rs_name[1] = Copy_string (link->rs_name[1], 
						link->conf_rs_name[1]);
	    }
	    link->port_number = link->conf_port_number;
	}
	if (user_spec_ip || link->user_spec_ip) {	/* free address and 
		ch2_address so they will be updated since IP/port may change */
	    if (link->ch2_address != NULL && 
		link->ch2_address != link->address)
		free (link->ch2_address);
	    if (link->address != NULL)
		free (link->address);
	    link->address = link->ch2_address = NULL;
	}
	link->user_spec_ip = user_spec_ip;
    }

    link->link_state = LINK_DISCONNECTED;
    CMPR_cleanup (link);
    Disconnect (link);

    return;
}

/**************************************************************************

    Copies string from from_name to to_name. Both are malloced spaces. In 
    case of malloc failure, we do not do the copy. Returns the new to_name.

**************************************************************************/

static char *Copy_string (char *to_name, char *from_name) {
    char *p = malloc (strlen (from_name) + 1);
    if (p == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (to_name);
    }
    strcpy (p, from_name);
    if (to_name != NULL)
	free (to_name);
    return (p);
}

/**************************************************************************

    Description: This function implements the first step of the TCP
		connection procedure. It disconnects the connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void Disconnect (Link_struct *link)
{
    int pvc;

    if (link->server_fd >= 0) {	/* close parent socket for the server */
       close (link->server_fd);
       link->server_fd = -1;
    }

    if (link->server == CMT_FAACLIENT &&
        link->ch2_link != NULL) {

       /* close all PVC sockets on channel 2 */
       for (pvc = 0; pvc < link->n_pvc; pvc++) {
           if (link->ch2_link->pvc_fd[pvc] >= 0) {
           shutdown (link->ch2_link->pvc_fd[pvc], 2);
           close (link->ch2_link->pvc_fd[pvc]);
        }
        if (link->ch2_link->ch2_fd[pvc] >= 0) {
           shutdown (link->ch2_link->ch2_fd[pvc], 2);
           close (link->ch2_link->ch2_fd[pvc]);
        }
        link->ch2_link->pvc_fd[pvc] = -1;
        link->ch2_link->ch2_fd[pvc] = -1;
        link->ch2_link->login_state[pvc] = DISABLED;
        link->ch2_link->connect_state[pvc] = DISABLED;
        link->ch2_link->r_msg_size[pvc] = 0;
        link->ch2_link->w_blocked[pvc] = 0;
        link->ch2_link->read_ready[pvc] = 1;
        link->ch2_link->r_hd_cnt[pvc] = 0;
        link->ch2_link->w_hd_cnt[pvc] = 0;
        link->ch2_link->w_started[pvc] = 0;
       }
       link->ch2_link->connect_st_time = 0;
       link->ch2_link->ready_pvc = NOT_READY;
       link->ch2_link->client_ip = CLIENT_ADDR_UNDEFINED;
	
    }

    /* close all PVC sockets */
    for (pvc = 0; pvc < link->n_pvc; pvc++) {
	if (link->pvc_fd[pvc] >= 0) {
           shutdown (link->pvc_fd[pvc], 2);
           close (link->pvc_fd[pvc]);
        }
	if (link->ch2_fd[pvc] >= 0) {
           shutdown (link->ch2_fd[pvc], 2);
           close (link->ch2_fd[pvc]);
        }
	link->pvc_fd[pvc] = -1;
	link->ch2_fd[pvc] = -1;
	link->login_state[pvc] = DISABLED;
	link->connect_state[pvc] = DISABLED;
	link->r_msg_size[pvc] = 0;
	link->w_blocked[pvc] = 0;
	link->read_ready[pvc] = 1;
	link->r_hd_cnt[pvc] = 0;
	link->w_hd_cnt[pvc] = 0;
	link->w_started[pvc] = CMT_WRITE_DONE;
	link->log_cnt[pvc] = 0;
	Buffer_msgs (BM_CLEAR, link, pvc, NULL);
    }
    link->connect_st_time = 0;
    link->ready_pvc = NOT_READY;
    link->client_ip = CLIENT_ADDR_UNDEFINED;
    if (Simple_code && link->server == CMT_FAACLIENT) {
	int i;
	for (i = 0; i < link->n_rss; i++) {
	    if (link->tfd[i] >= 0)
		close (link->tfd[i]);
	    link->tfd[i] = -1;
	    if (link->rs_addrs[i] != NULL)
		free (link->rs_addrs[i]);
	    link->rs_addrs[i] = NULL;
	}
	link->ch_num = 0;
    }

    End_disconnecting (link, -1);

    return;
}

/**************************************************************************

    Description: This function implements the second step of the TCP
		connection procedure. It finishes the connection request 
		if the goal is to disconnect. Otherwise it continues the 
		procedure to make a new connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_disconnecting (Link_struct *link, int fd)
{

    link->tnb_sent = 0;
    link->tnb_received = 0;

    if (link->server == CMT_FAACLIENT && link->ch2_link != NULL) {
	End_faa_disconnecting (link, fd);
	return;
    }

    if (link->conn_activity == DISCONNECTING) {	/* stop here */

        if (link->network == CMT_PPP) {
           SNMP_disable (link);
        }

        if (link->dynamic_dial) {
           link->server = CMT_DYNADIAL;
        }

	CMPR_cleanup (link);
	LE_send_msg (LE_VL1, "Link disconnected, link %d\n", link->link_ind);

	link->link_state = LINK_DISCONNECTED;
	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    }
    else if (link->conn_activity == CONNECTING) {/* go on to next step */

       if (link->server == CMT_SERVER) {
          Server_connection_procedure (link);
       }
       else if (link->server == CMT_CLIENT ||
                link->server == CMT_FAACLIENT ) {

          Client_connection_procedure (link);
       }

    }

    return;
}

/**************************************************************************

    Description: This function implements the second step of the TCP
		connection procedure. It finishes the connection request 
		if the goal is to disconnect. Otherwise it continues the 
		procedure to make a new connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_faa_disconnecting (Link_struct *link, int afd)
{
    int n_clean_pvc = 0;
    int pvc;

    if (link->conn_activity == DISCONNECTING) {	/* stop here */

        link->faa_init_flag = 0; 

        if (link->network == CMT_PPP) {
           SNMP_disable (link);
        }

        if (link->dynamic_dial) {
           link->server = CMT_DYNADIAL;
        }

	CMPR_cleanup (link);
	LE_send_msg (LE_VL1, "Link disconnected, link %d\n", link->link_ind);

	link->link_state = LINK_DISCONNECTED;
	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    }
    else if (link->conn_activity == CONNECTING) {/* go on to next step */
	int ch1_some_pvc_connected, ch2_some_pvc_connected;

       if (link->server == CMT_SERVER) {
          Server_connection_procedure (link);
       }
       else if (link->server == CMT_CLIENT ||
                link->server == CMT_FAACLIENT ) {

          if (link->server == CMT_FAACLIENT && link->faa_init_flag==0 &&
             link->ch2_link != NULL ) {
             /* initialize the ch2 link only once */
             for (pvc = 0; pvc < link->ch2_link->n_pvc; pvc++) {
                if (link->ch2_link->pvc_fd[pvc] < 0) {
                   n_clean_pvc++;
                } else {
                   break;
                }
             }
             if (n_clean_pvc >= link->ch2_link->n_pvc) {

                link->faa_init_flag=1;
                memcpy (link->ch2_link, link, sizeof (Link_struct)); 
                for (pvc = 0; pvc < link->ch2_link->n_pvc; pvc++) { 
                   link->ch2_link->pvc_fd[pvc] = -1;
                }

                link->ch2_link->ch_num = 2;
                link->ch2_link->server_name = link->ch2_name;
                link->ch2_link->address = link->ch2_address;
             }

          }

	  ch1_some_pvc_connected = 0;
	  ch2_some_pvc_connected = 0;
	  for (pvc = 0; pvc < link->n_pvc; pvc++) {
		if (link->connect_state[pvc] == ENABLED) {
		    ch1_some_pvc_connected = 1;
		    break;
		}
	  }
	  if (link->server == CMT_FAACLIENT &&
              link->ch2_link != NULL) {
	      for (pvc = 0; pvc < link->ch2_link->n_pvc; pvc++) {
		    if (link->ch2_link->connect_state[pvc] == ENABLED) {
			ch2_some_pvc_connected = 1;
			break;
		    }
	      }
	  }
	  if (ch1_some_pvc_connected && ch2_some_pvc_connected) {
	      LE_send_msg (GL_ERROR, "Both channels are connected - Check comms configuration");
	      Process_exception (link);
	      return;
	  }
   
          for (pvc = 0; pvc < link->n_pvc; pvc++) {
             if (link->pvc_fd[pvc] == afd
		  || (afd == -1 && link->connect_state[pvc] == ENABLING)
                  || link->connect_state[pvc] != ENABLING
                  || link->dynamic_dial) {

                if (afd != -1 && link->connect_state[pvc] == DISABLED) {
                   break;
                }

		if (link->conn_activity == NO_ACTIVITY)
		    return;
                Client_connection_procedure (link);
                break;
             }
          }

          if (link->link_state != LINK_CONNECTED &&
              link->server == CMT_FAACLIENT &&
              link->ch2_link != NULL ) {

             for (pvc = 0; pvc < link->ch2_link->n_pvc; pvc++) {
                if (link->ch2_link->pvc_fd[pvc] == afd
		    || (afd == -1 && link->ch2_link->connect_state[pvc] == ENABLING)
                    || link->ch2_link->connect_state[pvc] != ENABLING
                    || link->ch2_link->dynamic_dial ) {

                   if (afd != -1 && link->ch2_link->connect_state[pvc] == DISABLED) {
                      break;
                   }

		   link->ch2_link->ch1_link = link;
                   Client_connection_procedure (link->ch2_link);
		   if (link->conn_activity == NO_ACTIVITY)
		       return;
                   break;
                }
             }

             if (link->ch2_link->link_state == LINK_CONNECTED) {
                int i;

                LE_send_msg (LE_VL1, "Channel 2 is connected, link %d\n", link->link_ind);

                
                for (i=0; i< link->n_pvc; i++) {
                   close (link->pvc_fd[i]);
                   link->pvc_fd[i] = link->ch2_link->pvc_fd[i];
                   link->read_ready[i] = link->ch2_link->read_ready[i];
                   link->ch2_link->pvc_fd[i]= -1;
                   link->connect_state[i] = link->ch2_link->connect_state[i]; 
                   link->login_state[i] = link->ch2_link->login_state[i];
                   link->ch2_link->connect_state[i] = DISABLED ;
                   link->ch2_link->login_state[i] = DISABLED ;
                   link->alive_time[i] = link->ch2_link->alive_time[i];
                }
                link->link_state = link->ch2_link->link_state;
                link->connect_st_time = link->ch2_link->connect_st_time;
                link->ch2_link->link_state = LINK_DISCONNECTED; 
                link->conn_activity = link->ch2_link->conn_activity;
                /* MORE VARS???? */

                link->dial_activity = link->ch2_link->dial_activity;
                link->dial_state = link->ch2_link->dial_state;

                link->ch2_link->conn_activity = DISCONNECTING; 
                link->n_reqs = link->ch2_link->n_reqs;
                for (i=0; i < link->n_reqs; i++) {
                   memcpy (&link->req[i], &link->ch2_link->req[i], sizeof (Req_struct));
                }

             }
          } else if (link->link_state == LINK_CONNECTED &&
                     link->server == CMT_FAACLIENT &&
                     link->ch2_link != NULL ) {
             int i;

             LE_send_msg (LE_VL1, "Channel 1 is connected, link %d\n", link->link_ind);

             for (i=0; i< link->n_pvc; i++) {
                close (link->ch2_link->pvc_fd[i]);
                link->ch2_link->pvc_fd[i]= -1;
                link->ch2_link->connect_state[i] = DISABLED ;
                link->ch2_link->login_state[i] = DISABLED ;
             }
             link->ch2_link->link_state = LINK_DISCONNECTED;
          }

       }

    }

    return;
}

/**************************************************************************

    Description: This function starts a client connection procedure.

    Inputs:	link - the link involved.

**************************************************************************/
static void Client_connection_procedure (Link_struct *link)
{
    int pvc, done, ret_val;
    int n_clean_pvc = 0;

    /* initialize the hardware only once */
    for (pvc = 0; pvc < link->n_pvc; pvc++) {
        if (link->pvc_fd[pvc] < 0) {
           n_clean_pvc++;
        } else {
           break;
        }
    }
    if (n_clean_pvc >= link->n_pvc) {
           if (link->network == CMT_PPP 
              && link->server != CMT_DYNADIAL) { 
              if (link->dynamic_dial) {
                 /* initialize dial */
                 DO_init (link);
              }
              if (SNMP_enable (link) < 0) {
		Process_exception (link);
		return;
	      }
            }
    }

    if (Simple_code && Test_connect (link) < 0)
	return;

    done = 1;
    for (pvc = 0; pvc < link->n_pvc; pvc++) {

	if (link->pvc_fd[pvc] < 0) {
	    int fd;

	    fd = SOCK_open_client (&link->address, 
                              link->server_name, link->port_number);
	    if (fd < 0) {
		Process_exception (link);
		return;
	    }
	    link->pvc_fd[pvc] = fd;
	    link->last_conn_time = MISC_systime (NULL);

	} else if (link->network == CMT_PPP && link->server != CMT_DYNADIAL) {
            if (link->dynamic_dial) {
               ret_val = DO_chk (link);
               if (ret_val != CM_IN_PROCESSING) {
                  DO_Process_exception (link, ret_val);
		  return;
               }
            }
        }

	if (link->connect_state[pvc] != ENABLED ) {
	    int ret;

	    if (link->log_cnt[pvc] <= 0) {
		if (link->ch_num > 0)
		    LE_send_msg (LE_VL1,
			"    Connecting, ch %d,  pvc %d, link %d\n",
			link->ch_num, pvc, link->link_ind);
		else
		    LE_send_msg (LE_VL1,
			"    Connecting, pvc %d, link %d\n",
			pvc, link->link_ind);
		link->log_cnt[pvc] = 5;
	    }
            link->connect_state[pvc] = ENABLING;

	    if (Simple_code && link->server == CMT_FAACLIENT && pvc == 0)
		ret = 0;			/* already connected */
	    else
		ret = SOCK_connect (&link->pvc_fd[pvc], link->address);

	    if (ret == SOCK_CONNECT_ERROR) {	/* error */
	        Process_exception (link);
	        return;
	    }

	    if (ret == SOCK_NOT_CONNECTED) {	/* not connected */
		link->log_cnt[pvc]--;
		done = 0;
	        continue;
	    }

   	    if (LOGIN_send_login (link, pvc) < 0) {
	       Process_exception (link);
	       return;
            }
	    link->connect_state[pvc] = ENABLED;
	    if (pvc > 0)		/* stop reading all PVCs except 0 */
	       link->read_ready[pvc] = 0;
	    if (link->connect_st_time == 0)
	       link->connect_st_time = MISC_systime (NULL);
	}
	if (link->connect_state[pvc] != ENABLED)
	    done = 0;   
    }

    if (done) {
	Client_login (link);
    }

    return;
}

/*********************************************************************
			
    Tries to connect to the two servers. If one of them is connected,
    we close the other and sets the ch_num. This is the first step for
    CMT_FAACLIENT client connection. Before connection procedure starts,
    tfd? must be set to -1, tfd?_conn to 0 and ch_num to 0. Returns 0
    if one of the connections is established or -1 otherwise. The 
    connected fd is assigned to pvc[0] and its address (either address
    or ch2_address) is used for the link.
            
********************************************************************/

static int Test_connect (Link_struct *link) {
    int i, conn_ind;

    if (link->ch_num > 0 || link->server != CMT_FAACLIENT)
	return (0);

    for (i = 0; i < link->n_rss; i++) {
	if (link->tfd[i] < 0) {
	    if (i == 0)
		LE_send_msg (LE_VL2, "    Testing active server (%s %s), link %d", 
			link->rs_name[0], link->rs_name[1], link->link_ind);
	    link->tfd[i] = SOCK_open_client (&(link->rs_addrs[i]), 
			      link->rs_name[i], link->port_number);
	    if (link->tfd[i] < 0) {
		Process_exception (link);
		return (-1);
	    }
	    link->last_conn_time = MISC_systime (NULL);
	}
    }

    conn_ind = -1;
    for (i = 0; i < link->n_rss; i++) {
	if (link->tfd[i] >= 0) {
	    int ret = SOCK_connect (&(link->tfd[i]), link->rs_addrs[i]);
	    if (ret == 0) {
		conn_ind = i;
		break;
	    }
	    else if (ret == SOCK_CONNECT_ERROR) {
		Process_exception (link);
		return (-1);
	    }
	}
    }
    if (conn_ind < 0)
	return (-1);

    for (i = 0; i < link->n_rss; i++) {
	if (i == conn_ind) {
	    link->ch_num = i + 1;
	    link->pvc_fd[0] = link->tfd[i];
	    link->address = link->rs_addrs[i];
	    link->server_name = Copy_string (link->server_name,
					link->rs_name[i]);
	    LE_send_msg (LE_VL2, "    Ch %d active (%s), link %d", 
			link->ch_num, link->server_name, link->link_ind);
	}
	else {
	    if (link->rs_addrs[i] != NULL)
		free (link->rs_addrs[i]);
	    if (link->tfd[i] >= 0)
		close (link->tfd[i]);

	}
	link->tfd[i] = -1;
	link->rs_addrs[i] = NULL;
    }

    return (0);
}

/**************************************************************************

    Description: This function waits for the login acknowledgment.

    Inputs:	link - the link involved.

**************************************************************************/

static void Client_login (Link_struct *link)
{

    if (link->login_state[0] != ENABLED && 
	link->ready_pvc == 0) {
	int ret;

        link->login_state[0] = ENABLING;
	if ((ret = LOGIN_receive_ack (link)) == TCM_BAD_MSG) {	/* error */
            if (link->server == CMT_FAACLIENT && link->ch2_link != NULL) {
               int pvc = 0;
 
               /* retry for a faaclient. This is needed to get
                  through the OPUP firewall */
               for (pvc = 0; pvc < link->n_pvc; pvc++) {
                  close (link->pvc_fd[pvc]);
                  link->pvc_fd[pvc] = SOCK_open_client (&link->address, 
                              link->server_name, link->port_number);
                  if (link->pvc_fd[pvc] < 0) {
	             Process_exception (link);
	             return;
                  }
                  link->login_state[pvc] = DISABLED;
	          link->connect_state[pvc] = DISABLED;
               }
            }else {
	       Process_exception (link);
            }             
	    return;
	}
	else if (ret == TCM_MSG_INCOMPLETE)		/* no login info */
	    return;
	else {				/* login succeeded */
	    link->login_state[0] = ENABLED;
	}
    }

    if (link->login_state[0] == ENABLED)
	End_connecting (link);

    return;
}

/**************************************************************************

    Description: This function starts a connection procedure.

    Inputs:	link - the link involved.

**************************************************************************/

static void Server_connection_procedure (Link_struct *link)
{
    int pvc, done;

    if ((link->server_fd < 0)  &&
        (link->connect_state[0] != ENABLED)) {

	if (link->network == CMT_PPP &&
	    SNMP_enable (link) < 0) {
	    Process_exception (link);
	    return;
	}

	link->server_fd = SOCK_open_server (link);
	if (link->server_fd < 0) {
	    Process_exception (link);
	    return;
	}
    }

    done = 1;
    for (pvc = 0; pvc < link->n_pvc; pvc++) {
	if (link->connect_state[pvc] != ENABLED && 
	    link->ready_pvc == SERVER_READY) {
	    int fd;
	    unsigned int cadd;

            link->connect_state[pvc] = ENABLING;
	    fd = SOCK_accept_client (link, &cadd);
	    if (fd == SOCK_CONNECT_ERROR) {		/* error */
	        Process_exception (link);
	        return;
	    }
	    else if (fd == SOCK_NOT_CONNECTED) {	/* no client */
	        return;
	    }
	    else {				/* connected */
		LE_send_msg (LE_VL1,
			"    Accepting connect for pvc %d, link %d\n",
						pvc, link->link_ind);
		if (link->client_ip == CLIENT_ADDR_UNDEFINED)
		    link->client_ip = cadd;
		else {
		    if (link->client_ip != cadd) {
		        ALIGNED_t buf[ALIGNED_T_SIZE 
					(sizeof (CM_resp_struct) + 128)];
		        CM_resp_struct *resp;
			char *cp;

			LE_send_msg (GL_INFO,  
			"pvc from different host (%x %x), pvc %d, link %d\n", 
				link->client_ip, cadd, pvc, link->link_ind);
			cp = (char *)buf + sizeof (CM_resp_struct);
			sprintf (cp, "Connection From Different IPs (%s %s)", 
			    Get_ip_text (link->client_ip), Get_ip_text (cadd));
			resp = (CM_resp_struct *)buf;
			resp->data_size = strlen (cp) + 1;
			CMPR_send_event_response (link, CM_STATUS_MSG, (char *)buf);
		        Process_exception (link);
		        return;
		    }
		}
	        link->pvc_fd[pvc] = fd;
		if (link->connect_st_time == 0)
		    link->connect_st_time = MISC_systime (NULL);
	        link->connect_state[pvc] = ENABLED;
	    }
	}
	if (link->connect_state[pvc] != ENABLED)
	    done = 0;   
    }

    if ((done) && (link->server_fd >= 0)) {
         close (link->server_fd);  /* close server parent socket */
         link->server_fd = -1;
    }

    Server_accept_login (link);

    return;
}

/**************************************************************************

    Description: This function accepts a client's login.

    Inputs:	link - the link involved.

**************************************************************************/

static void Server_accept_login (Link_struct *link)
{
    int pvc, done;

    for (pvc = 0; pvc < link->n_pvc; pvc++) {
	if (link->login_state[pvc] != ENABLED && 
	    link->ready_pvc == pvc) {
	    int ret;

            link->login_state[pvc] = ENABLING;
	    if ((ret = LOGIN_accept_login (link, pvc)) == TCM_BAD_MSG) {
		Send_status_event (link, "Bad CM_TCP Login");
		Process_exception (link);
	        return;
	    }
	    else if (ret == TCM_MSG_INCOMPLETE)		/* no login info */
	        return;
	    else {				/* login succeeded */
	        link->login_state[pvc] = ENABLED;
		link->read_ready[pvc] = 0;	/* stop reading until all pvcs
						   log in */
		if (ret != pvc) {
		    Exchange_pvcs (link, ret, pvc);
		    pvc = -1;
		}
	    }
	}
    }

    done = 1;
    for (pvc = 0; pvc < link->n_pvc; pvc++) {
	if (link->login_state[pvc] != ENABLED)
	    done = 0;   
    }
    if (done) {
	LE_send_msg (LE_VL1, "    All pvc login succeeded, link %d\n", 
						link->link_ind);
	if (LOGIN_send_server_ack (link) < 0) {	/* send ack to the client */
	    Send_status_event (link, "Unexpected CM_TCP Client Disconnect");
	    Process_exception (link);
	    return;
	}
	End_connecting (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the last step of the TCP server 
		connection procedure.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_connecting (Link_struct *link)
{
    int pvc;
    time_t cr_time;

    /* stop here */
    link->connect_st_time = 0;
    cr_time = MISC_systime (NULL);
    for (pvc = 0; pvc < link->n_pvc; pvc++) {
	link->read_ready[pvc] = 1;
	link->alive_time[pvc] = cr_time;
    }
    
    link->n_bytes_in_window = 0;
    link->tnb_sent = 0;
    link->tnb_received = 0;
    LE_send_msg (LE_VL1 | 1045,  "link %d connected\n", link->link_ind);
    Send_status_event (link, "CONNECTED");

    CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    link->link_state = LINK_CONNECTED;

    return;
}

/**************************************************************************

    Copies changed fields in ch2_link to link so CM common code will perform
    correctly.

**************************************************************************/

static void Copy_req_fields (Link_struct *link) {

    if (!Simple_code && link->ch_num == 2) {
	Req_struct *req, *req2;
	req = &(link->ch1_link->req[(int)link->conn_req_ind]);
	req2 = &(link->req[(int)link->conn_req_ind]);
	req->state = req2->state;
	req->data = req2->data;
    }
}

/**************************************************************************

    Description: This function writes data to the "pvc" on "link". It 
		tries first to write any data in the header (data message
		header or a control message). When the header write is
		completed, it continues to write any user data.

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

void TCP_write_data (Link_struct *link, int pvc)
{

    if (link->link_state != LINK_CONNECTED) {		/* link failed */
	link->w_started[pvc] = CMT_WRITE_DONE;
	CMPR_send_response (link, &(link->req[(int)link->w_req_ind[pvc]]), 
					CM_DISCONNECTED);
	Buffer_msgs (BM_WRITE, link, pvc, NULL);
	return;
    }

    if (link->pvc_fd[pvc] < 0) {
	LE_send_msg (GL_ERROR, "PVC fd error, pvc %d, link %d", 
						pvc, link->link_ind);
	CMPR_send_response (link, 
			&(link->req[(int)link->w_req_ind[pvc]]), CM_FAILED);
	Process_exception (link);
	return;
    }

    if (link->w_started[pvc] != CMT_WRITE_STARTED) {
	if (Buffer_msgs (BM_WRITE, link, pvc, NULL) != 0)
	    return;
	else if (link->w_started[pvc] == CMT_WRITE_PENDING)
	    XP_write_data (link, pvc);		/* recursive call */
    }
    else if (link->w_cnt[pvc] >= link->w_size[pvc])
	Buffer_msgs (BM_WRITE, link, pvc, NULL);

    if (link->w_buf[pvc] == NULL)
	return;

   if (link->w_cnt[pvc] < link->w_size[pvc]) {		/* write user data */
	int len, ret;

	len = TCP_flow_cntl_len (link, link->w_size[pvc] - link->w_cnt[pvc]);
	if (len > 0) {
	    char tbuf[sizeof (Tcp_msg_header)];
            char *hp = NULL;
            char *wp = NULL;
	    int w_hd_cnt;  /* for writing data with a single socket write */

	    LE_send_msg (LE_VL3, 
		"    TCP_write_data %d bytes (rate %d), sequence %d, pvc %d, link %d", 
			len, link->line_rate, link->sent_seq_num[pvc], 
			pvc, link->link_ind);
	    wp = link->w_buf[pvc] + link->w_cnt[pvc];
	    w_hd_cnt = link->w_hd_cnt[pvc];
	    if (w_hd_cnt > 0) {		/* copy header and adjust wp and len */
		hp = link->w_buf[pvc] - sizeof (Tcp_msg_header);
		len += w_hd_cnt;
		wp -= w_hd_cnt;
		memcpy (tbuf, hp, sizeof (Tcp_msg_header));
		memcpy (hp, link->w_tcp_hd[pvc], sizeof (Tcp_msg_header));
	    }
	    ret = SOCK_write (link, pvc, wp, len);
	    if (w_hd_cnt > 0)
		memcpy (hp, tbuf, sizeof (Tcp_msg_header));
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, "Data write failed, pvc %d, link %d", 
						    pvc, link->link_ind);
		CMPR_send_response (link, 
			&(link->req[(int)link->w_req_ind[pvc]]), CM_FAILED);
		Process_exception (link);
		return;
	    }
	    if (ret != len)
		LE_send_msg (LE_VL1, 
		  "    SOCK_write blocked %d bytes written, pvc %d, link %d", 
					    ret, pvc, link->link_ind);
	    if (ret > 0) {
		link->alive_time[pvc] = MISC_systime (NULL);
		link->tnb_sent += ret;
	    }
	    if (w_hd_cnt > 0) {
		int n = w_hd_cnt;
		if (n > ret)
		    n = ret;
		link->w_hd_cnt[pvc] -= n;
		ret -= n;
	    }
	    link->w_cnt[pvc] += ret;
	    if (link->line_rate > 0)
		link->n_bytes_in_window += ret;
	}
	if (link->w_cnt[pvc] < link->w_size[pvc])
	    link->w_blocked[pvc] = 1;
    }
    if (link->data_en == 0)			/* do not expect ACK */
	link->w_ack[pvc] = link->w_cnt[pvc];

    if (link->w_ack[pvc] >= link->w_size[pvc]) {	/* done */
	Req_struct *req = link->req + (int)link->w_req_ind[pvc];
	link->w_started[pvc] = CMT_WRITE_DONE;
	CMPR_send_response (link, req, CM_SUCCESS);
	Buffer_msgs (BM_WRITE, link, pvc, NULL);
    }

    return;
}

/**************************************************************************

    Description: This function reads data from the "pvc" on "link".

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

static void Read_data (Link_struct *link, int pvc)
{
    Tcp_msg_header *hd;
    int len;


    if (link->link_state != LINK_CONNECTED)		/* link failed */
	return;

    if (pvc != link->ready_pvc || link->pvc_fd[pvc] < 0) {
	LE_send_msg (GL_ERROR,  "Read data error, pvc %d, link %d", 
						pvc, link->link_ind);
	return;
    }

    hd = (Tcp_msg_header *)link->r_tcp_hd[pvc];
    if (link->r_hd_cnt[pvc] < (int)sizeof (Tcp_msg_header) || 
						hd->type != TCM_DATA) {
	int type;

	type = SH_read_message (link, pvc);
	switch (type) {

	    case TCM_MSG_INCOMPLETE:
		return;

	    case TCM_BAD_MSG:
		Process_exception (link);
		return;

	    case TCM_DATA_ACK:
		Process_data_ack (link, pvc);
		link->r_hd_cnt[pvc] = 0;
		return;

	    case TCM_KEEP_ALIVE:
		Process_keep_alive (link, pvc);
		link->r_hd_cnt[pvc] = 0;
		return;

	    case TCM_DATA:
		break;

	    default:
		LE_send_msg (GL_INFO, 
		"Unexpected message (type %d), pvc %d, link %d", 
					type, pvc, link->link_ind);
		Process_exception (link);
		return;
	}
    }

    /* process TCM_DATA message */
    if (link->r_cnt[pvc] == 0)
	link->read_start_time[pvc] = MISC_systime (NULL);

    if (link->r_msg_size[pvc] > MAX_INCOMING_DATA_SIZE) {
	char buf[128];
	LE_send_msg (GL_ERROR, 
		"Unexpected incoming data size (%d), pvc %d, link %d", 
				link->r_msg_size[pvc], pvc, link->link_ind);
	sprintf (buf, "Unexpected Incoming Data Size - %d Bytes", 
						link->r_msg_size[pvc]);
	Send_status_event (link, buf);
	Process_exception (link);
	return;
    }

    /* reallocate the buffer */
    while (link->r_msg_size[pvc] > link->r_buf_size[pvc]) {
	if (CMC_reallocate_input_buffer (link, pvc, link->r_msg_size[pvc]) < 0)
	    return;
    }

    len = TCP_flow_cntl_len (link, link->r_msg_size[pvc] - link->r_cnt[pvc]);
    if (len > 0)
	len = SOCK_read (link, link->pvc_fd[pvc], 
				link->r_buf[pvc] + link->r_cnt[pvc], len);

    if (len < 0) {
	LE_send_msg (GL_ERROR, "SOCK_read failed (ret %d), pvc %d, link %d", 
						len, pvc, link->link_ind);
	Process_exception (link);
	return;
    }
    link->tnb_received += len;
    link->r_cnt[pvc] += len;
    if (link->line_rate > 0)
	link->n_bytes_in_window += len;
    if (len > 0)
	link->alive_time[pvc] = MISC_systime (NULL);

    if (link->r_cnt[pvc] < link->r_msg_size[pvc])
	return;

    /* done */
    if (hd->length & TCM_COMPRESS_FLAG)
	CMPR_send_compressed_data (link, pvc);
    else {
	CMPR_send_data (link, pvc, link->r_cnt[pvc], link->r_buf[pvc]);
	LE_send_msg (LE_VL2, "    Message read %d bytes, pvc %d, link %d", 
			    link->r_cnt[pvc], pvc, link->link_ind);
    }
    link->r_cnt[pvc] = 0;
    link->r_hd_cnt[pvc] = 0;
    link->r_msg_size[pvc] = 0;
    Send_data_ack (link, pvc);

    return;
}

/**********************************************************************

    Description: This function processes data acknowledgment message.

    Inputs:	link - the link involved.
		pvc - PVC index.

**********************************************************************/

static void Process_data_ack (Link_struct *link, int pvc) {
    Tcp_msg_header *hd;
    unsigned int seq_num;

    link->alive_time[pvc] = MISC_systime (NULL);
    hd = (Tcp_msg_header *)link->r_tcp_hd[pvc];
    seq_num = hd->param;
    if (link->w_buf[pvc] == NULL || seq_num < link->sent_seq_num[pvc]) {
	LE_send_msg (GL_INFO, "Data ACK (seq %u) discarded, pvc %d, link %d", 
						seq_num, pvc, link->link_ind);
	return;
    }
    if (link->w_cnt[pvc] < link->w_size[pvc]) { /* should never happen */
	LE_send_msg (GL_ERROR, 
			"Unexpected data ACK (seq %u), pvc %d, link %d", 
					    seq_num, pvc, link->link_ind);
	Process_exception (link);
	return;
    }
    LE_send_msg (LE_VL3, "    Data ACK received (seq %u), pvc %d, link %d", 
					    seq_num, pvc, link->link_ind);
    link->w_ack[pvc] = link->w_cnt[pvc];
    (link->sent_seq_num[pvc])++;
    if (link->sent_seq_num[pvc] == 0)
	link->sent_seq_num[pvc] = 1;
    TCP_write_data (link, pvc);
}

/**************************************************************************

    Description: This function processes lost connection and other
		exception conditions.

    Inputs:	link - the link involved.

**************************************************************************/

static void Process_exception (Link_struct *link)
{

    if (link->dynamic_dial) {
       if ( link->link_state == LINK_CONNECTED ) {
         DO_Process_exception (link, CM_MODEM_PROBLEMS);
       } else {
         DO_Process_exception (link, CM_FAILED);
       }
       return;
    } 
    LE_send_msg (LE_VL1, "Processing exception..., link %d", link->link_ind);

    if (link->server == CMT_FAACLIENT && link->ch2_link != NULL && link->ch_num == 2) {
      CMPR_connect_failure (link->ch1_link);
      Disconnect (link->ch1_link);
    } else {
      CMPR_connect_failure (link);
      Disconnect (link);
    }
    LE_send_msg (LE_VL1, "disconnected, link %d", link->link_ind);

    return;
}

/**************************************************************************

    Description: This function sends statistic data to the user.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_statistics_request (struct link_struct *link)
{
    ALIGNED_t buf[ALIGNED_T_SIZE (sizeof (CM_resp_struct) + 
					sizeof (TCP_statistics))];
    TCP_statistics *out;
    CM_resp_struct *resp;
    time_t alive_tm;
    int pvc;

    out = (TCP_statistics *)((char *)buf + sizeof (CM_resp_struct));
    resp = (CM_resp_struct *)buf;
    out->keep_alive_test_period = KEEP_ALIVE_PROB_TIME;
    alive_tm = link->alive_time[0];
    for (pvc = 1; pvc < link->n_pvc; pvc++) {
	if (link->alive_time[pvc] < alive_tm)
	    alive_tm = link->alive_time[pvc];
    }
    out->no_input_time = MISC_systime (NULL) - alive_tm;
    out->rate = CMRATE_get_rate (link);
    out->tnb_sent = link->tnb_sent;
    out->tnb_received = link->tnb_received;
    resp->data_size = sizeof (TCP_statistics);

    CMPR_send_event_response (link, CM_STATISTICS, (char *)buf);
    LE_send_msg (LE_VL1, "sending statistics (rate %d)\n", out->rate);

    return;
}

/**************************************************************************

    Description: This function resets X25/PVC statistics on a port.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_statistics_reset (struct link_struct *link)
{
    return;
}

/**************************************************************************

    Description: This function processes an incoming TCM_KEEP_ALIVE msg.

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

static void Process_keep_alive (Link_struct *link, int pvc)
{
    Tcp_msg_header *hd;

    link->alive_time[pvc] = MISC_systime (NULL);
    hd = (Tcp_msg_header *)link->r_tcp_hd[pvc];
    if (hd->param == 0) {	/* a responding keep alive msg */
	LE_send_msg (LE_VL3, 
		"    KEEP_ALIVE response received, pvc %d, link %d", 
						pvc, link->link_ind);
	return;
    }

    /* send a response */
    Send_short_msg (link, pvc, TCM_KEEP_ALIVE, 0);
    LE_send_msg (LE_VL3, 
		"    KEEP_ALIVE echoed, pvc %d, link %d", 
						pvc, link->link_ind);

    return;
}

/**************************************************************************

    Description: This function initiates a TCM_KEEP_ALIVE msg.

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

static void Send_keep_alive (Link_struct *link, int pvc)
{

    if (link->w_blocked[pvc] ||		/* in the middle of a write */
	link->r_cnt[pvc] < link->r_msg_size[pvc]) /* in the middle of read */
	return;
    Send_short_msg (link, pvc, TCM_KEEP_ALIVE, 1);
    LE_send_msg (LE_VL3, "    KEEP_ALIVE initiated, pvc %d, link %d", 
						pvc, link->link_ind);

    return;
}

/************************************************************************

    Description: This function sends data reception acknowledgment message
		to the sender.

    Inputs:	link - the link involved.
		pvc - PVC index.

************************************************************************/

static void Send_data_ack (Link_struct *link, int pvc) {
    Tcp_msg_header *hd;
    unsigned int seq_num;

    hd = (Tcp_msg_header *)link->r_tcp_hd[pvc];
    seq_num = hd->param;
    if (seq_num == 0)		/* ACK not needed */
	return;

    /* send a ack */
    Send_short_msg (link, pvc, TCM_DATA_ACK, seq_num);

    return;
}

/**************************************************************************

    Description: This function is the house keeping function for TCP.

    Inputs:	link - the link structure;
		cr_time - current time;

**************************************************************************/

void TCP_house_keeping (Link_struct *link, time_t cr_time)
{
    static time_t last_time = 0;

    /* client connection retrial is required */
    if (link->conn_activity == CONNECTING && 
	( link->server == CMT_CLIENT || link->server == CMT_FAACLIENT ) &&
	cr_time > link->last_conn_time + CONN_RETRY_CHECK_TIME) {
	TCP_process_input (link, -1);
	link->last_conn_time = cr_time;
    }

    if (link->connect_st_time > 0 &&	/* connection procedure timed out */
	cr_time >= link->connect_st_time + connection_procedure_time_limit) {
	LE_send_msg (GL_INFO, "Connection procedure timed out, link %d",
					link->link_ind);
	Send_status_event (link, "CM_TCP Connection Procedure Time-out");
	Process_exception (link);
	return;
    }

    /* check status of dial line */
    if (link->link_state == LINK_CONNECTED && link->dynamic_dial) {
       int ret_val;

       ret_val = DO_chk (link);
       if (ret_val != CM_IN_PROCESSING) {
          DO_Process_exception (link, ret_val);
	  return;
       }
    }

    /* keep alive check */
    if (cr_time > last_time + KEEP_ALIVE_CHECK_TIME) {
	int i;
	last_time = cr_time;
	for (i = 0; i < N_links; i++) {
	    int pvc;
    
	    if (Links[i]->link_state != LINK_CONNECTED)
		continue;
	    for (pvc = 0; pvc < Links[i]->n_pvc; pvc++) {
		if (cr_time > Links[i]->alive_time[pvc] + 
				KEEP_ALIVE_PROB_TIME - (Rand_cnt % 5))
		    Send_keep_alive (Links[i], pvc);
	    }
	}
    }

    /* no response to keep alive check */
    if (no_keepalive_response_disconnect_time > MIN_NO_INPUT_TIME) {
	int i;
	for (i = 0; i < N_links; i++) {
	    int pvc;
            int no_input_time;
            time_t alive_tm;
    
	    if (Links[i]->link_state != LINK_CONNECTED)
		continue;
	    for (pvc = 0; pvc < Links[i]->n_pvc; pvc++) {

               alive_tm = Links[i]->alive_time[pvc];
               no_input_time = MISC_systime (NULL) - alive_tm;

               if ( no_input_time > no_keepalive_response_disconnect_time) {
                  LE_send_msg (GL_ERROR, "No Response to Keep Alive Msgs! link %d, no_input_time (%d) > %d\n",
                                       Links[i]->link_ind, no_input_time, no_keepalive_response_disconnect_time);
	          Process_exception (Links[i]);
                  pvc = Links[i]->n_pvc; /* no need to check other pvcs */
               }
	    }
	}
    }

    return;
}

/**************************************************************************

    Processes flow control. This function must be called about 10 times a 
    second.

**************************************************************************/

void TCP_process_flow_control () {
    static double last_time = 0.;
    int i, bw_available, n_bytes_allowed, ms;
    int n_bytes[MAX_N_LINKS];
    double ct, t_delta;
    time_t t;

    t = MISC_systime (&ms);
    ct = (double)t * 1000 + (double)ms;
    if (last_time <= 0.)
	last_time = ct;
    if (ct - last_time < 60.)
	return;

    t_delta = (ct - last_time) * .001;
    last_time = ct;

    if (!Share_bw) {
	for (i = 0; i < N_links; i++) {
	    Link_struct *link = Links[i];
	    if (link->link_state == LINK_CONNECTED) {
		link->n_bytes_in_window -= 
			    (int)(t_delta * (link->line_rate >> 3)) + 1;
		if (link->n_bytes_in_window < 0)
		    link->n_bytes_in_window = 0;
	    }
	}
	return;
    }

    bw_available = (int)(t_delta * (Links[0]->line_rate >> 3)) + 1;
    for (i = 0; i < N_links; i++) {
	if (Links[i]->link_state == LINK_CONNECTED)
	    n_bytes[i] = Links[i]->n_bytes_in_window;
	else
	    n_bytes[i] = 0; 		/* bytes in window */
    }

    while (1) {
	int cnt, ind, min;
	cnt = 0;
	ind = -1;
	min = 0x7fffffff;
	for (i = 0; i < N_links; i++) {
	    if (n_bytes[i] <= 0)
		continue;
	    if (n_bytes[i] < min) {
		min = n_bytes[i];
		ind = i;
	    }
	    cnt++;
	}
	if (cnt == 0)
	    break;
	if (n_bytes[ind] < bw_available / cnt)
	    n_bytes_allowed = n_bytes[ind];
	else
	    n_bytes_allowed = bw_available / cnt;
	bw_available -= n_bytes_allowed;
	Links[ind]->n_bytes_in_window -= n_bytes_allowed;
	if (Links[ind]->n_bytes_in_window < 0)
	    Links[ind]->n_bytes_in_window = 0;
	n_bytes[ind] = 0;
    }
}

/************************************************************************

    Description: This function sends a 0 data length message (keep alive 
		or data ACK message).

    Inputs:	link - the link involved.
		pvc - PVC index.
		type - message type.
		param - the parameter.

************************************************************************/

static void Send_short_msg (Link_struct *link, 
			int pvc, int type, unsigned int param) {
    Tcp_msg_header hd;

    hd.param = htonl (param);
    hd.type = htonl (type);
    hd.length = htonl (0);
    Buffer_msgs (BM_SAVE, link, pvc, &hd);
    TCP_write_data (link, pvc);

    return;
}

/**************************************************************************

    Description: This function processes lost dial connection and other
                exception conditions.

    Inputs:     link - the link involved.
                error - error return code

**************************************************************************/

void DO_Process_exception (Link_struct *link, int ret_value)
{

    LE_send_msg (LE_VL1, "Processing dial-out exception...\n");
    if (link->server == CMT_FAACLIENT && link->ch2_link != NULL && link->ch_num == 2) {
       link->ch1_link->conn_activity = DISCONNECTING;
       Disconnect (link->ch1_link);
       CMPR_dialout_failure (link->ch1_link, ret_value);
    } else {
       link->conn_activity = DISCONNECTING;
       Disconnect (link);
       CMPR_dialout_failure (link, ret_value);
    }

    if (link->network == CMT_PPP) {
       SNMP_disable (link);
    }
    LE_send_msg (LE_VL1, "disconnected, link %d", link->link_ind);

    return;
}

/**************************************************************************

    Exchanges PVC array elements of "ind1" and "ind2".

**************************************************************************/

static void Exchange_pvcs (Link_struct *link, int ind1, int ind2) {
    int t;
    char *p;

    t = link->pvc_fd[ind1];
    link->pvc_fd[ind1] = link->pvc_fd[ind2];
    link->pvc_fd[ind2] = t;
    t = link->ch2_fd[ind1];
    link->ch2_fd[ind1] = link->ch2_fd[ind2];
    link->ch2_fd[ind2] = t;
    t = link->login_state[ind1];
    link->login_state[ind1] = link->login_state[ind2];
    link->login_state[ind2] = t;
    t = link->connect_state[ind1];
    link->connect_state[ind1] = link->connect_state[ind2];
    link->connect_state[ind2] = t;
    p = link->r_buf[ind1];
    link->r_buf[ind1] = link->r_buf[ind2];
    link->r_buf[ind2] = p;
    t = link->r_cnt[ind1];
    link->r_cnt[ind1] = link->r_cnt[ind2];
    link->r_cnt[ind2] = t;
    t = link->r_msg_size[ind1];
    link->r_msg_size[ind1] = link->r_msg_size[ind2];
    link->r_msg_size[ind2] = t;
    t = link->r_buf_size[ind1];
    link->r_buf_size[ind1] = link->r_buf_size[ind2];
    link->r_buf_size[ind2] = t;
    t = link->read_start_time[ind1];
    link->read_start_time[ind1] = link->read_start_time[ind2];
    link->read_start_time[ind2] = t;
    t = link->r_hd_cnt[ind1];
    link->r_hd_cnt[ind1] = link->r_hd_cnt[ind2];
    link->r_hd_cnt[ind2] = t;
    p = link->r_tcp_hd[ind1];
    link->r_tcp_hd[ind1] = link->r_tcp_hd[ind2];
    link->r_tcp_hd[ind2] = p;
    t = link->alive_time[ind1];
    link->alive_time[ind1] = link->alive_time[ind2];
    link->alive_time[ind2] = t;
    t = link->read_ready[ind1];
    link->read_ready[ind1] = link->read_ready[ind2];
    link->read_ready[ind2] = t;
}

/**************************************************************************

    Returns the read/write length in terms of the flow control on link 
    "link". "len" is the requested length.

**************************************************************************/

int TCP_flow_cntl_len (Link_struct *link, int len) {
    int cnt, i, bytes_allocated;

    if (link->line_rate == 0)
	return (len);

    if (Share_bw) {
	cnt = 0;
	for (i = 0; i < N_links; i++) {
	    int pvc, n_bytes;
	    Link_struct *ln = Links[i];
	    n_bytes = 0;
	    if (ln->link_state != LINK_CONNECTED)
		continue;
	    for (pvc = 0; pvc < ln->n_pvc; pvc++) {
		n_bytes += ln->r_msg_size[pvc] - ln->r_cnt[pvc] + 
				    ln->w_size[pvc] - ln->w_cnt[pvc];
	    }
	    if (n_bytes > 0)
		cnt++;
	}
	if (cnt == 0)
	    cnt = 1;
    }
    else
	cnt = 1;
    bytes_allocated = (link->line_rate >> 3) / cnt;
    if (link->n_bytes_in_window >= bytes_allocated)
	return (0);
    if (len > bytes_allocated)
	len = bytes_allocated;

    return (len);
}


/**************************************************************************

    Buffers header only messages and sends them to the socket. If
    "func" = BM_CLEAR, all records matching "link" and "pvc" are
    discarded. If "func" = BM_SAVE, a new record "data" is added with
    "link" and "pvc". If "func" = BM_WRITE, it writes all records for
    "link" and "pvc" to the socket. The function returns -1 on error,
    1 if the socket is blocked, or 0 otherwise.

**************************************************************************/

static int Buffer_msgs (int func, Link_struct *link, 
					int pvc, Tcp_msg_header *data) {
    typedef struct {
	int link_ind;
	int pvc;
	int n_bytes;
	Tcp_msg_header data;
    } Saved_msg_t;
    static void *sm_tid = NULL;	/* table id */
    static Saved_msg_t *sms;	/* saved messages */
    static int n_sms = 0;	/* number of saved messages */
    int l_ind, i, ret;

    l_ind = link->link_ind;
    if (func == BM_WRITE) {
	if (n_sms == 0 || link->link_state != LINK_CONNECTED ||
					link->pvc_fd[pvc] < 0)
	    return (0);

	for (i = 0; i < n_sms; i++) {
	    Saved_msg_t *sm;

	    sm = sms + i;
	    if (sm->link_ind != l_ind || sm->pvc != pvc)
		continue;
	    ret = SOCK_write (link, pvc, 
		(char *)&(sm->data) + (sizeof (Tcp_msg_header) - sm->n_bytes), 
						sm->n_bytes);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, 
				"Buffer_msgs write failed, pvc %d, link %d", 
						pvc, link->link_ind);
		Process_exception (link);
		return (-1);
	    }
	    link->tnb_sent += ret;
	    sm->n_bytes -= ret;
	    if (sm->n_bytes > 0)
		return (1);
	    MISC_table_free_entry (sm_tid, i);
	    i--;
	}
	return (0);
    }
    else if (func == BM_SAVE) {
	Saved_msg_t *new_rec;
	if (sm_tid == NULL) {
	    sm_tid = MISC_open_table (sizeof (Saved_msg_t), 
					16, 1, &n_sms, (char **)&sms);
	    if (sm_tid == NULL) {
		LE_send_msg (GL_ERROR, "malloc failed");
		Process_exception (link);
		return (-1);
	    }
	}
	new_rec = (Saved_msg_t *)MISC_table_new_entry (sm_tid, NULL);
	if (new_rec == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed");
	    Process_exception (link);
	    return (-1);
	}
	new_rec->link_ind = l_ind;
	new_rec->pvc = pvc;
	new_rec->n_bytes = sizeof (Tcp_msg_header);
	memcpy (&(new_rec->data), data, sizeof (Tcp_msg_header));
	return (0);
    }
    else if (func == BM_CLEAR) {
	for (i = n_sms - 1; i >= 0; i--) {
	    if (sms[i].link_ind != l_ind || sms[i].pvc != pvc)
		continue;
	    MISC_table_free_entry (sm_tid, i);
	}
	return (0);
    }
    LE_send_msg (GL_ERROR, "unexpected call to Buffer_msgs");
    return (-1);
}

/**************************************************************************

    Sends status message "msg" to the comm_manager user. The message is
    appended with the client IP address.

**************************************************************************/

static void Send_status_event (Link_struct *link, char *msg) {
    ALIGNED_t buf[ALIGNED_T_SIZE (sizeof (CM_resp_struct) + 128)];
    CM_resp_struct *resp;
    char *cp;

    cp = (char *)buf + sizeof (CM_resp_struct);
    sprintf (cp, "%s (From %s)", msg, Get_ip_text (link->client_ip));
    resp = (CM_resp_struct *)buf;
    resp->data_size = strlen (cp) + 1;
    CMPR_send_event_response (link, CM_STATUS_MSG, (char *)buf);
}

/**************************************************************************

    Returns the IP address "ip" in the text format.

**************************************************************************/

static char *Get_ip_text (unsigned int ip) {
    static char buf[32];
    sprintf (buf, "%d.%d.%d.%d", 
		ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return (buf);
}



