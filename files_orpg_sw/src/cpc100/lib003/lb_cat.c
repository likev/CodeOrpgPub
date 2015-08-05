
/*****************************************************************

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/03/03 21:07:27 $
 * $Id: lb_cat.c,v 1.32 2008/03/03 21:07:27 jing Exp $
 * $Revision: 1.32 $
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

#define LB_CAT_NAME_SIZE 	256

static char Prog_name [LB_CAT_NAME_SIZE];
static char LB_name [LB_CAT_NAME_SIZE] = "";
static LB_id_t Rmsg_id;
static LB_id_t Wmsg_id;
static int Print_att;
static int Write_lb;
static int Tag_write;
static int Tag;
static int All_msgs;
static int N_out_bytes = 0x7fffffff;
static int Out_offset = 0;

static void Print_msg ();
static void Write_msg (int fd, char *msg, int len);
static int Read_stdin (char **msg, int n_bytes);
static void Print_usage ();
static int Read_options (int argc, char **argv);
static void Write_all_msgs (int fd);


/****************************************************************
*/

int main (int argc, char **argv)
{

    strncpy (Prog_name, argv[0], LB_CAT_NAME_SIZE);
    Prog_name [LB_CAT_NAME_SIZE - 1] = '\0';

    if (Read_options (argc, argv) < 0)
	exit (1);

    if (Write_lb) {
	int fd, len;
	char *msg;

	fd = LB_open (LB_name, LB_WRITE, NULL);
	if (fd < 0) {               /* open failed */
	    fprintf (stderr, "LB_open failed (name: %s, ret = %d) - %s\n", 
					LB_name, fd, Prog_name);
	    exit (-1);
	}

	if (All_msgs) 
	    Write_all_msgs (fd);
	else if (Tag_write) {
	    Write_msg (fd, NULL, 0);
	}
	else {
	    len = Read_stdin (&msg, 0);
	    Write_msg (fd, msg, len);
	    if (msg != NULL)
		free (msg);
	}
    }
    else
	Print_msg ();

    exit (0);
}

/*********************************************************************

    Description: This function reads an LB message and prints
		it to STDOUT.

*********************************************************************/

static void Print_msg ()
{
    int len, fd;
    LB_id_t msg_id;
    char *msg;
    int tag;

    fd = LB_open (LB_name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	fprintf (stderr, "LB_open failed (name: %s, ret = %d) - %s\n", 
					LB_name, fd, Prog_name);
	exit (-1);
    }

    LB_register (fd, LB_ID_ADDRESS, &msg_id);
    LB_register (fd,  LB_TAG_ADDRESS, &tag);

    if (!All_msgs) {
	if (Rmsg_id == LB_LATEST) {
	    LB_seek (fd, 0, LB_LATEST, NULL);
	    Rmsg_id = LB_NEXT;
	}

	len = LB_read (fd, &msg, LB_ALLOC_BUF, Rmsg_id);
	if (len < 0) {
	    fprintf (stderr, "LB_read failed (ret %d) - %s\n", len, Prog_name);
	    exit (-1);
	}

	if (Print_att)
	    fprintf (stderr, "ID %x, len %d, tag %x\n", 
			(unsigned int)msg_id, len, (unsigned int)tag);
	if (Out_offset < len) {
	    msg += Out_offset;
	    len -= Out_offset;
	    if (len > N_out_bytes)
		len = N_out_bytes;
	    fwrite (msg, 1, len, stdout);
	}
    }
    else {
	LB_info info;

	LB_seek (fd, 0, LB_FIRST, NULL);
	while (1) {
	    if (LB_msg_info (fd, LB_CURRENT, &info) == LB_SUCCESS) {
		if (info.size < 0) {
		    LB_seek (fd, 1, LB_CURRENT, NULL);
		    continue;
		}
		len = LB_read (fd, &msg, LB_ALLOC_BUF, LB_NEXT);
		if (len < 0) {
		    fprintf (stderr, "LB_read failed (ret %d) - %s\n", 
							len, Prog_name);
		    exit (-1);
		}
		if (Print_att) {
		    fwrite (&msg_id, 1, sizeof (int), stdout);
		    fwrite (&len, 1, sizeof (int), stdout);
		    fwrite (&tag, 1, sizeof (int), stdout);
		}
		fwrite (msg, 1, len, stdout);
	    }
	    else
		break;
	}
    }

    LB_close (fd);
    return;
}

/*********************************************************************

    Description: This function writes a message to the LB.

*********************************************************************/

static void Write_msg (int fd, char *msg, int len)
{
    int ret;

    if (Tag_write) {
	if (Wmsg_id == LB_NEXT) {
	    fprintf (stderr, 
		"msg ID not specified for tag write - %s\n", Prog_name);
	}
	else
	    LB_set_tag (fd, Wmsg_id, Tag);
    }
    else {
	ret = LB_write (fd, msg, len, Wmsg_id);
	if (ret < 0) {
	    fprintf (stderr, "LB_write failed (ret %d) - %s\n", ret, Prog_name);
	}
    }

    LB_close (fd);
    return;
}

/*********************************************************************

    Description: This function reads all messages from the SIDIN port
		and writes them to the LB.

*********************************************************************/

