/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:58:03 $
 * $Id: crdat.c,v 1.22 2014/03/18 18:58:03 jeffs Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

/*******************************************************************

	Main module for RDA Control test program

*******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <lb.h>
#include <orpgdat.h>
#include <gen_stat_msg.h>
#include <en.h>
#include <orpgevt.h>
#include <basedata.h>
#include <crda_control_rda.h>
#include <rda_control.h>
#include <orpgrda.h>

#define FROM_HCI		HCI_INITIATED_RDA_CTRL_CMD
#define FROM_RMS		RMS_INITIATED_RDA_CTRL_CMD

static int From = FROM_HCI;

enum{ STATE, DATA_ENABLE, AUX_PWR_GEN, AUTHORIZATION, RESTART_ELEVATION,
      SELECT_VCP, AUTO_CALIB, SPARE8, SPARE9, INTERFERENCE, OPERATE_MODE,
      CHANNEL, ARCHIVE_II, ARCHIVE_VOLS };

/*
  Local functions.
*/
static void Request_for_data_menu( int *data_type, int *process_request );
static int Rda_control_menu( Rda_cmd_t *rda_command );
static void List_rda_status_data( ORDA_status_msg_t *rda_status );
static int Vcp_download_menu( int *vcp_num );
static int Process_rda_command( Rda_cmd_t *rda_command );
static int Send_clutter_sensor_zones_menu( int *parameter_1,
                                           int *parameter_2 );
