/****************************************************************
		
    Module: le_logerr.c	
		
    Description: This file contains the functions of the log and 
		error (LE) messaging module.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:33 $
 * $Id: le_logerr.c,v 1.93 2012/07/27 19:33:33 jing Exp $
 * $Revision: 1.93 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h> 
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

#include <infr.h>
#include <le_def.h>


#define TIME_FIELD_SIZE 9	/* size of time field */
#define REP_FIELD_SIZE	24	/* max size of n_reps field */

#define RE_TIME		1	/* check time (in seconds) for repeated 
				   messages */
#define N_FIRST_REP	10	/* number of the first repeated msgs that are
				   sent */
#define N_NONBLOCKING_CALLS	10
				/* number of critical messages in RE_TIME 
				   time period without introducing blocking */
#define BLOCKING_TIME	20	/* blocking time in MS */

#define FROM_CB_BIT	0x80000000
				/* We overload this bit on LE_message.n_reps 
				   for saving LE msgs from LE callback */

static char Prog_name_field[LE_NAME_LEN + 32] = "";
				/* name field (Prog_name: ) */
static char Process_name[LE_NAME_LEN] = "";
				/* name of this process */
static int Argc = 0;		/* saved argc */
static char **Argv = NULL;	/* saved argv */
static int Name_field_len = 0;	/* size of name field */

static int Event_number = 0;	/* event number used. 0 means not available */

static int Lb_fd = -1;		/* the LB file descriptor; -1 means that LB 
				   is not used */

static int Line_num = 0;	/* source line number; 0 indicates that the
				   source file info is not available. */
static char *Src_name = "";	/* source file name */

static int Le_send_msg_called = 0;
				/* LE_send_msg has been called */

static int Set_foreground = 0;	/* ignore the LE_DIR_EVENT variable and use
				   stderr for LE message output */
static int Also_print_stderr = 0;
				/* Also print on stderr when Lb is used */
static int Le_disabled = 0;	/* LE_send_message disabled */
static int Print_src_mask = 0;	/* bit mask for printing source file info */
static int Use_plain_file = 0;

static void (*User_callback) (char *, int) = NULL;
				/* user registered callback function */
static int In_callback = 0;	/* in LE callback function */
static int Process_saved_from_cb = 0;
				/* processing saved msg send from callback */

static volatile int Inuse = 0;	/* flag indicating a msg is in processing */
static volatile int Saved_msg_cnt = 0;	/* count of saved messages */
static volatile int Terminate = 0;	/* termination flag */
static void (*Term_func)();		/* termination callback function */
static int Term_status;			/* termination status */

static int Current_VL = 0;		/* current process verbose level */

static FILE *Out_port = NULL;

/* local functions */
static void Output_message (char *msg, time_t cr_time);
static void Print_message (char *buf, int code, int to_file);
static int Block_signals (int how);
static void Process_message (char *buf);
static void Handle_termination ();
static void Process_saved_msgs ();
static void Vl_change_callback (EN_id_t evtcd, char *buf, int msglen, void *arg);
static void Send_msg_vargs (int code, const char *format, va_list args);
static void Misc_log_cb (char *msg);
static void Rss_rpc_stdout_cb (char *msg);
static char *Process_time_label (int code, char *buf);


/********************************************************************
			
    Description: Registers a callback function which is called when
		an LE message is reported. The callback function
		has an interface:

		void callback (char *msg, int msg_len);

		where "msg" is the binary data structure of LE_message
		or LE_critical_message and "msg_len" is the number of 
		bytes in the message.

    Inputs:	callback - the callback function pointer.

********************************************************************/

void LE_set_callback (void (*callback) (char *, int)) {
    User_callback = callback;
}

/********************************************************************
			
    Closes the LE service. 

********************************************************************/

int LE_close () { 

    if (Lb_fd < 0)
	return (0);
    if (Use_plain_file)
	close (Lb_fd);
    else
	LB_close (Lb_fd);
    Lb_fd = -1;
    if (Event_number != 0) {
	MISC_log_disable (1);
	EN_control (EN_DEREGISTER);
	EN_register (Event_number, Vl_change_callback);
	MISC_log_disable (0);
    }
    Le_send_msg_called = 0;
    User_callback = NULL;
    In_callback = 0;
    Process_saved_from_cb = 0;
    Inuse = 0;
    Saved_msg_cnt = 0;
    return (0);
}

