
/*****************************************************************

    Description: The LB replication tool. lb_rep does not terminate
    if an LB involved does not exist or not accessible. It retries
    indefinitely.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/03/03 21:07:28 $
 * $Id: lb_rep.c,v 1.56 2008/03/03 21:07:28 jing Exp $
 * $Revision: 1.56 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <infr.h>

static int Verbose = 0;			/* verbose mode */
static char *Byteswap_func = NULL;	/* byte swap function name */
static char *Le_label = NULL;		/* LE msg label */

#define MAX_REPS 	128		/* maximum number of replication pathes
					   processed */
#define POLL_SECONDS	6
#define POLL_WAIT_MS	200

typedef struct {			/* A replication path */
    char *src_name;			/* name of the source LB */
    char *dest_name;			/* name of the destination LB */
    unsigned int stype;			/* source LB type; The source and 
					   destination must be both LB_DB or
					   both message queue. */
    int src;				/* source LB file descriptor */
    int dest;				/* destination LB file descriptor */
    char *data_type;			/* byte swap data type - struct name */
    int n_msgs;				/* number of message when opened */
    char *ids;				/* list of msg IDs to be replicated */
} Lb_rep_pair;

static Lb_rep_pair Rep_list[MAX_REPS];  /* The list of all specified 
					   replication pathes */

static int N_reps = 0;			/* size of list Rep_list */
static int Max_static_msgs = 0; 	/* max number of messages for static 
					   replication */
static int Static_rep_only = 0;		/* perform static replication only  */
static int Poll_ms = 500;		/* milli-seconds between each poll
					   in dynamic replication */
static int Server_port = 0;		/* server (rssd) port number */
static int Comp_type = -1;		/* compression type (MISC_GZIP or 
					   MISC_BZIP2). -1: No compression. */
static int Un_received = 0;		/* any UN received */
static int Poll_set = 0;

/* local functions */
static int Read_options (int argc, char **argv);
static int Open_lbs ();
static int Copy_mq_msg (Lb_rep_pair *rep);
static int Copy_cmp_mq_msg (Lb_rep_pair *rep);
static void Sig_callback (int sig);
static void Set_byte_swap ();
static double Get_cr_time ();
static int Decompress (char *buf, int len, char **dcmp);
static int Write_lb (Lb_rep_pair *rep, char *msg, int len, LB_id_t id);
static void Static_rp ();
static void Static_db_rp (Lb_rep_pair *rep);
static void Un_cb (int fd, LB_id_t msg_id, int msg_len, void *args);
static void Replic_db_lbs ();


/****************************************************************

    The main function for lb_rep.

******************************************************************/

int main (int argc, char **argv) {
    double cr_t, l_open, l_read;

    /* get command line options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Le_label != NULL)
	LE_set_option ("label", Le_label);
    if (!(Le_label != NULL && strcmp (Le_label, "stderr") == 0))
	LE_init (argc, argv);

    sigset (SIGPIPE, Sig_callback);

    if (Server_port > 0 &&
	RMT_port_number (Server_port) < 0) {
	MISC_log ("RMT_port_number (%d) failed\n", Server_port);
	exit (1);
    }

    Open_lbs ();

    RMT_time_out (0x7ffffff);	/* never timed out */

    if (Byteswap_func != NULL)
	Set_byte_swap ();

    /* static replication */
    if (Max_static_msgs > 0) {
	Static_rp ();
	MISC_log ("Static replication done\n");
	if (Static_rep_only) {
	    MISC_log ("Static replication only - exit\n");
	    exit (0);
	}
    }

    /* dynamic replication */
    l_open = Get_cr_time ();
    l_read = l_open;
    while (1) {
	int i;
	l_read += Poll_ms;
	for (i = 0; i < N_reps; i++) {
	    if (!(Rep_list[i].stype & LB_DB))
	        Copy_mq_msg (Rep_list + i);  /* may not return if Poll_set */
	}
	while ((cr_t = Get_cr_time ()) < l_read) {
	    msleep (100);
	    Replic_db_lbs ();
	}
	if (Poll_set)
	    l_read = cr_t;
	Replic_db_lbs ();
	if (cr_t >= l_open + 10000) {		/* open every 10 seconds */
	    Open_lbs ();
	    l_open = cr_t;
	}
    }

    exit (0);
}

/************************************************************************

    Returns the current time in ms.

************************************************************************/

static double Get_cr_time () {
    int ms;
    time_t t;

    t = MISC_systime (&ms);
    return ((double)t * 1000. + ms);
}

