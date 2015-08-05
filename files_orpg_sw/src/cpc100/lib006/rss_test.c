/****************************************************************
		
    Module: rss_test.c	
				
    Description: This is a test function for the RSS library.

    Notes: To run this program, one should make rss_t and put
	rss_t in $(HOME)/tmp on the remote hosts. If rssd is
	running in foreground, a "ls -l" output should appear
	on the remote host's screen.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 1998/12/01 21:18:04 $
 * $Id: rss_test.c,v 1.11 1998/12/01 21:18:04 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 * $Log: rss_test.c,v $
 * Revision 1.11  1998/12/01 21:18:04  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.8  1998/06/19 17:05:44  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:44:43  cm
 * Initial revision
 *
 * 
*/

/* include files */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "rss_replace.h"

#define TEST_DATA_SIZE	16000		/* in number of integers */
#define NAME_LEN	256		/* name length */

enum {SET_DATA, TEST_DATA};		/* values for the "sw" argument
					   of function Test_data */

static int Test_file (char *rhost, int *t_data);
static int Read_seek_write (int fd, int *t_data, int length);
static int Test_lb (char *rhost, int *t_data);
static int Test_uu (char *rhost);
static int Lb_read_write (int fd, int *t_data, int length);
static int Test_data (int sw, int *t_data, int length);


/****************************************************************
			
    Description: This is the main function of rss_test.

    Input:	argc - the number of the command line arguments
		argv - the command line arguments.

    Returns: 0 on success or -1 on failure.

****************************************************************/

void 
main (int argc, char **argv)
{
    static int t_data [TEST_DATA_SIZE];	/* test data */
    char rhost [NAME_LEN];
    int status;

    status = 0;

    /* remote host name */
    if (argc > 1 && strcmp (argv [1], "-h") != 0) {
	strncpy (rhost, argv [1], NAME_LEN);
	rhost [NAME_LEN - 1] = 0;
    }
    else {
	printf ("Usage: rss_test remote_host_name\n");
	exit (status);
    }

    printf ("RMT_terminate_service: ret = %d\n", 
	RMT_terminate_service (rhost, "osrpg005 osrpg012"));
    exit (0);


    /* prepare known test data */
    Test_data (SET_DATA, t_data, TEST_DATA_SIZE);

    /* switch on the RMT message */
    RMT_messages (RMT_ON);

    /* testing file access */
    printf ("\nTesting file access:\n");
    if (Test_file (rhost, t_data) < 0)
	status = -1;

    /* testing the LB */
    printf ("\nTesting LB:\n");
    if (Test_lb (rhost, t_data) < 0)
	status = -1;

    /* testing UU */
    printf ("\nTesting uu:\n");
    if (Test_uu (rhost) < 0)
	status = -1;

    if (status == 0)
	printf ("\nRSS test successful - no error found\n");
    exit (status);
}

/****************************************************************

    Description: This function tests the uu functions

    Input:	rhost - name of the remote host

    Return: 0 on success or -1 on failure.

****************************************************************/

static int Test_uu (char *rhost)
{
    static char other_host [] = "nsslsun";
    static char local_host [256];
    static char *argv[3] = {"tmp/rss_t", "argv1", "argv2"};
    char *name;
    int i;

    /* get local host name */
#if (defined (HPUX) || defined (SUNOS4))
    gethostname (local_host, 256);
#else
    sysinfo (SI_HOSTNAME, local_host, 256);
#endif

    for (i = 0; i < 3; i++) {
	char tmp [512];
	int ret;
	int pid;

	switch (i) {

	    case 0:
		name = rhost;
		break;

	    case 1:
		name = other_host;
		break;

	    case 2:
		name = local_host;
		break;

	    default:
		name = rhost;
		break;
	}

	/* connection and time */
	if (RSS_test_rpc (name) == RSS_SUCCESS) {
	    time_t tt;

	    RSS_time (name, &tt);
	    printf ("host %s is OK - time = %d\n", name, (int)tt);
	}
	else
	    printf ("host %s is not RPC connectable\n", name);

	/* system */
	sprintf (tmp, "%s:ls -l", name);
	ret = RSS_system (tmp);
	printf ("RSS_system: ret = %d\n", ret);

	/* exec */
	sprintf (tmp, "%s:%s", name, argv[0]);
	pid = RSS_exec (tmp, argv);
	printf ("RSS_exec: ret = %d\n", pid);
	sleep (1);

	/* kill */
	ret = RSS_kill (name, pid, SIGTERM);
	printf ("RSS_kill: ret = %d\n", ret);
	ret = RSS_kill (name, pid, SIGTERM);
	printf ("RSS_kill (second): ret = %d\n", ret);

	printf ("\n");
    }
    return (0);
}