/********************************************************************
			
    Returns the LE LB file descriptor.

********************************************************************/

int LE_fd () {
    return (Lb_fd);
}

/********************************************************************
			
    Description: This function sets up the termination flag. See
		LE manpage for further details.

    Inputs:	term_func - termination callback function.
		status - program exit status value.

********************************************************************/

void LE_terminate (void (*term_func)(), int status)
{

    Terminate = 1;
    Term_func = term_func;
    Term_status = status;

    if (!Inuse)
	Handle_termination ();

    return;
}

/********************************************************************
			
    Description: This function handles the termination procedure.

********************************************************************/

static void Handle_termination ()
{

    if (Term_func != NULL)
	Term_func ();
    else 
	exit (Term_status);
    return;
}

/********************************************************************
			
    Description: This function accepts the source file name and the
		line number. 

    Inputs:	file - source file name.
		line_num - line number where LE_send_msg is called.

    Return:	This function returns LE_CRITICAL_BIT.

********************************************************************/

unsigned int LE_file_line (char *file, int line_num)
{

    Src_name = file;
    Line_num = line_num;
    return (LE_CRITICAL_BIT);
}

/********************************************************************
			
    Description: This function sets the Set_foreground flag. 

********************************************************************/

void LE_set_foreground () {
    Set_foreground = 1;
    return;
}

/********************************************************************
			
    Description: This function sets Print_src_mask. 

********************************************************************/

void LE_set_print_src_mask (unsigned int mask) {
    Print_src_mask = mask;
    return;
}

/********************************************************************
			
    Description: This function sets Out_port. 

********************************************************************/

void LE_set_output_fd (void *fl) {
    Out_port = (FILE *)fl;
    return;
}

/********************************************************************
			
    Sets the LE disable level. 

********************************************************************/

int LE_set_disable (int yes) {
    if (yes) {
	Le_disabled++;
	MISC_log_disable (1);
    }
    else {
	MISC_log_disable (0);
	if (Le_disabled > 0)
	    Le_disabled--;
    }
    return (Le_disabled);
}

/********************************************************************
			
    Description: This function sets the Also_print_stderr flag. 

********************************************************************/

void LE_also_print_stderr (int yes) {
    Also_print_stderr = yes;
    return;
}

/********************************************************************
			
    Returns the name of this process in "buf" of size "buf_size". If
    "buf_size" is too small, the name is truncated and null-terminated.
    The name is an empty string in case LE_init has never been called.

********************************************************************/

void LE_get_process_name (char *buf, int buf_size) {
    char *cpt;

    strncpy (buf, Process_name, buf_size);
    buf[buf_size - 1] = '\0';
    cpt = buf + strlen (buf) - 1;
    while (cpt >= buf && (*cpt == ' ' || *cpt == ':')) {
	*cpt = '\0';
	cpt--;
    }
    return;
}

/********************************************************************
			
    Returns this process's "argv" and "argc". Returns 0 if not available.

********************************************************************/

int LE_get_argv (char ***argv_p) {

    *argv_p = Argv;
    return (Argc);
}

/********************************************************************
			
    Description: This function retrieves the process name from the 
		command line arguments, parses the LE_DIR_EVENT 
		environmental variable, opens the LE LB and retrieves 
		the host name. 

    Input:	argc - number of arguments.
		argv - the argument list.

    Return:	0 on success or a negative error number.

********************************************************************/