static void Write_all_msgs (int fd)
{
    int tag;

    LB_register (fd,  LB_TAG_ADDRESS, &tag);

    while (1) {
	char *msg;
	int *ipt, size, ret;
	LB_id_t id;

	ret = Read_stdin (&msg, 3 * sizeof (int));
	if (ret < 3 * (int)sizeof (int)) 
	    exit (0);

	ipt = (int *)msg;
	id = ipt[0];
	size = ipt[1];
	tag = ipt[2];

	if (size > 0) {
	    ret = Read_stdin (&msg, size);
	    if (ret < size) 
		exit (0);
	}

	ret = LB_write (fd, msg, size, id);
	if (ret < 0) {
	    fprintf (stderr, "LB_write failed (ret %d) - %s\n", 
							ret, Prog_name);
	    exit (-1);
	}
    }
}

/*********************************************************************

    Description: This function reads a message from the stdio port.

    Input:	n_bytes - Number of bytes to read. 0 indicates 
			reading until EOF.

    output:	msg - the message read;

    Return:	length of the message.

*********************************************************************/

static int Read_stdin (char **msg, int n_bytes)
{
    static char *buf = NULL;
    int bsize, len, ret;

    if (buf != NULL) {
	free (buf);
	buf = NULL;
    }

    bsize = 0;
    len = 0;
    while (1) {
	if (len >= bsize) {
	    char *cpt;
	    int new_s;

	    if (n_bytes == 0) {
		new_s = bsize * 2;
		if (new_s == 0)
		     new_s = 1024;
	    }
	    else 
		new_s = n_bytes;
	    cpt = malloc (new_s);
	    if (cpt == NULL) {
		fprintf (stderr, "malloc failed - %s\n", Prog_name);
		exit (-1);
	    }
	    if (buf != NULL) {
		memcpy (cpt, buf, len);
	        free (buf);
	    }
	    buf = cpt;
	    bsize = new_s;
	}

	if (n_bytes == 0)
	    ret = fread (buf + len, 1, bsize - len, stdin);
	else
	    ret = fread (buf + len, 1, n_bytes - len, stdin);
	if (ret > 0)
	    len += ret;
	if (feof (stdin) || (n_bytes > 0 && len >= n_bytes))
	    break;
	if (ret <= 0) {
	    fprintf (stderr, "fread failed - %s\n", Prog_name);
	    exit (-1);
	}
    }
    *msg = buf;
    return (len);
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
    int c, ret;             /* used by getopt */

    LB_name[0] = '\0';
    Rmsg_id = LB_LATEST;
    Print_att = 0;
    Write_lb = 0;
    Wmsg_id = LB_ANY;
    Tag_write = 0;
    All_msgs = 0;
    while ((c = getopt (argc, argv, "i:t:p:wash?")) != EOF) {
	switch (c) {

	    case 'i':
		if (sscanf (optarg, "%u", &Rmsg_id) != 1)
		    Print_usage ();
		Wmsg_id = Rmsg_id;
		break;

	    case 't':
		if (sscanf (optarg, "%d", &Tag) != 1)
		    Print_usage ();
		Tag_write = 1;
		break;

	    case 'p':
		ret = sscanf (optarg, "%d%*c%d", &Out_offset, &N_out_bytes);
		if (ret < 1 || Out_offset < 0)
		    Print_usage ();
		if (ret == 1)
		    N_out_bytes = 0x7fffffff;
		if (N_out_bytes < 0)
		    Print_usage ();		    
		break;

	    case 'w':
		Write_lb = 1;
		break;

	    case 'a':
		Print_att = 1;
		break;

	    case 's':
		All_msgs = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1) {      /* get the LB name */
	strncpy (LB_name, argv[optind], LB_CAT_NAME_SIZE);
	LB_name [LB_CAT_NAME_SIZE - 1] = '\0';
    }

    if (LB_name [0] == '\0') {
	fprintf (stderr, "LB name not specified - %s\n", Prog_name);
	Print_usage ();
    }

    return (0);
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage ()
{

    fprintf (stderr, "Usage: %s options LB_name\n", Prog_name);
    fprintf (stderr, "       read/write LB messages\n");
    fprintf (stderr, "       options:\n");
    fprintf (stderr, "       -i msg_id (message ID; \n");
    fprintf (stderr, "          default - read LB_LATEST; write LB_NEXT)\n");
    fprintf (stderr, "       -t tag (tag value; default - write message)\n");
    fprintf (stderr, "       -w (write a message or tag to the LB)\n");
    fprintf (stderr, "       -a (print message attribute first)\n");
    fprintf (stderr, "       -p offset[,n_bytes] (output part of the message:\n");
    fprintf (stderr, "          \"n_bytes\" bytes at offset \"offset\"\n");
    fprintf (stderr, "       -s (read/write all messages. e.g.\n");
    fprintf (stderr, "           lb_cat -s -a t.lb | lb_cat -w -s tt.lb)\n");
    fprintf (stderr, "       -h (print usage information)\n");
    fprintf (stderr, "       example: %s -i 55 lb_name\n", Prog_name);
    fprintf (stderr, "\n");

    exit (0);
}

