/*************************************************************************

   Module:  mnttsk_dp_qpe.c

   Description:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/12 20:55:14 $
 * $Id: mnttsk_dp_qpe.c,v 1.3 2014/05/12 20:55:14 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <orpg.h>
#include <dp_lt_accum_Consts.h> /* HOURLY_ID */
#include <dpprep_isdp.h>

/* Constant Definitions/Macro Definitions/Type Definitions
 *
 * 12/2011 Ward CCR NA11-00373:
 *
 * Added 2 new states, CLEAR_ACCUM and CLEAR_DIFF. CLEAR will still
 * work as previously defined; it will do a reset of all of QPE. */

#define STARTUP         1
#define RESTART         2
#define CLEAR           3
#define CHECK           4
#define CLEAR_ACCUM     5
#define CLEAR_DIFF      6

#define LOG_STATUS_MSGS 0

extern int errno;

/* Static Function Prototypes */
static int Remove_database_file_msgs(void) ;
static int Remove_database_file_msgs_accum(void) ;
static int Remove_database_file_msgs_diff(void) ;
static int Get_command_line_args( int argc, char **argv, int *startup_action );


/**************************************************************************
    Description:
       Initialization routine for DP QPE Algorithm files.

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
    Dpp_isdp_est_t Isdp_est;

    /* Initialize log-error services. */
    ORPGMISC_init( argc, argv, 200, 0, -1, LOG_STATUS_MSGS );

    /* Get command line arguments.  On failure, exit. */
    if( (retval = Get_command_line_args( argc, argv, &startup_action ) != 0) )
       exit(retval);

    /* Get the current RPG state. */
    retval = ORPGMGR_get_RPG_states( &rpg_state );
    if( retval < 0 )
       rpg_state.state = MRPG_ST_SHUTDOWN;


    /* If startup_action indicates to check the files, do nothing.
       Currently there is no requirements for this option. */
    if( (startup_action == CHECK)
                  ||
        (startup_action == RESTART) ){

        /* Exit gracefully. */
        exit(0);

    }

    /* Initialize ISDP Estimate data (only if does not already exist). */
    if( startup_action == STARTUP ){

       /* Set write permission. */
       ORPGDA_write_permission( DP_ISDP_EST );

       if( (retval = ORPGDA_read( DP_ISDP_EST, (char *) &Isdp_est.isdp_est,
                                  sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID )) <= 0 ){

          LE_send_msg( GL_INFO, "Initializing ISDP Estimate\n" );

          Isdp_est.isdp_est = -99;
          Isdp_est.isdp_yy = 96;
          Isdp_est.isdp_mm = 1;
          Isdp_est.isdp_dd = 1;
          Isdp_est.isdp_hr = 12;
          Isdp_est.isdp_min = 0;
          Isdp_est.isdp_sec = 0;

          if( (retval = ORPGDA_write( DP_ISDP_EST, (char *) &Isdp_est.isdp_est,
                                      sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID )) <= 0 )
             LE_send_msg( GL_ERROR, "DP_ISDP_EST Write Failed (%d)\n", retval );

       }

       /* No more to do. */
       exit(0);

    }

    /* If startup_action is clear DP QPE, then do the following ... */
    if ( startup_action == CLEAR ){

        if( rpg_state.state == MRPG_ST_OPERATING ){

           retval = EN_post_event( ORPGEVT_RESTART_LT_ACCUM );
           if( retval < 0 )
               LE_send_msg( GL_ERROR,
                            "EN_post_event( ORPGEVT_RESTART_LT_ACCUM ) Failed (%d)\n",
                            retval );
           else
               LE_send_msg( GL_INFO, "ORPGEVT_RESTART_LT_ACCUM event posted\n" );

           exit(0);
        }
        else{

            LE_send_msg( GL_INFO,"CLEAR DP QPE DUTIES\n" ) ;
            LE_send_msg( GL_INFO,"\t1. Remove All Messages in DP_HRLY_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t2. Remove All Messages in DP_STORM_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t3. Remove All Messages in DP_HRLY_ACCUM.DAT" );
            LE_send_msg( GL_INFO,"\t4. Remove All Messages in DP_DIFF_ACCUM.DAT" );
            LE_send_msg( GL_INFO,"\t5. Remove All Messages in DP_OLD_RATE.DAT" );

            if (Remove_database_file_msgs() < 0) {

                LE_send_msg( GL_INFO, "Remove_database_file_msgs() failed\n!") ;
                exit(7) ;
            }
        }
    }
    else if ( startup_action == CLEAR_ACCUM ){

        if( rpg_state.state == MRPG_ST_OPERATING ){

           retval = EN_post_event( ORPGEVT_RESTART_LT_ACCUM );
           if( retval < 0 )
               LE_send_msg( GL_ERROR,
                            "EN_post_event( ORPGEVT_RESTART_LT_ACCUM ) Failed (%d)\n",
                            retval );
           else
               LE_send_msg( GL_INFO, "ORPGEVT_RESTART_LT_ACCUM event posted\n" );

           exit(0);
        }
        else{

            LE_send_msg( GL_INFO,"CLEAR DP QPE ACCUM DUTIES\n" ) ;
            LE_send_msg( GL_INFO,"\t1. Remove HOURLY_ID Messages in DP_HRLY_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t2. Remove STORM_ID  Messages in DP_STORM_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t3. Remove All       Messages in DP_HRLY_ACCUM.DAT" );
            LE_send_msg( GL_INFO,"\t4. Remove All       Messages in DP_OLD_RATE.DAT" );

            if (Remove_database_file_msgs_accum() < 0) {

                LE_send_msg( GL_INFO, "Remove_database_file_msgs_accum() failed\n!") ;
                exit(7) ;
            }
        }
    }
    else if ( startup_action == CLEAR_DIFF ){

        if( rpg_state.state == MRPG_ST_OPERATING ){

           retval = EN_post_event( ORPGEVT_RESTART_LT_DIFF );
           if( retval < 0 )
               LE_send_msg( GL_ERROR,
                            "EN_post_event( ORPGEVT_RESTART_LT_DIFF ) Failed (%d)\n",
                            retval );
           else
               LE_send_msg( GL_INFO, "ORPGEVT_RESTART_LT_DIFF event posted\n" );

           exit(0);
        }
        else{

            LE_send_msg( GL_INFO,"CLEAR DP QPE DIFF DUTIES\n" ) ;
            LE_send_msg( GL_INFO,"\t1. Remove HOURLY_DIFF_ID Messages in DP_HRLY_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t2. Remove STORM_DIFF_ID  Messages in DP_STORM_BACKUP.DAT" );
            LE_send_msg( GL_INFO,"\t3. Remove All            Messages in DP_DIFF_ACCUM.DAT" );

            if (Remove_database_file_msgs_diff() < 0) {

                LE_send_msg( GL_INFO, "Remove_database_file_msgs_diff() failed\n!") ;
                exit(7) ;
            }
        }
    }

    /* Normal termination. */
    exit(0) ;

