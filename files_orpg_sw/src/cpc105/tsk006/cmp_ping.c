/****************************************************************
		
    File: 	cmp_ping.c	
				
    Description: This module contains routines that check whether
		the INTERNET connections are alive by sending ICMP 
		packets to the remote hosts and receiving the echo. 
		This is adapted from cl_register.c of the rmt module.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/01/29 23:01:02 $
 * $Id: cmp_ping.c,v 1.16 2004/01/29 23:01:02 jing Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <stdlib.h>
 
#include <infr.h>
#include <orpg.h>
#include <cmp_def.h>

#define STATUS_BUF_SIZE 512
#define MAX_N_NODES 128		/* max number of nodes to monitor */

static int Test_time;		/* time period (in seconds) this module sends
				   an ICMP packet */
static int Mping_not_available = 0;
				/* mping is not available */

/* local functions */
static char *Convert_addr_to_string (int addr);
static void Parse_connect_status (char *status, int n_rhosts, 
					Remote_host *rhosts);


/*****************************************************************************

    Description: This function initializes this module.

    Inputs:	keepalive_time - time period (in seconds) a child will
			survive after a connection problem is detected.

    Return:	0 on success or -1 on failure.

*****************************************************************************/

int CMP_initialize (int test_time) {

    Test_time = test_time;
    return (0);
}

/*****************************************************************************

    Description: This function is called frequently by the application.

    Inputs:	n_rhosts - number of remote hosts;
		rhosts - list of remote hosts;

*****************************************************************************/

void CMP_check_connect (int n_rhosts, Remote_host *rhosts) {
    static int mping_started = 0;
    static int n_mp_addrs = 0;
    static int mp_addrs[MAX_N_NODES];
    static void *cp = NULL;
    int i, ret;
    char status[STATUS_BUF_SIZE];

    if (Mping_not_available)
	return;

    if (!mping_started && cp != NULL) {
	MISC_cp_close (cp);
	n_mp_addrs = 0;
    }

    if (!mping_started) {		/* restart mping */
	if ((ret = MISC_cp_open ("mping", 0, &cp)) < 0) {
	    LE_send_msg (GL_ERROR, "MISC_cp_open mping failed (%d)", ret);
	    if (ORPGMISC_is_operational ())
		exit (1);
	    else {
		Mping_not_available = 1;
		return;
	    }
	}
	mping_started = 1;
    }

    for (i = 0; i < n_rhosts; i++) {	/* check if new destinations need to 
					   be added to mping */
	int k;
	char *addr, buf[128];

	for (k = 0; k < n_mp_addrs; k++) {
	    if (rhosts[i].addr == mp_addrs[k])
		break;
	}
	if (k < n_mp_addrs)		/* already in mping */
	    break;
	addr = Convert_addr_to_string (rhosts[i].addr);
	if (n_mp_addrs >= MAX_N_NODES) {
	    LE_send_msg (GL_ERROR, 
		"Too many destinations to monitor - (%s) discarded", addr);
	    break;
	}
	sprintf (buf, "++%s\n", addr);
	LE_send_msg (0, "Address %s sent to mping\n", addr);
	if ((ret = MISC_cp_write_to_cp (cp, buf)) < 0) {
	    LE_send_msg (GL_ERROR, "MISC_cp_write_to_cp failed (%d)", ret);
	    mping_started = 0;
	    break;
	}
	mp_addrs[n_mp_addrs] = rhosts[i].addr;
	n_mp_addrs++;
    }

    /* read mping output */
    while ((ret = MISC_cp_read_from_cp (cp, status, STATUS_BUF_SIZE)) > 0) {
	if (ret == MISC_CP_STDERR) {
	    int len = strlen (status);
	    if (status[len - 1] == '\n')
		status[len - 1] = '\0';
	    LE_send_msg (GL_ERROR, "mping: (%s)", status);
	}
	else
	    Parse_connect_status (status, n_rhosts, rhosts);
    }
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "mping died - we will restart it");
	mping_started = 0;
    }
}

/*****************************************************************

    Parses connectivity status "status" from mping.

*****************************************************************/

static void Parse_connect_status (char *status, int n_rhosts, 
					Remote_host *rhosts) {
    int i, qtime;
    char *t, *p;

    t = strtok (status, " \t\n");
    while (t != NULL) {
	p = strstr (t, "--");
	if (p == NULL ||
	    sscanf (p + 2, "%d", &qtime) != 1) {
	    LE_send_msg (GL_INFO, "Unexpected mping token (%s)", t);
	}
	else {
	    *p = '\0';
	    for (i = 0; i < n_rhosts; i++) {
		if (strcmp (Convert_addr_to_string (rhosts[i].addr), t) == 0) {
		    rhosts[i].qtime = qtime;
		    break;
		}
	    }
	}
	t = strtok (NULL, " \t\n");
    }
}

/*****************************************************************

    Converts IP address "addr" to ASCII string form.

*****************************************************************/

static char *Convert_addr_to_string (int addri) {
    static char buf[64];
    unsigned int addr;

    addr = (unsigned int)addri;
    sprintf (buf, "%d.%d.%d.%d", addr >> 24, (addr >> 16) & 0xff, 
					(addr >> 8) & 0xff, addr & 0xff);
    return (buf);
}