int LE_init (int argc, char **argv) {
    char *dir_ev;			/* the LE environmental variable */
    char prog_name[LE_NAME_LEN + 32];	/* name of this program */
    char *pt, *label;
    int ret;

    if (Lb_fd >= 0)		/* LE LB already open */
	return (0);

    /* get the process name */
    Argc = argc;
    Argv = argv;
    if (argc > 0 && argv != NULL) {
	strncpy (prog_name, MISC_string_basename (argv[0]), LE_NAME_LEN);
	prog_name [LE_NAME_LEN - 1] = '\0';
    }
    else
	prog_name[0] = '\0';
    strcpy (Process_name, prog_name);
    if (LE_get_instance () >= 0)
	sprintf (prog_name + strlen (prog_name), ".%d", LE_get_instance ());
    label = LE_get_label ();
    if (label == NULL)
	label = prog_name;
    if (label[0] == '\0')
	Prog_name_field[0] = '\0';
    else
	sprintf (Prog_name_field, "%s: ", label);
    Name_field_len = strlen (Prog_name_field);

    if (Set_foreground)
	return (0);

    /* get the environmental variable */
    dir_ev = getenv ("LE_DIR_EVENT");
    if (dir_ev == NULL)
	return (0);
    pt = strstr (dir_ev, ":");
    if (pt != NULL &&
	(sscanf (pt + 1, "%d", &Event_number) != 1 ||
	 Event_number <= 0))
	return (LE_ENV_DEF_ERROR);

    /* register VL change event */
    if (Event_number != 0) {
	int ret;
	MISC_log_disable (1);
	ret = EN_register (Event_number, Vl_change_callback);
	MISC_log_disable (0);
    }

    if (argc > 0 && argv != NULL)
	ret = LE_check_and_open_lb (argv[0], &Use_plain_file);
    else
	ret = LE_check_and_open_lb (NULL, &Use_plain_file);
    if (ret < 0)
	return (ret);
    Lb_fd = ret;

    MISC_log_reg_callback (Misc_log_cb);
    RSS_rpc_stdout_redirect (Rss_rpc_stdout_cb);

    return (0);
} 

/********************************************************************

    Prints stdout/stderr from RSS_rpc.

********************************************************************/

static void Rss_rpc_stdout_cb (char *msg) {

    LE_send_msg (0, "%ms", msg);
}

/********************************************************************

    Callback from MISC_log_reg_callback.

********************************************************************/

static void Misc_log_cb (char *msg) {

    LE_send_msg (0, msg + 18);	/* skip time stamp */
}

/********************************************************************
			
    Description: See LE manpage. This function sets the process
		verbose level to "new_vl". It returns the previous
		process verbose level.

    Input:	new_vl - the new verbose level.

    Return:	the previous process verbose level.

********************************************************************/

int LE_local_vl (int new_vl)
{
    int ovl;

    if (new_vl < 0)
	return (Current_VL);

    ovl = Current_VL;
    Current_VL = new_vl;	/* to prevent from disabling cirtical msgs 
				   Current_VL must always >= 0. */

/*    LE_send_msg(LE_VL0,"LE VL changed from %d to %d", ovl, Current_VL) ; */

    return (ovl);
}

/********************************************************************
			
    Description: This is the callback function upon receiving a
		VL change event. It sets the process VL if both
		host IP and pid match.

    Input:	evtcd - event number.
		buf - event msg.
		msglen - event msg length.

********************************************************************/

static void Vl_change_callback (EN_id_t evtcd, char *buf, int msglen, void *arg) 
{
    unsigned int ip, host_ip;
    int pid, new_vl;
    LE_VL_change_ev_t *msg;

    if (msglen != sizeof (LE_VL_change_ev_t)) {
	MISC_log ("LE: bad VL change event msg len (%d)\n", msglen);
	return;
    }

    msg = (LE_VL_change_ev_t *)buf;
    host_ip = ntohl(msg->host_ip);
    pid = ntohl (msg->pid);
    new_vl = ntohl (msg->new_vl);

    if (RMT_lookup_host_index (RMT_LHI_IX2I, &ip, 0) > 0 &&
	host_ip == ntohl (ip) && pid == getpid ()) {
	LE_local_vl (new_vl);
	return;
    }
    return;
}

/********************************************************************
			
    Description: See LE manpage. This function posts an event to direct
		the process "pid" on host "host_ip" to switch its
		process VL to "new_vl". All arguments are in local 
		byte order.

    Input:	host_ip - host IP address.
		pid - process id.
		new_vl - the new verbose level.

********************************************************************/

void LE_set_vl (unsigned int host_ip, int pid, int new_vl)
{
    LE_VL_change_ev_t msg;
    int ret;

    msg.host_ip = htonl (host_ip);
    msg.pid = htonl (pid);
    msg.new_vl = htonl (new_vl);

    ret = EN_post (Event_number, (void *)&msg, 
					sizeof (LE_VL_change_ev_t), 0);
    if (ret < 0)
	MISC_log ("LE: EN_post failed (ret %d)\n", ret);

    return;
}

/********************************************************************
			
    Description: See LE manpage. This function calls Send_msg_vargs
		to perform the work such that the latter can be called
		directly.

    Input:	code - an integer message code.
		format - a format string used for generating the 
			message. The format is used as the same way 
			as that used in printf.
		... - the variable list.

********************************************************************/

