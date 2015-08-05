/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:45:40 $
 * $Id: incoming_msgs.c,v 1.16 2014/11/07 21:45:40 steves Exp $
 * $Revision: 1.16 $
 * $State: Exp $
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <orpg.h>
#include <comm_manager.h>
#include <prod_user_msg.h>
#include <time.h>
#include <mon_nb.h>
#include <mon_nb_byte_swap.h>
#include <legacy_prod.h>

extern char Message[256];
  
/* Static Global Function Prototypes. */
static void Process_prod_request( int type, short *msg_data );
static char *Print_parameters (short *params);
static void Process_alert_request( short *msg_data );
static void Process_sign_on_request( short *msg_data );
static void Process_max_conn_time_disable_msg( short *msg_data );
static void Process_environmental_data_msg( short *msg_data );
static void Process_external_data_msg( short *msg_data );
static void Process_bias_table( void *msg_data );

#define REQUEST    1
#define REQ_CANCEL 2
/*************************************************************************************

   Description: Process each incoming ICD message (RPG/APUP)

   Inputs:     msg_data - ICD message (starts at message header block)
               size     - size of messsage in bytes.

*************************************************************************************/
void Process_incoming_ICD_message( short *msg_data, int size ){

   int len;
   short msg_type;

   /* Used for byte-swapping. */
   int ret, c_len;
   char *c_format = NULL;

   msg_type = msg_data[0];

#ifdef LITTLE_ENDIAN_MACHINE
   MISC_swap_shorts( 1, &msg_type );
#endif

   /* Process based on message code. */
   switch( msg_type ){

      case MSG_PROD_REQUEST:
      {
         /* For Product Request Messages, we call UMC first. */
         c_len = UMC_from_ICD( msg_data, size, 0, (void *) &c_format );
         if( c_len < 0 ){

            fprintf( stderr, "UMC_from_ICD Failed For MSG_PROD_REQUEST (%d)\n",
                     c_len );
            return;

         }

         Process_prod_request( REQUEST, (short *) c_format );
         free( c_format );
         c_format = NULL;

         break;
      }

      case MSG_MAX_CON_DISABLE:
      {
         /* For Maximum Connection Time Disable Request Messages, 
            we check if byte-swapping is necessary.  If necessary,
            byte-swapping is done in-place. */
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_MAX_CON_DISABLE (%d)\n",
                     ret );
            return;

         }

         Process_max_conn_time_disable_msg( (short *) msg_data );
         break;

      }

      case MSG_SIGN_ON:
      {
         /* For Sign On Request Messages, we check if byte-swapping
            is necessary.  If necessary, byte-swapping is done in-place. */
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_SIGN_ON (%d)\n",
                     ret );
            return;

         }

         Process_sign_on_request( (short *) msg_data );
         break;

      }

      case MSG_ALERT_REQUEST:
      {
         /* For Alert Request Messages, we call UMC first. */
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_ALERT_REQUEST (%d)\n",
                     ret );
            return;

         }

         Process_alert_request( (short *) msg_data );
         break;

      }

      case MSG_PROD_REQ_CANCEL:
      {

         /* For Product Request Messages, we call UMC first. */
         c_len = UMC_from_ICD( msg_data, size, 0, (void *) &c_format );
         if( c_len < 0 ){

            fprintf( stderr, "UMC_from_ICD Failed (%d)\n", c_len );
            return;

         }

         Process_prod_request( REQ_CANCEL, (short *) c_format );
         free( c_format );
         break;

      }

      case MSG_PROD_LIST:
      {
         /* For Product Request Messages, we call UMC first. */
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "UMC_from_ICD Failed (%d)\n", ret );
            return;

         }

         {
            Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

            len = sprintf( Message, 
               "---> PRODUCT LIST REQUEST (Msg Code 8):   From ID: %04d, To ID: %04d\n",
               hdr->src_id, hdr->dest_id );
            Print_msg( len, -1 );

         }

         break;
      }

      case MSG_ENVIRONMENTAL_DATA:
      {
         /* For Product Request Messages, we call UMC first. */
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed (%d)\n", ret );
            return;

         }

         Process_environmental_data_msg( (short *) msg_data );
         break;

      }

      case MSG_EXTERNAL_DATA:
      {
         ret = From_ICD( msg_data, size );
         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed (%d)\n", ret );
            return;

         }

         Process_external_data_msg( (short *) msg_data );
         break;

      }

      default:
      {
          fprintf( stderr, "Unknown Message Type Received: %d\n", msg_type );
          break;
      }

   /* End of "switch" statment. */
   }

/* End of Process_incoming_ICD_message() */
}

