/*************************************************************************

      Module: MISC_rsrc.c

 Description:

	Miscellaneous Services "wrapper" resource-management functions.

    Resource-management functions are typically (at present) non-POSIX,
    so we have decided to "wrap" these functions.

	Functions that are public are defined in alphabetical order at
	the top of this file and are identified with a prefix of
	"MISC_rsrc_".

	Functions that are private to this file are defined in alphabetical
	order, following the definition of the public functions.


 Assumptions:

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/30 16:55:16 $
 * $Id: misc_rsrc.c,v 1.11 2012/07/30 16:55:16 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>            /* strcat()                                */
#include <stdarg.h> 
#include <time.h> 
#include <unistd.h>

#include <misc.h>
#include <cs.h>                /* CS_NAME                                 */
#include <en.h>              

                               /* The placement of these sytem header file*/
                               /* #include's is important: otherwise      */
                               /* we have Solaris compiler problems       */
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#include <sys/resource.h>      /* getrlimit(), setrlimit(), struct rlimit */
#define _POSIX_C_SOURCE
#else
#include <sys/resource.h>      /* getrlimit(), setrlimit(), struct rlimit */
#endif

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#ifdef LINUX
/* typedef long int rlim_t ; */
#endif
#define TMP_BUF_SIZE 1024

/*
 * External Globals
 */

/*
 * Static Globals
 */

/*
 * Static Function Prototypes
 */
static int Rm_trailing_slash_and_ret_strlen (char *str);

static int Misc_log_disable = 0;
static void (*Misc_log_callback) (char *) = NULL;


/********************************************************************

    Processes a log message for a library routine. Time stamp is added
    in front of the message. The message is sent to stderr or a caller
    registered callback function.	

********************************************************************/

void MISC_log (const char *format, ...) {
    va_list args;
    char buf[256];

    if (Misc_log_disable && EN_control (EN_GET_IN_CALLBACK) == 0)
	return;

    MISC_string_date_time (buf, 128, (const time_t *) NULL);
    strcat (buf, " ");
    va_start (args, format);
    vsprintf (buf + strlen (buf), format, args);
    if (Misc_log_callback != 0)
	Misc_log_callback (buf);
    else
	fprintf (stderr, buf);
    va_end (args);
}

/**************************************************************************

    Converts time "intime" to ASCII form and outputs it in buffer "date_time"
    of size "date_time_size".
 
 **************************************************************************/

void MISC_string_date_time (char *date_time,
                      int date_time_size, const time_t *intime) {
    time_t curtime;
    int day, hr, min, month, sec, year;
    char buf[128];

    if (intime == NULL)
        curtime = time (NULL) ;
    else
        curtime = *intime ;

    if (date_time == NULL)
	return;
    if (date_time_size > 0)
	date_time[0] = '\0';
    if (curtime == 0)
        return;

    unix_time (&curtime, &year, &month, &day, &hr, &min, &sec);
    sprintf (buf, "%02d/%02d/%02d %02d:%02d:%02d",
					month, day, year % 100, hr, min, sec);
    strncpy (date_time, buf, date_time_size);
    date_time[date_time_size - 1] = '\0';

    return;
}

/********************************************************************

    Registers a callback function to accept the log.	

********************************************************************/

void MISC_log_reg_callback (void (*log_cb)(char *)) {
    Misc_log_callback = log_cb;
}

/********************************************************************

    Disable/enable the log processing.	

********************************************************************/

void MISC_log_disable (int disable) {
    if (disable)
	Misc_log_disable++;
    else if (Misc_log_disable > 0)
	Misc_log_disable--;
}

/**************************************************************************
 Description: Get/Set "number of open files" resource (per-process).
       Input: requested resource limit
      Output: process resource limit may be changed
     Returns: (positive) resource limit upon success; otherwise one of the
              following negative values:

              MISC_RSRC_GET_ERR
              MISC_RSRC_SET_ERR

       Notes: The RLIMIT_NOFILE resource limit will be incremented iff.
              the requested resource limit exceeds the current resource
              limit.  Otherwise, we simply return the current resource
              limit.
 **************************************************************************/
