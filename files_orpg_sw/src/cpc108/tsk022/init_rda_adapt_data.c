/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:26 $
 * $Id: init_rda_adapt_data.c,v 1.2 2005/12/27 16:41:26 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */

#include <orpg.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

#define	STARTUP		1
#define	RESTART		2
#define	CLEAR		3

/*
 * Static Function Prototypes
 */
static int Write_rda_adapt_msg(void);
static int Get_command_line_options( int argc, char *argv[], int *startup_action );


/**************************************************************************
   Description:
      Main module for the RDA Adaptation Data message initialization.

   Input:
      argc - number of command line arguments
      argv - command line arguments

   Output:

   Returns:
      Non-zero exit code on error, zero exit code on success.

   Notes:

**************************************************************************/
int main(int argc, char *argv[])
{
   int startup_type;
   int ret;
   char *buf;


   /* Initialize log-error services. */
   (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

   /* Process command line arguments */
   if( (ret = Get_command_line_options(argc, argv, &startup_type )) < 0 )
      exit(1) ;

   /* First check to see if the message has been already initialized.  
      If it has, do nothing and return. */

   ret = ORPGDA_read (ORPGDAT_RDA_ADAPT_DATA, &buf,
                      LB_ALLOC_BUF, ORPGDAT_RDA_ADAPT_MSG_ID);
   if (ret >= 0 )
   { 
      /* msg exists, free buffer and do nothing */
      free (buf);
      return 0; 
   }

   if( startup_type == STARTUP )
   {
      LE_send_msg( GL_INFO,
         "Initialization of the RDA Adaptation Data Message.") ;

      ret = Write_rda_adapt_msg();
      if ( ret < 0 )
      {
         LE_send_msg(GL_ERROR, 
            "init_rda_adapt_data: error writing msg (%d)\n", ret);
      }
   }
     
   return 0;

} /* End main() */


/****************************************************************************
  
    Description:
       Write the adaptation data message.
  
    Inputs:
       none

    Outputs:
       Writes the RDA Adaptation Data msg to the ORPGDAT_RDA_ADAPT_DATA LB

    Returns:
       0 on success, negative otherwise
  
    Globals:
  
    Notes:
  
****************************************************************************/
static int Write_rda_adapt_msg(void)
{
   ORDA_adpt_data_msg_t *adapt_data;
   int ret;

   /* allocate and initialize the space */
   adapt_data = (ORDA_adpt_data_msg_t *)calloc( 1, sizeof(ORDA_adpt_data_msg_t)); 
   if ( adapt_data == NULL )
   {
      LE_send_msg( GL_INFO, 
         "Failed allocating space for RDA adapt data msg\n");
      return (-1);
   }

   /* set the necessary field values */
   adapt_data->rda_adapt.deltaprf = 3;
   adapt_data->rda_adapt.nbr_el_segments = 2;
   adapt_data->rda_adapt.seg1lim = 1.65;
   adapt_data->rda_adapt.seg2lim = 4.50;
   adapt_data->rda_adapt.seg3lim = 5.00;
   adapt_data->rda_adapt.seg4lim = 5.50;

   /* Write the message to the LB */
   ret = ORPGDA_write( ORPGDAT_RDA_ADAPT_DATA, (char *)adapt_data, 
      sizeof (ORDA_adpt_data_msg_t), ORPGDAT_RDA_ADAPT_MSG_ID);

   if( ret != (int) sizeof(ORDA_adpt_data_msg_t) )
   {
      LE_send_msg( GL_ERROR,
         "ORPGDA_write (ORPGDAT_RDA_ADAPT_MSG_ID) failed: %d\n", ret);
      return (-2);
   }
   else 
   {
      LE_send_msg(GL_INFO,
         "Data ID %d: wrote %d bytes to ORPGDAT_RDA_ADAPT_MSG_ID",
         ORPGDAT_RDA_ADAPT_DATA, ret) ;
   }

   return 0;

} /* End Write_rda_adapt_msg() */


/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, RESTART, or CLEAR)

   Returns:
      exits on error, or returns 0 on success.

****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], int *startup_action )
{
   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) {

      switch (c) 
      {
         case 't':
            if( strlen( optarg ) < 255 )
            {
               ret = sscanf(optarg, "%s", start_up);
               if (ret == EOF) 
               {
                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" );
                  err = 1;
               }
               else 
               {
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
            {
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

   if (err == 1)               /* Print usage message */
   {
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-l Startup Action (optional - default: RESTART)\n" );
      exit (1);
   }

   return (0);

} /* End of Get_command_line_options() */


