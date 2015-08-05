/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/03 16:01:21 $
 * $Id: prod_delay.c,v 1.6 2014/07/03 16:01:21 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*	System and local include file definitions.			*/

#include <stdio.h>
#include <orpg.h>
#include <orpgevt.h>
#include <misc.h>
#include <time.h>
#include <product.h>
#include <rpg_port.h>
#include <rdacnt.h>
#include <vcp.h>

#include <prod_gen_msg.h>		
#include <prod_status.h>

#define MAX_VOLUMES      81
#define MAX_ELEVATIONS   20
#define MAX_STR_LEN   59
typedef struct Elev_sum{

   time_t time;

} Elev_sum_t;

typedef struct Scan_sum{

   Elev_sum_t elev[MAX_ELEVATIONS];

} Scan_sum_t;

static Scan_sum_t Sum[MAX_VOLUMES];

static time_t Max_delay = 0;
static int Report_only_max = 0;

static Prod_gen_msg product_message;
static RDA_rdacnt_t Rdacnt_info;
static Rda_rdacnt_updated = 0;

void notify_func( EN_id_t id, char *msg, int msg_len, void *arg );
static int Get_rpg_elev_index( int vol_num, int vcp_num, int rda_elev_index );
static int Get_command_line_args( int argc, char *argv[] );
static void Write_abort_reason( int prod_id, int prod_code, int reason );
void Rda_rdacnt_lb_notify (int fd, LB_id_t msg_id, int msg_info, void *arg);


