/*************************************************************************

      The main source file of the tool "lem" - the LE monitor.

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/30 21:43:55 $
 * $Id: lem.c,v 1.20 2012/07/30 21:43:55 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

#include <orpg.h>
#include <infr.h>

/* Constant Definitions/Macro Definitions/Type Definitions */
#define CRIT_MSGCODE_CHAR	'C'
#define NONCRIT_MSGCODE_CHAR	' '

#define DFLT_DYN_POLL_MSEC	500
#define NAME_LEN 256
#define PROCNAME_LEN 128
#define MAX_MSG_SIZE 512

typedef struct {		/* process struct */
    char procname[PROCNAME_LEN];
    char lbname[3 * NAME_LEN + 16];	/* add'l chars for / and .log */
    char host[NAME_LEN];
    int lbd;				/* LB descriptor */
    FILE *fh;
    int access_state;
    int init_n_msgs;			/* number of message when opened */
    int msg_cnt;			/* number of messages read */
    unsigned char msg_to_print;
    char text_file;
    char plain_log;
    char byteswap;
    ALIGNED_t curmsg[ALIGNED_T_SIZE (MAX_MSG_SIZE)];
				/* temp storage for msg before printing */
} Procary_entry;

/* Static variables */
static char Le_dir[NAME_LEN];
static int Old_msgs_only;
static int New_msgs_only;
static int Numprocs = 0;
static int Poll_rate_msec;
static Procary_entry *Procary_p = NULL;		/* Process List */
static char Prog_name[NAME_LEN];
static char *Le_file_name = NULL;
static int Orpg_env = -1;

static int Pid, New_vl;

static int (*Orpgda_read) (int, void *, int, LB_id_t);
static SMI_info_t *(*Orpg_smi_info) (char *, void *);

/* Static Function Prototypes */
static int Print_interleaved_le (int num_procs, Procary_entry *procary_p);
static void Print_lemsg (Procary_entry *proc);
static int Read_next_msg (Procary_entry *proc_p);
static int Read_options (int argc, char **argv);
static int Change_process_verbose_level ();
static void Remove_trailing_line_return (char *in_string);
static void Get_lb_name (char *argv, Procary_entry *proc);
static int Read_file (Procary_entry *proc);
static int Open_file (Procary_entry *proc);
static void File_lock (int lock, Procary_entry *proc);
static void *Rfgets (Procary_entry *proc, char *buf, int buf_size);
static int Rfseek (Procary_entry *proc, int off, int where);


/**************************************************************************

    The main function.

 **************************************************************************/