void LE_send_msg (int code, const char *format, ...)
{
    static int call_level = 0;
    va_list args;
    int mvl;

    if (Le_disabled && EN_control (EN_GET_IN_CALLBACK) == 0)
	return;

    /* check verbose level */
    mvl = LE_GET_MVL (code);
    if (mvl > Current_VL && !(code & LE_CRITICAL_BIT))
	return;

    if (format != NULL &&
	strcmp (format, "%ms") == 0 &&
	call_level == 0) {	/* process a multiple line string */
	char *str, *p, buf[LE_MAX_MSG_LENGTH];
	va_start (args, format);
	str = va_arg (args, char *);
	va_end (args);
	while (1) {
	    int len;
	    p = strstr (str, "\n");
	    if (p == NULL)
		p = str + strlen (str);
	    len = p - str;
	    if (len > 0) {
		if (len >= LE_MAX_MSG_LENGTH)
		    len = LE_MAX_MSG_LENGTH - 1;
		strncpy (buf, str, len);
		buf[len] = '\0';
		call_level++;
		LE_send_msg (code, "%s", buf);
		call_level--;
	    }
	    if (*p == '\0')
		break;
	    str = p + 1;
	}
	return;
    }

    if (In_callback && Process_saved_from_cb)
	return;

    va_start (args, format);
    Send_msg_vargs (code, format, args);
    va_end (args);
    return;
}

/********************************************************************

    Description: The second level implementation of LE_send_msg (See 
		LE manpage). Ths level implements the reentrant 
		mechanism. Note that there are two different cases a 
		msg needs to be saved: An LE_send_msg is interupted
		and LE_send_msg is called from LE callback. We use the 
		FROM_CB_BIT to mark the difference. In the later case
		the LE message from callback is saved. However, when
		this saved message is processed, we will not accept
		the LE_send_msg from its callback to avoid recursive
		calls.
			
    Input:	code - an integer message code.
		format - a format string used for generating the 
			message. The format is used as the same way 
			as that used in printf.
		args - the variable list, similiar to vprintf.

********************************************************************/

static void Send_msg_vargs (int code, const char *format, va_list args) {
    static int pid = 0;		/* pid of this process; 0 indicates that 
				   it is not defined */
    ALIGNED_t buf[ALIGNED_T_SIZE (sizeof (LE_critical_message) + 
						LE_MAX_MSG_LENGTH)];
				/* This must be on the stack for reentrant */
    char *text, *pt;

    Le_send_msg_called = 1;	/* set called flag */

    if (pid == 0)
	pid = getpid ();

    /* prepare the message structure */
    if (code & LE_CRITICAL_BIT) {
	LE_critical_message *msg;
	char tmp[LE_SOURCE_NAME_SIZE];

	/* process the source name */
	MISC_string_fit (tmp, LE_SOURCE_NAME_SIZE, 
		MISC_STRING_FIT_MIDDLE, '*', MISC_string_basename (Src_name));

	msg = (LE_critical_message *)buf;
	msg->code = code;
	msg->pid = pid;
	msg->n_reps = 0;
	msg->line_num = Line_num;
	strcpy (msg->fname, tmp);
	text = msg->text;
    }
    else {
	LE_message *msg;

	msg = (LE_message *)buf;
	msg->code = code;
	msg->n_reps = 0;
	text = msg->text;
    }

    strncpy (text, Prog_name_field, Name_field_len);
    pt = text + Name_field_len;
    if (format != NULL && *format != '\0') {
	vsnprintf (pt, LE_MAX_MSG_LENGTH - Name_field_len, format, args);
    }
    else
	pt[0] = '\0';

    if (!Inuse && Saved_msg_cnt > 0)	
	Process_saved_msgs ();	/* process saved msgs; This is needed to 
				   preserve message order */

    if (Inuse) {		/* The message has to be saved. */
	Block_signals (SIG_BLOCK);
	if (In_callback) {
	    int *n_reps;
	    if (code & LE_CRITICAL_BIT) 
		n_reps = &(((LE_critical_message *)buf)->n_reps);
	    else
		n_reps = &(((LE_message *)buf)->n_reps);
	    *n_reps = *n_reps | FROM_CB_BIT;
	}
	LE_save_message ((char *)buf);
	Saved_msg_cnt++;
	Block_signals (SIG_UNBLOCK);
	return;
    }
    else
	Inuse = 1;		/* start the critical session */

    Process_message ((char *)buf);

    Inuse = 0;			/* end of the critical session */

    if (Saved_msg_cnt > 0)	/* process saved msgs */
	Process_saved_msgs ();

    if (Terminate)
	Handle_termination ();

    return;
}