static void Change_vel_reso();
/****************************************************************

   Description:
      The main function for the Control RDA Tool.

*****************************************************************/
int main (int argc, char **argv){
   
   int reply, command_rda = 0, process_request; 
   int select_ret;

   Rda_cmd_t rda_command;

   fd_set rset;
   struct timeval tvptr;

   /* 
     Set up select so standard input will block for specified
     period of time.
   */
   FD_ZERO( &rset );
   FD_SET( 0, &rset );

   /*
     Set timeout value.
   */
   tvptr.tv_sec = 0;
   tvptr.tv_usec = 100000;

   /* 
     To prevent annoying error message. 
   */
   ORPGMISC_init( argc, argv, 100, 0, -1, 0 );

   /*
     Operation Menu.
   */
   fprintf( stderr, "\n\n" );
   fprintf( stderr, "Selection Menu\n\n" );
   fprintf( stderr, " 0 - Exit\n" );
   fprintf( stderr, " 1 - Process Line Connection\n" );
   fprintf( stderr, " 2 - Process Line Disconnection\n" );
   fprintf( stderr, " 3 - Request For Data\n" );
   fprintf( stderr, " 4 - Download VCP\n" );
   fprintf( stderr, " 5 - Control RDA\n" );
   fprintf( stderr, " 6 - Send RPG/RDA Loopback Message\n" );
   fprintf( stderr, " 7 - Send Clutter Sensor Zones FIle\n" );
   fprintf( stderr, " 8 - Send Clutter Bypass Map\n" );
   fprintf( stderr, " 9 - List RDA Status Data\n" );
   fprintf( stderr, "10 - From Toggle (HCI or RMS, default HCI)\n" );
   fprintf( stderr, "11 - Change VCP Velocity Resolution\n" );

   while(1){

      ORDA_status_t rda_status;
      int status_available = 0;

      /*
        Call select to see if standard input is ready for reading.
      */
      if( (select_ret = select( 1, &rset, NULL, NULL, &tvptr)) == -1 ){

         fprintf( stderr, "SELECT function call returned error\n" ); 
         exit(3);
      }

      if( select_ret > 0 && FD_ISSET(0, &rset) ){
  
         /* 
           Input ready to read from operator.
         */
         scanf( "%d", &reply );

         switch( reply ){

            case 0:
               exit(0);

            case 1:
            {
               rda_command.cmd = COM4_WBENABLE;
               rda_command.line_num = From;
               Process_rda_command( &rda_command );
               break;
            }   

            case 2:
            {
               rda_command.cmd = COM4_WBDISABLE;
               rda_command.line_num = From;
               Process_rda_command( &rda_command );
               break;
            }   

            case 3:
            {
               /*
                 Set the process_request flag.
               */
               process_request = 1;

               rda_command.cmd = COM4_REQRDADATA;
               Request_for_data_menu( &rda_command.param1, 
                                      &process_request );
               if( process_request )
                  Process_rda_command( &rda_command );

               break;
            }

            case 4:
            {
               int vcp_num = 0;

               if( Vcp_download_menu( &vcp_num ) < 0 ){

                  /*
                    An error occurred or an invalid selection.
                  */
                  process_request = 0;
                   
               }
               else{
 
                  rda_command.cmd = COM4_DLOADVCP;
                  rda_command.param1 = vcp_num;
                  rda_command.param2 = 0;
                  command_rda = SELECT_VCP;

               }

               Process_rda_command( &rda_command );
  
               /*
                 Is this message to be followed by a RDA Control 
                 command?
               */
               if( command_rda == SELECT_VCP ){

                  rda_command.cmd = COM4_RDACOM;
                  rda_command.param1 = CRDA_SELECT_VCP;
                  rda_command.param2 = RCOM_USE_REMOTE_PATTERN;
                  Process_rda_command( &rda_command );

                  /*
                    Reset command_rda and command_rda_option.
                  */
                  command_rda = -1;

               }
                  
               break;  

            }

            case 5:
            {
               rda_command.cmd = COM4_RDACOM;
               rda_command.line_num = From;
               process_request = Rda_control_menu( &rda_command );
               if( process_request )
                  Process_rda_command( &rda_command );
               break;
            }  

            case 6:
            {
               rda_command.cmd = COM4_LBTEST;
               rda_command.line_num = From;
               Process_rda_command( &rda_command );
               break;
            }

            case 7:
            {
               rda_command.cmd = COM4_SENDCLCZ;
               rda_command.line_num = From;
               process_request = 
                  Send_clutter_sensor_zones_menu( &rda_command.param1,
                                                  &rda_command.param2 );
               if( process_request )
                  Process_rda_command( &rda_command );
               break;
            }

            case 8:
            {
               rda_command.cmd = COM4_SENDEDCLBY;
               rda_command.line_num = From;
               Process_rda_command( &rda_command );
               break;
            }
               
            case 9:
            {
               /*
                 Write status to RDA STATUS linear buffer.
               */
               int ret, lbfd;

               /*
                 Find the file descriptor for RDA Status Linear Buffer.
               */
               lbfd = ORPGDA_lbfd( ORPGDAT_RDA_STATUS );
               if( lbfd >= 0 ){


                  ret = ORPGDA_read( ORPGDAT_RDA_STATUS, (char *) &rda_status,
                                     sizeof( ORDA_status_t ), RDA_STATUS_ID );
  
                  if( ret < 0 && ret != LB_TO_COME ){

                     fprintf( stderr,
                              "RDA Status Read Failed! (return code: %d)\n",
                              ret );
                     status_available = 0;

                  }
    
                  else if( ret == LB_TO_COME ){

                     if( status_available == 0 )
                        fprintf( stderr, "Status Data is Not Currently Available!\n" );
               
                  }

                  else
                     status_available = 1;

               }

               else{
 
                  fprintf( stderr, "ORPGDA_lb2f failed.  Ret = %d\n", lbfd );
                  status_available = 0;

               }

               if( status_available ) List_rda_status_data( &rda_status.status_msg );
               break;

            }
  
            case 10:
            {

               if( From == FROM_HCI ){

                  From = FROM_RMS;
                  fprintf( stderr, "Command Sent From RMS\n" );

               }
               else{

                  From = FROM_HCI;
                  fprintf( stderr, "Command Sent From HCI\n" );

               }

               break;

            }

            case 11:
            {

               Change_vel_reso();
               break;

            }

            default:
            {
               fprintf( stderr, "Invalid selection. Try again.\n" );
               break;
            }

         } 


         fprintf( stderr, "\n\n" );
         fprintf( stderr, "Selection Menu\n\n" );
         fprintf( stderr, " 0 - Exit\n" );
         fprintf( stderr, " 1 - Process Line Connection\n" );
         fprintf( stderr, " 2 - Process Line Disconnection\n" );
         fprintf( stderr, " 3 - Request For Data\n" );
         fprintf( stderr, " 4 - Download VCP\n" );
         fprintf( stderr, " 5 - Control RDA\n" );
         fprintf( stderr, " 6 - Send RPG/RDA Loopback Message\n" );
         fprintf( stderr, " 7 - Send Clutter Sensor Zones File\n" );
         fprintf( stderr, " 8 - Send Clutter Bypass Map\n" );
         fprintf( stderr, " 9 - List RDA Status Data\n" );
         fprintf( stderr, "10 - From Toggle (HCI or RMS, default HCI)\n" );
         fprintf( stderr, "11 - Change VCP Velocity Resolution\n" );
    
      }
      
      /*
        Set rset for next select call.
      */
      FD_SET( 0, &rset );
         
   }        
   
}

