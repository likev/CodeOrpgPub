/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/10/29 14:04:09 $
 * $Id: hci_sails_tool.c,v 1.2 2013/10/29 14:04:09 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <infr.h>
#include <orpg.h>
#include <orpgrda.h>
#include <vcp.h>

/* Global Variables. */
GtkWidget *Window = NULL;

/* Used for providing Number of SAILS Cuts. */
GtkWidget *Button1 = NULL;
GtkWidget *Button2 = NULL;
GtkWidget *Button3 = NULL;
GtkWidget *Box1 = NULL;
GtkWidget *Active_button = NULL;
GtkWidget *Prev_active_button = NULL;

/* Used for providing SAILS Status */
GtkWidget *Table1 = NULL;
GtkWidget *Table2 = NULL;
GtkWidget *State_label = NULL;
GtkWidget *State_button = NULL;
GtkWidget *Status_label1 = NULL;
GtkWidget *Status_label2 = NULL;
GtkWidget *RDA_VCP_label = NULL;
GtkWidget *VCP_label1 = NULL;
GtkWidget *VCP_label2 = NULL;
GtkWidget *Avset_label1 = NULL;
GtkWidget *Avset_label2 = NULL;
GtkWidget *Avset_label3 = NULL;
GtkWidget *Volume_label = NULL;
GdkColor Yellow;
GdkColor Green;
GdkColor Canvas;
GdkColor Steelblue;
GdkColor White;

#define STATUS_UNKNOWN		0
#define STATUS_OFF		1
#define STATUS_ACTIVE		2

static int N_Sails = 0;
static int First_time_cuts = 1;
static int First_time_status = 1;
static int Sails_enabled = 0;
static char *Sails_state[] = { " Disabled ", " Enabled  ", " Unknown  " };
static int Disable_avset_reporting = 0;
static int Disable_RDAVCP_reporting = 0;
static Scan_Summary Avset_scan_summary;
static Scan_Summary *Avset_scan_summary_p = NULL;
static int Ev_end_avset_volume = 0; 
static int AVSET_elev_i = -999;
static float AVSET_elev_f = -99.9f;
static int SAILS_status = STATUS_UNKNOWN;
static int Refresh_VCP_data = 1;
char *Status[] = { "????????" ,
                   " Inactive " ,
                   "  Active" };

#define RDA_STANDBY		4
#define RDA_OFFLINE_OPERATE	64

/* Function Prototypes and Definitions. */
static int Check_SAILS_cuts();
static int Check_SAILS_status();
static int Check_AVSET( );
static int Get_VCP();
static int VCP_display();
static int Service_timeout();
static void Failure_and_exit();

/* Callback functions. */
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data );
static gboolean Radio_action_callback( gpointer data );
static gboolean Status_action_callback( gpointer data );
static gboolean Help_callback( gpointer data );
static void En_callback( EN_id_t evtcd, char *msg, int msglen, void *arg );


