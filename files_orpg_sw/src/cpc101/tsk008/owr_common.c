/****************************************************************
		
    This module contains the functions that are shared between the
    client and the server for one-way-replicator (owr).

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/05 17:45:47 $
 * $Id: owr_common.c,v 1.2 2011/06/05 17:45:47 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* System include files */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <infr.h>
#include "owr_def.h"

static char *Cmd = NULL;

/***********************************************************************

    Returns the port number defined in RMTPORT. Returns 0 if not defined.

***********************************************************************/

int OC_get_env_port () {
    char *env, *p;
    int port = 40000;
    env = getenv ("RMTPORT");
    if (env != NULL) {
	if ((p = strstr (env, ":")) != NULL)
	    env = p + 1;
	if (sscanf (env, "%d", &port) == 1)
	    port += 3;
	else
	    port = 40000;
    }
    return (port);
}

/***********************************************************************

    Saves the comman line options so this program can be restarted.

***********************************************************************/

void OC_prepare_for_restart (int argc, char **argv) {
    int i;

    Cmd = STR_copy (Cmd, "");
    for (i = 0; i < argc; i++) {
	Cmd = STR_cat (Cmd, argv[i]);
	Cmd = STR_cat (Cmd, " ");
    }
    Cmd = STR_cat (Cmd, "&");
}

/***********************************************************************

    Restarts this program.

***********************************************************************/

void OC_restart () {
    time_t cr_t, st_t, p_t;
    int pid;

    LB_close (LE_fd ());			/* to allow a new version */
    cr_t = MISC_systime (NULL);
    st_t = p_t = cr_t;
    pid = -1;
    while (1) {

	if (pid >= 0 && kill (pid, 0) < 0)	/* process terminated */
	    pid = -1;
	if (cr_t > p_t + 5)			/* success */
	    break;
	if (pid < 0 && cr_t > st_t + 15)	/* failed */
	    break;
	if (pid < 0) {
	    pid = MISC_system_to_buffer (Cmd, NULL, 0, NULL);
	    if (pid >= 0)
		p_t = cr_t;
	}	
	while (1) {
	    time_t t = MISC_systime (NULL);
	    if (t == cr_t)
		msleep (200);
	    else {
		cr_t = t;
		break;
	    }
	}
    }
    exit (0);
}

static int Cp_s = 0;
static char *Cp_buf = NULL;

/************************************************************************

    Compress "pload" of "pl_size" bytes and place it in a buffer at offset
    "hd_len". The buffer treated as Message_t and fields "cmpr_method" and
    "size" are set. "msp" returns the size of the message. Returns the 
    message buffer.

************************************************************************/

char *OC_compress_payload (int hd_len, char *pload, int pl_size, int *msp) {
    int clen, ms;
    Message_t *mh;

    if (hd_len + pl_size > Cp_s) {
	if (Cp_buf != NULL)
	    free (Cp_buf);
	Cp_s = hd_len + pl_size;
	Cp_buf = MISC_malloc (Cp_s);
    }

    if (pl_size > 512)
	clen = MISC_compress (MISC_GZIP, pload, pl_size, 
					Cp_buf + hd_len, pl_size);
    else
	clen = 0;
    memset (Cp_buf, 0, sizeof (Message_t));
    mh = (Message_t *)Cp_buf;
    if (clen <= 0) {
	mh->cmpr_method = -1;
	ms = hd_len + pl_size;
	memcpy (Cp_buf + hd_len, pload, pl_size);
    }
    else {
	mh->cmpr_method = MISC_GZIP;
	ms = clen + hd_len;
    }
    mh->size = hd_len + pl_size;

    *msp = ms;
    return (Cp_buf);
}

/************************************************************************

    Decompress payload in "msg" of "msg_len" bytes. The header size is
    "hd_size". "ds" returns the size of the decompressed data. Returns the 
    pointer to the decompressed data.

************************************************************************/

char *OC_decompress_payload (Message_t *msg, 
				int hd_size, int msg_len, int *ds) {

    if (msg_len > hd_size && msg->cmpr_method >= 0) {
	int dlen;
	int s = msg->size - hd_size;
	if (Cp_s < s) {
	    if (Cp_buf != NULL)
		free (Cp_buf);
	    Cp_s = s;
	    Cp_buf = MISC_malloc (Cp_s);
	}
	dlen = MISC_decompress (msg->cmpr_method, 
		(char *)msg + hd_size, msg_len - hd_size, Cp_buf, Cp_s);
	if (dlen <= 0) {
	    MISC_log ("MISC_decompress failed (%d)\n", dlen);
	    return (NULL);
	}
	*ds = dlen;
	return (Cp_buf);
    }
    *ds = msg_len - hd_size;
    return ((char *)msg + hd_size);
}

/************************************************************************

    Frees "buf" if it is too large. Otherwise the buffer is reused.

************************************************************************/

void OS_free_buffer () {

    if (Cp_s > LARGE_BUF_SIZE) {
	free (Cp_buf);
	Cp_buf = NULL;
	Cp_s = 0;
    }
}




