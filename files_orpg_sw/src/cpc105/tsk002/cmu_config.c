
/******************************************************************

	file: cms_config.c

	This module reads the link config file for the 
	UCONX/MPS comm manager.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2006/05/24 16:52:24 $
 * $Id: cmu_config.c,v 1.26 2006/05/24 16:52:24 jing Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef CMU_GENERIC
#define DEDICATED_DEFINED_ELSEWHERE
#include "cmu_def.h"
#include <orpg.h>
#else
#include "cmu_def.h"
#endif
#include <xstypes.h>
#include <dlpiabm.h>
#include <mpsproto.h>

#define COMMAND_BUFFER_SIZE	256
#define PC_MS_DEVICE		1
#define PC_MSP_DEVICE		2 
#define PC_DEVICE_NO_RESPONSE	3
#define PC_MS_VERSION_ID	"masterSwitchV2"
#define PC_MSP_VERSION_ID	"masterSwitchMSP"
#define PC_NO_RESPONSE_ID	"Timeout: No Response"

static char *Conf_name;		/* name of the link config file */

static char X25_server[MAXSERVERLEN] = "";
static char X25_service[] = "mps";

static char Uconx_conf_file[NAME_LEN];
static char Reboot_command_ms[COMMAND_BUFFER_SIZE];
static char Reboot_command_msp[COMMAND_BUFFER_SIZE];

/* local functions */
static int Send_err_and_ret (char *text);
static int Read_uconx_config_file (char *conf_file, 
				int n_links, Link_struct **links);
static int Read_HDLC_ABM_conf (Link_struct *link);
static int Read_X25_PVC_conf (Link_struct *link, char *conf_dir);
#ifndef CMU_GENERIC
static void Read_comms_link_conf (int n_links, Link_struct **links);
#endif
static int Pc_get_device_version ();


/**************************************************************************

    Description: returns the X25 comms server name.

**************************************************************************/

char *CC_get_server_name () {
    return (X25_server);
}

/**************************************************************************

    Description: returns the X25 service name.

**************************************************************************/

char *CC_get_X25_service_name () {
    return (X25_service);
}

/**************************************************************************

    Description: returns Reboot_command.

**************************************************************************/

char *CC_get_reboot_command () {
    static int pc_version = -1;

    if (pc_version < 0) {
	CS_cfg_name (Uconx_conf_file);
	pc_version = Pc_get_device_version ();
	CS_cfg_name ("");
	if (pc_version < 0)
	    return (NULL);
    }
    if (pc_version == PC_MS_DEVICE && Reboot_command_ms[0] != '\0')
	return (Reboot_command_ms);
    if (pc_version == PC_MSP_DEVICE && Reboot_command_msp[0] != '\0')
	return (Reboot_command_msp);
    LE_send_msg (GL_ERROR, "SNMP rebooting command not configured");
    return (NULL);
}

/**************************************************************************

    Description: This function reads the link configuration file.

    Inputs:	links - the link structure list;

    Output:	device_number - device number;
		cmsv_addr - comms server internet address;

    Return:	It returns the number of links on success or -1 on failure.

**************************************************************************/

#define BUF_SIZE 256
#define ADDR_SIZE 32

