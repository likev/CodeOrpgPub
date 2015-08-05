/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/19 18:57:33 $
 * $Id: mnttsk_clutter.c,v 1.21 2012/11/19 18:57:33 ccalvert Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */


#include "mnttsk_clutter.h"

#include <clutter.h>
#include <basedata.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define MAX_SEGMENT_SIZE       1208
#define DEFAULT                0
#define BASELINE               1


/*
 * Static Global Variables
 */


/*
 * Static Function Prototypes
 */
static void Build_bogus_header_lgcy( RDA_RPG_message_header_t *msg_hdr,
   unsigned short message_type );
static void Build_bogus_header_orda( RDA_RPG_message_header_t *msg_hdr,
   unsigned short message_type );
static void Init_censor_zones_default_lgcy( RPG_clutter_regions_t *regions );
static void Init_censor_zones_default_orda( ORPG_clutter_regions_t *regions );
static void Init_lbid_bypassmap_lgcy(void);
static void Init_lbid_bypassmap_orda(void);
static void Init_censor_zones_lgcy( int startup_action );
static void Init_censor_zones_orda( int startup_action );
static int Validate_censor_zones_orda( int startup_action, int copy );
static void Init_lbid_cluttermap_lgcy(void);
static void Init_lbid_cluttermap_orda(void);
static int Get_command_line_options( int argc, char *argv[], int *startup_type );


/**************************************************************************
   Description:
      Driver module for clutter files
          1) Bypass Map
          2) Notchwidth Map
          3) Clutter Sensor Zones

   Input:
      argc - number of command line arguments
      argv - command line arguments

   Output:

   Returns:
      Exits with non-zero exit code on error, 0 exit code on success.

   Notes:

**************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;

    (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

    retval = Get_command_line_options(argc, argv, &startup_action) ;
    if (retval < 0) 
        exit(1) ;

    /*  _CLUTTERMAP */
    retval = MNTTSK_init_clutter(startup_action) ;
    if (retval < 0) {

        LE_send_msg(GL_INFO, "MNTTSK_init_clutter(%d) failed: %d",
                    startup_action, retval) ;
        exit(3) ;

    }

    exit(0) ;

/*END of main()*/
}


/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, RESTART or CLEAR)

   Returns:
      Exits on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
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
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}

/*************************************************************************

   Description:
      Driver module for initialization of clutter censor zones, bypass
      map, and clutter map data.

   Inputs:
      startup_action - startup mode/action.  Either STARTUP, RESTART, or
                       CLEAR.

   Outputs:

   Returns:
      Returns 0 on success or negative number on error.

   Notes:

**************************************************************************/
int MNTTSK_init_clutter(int startup_action)
{
   if (startup_action == STARTUP)
   {
      LE_send_msg(GL_INFO,
         "Init_clutter Maint. INIT FOR STARTUP DUTIES:");

      LE_send_msg(GL_INFO, "\t1. Initialize the Clutter Filter Map data.");
      LE_send_msg(GL_INFO, "\t2. Initialize the Bypass Map data.");
      LE_send_msg(GL_INFO, "\t3. Initialize the Edited Bypass Map data.");
      LE_send_msg(GL_INFO, "\t4. Initialize the Clutter Censor Zones data.");

      Init_lbid_cluttermap_lgcy();
      Init_lbid_cluttermap_orda();
      Init_lbid_bypassmap_lgcy();
      Init_lbid_bypassmap_orda();
      Init_censor_zones_lgcy( startup_action );
      Init_censor_zones_orda( startup_action );
   }

   else if (startup_action == CLEAR)
   {

      LE_send_msg(GL_INFO,
         "Init_clutter Maint. INIT FOR CLEAR DUTIES:");

      LE_send_msg(GL_INFO, "\t1. Clear the Clutter Censor Zones.");
      LE_send_msg(GL_INFO, "\t2. Initialize the Clutter Censor Zones data.");
      Init_censor_zones_lgcy( startup_action );
      Init_censor_zones_orda( startup_action );

   }
   else if (startup_action == RESTART)
   {

      LE_send_msg(GL_INFO,
         "Init_clutter Maint. INIT FOR RESTART DUTIES:");

      LE_send_msg(GL_INFO, "\t1. Initialize the Clutter Censor Zones data.");
      Init_censor_zones_lgcy( startup_action );
      Init_censor_zones_orda( startup_action );

   }

   return(0);

} /* end of MNTTSK_init_clutter() */


