/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/14 21:15:46 $
 * $Id: crda_rda_control.c,v 1.48 2014/08/14 21:15:46 steves Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <crda_control_rda.h>
#include <time.h>

/* 
  File scope global variables.
*/
/*
  Default rda control message.
*/
static short Default_rda_control[] = { 	0, 	/* RDA State: HW 1 (NO CHANGE) */
					0, 	/* Data Enable: HW 2  (NO CHANGE) */
					0,	/* Aux Pwr/Gen: HW 3 (NO CHANGE) */
					0, 	/* Authorization: HW 4 (NO CHANGE) */
					0,	/* Restart Elev: HW 5 (UNDEFINED) */
					32767,	/* Select VCP: HW 6 (NO CHANGE) */
					0,	/* HW 7 (Spare) */
					0, 	/* Super Res: HW 8 (NO CHANGE) */
					0, 	/* CMD: HW 9 (NO CHANGE) */
					0, 	/* AVSET: HW 10 (NO CHANGE) */
					0, 	/* Oper Mode: HW 11 (NO CHANGE) */
					0,	/* Channel Control: HW 12 (NO CHANGE) */ 
					0, 	/* Performance Check (NO CHANGE) */
					0,	/* HW 14 (Spare) */ 
					0,	/* HW 15 (Spare) */ 
					0,	/* HW 16 (Spare) */ 
					0,	/* HW 17 (Spare) */ 
					0,	/* HW 18 (Spare) */ 
					0, 	/* HW 19 (Spare) */
					0, 	/* HW 20 (Spare) */
					0, 	/* Sport Blanking: HW 21 (NO CHANGE) */
					0,	/* HW 22 (Spare) */ 
					0,	/* HW 23 (Spare) */ 
					0,	/* HW 24 (Spare) */ 
					0,	/* HW 25 (Spare) */ 
					0,	/* HW 26 (Spare) */ };