/************************************************************************

    The SIGPIPE callback function.

************************************************************************/

static void Sig_callback (int sig) {
    MISC_log ("Signal SIGPIPE received and ignored\n"); 
}

/************************************************************************

    Gets the byte swap function and calls SMIA_set_smi_func.

************************************************************************/

static void Set_byte_swap () {
    SMI_info_t *(*get_info)(char *, void *) = NULL;
    char file[128], func[128];

    if (MISC_get_token (Byteswap_func, "S-", 0, file, 128) < 0 ||
	strlen (file) == 0) {
	MISC_log ("Bad -t option (%s)\n", Byteswap_func);
	exit (1);
    }
    if (MISC_get_token (Byteswap_func, "S-", 1, func, 128) < 0 ||
	strlen (func) == 0)
	strcpy (func, "SMI_get_info");

    get_info = (SMI_info_t *(*)(char *, void *))MISC_get_func (file, func, 0);
    if (get_info == NULL) {
	MISC_log ("MISC_get_func (func %s) failed\n", func);
	exit (1);
    }
    SMIA_set_smi_func (get_info);
}

/*******************************************************************

    Replicate existing messages.

*******************************************************************/

static void Static_rp () {
    int i, ret, n_seek;

    for (i = 0; i < N_reps; i++) {
	Lb_rep_pair *rep = Rep_list + i;
	if (rep->src < 0 || rep->dest < 0) {
	    MISC_log ("Static rep path (%s -> %s) not available\n", 
				rep->src_name, rep->dest_name);
	    continue;
	}
	if (rep->stype & LB_DB) {
	    Static_db_rp (rep);
	    continue;
	}
	if (Poll_set)
	    LB_set_poll (rep->src, 0, POLL_WAIT_MS);
	n_seek = Max_static_msgs;
	if (n_seek > rep->n_msgs)
	    n_seek = rep->n_msgs;
	ret = LB_seek (rep->src, -n_seek, LB_CURRENT, NULL);
	if (ret < 0) {
	    MISC_log ("LB_seek (%s) failed (%d)\n", rep->src_name, ret);
	    exit (1);
	}
	Copy_mq_msg (rep);
	if (Poll_set)
	    LB_set_poll (rep->src, POLL_SECONDS, POLL_WAIT_MS);
    }
}

/*******************************************************************

    Copy all messages from the source to the destination of "rep". This
    is the version for message queue. Returns the number of messages
    copied or a negative error code.

*******************************************************************/

static int Copy_mq_msg (Lb_rep_pair *rep) {
    char *msg;
    int len, cnt;

    if (rep->src < 0 || rep->dest < 0)
	return (-1);
    if (Comp_type >= 0 && (rep->src & 0xffff0000) && !(rep->dest & 0xffff0000))
	return (Copy_cmp_mq_msg (rep));

    cnt = 0;
    while (1) {
	while (1) {
	    len = LB_read (rep->src, (char *)&msg, LB_ALLOC_BUF, LB_NEXT);
	    if (len <= 0) {
		if (len == LB_EXPIRED) {
		    MISC_log ("LB_read msg expired\n");
		    LB_seek (rep->src, 0, LB_FIRST, NULL);
		    continue;
		}
		if (len == LB_TO_COME) {
		    return (cnt);
		}
		MISC_log ("LB_read failed (ret %d)\n", len);
		LB_close (rep->src);
		rep->src = -1;
		return (-1);
	    }
	    break;
	}
	if (Write_lb (rep, msg, len, LB_ANY) < 0) {
	    free (msg);
	    return (-1);
	}
	free (msg);
	cnt++;
    }
    return (cnt);
}

/*******************************************************************

    Writes "msg" of "len" bytes to the destination LB of "rep".
    Returns 0 on success or -1 on failure.

*******************************************************************/

static int Write_lb (Lb_rep_pair *rep, char *msg, int len, LB_id_t id) {
    int ret;

    if (rep->data_type != NULL && Byteswap_func != NULL) {
	ret = SMIA_bswap_input (rep->data_type, msg, len);
	if (ret < 0) {
	    MISC_log ("SMIA_bswap_input (%s) failed (ret %d)\n", 
					rep->data_type, ret);
	    return (-1);
	}
    }

    ret = LB_write (rep->dest, msg, len, id);
    if (ret < 0) {
	LB_close (rep->dest);
	rep->dest = -1;
	MISC_log ("LB_write failed (ret %d)\n", ret);
	return (-1);
    }
    return (0);
}