/********************************************************************
			
    Description: This function retrieves all saved messages and 
		processes them. 

********************************************************************/

static void Process_saved_msgs ()
{

    Block_signals (SIG_BLOCK);
    while (Saved_msg_cnt > 0) {
	int *n_reps;
	LE_message *msg = (LE_message *)LE_get_saved_msg ();
	if (msg->code & LE_CRITICAL_BIT) 
	    n_reps = &(((LE_critical_message *)msg)->n_reps);
	else
	    n_reps = &(msg->n_reps);
	if (*n_reps & FROM_CB_BIT)
	    Process_saved_from_cb = 1;
	*n_reps = *n_reps & (~FROM_CB_BIT);	/* reset the bit */
	Process_message ((char *)msg);
	Saved_msg_cnt--;
	Process_saved_from_cb = 0;
    }
    Block_signals (SIG_UNBLOCK);

    return;
}

/********************************************************************
			
    Description: This function processes the repeated messages and
		send the message for output. 

    Input:	buf - Pointer to the message structure.

********************************************************************/

static void Process_message (char *buf)
{
    static char prev_text [LE_MAX_MSG_LENGTH] = "";
				/* previous message text */
    static unsigned int prev_code;	/* previous message code */
    static int prev_time = 0;	/* the previous msg output time */
    static char prev_msg[sizeof (LE_critical_message)];
				/* previous unprinted message structure */
    static int n_not_printed = 0;
				/* count of not printed messages */
    static int print_cnt = 0;	/* print count of the repeated message */
    time_t cr_time, *ptime;
    int *pn_reps;
    char *text;
    unsigned int code;

    cr_time = MISC_systime (NULL);	/* For control purposes */

    code = *((unsigned int *)buf);
    if (code & LE_CRITICAL_BIT) {
	LE_critical_message *msg;

	msg = (LE_critical_message *)buf;
	ptime = &(msg->time);
	pn_reps = &(msg->n_reps);
	text = msg->text;
    }
    else {
	LE_message *msg;

	msg = (LE_message *)buf;
	ptime = &(msg->time);
	pn_reps = &(msg->n_reps);
	text = msg->text;
    }

    *ptime = time (NULL);		/* LE time label */

    if (code == prev_code &&
	strcmp (prev_text, text) == 0) {	/* a repeated msg */

	if (cr_time - prev_time > RE_TIME || print_cnt < N_FIRST_REP) {
	    *pn_reps = n_not_printed + 1;
	    Output_message (buf, cr_time);
	    n_not_printed = 0;
	    print_cnt++;
	    if (cr_time - prev_time > RE_TIME) {
		prev_time = cr_time;
		print_cnt = 0;
	    }
	}
	else {
	    if (code & LE_CRITICAL_BIT)
		memcpy (prev_msg, buf, sizeof (LE_critical_message));
	    else
		memcpy (prev_msg, buf, sizeof (LE_message));
	    n_not_printed++;
	}
    }
    else {
	if (n_not_printed > 0) {	/* message repetition stopped */
	    ALIGNED_t tmp [ALIGNED_T_SIZE (sizeof (LE_critical_message)
						 + LE_MAX_MSG_LENGTH)];

	    if (prev_code & LE_CRITICAL_BIT) {
		memcpy (tmp, prev_msg, sizeof (LE_critical_message));
		strcpy (((LE_critical_message *)tmp)->text, prev_text);
		((LE_critical_message *)tmp)->n_reps = n_not_printed;
	    }
	    else {
		memcpy (tmp, prev_msg, sizeof (LE_message));
		strcpy (((LE_message *)tmp)->text, prev_text);
		((LE_message *)tmp)->n_reps = n_not_printed;
	    }
	    Output_message ((char *)tmp, cr_time);
	    n_not_printed = 0;
	}

	strcpy (prev_text, text);
	prev_code = code;
	*pn_reps = 1;
	Output_message (buf, cr_time);
	prev_time = cr_time;
	n_not_printed = 0;
	print_cnt = 1;
    }

    return;
}

