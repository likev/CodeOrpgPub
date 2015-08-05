/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/01/22 21:34:20 $
 * $Id: misc_msleep.c,v 1.9 2007/01/22 21:34:20 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <config.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/times.h>

#include <misc.h>

#ifdef IBM
#include <values.h>
typedef struct sellist {int fdsmask [2];} fd_set;
#define FD_ZERO(a) ((a)->fdsmask[0] = (a)->fdsmask[1] = 0)
#define FD_SET(a,b) ((b)->fdsmask[a / BITS(int)] |= (1 << (a % BITS(int))))
#endif

/**********************************************************************

    Returns the current system time in seconds. The time is independent
    of date and time setting. The return value of the first call of this
    is machine's clock time. This function must be called once in no less
    than 6 months to maintain consistent results.

**********************************************************************/

time_t MISC_systime (int *ms) {
    static unsigned int prev_t = 0, t_max = 0x7fffffff, prev_s = 0, prev_fs = 0, hz = 0;
    static int init = 0x7fffffff;
    static struct timeval prev_tv;
    unsigned int t, dt, s, fs;
    struct timeval tv;

#ifdef LINUX
    t = (unsigned int)times (NULL);
#else
    struct tms buf;
    t = (unsigned int)times (&buf);
#endif

    if (gettimeofday (&tv, NULL) < 0) {
	MISC_log ("gettimeofday failed\n");
	exit (1);
    }
    if (hz == 0) {
	hz = sysconf (_SC_CLK_TCK);
	if ((clock_t)t == (clock_t)-1 || hz == 0 || sizeof (clock_t) != 4) {
	    MISC_log ("times failed\n");
	    exit (1);
	}
	prev_tv = tv;
    }
    if (t > 0x7fffffff)
	t_max = 0xffffffff;
    if (t < prev_t) {
	dt = t + (t_max - prev_t) + 1;
	if (dt > 10/* 4 * hz */) {
	    int d;
	    double diff = (double)tv.tv_sec + (double)tv.tv_usec * .000001 - 
		(double)prev_tv.tv_sec - (double)prev_tv.tv_usec * .000001;
	    if (diff < 0.)
		diff = 0.;
	    d = (int)((diff * (double)hz) + .5);
	    if (d < dt)
		dt = d;
	}
    }
    else {
	dt = t - prev_t;
    }
    prev_t = t;
    prev_tv = tv;

    s = prev_s + dt / hz;
    fs = prev_fs + dt % hz;
    if (fs >= hz) {
	s++;
	fs -= hz;
    }
    prev_s = s;
    prev_fs = fs;

    if (init == 0x7fffffff)
	init = tv.tv_sec - s;
    if (ms != NULL)
	*ms = fs * 1000 / hz;
    return ((time_t)(s + init));
}

#ifdef OLD_IMPLEMENTATION_for_reference

/**********************************************************************

    Returns the current system time in seconds. The time is independent
    of date and time setting. The return value of the first call of this
    is machine's clock time. This function must be called once in no less
    than 6 months to maintain consistent results.

**********************************************************************/

time_t MISC_systime (int *ms) {
    static unsigned int prev_t = 0, t_max = 0x7fffffff, page = 0, hz = 0;
    static int init = 0x7fffffff;
    unsigned int t, m;

#ifdef LINUX
    t = (unsigned int)times (NULL);
#else
    struct tms buf;
    t = (unsigned int)times (&buf);
#endif

    if (hz == 0) {
	hz = sysconf (_SC_CLK_TCK);
	if ((clock_t)t == (clock_t)-1 || hz == 0 || sizeof (clock_t) != 4) {
	    MISC_log ("times failed\n");
	    exit (1);
	}
    }
    if (t > 0x7fffffff)
	t_max = 0xffffffff;
    if (t < prev_t)
	page++;
    prev_t = t;
    if (page == 0)
	t = t / hz;
    else
	t = page * (t_max / hz) + t / hz + (page * ((t_max % hz) + 1) + (t % hz)) / hz;
    if (init == 0x7fffffff)
	init = time (NULL) - t;
    if (ms != NULL) {
	if (page == 0)
	    m = prev_t % hz;
	else
	    m = (page * ((t_max % hz) + 1) + (prev_t % hz)) % hz;
	*ms = m * 1000 / hz;
    }
    return ((time_t)(t + init));
}
#endif

/**********************************************************************

    This is replaced by MISC_systime.

**********************************************************************/

time_t MISC_cr_time () {

    return (MISC_systime (NULL));
}

/**********************************************************************
my timer                                                         */

