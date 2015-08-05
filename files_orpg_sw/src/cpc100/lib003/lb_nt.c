
/*****************************************************************

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:54 $
 * $Id: lb_nt.c,v 1.24 2012/06/14 18:57:54 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <infr.h>

#define LB_NTF_NAME_SIZE 	256

#define MAX_N_NTFS	16

static int Post_an_defined;
static int Post_an;
static int Un_cnt;
static int An_cnt;
static EN_id_t Ans[MAX_N_NTFS];
static char Lb_name[MAX_N_NTFS][LB_NTF_NAME_SIZE];
static EN_id_t Lb_msgid[MAX_N_NTFS];
static char Post_msg[LB_NTF_NAME_SIZE];
static int Lb_fds[MAX_N_NTFS];
static int Print_len;
static int Nr_record;
static int Rssd_report;
static char Lb_nr_name[LB_NTF_NAME_SIZE];
static unsigned int Disc_ip;
static int Print_ascii = 0;
static char *Bin_msg = NULL;
static int Bm_size = 0;

static char Prog_name [LB_NTF_NAME_SIZE];



static void Print_usage ();
static int Read_options (int argc, char **argv);
static void An_callback (EN_id_t an, char *msg, int msg_len, void *arg);
static void Un_callback (int fd, EN_id_t msgid, int msg_len, void *arg);
static void Print_nr_records ();
static void Process_rssd_report ();
static void Print_rssd_report (EN_id_t event, 
				char *msg, int msg_len, void *arg);

/****************************************************************
*/

int main (int argc, char **argv)
{
    int i, ret;

    strncpy (Prog_name, argv[0], LB_NTF_NAME_SIZE);
    Prog_name [LB_NTF_NAME_SIZE - 1] = '\0';

    if (Read_options (argc, argv) < 0)
	exit (1);

    EN_control (EN_SET_ERROR_FUNC, printf);

    if (Rssd_report)
	Process_rssd_report ();

    if (Nr_record)
	Print_nr_records ();

    for (i = 0; i < Un_cnt; i++) {
	int fd;

	fd = LB_open (Lb_name[i], LB_READ, NULL);
	if (fd < 0) {
	    printf ("LB_open %s failed, ret %d\n", Lb_name[i], fd);
	    exit (1);
	}
	ret = LB_UN_register (fd, Lb_msgid[i], Un_callback);
	if (ret < 0) {
	    printf ("LB_UN_register %x failed, ret %d\n", 
							Lb_msgid[i], ret);
	    exit (1);
	}
	else
	    printf ("LB_UN_register %s,%x done\n", Lb_name[i], Lb_msgid[i]);
	Lb_fds[i] = fd;
    }

    for (i = 0; i < An_cnt; i++) {
	ret = EN_register (Ans[i], An_callback);
	if (ret < 0) {
	    printf ("EN_register %x failed, ret %d\n", Ans[i], ret);
	    exit (1);
	}
    }

    if (Post_an_defined) {
	if (Post_an == EN_DISC_HOST)
	    ret = EN_post_msgevent (Post_an, 
				(char *)&Disc_ip, sizeof (unsigned int));
	else if (strlen (Post_msg) > 0)
	    ret = EN_post_msgevent (Post_an, Post_msg, strlen (Post_msg) + 1);
	else if (Bm_size > 0)
	    ret = EN_post_msgevent (Post_an, Bin_msg, Bm_size);
	else
	    ret = EN_post_msgevent (Post_an, Post_msg, 0);
	if (ret < 0) {
	    printf ("EN_post_msgevent %x failed, ret %d\n", 
							Post_an, ret);
	    exit (1);
	}
    }

    if (An_cnt == 0 && Un_cnt == 0)
	exit (0);

    while (1) {
	sleep (100);
    }
}

/********************************************************************

    Description: Register a callback function for printing the local 
		rssd EN report messages.

********************************************************************/

static void Process_rssd_report ()
{
    int ret;

    ret = EN_register (EN_REPORT, Print_rssd_report);
    if (ret < 0) {
	printf ("EN_register EN_REPORT failed, ret %d\n", ret);
	exit (1);
    }
    printf ("waiting for rssd reporting ...\n");
    fflush (stdout);
    while (1) {
	sleep (100);
    }
}

/********************************************************************

    Description: Prints the local rssd EN report messages.

********************************************************************/

static void Print_rssd_report (EN_id_t event, char *msg, int msg_len, void *arg)
{

    if (event != EN_REPORT) {
	printf ("unexpected AN %d\n", event);
	fflush (stdout);
	return;
    }

    msg[msg_len - 1] = '\0';
    printf ("%s", msg);
    fflush (stdout);
}