/**************************************************************************

   Description: This function prints the routine product distribution list.
  
   Inputs:      msg_data - the routine distribution list.

**************************************************************************/
static void Process_prod_request( int type, short *msg_data ){

   int color = -1, i, len, cde, cnt;
   char *mne, *date_time_str;
   char priority[3], date_time[132];
   Pd_prod_header *req_hdr;
   Pd_request_products *req_data;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;
  
 
   if( type == REQUEST ){

      color = BLUE;
      len = sprintf( Message, 
        "---> PRODUCT REQUEST(S) (Msg Code 0):   From ID: %04d, To ID: %04d",
        hdr->src_id, hdr->dest_id );

   }
   else if( type == REQ_CANCEL ){

      color = ORANGE;
      len = sprintf( Message, 
        "---> PRODUCT REQUEST(S) CANCELLED (Msg Code 13):   From ID: %04d, To ID: %04d",
        hdr->src_id, hdr->dest_id );

   }
   else
      return;
   
   Print_msg( len, color );

   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   req_hdr = (Pd_prod_header *) msg_data;
   cnt = req_hdr->n_blocks - 1;
   if( cnt > 0 ){

      len = sprintf( Message,  
            "    cde mne pri num int   seq     Vol Date Vol Time     p1  p2  p3  p4  p5  p6" );
      Print_msg( len, GRAY ); 

      req_data = (Pd_request_products *) (msg_data + sizeof(Pd_prod_header)/sizeof(short));
      for (i = 0; i < cnt; i++){
  
         /* Get product mneumonic. */
         cde = (int) req_data->prod_id;
         mne = Get_product_mnemonic_str( cde );

         /* Get flag bits. */
         if( req_data->flag_bits & 0x8000 ){

            memcpy( priority, "HP", 2);
            priority[2] = '\0';

         }
         else{
 
            memcpy( priority, "LP", 2);
            priority[2] = '\0';

         }
 
         date_time[0] = '\0';
         if( req_data->VS_start_time > 0 ){

            date_time_str = Get_date_time_str( req_data->VS_date, req_data->VS_start_time );
            strcat( date_time, date_time_str );

         }
         else if( req_data->VS_start_time == -1 ){

            len = sprintf( date_time, " Current Product " );
            date_time[len] = '\0';

         }
         else if( req_data->VS_start_time == -2 ){

            len = sprintf( date_time, " Latest Available" );
            date_time[len] = '\0';

         }
         else if( req_data->VS_start_time == 0 ){

            len = sprintf( date_time, " Unspecified Time" );
            date_time[len] = '\0';

         }
         else
            date_time[0] = '\0';

         /* Print product data. */
         len = sprintf( Message,  
               "    %3d %3s %3s %3d %3d %5d    %s     %s",
               cde, mne, priority, req_data->num_products, req_data->req_interval, 
               req_data->seq_number, date_time, Print_parameters (req_data->params));
  
         Print_msg( len, GRAY );
  
         /* Go to next product. */
         req_data++;

      /* End of "for" loop. */
      }

      len = sprintf( Message, "\n" );
      Print_msg( len, -1 );

   }

   return;

/* End of Process_prod_request() */
}

/**************************************************************************

   Description: This function prints the product parameters.
  
   Inputs:     params - the product parameters.
 
   Return:     A pointer to the buffer of the printed text.
 
**************************************************************************/
static char *Print_parameters (short *params){

   static char buf[64];        /* buffer for the parameter text */
   char *pt;
   int i;
  
   pt = buf;
   for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {

      int p;
  
      p = params[i];
      switch (p) {

         case PARAM_UNUSED:
            strcpy (pt, "UNU ");
            pt += 4;
            break;
         case PARAM_ANY_VALUE:
            strcpy (pt, "ANY ");
            pt += 4;
            break;
         case PARAM_ALG_SET:
            strcpy (pt, "ALG ");
            pt += 4;
            break;
         case PARAM_ALL_VALUES:
            strcpy (pt, "ALL ");
            pt += 4;
            break;
         case PARAM_ALL_EXISTING:
            strcpy (pt, "EXS ");
            pt += 4;
            break;
         default:
            sprintf (pt, "%3d ", p);
            pt += strlen (pt);
            break;

      }

   }
   return (buf);

/* End of Print_parameters() */
}                       

