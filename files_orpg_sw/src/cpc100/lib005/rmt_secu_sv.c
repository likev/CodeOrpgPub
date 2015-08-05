/****************************************************************
		
	File: sec_rmtd.c	
				
	2/23/94

	Purpose: This module contains client authentication 
	(security check) functions for the RMT server. 
	This module will not be linked into the executable if
	RMT_SECURITY_ON is not defined.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/14 17:06:54 $
 * $Id: rmt_secu_sv.c,v 1.5 2005/04/14 17:06:54 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


/*** Local include files ***/

#include <rmt.h>
#include <misc.h>
#include "rmt_def.h"


static char Password[PASSWORD_SIZE] = ""; /* The RMT server's password */


/****************************************************************
			
	SECD_initialize_sec_rmtd()		Date: 4/13/94

	This function performs the RMT initiation security check. This is 
	called only if the security RMT is required.

	This function first try to read the RMT password from the file 
	$HOME/.password. If the file is not found, the user is asked to
	type in the password. The password is, then compared with the RMT 
	password stored in file $HOME/.rmt_passwd. If the two passwords 
	are identical, the RMT deamon can be initiated. 

	The RMT password is stored in $HOME/.rmt_passwd in a coded form.

	The mechanism of getting the password from file $HOME/.password
	is designed for operational use of the RMT tool. The file should
	be created right before starting the system and removed as soon as 
	all processes are started. The file should be readable only by the 
	user.

	This function returns SUCCESS on success or RMT_ABORT if the password
	check is failed. On success, the "Password" string is initialized.
*/

int
  SECD_initialize_sec_rmtd () 
{
    char f_name[128];
    FILE *fl;
    char pw[PASSWORD_SIZE];
    char code [PASSWORD_SIZE];
    char new_code[PASSWORD_SIZE];
    int code_length;
    int len;
    int i;
    struct stat status;
    struct timeval tp;
    struct timezone tzp;
    unsigned long seed;
    char *hm;

    hm = getenv ("HOME");
    if (hm == NULL) {
	MISC_log ("HOME env undefined\n");
	return (RMT_ABORT);
    }	

    /* get the password */
    sprintf (f_name, "%s/.password", hm);
    fl = MISC_fopen (f_name, "r");     /* first try the file */
    if (fl != NULL) {
	MISC_log ("Read password from file\n");
	len = fread (pw, 1, PASSWORD_SIZE - 1, fl);
	pw[len] = '\0';
	MISC_fclose (fl);
    }
    else {  /* ask the user */
	strcpy (pw, (char *)getpass ("Please input the password: "));
    }

    /* check the password */

    /* name of the encrypt password file */
    sprintf (f_name, "%s/.rssd/.rmt_passwd", hm);

    /* check the file owner */
    if (MISC_stat (f_name, &status) < 0) {
	MISC_log ("Failed in finding status of %s\n", f_name);
	return (RMT_ABORT);
    }	
	
    if (status.st_uid != 0) {
	MISC_log ("File %s must be owned by root\n", f_name);
	return (RMT_ABORT);
    }	

    /* open and read the encrypt password file */
    fl = MISC_fopen (f_name, "r");
    if (fl == NULL) {
	MISC_log ("File %s is not found\n", f_name);
	return (RMT_ABORT);
    }	
    code_length = fread (code, 1, PASSWORD_SIZE, fl);
    if (code_length <= 0) {
	MISC_log ("Failed in reading file %s (size read = %d)\n", f_name, code_length);
	return (RMT_ABORT);
    }
    MISC_fclose (fl);

    /* encrypt the new password */
    len = ENCR_encrypt (pw, PASSWORD_SIZE, new_code);

    /* verify */
    if (len != code_length) {
	MISC_log ("Different encrypt methods\n");
	return (RMT_ABORT);
    }

    for (i = 0; i < len ; i++) {
	if (code[i] != new_code[i]) {
	    MISC_log ("Password check failed\n");
	    return (RMT_ABORT);
	}
    }

    /* save the password */
    strncpy (Password, pw, PASSWORD_SIZE);
    Password [PASSWORD_SIZE - 1] = '\0';

    /* initialize the random number generator */
    gettimeofday(&tp,&tzp);
    seed = ((unsigned long)tp.tv_sec * 100) + ((unsigned long)tp.tv_usec / 10000);
    srand (seed);

    return (SUCCESS);
}

/****************************************************************
			
	SECD_get_random_key()			Date: 4/20/94

	This function generates a random key for password encryption.

	The first 4 bytes of the key contain a random number and the next
	4 bytes are generated by the current time.

	The 8 bytes must be non-zero to be part of a string.
*/

void
  SECD_get_random_key (char *buf, int len) 
{
    unsigned long *lpt;
    unsigned char *cpt;
    int i;
    struct timeval tp;
    struct timezone tzp;

    if (len < 8) return;

    lpt = (unsigned long *)buf;
    *lpt = rand();

    lpt++;
    gettimeofday (&tp,&tzp);
    *lpt = ((unsigned long)tp.tv_sec * 100) + ((unsigned long)tp.tv_usec / 10000);

    cpt = (unsigned char *) buf;
    for (i = 0; i < 8; i++) {
	if (cpt[i] == '\0') cpt[i] = (rand() % 255) + 1;
    }

    return;
}

/****************************************************************

	This function returns RMT_TURE if password is needed. Otherwise
	it returns RMT_FALSE.
*/

char *SECD_get_password ()
{

    return (Password);
}