int main (int argc, char **argv) {
    int i, retval, optind;
    Procary_entry *proc_p;

    optind = Read_options (argc, argv);
    if (optind < 0)
       exit (1);

    if (Pid > 0)
	exit (Change_process_verbose_level ());

    if (Le_file_name != NULL) {
	Numprocs = 1; 
	Procary_p = malloc (Numprocs * sizeof (Procary_entry));
	if (Procary_p == NULL) {
	    fprintf (stderr, "malloc failed - lem");
	    exit (1);
	}
	proc_p = Procary_p;
	strcpy (proc_p->lbname, (char *)Le_file_name);
	strcpy (proc_p->procname, MISC_basename ((char *)Le_file_name));
	if (strcmp (proc_p->procname + strlen (proc_p->procname) - 4,
							".log") == 0)
	    proc_p->procname[strlen (proc_p->procname) - 4] = '\0';
    }
    else {	/* Read the process name(s) from the command-line */

	Numprocs = argc - optind; 
        if (Numprocs <= 0) {
            fprintf(stderr, "No processes specified - lem\n");
            exit (0);
        }
	Procary_p = malloc (Numprocs * sizeof (Procary_entry));
	if (Procary_p == NULL) {
	    fprintf (stderr, "malloc failed - lem");
	    exit (1);
	}

	for (i = optind; i < argc; i++) {
	    proc_p = Procary_p + (i - optind);
	    Get_lb_name (argv[i], proc_p);
	}
    }

    /* monitoring of LE LB messages */
    for (i = 0; i < Numprocs; i++) {	/* open each of the LB files */
	int ret;

	proc_p = (Procary_p + i);
	proc_p->lbd = LB_open (proc_p->lbname, LB_READ, NULL);
	if (proc_p->lbd == LB_NON_LB_FILE || proc_p->lbd == LB_LB_ERROR)
	    Open_file (proc_p);
	else
	    proc_p->text_file = 0;
	if (proc_p->lbd == LB_BAD_BYTE_ORDER) {
	    LB_fix_byte_order (proc_p->lbname);
	    proc_p->lbd = LB_open (proc_p->lbname, LB_READ, NULL);
	}
	if (proc_p->lbd < 0) {
	    fprintf (stderr, "LB_open (%s) returned %d - lem\n",
					proc_p->lbname, proc_p->lbd);
	    exit (1);
	}

	if (!proc_p->text_file) {
	    if (!New_msgs_only) {
		if ((retval = LB_seek (proc_p->lbd, 0, LB_FIRST, NULL)) < 0) {
		    fprintf (stderr, "LB_seek (%s) returned %d - lem\n",
						    proc_p->lbname, retval);
		    exit (1);
		}
	    }
    
	    ret = LB_misc (proc_p->lbd, LB_IS_BIGENDIAN);
	    if (ret < 0) {
		fprintf (stderr, 
		    "LB_misc LB_IS_BIGENDIAN failed (ret = %d) - lem\n", ret);
		exit (1);
	    }
	    if (ret == MISC_i_am_bigendian ())
		proc_p->byteswap = 0;
	    else
		proc_p->byteswap = 1;
	}
    }

    fprintf (stderr, "Monitoring messages (every %d msecs) in ",
						Poll_rate_msec);
    for (i = 0; i < Numprocs; i++) {
	proc_p = (Procary_p + i);
	if (proc_p->lbd > 0)
	    fprintf (stderr, "%s ", proc_p->lbname);
    }
    fprintf (stderr, " - lem\n");
    fflush (stdout);
    for (;;) {
	msleep (Poll_rate_msec);
	Print_interleaved_le (Numprocs, Procary_p);
	fflush (stdout);
	for (i = 0; i < Numprocs; i++) {
	    proc_p = (Procary_p + i);
	    if (proc_p->access_state > 0)
		proc_p->access_state = 0;
	}
	if (Old_msgs_only)
	    break;
    }

    exit (0);
}

/********************************************************************

    Changes process's ("Pid") verbose level to "New_vl". We don't call
    LE_set_vl because we don't want call LE_init, which requires the
    LE LB to exist. Returns 0 on success or 1 on failure.

*********************************************************************/

static int Change_process_verbose_level () {
    unsigned int *add;
    int nadd;
    char *dir_ev, *pt;
    LE_VL_change_ev_t msg;
    int event_num, ret;

    nadd = NET_find_local_ip_address (&add);
    if (nadd <= 0) {
	fprintf (stderr, "NET_find_local_ip_address failed (ret %d) - lem\n", nadd);
	return (1);
    }

    if ((dir_ev = getenv ("LE_DIR_EVENT")) == NULL) {
	fprintf (stderr, "Environment LE_DIR_EVENT not defined - lem\n");
	return (1);
    }
    pt = dir_ev;
    while (*pt != '\0' && *pt != ':')
	pt++;
    if (*pt != ':' ||
	sscanf (pt + 1, "%d", &event_num) != 1) {
	fprintf (stderr, "LE event number not found - lem\n");
	return (1);
    }

    msg.host_ip = htonl (add[0]);
    msg.pid = htonl (Pid);
    msg.new_vl = htonl (New_vl);

    ret = EN_post (event_num, (void *)&msg, 
					sizeof (LE_VL_change_ev_t), 0);
    if (ret < 0) {
	fprintf (stderr, "EN_post failed (ret %d) - lem\n", ret);
	return (1);
    }

    return (0);
}

/**************************************************************************

    Prints all LE messages of "num_procs" processes of "procary_p" from
    the current LE message pointer. Returns the number of messages printted.

 **************************************************************************/

