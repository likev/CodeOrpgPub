/****************************************************************
		
	File: rmtd.c	
				
	2/16/94
	Purpose: The main module for the RMT server.
	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:35 $
 * $Id: rmt_server.c,v 1.44 2012/07/27 19:33:35 jing Exp $
 * $Revision: 1.44 $
 * $State: Exp $
 */  

/* 
 * System include files
 */
#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <dirent.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#ifdef LINUX
/* typedef char * caddr_t ; */
/* #include <iovec.h> */
#include <sys/uio.h> /* if not iovec.h */
#endif

#if (defined (SUNOS) && defined (THREADED))
#include <sys/socket.h>
#endif

#ifdef SUNOS
#define _XPG4_2
#define CMSG_SPACE(len) (_CMSG_DATA_ALIGN (len) \
			 + _CMSG_HDR_ALIGN (sizeof (struct cmsghdr)))
#define CMSG_LEN(len)   (_CMSG_HDR_ALIGN (sizeof (struct cmsghdr)) + (len))
#endif
#include <sys/socket.h>

#include <misc.h>
#include <rmt.h>
#include <rmt_def.h>
#include <net.h>
#include <en.h>

/* Definitions / macros / types */

#define ARG_SIZE_IN_REQ (REQ_MSG_SIZE - HEAD_SIZE)   /* The argument space size in 
							the request */
#define DATA_SIZE_IN_MSG (RET_MSG_SIZE - HEAD_SIZE)  /* The data space size in the 
							return message */

enum {MC_POLL, MC_DELETE, MC_INIT};	/* for arg "func" of Manage_clients */
#define MAX_N_CLIENTS_A_CHILD 128	/* used by Manage_clients */

typedef struct {		/* structure for per-thread static vars */
    int buf_size;		/* size of the buffer used in 
				   Process_request */
    char *buffer;		/* pointer to a buffer used in 
				   Process_request */
    char **user_buf;		/* user allocated buffer that needs to
				   be freed by the server */
} Sv_per_thread_data_t;

/* Local references / local variables */

static char Conf_name[NAME_SIZE];  	/* configuration file name */
static char Log_name[NAME_SIZE];  	/* log file name */
static FILE *RMT_logfl = NULL;		/* log file handler */
static char Prog_name[NAME_SIZE];	/* The name of the executable file */
static int N_log_msgs = 1000;		/* max number of log messages */
static int Msg_cnt = 0;
static int N_child;			/* max number of children allowed */
static unsigned int Cl_addr = 0;	/* client address */

static int Rpc_pfd;                    	/* RPC parent socket file descriptor */
static int Run_in_background;		/* running in background */
static int No_stdport_closing;
static int In_foreground = 1;		/* The server is still in foreground */
int RMT_reread_config = 0;
static int Conf_update_count = 0;	/* configuration update count */

static void (*Set_up_user_functions) (int (**user_func) (int,char *,char **));


static int Get_options (int argc, char **argv, 
				char *Prog_name, int *max_nfds);
static void Termination_exit (int sig);
static void Sigcld_int (int);
static int Process_request (int, char *);
static int Process_control_request (int cfd, int len, char *msg);
static int Send_error_return (int cfd, int id, int err);
static void Goto_background (void);
static int Set_signal_action (int sig, void (*func) (int ));
static int Get_user_functions (void);
static int Open_log_file (void);
static void Write_log (char *msg);

static int Iamchild = RMT_FALSE;	/* I am the child process */
static int (*User_func[MAX_NUM_FUNCTIONS]) (int, char *, char **);
		/* The user functions */
static int Receive_pipe_msg_from_parent (int sockfd);
static int Send_pipe_msg_to_child (int sockfd, int fd);

#ifdef THREADED
#include <pthread.h>
static pthread_t newThread;
static pthread_attr_t pthread_custom_attr;
static int Num_threads = 0;
static pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t Ptd_key;
static pthread_once_t Key_init_once = {PTHREAD_ONCE_INIT};
static int Msg_buf_key_init_failed = 0;

static void Key_init_func ();
static void Free_ptd (void *arg);
static void Child_exit (int);
static void *Process_child (void *fd);
#else
static int Manage_clients (int func, int ifd, void **buf);
static int Process_child (int fd);
#endif

static Sv_per_thread_data_t *Get_sv_per_thread_data (void);


/****************************************************************
                        
    Returns Run_in_background.
*/

int RMT_is_in_backgroud () {
    return (Run_in_background);
}

/********************************************************************

    Description: This function returns the pointer to the per thread 
		data structure. This function will retry until success
		if memory is not available.

    Return:	The pointer.

*********************************************************************/

static Sv_per_thread_data_t *Get_sv_per_thread_data (void)
{

#ifndef THREADED
    static Sv_per_thread_data_t ptd = {0, NULL, NULL};
    return (&ptd);
#else
    Sv_per_thread_data_t *ptd;

    pthread_once (&Key_init_once, Key_init_func);
    if (Msg_buf_key_init_failed)
	return (NULL);
    ptd = (Sv_per_thread_data_t *)pthread_getspecific (Ptd_key);
    if (ptd == NULL) {
	while ((ptd = (Sv_per_thread_data_t *)malloc 
				(sizeof (Sv_per_thread_data_t))) == NULL)
	    msleep (200);
	ptd->buf_size = 0;
	ptd->buffer = NULL;
	ptd->user_buf = NULL;
	pthread_setspecific (Ptd_key, (void *)ptd);
    }
    return (ptd);
#endif
}

/****************************************************************
                        
        This initializes the RMT server. This routine first gets
        the executable file name for messages. It then interprets the
        command line options and sets up the defaults. After performing 
        some initialization tasks, it opens a server socket and turns 
        itself into a daemon program. Finally it catches signals

        Returns: This function returns 0.
*/