/*******************************************************************

    Copies, with packing and compression, all messages from the source
    to the destination of "rep". This is the version for message queue.
    Returns the number of messages copied or a negative error code.
    We don't use LB_ALLOC_BUF and manage read buffer for efficiency.

*******************************************************************/

#define N_MSGS 10

static int Copy_cmp_mq_msg (Lb_rep_pair *rep) {
    static char *buf = NULL;
    static int b_size = 0;
    int cnt, ret;

    if (b_size == 0) {
	b_size = 50000;
	buf = MISC_malloc (b_size);
    }
    cnt = 0;
    while (1) {
	char *dcmp, *p;

	ret = LB_read (rep->src, buf, b_size, 
			LB_MULTI_READ_FULL | LB_MR_COMP (Comp_type) | N_MSGS);
	if (ret == LB_EXPIRED) {
	    MISC_log ("LB_read message expired\n");
	    LB_seek (rep->src, 0, LB_FIRST, NULL);
	    continue;
	}
	else if (ret < 0) {
	    MISC_log ("LB_read failed (ret %d)\n", ret);
	    LB_close (rep->src);
	    rep->src = -1;
	    return (-1);
	}
	ret = Decompress (buf, ret, &dcmp);
	if (ret > 0) {
	    int *ip;
	    int t_size, n_msgs, i, s;

	    ip = (int *)dcmp;
	    n_msgs = INT_BSWAP_L (ip[0]);
	    t_size = 0;
	    for (i = 0; i < n_msgs; i++) {
		s = INT_BSWAP_L (ip[2 * i + 1]);
		p = dcmp + INT_BSWAP_L (ip[2 * i + 2]);
		if (Write_lb (rep, p, s, LB_ANY) < 0)
		    return (-1);
		cnt++;
		t_size += s;
	    }
	    if (n_msgs > 0) {		/* adjuct buffer based on msg sizes */
		int n_s = ((t_size / n_msgs + 1) + 8) * N_MSGS + 8;
		if (n_s > b_size) {
		    b_size = n_s;
		    free (buf);
		    buf = MISC_malloc (b_size);
		}
	    }
	    if (n_msgs < N_MSGS) {
		if (ip[2 * n_msgs + 1] == 0)	/* to come */
		    return (cnt);
		else if (n_msgs == 0) {		/* buffer too small */
		    if (b_size > 1024000) {
			MISC_log ("buffer size grows too large\n");
			exit (1);
		    }
		    b_size *= 2;
		    free (buf);
		    buf = MISC_malloc (b_size);
		}
	    }
	}
    }

    return (0);
}

/*******************************************************************

    Decompresses msg in "buf" of "len" bytes. The pointer to the 
    decompressed message is returned by "dcmp". Returns the size of 
    the decompressed message on success of -1 on failure.

*******************************************************************/

static int Decompress (char *buf, int len, char **dcmp) {
    static char *b = NULL;
    static int b_s = 0;
    int ol, ret;

    if (len <= 4) {
	MISC_log ("Decompress: unexpected data size %d\n", len);
	return (-1);
    }
    ol = INT_BSWAP_L (*((int *)buf));
    if (ol > 0) {
	if (ol > b_s) {
	    if (b != NULL)
		free (b);
	    b_s = ol;
	    b = MISC_malloc (b_s);
	}
	ret = MISC_decompress (Comp_type, 
				buf + sizeof (int), len - 4, b, b_s);
	if (ret != ol) {
	    MISC_log ("MISC_decompress failed (ret %d)\n", ret);
	    return (-1);
	}
	*dcmp = b;
	return (ret);
    }
    else {
	*dcmp = buf + sizeof (int);
	return (len - sizeof (int));
    }
}

/*******************************************************************

    Replicate all DB type LBs.

    Question? How do we deal with POOL type LB? The POOL type 
    messages are not possible to create by msg ID.

    UN is used for replication. If UN registration fails, replication
    path will not start.

*******************************************************************/

