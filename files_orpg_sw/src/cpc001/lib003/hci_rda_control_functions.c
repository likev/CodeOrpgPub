/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 16:17:32 $
 * $Id: hci_rda_control_functions.c,v 1.27 2014/03/13 16:17:32 steves Exp $
 * $Revision: 1.27 $
 * $State: Exp $
 */

/************************************************************************
  Module:  hci_rda_control_functions.c

  Description: This file contains a collection of modules used
               for RDA control
 ************************************************************************/

/* Local include files */

#include <hci.h>
#include <hci_vcp_data.h>
#include <hci_rda_control_functions.h>

/* Static/Global variables */

static char Cmd [HCI_BUF_128];
static int  User_selected_power_source = -1;
static int  User_selected_superres_flag = -1;
static int  User_selected_cmd_flag = -1;
static int  User_selected_avset_flag = -1;
static int  PRF_Mode_status_update_flag = HCI_YES_FLAG;
static int  PRF_Mode_status_init_flag = HCI_NO_FLAG;
static Prf_status_t  PRF_Mode_status;

static void PRF_Mode_status_callback( int, LB_id_t, int, void * );
static void Read_PRF_Mode_status_msg();
static void PRF_Mode_status_init();
static void Accept_power_source_change( Widget, XtPointer, XtPointer );
static void Accept_super_res_change( Widget, XtPointer, XtPointer );
static void Accept_cmd_change( Widget, XtPointer, XtPointer );
static void Accept_avset_change( Widget, XtPointer, XtPointer );

/************************************************************************
  Description: This function is called after the user selects one of
               the RDA Power Source radio buttons. It generates a
               confirmation popup.
 ************************************************************************/

void Verify_power_source_change( Widget parent_widget, XtPointer y )
{
  char buf[HCI_BUF_128];

  User_selected_power_source = (int) y;

  sprintf( buf, "You are about to change the RDA power source.\nDo you want to continue?" );
  hci_confirm_popup( parent_widget, buf, Accept_power_source_change, NULL );
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               button from the RDA Power Source confirmation popup.
 ************************************************************************/

static void Accept_power_source_change( Widget w, XtPointer y, XtPointer z )
{
  /* Check if selected state is different than current state. If
     so, request power source to be changed. Otherwise, do nothing. */

  if( User_selected_power_source == UTILITY_POWER )
  {
    HCI_LE_log( "Utility Power Source selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      sprintf( Cmd,"Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */
      strcpy( Cmd, "Request switch to Utility Power" );
      HCI_display_feedback( Cmd );

      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD,
                        CRDA_UTIL, 0, 0, 0, 0, NULL );
    }
  }
  else if( User_selected_power_source == AUXILLIARY_POWER )
  {
    HCI_LE_log( " Auxiliary Power Source selected" );

    if( ORPGRDA_get_status( RS_CONTROL_STATUS ) == CS_LOCAL_ONLY )
    {
      /* Generate a feedback message which can be displayed
         in the RPG Control/Status window. */

      sprintf( Cmd, "Rejected - RDA currently in control of RDA" );
      HCI_display_feedback( Cmd );
    }
    else
    {
      /* Generate a feedback message which can be displayed in the
         RPG Control/Status window. */

      sprintf( Cmd,"Request switch to Auxiliary Power" );
      HCI_display_feedback( Cmd );

      ORPGRDA_send_cmd( COM4_RDACOM, HCI_INITIATED_RDA_CTRL_CMD,
                        CRDA_AUXGEN, 0, 0, 0, 0, NULL );
    }
  }
}

/************************************************************************
  Description: This function is activated when the user clicks on the
               Super Res label.
 ************************************************************************/