/********************************************************************
			
    Description: This function sends the message in "buf" to an
		appropriate output. It posts an event for critical
		messages.

    Input:	buf - the buffer holding the message.

********************************************************************/

static void Output_message (char *buf, time_t cr_time)
{
    static int prev_time = 0;		/* time of the previous call */
    static int call_cnt = 0;		/* number of calls in this period */
    int code, line_ret_added;
    LE_message *msg;
    int msg_len, ret;
    char *text, *p;

    msg = (LE_message *)buf;
    code = msg->code;

    /* process execution blocking if too many critical messages are received */
    if (code & LE_CRITICAL_BIT)
	call_cnt++;
    if (cr_time != prev_time) {
	prev_time = cr_time;
	call_cnt = 0;
    }

    if (call_cnt > N_NONBLOCKING_CALLS)
	msleep (BLOCKING_TIME);

    /* process time label and remove trailing line returns */
    text = Process_time_label (code, buf);
    p = text + strlen (text) - 1;
    while (p >= text && *p == '\n') {
	*p = '\0';
	p--;
    }
    line_ret_added = 0;

    if (code & LE_CRITICAL_BIT)
	msg_len = sizeof (LE_critical_message) + strlen (text);
    else
	msg_len = sizeof (LE_message) + strlen (text);

    if (Lb_fd < 0)		/* output to the stderr */
	Print_message (buf, code, 0);
    else if (Use_plain_file) {
	Print_message (buf, code, 1);
	if (Also_print_stderr)
	    Print_message (buf, code, 0);
    }
    else {				/* output to the LB */
	if (Also_print_stderr)
	    Print_message (buf, code, 0);

	p[1] = '\n';		/* add line return */
	p[2] = '\0';
	line_ret_added = 1;
	ret = LB_write (Lb_fd, buf, msg_len + 1, LB_NEXT);
	if (ret < 0) {
	    MISC_log ("LE: LB_write failed (ret %d) - we use stderr\n", ret);
	    p[1] = '\0';
	    Print_message (buf, code, 0);
	}
    }
    if (User_callback != NULL) {	/* calling user callback function */
	In_callback = 1;
	if (!line_ret_added) {	/* add line return */
	    p[1] = '\n';	
	    p[2] = '\0';
	}
	User_callback (buf, msg_len + 1);
	In_callback = 0;
    }

    return;
}

/********************************************************************
			
    If the LE text is generated from LE, we want to move the time
    in the text to ->time and remove the time lables from the text. This
    is necessary when the LE is used in remote functions.

********************************************************************/

static char *Process_time_label (int code, char *buf) {
    static int year = 0, month = 0, day = 0;
    time_t t;
    int y, mon, d, h, m, s;
    char *p, *p1, *text;

    if (code & LE_CRITICAL_BIT)
	p = ((LE_critical_message *)buf)->text;
    else
	p = ((LE_message *)buf)->text;
    p1 = p + Name_field_len;
    text = p1;
    if (text[2] == '/' && text[5] == '/' && 
		sscanf (text, "%d%*c%d%*c%d", &mon, &d, &y) == 3) {
	if (y < 40)
	    y += 2000;
	else
	    y += 1900;
	year = y;
	month = mon;
	day = d;
	text += 9;
    }
    if (text[2] == ':' && text[5] == ':' && 
		sscanf (text, "%d%*c%d%*c%d", &h, &m, &s) == 3) {
	t = 0;
	unix_time (&t, &year, &month, &day, &h, &m, &s);
	((LE_message *)buf)->time = t;
	text += 9;
    }
    if (text != p1)
	memmove (p1, text, strlen (text) + 1);
    return (p);
}

/********************************************************************
			
    Description: This function prints the message in "buf" to the
		stderr port. 

    Input:	buf - the buffer holding message.
		code - the message code.

********************************************************************/