static int Print_interleaved_le (int num_procs, Procary_entry *procary_p) {
    register int i;
    unsigned int msgs_to_print;
    short oldest_index;
    time_t oldest_time = (time_t) 0;
    Procary_entry *proc_p;
    int cnt;

    cnt = 0;
    /* Read initial batch of messages (one message per process) */
    msgs_to_print = 0;
    for (i = 0; i < num_procs; i++) {
        proc_p = (procary_p + i);
        if (Read_next_msg (proc_p) == 0)
	    msgs_to_print++;
    }

    while (msgs_to_print) {
        oldest_index = 0;
        oldest_time = (time_t)0;
        for (i = 0; i < num_procs; i++) {  /* search for the oldest msg */
            LE_message *lemsg_p;

            proc_p = (procary_p + i);
            if (proc_p->msg_to_print) {	/* no message to be printed */
		if (proc_p->text_file) {
		    oldest_index = i;
		    break;
		}
                lemsg_p = (LE_message *)proc_p->curmsg;
                if (oldest_time == (time_t)0) {
                    oldest_time = lemsg_p->time;
                    oldest_index = i;
                }
                else {
                    if (lemsg_p->time < oldest_time) {
                        oldest_time = lemsg_p->time;
                        oldest_index = i;
                    }
                }
            }

        }

        /* Print the oldest LE LB message */
        proc_p = (procary_p + oldest_index);
        Print_lemsg (proc_p);
        proc_p->msg_to_print = 0;
        msgs_to_print--;
	cnt++;

        /* read next message for the process for which we just printed an LE 
	   message */
        if (Read_next_msg (proc_p) == 0)
            msgs_to_print++;
    }

    return (cnt);
}

/**************************************************************************

    Prints the LE message "msg_p" from process "procname".

 **************************************************************************/

static void Print_lemsg (Procary_entry *proc) {
    char *procname;
    int code;
    LE_critical_message *critmsg_p;
    char fname[LE_SOURCE_NAME_SIZE];
    LE_message* lemsg_p;
    char msgcode_char;        /* use unless code-mapping specified       */
    int n_reps;
    pid_t pid = 0;
    int line_num = 0;
    char text[MAX_MSG_SIZE];
    time_t time;
    int day, hr, min, month, sec, year;
    int yr;
    static char disp_date[9]; /* static so will be nulled-out            */
    static char disp_time[9]; /* static so will be nulled-out            */

    static int cur_day;       /* static to survive from call to call     */
    static int cur_month;
    static int cur_yr;
    char msg_types[16];

    if (proc->text_file) {
	printf ("%s", (char *)proc->curmsg);
	return;
    }

    procname = proc->procname;
    lemsg_p = (LE_message *)proc->curmsg;

    code = lemsg_p->code;

    if (code & LE_CRITICAL_BIT) {

        msgcode_char = CRIT_MSGCODE_CHAR;

        /* Display Critical LE Message */
        critmsg_p = (LE_critical_message *)proc->curmsg;

        pid = (pid_t) critmsg_p->pid;
        time = critmsg_p->time;
        n_reps = critmsg_p->n_reps;

        line_num = critmsg_p->line_num;
        strncpy (fname, critmsg_p->fname, LE_SOURCE_NAME_SIZE-1);
        fname[LE_SOURCE_NAME_SIZE-1] = '\0';

        /* Retrieve the message text ... note that we must "back up"
           the pointer because of the padding bytes */
        strcpy ((char *)text, critmsg_p->text);

        /* Remove trailing line-returns from the message text */
        Remove_trailing_line_return ((char *)text);
    }
    else {

        msgcode_char = NONCRIT_MSGCODE_CHAR;

        time = lemsg_p->time;
        n_reps = lemsg_p->n_reps;

        /* Retrieve the message text ... note that we must "back up"
           the pointer because of the padding bytes */
        strcpy ((char *)text, lemsg_p->text);

        /* Remove trailing line-returns from the message text */
        Remove_trailing_line_return ((char *)text);
   }

    if (time == 0)
	time = 1;	/* make sure the following conversion to y, m, d... */
    unix_time ((time_t *) &time,&year,&month,&day,&hr,&min,&sec);
    yr = year % 100;

    if ((cur_day != day) || (cur_month != month) || (cur_yr != yr)) {
        /* Display the day/month/year information only if it has changed */
        cur_day = day; cur_month = month; cur_yr = yr;
        sprintf (disp_date, "%02d/%02d/%02d", cur_month,cur_day,cur_yr);
        printf ("%s\n", disp_date);
    }

    sprintf (disp_time, "%02d:%02d:%02d", hr,min,sec);

    msg_types[0] = '\0';
    if (code & GL_ERROR_BIT)
	strcat (msg_types, "E");
    if (code & GL_STATUS_BIT)
	strcat (msg_types, "S");
    if (code & GL_GLOBAL_BIT)
	strcat (msg_types, "G");
    if (msg_types[0] == '\0' && (code & LE_CRITICAL_BIT))
	strcat (msg_types, "C");
    if ((code & LE_VL3) == LE_VL3)
	strcat (msg_types, "3");
    else {
	if ((code & LE_VL1) == LE_VL1)
	    strcat (msg_types, "1");
	if ((code & LE_VL2) == LE_VL2)
	    strcat (msg_types, "2");
    }

    printf ("%s %2s %s", disp_time, msg_types, (char *)text);

    if (n_reps != 1)
        printf (" (n_reps: %d)", n_reps);

    if (code & LE_CRITICAL_BIT)
        printf (" -%s:%d\n", fname, line_num);
    else
	printf ("\n");
}