void Verify_super_res_change( Widget parent_widget, XtPointer y )
{
  char buf[HCI_BUF_512];

  User_selected_superres_flag = (int) y;

  if( User_selected_superres_flag == HCI_NO_FLAG )
  {
    sprintf( buf, "You are about to disable Super Resolution.\n\nDisabling disallows Super Resolution data collection if Super Resolution\nis specified in the VCP definition. Change will not take effect until the\nnext start of volume scan.\n\nDo you want to continue?" );
  }
  else
  {
    sprintf( buf, "You are about to enable Super Resolution.\n\nEnabling allows Super Resolution data collection if Super Resolution\nis specified in the VCP definition. Change will not take effect until\nthe next start of volume scan.\n\nDo you want to continue?" );
  }

  hci_confirm_popup( parent_widget, buf, Accept_super_res_change, NULL );
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               the "Yes" button from the Super Res confirmation popup.
 ************************************************************************/

static void Accept_super_res_change( Widget w, XtPointer y, XtPointer z )
{
  int status;
  unsigned char flag;

  if( User_selected_superres_flag == HCI_NO_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                    ORPGINFO_STATEFL_CLR, &flag );

    sprintf( Cmd, "Super Resolution DISABLED" );
    HCI_LE_log("Super Resolution data collection Commanded DISABLED");

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                      CRDA_SR_DISAB, 0, 0, 0, 0, NULL );

  }
  else if( User_selected_superres_flag == HCI_YES_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                    ORPGINFO_STATEFL_SET, &flag );

    sprintf( Cmd, "Super Resolution ENABLED" );
    HCI_LE_log("Super Resolution data collection Commanded ENABLED");

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                      CRDA_SR_ENAB, 0, 0, 0, 0, NULL );   
  }
}

/************************************************************************
  Description: This function is activated when the user clicks on the
               CMD lable.
 ************************************************************************/

void Verify_cmd_change( Widget parent_widget, XtPointer y )
{
  char buf[HCI_BUF_256];

  User_selected_cmd_flag = (int) y;

  if( User_selected_cmd_flag == HCI_NO_FLAG )
  {
    sprintf( buf, "You are about to disable Clutter Mitigation Decision.  Change\nwill not take effect until the next start of volume scan.\n\nDo you want to continue?" );
  }
  else
  {
    sprintf( buf, "You are about to enable Clutter Mitigation Decision. Change\nwill not take effect until the next start of volume scan.\n\nDo you want to continue?" );
  }

  hci_confirm_popup( parent_widget, buf, Accept_cmd_change, NULL );
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               the "Yes" button from the Clutter Mitigation Decision
               Decision confirmation popup.
 ************************************************************************/

static void Accept_cmd_change( Widget w, XtPointer y, XtPointer z )
{
  int status;
  unsigned char flag;

  if( User_selected_cmd_flag == HCI_NO_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                    ORPGINFO_STATEFL_CLR, &flag );

    sprintf( Cmd, "Clutter Mitigation Decision DISABLED" );
    HCI_LE_log( "Clutter Mitigation Decision Commanded DISABLED" );

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                      CRDA_CMD_DISAB, 0, 0, 0, 0, NULL );

  }
  else if( User_selected_cmd_flag == HCI_YES_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                    ORPGINFO_STATEFL_SET, &flag );

    sprintf( Cmd, "Clutter Mitigation Decision ENABLED" );
    HCI_LE_log( "Clutter Mitigation Decision Commanded ENABLED" );

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                      CRDA_CMD_ENAB, 0, 0, 0, 0, NULL );
  }
}

/************************************************************************
  Description: This function is activated when the user clicks on the
               AVSET lable.
 ************************************************************************/

void Verify_avset_change( Widget parent_widget, XtPointer y )
{
  char buf[HCI_BUF_256];

  User_selected_avset_flag = (int) y;

  if( User_selected_avset_flag == HCI_NO_FLAG )
  {
    sprintf( buf, "You are about to disable Automatic Volume Scan Evaluation and Termination.\nChange will not take effect until the next start of volume scan.\n\nDo you want to continue?" );
  }
  else
  {
    sprintf( buf, "You are about to enable Automatic Volume Scan Evaluation and Termination.\nChange will not take effect until the next start of volume scan.\n\nDo you want to continue?" );
  }

  hci_confirm_popup( parent_widget, buf, Accept_avset_change, NULL );
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               the "Yes" button from the Automatic Volume Scan Evaluation
               and Termination confirmation popup.
 ************************************************************************/

static void Accept_avset_change( Widget w, XtPointer y, XtPointer z )
{
  int status;
  unsigned char flag;

  if( User_selected_avset_flag == HCI_NO_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                    ORPGINFO_STATEFL_CLR, &flag );

    sprintf( Cmd, "Automatic Volume Scan Evaluation and Termination DISABLED" );
    HCI_LE_log( "Automatic Volume Scan Evaluation and Termination Commanded DISABLED" );

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                      CRDA_AVSET_DISAB, 0, 0, 0, 0, NULL );

  }
  else if( User_selected_avset_flag == HCI_YES_FLAG )
  {
    status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                                    ORPGINFO_STATEFL_SET, &flag );

    sprintf( Cmd, "Automatic Volume Scan Evaluation and Termination ENABLED" );
    HCI_LE_log( "Automatic Volume Scan Evaluation and Termination Commanded ENABLED" );

    ORPGRDA_send_cmd( COM4_RDACOM, MSF_INITIATED_RDA_CTRL_CMD,
                              CRDA_AVSET_ENAB, 0, 0, 0, 0, NULL );
  }
}

