/******************************************************************

      This is the same program as rtest.c in the LB module except
       that this one uses the RSS_LB module.
 
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 1999/06/29 21:20:50 $
 * $Id: rtest.c,v 1.40 1999/06/29 21:20:50 jing Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/* #include <rss_replace.h> */
#include <rss.h>

#include <lb.h>
#include <misc.h>


static char Name [256];
static int N_tests = 1000;

static void host_exit ();

static int test0 ();
static int test1 ();
static int test2 ();
static int test3 ();
static int test4 ();
static int test5 ();
static int test6 ();
static int test7 ();

static int Check_msg (char *msg);
static void Print_list (int fd);


/*********************************************************************
*/

void main (int argc, char **argv)
{
    int t_number;
    int i, t;

    /* get arguments */
    t_number = -1;
    strcpy (Name, "test.lb");
    for (i = 1; i < argc; i++) {
	if (sscanf (argv[i], "%d", &t) == 1) {
	    t_number = t;
	    continue;
	}

	if (strcmp ("-h", argv [i]) == 0) {
	    goto print_usage;
	    continue;
	}
	if (strcmp ("-n", argv [i]) == 0) {
	    if (sscanf (argv[i + 1], "%d", &t) != 1)
		goto print_usage;
	    N_tests = t;
	    i++;
	    continue;
	}
	 if (strcmp ("-lb", argv [i]) == 0) {
	    if (sscanf (argv[i + 1], "%s", Name) != 1)
		goto print_usage;
	    i++;
	    continue;
	}
   
	printf ("Unexpected argument %s\n", argv [i]);
	    goto print_usage;
    }
    if (t_number < 0)
	goto print_usage;

    /* catch the signals */
/*
    signal (SIGTERM, host_exit);
    signal (SIGHUP, host_exit);
    signal (SIGINT, host_exit); 
*/

    switch (t_number) {

	case 0:
	test0 ();
	break;

	case 1:
	test1 ();
	break;

	case 2:
	test2 ();
	break;

	case 3:
	test3 ();
	break;

	case 4:
	test4 ();
	break;

	case 5:
	test5 ();
	break;

	case 6:
	test6 ();
	break;

	case 7:
	test7 ();
	break;

	default:
	printf ("test %d not implemented\n", t_number);
	break;
    }

    exit (0);

print_usage:
    printf ("Usage: rtest test_number options ...\n");
    printf ("       Options:\n");
    printf ("\
	       -n  number of messages to test (tests 2, 3 and 6).\n");
    printf ("\
	       -lb  LB_name (default = 'test.lb').\n");
    printf ("\n");
    printf ("\
       test 0: Preliminary test - ASCII messages are read from the LB.\n\
               The messages are verified, assuming they are written by \n\
               wtest 0, and printed on the standard output\n\n");
    printf ("\
       test 1: Large message test - Binary messages are read from the LB.\n\
               The messages are verified, assuming they are written by \n\
               wtest 1\n\n");
    printf ("\
       test 2: Speed test - messages are read without pause. No \n\
               verification is performed. The number of messages read is\n\
               1000, which can be reset by -n option\n\n");
    printf ("\
       test 3: LB_list, LB_mark and random read test - Applies to LB\n\
               created by wtest 0 or wtest 5 only because it prints\n\
               the message. It alternatively sets and resets the mark on\n\
               every the other messages. Note that the message printed out\n\
               and mark setting in this test are based on message ids (not\n\
               in sequence). Thus they may be complicated to understand if\n\
               the LB is created by wtest 0, which uses non-unique ids.\n\n");
    printf ("\
       test 4: LB_clear test - This clears and lists, step by step, all \n\
               messages. This is also a test for LB_list.\n\n");
    printf ("\
       test 5: Replaceable LB test - This test should run with wtest 5.\n\
               LB_read is applied to a list of message ids. Some should\n\
               be 'message not found'\n\n");
    printf ("\
       test 6: LB_stat and LB_size test - This first reads and prints out \n\
               general LB attributes including the size and then enters a\n\
               loop printing the update status. To verify individual message\n\
               update status one can run 'wtest 5 LB_REPLACE' \n\
               simultaneously. If -n N_tests with N_tests >= 10000, this\n\
               will perform a LB_stat speed test\n\n");
    printf ("\
       test 7: LB_seek test - This test moves the read pointer and read the \n\
               message for several typical LB_seek arguments and exists.\n\n");
    exit (0);
}


/********************************************************************
	Initial Test
*/


#include <signal.h>
#include <sys/time.h>


