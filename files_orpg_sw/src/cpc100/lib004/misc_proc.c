/*************************************************************************

    Module: misc_proc.c

    Miscellaneous process-related functions.

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:56 $
 * $Id: misc_proc.c,v 1.57 2012/06/14 18:57:56 jing Exp $
 * $Revision: 1.57 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>            /* system()                                */
#include <unistd.h>            /* getpid(), unlink()                      */
#include <time.h>

#include <sys/types.h>         /* closedir(), open(), opendir(),readdir() */
                               /* stat(), fcntl()                         */
#include <dirent.h>            /* closedir(), opendir(), readdir()        */
#include <sys/stat.h>          /* open(), stat()                          */
#include <fcntl.h>             /* fcntl(), open()                         */
#include <unistd.h>            /* read(), fcntl()                         */
#include <string.h>            /* strspn()                                */
#include <errno.h>            /*                                 */
#include <signal.h>
#include <dlfcn.h>

#if (defined (LINUX)) || (defined (__WIN32__))
#include <sys/procfs.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif
#if (defined (IRIX))
#include <sys/procfs.h>
#include <sys/wait.h>
#endif
#if (defined (SUNOS))
#include <procfs.h>
#endif

#include <misc.h>
#include <str.h>

#define MAX_N_ARGS 32
#define MAX_CMD_LENGTH 512
#define MAX_SH_LENGTH 64

#define PSINFO_PATHLEN 24     /* we need only enough space for           */
                               /* "/proc/######/psinfo"                   */
#define SYSTEM_CMDLEN   128
#define SYSTEM_CMDSIZ   ((SYSTEM_CMDLEN) + 1)
#define NAME_LEN 128

#define MISC_CP_BUFSIZE	 512
#define MISC_CP_ERR_FLAG "58361903528391"


typedef struct {
    char buf[MISC_CP_BUFSIZE];	/* buffers for reading pipe */
    int nbytes;			/* number of bytes in buf */
    int offset;			/* offset of starting byte */
} Cp_buf_t;

typedef struct {		/* coprocess struct */
    char *cmd;			/* coprocess command */
    short up;			/* The coprocess is up and running */
    short first_read;		/* No data been read after open */
    int in_fd;			/* coprocess stdin fd */
    int out_fd;			/* coprocess stdout fd */
    int err_fd;			/* coprocess stderr fd */
    Cp_buf_t out;		/* for stdout */
    Cp_buf_t err;		/* for stderr */
    pid_t pid;			/* coprocess pid */
    int status;			/* coprocess termination status */
} Cp_struct_t;

typedef struct {
    pid_t pid;
    pid_t ppid;
} Proc_t;

static int Sig_pipe_received;	/* SIGPIPE received - used by MISC_cp */

static int Setsid = 1;

static int Execute_cmd (const char *cmd);
static void Redirect_output (int *fd);
static int Read_output (int fd, int ofd, char *obuf, int bsize, 
							int pid, int *st);
static int Execute (char *cmd, char *ofile,
		char *obuf, int bsize, int *ret_cnt, int fg, int pp1);
static int Fork_execute (char *cmd, char *ofile,
			char *obuf, int bsize, int *n_bytes, int fg);
static void Close_files (int *opp, int fd, int pp1, int nfds);
static char *Get_token (char *str, char **next, int *len);
static int Read_pipe (int fd, char *buf, int buf_size);
static char **Parse_cmd (char *cmd, char **buf, int buf_size, 
					char **avb, int avb_size);
static int Read_cp_output (int fd, Cp_buf_t *rbuf, char *buf, int b_size);
static void Sig_pipe (int sig);
static int Close_fd_return (int *fd, Cp_struct_t *cp, int ret);
static int Rm_proc_and_children (pid_t rmpid, Proc_t *procs, int pcnt, int sig);static int Rm_decedent_processes (pid_t rmpid, int sig);


/******************************************************************

    This function is removed.

******************************************************************/

void MISC_system_shell (char *shell_cmd) {
    return;
}

/******************************************************************

    Turns on/off setsid call.

******************************************************************/

void MISC_system_setsid (int yes) {
    Setsid = yes;
}

/******************************************************************

    MISC_system with output piped into "obuf" of size "bsize". Total
    number of output bytes is returned in "n_bytes".

******************************************************************/