int
MISC_rsrc_nofile(int nofile)
{
    struct rlimit rsrc_limit ; /* resource limits structure               */

    /*
     * Get the current resource limit ...
     */
    if (getrlimit(RLIMIT_NOFILE, &rsrc_limit) != 0) {
        return(MISC_RSRC_GET_ERR) ;
    }

    if ((unsigned int)nofile > (unsigned int) rsrc_limit.rlim_cur) {
        /*
         * Set the resource limit value ...
         */
        rsrc_limit.rlim_cur = nofile ;
        if (setrlimit(RLIMIT_NOFILE, (const struct rlimit *) &rsrc_limit)
                != 0) {
            return(MISC_RSRC_SET_ERR) ;
        }
    }

    return((int) rsrc_limit.rlim_cur) ;
}

/**********************************************************************

    Recursively expands all environmental variable ($(.)) in "str" and
    returns a pointer to the result. "buf" of "buf_size" bytes is a
    work buffer used by this funcion. If buf is too small or an error is
    detected, an error message is logged, buf[0] is set to null and "str"
    is returned. Missing env is not resolved.

***********************************************************************/

char *MISC_expand_env (const char *str, char *buf, int buf_size) {
    char *cr;
    int err, i;

    err = 0;
    if (buf_size > 0)
	buf[0] = '\0';
    cr = (char *)str;
    for (i = 0; i < 6; i++) {			/* do recursively */
	int len, recur;
	char rb[256];
	len = recur = 0;
	while (1) {
	    char b[256], *p, *pe, *env;
	    int elen;
    
	    p = cr;
	    while (*p != '\0') {
		if (*p == '$' && p[1] == '(')
		    break;
		p++;
	    }
	    if (*p == '\0')
		break;
	    pe = p + 2;
	    while (*pe != '\0' && *pe != ')') {
		if (*pe == '$' && pe[1] == '(') {	/* nested env found */
		    recur = 1;
		    break;
		}
		pe++;
	    }
	    if (err)
		break;
	    if (*pe == '\0') {
		err = 1;
		break;
	    }
	    else if (*pe == ')') {	/* an env found */
		if (pe - p - 2 >= 256) {
		    err = 1;
		    break;
		}
		memcpy (b, p + 2, pe - p - 2);
		b[pe - p - 2] = '\0';
		env = getenv (b);
		if (env == NULL) {
		    env = p;
		    elen = pe - p + 1;
		}
		else {
		    char *pp = env;
		    elen = strlen (env);
		    while (*pp != '\0') {
			if (*pp == '$' && pp[1] == '(')
			    break;
			pp++;
		    }
		    if (*pp != '\0')
			recur = 1;
		}
	    }
	    else {		/* a nested $( starts at pe */
		pe--;
		env = p;
		elen = pe - p + 1;
	    }
	    if ((p - cr) + elen + len >= buf_size) {
		err = 1;
		break;
	    }
	    memcpy (buf + len, cr, p - cr);
	    len += p - cr;
	    buf[len] = '\0';
	    memcpy (buf + len, env, elen);
	    len += elen;
	    buf[len] = '\0';
	    cr += (p - cr) + (pe - p + 1);
	}
	if (!err) {
	    buf[buf_size - 1] = '\0';
	    strncpy (buf + len, cr, buf_size - len);
	    if (buf[buf_size - 1] != '\0')
		err = 1;
	}
	if (err || !recur)
	    break;
	if (strlen (buf) >= 256) {
	    err = 1;
	    break;
	}
	strcpy (rb, buf);
	cr = rb;
    }
    if (i >= 6)		/* Infinite extansion must be avoided */
	err = 1;

    if (!err)
	return (buf);

    MISC_log ("MISC: Error found or buffer too small in MISC_expand_env\n");
    if (buf_size > 0)
	buf[0] = '\0';
    return ((char *)str);
}

/**********************************************************************

    Returns the working directory path in "buf" of size "buf_size".
    The return value is the size of the path on success or a negative 
    error code. If the directory does not exist, it is created.

***********************************************************************/