/**************************************************************************

    Removes trailing line return character in string "in_string".

 **************************************************************************/

static void Remove_trailing_line_return (char *in_string) {
    char *ptr;

    if (in_string == NULL)
        return;

    ptr = in_string + strlen (in_string) - 1;
    while (ptr >= in_string) {
        if (*ptr == '\n')
	    *ptr = '\0';
	else
	    break;
        ptr--;
    }

    return;
}

/**************************************************************************

    Reads the next LE LB message for the specified process. Returns: 0 
    upon success or -1 if the next message is not found.

 **************************************************************************/

static int Read_next_msg (Procary_entry *proc_p) {

    proc_p->msg_to_print = 0;
    while (1) {
	int len;
	if (proc_p->text_file) {
            len = Read_file (proc_p);
	    if (len == -1)
		exit (1);
	}
	else
            len = LB_read (proc_p->lbd, proc_p->curmsg, 
					MAX_MSG_SIZE, LB_NEXT);
        if (len <= 0) {
            if (len == LB_TO_COME)		/* We're through */
                return (-1);
            else if (len != LB_EXPIRED) {
                fprintf (stderr, "LB_read (%s) failed (returned %d) - lem\n",
						proc_p->lbname, len);
		exit (1);
	    }
	    else
                fprintf (stderr, "LB_read - LB_EXPIRED - lem\n");
        }
        else {
	    if (Orpg_env == 1 && !proc_p->text_file) {
		int code, ret;
		unsigned int n_reps;
		n_reps = ((LE_message *)(proc_p->curmsg))->n_reps;
		if (proc_p->byteswap || n_reps > 0xffff) {
		    SMIA_set_smi_func (Orpg_smi_info);
		    code = *((int *)proc_p->curmsg);
		    code = INT_BSWAP (code);
		    if (code & LE_CRITICAL)
			ret = SMIA_bswap_input ("LE_critical_message", 
						    proc_p->curmsg, len);
		    else
			ret = SMIA_bswap_input ("LE_message", proc_p->curmsg, len);
		    if (ret < 0) {
			fprintf (stderr, "SMIA_bswap_input failed (%d) - lem\n", ret);
			exit (1);
		    }
		}
	    }

            proc_p->msg_to_print = 1;
            return (0);
        }
    }

    return (-1);
}

/**************************************************************************
 
    Read a line from file "proc->fh" into "proc_p->curmsg" of size 
    MAX_MSG_SIZE. Returns the number of bytes or LB_TO_COME if no more data
    in the file. Returns -1 on fatal error.

 **************************************************************************/