static void Replic_db_lbs () {
    int n;

    if (!Un_received)
	return;

    EN_control (EN_BLOCK);
    Un_received = 0;
    for (n = 0; n < N_reps; n++) {
	Lb_rep_pair *rep;
	int n_ids, i, ret;
	LB_id_t *ids;

	rep = Rep_list + n;
	n_ids = STR_size (rep->ids) / sizeof (LB_id_t);
	if (n_ids == 0)
	    continue;
    
	ids = (LB_id_t *)(rep->ids);
	for (i = 0; i < n_ids; i++) {
	    char *msg;
	    ret = LB_read (rep->src, &msg, LB_ALLOC_BUF, ids[i]);
	    if (ret == LB_NOT_FOUND) {
		LB_delete (rep->dest, ids[i]);
		continue;
	    }
	    if (ret < 0) {
		MISC_log ("LB_read (id %d) failed (ret %d)\n", ids[i], ret);
		continue;
	    }
	    Write_lb (rep, msg, ret, ids[i]);
	    if (ret > 0)
		free (msg);
	}
	rep->ids = STR_reset (rep->ids, 128);
    }
    EN_control (EN_UNBLOCK);

    return;
}

/*******************************************************************

    Copy all exising messages from the source to the destination of 
    "rep" - DB.
    
*******************************************************************/

static void Static_db_rp (Lb_rep_pair *rep) {
    LB_info *inlist = NULL;	/* LB info list */
    int nmsgs, ret, i;

    /* get message list */
    if (rep->n_msgs == 0)
	return;
    inlist = (LB_info *)MISC_malloc (rep->n_msgs * sizeof (LB_info));
    nmsgs = LB_list (rep->src, inlist, rep->n_msgs);
    if (nmsgs < 0) {
	MISC_log ("LB_list %s failed (ret %d)\n", rep->src_name, nmsgs);
	exit (1);
    }

    for (i = 0; i < nmsgs; i++) {
	char *msg;
	if (inlist[i].size < 0)
	    continue;
	ret = LB_read (rep->src, &msg, LB_ALLOC_BUF, inlist[i].id);
	if (ret < 0) {
	    MISC_log ("LB_read (id %d) failed (ret %d)\n", inlist[i].id, ret);
	    continue;
	}
	Write_lb (rep, msg, ret, inlist[i].id);
	if (ret > 0)
	    free (msg);
    }
    free (inlist);
}

/*******************************************************************

    Opens all LBs that needs to be opened. It returns 0 on success 
    or -1 on failure.

*******************************************************************/

static int Open_lbs () {
    int i;

    for (i = 0; i < N_reps; i++) {
	LB_status status;
	LB_attr attr;
	int ret;
	Lb_rep_pair *rep;

	rep = Rep_list + i;
	if (rep->src >= 0 && rep->dest >= 0)
	    continue;

	status.attr = &attr;
	status.n_check = 0;

	/* open source LB */
	if (rep->src < 0) {
	    rep->src = LB_open (rep->src_name, LB_READ, NULL);
	    if (rep->src < 0)
		MISC_log ("LB_open %s failed (ret %d)\n", 
				rep->src_name, rep->src);
	    else {
		ret = LB_stat (rep->src, &status);
		if (ret < 0) {
		    MISC_log ("lb_stat %s failed (ret %d)\n", 
					rep->src_name, ret);
		    LB_close (rep->src);
		    rep->src = -1;
		}
		else {
		    rep->stype = attr.types;
		    rep->n_msgs = status.n_msgs;
		    if (rep->stype & LB_DB) {
			rep->ids = STR_reset (rep->ids, 128);
			ret = LB_UN_register (rep->src, LB_ANY, Un_cb);
			if (ret < 0) {
			    MISC_log ("LB_UN_register %s failed (ret %d)\n", 
					rep->src_name, ret);
			    LB_close (rep->src);
			    rep->src = -1;
			}
		    }
		    else if (N_reps == 1) {
			Poll_set = 1;
			LB_set_poll (rep->src, POLL_SECONDS, POLL_WAIT_MS);
			MISC_log ("LB_set_poll to %d seconds\n", POLL_SECONDS);
		    }
		}
	    }
	}

	/* open destination LB */
	if (rep->dest < 0) {
	    rep->dest = LB_open (rep->dest_name, LB_WRITE, NULL);
	    if (rep->dest < 0)
		MISC_log ("LB_open %s failed (ret %d)\n", 
				    rep->dest_name, rep->dest);
	    else {
		ret = LB_stat (rep->dest, &status);
		if (ret < 0) {
		    MISC_log ("lb_stat %s failed (ret %d)\n", 
					    rep->dest_name, ret);
		    LB_close (rep->dest);
		    rep->dest = -1;
		}
		else if ((rep->stype & LB_DB) != 
				(attr.types & LB_DB)) {
		    MISC_log ("LBs %s and %s are of different types\n", 
				rep->src_name, rep->dest_name);
		    exit (1);
		}
	    }
	}
	if (rep->dest >= 0 && rep->src >= 0)
	    MISC_log ("Replication path (%s -> %s) established\n", 
			rep->src_name, rep->dest_name);
    }
    return (0);
}