int CC_read_link_config (Link_struct **links, 
				int *device_number, int *cmsv_addr)
{
    int n_links;		/* number of links managed by this process */
    int device;
    int addr;

    Conf_name = CMC_get_link_conf_name ();
    CS_cfg_name (Conf_name);

    n_links = 0;
    device = -1;
    while (1) {
	Link_struct *link;

	link = CMC_read_link_config ("cm_uconx");
	if (link == CMC_CONF_READ_DONE)
	    break;
	else if (link == CMC_CONF_READ_FAILED)
	    return (-1);

	/* UCONX comm_manager only processes one device */
	if (device < 0)
	    device = link->device;
	else if (device != link->device) {
	    LE_send_msg (GL_ERROR, 
		"comm_manager_uconx only processes single device\n");
	    return (-1);
	}

	/* store in the link config table */
	links[n_links] = link;
	n_links++;
    }

    *device_number = device;

    if (n_links > 0) {
	if (CS_entry ("UCONX_config_file", 1, NAME_LEN, Uconx_conf_file) < 0)
	    return (-1);
	if (strlen (Uconx_conf_file) <= 1) {
 	    LE_send_msg (GL_ERROR, "bad UCONX_config_file (%s) in %s\n", 
					Uconx_conf_file, Conf_name);
	    return (-1);
	}
    }

    /* close this config file and return to the generic system config text */
    CS_cfg_name ("");

#ifndef CMU_GENERIC
    /* read binary comms link conf to get the up-to-date packet size */
    Read_comms_link_conf (n_links, links);
#endif

    /* read the UCONX config file */
    if (n_links > 0 &&
	Read_uconx_config_file (Uconx_conf_file, n_links, links) < 0)
	return (-1);

    addr = NET_get_ip_by_name (CC_get_server_name ());

    if (addr == INADDR_NONE) {
 	LE_send_msg (GL_ERROR, 
		"bad comms server address (%s)\n", CC_get_server_name ());
	return (-1);
    }
    addr = ntohl (addr);

    if (CMMON_register_address (addr) != 0)
	return (-1);
    *cmsv_addr = addr;

    return (n_links);
}

/**************************************************************************

    Reads the binary comms link configure info to get the updated packet
    size. This function will need ORPG specific MACROs. If one does not
    have them, CMU_GENERIC must be set.

    Inputs/Outputs:	n_links - number of links managed;
		links - the link structure list;

**************************************************************************/

#ifndef CMU_GENERIC

static void Read_comms_link_conf (int n_links, Link_struct **links) {
    Pd_distri_info *p_tbl;
    Pd_line_entry *l_tbl;
    int len, i, ret, fd;
    char name[NAME_SIZE];

    ret = CS_entry ((char *)ORPGDAT_PROD_INFO, CS_INT_KEY | ORPGSC_LBNAME_COL, 
							NAME_SIZE, name);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, "CS_entry failed (key %d, ret %d)\n", 
						ORPGDAT_PROD_INFO, ret);
	return;
    }

    fd = LB_open (name, LB_READ, NULL);
    if (fd < 0) {
	LE_send_msg (GL_ERROR, "LB_open %s failed (ret %d)\n", name, fd);
	return;
    }

    len = LB_read (fd, (char **)&p_tbl, LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID);
    if (len < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list +
		p_tbl->n_lines * (int)sizeof (Pd_line_entry) > len) {
	LE_send_msg (GL_ERROR,
		"Read PD_LINE_INFO_MSG_ID message failed (ret %d)", len);
	if (len > 0)
	    free (p_tbl);
	LB_close (fd);
	return;
    }
    l_tbl = (Pd_line_entry *)((char *)p_tbl + p_tbl->line_list);

    for (i = 0; i < p_tbl->n_lines; i++) {	/* update packet size */
	int lnind, k;

	lnind = l_tbl[i].line_ind;
	for (k = 0; k < n_links; k++) {
	    if (links[k]->link_ind == lnind) {
		links[k]->packet_size = l_tbl[i].packet_size;
		links[k]->link_type = l_tbl[i].line_type;
		links[k]->line_rate = l_tbl[i].baud_rate;
		break;
	    }
	}
    }
    free (p_tbl);
    LB_close (fd);
}
#endif

