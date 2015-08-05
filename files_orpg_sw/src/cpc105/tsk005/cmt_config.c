
/******************************************************************

	This module reads the link config file for the 
	TCP comm manager.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/08/18 22:30:34 $
 * $Id: cmt_config.c,v 1.32 2010/08/18 22:30:34 ccalvert Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 *
 * History:
 * 11JUN2003 - Chris Gilbert - CCR #NA03-06201 Issue 2-150. Add "faaclient"
 *             support going through a proxy firewall.
 *
 * 02Feb2002 - C. Gilbert - CCR #NA01-34801 Issue 1-886 Part 1. Add FAA support to 
 *                           the client cm_tcp.
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
 *
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing.
 * 12DEC2002 Chris Gilbert - NA02-35402 Issue 2-103 - Changed MAXIF from
 *                           100 to 110 and made if configurable.
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-03502 Issue 2-129 - Add connection_procedure_
 *           time_limit to increase amount of time to go through a firewall.
 *
 * 06FEB2003 Chris Gilbert - CCR NA03-02901 Issue 2-130 - Add no_keepalive_
 *           response_disconnect_time in order to disconect in a timely matter
 *           in case of link breaks.
 * 19Mar2003 Chris Gilbert - CCR NA03-30901 Issue 2-279 - added "redclient" for nws
 *           redundant rda connections. "redclient" has the same functionality as "faaclient".
 *
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmt_def.h"

extern int MAXIF;  /* max interface index number on device (Cisco Router) */
extern int connection_procedure_time_limit; /* defined in cmt_main.c */
extern int no_keepalive_response_disconnect_time; /* defined in cmt_main.c */
extern int Simple_code;

/* local functions */
static int Read_snmp_glob_configs (char *tcp_conf_file);
static int Read_glob_configs (char *tcp_conf_file);
static int Read_tcp_config_file (char *tcp_conf_file, 
				int n_links, Link_struct **links);
static int Read_dialin_configs (char *tcp_conf_file, 
				int link, Link_struct **links);
static int Read_dialout_configs (char *tcp_conf_file, 
				int link, Link_struct **links);
static int Read_phone_table (char *tcp_conf_file,
                             void **phone_recs,
                             int *num_recs);


/**************************************************************************

    Description: This function reads the link configuration file.

    Inputs:	links - the link structure list;

    Output:	device_number - device number;

    Return:	It returns the number of links on success or -1 on failure.

**************************************************************************/

int CC_read_link_config (Link_struct **links)
{
    int n_links;		/* number of links managed by this process */
    char tcp_conf_file[NAME_LEN];

    CS_cfg_name (CMC_get_link_conf_name ());

    n_links = 0;
    while (1) {
	Link_struct *link;

	link = CMC_read_link_config ("cm_tcp");
	if (link == CMC_CONF_READ_DONE)
	    break;
	else if (link == CMC_CONF_READ_FAILED)
	    return (-1);

	if (link->proto == PROTO_HDLC) {
	    LE_send_msg (GL_ERROR,  
		"Error found in %s - Number of PVCs cannot be 0, link %d", 
		CMC_get_link_conf_name (), link->link_ind);
	    return (-1);
	}

	/* store in the link config table */
	links[n_links] = link;
	n_links++;
    }

    if (n_links > 0) {
	if (CS_entry ("TCP_config_file", 1, NAME_LEN, tcp_conf_file) <= 0)
	    return (-1);
    }

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    /* read the TCP config file */
    if (n_links > 0 &&
	Read_tcp_config_file (tcp_conf_file, n_links, links) < 0)
	return (-1);

    return (n_links);
}