/*\/////////////////////////////////////////////////////////////
//
//   Description:
//      a)  Allocates storage for Legacy RDA Clutter Filter Map. 
//          (Allocation function is calloc so all storage is
//          initialized to 0.)
//
//      b)  Calls function to build bogus RDA/RPG message header.
//
//      c)  Writes data to Linear Buffer ORPGDAT_CLUTTERMAP.
//
/////////////////////////////////////////////////////////////\*/
static void Init_lbid_cluttermap_lgcy(void)
{
   RDA_notch_map_msg_t      *cluttermap_msg;
   int                       ret;
   char			    *buf;

/*   First check to see if the message has been already initialized.	*
 *   If it has, do nothing and return.					*/

   ret = ORPGDA_read (ORPGDAT_CLUTTERMAP, &buf,
		      LB_ALLOC_BUF, LBID_CLUTTERMAP_LGCY);

   if (ret == sizeof (RDA_notch_map_msg_t))
   {  
        /* msg exists, free buffer and do nothing */
	free (buf);
	return;
   }

   /*
     Allocate and zero space for one clutter map with 
     message header.
   */
   cluttermap_msg = (RDA_notch_map_msg_t *) calloc( (int) 1,
                             sizeof( RDA_notch_map_msg_t ) );

   if( cluttermap_msg != NULL )
   {
      /*
        Build bogus message header.
      */

      Build_bogus_header_lgcy( &cluttermap_msg->msg_hdr, 
         (unsigned short) NOTCHWIDTH_MAP_DATA ); 
   


      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) cluttermap_msg, 
                          sizeof(RDA_notch_map_msg_t), LBID_CLUTTERMAP_LGCY);

      if( ret != (int) sizeof(RDA_notch_map_msg_t))
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_NOTCHMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else 
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_CLUTTERMAP_LGCY);
      }

      free( cluttermap_msg );

   }
   else
      fprintf( stderr, "calloc Failed For Clutter Filter Map.\n" );

   return;

} /* end of Init_lbid_cluttermap_lgcy() */ 


/*\/////////////////////////////////////////////////////////////
//
//   Description:
//      a)  Allocates storage for Open RDA Clutter Filter Map. 
//          (Allocation function is calloc so all storage is
//          initialized to 0.)
//
//      b)  Calls function to build bogus RDA/RPG message header.
//
//      c)  Writes data to Linear Buffer ORPGDAT_CLUTTERMAP.
//
/////////////////////////////////////////////////////////////\*/
static void Init_lbid_cluttermap_orda(void)
{
   ORDA_clutter_map_msg_t	*cluttermap_msg;
   int				ret;
   char				*buf;


/*   First check to see if the message has been already initialized.	*
 *   If it has, do nothing and return.					*/

   ret = ORPGDA_read (ORPGDAT_CLUTTERMAP, &buf,
		      LB_ALLOC_BUF, LBID_CLUTTERMAP_ORDA);

   if (ret == sizeof (ORDA_clutter_map_msg_t))
   {  
        /* msg exists, free buffer and do nothing */
	free (buf);
	return;
   }

   /*
     Allocate and zero space for one clutter map with 
     message header.
   */
   cluttermap_msg = (ORDA_clutter_map_msg_t *) calloc( (int) 1,
                             sizeof( ORDA_clutter_map_msg_t ) );

   if( cluttermap_msg != NULL )
   {
      /*
        Build bogus message header.
      */
      Build_bogus_header_orda( &cluttermap_msg->msg_hdr, 
                          (unsigned short) CLUTTER_MAP_DATA ); 
   
      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) cluttermap_msg, 
                          sizeof(ORDA_clutter_map_msg_t), LBID_CLUTTERMAP_ORDA);
      if( ret != (int) sizeof(ORDA_clutter_map_msg_t))
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_CLUTTERMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else 
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_CLUTTERMAP_ORDA);
      }

      free( cluttermap_msg );

   }
   else
      fprintf( stderr, "calloc Failed For Clutter Filter Map.\n" );

   return;

} /* end of Init_lbid_cluttermap_orda() */ 