/*\///////////////////////////////////////////////////////////////////

   Description:
      Main function for SAILS gui.

///////////////////////////////////////////////////////////////////\*/
int main( int   argc, char *argv[] ){

    GtkWidget *box2;
    GtkWidget *table;
    GtkWidget *button;
    GtkWidget *separator;
    GtkWidget *frame;
    GtkWidget *scrolled;
    GtkWidget *child;
    GSList *group;

    int ret;
  
    /* Set up RPG environment. */
    if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGMISC_init Failed\n" );
       exit(0);

    }

    /* Register for the ORPGEVT_LAST_ELEV_CUT event. */
    Disable_avset_reporting = 0;
    if( (ret = EN_register( ORPGEVT_LAST_ELEV_CUT, En_callback )) < 0 ) 
       Disable_avset_reporting = 1;

    /* Register for RDA RDACNT updates. */
    Refresh_VCP_data = 1;
    Disable_RDAVCP_reporting = 0;
    if( (ret = EN_register( ORPGEVT_START_OF_VOLUME, En_callback )) < 0 )
       Disable_RDAVCP_reporting = 1;

    /* Get the SAILS Status. */
    Sails_enabled = ORPGINFO_is_sails_enabled();
    if( Sails_enabled < 0 )
       Sails_enabled = 2;

    /* Initialize GTK. */
    gtk_init (&argc, &argv);    
      
    /* Do some initialization ... */
    gdk_color_parse( "yellow", &Yellow );
    gdk_color_parse( "green", &Green );
    gdk_color_parse( "steel blue", &Steelblue );
    gdk_color_parse( "white", &White );
    
    /* Create the top-level window. */
    Window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* We don't want the maximum button ... just minimize and 
       close. */
    gtk_window_set_resizable( GTK_WINDOW( Window ), FALSE );

    /* Set the background color to "peachpuff3" */
    gdk_color_parse( "#cdaf95", &Canvas );
    gtk_widget_modify_bg( Window, GTK_STATE_NORMAL, &Canvas );
  
    /* Set up the close application callback.  This 
       function is called when the X button in the 
       upper right corner is pressed. */
    g_signal_connect( Window, "delete-event",
		      G_CALLBACK (Close_application),
                      NULL );

    gtk_window_set_title (GTK_WINDOW (Window), "SAILS Status/Control");
    gtk_container_set_border_width (GTK_CONTAINER (Window), 0);

    Box1 = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER (Window), Box1 );
    gtk_widget_show( Box1 );

    /* Add the close and help buttons. */
    table = gtk_table_new( 1, 3, TRUE );

    /* Define close section of the GUI. */
    {
       /* Make the "Close" button blue with white text. */
       button = gtk_button_new_with_label( " Close " );
       gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
       child = gtk_bin_get_child( GTK_BIN(button) );
       gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );
       gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

       /* Attach the button to the table. */
       gtk_table_attach( GTK_TABLE(table), button, 0, 1, 0, 1, 
                         GTK_EXPAND, GTK_FILL, 3, 3 );

       /* Define signal handler for the "Close" button. */
       g_signal_connect( button, "clicked",
                         G_CALLBACK (Close_application),
                         Window );

       /* Define the "Help" button. */
       button = gtk_button_new_from_stock( GTK_STOCK_HELP );
       gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
       child = gtk_bin_get_child( GTK_BIN(button) );
       gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );

       /* Define signal handler for the "Help" button. */
       g_signal_connect_swapped( button, "clicked",
                                 G_CALLBACK (Help_callback),
                                 Window );

       /* Attach the button to the table and the table to the container. */
       gtk_table_attach( GTK_TABLE(table), button, 2, 3, 0, 1, 
                         GTK_EXPAND, GTK_FILL, 3, 3 );
       gtk_container_add( GTK_CONTAINER (Box1), table );

    }

    /* Add a separator. */
    separator = gtk_hseparator_new ();
    {
       gtk_box_pack_start (GTK_BOX (Box1), separator, FALSE, TRUE, 0);
    }

    /* Add the "SAILS INFO" frame. */
    frame = gtk_frame_new( "SAILS Info" );
    {
       Table1 = gtk_table_new( 2, 3, TRUE );

       /* Add the State label and button.  The State indicates whether the 
          SAILS state is enabled or disabled. */
       State_label = gtk_label_new( "SAILS State: ");
       State_button = gtk_button_new_with_label( Sails_state[Sails_enabled] );
       if( Sails_enabled == 1 ){

          gtk_widget_modify_bg( State_button, GTK_STATE_NORMAL, &Green );
          gtk_widget_modify_bg( State_button, GTK_STATE_PRELIGHT, &Green );
    
       }
       else{

          gtk_widget_modify_bg( State_button, GTK_STATE_NORMAL, &Yellow );
          gtk_widget_modify_bg( State_button, GTK_STATE_PRELIGHT, &Yellow );

       }

       gtk_button_set_relief( GTK_BUTTON( State_button ), GTK_RELIEF_HALF );
       gtk_label_set_justify( GTK_LABEL(State_label), GTK_JUSTIFY_LEFT );

       /* Add the Status labels .....The label indicates either OFF (Yellow)
          which indicates a non-SAILS enabled VCP is in execution, ON (Green) 
          which indicates a SAILS enabled VCP is in execution but SAILS cuts
          are not defined and ACTIVE (GREEN) indicates that a SAILS enabled
          VCP is in execution and SAILS cuts are defined. */
       Status_label1 = gtk_label_new( "SAILS Status: ");
       gtk_label_set_justify( GTK_LABEL(Status_label1), GTK_JUSTIFY_LEFT );

       /* Create an Entry widget for the SAILS Status.  Make this Entry 
          widget not editable with a limit of 8 characters of text. */
       Status_label2 = gtk_entry_new( );
       gtk_editable_set_editable( GTK_EDITABLE( Status_label2 ), FALSE );
       gtk_entry_set_width_chars( GTK_ENTRY( Status_label2 ), 8 );
       if( Sails_enabled == 1 ){

          /* Sets the background color of the Entry widget.  Note:  You can
             use get_widget_modify_text to change the text color ... for now
             we use the default text color of black. */
          gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Green );

       }
       else{

          /* Sets the background color of the Entry widget.  Note:  You can 
             use get_widget_modify_text to change the text color ... for now
             we use the default text color of black. */
          gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );

       }

       /* Attach the widgets to the table, and add the table to the frame container. */
       gtk_container_add( GTK_CONTAINER( frame ), Table1 );
       gtk_table_attach( GTK_TABLE(Table1), State_label, 0, 1, 0, 1, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table1), State_button, 2, 3, 0, 1, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table1), Status_label1, 0, 1, 1, 2, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table1), Status_label2, 2, 3, 1, 2, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );

       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);

       /* Initialize the SAILS Status label text. */
       gtk_entry_set_text( GTK_ENTRY( Status_label2 ), 
                           Status[ STATUS_UNKNOWN ] ); 

    }

    /* Add a separator. */
    separator = gtk_hseparator_new ();
    {
       gtk_box_pack_start (GTK_BOX (Box1), separator, FALSE, TRUE, 0);
    }

    /* Add the "VCP Info" frame. */
    frame = gtk_frame_new( "VCP Info" );
    {
       Table2 = gtk_table_new( 4, 3, TRUE );

       /* Create a Current VCP Button. */
       {
          VCP_label1 = gtk_label_new( "Current VCP: ");
          gtk_label_set_justify( GTK_LABEL(VCP_label1), GTK_JUSTIFY_LEFT );

          /* Create an Button widget with label for the Current VCP. */
          VCP_label2 = gtk_entry_new();
          gtk_editable_set_editable( GTK_EDITABLE( VCP_label2 ), FALSE );
          gtk_entry_set_width_chars( GTK_ENTRY( VCP_label2 ), 8 );
       }

       /* Create a AVSET State Button. */
       {
          Avset_label1 = gtk_label_new( "AVSET State: ");
          gtk_label_set_justify( GTK_LABEL(Avset_label1), GTK_JUSTIFY_LEFT );

          /* Create an Button widget with label for the Current VCP. */
          Avset_label2 = gtk_entry_new();
          gtk_editable_set_editable( GTK_EDITABLE( Avset_label2 ), FALSE );
          gtk_entry_set_width_chars( GTK_ENTRY( Avset_label2 ), 8 );
       }

       /* Provide the AVSET termination angle, when AVSET is active. */
       Avset_label3 = gtk_label_new( "Last Known AVSET Termination Angle: ????  ");

       /* Create an Label widget for the Volume label. */
       Volume_label = gtk_label_new( "Vol xx: mm/dd/yy hh/mm/ss UT" );

       /* Attach the widgets to the table, and add the table to the frame container. */
       gtk_container_add( GTK_CONTAINER( frame ), Table2 );
       gtk_table_attach( GTK_TABLE(Table2), Volume_label, 0, 3, 0, 1, 
                         GTK_SHRINK, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table2), VCP_label1, 0, 1, 1, 2, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table2), VCP_label2, 2, 3, 1, 2, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table2), Avset_label1, 0, 1, 2, 3, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table2), Avset_label2, 2, 3, 2, 3, 
                         GTK_EXPAND, GTK_SHRINK, 3, 3 );
       gtk_table_attach( GTK_TABLE(Table2), Avset_label3, 0, 3, 3, 4, 
                         GTK_SHRINK, GTK_SHRINK, 3, 3 );

       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);

    }

    /* Add the "SAILS Cuts" frame. */
    frame = gtk_frame_new( "SAILS Cuts" );
    {
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);
       box2 = gtk_hbox_new (FALSE, 0);
       gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
       gtk_container_add( GTK_CONTAINER( frame ), box2 );

       /* Add the radio buttons for the various number of SAILS cuts. */
       Button1 = gtk_radio_button_new_with_label( NULL, "SAILSx1" );
       gtk_box_pack_start( GTK_BOX (box2), Button1, TRUE, TRUE, 0 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( Button1 ) );

       Button2 = gtk_radio_button_new_with_label( group, "SAILSx2" );
       gtk_box_pack_start( GTK_BOX (box2), Button2, TRUE, TRUE, 0 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button2) );

       Button3 = gtk_radio_button_new_with_label( group, "SAILSx3" );
       gtk_box_pack_start( GTK_BOX (box2), Button3, TRUE, TRUE, 0 );
    }

    /* Add the "RDA VCP" frame. */
    frame = gtk_frame_new( "RDA VCP" );
    {
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);

       /* Create a scrolled window. */
       scrolled = gtk_scrolled_window_new( NULL, NULL );
       gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

       /* Create a label containing the VCP information and add to the scrolled window. */
       RDA_VCP_label = gtk_label_new( "VCP Information will be displayed here!" );   
       gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( scrolled ), 
                                              GTK_WIDGET( RDA_VCP_label ) );
       gtk_widget_set_size_request( scrolled, 375, 375 );

       /* Add the scrolled window to the frame container. */
       gtk_container_add( GTK_CONTAINER(frame), scrolled ); 


    }

    gtk_widget_show_all( Window );
    gtk_widget_hide( Avset_label3 );
     
    /* Do initial check to see which radio button to depress and
       determine the SAILS status. */
    First_time_cuts = 1;
    Check_SAILS_cuts();

    First_time_status = 1;
    Check_SAILS_status();

    /* Define callback functions. */
    g_signal_connect_swapped( Button1, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "SAILSx1" );
    g_signal_connect_swapped( Button2, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "SAILSx2" );
    g_signal_connect_swapped( Button3, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "SAILSx3" );
    g_signal_connect_swapped( State_button, "clicked",
                              G_CALLBACK( Status_action_callback ),
                              "NULL" );

    /* Set a period timer to check the value of the number of
       SAILS cuts. */
    g_timeout_add( 1000, Service_timeout, NULL );  

    /* Do GTK Main Loop. */
    gtk_main( ); 
    return 0;
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function contains a list of functions to be called by gtk_main()
      whenever a timeout occurs.