int MISC_system_to_buffer (char *cmd, char *obuf, int bsize, int *n_bytes) {
    char *cmd1, *ofile;
    int fg;
    int bg_found, redirect_found, len, ret;
    int redirect_count, bg_count;
    char dbuf[MAX_CMD_LENGTH], *cbuf;
    char *pt, *next;

    if (bsize > 0 && obuf != NULL)
	obuf[0] = '\0';
    if (n_bytes != NULL)
	*n_bytes = 0;

    if ((len = strlen (cmd)) >= MAX_CMD_LENGTH)
	cbuf = MISC_malloc (len + 1);
    else
	cbuf = dbuf;
    strcpy (cbuf, cmd);
    ret = MISC_SYSTEM_SYNTAX_ERROR;
    cmd1 = ofile = NULL;
    fg = 1;
    redirect_count = bg_count = 0;
    bg_found = redirect_found = 0;
    pt = Get_token (cbuf, &next, &len);
    while (1) {
	if (next == NULL)
	    goto syntax_err;
	if (pt == NULL)
	    break;

	if (*pt == '|' && len == 1) {
	    goto syntax_err;
	}
	else if (*pt == '&' && len == 1) {
	    *pt = '\0';
	    bg_found = 1;
	    bg_count++;
	    fg = 0;
	}
	else if (*pt == '>' && len == 1) {
	    *pt = '\0';
	    redirect_found = 1;
	    redirect_count++;
	}
	else if (redirect_found) {
	    if (cmd1 == NULL || ofile != NULL)
		goto syntax_err;
	    ofile = pt;
	    if (ofile[len] != ' ' && ofile[len] != '\0')
		goto syntax_err;
	    ofile[len] = '\0';
	    redirect_found = 0;
	}
	else if (bg_found)
	    goto syntax_err;
	else if (cmd1 == NULL)
	    cmd1 = pt;
	pt = Get_token (next, &next, &len);
    }
    if (cmd1 == NULL || bg_count > 1 || redirect_count > 1)
	goto syntax_err;

    ret = Fork_execute (cmd1, ofile, obuf, bsize, n_bytes, fg);

syntax_err:
    if (cbuf != dbuf)
	free (cbuf);
    return (ret);
}

/******************************************************************

    Returns the pointer to the first token in "str" or NULL on
    failure. A token is a string of chars separated by spaces. A
    string of arbitrary chars quated by " is also a token. " is not
    considered as part of the token. There is no escape mechanism
    for ". Quated token must also be separated by space. The token
    length and the pointer that pointing to the char after the
    token are returned with "len" and "next". On syntex error,
    next is set to NULL. Get_token dosn't modify "str".

******************************************************************/

static char *Get_token (char *str, char **next, int *len) {
    char *st, tok[4], *p;
    int ret, cnt;

    st = str + MISC_char_cnt (str, " \t");
    if (*st == '(') {
	cnt = 1;
	st++;
	p = st;
	while (*p != '\0') {
	    if (*p == '(')
		cnt++;
	    if (*p == ')')
		cnt--;
	    if (cnt == 0) {
		*next = p + 1;
		*len = p - st;
		return (st);
	    }
	    p++;
	}
	*next = NULL;
	return (NULL);
    }
    *next = st;
    ret = MISC_get_token (st, "Q\"", 0, tok, 4);
    if (ret > 0) {
	*next = st + ret;
	p = st + ret - 1;
	while (p > st && (*p == ' ' || *p == '\t'))
	    p--;
	ret = p - st + 1;
	if (*st == '"') {
	    *len = ret - 2;
	    return (st + 1);
	}
	else {
	    *len = ret;
	    return (st);
	}
    }
    else if (ret < 0)
	*next = NULL;
    return (NULL);
}

/******************************************************************

    In order to start a background command running independently of 
    this process, we have to fork child, which executes the command
    and then terminates (This works regardless of the calling
    process's SIGCHLD action). In order to pass the return value 
    (pid on success or error code on failure) we build a pipe. We 
    must also set the SIGCHLD to SIG_DFL so waitpid will work as 
    expected regardless the calling process's SIGCHLD action. We
    block signals while we call fork. This is necessary for SUNOS's
    pthreaded applications which can block in fork while receiving 
    a signal and an i/o being performed in the signal handler. This
    does not happen in single threaded applications as tested. I 
    don't know what will happen on other OS. Do I need to do this 
    for other fork calls (running in foreground) in this module? I 
    havn't tested yet.

******************************************************************/