static int test0 ()
{
    int fd;
    char *message;
    int cnt;
    extern int errno;
    int utag;
    LB_id_t id;
    LB_wait_list_t wlist[2];

    /* open an LB for reading */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d, errno = %d)\n", fd, errno);
	exit (-1);
    }

    LB_register (fd, LB_TAG_ADDRESS, &utag);
    LB_register (fd, LB_ID_ADDRESS, &id);

/*
    LB_seek (fd, 0, LB_FIRST, NULL);
    LB_seek (fd1, 0, LB_FIRST, NULL);
*/

    cnt = 0;
    while (1) {
	int ret;
	LB_info info;
	LB_status status;
	LB_check_list list[5];

	status.attr = NULL;
	status.n_check = 4;
	status.check_list = list;
	list[0].id = 4;
	list[1].id = 5;
	list[2].id = 6;
	list[3].id = 7;
	LB_stat (fd, &status);

	wlist[0].fd = fd;
 	ret = LB_wait (1, wlist, 10000);
	printf ("LB_wait ret %d,   %d\n", ret, wlist[0].status);

	status.n_check = 0;
	ret = LB_stat (fd, &status);
	printf ("%d   %d %d %d %d\n", status.updated, list[0].status, 
			list[1].status, list[2].status, list[3].status);

	ret = LB_read (fd, &message, LB_ALLOC_BUF, list[0].id);
	printf ("read msg 0 ret %d\n", ret);
	ret = LB_read (fd, &message, LB_ALLOC_BUF, list[1].id);
	printf ("read msg 1 ret %d\n", ret);
	ret = LB_read (fd, &message, LB_ALLOC_BUF, list[2].id);
	printf ("read msg 2 ret %d\n", ret);
	ret = LB_read (fd, &message, LB_ALLOC_BUF, list[3].id);
	printf ("read msg 3 ret %d\n", ret);

	ret = LB_stat (fd, &status);
	printf ("AFTER %d   %d %d %d %d\n", status.updated, list[0].status, 
			list[1].status, list[2].status, list[3].status);

    }

}

/********************************************************************

*/

static int Check_msg (char *msg)
{
    int n1, n2;

    if (sscanf (msg, "%d", &n1) != 1 ||
	sscanf (msg + strlen (msg) - 8, "%d", &n2) != 1 ||
	n1 != n2) {
	printf ("message error!\n");
	return (-1);
    }
    return (0);
}

/********************************************************************
	Testing large messages
*/

#define LARGE_MSG_SIZE 100000

static int test1 ()
{
    int fd;
    static char message [LARGE_MSG_SIZE];
    static char true_message [LARGE_MSG_SIZE];
    int cnt;

    for (cnt = 0; cnt < LARGE_MSG_SIZE; cnt++) 
	true_message[cnt] = (char)(cnt % 256);

    /* open an LB for reading. */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    cnt = 0;
    while (1) {
	int ret;
	int i;

	ret = LB_read (fd, message, LARGE_MSG_SIZE, LB_NEXT);

	if (ret >= 0) {  /* success */
	    printf ("Message read (len = %d)\n", ret); 
    	    for (i = 0; i < ret; i++) {
		if (true_message[i] != message[i]) {
		    printf ("error found at %d. cnt = %d\n", i, cnt);
		    exit (0);
		}
	    }
	    cnt++;
	    continue;
	}

	if (ret == LB_TO_COME) {    /* LB is empty. We will retry */
	    msleep (1000);  
	    continue;
	}

	else {                     /* other errors */
	    printf ("LB_read failed. The return number = %d\n", ret);
	    exit (-1);
	}
    }
}

/********************************************************************
	Testing speed
*/

#define LARGE_MSG_SIZE 100000

static int test2 ()
{
    int fd;
    static char message [LARGE_MSG_SIZE];
    int cnt;
    int nbytes, dtime;

    /* open an LB for reading. */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    cnt = 0;
    dtime = time (NULL);
    nbytes = 0;
    printf ("Reading %d messages ...\n", N_tests);
    while (1) {
	int ret;
	int i;

	if (cnt >= N_tests) {
	    printf ("%d bytes read in %d seconds\n", nbytes, 
			time(NULL) - dtime);
	    exit (0);
	}

	ret = LB_read (fd, message, LARGE_MSG_SIZE, LB_NEXT);

	if (ret >= 0) {  /* success */
	    nbytes += ret;
	    cnt++;
	    continue;
	}

	if (ret == LB_TO_COME) {    /* LB is empty. go back */
	    LB_seek (fd, 0, LB_FIRST, NULL);  
	    continue;
	}

	if (ret == LB_EXPIRED) {    /* expired, this will advance the pointer */
	    continue;
	}

	else {                     /* other errors */
	    printf ("LB_read failed. The return number = %d\n", ret);
	    exit (-1);
	}
    }
}

