
/*****************************************************************


*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:54 $
 * $Id: lb_create.c,v 1.57 2012/06/14 18:57:54 jing Exp $
 * $Revision: 1.57 $
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
#include "rss_replace.h"
#endif
#include <infr.h>
#endif

#define LB_CREATE_NAME_SIZE 	256
#define MAX_NORMAL_ARGV		32

static char LB_name [LB_CREATE_NAME_SIZE] = "";

static void Print_usage ();


/****************************************************************
*/

int main (int argc, char **argv)
{
    char prog_name[LB_CREATE_NAME_SIZE];
    char remark[LB_CREATE_NAME_SIZE];
    int msg_size, maxn_msgs, tag_size;
    unsigned int mode;
    int flags;
    LB_attr attr;
    int lbd;
    int i;
    int write_message, msize, id, num, fix_byte_order;
    int nra_size;		/* notification request area size */
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int nargc;
    char *nargv[MAX_NORMAL_ARGV];

    strncpy (prog_name, argv[0], LB_CREATE_NAME_SIZE);
    prog_name [LB_CREATE_NAME_SIZE - 1] = '\0';

    /* default values */
    flags = 0;
    mode = 0666;
    msg_size = 0;
    maxn_msgs = -1;
    tag_size = 0;
    remark[0] = '\0';
    nra_size = -1;
    write_message = 0;
    num = 1;
    fix_byte_order = 0;

    /* get arguments */
    nargc = 0;			/* normal arg count */
    for (i = 0; i < argc; i++) {
	if (strcmp ("LB_MEMORY", argv[i]) == 0)
	    flags |= LB_MEMORY;
	else if (strcmp ("LB_SINGLE_WRITER", argv[i]) == 0)
	    flags |= LB_SINGLE_WRITER;
	else if (strcmp ("LB_NOEXPIRE", argv[i]) == 0)
	    flags |= LB_NOEXPIRE;
	else if (strcmp ("LB_UNPROTECTED", argv[i]) == 0)
	    flags |= LB_UNPROTECTED;
	else if (strcmp ("LB_REPLACE", argv[i]) == 0)
	    flags |= LB_REPLACE;
	else if (strcmp ("LB_MUST_READ", argv[i]) == 0)
	    flags |= LB_MUST_READ;
	else if (strcmp ("LB_SHARE_STREAM", argv[i]) == 0)
	    flags |= LB_SHARE_STREAM;
	else if (strcmp ("LB_FILE", argv[i]) == 0)
	    flags |= LB_FILE;
	else if (strcmp ("LB_NORMAL", argv[i]) == 0)
	    flags |= LB_NORMAL;
	else if (strcmp ("LB_DIRECT", argv[i]) == 0)
	    flags |= LB_DIRECT;
	else if (strcmp ("LB_UN_TAG", argv[i]) == 0)
	    flags |= LB_UN_TAG;
	else if (strcmp ("LB_MSG_POOL", argv[i]) == 0)
	    flags |= LB_MSG_POOL;
	else if (strcmp ("LB_DB", argv[i]) == 0)
	    flags |= LB_DB;
	else if (strcmp ("LB_POOL", argv[i]) == 0)
	    flags |= LB_DB;
	else {
	    if (nargc >= MAX_NORMAL_ARGV)
		Print_usage ();
	    nargv[nargc++] = argv[i];
	}
    }

    while ((c = getopt (nargc, nargv, "n:s:t:u:w:r:m:bh?")) != EOF) {
	switch (c) {

	    case 'n':
		if (sscanf (optarg, "%d", &maxn_msgs) != 1)
		    Print_usage ();
		break;

	    case 's':
		if (sscanf (optarg, "%d", &msg_size) != 1)
		    Print_usage ();
		break;

	    case 't':
		if (sscanf (optarg, "%d", &tag_size) != 1)
		    Print_usage ();
		break;

	    case 'u':
		if (sscanf (optarg, "%d", &nra_size) != 1)
		    Print_usage ();
		break;

	    case 'w':
		if (sscanf (optarg, "%d%*c%d%*c%d", &msize, &id, &num) < 2)
		    Print_usage ();
		write_message = 1;
		break;

	    case 'r':
		strncpy (remark, optarg, LB_CREATE_NAME_SIZE);
		break;

	    case 'm':
		if (sscanf (optarg, "%o", &mode) != 1)
		    Print_usage ();
		break;

	    case 'b':
		fix_byte_order = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage ();
		break;
	}
    }

    if (optind == nargc - 1) {      /* get the LB name */
	strncpy (LB_name, nargv[optind], LB_CREATE_NAME_SIZE);
	LB_name[LB_CREATE_NAME_SIZE - 1] = '\0';
    }
    if (LB_name[0] == '\0') {
	fprintf (stderr, "LB name not specified - %s\n", prog_name);
	Print_usage ();
    }

    if (write_message) {
	char *buf;
	int ret;

	lbd = LB_open (LB_name, LB_WRITE, NULL);
	if (lbd < 0) {
	    printf ("LB_open %s failed (ret %d) - %s\n",
				LB_name, lbd, prog_name);
	    exit (-1);
	}
	buf = (char *)malloc (msize);
	if (buf == NULL) {
	    printf ("malloc failed (size %d) - %s\n", msize, prog_name);
	    exit (-1);
	}
	for (i = 0; i < num; i++) {
	    ret = LB_write (lbd, buf, msize, id + i);
	    if (ret != msize) {
		printf ("LB_write failed (size %d, ret %d) - %s\n", 
						msize, ret, prog_name);
		printf ("%d messages written - %s\n", i, prog_name);
		exit (-1);
	    }
	    ret = LB_write (lbd, buf, 0, id + i);
	    if (ret != 0) {
		printf ("LB_write failed (size %d, ret %d) - %s\n", 
							0, ret, prog_name);
		printf ("%d messages written - %s\n", i, prog_name);
		exit (-1);
	    }
	}
	exit (0);
    }

    if (fix_byte_order) {
	int ret;

	ret = LB_fix_byte_order (LB_name);
	if (ret < 0)
	    printf ("LB_fix_byte_order %s failed (ret %d) - %s\n",
				LB_name, ret, prog_name);
	exit (0);
    }

    if (maxn_msgs < 0) {		/* set default */
	if (flags & (LB_REPLACE | LB_MSG_POOL | LB_DB))
	    maxn_msgs = 10;
	else
	    maxn_msgs = 100;
    }
    if (nra_size < 0)			/* set default */
	nra_size = LB_DEFAULT_NRS;

    LB_remove (LB_name);

    strncpy (attr.remark, remark, LB_REMARK_LENGTH);
    attr.remark [LB_REMARK_LENGTH - 1] = '\0';
    
    attr.mode = mode;
    attr.msg_size = msg_size;
    attr.maxn_msgs = maxn_msgs;
    attr.types = flags;
    attr.tag_size = tag_size | (nra_size << NRA_SIZE_SHIFT);
    lbd = LB_open (LB_name, LB_CREATE, &attr);

    if (lbd < 0) {
	printf ("LB_open (create) %s failed (ret %d) - %s\n",
				LB_name, lbd, prog_name);
	exit (-1);
    }

    LB_close (lbd);
    exit (0);
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage () {

    printf ("Usage: lb_create options LB_type_flags LB_name\n");
    printf ("       Creates LB of LB_name\n");
    printf ("       options:\n");
    printf ("       -s average_message_size (default: 0 - arbitrary message size)\n"); 
    printf ("       -n max_n_msgs (maximum number of messages; Default: 100 for\n");
    printf ("          sequential LB and 10 for replaceable)\n");
    printf ("       -m access_mode (default: 0666)\n");
    printf ("       -r remark (default: \"\")\n");
    printf ("       -t tag_size (default: 0)\n");
    printf ("       -u notification_request_area_size (default: 32 for\n");
    printf ("          sequential LB and 4 * max_n_msgs for replaceable)\n");
/*
    printf ("       -w size,id[,num] (write a message of \"size\" and ID \"id\";\n");
    printf ("          and, then, write a message of size 0 and the same ID;\n");
    printf ("          This option assumes that the LB exists and LB creation\n");
    printf ("          is disabled. The message has no defined values in it.\n");
    printf ("          This option is designed for initializing message sizes\n");
    printf ("          for replaceable LBs.\n");
    printf ("          If \"num\" is specified, \"num\" messages are initialized \n");
    printf ("          with ID id, id + 1, ...)\n");
*/
    printf ("       -b (fix_byte_order)\n");
    printf ("       -h (print usage information)\n");
    printf ("       example: lb_create -s 10 -n 32 -r \"My remark\" LB_FILE LB_DB my_lb\n");
    printf ("\n");

    exit (0);
}