/*******************************************************************

    Message update callback function for all DB LBs.

*******************************************************************/

static void Un_cb (int fd, LB_id_t msg_id, int msg_len, void *args) {
    int i;

    for (i = 0; i < N_reps; i++) {
	Lb_rep_pair *rep = Rep_list + i;
	if (rep->src == fd) {
	    rep->ids = STR_append (rep->ids, (char *)&msg_id, 
						sizeof (LB_id_t));
	    Un_received = 1;
	    break;
	}
    }
}

/*******************************************************************

    Description: This function interprets command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

*******************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;      /* used by getopt */
    int c;            
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "n:r:c:p:t:sC:l:vh?")) != EOF) {
	switch (c) {
	    char tok[256], *sep;

	    case 'n':
		if (sscanf (optarg, "%d", &Max_static_msgs) != 1)
		    err = 1;
		break;

	    case 'p':
		if (sscanf (optarg, "%d", &Poll_ms) != 1)
		    err = 1;
		break;

	    case 'c':
		if (sscanf (optarg, "%d", &Server_port) != 1)
		    err = 1;
		break;

	    case 't':
		Byteswap_func = STR_copy (NULL, optarg);
		break;

	    case 'l':
		Le_label = STR_copy (NULL, optarg);
		break;

	    case 's':
		Static_rep_only = 1;
		break;

	    case 'r':
		sep = "S,";
		if (MISC_get_token (optarg, sep, 1, tok, 0) <= 0)
		    sep = "S-";
		if (MISC_get_token (optarg, sep, 1, tok, 256) > 0) {
		    Lb_rep_pair *rep = Rep_list + N_reps;
		    rep->dest_name = STR_copy (NULL, tok);
		    MISC_get_token (optarg, sep, 0, tok, 256);
		    rep->src_name = STR_copy (NULL, tok);
		    if (MISC_get_token (optarg, sep, 2, tok, 256) > 0)
			rep->data_type = STR_copy (NULL, tok);
		    else
			rep->data_type = NULL;
		    rep->src = rep->dest = -1;
		    rep->stype = 0;
		    rep->ids = NULL;
		    N_reps++;
		}
		else {
		    MISC_log ("bad argument in -r option (%s)\n", optarg);
		    err = 1;
		}
		break;

            case 'C':
		if (sscanf (optarg, "%d", &Comp_type) != 1)
		    err = 1;
                if (Comp_type != MISC_GZIP && Comp_type != MISC_BZIP2) {
                    MISC_log ("bad argument in -C option (%s)\n", optarg);
                    err = 1;
                }
                break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	    default:
		MISC_log ("Unexpected option (-%c)\n", c);
		err = 1;
		break;
	}
    }

    if (N_reps == 0 && err == 0) {
	MISC_log ("no replication path specified\n");
	exit (0);
    }

    if (Static_rep_only && Max_static_msgs == 0) {
	MISC_log ("Number of static messages to replicate not specified\n");
	exit (0);
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s options\n", argv[0]);
	printf ("       options: \n");
	printf ("       -r src_lb_name,dest_lb_name,type (Multiple -r options\n");
	printf ("          are fine. '-' can be used to replace ','. \"type\" is\n");
	printf ("          optional. It is the data type for byte swapping)\n");
	printf ("       -t lib_file:func_name (SMI lib name and function for byte\n");
	printf ("          swapping. func_name is optional with SMI_get_info assumed)\n");
	printf ("       -c server_port_number\n");
	printf ("       -n max_static_msg_number (maximum number of existing messages\n");
	printf ("          replicated. For DB, all existing messages are replicated\n");
	printf ("          if non-zero. The default is 0)\n");
	printf ("       -s (static, i.e. existing message, replication only)\n");
	printf ("       -p poll_ms (milli-seconds between each poll in dynamic\n");
	printf ("          replication. The default is 500 ms (.5 second))\n");
        printf ("       -C cmp_method (0 = gzip or 1 = bzip2. Multiple messages\n");
        printf ("          are read, packed and compressed on the reading side.\n");
        printf ("          Messages are then uncompressed, unpacked and written to the\n");
        printf ("          destination. Message queue LB only)\n"); 
	printf ("       -l LE_lable (sets LE message label. \"stderr\" directs to STDERR)\n");
	printf ("       -v (verbose mode)\n");
	return (-1);
    }

    return (0);
}