int
  RMTD_server_init
  (
      int argc,                 /* Number of command arguments */
      char **argv,              /* UNIX command arguments */
      char **envp,              /* UNIX command environment */
      void (*set_up_user_functions) (int (**user_func) (int,char *,char **))
) {
    char *dir = NULL;
    int where; 
    int max_nfds, n_clients;
    int ret;

    MISC_malloc_retry (1);

    MISC_log_reg_callback (Write_log);	/* set log function */

    Set_up_user_functions = set_up_user_functions;

    /* initialize Prog_name, the executable file name */
    where = strlen (argv[0]) - 1;
    while (where >= 0 && argv[0][where] != '/') where--;
    strncpy (Prog_name, argv[0] + where + 1, NAME_SIZE);
    Prog_name [NAME_SIZE - 1] = '\0';

    /* The default values and options */
    Conf_name[0] = '\0';
    Log_name[0] = '\0';
    Run_in_background = 1;
    max_nfds = 0;
    if (Get_options (argc, argv, Prog_name, &max_nfds) == FAILURE)
        exit (1);
    if ((dir = getenv ("HOME")) == NULL || 
             strlen (dir) > NAME_SIZE - 32) {   /* reserve 32 chars for internal names */
            MISC_log ("Environmental variable HOME is not defined\n");
            exit (1);
    }
    
    if (Conf_name[0] == '\0')
        sprintf (Conf_name, "%s/.%s.conf", dir, Prog_name);     
    if (Log_name[0] == '\0') {
	char buf[64];
	unsigned int rmtport_ip;
	rmtport_ip = PNUM_get_local_ip_from_rmtport ();
	if (rmtport_ip == INADDR_NONE)
             sprintf (Log_name, "%s/%s.log", dir, Prog_name);
	else
             sprintf (Log_name, "%s/.rssd/%s/%s.log",
			dir, NET_string_IP (rmtport_ip, 1, buf), Prog_name);
    }

    /* Open the log message file */
    if (Run_in_background == 1) {
        if (Open_log_file () == FAILURE) exit (1);
    }

    /* gets/sets max number of fds */
    if ((ret = MISC_rsrc_nofile (max_nfds)) < max_nfds) {
        MISC_log ("MISC_rsrc_nofile failed (ret %d < req %d)", ret, max_nfds);
        exit (1);
    }
    max_nfds = ret;

    /* read configuration file */
    if ((n_clients = GCLD_initialize_host_table (Conf_name)) <= 0)
        exit (1);
    MSGD_set_max_n_local_senders (max_nfds - n_clients - N_child - 64);

    /* Retrieve the user defined functions */
    if (Get_user_functions () == FAILURE)
        exit (1);

    MISC_log ("Max number of fds set to %d", max_nfds);

    return (0);
}

/****************************************************************
			
	RMTD_server_main()		Date: 2/16/94

	The main loop of the RMT server. In the loop, the process 
        waits fora client's connection request. If a request is 
        received, it forks a child process to serve the client and 
        continues to wait for another connection request.

        The child process waits on the socket for a client request. If
        a request is received, it calls Process_request() to process the
        request. If a communication error is encountered in reading the 
        request or sending back the return message, the child process 
        exits.

        Returns: This function never returns.

*/

