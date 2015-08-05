/**************************************************************************

   Module:  iti_main.c

   Description:
        This file defines the "main" Initialize Task Information (ITI)
        routine.  Task information is the Task Attributes Table (TAT).

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/08 14:49:02 $
 * $Id: iti_main.c,v 1.11 2012/05/08 14:49:02 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <stdlib.h>            
#include <string.h>           
#include <sys/param.h>       
#include <dirent.h>

#define ITI_MAIN
#include <iti_globals.h>
#undef ITI_MAIN

/* Constants */
#define NAME_LEN 128

/* Static Global Variables */
static char Ttcf_fname[MAXPATHLEN+1];       /* Task Table Configuration File */
static char Extensions_fname[MAXPATHLEN+1]; /* Task Table Extensions Files */
static char Task_table_basename[MAXPATHLEN+1]; /* Task Table Base File Name */

/* Static Function Prototypes */
static int Get_command_line_args(int argc, char **argv, int *startup );
static int Get_next_file_name( char *dir_name, char *basename, 
                               char *buf, int buf_size );

/**************************************************************************

   Description:
      Initialize Task Attribute Table (TAT).

   Input: 
      argc - number of command line arguments
      argv - command line arguments

   Output: 

   Returns: 
      Exits with 0 exit code on success, or non-zero exit code on error.

**************************************************************************/
int main(int argc, char *argv[]){

    int retval ;
    int startup = STARTUP;
    int ret;

    char ext_name[ MAXPATHLEN+1 ], *call_name;

    char *tat_dir = NULL;

    if( (retval = Get_command_line_args( argc, argv, &startup ) ) != 0 )
        exit(1) ;

    if( (startup == STARTUP) || (startup == CLEAR) ){

       /* Open the TAT LB for write and clear out all the messages. */
       if( ((retval = ORPGDA_write_permission( ORPGDAT_TAT )) < 0)
                                    ||
           ((retval = ORPGDA_clear( ORPGDAT_TAT, LB_ALL )) < 0) ){

           LE_send_msg( GL_ERROR, "ORPGDA_clear( ORPGTAT_TAT ) Failed: %d\n", retval );
           exit(1);

       }
    }

    (void) ORPGMISC_init(argc, argv, 5000, 0, -1, 1 ) ;

    /* Default Task Tables Configuration File. */
    ret = MISC_get_cfg_dir (Ttcf_fname, MAXPATHLEN);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed (%d)\n", ret);
	return (-1);
    }

    /* We need to concatenate ... "/task_attr_table" (plus terminating null) */
    if (strlen(Ttcf_fname) <= (MAXPATHLEN - sizeof("/task_attr_table"))) 
       (void) strcat(Ttcf_fname, "/task_attr_table");

    else{

       (void) fprintf(stderr,
                      "CFG dir \"%s\" + \"/%s\" too long (%d chars max)!\n",
                      Ttcf_fname, "task_attr_table", MAXPATHLEN) ;
       return(-1) ;

    }

    /* Default Task Table Extensions File Path. */
    ret = MISC_get_cfg_dir (Extensions_fname, MAXPATHLEN);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed (%d)\n", ret);
	return (-1);
    }

    /* We need to concatenate ... "/extensions" (plus terminating null) */
    if (strlen(Extensions_fname) <= (MAXPATHLEN - sizeof("/extensions"))) 
       (void) strcat(Extensions_fname, "/extensions");

    else{

       (void) fprintf(stderr,
                      "CFG dir \"%s\" + \"/\" too long (%d chars max)!\n",
                      Extensions_fname, MAXPATHLEN) ;
       return(-1) ;

    }

    strcat( Task_table_basename, "task_attr_table" );

    LE_send_msg(GL_INFO, "Task Table Cfg. File: %s", Ttcf_fname) ;

    if( (startup == STARTUP) || (startup == CLEAR) ){

       if( startup == CLEAR )
          exit(0);

       /* Write TAT directory record to reserve LB msg ID. */
       retval = ORPGDA_write( ORPGDAT_TAT, tat_dir, 0, ORPGTAT_TAT_DIRECTORY );
       if( retval < 0 ){

          LE_send_msg( GL_ERROR, "TAT Directory Record Write Failed (%d)\n", retval );
          exit(1);

       }

       /* Parse the primary ASCII Task Attribute Table. */
       if (ITI_ATTR_init_table( Ttcf_fname, &tat_dir ) < 0) {

          LE_send_msg(GL_ERROR, "Failed to initialize Task Attribute Table") ;
          exit(1) ;

       }

       /* Parse all the Task Attribute Table Extensions. */
       call_name = Extensions_fname;
       while(1){

          ret = Get_next_file_name( call_name, Task_table_basename, 
                                    ext_name, MAXPATHLEN );
          if( ret != 0 )
             break;

          LE_send_msg( GL_INFO, "--->Reading Task Table Extension %s\n",
                       ext_name );
          if( ITI_ATTR_init_table( (const char*) ext_name, &tat_dir ) < 0 )
             exit(1);

          call_name = NULL;

       }

       /* Write the TAT directory record. */
       if( ITI_ATTR_write_directory_record( tat_dir ) < 0 )
          exit(1);
    }

    exit(0) ;