/************************************************************************
  Description: Return current PRF mode state.
 ************************************************************************/

int hci_get_PRF_Mode_state()
{
  if( !PRF_Mode_status_init_flag )
  {
    PRF_Mode_status_init_flag = HCI_YES_FLAG;
    PRF_Mode_status_init();
  }

  if( PRF_Mode_status_update_flag )
  {
    PRF_Mode_status_update_flag = HCI_NO_FLAG;
    Read_PRF_Mode_status_msg();
  }

  return PRF_Mode_status.state;
}

/************************************************************************
  Description: Return current PRF mode status message.
 ************************************************************************/

Prf_status_t hci_get_PRF_Mode_status_msg()
{
  if( !PRF_Mode_status_init_flag )
  {
    PRF_Mode_status_init_flag = HCI_YES_FLAG;
    PRF_Mode_status_init();
  }
 
  if( PRF_Mode_status_update_flag )
  {
    PRF_Mode_status_update_flag = HCI_NO_FLAG;
    Read_PRF_Mode_status_msg();
  }

  return PRF_Mode_status;
}

/************************************************************************
  Description: Write command to set PRF Mode.
 ************************************************************************/

int hci_write_PRF_command( Prf_command_t cmd )
{
  return ORPGDA_write( ORPGDAT_PRF_COMMAND_INFO,
                       (char *) &cmd,
                       sizeof( Prf_command_t ),
                       ORPGDAT_PRF_COMMAND_MSGID );
}

/************************************************************************
  Description: This function initializes and registers a callback for the
               PRF Mode status.
 ************************************************************************/

static void PRF_Mode_status_init()
{
  int status = -1;

  /* Register for PRF Mode updates. */

  ORPGDA_write_permission( ORPGDAT_PRF_COMMAND_INFO );
  status = ORPGDA_UN_register( ORPGDAT_PRF_COMMAND_INFO,
                               ORPGDAT_PRF_STATUS_MSGID,
                               PRF_Mode_status_callback );

  if(status != 0)
  {
    HCI_LE_log("UN_register ORPGDAT_PRF_COMMAND_INFO failed: %d", status);
    HCI_task_exit( HCI_EXIT_FAIL );
  }
}

/************************************************************************
  Description: This function is the callback activated when the PRF
               Mode is updated.
 ************************************************************************/

static void PRF_Mode_status_callback( int fd, LB_id_t msgid, int msginfo, void *arg )
{
  PRF_Mode_status_update_flag = HCI_YES_FLAG;
}

/************************************************************************
  Description: This function reads the PRF Mode status LB message.
 ************************************************************************/

static void Read_PRF_Mode_status_msg()
{
  int status = -1;
  Prf_status_t temp;

  status = ORPGDA_read( ORPGDAT_PRF_COMMAND_INFO,
                        (char *) &temp,
                        sizeof( Prf_status_t ),
                        ORPGDAT_PRF_STATUS_MSGID );

  if( status <= 0 )
  {
    HCI_LE_error("ORPGDA_read(ORPGDAT_PRF_COMMAND_INFO) Failed (%d)", status );
    PRF_Mode_status.state = PRF_COMMAND_UNKNOWN;
  }
  else
  {
     /* Everything is okay, copy from temporary variable. */
     memcpy( &PRF_Mode_status, &temp, status );
  }
}

/************************************************************************
  Description: This function is activated when the user selects the "Yes"
               button from the Velocity Resolution confirmation popup.
 ************************************************************************/

void hci_change_velocity_resolution( int reso_flag )
{
  int rda_vel_reso_code = 0;

  if( reso_flag == HCI_VELOCITY_RESOLUTION_LOW )
  {
    sprintf( Cmd, "Changing velocity resolution to LOW" );
    rda_vel_reso_code = CRDA_VEL_RESO_LOW;
  }
  else
  {
    sprintf( Cmd, "Changing velocity resolution to HIGH" );
    rda_vel_reso_code = CRDA_VEL_RESO_HIGH;
  }

  HCI_display_feedback( Cmd );

  ORPGRDA_send_cmd( COM4_VEL_RESO, HCI_INITIATED_RDA_CTRL_CMD, rda_vel_reso_code, 0, 0, 0, 0, NULL );
}