void
  RMTD_server_main
  (
) {
    int port_number, sig;            /* parent port number */

    /* Get a port number for the server */
    port_number = PNUM_get_port_number ();
    if (port_number == FAILURE) {
        MISC_log ("Can not find a port number\n");
        exit (1);
    }

    /* initialize client registration module */
    if (CLRG_initialize (N_child) == FAILURE)
        exit (1);

    /* open the message server sockets */
    if (MSGD_open_msg_server (port_number) == FAILURE)
        exit (2);

    /* open the RPC server socket */
    if ((Rpc_pfd = SOCD_open_server (port_number)) == FAILURE) {
        MISC_log ("Opening server failed\n");
        exit (2);
    }

    MISC_log ("Max number of children set to %d", N_child);
    MISC_log ("The port number is %d", port_number);

    /* go to background */
    if (Run_in_background)
        Goto_background ();
  
    RMT_access_disc_file (1, " \n", 3);

    /* Catch SIGCLD for calling wait to remove dead child */
    if (Set_signal_action (SIGCLD, Sigcld_int) == FAILURE)
        exit (1);

    for (sig = 1; sig <= 32; sig++) {	/* catch other signals */
	if (sig == SIGCLD || sig == SIGKILL || sig == SIGSTOP)
	    continue;
	if (sig == SIGPIPE)
	    Set_signal_action (sig, SIG_IGN);
	else
	    Set_signal_action (sig, Termination_exit);
    }

    /* write the PID to the file */
    MISC_log ("PID: %d", (int) getpid());

    MISC_log ("%s starts operation\n", Prog_name);
    while (1) {		/* The main loop. It never ends. */
	int sfd[2];     /* socket fd pairs created by the stream pipe */

	if (Iamchild == RMT_TRUE) {	/* The child */

#ifdef THREADED
	    int rtn;
	    if ((rtn = pthread_attr_init (&pthread_custom_attr)) != 0) {
                MISC_log ("pthread_attr_init failed %d:%s\n", 
						rtn, strerror (rtn));
                exit(0);
            }
	    if (( rtn = pthread_attr_setdetachstate (&pthread_custom_attr,
                                       PTHREAD_CREATE_DETACHED)) != 0) {
                MISC_log ("pthread_attr_setdetachstate failed %d:%s\n", 
					rtn,strerror(rtn));
                exit(0);
            }
#else
	    void *buf;
	    Manage_clients (MC_INIT, 0, &buf);
#endif

	    while (1) {
		int fd;		/* client fd */
	
#ifdef THREADED
		{		/* waiting for a new fd from the parent */
		    fd_set readfds;
		    FD_ZERO (&readfds);
		    FD_SET (sfd[0], &readfds);
		    select (FD_SETSIZE, &readfds, NULL, NULL, NULL);
		}
		if ((fd = Receive_pipe_msg_from_parent (sfd[0])) < 0)
		    continue;
		pthread_mutex_lock (&countMutex);
		Num_threads++;
		pthread_mutex_unlock (&countMutex);
                pthread_create (&newThread, &pthread_custom_attr, 
						Process_child, (void *)fd);
#else
		fd = Manage_clients (MC_POLL, sfd[0], &buf);
		if (fd >= 0) {
		    int ret = Process_child (fd);
		    if (ret < 0)
			Manage_clients (MC_DELETE, fd, &buf);
		}
#endif
	    }
	}
	else {				/* parent */
	    int cpid;
	    int fd;		/* client fd */

	    sigrelse (SIGCHLD);
	    if (RMT_reread_config) {
		MISC_log ("Re-read configuration file %s\n", Conf_name);
		GCLD_initialize_host_table (Conf_name);
		MSGD_process_msgs (-1); 
		RMT_reread_config = 0;
		Conf_update_count++;
	    }
	    MSGD_process_msgs (Rpc_pfd); 
	    sighold (SIGCHLD);

	    /* accept new clients */
	    while (1) {
		int cl_type, cl_pid;
		unsigned int cl_addr;	/* LBO */
		int ret = 0;
		Client_regist newCl;

		if ((fd = GCLD_get_client (Rpc_pfd, &cl_type, 
					&cl_pid, &cl_addr)) == FAILURE)
		    break;

		if (cl_type == RMT_MULTIPLE_CLIENT && 
			(ret = CLRG_get_client_info (cl_pid, cl_addr)) > 0) {
		    Send_pipe_msg_to_child (ret, fd);
		    close (fd);			/* close client socket */
		}
		else {	/* no child for this client */

		    /* create a pipe to the child */
		    if (socketpair (AF_UNIX, SOCK_STREAM, 0, sfd) < 0) {
			MISC_log ("Pipe creation failed (errno = %d)", errno);
			close (fd);		/* close client socket */
			continue;
		    }
		    if (Send_pipe_msg_to_child (sfd[1], fd) == FAILURE) {
			close (fd);		/* close client socket */
		    	close (sfd[0]);		/* close pipe */
		    	close (sfd[1]);		/* close pipe */
			continue;
		    }

		    if (cl_type == RMT_SINGLE_CLIENT && cl_pid != 0)
			CLRG_term_client (cl_pid, cl_addr);

		    /* fork a child */
		    Cl_addr = cl_addr;
		    if ((cpid = fork ()) == 0) {
			int sig;
			for (sig = 1; sig <= 32; sig++) {
			    if (sig == SIGPIPE)
				Set_signal_action (sig, SIG_IGN);
			    else
				Set_signal_action (sig, SIG_DFL);
			}
		    	close (Rpc_pfd); 	/* Close parent socket fd */
			MSGD_close_msg_clients ();	/* close msg fds */
			RMTSM_close_msg_hosts ();	/* close msg fds */
		    	CLRG_close_other_client_fd (sfd[0]);
			close (fd);		/* close client socket */
		    	close (sfd[1]);		/* close parent side pipe */
			RMT_access_disc_file (0, NULL, 0);
		    	Iamchild = RMT_TRUE;	/* we must set this after 
						   	stopping timer */
		    	break;
		    }
		    else if (cpid < 0) {
		    	close (fd);	      /* close client socket */
		    	close (sfd[0]);       /* close pipe */
		    	close (sfd[1]);	      /* close pipe */
		    	MISC_log ("fork failed (errno = %d)",errno);
		    }
		    else {
			/* register the new client */
			newCl.childPid = cpid;		/* child pid */
			newCl.addr = cl_addr;		/* client addr */
			newCl.clientPid = cl_pid;	/* client pid */
			newCl.pipeFd = sfd[1];		/* to-child pipe fd */
		    	if (CLRG_register_client (&newCl) != SUCCESS)
			    close (sfd[1]);
			close (sfd[0]);		/* close child side pipe */
		    	close (fd);		/* close client socket */
		    }
		}
	    }
	}
    }
}

/***********************************************************************

    Managing multiple clients served by this child rmt server.

    input:	func - function (MC_POLL, MC_DELETE or MC_INIT)
		ifd - pipe fd to perent rmt server for MC_POLL;
		      fd to be deleted for MC_DELETE.
		buf - internal buffer.

    Returns the fd number of the client that has request pending (MC_POLL).

***********************************************************************/

#ifndef THREADED
static int Manage_clients (int func, int ifd, void **buf) {
    struct mng_clients {
	int n_clients;
	int fd[MAX_N_CLIENTS_A_CHILD];
	struct pollfd pfds[MAX_N_CLIENTS_A_CHILD + 1];
    } *mngc;
    int cnt, i;

    mngc = (struct mng_clients *)(*buf);
    if (func == MC_POLL) {
	cnt = 0;
	for (i = 0; i < mngc->n_clients; i++) {
	    if (mngc->fd[i] >= 0) {
		mngc->pfds[cnt].fd = mngc->fd[i];
		mngc->pfds[cnt].events = POLL_IN_FLAGS;
		cnt++;
	    }
	}
	mngc->pfds[cnt].fd = ifd;
	mngc->pfds[cnt].events = POLL_IN_FLAGS;
	cnt++;
	while (poll (mngc->pfds, cnt, 1000) < 0) {
	    if (errno == EINTR || errno == EAGAIN)
		continue;
	    MISC_log ("poll failed (errno %d) - child exits\n", errno);
	    exit (0);
	}
	if (mngc->pfds[cnt - 1].revents & (POLL_IN_RFLAGS)) { /* new client */
	    int fd;
	    if ((fd = Receive_pipe_msg_from_parent (ifd)) >= 0) {
						/* register the new client */
		for (i = 0; i < mngc->n_clients; i++) {
		    if (mngc->fd[i] < 0) {
			mngc->fd[i] = fd;
			break;
		    }
		}
		if (i >= mngc->n_clients) {
		    if (mngc->n_clients >= MAX_N_CLIENTS_A_CHILD) {
			MISC_log ("Too many clients per child\n");
			close (fd);
		    }
		    else {
			mngc->fd[mngc->n_clients] = fd;
			mngc->n_clients++;
		    }
		}
#ifdef TEST_OPTIONS
		MISC_TO_add_fd (fd);
#endif
	    }
	}
	for (i = 0; i < cnt - 1; i++) {
	    if (mngc->pfds[i].revents & (POLL_IN_RFLAGS)) /* request ready */
		return (mngc->pfds[i].fd);
	}
    }
    else if (func == MC_DELETE) {
	cnt = 0;
	for (i = 0; i < mngc->n_clients; i++) {
	    if (mngc->fd[i] == ifd) {
		mngc->fd[i] = -1;
		close (ifd);
	    }
	    else if (mngc->fd[i] >= 0)
		cnt++;
	}
	if (cnt == 0) {		/* all clients are gone */
	    exit (0);
	}
    }
    else if (func == MC_INIT) {
	mngc = (struct mng_clients *)malloc (sizeof (struct mng_clients));
	if (mngc == NULL) {
	    MISC_log ("malloc failed - child exits\n");
	    exit(0);
	}
	mngc->n_clients = 0;
	*buf = (void *)mngc;
    }
    return (-1);
}
#endif