static int Fork_execute (char *cmd, char *ofile,
			char *obuf, int bsize, int *n_bytes, int fg) {
    struct sigaction child;
    int pp[2];		/* for status reporting from children */
    int nread, stat, ret;

    if (sigaction (SIGCHLD, NULL, &child) < 0 ||
	sigset (SIGCHLD, SIG_DFL) < 0)
	return (MISC_SYSTEM_SIGACTION);

    pp[0] = pp[1] = -1;
    if (pipe (pp) == -1) {
	ret = MISC_SYSTEM_PIPE;
	goto cleanup;
    }
    fcntl (pp[0], F_SETFD, FD_CLOEXEC | fcntl (pp[0], F_GETFD));
    fcntl (pp[1], F_SETFD, FD_CLOEXEC | fcntl (pp[1], F_GETFD));

    ret = 0;
    if (!fg) {
	int pid, child_status;
	sigset_t new_set, old;

	sigfillset (&new_set);
	if (sigprocmask (SIG_BLOCK, &new_set, &old) < 0) {
	    ret = MISC_SYSTEM_SIGPROCMASK;
	    goto cleanup;
	}
	pid = fork();
	if (pid == -1) {
	    ret = MISC_SYSTEM_FORK;
	    goto cleanup;
	}
	sigprocmask (SIG_SETMASK, &old, NULL);
	if (pid == 0) {		/* in child */
	    int ppb[2];		/* for status reporting from children */
	    int status;

	    if (Setsid)
		setsid ();
	    close (pp[0]);
	    if (pipe (ppb) == -1)
		status = MISC_SYSTEM_PIPE;
	    else {
		fcntl (ppb[0], F_SETFD, FD_CLOEXEC | fcntl (ppb[0], F_GETFD));
		fcntl (ppb[1], F_SETFD, FD_CLOEXEC | fcntl (ppb[1], F_GETFD));
		status = Execute (cmd, ofile, obuf, 
					    bsize, n_bytes, fg, ppb[1]);
		close (ppb[1]);
		if (Read_pipe (ppb[0], (char *)&stat, 
					    sizeof (int)) == sizeof (int))
		    status = stat;
	    }
	    while ((ret = write (pp[1], &status, sizeof (int))) < 0 && 
							errno == EINTR);
	    exit (0);
	}
	close (pp[1]);
	pp[1] = -1;
	nread = Read_pipe (pp[0], (char *)&stat, sizeof (int));
	while ((ret = waitpid (pid, &child_status, 0)) == -1 && 
							errno == EINTR);
	if (ret < 0) {
	    ret = MISC_SYSTEM_BG_DIED;
	    goto cleanup;
	}
	if (child_status != 0) {
	    ret = MISC_SYSTEM_BG_FAILED; 
	    goto cleanup;
	}
    }
    else {
	ret = Execute (cmd, ofile, obuf, bsize, n_bytes, fg, pp[1]);
	close (pp[1]);
	pp[1] = -1;
	nread = Read_pipe (pp[0], (char *)&stat, sizeof (int));
    }

    if (nread >= (int)sizeof (int))
	ret = stat;

cleanup:
    if (pp[0] >= 0)
	close (pp[0]);
    if (pp[1] >= 0)
	close (pp[1]);
    sigaction (SIGCHLD, &child, NULL);	/* recorver */
    return (ret);
}

/*********************************************************************

    Read at most "buf_size" bytes from pipe "fd" and put them in "buf".
    Returns the number of bytes read.

*********************************************************************/

static int Read_pipe (int fd, char *buf, int buf_size) {
    int nread, n;

    nread = 0;
    while (1) {
	n = read (fd, buf + nread, buf_size - nread);
	if (n == 0 || (n < 0 && errno != EINTR))
	    return (nread);
	if (n > 0)
	    nread += n;
	if (nread >= buf_size)
	    return (nread);
    }
}

/*****************************************************************

    Recursively remove process id "rmpid" and all its children.
    Returns the number of processes removed.

*****************************************************************/

#define RMDP_BUF_SIZE 512

static int Rm_proc_and_children (pid_t rmpid, 
					Proc_t *procs, int pcnt, int sig) {
    static int level = 0;
    int cnt, i;

    level++;
    if (level > 32) {
	MISC_log ("Unexpected child process level (%d)\n", level);
	return (0);
    }
    cnt = 0;
    for (i = 0; i < pcnt; i++) {
	if (procs[i].ppid == rmpid)
	    cnt += Rm_proc_and_children (procs[i].pid, procs, pcnt, sig);
	if (procs[i].pid == rmpid) {
	    kill (procs[i].pid, sig);
	    cnt++;
	}
    }
    level--;
    return (cnt);
}

/*****************************************************************

    Remove process id "rmpid" and all its decendent.

*****************************************************************/

static int Rm_decedent_processes (pid_t rmpid, int sig) {
    int cnt, rmcnt;
    DIR *proc_dir;
    struct dirent *dp;
    Proc_t *procs;

    proc_dir = opendir ("/proc");
    if (proc_dir == NULL) {
	kill (rmpid, sig);
	MISC_log ("opendir /proc failed, errno %d\n", errno);
	return (-1);
    }

    cnt = 0;
    procs = (Proc_t *)STR_reset (NULL, 512 * sizeof (Proc_t));
    while ((dp = readdir (proc_dir)) != NULL) {
	int pid, len, ppid, fd;
	char buf[RMDP_BUF_SIZE], *p, *key;
	Proc_t proc;

	pid = atoi (dp->d_name);
	if (pid == 0)		/* not a process dir */
	    continue;

	sprintf (buf, "/proc/%s/status", dp->d_name);
	fd = MISC_open (buf, O_RDONLY, 0);
	if (fd < 0)
	    continue;
	len = MISC_read (fd, buf, RMDP_BUF_SIZE);
	MISC_close (fd);
	if (len <= 0)
	    continue;
	len--;
	buf[len] = '\0';
	key = "PPid:";
	p = strstr (buf, key);
	if (p == NULL ||
	    sscanf (p + strlen (key), "%d", &ppid) != 1)
	    continue;
	proc.pid = pid;
	proc.ppid = ppid;
	procs = (Proc_t *)STR_append ((char *)procs, 
					(char *)&proc, sizeof (Proc_t));
	cnt++;
    }
    closedir (proc_dir);

    if (cnt == 0) {		/* something was wrong */
	kill (rmpid, sig);
	MISC_log ("No process found in /proc\n");
	rmcnt = -1;
    }
    else
	rmcnt = Rm_proc_and_children (rmpid, procs, cnt, sig);
    STR_free ((char *)procs);
    return (rmcnt);
}