static int Read_file (Procary_entry *proc) {
    int ret_value;
    char *buf;

/* Read_file (proc_p->fd, (char *)proc_p->curmsg, MAX_MSG_SIZE) */

    ret_value = 0;
    buf = (char *)proc->curmsg;
    File_lock (1, proc);
    while (1) {
	char *fg_ret;
	int ret;

	fg_ret = Rfgets (proc, buf, MAX_MSG_SIZE);

	if (!proc->plain_log) {
	    if (fg_ret == NULL) {
		ret_value = LB_TO_COME;
		break;
	    }
	    ret_value = strlen (buf);
	    break;
	}

	if (fg_ret == NULL) {
	    if (proc->access_state == -3) {
		fprintf (stderr, "Cannot find the latest message - Done - lem\n");
		exit (1);
	    }
	    if (proc->access_state < 0 && proc->msg_cnt >= proc->init_n_msgs)
		proc->access_state = 0;
	    if (proc->access_state >= 0)
		proc->access_state++;
	    if (proc->access_state >= 2) {
		fprintf (stderr, "Cannot continue without repeating messages - Done - lem\n");
		exit (1);
	    }
	    Rfseek (proc, strlen (LE_LOG_LABEL), SEEK_SET);
	    continue;
	}

	if (buf[0] == '\n') {
	    if (proc->access_state == -2)
		proc->access_state = -1;
	    if (proc->msg_cnt < proc->init_n_msgs)
		continue;
	    ret = Rfseek (proc, -1, SEEK_CUR);
	    ret_value = LB_TO_COME;
	    break;
	}
	proc->msg_cnt++;
	if (proc->access_state == -2)
	    continue;

	if ((buf[2] != ':' && buf[2] != '/') || 
					(buf[5] != ':' && buf[5] != '/'))
	    continue;
	ret_value = strlen (buf);
	break;
    }
    File_lock (0, proc);
    return (ret_value);

}

/**************************************************************************
 
    fflush the file and reads the next line of it.

 **************************************************************************/

static void *Rfgets (Procary_entry *proc, char *buf, int buf_size) {
    int ret, int_ret;
    char cmd[128];
    FILE *fg_ret;

    sprintf (cmd, "%s:fflush", proc->host);
    ret = RSS_rpc (cmd, "i-r p", &int_ret, proc->fh);
    if (ret < 0) {
	fprintf (stderr, "fflush (%s) failed (ret %d) - lem\n",
				    proc->lbname, ret);
	exit (1);
    }

    sprintf (cmd, "%s:fgets", proc->host);
    ret = RSS_rpc (cmd, "p-r ba-%d-o i p", buf_size,
			    &fg_ret, buf, buf_size, proc->fh);
    if (ret < 0) {
	fprintf (stderr, "fgets (%s) failed (ret %d) - lem\n",
				    proc->lbname, ret);
	exit (1);
    }
    return (fg_ret);
}

/**************************************************************************

    Opens the text file and position the file pointer approriately.

 **************************************************************************/