int MISC_get_work_dir (char *buf, int buf_size) {
    char tb[TMP_BUF_SIZE], *env;
    int len, ret;

    if ((env = getenv ("WORK_DIR")) != NULL) {
	len = strlen (env);
	if (len == 0)
	    return (MISC_RSRC_ENVS_UNDEFINED);
	if (len + 1 > TMP_BUF_SIZE)
	    return (MISC_RSRC_VAR_TOO_LARGE);
	strcpy (tb, env);
	len = Rm_trailing_slash_and_ret_strlen (tb);
    }

    else if ((env = getenv ("HOME")) != NULL) {
	if (strlen (env) + 5 > TMP_BUF_SIZE)
	    return (MISC_RSRC_VAR_TOO_LARGE);
	strcpy (tb, env);
	len = Rm_trailing_slash_and_ret_strlen (tb);
	(void) strcat (tb, "/tmp");
	len += 4;
    }
    else
	return (MISC_RSRC_ENVS_UNDEFINED);

    if (len >= buf_size)
	return (MISC_RSRC_BUF_TOO_SMALL);
    else
	strcpy (buf, tb);

    ret = MISC_mkdir (tb);
    if (ret < 0)
	return (ret);

    return (len);
}

/**********************************************************************

    Returns a unique path for temporary for directory or file in "buf" 
    of size "buf_size". The return value is the size of the path on 
    success or a negative error code.

***********************************************************************/

int MISC_get_tmp_path (char *buf, int buf_size) {
    static int cnt = 0;
    struct timeval time_val;
    char b[80];
    int len;

    if ((len = MISC_get_work_dir (buf, buf_size)) <= 0)
	return (len);
    gettimeofday (&time_val, NULL);
    sprintf (b, "/tmp%ld.%ld.%d.%d", time_val.tv_sec, 
				time_val.tv_usec, (int)getpid (), cnt);
    cnt++;
    len += strlen (b);
    if (len >= buf_size)
	return (MISC_RSRC_BUF_TOO_SMALL);
    strcat (buf, b);
    return (len);
}

/**
 Description: Returns the configuration directory path.
       Input: buf_size - size of the buffer for returning the path.
      Output: buf - the buffer for returning the path.
     Returns: (positive) path string length upon success; otherwise a 
		negative MISC_RSRC error number.
 */
int
MISC_get_cfg_dir (char *buf, int buf_size)
{
    char tb[TMP_BUF_SIZE], *env;
    int len;

    if ((env = getenv ("CFG_DIR")) != NULL) {
	len = strlen (env);
	if (len == 0)
	    return (MISC_RSRC_ENVS_UNDEFINED);
	if (len + 1 > TMP_BUF_SIZE)
	    return (MISC_RSRC_VAR_TOO_LARGE);
	strcpy (tb, env);
	len = Rm_trailing_slash_and_ret_strlen (tb);
    }
    else
	return (MISC_RSRC_ENVS_UNDEFINED);

    if (len + 1 > buf_size)
	return (MISC_RSRC_BUF_TOO_SMALL);
    else {
	strcpy (buf, tb);
	return (len);
    }

/*END of MISC_get_cfg_dir()*/
}



/**************************************************************************
 Description: Removes trailing "/" of a string, except the first char in
		the string and returns the string length.
       Input/Output: str - the string to be processed.
     Returns: the string length.
 **************************************************************************/

static int Rm_trailing_slash_and_ret_strlen (char *str)
{
    char *pt;
    int len;

    len = strlen (str);
    pt = str + len - 1;
    while (pt > str && *pt == '/') {
	*pt = '\0';
	pt--;
	len--;
    }
    return (len);

/*END of Rm_trailing_slash_and_ret_strlen()*/
}

/**************************************************************************

    Make directoty "path". This calls mkdir recursively to create the 
    dir. Returns 0 on success or -1 on failure.

 **************************************************************************/

int MISC_mkdir (char *path) {
    char buf[256], *p;

    if (mkdir (path, 0777) == 0 ||
	errno == EEXIST)
	return (0);
    if (errno != ENOENT ||
	strlen (path) >= 256)
	return (MISC_MKDIR_FAILED);
    strcpy (buf, path);
    p = buf + strlen (buf) - 1;
    while (p > buf && *p != '/')
	p--;
    if (p == buf)
	return (MISC_MKDIR_FAILED);
    *p = '\0';
    if (MISC_mkdir (buf) < 0)
	return (MISC_MKDIR_FAILED);
    return (MISC_mkdir (path));
}
