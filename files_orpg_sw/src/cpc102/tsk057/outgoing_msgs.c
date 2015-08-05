/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/10 15:19:15 $
 * $Id: outgoing_msgs.c,v 1.41 2014/11/10 15:19:15 steves Exp $
 * $Revision: 1.41 $
 * $State: Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orpg.h>
#include <comm_manager.h>
#include <prod_user_msg.h>
#include <time.h>
#include <mon_nb.h>
#include <mon_nb_byte_swap.h>
#include <legacy_prod.h>
#include <product.h>

extern char Message[256];

static unsigned int Cum_size = 0;
  
#define PROD_HDR_DESC_BLK_SIZE_SHORTS  (sizeof(Graphic_product)/sizeof(short))
#define FTM_CODE        75

/* Static Global Function Prototypes. */
static void Process_gen_stat_message( short *msg_data );
static void Process_req_resp_message( short *msg_data );
static void Process_alert_message( short *msg_data );
static void Process_alert_parameter_message( short *msg_data );
static void Process_product_list_message( short *msg_data );

/*************************************************************************************

   Description: Process each outgoing ICD message (RPG/APUP)

   Inputs:     msg_data - ICD message (starts at message header block)
               size     - size of messsage in bytes.

*************************************************************************************/
void Process_outgoing_ICD_message( CM_req_struct *req, short *msg_data, int size ){

   int prod_id, len, ret;
   short msg_type;
   static int prod_size[MAX_N_STATIONS] = { 0, 0, 0 };
   static int remaining_size[MAX_N_STATIONS] = { 0, 0, 0 };
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;


   if( remaining_size[req->parm] > 0 ){

      if( !Terse_mode ){

         len = sprintf( Message, "<---- Continuing Message (Size: %d)\n", size );
         Print_msg( len, GRAY );

      }      

      remaining_size[req->parm] -= req->data_size;

      if( remaining_size[req->parm] < 0 )
         remaining_size[req->parm] = 0;

      return;

   }

   msg_type = msg_data[0];

#ifdef LITTLE_ENDIAN_MACHINE
   MISC_swap_shorts( 1, &msg_type );
#endif
 
   /* Process Product Messages */
   switch( msg_type ){

      case MSG_GEN_STATUS:
      {
         /* Perform byte-swapping, if necessary */
         ret = From_ICD( msg_data, size );

         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_GEN_STATUS (%d)\n",
                     ret );
            return;

         }

         Process_gen_stat_message( msg_data ); 
         break;
      }

      case MSG_REQ_RESPONSE:
      {
         /* Perform byte-swapping, if necessary */
         ret = From_ICD( msg_data, size );

         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_REQ_RESPONSE (%d)\n",
                     ret );
            return;

         }
         Process_req_resp_message( msg_data );
         break;
      }

      case MSG_ALERT_PARAMETER:
      { 
         /* Perform byte-swapping, if necessary */
         ret = From_ICD( msg_data, size );

         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_ALERT_PARAMETER (%d)\n",
                     ret );
            return;

         }
         Process_alert_parameter_message( msg_data );
         break;
      }

      case MSG_PROD_LIST:
      {
         /* Perform byte-swapping, if necessary */
         ret = From_ICD( msg_data, size );

         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_PROD_LIST (%d)\n",
                     ret );
            return;

         }
         Process_product_list_message( msg_data );
         break;
      }

      case MSG_ALERT:
      {
         /* Perform byte-swapping, if necessary */
         ret = From_ICD( msg_data, size );

         if( ret < 0 ){

            fprintf( stderr, "From_ICD Failed For MSG_ALERT (%d)\n",
                     ret );
            return;

         }
         Process_alert_message( msg_data );
         break;
      }

      default:

#ifdef LITTLE_ENDIAN_MACHINE
         MISC_swap_shorts( PROD_HDR_DESC_BLK_SIZE_SHORTS, msg_data );