////////////////////////////////////////////////////////////////////////////\*/
int Service_timeout(){

   int ret = 0;
   static Mrpg_state_t rpg_state;

   /* Check the RPG state. */
   ret = ORPGMGR_get_RPG_states( &rpg_state );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "ORPGMGR_get_RPG_states Returned Error.\n" );
      Failure_and_exit();

   }

   /* Check the Status and State of SAILS.  Report any updates. */
   Check_SAILS_status();

   /* Check the Status of AVSET and get the last termination angle. */
   Check_AVSET(); 

   /* Update the VCP information. */
   Get_VCP();

   /* Update the RDA VCP display. */
   if( Refresh_VCP_data ){

      Refresh_VCP_data = 0;
      VCP_display();

   }
   
   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Checks the number of SAILS Cuts defined in adaptation data.  Also sets
      the default state for the SAILS Cuts Radio Buttons on first call.

      Raises a signal if the number of SAILS Cuts has changed.

////////////////////////////////////////////////////////////////////////////\*/
static int Check_AVSET( ){

   static char buf[128];

   /* Check if AVSET is enabled. */
   unsigned char avset_state = ORPGINFO_is_avset_enabled();

   int wbstat, n_ele_vst = 0, avset_status = 0;
   static int n_ele = -1;

   /* Get wideband connection status. */
   wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
   if( wbstat != RS_CONNECTED ){

      /* The wideband is not connected so we don't know what the 
         AVSET status is.  Set the background color and text of the 
         entry widgeti to yellow and question marks. */
      gtk_widget_modify_base( Avset_label2, GTK_STATE_NORMAL, &Yellow );
      gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) "   ????   " );

   }
   else {

      /* Get the AVSET state from RDA Status. */
      avset_status = ORPGRDA_get_status( RS_AVSET );

      if( (avset_status != AVSET_ENABLED) 
                        &&
          (avset_status != AVSET_DISABLED) ){

         /* Handle an unknown AVSET status value. */
         gtk_widget_modify_base( Avset_label2, GTK_STATE_NORMAL, &Yellow );
         gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) "   ????   " );

      }
      else {

         /* Indicate the AVSET state. */
         if( avset_state == 0 ){

            /* The local AVSET status (via RPG state data) indicates disabled. 
               Set the background color and text of the entry widget based on
               RDA Status. */
            gtk_widget_modify_base( Avset_label2, GTK_STATE_NORMAL, &Yellow );

            if( avset_status == AVSET_ENABLED )
               gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) "  Pending " );

            else if( avset_status == AVSET_DISABLED )
               gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) " Disabled " );

         }
         else {

            /* The local AVSET status (via RPG state data) indicates enabled.
               Set the background color and text of the entry widget based on
               RDA Status. */
            gtk_widget_modify_base( Avset_label2, GTK_STATE_NORMAL, &Green );

            if( avset_status == AVSET_ENABLED )
               gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) " Enabled  " );

            else if( avset_status == AVSET_DISABLED )
               gtk_entry_set_text( GTK_ENTRY( Avset_label2 ), (gchar *) "  Pending " );

         }

      }

   }
      
   /* If AVSET is not enabled, hide the AVSET termination angle. */
   if( (wbstat != RS_CONNECTED) 
                  ||
       (avset_status != AVSET_ENABLED)
                  ||
       (Disable_avset_reporting) ){

      /* Hide the widget. */
      gtk_widget_hide( Avset_label3 );

      /* Initialize the number of elevations. */
      n_ele = -1;

      /* Initialize the AVSET termination angle. */
      AVSET_elev_i = -999;

   }
   else{

      /* This event only gets posted if AVSET is enabled. */
      if( Ev_end_avset_volume ){

         Ev_end_avset_volume = 0; 

         /* Get the number of elevations from volume status. */
         n_ele_vst = ORPGVST_get_number_elevations();

         /* If Avset is enabled and processing Scan Summary, set the number of cuts
            as defined in the scan summary data. */
         if( Avset_scan_summary_p != NULL ){

            /* If the AVSET status was enabled. */
            if( Avset_scan_summary_p->avset_status == AVSET_ENABLED ){

               if( Avset_scan_summary_p->last_rda_cut != 0xff )
                  n_ele = Avset_scan_summary_p->last_rda_cut;
 
               else
                  n_ele = n_ele_vst;

               /* Derive the termination angle. */
               AVSET_elev_i = ORPGVST_get_elevation( n_ele-1 );
               AVSET_elev_f = AVSET_elev_i / 10.0;

               /* If AVSET terminated the VCP, display angle with green background ... 
                  otherwise white. */
               if( n_ele_vst != n_ele )
                  sprintf( buf, 
                      "Last Known AVSET Termination Angle: <span background=\"green\" weight=\"bold\">%4.1f</span> deg", 
                      AVSET_elev_f );      
               else
                  sprintf( buf, 
                      "Last Known AVSET Termination Angle: <span background=\"white\" weight=\"bold\">%4.1f</span> deg", 
                      AVSET_elev_f );      

               gtk_label_set_markup( GTK_LABEL( Avset_label3 ), (gchar *) buf ); 
               gtk_widget_show( Avset_label3 );

            }
            else if( Avset_scan_summary_p->avset_status == AVSET_DISABLED )
               AVSET_elev_i = -999;

         }

      }

      /* If the last known termination angle is not set, hide the widget. */
      if( AVSET_elev_i == -999 )
         gtk_widget_hide( Avset_label3 );

   }
   
   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Checks the number of SAILS Cuts defined in adaptation data.  Also sets
      the default state for the SAILS Cuts Radio Buttons on first call.

      Raises a signal if the number of SAILS Cuts has changed.