/*\/////////////////////////////////////////////////////////////
//
//   Description:
//      a)  Allocates storage for Lgcy Clutter Filter Bypass Map. 
//          (Allocation function is calloc so all storage is
//          initialized to 0.)
//
//      b)  Calls function to build bogus RDA/RPG message header.
//
//      c)  Writes data to Linear Buffer ORPGDAT_CLUTTERMAP.
//
/////////////////////////////////////////////////////////////\*/
static void Init_lbid_bypassmap_lgcy(void)
{
   RDA_bypass_map_msg_t     *bypassmap_msg;
   int                       ret;
   char			    *buf;

/*   First check to see if the message has already been initialized.	*
 *   If it has, do nothing and return.					*/

   ret = ORPGDA_read (ORPGDAT_CLUTTERMAP, &buf,
		      LB_ALLOC_BUF, LBID_BYPASSMAP_LGCY);

   if (ret == sizeof (RDA_bypass_map_msg_t))
   {
	free (buf);
	return;
   }

   /*
     Allocate and zero space for one bypass map with 
     message header.
   */
   bypassmap_msg = (RDA_bypass_map_msg_t *) calloc( (int) 1,
                            sizeof( RDA_bypass_map_msg_t ) );

   if( bypassmap_msg != NULL )
   {
      /*
        Build bogus message header.
      */
      Build_bogus_header_lgcy( &bypassmap_msg->msg_hdr, 
         (unsigned short) CLUTTER_FILTER_BYPASS_MAP ); 
   
      /*
	Define 2 segments.
      */

      bypassmap_msg->bypass_map.num_segs = 2;

      /*
	Define the segment numbers for each segment
      */

      bypassmap_msg->bypass_map.segment[0].seg_num = 1;
      bypassmap_msg->bypass_map.segment[1].seg_num = 2;



      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) bypassmap_msg, 
                          sizeof(RDA_bypass_map_msg_t), LBID_BYPASSMAP_LGCY );

      if( ret != (int) sizeof(RDA_bypass_map_msg_t))
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_BYPASSMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else 
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_BYPASSMAP_LGCY);
      }


      /*
        Write the edited bypass map with message header so we can detemine
	if it is based on data sent from the RDA (date not 0) or not.  
      */

      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) bypassmap_msg, 
                          sizeof(RDA_bypass_map_msg_t), LBID_EDBYPASSMAP_LGCY );

      if( ret != (int) sizeof(RDA_bypass_map_msg_t)) 
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_EDITED_BYPASSMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else 
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_EDBYPASSMAP_LGCY);
      }

      free( bypassmap_msg );
   }
   else
      fprintf( stderr, "calloc Failed For Bypass Map.\n" );

   return;

} /* end of Init_lbid_bypassmap_lgcy() */
 

/*\/////////////////////////////////////////////////////////////
//
//   Description:
//      a)  Allocates storage for ORDA Clutter Filter Bypass Map. 
//          (Allocation function is calloc so all storage is
//          initialized to 0.)
//
//      b)  Calls function to build bogus RDA/RPG message header.
//
//      c)  Writes data to Linear Buffer ORPGDAT_CLUTTERMAP.
//
/////////////////////////////////////////////////////////////\*/
static void Init_lbid_bypassmap_orda(void)
{
   ORDA_bypass_map_msg_t	*bypassmap_msg;
   int				ret;
   char				*buf;

/*   First check to see if the message has already been initialized.	*
 *   If it has, do nothing and return.					*/

   ret = ORPGDA_read (ORPGDAT_CLUTTERMAP, &buf,
		      LB_ALLOC_BUF, LBID_BYPASSMAP_ORDA);

   if (ret == sizeof (ORDA_bypass_map_msg_t))
   {
	free (buf);
	return;
   }

   /*
     Allocate and zero space for one bypass map with 
     message header.
   */
   bypassmap_msg = (ORDA_bypass_map_msg_t *) calloc( (int) 1,
                            sizeof( ORDA_bypass_map_msg_t ) );

   if( bypassmap_msg != NULL )
   {
      /*
        Build bogus message header.
      */
      Build_bogus_header_orda( &bypassmap_msg->msg_hdr, 
         (unsigned short) CLUTTER_FILTER_BYPASS_MAP ); 
   
      /*
	Define 5 segments.
      */

      bypassmap_msg->bypass_map.num_segs = 5;

      /*
	Define the segment numbers for each segment
      */

      bypassmap_msg->bypass_map.segment[0].seg_num = 1;
      bypassmap_msg->bypass_map.segment[1].seg_num = 2;
      bypassmap_msg->bypass_map.segment[2].seg_num = 3;
      bypassmap_msg->bypass_map.segment[3].seg_num = 4;
      bypassmap_msg->bypass_map.segment[4].seg_num = 5;

      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) bypassmap_msg, 
                          sizeof(ORDA_bypass_map_msg_t), LBID_BYPASSMAP_ORDA );
      if( ret != (int) sizeof(ORDA_bypass_map_msg_t))
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_BYPASSMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_BYPASSMAP_ORDA);
      }

      /*
        Write the edited bypass map with message header so we can detemine
	if it is based on data sent from the RDA (date not 0) or not.  
      */

      ret = ORPGDA_write( ORPGDAT_CLUTTERMAP, (char *) bypassmap_msg, 
                          sizeof(ORDA_bypass_map_msg_t), LBID_EDBYPASSMAP_ORDA );
      if( ret != (int) sizeof(ORDA_bypass_map_msg_t))
      {
         LE_send_msg( GL_ERROR,
                      "Data ID %d: ORPGDA_write(_EDITED_BYPASSMAP) failed: %d",
                      ORPGDAT_CLUTTERMAP, ret );
      }
      else
      {
         LE_send_msg( GL_INFO,
                      "Data ID %d: wrote %d bytes to msgid %d",
                      ORPGDAT_CLUTTERMAP, ret, LBID_EDBYPASSMAP_ORDA);
      }

      free( bypassmap_msg );
   }
   else
      fprintf( stderr, "calloc Failed For Bypass Map.\n" );

   return;

} /* end of Init_lbid_bypassmap_orda() */
 