/******************************************************************
			
	Get_options()			Date: 2/21/94

	This function parses the command line options. The -h option,
	an incorrect option or an unexpected option causes the 
	function to print a usage message and return an error.
	If a user specified port number is specified, the function
	calls PNUM_set_port_number to set up the alternative port 
	number. If fast crash detection mode is specified, the function
	calls SOCD_set_fast_exit() to set the mode.

	Returns: This function returns SUCCESS on success and FAILURE 
	on failure. 
*/

static int
  Get_options
  (
      int argc,
      char **argv,
      char *Prog_name,		/* program name */
      int *max_nfds
) {
    int port_number;		/* user input port number */
    extern char *optarg;	/* used by getopt */
    int c;			/* used by getopt */
    int err;			/* error flag */

    err = 0;
    N_child = 64;
    No_stdport_closing = 0;
    while ((c = getopt (argc, argv, "c:p:l:n:f:hst")) != -1) {
	switch (c) {
	case 'c':
	    strncpy (Conf_name, optarg, NAME_SIZE);
	    if (Conf_name [0] == '\0')
		err = 1;
	    else
		Conf_name [NAME_SIZE - 1] = '\0';
	    break;
	case 'l':
	    strncpy (Log_name, optarg, NAME_SIZE);
	    if (Log_name [0] == '\0')
		err = 1;
	    else
		Log_name [NAME_SIZE - 1] = '\0';
	    break;
	case 'p':
	    if (sscanf (optarg, "%d", &port_number) != 1)
		err = 1;
	    else if (PNUM_set_port_number (port_number) == FAILURE) {
		MISC_log ("Setting port number (%d) failed\n", port_number);
		err = 1;
	    }
	    break;
	case 'n':
	    if (sscanf (optarg, "%d", &N_child) != 1 ||
		N_child <= 0)
		err = 1;
	    break;
	case 'f':
	    if (sscanf (optarg, "%d", max_nfds) != 1 ||
		*max_nfds <= 0)
		err = 1;
	    break;
	case 't':
	    Run_in_background = 0;
	    break;
	case 's':
	    No_stdport_closing = 1;
	    break;
	case 'h':
	    err = 1;
	    break;
	case '?':
	    err = 1;
	    break;
	}
    }
    if (err) {			/* Print usage message */
	printf ("Usage: %s\n", Prog_name);
	printf ("       -c config_file_name [default: $HOME/rmt/rmt.conf]\n");
	printf ("       -l log_file_name [default: $HOME/rmt/rmt.log]\n");
	printf ("       -p port_number [alternative RMT port number]\n");
	printf ("       -n N_child [max number of children; default = 64]\n");
	printf ("       -f max_nfds [max # of open fds; default = OS default]\n");
	printf ("       -s [closing no standard ports]\n");
	printf ("       -t [run in foreground]\n");
	return (FAILURE);
    }

    return (SUCCESS);
}


/******************************************************************

	Process_request()			Date: 2/16/94

	This is the server's basic function for processing a client
	request. It first parses the header information in the request and 
	checks if the the information is legal. If the request is a control
	request, the function responds to it by calling 
	Process_control_request. Otherwise, it checks if the required 
	user function is defined. If everything is OK, the function gets 
	the calling input byte string. 

	To avoid frequent memory allocations, the function allocates a 
	static buffer for the input byte string. If the input byte string is 
	too large for the static buffer, Process_request allocates a work 
	space for the input byte string. The allocated space is freed after 
	calling the user function. 

	Note that if the input byte string length is small, the input byte 
	string will be completely contained in the the request. Otherwise, 
	another socket read is needed to get the remaining part of the input 
	byte string.

	Then the appropriate user function is called. Finally the
	return message is generated and sent to the client. Note
	that if the return message is small only one SOCD_send_msg
	call is necessary.

	The RMT tool uses only the lower 24 bites of the argument "length"
	for the input byte string length. This is processed in this function.

	Error processing: This function returns SUCCESS on success. If a 
	communication error is detected, it returns FAILURE. In other 
	error situations, the function sends a return message, 
	indicating the error encountered, to the client. The error 
	indicator is one of the negative numbers as listed in the following:   

	RMT_BAD_ARG_SERVER_SIDE : function id number is out of range.
	RMT_USER_FUNC_UNDEFINED : The specified user function is not defined.
	RMT_BAD_USER_FUNC_RETN : The user function returned an illegal negative 
			     length.
	RMT_REQ_MSG_TOO_LARGE : The remote call failed in allocating work 
			    space. The message is probably too large or the
			    server's host lacks sufficient memory.

	If an error is encountered, a log messages is written to the 
	log file. Note that if this function returns FAILURE, the child 
	server will exit. 

*/