////////////////////////////////////////////////////////////////////////////\*/
int Check_SAILS_cuts( ){

   double dtemp = 0.0;
   int n_sails = 0, ret = 0;

   /* Read the number of SAILS cuts. */
   if( (ret = DEAU_get_values( "pbd.n_sails_cuts", &dtemp, 1 )) < 0 )
      return FALSE;

   else
      n_sails = (int) dtemp;

   /* Do on the first time through this function. */
   if( First_time_cuts ){
 
      N_Sails = n_sails;
      First_time_cuts = 0;

      /* Activate the radio button corresponding to the number of SAILS 
         Cuts defined in adaptation data. */
      if( N_Sails == 1 ){

         Active_button = Button1;
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button1 ), TRUE );

      }
      else if( N_Sails == 2 ){

         Active_button = Button2;
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button2 ), TRUE );

      }
      else if( N_Sails == 3 ){

         Active_button = Button3;
         gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button3 ), TRUE );

      }

      Prev_active_button = Active_button;

   }
   else {

      /* If n_sails != N_Sails, raise the "clicked" signal. */
      if( n_sails != N_Sails ){

         if( n_sails == 1 )
            gtk_button_clicked( GTK_BUTTON( Button1 ) );  

         else if( n_sails == 2 )
            gtk_button_clicked( GTK_BUTTON( Button2 ) );  

         else if( n_sails == 3 )
            gtk_button_clicked( GTK_BUTTON( Button3 ) );  

      }

   }
   
   return TRUE;

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Checks the SAILS status.  Set the SAILS Status and State as appropriate.