/****************************************************************

    Description: This function tests the file access functions

    Input:	rhost - name of the remote host
		t_data - the known test data

    Return: 0 on success or -1 on failure.

****************************************************************/

static int Test_file (char *rhost, int *t_data)
{
    int rfd, lfd;
    static char lname [] = "rss_tmp";
    char rname [NAME_LEN + 32];
    char rname_new [NAME_LEN + 32];
    char tmp [NAME_LEN * 2 + 64 + 16];
    int i, ret, ret1, ret2, ret3;

    sprintf (rname, "%s:tmp/rss_tmp", rhost);	/* the remote file */

    /* test open, close and unlink */
    printf ("Testing (64) open, close and unlink ... \n");
    for (i = 0; i < 64; i++) {
	if ((rfd = open (rname, O_RDWR | O_CREAT, 0777)) < 0) {
	    printf ("open %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, rfd, errno);
	    return (-1);
	}
	if ((lfd = open (lname, O_RDWR | O_CREAT, 0777)) < 0) {
	    printf ("open %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, lfd, errno);
	    return (-1);
	}
	if ((ret = close (rfd)) < 0) {
	    printf ("close rfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if ((ret = close (lfd)) < 0) {
	    printf ("close lfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if ((ret = unlink (rname)) < 0) {
	    printf ("unlink %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, ret, errno);
	    return (-1);
	}
	if ((ret = unlink (lname)) < 0) {
	    printf ("unlink %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, ret, errno);
	    return (-1);
	}
    }
    printf ("open, close and unlink test done\n");

    /* open a remote file */
    if ((rfd = open (rname, O_RDWR | O_CREAT, 0777)) < 0) {
	printf ("open %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, rfd, errno);
	return (-1);
    }
    printf ("file %s opened (fd = %d) - rss_test\n", rname, rfd);

    /* open a local file */
    if ((lfd = open (lname, O_RDWR | O_CREAT, 0777)) < 0) {
	printf ("open %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, lfd, errno);
	return (-1);
    }
    printf ("file %s opened (fd = %d) - rss_test\n", lname, lfd);

    /* test remote read, seek and and write */
    printf ("test remote read, seek and write - small segments \n");
    if (Read_seek_write (rfd, t_data, 1000) < 0)
	return (-1);
    printf ("test remote read, seek and write - large segments \n");
    if (Read_seek_write (rfd, t_data, TEST_DATA_SIZE) < 0)
	return (-1);

    /* test local read, seek and and write */
    printf ("test local read, seek and write - small segments \n");
    if (Read_seek_write (lfd, t_data, 1000) < 0)
	return (-1);
    printf ("test local read, seek and write - large segments \n");
    if (Read_seek_write (lfd, t_data, TEST_DATA_SIZE) < 0)
	return (-1);

    /* testing RSS_copy */
    printf ("test RSS_copy\n");
    if ((ret = RSS_copy (rname, "rss_t_f")) != RSS_SUCCESS ||
	(ret1 = unlink (rname)) != 0 ||
	(ret2 = RSS_copy ("rss_t_f", rname)) != RSS_SUCCESS ||
	(ret3 = RSS_copy (rname, "rss_t_f1")) != RSS_SUCCESS) {
	printf ("RSS_copy failed (RSS_copy %d, unlink %d, RSS_copy %d, RSS_copy %d) - rss_test\n",
			ret, ret1, ret2, ret3);
	return (-1);
    }
    if (system ("diff rss_t_f rss_t_f1") != 0) {
	printf ("RSS_copy error - rss_test\n");
	return (-1);
    }

    /* testing rename */
    printf ("test RSS_rename\n");
    sprintf (tmp, "%s:diff /tmp/rss_tmp /tmp/rss_tmp1", rhost);
    sprintf (rname, "%s:/tmp/rss_tmp", rhost);
    sprintf (rname_new, "%s:/tmp/rss_tmp1", rhost);
    if ((ret = RSS_copy ("rss_t_f", rname_new)) != RSS_SUCCESS ||
	(ret1 = rename (rname_new, rname)) != 0 ||
	(ret2 = RSS_copy ("rss_t_f", rname_new)) != RSS_SUCCESS ||
	(ret3 = system (tmp)) != 0) {
	printf ("RSS_rename failed (RSS_copy %d, rename %d, RSS_copy %d, system %d) - rss_test\n", 
			ret, ret1, ret2, ret3);
	return (-1);
    }
    unlink ("rss_t_f");
    unlink ("rss_t_f1");
    unlink ("/tmp/rss_tmp");
    unlink ("/tmp/rss_tmp1");

    return (0);
}

/****************************************************************

    Description: This function tests file read, write and seek.

    Input:	fd - the file descriptor
		t_data - the known test data
		length - test segment length

    Return: 0 on success or -1 on failure.

****************************************************************/

static int Read_seek_write (int fd, int *t_data, int length)
{
    static int buf [TEST_DATA_SIZE];
    int i, ret, nbytes;

    nbytes = length * sizeof (int);
    for (i = 0; i < 4; i++) 
	if ((ret = write (fd, (char *)t_data, nbytes)) != nbytes) {
	    printf ("write failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
    if ((ret = lseek (fd, nbytes, SEEK_CUR)) < 0) {
	printf ("lseek failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    if ((ret = write (fd, (char *)t_data, nbytes)) != nbytes) {
	printf ("write failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    if ((ret = lseek (fd, nbytes * 4, SEEK_SET)) < 0) {
	printf ("lseek failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    if ((ret = write (fd, (char *)t_data, nbytes)) != nbytes) {
	printf ("write failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    if ((ret = lseek (fd, 0, SEEK_SET)) < 0) {
	printf ("lseek failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    for (i = 0; i < 6; i++) {
	if ((ret = read (fd, (char *)buf, nbytes)) != nbytes) {
	    printf ("read failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if (Test_data (TEST_DATA, buf, length) < 0)
	    return (-1);
    }
    if ((ret = lseek (fd, 0, SEEK_SET)) < 0) {
	printf ("lseek failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    return (0);
}

/****************************************************************

    Description: This function tests the LB functions

    Input:	rhost - name of the remote host
		t_data - the known test data

    Returns: 0 on success or -1 on failure.

****************************************************************/

static int Test_lb (char *rhost, int *t_data)
{
    int rfd, lfd;
    static char lname [] = "rss_tmp.lb";
    char rname [NAME_LEN + 32];
    int i, ret;
    LB_attr attr;

    sprintf (rname, "%s:tmp/rss_tmp.lb", rhost);/* the remote LB */

    /* test open, close and unlink */
    printf ("Testing (64) LB_open, LB_close and LB_remove ... \n");
    unlink (rname);
    unlink (lname);
    for (i = 0; i < 64; i++) {

	strcpy (attr.remark, "My remark");
	attr.mode = 0666;
	attr.msg_size = 1000;
	attr.maxn_msgs = 100;
	attr.types = 0;
	if ((rfd = LB_open (rname, LB_CREATE, &attr)) < 0) {
	    printf ("LB_open %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, rfd, errno);
	    return (-1);
	}
	if ((lfd = LB_open (lname, LB_CREATE, &attr)) < 0) {
	    printf ("LB_open %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, lfd, errno);
	    return (-1);
	}
	if ((ret = LB_close (rfd)) < 0) {
	    printf ("LB_close rfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if ((ret = LB_close (lfd)) < 0) {
	    printf ("LB_close lfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if ((ret = LB_remove (rname)) < 0) {
	    printf ("LB_remove %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, ret, errno);
	    return (-1);
	}
	if ((ret = LB_remove (lname)) < 0) {
	    printf ("LB_remove %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, ret, errno);
	    return (-1);
	}
    }
    printf ("LB_open, LB_close and unlink test done\n");

    /* open a remote LB */
    strcpy (attr.remark, "My remark");
    attr.mode = 0666;
    attr.msg_size = TEST_DATA_SIZE * sizeof (int);
    attr.maxn_msgs = 100;
    attr.types = 0;
    if ((rfd = LB_open (rname, LB_WRITE, &attr)) != LB_DIFFER) {
	printf ("LB_open %s failed (ret = %d, should be %d or %d, errno = %d) - rss_test\n", 
				rname, rfd, LB_DIFFER, LB_OPEN_FAILED, errno);
    }
    if ((rfd = LB_open (rname, LB_CREATE, &attr)) < 0) {
	printf ("LB_open %s failed (ret = %d, errno = %d) - rss_test\n", 
					rname, rfd, errno);
	return (-1);
    }
    printf ("LB %s opened (fd = %d) - rss_test\n", rname, rfd);

    /* open a local LB */
    if ((lfd = LB_open (lname, LB_CREATE, &attr)) < 0) {
	printf ("LB_open %s failed (ret = %d, errno = %d) - rss_test\n", 
					lname, lfd, errno);
	return (-1);
    }
    printf ("LB %s opened (fd = %d) - rss_test\n", lname, lfd);

    /* test remote LB_read and LB_write */
    printf ("test remote LB_read and LB_write - small messages \n");
    if (Lb_read_write (rfd, t_data, 1000) < 0)
	return (-1);
    if ((ret = LB_clear (rfd, LB_ALL)) < 0) {
	printf ("LB_clear rfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    printf ("test remote LB_read and LB_write - large messages \n");
    if (Lb_read_write (rfd, t_data, TEST_DATA_SIZE) < 0)
	return (-1);

    /* test local LB_read and LB_write */
    printf ("test local LB_read and LB_write - small messages \n");
    if (Lb_read_write (lfd, t_data, 1000) < 0)
	return (-1);
    if ((ret = LB_clear (lfd, LB_ALL)) < 0) {
	printf ("LB_clear lfd failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	return (-1);
    }
    printf ("test local LB_read and LB_write - large messages \n");
    if (Lb_read_write (lfd, t_data, TEST_DATA_SIZE) < 0)
	return (-1);

    return (0);
}

/****************************************************************

    Description: This function tests LB read and write.

    Input:	fd - the LB descriptor
		t_data - the known test data
		length - test message length

    Return: 0 on success or -1 on failure.

****************************************************************/

static int Lb_read_write (int fd, int *t_data, int length)
{
    static int buf [TEST_DATA_SIZE];
    int i, ret, nbytes;

    nbytes = length * sizeof (int);
    for (i = 0; i < 18; i++) 
	if ((ret = LB_write (fd, (char *)t_data, nbytes, i)) != nbytes) {
	    printf ("LB_write failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
    for (i = 18; i < 32; i++) 
	if ((ret = LB_write (fd, (char *)t_data, nbytes, i)) != nbytes) {
	    printf ("LB_write failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
    for (i = 0; i < 20; i++) {
	if ((ret = LB_read (fd, (char *)buf, nbytes, i)) != nbytes) {
	    printf ("LB_read failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if (Test_data (TEST_DATA, buf, length) < 0)
	    return (-1);
    }
    for (i = 20; i < 32; i++) {
	if ((ret = LB_read (fd, (char *)buf, nbytes, i)) != nbytes) {
	    printf ("LB_read failed (ret = %d, errno = %d) - rss_test\n", 
					ret, errno);
	    return (-1);
	}
	if (Test_data (TEST_DATA, buf, length) < 0)
	    return (-1);
    }
    if ((ret = LB_read (fd, (char *)buf, 100, 32)) != LB_NOT_FOUND) {
	printf ("LB_read failed (ret = %d, should be %d, errno = %d) - rss_test\n", 
					ret, LB_NOT_FOUND, errno);
	return (-1);
    }
 
    return (0);
}

/****************************************************************

    Description: This function generate and verifies the test
	data.

    Input:	sw - a function switch: SET_DATA or TEST_DATA
		t_data - the test data
		length - test data length

    Returns: 0 if data is correct or -1 if an error is found.

****************************************************************/

static int Test_data (int sw, int *t_data, int length)
{
    int i;

    if (sw == SET_DATA) {
	for (i = 0; i < length; i++)
	    t_data [i] = i;
	return (0);
    }
    else {
	int st;

	st = t_data [0];
	for (i = 0; i < length; i++) {
	    if (t_data [i] != st + i) {
		printf ("An error found in test data - rss_test\n");
		return (-1);
	    }
	}
	return (0);
    }
}