/******************************************************************

    Executes commands "cmd". If "fg" is zero (background mode), this 
    function returns with cmd running in background. The outputs of 
    cmd is piped to the file named "ofile" if ofile != NULL. The file 
    contents are deleted if file "ofile" exists.

    In the foreground mode, "fg" != 0, The function will not return 
    until "cmd" terminates. If "ofile" != NULL, the output is
    piped into file "ofile". Otherwise, if bsize > 0, the outputs 
    are piped into buffer "obuf" of size "bsize". Extra data are 
    discarded. If bsize == 0, all output data are discarded. If 
    bsize < 0, all output data goes to the stdout.

    Note that here we use pipes to implement the behavior that if 
    this application is killed, then cmd will die. This function does 
    not change the signal status and frees all resources used in it. 
    We don't currently do SIGINT and SIGQUIT as done in system.c.

    On success, returns the pid of cmd if "fg" == 0 or the 
    status as returned by "system" if "fg" != 0. The total number
    of output bytes is returned in "n_bytes" if "n_bytes" is not 
    NULL. The function returns a negative error code on failure.

******************************************************************/

static int Execute (char *cmd, char *ofile,
		char *obuf, int bsize, int *n_bytes, int fg, int pp1) {
    int pid, fd, opp[2];
    int status, err, t;
    static int max_n_files = 0;

    if (max_n_files == 0) {
	struct rlimit rsrc_limit;	/* resource limits structure */
	if (getrlimit (RLIMIT_NOFILE, &rsrc_limit) != 0)
            return (MISC_SYSTEM_GETRLIMIT);
	max_n_files = rsrc_limit.rlim_cur;
    }

    if (cmd == NULL)
	return (MISC_SYSTEM_SYNTAX_ERROR);

    status = 0;
    opp[0] = opp[1] = -1;
    fd = -1;
    if (ofile != NULL) {
	bsize = 0;
	fd = MISC_open (ofile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd < 0) {
	    status = MISC_SYSTEM_OPEN;
	    goto cleanup;
	}
    }
    if (fg) {
	if (pipe (opp) == -1) {
	    status = MISC_SYSTEM_PIPE;
	    goto cleanup;
	}
    }

    pid = fork();
    if (pid == -1) {
	status = MISC_SYSTEM_FORK;
	goto cleanup;
    }
    if (pid == 0) {		/* in child */
	Close_files (opp, fd, pp1, max_n_files);
	if (fd >= 0)	/* send to fd */
	    Redirect_output (&fd);
	else if (fg)		/* pipe to parent */
	    Redirect_output (opp + 1);

	if (fd >= 0)
	    close (fd);
	if (opp[0] >= 0)
	    close (opp[0]);
	if (opp[1] >= 0)
	    close (opp[1]);

	status = Execute_cmd (cmd);
	if (status < 0) {
	    while (write (pp1, &status, sizeof (int)) < 0 && 
							errno == EINTR);
	}
	exit (status);
    }

    /* in parent */
    if (fg) {
	int cnt, st;
	if (opp[1] >= 0) {
	    close (opp[1]);
	    opp[1] = -1;
	}
	if (bsize == -2) {
	    status = opp[0];	/* return this pipe fd */
	    opp[0] = -1;	/* prevent from being closed */
	    goto cleanup;
	}
	st = -1;
	cnt = Read_output (opp[0], fd, obuf, bsize, pid, &st);
	if (cnt < 0) {
	    status = cnt;
	    if (st == -1)
		kill ((pid_t)pid, SIGKILL);
	}
	else {
	    if (n_bytes != NULL)
		*n_bytes = cnt;
	    if (cnt >= bsize)
		cnt = bsize - 1;
	    if (bsize > 0 && obuf != NULL)
		obuf[cnt] = '\0';
	}
	close (opp[0]);
	opp[0] = -1;
	if (st == -1) {
	    while ((t = waitpid (pid, &st, 0)) == -1 && errno == EINTR);
	    if (t == -1)
		status = MISC_SYSTEM_WAITPID;
	}
	if (status >= 0)
	    status = st;
    }
    else
	status = pid;

cleanup:
    err = errno;
    if (fd >= 0)
	close (fd);
    if (opp[0] >= 0)
	close (opp[0]);
    if (opp[1] >= 0)
	close (opp[1]);
    errno = err;

    return (status);
}

/*********************************************************************

    Redirects stdout and stderr to "fd".

*********************************************************************/

static void Redirect_output (int *fd) {
    dup2 (*fd, 1);
    dup2 (*fd, 2);
    close (*fd);
    *fd = -1;
}

/*********************************************************************

    Close all fds other than standard ports and those listed.

*********************************************************************/

static void Close_files (int *opp, int fd, int pp1, int nfds) {
    int i;

    for (i = 3; i < nfds; i++) {
	if (i == opp[0] || i == opp[1] || 
	    i == fd || i == pp1)
	    continue;
	if (fcntl (i, F_GETFD) != -1) 
	    MISC_close (i);
    }
}

/*********************************************************************

    Reads the the output from pipe "fd" until the pipe is broken. The
    data read are put in file "ofd" if it is >= 0. Otherwise if bsize 
    >= 0, they are put in "obuf" of size "bsize". Extra data are  
    discarded. If bsize < 0, data are sent to stdout. Returns the 
    total number of bytes read on success or a negative error code.

*********************************************************************/