////////////////////////////////////////////////////////////////////////////\*/
int Check_SAILS_status( ){

   int sails_cuts = 0, local_sails_enabled = 0;
   int ret, vcp_num = -1;
   char *buf = NULL;
   char temp[32];
   
   /* Check whether SAILS is enabled or disabled. */
   local_sails_enabled = ORPGINFO_is_sails_enabled();
   if( local_sails_enabled < 0 )
      local_sails_enabled = 0;

   if( First_time_status ){

      First_time_status = 0;
      Sails_enabled = local_sails_enabled; 

   }
   else {

      /* Has the status changed. */
      if( Sails_enabled != local_sails_enabled ){

         /* Yes ... */

         /* The local SAILS status is enabled. */
         if( local_sails_enabled == 1 ){

            gtk_button_set_label( GTK_BUTTON( State_button ), 
                                  Sails_state[ local_sails_enabled ] ); 
            gtk_widget_modify_bg( State_button, GTK_STATE_NORMAL, &Green );
            gtk_widget_modify_bg( State_button, GTK_STATE_PRELIGHT, &Green );

         } 
         else if( local_sails_enabled == 0 ){

         /* The local SAILS status is disabled. */
            gtk_button_set_label( GTK_BUTTON( State_button ), 
                                  Sails_state[ local_sails_enabled ] ); 
            gtk_widget_modify_bg( State_button, GTK_STATE_NORMAL, &Yellow );
            gtk_widget_modify_bg( State_button, GTK_STATE_PRELIGHT, &Yellow );
         
         }

         Sails_enabled = local_sails_enabled; 

      }

   }

   /* Get the current VCP number from Volume Status.  This will be 
      used to display the SAILS state. */
   SAILS_status = STATUS_UNKNOWN;
   vcp_num = ORPGVST_get_vcp();
   if( vcp_num != ORPGVST_DATA_NOT_FOUND ){

      /* Read the RDA_RDACNT data, if available.  If the vcp number matches
         use this data, otherwise, use the RDACNT data (via ORPGVCP 
         functions. ) */
      ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &buf, LB_ALLOC_BUF,
                         RDA_RDACNT );

      if( (ret > 0) && (buf != NULL) ){

         RDA_rdacnt_t *rda_rdacnt = (RDA_rdacnt_t *) buf;
         int i, last = rda_rdacnt->last_entry;
         Vcp_struct *vcp = (Vcp_struct *) &rda_rdacnt->data[last].rdcvcpta[0];

         /* Verify the vcp numbers are consistent. */
         if( vcp->vcp_num == vcp_num ){

            /* Check the supplemental data to see if the Supplemental
               Scan flag is set. */
            SAILS_status = STATUS_OFF;
            sails_cuts = 0;
            for( i = 0; i < vcp->n_ele; i++ ){

               /* If Supplemental Scan, set flag and break out of loop. */
               if( rda_rdacnt->data[last].suppl[i] & RDACNT_SUPPL_SCAN ){

                  SAILS_status = STATUS_ACTIVE;
                  sails_cuts++;

               }

            } /* End of "for" loop. */

         }
         else
           SAILS_status = STATUS_UNKNOWN;

         free(buf);

      }

   }

   /* Set the SAILS Status. */
   switch( SAILS_status )
   {
      case STATUS_UNKNOWN:
      {
         /* Set the background color of the entry widget. */
         gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );

         /* Set the text. */
         gtk_entry_set_text( GTK_ENTRY( Status_label2 ), 
                             Status[ STATUS_UNKNOWN ] ); 
         break;
      }

      case STATUS_OFF:
      {
         /* Set the background color of the entry widget. */
         gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );

         /* Set the text. */
         gtk_entry_set_text( GTK_ENTRY( Status_label2 ), 
                             Status[ STATUS_OFF ] ); 
         break;
      }

      case STATUS_ACTIVE:
      {
         /* Set the background color of the entry widget. */
         gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Green );

         /* Set the text.  Note: The number of SAILS cuts should be a multiple
            of 2. */
         if( sails_cuts%2 != 0 )
            LE_send_msg( GL_INFO, "Unexpected Number of SAILS Elevation Cuts: %d\n", 
                         sails_cuts );

         sprintf( temp, "%s/%d  ",  Status[ STATUS_ACTIVE ], sails_cuts/2 );
         gtk_entry_set_text( GTK_ENTRY( Status_label2 ), temp ); 
         break;
      }

   }

   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for setting the VCP information.

   Note:
      This code was taken from the HCI (hci_control_penel_vcp.c)