#endif

         prod_id = ORPGPAT_get_prod_id_from_code( (int) msg_data[0] );
         if( (prod_id > 0) && (msg_data[0] >= 16) ){

            Graphic_product *phd = (Graphic_product *) msg_data;
            char *mnemonic = NULL, *date_time_str = NULL, *gen_date_time_str = NULL;
            int vol_date = phd->vol_date;
            int vol_time = ((phd->vol_time_ms << 16) & 0xffff0000) +
                           (phd->vol_time_ls & 0x0000ffff);
            short *msg_len = (short *) &phd->msg_len;
 
            prod_size[req->parm] = (msg_len[0] << 16) | (msg_len[1] & 0xffff);
            remaining_size[req->parm] = prod_size[req->parm] - req->data_size;

            /* Add to cumulative size. */
            Cum_size += prod_size[req->parm];

            len = sprintf( Message, "<--- PRODUCT (Size: %d Bytes, Total Bytes: %u):   From ID: %04d, To ID: %04d", 
                           prod_size[req->parm], Cum_size, hdr->src_id, hdr->dest_id );
            Print_msg( len, BLUE );

            /* Get product mnemonic. */
            mnemonic = Get_product_mnemonic_str((int) phd->msg_code );

            /* Convert volume scan date and time. */
            date_time_str = Get_date_time_str( vol_date, vol_time );


            len = sprintf( Message, 
                     "   Code: %3d (%3s)  Seq#: %5d Vol#: %2d  Vol Date/Time: %s", 
                     phd->msg_code, mnemonic, phd->seq_num, phd->vol_num, date_time_str );
            Print_msg( len, GRAY );

            if( !Terse_mode ){

               unsigned short *btime = (unsigned short *) &phd->gen_time;
               int gen_time = ((btime[0] << 16) & 0xffff0000) + (btime[1] & 0x0000ffff);

               /* Convert generation date/time to string. */
               len = sprintf( Message, "   Param: %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d", 
                              phd->param_1, phd->param_2, phd->param_3, phd->param_4, phd->param_5, 
                              phd->param_6, phd->param_7, phd->param_8, phd->param_9, phd->param_10 );
               Print_msg( len, GRAY );

               gen_date_time_str = Get_date_time_str( (int) phd->gen_date, gen_time );
               len = sprintf( Message, "   Gen Date/Time: %s\n", gen_date_time_str );
               Print_msg( len, GRAY );

               /* If this is a FTM, dump the contents. */
               if( phd->msg_code == FTM_CODE ){

                  int i;
                  short *np = (short *) ((char *) msg_data + sizeof(Graphic_product)) + 1;
                  short num_pages;
                  short num_char;
                  char msg[81];

#ifdef LITTLE_ENDIAN_MACHINE
                  MISC_swap_shorts( 1, np );
#endif
                  num_pages = *np;
                  len = sprintf( Message, "   FTM Number Pages: %2d", num_pages );
                  Print_msg( len, GRAY );

                  np++;
                  for( i = 0; i < num_pages; i++ ){

#ifdef LITTLE_ENDIAN_MACHINE
                     MISC_swap_shorts( 1, np );
#endif
                     num_char =  *np;
                     while( num_char != -1 ){

                        if( (num_char <= 0) || (num_char > 80) ){

                           len = sprintf( Message, "Bad Number of Characters (%d) in FTM", num_char );
                           Print_msg( len, RED );
                           break;

                        }

                        len = sprintf( Message, "   Characters this line: %2d", num_char );
                        Print_msg( len, -1 );

                        /* Copy contents and print. */
                        np++;
                        memcpy( msg, (char *) np, num_char );
                        msg[num_char] = '\0';
                        len = sprintf( Message, "   FTM: %s", msg );
                        Print_msg( len, BLUE );

                        /* Go to next line. */
                        if( (num_char % 2) != 0 )
                           num_char++;

                        np = (short *) ((char *) np + num_char);
#ifdef LITTLE_ENDIAN_MACHINE
                        MISC_swap_shorts( 1, np );
#endif
                        num_char = *np;
                        if( num_char == -1 )
                           break;

                     }

                  }

               }

            }
            else
               fprintf( stdout, "\n" );

         }
         break;

   /* End of "switch" statment. */
   }

/* End of Process_incoming_ICD_message() */
}

#define MOMENT_MASK		0x1f
#define DUAL_POL_MASK		0x20
#define BITS_IN_SHORT		16