#define BUF_SIZE 256

static int Read_output (int fd, int ofd, char *obuf, int bsize, 
							int pid, int *st) {
    int cnt, nread;

    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) < 0)
	return (MISC_SYSTEM_PIPE);
    cnt = nread = 0;
    while (1) {
	char buf[BUF_SIZE + 1], *pt;
	int len, ret;

	if (cnt < bsize) {
	    pt = obuf + cnt;
	    len = bsize - cnt;
	}
	else {
	    pt = buf;
	    len = BUF_SIZE;
	}
	ret = read (fd, pt, len);
	if (ret > 0) {
	    nread += ret;
	    if (ofd >= 0) {
		if (write (ofd, pt, ret) != ret) {
		    return (MISC_SYSTEM_WRITE);
		}
	    }
	    else if (bsize < 0) {
		pt[ret] = '\0';
		printf ("%s", pt);
	    }
	    else if (cnt < bsize)
		cnt += ret;
	}
	else {
	    int t, tst;

	    while ((t = waitpid (pid, &tst, WNOHANG)) == -1 && errno == EINTR);
	    if (t == -1)
		return (MISC_SYSTEM_WAITPID);
	    if (t > 0) {
		*st = tst;
		break;
	    }
	    if (ret < 0) {
		if (errno == EWOULDBLOCK) {
		    msleep (20);
		    continue;
		}
		if (errno == EINTR)
		    continue;
		return (MISC_SYSTEM_READ_PIPE);
	    }
	    break;
	}
    }
    return (nread);
}

/*********************************************************************

    Executes command "cmd" and returns -1 in case of failure. errno
    contains failure code. On success, it does not return.

*********************************************************************/

static int Execute_cmd (const char *cmd) {
    char *avb[MAX_N_ARGS];
    char buf[MAX_CMD_LENGTH], *b, **av;
    int ret;

    b = buf;
    av = Parse_cmd ((char *)cmd, &b, MAX_CMD_LENGTH, avb, MAX_N_ARGS);
    if (av == NULL) {
	MISC_log ("Empty command (%s)\n", cmd);
	return (MISC_SYSTEM_EXECVP);
    }

    ret = execvp (av[0], av);
    if (av != avb)
	free (av);
    if (b != buf)
	free (b);
    if (ret < 0)
	return (MISC_SYSTEM_EXECVP);
    return (0);
}

/******************************************************************

    Terminates the coprocess "cp" and frees up all resources.

******************************************************************/

void MISC_cp_close (void *cp) {
    Cp_struct_t *cps;

    cps = (Cp_struct_t *)cp;
    if (cps->up) {
	int t;
	Rm_decedent_processes ((pid_t)(cps->pid), SIGTERM);
	while (waitpid (cps->pid, &t, 0) == -1 && errno == EINTR);
    }
    close (cps->in_fd);
    close (cps->out_fd);
    close (cps->err_fd);
    MISC_free (cp);
}

/******************************************************************

    Reads a line from the output, stderr or stdout, of the coprocessor
    and puts it in "buf" of size "b_size". If b_size if too small,
    the line is truncated. The returned line is always a null-terminated 
    string. The read is non_blocking. If no line is ready, it returns 0.
    If reads stderr first. It returns MISC_CP_STDERR or MISC_CP_STDOUT
    to indicated where the line is read. If an error is detected, it
    terminates the coprocess and returns error code MISC_CP_DOWN.

******************************************************************/

int MISC_cp_read_from_cp (void *cp, char *buf, int b_size) {
    Cp_struct_t *cps;
    int ret, t, st;
    time_t st_t;

    cps = (Cp_struct_t *)cp;
    if (!cps->up)
	return (MISC_CP_DOWN);

    ret = Read_cp_output (cps->err_fd, &(cps->err), buf, b_size);
    if (ret > 0) {
	if (cps->first_read) {
	    char *f;
	    cps->first_read = 0;
	    if ((f = strstr (buf, MISC_CP_ERR_FLAG)) != NULL) {
		cps->status = MISC_SYSTEM_EXECVP;
		MISC_log ("%s", f + strlen (MISC_CP_ERR_FLAG));
		return (MISC_CP_DOWN);
	    }
	}
	return (MISC_CP_STDERR);
    }
    else if (ret == 0) {
	ret = Read_cp_output (cps->out_fd, &(cps->out), buf, b_size);
	if (ret > 0)
	    return (MISC_CP_STDOUT);
    }

    st_t = 0;
    while ((t = waitpid (cps->pid, &st, WNOHANG)) <= 0) {
	if (t < 0 && errno == EINTR)
	    continue;
	if (t == 0 && ret == 0)		/* cp still alive but no output */
	    return (0);
	if (t < 0)
	    break;
	if (st_t == 0)	/* wait up to 5 seconds for cp to terminate */
	    st_t = time (NULL);
	else if (time (NULL) >= st_t + 5)
	    break;
	sleep (1);
    }
    if (t == 0) {		/* cp does not terminate */
	kill (cps->pid, SIGKILL);	/* pipe broken and cp still running */
	while ((t = waitpid (cps->pid, &st, 0)) == -1 && errno == EINTR);
    }

    cps->up = 0;
    if (t <= 0)
	cps->status = MISC_SYSTEM_WAITPID;
    else
	cps->status = st;
    return (MISC_CP_DOWN);
}