////////////////////////////////////////////////////////////////////////////\*/
static int Get_VCP(){

   char temp[128];
   int status_vcp_number, scan_vcp_number;
   int scan_mode_operation, vol_num;

   unsigned long volume_time;
   static unsigned long prev_volume_time = (unsigned long) -1;

   /* Get VCP information from the latest RDA status data message and
      from the volume status message. */
   status_vcp_number = ORPGRDA_get_status( RS_VCP_NUMBER );
   scan_vcp_number = ORPGVST_get_vcp();
   scan_mode_operation = ORPGVST_get_mode();

   /* If the VCP number in the latest RDA status data message is
      negative, this is a local pattern.  Preceed the VCP number
      with the letter "L". If the VCP number in the latest RDA
      status message is positive, this is a remote pattern.
      Proceed the VCP number with the letter "R". */
   if( status_vcp_number < 0 ){

      if( scan_mode_operation == CLEAR_AIR_MODE )
         sprintf( temp, " L%d/B ", scan_vcp_number );
    
      else if( scan_mode_operation == PRECIPITATION_MODE )
         sprintf( temp, " L%d/A ", scan_vcp_number );
    
      else
         sprintf( temp, " L%d ", scan_vcp_number );

   }
   else if( status_vcp_number > 0 ){

      if( scan_mode_operation == CLEAR_AIR_MODE )
         sprintf( temp, " R%d/B ", scan_vcp_number );
   
      else if( scan_mode_operation == PRECIPITATION_MODE )
         sprintf( temp, " R%d/A ", scan_vcp_number );
   
      else
         sprintf( temp, " R%d/M ", scan_vcp_number );

   }
   else{

      if( scan_mode_operation == CLEAR_AIR_MODE )
         sprintf( temp, " %d/B ", scan_vcp_number );
   
      else if( scan_mode_operation == PRECIPITATION_MODE )
         sprintf( temp, " %d/A ", scan_vcp_number );
   
      else
         sprintf( temp, " %d ", scan_vcp_number );

   }

   gtk_entry_set_text( GTK_ENTRY( VCP_label2 ), temp ); 

   volume_time = ORPGVST_get_volume_time();

   if( volume_time != prev_volume_time ){

      /* If the volume date isn't defined yet, display a string
         without the date/time. */
      if( ORPGVST_get_volume_date() <= 1 ){

         vol_num = ORPGVST_get_volume_number();
         sprintf( temp, "<span foreground=\"dark green\" weight=\"heavy\">Vol: %d</span>",
                  vol_num );

      }
      else{

         time_t j_sec;
         int yr, mon, day, hr, min, sec;

         /* Get the current volume time, remove the fraction
            of seconds, and then convert from julian seconds to
            normal date/time. */
         sec = ORPGVST_get_volume_time()/1000;  
         j_sec = (time_t) ( ORPGVST_get_volume_date() - 1 ) * 86400 + sec;
         unix_time( &j_sec, &yr, &mon, &day, &hr, &min, &sec );
         if( yr >= 2000 )
            yr -= 2000;

         else
            yr -= 1900;
         vol_num = ORPGVST_get_volume_number();

         sprintf( temp,
                  "<span foreground=\"dark green\" weight=\"heavy\">Vol: %d   Start: %02d/%02d/%02d - %2.2d:%2.2d:%2.2d UT</span>",
                  vol_num, mon, day, yr, hr, min, sec );

      }

      gtk_label_set_markup( GTK_LABEL( Volume_label ), temp );
      prev_volume_time = volume_time;

   }

   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for Closing the application.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data ){

  /* Close this application. */
  gtk_main_quit ();
  return FALSE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Popup error message and exit.

////////////////////////////////////////////////////////////////////////////\*/
static void Failure_and_exit(){

   static char *text = "The RPG does not appear to be running.\nStart the RPG and try again.\n";
   GtkWidget *popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                     "%s", text );

   /* Run the dialog ... function returns when operator responds. */
   gtk_dialog_run( GTK_DIALOG(popup_window) );
   gtk_widget_destroy( popup_window );

   exit(0);

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for Help.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Help_callback( gpointer   data ){

   static char *text1 = "\nSAILS State is a binary state variable.  It has values\nof \"Enabled\" or \"Disabled\".  The SAILS State is\nindependent of VCP in operation.\n";
   static char *text2 = "SAILS Status has values of \"Active\" or \"Inactive\".  The Status\ndepends on both SAILS State and VCP in Operation.  Active\nindicates SAILS State is Enabled and a SAILS Enabled VCP is\nin operation.  If SAILS Active, the number following the\nforward slash is the number of SAILS cuts in the current VCP.\n";
   static char *text21 = "AVSET State shows the current state of AVSET.  Possible states\nare Enabled, Pending (Enabled), Disabled and Pending\n(Disabled).  A Pending State occurs when the RPG commands\na change but the RDA has not changed the status.  The\nRDA changes the AVSET status prior to the start of volume.\n";
   static char *text3 = "AVSET Termination Angle is only displayed when AVSET is\n\"Enabled\" and is the angle last scanned for the previous\nvolume scan.  If the termination angle is unknown, the\nlast elevation angle in the VCP definition will appear\nwith yellow background.  If AVSET does not terminate the\nVCP, the last elevation angle in the VCP definition will\nappear with white background.  If AVSET terminates the VCP,\nthe termination angle will appear with green background.\n";
   static char *text4 = "The SAILS Cuts radio buttons select the number of SAILS\ncuts to insert in the VCP.  Selection of a value is the\nsame as using the \"edit_dea\" tool to change the value.\nNote: The value chosen is ignored unless explicitly\nsupported by RPG software.\n";
   static char *text5 = "The RDA VCP frame shows the VCP definition from the RDA.\nAll SAILS cuts are highlighed in BOLD.  AVSET termination\nangle from the previous volume scan is highlighted in red\nif the angle is not known or AVSET is was not active and by\ngreen text if angle is known.\n";
   static char *text6 = "The contents of the GUI are dynamically updated as\ninformation changes.\n"; 

   GtkWidget *popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                     "%s\n%s\n%s\n%s\n%s\n%s\n%s", 
                                                     text1, text2, text21, text3, text4, text5, text6 );

   /* Run the dialog ... function returns when operator responds. */
   gtk_dialog_run( GTK_DIALOG(popup_window) );
   gtk_widget_destroy( popup_window );

   return TRUE;

}