/*
  For RMS Base Data Enable System Status Log message processing.
*/
#define NONE       RCOM_BDENABLEN
#define REF        RCOM_BDENABLER
#define VEL        RCOM_BDENABLEV
#define RV         ( RCOM_BDENABLER | RCOM_BDENABLEV )
#define WID        RCOM_BDENABLEW
#define RW         ( RCOM_BDENABLER | RCOM_BDENABLEW )
#define VW         ( RCOM_BDENABLEV | RCOM_BDENABLEW )
#define ALL        RCOM_BDENABLE

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Builds the ORDA Control Command message according to the RDA/RPG 
//      ICD format.  The command and parameters for the control command
//      are passed as input. 
//
//   Inputs:    
//      rda_command - pointer to command buffer.
//      message_data - pointer to pointer where ORDA Control Comamnd 
//                     message data is to be stored.
//      message_size - pointer where size of ORDA Control Command message,
//                     in shorts, is to be stored.
//
//   Outputs:
//      message_data - pointer to pointer where ORDA Control Comamnd 
//                     message data is stored.
//      message_size - pointer where size of ORDA Control Command message,
//                     in shorts, is stored.
//
//   Returns:    
//      RC_FAILURE on error, RC_SUCCESS otherwise.
//
//   Globals:
//      CR_control_status - see crda_control_rda.h
//      CR_operational_mode - see crda_control_rda.h
//
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
int RC_orda_control( Rda_cmd_t *rda_command, short **message_data,
                    int *message_size ){

   ORDA_control_commands_t *rda_control_message;
   int elevind, command_accepted;
 
   /* Allocate buffer for ORDA Control message. */
   if( (rda_control_message = (ORDA_control_commands_t *) calloc( (size_t) 1, 
      (size_t) sizeof(ORDA_control_commands_t) )) == NULL )
   {
      LE_send_msg( GL_MEMORY, "Rda Control Command calloc Failed\n" );
      return ( RC_FAILURE );
   }

   /* Copy default control data to the ORDA message buffer. */
   memcpy( rda_control_message, Default_rda_control, 
           (size_t) sizeof(ORDA_control_commands_t) );

   /* Add the control command. */
   command_accepted = 1;

   switch( rda_command->param1 )
   {
      case CRDA_STANDBY:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->state = RCOM_STANDBY;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A RDA To STANDBY Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_OFFOPER:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->state = RCOM_OFFOPER;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A RDA To OFF-LINE OPERATE Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      } 

      case CRDA_OPERATE:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->state = RCOM_OPERATE;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A RDA To OPERATE Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_RESTART:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->state = RCOM_RESTART;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A RDA To RESTART Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_BDENABLE:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->data_enbl = rda_command->param2;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD ) 
            {
               if( rda_control_message->data_enbl == NONE )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
                     "RMS Issued a BASE DATA DISABLE ALL MOMENTS Command\n" );
               else if( rda_control_message->data_enbl == REF )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /R/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == VEL )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /V/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == WID )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /W/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == RV )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /R/V/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == RW )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /R/W/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == VW )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                               "RMS Issued a BASE DATA /V/W/ ENABLE Command\n" );
               else if( rda_control_message->data_enbl == ALL )
                  LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
                               "RMS Issued a BASE DATA ENABLE ALL MOMENTS Command\n" );
            }
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_AUXGEN:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->aux_pwr_gen= RCOM_AUXGEN;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A SWITCH TO AUX GEN Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_UTIL:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->aux_pwr_gen = RCOM_UTIL;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A SWITCH TO UTIL PWR Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_REQREMOTE:
      {
         rda_control_message->authorization = RCOM_REQREMOTE;
         if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "RMS Issued A SELECT REMOTE CONTROL Command\n" );

         break;
      }

      case CRDA_ACCREMOTE:
      {
         rda_control_message->authorization = RCOM_ACCREMOTE;
         break;
      }

      case CRDA_ENALOCAL:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->authorization = RCOM_ENALOCAL;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued AN ENABLE LOCAL CONTROL Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_RESTART_VCP:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->restart_elev = RCOM_RESTART_VCP;
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "Commanding RDA to Restart Volume Scan\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_RESTART_ELEV:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            if( rda_command->param2 == 0 )
                elevind = RW_get_current_elev_index();
            else
                elevind = rda_command->param2; 

            rda_control_message->restart_elev = RCOM_RESTART_ELEV + elevind;
       
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                         "Commanding RDA to Restart RDA Elevation Cut %d\n",
                         elevind );

         }
         else
         {
            command_accepted = 0;
         } 

         break;
      }

      case CRDA_SELECT_VCP:
      {
         if( CR_control_status != CS_LOCAL_ONLY ){

            rda_control_message->select_vcp = rda_command->param2;

            /* A VCP number of 0 refers to the local VCP which should
               have been previously downloaded. */
            if( rda_control_message->select_vcp != 0 )
               CR_last_commanded_vcp = rda_control_message->select_vcp;

         }
         else
            command_accepted = 0;

         break;
      }

      case CRDA_SR_ENAB:
      {
         rda_control_message->super_res = RCOM_ENABLE_SR;

         break;
      }

      case CRDA_SR_DISAB:
      {
         rda_control_message->super_res = RCOM_DISABLE_SR;

         break;
      }

      case CRDA_CMD_ENAB:
      {
         rda_control_message->cmd = RCOM_ENABLE_CMD;

         break;
      }

      case CRDA_CMD_DISAB:
      {
         rda_control_message->cmd = RCOM_DISABLE_CMD;

         break;
      }

      case CRDA_AVSET_ENAB:
      {
         rda_control_message->avset = RCOM_ENABLE_AVSET;

         break;
      }

      case CRDA_AVSET_DISAB:
      {
         rda_control_message->avset = RCOM_DISABLE_AVSET;

         break;
      }

      case CRDA_SB_ENAB:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->spot_blanking = RCOM_SB_ENAB;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A SPOT BLANKING ENABLE Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_SB_DIS:
      {
         if( CR_control_status != CS_LOCAL_ONLY ) 
         {
            rda_control_message->spot_blanking = RCOM_SB_DIS;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A SPOT BLANKING DISABLE Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_CHAN_CTL:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->channel = RCOM_CHAN_CONTRL;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A CHANNEL CONTROLLING Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_CHAN_NONCTL:
      {
         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->channel = RCOM_CHAN_NONCONTRL;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A CHANNEL NON-CONTROLLING Command\n" );
         }
         else
         {
            command_accepted = 0;
         }

         break;
      }

      case CRDA_PERF_CHECK:
      {

         if( CR_control_status != CS_LOCAL_ONLY )
         {
            rda_control_message->perf_check = RCOM_PERF_CHECK_PPC;
            if( rda_command->line_num == RMS_INITIATED_RDA_CTRL_CMD )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                            "RMS Issued A Performance Check Command\n" );

            /* Write out message to status log indicating the Perf Check 
               was commanded. */
            LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                         "RDA CAL: Performance Check Commanded by RPG\n" );

         }
         else
         {
            command_accepted = 0;
         }

         break;

      }
      default:
      {
         LE_send_msg( GL_INFO, "Unknown Or Unsupported RDA Control Command (%d)\n",
                      rda_command->param1 );
         command_accepted = 0;
         break;
      }
   } /* End of "switch" */

   /* 
     Assign pointer and size to message_data and message_size, 
     respectively.
   */
   if( command_accepted )
   {
      *message_data = (short*) rda_control_message;
      *message_size = sizeof(ORDA_control_commands_t)/sizeof(short);  /* In shorts. */
   }
   else
   {
      free( rda_control_message );
      rda_control_message = (ORDA_control_commands_t *) NULL;
      LE_send_msg( GL_CODE, "Invalid RDA Control Command: %d %d %d %d %d\n",
         rda_command->cmd,
         rda_command->line_num,
         rda_command->param1,
         rda_command->param2,
         rda_command->param3 );
      return ( RC_FAILURE );
   }

   return ( RC_SUCCESS );

} /* End of RC_orda_control() */
