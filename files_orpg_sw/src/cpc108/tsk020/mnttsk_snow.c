/*************************************************************************

   Module:  mnttsk_hydromet.c

   Description:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:52 $
 * $Id: mnttsk_snow.c,v 1.3 2005/12/27 16:41:52 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <errno.h>
#include <sys/types.h>         /* open(),lseek()                          */
#include <sys/stat.h>          /* open()                                  */
#include <fcntl.h>             /* open()                                  */

#include <orpg.h>
#include <saa_file_names.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define STARTUP         1
#define RESTART         2
#define CLEAR           3
#define CHECK           4

#define LOG_STATUS_MSGS 0

extern int errno;

/* Static Global Variables */
static char Tmp_path[ORPG_PATHNAME_SIZ] ;


/* Static Function Prototypes */
static int Remove_database_files(void) ;
static int Check_files(void) ;
static int Get_command_line_args( int argc, char **argv, int *startup_action );


/**************************************************************************
    Description:
       Initialization routine for Snow Accumulation Algorithms files.

    Input:
       argc - the number of command line arguments.
       argv - the command line arguments.

    Output:

    Returns:
       On failure of any sort, exits with exit code > 0.  Normal termination,
       exit code is 0.

    Notes:
 **************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;
    Mrpg_state_t rpg_state;

    /* Initialize log-error services. */
    ORPGMISC_init( argc, argv, 200, 0, -1, LOG_STATUS_MSGS );

    /* Get command line arguments.  On failure, exit. */
    if( (retval = Get_command_line_args( argc, argv, &startup_action ) != 0) )
       exit(retval);

    /* Get the current RPG state. */
    retval = ORPGMGR_get_RPG_states( &rpg_state );
    if( retval < 0 )
       rpg_state.state = MRPG_ST_SHUTDOWN;

    
    /* If startup_action indicates to check the files, do the following. */
    if( startup_action == CHECK ){

         if( rpg_state.state != MRPG_ST_OPERATING ){

             LE_send_msg(GL_INFO,"CHECK DUTIES:") ;

             LE_send_msg( GL_INFO,
                          "\t1. Check the Snow Accumulation Hourly File.") ;
             LE_send_msg( GL_INFO,
                          "\t2. Check the Snow Accumulation Total File.") ;

             Check_files();

             exit(0);

         }
         else{

            LE_send_msg( GL_ERROR, "Can Not Check Snow Accumulation Hourly and Totals Files Because:\n" );
            LE_send_msg( GL_ERROR, "--->RPG State is MRPG_ST_OPERATING\n" );
            exit(0);

         }

    }

    /* If startup_action is clear_hydro, then do the following ... */
    else if ( startup_action == CLEAR ){

        if( rpg_state.state == MRPG_ST_OPERATING ){

           retval = EN_post_event( ORPGEVT_RESET_SAAACCUM );
           if( retval < 0 )
               LE_send_msg( GL_ERROR, 
                            "EN_post_event( ORPGEVT_RESET_SAAACCUM ) Failed (%d)\n",
                            retval );
           
           else
               LE_send_msg( GL_INFO, "ORPGEVT_RESET_SAACCUM event posted\n" );

           exit(0);
           

        }
        else{

            LE_send_msg( GL_INFO,"CLEAR HYDROMET DATABASE DUTIES\n" ) ;
            LE_send_msg(GL_INFO,"\t1. Delete %s", SAAHOURLY ) ;
            LE_send_msg(GL_INFO,"\t2. Delete %s", SAATOTAL ) ;

            if (Remove_database_files() < 0) {

                LE_send_msg( GL_INFO, "Remove_database_files() failed\n!") ;
                exit(7) ;

            }

            LE_send_msg( GL_INFO, "Clearing SAAUSERSEL of All Messages ...\n" );
            retval = ORPGDA_clear( SAAUSERSEL, LB_ALL );
            if( retval < 0 ){

                LE_send_msg( GL_INFO, "ORPGDA_clear(SAAUSERSEL) failed: %d\n", retval );
                exit(7);

            }

        }

    }

    /* Normal termination. */
    exit(0) ;

/*END of main()*/
}


#define DB_FILES_NAMELEN 12
#define DB_FILES_NAMESIZ ((DB_FILES_NAMELEN) + 1)
#define CFG_DIR_NAME_SIZE 128

/**************************************************************************
    Description:
       Removes Snow Accumulation Algorithms files.

    Output:

    Returns:
       On failure of any sort, returns -1i, otherwise 0. 

    Notes:
 **************************************************************************/
