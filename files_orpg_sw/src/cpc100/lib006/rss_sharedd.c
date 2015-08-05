/****************************************************************
		
    Module: rss_sharedd.c	
				
    Description: This module contains common internal functions for
	the RSS server.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 21:32:01 $
 * $Id: rss_sharedd.c,v 1.22 2012/07/27 21:32:01 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 * $Log: rss_sharedd.c,v $
 * Revision 1.22  2012/07/27 21:32:01  jing
 * Update
 *
 * Revision 1.16  2000/08/21 20:51:59  jing
 * @
 *
 * Revision 1.15  1999/04/01 14:32:53  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.13  1999/03/31 19:36:42  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.11  1998/12/01 21:17:58  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.8  1998/06/19 17:05:38  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:44:40  cm
 * Initial revision
 *
 * 
*/

/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>

/*** Local include files ***/
#include <rss.h>
#include "rss_def.h"
#include <rmt.h>
#include <misc.h>


/*** Definitions / macros / types ***/

#define STRING_SIZE 256 /* size of the string processed */


/* private functions */
#ifdef RWOTHERS_REQUIRED
static int Check_file_mode (char *path);
#endif


/****************************************************************
			
    Description: This function reads in the path list from the RMT 
	configuration file. The list is used for access permission 
	check of a path name. If the path name "path" matches one
	of the listed path names, the path name is assumed to be 
	permissible and function returns RSS_SUCCESS. If non of the
	listed paths can be matched it returns RSS_FAILURE.

    Input:	path - the file name to be checked

    Returns: RSS_SUCCESS if permission is OK or RSS_FAILURE 
	otherwise.

    Notes: The configuration file is read when this function is 
	called in the first time. A path name in the configuration 
	file is a line lead with "Path:". In case the configuration 
	file reading fails, the function will set path_cnt, the number
	of listed paths to be 0. The file will never be read again 
	and subsequent permission check will return RSS_FAILURE. 
	Error messages are written to the RMT log file in this case.

	Leading spaces and TABs are ignored in a line. All path
	names in the configuration file must be full path names
	starting with "/".

	If path_cnt = 0, we don't print permission check failure
	messages because there will be too many messages otherwise.

****************************************************************/

int 
  RSS_check_file_permission 
  (
    char *path			/* path name to be checked */
) {
    static char *path_list[PATH_LIST_SIZE];  /* path list */
    static int path_cnt = -1;	/* number of paths configured */
    static int update_cnt = 0;	/* keeps track of the config update */
    int i, upd_c;
    char *fname, buf[256];

    if (path_cnt < 0)
	memset (path_list, 0, PATH_LIST_SIZE * sizeof (char *));
    fname = RMT_get_conf_file (&upd_c);
    if (path_cnt < 0 || upd_c != update_cnt) {	/* read the config file */
        char name[NAME_SIZE];
        char line[NAME_SIZE];
        FILE *fl;

        path_cnt = 0;		/* set to 0. we read the file only once */
	update_cnt = upd_c;

	/* get file handler */
	fl = MISC_fopen (fname, "r");
	if (fl == NULL) {
	    MISC_log ("Cannot open conf file %s", fname);
	    return (RSS_FAILURE);
	}

        /* reads in the file */
        while (fgets (line, NAME_SIZE, fl) != NULL) {
	    int len;
	    char *p;

	    p = line;
	    while (*p == ' ' || *p == '\t')
		p++;
	    if (strncmp (p, "Path:", 5) == 0) {
	        sscanf (p + 5, "%s", name);
	        len = strlen (name);
	        if (len == 0) continue;
		if (name[len - 1] == '/')
		    name[len - 1] = '\0';
		if (path_list[path_cnt] != NULL)
		    MISC_free (path_list[path_cnt]);
	        path_list[path_cnt] = (char *)MISC_malloc (len + 1);
	        strcpy (path_list[path_cnt], name);
		path_cnt++;
	        if (path_cnt >= PATH_LIST_SIZE) {
		    MISC_log ("Too many \"Path:\" in %s - Truncated", fname);
		    break;
		}
	    }
        }

        MISC_fclose (fl);
    }
    if (path == NULL)
	return (RSS_SUCCESS);

    /* check permission */
    path = MISC_expand_env (path, buf, 256);
    for (i = 0; i < path_cnt; i++){
	if (strncmp (path, path_list[i], strlen (path_list[i])) == 0) 
#ifdef RWOTHERS_REQUIRED
	    return (Check_file_mode (path));
#else
	    return (RSS_SUCCESS);
#endif
    }
    if (path_cnt > 0) {  /* we don't print if path_cnt = 0 */
        MISC_log ("File %s permission denied", path);    
    }

    return (RSS_FAILURE);
}

#ifdef RWOTHERS_REQUIRED

/****************************************************************

    Descriptions: This function checks the mode of the file "path".

    Input: path - the file to be checked

    Returns: It returns RSS_SUCCESS if the file is read/write 
	accessible for "others". It returns RSS_FAILURE otherwise.

    Notes: This function is not used for the moment. To use is 
	RWOTHERS_REQUIRED need to be defined.

*****************************************************************/

static int 
  Check_file_mode 
  (
    char *path
) {
    struct stat buf;

    /* get file status */
    if (MISC_stat (path, &buf) == -1) {
	if (errno == ENOENT || errno == ENOTDIR) /* file does not exist */
	    return (RSS_SUCCESS); 
        MISC_log ("stat call failed (File %s); errno = %d", path, errno);    
	return (RSS_FAILURE);
    }

    /* check file mode */
    if (((buf.st_mode & S_IROTH) != 0) && ((buf.st_mode & S_IWOTH) != 0))
	return (RSS_SUCCESS); 
	
    MISC_log ("File %s permission denied (mode)", path);    

    return (RSS_FAILURE);
}

#endif


/****************************************************************
	
    Description: This function extends a path by adding the $HOME 
		part if the path name is incomplete.

    Input/Output:	path - the path to be processed

    Returns: The function returns RSS_SUCCESS on success or 
	RSS_FAILURE if the "HOME" environment variable is not found
	or the path buffer size is too small.

    Notes: This function is similar to RSS_form_explicit_local_path.
	It is a version for the server.

*****************************************************************/

int
  RSS_add_home_path
  (
      char *path,		/* input/output path */
      int path_size		/* buffer size for "path" */
) {
    char home_path[NAME_SIZE];	/* user's home path */
    char tmp[NAME_SIZE];
    
    if (path[0] != '/' && path[0] != '$') {
	int hlen, plen;
        char *home;

	home = getenv ("HOME");
	if (home == NULL) 
	     return (RSS_FAILURE);
	strncpy (home_path, home, NAME_SIZE);
        home_path[NAME_SIZE - 1] = '\0';
	hlen = strlen (home_path);     

        /* append a "/" */
	if (home_path[0] != '\0' && hlen < NAME_SIZE - 1 && 
			home_path[hlen - 1] != '/') {
	    home_path[hlen] = '/';
	    home_path[hlen + 1] = '\0';
	    hlen++;
	}

	plen = strlen (path);
	if (plen + 1 > NAME_SIZE || plen + hlen + 1 > path_size)
	    return (RSS_FAILURE);
	strcpy (tmp, path);
	strcpy (path, home_path);
	strcat (path, tmp);
    }

    return (RSS_SUCCESS);
}