static void Print_message (char *buf, int code, int to_file)
{
    char text_buf[TIME_FIELD_SIZE * 2 + sizeof (Prog_name_field) + 
			LE_MAX_MSG_LENGTH + REP_FIELD_SIZE + 
			LE_SOURCE_NAME_SIZE + 32];
    char *text, *pt;
    int n_reps, st_pos;
    time_t msg_time;
    FILE *fh;

    if (code & LE_CRITICAL_BIT) {
	LE_critical_message *msg = (LE_critical_message *)buf;
	text = msg->text;
	n_reps = msg->n_reps;
	msg_time = msg->time;
    }
    else {
	LE_message *msg = (LE_message *)buf;
	text = msg->text;
	n_reps = msg->n_reps;
	msg_time = msg->time;
    }

    pt = text_buf;
    {				/* print message time */
	static int last_days = 0;
				/* The last day count */
	int days, secs, mon, dd, yy, hh, min, ss;

	days = msg_time / 86400;	/* 86400: seconds in a day */
	secs = msg_time % 86400;
	if (days != last_days) {
	    unix_time (&msg_time, &yy, &mon, &dd, &hh, &min, &ss);
	    sprintf (pt, "%.2d/%.2d/%.2d ", mon, dd, (yy - 1900) % 100);
	    pt += TIME_FIELD_SIZE;
	    last_days = days;
	}
	sprintf (pt, "%.2d:%.2d:%.2d ", 
				secs / 3600, (secs / 60) % 60, secs % 60);
	pt += TIME_FIELD_SIZE;
    }

    strcpy (pt, text);
    pt += strlen (text);

    if (n_reps > 1)
        sprintf (pt, " (%d repeats)", n_reps);

    /* When writing to the plain log file, to allow lem to work, we must write
       the first char after writing the message (keeping the previous line
       of \n until the new message is completely written). */
    st_pos = -1;
    pt = text_buf;
    if (to_file) {
	fh = LE_get_plain_file_fh ();
	if (fh != NULL) {	/* to write to the plain log file */
	    pt++;	/* we write the first char after writing the message */
	    st_pos = ftell (fh);
	}
    }
    else
	fh = Out_port;
    if (fh == NULL)
	fh = stderr;
    if ((code & LE_CRITICAL_BIT) && 
	(Print_src_mask == 0 || (code & Print_src_mask) != Print_src_mask)) {
	fprintf (fh, "%s - %s:%d\n", pt, 
			((LE_critical_message *)buf)->fname, 
			((LE_critical_message *)buf)->line_num);
    }
    else
	fprintf (fh, "%s\n", pt);
    if (st_pos >= 0) {	/* finishing plain log file writing */
	int epos;
	fprintf (fh, "\n");
	fflush (fh);
	epos = ftell (fh);
	fseek (fh, st_pos - 1, SEEK_SET);
	fprintf (fh, "%c", text_buf[0]);
	fseek (fh, epos, SEEK_SET);
	fflush (fh);
	LE_file_lock (0);
    }
    else
	fflush (fh);

    return;
}

/********************************************************************
			
    Description: This function blocks/unblocks a set of functions. 

    Input:	how - function selection: SIG_BLOCK or SIG_UNBLOCK.

    Return:	0 on success or -1 on failure.

********************************************************************/

static int Block_signals (int how)
{
    static int init = 0;
    static sigset_t set;

    if (init == 0) {
	if (sigemptyset (&set) < 0 ||
	    sigaddset (&set, SIGTERM) < 0 ||
	    sigaddset (&set, SIGINT) < 0 ||
	    sigaddset (&set, SIGIO) < 0 ||
	    sigaddset (&set, SIGUSR1) < 0 ||
	    sigaddset (&set, SIGUSR2) < 0 ||
	    sigaddset (&set, SIGALRM) < 0) {
	    MISC_log ("sigaddset failed (errno = %d)\n", errno);
	    return (-1);
	}
	init = 1;
    }

    if (sigprocmask (how, &set, NULL) < 0) {
	MISC_log ("sigprocmask failed (errno = %d)\n", errno);
	return (-1);
    }

    return (0);
}

/********************************************************************
			
    Description: See LE manpage. This function genrates the text 
		message in term of "format". It is expected to be
		used by LE message macros.

    Input:	format - a format string used for generating the 
			message. The format is used as the same way 
			as that used in printf.
		... - the variable list.

    Return:	pointer to the message.

********************************************************************/

char *LE_gen_text (const char *format, ...)
{
    static char *text = NULL;
    va_list args;

    if (text == NULL) {
	text = (char *)malloc (LE_MAX_MSG_LENGTH);
	if (text == NULL) {
	    MISC_log ("LE: malloc failed");
	    return ("");
	}
    }

    va_start (args, format);
    vsnprintf (text, LE_MAX_MSG_LENGTH, format, args);
    va_end (args);
    return (text);
}