/********************************************************************

    Description: Prints the notification request records in an LB.

********************************************************************/

static void Print_nr_records ()
{
    int fd;
    char *nr_msg;
    int len, ret;

    fd = LB_open (Lb_nr_name, LB_READ, NULL);
    if (fd < 0) {
	printf ("LB_open %s failed, ret %d\n", Lb_nr_name, fd);
	exit (1);
    }

    ret = LB_read (fd, &nr_msg, LB_ALLOC_BUF, LB_GET_NRS);
    if (ret < 0) {
	printf ("LB_read LB_GET_NRS (%s) failed, ret %d\n", 
					Lb_nr_name, ret);
	exit (1);
    }
    if (ret == 0) {
	printf ("No UN request records in LB %s\n", Lb_nr_name);
	exit (0);
    }
    len = 0;
    printf ("UN request records in LB %s:\n", Lb_nr_name);
    while (len < ret) {
	printf ("    %s", nr_msg + len);
	len += strlen (nr_msg + len) + 1;
    }
    printf ("\n");
    free (nr_msg);
    exit (0);
}

/********************************************************************

    Description: This is the UN callback function.

********************************************************************/

static void Un_callback (int fd, EN_id_t msgid, int msg_len, void *arg)
{
    int i;

    for (i = 0; i < Un_cnt; i++)
	if (Lb_fds[i] == fd)
	    break;
    if (i >= Un_cnt) {
	printf (
	"UN: (LB not found) fd %d, msgid %d, msg_info %x, sender %d\n", 
				fd, msgid, msg_len, EN_sender_id ());
    }
    else {
	printf (
	"UN: LB %s (fd %d), msgid %d, msg_info %x, sender %d\n", 
			Lb_name[i], fd, msgid, msg_len, EN_sender_id ());
    }
    fflush (stdout);
    return;
}

/********************************************************************

    Description: This is the AN callback function.

********************************************************************/

#define N_CHARS_PER_LINE 16