/**************************************************************************

    Description: This function reads the uconx configuration file.

    Inputs:	conf_file - name of the uconx config file;
		n_links - number of links managed;
		links - the link structure list;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

#define KEY_LEN  64

static int Read_uconx_config_file (char *conf_file, 
				int n_links, Link_struct **links)
{
    int len, i;
    char conf_dir[NAME_LEN], buf[COMMAND_BUFFER_SIZE];
    char temp_tag[MAXSERVERLEN + 4];

    CS_cfg_name (conf_file);
    Conf_name = conf_file;

    conf_dir[0] = '\0';
    for (i = 0; i < n_links; i++) {
	int ret;
	if (links[i]->proto == PROTO_PVC)
	    ret = Read_X25_PVC_conf (links[i], conf_dir);
	else
	    ret = Read_HDLC_ABM_conf (links[i]);
	if (ret < 0)
	    return (-1);
    }

    /* Test version of power control device. */
    Pc_get_device_version ();

    /* Read reboot command */
    CS_control (CS_KEY_OPTIONAL);
    sprintf (temp_tag, "%s_ms", X25_server);
    Reboot_command_ms[0] = '\0';
    len = CS_entry (temp_tag, CS_FULL_LINE, COMMAND_BUFFER_SIZE - 1, buf);

    if (len > (int)strlen (temp_tag)) {
	char *cpt = buf + strlen (temp_tag);
	strcpy (Reboot_command_ms, cpt + MISC_char_cnt (cpt, " \t"));
    }
    sprintf (temp_tag, "%s_msp", X25_server);
    Reboot_command_msp[0] = '\0';
    len = CS_entry (temp_tag, CS_FULL_LINE, COMMAND_BUFFER_SIZE - 1, buf);
    if (len > (int)strlen (temp_tag)) {
	char *cpt = buf + strlen (temp_tag);
	strcpy (Reboot_command_msp, cpt + MISC_char_cnt (cpt, " \t"));
    }

    CS_control (CS_KEY_REQUIRED);

    /* close this config file and return to the generic system config text */

    CS_cfg_name ("");
    return (0);
}

/**************************************************************************

    Determines version of power control device (power administrator).

    Returns macro of device version or -1 on failure.

***************************************************************************/

#define MAX_OUTPUT_TEXT_SIZE 2000
#define PC_CMD_LEN 128

static int Pc_get_device_version () {
    int ret;
    char cmd[PC_CMD_LEN];
    char pc_type[MAX_OUTPUT_TEXT_SIZE]; /* output from SNMP command */

    if (CS_entry ("Pv_cmd", 1, PC_CMD_LEN, (void *)cmd) > 0) {
	pc_type[0] = '\0';
	ret = MISC_system_to_buffer (cmd, pc_type, MAX_OUTPUT_TEXT_SIZE, NULL);
	if (ret == 0) {
	    if (strstr (pc_type, PC_MS_VERSION_ID) != NULL)
		return PC_MS_DEVICE;
	    else if (strstr (pc_type, PC_MSP_VERSION_ID) != NULL)
		return PC_MSP_DEVICE;
	    else if (strstr (pc_type, PC_NO_RESPONSE_ID) != NULL)
		LE_send_msg (GL_INFO, "Getting PC type - SNMP no response");
	    else 
		LE_send_msg (GL_INFO, 
			"Getting PC type - Unexpected SNMP response");
	}
	else 
	    LE_send_msg (GL_INFO, 
			"Failed (%d) in executing \"%s\"\n", ret, cmd);
    }
    else
	LE_send_msg (GL_INFO, "Pv_cmd not found in UCONX config file");
    LE_send_msg (GL_ERROR, "Failed in SNMP quering PC device type");
    return (-1);
}

/**************************************************************************

    Reads configuration info for X25/PVC line "link".

    Returns 0 on success or -1 on failure.

***************************************************************************/

#define MAX_PSF	32	/* maximum ASCII string size of packet size field */

