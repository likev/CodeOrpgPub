
/*****************************************************************

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/25 16:06:43 $
 * $Id: lb_rm.c,v 1.46 2008/01/25 16:06:43 jing Exp $
 * $Revision: 1.46 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#ifdef LBLIB
#include <lb.h>
#else
#ifndef TEST
#include "rss_replace.h"
#endif
#include <infr.h>
#endif

#define LB_RM_NAME_SIZE 	256
#define MAX_N_DELETES		64

static int Verbose;
static int N_keep_files = 0;
static char *Keep_files = NULL;

static void Print_usage ();
static void Recursive_remove (char *dir_path);
static int Match_star_wild_card (char *s1, char *s2);


/****************************************************************
*/

int main (int argc, char **argv) {
    char prog_name[LB_RM_NAME_SIZE], dir[LB_RM_NAME_SIZE];
    char LB_name[LB_RM_NAME_SIZE];
    int n_clears, n_deletes, re_remove;
    LB_id_t delete_ids[MAX_N_DELETES];
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */

    strncpy (prog_name, argv[0], LB_RM_NAME_SIZE);
    prog_name[LB_RM_NAME_SIZE - 1] = '\0';
    n_clears = 0;
    n_deletes = 0;
    Verbose = 0;
    re_remove = 0;

    /* get arguments */
    while ((c = getopt (argc, argv, "i:r:a:k:cvh?")) != EOF) {
	switch (c) {

	    case 'i':
		if (n_deletes > 0 || n_clears > 0)
		    Print_usage ();
		if (sscanf (optarg, "%d", &(delete_ids[n_deletes])) != 1)
		    Print_usage ();
		n_deletes++;
		break;

	    case 'c':
		if (n_deletes > 0 || n_clears > 0)
		    Print_usage ();
		n_clears = 0x7fffffff;
		break;

	    case 'r':
		if (n_deletes > 0 || n_clears > 0)
		    Print_usage ();
		if (sscanf (optarg, "%d", &n_clears) != 1)
		    Print_usage ();
		break;

	    case 'a':
		if (sscanf (optarg, "%s", dir) != 1)
		    Print_usage ();
		re_remove = 1;
		break;

	    case 'k':
		Keep_files = 
			STR_append (Keep_files, optarg, strlen (optarg) + 1);
		N_keep_files++;
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage ();
		break;
	}
    }

    if (re_remove) {
	Recursive_remove (dir);
	exit (0);
    }

    if (n_deletes > 0) {
	while (optind < argc) {
	    if (sscanf (argv[optind], "%d", &(delete_ids[n_deletes])) == 1) {
		n_deletes++;
		optind++;
	    }
	    else
		break;
	}
    }

    if (optind <= argc - 1) {      /* get the first LB name */
	strncpy (LB_name, argv[optind], LB_RM_NAME_SIZE);
	LB_name[LB_RM_NAME_SIZE - 1] = '\0';
    }
    if (LB_name[0] == '\0') {
	fprintf (stderr, "LB name not specified - %s\n", prog_name);
	Print_usage ();
    }

    if (n_deletes > 0) {	/* process the first file only */
	int fd, k, ret;

	fd = LB_open (LB_name, LB_WRITE, NULL);
	if (fd < 0) {
	    if (Verbose)
		printf ("LB_open (%s) failed, (ret %d) - %s\n", 
					LB_name, fd, prog_name);
	    exit (1);
	}
	for (k = 0; k < n_deletes; k++) {
	    ret = LB_delete (fd, delete_ids[k]);
	    if (ret < 0 && Verbose) {
		printf ("LB_delete (%s) failed, (ret %d) - %s\n",
			    	LB_name, ret, prog_name);
	    }
	}
	LB_close (fd);
	if (optind != argc - 1)
	    printf ("Only the first LB (%s) processed - %s\n", 
					LB_name, prog_name);
	exit (0);
    }

    while (optind <= argc - 1) {
	int ret;

	if (n_clears) {		/* remove messages */
	    int fd;
    
	    fd = LB_open (argv[optind], LB_WRITE, NULL);
	    if (fd < 0) {
		if (Verbose)
		    printf ("LB_open (%s) failed (ret %d) - %s\n", 
						argv[optind], fd, prog_name);
	    }
	    else {
		ret = LB_clear (fd, n_clears);
		if (ret < 0 && Verbose) {
		    printf ("LB_clear (%s) failed (ret = %d) - %s\n",
						argv[optind], ret, prog_name);
		}
		LB_close (fd);
	    }
	}
	else {			/* remove LBs */
	    ret = LB_remove (argv[optind]);
	    if (ret == LB_NOT_EXIST) {
		if (Verbose)
		    printf ("LB (%s) not found - %s\n", argv[optind], prog_name);
	    }
	    else if (ret < 0) {
		if (Verbose)
		    printf ("LB_remove (%s) failed (ret = %d) - %s\n",
				argv[optind], ret, prog_name);
	    }
	}
	optind++;
    }

    exit (0);
}