/************************************************************************
 *	Description: This is the main function for measuring the	*
 *		     product delay (time between end of elevation or	*
 *		     volume) and when product was generated.		*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *									*
 *		argv - pointer to commandline argument data		*
 *									*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/
int main (int argc, char *argv[]){

   char* hdr_line1 = "---";
   char* hdr_line2 = "------------------------------------------------------------";
   char* hdr_line3 = "-----";
   char* hdr_line4 = "-----";
   int ret, status;
   int pcode, vol_num, elev_index;
   float elev_deg;
   time_t span = 0, start_time;

   char *buf = NULL, *temp_desc = NULL;
   char desc[MAX_STR_LEN + 1];
   Prod_header *phd = NULL;

   Get_command_line_args( argc, argv );

   memset( Sum, 0, sizeof( Scan_sum_t ) );
   memset( desc, 0, MAX_STR_LEN + 1 );   

   /* Register for scan info event. */
   if( (ret = EN_register( ORPGEVT_SCAN_INFO, notify_func )) < 0 ){

      fprintf( stderr, "Scan Info Event Registration Failed\n" );
      exit(0);

   }
   
   /* Register for updates. */
   ORPGDA_write_permission(ORPGDAT_ADAPTATION);
   if( (ret = ORPGDA_UN_register( ORPGDAT_ADAPTATION, RDA_RDACNT, 
                                  Rda_rdacnt_lb_notify ) ) < 0 ){

      fprintf( stderr, "RDA_RDACNT LB Notification Failed\n" );
      exit(0);

   }
   
   /* Do an initial read of adaptation data block RDA_RDACNT. */
   if( (ret = ORPGDA_read( ORPGDAT_ADAPTATION, (char *) &Rdacnt_info, 
                           sizeof(RDA_rdacnt_t), RDA_RDACNT ) ) < 0 ){
 
      fprintf( stderr, "RDA_RDACNT Read Failed (%d)\n", ret );
      exit(0);

   }


   buf = (char *) malloc( sizeof(Prod_header) + sizeof(Graphic_product) );
   if( buf == NULL ){

      fprintf( stderr, "malloc failed for %d bytes\n", sizeof(Prod_header) + sizeof(Graphic_product) );
      exit(1);

   }

   /****** Print header *******/
   fprintf( stdout, "\n\n");
   if ( Report_only_max ) 
      fprintf( stdout, "%-4s %-60s %-5s %-5s\n", " ", " ", " ", "Max" );
   fprintf( stdout, "%-4s %-60s %-5s %-5s\n", "Prod", " ", "Elev", "Delay" );
   fprintf( stdout, "%-3s  %-60s %-5s %-5s\n", "Num", "Description", "(deg)", "(sec)" );
   fprintf( stdout, "%-3s  %-60s %-5s %-5s\n", hdr_line1, hdr_line2, hdr_line3, hdr_line4 );


   /* Read all new product generation messages */
   while (1) {

      /* Check if RDA_RDACNT needs to be re-read. */
      if( Rda_rdacnt_updated ){

         Rda_rdacnt_updated = 0;
         if( (ret = ORPGDA_read( ORPGDAT_ADAPTATION, (char *) &Rdacnt_info, 
                                 sizeof(RDA_rdacnt_t), RDA_RDACNT ) ) < 0 ){
 
            fprintf( stderr, "RDA_RDACNT Read Failed (%d)\n", ret );
            exit(0);

         }

      }

      status = ORPGDA_read( ORPGDAT_PROD_GEN_MSGS,
                            (char *) &product_message,
                            sizeof (Prod_gen_msg),
                            LB_NEXT );

      if (status > 0) {

         /* Does product have product code? */
         pcode = ORPGPAT_get_code( product_message.prod_id );
         if( pcode > 0 ){

            if( product_message.len < 0 )
               Write_abort_reason( product_message.prod_id, pcode, product_message.len );

            else{
    
               /* Read the product from the product data base.  Only need to read the
                  product header and description blocks. */ 
               status = ORPGDA_read( ORPGDAT_PRODUCTS, (char *) buf, 
                                     sizeof(Prod_header),
                                     product_message.id );
               if( (status > 0) || (status == LB_BUF_TOO_SMALL) ){

                  phd = (Prod_header *) buf;

                  vol_num = ORPGMISC_vol_scan_num( phd->g.vol_num );

                  /* Find time difference between when data available and when product
                     available. */
                  if( (start_time = Sum[ vol_num ].elev[ phd->g.elev_ind ].time) != 0 ){

                     span = phd->g.gen_t - start_time;
                     if( Report_only_max ){

                        if( span > Max_delay ){

                           Max_delay = span;

                           temp_desc = ORPGPAT_get_description( product_message.prod_id, STRIP_NOTHING );
                           strncpy( desc, temp_desc, MAX_STR_LEN - 1);
                           strcat( desc, "\0");
                           elev_index = ORPGPAT_elevation_based( product_message.prod_id );
                           if ( elev_index < 0 )   /* Not elevation based product */
                           {
                              fprintf( stdout, "%3d  %-60s %5s %5d\n", pcode, desc, " ", (int)span );
                           }
                           else			/* Elevation based product */
                           {
                              /* Note: the elev angle in the prod hdr is in deg*10  */
                              elev_deg = ( phd->g.resp_params[elev_index] ) * .1;
                              fprintf( stdout, "%3d  %-60s %5.1f %5d\n", pcode, desc, elev_deg, (int)span );
                           }

                        }

                     }
                     else{
                        temp_desc = ORPGPAT_get_description( product_message.prod_id, STRIP_NOTHING );
                        strncpy( desc, temp_desc, MAX_STR_LEN - 1);
                        strcat( desc, "\0");
                        elev_index = ORPGPAT_elevation_based( product_message.prod_id );
                        if ( elev_index < 0 )   /* Not elevation based product */
                        {
                           fprintf( stdout, "%3d  %-60s %5s %5d\n", pcode, desc, " ", (int)span );
                        }
                        else			/* Elevation based product */
                        {
                           /* Note: the elev angle in the prod hdr is in deg*10  */
                           elev_deg = ( phd->g.resp_params[elev_index] ) * .1;
                           fprintf( stdout, "%3d  %-60s %5.1f %5d\n", pcode, desc, elev_deg, (int)span );
                        }
   
                     }
   
                  }

               }
   
            }

         }

      } else if (status == LB_TO_COME) {

         msleep(250);

      } else 
         fprintf (stderr,"ERROR: ORPGDA_read returned %d\n", status);

   }

   return 0;
}