static int Open_file (Procary_entry *proc) {
    char fname[128], host[128], cmd[256], buf[256];
    FILE *fhdl;
    int ret, bytes_read, lr_pos, st_pos, int_ret, cnt, ucnt;

    if (MISC_get_token (proc->lbname, "S:", 1, fname, 128) > 0)
	MISC_get_token (proc->lbname, "S:", 0, host, 128);
    else {
	MISC_get_token (proc->lbname, "S:", 0, fname, 128);
	host[0] = '\0';
    }
    strncpy (proc->host, host, NAME_LEN);
    proc->host[NAME_LEN - 1] = '\0';

    sprintf (cmd, "%s:fopen", host);
    ret = RSS_rpc (cmd, "p-r s-i s-i", &fhdl, fname, "r");
    if (ret < 0 || fhdl == NULL) {
	fprintf (stderr, "fopen (%s) failed (ret %d %p) - lem\n",
					proc->lbname, ret, fhdl);
	exit (1);
    }

    sprintf (cmd, "%s:fileno", host);
    ret = RSS_rpc (cmd, "i-r p", &int_ret, fhdl);
    if (ret < 0) {
	fprintf (stderr, "fileno (%s) failed (ret %d) - lem\n",
						proc->lbname, ret);
	exit (1);
    }
    proc->lbd = int_ret;

    proc->access_state = 0;
    proc->text_file = 1;
    proc->plain_log = 0;
    proc->fh = fhdl;
    bytes_read = 0;
    lr_pos = -1;
    st_pos = 0;
    cnt = ucnt = 0;
    File_lock (1, proc);
    while (1) {
	char *fget_ret = Rfgets (proc, buf, 256);
	if (bytes_read == 0) {
	    if (fget_ret == NULL || strcmp (buf, LE_LOG_LABEL) != 0)
		break;
	    proc->plain_log = 1;
	    bytes_read = strlen (buf);
	    st_pos = bytes_read;
	    continue;
	}
	if (fget_ret == NULL)
	    break;
	cnt++;
	bytes_read += strlen (buf);
	if (buf[0] == '\n') {
	    cnt--;
	    lr_pos = bytes_read - 1;
	    cnt -= ucnt;
	    ucnt = 0;
	    continue;
	}
	if (lr_pos >= 0)
	    ucnt++;
    }
    if (lr_pos >= 0) {
	st_pos = lr_pos;
	proc->access_state = -2;
    }
    else
	proc->access_state = -3;

    sprintf (cmd, "%s:fseek", host);
    ret = RSS_rpc (cmd, "i-r p i i", &int_ret, fhdl, st_pos, SEEK_SET);
    if (ret < 0) {
	fprintf (stderr, "fseek (%s) failed (ret %d) - lem\n",
						proc->lbname, ret);
	exit (1);
    }
    File_lock (0, proc);
    proc->init_n_msgs = cnt / 2;
    proc->msg_cnt = 0;

    return (0);
}

/********************************************************************
			
    Locks/unlocks the plain log file for read access.

********************************************************************/

static void File_lock (int lock, Procary_entry *proc) {
    int ret, int_ret;
    struct flock fl;		/* structure used by fcntl */
    char cmd[128];

    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 1;
    if (lock)
	fl.l_type = F_RDLCK;
    else
	fl.l_type = F_UNLCK;

    sprintf (cmd, "%s:fcntl", proc->host);
    ret = RSS_rpc (cmd, "i-r i i ba-%d-i", sizeof (struct flock), &int_ret, proc->lbd, F_SETLKW, &fl);
}

/********************************************************************
			
    fseek the remote file.

********************************************************************/

static int Rfseek (Procary_entry *proc, int off, int where) {
    char cmd[128];
    int ret, int_ret;

    sprintf (cmd, "%s:fseek", proc->host);
    ret = RSS_rpc (cmd, "i-r p i i", &int_ret, proc->fh, off, where);
    if (ret < 0) {
	fprintf (stderr, "fseek (%s) failed (ret %d) - lem\n",
						proc->lbname, ret);
	exit (1);
    }
    return (int_ret);
}

/**************************************************************************
 
    Constructs the LB name for command line process name "argv".

 **************************************************************************/

