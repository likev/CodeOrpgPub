
/****************************************************************
		
    This module implements various client-side functions for the 
    one-way-replicator (owr).

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/13 20:12:15 $
 * $Id: owr_cl_funcs.c,v 1.5 2011/06/13 20:12:15 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* System include files */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <orpg.h>
#include <infr.h>
#include "owr_def.h"

#define ST_REPORT_PERIOD 120
#define FD_NOT_INIT -1
#define FD_FAILED -2

typedef struct {
    int did;
    int fd;
    int rep_type;
    int in_bytes;
    int in_cnt;
    int out_bytes;
    int out_cnt;
    char *path;
} Rep_lb_t;

static int N_lbs = 0;
static Rep_lb_t *Lbs = NULL;
static int N_Ans = 0;
static unsigned int *Ans = NULL;
extern int Verbose;
int All_lb_created = 0;
static Not_rep_msg_t *Nreps = NULL;
static int N_nreps = 0;

static void Lb_callback (int fd, LB_id_t msg_id, int msg_info, void *arg);
static int Send_lb_update_msg (int id, int fd, LB_id_t msg_id, int msg_len);
static void An_callback (EN_id_t event, char *msg, int len, void *arg);
static void Check_all_processed (int copy_bytes);

/******************************************************************

    Pre-register all events in Ans.

******************************************************************/

int CF_prereg_ans () {
    int max, cnt, i;

    max = 10;
    cnt = 0;
    for (i = 0; i < N_Ans; i += max) {
	int n, ret;
	n = max;
	if (i + n > N_Ans)
	    n = N_Ans - i;
	ret = EN_multi_register (Ans + i, n, An_callback);
	if (ret != n)
	    MISC_log ("Failed in EN_multi_register (%d, %d, %d)\n", i, n, ret);
	else
	    cnt += n;
    }
    if (cnt == N_Ans)
	MISC_log ("All %d pre-reg events completed\n", cnt);
    return (N_Ans);
}

static void An_callback (EN_id_t event, char *msg, int len, void *arg) {
}

/******************************************************************

    Processes a LB_UPDATE message from the server.

******************************************************************/

int CF_lb_update (int len, Message_t *msg) {
    int i, ret, ds;
    LB_id_t msg_id;
    Rep_lb_t *lb;
    char *data;

    for (i = 0; i < N_lbs; i++) {
	if (Lbs[i].did == msg->id)
	    break;
    }
    if (i >= N_lbs) {
 	MISC_log ("Bad LB_UPDATE - did (%d) not found\n", msg->id);
	return (-1);
    }

    lb = Lbs + i;
    if (lb->rep_type == REP_LB_ANY)
	msg_id = LB_ANY;
    else
	msg_id = msg->msg_id;
    data = OC_decompress_payload (msg, sizeof (Message_t), len, &ds);
    if (msg->cl_upd)
	LB_misc (lb->fd, LB_CHECK_AND_WRITE);
    ret = LB_write (lb->fd, data, ds, msg_id);
    OS_free_buffer ();
    if (ret < 0) {
 	MISC_log ("LB_write %s failed (%d)\n", lb->path, ret);
	return (-1);
    }
    lb->in_bytes += len;
    lb->in_cnt++;
    if (Verbose) {
	if (ret > 0)
	    MISC_log ("%s (msg %d) updated  %d fd %d\n", lb->path, 
			msg_id, len - sizeof (Message_t), lb->fd);
	else
	    MISC_log ("%s (msg %d) update not needed\n", 
			lb->path, msg->msg_id);
    }
    return (0);
}

/******************************************************************

    Processes a RESP_LB_REP from the server.

******************************************************************/

