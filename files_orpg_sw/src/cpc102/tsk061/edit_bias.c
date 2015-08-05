#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <rpgc.h>
#include <rpg.h>
#include <rpgcs.h>
#include <rpgp.h>
#include <orpg.h>
#include <lb.h>
#include <legacy_prod.h>
#include <comm_manager.h>
#include <prod_user_msg.h>   
#include <time.h>


#define ORPGDAT_ENVIRON_DATA 10002
#define SHORT_BSWAP(a) ((((a) & 0xff) << 8) | (((a) >> 8) & 0xff))
#define INT_SSWAP(a) ((((a) & 0xffff) << 16) | (((a) >> 16) & 0xffff))
#define AWIPS_BIAS_MSG_FILENAME "awips_bias.msg"


short *outbuf;

/* Static Function Prototypes. */
static void Process_environmental_data_msg( short *msg_data );
static void Process_bias_table( void *msg_data );


int main( int argc, char **argv )
{
   char *buf;
   char *data_dir = NULL;
   char awips_msg_file[ 256 ] = "";
   int  status, size;
   int  fd, fout;

   data_dir = getenv( "ORPGDIR" );
   if( data_dir == NULL ) {
        fprintf( stderr, "Cannot read ORPGDIR\n" );
        exit(1);
   }
   sprintf( awips_msg_file, "%s/%s", data_dir, AWIPS_BIAS_MSG_FILENAME );
 
  fd = open (awips_msg_file, O_RDONLY);
   if (fd < 0) {
        fprintf (stderr, "open (%s, for reading) failed (errno %d)\n",
                         awips_msg_file, errno);
        exit (1);
   }

   size = lseek (fd, 0, SEEK_END);
   if (size < 0) {
        fprintf (stderr, "lseek failed (errno %d)\n", errno);
        exit (1);
   }


   buf = malloc (size);

   if (buf == NULL) {
     fprintf (stderr, "malloc failed\n");
     exit (1);
   }

   lseek (fd, 0, SEEK_SET);

   status = read (fd, buf, size ); 
  
   close (fd);

   /* Display bias table and user can edit a new value for bias field, 
    * if desired. 
    */
   Process_environmental_data_msg( (short *)buf ); 


   /* Write to environ_data.lb file. First try to read the attr from
      a pre-existing LB if it exists. */

   if( ( fout = RPGC_data_access_write( ORPGDAT_ENVIRON_DATA_MSG, (char *)outbuf, 242, ORPGDAT_BIAS_TABLE_MSG_ID ) ) < 0 )
   {
        fprintf (stderr, "ORPGDA_write() for BIAS failed (%d)\n", fout);
        exit (1);
   }

   if (buf != NULL )
      free( buf );

   return 0;

}

static void Process_environmental_data_msg( short *msg_data )
{

   int itime, ilen;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;
   Block_id_t *block_hdr =
      (Block_id_t *) (msg_data + sizeof(Prod_msg_header_icd)/sizeof(short));
 
   /* Get to the start of the block. */

   if (1)
   {
      itime = ((hdr->timem << 16 ) | (hdr->timel & 0xffff));
      itime = INT_SSWAP(itime);
      ilen = ((hdr->lengthm << 16 ) | (hdr->lengthl & 0xffff));    
      ilen = INT_SSWAP(ilen);

      fprintf(stdout,"\n==> Prod_msg_header_icd <==\n");
      fprintf(stdout,"msg_code: %hd\n",hdr->msg_code);
      fprintf(stdout,"date:     %hd\n",hdr->date);
      fprintf(stdout,"time:     %d\n",itime);
      fprintf(stdout,"length:   %d\n", ilen);
      fprintf(stdout,"src_id:   %hd\n",hdr->src_id);
      fprintf(stdout,"des_id:   %hd\n",hdr->dest_id);
      fprintf(stdout,"n_blocks:   %hd\n",hdr->n_blocks);
      fprintf(stdout,"\n==> Block_id_t <==\n");
      fprintf(stdout,"divider:   %hd\n",block_hdr->divider);
      fprintf(stdout,"block_id:   %hd\n",block_hdr->block_id);
      fprintf(stdout,"version:   %hd\n",block_hdr->version);
      fprintf(stdout,"length:   %hd\n",block_hdr->length);
   }
   ORPGMISC_pack_ushorts_with_value( (void *)&hdr->timem, 
                                     (void *)&itime );
   ORPGMISC_pack_ushorts_with_value( (void *)&hdr->lengthm, 
                                     (void *)&ilen );

   fprintf(stdout, "Bias Table Data (%d) Received\n", block_hdr->block_id );

   if ( block_hdr->block_id == BIAS_TABLE_BLOCK_ID )
   {
      Process_bias_table( (void *) msg_data );
   }

} /* End of Process_environmental_data_msg() */