/*END of main()*/
}

/**************************************************************************
    Description:
       Removes DP QPE file messages.

    Output:

    Returns:
       On failure of any sort, returns -1, otherwise 0.

    Notes:
 **************************************************************************/
static int Remove_database_file_msgs(void){

    int db_file_ids[] = { DP_HRLY_BACKUP, DP_STORM_BACKUP,
                          DP_HRLY_ACCUM, DP_DIFF_ACCUM,
                          DP_OLD_RATE };
    int i;
    int retval;
    LB_status status;

    /* For All Data Base files ..... */
    for( i = 0; i <= 4; ++i ){

        /* Open the LB with write permission. This is needed for
           ORPGDA_clear(). */
        ORPGDA_write_permission( db_file_ids[i] );

        /* Find the number of messages in LB. */
        status.attr = NULL;
        status.n_check = 0;
        retval = ORPGDA_stat( db_file_ids[i], &status );
        if( retval == LB_SUCCESS ){

           /* Remove the file messages. */

           retval = ORPGDA_clear( db_file_ids[i], LB_ALL );

           if( retval == status.n_msgs)
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Successful (%d)\n",
	 	           db_file_ids[i], retval );
           else
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Failed (%d)\n",
	                   db_file_ids[i], retval );
         }
         else
            LE_send_msg( GL_INFO, "ORPGDA_stat(%d) Failed (%d)\n",
	                 db_file_ids[i], retval );
    }

    return(0) ;

} /* end Remove_database_file_msgs()*/