/**************************************************************************

    Removes all LBs and files in directory "dir_path" and all its 
    subdirectories.

**************************************************************************/

static void Recursive_remove (char *dir_path) {
    DIR *Dir = NULL;			/* directory struct */
    struct dirent *dp;
    char buf[LB_RM_NAME_SIZE * 2 + 8], *p;

    if (strlen (dir_path) >= LB_RM_NAME_SIZE) {
	MISC_log ("dir path (%s) too long - not processed\n", dir_path);
	return;
    }

    Dir = opendir (dir_path);
    if (Dir == NULL) {
	MISC_log ("opendir (%s) failed, errno %d\n", dir_path, errno);
	exit (1);
    }

    strcpy (buf, dir_path);
    if (buf[strlen (buf) - 1] != '/')
	strcat (buf, "/");
    p = buf + strlen (buf);

    while ((dp = readdir (Dir)) != NULL) {
	struct stat st;

	if (strcmp (dp->d_name, ".") == 0 ||
	    strcmp (dp->d_name, "..") == 0)
	    continue;

	if (strlen (dp->d_name) > LB_RM_NAME_SIZE) {
	    MISC_log ("name (%s) too long - not processed\n", dp->d_name);
	    continue;
	}
	strcpy (p, dp->d_name);
	if (stat (buf, &st) < 0) {
	    if (errno != ENOENT)
		MISC_log ("stat (%s) failed, errno %d\n", buf, errno);
	    continue;
	}

	if (st.st_mode & S_IFDIR) {
	    Recursive_remove (buf);
	    continue;
	}

	if (st.st_mode & S_IFREG) {	/* a regular file */
	    char *pt;
	    int i;

	    pt = Keep_files;
	    for (i = 0; i < N_keep_files; i++) {
		if (Match_star_wild_card (pt, MISC_basename (buf)) == 0)
		    break;
		pt += strlen (pt) + 1;
	    }
	    if (i < N_keep_files)
		continue;

	    if (LB_remove (buf) < 0 && Verbose)
		MISC_log ("LB_remove %s failed\n", buf);
	}
    }
}

/**************************************************************************

    Matches strings "s2" with "s1" which can have the wild card """.
    Returns 0 if they match or non-zero otherwise.

**************************************************************************/

static int Match_star_wild_card (char *s1, char *s2) {
    int ind;
    char *p, *p1, *p2;

    ind = 0;
    p = s2;
    p1 = s1;
    while (1) {
	int len, star;

	star = 0;
	while (*p1 == '*') {
	    p1++;
	    star = 1;
	}

	p2 = p1;
	while (*p2 != '*' && *p2 != '\0')
	    p2++;

	len = p2 - p1;
	if (star) {
	    char *t, *t1, *t2;
	    int i;

	    if (len == 0)
		return (0);

	    t = p;		/* find p1 of len in p */
	    while (1) {
		t1 = t;
		t2 = p1;
		for (i = 0; i < len; i++) {
		    if (*t1 == '\0')
			return (1);
		    if (*t1 != *t2)
			break;
		    t1++;
		    t2++;
		}
		if (i >= len)
		    break;
		t++;
	    }
	    p = t + len;
	}
	else {
	    if (strncmp (p1, p, len) != 0)
		return (1);
	    p += len;
	}
	if (p1[0] == '\0' && p[0] != '\0')
	    return (1);
	if (p[0] == '\0')
	    return (0);
	p1 = p2;
    }
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage () {

    printf ("Usage: lb_rm options LB_name1, LB_name2 ...\n");
    printf ("       Remove LBs or messages in LBs\n");
    printf ("       options:\n");
    printf ("       -c remove all messages\n");
    printf ("       -i msg_id1 msg_id2 ... (Delete messages;\n");
    printf ("          LB_DB type and single LB only)\n");
    printf ("       -r n_messages (remove \"n_messages\" messages)\n");
    printf ("       -v (verbose mode; Without this option, LB function\n");
    printf ("           call errors are not reported)\n");
    printf ("       -a dir (remove all LBs in dir and all its subdirs)\n");
    printf ("       -k fname (keep \"fname\" when removing all - must be before -a)\n");
    printf ("       -h print usage information\n");
    printf ("       Options -c, -i and -r are mutually exclusive.\n");
    printf ("       examples:\n");
    printf ("           lb_rm lb1 lb2 (remove LBs lb1 and lb2)\n");
    printf ("           lb_rm -c lb1 lb2 (remove all msgs in lb1 and lb2)\n");
    printf ("           lb_rm -i 12 35 lb1 (delete messages of IDs 12 and 35 in lb1)\n");
    printf ("           lb_rm -r 12 lb1 lb2 (remove 12 messages in lb1 and lb2)\n");
    printf ("\n");

    exit (0);
}