static int Remove_database_files(void){

    char *db_files[] = { SAAHOURLY, SAATOTAL } ;
    int i ;
    int retval ;
    char workpath[ORPG_PATHNAME_SIZ] ;

    (void) memset(workpath, 0, sizeof(workpath)) ;
    retval = MISC_get_work_dir(workpath, sizeof(workpath)) ;
    if (retval < 0) {

        LE_send_msg(GL_INFO, "MISC_get_work_dir() failed: %d", retval) ;
        return(-1) ;

    }

    /* Delete the data base files. */
    for (i=0; i < 2; ++i) {

        (void) memset(Tmp_path, 0, sizeof(Tmp_path)) ;
        if ((strlen(workpath) + strlen("/") +
                                strlen(db_files[i])) < sizeof(Tmp_path)) {

            (void) sprintf(Tmp_path, "%s/%s", workpath, db_files[i]) ;
            LE_send_msg(GL_INFO,"Deleting %s ...", db_files[i]) ;
            (void) unlink((const char *) Tmp_path) ;

        }
        else 
            LE_send_msg(GL_INFO,"%s pathname exceeds %d bytes!",
                        db_files[i], sizeof(Tmp_path) - 1) ;
        
    }

    return(0) ;

/*END of Remove_database_files()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (CLEAN, CLEAR, or RESTART)
      input_file_path - path of terrain data file (excludes file name) 

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int Argc, char **Argv, int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (Argc, Argv, "ht:")) != EOF) {

      switch (c) {
  
         case 't':
            if( strlen( optarg ) < 255 ){    

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                   LE_send_msg(GL_INFO, "sscanf failed to read startup action\n") ;
                   err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "check" ) != NULL )
                     *startup_action = CHECK;

                  else
                     *startup_action = CHECK;

               }
            }
            else
               err = 1;

            break;
 
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1 ){

      printf ("Usage: %s [options]\n", MISC_string_basename(Argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Action (optional - default: restart)\n" );
      exit (1);

   }
  
   return (0);

}

#define FILE_NAME_SIZE    32

/******************************************************************************

   Description:
      Checks for empty snow accumulation files.  If the file is empty, it is removed.
      The files which are checked are:

         SAATOTAL.DAT
         SAAHOURLY.DAT

   Inputs:

   Outputs:

   Returns:
      -1 on error, 0 on success.

   Notes:

******************************************************************************/
static int Check_files(){

   int ret, path_size;
   char *path_str = NULL, workpath[ORPG_PATHNAME_SIZ];
   struct stat stats;
   
   (void) memset(workpath, 0, sizeof(workpath)) ;
   ret = MISC_get_work_dir(workpath, sizeof(workpath)) ;
   if( ret < 0 ){

      LE_send_msg(GL_INFO, "MISC_get_work_dir() Failed (%d)", ret ) ;
      return(-1) ;

   }

   path_size = strlen( workpath ) + FILE_NAME_SIZE + 2; 
   path_str = (char *) calloc( (size_t) 1, (size_t) path_size );
   if( path_str == NULL ){

      LE_send_msg( GL_INFO, "malloc Failed For %d Bytes\n", path_size );
      return (-1);

   }

   /* Construct the path for the Snow Accumulation Total File. */
   strcat( path_str, workpath );
   path_str[ strlen(workpath) ] = '/';
   strcat( path_str, SAATOTAL );

   LE_send_msg( GL_INFO, "The Snow Accumulation Total File Path:  %s\n", path_str );
   
   /* Check if the file has any bytes in it. */
   if( (ret = stat( path_str, &stats )) == -1 )
      LE_send_msg( GL_INFO, "Unable to stat %s (%d)\n", path_str, errno ) ;

   else if( stats.st_size == 0 ){

      /* File is empty.... Delete the file. */
      ret = remove( path_str );
      if( ret < 0 )
         LE_send_msg( GL_INFO, "Unable to Delete Empty File %s\n", path_str );

      else
         LE_send_msg( GL_INFO, "Empty File %s Deleted\n", path_str );

   }
   else
      LE_send_msg( GL_INFO, "File %s Has %d Bytes\n", path_str, stats.st_size );

   /* Construct the path for the Snow Accumulation Hourly File. */
   memset( path_str, 0, path_size );
   strcat( path_str, workpath );
   path_str[ strlen(workpath) ] = '/';
   strcat( path_str, SAAHOURLY );

   LE_send_msg( GL_INFO, "The Snow Accumulation Hourly File Path:  %s\n", path_str );

   /* Check if the file has any bytes in it. */
   if( (ret = stat( path_str, &stats )) == -1 )
      LE_send_msg( GL_INFO, "Unable to stat %s (%d)\n", path_str, errno ) ;

   else if( stats.st_size == 0 ){

      /* File is empty.... Delete the file. */
      ret = remove( path_str );
      if( ret < 0 )
         LE_send_msg( GL_INFO, "Unable to Delete Empty File %s\n", path_str );

      else
         LE_send_msg( GL_INFO, "Empty File %s Deleted\n", path_str );

   }
   else
      LE_send_msg( GL_INFO, "File %s Has %d Bytes\n", path_str, stats.st_size );
   
   return (0);

}