static int
  Process_request
  (
      int cfd,			/* The client socket fd */
      char *msg			/* The request message from the client */
) {
    int id;			/* The user function number (0 for testing
				   request) */
    int inp_len;		/* calling input byte string length including
				   info in highest 4 bites */
    char *input_string;		/* calling input byte string */
    char *output_string;	/* Output string from the user function */
    int output_len;		/* Size of the output byte string */
    ALIGNED_t ret_msg_buf[ALIGNED_T_SIZE (RET_MSG_SIZE)];
    char *ret_msg;
				/* Buffer for the return message */
    int len_copy;		/* The length of return data to be copied to
				   the return  message */
    int compressed;		/* RMT_COMPRESSION_ON if compression is on, 
				   RMT_COMPRESSION_OFF otherwise */				   
    int in_len;			/* true calling input byte string length */
    Sv_per_thread_data_t *ptd;
    int cancel_enabled;		/* PRC cancelation enabled */
    int ret;

    ret_msg = (char *)ret_msg_buf;

    /* parse the function id and the argument "length" */
    compressed = msg[1];   
    cancel_enabled = msg[2];
    id = msg[3];
    inp_len = ntohrmt(*(rmt_t *)(msg + 4));
    in_len = inp_len & LENGTH_MASK;
    if (id < 0 || id > MAX_NUM_FUNCTIONS)
	return (Send_error_return (cfd, id, RMT_BAD_ARG_SERVER_SIDE));

    /* if not control request, check user function id */
    if (id > 0 && User_func[id - 1] == NULL) {	/* function undefined */
	MISC_log ("Undefined function: id = %d", id);
	return (Send_error_return (cfd, id, RMT_USER_FUNC_UNDEFINED));
    }

    /* get the input byte string for the user function */
    input_string = msg + HEAD_SIZE;

    ptd = Get_sv_per_thread_data ();
    if (ptd == NULL)
	return (Send_error_return (cfd, id, RMT_PTHREAD_KEY_CREATE_FAILED));
    if (in_len > ARG_SIZE_IN_REQ) {	/* Need additional read */

	if (ptd->buf_size < in_len) {

	    if (ptd->buffer != NULL)
		free (ptd->buffer);
	    ptd->buf_size = in_len;
	    if (ptd->buf_size < STATIC_BUFFER)
		ptd->buf_size = STATIC_BUFFER;
	    ptd->buffer = (char *)malloc (ptd->buf_size);
	    if (ptd->buffer == NULL) {
		ptd->buf_size = 0;
		return (Send_error_return (cfd, id, RMT_REQ_MSG_TOO_LARGE));
	    }
	}

	memcpy (ptd->buffer, input_string, ARG_SIZE_IN_REQ);
	ret = SOCD_recv_msg (cfd, in_len - ARG_SIZE_IN_REQ,
			      	ptd->buffer + ARG_SIZE_IN_REQ, cancel_enabled);
	if (ret == FAILURE) {
	    MISC_log ("Failed in recv data: len = %d", in_len);
	    return (FAILURE);
	}
	else if (ret == RMT_FUNC_CANCELED)
	    return (SUCCESS);
	input_string = ptd->buffer;
    }

    /*  Decompress data if necessary */
    if (compressed == 3) {
        input_string = RMT_decompress (in_len, input_string, &in_len);
        if (input_string == NULL) {
           MISC_log ("Decompression failed\n");
           return (Send_error_return (cfd, id, RMT_DECOMPRESSION_FAILED));
	}
	inp_len &= (~LENGTH_MASK);
	inp_len |= (in_len & LENGTH_MASK);
    }
    
    /* process control request */
    if (id == 0)
	return (Process_control_request (cfd, in_len, input_string));

    /* call the appropriate user function */
	    
    output_len = User_func[id - 1] (inp_len, input_string, &output_string);

    /* Free ptd->buffer if it is too large */
    if (ptd->buf_size > STATIC_BUFFER) {
	free (ptd->buffer);
	ptd->buffer = NULL;
	ptd->buf_size = 0;
    }
    
    if (compressed == 3)
       RMT_free_buffer(input_string);

    if (output_len < MIN_USER_FUNC_RETN)   /* incorrect return */
	return (Send_error_return (cfd, id, RMT_BAD_USER_FUNC_RETN));

    /* send back the return message */
    if ((compressed & 1) && output_len >= 256) {
	int len;
	char *output = RMT_compress (output_len, output_string, &len);
	if (output == NULL) {
	    if (len < 0)
        	return (Send_error_return(cfd, id, RMT_COMPRESSION_FAILED));
	    compressed = 0;
	}
	else {			/* compressed */
	    compressed = 3;
	    output_string = output;
	    output_len = len;
	}
    }
    else
	compressed = 0;

    /* Generate the return message */
#ifdef USE_MEMORY_CHECKER
    memset (ret_msg, 0, RET_MSG_SIZE);
#endif
    ret_msg [0] = '*';
    ret_msg [1] = compressed;
    ret_msg [3] = id;
    *((rmt_t *)(ret_msg + 4)) = htonrmt (output_len);

    len_copy = output_len;
    if (len_copy > DATA_SIZE_IN_MSG)
	len_copy = DATA_SIZE_IN_MSG;
    if (len_copy > 0)
	memcpy (&ret_msg[HEAD_SIZE], output_string, len_copy);

    /* Send the return message */
    if ((ret = SOCD_send_msg (cfd, 
		RET_MSG_SIZE, ret_msg, cancel_enabled)) == FAILURE) {
	MISC_log ("Error sending ret msg");
	return (FAILURE);
    }

    /* Send the remaining part of the return value */
    if (ret == SUCCESS && output_len > len_copy) {
	if ((ret = SOCD_send_msg (cfd, output_len - len_copy,
		    output_string + len_copy, cancel_enabled)) == FAILURE) {
	    MISC_log ("Error sending ret data: len = %d", output_len);
	    return (FAILURE);
	}
    }

    /* free user allocated buffer */
    if (ptd->user_buf != NULL) {
	if (*(ptd->user_buf) != NULL) {
	    free (*(ptd->user_buf));
	    *(ptd->user_buf) = NULL;
	}
	ptd->user_buf = NULL;
    }
    
    if (compressed)
       RMT_free_buffer(output_string);

    return (SUCCESS);
}

/******************************************************************

	This function processes the control requests.

	Currently, there is no useful control message defined. This
	function simply sends an echo back.

	Returns: It returns SUCCESS on success and FAILURE if a 
	communication error is detected.
*/