/*\/////////////////////////////////////////////////////////
//
//   Description:
//      Initializes the Clutter Censor Zone file data.
//
//      There will be one file initialized:
//         File "Default" 
//
//      There will be one file initialized:
//	   File "Default"
//
//         last download file - 0
//         last download time - 0
//
//         The last modification time will be
//         0 and the file label "Default".
//
//      The censor zone data is initialized by calling 
//      special initialization routines. 
//
/////////////////////////////////////////////////////////\*/
static void Init_censor_zones_lgcy( startup_action )
{
   RPG_clutter_regions_msg_t  *clutter_regions_msg;
   RPG_clutter_regions_file_t *file_data;
   file_tag_t                 *file_tag;
   int                         ret;
   char			      *buf;

/*   First check to see if the message has been already initialized.	*
 *   If it has and startup action is STARTUP, do nothing and return.	*/

   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_LEGACY_ZONES, &buf, ORPGCCZ_DEFAULT );

   if (((startup_action == STARTUP) || (startup_action == RESTART))
                                    && 
                  (ret == sizeof(RPG_clutter_regions_msg_t)))
   {
	free (buf);
	return;
   }

   /*
     Allocate storage for the clutter censor zone data.
   */
   clutter_regions_msg = (RPG_clutter_regions_msg_t *)
                          calloc( (int) 1, sizeof(RPG_clutter_regions_msg_t) );

   if( clutter_regions_msg != NULL )
   {
      /*
        For File "Default".
      */
           file_data = &clutter_regions_msg->file[0];
           file_tag  = &file_data->file_id;

           /*
             For the file tag, set the time of last modification
             (Julian Seconds) to 0.  Set file label.
           */
           file_tag->time = 0;
           strcpy( file_tag->label, "Default" ); 

           /*
             Initialize the Clutter Censor Zones.
           */
           Init_censor_zones_default_lgcy( &file_data->regions );

      /*
        Initialize the last download time for all files and
        last download file to 0 and 0, respectively.
      */
      clutter_regions_msg->last_dwnld_time = 0;
      clutter_regions_msg->last_dwnld_file = 0;

      ret = ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, (char *) clutter_regions_msg, 
                                      sizeof(RPG_clutter_regions_msg_t), ORPGCCZ_DEFAULT );
      if( ret <= 0 )
      {
         LE_send_msg( GL_ERROR,
              "ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, ORPGCCZ_DEFAULT ) failed: %d", ret );
      }
      else
      {
         LE_send_msg( GL_INFO,
                "ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, ORPGCCZ_DEFAULT ) wrote %d encoded bytes", ret);
      }

      ret = ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, (char *) clutter_regions_msg, 
                                      sizeof(RPG_clutter_regions_msg_t), BASELINE );
      if( ret <= 0 )
      {
         LE_send_msg( GL_ERROR,
                 "ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, ORPGCCZ_BASELINE ) failed: %d", ret);
      }
      else
      {
         LE_send_msg( GL_INFO,
                "ORPGCCZ_set_censor_zones( ORPGCCZ_set_censor_zones, ORPGCCZ_BASELINE ) wrote %d encoded bytes", ret );
      }
      
      free( clutter_regions_msg );
   }
   else
      fprintf( stderr, "calloc Failed For Clutter Censor Zones.\n" );

   return;

} /* end of Init_censor_zones_lgcy() */