/**************************************************************************

   Description: Processes General Status Message

   Inputs:      msg_data - General Status Message.

**************************************************************************/
static void Process_gen_stat_message( short *msg_data ){

   int len, data_trans_enable, dual_pol_enable, n_elev, color, level, i;
   Prod_general_status_msg_icd *gsm;

   int op_mode;
   char *op_mode_str[ ] = { " Maintenance ",
                            " Clear Air  ",
                            " Precipitation " };

   char rda_oper_str[128];
   char *rda_oper[ ] = { " Auto Cal Disabled ",
                         " On Line ",
                         " Maint Required ",
                         " Maint Mandatory ",
                         " Commanded Shutdown ",
                         " Inoperable ",
                         " Wideband Disconnect ",
                         " Indeterminate " };

   char elev[8];

   char rda_stat_str[128];
   char *rda_stat[ ] = { " Startup ",
                         " Standby ",
                         " Restart ",
                         " Operate ",
                         " Playback ",
                         " Off-line Operate ",
                         " Indeterminate " };

   char rda_alarms_str[128];
   char *rda_alarms[ ] = { " Indeterminate ",
                           " Tower/Utilities ",
                           " Pedestal ",
                           " Transmitter ",
                           " Receiver ",
                           " RDA Control ",
                           " Communication ",
                           " Signal Processor ",
                           " No Alarms " };
 
   char moment_str[10];
   char *moments[] = { " None ",
                       " All  ",
                       " R",
                       " V",
                       " W" }; 

   char *dual_pol[] = { " Enabled  ",
                        " Disabled " };
   
   char rpg_oper_str[128];
   char *rpg_oper[ ] = { " Loadshed ",
                         " On Line ",
                         " Maint Required ",
                         " Maint Mandatory ",
                         " Commanded Shutdown "};

   char rpg_alarms_str[128];
   char *rpg_alarms[ ] = { " No Alarms ",
                           " Node Connectivity ",
                           " Wideband Failure ",
                           " RPG Control Failure ",
                           " Data Base Failure ",
                           " Spare Failure ",
                           " RPG Input Buffer Loadshed ",
                           " Spare (26) Failure ",
                           " Product Storage Loadshed ",
                           " Spare (22) Failure ",
                           " Spare (21) Failure ",
                           " RDA Wideband ",
                           " RPG/RPG Intercomputer Link Failure ",
                           " Redundant Channel Error ",
                           " RPG Task Failure ",
                           " Media Failure" };

   char rpg_stat_str[128];
   char *rpg_stat[ ] = { " Restart ",
                         " Operate ",
                         " Standby ",
                         " Test Mode " };

   char rpg_nb_stat_str[128];
   char *rpg_nb_stat[ ] = { " Commanded Disconnect ",
                            " Narrowband Loadshed " };

   char rpg_prod_avail_str[128];
   char *rpg_prod_avail[ ] = { " Products Available ",
                               " Product Availability Degraded ",
                               " Products Unavailable " };

#define MAX_SUPPL_BITS		16
   char *vcp_supplemental[] = { " AVSET Enabled ",
                                " SAILS Active ",
                                " Site-Specific VCP ",
                                " RxRN Enabled ",
                                " CBT Enabled ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? ",
                                " ???? " };
   
   float calib_const = 0.0;

   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Add to cumulative size. */
   unsigned int size = (hdr->lengthm << 16) | (hdr->lengthl & 0xffff);
   Cum_size += size;

   len = sprintf( Message, 
      "<--- GENERAL STATUS MESSAGE (Msg Code 2):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );

   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   gsm = (Prod_general_status_msg_icd *) msg_data;

   /* Determine Operational Mode. */
   op_mode = gsm->wx_mode;
   len = sprintf( Message, "   Mode (%4x):            %s", op_mode, op_mode_str[op_mode] );
   Print_msg( len, GRAY );     

   /* Determine RDA Operability Status. */
   memset( rda_oper_str, 0, 128 );

   color = GRAY;
   if( (gsm->rda_op_status & GSM_RDA_OPER_AUTO_CAL_DIS) )
      strcat( rda_oper_str, rda_oper[0] );

   if( (gsm->rda_op_status & GSM_RDA_OPER_ON_LINE) ){

      color = GREEN;
      strcat( rda_oper_str, rda_oper[1] );

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_MR) ){

      color = YELLOW;
      strcat( rda_oper_str, rda_oper[2] );

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_MM) ){

      color = ORANGE;
      strcat( rda_oper_str, rda_oper[3] );

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_CMD_SHUTDOWN) ){

      strcat( rda_oper_str, rda_oper[4] );
      color = RED;

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_INOPERABLE) ){

      strcat( rda_oper_str, rda_oper[5] );
      color = RED;

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_WB_DISC) ){

      strcat( rda_oper_str, rda_oper[6] );
      color = RED; 

   }

   if( (gsm->rda_op_status & GSM_RDA_OPER_INDETERMINATE) == 0 ){

      strcat( rda_oper_str, rda_oper[7] );
      color = RED;

   }
   
   level = color;
   len = sprintf( Message, "   RDA Operability (%4x): %s", gsm->rda_op_status, rda_oper_str );

   Print_msg( len, color );     
   
   /* Process the VCP. */
   len = sprintf( Message, "   VCP:                     %3d", gsm->vcp );
   Print_msg( len, GRAY );

   /* Process the number of elevations in the VCP. */
   len = sprintf( Message, "   Number Elevs:            %2d", gsm->n_elev );
   Print_msg( len, GRAY );

   sprintf( Message, "      Elevs:             " );

   n_elev = gsm->n_elev;
   if( gsm->n_elev > MAX_GS_N_ELEV )
      n_elev = MAX_GS_N_ELEV;
   for( i = 0; i < n_elev; i++ ){

      sprintf( elev, "%4d", gsm->elev_angle[i] );
      strcat( Message, elev );

   }

   /* Add any additional elevations up to a maximum of MAX_GS_N_ELEV + MAX_GS_ADD_ELEV. */
   n_elev = gsm->n_elev - MAX_GS_N_ELEV;
   if( n_elev > 0 ){

      for( i = 0; i < n_elev; i++ ){

         sprintf( elev, "%4d", gsm->add_elev_angle[i] );
         strcat( Message, elev );

      }

   }

   len = strlen( Message ); 
   Print_msg( len, GRAY );

   /* Process the Super Resolution cuts. */
   sprintf( Message, "   Super Res Elevs:      " );
   for( i = 0; i < gsm->n_elev; i++ ){

      /* There are only BITS_IN_SHORT bits so only report
         the first BIT_IN_SHORT elevations. */
      if( (i < BITS_IN_SHORT) && (gsm->super_res & (1 << i)) )
         sprintf( elev, "%4d", gsm->elev_angle[i] );

      else
         sprintf( elev, "%4s", "  --" );

      strcat( Message, elev );

   }

   len = strlen( Message ); 
   Print_msg( len, GRAY );

   /* Process the Clutter Mitigation Decision. */
   sprintf( Message, "   Clut Mit Dec Elevs:   " );
   for( i = 1; i <= 5; i++ ){

      if( gsm->cmd & (1 << i) ){

         sprintf( elev, " %2d", i );
         strcat( Message, elev );

      }

   }

   len = strlen( Message );
   Print_msg( len, GRAY );


   /* Determine RDA Status. */
   memset( rda_stat_str, 0, 128 );

   if( (gsm->rda_status & GSM_RDA_STAT_STARTUP) )
      strcat( rda_stat_str, rda_stat[0] );

   if( (gsm->rda_status & GSM_RDA_STAT_STANDBY) )
      strcat( rda_stat_str, rda_stat[1] );

   if( (gsm->rda_status & GSM_RDA_STAT_RESTART) )
      strcat( rda_stat_str, rda_stat[2] );

   if( (gsm->rda_status & GSM_RDA_STAT_OPERATE) )
      strcat( rda_stat_str, rda_stat[3] );

   if( (gsm->rda_status & GSM_RDA_STAT_PLAYBACK) )
      strcat( rda_stat_str, rda_stat[4] );

   if( (gsm->rda_status & GSM_RDA_STAT_OFFLINE) )
      strcat( rda_stat_str, rda_stat[5] );

   if( (gsm->rda_status & GSM_RDA_STAT_INDETERMINATE) == 0 )
      strcat( rda_stat_str, rda_stat[6] );

   len = sprintf( Message, "   RDA Status (%4x):      %s", 
                  gsm->rda_status, rda_stat_str );
   Print_msg( len, GRAY );     

   /* Determine RDA Alarms. */
   memset( rda_alarms_str, 0, 128 );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_INDETERMINATE) )
      strcat( rda_alarms_str, rda_alarms[0] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_TOWER_UTILS) )
      strcat( rda_alarms_str, rda_alarms[1] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_PEDESTAL) )
      strcat( rda_alarms_str, rda_alarms[2] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_TRANSMITTER) )
      strcat( rda_alarms_str, rda_alarms[3] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_RECEIVER) )
      strcat( rda_alarms_str, rda_alarms[4] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_RDA_CONTROL) )
      strcat( rda_alarms_str, rda_alarms[5] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_COMMUNICATION) )
      strcat( rda_alarms_str, rda_alarms[6] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_SIG_PROC) )
      strcat( rda_alarms_str, rda_alarms[7] );

   if( (gsm->rda_alarms & GSM_RDA_ALARMS_NO_ALARMS) == 0 ){

      level = GREEN;
      strcat( rda_alarms_str, rda_alarms[8] );

   }

   /* Note: The color is based on Operability Status. */
   len = sprintf( Message, "   RDA Alarms (%4x):      %s", 
                  gsm->rda_alarms, rda_alarms_str );

   Print_msg( len, level );     

   /* Process data transmission enabled. */
   moment_str[0] = '\0';
   data_trans_enable = gsm->data_trans_enable & MOMENT_MASK;
   if( data_trans_enable == GSM_MOMENTS_NONE)
      strcat( moment_str, moments[0] );
   
   else if( data_trans_enable == (GSM_MOMENTS_REF | GSM_MOMENTS_VEL | GSM_MOMENTS_WID) )
      strcat( moment_str, moments[1] );
   
   else{
  
      if( (data_trans_enable & GSM_MOMENTS_REF) )
         strcat( moment_str, moments[2] );
   
      if( (data_trans_enable & GSM_MOMENTS_VEL) ){
  
         if( strlen( moment_str ) != 0 )
            strcat( moment_str, "/" );

         strcat( moment_str, moments[3] );
  
      }
   
      if( (data_trans_enable & GSM_MOMENTS_WID) ){
   
         if( strlen( moment_str ) != 0 )
            strcat( moment_str, "/" );

         strcat( moment_str, moments[4] );
 
      }
  
   }   

   len = sprintf( Message, "   Moments (%4x):         %s",
                  gsm->data_trans_enable, moment_str );
   Print_msg( len, GRAY );

   /* Process Dual Pol enabled. */
   dual_pol_enable = gsm->data_trans_enable & DUAL_POL_MASK;

   if( dual_pol_enable )
      len = sprintf( Message, "   Dual Pol (%4x):        %s", 
                     dual_pol_enable, dual_pol[0] );

   else 
      len = sprintf( Message, "   Dual Pol (%4x):        %s", 
                     dual_pol_enable, dual_pol[1] );

   Print_msg( len, GRAY );

   /* Print calibration constant. */
   calib_const = (float) gsm->ref_calib/4.0;
   len = sprintf( Message, "   H Calib Const (%3d):   %6.2f", 
                  gsm->ref_calib, calib_const );
   Print_msg( len, GRAY );

   calib_const = (float) gsm->v_ref_calib/4.0;
   len = sprintf( Message, "   V Calib Const (%3d):   %6.2f", 
                  gsm->v_ref_calib, calib_const );
   Print_msg( len, GRAY );

   /* Determine Product Availability. */
   memset( rpg_prod_avail_str, 0, 128 );

   if( (gsm->prod_avail & GSM_PA_PROD_AVAIL) ){

      color = GREEN;
      strcat( rpg_prod_avail_str, rpg_prod_avail[0] );

   }

   if( (gsm->prod_avail & GSM_PA_DEGRADED_AVAIL) ){

      color = ORANGE;
      strcat( rpg_prod_avail_str, rpg_prod_avail[1] );

   }

   if( (gsm->prod_avail & GSM_PA_PROD_NOT_AVAIL) ){

      color = RED;
      strcat( rpg_prod_avail_str, rpg_prod_avail[2] );

   }

   len = sprintf( Message, "   Prods Available (%4x): %s", 
                  gsm->prod_avail, rpg_prod_avail_str );

   Print_msg( len, color );     

   /* Determine RPG Operability Status. */
   memset( rpg_oper_str, 0, 128 );

   color = GRAY;
   if( (gsm->rpg_op_status & GSM_RPG_OPER_LOADSHED) ){

      color = YELLOW;
      strcat( rpg_oper_str, rpg_oper[0] );

   }

   if( (gsm->rpg_op_status & GSM_RPG_OPER_ON_LINE) ){

      color = GRAY;
      strcat( rpg_oper_str, rpg_oper[1] );

   }

   if( (gsm->rpg_op_status & GSM_RPG_OPER_MR) ){

      color = YELLOW;
      strcat( rpg_oper_str, rpg_oper[2] );

   }

   if( (gsm->rpg_op_status & GSM_RPG_OPER_MM) ){

      color = ORANGE;
      strcat( rpg_oper_str, rpg_oper[3] );

   }

   if( (gsm->rpg_op_status & GSM_RPG_OPER_CMD_SHUTDOWN) ){

      color = RED;
      strcat( rpg_oper_str, rpg_oper[4] );

   }

   len = sprintf( Message, "   RPG Operability (%4x): %s", gsm->rpg_op_status, rpg_oper_str );

   Print_msg( len, color );     

   /* Determine RPG Alarms. */
   memset( rpg_alarms_str, 0, 128 );

   /* Note: Color is based on RPG Operability Status. */
   level = color;

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_NO_ALARMS) )
      strcat( rpg_alarms_str, rpg_alarms[0] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_NODE_CONNECT) )
      strcat( rpg_alarms_str, rpg_alarms[1] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_WIDEBAND_FAILRE) )
      strcat( rpg_alarms_str, rpg_alarms[2] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_RPG_CNTL) )
      strcat( rpg_alarms_str, rpg_alarms[3] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_BASEDATA_FAILURE) )
      strcat( rpg_alarms_str, rpg_alarms[4] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_SPARE26) )
      strcat( rpg_alarms_str, rpg_alarms[5] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_INPUT_BUFFER_LS) )
      strcat( rpg_alarms_str, rpg_alarms[6] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_SPARE24) )
      strcat( rpg_alarms_str, rpg_alarms[7] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_PROD_STORAGE_LS) )
      strcat( rpg_alarms_str, rpg_alarms[8] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_SPARE22) )
      strcat( rpg_alarms_str, rpg_alarms[9] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_SPARE21) )
      strcat( rpg_alarms_str, rpg_alarms[10] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_INTER_LINK) )
      strcat( rpg_alarms_str, rpg_alarms[12] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_RED_CHAN) )
      strcat( rpg_alarms_str, rpg_alarms[13] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_TSK_FAIL) )
      strcat( rpg_alarms_str, rpg_alarms[14] );

   if( (gsm->rpg_alarms & GSM_RPG_ALARMS_MEDIA_FAILURE) )
      strcat( rpg_alarms_str, rpg_alarms[15] );

   len = sprintf( Message, "   RPG Alarms (%4x):      %s",
                  gsm->rpg_alarms, rpg_alarms_str );

   Print_msg( len, level );     

   /* Determine RPG Status. */
   memset( rpg_stat_str, 0, 128 );

   if( (gsm->rpg_status & GSM_RPG_STAT_RESTART) )
      strcat( rpg_stat_str, rpg_stat[0] );

   if( (gsm->rpg_status & GSM_RPG_STAT_OPERATE) )
      strcat( rpg_stat_str, rpg_stat[1] );

   if( (gsm->rpg_status & GSM_RPG_STAT_STANDBY) )
      strcat( rpg_stat_str, rpg_stat[2] );

   len = sprintf( Message, "   RPG Status (%4x):      %s", 
                  gsm->rpg_status, rpg_stat_str );
   Print_msg( len, GRAY );     

   /* Determine NB Status. */
   memset( rpg_nb_stat_str, 0, 128 );

   color = GRAY;
   if( (gsm->rpg_nb_status & GSM_NB_STATUS_COMM_DISC) ){

      color = RED;
      strcat( rpg_nb_stat_str, rpg_nb_stat[0] );

   }

   if( (gsm->rpg_nb_status & GSM_NB_STATUS_NB_LS) ){

      color = YELLOW;
      strcat( rpg_nb_stat_str, rpg_nb_stat[1] );

   }

   len = sprintf( Message, "   NB Status (%4x):       %s", 
                  gsm->rpg_nb_status, rpg_nb_stat_str );

   Print_msg( len, color );     

   /* Display RDA Channel Number. */
   len = sprintf( Message, "   RDA Chan # (%4x):      %2d",
                  gsm->RDA_channel_num, gsm->RDA_channel_num );
   Print_msg( len, GRAY );

   /* Display RPG Build Number. */
   len = sprintf( Message, "   RPG Build # (%4x):     %4.1f",
                  gsm->build_version, (float) gsm->build_version / 10.0 );
   Print_msg( len, GRAY );

   /* Display RDA Build Number. */
   len = sprintf( Message, "   RDA Build # (%4x):     %4.1f",
                  gsm->RDA_build_num, (float) gsm->RDA_build_num / 10.0 );
   Print_msg( len, GRAY );

   /* Display VCP Supplemental Data. */
   len = sprintf( Message, "   VCP Suppl Data (%4x):     ", gsm->vcp_supp_data );
   Print_msg( len, GRAY );
   for( i = 0; i < MAX_SUPPL_BITS; i++ ){

      if( gsm->vcp_supp_data & (1 << i) ){

         len = sprintf( Message, 
                        "                          %s", vcp_supplemental[i] );
         Print_msg( len, GRAY );

      }

   }

   /* Put in blank line. */
   len = sprintf( Message, "    " );
   Print_msg( len, -1 );