int
msleep (int ms)
{
    struct timeval timeout;
    fd_set fds;

    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = 1000 * (ms % 1000);

    select (0, &fds, &fds, &fds, &timeout);

    return (0);

}

/* microsecond sleep */
int MISC_usleep(long seconds, long microseconds)
{
  struct timeval timeout;
  fd_set fds;

  timeout.tv_sec  = seconds + microseconds / 1000000;
  timeout.tv_usec = microseconds % 1000000;

  select(0, (fd_set *)&fds, (fd_set *)&fds, (fd_set *)&fds, &timeout);

  return(0);
}

/********************************************************************
			
    Calls the write system call to write specified number of bytes. 
    It retries if the OS call is interrupted.

    Input:	fd - the file descriptor;
		buf - the message to write;
		wlen - number of bytes to write;

    Returns:	This function returns "wlen" or a negative LB error 
		number on failure.

********************************************************************/

int MISC_write (int fd, const char *buf, int wlen) {
    int cnt;

    cnt = 0;
    while (1) {
	int ret;
	ret = write (fd, buf + cnt, wlen - cnt);
	if (ret < 0) {
	    if (errno == EDQUOT || errno == ENOSPC)
		return (MISC_FILE_SYSTEM_FULL);
	    else if (errno == EINTR)
		continue;
	    else {
		MISC_log ("MISC_write: write failed (errno %d)\n", errno);
		return (MISC_WRITE_FAILED);
	    }
	}
	cnt += ret;
	if (cnt >= wlen)
	    return (wlen);
    }
}

/********************************************************************
			
    Calls the read system call to read specified number of bytes. 
    It retries if the OS call is interrupted.

    Input:	the same as read;

    Returns:	The same as read.

********************************************************************/

int MISC_read (int fd, char *buf, int rlen) {
    int cnt;

    cnt = 0;
    while (1) {
	int ret;
	ret = read (fd, buf + cnt, rlen - cnt);
	if (ret < 0) {
	    if (errno == EINTR)
		continue;
	    else {
		MISC_log ("MISC_read: read failed (errno %d)\n", errno);
		return (MISC_READ_FAILED);
	    }
	}
	cnt += ret;
	if (cnt >= rlen || ret == 0)
	    return (cnt);
    }
}

/********************************************************************
			
    Calls close system call. Repeats the call if interrupted.

    Input:	fd - the file descriptor;

    Returns:	This function returns the return value of close.

********************************************************************/

int MISC_close (int fd) {

    while (1) {
	int ret;
	ret = close (fd);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls open system call. Repeats the call if interrupted.

    Input:	The same as open;

    Returns:	The same as open.

********************************************************************/

int MISC_open (const char *path, int oflag, mode_t mode) {

    while (1) {
	int ret;
	ret = open (path, oflag, mode);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls unlink system call. Repeats the call if interrupted.

    Input:	The same as unlink;

    Returns:	The same as unlink.

********************************************************************/

int MISC_unlink (const char *path) {

    while (1) {
	int ret;
	ret = unlink (path);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls stat system call. Repeats the call if interrupted.

    Input:	The same as stat;

    Returns:	The same as stat.

********************************************************************/

int MISC_stat (const char *path, struct stat *buf) {

    while (1) {
	int ret;
	ret = stat (path, buf);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls fstat system call. Repeats the call if interrupted.

    Input:	The same as fstat;

    Returns:	The same as fstat.

********************************************************************/

int MISC_fstat (int fd, struct stat *buf) {

    while (1) {
	int ret;
	ret = fstat (fd, buf);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls system function call. Repeats the call if interrupted.

    Input:	The same as system;

    Returns:	The same as system.

********************************************************************/

int MISC_system (const char *string) {

    while (1) {
	int ret;
	ret = system (string);
	if (ret >= 0 || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls fopen function call. Repeats the call if interrupted.

    Input:	The same as fopen;

    Returns:	The same as fopen.

********************************************************************/

FILE *MISC_fopen (const char *filename, const char *mode) {

    while (1) {
	FILE *ret;
	ret = fopen (filename, mode);
	if (ret != NULL || errno != EINTR)
	    return (ret);
    }
}

/********************************************************************
			
    Calls fclose function call. Repeats the call if interrupted.

    Input:	The same as fclose;

    Returns:	The same as fclose.

********************************************************************/

int MISC_fclose (FILE *stream) {

    while (1) {
	int ret;
	ret = fclose (stream);
	if (ret == 0 || errno != EINTR)
	    return (ret);
    }
}