static int Read_X25_PVC_conf (Link_struct *link, char *conf_dir) {
    char key [KEY_LEN];
    char snid[4], server[MAXSERVNAMELEN], wan[NAME_LEN];
    char lapb[NAME_LEN], x25[NAME_LEN];

    if (conf_dir[0] == '\0') {
	if (CS_entry ("Conf_file_dir", 1, NAME_LEN - 1, conf_dir) <= 0) {
	    LE_send_msg (GL_ERROR, 
		    "Conf_file_dir not found in file %s\n", Conf_name);
	    return (-1);
	}
	if (conf_dir[0] != '/') {
	    LE_send_msg (GL_ERROR, 
		    "Conf_file_dir not a full path in %s\n", Conf_name);
	    return (-1);
	}
	if (conf_dir[strlen (conf_dir) - 1] != '/')
	    strcat (conf_dir, "/");
    }

    sprintf (key, "%d", link->link_ind);
    if (CS_entry (key, 1, 4, snid) <= 0 ||
	CS_entry (key, 2, MAXSERVNAMELEN, server) <= 0 ||
	CS_entry (key, 3, NAME_LEN, wan) <= 0 ||
	CS_entry (key, 4, NAME_LEN, lapb) <= 0 ||
	CS_entry (key, 5, NAME_LEN, x25) <= 0) {
 	LE_send_msg (GL_ERROR, 
		"X25/PVC link %d configuration error in file %s\n", 
					link->link_ind, Conf_name);
	return (-1);
    }
    if (X25_server[0] == '\0')
	strcpy (X25_server, server);
    else if (strcmp (X25_server, server) != 0) {
 	LE_send_msg (GL_ERROR, 
		"X25/PVC link %d server name different in file %s\n", 
					link->link_ind, Conf_name);
	return (-1);
    }

    link->wan_conf = malloc (3 * (strlen (conf_dir) + 1 + MAX_PSF) + 
				strlen (wan) + strlen (lapb) + strlen (x25));
    if (link->wan_conf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }
    strcpy (link->snid, snid);
    sprintf (link->wan_conf, "%s%s.%d", conf_dir, wan, link->packet_size);
    link->lapb_conf = link->wan_conf + strlen (link->wan_conf) + 1;
    sprintf (link->lapb_conf, "%s%s.%d", conf_dir, lapb, link->packet_size);
    link->x25_conf = link->lapb_conf + strlen (link->lapb_conf) + 1;
    sprintf (link->x25_conf, "%s%s.%d", conf_dir, x25, link->packet_size);

    return (0);
}

/**************************************************************************

    Reads configuration info for HDLC/ABM line "link".

    Returns 0 on success or -1 on failure.

***************************************************************************/