/* End of Process_gen_stat_message() */
}

/**************************************************************************

   Description: Processes Request/Response Message

   Inputs:      msg_data - Request/Response Message.

**************************************************************************/
static void Process_req_resp_message( short *msg_data ){

   int len, prod_code, vol_date, vol_time;
   unsigned int error_code;
   Prod_request_response_msg_icd *rr_msg;
   char err[24];
   char *mnemonic = NULL, *date_time_str = NULL;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Add to cumulative size. */
   unsigned int size = (hdr->lengthm << 16) | (hdr->lengthl & 0xffff);
   Cum_size += size;

   len = sprintf( Message, 
      "<--- REQUEST/RESPONSE MESSAGE (Msg Code 3):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, RED );

   rr_msg = (Prod_request_response_msg_icd *) msg_data;
   prod_code = (int) rr_msg->msg_code;
   error_code = (((unsigned int) rr_msg->error_codem << 16) & 0xffff0000) + 
                ((unsigned int) rr_msg->error_codel & 0x0000ffff);

   /* Get product mnemonic. */
   mnemonic = Get_product_mnemonic_str( prod_code );

   switch( error_code ){

      case RR_NO_SUCH_MESSAGE:

         sprintf( err, "  NO SUCH MSG CODE" );
         break;

      case RR_NO_SUCH_PRODUCT:

         sprintf( err, " NO SUCH PROD CODE" );
         break;

      case RR_PRODUCT_NOT_GEN:

         sprintf( err, "PROD NOT GENERATED" );
         break;

      case RR_ONE_TIME_FAULT: 

         sprintf( err, "     OT GEN FAILED" );
         break;

      case RR_NARROWBAND_LS:  

         sprintf( err, "       NB LOADSHED" );
         break;

      case RR_ILLEGAL_REQUEST: 

         sprintf( err, "   ILLEGAL REQUEST" );
         break;

      case RR_MEMORY_LOADSHED:

         sprintf( err, "   MEMORY LOADSHED" );
         break;

      case RR_RPG_CPU_LOADSHED: 

         sprintf( err, "      CPU LOADSHED" );
         break;

      case RR_SLOT_UNAVAIL:   

         sprintf( err, "  UNAVAILABLE SLOT" );
         break;
  
      case RR_TASK_FAILURE: 

         sprintf( err, "      TASK FAILURE" );
         break;

      case RR_TASK_UNAVAIL: 

         sprintf( err, "  TASK UNAVAILABLE" );
         break;

      case RR_AVAIL_NEXT_SCAN: 

         sprintf( err, "   AVAIL NEXT SCAN" );
         break;

      case RR_DISABLED_MOMENT:

         sprintf( err, "   MOMENT DISABLED" );
         break;

      case RR_INVALID_PASSWORD: 

         sprintf( err, "  INVALID PASSWORD" );
         break;

      case RR_VOLUME_SCAN_ABORT: 

         sprintf( err, "    VOLUME ABORTED" );
         break;

      case RR_INVLD_PROD_PARAMS: 

         sprintf( err, "  INVLD PROD PARMS" );
         break;

      case RR_DATA_SEQ_ERROR: 

         sprintf( err, "    DATA SEQ ERROR" );
         break;

      case RR_TASK_TERM: 

         sprintf( err, "    TASK SELF TERM" );
         break;

      default:

         sprintf( err, "????????????? %8x", error_code );
         break;

   /* End of "switch" statement. */
   }

   /* Convert volume scan date and time. */
   vol_date = rr_msg->vol_date;
   vol_time = (rr_msg->vol_time_msw << 16) | (rr_msg->vol_time_lsw & 0xffff);
   date_time_str = Get_date_time_str( vol_date, vol_time );
   len = sprintf( Message, "   Product %3d (%3s) For Vol Date/Time %s and Elev %3d (Seq # %6d) Unavailable",
                  rr_msg->msg_code, mnemonic, date_time_str,  
                  rr_msg->elev_angle, rr_msg->seq_number );

   Print_msg( len, RED );

   len = sprintf( Message, "   Reason: %s\n", err );
   Print_msg( len, RED );

/* End of Process_req_resp_message() */
}

#define GRID_GROUP     1
#define VOLUME_GROUP   2
#define FORECAST_GROUP 3
/**************************************************************************

   Description: Processes Alert Parameter Message.

   Inputs:      msg_data - Alert Parameter Message.

**************************************************************************/
static void Process_alert_parameter_message( short *msg_data ){

   int len, num_cats, i, j;
   int first_time, group;
   short *tmp_ptr;
   char *mne = NULL;
   Prod_alert_adaptation_parameter_entry_icd *parm;
   Prod_alert_adaptation_parameter_msg_icd *parm_msg;
   char thresh[8], thresh_str[128];
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Add to cumulative size. */
   unsigned int size = (hdr->lengthm << 16) | (hdr->lengthl & 0xffff);
   Cum_size += size;

   len = sprintf( Message,
      "<--- ALERT ADAPTATION PARAMETER MESSAGE (Msg Code 6):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, BLUE );

   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   parm_msg = (Prod_alert_adaptation_parameter_msg_icd *) msg_data;
   num_cats = parm_msg->length / sizeof(Prod_alert_adaptation_parameter_entry_icd);

   /* Get pointer to start of parameter data. */
   tmp_ptr = (short *) &(parm_msg->length);

   for( group = GRID_GROUP; group <= FORECAST_GROUP; group++ ){

      first_time = 1;
      parm = (Prod_alert_adaptation_parameter_entry_icd *) (tmp_ptr+1);

      for( i = 0; i < num_cats; i++ ){

         if( parm->alert_group != group ){

            parm++;
            continue;

         }

         if( first_time ){
 
            if(  parm->alert_group == GRID_GROUP ){

               len = sprintf( Message, "   Grid Group" );
               Print_msg( len, GRAY );
               first_time = 0;
      
            }
            else if( parm->alert_group == VOLUME_GROUP ){

               len = sprintf( Message, "   Volume Group" );
               Print_msg( len, GRAY );
               first_time = 0;

            }
            else if( parm->alert_group == FORECAST_GROUP ){

               len = sprintf( Message, "   Forecast Group" );
               Print_msg( len, GRAY );
               first_time = 0;

            }

         }
         
         sprintf( thresh_str, "      Category: %3d   Threshold Values: ",
                  parm->alert_category );
         for( j = 0; j < parm->max_n_thresholds; j++ ){

            if( j == 0 )
               sprintf( thresh, "%3d", parm->threshold1 );

            if( j == 1 )
               sprintf( thresh, "%3d", parm->threshold2 );

            if( j == 2 )
               sprintf( thresh, "%3d", parm->threshold3 );

            if( j == 3 )
               sprintf( thresh, "%3d", parm->threshold4 );

            if( j == 4 )
               sprintf( thresh, "%3d", parm->threshold5 );

            if( j == 5 )
               sprintf( thresh, "%3d", parm->threshold6 );

            strcat( thresh_str, thresh );

         }

         for( j = parm->max_n_thresholds+1; j <= 6; j++ ){

            sprintf( thresh, "   " );
            strcat( thresh_str, thresh );

         }
      
         /* Get product mneumonic. */
         mne = Get_product_mnemonic_str( (int) parm->prod_code );
         len = sprintf( Message, "   %s   Paired-Product: %3d (%3s)", 
                        thresh_str, parm->prod_code, mne );
         Print_msg( len, GRAY );

         parm++;

      }
      
   }

   len = sprintf( Message, "\n" );
   Print_msg( len, -1 );
   
/* End of Process_alert_parameter_message() */
}

/**************************************************************************

   Description: Processes Alert Message.

   Inputs:      Alert Message.

**************************************************************************/
static void Process_alert_message( short *msg_data ){

   int len, thresh_value, exceed_value;
   Prod_alert_msg_icd *alert = (Prod_alert_msg_icd *) msg_data;
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Add to cumulative size. */
   unsigned int size = (hdr->lengthm << 16) | (hdr->lengthl & 0xffff);
   Cum_size += size;

   len = sprintf( Message, 
      "<--- ALERT MESSAGE (Msg Code 9):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, ORANGE );

   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   /* Print the alert status */
   if( alert->alert_status == 1 ){

      len = sprintf( Message, "   Alert Status:      First Time Alert" );
      Print_msg( len, ORANGE );

   }
   else if( alert->alert_status == 2 ){

      len = sprintf( Message, "   Alert Status:      Alert Ended" );
      Print_msg( len, ORANGE );

   }

   /* Print the alert area number. */
   len = sprintf( Message, "   Alert Area:        %1d", alert->alert_area_num );
   Print_msg( len, ORANGE );

   /* Print the alert category. */
   switch( alert->alert_category ){

      case 1:

         len = sprintf( Message, "   Alert Category:    GRID VELOCITY" );
         Print_msg( len, ORANGE );
         break;

      case 2:

         len = sprintf( Message, "   Alert Category:    GRID COMP REF" );
         Print_msg( len, ORANGE );
         break;

      case 3:

         len = sprintf( Message, "   Alert Category:    GRID ECHO TOPS" );
         Print_msg( len, ORANGE );
         break;

      case 4:

         len = sprintf( Message, "   Alert Category:    GRID SEVERE WX PROB" );
         Print_msg( len, ORANGE );
         break;

      case 5:
         break;

      case 6:

         len = sprintf( Message, "   Alert Category:    GRID VIL" );
         Print_msg( len, ORANGE );
         break;

      case 7:

         len = sprintf( Message, "   Alert Category:    VOLUME VAD" );
         Print_msg( len, ORANGE );
         break;

      case 8:

         len = sprintf( Message, "   Alert Category:    VOLUME MAX HAIL SIZE" );
         Print_msg( len, ORANGE );
         break;

      case 9:

         len = sprintf( Message, "   Alert Category:    VOLUME MESOCYCLONE" );
         Print_msg( len, ORANGE );
         break;

      case 10:

         len = sprintf( Message, "   Alert Category:    VOLUME TVS" );
         Print_msg( len, ORANGE );
         break;

      case 11:

         len = sprintf( Message, "   Alert Category:    VOLUME MAX STORM REFL" );
         Print_msg( len, ORANGE );
         break;

      case 12:

         len = sprintf( Message, "   Alert Category:    VOLUME PROB HAIL" );
         Print_msg( len, ORANGE );
         break;

      case 13:

         len = sprintf( Message, "   Alert Category:    VOLUME PROB SVR HAIL" );
         Print_msg( len, ORANGE );
         break;

      case 14:

         len = sprintf( Message, "   Alert Category:    VOLUME STORM TOP" );
         Print_msg( len, ORANGE );
         break;

      case 15:

         len = sprintf( Message, "   Alert Category:    VOLUME MAX 1 HR PRECIP" );
         Print_msg( len, ORANGE );
         break;

      case 16:

         len = sprintf( Message, "   Alert Category:    VOLUME MDA STRENGTH RANK" );
         Print_msg( len, ORANGE );
         break;

      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
      case 22:
      case 23:
      case 24:
         break;

      case 25:

         len = sprintf( Message, "   Alert Category:    FORECAST MAX HAIL SIZE" );
         Print_msg( len, ORANGE );
         break;

      case 26:

         len = sprintf( Message, "   Alert Category:    FORECAST MESOCYCLONE" );
         Print_msg( len, ORANGE );
         break;

      case 27:

         len = sprintf( Message, "   Alert Category:    FORECAST TVS" );
         Print_msg( len, ORANGE );
         break;

      case 28:

         len = sprintf( Message, "   Alert Category:    FORECAST MAX STORM REFL" );
         Print_msg( len, ORANGE );
         break;

      case 29:

         len = sprintf( Message, "   Alert Category:    FORECAST PROB HAIL" );
         Print_msg( len, ORANGE );
         break;

      case 30:

         len = sprintf( Message, "   Alert Category:    FORECAST PROB SVR HAIL" );
         Print_msg( len, ORANGE );
         break;

      case 31:

         len = sprintf( Message, "   Alert Category:    FORECAST STORM TOP" );
         Print_msg( len, ORANGE );
         break;

      case 32:

         len = sprintf( Message, "   Alert Category:    FORECAST MDA STRENGTH RANK" );
         Print_msg( len, ORANGE );
         break;

      default:
         break;
      
   }
   /* End of "switch" statement. */

   /* Print the alert thresold code. */
   len = sprintf( Message, "   Alert Code:        %2d", alert->thd_code );
   Print_msg( len, ORANGE );

   /* Print threshold value. */
   thresh_value = ((alert->thd_valuem << 16) & 0xffff0000) + 
                   (alert->thd_valuel & 0x0000ffff);
   len = sprintf( Message, "   Alert Thresh:      %4d", thresh_value );
   Print_msg( len, ORANGE );

   /* Print exceeding value. */
   exceed_value = ((alert->exd_valuem << 16) & 0xffff0000) + 
                   (alert->exd_valuel & 0x0000ffff);
   len = sprintf( Message, "   Exceeding Value:   %4d", exceed_value );
   Print_msg( len, ORANGE );

   /* Print the alert box azimuth. */
   len = sprintf( Message, "   Alert Box Azm:     %5.1f", (float) alert->azim/10.0 );
   Print_msg( len, ORANGE );

   /* Print the alert box range. */
   len = sprintf( Message, "   Alert Box Rng:     %5.1f", (float) alert->range/10.0 );

   Print_msg( len, ORANGE );

   /* Print the strom ID. */
   len = sprintf( Message, "   Storm ID:          %c%c", alert->c1, alert->c2 );
   Print_msg( len, ORANGE );

   /* Print the strom ID. */
   len = sprintf( Message, "   Volume Scan #:     %3d", alert->vol_num );
   Print_msg( len, ORANGE );
   
   /* Convert volume scan date and time. */
   {
      int vol_date = alert->vol_date;
      int vol_time = ((alert->vol_timem << 16) & 0xffff0000) + 
                     (alert->vol_timel & 0x0000ffff);
      char *date_time_str = NULL;

      date_time_str = Get_date_time_str( vol_date, vol_time );
      len = sprintf( Message, "   Vol Date/Time:     %s\n", date_time_str );
      Print_msg( len, ORANGE );

   }

/* End of Process_alert_message() */
}

/**************************************************************************

   Description: Process Product List Message.

   Inputs:      Product List Message.

**************************************************************************/
static void Process_product_list_message( short *msg_data ){

   int i, len, num_prods;
   char *mnemonic = NULL;
   Prod_list_msg_icd *pl = (Prod_list_msg_icd *) msg_data;
   Prod_list_entry_icd *pl_entry = (Prod_list_entry_icd *) (msg_data + 
                        sizeof(Prod_list_msg_icd) / sizeof(short));
   Prod_msg_header_icd *hdr = (Prod_msg_header_icd *) msg_data;

   /* Add to cumulative size. */
   unsigned int size = (hdr->lengthm << 16) | (hdr->lengthl & 0xffff);
   Cum_size += size;

   len = sprintf( Message, 
      "<--- PRODUCT LIST (Msg Code 8):   From ID: %04d, To ID: %04d",
      hdr->src_id, hdr->dest_id );
   Print_msg( len, -1 );

   if( Terse_mode ){

      fprintf( stdout, "\n" );
      return;

   }

   num_prods = pl->num_products;

   if( num_prods > 0 ){

      len = sprintf( Message, 
                     "    code  mne   elev      p1      p2      p3      p4   dist" );
      Print_msg( len, -1 );

      for( i = 0; i < num_prods; i++ ){

         mnemonic = Get_product_mnemonic_str( (int) pl_entry->prod_id );

         len = sprintf( Message, "    %3d   %3s    %3d  %6d  %6d  %6d  %6d   %3d",
                        pl_entry->prod_id, mnemonic, pl_entry->elevation,
                        pl_entry->params[0], pl_entry->params[1],  
                        pl_entry->params[2], pl_entry->params[3],
                        pl_entry->distribution_class );

         Print_msg( len, -1 );

         pl_entry++;

      }

   }

   sprintf( Message, "\n" );
   Print_msg( len, -1 );

/* End of Process_product_list_message() */
}
