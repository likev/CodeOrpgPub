
/*****************************************************************

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:54 $
 * $Id: lb_info.c,v 1.51 2012/06/14 18:57:54 jing Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef LBLIB
#include <lb.h>
#else
#ifndef TEST
#include <rss_replace.h>
#endif
#include <infr.h>
#endif

#define LB_INFO_NAME_SIZE 	256
static char LB_name [LB_INFO_NAME_SIZE] = "";

static void Print_usage ();

/****************************************************************
*/

int main (int argc, char **argv)
{
    char prog_name [LB_INFO_NAME_SIZE];
    char buf[128];
    LB_attr attr;
    LB_status stat;
    int n_list, server_port;
    int ret, fd;
    int lb_size;
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */

    strncpy (prog_name, argv[0], LB_INFO_NAME_SIZE);
    prog_name [LB_INFO_NAME_SIZE - 1] = '\0';
    n_list = 8;

    /* get arguments */
    server_port = 0;
    while ((c = getopt (argc, argv, "n:p:ch?")) != EOF) {
	switch (c) {

	    case 'n':
		if (sscanf (optarg, "%d", &n_list) != 1)
		    Print_usage ();
		break;

	    case 'c':
		LB_test_compiler_flag ();
		break;

	    case 'p':
		if (sscanf (optarg, "%d", &server_port) != 1)
		    Print_usage ();
		break;

	    case 'h':
	    case '?':
		Print_usage ();
		break;
	}
    }
    if (optind == argc - 1) {      /* get the LB name */
	strncpy (LB_name, argv[optind], LB_INFO_NAME_SIZE);
	LB_name[LB_INFO_NAME_SIZE - 1] = '\0';
    }
    if (LB_name[0] == '\0') {
	fprintf (stderr, "LB name not specified - %s\n", prog_name);
	Print_usage ();
    }

    if (server_port > 0 &&
	RMT_port_number (server_port) < 0) {
	printf ("RMT_port_number (%d) failed\n", server_port);
	exit (1);
    }

    fd = LB_open (LB_name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (name: %s, ret = %d) - %s\n", 
					LB_name, fd, prog_name);
	exit (1);
    }

    stat.n_check = 0;
    stat.attr = &attr;
    ret = LB_stat (fd, &stat);
    if (ret < 0) {
	printf ("LB_stat failed (ret = %d) - %s\n", ret, prog_name);
	exit (1);
    }
    printf ("      \n");
    printf ("      version  = %d\n", (int)attr.version);
    attr.remark[LB_REMARK_LENGTH - 1] = '\0';
    printf ("      remark = %s\n", attr.remark);
    printf ("      mode = %o\n", (unsigned int)attr.mode);
    if (attr.msg_size > 0)
	printf ("      msg_size = %d\n", attr.msg_size);
    else
	printf ("      msg_size = undefined\n");
    printf ("      maxn_msgs = %d\n", attr.maxn_msgs);
    printf ("      tag_size  = %d\n", (int)(attr.tag_size & TAG_SIZE_MASK));
    printf ("      nra_size  = %d\n", (int)(attr.tag_size >> NRA_SIZE_SHIFT));
    if (attr.types & LB_MEMORY)
	printf ("      LB_MEMORY type\n");
    if (attr.types & LB_REPLACE)
	printf ("      LB_REPLACE type\n");
    if (attr.types & LB_MUST_READ)
	printf ("      LB_MUST_READ type\n");
    if (attr.types & LB_SHARE_STREAM)
	printf ("      LB_SHARE_STREAM type\n");
    if (attr.types & LB_SINGLE_WRITER)
	printf ("      LB_SINGLE_WRITER type\n");
    if (attr.types & LB_NOEXPIRE)
	printf ("      LB_NOEXPIRE type\n");
    if (attr.types & LB_UNPROTECTED)
	printf ("      LB_UNPROTECTED type\n");
    if (attr.types & LB_DIRECT)
	printf ("      LB_DIRECT type\n");
    if (attr.types & LB_UN_TAG)
	printf ("      LB_UN_TAG type\n");
    if (attr.types & LB_MSG_POOL)
	printf ("      LB_MSG_POOL type\n");
    if ((attr.types & LB_DB) && !(attr.types & (LB_REPLACE | LB_MSG_POOL)))
	printf ("      LB_DB type\n");

    printf ("      \n");
    printf ("      n_msgs  = %d\n", stat.n_msgs);
    MISC_string_date_time (buf, 128, (time_t *)&stat.time);
    printf ("      time  = %s\n", buf);
    lb_size = LB_misc (fd, LB_GET_LB_SIZE);
    if (lb_size < 0) {
	printf ("LB_misc LB_GET_LB_SIZE failed (ret %d)\n", lb_size);
	exit (1);
    }
    printf ("      LB size = %d\n", lb_size);
    printf ("      \n");

    if (n_list > 0) {
	LB_info *list;
	int n;

	list = (LB_info *)malloc (n_list * sizeof (LB_info));
	if (list == NULL) {
	    printf ("malloc failed\n");
	    exit (1);
	}

	n = LB_list (fd, list, n_list);
	if (n < 0) {
	    printf ("LB_list failed\n");
	}
	else {
	    int i;

	    printf ("                id      size    mark\n");
	    for (i = 0; i < n; i++) 
		printf ("%8d%10u%10d%10x\n", i, list[i].id, list[i].size, (unsigned int)list[i].mark);
	    printf ("      \n");
	}
    }

    LB_close (fd);

    exit (0);
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage () {

    printf ("Usage: lb_info options LB_name\n");
    printf ("       print information of LB LB_name\n");
    printf ("       options:\n");
    printf ("       -n n_msgs (print a list of latest \"n_msgs\" messages)\n");
    printf ("       -p port_number (specifies the rssd port number)\n");
    printf ("       -c (print LB compiler flags and terminate)\n");
    printf ("       -h (print usage information)\n");
    printf ("       example: lb_info -n 5 my_lb\n");
    printf ("\n");

    exit (0);
}