static int
  Process_control_request
  (
      int cfd,			/* The socket fd */
      int len,			/* length of the control message */
      char *msg			/* The request message */
) {
    ALIGNED_t ret_msg_buf[ALIGNED_T_SIZE (RET_MSG_SIZE)];
    char *ret_msg;	/* return message buffer */
    char *tmp_buffer;	/* buffer for strtok_r*/
    int err, ret;

    ret_msg = (char *)ret_msg_buf;
    if (len <= 0) {
	err = RMT_BAD_CONTROL_MSG;
    }
    else {
	char *tok;

	tok = strtok_r (msg, " \n\t,", &tmp_buffer);
	if (tok != NULL) {
	    if (strcmp (tok, "NOOP") == 0)	/* no operation */
		err = 0;
	    else if (strcmp (tok, "TERMINATE") == 0) {
						/* terminating services */
		while ((tok = strtok_r (NULL, " \n\t,",&tmp_buffer)) != NULL)
		    CLRG_terminate (tok);
		err = 0;
	    }
	    else
		err = RMT_BAD_CONTROL_MSG;
	}
	else
	    err = RMT_BAD_CONTROL_MSG;
    }

    /* Send a return message */
    memset (ret_msg, 0, RET_MSG_SIZE);
    ret_msg [0] = '*';
    *((rmt_t *)(ret_msg + 4)) = htonrmt (err);
    if ((ret = SOCD_send_msg (cfd, RET_MSG_SIZE, ret_msg, 1)) == FAILURE) {
	MISC_log ("Error sending cntl ret msg");
	return (FAILURE);
    }

    return (SUCCESS);
}

/******************************************************************

	Send_error_return()		Date: 2/17/94

	In case the Process_request() failed to process a request,
	it sends a return message to the client indicating the
	error. This function sends the error return message.

	Returns: It returns SUCCESS if the error message is sent with
	success or FAILURE if a communication error is found.
*/

static int
  Send_error_return
  (
      int cfd,			/* The socket fd */
      int id,			/* The request id number */
      int err			/* The error number */
) {
    ALIGNED_t ret_msg_buf[ALIGNED_T_SIZE (RET_MSG_SIZE)];
    char *ret_msg;

    ret_msg = (char *)ret_msg_buf;
    memset (ret_msg, 0, RET_MSG_SIZE);
    ret_msg [0] = '*';
    ret_msg [3] = id;
    *((rmt_t *)(ret_msg + 4)) = htonrmt (err);
    if (SOCD_send_msg (cfd, RET_MSG_SIZE, ret_msg, 1) == FAILURE) {
	MISC_log ("Error sending error ret msg");
	return (FAILURE);
    }
    return (SUCCESS);
}

/******************************************************************

	Child_exit()		Date: 2/16/94

	This is the cleanup function before the child process's 
	termination.

	Returns: It does not return.
*/

#ifdef THREADED
static void Child_exit (
      int fd			/* The child socket fd */
) {

    if (fd >= 0)
	close (fd);
    pthread_mutex_lock (&countMutex);
    Num_threads--;
    pthread_mutex_unlock (&countMutex);
    if (Num_threads == 0){
/*	sleep (1); */
	exit(0);
    }
    pthread_exit(NULL);
}
#endif

/******************************************************************

	This function performs cleanup before exiting. It is called 
	when a termination signal is received. If the signal is SIGTERM
	the parent process will not terminate.

	Returns: This function does not return.
*/

static void
Termination_exit (int sig)
{
    char buf[128], sig_str[32];

    if (Iamchild != RMT_TRUE) {
	if (sig == SIGPIPE)
	    return;
	if (sig == SIGHUP) {
	    RMT_reread_config = 1;
	    return;
	}
	if (sig == SIGTERM || (Run_in_background && sig == SIGINT) ||
	    sig == SIGABRT || sig == SIGPWR || sig == SIGURG || 
	    sig == SIGPOLL || sig == SIGSTOP || sig == SIGCONT || 
	    sig == SIGALRM || sig == SIGUSR1 || sig == SIGUSR2) {
	    MISC_log ("Parent received signal (%d) - ignored", sig);
	    return;
	}
    }

    if (sig >= 0)
	sprintf (sig_str, ", sig %d", sig);
    else
	sig_str[0] = '\0';

    if (Iamchild == RMT_TRUE)
	MISC_log ("Child exit (pid %d, %s%s)", getpid (), NET_string_IP (Cl_addr, 0, buf), sig_str);
    else
	MISC_log ("Parent exit (pid %d%s)", getpid (), sig_str);

    exit (0);
}

/******************************************************************

	Sigcld_int()			Date: 2/16/94

	This function allows a child to die. It is called by the
	SIGCLD signal.

	Returns: There is no return value.
*/

static void
Sigcld_int (int sig)
{

    CLRG_sigchld ();
    return;
}

/******************************************************************

	Set_signal_action ()		Date: 2/16/94

	This function registers a signal call back function.

	Returns: It returns SUCCESS on success and FAILURE on failure.
*/

static int
  Set_signal_action
  (
      int sig,				/* The signal number */
      void (*func) (int )		/* The call back function */
) {

#if (defined(LINUX))
    struct sigaction act;

    memset (&act, 0, sizeof (struct sigaction));
    act.sa_handler = func;
    if (sig == SIGCHLD)
	act.sa_flags = SA_NOCLDSTOP;
    if (sigaction (sig, &act, NULL) < 0)
	return (FAILURE);
    else
	return (SUCCESS);
#elif (defined(SUNOS4))
    if (signal (sig, func) == SIG_ERR)
	return (FAILURE);
    else
	return (SUCCESS);
#else
    if (sigset (sig, func) == SIG_ERR)
	return (FAILURE);
    else
	return (SUCCESS);
#endif
}

/******************************************************************

	Goto_background ()		Date: 2/16/94

	This function puts the calling process into the background
	by forking a child and exiting. It closes the standard input
	standard output and standard error ports before forking the
	child. If it fails to fork a child, it will print an error 
	message and exit.

	Returns: The function has no return value.
*/