/*END of main()*/
}

/**************************************************************************
   Description: 
      Read the command-line options

   Input: 
      argc - number of command line arguments
      argv - the command line arguments
      startup - either STARTUP or RESTART

   Output: 

   Returns: 
      0 if successful; -1 otherwise

   Notes:
 **************************************************************************/
static int Get_command_line_args(int argc, char **argv, int *startup_action ){

    int  err ;
    char startup[255];
    int input ;
    int ret, retval = 0 ;

    err = 0;
    while ((input = getopt (argc, argv, "vht:")) != EOF) {
    switch (input) {

       case 't':
          if( strlen( optarg ) < 255 ){
  
             ret = sscanf(optarg, "%s", startup) ;
             if (ret == EOF) {
  
                LE_send_msg(GL_INFO, "sscanf failed to read startup\n") ;
                err = 1 ;
   
             }
             else{
  
                if( strstr( startup, "startup" ) != NULL )
                    *startup_action = STARTUP;
   
                else if( strstr( startup, "restart" ) != NULL )
                    *startup_action = RESTART;

                else if( strstr( startup, "clear" ) != NULL )
                    *startup_action = CLEAR;
  
                else
                    *startup_action = RESTART;
 
             }

          }
          else
             err = 1;
   
          break;  

        case 'v':
           Verbose = 1;
           break;

        case 'h':
        case '?':
            err = 1;
            break;

        } /*endswitch*/

    } /*endwhile command-line characters to read*/

    if (err == 1) {

        (void) printf ("Usage: %s [options]\n", argv[0] );
        (void) printf ("\n") ;
        (void) printf ("OPTIONS\n") ;
        (void) printf ("\t-t startup mode [optional:  default restart]\n" );
        (void) printf ("\n\t") ;

        retval = -1 ;
    }


    return(retval) ;

/*END of Read_options()*/
}

/*******************************************************************

   Description:

      Returns the name of the first (dir_name != NULL) or the next 
      (dir_name = NULL) file in directory "dir_name" whose name matches 
      "basename".*. The caller provides the buffer "buf" of size 
      "buf_size" for returning the file name. 

   Inputs:  
      dir_name - directory name or NULL
      basename - product table base name
      buf - receiving buffer for next file name
      buf_size - size of receiving buffer

   Outputs:
      buf - contains next file name

   Returns:
      It returns 0 on success or -1 on failure.

*******************************************************************/
static int Get_next_file_name( char *dir_name, char *basename, 
                               char *buf, int buf_size ){

    static DIR *dir = NULL;	/* the current open dir */
    static char saved_dirname[MAXPATHLEN] = "";
    struct dirent *dp;

    /* If directory is not NULL, open directory. */
    if (dir_name != NULL) {

	int len;

	len = strlen (dir_name);
	if (len + 1 >= MAXPATHLEN) {
	    LE_send_msg (GL_ERROR, 
		"dir name (%s) does not fit in tmp buffer\n", dir_name);
	    return (-1);
	}
	strcpy (saved_dirname, dir_name);
	if (len == 0 || saved_dirname[len - 1] != '/')
	    strcat (saved_dirname, "/");
	if (dir != NULL)
	    closedir (dir);
	dir = opendir (dir_name);
	if (dir == NULL)
	    return (-1);
    }

    if (dir == NULL)
	return (-1);

    /* Read the directory. */
    while ((dp = readdir (dir)) != NULL) {

	struct stat st;
	char fullpath[2 * MAXPATHLEN];

	if (strncmp (basename, dp->d_name, strlen (basename)) != 0)
	    continue;

	if (strlen (dp->d_name) >= MAXPATHLEN) {
	    LE_send_msg (GL_ERROR, 
		"file name (%s) does not fit in tmp buffer\n", dp->d_name);
	    continue;
	}
	strcpy (fullpath, saved_dirname);
	strcat (fullpath, dp->d_name);
	if (stat (fullpath, &st) < 0) {
	    LE_send_msg (GL_ERROR, 
		"stat (%s) failed, errno %d\n", fullpath, errno);
	    continue;
	}
	if (!(st.st_mode & S_IFREG))	/* not a regular file */
	    continue;

	if (strlen (fullpath) >= buf_size) {
	    LE_send_msg (GL_ERROR,
		"caller's buffer is too small (for %s)\n", fullpath);
	    continue;
	}
	strcpy (buf, fullpath);
	return (0);
    }

    return (-1);

/* End of Get_next_file_name() */
}