/********************************************************************
	Testing LB_list and random read
*/

static int test3 ()
{
    int fd;
    char message[256];
    int cnt;

    /* open an LB for reading */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    cnt = 0;
    message [255] = 0;
    while (1) {
	int ret, nlist, this;
	LB_info info [32];

	cnt++;
	nlist = LB_list (fd, info, 32);

	if (nlist >= 0) {
	    int i;

	    for (i = 0; i < nlist; i++) {
		printf ("list %d: id = %d, size = %d, mark = %d\n", 
			i, info [i].id, info [i].size, info [i].mark);

		ret = LB_read (fd, message, 254, info [i].id);
		if (ret >= 0 && ret != info [i].size)
		    printf ("message size error (%d %d)\n", ret, info [i].size);
		if (ret >= 0)
		    printf ("message read: %s (len = %d, id = %d)\n", 
					message, ret, info [i].id);
		if (ret < 0)
		    printf ("read error (ret = %d)\n", ret);

		if ((i % 2) == 0) {
		    ret = LB_mark (fd, info [i].id, (cnt % 2));
		    if (ret < 0)
			printf ("LB_mark failed (ret = %d)\n", ret);
		}
	    }
	}
	else {
	    printf ("LB_list failed (ret = %d)\n", nlist);
	    exit (-1);
	}

	msleep (2000);
    }	
}

/********************************************************************
	Initial LB_clear
*/