/*\/////////////////////////////////////////////////////////
//
//   Description:
//      Initializes the ORDA Clutter Censor Zone file data.
//
//      There will be one file initialized:
//         File "Default" 
//
//      There will be one file initialized:
//	   File "Default"
//
//         last download file - 0
//         last download time - 0
//
//         The last modification time will be
//         0 and the file label "Default".
//
//      The censor zone data is initialized by calling 
//      special initialization routines. 
//
/////////////////////////////////////////////////////////\*/
static void Init_censor_zones_orda( int startup_action )
{
   ORPG_clutter_regions_msg_t	*clutter_regions_msg;
   ORPG_clutter_regions_file_t	*file_data;
   file_tag_t			*file_tag;
   int				ret;


   /*  First check to see if the message has been already 
       initialized.  If it has, validate the message. */
   if( (Validate_censor_zones_orda( startup_action, ORPGCCZ_DEFAULT ) >= 0)
                                   && 
       (Validate_censor_zones_orda( startup_action, ORPGCCZ_BASELINE ) >= 0) )
      return;

   /* Allocate storage for the clutter censor zone data. */
   clutter_regions_msg = (ORPG_clutter_regions_msg_t *)
      calloc( (int) 1, sizeof(ORPG_clutter_regions_msg_t) );

   if( clutter_regions_msg != NULL ){

      /* For File "Default". */
      file_data = &clutter_regions_msg->file[0];
      file_tag  = &file_data->file_id;

      /* For the file tag, set the time of last modification
         (Julian Seconds) to 0.  Set file label. */
      file_tag->time = 0;
      strcpy( file_tag->label, "Default" ); 

      /* Initialize the Open RDA Clutter Censor Zones. */
      Init_censor_zones_default_orda( &file_data->regions );

      /* Initialize the last download time for all files and
         last download file to 0 and 0, respectively. */
      clutter_regions_msg->last_dwnld_time = 0;
      clutter_regions_msg->last_dwnld_file = 0;

      ret = ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, (char *) clutter_regions_msg, 
                                      sizeof(ORPG_clutter_regions_msg_t), ORPGCCZ_DEFAULT );
      if( ret <= 0 )
         LE_send_msg( GL_ERROR,
                 "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, ORPGCCZ_DEFAULT ) failed: %d", ret );

      else
         LE_send_msg( GL_INFO,
                "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, ORPGCCZ_DEFAULT ) wrote %d encoded bytes", ret);
       
      ret = ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, (char *) clutter_regions_msg, 
                                      sizeof(ORPG_clutter_regions_msg_t), ORPGCCZ_BASELINE );
      if( ret <= 0 )
         LE_send_msg( GL_ERROR,
                 "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, ORPGCCZ_BASELINE ) failed: %d", ret );
       
      else
         LE_send_msg( GL_INFO,
                "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, ORPGCCZ_BASELINE ) wrote %d encoded bytes", ret);
      free( clutter_regions_msg );

   }
   else
      fprintf( stderr, "calloc Failed For Clutter Censor Zones.\n" );

   return;

} /* end of Init_censor_zones_orda() */