/**************************************************************************
    Description:
       Removes DP QPE file accum messages.

    Output:

    Returns:
       On failure of any sort, returns -1, otherwise 0.

    Notes: 12/2011 Ward Added for CCR NA11-00373.
 **************************************************************************/
static int Remove_database_file_msgs_accum(void){

    int db_file_ids[] = { DP_HRLY_BACKUP, DP_STORM_BACKUP,
                          DP_HRLY_ACCUM,  DP_OLD_RATE };
    int i;
    int retval;
    LB_status status;

    /* For All Data Base files ..... */
    for( i = 0; i <= 3; ++i ){

        /* Open the LB with write permission. This is needed for
           ORPGDA_clear(). */
        ORPGDA_write_permission( db_file_ids[i] );

        /* Find the number of messages in LB. */
        status.attr = NULL;
        status.n_check = 0;
        retval = ORPGDA_stat( db_file_ids[i], &status );
        if( retval == LB_SUCCESS ){

           /* Remove the file messages. */

           if(i==0) /* DP_HRLY_BACKUP */
              retval = ORPGDA_clear( db_file_ids[i], HOURLY_ID );
           else if(i==1) /* DP_STORM_BACKUP, */
              retval = ORPGDA_clear( db_file_ids[i], STORM_ID );
           else /* DP_HRLY_ACCUM or DP_OLD_RATE */
              retval = ORPGDA_clear( db_file_ids[i], LB_ALL );

           if( retval == status.n_msgs)
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Successful (%d)\n",
	 	           db_file_ids[i], retval );
           else
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Failed (%d)\n",
	                   db_file_ids[i], retval );
         }
         else
            LE_send_msg( GL_INFO, "ORPGDA_stat(%d) Failed (%d)\n",
	                 db_file_ids[i], retval );
    }

    return(0) ;

} /* end Remove_database_file_msgs_accum()*/

/**************************************************************************
    Description:
       Removes DP QPE file messages.

    Output:

    Returns:
       On failure of any sort, returns -1, otherwise 0.

    Notes: 12/2011 Ward Added for CCR NA11-00373.
 **************************************************************************/
static int Remove_database_file_msgs_diff(void){

    int db_file_ids[] = { DP_HRLY_BACKUP, DP_STORM_BACKUP,
                          DP_DIFF_ACCUM };
    int i;
    int retval;
    LB_status status;

    /* For All Data Base files ..... */
    for( i = 0; i <= 2; ++i ){

        /* Open the LB with write permission. This is needed for
           ORPGDA_clear(). */
        ORPGDA_write_permission( db_file_ids[i] );

        /* Find the number of messages in LB. */
        status.attr = NULL;
        status.n_check = 0;
        retval = ORPGDA_stat( db_file_ids[i], &status );
        if( retval == LB_SUCCESS ){

           /* Remove the file messages. */

           if(i==0) /* DP_HRLY_BACKUP */
              retval = ORPGDA_clear( db_file_ids[i], HOURLY_DIFF_ID );
           else if(i==1) /* DP_STORM_BACKUP, */
              retval = ORPGDA_clear( db_file_ids[i], STORM_DIFF_ID );
           else /* DP_DIFF_ACCUM */
              retval = ORPGDA_clear( db_file_ids[i], LB_ALL );

           if(retval == status.n_msgs)
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Successful (%d)\n",
	 	           db_file_ids[i], retval );
           else
              LE_send_msg( GL_INFO, "ORPGDA_clear(%d) Failed (%d)\n",
	                   db_file_ids[i], retval );

         }
         else
            LE_send_msg( GL_INFO, "ORPGDA_stat(%d) Failed (%d)\n",
	                 db_file_ids[i], retval );

    }

    return(0) ;

} /* end Remove_database_file_msgs_diff()*/

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (startup, clear, check,
                       or restart)

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int Argc, char **Argv, int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART. */
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

                  else if( strstr( start_up, "clear_accum" ) != NULL )
                     *startup_action = CLEAR_ACCUM;

                  else if( strstr( start_up, "clear_diff" ) != NULL )
                     *startup_action = CLEAR_DIFF;

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