/**************************************************************************

    Description: This function reads the tcp configuration file.

    Inputs:	tcp_conf_file - name of the tcp config file;
		n_links - number of links managed;
		links - the link structure list;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

#define BUF_SIZE 84

static int Read_tcp_config_file (char *tcp_conf_file, 
				int n_links, Link_struct **links)
{
    int i,k;
/*    int rate; */
    int dial_table_in = 0;
    void *phone_nums = NULL;
    int            n_phone = 0;


    Read_glob_configs (tcp_conf_file);

    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    if (CS_entry ("TCP_link_specification", 0, 0, NULL) < 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (GL_ERROR | 1001,  
	    "TCP_link_specification section not found in %s", tcp_conf_file);
	return (-1);
    }


    for (i = 0; i < n_links; i++) {
	char buf[BUF_SIZE];
	char server_name[NAME_LEN];
	int tmp;
        int offset;

        offset = 0;

	if (CS_entry_int_key (links[i]->link_ind, 0, BUF_SIZE, buf) <= 0) {
	    LE_send_msg (GL_ERROR | 1002,  
			"TCP config for link %d is not found in %s", 
			links[i]->link_ind, tcp_conf_file);
	    return (-1);
	}


	CS_entry (CS_THIS_LINE, 1, BUF_SIZE, buf);

        links[i]->dynamic_dial = 0;
	if (strcmp (buf, "server") == 0)
	    links[i]->server = CMT_SERVER;
	else if (strcmp (buf, "client") == 0)
	    links[i]->server = CMT_CLIENT;
	else if (strcmp (buf, "faaclient") == 0)
	    links[i]->server = CMT_FAACLIENT;
	else if (strcmp (buf, "redclient") == 0)
	    links[i]->server = CMT_FAACLIENT;
	else if (strcmp (buf, "dynamicdial") == 0) {
	    links[i]->server = CMT_DYNADIAL;
            links[i]->dynamic_dial = 1;
        } 
	else {
	    LE_send_msg (GL_ERROR | 1003,  
			"server/client for link %d not defined", 
			links[i]->link_ind);
	    return (-1);
	}
	links[i]->address = links[i]->ch2_address = NULL;
	links[i]->user_spec_ip = 0;

        if (links[i]->server == CMT_DYNADIAL) {

           if (!dial_table_in) {
              /* Read in the phone number table */
              if (Read_phone_table (tcp_conf_file, &phone_nums, &n_phone) < 0) {
	         return (-1);
	      }
              dial_table_in = 1;
           }

           links[i]->phone_nums = (Phone_struct **)phone_nums;
           links[i]->n_phone = n_phone;
	   links[i]->network = CMT_PPP;

           /* Initialize  */
	   links[i]->server_name = NULL;
	   links[i]->dialout_name = NULL;
           links[i]->faa_init_flag = 0;
	   links[i]->ch2_name = NULL;
	   links[i]->password = NULL;
	   links[i]->ch2_link = NULL;
           continue; 
        }

	if (CS_entry (CS_THIS_LINE, 2 | CS_INT, 0, (char *)&tmp) <= 0) {
	    LE_send_msg (GL_ERROR | 1004,  
			"port number for link %d not defined", 
			links[i]->link_ind);
	    return (-1);
	}
	links[i]->port_number = tmp;
	links[i]->conf_port_number = tmp;

        if (CS_entry (CS_THIS_LINE, 3, NAME_LEN, server_name) <= 0) {
           LE_send_msg (GL_ERROR | 1005,  
                    "interface/hostname for link %d not defined", 
                     links[i]->link_ind);
           return (-1);
        }
        links[i]->server_name = malloc (strlen (server_name) + 1);
        links[i]->conf_rs_name[0] = malloc (strlen (server_name) + 1);

        if (links[i]->server_name == NULL || 
	    links[i]->conf_rs_name[0] == NULL) {
           LE_send_msg (GL_ERROR | 1007,  "malloc failed");
           return (-1);
        }

        strcpy (links[i]->server_name, server_name);
        strcpy (links[i]->conf_rs_name[0], server_name);


        links[i]->ch2_link = NULL;
	if ( links[i]->server == CMT_FAACLIENT ) {

           offset++;

 	   if (!Simple_code) {
           links[i]->ch1_link = NULL;
           links[i]->ch2_link = malloc (sizeof (Link_struct));
	   if (links[i]->ch2_link == NULL) {
	      LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	      return (-1);
	   }

           /* init ch2_link */           
           links[i]->ch2_link->ch_num = 2;
           links[i]->ch2_link->n_pvc = links[i]->n_pvc;
           links[i]->ch2_link->ch1_link = links[i];
           links[i]->ch2_link->link_state = LINK_DISCONNECTED;
           links[i]->ch2_link->conn_activity = NO_ACTIVITY;
           links[i]->ch2_link->dial_state = NORMAL;
           links[i]->ch2_link->dial_activity = NO_ACTIVITY;
           links[i]->ch2_link->rep_period = 0;
           links[i]->ch2_link->rw_time = 0;
           links[i]->ch2_link->n_added_bytes = 0;
           links[i]->ch2_link->r_seq_num = 0;

           for (k = 0; k < links[i]->n_pvc; k++) {
              links[i]->ch2_link->r_buf[k] = NULL;
              links[i]->ch2_link->r_cnt[k] = 0;
              links[i]->ch2_link->r_buf_size[k] = 0;
              links[i]->ch2_link->w_buf[k] = NULL;
           }
	   }

           if (CS_entry (CS_THIS_LINE, 4, NAME_LEN, server_name) <= 0) {
	      LE_send_msg (GL_ERROR | 1005,  
                       "second interface/hostname for faa link %d not defined", 
                        links[i]->link_ind);
	      return (-1);
           }
	   links[i]->ch2_name = malloc (strlen (server_name) + 1);
	   links[i]->conf_rs_name[1] = malloc (strlen (server_name) + 1);

	   if (links[i]->ch2_name == NULL || 
					links[i]->conf_rs_name[1] == NULL) {
	       LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	       return (-1);
           }

 	   strcpy (links[i]->ch2_name, server_name);
 	   strcpy (links[i]->conf_rs_name[1], server_name);

        }

	if (CS_entry (CS_THIS_LINE, 5 + offset, BUF_SIZE, buf) <= 0) {
	    LE_send_msg (GL_ERROR | 1006,  
			"password for link %d not defined", 
			links[i]->link_ind);
	    return (-1);
	}

	links[i]->password = malloc (strlen (buf) + 1);
	if (links[i]->password == NULL) {
	    LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	    return (-1);
	}
	strcpy (links[i]->password, buf);

/*
        CS_control (CS_KEY_OPTIONAL);
	if (CS_entry (CS_THIS_LINE, (6 + offset)|CS_INT, 0, (char *)&rate) > 0) {
*/
            /* a value was found to override the line_rate. Assign new value  *//*
            links[i]->line_rate = rate;
	    LE_send_msg (LE_VL2, "tcp.conf: link %d line_rate = %d ", 
                                  links[i]->link_ind, rate);
	}
        CS_control (CS_KEY_REQUIRED);
*/


	CS_entry (CS_THIS_LINE, 4 + offset, BUF_SIZE, buf);

	if (strcmp (buf, "DED") == 0 || strcmp (buf, "LAN") == 0 ||
            strcmp (buf, "WAN") == 0 )
	    links[i]->network = CMT_LAN;
	else if (strcmp (buf, "DIAL") == 0 || strcmp (buf, "PPP") == 0)
	    links[i]->network = CMT_PPP;
	else {
	    LE_send_msg (GL_ERROR | 1008,  
			"network type for link %d not defined", 
			links[i]->link_ind);
	    return (-1);
	}

    } /* end for */

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    Read_snmp_glob_configs (tcp_conf_file);

    for (i = 0; i < n_links; i++) {

	if (links[i]->network == CMT_PPP &&
	    links[i]->server == CMT_SERVER) {
           /* read in Ded or Dialin_control */
           if (Read_dialin_configs (tcp_conf_file,i,links) < 0)
	      return (-1);
	}
	if (links[i]->network == CMT_PPP &&
	   (links[i]->server == CMT_CLIENT ||
	    links[i]->server == CMT_FAACLIENT) ) {
           /* read in Ded or Dialin_control */
           if (Read_dialin_configs (tcp_conf_file,i,links) < 0)
	      return (-1);
        }
	if (links[i]->network == CMT_PPP &&
            links[i]->server == CMT_DYNADIAL ) {
           /* read in Dialout_control */
           if (Read_dialout_configs (tcp_conf_file,i,links) < 0) 
	      return (-1);
	}

    } /* end for */

    return (0);
}
/**************************************************************************

    Description: This function reads in any the global configs.

    Inputs:	tcp_conf_file - name of the tcp config file;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_glob_configs (char *tcp_conf_file) 
{

    int tmp; 


    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    CS_control (CS_KEY_OPTIONAL);


    if (CS_entry ("connection_procedure_time_limit", 1 | CS_INT, 0, (void *)&tmp) > 0) {
       if ( tmp < CONNECTION_PROCEDURE_TIME_LIMIT ) {
          LE_send_msg (GL_ERROR, "Invalid Value: connection_procedure_time_limit must be > %d \n", CONNECTION_PROCEDURE_TIME_LIMIT);
       } else {
          connection_procedure_time_limit = tmp;
          LE_send_msg (LE_VL1, "Connection_procedure_time_limit=%d \n", connection_procedure_time_limit);
       }
    } 

    if (CS_entry ("no_keepalive_response_disconnect_time", 1 | CS_INT, 0, (void *)&tmp) > 0) {
       if ( tmp < MIN_NO_INPUT_TIME ) {
          LE_send_msg (GL_ERROR, "Invalid Value: no_keepalive_response_disconnect_time must be > %d \n", MIN_NO_INPUT_TIME);
       } else {
          no_keepalive_response_disconnect_time = tmp;
          LE_send_msg (LE_VL1, "no_keepalive_response_disconnect_time=%d \n", no_keepalive_response_disconnect_time);
       }
    } 

    CS_control (CS_KEY_REQUIRED);

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    return (0);
}


/**************************************************************************

    Description: This function reads in any the SNMP global configs.

    Inputs:	tcp_conf_file - name of the tcp config file;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_snmp_glob_configs (char *tcp_conf_file) 
{
    char buf[BUF_SIZE];
    int tmp; 

    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    while (1) {

       if (CS_entry (CS_NEXT_LINE, 0, BUF_SIZE, buf) < 0 ) {
          break;
       }

       if (strcmp (buf, "SNMP_configs") != 0  ) {
          continue;
       }

       if (CS_level (CS_DOWN_LEVEL) < 0) {
          break;
       }


       if (CS_entry ("max_index", 1 | CS_INT, 0, (void *)&tmp) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       MAXIF = tmp;

       LE_send_msg (LE_VL1, "New MAXIF=%d \n", MAXIF);
 
       CS_level (CS_UP_LEVEL); 
       break;

    } /* end while */

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    return (0);
}


