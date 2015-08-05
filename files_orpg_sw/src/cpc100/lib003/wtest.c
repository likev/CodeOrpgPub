
/*****************************************************************

	This is the same program as wtest.c in the LB module except
	that this one uses the RSS_LB module.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 1999/06/29 21:21:08 $
 * $Id: wtest.c,v 1.40 1999/06/29 21:21:08 jing Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>

/* #include <rss_replace.h> */
#include <rss.h>

#include <lb.h>
#include <misc.h>

static int Md = -1, Md1 = -1;
static int N_tests = 1000;

static char Name [256];
static int Flags;


static int test0 ();
static int test1 ();
static int test2 ();
static int test3 ();
static int test4 ();
static int test5 ();
static int test6 ();
static int test7 ();

static void Set_timer (int sec);
static void sigalrm_int();
static int Open_lb (int msg_size, int maxn_msgs);
static void Read_msg (int fd);
static int Write_msg (int fd, int len);


/****************************************************************
*/

void main (int argc, char **argv)
{
    int t_number;
    int remove;
    int t, i, ret;

    /* get arguments */
    Flags = 0;
    t_number = -1;
    remove = 0;
    strcpy (Name, "test.lb");
    for (i = 1; i < argc; i++) {
	if (sscanf (argv[i], "%d", &t) == 1) {
	    t_number = t;
	    continue;
	}
	if (strcmp ("-r", argv [i]) == 0) {
	    remove = 1;
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
	    
	if (strcmp ("LB_MEMORY", argv [i]) == 0) {
	    Flags |= LB_MEMORY;
	    continue;
	}
	if (strcmp ("LB_SINGLE_WRITER", argv [i]) == 0) {
	    Flags |= LB_SINGLE_WRITER;
	    continue;
	}
	if (strcmp ("LB_NOEXPIRE", argv [i]) == 0) {
	    Flags |= LB_NOEXPIRE;
	    continue;
	}
	if (strcmp ("LB_UNPROTECTED", argv [i]) == 0) {
	    Flags |= LB_UNPROTECTED;
	    continue;
	}
	if (strcmp ("LB_REPLACE", argv [i]) == 0) {
	    Flags |= LB_REPLACE;
	    continue;
	}
	if (strcmp ("LB_MUST_READ", argv [i]) == 0) {
	    Flags |= LB_MUST_READ;
	    continue;
	}
	if (strcmp ("LB_DIRECT", argv [i]) == 0) {
	    Flags |= LB_DIRECT;
	    continue;
	}

	printf ("Unexpected argument %s\n", argv [i]);
	    goto print_usage;
    }

    if (remove && (ret = LB_remove (Name)) < 0) {
	printf ("LB_remove failed (ret = %d)\n", ret);
	exit (-1);
    }

    if (t_number < 0) {
	if (remove == 0)
	    goto print_usage;
	else
	    exit (0);
    }

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
    printf ("Usage: wtest test_number options flags ...\n");
    printf ("       Options:\n");
    printf ("\
	       -r  remove the LB first [default: use existing LB];\n\
                   wtest -r can be used to remove the test LB.\n");
    printf ("\
	       -n  number of messages to test (tests 2, 3).\n");
    printf ("\
	       -lb  LB_name (default = 'test.lb').\n");
    printf ("\n");
    printf ("\
       test 0: Preliminary test - ASCII messages of small random \n\
               sizes and random ids are written to a small LB. The\n\
               messages can be verified by rtest 0. This test can be\n\
               performed with resetting POINTER_RANGE to 80\n\n");
    printf ("\
       test 1: Large message test - large messages of different sizes are \n\
               written to the LB. The message can be verified by rtest 1\n\n");
    printf ("\
       test 2: Speed test (Large messages) - large messages of different  \n\
               sizes are written to the LB without pause. No verification \n\
               is performed. The number of messages send is 1000, which  \n\
               can be reset by -n option\n\n");
    printf ("\
       test 3: Speed test (Small messages) - small messages of different  \n\
               sizes are written to the LB without pause. No verification \n\
               is performed. The number of messages send is 1000, which  \n\
               can be reset by -n option\n\n");
    printf ("\
       test 4: Interrupt test - The same as test 1 except that timer interrupt\n\
               are frequently added. One should run rtest 1 and verify the\n\
               messages.\n\n");
    printf ("\
       test 5: Replaceable LB test - This test should run with LB_REPLACE \n\
               flag. We should see some 'LB is full', and 'Bad message \n\
               length'. If we also run rtest 5, we can verify the \n\
               messages.\n\n");
    printf ("\
       test 6: LB_open, LB_close and LB_remove test - This test creates \n\
               large number of LBs and then removes them. There should be \n\
               no LBs left over after this test is finished. It also tests \n\
               how many LBs can be opened simultaneously.\n\n");
    printf ("\
       test 7: LB_read/LB_write and special LB size test - This test writes\n\
               to and reads messages from the same LB. It tests single \n\
               message and two-message LBs with critical message length. \n\
               This should be tested for both normal and replaceable \n\
               types. This is also a test for LB_DIRECT.\n\n");

    exit (0);
}


/**************************************************************************
	Preliminary Testing
*/

static int test0 ()
{
    int fd;
    static char message [256];
    int cnt;
    int utag;
    LB_id_t umid;

    /* open an LB for writing */
    fd = Open_lb (64, 15);

    LB_register (fd, LB_ID_ADDRESS, &umid);
    LB_register (fd, LB_TAG_ADDRESS, &utag);

/*    RSS_LB_compress (fd, 1);  */

    cnt = 0;
    while (1) {
        int ret;
	int rad;
	int len;
	char ct [64];

	rad = rand() % 10;

	sprintf (message, "This is a test message: %8d\n", cnt);
	sprintf (ct, "%8d", cnt);
	strncpy (message + rad, ct, 8);
	len = strlen (message + rad) + 1;    /* */

	utag = rand() % 10000;
        ret = LB_write (fd, message + rad, len, rad);

        if (ret == len) {   /* success */
	    printf ("Message sent (cnt = %d, id = %d, len %d)\n", cnt, rad, len); 
	    cnt++;
	    msleep (300); 
	    continue;
        }
	else if (ret == LB_FULL) {
	    printf ("Lb full\n");
	    msleep (300);
	    continue;
	}
        else {                     /* other errors */
	    printf ("LB_write failed. The return number = %d\n", ret);
	    exit (-1);
        }
    }

}

/**************************************************************************
	Testing large messages
*/

#define LARGE_MSG_SIZE 100000

static int test1 ()
{
    int fd;
    static char message [LARGE_MSG_SIZE];
    int cnt;

    printf ("Writing large messages:\n");

    /* open an LB for writing. create it if needed. */
    fd = Open_lb (30000, 10);

    for (cnt = 0; cnt < LARGE_MSG_SIZE; cnt++) 
	message[cnt] = (char) (cnt % 256);

    cnt = 0;
    while (1) {
        int ret;
	int len;

	len = 60001;
	if (cnt % 5 == 1) len = 20003;
	if (cnt % 5 == 4) len = 3103;
	if (cnt % 5 == 3) len = 7401;
	if (cnt % 5 == 2) len = 111;

        ret = LB_write (fd, message, len, LB_ANY);

        if (ret > 0) {   /* success */
	    printf ("Message sent: %d. len = %d\n", cnt, len);
	    msleep (2000); 
	    cnt++;
	    continue;
        }
    
        if (ret == LB_FULL) {    /* LB is full. We will retry */
	    printf ("LB is full\n");
	    msleep (20); 
	    continue;
        }

        else {                     /* other errors */
	    printf ("LB_write failed. The return number = %d\n", ret);
	    exit (-1);
        }
    }
}

/**************************************************************************
	Testing speed large messages
*/

#define LARGE_MSG_SIZE 100000

static int test2 ()
{
    int fd;
    static char message [LARGE_MSG_SIZE];
    int cnt;
    int nbytes, dtime;

    /* open an LB for writing. create it if needed. */
    fd = Open_lb (30000, 100);

    cnt = 0;
    dtime = time (NULL);
    nbytes = 0;
    printf ("Sending %d large messages ...\n", N_tests);
    while (1) {
        int ret;
	int len;

	if (cnt >= N_tests) {
	    printf ("%d bytes transferred in %d seconds\n", nbytes, 
			time(NULL) - dtime);
	    exit (0);
	}

	len = 60000;
	if (cnt % 5 == 4) len = 10003;
	if (cnt % 5 == 4) len = 30103;
	if (cnt % 5 == 3) len = 4801;
	if (cnt % 5 == 2) len = 44111;

        ret = LB_write (fd, message, len, LB_ANY);

        if (ret > 0) {   /* success */
	    nbytes += len;
	    cnt++;
	    continue;
        }
    
        if (ret == LB_FULL) {    /* LB is full. We will retry */
	    msleep (20); 
	    continue;
        }

        else {                     /* other errors */
	    printf ("LB_write failed. The return number = %d\n", ret);
	    exit (-1);
        }
    }
}

/**************************************************************************
	Testing speed - small messages
*/


static int test3 ()
{
    int fd;
    static char message [512];
    int cnt;
    int nbytes, dtime;

    /* open an LB for writing. create it if needed. */
    fd = Open_lb (300, 500);

    cnt = 0;
    dtime = time (NULL);
    nbytes = 0;
    printf ("Sending %d small messages ...\n", N_tests);
    while (1) {
        int ret;
	int len;

	if (cnt >= N_tests) {
	    printf ("%d bytes transferred in %d seconds\n", nbytes, 
			time(NULL) - dtime);
	    exit (0);
	}

	len = 100;
	if (cnt % 5 == 4) len = 103;
	if (cnt % 5 == 3) len = 201;
	if (cnt % 5 == 2) len = 111;

        ret = LB_write (fd, message, len, LB_ANY);

        if (ret > 0) {   /* success */
	    nbytes += len;
	    cnt++;
	    continue;
        }
    
        if (ret == LB_FULL) {    /* LB is full. We will retry */
	    msleep (20); 
	    continue;
        }

        else {                     /* other errors */
	    printf ("LB_write failed. The return number = %d\n", ret);
	    exit (-1);
        }
    }
}


/**************************************************************************
	Testing interrupts
*/

static int test4 ()
{

    printf ("Testing interrupts\n");

    signal (SIGALRM, sigalrm_int); 
    Set_timer (20);  

    test1 ();
    return (0);
}

/**************************************************************************
	Test 5: Testing replaceable write
*/

static int test5 ()
{
    int fd;
    static char message [256];
    int cnt;
    static LB_id_t id [] = {23, 1256, 0, 12, 56, 11111111, 2444};

    printf ("Testing replaceable lb:\n");

    /* open an LB for writing. create it if needed. */
    fd = Open_lb (128, 4);

    strcpy (message, "This is the first test message $$$$$$$$$$");
    printf ("First write id = 2444 (ret = %d)\n", 
	LB_write (fd, message, strlen (message) + 1, 2444));

    cnt = 0;
    while (1) {
        int ret;
	int rad;
	int len;

	rad = rand() % 10;
	cnt++;

	sprintf (message, "This is a test message: %6d ........", cnt);
	if ((cnt % 5) == 0)
	    len = strlen (message + rad) + 1;    /* */
	else
	    len = strlen (message) + 1;

        ret = LB_write (fd, message, len, id[cnt % 6]);

        if (ret > 0) {   /* success */
	    printf ("Message sent (id = %d, len = %d): %d\n", id[cnt % 6], 
						len, cnt); 
	    msleep (300); 
	    continue;
        }
    
        if (ret == LB_FULL) {    /* LB is full. We will retry */
	    printf ("LB is full (id = %d)\n", id[cnt % 6]); 
	    msleep (300); 
	    continue;
        }
    
        if (ret == LB_LENGTH_ERROR) {  
	    printf ("Bad message length (id = %d, len = %d)\n", 
						id[cnt % 6], len); 
	    msleep (300); 
	    continue;
        }

        else {                     /* other errors */
	    printf ("LB_write failed. The return number = %d\n", ret);
	    exit (-1);
        }
    }
}

/**********************************************************************
	Testing create, open, close and remove.
*/

#define N_TEST 128

static int test6 ()
{
    LB_attr attr;
    char name [128];
    int fd, cnt, i, ret;
    int lbd [N_TEST];

    strcpy (attr.remark, "This is a test LB");
    attr.mode = 0666;
    attr.msg_size = 128;
    attr.maxn_msgs = 4;
    attr.types = Flags;

    printf ("Creating %d LBs ...\n", N_TEST);
    for (i = 0; i < N_TEST; i++) {

	sprintf (name, "%s_%d", Name, i);

	ret = LB_remove (name);
	if (ret < 0) {
	    printf ("LB_remove failed (ret = %d)\n", ret);
	    exit (0);
	}

	fd = LB_open (name, LB_CREATE, &attr);
	if (fd < 0) {
	    printf ("LB_open (create) failed (ret = %d)\n", fd);
	    break;
	}
	LB_close (fd);
    }
    cnt = i;
    printf ("%d LB created\n", cnt);

    printf ("Testing how many LB can be opened ... \n");
    for (i = 0; i < N_TEST; i++) {

	sprintf (name, "%s_%d", Name, i);

	lbd [i] = LB_open (name, LB_WRITE, NULL);
	if (lbd [i] < 0) {
	    printf ("LB_open failed (ret = %d)\n", lbd [i]);
	    printf ("This failure should be (-43 - all file descriptors \n");
	    printf ("    are used or -40 - LB_TOO_MANY_OPENED)\n");
	    break;
	}
    }
    printf ("%d LB opened simultaneously\n", i);

    printf ("We close them ... \n");
    /* we must close some otherwise the later test will fail since now new 
       LB can be opened */
    for (i = 0; i < N_TEST; i++)
	LB_close (lbd [i]);

    printf ("We remove them ... \n");
    for (i = 0; i < cnt; i++) {

	sprintf (name, "%s_%d", Name, i);

	ret = LB_remove (name);
	if (fd < 0) {
	    printf ("LB_remove failed (name = %s, ret = %d)\n", name, fd);
	    break;
	}
    }
    printf ("%d LB removed\n", i);

    printf ("We LB_open/LB_close an LB 1000 times and see if there are leaks ... \n");
    for (i = 0; i < 1000; i++) {

	fd = LB_open (Name, LB_CREATE, &attr);
	if (fd < 0) {
	    printf ("LB_open failed (ret = %d)\n", fd);
	    break;
	}
	ret = LB_close (fd);
	if (fd < 0) {
	    printf ("LB_close failed (ret = %d)\n", ret);
	    break;
	}
    }

    exit (0);
}

/**********************************************************************
*/

static void sigalrm_int()
{
    static int cnt = 0;
    int i, buf [100];

    signal (SIGALRM, sigalrm_int); 

    /* waist some time */
    for (i = 0; i < 1000; i++)
	buf [i] = buf [i] / (i + 3);

    if (cnt % 10 == 0) printf ("timer called %d times\n", cnt);
    cnt++;

    return;
}

/******************************************************************

	Set_timer ()			Date: 2/16/94

	This function sets the UNIX timer. It causes UNIX to send 
	back a SIGALRM signal every "sec" seconds if sec > 0. It 
	stops the timer if sec = 0.

	Returns: This function has no return value.
*/

static void
  Set_timer
  (
      int sec			/* The timer period in seconds */
) {
    struct itimerval value;

    value.it_value.tv_sec = 0;
    value.it_interval.tv_sec = 0;
    value.it_value.tv_usec = sec * 1000;
    value.it_interval.tv_usec = sec * 1000;
    setitimer (ITIMER_REAL, &value, NULL);

    return;
}

/************************************************************************

*/

static int Open_lb (int msg_size, int maxn_msgs)
{
    LB_attr attr;
    int fd;

    fd = LB_open (Name, LB_WRITE, NULL);
    if (fd < 0) {
	LB_attr attr;

	if (fd == LB_TOO_MANY_WRITERS) {
	    printf ("Another writer is writing the LB\n");
	    exit (0);
	}
	printf ("LB_open failed (ret = %d) - we try to create ... \n", fd);   

	strcpy (attr.remark, "This is a test LB");
	attr.mode = 0666;
	attr.msg_size = msg_size;
	attr.maxn_msgs = maxn_msgs;
	attr.types = Flags;
	attr.tag_size = 0;
	fd = LB_open (Name, LB_CREATE, &attr);
	if (fd < 0) {
	    printf ("LB_open (create) failed (ret = %d)\n", fd);
	    exit (-1);
	}
	printf ("LB created: size = %d, nmsgs = %d, type flags = %d\n", 
			attr.msg_size, attr.maxn_msgs, attr.types);
    }

    return (fd);
}

/**************************************************************************
	Testing critical message and LB sizes
*/

static int test7 ()
{
    int fd;
    static char org_msg [] = "This is a test message 123456789123456789\n";
    static char message [256];
    char tmp [64];
    LB_id_t id;
    int i;

    printf ("Test single message LB\n");
    /* open an LB for writing */
    fd = Open_lb (31, 1);

    for (i = 0; i < 5; i++) {
	if (Write_msg (fd, 31) < 0)
	    break;
	Read_msg (fd);
    }

    for (i = 0; i < 3; i++) {
	if (Write_msg (fd, 32) < 0)
	    break;
	Read_msg (fd);
    }

    for (i = 0; i < 33; i++) {
	if (Write_msg (fd, 30) < 0)
	    break;
	Read_msg (fd);
    }

    LB_close (fd);

    printf ("Test two-message LB\n");
    /* open an LB for writing */
    fd = Open_lb (31, 2);

    for (i = 0; i < 5; i++) {
	if (Write_msg (fd, 31) < 0)
	    break;
	Read_msg (fd);
    }

    for (i = 0; i < 33; i++) {
	if (Write_msg (fd, 32) < 0)
	    break;
	Read_msg (fd);
    }

    for (i = 0; i < 33; i++) {
	if (Write_msg (fd, 30) < 0)
	    break;
	Read_msg (fd);
    }

    return (0);
}

/********************************************************************

*/

static int Write_msg (int fd, int len)
{
    static char org_msg [] = "This is a test message 123456789123456789\n";
    char message [256];
    static int cnt = 0;
    char tmp [64];
    static LB_id_t id = 0;
    int ret;

    if (Flags & LB_REPLACE)
	id = 35;
    else
	id++;

    strcpy (message, org_msg);
    sprintf (tmp, "%6d", cnt);
    strncpy (message + 11, tmp, 6);
    message [len - 1] = '\0';
    cnt++;

    ret = LB_write (fd, message, len, id);

    if (ret == LB_MSG_TOO_LARGE) {
	printf ("LB_write: message is too large\n");
	return (-1);
    }

    if (ret == LB_LENGTH_ERROR) {
	printf ("LB_write: message length error\n");
	return (-1);
    }

    if (ret <= 0) {
	printf ("LB_write failed (ret = %d)\n", ret);
 	exit (-1);
    }

    return (0);
}

/******************************************************************
*/

static void Read_msg (int fd)
{
    char message [256];
    char *msg;
    int ret;

    msg = message;
    if (Flags & LB_REPLACE)
	ret = LB_read (fd, message, 256, 35);
    else if (Flags & LB_DIRECT)
	ret = LB_direct (fd, &msg, LB_NEXT);
    else
	ret = LB_read (fd, message, 256, LB_NEXT);

    if (ret < 0) {
	printf ("LB_read failed (ret = %d)\n", ret);
 	exit (-1);
    }

    if (ret > 0)
	printf ("Msg (len = %d) read: %s\n", ret, msg);

    return;
}
