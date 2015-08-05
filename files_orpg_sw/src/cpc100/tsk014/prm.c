
/******************************************************************

	file: prm.c

	A tool that kills specified processes by names. It also 
	cleans up specified shared memory segments, message queues 
	and semaphores.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/10/03 21:28:15 $
 * $Id: prm.c,v 1.15 2011/10/03 21:28:15 jing Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>

#define NAME_LEN	128
#define MAX_ITEMS	64
#define OBUF_SIZE	40000

static int Get_next_line (char *text, char *buf, int buf_size);
static char *Run_command (char *cmd, char *prog_name);


/*********************************************************************

    The main function.

*********************************************************************/

int main (int argc, char **argv)
{
    char prog_name[NAME_LEN];
    char *prs[MAX_ITEMS];		/* process names */
    int prmd[MAX_ITEMS];		/* process removed */
    int cnt;
    char buf[2 * NAME_LEN + 64];
    char name[NAME_LEN];
    int i, pid, n_keep_pids, k_pid;
    int *keep_pids;
    int sema, shm, mq;
    int nshm, nsema, nmq;
    int semakey[MAX_ITEMS];
    int shmkey[MAX_ITEMS];
    int mqkey[MAX_ITEMS];
    char flags[NAME_LEN];
    char cmd_pat[NAME_LEN];
    int message, format;
    int signal;
    int com_offset, pid_offset;
    char *obuf;

    sema = shm = mq = nshm = nsema = nmq = n_keep_pids = 0;
    keep_pids = NULL;
    cnt = 0;
    message = 1;
    signal = SIGTERM;
    strncpy (prog_name, argv[0], NAME_LEN);
    prog_name[NAME_LEN - 1] = '\0';
    cmd_pat[0] = '\0';
    strcpy (flags, "-ef");
    while (argc > 1) {
	argc--;
	argv++;
	if (*argv[0] == '-') {
	    char *s;
	    s = argv[0] + 1;
	    switch (*s) {
	      mes:
	    case 'h':
		printf ("prm - removes processes by names, shm segments and semaphores\n");
		printf ("Usage: prm (options) name1 name2 -m... -s... \n");
		printf ("           where name1, name2 ... are process names.\n");
		printf ("           Note that the names may be truncated on some systems while\n");
		printf ("           displayed with ps. Always use the names displayed by ps.\n");
		printf ("       -pat-pattern [remove only if command matches pattern]\n");
		printf ("       -keep-pid [do not remove pid]\n");
		printf ("       -quiet [suppressing messages]\n");
		printf ("       -ver [printing messages]\n");
		printf ("       -SIGKILL [use SIGKILL instead of SIGTERM]\n");
		printf ("       -9 [the same as -SIGKILL]\n");
		printf ("       -fflags [flags used for ps, e.g. -f-ef, default = \"-ef\"]\n");
		printf ("           You can also directly specify ps flags\n");
		printf ("           e.g. prm -ef pro1 pro2 ...\n");
		printf ("       -mkey0,key1... [e.g. -m32045,42011. clears shm segments of specified keys]\n");
		printf ("       -skey0,key1... [e.g. -s62045,72011. clears semaphores of specified keys]\n");
		printf ("       -qkey0,key1... [e.g. -q45,11. clears messagequeues of specified keys]\n");
		printf ("           Multiple -m..., -q... and -s... are accepted\n");
		printf ("           At most 6 keys in each -m, -q or -s option\n");
		printf ("       -m [-m clears all shm segments]\n");
		printf ("       -s [-s clears all semaphores]\n");
		printf ("       -q [-q clears all message queues]\n");

		exit (0);
	    case 'f':
		if (*(s + 1) == '-')
		    sscanf (s + 1, "%s", flags);
		else
		    goto nextp;
		break;
	    case 's':
		i = sscanf (s + 1, 
		    "%i%*c%i%*c%i%*c%i%*c%i%*c%i",
		    &semakey[nsema], &semakey[nsema + 1], &semakey[nsema + 2],
		    &semakey[nsema + 3], &semakey[nsema + 4], 
		    &semakey[nsema + 5]);
		if (i > 0)
		    nsema += i;
		if (nsema > MAX_ITEMS - 6)
		    goto mes;
		sema = 1;
		break;
	    case 'm':
		i = sscanf (s + 1, 
		    "%i%*c%i%*c%i%*c%i%*c%i%*c%i",
		    &shmkey[nshm], &shmkey[nshm + 1], &shmkey[nshm + 2],
		    &shmkey[nshm + 3], &shmkey[nshm + 4], 
		    &shmkey[nshm + 5]);
		if (i > 0)
		    nshm += i;
		if (nshm > MAX_ITEMS - 6)
		    goto mes;
		shm = 1;
		break;
	    case 'q':
		if (strcmp (s, "quiet") != 0) {
		    i = sscanf (s + 1, 
			"%i%*c%i%*c%i%*c%i%*c%i%*c%i",
			&mqkey[nmq], &mqkey[nmq + 1], &mqkey[nmq + 2],
			&mqkey[nmq + 3], &mqkey[nmq + 4], 
			&mqkey[nmq + 5]);
		    if (i > 0)
			nmq += i;
		    if (nmq > MAX_ITEMS - 6)
			goto mes;
		    mq = 1;
		    break;
		}
	    default:
	      nextp:
		if (strcmp ("SIGKILL", s) == 0)
		    signal = SIGKILL;
		else if (strcmp ("9", s) == 0)
		    signal = SIGKILL;
		else if (strcmp ("quiet", s) == 0)
		    message = 0;
		else if (strcmp ("ver", s) == 0)
		    message = 1;
		else if (strncmp ("keep-", s, 5) == 0) {
		    if (sscanf (s + 5, "%d", &k_pid) != 1) {
			printf ("Bad keep option (%s) - %s\n", s, prog_name);
			exit (1);
		    }
		    keep_pids = (int *)STR_append ((char *)keep_pids, 
						(char *)&k_pid, sizeof (int));
		    n_keep_pids++;
		}
		else if (strncmp ("pat-", s, 4) == 0) {
		    strcpy (cmd_pat, s + 4);
		    if (strlen (cmd_pat) == 0) {
			printf ("Bad pat option (%s) - %s\n", s, prog_name);
			exit (1);
		    }
		}
		else
		    sscanf (s - 1, "%s", flags);
		break;
	    }
	}
	else {
	    prmd[cnt] = 0;
	    prs[cnt++] = *argv;
	    if (cnt >= MAX_ITEMS)
		break;
	}
    }

    if (cnt == 0)
	goto next;

    sprintf (buf, "ps %s", flags);
    obuf = Run_command (buf, prog_name);
    if (obuf == NULL)
	exit (1);

    pid_offset = -1;
    while (1) {
	char tok[256];
	int args_of;

	if (Get_next_line (obuf, buf, 2 * NAME_LEN) < 0) {
	    goto next;
	}

        /* look for pid and command columns */
	if (pid_offset == -1) {
	    int ind;

	    com_offset = -1;
	    pid_offset = -1;
	    ind = 0;
	    while (MISC_get_token (buf, "", ind, tok, 256) > 0) {
		if (strncmp (tok, "CMD", 3) == 0 ||
						strncmp (tok, "COM", 3) == 0)
		    com_offset = ind;
		if (strncmp (tok, "PID", 3) == 0)
		    pid_offset = ind;
		ind++;
	    }

	    if (com_offset < 0 || pid_offset < 0) {
		printf ("Unknown ps format - %s\n", prog_name);
		exit (1);
	    }
	    continue;
	}

	if (MISC_get_token (buf, "", pid_offset, tok, 256) <= 0 ||
	    sscanf (tok, "%d", &pid) < 1 ||
	    (args_of = MISC_get_token (buf, "", com_offset, tok, 256)) <= 0 ||
	    sscanf (tok, "%s", name) < 1)
	    continue;
	for (i = 0; i < n_keep_pids; i++) {
	    if (pid == keep_pids[i])
		break;
	}
	if (i < n_keep_pids)
	    continue;

	if (cmd_pat[0] != '\0') {
	    char *cmd = buf + MISC_get_token (buf, "", 
					com_offset - 1, tok, 256);
	    if (cmd < buf || strstr (cmd, cmd_pat) == NULL)
		continue;
	}

	for (i = 0; i < cnt; i++) {
	    int ret;
	    char *p, *p0, *script_name, *pname, nb[NAME_LEN];

	    p0 = MISC_basename (prs[i]);
	    p = p0;
	    script_name = NULL;
	    pname = prs[i];
	    while (*p != '\0') {
		if (*p == '@') {
		    if (p > p0 && p[1] != '\0') {
			int len = p - prs[i];
			if (len >= NAME_LEN)
			    len = NAME_LEN - 1;
			memcpy (nb, prs[i], len);
			nb[len] = '\0';
			script_name = p + 1;
			pname = nb;
		    }
		    break;
		}
		p++;
	    }
	    if ((MISC_basename (pname) == pname && 
		 strcmp (pname, MISC_basename (name)) == 0) ||
		strcmp (pname, name) == 0) {

		if (script_name != NULL) {  /* match script name of a shell */
		    int c, match;
		    char t[256];
		    c = match = 0;
		    while (MISC_get_token (buf + args_of, "", c, t, 256) > 0) {
			if (strcmp (script_name, MISC_basename (t)) == 0) {
			    match = 1;
			    break;
			}
			c++;
		    }
		    if (!match)
			continue;
		}

		prmd[i] = 1;
		ret = kill (pid, signal);
		if (message == 1) {
		    if (ret == 0)
			printf ("Process %d (%s) removed - %s\n", 
					pid, prs[i], prog_name);
		    else
			printf ("Removing process %d (%s) failed - %s\n", 
					pid, prs[i], prog_name);
			/* can not remove (permission...) or already died */
		}
	    }
	}
    }

  next:
    if (message) {
	for (i = 0; i < cnt; i++) {
	    if (!prmd[i]) {
		if (cmd_pat[0] == '\0')
		    printf ("Process %s not found - %s\n", prs[i], prog_name);
		else
		    printf ("Process %s matching \"%s\" not found - %s\n", prs[i], cmd_pat, prog_name);
	    }
	}
    }

    /* remove semaphore and shared memory */
    if (sema == 0 && shm == 0 && mq == 0)
	exit (0);

    obuf = Run_command ("ipcs", prog_name);
    if (obuf == NULL)
	exit (1);
    name[0] = '\0';
    format = 0;
    while (1) {
	long key;

	if (Get_next_line (obuf, buf, 2 * NAME_LEN) < 0) {
	    exit (0);
	}
	if (buf[0] == '-') {	/* linux format */
	    format = 1;
	    continue;
	}
	if (format == 1) {
	    if (strstr (buf, "key") != NULL) {
		if (strstr (buf, "shmid") != NULL)
		    name[0] = 'm';
		if (strstr (buf, "semid") != NULL)
		    name[0] = 's';
		if (strstr (buf, "msqid") != NULL)
		    name[0] = 'q';
		continue;
	    }
	    if (sscanf (buf, "%x %i", (int *)&key, &pid) != 2)
		continue;
	}
	else {				/* unix format */
	    if (sscanf (buf, "%s %d %i", name, &pid, (int *)&key) != 3)
		continue;
	}
	if (name[0] == 'm' && shm == 1) {
	    if (nshm > 0) {
		for (i = 0; i < nshm; i++)
		    if (key == shmkey[i])
			break;
	    }
	    if (nshm == 0 || (nshm > 0 && i < nshm)) {
		if (format == 1)
		    sprintf (buf, "ipcrm shm %d > /dev/null 2>&1", pid);
		else
		    sprintf (buf, "ipcrm -m %d", pid);
		obuf = Run_command (buf, prog_name);
		if (obuf == NULL)
		    continue;
		if (message == 1)
		    printf ("Shm (key=%d, %x) removed - %s\n", 
				(int)key, (unsigned int)key, prog_name);
	    }
	}
	if (name[0] == 's' && sema == 1) {
	    if (nsema > 0) {
		for (i = 0; i < nsema; i++)
		    if (key == semakey[i])
			break;
	    }
	    if (nsema == 0 || (nsema > 0 && i < nsema)) {
		if (format == 1)
		    sprintf (buf, "ipcrm sem %d > /dev/null 2>&1", pid);
		else
		    sprintf (buf, "ipcrm -s %d", pid);
		obuf = Run_command (buf, prog_name);
		if (obuf == NULL)
		    continue;
		if (message == 1)
		    printf ("Semaphore (key=%d %x) removed - %s\n", 
				(int)key, (unsigned int)key, prog_name);
	    }
	}
	if (name[0] == 'q' && mq == 1) {
	    if (nmq > 0) {
		for (i = 0; i < nmq; i++)
		    if (key == mqkey[i])
			break;
	    }
	    if (nmq == 0 || (nmq > 0 && i < nmq)) {
		if (format == 1)
		    sprintf (buf, "ipcrm msg %d > /dev/null 2>&1", pid);
		else
		    sprintf (buf, "ipcrm -q %d", pid);
		obuf = Run_command (buf, prog_name);
		if (obuf == NULL)
		    continue;
		if (message == 1)
		    printf ("Message queue (key=%d %x) removed - %s\n", 
				(int)key, (unsigned int)key, prog_name);
	    }
	}
    }


}