/******************************************************************

    Returns the first line or the part of the line, if "b_size" is
    too small for the line, from "rbuf" in "buf" of size "b_size". If
    there is no such data in "rbut", additional data is read from
    pipe "fd". Return 1 if a line is put in "buf" or 0 if a line
    is not yet available. It returns -1 if the pipe is broken. 

******************************************************************/

static int Read_cp_output (int fd, Cp_buf_t *rbuf, char *buf, int b_size) {
    int b_s;

    b_s = b_size & (~MISC_CP_MATCH_STR);
    if (b_s <= 1)
	return (0);
    if (b_s > MISC_CP_BUFSIZE)
	b_s = MISC_CP_BUFSIZE;
    while (1) {
	char *st, *p1;
	int ret, len, lenm;

	st = rbuf->buf + rbuf->offset;
	st[rbuf->nbytes] = '\0';
	lenm = 0;
	if ((b_size & MISC_CP_MATCH_STR) &&
	    (p1 = strstr (st, buf)) != NULL)
	    lenm = p1 - st + strlen (buf);
	len = lenm;
	if ((p1 = strstr (st, "\n")) != NULL)
	    len = p1 - st + 1;
	if (lenm > 0 && lenm < len)
	    len = lenm;
	if (len > 0) {
	    if (len >= b_s)
		len = b_s - 1;
	}
	else {
	    if (rbuf->nbytes >= b_s - 1)
		len = b_s - 1;
	}
	if (len > 0) {
	    memcpy (buf, st, len);
	    buf[len] = '\0';
	    rbuf->nbytes -= len;
	    rbuf->offset += len;
	    return (1);
	}

	if (rbuf->offset > 0) {
	    memmove (rbuf->buf, rbuf->buf + rbuf->offset, rbuf->nbytes);
	    rbuf->offset = 0;
	}

	while ((ret = read (fd, rbuf->buf + rbuf->nbytes, 
		MISC_CP_BUFSIZE - rbuf->nbytes - 1)) == -1 && errno == EINTR);
	if (ret == 0)
	    return (-1);
	if (ret < 0 &&
	    (errno != EWOULDBLOCK && errno != EINTR)) {
	    MISC_log ("MISC_cp: read failed (errno %d)\n", errno);
	    return (-1);
	}
	if (ret > 0)
	    rbuf->nbytes += ret;
	else
	    return (0);
    }
    return (0);
}

/******************************************************************

    Writes null-terminated string "str" to the stdin of the coprocess
    "cp". It will block until success. If an error is detected, it
    terminates the coprocess and returns error code MISC_CP_DOWN. it
    returns 0 on success.

******************************************************************/

int MISC_cp_write_to_cp (void *cp, char *str) {
    Cp_struct_t *cps;
    int l, cnt, ret;
    sigset_t oset, nset;
    struct sigaction oact, nact;

    cps = (Cp_struct_t *)cp;
    if (!cps->up)
	return (MISC_CP_DOWN);

    sigprocmask (SIG_UNBLOCK, NULL, &nset);
    sigaddset (&nset, SIGPIPE);
    sigprocmask (SIG_SETMASK, &nset, &oset);
    sigaction (SIGPIPE, NULL, &nact);
    nact.sa_handler = Sig_pipe;
    nact.sa_flags = nact.sa_flags & (~SA_SIGINFO);
    sigaction (SIGPIPE, &nact, &oact);

    Sig_pipe_received = 0;
    l = strlen (str);
    cnt = 0;
    while (1) {
	ret = write (cps->in_fd, str, l);
	if (ret < 0) {
	    if (errno == EINTR)
		continue;
	    if (errno == EWOULDBLOCK) {
		sleep (1);
		continue;
	    }
	    if (errno != EPIPE)
 		MISC_log ("MISC_cp: write failed (errno %d)\n", errno);
	    cps->up = 0;
	    break;
	}
	if (Sig_pipe_received || ret == 0) {
	    cps->up = 0;
	    break;
	}
	cnt += ret;
	if (cnt >= l)
	    break;
    }
    sigprocmask (SIG_SETMASK, &oset, NULL);
    sigaction (SIGPIPE, &oact, NULL);

    if (!cps->up) {
	int t, st;
	time_t st_t;

	st_t = 0;	/* wait up to 5 seconds for cp to terminate */
	while ((t = waitpid (cps->pid, &st, WNOHANG)) <= 0) {
	    if (errno == EINTR)
		continue;
	    if (t < 0)
		break;
	    if (st_t == 0)
		st_t = time (NULL);
	    else if (time (NULL) >= st_t + 5)
		break;
	    sleep (1);
	}
	if (t == 0) {		/* cp does not terminate */
	    kill (cps->pid, SIGKILL);	/* pipe broken and cp still running */
	    while ((t = waitpid (cps->pid, &st, 0)) == -1 && errno == EINTR);
	}
	if (t <= 0)
	    cps->status = MISC_SYSTEM_WAITPID;
	else
	    cps->status = st;
	return (MISC_CP_DOWN);
    }
    return (0);
}