int CF_process_resp_lb_rep (int len, Message_t *msg) {
    static int copy_bytes = 0;	/* # of bytes copied from server initially */
    char *path, *data, *lpath, nbuf[MAX_STR_SIZE];
    int dlen, i, fd, ret;
    Rep_lb_t *lb;
    LB_attr *attr;

    for (i = 0; i < N_lbs; i++) {
	if (Lbs[i].did == msg->id)
	    break;
    }
    if (i >= N_lbs) {
 	MISC_log ("Bad RESP_LB_REP - did (%d) not found\n", msg->id);
	return (-1);
    }
    lb = Lbs + i;
    if (lb->fd != FD_NOT_INIT) {
 	MISC_log ("Bad RESP_LB_REP - did (%d) already processed\n", msg->id);
	return (-1);
    }

    if (len == sizeof (Message_t)) {
	MISC_log ("RESP_LB_REP - Req %d failed (code %d)\n", 
					msg->id, msg->code);
	lb->fd = FD_FAILED;
	Check_all_processed (copy_bytes);
	return (-1);
    }

    attr = (LB_attr *)((char *)msg + sizeof (Message_t));
    path = (char *)attr + sizeof (LB_attr);
    if (len < strlen (path) + 1 + sizeof (Message_t) + sizeof (LB_attr)) {
	MISC_log ("Bad RESP_LB_REP - LB path not found (did %d)\n", msg->id);
	return (-1);
    }

    data = OC_decompress_payload (msg, 
		(path + strlen (path) + 1) - (char *)msg, len, &dlen);

    lpath = CL_get_local_path (path);
    ret = MISC_mkdir (MISC_dirname (lpath, nbuf, MAX_STR_SIZE));
    if (ret < 0) {
	MISC_log ("MISC_mkdir (%s) failed (%d)\n", lpath, ret);
	return (-1);
    }

    if (dlen == 0) {		/* Message queue or CCOCM */
	fd = LB_open (lpath, LB_WRITE | LB_CREATE, attr);
	if (fd < 0) {
	    MISC_log ("LB_open (creating) %s failed (%d)\n", lpath, fd);
	    return (-1);
	}
	lb->rep_type |= REP_LB_ANY;
    }
    else {				/* DB LB */
	int f;
	unlink (lpath);
	f = open (lpath, O_RDWR | O_CREAT, 0770);
	if (f < 0) {
	    MISC_log ("open (creating) file %s failed\n", lpath);
	    return (-1);
	}
	if (write (f, data, dlen) != dlen) {
	    MISC_log ("write LB file %s (%d bytes) failed\n", lpath, dlen);
	    close (f);
	    return (-1);
	}
	close (f);
	fd = LB_open (lpath, LB_WRITE, attr);
	if (fd < 0) {
	    MISC_log ("LB_open (DB) %s failed (%d)\n", lpath, fd);
	    return (-1);
	}
    }
    OS_free_buffer ();

    if (lb->rep_type & (REP_TO_CLIENT | REP_TO_NONE))
	ret = 0;
    else
	ret = LB_UN_register (fd, LB_ANY, Lb_callback);
    if (ret < 0) {
	LB_close (fd);
	MISC_log (" - LB_register %s failed (%d)\n", lpath, ret);
	return (-1);
    }

    lb->path = STR_copy (lb->path, lpath);
    lb->fd = fd;
    if (Verbose)
	MISC_log ("LB %s created\n", lb->path);
    copy_bytes += len;

    Check_all_processed (copy_bytes);

    return (0);
}

/************************************************************************

    Checks if all LBs to replicate are ready.

************************************************************************/

static void Check_all_processed (int copy_bytes) {
    int i, done, cnt;

    done = 1;
    cnt = 0;
    for (i = 0; i < N_lbs; i++) {
	if (Lbs[i].fd == FD_NOT_INIT)
	    done = 0;
	else if (Lbs[i].fd == FD_FAILED)
	    cnt++;
    }
    if (done) {
	if (cnt * 2 >= N_lbs) {
	    MISC_log ("Too many (%d of %d) LBs missing rep info - owr_client terminates\n", cnt, N_lbs);
	    exit (1);
	}
	MISC_log ("All (%d, %d failed) to-replicate LBs ready are (%d bytes)\n",
					N_lbs, cnt, copy_bytes);
	All_lb_created = 1;
    }
}

/************************************************************************

    The LB event callback function.

************************************************************************/