/******************************************************************

   Description:
      Changes the velocity resolution in the VCP

   Returns:
      There is no return value for this function.

******************************************************************/
static void Change_vel_reso(){

   int reply;
   Rda_cmd_t rda_command;

   fprintf( stderr, "\n\n" );
   fprintf( stderr, "Selection Menu\n\n" );
   fprintf( stderr, "0 - Exit\n" );
   fprintf( stderr, "1 - Velocity Resolution 0.5 m/s\n" );
   fprintf( stderr, "2 - Velocity Resolution 1.0 m/s\n" );

   scanf( "%d", &reply );

   if( reply == 0 )
      return;

   rda_command.cmd = COM4_VEL_RESO;

   if( reply == 1 )
      rda_command.param1 = CRDA_VEL_RESO_HIGH;

   else if( reply == 2 )
      rda_command.param1 = CRDA_VEL_RESO_LOW;

   else
      return;

   Process_rda_command( &rda_command );

}

/******************************************************************
   
   Description:
      Displays the Request for Data menu and services the request.

   Output:
      data_type - The type of data to be requested.
      process_request - Whether the request for data should be 
                        processed.

   Returns:
      There is no return value for this function.

******************************************************************/   
static void Request_for_data_menu( int *data_type, int *process_request ){

   int reply;

   fprintf( stderr, "\n\n" );
   fprintf( stderr, "Selection Menu\n\n" );
   fprintf( stderr, "0 - Exit\n" );
   fprintf( stderr, "1 - Request Status Data\n" );
   fprintf( stderr, "2 - Request Performance Data\n" );
   fprintf( stderr, "3 - Request Bypass Map Data\n" );
   fprintf( stderr, "4 - Request Clutter Filter Map Data\n" );

   scanf( "%d", &reply );

   if( reply == 0 ){

      *process_request = 0;
      return;

   }

   else if ( reply == 1 )
      *data_type = DREQ_STATUS;

   else if ( reply == 2 )
      *data_type = DREQ_PERFMAINT;

   else if ( reply == 3 )
      *data_type = DREQ_CLUTMAP;

   else if ( reply == 4 )
      *data_type = DREQ_NOTCHMAP;

}

/************************************************************************

   Description:
      Processes sending clutter sensor zone file.

   Outputs:
      number_zones - Number of zones in the file.
      file_number - File number of censor zones.

   Returns:
      Returns 0 on success or -1 on failure.

************************************************************************/
static int Send_clutter_sensor_zones_menu( int *number_zones, 
                                           int *file_number ){

   int reply;

   fprintf( stderr, "\n\n" );
   fprintf( stderr, "Selection Menu\n\n" );
   fprintf( stderr, "0 - Exit\n" );
   fprintf( stderr, "1 - Send Clutter Sensor Zone File\n" );

   scanf( "%d", &reply );

   if( reply == 0 ){

      return (0);

   }

   else{

      fprintf( stderr, "\n" );
      fprintf( stderr, "Enter the Sensor Zone File Number\n" );
      scanf( "%d", file_number );

      fprintf( stderr, "Enter the Number of Sensor Zones in File\n" );
      scanf( "%d", number_zones );
  
      return(1);

   }

}