/******************************************************************

    Returns the termination status of coprocess "cp". Returns 0 if 
    the coprocess is still running.

******************************************************************/

int MISC_cp_get_status (void *cp) {
    Cp_struct_t *cps;

    cps = (Cp_struct_t *)cp;

    if (cps->up) {
	int t, st;
	while ((t = waitpid (cps->pid, &st, WNOHANG)) == -1 && errno == EINTR);
	if (t == -1) {
	    cps->status = MISC_SYSTEM_WAITPID;
	    cps->up = 0;
	}
	else if (t == 0)
	    cps->status = 0;
	else {
	    cps->status = st;
	    cps->up = 0;
	}
    }
    return (cps->status);
}

/******************************************************************

    Starts a coprocess "cmd". Pipes are set to connect to the 
    coprocess for stdin, stdout and stderr. Reading of stdout and 
    stderr are set to non-blocking. On success, the pointer to
    the coprocess struct is returned which can be used as paramter
    for calling other MISC_cp functions. The function returns 0
    on success or a negative error code. This function changes the
    SIGCLD and SIGPIPE behaviors of the calling process. This can
    be improved later (using sigaction).

    The MISC_cp functions provide a convenient ways of running and 
    managing coprocesses. A failed coprocess can be detected when
    calling MISC_cp read/write functions. One can run multiple 
    coprocesses and can run them remotely with RPC. The limitation
    of using MISC_cp is the possible buffering of the standard ports.
    The coprocess must call fflush after writing each line to stdout
    or set buffer size to 0.

******************************************************************/

int MISC_cp_open (char *cmd, int flag, void **rcp) {
    int fd[6], i;
    pid_t pid;
    Cp_struct_t *cp;

    cp = (Cp_struct_t *)MISC_malloc (sizeof (Cp_struct_t) + strlen (cmd) + 1);
    cp->cmd = (char *)cp + sizeof (Cp_struct_t);
    strcpy (cp->cmd, cmd);
    cp->up = 0;
    cp->in_fd = cp->out_fd = -1;
    cp->out.nbytes = cp->err.nbytes = 0;
    cp->out.offset = cp->err.offset = 0;
    cp->first_read = 1;

    for (i = 0; i < 6; i++)
	fd[i] = -1;

    for (i = 0; i < 6; i += 2) {
	if (pipe (fd + i) < 0) {
 	    MISC_log ("MISC_cp: pipe failed (errno %d)\n", errno);
	    return (Close_fd_return (fd, cp, MISC_CP_PIPE_FALIED));
	}
    }

    pid = fork ();
    if (pid < 0) {
 	MISC_log ("MISC_cp: fork failed (errno %d)\n", errno);
	return (Close_fd_return (fd, cp, MISC_CP_FORK_FALIED));
    }
    else if (pid > 0) {			/* parent */
	close (fd[0]);
	close (fd[3]);
	close (fd[5]);
	cp->in_fd = fd[1];
	cp->err_fd = fd[2];
	cp->out_fd = fd[4];
	if (fcntl (cp->out_fd, F_SETFL, 
			fcntl (cp->out_fd, F_GETFL) | O_NONBLOCK) < 0 ||
	    fcntl (cp->err_fd, F_SETFL, 
			fcntl (cp->err_fd, F_GETFL) | O_NONBLOCK) < 0) {
 	    MISC_log ("MISC_cp: fcntl (set non-blocking) failed (errno %d)\n", 
								errno);
	    return (Close_fd_return (fd, cp, MISC_CP_FCNTL_FALIED));
	}
	cp->up = 1;
	cp->pid = pid;
	cp->status = 0;
	*rcp = cp;
    }
    else {				/* child */
	char *avb[MAX_N_ARGS], buf[MAX_CMD_LENGTH], *b, **av;

	close (fd[1]);
	close (fd[2]);
	close (fd[4]);
	if (fd[0] != STDIN_FILENO &&
	    dup2 (fd[0], STDIN_FILENO) != STDIN_FILENO)
	    exit (1);
	if (fd[3] != STDERR_FILENO &&
	    dup2 (fd[3], STDERR_FILENO) != STDERR_FILENO)
	    exit (1);
	if (fd[5] != STDOUT_FILENO &&
	    dup2 (fd[5], STDOUT_FILENO) != STDOUT_FILENO)
	    exit (1);
	close (fd[0]);
	close (fd[3]);
	close (fd[5]);
	b = buf;
	av = Parse_cmd (cmd, &b, MAX_CMD_LENGTH, avb, MAX_N_ARGS);
	if (av == NULL) {
 	    MISC_log ("%sMISC_cp: Empty command (%s)\n", MISC_CP_ERR_FLAG, cmd);
	    exit (1);
	}
	if (execvp (av[0], av) < 0) {
 	    MISC_log ("%sMISC_cp: execvp (%s) failed (errno %d)\n", MISC_CP_ERR_FLAG, cmd, errno);
	    exit (1);
	}
	if (av != avb)
	    free (av);
	if (b != buf)
	    free (b);
    }
    return (0);
}