static void
Goto_background ()
{
    int fd;

    MISC_log ("Going to background\n");
    switch (fork ()) {
    case -1:			/* error in fork */
	MISC_log ("Failed in Goto_background - fork\n");
	exit (1);
    case 0:
	setsid ();

	Set_signal_action (SIGHUP, SIG_IGN);
	if (fork () != 0)
	    exit (0);

	if (!No_stdport_closing) {

	    /* we must close these to allow rsh to run rmtd in the background
	    close (0);		
	    close (1);
	    close (2);
	    make sure that the first 3 fds are occupied. Rsh may write to those
	    fds and generate errors. The user applications started by the rmtd
	    will not have standard io. However this problem can be fixed by 
	    using a shell (sh -c "exec application")
    
	    open ("/dev/null", O_RDONLY, 0);
	    open ("/dev/null", O_RDONLY, 0);
	    open ("/dev/null", O_RDONLY, 0);
	    */
	    if ((fd = open ("/dev/null", O_RDWR, 0)) < 0 ||
		dup2 (fd, STDIN_FILENO) != STDIN_FILENO ||
		dup2 (fd, STDOUT_FILENO) != STDOUT_FILENO ||
		dup2 (fd, STDERR_FILENO) != STDERR_FILENO) {
		MISC_log ("Failed in setting the STDIO ports\n");
		exit (1);
	    }
	    if (fd != STDIN_FILENO && fd != STDOUT_FILENO && 
						fd != STDERR_FILENO)
		close (fd);
	}
	In_foreground = 0;
#ifdef TEST_OPTIONS
	MISC_TO_add_fd (-1);
#endif
	return;			/* child */
    default:
	exit (0);
    }
}

/******************************************************************

	Get_user_functions ()		Date: 2/16/94

	This function initializes the array of User_func[]
	and retrieves the user defined functions by calling
	Set_up_user_functions(). It checks, then, how many 
	user functions are defined. If there is no user function 
	defined it assumes an error. If a user function uses two
	function numbers, an error is returned.

	Returns: The function returns SUCCESS on success and FAILURE 
	if there is no user function defined.
*/

static int
Get_user_functions ()
{
    int i, cnt;

    for (i = 0; i < MAX_NUM_FUNCTIONS; i++)
	User_func[i] = NULL;

    Set_up_user_functions (User_func);

    cnt = 0;
    for (i = 0; i < MAX_NUM_FUNCTIONS; i++) {
	int j;

	if (User_func[i] == NULL)
	    continue;
	cnt++;

	for (j = 0; j < i; j++) {  /* make sure there is no duplication */
	    if (User_func[i] == User_func[j]) {
		MISC_log ("Duplicated user function (%d %d)\n", i, j);
		return (FAILURE);
	    }
	}
    }

    if (cnt == 0) {
	MISC_log ("No user function is defined\n");
	return (FAILURE);
    }
    else
	MISC_log ("%d user function(s) defined\n", cnt);

    return (SUCCESS);
}


/******************************************************************

	Open_log_file ()			Date: 8/16/94

	This function opens the log file and writes in the file
	the first message indicating the rmt server is started.

	It returns SUCCESS on success or FAILURE on failure.
*/

static int
  Open_log_file () 
{
    char buf[256];

    /* Open the log file */
    MISC_mkdir (MISC_dirname (Log_name, buf, 256));
    if ((RMT_logfl = MISC_fopen (Log_name, "r+")) == NULL &&
	(RMT_logfl = MISC_fopen (Log_name, "w")) == NULL) {
	MISC_log ("Failed in opening log file %s\n", Log_name);
	return (FAILURE);
    }
    fseek (RMT_logfl, 0, 0);
    if (fileno (RMT_logfl) <= 2) {
	MISC_log ("This program started with STD ports closed\n");
	No_stdport_closing = 1;
    }
    Msg_cnt = 0;
    while (fgets (buf, 256, RMT_logfl) != NULL) {
	Msg_cnt++;
	if (buf[0] == '\n')
	    break;
    }

    return (SUCCESS);
}

/********************************************************************

    Writes log message "msg" received from MISC_log to the log file
    or on the stderr.

*********************************************************************/

static void Write_log (char *msg) {
    int slen;

    slen = strlen (msg);
    if (slen > 0 && msg[slen - 1] == '\n')
	msg[slen - 1] = '\0';
    if (RMT_logfl != NULL) {			/* running as a daemon */
	if (Msg_cnt > N_log_msgs) {
	    fseek (RMT_logfl, 0, 0);
	    Msg_cnt = 0;
	}
	if (Msg_cnt > 0)
	    fseek (RMT_logfl, -1, SEEK_CUR);
        fprintf (RMT_logfl, "%s\n\n", msg);
        fflush (RMT_logfl);  
	Msg_cnt++;
    }
    if (In_foreground)
	fprintf (stderr, "%s\n", msg);

    return;
}

/********************************************************************

    Old rssd log function. The new one shoud be MISC_log.

*********************************************************************/

void RMT_send_log (char *msg, int beep) {
    MISC_log (msg);
}


/****************************************************************
			
	This function returns the configuration file name.

****************************************************************/

char *
  RMT_get_conf_file (int *update_cnt)
{

    if (update_cnt != NULL)
	*update_cnt = Conf_update_count;
    return (Conf_name);
}

/****************************************************************
			
	This function stores the pointer of a user allocated buffer
	that needs to be freed after the remote procedure call.

****************************************************************/

void
  RMT_free_user_buffer (char **buf)
{

    (Get_sv_per_thread_data ())->user_buf = buf;
    return;
}

/****************************************************************

    Processes client requests. This is the main function for the 
    child rmt server.

****************************************************************/

#ifdef THREADED
static void *Process_child (void *tmpFd) {
    ALIGNED_t buffer[ALIGNED_T_SIZE (REQ_MSG_SIZE)];	/* buffer space */
    int fd;

    fd = (int)tmpFd;	
    while (1) {
	int ret;

	ret = SOCD_recv_msg (fd, REQ_MSG_SIZE, (char *)buffer, 0);
	if (ret == FAILURE)
	    Child_exit (fd);
	if (ret == RMT_FUNC_CANCELED)
	    continue;
	if (Process_request (fd, (char *)buffer) == FAILURE)
	    Child_exit (fd);        
    }
}
#else
static int Process_child (int fd) {
    ALIGNED_t buffer[ALIGNED_T_SIZE (REQ_MSG_SIZE)];	/* buffer space */
    int ret;

    ret = SOCD_recv_msg (fd, REQ_MSG_SIZE, (char *)buffer, 0);
    if (ret == FAILURE)
	return (-1);
    if (ret == RMT_FUNC_CANCELED)
	return (0);
    if (Process_request (fd, (char *)buffer) == FAILURE)
	return (-1);
    return (0);
}
#endif