static int Read_HDLC_ABM_conf (Link_struct *link) {
    OpenRequest oreq;		/* MPSopen open request data struct */
    char buf [BUF_SIZE];
    char key [KEY_LEN];
    static int n_baud_rates = 18;
    static int baud_rate[] = {400, 600, 1200, 2400, 4800, 9600, 
	19200, 38400, 64000, 128000, 256000, 512000, 768000, 
	1000000, 1540000, 2000000, 3000000, 4000000};
    dl_bind_req_t       *p_bind;
    int baud;
    int t1, t3, n2, k, rd, um, ra, ru, ms, hdlc_op;
    int rej, sre, ui, ead, mod, ladd, radd;

    /* initialize the OpenRequest data structure */
    memset (&oreq, 0, sizeof (oreq));
    if (CS_entry ("Server_name", 1, MAXSERVERLEN, oreq.serverName) < 0 ||
	CS_entry ("Service_name", 1, MAXSERVNAMELEN, oreq.serviceName) < 0) {
 	LE_send_msg (GL_ERROR, 
		"HDLC/ABM server not found in %s\n", Conf_name);
	return (-1);
    }
    strcpy ( oreq.protoName, "abm" );
    oreq.ctlrNumber = 0;		/* embedded board 0 */

    /* allocate the space */
    link->oreq = (OpenRequest *)malloc (sizeof (oreq));
    link->p_bind = (dl_bind_req_t *)malloc (sizeof (dl_bind_req_t));
    if (link->oreq == NULL || link->p_bind == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }
    memset (link->p_bind, 0, sizeof (dl_bind_req_t));
    memcpy ((char *)link->oreq, (char *)&oreq, sizeof (OpenRequest));
    strcpy ((char *)((OpenRequest *)(link->oreq))->protoName, "abm");

    /* read in the HDLC config */
    p_bind = link->p_bind;
    p_bind->dl_primitive  = htonl (DL_BIND_REQ);
    p_bind->dl_sap        = htonl (link->port);
    p_bind->dl_service    = htonl (DL_DATALINK);
    p_bind->dl_max_frame  = htonl (link->packet_size);
    if (link->line_rate > 0) {
	for (baud = 0; baud < n_baud_rates; baud++)
	    if (baud_rate[baud] >= link->line_rate)
		break;
	p_bind->dl_baud_rate  = htonl (baud);
    }
    else
	p_bind->dl_baud_rate = htonl (-1);

    /* DTE or DCE */
    sprintf (key, "HDLC_%d", link->port);
    if (CS_entry (key, 1, BUF_SIZE, buf) < 0)
	return (-1);
    if (strcmp (buf, "DTE") == 0)
	p_bind->dl_addr_type = htonl (DL_DTE);
    else if (strcmp (buf, "DCE") == 0)
	p_bind->dl_addr_type = htonl (DL_DCE);
    else {
	LE_send_msg (GL_ERROR, "neither DTE nor DCE is specified\n");
	return (Send_err_and_ret (buf));
    }

    /* other HDLC/ABM config for the port */
    if (CS_entry (key, CS_FULL_LINE, BUF_SIZE, buf) < 0)
	return (-1);
    if (sscanf (buf, 
	"%*s %*s %d %d %d %d %d %d %d %d %d %d",
	&t1, &t3, &n2, &k, &rd, &um, &ra, &ru, &ms, &hdlc_op) != 10) 
	return (Send_err_and_ret (buf));
    if (t1 < 0 || t3 < 0 || n2 < 0 || k < 0) {
	LE_send_msg (GL_ERROR, "unexpected number specified\n");
	return (Send_err_and_ret (buf));
    }
    p_bind->dl_t1 = htonl (t1);
    p_bind->dl_t3 = htonl (t3);
    p_bind->dl_n2 = htonl (n2);
    p_bind->dl_k = htonl (k);
    p_bind->dl_rr_delay = htonl (rd);
    p_bind->dl_unack_max = htonl (um);
    p_bind->dl_rualive = htonl (ra);
    p_bind->dl_Reuters = htonl (ru);
    p_bind->dl_modem_sigs = htonl (ms);

    /* options */
    sprintf (key, "HDLC_option_%d", hdlc_op);
    if (CS_entry (key, CS_FULL_LINE, BUF_SIZE, buf) < 0)
	return (-1);
    if (sscanf (buf, "%*s %d %d %d %d %d %d %d",
	&rej, &sre, &ui, &ead, &mod, &ladd, &radd) != 7) 
	return (Send_err_and_ret (buf));
    p_bind->dl_options_list.dl_add_rej = htonl (rej);
    p_bind->dl_options_list.dl_add_srej = htonl (sre);
    p_bind->dl_options_list.dl_add_ui = htonl (ui);
    p_bind->dl_options_list.dl_add_ext_addr = htonl (ead);
    p_bind->dl_options_list.dl_add_mod128 = htonl (mod);
    p_bind->dl_options_list.dl_addr0_local = htonl (ladd / 1000000);
    p_bind->dl_options_list.dl_ext_addr1_local = 
				htonl ((ladd / 10000) % 100);
    p_bind->dl_options_list.dl_ext_addr2_local = 
				htonl ((ladd / 100) % 100);
    p_bind->dl_options_list.dl_ext_addr3_local = htonl ((ladd) % 100);
    p_bind->dl_options_list.dl_addr0_remote = htonl (radd / 1000000);
    p_bind->dl_options_list.dl_ext_addr1_remote = 
				htonl ((radd / 10000) % 100);
    p_bind->dl_options_list.dl_ext_addr2_remote = 
				htonl ((radd / 100) % 100);
    p_bind->dl_options_list.dl_ext_addr3_remote = htonl ((radd) % 100);

    return (0);
}

/**************************************************************************

    Description: This function sends an error message.

    Input:	text - text in which the error is found.

    Return:	It returns -1.

**************************************************************************/

static int Send_err_and_ret (char *text)
{

    LE_send_msg (GL_ERROR, "error found in \"%s\" (file %s)\n",
				text, Conf_name);
    return (-1);
}

/**************************************************************************

    Returns the Uconx configuration file name.

**************************************************************************/

char *CC_uconx_conf_file () {
    return (Uconx_conf_file);
}

