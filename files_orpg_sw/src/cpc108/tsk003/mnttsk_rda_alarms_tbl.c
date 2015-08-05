/*************************************************************************

   Module:  mnttsk_prod_gen_tbl.c

   Description:
      Maintenance Task: Product Generation Table

   Assumptions:
      The product attribute table (PAT) exists.

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:51 $
 * $Id: mnttsk_rda_alarms_tbl.c,v 1.4 2005/12/27 16:41:51 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */


#include <mnttsk_rda_alarms_tbl.h>

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Global Variables */
char RDA_alarms_table_name[NAME_SIZE];    /* file name for RDA alarms tables */ 
char ORDA_alarms_table_name[NAME_SIZE];   /* file name for ORDA alarms tables */ 

/* Static Global Variables */

/* Static Function Prototypes */
static int Get_command_line_options( int argc, char *argv[], int *startup_type );


/**************************************************************************
   Description:
      Main module for maintenance task which initializes the RDA alarms 
      table.

   Input:
      argc - number of command line arguments.
      argv - command line arguments.

   Output:

   Returns:

   Notes:
      On error, this process exits with non-zero exit code.

 **************************************************************************/
int main(int argc, char *argv[]){

    int startup_type ;
    int retval ;


    /* Initialize log-error services. */
    (void) ORPGMISC_init(argc, argv, 2000, 0, -1, 0) ;

    if( (retval = Get_command_line_options( argc, argv, &startup_type )) < 0 )
       exit(1) ;

    /* Initialize the RDA alarms table controlled by startup
       mode "startup_type". */

    if( (retval = MNTTSK_RDA_ALARMS_TBL_maint( startup_type )) < 0 ){
 
       LE_send_msg( GL_INFO ,
                    "Data IDs %d, %d: MNTTSK_RDA_ALARMS_maint() failed: %d",
                    ORPGDAT_RDA_ALARMS_TBL, retval) ;
       exit(2);

    }

    exit(0) ;

/*END of main()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, RESTART, or CLEAR)

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], 
                                     int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize the Product_tables_name. */
   strcpy (RDA_alarms_table_name, "rda_alarms_table");
   strcpy (ORDA_alarms_table_name, "orda_alarms_table");

   /* Initialize startup_action to RESTART. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) {

      switch (c) {

         case 't':
            if( strlen( optarg ) < 255 ){

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else
                     *startup_action = RESTART;

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

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Mode (optional - default: restart)\n" );
      printf ("\t\t      startup, restart, and clear supported\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