/*****************************************************************************

   Description: Processes Bias Table Data Message.

   Inputs:      msg_data - bias table message.
      
******************************************************************************/
static void Process_bias_table( void *msg_data )
{
   FILE   *fout;
   int    row,col;
   double value;
   char   ans = 'y';
   int    n_rows, i;
   char   awips_id[4], radar_id[4];
   double span, size, gage, radar, bias;
   int    ispan, isize, igage, iradar, ibias;

   Prod_bias_table_msg *bias_tab = (Prod_bias_table_msg *) msg_data;
   outbuf = (short *)bias_tab + 9;  /* Strip off product header - 9 hw */

   /* Open environ_data.lb file */
   fout = fopen( "bias_table.log", "a+" );

   /* Extract the AWIPS ID and Radar ID. */
   memcpy( awips_id, bias_tab->awips_id, 2*sizeof(short) );
   memcpy( radar_id, bias_tab->radar_id, 2*sizeof(short) );

   fprintf(stdout , "\n   Bias Table Data Follows .... \n\n");
   fprintf(fout , "\n   Bias Table Data Follows .... \n\n");

   fprintf(stdout , "      AWIPS ID: %s,  Radar ID: %s\n", awips_id, radar_id );
   fprintf(fout , "      AWIPS ID: %s,  Radar ID: %s\n", awips_id, radar_id );

   bias_tab->awips_id[0]=awips_id[1];
   bias_tab->awips_id[1]=awips_id[0];
   bias_tab->awips_id[2]=awips_id[3];
   bias_tab->awips_id[3]=awips_id[2];

   bias_tab->radar_id[0]=radar_id[1];
   bias_tab->radar_id[1]=radar_id[0];
   bias_tab->radar_id[2]=radar_id[3];
   bias_tab->radar_id[3]=radar_id[2];
      
   if( bias_tab->obs_yr >= 2000 )
      bias_tab->obs_yr -= 2000;
   else
      bias_tab->obs_yr -= 1900;

   fprintf( stdout, "      Observation Date:  %02d/%02d/%02d   "
                    "Observation Time:  %02d:%02d:%02d\n",
                    bias_tab->obs_mon, bias_tab->obs_day, bias_tab->obs_yr,
                    bias_tab->obs_hr, bias_tab->obs_min, bias_tab->obs_sec );
   fprintf( fout, "      Observation Date:  %02d/%02d/%02d   "
                    "Observation Time:  %02d:%02d:%02d\n",
                    bias_tab->obs_mon, bias_tab->obs_day, bias_tab->obs_yr,
                    bias_tab->obs_hr, bias_tab->obs_min, bias_tab->obs_sec );

   if( bias_tab->gen_yr >= 2000 )
      bias_tab->gen_yr -= 2000;
   else
      bias_tab->gen_yr -= 1900;

   fprintf( stdout, "      Generation Date:   %02d/%02d/%02d   "
                    "Generation Time:   %02d:%02d:%02d\n\n",
                    bias_tab->gen_mon, bias_tab->gen_day, bias_tab->gen_yr,
                    bias_tab->gen_hr, bias_tab->gen_min, bias_tab->gen_sec );
   fprintf( fout, "      Generation Date:   %02d/%02d/%02d   "
                    "Generation Time:   %02d:%02d:%02d\n\n",
                    bias_tab->gen_mon, bias_tab->gen_day, bias_tab->gen_yr,
                    bias_tab->gen_hr, bias_tab->gen_min, bias_tab->gen_sec );

   n_rows = bias_tab->n_rows;
   
   fprintf(stdout,"MEMORY SPAN  | EFFECTIVE NO. |   AVG. GAGE   |   AVG. RADAR"
                  "  |   MEAN FIELD  |\n");
   fprintf(stdout,"  (HOURS)    |   G-R PAIRS   |   VALUE (MM)  |   VALUE (MM)"
                  "  |      BIAS     |\n");
   fprintf(fout,"MEMORY SPAN  | EFFECTIVE NO. |   AVG. GAGE   |   AVG. RADAR"
                  "  |   MEAN FIELD  |\n");
   fprintf(fout,"  (HOURS)    |   G-R PAIRS   |   VALUE (MM)  |   VALUE (MM)"
                  "  |      BIAS     |\n");

   for ( i = 0; i < n_rows; i++ )
   {

      /** Display memory span field */
      RPGC_get_product_int(&bias_tab->span[i].mem_span_msw, &ispan);
      ispan = INT_SSWAP(ispan);
      span = (double) ispan / 1000.0;
      span = exp( span );

      /* Write memory span field to LB file */
      RPGC_set_product_int((void *)&bias_tab->span[i].mem_span_msw, 
                           (int) ispan );

      /** Display number pairs gage-radar field */
      RPGC_get_product_int(&bias_tab->span[i].n_pairs_msw, &isize);
      isize = INT_SSWAP(isize); 
      size = (double) isize / 1000.0;

      /* Write num gr_pairs field to LB file */
      RPGC_set_product_int( (void *)&bias_tab->span[i].n_pairs_msw,
                            (int) isize );
      /** Display avg. gage field */
      RPGC_get_product_int(&bias_tab->span[i].avg_gage_msw, &igage);
      igage = INT_SSWAP(igage); 
      gage = (double) igage / 1000.0;

      /* Write avg. gage to LB file */
      RPGC_set_product_int( (void *)&bias_tab->span[i].avg_gage_msw,
                            (int) igage );

      /** Display avg. radar field */
      RPGC_get_product_int(&bias_tab->span[i].avg_radar_msw, &iradar);
      iradar = INT_SSWAP(iradar); 
      radar = (double) iradar / 1000.0;

      /* Write avg. radar to LB file */
      RPGC_set_product_int( (void *)&bias_tab->span[i].avg_radar_msw,
                            (int) iradar );

      /** Display mean field bias */
      RPGC_get_product_int(&bias_tab->span[i].bias_msw, &ibias);
      ibias = INT_SSWAP(ibias); 
      bias = (double) ibias / 1000.0;

      /* Write mean field bias to LB file */
      RPGC_set_product_int( (void *)&bias_tab->span[i].bias_msw,
                            (int) ibias );
      fprintf( stdout, "%13.3f %13.3f %13.3f %13.3f %13.3f\n\n",
                     span,size,gage,radar,bias);
      fprintf( fout, "%13.3f %13.3f %13.3f %13.3f %13.3f\n",
                     span,size,gage,radar,bias);
   }
   while ( ans == 'y' )
   {
      fprintf(stdout,"Do you want to edit Bias fields? (y or n): ");
      scanf("%s",&ans);
      if ( ans == 'n' ) break;
      fprintf(stdout,"Which ROW and COL you want to edit (e.g: 1  4): ");
      scanf("%d %d", &row, &col);
      fprintf(stdout,"Enter a new value to be changed: ");
      scanf("%lf", &value);

      if ( col == 1 )
      {
         fprintf(fout, "\nYou've edited \"Memory Span\" field at ROW ( %d ): "
                       "%12.3f\n", row, value );
         ispan  = (int)log(1000. * value);
         RPGC_set_product_int( (void *)&bias_tab->span[row-1].mem_span_msw,
                               (int) ispan );
      }
      else if ( col == 2 )
      {
         fprintf(fout, "\nYou've edited \"# Pair-Gage\" field at ROW ( %d ): "
                       "%12.3f\n", row, value );
         isize = (int)(1000. * value);
         RPGC_set_product_int( (void *)&bias_tab->span[row-1].n_pairs_msw,
                               (int) isize );
      }
      else if ( col == 3 )
      {
         fprintf(fout, "\nYou've edited \"Average Gage\" field at ROW ( %d ): "
                       "%12.3f\n", row, value );
         igage = (int)(1000. * value);
         RPGC_set_product_int( (void *)&bias_tab->span[row-1].avg_gage_msw,
                               (int) igage );
      }
      else if ( col == 4 )
      {
         fprintf(fout, "\nYou've edited \"Average Radar\" field at ROW ( %d ): "
                       "%12.3f\n", row, value );
         iradar = (int)(1000. * value);
         RPGC_set_product_int( (void *)&bias_tab->span[row-1].avg_radar_msw,
                               (int) iradar );
      }
      else
      {
         fprintf(fout, "\nYou've edited \"Mean Field Bias\" field at ROW "
                       "( %d ): %12.3f\n", row, value );
         ibias = (int)(1000. * value);
         RPGC_set_product_int( (void *)&bias_tab->span[row-1].bias_msw,
                               (int) ibias );
      }
   }

   fclose ( fout );

} /* End of Process_bias_table() */