static void Lb_callback (int fd, LB_id_t msg_id, int msg_info, void *arg) {
    static int pid = 0;
    extern int Terminating;
    int i;

    if (Terminating)
	return;

    if (msg_info <= 0)
	return;
    if (pid == 0)
	pid = getpid ();
    if (EN_sender_id () == pid)		/* ignore event posted by myself */
	return;

    for (i = 0; i < N_lbs; i++) {
	Rep_lb_t *lb = Lbs + i;
	if (lb->fd == fd) {
	    int nb;
	    if (lb->rep_type & (REP_TO_CLIENT | REP_TO_NONE))
		continue;
	    nb = Send_lb_update_msg (lb->did, fd, msg_id, msg_info);
	    lb->out_bytes += nb;
	    lb->out_cnt++;
	}
    }
}

/************************************************************************

    Returns the local replicated LB path for data "did". NULL is returned
    if "did" is not a to-replicate data store.

************************************************************************/

char *CF_get_local_path (int did) {
    int i;

    for (i = 0; i < N_lbs; i++) {
	Rep_lb_t *lb = Lbs + i;
	if (lb->did == did)
	    return (lb->path);
    }
    return (NULL);
}

/************************************************************************

    Sends updated message "msg_id" ("msg_len" bytes) of Lb "fd' to the
    server. The data store id is "id". Returns the number of bytes sent.

************************************************************************/

static int Send_lb_update_msg (int id, int fd, LB_id_t msg_id, int msg_len) {
    static int bs = 0;
    static char *rbuf = NULL;
    Message_t *mh;
    int len, ms;
    char *cbuf;

    if (msg_len + sizeof (Message_t) > bs) {
	if (rbuf != NULL)
	    free (rbuf);
	bs = msg_len + sizeof (Message_t);
	rbuf = MISC_malloc (bs);
    }

    len = LB_read (fd, rbuf + sizeof (Message_t), msg_len, msg_id);
    if (len == LB_BUF_TOO_SMALL) {
	char *buf;
	len = LB_read (fd, &buf, LB_ALLOC_BUF, msg_id);
	if (len > 0) {
	    if (rbuf != NULL)
		free (rbuf);
	    bs = len + sizeof (Message_t);
	    rbuf = MISC_malloc (bs);
	    memcpy (rbuf + sizeof (Message_t), buf, len);
	    free (buf);
	}
    }
    if (len <= 0) {
	MISC_log ("LB_read updated msg %d failed (%d)\n", msg_id, len);
	return (0);
    }

    cbuf = OC_compress_payload (sizeof (Message_t), 
				rbuf + sizeof (Message_t), len, &ms);

    mh = (Message_t *)cbuf;
    mh->type = LB_UPDATE;
    mh->id = id;
    mh->msg_id = msg_id;
    CL_send_to_server (cbuf, ms);

    if (bs > LARGE_BUF_SIZE) {
	free (rbuf);
	rbuf = NULL;
	bs = 0;
    }
    OS_free_buffer (cbuf);
    return (len + sizeof (Message_t));
}

/******************************************************************

    Sends all LB replication requests to the server.

******************************************************************/

int CF_send_lb_reqs () {
    int i;

    for (i = 0; i < N_lbs; i++) {
	Message_t msg;

	memset (&msg, 0, sizeof (Message_t));
	msg.type = REQ_LB_REP;
	msg.size = sizeof (Message_t);
	msg.id = Lbs[i].did;
	msg.code = Lbs[i].rep_type;
	CL_send_to_server ((char *)&msg, msg.size);
    }
    if (Verbose)
	MISC_log ("Requests (%d) for LN rep info sent\n", N_lbs);

    if (N_nreps > 0) {
	char *b;
	Message_t *mh;
	int ms = sizeof (Message_t) + N_nreps * sizeof (Not_rep_msg_t);
	b = MISC_malloc (ms);
	memset (b, 0, sizeof (Message_t));
	mh = (Message_t *)b;
	mh->type = REQ_NOT_REP;
	mh->code = N_nreps;
	mh->size = ms;
	memcpy (b + sizeof (Message_t), Nreps, 
					N_nreps * sizeof (Not_rep_msg_t));
	CL_send_to_server (b, ms);
	free (b);
	STR_free (Nreps);
	Nreps = NULL;
	N_nreps = 0;
    }

    return (0);
}

