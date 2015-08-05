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
 * $Date: 2012/09/26 20:56:42 $
 * $Id: mnttsk_prod_gen_tbl.c,v 1.8 2012/09/26 20:56:42 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */
#include <orpg.h>


#include <mnttsk_pgt_def.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Global Variables
 */
char Product_gen_tables_name[NAME_SIZE];    /* file name for prod gen tables */ 
char Product_gen_tables_basename[NAME_SIZE]; /* file name for prod gen tables 
                                                extensions*/ 
char Cfg_extensions_name[CFG_NAME_SIZE];    /* "extensions directory name. */

int Rda_config;

/*
 * Static Global Variables
 */
/*
 * Static Function Prototypes
 */
static int Get_command_line_options( int argc, char *argv[], int *startup_type,
                                     int *init_tables );


/**************************************************************************
   Description:
      Main module for maintenance task which initializes the product 
      generation table(s), PGT(s).

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
    int init_tables ;
    int retval ;


    /* Initialize log-error services. */
    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

    if( (retval = Get_command_line_options(argc, argv, &startup_type,
                                           &init_tables )) < 0 )
       exit(1) ;

    /* Tell the operator which RDA configuration we believe we are. */
    if( Rda_config == ORPGRDA_LEGACY_CONFIG )
       LE_send_msg( GL_INFO, "The RDA Configuration is Legacy\n" );

    else
       LE_send_msg( GL_INFO, "The RDA Configuration is ORDA\n" );

    /* Initialize the product generation table(s) controlled by startup
       mode "startup_type" and dictated by "init_tables". */

    if( (retval = MNTTSK_PGT_INFO_maint(startup_type, init_tables)) < 0 ){
 
       LE_send_msg( GL_INFO ,
                   "Data IDs %d, %d: MNTTSK_PGT_INFO_maint() failed: %d",
                   ORPGDAT_PROD_INFO, retval) ;
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
static int Get_command_line_options( int argc, char *argv[], int *startup_action,
                                     int *init_tables ){

   extern char *optarg;
   extern int optind;
   int c, err, ret, len;
   char start_up[255], cfg_dir[CFG_NAME_SIZE], tmpbuf[CFG_NAME_SIZE];

   /* Initialize the Product_gen_tables_name. */
   strcpy (Product_gen_tables_name, "product_generation_tables");
   strcpy (Product_gen_tables_basename, "product_generation_tables");
   strcpy (Cfg_extensions_name, "extensions");

   /* Check to see if the configuration source (CFG_SRC) environmental
       variable is defined.  If so, make this the directory path to the
       various ASCII configuration files. */
    len = MISC_get_cfg_dir (cfg_dir, CFG_NAME_SIZE);
    if (len > 0)
       strcat (cfg_dir, "/");

    /* Process the configuration extensions. */
    strcpy (tmpbuf, cfg_dir);
    strcat (tmpbuf, Cfg_extensions_name);
    strcpy (Cfg_extensions_name, tmpbuf);

   /* Initialize startup_action to RESTART, init_tables to NONE,
      and default RDA configuration. */
   *startup_action = RESTART;
   *init_tables = 0;
   Rda_config = ORPGRDA_get_rda_config( NULL );
   if( Rda_config == ORPGRDA_ERROR )
      Rda_config = ORPGRDA_LEGACY_CONFIG;

   err = 0;
   while ((c = getopt (argc, argv, "ht:CABar:")) != EOF) {

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

         case 'A':
            *init_tables |= DEFAULT_A;
            break;
         case 'B':
            *init_tables |= DEFAULT_B;
            break;
         case 'C':
            *init_tables |= CURRENT;
            break;
         case 'a':
            *init_tables |= ALL_TBLS;
            break;
         case 'r':
            Rda_config = atoi( optarg );
            if( (Rda_config != ORPGRDA_LEGACY_CONFIG)
                            &&
                (Rda_config != ORPGRDA_ORDA_CONFIG)){

               LE_send_msg( GL_INFO, "RDA Configuration Error (%d)\n",
                            Rda_config );
               err = 1;

            }
            break; 
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if( *startup_action != CLEAR ){

      if( *init_tables != 0 ){

         LE_send_msg( GL_INFO, "Table Initialization Only Value with \"clear\" \n" );
         *init_tables = 0;

      }

   }

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Mode (optional - default: restart)\n" );
      printf ("\t\t      startup, restart, and clear supported\n" );
      printf ("\t\t-C Initialize CURRENT Product Generation Table\n" );
      printf ("\t\t-A Initialize Default Weather Mode A Product Generation Table\n" );
      printf ("\t\t-B Initialize Default Weather Mode B Product Generation Table\n" );
      printf ("\t\t-a Initialize ALL Product Generation Tables\n" );
      printf ("\t\t-r Default RDA Configuration (0 - Legacy, 1 - ORDA)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}