/******************************************************************

   Description:
      Event notification function for receiving the scan info
      event.

   Notes:
      see event notication man page for details on function
      format.

******************************************************************/
void notify_func( EN_id_t id, char *msg, int msg_len, void *arg ){

   Orpgevt_scan_info_t *info;
   int elev_ind;
   extern SMI_info_t *ORPG_smi_info (char *type_name, void *data);

   time_t u_time;

   if( (id != ORPGEVT_SCAN_INFO)
                  ||
       (msg_len < sizeof(Orpgevt_scan_info_t)) ){

      fprintf( stderr, "Ignoring Event %d\n", id );
      return;

   }

#ifdef LITTLE_ENDIAN_MACHINE

   /* Perform necessary byte-swapping. */
   SMIA_set_smi_func( ORPG_smi_info );
   SMIA_bswap_input( "orpgevt_scan_info_t", msg, msg_len );

#endif

   info = (Orpgevt_scan_info_t *) msg;   
   if( (info->key == ORPGEVT_END_ELEV)
                  ||
       (info->key == ORPGEVT_END_VOL) ){

      u_time = time(NULL);
      elev_ind = Get_rpg_elev_index( info->data.vol_scan_number, 
                                     info->data.vcp_number, 
                                     info->data.elev_cut_number );
      Sum[ info->data.vol_scan_number ].elev[ elev_ind ].time = u_time; 

   }
}

/****************************************************************

   Description:
      Given the volume scan number, the vcp number and RDA 
      elevation index, return the RPG elevation index.

****************************************************************/
static int Get_rpg_elev_index( int vol_number, int vcp_num, 
                               int rda_elev_index ){

   int vcp_ind = -1;
   short *rdccon = NULL;

   vcp_ind = vol_number % 2;
   rdccon = (short *) &(Rdacnt_info.data[vcp_ind].rdccon[0]);

   return( (int) rdccon[rda_elev_index - 1] );

}
/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int argc, char *argv[] ){

   extern char *optarg;
   extern int optind;
   int c, err;

   err = 0;
   while ((c = getopt (argc, argv, "dh")) != EOF) {

      switch (c) {

         case 'd':
            Report_only_max = 1;
            break;

         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if( err == 1 ){

      printf ("Usage: %s [options]\n", argv [0]);
      printf ("\toptions:\n");
      printf ("\t   -d Report only the maximum delay to date\n");
      exit(0);

   }

   return 0;

}


/**************************************************************************

    Description: This function prints the abort reason.


**************************************************************************/
static void Write_abort_reason( int prod_id, int pcode, int reason ){

    char *desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );

    switch (reason) {

       case PGM_CPU_LOADSHED:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: CPU Loadshed\n", pcode, desc);
	  break;

       case PGM_MEM_LOADSHED:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: MEM Loadshed\n", pcode, desc);
	  break;

       case PGM_SCAN_ABORT:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Scan Abort\n", pcode, desc);
	  break;

       case PGM_DISABLED_MOMENT:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Disabled Moment\n", pcode, desc);
	  break;

       case PGM_TASK_FAILURE:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Task Failure\n", pcode, desc);
	  break;

       case PGM_REPLAY_DATA_UNAVAILABLE:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Replay Data Not Available\n", pcode, desc);
	  break;

       case PGM_SLOT_UNAVAILABLE:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Slot Unavailable\n", pcode, desc);
	  break;

       case PGM_INPUT_DATA_ERROR:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Input Data Error\n", pcode, desc);
	  break;

       case PGM_TASK_SELF_TERMINATED:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: Task Self Terminated\n", pcode, desc);
	  break;

       default:
	  fprintf( stderr, "**** Product %d (%s) Aborted. Reason: %8d\n", pcode, desc, reason);
	  break;
    }
}

/************************************************************************

    Description: 
      This function sets the init flag LB is updated.             

************************************************************************/
void Rda_rdacnt_lb_notify( int fd, LB_id_t msg_id, int msg_info, void *arg ){

   Rda_rdacnt_updated = 1;

/* End of Rda_rdacnt_lb_notify() */
}