static int test4 ()
{
    int fd;
    int ret;

    /* open an LB for reading */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    Print_list (fd);

    /* This should fail */
    ret = LB_clear (fd, LB_ALL);
    printf ("This LB_clear should fail (ret = %d)\n", ret);

    LB_close (fd);
    /* open an LB for writing */
    fd = LB_open (Name, LB_WRITE, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    printf ("Clear 1 message ... (ret = %d)\n", LB_clear (fd, 1));
    Print_list (fd);

    printf ("Clear 2 message ... (ret = %d)\n", LB_clear (fd, 2));
    Print_list (fd);

    printf ("Clear all message ... (ret = %d)\n", LB_clear (fd, LB_ALL));
    Print_list (fd);

    printf ("Clear 1 message ... (ret = %d)\n", LB_clear (fd, 1));

    exit (0);
}

/************************************************************************

*/

static void Print_list (int fd)
{
    int nlist;
    LB_info info [32];

    nlist = LB_list (fd, info, 32);

    if (nlist >= 0) {
	int i;

	for (i = 0; i < nlist; i++) {
	    printf ("list %d: id = %d, size = %d, mark = %d\n", 
			i, info [i].id, info [i].size, info [i].mark);
	}
    }
    else 
	printf ("LB_list failed (ret = %d)\n", nlist);

    return;
}

/********************************************************************
	Testing replaceable read
*/

static int test5 ()
{
    int fd;
    char message [256];
    int cnt;
    static LB_id_t id [] = {23, 1256, 0, 12, 56, 11111111, 2444};

    /* open an LB for reading */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    cnt = 0;
    while (1) {
	int ret, i;
	LB_info info;

	ret = LB_seek (fd, 0, LB_FIRST, &info);
	printf ("LB_seek: ret = %d, first id = %d\n", ret, info.id);

	for (i = 0; i < 7; i++) {
	    ret = LB_read (fd, message, 128, id [i]);

	    if (ret >= 0) {  /* success */
		message[ret - 1] = '\0';
		printf ("Message read (len = %d, id = %d): %s\n", 
						ret, id [i], message); 
		cnt++;
		continue;
	    }

	    else if (ret == LB_NOT_FOUND) {
		printf ("message not found (id = %d)\n", id [i]);
		continue;
	    }

	    else {                     /* other errors */
		printf ("LB_read failed. The return number = %d\n", ret);
		exit (-1);
	    }
	}

	msleep (3000);
    }	
}

/********************************************************************
	Testing LB_stat
*/

static int test6 ()
{
    int fd, ret, i;
    LB_attr attr;
    LB_check_list list[7];
    LB_status stat;
    static LB_id_t id [] = {23, 1256, 0, 12, 56, 11111111, 2444};

    /* open an LB for reading */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    printf ("LB size = %d\n", LB_misc (fd, LB_GET_LB_SIZE));

    printf ("Simple LB_stat test ... \n");
    stat.attr = NULL;
    stat.n_check = 0;
    ret = LB_stat (fd, &stat);
    printf ("LB_stat ret = %d, LB time = %d\n", ret, stat.time);
    printf ("        updated = %d (should be 1)\n", stat.updated);

    if (N_tests >= 10000) { 		/* testing speed */
	int st_time;

	printf ("Testing LB_stat speed ... \n");
	st_time = time (NULL);
	for (i = 0; i < N_tests; i++)
	    LB_stat (fd, &stat);
	printf ("%d LB_stat calls use %d seconds\n", 
				N_tests, time (NULL) - st_time);
	exit (0);
    }

    printf ("LB_stat test - list attributes and other fields ... \n");
    stat.attr = &attr;
    stat.n_check = 0;
    ret = LB_stat (fd, &stat);
    printf ("LB_stat ret = %d\n", ret);
    printf ("      remark = %s\n", attr.remark);
    printf ("      mode = %d\n", attr.mode);
    printf ("      msg_size = %d\n", attr.msg_size);
    printf ("      maxn_msgs = %d\n", attr.maxn_msgs);
    printf ("      types = %d\n", attr.types);
    printf ("      \n");
    printf ("      n_msgs  = %d\n", stat.n_msgs);
    printf ("      time  = %d\n", stat.time);
    printf ("      updated  = %d\n", stat.updated);
    printf ("      n_check  = %d\n", stat.n_check);
    printf ("      \n");

    stat.n_check = 7;
    for (i= 0; i < 7; i++)
	list[i].id = id [i];
    stat.check_list = list;

    while (1) {

	ret = LB_stat (fd, &stat);
	if (ret < 0) {
	    printf ("LB_stat failed (ret = %d)\n", ret);
	    exit (0);
	}

	printf ("updated = %d\n", stat.updated);
	printf ("id     = %8d %8d %8d %8d %8d %8d %8d\n", id [0], id [1], 
				id [2], id [3], id [4], id [5], id [6]);

	printf ("status = %8d %8d %8d %8d %8d %8d %8d\n", list [0].status, 
			list [1].status, list [2].status, list [3].status, 
			list [4].status, list [5].status, list [6].status); 

	sleep (4);
    }
}

/********************************************************************
	Testing seek
*/

static int test7 ()
{
    int fd;
    static char message [128];
    int ret;
    LB_info info;

    /* open an LB for reading. */
    fd = LB_open (Name, LB_READ, NULL);
    if (fd < 0) {               /* open failed */
	printf ("LB_open failed (ret = %d)\n", fd);
	exit (-1);
    }

    Print_list (fd);
    printf ("\n");

    printf ("LB_seek to the latest message and read ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, 0, LB_LATEST, &info), 
							info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    if (ret >= 0) {
	message [127] = 0;
	printf ("message read (len = %d): %s\n", ret, message);
    }

    printf ("LB_seek to the previous message and read ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, -1, LB_CURRENT, &info), 
							info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    if (ret >= 0) {
	message [127] = 0;
	printf ("message read (len = %d): %s\n", ret, message);
    }
 
    printf ("LB_seek to the above message using id and read ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, 0, info.id, &info), info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    if (ret >= 0) {
	message [127] = 0;
	printf ("message read (len = %d): %s\n", ret, message);
    }

    printf ("LB_seek to the message after the second and read ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, 2, LB_FIRST, &info), info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    if (ret >= 0) {
	message [127] = 0;
	printf ("message read (len = %d): %s\n", ret, message);
    }
   
    printf ("LB_seek to vary back (-43567839) ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, -43567839, LB_CURRENT, &info), info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    printf ("read return = %d (should be %d)\n\n", ret, LB_EXPIRED);
   
    printf ("LB_seek to very forward (143567839) ...\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, 143567839, LB_CURRENT, &info), info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    printf ("read return = %d (should be %d)\n\n", ret, LB_TO_COME);
   
    printf ("LB_seek to the old place (-100000002) ...\n");
    printf ("We must move back two more messages since there was a successful\n");
    printf ("read and an expired read - both advance the read pointer.\n");
    printf ("LB_seek ret = %d, id = %d\n", LB_seek (fd, -100000002, LB_CURRENT, &info), info.id);
    ret = LB_read (fd, message, 128, LB_NEXT);
    if (ret >= 0) {
	message [127] = 0;
	printf ("message read (len = %d): %s\n", ret, message);
    }

    exit (0);
}