/**************************************************************************

    Description: This function reads the dial in control configs.

    Inputs:	tcp_conf_file - name of the tcp config file;
                link - the link number
		links - the link structure list;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_dialin_configs (char *tcp_conf_file, 
				int link, Link_struct **links)
{
    char link_num;
    int found = 0;
    char buf[BUF_SIZE];

    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    while (1) {

       if (CS_entry (CS_NEXT_LINE, 0, BUF_SIZE, buf) < 0 ) {
          break;
       }

       if (strcmp (buf, "Dialin_control") != 0 &&
           strcmp (buf, "Ded_control")    != 0 ) {
          continue;
       }

       if (CS_level (CS_DOWN_LEVEL) < 0) {
          break;
       }


       if (CS_entry ("link", 1 | CS_CHAR, 0, (void *)&link_num) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }



       if (link_num != links[link]->link_ind) {
          CS_level (CS_UP_LEVEL);
          continue;
       }


       if (CS_entry ("snmp_type", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_type, buf);

       LE_send_msg (LE_VL3, "snmp_type=%s \n", links[link]->snmp_str.snmp_type);

       /* snmp_host */
       if (CS_entry ("snmp_host", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_host = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_host == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_host, buf);

       LE_send_msg (LE_VL3, "snmp_host=%s \n", links[link]->snmp_str.snmp_host);

       /* snmp_community */
       if (CS_entry ("snmp_community", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_community = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_community == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_community, buf);

       LE_send_msg (LE_VL3, "snmp_community=%s \n", links[link]->snmp_str.snmp_community);

       /* snmp_interface */
       if (CS_entry ("snmp_interface", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_interface = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_interface == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_interface, buf);

       LE_send_msg (LE_VL3, "snmp_interface=%s \n", links[link]->snmp_str.snmp_interface);

       /* enable_command */
       if (CS_entry ("enable_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_enable_cmd=%s \n",
                             links[link]->snmp_str.enable.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_enable_type=%s \n",
                            links[link]->snmp_str.enable.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_enable_value=%s \n",
                             links[link]->snmp_str.enable.oid_value);

       /* disable_command */
       if (CS_entry ("disable_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_disable_cmd=%s \n",
                             links[link]->snmp_str.disable.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_disable_type=%s \n",
                             links[link]->snmp_str.disable.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_disable_value=%s \n", 
                            links[link]->snmp_str.disable.oid_value);

       /* drop_dtr_command */
       if (CS_entry ("drop_dtr_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_cmd=%s \n", 
                            links[link]->snmp_str.drop_dtr.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_type=%s \n", 
                    links[link]->snmp_str.drop_dtr.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_value=%s \n",
                             links[link]->snmp_str.drop_dtr.oid_value);

       found = 1;
       CS_level (CS_UP_LEVEL); 
       break;

    } /* end while */

    if (!found) {
	LE_send_msg (GL_ERROR | 1001,  
	    "Dialin_control section not found for link %d in %s",
             links[link]->link_ind, tcp_conf_file);
	return (-1);
    }

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    return (0);
}

/**************************************************************************

    Description: This function reads the dial out control configs.

    Inputs:	tcp_conf_file - name of the tcp config file;
                link - the link number
		links - the link structure list;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_dialout_configs (char *tcp_conf_file, 
				int link, Link_struct **links)
{
    char link_num;
    int found = 0;
    char buf[BUF_SIZE];

    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    while (1) {

       if (CS_entry (CS_NEXT_LINE, 0, BUF_SIZE, buf) < 0 ) {
          break;
       }

       if (strcmp (buf, "Dialout_control") != 0) {
          continue;
       }

       if (CS_level (CS_DOWN_LEVEL) < 0) {
          break;
       }


       if (CS_entry ("link", 1 | CS_CHAR, 0, (void *)&link_num) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }



       if (link_num != links[link]->link_ind) {
          CS_level (CS_UP_LEVEL);
          continue;
       }


       if (CS_entry ("snmp_type", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_type, buf);

       LE_send_msg (LE_VL3, "snmp_type=%s \n", links[link]->snmp_str.snmp_type);

       /* snmp_host */
       if (CS_entry ("snmp_host", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_host = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_host == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_host, buf);

       LE_send_msg (LE_VL3, "snmp_host=%s \n", links[link]->snmp_str.snmp_host);

       /* snmp_community */
       if (CS_entry ("snmp_community", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_community = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_community == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_community, buf);

       LE_send_msg (LE_VL3, "snmp_community=%s \n", links[link]->snmp_str.snmp_community);

       /* snmp_interface */
       if (CS_entry ("snmp_interface", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.snmp_interface = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.snmp_interface == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.snmp_interface, buf);

       LE_send_msg (LE_VL3, "snmp_interface=%s \n", links[link]->snmp_str.snmp_interface);

       /* enable_command */
       if (CS_entry ("enable_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_enable_cmd=%s \n",
                             links[link]->snmp_str.enable.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_enable_type=%s \n",
                            links[link]->snmp_str.enable.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.enable.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.enable.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.enable.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_enable_value=%s \n",
                             links[link]->snmp_str.enable.oid_value);

       /* disable_command */
       if (CS_entry ("disable_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_disable_cmd=%s \n",
                             links[link]->snmp_str.disable.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_disable_type=%s \n",
                             links[link]->snmp_str.disable.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.disable.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.disable.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.disable.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_disable_value=%s \n", 
                            links[link]->snmp_str.disable.oid_value);

       /* drop_dtr_command */
       if (CS_entry ("drop_dtr_cmd", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_cmd=%s \n", 
                            links[link]->snmp_str.drop_dtr.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_type=%s \n", 
                    links[link]->snmp_str.drop_dtr.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.drop_dtr.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.drop_dtr.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.drop_dtr.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_drop_dtr_value=%s \n",
                             links[link]->snmp_str.drop_dtr.oid_value);

       /* failure_cnt command */
       if (CS_entry ("failure_cnt", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.failure_cnt.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.failure_cnt.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.failure_cnt.oid_cmd, buf);

       LE_send_msg (LE_VL3, "failure_cnt_cmd=%s \n", 
                            links[link]->snmp_str.failure_cnt.oid_cmd);

       /* conn_cnt command */
       if (CS_entry ("connection_cnt", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.conn_cnt.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.conn_cnt.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.conn_cnt.oid_cmd, buf);

       LE_send_msg (LE_VL3, "connection_cnt_cmd=%s \n", 
                            links[link]->snmp_str.conn_cnt.oid_cmd);

       /* disconnect_code command */
       if (CS_entry ("disconnect_code", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.discon_code.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.discon_code.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.discon_code.oid_cmd, buf);

       LE_send_msg (LE_VL3, "disconnect_code_cmd=%s \n", 
                            links[link]->snmp_str.discon_code.oid_cmd);

       /* modem_state command */
       if (CS_entry ("modem_state", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_state.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_state.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_state.oid_cmd, buf);

       LE_send_msg (LE_VL3, "modem_state_cmd=%s \n", 
                            links[link]->snmp_str.modem_state.oid_cmd);

       /* modem_busyout_set_command */
       if (CS_entry ("modem_busyout_set", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_set.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_set.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_set.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_set_cmd=%s \n", 
                            links[link]->snmp_str.modem_bsy_set.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_set.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_set.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_set.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_set_type=%s \n", 
                    links[link]->snmp_str.modem_bsy_set.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_set.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_set.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_set.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_set_value=%s \n",
                             links[link]->snmp_str.modem_bsy_set.oid_value);

       /* modem_busyout_clear_command */
       if (CS_entry ("modem_busyout_clear", 1, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_clr.oid_cmd = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_clr.oid_cmd == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_clr.oid_cmd, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_clr_cmd=%s \n", 
                            links[link]->snmp_str.modem_bsy_clr.oid_cmd);

       if (CS_entry (CS_THIS_LINE, 2, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_clr.oid_type = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_clr.oid_type == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_clr.oid_type, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_clr_type=%s \n", 
                    links[link]->snmp_str.modem_bsy_clr.oid_type);

       if (CS_entry (CS_THIS_LINE, 3, BUF_SIZE - 1, buf) <= 0) {
          CS_level (CS_UP_LEVEL);
          continue;
       }

       links[link]->snmp_str.modem_bsy_clr.oid_value = malloc (strlen (buf) + 1);
       if (links[link]->snmp_str.modem_bsy_clr.oid_value == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	  return (-1);
       }
       strcpy (links[link]->snmp_str.modem_bsy_clr.oid_value, buf);

       LE_send_msg (LE_VL3, "snmp_modem_bsy_clr_value=%s \n",
                             links[link]->snmp_str.modem_bsy_clr.oid_value);

       found = 1;
       CS_level (CS_UP_LEVEL); 
       break;

    } /* end while */

    if (!found) {
	LE_send_msg (GL_ERROR | 1001,  
	    "Dialout_control section not found or properly defined for link %d in %s",
             links[link]->link_ind, tcp_conf_file);
	return (-1);
    }

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

    return (0);

}

static int Read_phone_table (char *tcp_conf_file,
                             void **phone_recs,
                             int *num_recs)
{
    int i;
    int rate;
    Phone_struct **phone_nums;


    CS_cfg_name (tcp_conf_file);
    CS_control ( CS_RESET );
    CS_control (CS_COMMENT | '#');

    if (CS_entry ("Dial_out_specification", 0, 0, NULL) < 0 ||
	CS_level (CS_DOWN_LEVEL) < 0) {
	LE_send_msg (GL_ERROR | 1001,  
	    "Dial_out_specification section not found in %s", tcp_conf_file);
	return (-1);
    }

    if (CS_entry ("number_recs", 1 | CS_INT, 0, (void *)num_recs) <= 0) {
	LE_send_msg (GL_ERROR | 1001,  
	    "number_recs not found inside Dial_out_specification in file %s", tcp_conf_file);
	return (-1);
    }

    if (*num_recs > MAX_N_PHONERECS || *num_recs <= 0) {
	LE_send_msg (GL_ERROR | 1001,  
	    "Max number (%d > %d) of phone records exceeded in %s",
            *num_recs, MAX_N_PHONERECS, tcp_conf_file);
	return (-1);
    }

    /* allocate the memory for the phone array */
    phone_nums = (Phone_struct **)calloc (*num_recs + 1, sizeof(Phone_struct *));
    if (phone_nums == NULL) {
       LE_send_msg (GL_ERROR | 1007,  "malloc failed for Phone_struct array\n");
       return (-1);
    }

    LE_send_msg (LE_VL3, "number_recs = %d \n", *num_recs);

    /* read the phone number records in */
    for (i = 0; i < *num_recs; i++) {
       char buf[BUF_SIZE];
       char server_name [NAME_LEN];
       int  tmp;
       int  offset;

       offset = 0; 

       /* allocate the memory for it */
       phone_nums[i] = (Phone_struct *)malloc (sizeof (Phone_struct));
       if (phone_nums[i] == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed for Phone_struct number %d\n",i);
          return (-1);
       }

       if (CS_entry (CS_NEXT_LINE, 0, BUF_SIZE, buf) <= 0) {
          LE_send_msg (GL_ERROR | 1001,  
                     "phone number %d not defined in %s", i, tcp_conf_file);
	  return (-1);
       }

       phone_nums[i]->phone_num = malloc (strlen (buf) + 1);

       if (phone_nums[i]->phone_num == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return (-1);
       }

       strcpy (phone_nums[i]->phone_num, buf);

       if (CS_entry (CS_THIS_LINE, 1, NAME_LEN, server_name) <= 0) {
          LE_send_msg (GL_ERROR | 1005,  
                    "DDR address/hostname in column 2 line %d not defined", i);
          return (-1);
       }

       phone_nums[i]->dialout_name = malloc (strlen (server_name) + 1);

       if (phone_nums[i]->dialout_name == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return (-1);
       }

       strcpy (phone_nums[i]->dialout_name, server_name);

       CS_entry (CS_THIS_LINE, 2, BUF_SIZE, buf);

       if (strcmp (buf, "client") == 0)
          phone_nums[i]->server = CMT_CLIENT;
       else if (strcmp (buf, "faaclient") == 0)
          phone_nums[i]->server = CMT_FAACLIENT;
       else {
	    LE_send_msg (GL_ERROR | 1003,  
			"client type not allowed or not defined on phone number line %d", i);
	    return (-1);
       }

       if (CS_entry (CS_THIS_LINE, 3, NAME_LEN, server_name) <= 0) {
          LE_send_msg (GL_ERROR | 1005,  
                    "address/hostname for phone number line %d not defined", i);
          return (-1);
       }

       phone_nums[i]->server_name = malloc (strlen (server_name) + 1);

       if (phone_nums[i]->server_name == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return (-1);
       }

       strcpy (phone_nums[i]->server_name, server_name);

       /* are we going to talk to a FAA system? */
       if ( phone_nums[i]->server == CMT_FAACLIENT ) {

           offset++;

           if (CS_entry (CS_THIS_LINE, 4, NAME_LEN, server_name) <= 0) {
	      LE_send_msg (GL_ERROR | 1005,  
                       "second address/hostname for phone number line %d not defined", i);
	      return (-1);
           }
	   phone_nums[i]->ch2_name = malloc (strlen (server_name) + 1);

	   if (phone_nums[i]->ch2_name == NULL) {
	       LE_send_msg (GL_ERROR | 1007,  "malloc failed");
	       return (-1);
           }

 	   strcpy (phone_nums[i]->ch2_name, server_name);

       }

       if (CS_entry (CS_THIS_LINE, (4 + offset) | CS_INT, 0, (char *)&tmp) <= 0) {
          LE_send_msg (GL_ERROR | 1004,  
                       "port number in phone records line %d not defined", i);
          return (-1);
       }
       phone_nums[i]->port_num = tmp;

       if (CS_entry (CS_THIS_LINE, 5 + offset, BUF_SIZE, buf) <= 0) {
          LE_send_msg (GL_ERROR | 1006,  
                    "password for in phone records line %d not defined", i);
          return (-1);
       }

       phone_nums[i]->password = malloc (strlen (buf) + 1);
       if (phone_nums[i]->password == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return (-1);
       }
       strcpy (phone_nums[i]->password, buf);

       phone_nums[i]->line_rate = 0;
       CS_control (CS_KEY_OPTIONAL);
       if (CS_entry (CS_THIS_LINE, (6 + offset)|CS_INT, 0, (char *)&rate) > 0) {
          /* a value was found to override the line_rate. Assign new value  */
          phone_nums[i]->line_rate = rate;
          phone_nums[i]->override_rate = 1;
          LE_send_msg (LE_VL2, "tcp.conf: phone records line %d line_rate = %d ", i, rate);
       }
       CS_control (CS_KEY_REQUIRED);

       LE_send_msg (LE_VL3, "phone=%s type=%d port=%d ch1=%s pwd=%s rate=%d \n",
               phone_nums[i]->phone_num,
               phone_nums[i]->server,
               phone_nums[i]->port_num,
               phone_nums[i]->server_name,
               phone_nums[i]->password,
               phone_nums[i]->line_rate );

    } /* end for */

    *phone_recs = phone_nums;    


    /* return back to the TCP_link_specificiation */
    CS_level (CS_UP_LEVEL);
    if (CS_entry ("TCP_link_specification", 0, 0, NULL) < 0 ||
        CS_level (CS_DOWN_LEVEL) < 0) {
        LE_send_msg (GL_ERROR | 1001,
            "TCP_link_specification section not found in %s", tcp_conf_file);
        return (-1);
    }


    return (0);

} 