#define NUM_ROWS  58
#define NUM_COLS  58
/**************************************************************************

   Description: Processes Alert Request Message.  Writes out alert categories
                and alert grid.

   Inputs:     msg_data - ICD message data (starts at message header block).

**************************************************************************/
static void Process_alert_request( short *msg_data ){


   short *cat_def, *grid_def;
   unsigned short mask;
   int area_num, num_cols, num_cat, len, i, j, k;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Set alert area and number of alert categories. */
   area_num = msg_data[11];
   num_cat = msg_data[12];

   /* List the category definition. */
   cat_def = &msg_data[13];

   len = sprintf( Message, 
     "---> ALERT REQUEST MESSAGE (Msg Code 7):   From ID: %04d, To ID: %04d",
     hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );
  
   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   /* Write out alert area information. */
   len = sprintf( Message, "   Alert Area %d", area_num );
   Print_msg( len, GRAY );
   len = sprintf( Message, "      Category    Thresh Code    Prod Req Flg" );
   Print_msg( len, GRAY );
   for( i = 0; i < num_cat; i++ ){
  
      if( cat_def[2] == 1 ){

         len = sprintf( Message, "        %2d           %2d                Y",
                        cat_def[0], cat_def[1] );
         Print_msg( len, GRAY );

      }
      else if( cat_def[2] == 0 ){

         len = sprintf( Message, "        %2d           %2d                N",
                        cat_def[0], cat_def[1] );
         Print_msg( len, GRAY );

      }
      cat_def += 3;
   
   }

   grid_def = cat_def;
   len = sprintf( Message, "      Alert Grid %d", area_num );
   Print_msg( len, GRAY );

   for( i = 0; i < NUM_ROWS; i++ ){
  
      char grid_str[72];

      memset( grid_str, 0, 72 );
      num_cols = 0;

      for( j = 0; j < 4; j++ ){

         mask = 0x8000;
         for( k = 0; k < 16; k++ ){

            if( grid_def[j] & mask )
               strcat( grid_str, "1" );

            else
               strcat( grid_str, "0" );

            mask = mask/2;
            num_cols++;

            if( num_cols >= NUM_COLS )
               break;

         }

      }

      len = sprintf( Message, "         %s", grid_str );
      Print_msg( len, GRAY );
      grid_def += 4;
   
   }           

   len = sscanf( Message, "\n" );
   Print_msg( len, -1 );

/* End of Process_alert_request() */
}