/************************************************************************

    Runs command "cmd" and returns the pointer to the buffer that holds
    the command output. 

************************************************************************/

static char *Run_command (char *cmd, char *prog_name) {
    static int b_size = 0;
    static char *buf = NULL;
    int len, n_bytes, ret;

    len = OBUF_SIZE;
    while (1) {
	if (b_size < len) {
	    if (buf != NULL)
		free (buf);
	    buf = MISC_malloc (len);
	    b_size = len;
	}
	if ((ret = MISC_system_to_buffer (cmd, buf, len, &n_bytes)) != 0) {
	    printf ("Running cmd %s failed (%d) - %s\n", cmd, ret, prog_name);
	    return (NULL);
	}
	if (n_bytes < len)
	    break;
	len *= 5;
    }
    return (buf);
}

/**************************************************************

    Returns the next line, in "buf" of size "buf_size", of "text".
    The line is truncated to fit the buffer. The line is always
    null-terminated. Returns -1 if not more line found or the 
    number of chars in the line.

**************************************************************/

static int Get_next_line (char *text, char *buf, int buf_size) {
    static char *cr_text = NULL;
    static int cr_off = 0;
    char *pt, *st;
    int len, s;

    if (text != cr_text) {
	cr_text = text;
	cr_off = 0;
    }
    st = cr_text + cr_off;
    pt = st;
    if (*pt == '\0')
	return (-1);
    while (*pt != '\0' && *pt != '\n')
	pt++;
    len = pt - st;
    memcpy (buf, st, (s = (len > buf_size - 1? buf_size - 1: len)));
    buf[s] = '\0';
    cr_off += len;
    if (*pt == '\n')
	cr_off++;
    return len;
}