/*********************************************************************

   Description:
      Process RDA Control commands.

   Outputs:
      rda_command - RDA Control command.

   Returns:
      0 on success, -1 on error. 

*********************************************************************/
static int Rda_control_menu( Rda_cmd_t *rda_command ){

   int status, reply;
   char moments[20];
   unsigned char flag;

   /*
     Display RDA control command selection menu.
   */
   fprintf( stderr, "Selection Menu\n" );
   fprintf( stderr, " 0 - Exit\n" );
   fprintf( stderr, " 1 - State Change\n" );
   fprintf( stderr, " 2 - Data Enable\n" );
   fprintf( stderr, " 3 - AUX Power\n" );
   fprintf( stderr, " 4 - Control/Authorization\n" );
   fprintf( stderr, " 5 - Elevation Restart\n" );
   fprintf( stderr, " 6 - Select VCP\n" );
   fprintf( stderr, " 7 - CMD Enable/Disable\n" );
   fprintf( stderr, " 8 - AVSET Enable/Disable\n" );
   fprintf( stderr, " 9 - Super Res Enable/Disable\n" );
   fprintf( stderr, "10 - Spot Blanking Enable/Disable\n" );
   fprintf( stderr, "11 - Perform Performance Check\n" );

   scanf( "%d", &reply );

   switch( reply ){


      case 0:
         return (reply);

      case 1: {
         
         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Stand-by\n" );
         fprintf( stderr, "2 - Operate\n" );
         fprintf( stderr, "3 - Restart\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if ( reply == 1 )
            rda_command->param1 = CRDA_STANDBY;
     
         else if ( reply == 2 )
            rda_command->param1 = CRDA_OPERATE;

         else if ( reply == 3 )
            rda_command->param1 = CRDA_RESTART;

         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;

      }

      case 2: {

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "R/V/W/A/N\n" );
 
         scanf( "%s", moments );

         rda_command->line_num = From;
         rda_command->param1 = CRDA_BDENABLE;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if( strchr( moments, 'A' ) != NULL ){

            rda_command->param2 =
                         RCOM_BDENABLER | RCOM_BDENABLEV | RCOM_BDENABLEW;

         }
         else if( strchr( moments, 'N' ) != NULL ){

            rda_command->param2 = RCOM_BDENABLEN;

         }
         else{ 

            if( strchr( moments, 'R' ) != NULL )
               rda_command->param2 |= RCOM_BDENABLER;

            if( strchr( moments, 'V' ) != NULL )
               rda_command->param2 |= RCOM_BDENABLEV;

            if( strchr( moments, 'W' ) != NULL )
               rda_command->param2 |= RCOM_BDENABLEW;

         }

         break;

      }

      case 3:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Switch to Auxilliary Power\n" );
         fprintf( stderr, "2 - Switch to Utility Power\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if ( reply == 1 )
            rda_command->param1 = CRDA_AUXGEN;
     
         else if ( reply == 2 )
            rda_command->param1 = CRDA_UTIL;

         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;

      }

      case 4:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Control Command Clear\n" );
         fprintf( stderr, "2 - Local Control Enabled\n" );
         fprintf( stderr, "3 - Remote Control Accepted\n" );
         fprintf( stderr, "4 - Remote Control Requested\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if ( reply == 1 )
            rda_command->param1 = 2;
     
         else if ( reply == 2 )
            rda_command->param1 = CRDA_ENALOCAL;
     
         else if ( reply == 3 )
            rda_command->param1 = CRDA_ACCREMOTE;
     
         else if ( reply == 4 )
            rda_command->param1 = CRDA_REQREMOTE;

         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;

      }

      case 5:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - VCP Restart\n" );
         fprintf( stderr, "2 - Elevation Restart\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if ( reply == 1 )
            rda_command->param1 = CRDA_RESTART_VCP;
     
         else if ( reply == 2 )
            rda_command->param1 = CRDA_RESTART_ELEV;

         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;
     
      }

      case 6:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Use Remote Pattern\n" );
         fprintf( stderr, "2 - Select Pattern\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param1 = CRDA_SELECT_VCP;
         rda_command->param3 = 0;


         if ( reply == 1 )
            rda_command->param2 = 0;
     
         else if ( reply == 2 ){

            fprintf( stderr, "Enter Volume Coverage Pattern Number\n" );

            scanf( "%d", &reply );

            if( reply == 11 || reply == 21 || 
                reply == 31 || reply == 32 ) 
               rda_command->param2 = reply;

            else{

               fprintf( stderr, "Invalid VCP Number!\n" );
               return(0);;

            }

         }
         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;
     
      }

      case 7:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Enable CMD\n" );
         fprintf( stderr, "2 - Disable CMD\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         if( reply == 1 ){

            rda_command->line_num = From;
            rda_command->param1 = CRDA_CMD_ENAB;
            rda_command->param2 = 0;
            rda_command->param3 = 0;

         }
         else if( reply == 2 ){

            rda_command->line_num = From;
            rda_command->param1 = CRDA_CMD_DISAB;
            rda_command->param2 = 0;
            rda_command->param3 = 0;

         }
         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         /* This is required .... If the state is not set, then control_rda
            will attempt to set to the previous state. */
         if( rda_command->param1 == CRDA_CMD_ENAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                            ORPGINFO_STATEFL_SET,
                                            &flag );

         else if( rda_command->param1 == CRDA_CMD_DISAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                            ORPGINFO_STATEFL_CLR,
                                            &flag );

         break;
     
      }

      case 8:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - AVSET Enable\n" );
         fprintf( stderr, "2 - AVSET Disable\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if( reply == 1 )
            rda_command->param1 = CRDA_AVSET_ENAB;

         else if( reply == 2 )
            rda_command->param1 = CRDA_AVSET_DISAB;
         
         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         /* This is required .... If the state is not set, then control_rda
            will attempt to set to the previous state. */
         if( rda_command->param1 == CRDA_AVSET_ENAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                            ORPGINFO_STATEFL_SET,
                                            &flag );

         else if( rda_command->param1 == CRDA_AVSET_DISAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                            ORPGINFO_STATEFL_CLR,
                                            &flag );

         break;
     
      }

      case 9:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Super Res Enable\n" );
         fprintf( stderr, "2 - Super Res Disable\n" );
 
         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if( reply == 1 )
            rda_command->param1 = CRDA_SR_ENAB;

         else if( reply == 2 )
            rda_command->param1 = CRDA_SR_DISAB;
         
         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         /* This is required .... If the state is not set, then control_rda
            will attempt to set to the previous state. */
         if( rda_command->param1 == CRDA_SR_ENAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                            ORPGINFO_STATEFL_SET,
                                            &flag );

         else if( rda_command->param1 == CRDA_SR_DISAB )
            status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                            ORPGINFO_STATEFL_CLR,
                                            &flag );


         break;
     
      }

      case 10:{

         fprintf( stderr, "Select from the following\n" );
         fprintf( stderr, "0 - Exit\n" );
         fprintf( stderr, "1 - Spot Blanking Enable\n" );
         fprintf( stderr, "2 - Spot Blanking Disable\n" );

         scanf( "%d", &reply );

         if ( reply == 0 ) return (reply);

         rda_command->line_num = From;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         if( reply == 1 )
            rda_command->param1 = CRDA_SB_ENAB;

         else if( reply == 2 )
            rda_command->param1 = CRDA_SB_DIS;

         else{

            fprintf( stderr, "Invalid Selection!\n" );
            return(0);

         }

         break;

      }

      case 11: {

         rda_command->line_num = From;
         rda_command->param1 = CRDA_PERF_CHECK;
         rda_command->param2 = 0;
         rda_command->param3 = 0;

         break;

      }
      default:{

         fprintf( stderr, "Invalid Selection!\n" );
         return(0);
      
      }

   }

   return( reply );

}