/******************************************************************

    Closes any open fd in the 6 pipe fds "fd" and returns "ret".
    It also fees "cp".

******************************************************************/

static int Close_fd_return (int *fd, Cp_struct_t *cp, int ret) {
    int i;

    MISC_free (cp);
    for (i = 0; i < 6; i++) {
	if (fd[i] >= 0)
	    close (fd[i]);
	fd[i] = -1;
    }
    return (ret);
}

/******************************************************************

    The SIGPIPE callback function.

******************************************************************/

static void Sig_pipe (int sig) {
    Sig_pipe_received = 1;
    return;
}

/******************************************************************

    Converts the string form command line "cmd" into vector form
    and returns it. "*buf" is the caller provided buffer of size 
    "buf_size" and "avb_size" is the size of buffer "avb".

******************************************************************/

static char **Parse_cmd (char *cmd, char **buf, int buf_size, 
					char **avb, int avb_size) {
    char *t, *next, *b, **av;
    int n, len, l;

    if ((len = strlen (cmd)) >= buf_size)
	b = MISC_malloc (len + 1);
    else
	b = *buf;
    strcpy (b, cmd);

    n = 0;
    l = len;
    t = Get_token (b, &next, &l);
    while (t != NULL) {
	if (l > 0)
	    n++;
	t = Get_token (next, &next, &l);
    }
    if (n <= 0) {
	if (b != *buf)
	    free (b);
	return (NULL);
    }
    *buf = b;

    if (n >= avb_size)
	av = (char **)MISC_malloc ((n + 1) * sizeof (char *));
    else
	av = avb;

    n = 0;
    l = len;
    t = Get_token (b, &next, &l);
    while (t != NULL) {
	if (l > 0) {
	    t[l] = '\0';
	    av[n] = t;
	    n++;
	}
	t = Get_token (next, &next, &l);
    }
    av[n] = NULL;
    return (av);
}

/*************************************************************************
 
    Retrieves stack info of process "pid" and returns it with "out_buf" of 
    size "out_buf_size". If "out_buf_size" <= 0, the stack info is printed
    to the stderr port. In cases of error conditions, an error message is 
    put in "out_buf". If the buffer size is not sufficient, the data
    is truncated and null terminated. We invoke command "pstack" to get
    the stack info. This works for both SunOs and Linux. "pstack" must 
    be installed and in the user's search path. Functions on the stack 
    below MISC_proc_printstack are discarded.

**************************************************************************/

#define TMP_BUF_SIZE 2048

void MISC_proc_printstack (int pid, int out_buf_size, char *out_buf) {
    char cmd[64] ;
    int retval, n_bytes;
    char tmp_buf[TMP_BUF_SIZE], *cpt;

    sprintf (cmd, "pstack %d", pid);
    retval = MISC_system_to_buffer (cmd, tmp_buf, TMP_BUF_SIZE, &n_bytes);

    if (retval < 0) {
	sprintf (tmp_buf, "MISC_system_to_buffer pstack pid %d failed (%d)\n",
								pid, retval);
	cpt = tmp_buf;
    }
    else {
	if (n_bytes >= TMP_BUF_SIZE)
	    n_bytes--;
	tmp_buf[n_bytes] = '\0';
	cpt = strstr (tmp_buf, "MISC_proc_printstack");
	if (cpt == NULL)
	    cpt = tmp_buf;
	else {
	    while (cpt > tmp_buf && cpt[-1] != '\n')
		cpt--;
	}
    }

    if (out_buf_size <= 0)
	fprintf (stderr, "%s", cpt);
    else {
	strncpy (out_buf, cpt, out_buf_size);
	if (out_buf_size > 0)
	    out_buf[out_buf_size - 1] = '\0';
    }

    return;
}

/*************************************************************************
 
    Returns the function pointer of function "func" in shared library 
    "lib". Returns NULL on failure. No error message is logged if "quiet".

**************************************************************************/

void *MISC_get_func (char *lib, char *func, int quiet) {
    static char **libs = NULL;
    static int n_libs = 0;
    void *handle, *p;
    char *error;

    handle = RTLD_DEFAULT;
    if (lib != NULL && strlen (lib) > 0) {
	int i;
	
	for (i = 0; i < n_libs; i++) {
	    if (strcmp (libs[i], lib) == 0)
		break;
	}
	if (i >= n_libs) {
	    char *new_lib;
	    if ((handle = dlopen (lib, RTLD_LAZY | RTLD_GLOBAL)) == NULL) {
		if (!quiet)
		    MISC_log ("%s\n", dlerror ());
		return (NULL);
	    }
	    new_lib = (char *)MISC_malloc (strlen (lib) + 1);
	    strcpy (new_lib, lib);
	    libs = (char **)STR_append ((char *)libs, (char *)&new_lib, 
						sizeof (char *));
	    n_libs++;
	}
    }

    p = dlsym (handle, func);
    if ((error = dlerror()) != NULL) {
	if (!quiet)
	    MISC_log ("%s\n", error);
	return (NULL);
    }
    return (p);
}