static void An_callback (EN_id_t an, char *msg, int msg_len, void *arg)
{

    if (msg_len == 0)
	printf (
	"AN: 0x%x (%d), sender %d\n", (unsigned int)an, an, EN_sender_id ());
    else {
	int line_returned;
	int i;
	unsigned char *cpt;

	line_returned = 0;
	if (an <= 0xffff)
	    printf (
	    "AN: 0x%x (%d), sender %d, len %d: \n", 
		    (unsigned int)an, an, EN_sender_id (), msg_len);
	else
	    printf (
	    "AN: 0x%x (%d), sender %d, len %d: \n", 
		    (unsigned int)an, an, EN_sender_id (), msg_len);
	if (Print_ascii) {
	    msg[msg_len - 1] = '\0';
	    printf ("    %s\n", msg);
	}
	else {
	    cpt = (unsigned char *)msg;
	    for (i = 0; i < msg_len; i++) {
    
		if ((i % N_CHARS_PER_LINE) == 0)
		    printf ("        ");
		if (i >= Print_len) {
		    printf ("...");
		    line_returned = 0;
		    break;
		}
		printf ("%.2x ", cpt[i]);
		line_returned = 0;
		if ((i % N_CHARS_PER_LINE) == N_CHARS_PER_LINE - 1) {
		    printf ("\n");
		    line_returned = 1;
		}
	    }
	    if (!line_returned)
		printf ("\n");
	}
    }
    fflush (stdout);
    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c, t;                  /* used by getopt */
    unsigned int nt;

    Post_an_defined = 0;
    Un_cnt = 0;
    An_cnt = 0;
    Post_msg[0] = '\0';
    Print_len = 12;
    Nr_record = 0;
    Rssd_report = 0;
    while ((c = getopt (argc, argv, "p:m:b:n:rdah?")) != EOF) {
	switch (c) {
	    int ret, v;

	    case 'p':
		if (strcmp (optarg, "REG_TABLE") == 0)
		    Post_an = EN_REP_REG_TABLE;
		else if (strncmp (optarg, "DISC_HOST", 9) == 0) {
		    Post_an = EN_DISC_HOST;
		    if (sscanf (optarg + 10, "%x", &Disc_ip) != 1)
			Print_usage ();
		}
		else if (optarg[0] == '0' && optarg[1] == 'x' &&
				sscanf (optarg, "%x", &nt) == 1)
		    Post_an = nt;
		else if (sscanf (optarg, "%u", &t) == 1)
		    Post_an = t;
		else
		    Print_usage ();
		Post_an_defined = 1;
		break;

	    case 'm':
		strncpy (Post_msg, optarg, LB_NTF_NAME_SIZE);
		Post_msg[LB_NTF_NAME_SIZE - 1] = '\0';
		break;

	    case 'b':
		STR_reset (Bin_msg, 256);
		Bm_size = 0;
		while ((ret = MISC_get_token 
				(optarg, "Cx", Bm_size, &v, 0)) > 0 &&
		    v >= 0 && v < 256) {
		    Bin_msg = STR_append (Bin_msg, (char *)&v, sizeof (char));
		    Bm_size++;
		}
		if (ret < 0) {
		    printf ("incorrest binary message\n");
		    exit (1);
		}
		Post_an_defined = 1;
		break;

	    case 'n':
		if (sscanf (optarg, "%d", &Print_len) != 1)
		    Print_usage ();
		break;

	    case 'r':
		Nr_record = 1;
		break;

	    case 'd':
		Rssd_report = 1;
		break;

	    case 'a':
		Print_ascii = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (Nr_record) {
	if (optind == argc - 1) {
	    strncpy (Lb_nr_name, argv[optind], LB_NTF_NAME_SIZE);
	    Lb_nr_name [LB_NTF_NAME_SIZE - 1] = '\0';
	    return (0);
	}
	else
	    Print_usage (argv);
    }

    while (optind < argc) {      /* get the ANs */
	char *cpt, *arg;
	unsigned int msgid;

	arg = argv[optind];

	/* first try the UN: find the "," */
	cpt = arg;
	if (*cpt == '|' || *cpt == '>')
	    break;
	while (*cpt != '\0' && *cpt != ',')
	    cpt++;
	if (*cpt == ',') {		/* found . */
	    if (cpt == arg)
		Print_usage ();
	    if (strcmp (cpt + 1, "ALL_MSGS") == 0)
		msgid = EN_ANY;
	    else if (strcmp (cpt + 1, "MSG_EXPIRED") == 0)
		msgid = LB_MSG_EXPIRED;
	    else if (sscanf (cpt + 1, "%u", &msgid) != 1)
		Print_usage ();
	    *cpt = '\0';
	    if (Un_cnt >= MAX_N_NTFS) {
		printf ("Too many UN specified\n");
		Print_usage ();
	    }
	    strncpy (Lb_name[Un_cnt], arg, LB_NTF_NAME_SIZE);
	    Lb_name[Un_cnt][LB_NTF_NAME_SIZE - 1] = '\0';
	    Lb_msgid[Un_cnt] = msgid;
	    Un_cnt++;
	}
	else {
	    if (An_cnt >= MAX_N_NTFS) {
		printf ("Too many AN specified\n");
		Print_usage ();
	    }
	    if (strcmp (arg, "ALL_ANS") == 0)
		Ans[An_cnt] = EN_ANY;
	    else if (arg[0] == '0' && arg[1] == 'x' &&
			sscanf (arg + 2, "%x", &nt) == 1)
		Ans[An_cnt] = nt;
	    else if (sscanf (arg, "%d", &t) == 1)
		Ans[An_cnt] = t;
	    else {
		printf ("incorrest AN specified\n");
		Print_usage ();
	    }
	    An_cnt++;
	}
	optind++;
    }

    return (0);
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage ()
{

    printf ("Usage: %s [options] an1 an2 ... lb_name,msgid ... \n", 
						Prog_name);
    printf ("       options:\n");
    printf ("       -p an (post AN \"an\")\n");
    printf ("          (\"%s -p REG_TABLE\" prints the local rssd's reg table)\n", 
						Prog_name);
    printf ("       -m msg_string (message to post with \"an\")\n");
    printf ("       -b binary_msg (binary message to post with \"an\")\n");
    printf ("       -n n_chars (number of chars in AN msg printing)\n");
    printf ("       -r (\"%s -r lb_name\" prints current NR records)\n", 
						Prog_name);
    printf ("       -d (\"%s -d\" prints the local rssd EN report)\n", 
						Prog_name);
    printf ("       -a (AN message is printed as ASCII string)\n");
    printf ("\n");

    printf ("This tool listens to a set of ANs and UNs specified\n");
    printf ("on the command line. The -p option posts an AN.\n");
    printf ("If there is something to listen, the tool will not\n");
    printf ("terminate until stopped by cntl-c. \"ALL_ANS\" can be\n");
    printf ("used for all ANs and \"ALL_MSGS\" for all message IDs\n"); 
    printf ("and \"MSG_EXPIRED\" for message expiration notifications.\n"); 

    exit (0);
}