/******************************************************************

   Description:
      Lists the contents of the most current RDA Status.

   Inputs:
      rda_status - RDA Status message.

   Returns:
      There is no return value defined for this function.

*****************************************************************/
static void List_rda_status_data( ORDA_status_msg_t *rda_status ){

   int i,j, k;
   short auto_cal_disabled, rda_operability;
   char moment_string[10];

   static char *status[] = { "Start-Up        ",
                             "Standby         ",
                             "Restart         ",
                             "Operate         ",
                             "Playback        ",
                             "Off-Line Operate" };

   static char *operability[] = { "On-Line              ",
                                  "Maintenance Required ",
                                  "Maintenance Mandatory", 
                                  "Commanded Shutdown   ",
                                  "Inoperable           ",
                                  "Auto-Cal Disabled    " };

   static char *control[] = { "Local ",
                              "Remote",
                              "Either" };

   static char *aux_pwr[] = { "Yes",
                              "No ",
                              "On ",
                              "Off" };

   static char *moments[] = { "None",
                              "All ",
                              "R",
                              "V",
                              "W" };

   /*
     Output status data is readable format.
   */

   /* 
     Process status.
   */
   i = 0;
   if( (rda_status->rda_status & 0x02) )
       i = 0;

   if( (rda_status->rda_status & 0x04) )
       i = 1;

   if( (rda_status->rda_status & 0x08) )
       i = 2;

   if( (rda_status->rda_status & 0x10) )
       i = 3;

   if( (rda_status->rda_status & 0x20) )
       i = 4;

   if( (rda_status->rda_status & 0x40) )
       i = 5;

   /*
     Process operability status.
   */
   auto_cal_disabled = rda_status->op_status & 0x01;
   rda_operability = rda_status->op_status & 0xfffe;

   j = 0;
   if( (rda_operability & 0x02) )
      j = 0;

   if( (rda_operability & 0x04) )
      j = 1;

   if( (rda_operability & 0x08) )
      j = 2;

   if( (rda_operability & 0x10) )
      j = 3;

   if( (rda_operability & 0x20) )
      j = 4;

   fprintf( stderr,
            "STATUS:  %s    OPERABILITY:   %s\n",
            status[i], operability[j] );            
   /*
     Process Control Status.
   */

   if( (rda_status->control_status & 0x02) )
      i = 0;

   if( (rda_status->control_status & 0x04) )
      i = 1;

   if( (rda_status->control_status & 0x08) )
      i = 2;

   /*
     Process Generator/Auxilliary Power Status.
   */
   if( (rda_status->aux_pwr_state & 0x04) )
      j = 2;

   else 
      j = 3;

   if( (rda_status->aux_pwr_state & 0x02) )
      k = 0;

   else 
      k = 1;
  
   fprintf( stderr, 
            "CONTROL: %s              UTILITY AVAIL: %s                    GEN:      %s\n",
            control[i], aux_pwr[k], aux_pwr[j] );       

   /*
     Process VCP, average power, and calibration correction.
   */
   fprintf( stderr, 
            "VCP #:   %-4d                AVE PWR:       %-4d                   CALIB: %6.2f,%6.2f\n",
            rda_status->vcp_num, 
            rda_status->ave_trans_pwr, 
            rda_status->ref_calib_corr/4.0,
            rda_status->vc_ref_calib_corr/4.0 );

   /*
     Process Moments.
   */
   moment_string[0] = '\0';
   if( rda_status->data_trans_enbld == 2 )
      strcat( moment_string, moments[0] );

   else if( rda_status->data_trans_enbld == 28 )
      strcat( moment_string, moments[1] );

   else{

      if( (rda_status->data_trans_enbld & 0x04) )
         strcat( moment_string, moments[2] );

      if( (rda_status->data_trans_enbld & 0x08) ){ 

         strcat( moment_string, "/" );
         strcat( moment_string, moments[3] );
    
      }

      if( (rda_status->data_trans_enbld & 0x10) ){ 

         strcat( moment_string, "/" );
         strcat( moment_string, moments[4] );
    
      }

   }

   fprintf( stderr, "MOMENTS:       %s\n", moment_string );

   if( rda_status->rda_control_auth & 2 ){

      fprintf( stderr, 
         "\n\nRDA Control Authorization: Local Control Requested\n\n" );

   }
   else if( rda_status->rda_control_auth & 4 ){

      fprintf( stderr, 
         "\n\nRDA Control Authorization: Remote Control Requested\n\n" );

   }

   if( rda_status->rda_alarm != 0 ){

      fprintf( stderr, "\n\nThe following alarm types are active\n" );

      if( rda_status->rda_alarm & 2 )
         fprintf( stderr, "   TOWER/UTILITIES\n" );

      if( rda_status->rda_alarm & 4 )
         fprintf( stderr, "   PEDESTAL\n" );

      if( rda_status->rda_alarm & 8 )
         fprintf( stderr, "   TRANSMITTER\n" );

      if( rda_status->rda_alarm & 16 )
         fprintf( stderr, "   RECEIVER/SIGNAL PROCESSOR\n" );

      if( rda_status->rda_alarm & 32 )
         fprintf( stderr, "   RDA CONTROL\n" );

      if( rda_status->rda_alarm & 64 )
         fprintf( stderr, "   RPG COMMUNICATION\n" );

      if( rda_status->rda_alarm & 128 )
         fprintf( stderr, "   USER COMMUNICATION\n" );

   }

}

