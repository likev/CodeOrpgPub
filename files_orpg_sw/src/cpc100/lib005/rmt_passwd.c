/****************************************************************
		
	File: rmt_passwd.c	
				
	4/13/94

	Purpose: This is the program for setting and modifying
	the RMT passwd file.

	Files used: 
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 17:25:35 $
 * $Id: rmt_passwd.c,v 1.17 2014/03/18 17:25:35 jeffs Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <rmt.h>
#include "rmt_def.h"



int main (int argc, char **argv, char **arge)
{
    char f_name [256], d_name[256];
    FILE *fl;
    char password [256], password_rpt [256];
    char buf[256];
    char old[256];
    int len, ret, old_len;
    int i;
    extern int errno;
    char *hm;


    /* the dir and file name */
    hm = getenv ("HOME");
    if (hm == NULL) {
	printf ("HOME env is not defined\n");
	exit (1);
    }
    if (strlen (hm) < 3) {
        if (argc == 1 || strlen(argv[1]) == 0) {
	    printf ("Are you running this by root? You need to use: \n");
	    printf ("rmt_passwd rmt_user_home_directory_name\n");
	    exit (0);
	}
        sprintf (f_name, "%s/.rssd/.rmt_passwd", argv[1]);
        sprintf (d_name, "%s/.rssd", argv[1]);
    }
    else {
        sprintf (f_name, "%s/.rssd/.rmt_passwd", hm);
        sprintf (d_name, "%s/.rssd", hm);
    }

    /* set root */
    if (setuid (0) < 0){
	printf ("Failed in setting uid to 0\n");
	exit (0);
    }

    /* create directory if it does not exit */
    ret = mkdir(d_name, 0755);
    if (ret < 0 && errno != EEXIST) {
	printf ("Failed in creating directory %s\n", d_name);
	exit (0);
    }

    /* open the file */
    fl = fopen (f_name, "r+");
    if (fl == NULL) {
	char ch;

	printf ("File %s does not exist\n", f_name);
	printf ("Do you like to create one? (y/n) ");
	fread (&ch, 1, 1, stdin);
	if (ch != 'y') exit (0);
	fflush (stdin);  /* flush the input */

	printf ("Creating ... \n");
	fl = fopen (f_name, "w");

        if (fl == NULL) {
	    printf ("Failed in creating the file %s\n", f_name);
	    exit (0);
	}
	if (chmod(f_name, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP) < 0) {
	    printf ("Failed in setting mode for file %s\n", f_name);
	    exit (0);
	}

	strcpy (password, (char *)getpass ("Please type in a password: "));
	if (strlen (password) == 0) {
	    fclose (fl);
	    unlink (f_name);
	    printf ("Empty password - file %s is not created\n", f_name);
	    exit (0);
	}
	    
	len = ENCR_encrypt (password, 256, buf);
	if (len < 0) {
	    fclose (fl);
	    unlink (f_name);
	    printf ("Failed in ENCR_encrypt - file %s is no created\n", f_name);
	    exit (0);
	}

	ret = fwrite (buf, 1, len, fl);
	if (ret < len) {
	    fclose (fl);
	    unlink (f_name);
	    printf ("Failed in fwrite - file %s is no created\n", f_name);
	    exit (0);
	}
	fclose (fl);

	exit (0);
    }

    printf ("Updating ... \n");

    /* read the old */
    old_len = fread (old, 1, 256, fl);
    if (old_len <= 0) {
	printf ("Failed in reading file %s (size read = %d)\n", f_name, old_len);
	exit (0);
    }

    strcpy (password, (char *)getpass ("Please input the old password: "));
    len = ENCR_encrypt (password, 256, buf);
    if (len < 0) {
	fclose (fl);
	printf ("Failed in ENCR_encrypt - file %s is no modified\n", f_name);
	exit (0);
    }

    for ( i=0; i<len; i++) {
	if (buf[i] != old[i]) {
	    printf ("Incorrect password\n");
	    exit (0);
	}
    }

    strcpy (password, (char *)getpass ("Please input the new password: "));
    if (strlen(password) == 0) {
	printf ("Empty password is not allowed\n");
	exit (0);
    }

    strcpy (password_rpt, (char *)getpass ("Please input the new password again: "));
    if (strcmp(password, password_rpt) != 0) {
	printf ("Two passwords are different - exit\n");
	exit (0);
    }
	    
    len = ENCR_encrypt (password, 256, buf);
    if (len < 0) {
	fclose (fl);
	printf ("Failed in ENCR_encrypt - file %s is no modified\n", f_name);
	exit (0);
    }

    fseek (fl, 0, 0);

    ret = fwrite (buf, 1, len, fl);
    if (ret < len) {
	fclose (fl);
	printf ("Failed in fwrite - file %s may be corrupted\n", f_name);
	exit (0);
    }
    fclose (fl);

    exit (0);
}