#define TEXT_SIZE	25*128

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for VCP Button.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static int VCP_display(){

   int i, j, ret, vst_vcp;
   char *buf = NULL;
   unsigned short *rda_suppl = NULL;
   RDA_rdacnt_t *rda_rdacnt = NULL;
   Vcp_struct *rda_vcp = NULL;

   static char vcp_text[TEXT_SIZE];
   static char *wave_form[] = { "  UNK", "   CS", " CD/W", "CD/WO", "BATCH", "  STP" };
   static char temp[128];


   /* Read the RDA_RDACNT data, if available.  If the vcp number matches
         use this data, otherwise report error and return. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &buf, LB_ALLOC_BUF,
                      RDA_RDACNT );
   if( ret <= 0 ){

      GtkWidget *popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                        "Error Reading RDA VCP data (%d)", ret );

      /* Run the dialog ... function returns when operator responds. */
      gtk_dialog_run( GTK_DIALOG(popup_window) );
      gtk_widget_destroy( popup_window );

      sprintf( temp, "RDA VCP Data is currently UNAVAILABLE" );
      gtk_label_set_text( GTK_LABEL(RDA_VCP_label), (gchar *) temp );
      
      return FALSE;

   }

   /* Cast pointer.  Define */
   rda_rdacnt = (RDA_rdacnt_t *) buf;
   rda_vcp = (Vcp_struct *) &rda_rdacnt->data[rda_rdacnt->last_entry].rdcvcpta[0];
   rda_suppl = &rda_rdacnt->data[rda_rdacnt->last_entry].suppl[0];

   /* Get the VCP we think is executing and test if it is what was expected. */
   vst_vcp = ORPGVST_get_vcp();
   if( (vst_vcp < 0) 
             || 
       (vst_vcp != rda_vcp->vcp_num) ){

      /* The VCP is volume status does not match the VCP in RDA_RDACNT.  Report error. */
      GtkWidget *popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                        "RDA VCP Data Unavailable( %d != %d)",
                                                        vst_vcp, rda_vcp->vcp_num );

      /* Run the dialog ... function returns when operator responds. */
      gtk_dialog_run( GTK_DIALOG(popup_window) );
      gtk_widget_destroy( popup_window );

      /* Release memory associated with RDA_RDACNT. */
      free( buf );

      sprintf( temp, "RDA VCP Data is currently UNAVAILABLE" );
      gtk_label_set_text( GTK_LABEL(RDA_VCP_label), (gchar *) temp );

      return FALSE;

   }

   /* Initialize the VCP Text buffer. */
   memset( vcp_text, 0, TEXT_SIZE );

   /* Prepare the heading information. */
   strcat( vcp_text, "<span face=\"Monospace\">" );
   sprintf( temp, "\n                VCP %3d\n\n", vst_vcp );
   strcat( vcp_text, temp );

   /* Display the VCP information. */
   j = 0;
   for( i = 0; i < rda_vcp->n_ele; i++ ){
   
      Ele_attr *elev = (Ele_attr *) &rda_vcp->vcp_ele[i][0];
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );
      int wform = elev->wave_type;

      /* If a supplemental (SAILS) cut, make weight heavy. */
      if( rda_suppl[i] & RDACNT_SUPPL_SCAN )
         sprintf( temp, 
             "<span weight=\"heavy\">Elev # %2d: Elev: %5.2f  Wave Type: %5s\n</span>",
             i+1, elev_angle, wave_form[ wform ] );

      else if( (int) (elev_angle*10.0) == AVSET_elev_i ){

          /* Display the termination angle in green highlight. */
          sprintf( temp, 
            "<span weight=\"heavy\" foreground=\"green\">Elev # %2d: Elev: %5.2f  Wave Type: %5s\n</span>",
            i+1, elev_angle, wave_form[ wform ] );

      }
      else
         sprintf( temp, 
             "Elev # %2d: Elev: %5.2f  Wave Type: %5s\n",
             i+1, elev_angle, wave_form[ wform ] );

      strcat( vcp_text, temp );

   }
   strcat( vcp_text, "</span>" );
   
   /* Update the label with the VCP information. */
   gtk_label_set_markup( GTK_LABEL(RDA_VCP_label), (gchar *) vcp_text );

   return TRUE;
}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for SAILS Cuts Radio Buttons.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Radio_action_callback( gpointer data ){

   double dtemp;
   int value = 0, ret = 0;

   static char *text1 = "You are about to change the Number of SAILS cuts to ";
   static char *text2 = "\nDo You Want to Continue?\n";

   GtkWidget *popup_window = NULL;
   gint result = 0;

   if( (ret = DEAU_get_values( "pbd.n_sails_cuts", &dtemp, 1 )) < 0 )
      exit(0);

   else
      N_Sails = (int) dtemp;

   if( strstr( (gchar *) data, "SAILSx1" ) ){

      if( N_Sails == 1 )
         return TRUE;

       ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button1 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button1;
         value = 1;

      }

   }
   else if( strstr( (gchar *) data, "SAILSx2" ) ){

      if( N_Sails == 2 )
         return TRUE;

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button2 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button2;
         value = 2;

      }

   }
   else if( strstr( (gchar *) data, "SAILSx3" ) ){

      if( N_Sails == 3 )
         return TRUE;

      ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button3 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button3;
         value = 3;

      }

   }
   else
     g_print( "Bad Value in radio_action: value = %s\n", (gchar *) data );

   popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                          "%s %1d %s", text1, (int) value, text2 );

   /* Run the dialog ... function returns when operator responds. */
   result = gtk_dialog_run( GTK_DIALOG(popup_window) );
   switch( result ){

      /* On YES, set adaptation data to the selected value. */
      case GTK_RESPONSE_YES:
      {
         N_Sails = (int) value;

         /* Set the value in adaptation data. */
         dtemp = N_Sails;
         ret = DEAU_set_values( "pbd.n_sails_cuts", 0, (void *) &dtemp,
                          1, 0 );
 
         LE_send_msg( GL_STATUS, "Number SAILS Cuts --> %2d\n", N_Sails );
         Prev_active_button = Active_button;
         break;

      }
      default:
      {
         /* On No, activate the radio button previously active. */
         if( (Prev_active_button != NULL) && (Prev_active_button != Active_button) )
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (Prev_active_button), TRUE );

         Active_button = Prev_active_button;
         break;
      }

   }

   gtk_widget_destroy( popup_window );

   return TRUE;
}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for SAILS Status Button.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Status_action_callback( gpointer data ){


   static char *enable_text = "You are about to enable SAILS.   Change will not\ntake effect until next start of volume.\n";
   static char *disable_text = "You are about to disable SAILS.  Change will not\ntake effect until next start of volume.\n";
   static char *confirm_text = "\nDo You Want to Continue?\n";

   int local_sails_enabled = ORPGINFO_is_sails_enabled();

   GtkWidget *popup_window = NULL;
   gint result = 0;

   if( local_sails_enabled < 0 )
      local_sails_enabled = Sails_enabled;

   if( local_sails_enabled == 0 )
      popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                             "%s%s", enable_text, confirm_text );
   else if( local_sails_enabled == 1 )
      popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                             "%s%s", disable_text, confirm_text );

   /* Run the dialog ... function returns when operator responds. */
   result = gtk_dialog_run( GTK_DIALOG(popup_window) );

   /* Response is either Yes or No .... On Yes, enable/disable SAILS
      based on current state.  On No, do nothing. */
   switch( result ){

      case GTK_RESPONSE_YES:
      {
         /* Set the SAILS state. */
         if( local_sails_enabled == 1 )
            ORPGINFO_clear_sails_enabled();

         else if( local_sails_enabled == 0 )
            ORPGINFO_set_sails_enabled();

         break;

      }
      default:
      {
         break;
      }

   }

   gtk_widget_destroy( popup_window );


 return TRUE;

}

/*\///////////////////////////////////////////////////////////////////////

   Description:
      Callback function for the ORPGEVT_LAST_ELEV_CUT and 
      ORPGEVT_START_OF_VOLUME events.

///////////////////////////////////////////////////////////////////////\*/
static void En_callback( EN_id_t evtcd, char *msg, int msglen, void *arg ){

   /* Is the the ORPGEVT_LAST_ELEV_CUT event. */
   if( evtcd == ORPGEVT_LAST_ELEV_CUT ){

      if( (msglen != sizeof(Scan_Summary)) || (msg == NULL) ){

         Avset_scan_summary_p = NULL;
         LE_send_msg (GL_ERROR, "Bad msg in event ORPGEVT_LAST_ELEV_CUT - event discarded\n");
         return;

      }
            
      memcpy( &Avset_scan_summary, msg, sizeof(Scan_Summary) );
      Avset_scan_summary_p = &Avset_scan_summary;
      Ev_end_avset_volume = 1;

   }
   else if( evtcd == ORPGEVT_START_OF_VOLUME ){

      Refresh_VCP_data = 1;
      return;

   }

}