/***********************************************************************

   Description:
      Processes VCP download command.

   Outputs:
      vcp_num - The VCP to download.

***********************************************************************/
static int Vcp_download_menu( int *vcp_num ){

   int reply;

   /*
     Display Download VCP menu.
   */
   fprintf( stderr, "Selection Menu\n" );
   fprintf( stderr, " 0 - Exit\n" );
   fprintf( stderr, " 1 - VCP number\n" );

   scanf( "%d", &reply );

   switch( reply ){


      case 0:{

         return -1;
         break;

      }

      case 1: {
         
         fprintf( stderr, "Enter VCP number to Download\n" );
         scanf( "%d", &reply );
         *vcp_num = reply;

         break;

      }

      default:{

         fprintf( stderr, "Invalid Selection!\n" );
         return -1;

      }

   }

   return 0;

}


/*******************************************************************

   Description:
      Posts an RDA Command.

   Inputs:
      rda_command - RDA command buffer.

   Returns:
      0 on success, negative number on error.

*******************************************************************/
static int Process_rda_command( Rda_cmd_t *rda_command ){

   int ret, event_ret;

   /*
     Write RDA Command to the RDA Command Linear Buffer.
   */

   fprintf( stderr, "CRDAT: write RDA Control Command: %d\n", 
            rda_command->cmd );
 
   ret = ORPGDA_write( ORPGDAT_RDA_COMMAND, (char *) rda_command,
                       sizeof( Rda_cmd_t ), LB_ANY );

   if( ret > 0 ){

      fprintf( stderr, "CRDAT: ORPGDA_write returned %d\n", ret );
      event_ret = EN_post( ORPGEVT_RDA_CONTROL_COMMAND,
                           (void *) NULL,
                           (size_t) 0,
                           (int) 1 );

      fprintf( stderr, "CRDAT: posting event. event_ret = %d\n", event_ret );

      if( event_ret < 0 ){

         fprintf( stderr, "Error in Event Notification. Event_ret = %d\n",
                  event_ret );
         return( event_ret );

      }

   }
   else 
      fprintf( stderr, "Command Write Failed.  Ret = %d", ret );

   return ret;

}