/*\/////////////////////////////////////////////////////////
//
//   Description:
//      Validate the DEFAULT and BASELINE Clutter Censor   
//      Zone data.
//
//   Inputs:
//      startup_action - maintenance task start up directive
//      copy - adaptation data copy (ORPGCCZ_DEFAULT or
//             ORPGCCZ_BASELINE );
//   Returns:
//      -1 if censor zone data needs to be initialized.
//      0 otherwise.
//
/////////////////////////////////////////////////////////\*/
static int Validate_censor_zones_orda( int startup_action, int copy ){

   int ret;
   char	*buf;

   /* Validate adaptation data copy. */
   if( (copy != ORPGCCZ_BASELINE) && (copy != ORPGCCZ_DEFAULT) )
      return -1;

   /* Read the censor zone data. */
   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES, &buf, copy );

   if (((startup_action == STARTUP) || (startup_action == RESTART))
                                   && 
              (ret == sizeof(ORPG_clutter_regions_msg_t))){

      ORPG_clutter_regions_msg_t *msg = (ORPG_clutter_regions_msg_t *) buf;
      ORPG_clutter_regions_file_t *file = NULL;
      ORPG_clutter_regions_t *regions = NULL;

      int i, j, k, changed = 0;
 
      /* Validate the clutter regions ... discard bogus segments. */
      for( j = 0; j < MAX_CLTR_FILES; j++ ){

         file = &msg->file[j];

         /* Skip undefined files. */
         if( (file->file_id.label == NULL) || (strlen(file->file_id.label) == 0) )
            continue;

         regions = &file->regions;

         /* Validate each region defined in the file. */
         i = 0;
         while( i < regions->regions ){

            int start_az, stop_az, start_rng, stop_rng, segment, osc;

            start_az = regions->data[i].start_azimuth;
            stop_az = regions->data[i].stop_azimuth;
            start_rng = regions->data[i].start_range;
            stop_rng = regions->data[i].stop_range;
            segment = regions->data[i].segment;
            osc = regions->data[i].select_code;

            /* All attributes should be within ICD limits. */
            if( (start_az < 0) || (start_az > 360)
                                ||
                (stop_az < 0) || (stop_az > 360)
                               ||
                (start_rng < 0) || (start_rng > 511)
                               ||
                (stop_rng < 0) || (stop_rng > 511)
                               ||
                (segment < 1) || (segment > 5)
                               ||
                (osc < 0) || (osc > 2) ){

               /* Bad region encountered.   Remove it. */
               LE_send_msg( GL_INFO, "Bad Region Found ... Removing Region %d\n", i );
               k = i;
               if( k < (regions->regions - 1) ){

                  regions->data[k].start_azimuth = regions->data[k+1].start_azimuth;
                  regions->data[k].stop_azimuth = regions->data[k+1].stop_azimuth;
                  regions->data[k].start_range = regions->data[k+1].start_range;
                  regions->data[k].stop_range = regions->data[k+1].stop_range;
                  regions->data[k].segment = regions->data[k+1].segment;
                  regions->data[k].select_code = regions->data[k+1].select_code;

                  k++;

                  changed = 1;

               }

               regions->regions--;

            }
            else
               i++;

         }

         /* Validate whether there is at least one valid region. */
         if( regions->regions <= 0 ){

            changed = 1;
            file->file_id.label[0] = '\0';

         }

      }

      /* If changed, Write the censor zone data back to LB. */
      if( changed ){

         ret = ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, (char *) buf, 
                                            sizeof(ORPG_clutter_regions_msg_t), copy );
         if( ret <= 0 )  
            LE_send_msg( GL_ERROR,
              "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, %d ) failed: %d", copy, ret );

         else
            LE_send_msg( GL_INFO,
             "ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, %d ) wrote %d encoded bytes", copy, ret);

      }

      free (buf);
      return 0;

   }
   else
      return -1;

}

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//     Initialization routine for clutter censor zone data for file
//     "Default" for Legacy RDA.  The censor zone defaults are:
//
//         Bypass Map in control in the region bounded in
//         azimuth from 0 to 360 degress, and range 2 to 
//         510 km for both elevation segments (i.e., 1 
//         and 2).  The Surveillance Channel and Doppler
//         Channel widths are 2 (medium) and 3 (high),
//	   respectively.
//
//   Inputs:
//      regions - Pointer to censor zones regions data.
//
//   Outputs:
//      regions - Pointer to censor zones regions data which
//                has been initialized.
//
//////////////////////////////////////////////////////////////////\*/
static void Init_censor_zones_default_lgcy( RPG_clutter_regions_t *regions )
{
   RPG_clutter_region_data_t *data;
   int i;

   /*
     Set the number of regions to 2.
   */
   regions->regions = 2; 

   for( i = 0; i < regions->regions; i++ )
   {
      data = &regions->data[i];

      /*
        Set start and stop ranges, in km.
      */
      data->start_range = 2;
      data->stop_range = 510;

      /*
        Set start and stop azimuths, in degrees.
      */
      data->start_azimuth = 0;
      data->stop_azimuth = 360;

      /*
        Set segment number.
      */
      data->segment = i+1;

      /*
        Set operator select code to Bypass Map in Control.
      */
      data->select_code = 1;

      /*
        Set Doppler and Surveillance Channel thresholds.
      */
      data->doppl_level = 3;
      data->surv_level = 2;

   }

} /* end of Init_censor_zones_default_lgcy() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//     Initialization routine for clutter censor zone data for file
//     "Default" for Open RDA.  The censor zone defaults are:
//
//         Bypass Map in control in the region bounded in
//         azimuth from 0 to 360 degress, and range 2 to 
//         510 km for all elevation segments (1 to 5).
//
//   Inputs:
//      regions - Pointer to censor zones regions data.
//
//   Outputs:
//      regions - Pointer to censor zones regions data which
//                has been initialized.
//
//////////////////////////////////////////////////////////////////\*/
static void Init_censor_zones_default_orda( ORPG_clutter_regions_t *regions )
{
   ORPG_clutter_region_data_t *data;
   int i;

   /*
     Set the number of regions to 5.  Range is 0 - 19.
   */
   regions->regions = 5; 

   for( i = 0; i < regions->regions; i++ )
   {
      data = &regions->data[i];

      /*
        Set start and stop ranges, in km.
      */
      data->start_range = 0;
      data->stop_range = 511;

      /*
        Set start and stop azimuths, in degrees.
      */
      data->start_azimuth = 0;
      data->stop_azimuth = 360;

      /*
        Set segment number.
      */
      data->segment = i+1;

      /*
        Set operator select code to Bypass Map in Control.
      */
      data->select_code = 1;

   }

} /* end of Init_censor_zones_default_orda() */