#if (defined (SUNOS) && defined (THREADED))

/****************************************************************

    A special version for THREADED SUNOS.

****************************************************************/

static int Send_pipe_msg_to_child (int sockfd, int fd) {
    struct iovec    iov[1];
    struct msghdr   msg;
    char x[1];

    x[0] = 87;
    iov[0].iov_base = x;           	/* dummy data to send */
    iov[0].iov_len  = 1;

    memset (&msg, 0, sizeof (struct msghdr));
    msg.msg_iov          = iov;
    msg.msg_iovlen       = 1;
    msg.msg_name         = (caddr_t) 0;
    msg.msg_accrights    = (caddr_t)&fd;	/* address of descriptor */
    msg.msg_accrightslen = sizeof (int);	/* pass 1 descriptor */

    if (sendmsg (sockfd, &msg, 0) < 0) {
	MISC_log ("Error in sendmsg, errno %d\n", errno);
	return (FAILURE);
    }
    return (SUCCESS);
}

#else

/****************************************************************

    Passes the "fd" over a pipe "sockfd" to the child server.

****************************************************************/

static int Send_pipe_msg_to_child (int sockfd, int fd) {
    struct iovec iov[1];
    struct msghdr msg;
    char x[1];
    union {
	struct cmsghdr cm;
	char control[CMSG_SPACE (sizeof (int))];
    } control_un;
    struct cmsghdr *cmptr;

    memset (&msg, 0, sizeof (struct msghdr));
    memset (&control_un, 0, sizeof (control_un));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof (control_un.control);

    cmptr = CMSG_FIRSTHDR (&msg);
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    cmptr->cmsg_len = CMSG_LEN (sizeof (int));
    *((int *)CMSG_DATA (cmptr)) = fd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    x[0] = 87;
    iov[0].iov_base = x;           	/* dummy data to send */
    iov[0].iov_len  = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if (sendmsg (sockfd, &msg, 0) < 0) {
	MISC_log ("Error in sendmsg (fd-to-child), errno %d\n", errno);
	return (FAILURE);
    }
    return (SUCCESS);
}

#endif

#if (defined (SUNOS) && defined (THREADED))

/****************************************************************

    A special version for THREADED SUNOS.

****************************************************************/

static int Receive_pipe_msg_from_parent (int sockfd) {
    struct iovec    iov[1];
    struct msghdr   msg;
    fd_set readfds;
    int clientFd, ret;
    char x[1];

    memset (&msg,0,sizeof(msg));
    memset (&iov,0,sizeof(iov));

    iov[0].iov_base = x;           /* dummy data to receive */
    iov[0].iov_len  = 1;
    msg.msg_iov          = iov;
    msg.msg_iovlen       = 1;
    msg.msg_name         = (caddr_t) 0;
    msg.msg_accrights    = (caddr_t)&clientFd;   /* address of descriptor */
    msg.msg_accrightslen = sizeof (int);      /* receive 1 descriptor */
	
    FD_ZERO (&readfds);
    FD_SET (sockfd, &readfds);
    select (FD_SETSIZE, &readfds, NULL, NULL, NULL);

    while ((ret = recvmsg (sockfd, &msg, 0)) < 0){
	if (errno != EINTR) {
    	    MISC_log ("Error in recvmsg, errno %d\n", errno);
	    return (FAILURE);
	}
    }
    if (ret == 0) {
	MISC_log ("rssd parent pipe closed.\n");
	Termination_exit (-1);
    }
    if (ret != 1 || x[0] != 87)
	return (FAILURE);
    return (clientFd);
}

#else

/****************************************************************

    Receives the "client fd" over pipe "sockfd". Returns -1 on 
    failure.

****************************************************************/

static int Receive_pipe_msg_from_parent (int sockfd) {
    struct iovec iov[1];
    struct msghdr msg;
    union {
	struct cmsghdr cm;
	char control[CMSG_SPACE (sizeof (int))];
    } control_un;
    struct cmsghdr *cmptr;
    int clientFd, ret;
    char x[1];

    memset (&msg, 0, sizeof (struct msghdr)); 
    memset (&control_un, 0, sizeof (control_un));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof (control_un.control);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = x;
    iov[0].iov_len  = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    while ((ret = recvmsg (sockfd, &msg, 0)) < 0) {
	if (errno != EINTR) {
    	    MISC_log ("Error in recvmsg, errno %d\n", errno);
	    Termination_exit (-1);
	}
    }
    if (ret == 0) {
	MISC_log ("rssd parent pipe closed.");
	Termination_exit (-1);
    }

    clientFd = -1;
    if (ret == 1 &&
	x[0] == 87 &&
	(cmptr = CMSG_FIRSTHDR (&msg)) != NULL &&
	cmptr->cmsg_len == CMSG_LEN (sizeof (int)) &&
	cmptr->cmsg_level == SOL_SOCKET &&
	cmptr->cmsg_type == SCM_RIGHTS)
	clientFd = *((int *)CMSG_DATA (cmptr));

    return (clientFd);
}

#endif

#ifdef THREADED

/********************************************************************

    Description: This function initializes thread specific data key 
		for the per thread data structure.

*********************************************************************/

static void Key_init_func (void)
{
    int status;

    status = 0;
    while ((status = pthread_key_create (&Ptd_key, Free_ptd)) != 0) {
	if (status == ENOMEM) {
	    msleep (200);
	    continue;
	}
	Msg_buf_key_init_failed = 1;
    	MISC_log ("pthread_key_create failed (ret %d)\n", status);
	return;
    }
}

/********************************************************************

    Description: This function frees thread specific data key for 
		the per thread data structure.

    Input:	arg - pointer to the data.

*********************************************************************/

static void Free_ptd (void *arg)
{
    Sv_per_thread_data_t *ptd = (Sv_per_thread_data_t *)arg;
    if (ptd->buffer != NULL) {
	free (ptd->buffer);
	ptd->buffer = NULL;
    }
    free (ptd);
}

#endif