static void Get_lb_name (char *argv, Procary_entry *proc) {
    static char *pstat_buf, *nodei_buf;
    static int pstat_size, nodei_size;
    char *node_name, buf[PROCNAME_LEN], buf1[PROCNAME_LEN], *p, *env;

    p = argv;
    while (*p != '\0') {
	if (*p == ':')
	    break;
	p++;
    }
    if (*p == ':') {		/* node name found */
	if (strlen (Le_dir) > 0) {
	    fprintf (stderr, "node name unexpected - LE dir is specified - lem\n");
	    exit (1);
	}
	if (p - argv >= PROCNAME_LEN) {
	    fprintf (stderr, "node too long - lem\n");
	    exit (1);
	}
	memcpy (buf, argv, p - argv);
	buf[p - argv] = '\0';
	node_name = buf;
	strncpy (proc->procname, p + 1, PROCNAME_LEN);
    }
    else {
	node_name = NULL;
	strncpy (proc->procname, argv, PROCNAME_LEN);
    }
    proc->procname[PROCNAME_LEN - 1] = '\0';
    proc->lbname[0] = '\0';

    if (strlen (Le_dir) > 0) {
	sprintf(proc->lbname, "%s/%s.log", Le_dir, proc->procname);
	return;
    }

    if (Orpg_env < 0) {		/* get process and node info */

	LE_set_option ("LE disable", 1);
	Orpgda_read = MISC_get_func ("liborpg.so", "ORPGDA_read", 1);
	Orpg_smi_info = MISC_get_func ("liborpg.so", "ORPG_smi_info", 1);
	if (Orpgda_read == NULL || Orpg_smi_info == NULL)
	    Orpg_env = 0;
	else {
	    Orpg_env = 1;
	    pstat_size = Orpgda_read (ORPGDAT_TASK_STATUS, (char *)&pstat_buf, 
					    LB_ALLOC_BUF, MRPG_PS_MSGID);
	    if (pstat_size < 0) {
		pstat_buf = NULL;
		nodei_buf = NULL;
	    }
	    else {
		nodei_size = Orpgda_read (ORPGDAT_TASK_STATUS, 
			(char *)&nodei_buf, LB_ALLOC_BUF, MRPG_RPG_NODE_MSGID);
		if (nodei_size <= 0)
		    nodei_buf = NULL;
	    }
	}
	LE_set_option ("LE disable", 0);
    }
Orpg_env = 0;

    if (Orpg_env == 1 && pstat_buf != NULL) {	/* search for the node */
	int off, cnt;
	Mrpg_process_status_t *ps;
	Mrpg_node_t *ns;

	if (node_name == NULL) {
	    char *cpt, dot_found;
	    Mrpg_process_status_t *pfound[256];
	    int i;

	    cnt = 0;
	    off = 0;
	    while (1) {
		ps = (Mrpg_process_status_t *)(pstat_buf + off);
		if (off + sizeof (Mrpg_process_status_t) > pstat_size ||
		    off + ps->size > pstat_size ||
		    ps->name_off == 0)
		    break;
		dot_found = 0;
		cpt = proc->procname;
		while (*cpt != '\0') {
		    if (*cpt == '.') {
			*cpt = '\0';
			dot_found = 1;
			break;
		    }
		    cpt++;
		}
		if (strcmp (pstat_buf + off + ps->name_off, 
						    proc->procname) == 0) {
		    for (i = 0; i < cnt; i++) {
			if (ps->instance == pfound[i]->instance &&
			    strcmp (pstat_buf + off + ps->node_off,
			    (char *)(pfound[i]) + pfound[i]->node_off) != 0) {
			    fprintf (stderr, 
				"process %s found on multiple nodes - lem\n", 
							    proc->procname);
			    exit (1);
			}
		    }
		    node_name = pstat_buf + off + ps->node_off;
		    if (cnt >= 256) {
			fprintf (stderr, "Too many process instances (%s) - lem\n", 
							    proc->procname);
			exit (1);
		    }
		    pfound[cnt] = ps;
		    cnt++;
		}
		if (dot_found)		/* restore the name */
		    *cpt = '.';
		off += ps->size;
	    }
	}

	off = 0;
	if (nodei_buf != NULL && node_name != NULL) {
	    while (1) {
		ns = (Mrpg_node_t *)(nodei_buf + off);
		if (off + sizeof (Mrpg_node_t) > nodei_size ||
		    off + ns->size > nodei_size ||
		    ns->node_off == 0) {
		    break;
		}
		if (strcmp (nodei_buf + off + ns->node_off, node_name) == 0) {
		    sprintf (proc->lbname, "%s:%s/%s.log", 
			    nodei_buf + off + ns->host_off, 
			    nodei_buf + off + ns->logdir_off, proc->procname);
		    return;
		}
		off += ns->size;
	    }
	}
    }

    env = RSS_expand_env (node_name, "$(LE_DIR_EVENT)", buf1, PROCNAME_LEN);
    if (buf1[0] != '\0' && buf1[0] != '$') {	/* env found */
	char *p = buf1;
	while (*p != '\0') {
	    if (*p == ':') {
		*p = '\0';
		break;
	    }
	    p++;
	}
	if (node_name == NULL || node_name[0] == '\0')
	    sprintf(proc->lbname, "%s/%s.log", buf1, proc->procname);
	else
	    sprintf(proc->lbname, "%s:%s/%s.log", 
			    node_name, buf1, proc->procname);
    }
    else {
	if (node_name == NULL || node_name[0] == '\0')
	    sprintf(proc->lbname, "%s.log", proc->procname);	/* local dir */
	else {
	    fprintf (stderr, "LE_DIR_EVENT not defined on %s - lem\n",
							node_name);
	    exit (1);
	}
    }

    return;
}