/********************************************************************************

   Description: Processes Maximum Connection Time Disable for dial-in user.

   Inputs:      msg_data - Maximum Connection Time Message.

********************************************************************************/
static void Process_max_conn_time_disable_msg( short *msg_data ){

   int len;
   Prod_max_conn_time_disable_msg_icd *max = 
            (Prod_max_conn_time_disable_msg_icd *) msg_data;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   len = sprintf( Message, 
      "---> MAX CONNECT TIME DISABLE REQUEST (Msg Code 4):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );

   len = sprintf( Message, "   Additional Connect Time: %5d\n", max->add_conn_time );
   Print_msg( len, GRAY );

/* End of Process_max_conn_time_disable_msg() */
}

/********************************************************************************

   Description: Processes Sign On Message from dial-in user.

   Inputs:      msg_data - Sign on message.
      
********************************************************************************/
static void Process_sign_on_request( short *msg_data ){

   int len;
   char dial_password[USER_PASSWD_SIZE+1];  /* +1 for the nul terminator */
   char port_password[PORT_PASSWD_SIZE+1];  /* +1 for the nul terminator */
   Prod_sign_on_msg_icd *sign = (Prod_sign_on_msg_icd *) msg_data;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   len = sprintf( Message, 
      "---> SIGN ON REQUEST (Msg Code 11):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );
   
   memcpy( dial_password, sign->user_passwd, USER_PASSWD_LEN );
   dial_password[USER_PASSWD_SIZE] = '\0';
   len = sprintf( Message, "   Dial-up Password: %s", dial_password );
   Print_msg( len, GRAY );

   memcpy( port_password, sign->port_passwd, PORT_PASSWD_LEN );
   port_password[PORT_PASSWD_SIZE] = '\0';
   len = sprintf( Message, "   Port Password: %s", port_password );
   Print_msg( len, GRAY );
   
   if( sign->disconn_override_flag == 1 ){

      len = sprintf( Message, "   Disconnect Override Flag: Y\n" );
      Print_msg( len, GRAY );

   }
   else if( sign->disconn_override_flag == 0 ){

      len = sprintf( Message, "   Disconnect Override Flag: N\n" );
      Print_msg( len, GRAY );

   }
   else{

      len = sprintf( Message, "   Disconnect Override Flag: %d\n", sign->disconn_override_flag );
      Print_msg( len, GRAY );

   }

/* End of Process_sign_on_request() */
}

/********************************************************************************

   Description: Processes Environmental Data Message.

   Inputs:      msg_data - environmental message.
      
********************************************************************************/
static void Process_environmental_data_msg( short *msg_data ){

   int len;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;
   Block_id_t *block_hdr = 
      (Block_id_t *) (msg_data + sizeof(Prod_msg_header_icd)/sizeof(short));

   len = sprintf( Message, 
      "---> ENVIRONMENTAL DATA (Msg Code 15):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );
   
   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   switch( block_hdr->block_id ){ 

      case BIAS_TABLE_BLOCK_ID:
      {
         Process_bias_table( (void *) msg_data );
         break;

      }

      default:
         break;

   }
   
/* End of Process_environmental_data_msg() */
}

/********************************************************************************

   Description: Processes Bias Table Data Message.

   Inputs:      msg_data - bias table message.
      
********************************************************************************/
static void Process_bias_table( void *msg_data ){

   int len, n_rows, i;
   char awips_id[5], radar_id[5];
   Prod_bias_table_msg *bias_tab = (Prod_bias_table_msg *) msg_data;

   /* Extract the AWIPS ID and Radar ID. */
   memcpy( awips_id, bias_tab->awips_id, 2*sizeof(short) );
   awips_id[4] = 0;

   memcpy( radar_id, bias_tab->radar_id, 2*sizeof(short) );
   radar_id[4] = 0;

   len = sprintf( Message, "   Bias Table Data Follows .... ");
   Print_msg( len, GRAY );

   len = sprintf( Message, "      AWIPS ID: %s,  Radar ID: %s", awips_id, radar_id );
   Print_msg( len, GRAY );

   if( bias_tab->obs_yr >= 2000 )
      bias_tab->obs_yr -= 2000;
   else
      bias_tab->obs_yr -= 1900;

   len = sprintf( Message,
      "      Observation Date:  %02d/%02d/%02d   Observation Time:  %02d:%02d:%02d", 
      bias_tab->obs_mon, bias_tab->obs_day, bias_tab->obs_yr, 
      bias_tab->obs_hr, bias_tab->obs_min, bias_tab->obs_sec );
   Print_msg( len, GRAY );

   if( bias_tab->gen_yr >= 2000 )
      bias_tab->gen_yr -= 2000;
   else
      bias_tab->gen_yr -= 1900;

   len = sprintf( Message,
      "      Generation Date:   %02d/%02d/%02d   Generation Time:   %02d:%02d:%02d", 
      bias_tab->gen_mon, bias_tab->gen_day, bias_tab->gen_yr, 
      bias_tab->gen_hr, bias_tab->gen_min, bias_tab->gen_sec );
   Print_msg( len, GRAY );

   n_rows = bias_tab->n_rows;

   for( i = 0; i < n_rows; i++ ){

      double span, size, gage, radar, bias;
      int    ispan, isize, igage, iradar, ibias;

      len = sprintf( Message, "      Row %d", (i+1) );
      Print_msg( len, GRAY );

      ORPGMISC_unpack_value_from_ushorts( &bias_tab->span[i].mem_span_msw, &ispan );
      span = (double) ispan / 1000.0;
      span = exp( span );

      ORPGMISC_unpack_value_from_ushorts( &bias_tab->span[i].n_pairs_msw, &isize );
      size = (double) isize / 1000.0;

      ORPGMISC_unpack_value_from_ushorts( &bias_tab->span[i].avg_gage_msw, &igage );
      gage = (double) igage / 1000.0;

      ORPGMISC_unpack_value_from_ushorts( &bias_tab->span[i].avg_radar_msw, &iradar );
      radar = (double) iradar / 1000.0;

      ORPGMISC_unpack_value_from_ushorts( &bias_tab->span[i].bias_msw, &ibias );
      bias = (double) ibias / 1000.0;

      len = sprintf( Message, "         Period of Analysis:   %10.3f", span );
      Print_msg( len, GRAY );

      len = sprintf( Message, "         # Gage-Radar Pairs:   %10.3f", size );
      Print_msg( len, GRAY );

      len = sprintf( Message, "         Avg Hrly Gage Accum:  %10.3f", gage ); 
      Print_msg( len, GRAY );

      len = sprintf( Message, "         Avg Hrly Radar Accum: %10.3f", radar ); 
      Print_msg( len, GRAY );

      len = sprintf( Message, "         Mean Field Bias:      %10.3f", bias ); 
      Print_msg( len, GRAY );

   }

   len = sprintf( Message, "\n" );
   Print_msg( len, -1 );

/* End of Process_bias_table() */
}

/********************************************************************************

   Description: Processes External Data Message.

   Inputs:      msg_data - environmental message.
      
********************************************************************************/
static void Process_external_data_msg( short *msg_data ){

   int len;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* TBD - the External Data message is currently being defined.  For now
      we're just printing a message saying we received it. */

   len = sprintf( Message, 
      "---> EXTERNAL DATA (Msg Code 5):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );
   
/* End of Process_external_data_msg() */
}