/*\/////////////////////////////////////////////////////////
//
//  Description:
//     Builds a "dummy" RDA/RPG message header for Legacy
//     RDA messages.  The date and time stamps are used to
//     indicate invalid data.
//
//  Inputs:  
//     msg_hdr - Pointer to RDA/RPG message header structure.
//     message_type - Message type (see RDA/RPG ICD for valid
//                    message types).
//
/////////////////////////////////////////////////////////\*/
static void Build_bogus_header_lgcy( RDA_RPG_message_header_t *msg_hdr,
   unsigned short message_type )
{
   /*
     Initialize the size of the message to the maximum message
     size.
   */
   msg_hdr->size = MAX_SEGMENT_SIZE;

   /*
     Initialize the rda channel number to non-redundant 
     Legacy RDA configuration.
   */
   msg_hdr->rda_channel = 0;

   /* 
     Initialize the message type. 
   */
   msg_hdr->type = (unsigned char)  message_type;

   /*
     Initialize the sequence number to 0.
   */
   msg_hdr->sequence_num = 0;

   /*
     Initialize the Julian date to 0 (improbable).
    Initialize the number of seconds past midnight to 0.
   */
   msg_hdr->julian_date = 0;
   msg_hdr->milliseconds = 0;

   /*
     Initialize the number of message segments to 0 and the
     segment number to 0.
   */
   msg_hdr->num_segs = 0;
   msg_hdr->seg_num = 0;
   
} /* end of Build_bogus_header_lgcy() */ 


/*\/////////////////////////////////////////////////////////
//
//  Description:
//     Builds a "dummy" RDA/RPG message header for an Open
//     RDA message.  The date and time stamps are used to
//     indicate invalid data.
//
//  Inputs:  
//     msg_hdr - Pointer to RDA/RPG message header structure.
//     message_type - Message type (see RDA/RPG ICD for valid
//                    message types).
//
/////////////////////////////////////////////////////////\*/
static void Build_bogus_header_orda( RDA_RPG_message_header_t *msg_hdr,
   unsigned short message_type )
{
   /*
     Initialize the size of the message to the maximum message
     size.
   */
   msg_hdr->size = MAX_SEGMENT_SIZE;

   /*
     Initialize the rda channel number to non-redundant 
     Open RDA configuration.
   */
   msg_hdr->rda_channel = 8;

   /* 
     Initialize the message type. 
   */
   msg_hdr->type = (unsigned char)  message_type;

   /*
     Initialize the sequence number to 0.
   */
   msg_hdr->sequence_num = 0;

   /*
     Initialize the Julian date to 0 (improbable).
    Initialize the number of seconds past midnight to 0.
   */
   msg_hdr->julian_date = 0;
   msg_hdr->milliseconds = 0;

   /*
     Initialize the number of message segments to 0 and the
     segment number to 0.
   */
   msg_hdr->num_segs = 0;
   msg_hdr->seg_num = 0;
   
} /* end of Build_bogus_header_orda() */ 