/******************************************************************

    Reads the owr configuration file.

******************************************************************/

int CF_read_config (char *cfgname) {
    char buf[MAX_STR_SIZE], *fname, *ntc;
    FILE *fd;
    enum {READ_NONE, READ_LB, READ_EVENT};
    int cr_sec;

    if (cfgname[0] == '/') {
	fname = cfgname;
	fd = fopen (fname, "r");
    }
    else {
	fname = STR_copy (NULL, "./");	
	fname = STR_cat (fname, cfgname);	
	fd = fopen (fname, "r");
	if (fd == NULL) {
	    char dir[MAX_STR_SIZE];
	    if (MISC_get_cfg_dir (dir, MAX_STR_SIZE) >= 0) {
		fname = STR_copy (fname, dir);	
		fname = STR_cat (fname, "/");	
		fname = STR_cat (fname, cfgname);	
		fd = fopen (fname, "r");
	    }
	}
    }
    if (fd == NULL) {
	MISC_log ("owr config file (%s) not found\n", cfgname);
	exit (1);
    }

    ntc = "not_to_cl-";
    cr_sec = READ_NONE;
    while (fgets (buf, MAX_STR_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	int i;

	if (MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;
	if (strcmp (tk, "LBs_replicated:") == 0)
	    cr_sec = READ_LB;
	else if (strcmp (tk, "AN_preregistered:") == 0)
	    cr_sec = READ_EVENT;
	else if (cr_sec == READ_LB) {
	    int did;
	    char c, cnt;
	    Rep_lb_t lb;

	    if (sscanf (tk, "%d%c", &did, &c) != 1) {
		MISC_log ("Unexpected configuration\n");
		MISC_log ("    on line: %s\n", buf);
		MISC_log ("    of owr config file %s\n", fname);
		exit (1);
	    }
	    memset (&lb, 0, sizeof (Rep_lb_t));
	    lb.did = did;
	    lb.fd = FD_NOT_INIT;
	    lb.rep_type = 0;
	    cnt = 1;
	    while (MISC_get_token (buf, "", cnt, tk, MAX_STR_SIZE) > 0) {
		if (strcmp (tk, "to_server_only") == 0)
		    lb.rep_type |= REP_TO_SERVER;
		else if (strcmp (tk, "to_client_only") == 0)
		    lb.rep_type |= REP_TO_CLIENT;
		else if (strcmp (tk, "to_none") == 0)
		    lb.rep_type |= REP_TO_NONE;
		else if (strncmp (tk, ntc, strlen (ntc)) == 0) {
		    int id, c;

		    c = 0;
		    while (MISC_get_token (tk + strlen (ntc), "S,Ci", c,
						&id, 0) > 0) {
			Not_rep_msg_t nm;
			nm.did = did;
			nm.msg_id = id;
			Nreps = (Not_rep_msg_t *)STR_append (Nreps, &nm, 
						    sizeof (Not_rep_msg_t));
			N_nreps++;
			c++;
		    }
		    if (c == 0) {
			MISC_log ("Unexpected %s statement\n", ntc);
			MISC_log ("    on line: %s\n", buf);
			MISC_log ("    of owr config file %s\n", fname);
		 	exit (1);
		    }
		}
		else if (strcmp (tk, "client_can_only_create_message") == 0)
		    lb.rep_type |= REP_CCOCM | REP_TO_SERVER | REP_LB_ANY;
		else if (strcmp (tk, "copy_lb") == 0)
		    lb.rep_type |= REP_COPY_LB;
		cnt++;
	    }
	    if (((lb.rep_type & REP_TO_SERVER) && 
					(lb.rep_type & REP_TO_CLIENT)) ||
		((lb.rep_type & (REP_TO_SERVER | REP_TO_CLIENT)) && 
					(lb.rep_type & REP_TO_NONE))) {
		MISC_log ("Conflicting to_ statements\n");
		MISC_log ("    on line: %s\n", buf);
		MISC_log ("    of owr config file %s\n", fname);
		exit (1);
	    }
	    for (i = 0; i < N_lbs; i++) {
		if (Lbs[i].did == lb.did)
		    break;
	    }
	    if (i < N_lbs) {
		if (Lbs[i].rep_type != lb.rep_type)
		    MISC_log ("Duplicated data store %d, different type\n",
								lb.did);
		else
		    MISC_log ("Duplicated data store %d ignored\n", lb.did);
		MISC_log ("    on line: %s\n", buf);
		MISC_log ("    of owr config file %s\n", fname);
		if (Lbs[i].rep_type != lb.rep_type)
		    exit (1);
		continue;
	    }
	    Lbs = (Rep_lb_t *)STR_append (Lbs, &lb, sizeof (Rep_lb_t));
	    N_lbs++;
	}
	else if (cr_sec == READ_EVENT) {
	    unsigned int an;
	    char c;

	    if (sscanf (tk, "%u%c", &an, &c) != 1 &&
		sscanf (tk, "%x%c", &an, &c) != 1) {
		MISC_log ("Unexpected owr config line in %s:%s\n", fname, buf);
		exit (1);
	    }
	    for (i = 0; i < N_Ans; i++) {
		if (Ans[i] == an)
		    break;
	    }
	    if (i < N_Ans) {
		MISC_log ("Duplicated AN %d ignored\n", an);
		MISC_log ("    on line: %s\n", buf);
		MISC_log ("    of owr config file %s\n", fname);
		continue;
	    }
	    Ans = (unsigned int *)STR_append (Ans, &an, sizeof (unsigned int));
	    N_Ans++;
	}
    }
    fclose (fd);
    MISC_log ("%d to-replicate LBs read from %s\n", N_lbs, fname);
    if (fname != cfgname)
	STR_free (fname);

    return (0);
}

/******************************************************************

    Reports replication statistics.

******************************************************************/

void CF_report_statistics () {
    static time_t l_report_time = 0;
    time_t crt;
    char *max_b_dir, *max_c_dir;
    int i, in_b, out_b, in_c, out_c;
    int max_b, max_b_in, max_b_i, max_c, max_c_in, max_c_i;

    crt = MISC_systime (NULL);
    if (l_report_time == 0 || N_lbs == 0) {
	l_report_time = crt;
	return;
    }

    if (crt < l_report_time + ST_REPORT_PERIOD)
	return;

    in_b = out_b = in_c = out_c = 0;
    max_b = max_b_in = max_b_i = max_c = max_c_in = max_c_i = 0;
    for (i = 0; i < N_lbs; i++) {
	Rep_lb_t *lb;

	lb = Lbs + i;
	in_b += lb->in_bytes;
	out_b += lb->out_bytes;
	in_c += lb->in_cnt;
	out_c += lb->out_cnt;
	if (lb->in_bytes > max_b) {
	    max_b = lb->in_bytes;
	    max_b_in = 1;
	    max_b_i = i;
	}
	if (lb->out_bytes > max_b) {
	    max_b = lb->out_bytes;
	    max_b_in = 0;
	    max_b_i = i;
	}
	if (lb->in_cnt > max_c) {
	    max_c = lb->in_cnt;
	    max_c_in = 1;
	    max_c_i = i;
	}
	if (lb->out_cnt > max_c) {
	    max_c = lb->out_cnt;
	    max_c_in = 0;
	    max_c_i = i;
	}
	lb->in_bytes = 0;
	lb->in_cnt = 0;
	lb->out_bytes = 0;
	lb->out_cnt = 0;
    }

    MISC_log ("Statistics: IN %d bytes (%d Bps, %d upds), OUT %d bytes (%d Bps, %d upds)\n",
			in_b, in_b / ST_REPORT_PERIOD, in_c, 
			out_b, out_b / ST_REPORT_PERIOD, out_c);
    max_b_dir = "out";
    if (max_b_in)
	max_b_dir = "in";
    max_c_dir = "out";
    if (max_c_in)
	max_c_dir = "in";
    MISC_log ("  MAX bytes %d (%s, %s), MAX update count %d (%s, %s)\n",
			max_b, max_b_dir, MISC_basename (Lbs[max_b_i].path),
			max_c, max_c_dir, MISC_basename (Lbs[max_c_i].path));

    l_report_time = crt;
}