/**************************************************************************
 Description: Read the command-line options and initialize several global
              variables.
       Input: argc, argv,
      Output: none
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */
    int retval2;

    Poll_rate_msec = DFLT_DYN_POLL_MSEC;
    Old_msgs_only = New_msgs_only = 0;
    strncpy (Prog_name, argv[0], NAME_LEN);
    Prog_name[NAME_LEN - 1] = '\0';
    Le_file_name = NULL;
    Pid = 0;
    New_vl = 0;
    Le_dir[0] = '\0';

    err = 0;
    while ((c = getopt (argc, argv, "v:d:hf:p:sn")) != EOF) {
    switch (c) {

	case 'v':
	    if (sscanf (optarg, "%d%*c%d", &Pid, &New_vl) != 2) {
                fprintf (stderr, "Incorrect -v option\n");
		err = 1;
	    }
 	    return (0);

        case 'd':
            if (strlen (optarg) < NAME_LEN)
                strcpy (Le_dir, optarg);
            else {
                fprintf (stderr, "LE directory too long\n");    
                err = 1;
            }
            break;

        case 'f':
            if (strlen (optarg) < PROCNAME_LEN)
		Le_file_name = optarg;
            else {
                fprintf (stderr, "file name too long\n");
                err = 1;
            }
            break;

        case 'p':
            retval2 = sscanf (optarg, "%d", &Poll_rate_msec);
            if (retval2 != 1) {
                fprintf (stderr, "failed to read Poll_rate_msec\n");
                err = 1;
                break;
            }
            break;

        case 's':
            Old_msgs_only = 1;
            break;

        case 'n':
            New_msgs_only = 1;
            break;

        case 'h':
        case '?':
            err = 1;
            break;
        }
    }

    if (err == 1) {
        printf ("Usage: %s [options] [proc1 proc2 ... procN]\n",
                       Prog_name);
        printf ("\tProcess name can have node name in front of it.\n");
        printf ("\tOptions:\n");
        printf ("\t-v  pid,vl (change process pid's verbose level to vl)\n");
        printf ("\t-d  le_dir (directory for log files.\n");
        printf ("\t            Default: syscfg or LE_DIR_EVENT or cwd)\n");
        printf ("\t-f  le_file_name (explicit log file name. Default: none)\n");
        printf ("\t-p  poll_msec (dynamic poll rate. Default: %d)\n", Poll_rate_msec);
        printf ("\t-s  (prints existing messages and terminates)\n");
        printf ("\t-n  (prints new messages only)\n");
        printf ("\t\n");
        printf ("\tExamples:\n");
        printf ("\t%s -d /save_log/logs pbd\n", Prog_name);
        printf ("\t\t- monitor pbd in direcory /save_log/logs\n");
        printf ("\t%s pbd veldeal\n", Prog_name);
        printf ("\t\t- monitor pbd and veldeal\n");
        printf ("\t%s -v 2365,2\n", Prog_name);
        printf ("\t\t- change local process pid 2365's verbose level to 2\n");
        printf ("\t%s pc:nds lb_rep\n", Prog_name);
        printf ("\t\t- monitor nds on node pc and lb_rep\n");
        printf ("\t%s -f /save_log/logs/pbd.log\n", Prog_name);
        printf ("\t\t- monitor log file /save_log/logs/pbd.log\n");

        return (-1);
    }

    return (optind);
}
