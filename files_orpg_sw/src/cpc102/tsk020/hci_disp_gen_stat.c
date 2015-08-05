/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/20 14:42:18 $
 * $Id: hci_disp_gen_stat.c,v 1.8 2014/08/20 14:42:18 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <infr.h>
#include <orpg.h>
#include <orpgrda.h>
#include <vcp.h>

/* Macros for writing VCP data. */
#define FROM_VOLUME_STATUS      1
#define FROM_RDA_RDACNT         2
#define STATUS_WORDS            26
#define MAX_STATUS_LENGTH       64
#define MAX_PWR_BITS            5
#define COMSWITCH_BIT           4

typedef enum { ORPGSITE_NO_REDUNDANCY=0, ORPGSITE_FAA_REDUNDANT=1, ORPGSITE_NWS_REDUNDANT=2 } orpgsite_redundant_type_t;


/* Global Variables. */
static char *Buffer = NULL;
static int Buf_size = 0;
static int Text_size = 0;
static int Refresh = 0;
GtkWidget *Window = NULL;

/* Used for selecting radio buttons. */
GtkWidget *Button1 = NULL;
GtkWidget *Button2 = NULL;
GtkWidget *Button3 = NULL;
GtkWidget *Button4 = NULL;
GtkWidget *Active_button = NULL;
GtkWidget *Info_label = NULL;
GtkWidget *Box1 = NULL;
GtkWidget *Refresh_button = NULL;

/* Used for displaying VCP Information. */
GtkWidget *Window_vcp = NULL;
GtkWidget *Box_vcp = NULL;
GtkWidget *Frame_vcp = NULL;
GtkWidget *Scrolled_vcp = NULL;
GtkWidget *Label_vcp = NULL;

/* Used for providing SAILS Status */
GtkWidget *Table1 = NULL;
GdkColor Yellow;
GdkColor Green;
GdkColor Canvas;
GdkColor Steelblue;
GdkColor White;

static  char            status[][32]            = { "Start-Up",
                                                    "Standby",
                                                    "Restart",
                                                    "Operate",
                                                    "Playback",
                                                    "Off-Line Operate",
                                                    "      " };
static  char            *moments[]              = { "None",
                                                    "All ",
                                                    "R",
                                                    "V",
                                                    "W",
                                                    "      " };
static  char            orda_mode[][32]         = { "Operational",
                                                    "Test",
                                                    "Maintenance",
                                                    "      " };
static  char            authority[][32]         = { "No Action",
                                                    "Local Control Requested",
                                                    "Local Control Released",
                                                    "      " };
static  char            channel_status[][32]    = { "Ctl",
                                                    "Non-Ctl",
                                                    "      " };
static  char            spot_blanking[][32]     = { "Not Installed",
                                                    "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            operability[][32]       = { "On-Line",
                                                    "MAR",
                                                    "MAM",
                                                    "CommShut",
                                                    "Inoperable",
                                                    "WB Disc",
                                                    "      " };
static  char            control[][32]           = { "RDA",
                                                    "RPG",
                                                    "Eit",
                                                    "      " };
static  char            *set_aux_pwr[]          = { " Aux Pwr=On",
                                                    " Util Pwr=Yes",
                                                    " Gen=On",
                                                    " Xfer=Manual",
                                                    " Cmd Pwr Switch",
                                                    "      " };
static  char            *reset_aux_pwr[]        = { " Aux Pwr=Off",
                                                    " Util Pwr=No",
                                                    " Gen=Off",
                                                    " Xfer=Auto",
                                                    "",
                                                    "      " };
static  char            tps[][32]               = { "Off",
                                                    "Ok",
                                                    "      " };

static  char            perf_check[][32]        = { "Auto",
                                                    "Pending",
                                                    "      " };
static  char            super_res[][32]         = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            cmd[][32]               = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            avset[][32]             = { "Enabled",
                                                    "Disabled",
                                                    "      " };
static  char            alarm_sum[][32]         = { "No Alarms",
                                                    "Tow/Util",
                                                    "Pedestal",
                                                    "Transmitter",
                                                    "Receiver",
                                                    "RDA Cntrl",
                                                    "Comms",
                                                    "Sig Proc",
                                                    "        " };
static  char            cmd_ack[][32]           = { "Remote VCP Received",
                                                    "Bypass Map Received",
                                                    "Clutter Censor Zones Received",
                                                    "Red Chan Cntrl Cmd Accepted",
                                                    "        " };
static  char            rms[][32]               = { "Non-RMS System",
                                                    "RMS In Control",
                                                    "RDA In Control",
                                                    "        " };
static char             wbstat[][32]            = { "Not Implemented",
                                                    "Connect Pending",
                                                    "Disconnect Pending",
                                                    "Disconnected HCI",
                                                    "Disconnected CM",
                                                    "Disconnected/Shutdown",
                                                    "Connected",
                                                    "Down",
                                                    "WB Failure",
                                                    "Disconnected RMS",
                                                    "        " };

/* Function Prototypes and Definitions. */
static int Service_timeout();
static void Failure_and_exit();

/* Callback functions. */
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer data );
static gboolean Quit_vcp( GtkWidget *widget, GdkEvent  *event,
                          gpointer data );
static gboolean Radio_action_callback( gpointer data );
static gboolean Refresh_callback( gpointer data );
static int Add_text( char *text );
static gboolean Display_volume_status( int refresh );
static void Write_vcp_data( Vcp_struct *vcp, int from, int create );
static gboolean Display_rda_status( int refresh );
static gboolean Display_previous_rda_state( int refresh );
static gboolean Display_rda_rdacnt( int refresh );


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
    GtkWidget *child;
    GtkWidget *scrolled;
    GSList *group;

    int ret;
    static Mrpg_state_t rpg_state;

    /* Set up RPG environment. */
    if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGMISC_init Failed\n" );
       exit(0);

    }

    /* Allocate buffer. */
    Buffer = calloc( 1, 6400 );
    if( Buffer == NULL ){

       /* Exit on failure. */
       LE_send_msg( GL_INFO, "calloc failed for 6400 bytes\n" );
       exit(0);

    }

    /* Set the initial buffer size. */
    Buf_size = 6400;

    /* Initialize GTK. */
    gtk_init (&argc, &argv);    

    /* Check the RPG state. */
    ret = ORPGMGR_get_RPG_states( &rpg_state );
    if( ret < 0 ){

       LE_send_msg( GL_INFO, "ORPGMGR_get_RPG_states Returned Error.\n" );
       Failure_and_exit();

    }
      
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

    gtk_window_set_title (GTK_WINDOW (Window), "Display General Status");
    gtk_container_set_border_width (GTK_CONTAINER (Window), 0);

    Box1 = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER (Window), Box1 );
    gtk_widget_show( Box1 );

    /* Add the close and refresh buttons. */
    table = gtk_table_new( 1, 4, TRUE );

    /* Define close section of the GUI. */
    {
       /* Make the "Close" button blue with white text. */
       button = gtk_button_new_with_label( "  Close  " );
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

       /* Make "Refresh" button blue with white text. */
       Refresh_button = gtk_button_new_with_label( " Refresh " );
       gtk_widget_modify_bg( Refresh_button, GTK_STATE_NORMAL, &Steelblue );
       child = gtk_bin_get_child( GTK_BIN(Refresh_button) );
       gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );
       gtk_button_set_relief( GTK_BUTTON( Refresh_button ), GTK_RELIEF_HALF );

       /* Attach the button to the table. */
       gtk_table_attach( GTK_TABLE(table), Refresh_button, 1, 2, 0, 1, 
                         GTK_EXPAND, GTK_FILL, 3, 3 );

       gtk_container_add( GTK_CONTAINER (Box1), table );

    }

    /* Add a separator. */
    separator = gtk_hseparator_new ();
    {
       gtk_box_pack_start (GTK_BOX (Box1), separator, FALSE, TRUE, 0);
    }

    /* Add the "SAILS Cuts" frame. */
    frame = gtk_frame_new( "General Status" );
    {
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);
       box2 = gtk_hbox_new (FALSE, 0);
       gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
       gtk_container_add( GTK_CONTAINER( frame ), box2 );

       /* Add the radio buttons for the various parts of General Status. */
       Button1 = gtk_radio_button_new_with_label( NULL, "Volume Status" );
       gtk_box_pack_start( GTK_BOX (box2), Button1, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( Button1 ) );

       Button2 = gtk_radio_button_new_with_label( group, "RDA RDACNT" );
       gtk_box_pack_start( GTK_BOX (box2), Button2, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button2) );

       Button3 = gtk_radio_button_new_with_label( group, "Previous RDA State" );
       gtk_box_pack_start( GTK_BOX (box2), Button3, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button3) );

       Button4 = gtk_radio_button_new_with_label( group, "RDA Status" );
       gtk_box_pack_start( GTK_BOX (box2), Button4, TRUE, TRUE, 5 );

       /* Make toggle button 4 active. */
       gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button4 ), TRUE );
    }

    /* Add the "RDA VCP" frame. */
    frame = gtk_frame_new( "Info" );
    {
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);

       /* Create a scrolled window. */
       scrolled = gtk_scrolled_window_new( NULL, NULL );
       gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

       /* Create a label containing the VCP information and add to the scrolled window. */
       Info_label = gtk_label_new( "Information will be displayed here!" );
       gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( scrolled ),
                                              GTK_WIDGET( Info_label ) );
       gtk_widget_set_size_request( scrolled, 400, 400 ); 

       /* Add the scrolled window to the frame container. */
       gtk_container_add( GTK_CONTAINER(frame), scrolled );


    }

    gtk_widget_show_all( Window );
     
    /* Define callback functions. */
    g_signal_connect_swapped( Button1, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "Volume Status" );
    g_signal_connect_swapped( Button2, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "RDA RDACNT" );
    g_signal_connect_swapped( Button3, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "Previous RDA State" );
    g_signal_connect_swapped( Button4, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "RDA Status" );
    g_signal_connect_swapped( Refresh_button, "clicked",
                              G_CALLBACK( Refresh_callback ),
                              "REFRESH" );

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

   static int first_time = 1;
   int ret;

   /* First time called. */
   if( first_time || Refresh ){

      if( (ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button1 ))) == TRUE )
         Display_volume_status( Refresh );

      else if( (ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button2 ))) == TRUE )
         Display_volume_status( Refresh );

      else if( (ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button3 ))) == TRUE )
         Display_volume_status( Refresh );

      else if( (ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button4 ))) == TRUE )
         Display_rda_status( Refresh );

      first_time = 0;
      Refresh = 0;

   }
      
   return TRUE;

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for the Refresh button.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Refresh_callback( gpointer data ){

   if( strstr( (gchar *) data, "REFRESH" ) )
      Refresh = 1;

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
      Callback function for closing a top-level window.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Quit_vcp( GtkWidget *widget, GdkEvent  *event,
                          gpointer   data ){

  /* Destroy this widget. */
  gtk_widget_destroy( Window_vcp );

  /* Set the widget to NULL. */
  gtk_widget_destroyed( Window_vcp, &Window_vcp );
  if( Window_vcp != NULL )
     g_print( "Window VCP GtkWidget pointer NOT NULL" );
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
      Callback function for SAILS Cuts Radio Buttons.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Radio_action_callback( gpointer data ){

   int ret = 0;

   /* Check the passed data. */
   if( strstr( (gchar *) data, "Volume Status" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button1 ) );
      if( ret == 0 )
         return FALSE;

      else{

         /* Display Volume Status. */
         Active_button = Button1;
         return( Display_volume_status( 0 ) );

      }

   }
   else if( strstr( (gchar *) data, "RDA RDACNT" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button2 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button2;
         return( Display_rda_rdacnt( 0 ) );

      }

   }
   else if( strstr( (gchar *) data, "Previous RDA State" ) ){

      ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button3 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button3;
         return( Display_previous_rda_state( 0 ) );

      }

   }
   else if( strstr( (gchar *) data, "RDA Status" ) ){

      ret = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button4 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button4;
         return( Display_rda_status( 0 ) );

      }

   }
   else
     g_print( "Bad Value in radio_action: value = %s\n", (gchar *) data );

   return TRUE;

}

/*************************************************************

   Description:
      Function that displays the volume status.

*************************************************************/
static gboolean Display_volume_status( int refresh ){

   Vol_stat_gsm_t gsm, *ret = NULL;
   time_t cv_time;
   int i, year, mon, day, hr, min, sec;
   char text[256], temp[256], str[10];

   GtkWidget *popup = NULL;

   /* Read the volume status data. */
   ret = (Vol_stat_gsm_t *) ORPGVST_read( (char *) &gsm );
   if( ret == NULL )
      return -1;

   /* Initialize buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Volume Number. */
   sprintf( text, " <b>Volume Number:</b>         %d (Seq: %ld)\n", 
            ORPGMISC_vol_scan_num( gsm.volume_number ), gsm.volume_number );
   Add_text( text );

   /* Volume scan date/time. */
   cv_time = (gsm.cv_julian_date-1)*86400 + gsm.cv_time / 1000;
   unix_time( &cv_time, &year, &mon, &day, &hr, &min, &sec );

   year -= 2000;
   sprintf( text, " <b>Volume Date/Time:</b>      %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );
   Add_text( text );

   /* Initial volume scan? */
   if( gsm.initial_vol )
      sprintf( text, " <b>Initial Vol:</b>           Yes\n" );
   else
      sprintf( text, " <b>Initial Vol:</b>           No\n" );

   Add_text( text );

   /* Previous volume status. */
   if( gsm.pv_status )
      sprintf( text, " <b>Previous Vol:</b>          Completed\n" );
   else
      sprintf( text, " <b>Previous Vol:</b>          Aborted\n" );

   Add_text( text );

   /* Expected volume duration. */
   sprintf( text, " <b>Volume Duration:</b>       %3d secs\n", gsm.expected_vol_dur );
   Add_text( text );

   /* Expected volume scan number. */
   sprintf( text, " <b>Volume Scan:</b>           %2d\n", gsm.volume_scan );
   Add_text( text );

   /* Mode of operation. */
   if( gsm.mode_operation == MAINTENANCE_MODE )
      sprintf( text, " <b>Mode:</b>                  Maintenance\n" );
   else if( gsm.mode_operation == CLEAR_AIR_MODE )
      sprintf( text, " <b>Mode:</b>                  Clear Air\n" );
   else if( gsm.mode_operation == PRECIPITATION_MODE )
      sprintf( text, " <b>Mode:</b>                  Precipitation\n" );
   else
      sprintf( text, " <b>Mode:</b>                  ???????\n" );

   Add_text( text );

   /* Dual Pol expected? */
   if( gsm.dual_pol_expected )
      sprintf( text, " <b>Dual Pol Expected:</b>     Yes\n" );
   else
      sprintf( text, " <b>Dual Pol Expected:</b>     No\n" );

   Add_text( text );

   /* VCP. */
   sprintf( text, " <b>VCP:</b>                   %3d\n", gsm.vol_cov_patt );
   Add_text( text );

   /* RPGVCPID. */
   sprintf( text, " <b>VCP ID:</b>                %3d\n", gsm.rpgvcpid );
   Add_text( text );

   /* Number of elevation cuts. */
   sprintf( text, " <b>Number Cuts:</b>           %2d\n", gsm.num_elev_cuts );
   Add_text( text );

   /* Number of SAILS cuts. */
   sprintf( text, " <b>Number SAILS Cuts:</b>     %2d\n", gsm.n_sails_cuts );
   Add_text( text );

   /* AVSET Termination Angle. */
   sprintf( text, " <b>AVSET Term Angle:</b>      %3d\n", gsm.avset_term_ang );
   Add_text( text );

   /* Elevations. */
   memset( temp, 0, 128 );
   memset( str, 0, 10 );
   for( i = 0; i < gsm.num_elev_cuts; i++ ){

      sprintf( str, "%4.1f ", (float) gsm.elevations[i]/10.0 );
      strcat( temp, str );
      if( (i > 0) && ((i%6) == 0) )
         strcat( temp, "\n                           " );

   }

   sprintf( text, " <b>Elevations (deg):</b>      %s\n", temp );
   Add_text( text );

   /* RPG elevation index. */
   memset( temp, 0, 128 );
   memset( str, 0, 10 );
   for( i = 0; i < gsm.num_elev_cuts; i++ ){

      sprintf( str, "%2d ", gsm.elev_index[i] );
      strcat( temp, str );

   }

   sprintf( text, " <b>RPG Elev Index:</b>        %s\n", temp );
   Add_text( text );

   /* SAILs cut sequence numbers. */
   if( gsm.n_sails_cuts > 0 ){

      memset( temp, 0, 128 );
      memset( str, 0, 10 );
      for( i = 0; i < gsm.num_elev_cuts; i++ ){

         sprintf( str, "%2d ", gsm.sails_cut_seq[i] );
         strcat( temp, str );

      }

      sprintf( text, " <b>SAILS Cut Seq:</b>         %s\n", temp );
      Add_text( text );

   }

   /* Super Resolution cuts. */
   sprintf( text, " <b>Super Res Bit Map:</b>     %x\n", gsm.super_res_cuts );
   Add_text( text );

   /* VCP Supplemental Data. */
   sprintf( text, " <b>VCP Supplemental:</b>\n" );
   Add_text( text );

   if( gsm.vcp_supp_data & VSS_AVSET_ENABLED )
      sprintf( text, "    <b>AVSET Enabled:</b>      Yes\n" );
   else
      sprintf( text, "    <b>AVSET Enabled:</b>      No\n" );

   Add_text( text );

   if( gsm.vcp_supp_data & VSS_SAILS_ACTIVE )
      sprintf( text, "    <b>SAILS Active:</b>       Yes\n" );
   else
      sprintf( text, "    <b>SAILS Active:</b>       No\n" );

   Add_text( text );

   if( gsm.vcp_supp_data & VSS_SITE_SPECIFIC_VCP )
      sprintf( text, "    <b>Site-Specific VCP:</b>  Yes\n" );
   else
      sprintf( text, "    <b>Site-Specific VCP:</b>  No\n" );

   Add_text( text );

   /* Change label within scrolled window. */
   Add_text( "</span>" );
   gtk_label_set_markup( GTK_LABEL(Info_label), (gchar *) Buffer );

   /* Is the Window_vcp already defined? */
   if( Window_vcp == NULL ){

      /* Create a dialog ... ask if the VCP data is to be displayed. */
      popup = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
                                      "Do you want to display VCP data?" );

      /* Run the dialog ... function returns when operator responds. */
      int result = gtk_dialog_run( GTK_DIALOG(popup) );
      switch( result ){

         /* On YES, set adaptation data to the selected value. */
         case GTK_RESPONSE_YES:
         {
            /* Current VCP. */
            if( Window_vcp == NULL )
               Write_vcp_data( &gsm.current_vcp_table, FROM_VOLUME_STATUS, 1 );
            break;

         }
         default:
         {
            /* On No, activate the radio button previously active. */
            break;
         }

      }

      /* Destory the popup widget. */
      gtk_widget_destroy( popup );

   }
   else if( refresh )
      Write_vcp_data( &gsm.current_vcp_table, FROM_VOLUME_STATUS, 0 );


   return TRUE;
}


/*\//////////////////////////////////////////////////////////////

   Description:
      This function adds "text" to Buffer.

//////////////////////////////////////////////////////////////\*/
static int Add_text( char *text ){

   if( (Text_size + strlen(text)) > Buf_size ){

      /* Reallocate a larger buffer. */
      Buffer = realloc( Buffer, Buf_size*2 ); 
      if( Buffer == NULL ){

         exit(1);

      }

      Buf_size *= 2;

   }

   strcat( Buffer, text );
   Text_size += strlen( text );

   return 0;
}

/********************************************************************************
   
   Description:
      Writes out the VCP definition.

   Inputs:
      vcp - pointer to VCP data ... format specified in vcp.h.

********************************************************************************/
static void Write_vcp_data( Vcp_struct *vcp, int from, int create ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };
   static char text[256];

   int i, expected_size;
   short wform, phse;

   /* Initialize Buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Write out VCP data. */
   sprintf( text, "\n\n<b>VCP %d Data:</b>\n", vcp->vcp_num );
   Add_text( text );

   sprintf( text, " <b>VCP Header:</b>\n" );
   Add_text( text );

   sprintf( text, "    Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
            vcp->msg_size, vcp->type, vcp->n_ele );
   Add_text( text );

   sprintf( text, "    Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n",
            vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ],
            width[ vcp->pulse_width/4 ] );
   Add_text( text );

   /* Do some validation. */
   expected_size = VCP_ATTR_SIZE + vcp->n_ele*(sizeof(Ele_attr)/sizeof(short));
   if( vcp->msg_size != expected_size )
      g_print( "VCP Size: %d Not Expected: %d\n", vcp->msg_size, expected_size );

   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );
      float test_angle = 0.5;

      wform = elev->wave_type;
      phse = elev->phase;

      /* Color the text for the lowest elevation angle Green. */
      if( i == 0 )
         test_angle = elev_angle;

      sprintf( text, " <b>\nElevation %d:</b>\n", i+1 );
      Add_text( text );

      if( test_angle == elev_angle )
         sprintf( text, 
             "<span background=\"forest green\" foreground=\"white\">    Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res: %3d, Surv PRF: %2d   Surv Pulses: %4d</span>\n",
                   elev_angle, wave_form[ wform ], phase[ phse ],
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      else
         sprintf( text, 
             "    Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res: %3d, Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ],
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      Add_text( text );

      sprintf( text, "    Az Rate: %5.2f (0x%4x BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate*ORPGVCP_AZIMUTH_RATE_FACTOR, elev->azi_rate, (float) elev->surv_thr_parm/8.0,
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );
      Add_text( text );

      sprintf( text, "    PRF Sector 1:\n" );
      Add_text( text );

      if( from == FROM_VOLUME_STATUS )
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      (float) elev->azi_ang_1 / 10.0, elev->dop_prf_num_1, elev->pulse_cnt_1 );
      else
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_1 ),
                      elev->dop_prf_num_1, elev->pulse_cnt_1 );

      Add_text( text );

      sprintf( text, "    PRF Sector 2:\n" );
      Add_text( text );

      if( from == FROM_VOLUME_STATUS )
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      (float) elev->azi_ang_2 / 10.0, elev->dop_prf_num_2, elev->pulse_cnt_2 );
      else
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                      ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_2 ),
                      elev->dop_prf_num_2, elev->pulse_cnt_2 );

      Add_text( text );

      sprintf( text, "    PRF Sector 3:\n" );
      Add_text( text );

      if( from == FROM_VOLUME_STATUS )
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                     (float) elev->azi_ang_3 / 10.0, elev->dop_prf_num_3, elev->pulse_cnt_3 );
      else
         sprintf( text, "        Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                     ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_3 ),
                     elev->dop_prf_num_3, elev->pulse_cnt_3 );

      Add_text( text );

   }

   /* Close the markup. */
   Add_text( "</span>" );

   /* Does the VCP window need to be created? */
   if( create ){

      GtkWidget *table = NULL;
      GtkWidget *button = NULL;
      GtkWidget *child = NULL;
      GtkWidget *separator = NULL;

      /* Create the top-level window. */
      Window_vcp = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      /* We don't want the maximum button ... just minimize and 
         close. */
      gtk_window_set_resizable( GTK_WINDOW( Window_vcp ), FALSE );

      /* Set the background color to "peachpuff3" */
      gtk_widget_modify_bg( Window_vcp, GTK_STATE_NORMAL, &Canvas );

      /* Set up the close application callback.  This function is called 
         when the X button in the upper right corner is pressed. */
      g_signal_connect( Window_vcp, "delete-event",
                        G_CALLBACK (Quit_vcp),
                        NULL );

      gtk_window_set_title (GTK_WINDOW (Window_vcp), "SAILS Status/Control");
      gtk_container_set_border_width (GTK_CONTAINER (Window_vcp), 0);

      Box_vcp = gtk_vbox_new( FALSE, 0 );
      gtk_container_add( GTK_CONTAINER (Window_vcp), Box_vcp );

      /* Add the close button. */
      table = gtk_table_new( 1, 4, TRUE );

      /* Add a Close button. */
      button = gtk_button_new_with_label( "  Close  " );
      {
         gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
         child = gtk_bin_get_child( GTK_BIN(button) );
         gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );
         gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

         /* Attach the table to the Box. */
         gtk_box_pack_start (GTK_BOX (Box_vcp), table, FALSE, TRUE, 0);

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 0, 1, 0, 1, 
                           GTK_EXPAND, GTK_FILL, 3, 3 );

         /* Define signal handler for the "Close" button. */
         g_signal_connect( button, "clicked",
                           G_CALLBACK (Quit_vcp),
                           Window_vcp );
      }

      /* Add a separator. */
      separator = gtk_hseparator_new ();
      {
         gtk_box_pack_start (GTK_BOX (Box_vcp), separator, FALSE, TRUE, 0);
      }

      /* Add the "VCP Data" frame. */
      Frame_vcp = gtk_frame_new( "VCP Data" );
      {
         gtk_box_pack_start( GTK_BOX(Box_vcp), Frame_vcp, TRUE, TRUE, 10);

         /* Create a scrolled window. */
         Scrolled_vcp = gtk_scrolled_window_new( NULL, NULL );
         gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( Scrolled_vcp ),
                                         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

         /* Create a label containing the VCP information and add to the scrolled window. */
         Label_vcp = gtk_label_new( "VCP data will be displayed here!" );
         gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( Scrolled_vcp ),
                                                GTK_WIDGET( Label_vcp ) );
         gtk_widget_set_size_request( Scrolled_vcp, 875, 475 );

         /* Add the scrolled window to the frame container. */
         gtk_container_add( GTK_CONTAINER(Frame_vcp), Scrolled_vcp );

      }

   }

   /* Display the markup text. */
   gtk_label_set_markup( GTK_LABEL( Label_vcp ), (gchar *) Buffer );

   gtk_widget_show_all( Window_vcp );

   return;

}


/*******************************************************************************
 
  Description:
        Writes Open RDA status data in plain text format.  
 
  Return:
        TRUE
*******************************************************************************/
static gboolean Display_rda_status( int refresh ){

   int stat = 0;
   int i, hw = 0;
   int rda_stat = 0;
   int op_stat = 0;
   int control_stat = 0;
   int aux_pwr_stat = 0;
   int data_trans_enab = 0;
   int vcp = 0;
   int rda_contr_auth = 0;
   int opmode = 0;
   int chan_stat = 0;
   int spot_blank_stat = 0;
   int tps_stat = 0;
   int perf_check_status = 0;

   int avg_trans_pwr = 0;
   int h_ref_dBZ0 = 0;
   int v_ref_dBZ0 = 0;
   int rda_build_num = 0;
   int super_reso = 0;
   int cmd_status = 0;
   int avset_status = 0;
   int rda_alarm_sum = 0;
   int rda_command_ack = 0;
   int bpm_gen_date = 0;
   int bpm_gen_time = 0;
   int clm_gen_date = 0;
   int clm_gen_time = 0;
   int rms_stat = 0;

   int wblnstat = 0;
   int display_blanking = 0;
   int wb_failed = 0;

   double deau_ret_val = 0.0;
   char text[256];

   /* Initialize Buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Print header string. */
   sprintf( text, "<b>Wideband Comms Status:</b>\n" );
   Add_text( text );

   /* Get line status information. */
   wblnstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
   if( wblnstat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_WBLNSTAT\n" );

   display_blanking = ORPGRDA_get_wb_status( ORPGRDA_DISPLAY_BLANKING );   
   if( display_blanking == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_DISPLAY_BLANKING\n" );

   wb_failed = ORPGRDA_get_wb_status( ORPGRDA_WBFAILED );
   if( wb_failed == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_WB_FAILED\n" );

   /* Process wideband line status. */
   if( (wblnstat == RS_NOT_IMPLEMENTED) )
      i = 0;

   else if( wblnstat == RS_CONNECT_PENDING )
      i = 1;

   else if( wblnstat == RS_DISCONNECT_PENDING )
      i = 2;

   else if( wblnstat == RS_DISCONNECTED_HCI )
      i = 3;

   else if( wblnstat == RS_DISCONNECTED_CM )
      i = 4;

   else if( wblnstat == RS_DISCONNECTED_SHUTDOWN )
      i = 5;

   else if( wblnstat == RS_CONNECTED )
      i = 6;

   else if( wblnstat == RS_DOWN )
      i = 7;

   else if( wblnstat == RS_WBFAILURE )
      i = 8;

   else if( wblnstat == RS_DISCONNECTED_RMS )
      i = 9;

   else{

      /* Unknown value.  Place value in status buffer. */
      i = 10;
      sprintf( wbstat[i], "%6d", wblnstat );

   }

   /* Print Wideband Line Status information. */
   if( (wblnstat == RS_NOT_IMPLEMENTED)
                 ||
       (wblnstat == RS_DOWN)
                 ||
       (wblnstat == RS_WBFAILURE) )
      sprintf( text, 
               "    WB Line Status:           <span foreground=\"red\">%s</span>\n", 
               wbstat[i] );

   else if( (wblnstat == RS_CONNECT_PENDING)
                      ||
            (wblnstat == RS_DISCONNECT_PENDING)
                      ||
            (wblnstat == RS_DISCONNECTED_HCI)
                      ||
            (wblnstat == RS_DISCONNECTED_RMS)
                      ||
            (wblnstat == RS_DISCONNECTED_CM)
                      ||
            (wblnstat == RS_DISCONNECTED_SHUTDOWN) )
      sprintf( text, 
               "    WB Line Status:           <span foreground=\"yellow\">%s</span>\n", 
               wbstat[i] );

   else if( wblnstat == RS_CONNECTED )
      sprintf( text, 
               "    WB Line Status:           <span foreground=\"green\">%s</span>\n", 
               wbstat[i] );

   else 
      sprintf( text, 
               "    WB Line Status:           %s\n", 
               wbstat[i] );
   
   Add_text( text );

   if( display_blanking )
      sprintf( text, "    Display Blanking:         True\n" );

   else
      sprintf( text, "    Display Blanking:         False\n" );

   Add_text( text );

   if( wb_failed )
      sprintf( text, "    WB Failed:                True\n" );

   else
      sprintf( text, "    WB Failed:                False\n" );

   Add_text( text );

   /* Put a blank line .... */
   Add_text( "\n" );

   /* If the wideband is not connected, publish the line status
      and return. */
   if( wblnstat != RS_CONNECTED ){

      /* Put some blank lines .... */
      Add_text( "\n\n</span>" );

      /* Display the text. */
      gtk_label_set_markup( GTK_LABEL(Info_label), (gchar *) Buffer );

      return TRUE;

   }
   
   /* Print RDA Status information. */

   /* Print header string. */
   sprintf( text, "<b>RDA Status:</b>\n" );
   Add_text( text );

   /* Retrieve and store current rda status fields */
   rda_stat = ORPGRDA_get_status( ORPGRDA_RDA_STATUS );
   if ( rda_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_RDA_STATUS\n" );
    
   op_stat = ORPGRDA_get_status( ORPGRDA_OPERABILITY_STATUS );
   if ( op_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_OPERABILITY_STATUS\n" );
    
   control_stat = ORPGRDA_get_status( ORPGRDA_CONTROL_STATUS );
   if ( control_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_CONTROL_STATUS\n" );

   aux_pwr_stat = ORPGRDA_get_status( ORPGRDA_AUX_POWER_GEN_STATE );
   if ( aux_pwr_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_AUX_POWER_GEN_STATE\n" );

   data_trans_enab = ORPGRDA_get_status( ORPGRDA_DATA_TRANS_ENABLED );
   if ( data_trans_enab == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_DATA_TRANS_ENABLED\n" );

   vcp = ORPGRDA_get_status( ORPGRDA_VCP_NUMBER );
   if ( vcp == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_VCP_NUMBER\n" );

   rda_contr_auth = ORPGRDA_get_status( ORPGRDA_RDA_CONTROL_AUTH );
   if ( rda_contr_auth == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_RDA_CONTROL_AUTH\n" );

   opmode = ORPGRDA_get_status( ORPGRDA_OPERATIONAL_MODE );
   if ( opmode == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_OPERATIONAL_MODE\n" );

   chan_stat = ORPGRDA_get_status( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( chan_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_CHAN_CONTROL_STATUS\n" );

   spot_blank_stat = ORPGRDA_get_status( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( spot_blank_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_SPOT_BLANKING_STATUS\n" );

   tps_stat = ORPGRDA_get_status( ORPGRDA_TPS_STATUS );
   if ( tps_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_TPS_STATUS\n" );

   perf_check_status = ORPGRDA_get_status( ORPGRDA_PERF_CHECK_STATUS );
   if ( perf_check_status == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_PERF_CHECK_STATUS\n" );

   avg_trans_pwr = ORPGRDA_get_status( ORPGRDA_AVE_TRANS_POWER );
   if( avg_trans_pwr == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_AVE_TRANS_POWER" );

   h_ref_dBZ0 = ORPGRDA_get_status( ORPGRDA_REFL_CALIB_CORRECTION );
   if( h_ref_dBZ0 == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_REFL_CALIB_CORRECTION" );

   v_ref_dBZ0 = ORPGRDA_get_status( ORPGRDA_VC_REFL_CALIB_CORRECTION );
   if( v_ref_dBZ0 == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_VC_REFL_CALIB_CORRECTION" );

   rda_build_num = ORPGRDA_get_status( ORPGRDA_RDA_BUILD_NUM );
   if( rda_build_num == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_RDA_BUILD_NUM" );

   super_reso = ORPGRDA_get_status( ORPGRDA_SUPER_RES );
   if( super_reso == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_SUPER_RES" );

   cmd_status = ORPGRDA_get_status( ORPGRDA_CMD );
   if( cmd_status == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_CMD" );

   avset_status = ORPGRDA_get_status( ORPGRDA_AVSET );
   if( avset_status == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_AVSET" );

   rda_alarm_sum = ORPGRDA_get_status( ORPGRDA_RDA_ALARM_SUMMARY );
   if( rda_alarm_sum == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_RDA_ALARM_SUMMARY" );

   rda_command_ack = ORPGRDA_get_status( ORPGRDA_COMMAND_ACK );
   if( rda_command_ack == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_COMMAND_ACK" );

   bpm_gen_date = ORPGRDA_get_status( ORPGRDA_BPM_GEN_DATE );
   if( bpm_gen_date == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_BPM_GEN_DATE" );

   bpm_gen_time = ORPGRDA_get_status( ORPGRDA_BPM_GEN_TIME );
   if( bpm_gen_time == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_BPM_GEN_TIME" );

   clm_gen_date = ORPGRDA_get_status( ORPGRDA_NWM_GEN_DATE );
   if( clm_gen_date == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_NWM_GEN_DATE" );

   clm_gen_time = ORPGRDA_get_status( ORPGRDA_NWM_GEN_TIME );
   if( clm_gen_time == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_NWM_GEN_TIME" );

   rms_stat = ORPGRDA_get_status( ORPGRDA_RMS_CONTROL_STATUS );
   if( rms_stat == ORPGRDA_DATA_NOT_FOUND )
      g_print( "Could not retrieve ORPGRDA_RMS_CONTROL_STATUS" );

   /*
      Output status data is readable format. NOTE: range for hw should start at
      the value of the macro representing the first field in the status msg
      (defined in rda_status.h)
   */
   for( hw = 1; hw <= STATUS_WORDS; hw++ ){

      switch( hw ){

         case RS_RDA_STATUS:
         {
            int i;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( status[i], "%6d", rda_stat );

            }

            sprintf( text, "    RDA Status:               %s\n", status[i] );
            Add_text( text );
            break;

         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability;
            int i;

            /* Process operability status. */
            rda_operability = (unsigned short) op_stat;

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
              /* Unknown value.  Place value in status buffer. */
               i = 6;
               sprintf( operability[i], "%6d", op_stat );
            }

            sprintf( text, "    Operability Status:       %s\n", operability[i] );
            Add_text( text );
            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            /* Process RDA Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( control[i], "%6d", control_stat );

            }

            sprintf( text, "    RDA Control Status:       %s\n", control[i] );
            Add_text( text );
            break;

         }

         case RS_AUX_POWER_GEN_STATE:
         {

            int i;
            short test_bit, curr_bit;
            char temp[MAX_STATUS_LENGTH];

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH );

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );

            }

            sprintf( text, "    RDA Aux Power:            %s\n", temp );
            Add_text( text );
            break;

         }

         case RS_AVE_TRANS_POWER:
         {

            /* Process Average Transmitter Power. */
            sprintf( text, "    RDA Ave Trans Pwr:        %4d\n", avg_trans_pwr );
            Add_text( text );
            break;
         }

         case RS_REFL_CALIB_CORRECTION:
         {

            /* Process Horizontal dBZ0. */
            sprintf( text, "    H dBZ0:                   %7.2f\n", (float) h_ref_dBZ0 / 100.0 );
            Add_text( text );
            break;

         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );

               }

               if( (data_trans_enab & BD_WIDTH) ){

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );

               }

            }

            sprintf( text, "    Data Enabled:             %s\n", moment_string );
            Add_text( text );
            break;

         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );
            sprintf( text, "    VCP:                      %s\n", temp );
            Add_text( text );
            break;

         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            if( rda_contr_auth == CA_NO_ACTION )
               i = 0;

            else if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 1;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               sprintf( authority[i], "%6d", rda_contr_auth );

            }

            sprintf( text, "    Cntrl Auth:               %s\n", authority[i] );
            Add_text( text );
            break;

         }

         case RS_RDA_BUILD_NUM: 
         {

            float num;

            if( (num = (float) rda_build_num / 100.0f) > 2.0f ) 
               sprintf( text, "    RDA Build #:              %4.2f\n", num  );

            else{
  
               num = (float) rda_build_num / 10.0f;
               sprintf( text, "    RDA Build #:              %4.2f\n", num  );

            }

            Add_text( text );
            break;

         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            /* Process operational mode. */
            if( opmode == OP_MAINTENANCE_MODE )
               i = 1;

            else if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else if( opmode == OP_OFFLINE_MAINTENANCE_MODE )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( orda_mode[i], "%6d", opmode );

            }

            sprintf( text, "    Mode:                     %s\n", orda_mode[i] );
            Add_text( text );
            break;

         }

         case RS_SUPER_RES:
         {

            int i = 0;

            /* Process Super Resolution. */
            if( super_reso == SR_ENABLED )
               i = 0;

            else if( super_reso == SR_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( super_res[i], "%6d", super_reso );

            }

            sprintf( text, "    Super Reso:               %s\n", super_res[i] );
            Add_text( text );
            break;

         }

         case RS_CMD:
         {

            int i = 0;

            /* Process CMD Status. */
            if( cmd_status & 0x1 ){

               i = 0;
               sprintf( text, "    CMD:                      %s (%x)\n", 
                        cmd[0], cmd_status/2 );

            }
            else 
               sprintf( text, "    CMD:                      %s\n", cmd[1] );

            Add_text( text );
            break;

         }

         case RS_AVSET:
         {

            int i = 0;

            /* Process AVSET Status. */
            if( avset_status == AVSET_ENABLED )
               i = 0;

            else if( avset_status == AVSET_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               sprintf( avset[i], "%6d", avset_status );

            }

            sprintf( text, "    AVSET:                    %s\n", avset[i] );
            Add_text( text );
            break;

         }

         case RS_RDA_ALARM_SUMMARY:
         {

            /* Process RDA Alarm Summary. */
            if( rda_alarm_sum == 0 ){

               sprintf( text, "    RDA Alarm Summary:        %s\n", alarm_sum[0] );
               Add_text( text );

            }
            else{ 
   
               sprintf( text, "    RDA Alarm Summary (%4x):\n", rda_alarm_sum );
               Add_text( text );

               if( rda_alarm_sum & 0x2 ){
                  sprintf( text, "                              %s\n", alarm_sum[1] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x4 ){
                  sprintf( text, "                              %s\n", alarm_sum[2] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x8 ){
                  sprintf( text, "                              %s\n", alarm_sum[3] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x10 ){
                  sprintf( text, "                              %s\n", alarm_sum[4] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x20 ){
                  sprintf( text, "                              %s\n", alarm_sum[5] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x40 ){
                  sprintf( text, "                              %s\n", alarm_sum[6] );
                  Add_text( text );
               }

               if( rda_alarm_sum & 0x80 ){
                  sprintf( text, "                              %s\n", alarm_sum[7] );
                  Add_text( text );
               }

            }

            break;

         }

         case RS_COMMAND_ACK:
         {

            /* Process Command Acknowledgement. */
            if( rda_command_ack != 0 ){

               if( rda_command_ack == 1 )
                  sprintf( text, "    RDA Command Ack:          %s\n", cmd_ack[0] );

               else if( rda_command_ack == 2 )
                  sprintf( text, "    RDA Command Ack:          %s\n", cmd_ack[1] );

               else if( rda_command_ack == 3 )
                  sprintf( text, "    RDA Command Ack:          %s\n", cmd_ack[2] );

               else if( rda_command_ack == 4 )
                  sprintf( text, "    RDA Command Ack:          %s\n", cmd_ack[3] );

               Add_text( text );

            }

            break;

         }
 
         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            if ( (stat = DEAU_get_values("Redundant_info.redundant_type", 
                                         &deau_ret_val, 1)) >= 0){

               /* Process channel control status if FAA Redundant. */
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  sprintf( channel_status[i], "%6d", chan_stat );
               }
            }
            else
              g_print( "call to DEAU_get_values returned error.\n" );

            sprintf( text, "    Chan Cntrl:               %s\n", channel_status[i] );
            Add_text( text );
            break;

         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               sprintf( spot_blanking[i], "%6d", spot_blank_stat );

            }

            sprintf( text, "    Spot Blank:               %s\n", spot_blanking[i] );
            Add_text( text );
            break;

         }
         case RS_TPS_STATUS:
         {

            int i;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( tps[i], "%6d", tps_stat );

            }

            sprintf( text, "    TPS:                      %s\n", tps[i] );
            Add_text( text );
            break;

         }

         case RS_BPM_GEN_DATE:
         {

            int yr, mo, dy, hr, m, s;
            time_t bpm_time = (bpm_gen_date-1)*86400 + bpm_gen_time*60;
            
            unix_time( &bpm_time, &yr, &mo, &dy, &hr, &m, &s );
            if( yr >= 2000 )
               yr -= 2000;
            else
               yr -= 1900;

            sprintf( text, "    BPM Gen Time:             %02d:%02d:%02d %02d/%02d/%02d\n",
                     hr, m, s, mo, dy, yr );
            Add_text( text );
            break;

         }

         case RS_NWM_GEN_DATE:
         {

            int yr, mo, dy, hr, m, s;
            time_t clm_time = (clm_gen_date-1)*86400 + clm_gen_time*60;
            
            unix_time( &clm_time, &yr, &mo, &dy, &hr, &m, &s );
            if( yr >= 2000 )
               yr -= 2000;
            else
               yr -= 1900;

            sprintf( text, "    CLM Gen Time:             %02d:%02d:%02d %02d/%02d/%02d\n",
                     hr, m, s, mo, dy, yr );
            Add_text( text );
            break;

         }

         case RS_VC_REFL_CALIB_CORRECTION:
         {

            /* Process Vertical dBZ0. */
            sprintf( text, "    V dBZ0:                   %7.2f\n", (float) v_ref_dBZ0 / 100.0 );
            Add_text( text );
            break;

         }

         case RS_RMS_CONTROL_STATUS:
         {

            /* Process RMS Control status. */
            if( rms_stat == 0 )
               break;

            if( rms_stat == 2 )
               sprintf( text, "    RMS Cntrl Status:         %s\n", rms[0] );

            else if( rms_stat == 4 )
               sprintf( text, "    RMS Cntrl Status:         %s\n", rms[1] );

            Add_text( text );
            break;

         }

         case RS_PERF_CHECK_STATUS:
         {

            int i;

            /* Process Performance Check status. */
            if( perf_check_status == PC_AUTO )
               i = 0;

            else if( perf_check_status == PC_PENDING )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               sprintf( perf_check[i], "%6d", perf_check_status );

            }

            sprintf( text, "    Perf Check:               %s\n", perf_check[i] );
            Add_text( text );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Put some blank lines .... */
   Add_text( "\n\n</span>" );

   /* Display the text. */
   gtk_label_set_markup( GTK_LABEL(Info_label), (gchar *) Buffer );

   return TRUE;

} 

/*******************************************************************************
 
  Description:
     Writes Previous RDA state data in plain text.
 
  Inputs:
     none
 
  Return:
     void
*******************************************************************************/
static gboolean Display_previous_rda_state( int refresh ){

   int i = 0,ret = 0;
   int stat = 0;
   int p_rda_stat = 0;
   int p_vcp = 0;
   int p_rda_contr_auth = 0;
   int p_chan_control = 0;
   int p_spot_blank_stat = 0;
   Vcp_struct p_vcp_data = {0};
   int vcp_size = 0;
   double deau_ret_val = 0.0;
   char text[256];

   GtkWidget *popup = NULL;

   /* Read the previous state data. */
   ret = ORPGRDA_read_previous_state();
   if( ret < 0 ){

      g_print( "Read of Previous RDA Status Data Failed\n" );
      return TRUE;

   }

   /* Retrieve and store data fields */
   p_rda_stat = ORPGRDA_get_previous_state( ORPGRDA_RDA_STATUS );
   if ( p_rda_stat == ORPGRDA_DATA_NOT_FOUND ){

      g_print( "Problem retrieving ORPGRDA_RDA_STATUS\n" );
      p_rda_stat = 0; /* Reset value to 0 */

   }

   p_vcp = ORPGRDA_get_previous_state( ORPGRDA_VCP_NUMBER );
   if ( p_vcp == ORPGRDA_DATA_NOT_FOUND ){

      g_print( "Problem retrieving ORPGRDA_VCP_NUMBER\n" );
      p_vcp = 0;  /* Reset value to 0 */

   }

   p_rda_contr_auth = ORPGRDA_get_previous_state( ORPGRDA_RDA_CONTROL_AUTH );
   if ( p_rda_contr_auth == ORPGRDA_DATA_NOT_FOUND ){

      g_print( "Problem retrieving ORPGRDA_RDA_CONTROL_AUTH\n" );
      p_rda_contr_auth = 0;  /* Reset value to 0 */

   }

   p_chan_control = ORPGRDA_get_previous_state( ORPGRDA_CHAN_CONTROL_STATUS );
   if ( p_chan_control == ORPGRDA_DATA_NOT_FOUND ){

      g_print( "Problem retrieving ORPGRDA_CHAN_CONTROL_STATUS\n" );
      p_chan_control = 0;  /* Reset value to 0 */

   }

   p_spot_blank_stat = ORPGRDA_get_previous_state( ORPGRDA_SPOT_BLANKING_STATUS );
   if ( p_spot_blank_stat == ORPGRDA_DATA_NOT_FOUND ){

      g_print( "Problem retrieving ORPGRDA_SPOT_BLANKING_STATUS\n");
      p_spot_blank_stat = 0;  /* Reset value to 0 */

   }

   /* Initialize Buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   sprintf( text, "<b>Previous RDA State:</b>                    \n" );
   Add_text( text );

   /* case RS_RDA_STATUS  */
   if( (p_rda_stat & RS_STARTUP) )
      i = 0;

   else if( (p_rda_stat & RS_STANDBY) )
      i = 1;

   else if( (p_rda_stat & RS_RESTART) )
      i = 2;

   else if( (p_rda_stat & RS_OPERATE) )
      i = 3;

   else if( (p_rda_stat & RS_PLAYBACK) )
      i = 4;

   else if( (p_rda_stat & RS_OFFOPER) )
      i = 5;

   else{

      /* Unknown value. */
      i = 6;
      sprintf( status[i], "%6d", p_rda_stat );

   }

   sprintf( text, "    RDA Status:        %s\n", status[i] );
   Add_text( text );

   /* case RS_VCP_NUMBER */
   {
      char temp[10];

      /* Clear temporary buffer. */
      memset( temp, 0, 10 );

      /* Determine if vcp is "local" or "remote" pattern. */
      if( p_vcp < 0 ){

         p_vcp = -p_vcp;
         temp[0] = 'L';

      }
      else
         temp[0] = 'R';

      /* Encode VCP number. */
      sprintf( &temp[1], "%d", p_vcp );

      sprintf( text, "    RDA VCP:           %s\n", temp );
      Add_text( text );

   }

   /* case RS_RDA_CONTROL_AUTH */
   if( p_rda_contr_auth == CA_NO_ACTION )
      i = 0;

   else if( p_rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
      i = 1;

   else if( p_rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
      i = 2;

   sprintf( text, "    RDA Control Auth:  %s\n", authority[i] );
   Add_text( text );

   /* case RS_CHAN_CONTROL_STATUS */
   if ( (stat =
      DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1)) >= 0){

      if ( (int) deau_ret_val == ORPGSITE_FAA_REDUNDANT ){

         if( p_chan_control == RDA_IS_CONTROLLING )
            i = 0;

         else if( p_chan_control == RDA_IS_NON_CONTROLLING )
            i = 1;

         else{

            /* Unknown value.  Place value in status buffer. */
            i = 2;
            sprintf( channel_status[i], "%6d", p_chan_control );

         }

         sprintf( text, "    RDA Channel Cntrl: %s\n", channel_status[i] );
         Add_text( text );

      }

   }
   else
      g_print( "DEAU_get_values returned error.\n");

   /* case RS_SPOT_BLANKING_STATUS */
   /* If spot blanking not installed, break. */
   if( p_spot_blank_stat != SB_NOT_INSTALLED ){

      /* Process spot blanking status. */
      if( p_spot_blank_stat == SB_ENABLED )
         i = 1;

      else if( p_spot_blank_stat == SB_DISABLED )
         i = 2;

      else{

         /* Unknown value. Place value in status buffer. */
         i = 3;
         sprintf( spot_blanking[i], "%6d", p_spot_blank_stat );

      }

      sprintf( text, "    RDA Spot Blanking: %s\n", spot_blanking[i] );
      Add_text( text );

   }

   /* Change label within scrolled window. */
   Add_text( "</span>" );
   gtk_label_set_markup( GTK_LABEL(Info_label), (gchar *) Buffer );

   /* Is the Window_vcp already defined? */
   if( Window_vcp == NULL ){

      ret = ORPGRDA_get_previous_state_vcp( (char *) &p_vcp_data, &vcp_size );
      if( (ret != ORPGRDA_DATA_NOT_FOUND)
                     &&
              (vcp_size != 0) ) {


         /* Create a dialog ... ask if the VCP data is to be displayed. */
         popup = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
                                         "Do you want to display Previous State VCP data?" );

         /* Run the dialog ... function returns when operator responds. */
         int result = gtk_dialog_run( GTK_DIALOG(popup) );
         switch( result ){

            /* On YES, set adaptation data to the selected value. */
            case GTK_RESPONSE_YES:
            {
               /* Current VCP. */
               if( Window_vcp == NULL )
                  Write_vcp_data( &p_vcp_data, FROM_RDA_RDACNT, 1 );
               break;

            }
            default:
            {
               /* On No, activate the radio button previously active. */
               break;
            }

         } 

      }
     
      /* Destory the popup widget. */
      gtk_widget_destroy( popup );

   }
   else if( refresh )
      Write_vcp_data( &p_vcp_data, FROM_RDA_RDACNT, 0 );

   return TRUE;

}

/*************************************************************

   Description:
      Function that displays the RDA RDACNT data.

*************************************************************/
static gboolean Display_rda_rdacnt( int refresh ){

   int i, result, response, ret, year, mon, day, hr, min, sec;
   Vcp_struct *vcp = NULL;
   unsigned short *suppl = NULL;
   GtkWidget *popup = NULL;

   static RDA_rdacnt_t rdacnt;
   static char text[128], str[128];
   
   /* Read the data. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, &rdacnt,
                      sizeof(RDA_rdacnt_t), RDA_RDACNT );

   if( ret < sizeof(RDA_rdacnt_t) ){

      /* Create a dialog ... ask if the VCP data is to be displayed. */
      popup = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
                                      "Do you want to display RDA VCP data?" );

      /* Run the dialog ... function returns when operator responds. */
      result = gtk_dialog_run( GTK_DIALOG(popup) );

      /* Destory the popup widget. */
      gtk_widget_destroy( popup );

      return TRUE;

   }

   /* Initialize Buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Last index updated in rdacnt.data[]. */
   sprintf( text, "    <b>Last Entry (ind):</b>  %d\n", rdacnt.last_entry );
   Add_text( text );
   
   /* Last time (UTC) rdacnt was updated. */
   unix_time( &rdacnt.last_entry_time, &year, &mon, &day, &hr, &min, &sec );
   
   year -= 2000;
   sprintf( text, "    <b>Last Entry Time:</b>   %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );
   Add_text( text );

   /* Last time (UTC) VCP message updated. */
   unix_time( &rdacnt.last_message_time, &year, &mon, &day, &hr, &min, &sec );
   
   year -= 2000;
   sprintf( text, "    <b>Last Message Time:</b> %02d/%02d/%02d %02d:%02d:%02d\n",
            mon, day, year, hr, min, sec );
   Add_text( text );

   response = rdacnt.last_entry;

   /* Volume Scan number. */
   vcp = (Vcp_struct *) &rdacnt.data[response].rdcvcpta[0];
   sprintf( text, "\n    <b>Volume Scan #:</b>     %2d, <b>VCP:</b>      %3d\n",
            rdacnt.data[response].volume_scan_number, vcp->vcp_num );
   Add_text( text );

   /* Supplement Flags and RDA to RPG Elevation mapping. */
   sprintf( text, "    <b>VCP Supplemental Flags and RPG Elev Index:</b>\n" );
   Add_text( text );

   suppl = (unsigned short *) &rdacnt.data[response].suppl[0];
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *ele = (Ele_attr *) &vcp->vcp_ele[i][0];
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, ele->ele_angle );

      /* Initialize strings. */
      memset( text, 0, 128 );
      memset( str, 0, 128 );

      sprintf( str, "    RDA Elev %2d, RPG Elev Index %2d (Elev %5.2f): ",
               i+1, rdacnt.data[response].rdccon[i], elev_angle );
      strcat( text, str );

      /* Check supplemental flag bits. */
      if( suppl[i] & RDACNT_IS_CS ){

         sprintf( str, "CS " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_CD ){

         sprintf( str, "CD/W " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_CDBATCH ){

         sprintf( str, "CD/WO " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_BATCH ){

         sprintf( str, "BATCH " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SPRT ){

         sprintf( str, "SPRT " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SR ){

         sprintf( str, "SR " );
         strcat( text, str );

      }

      if( suppl[i] & RDACNT_IS_SZ2 ){

         sprintf( str, "SZ2 " );
         strcat( text, str );

      }

      /* We need to end the span for forest green background. */
      if( suppl[i] & RDACNT_SUPPL_SCAN ){

         sprintf( str, "<span foreground=\"forest green\">SAILS </span>" );
         strcat( text, str );

      }

      sprintf( text, "%s\n", text );
      Add_text( text );

   }

   /* Change label within scrolled window. */
   Add_text( "</span>" );
   gtk_label_set_markup( GTK_LABEL(Info_label), (gchar *) Buffer );

   /* Is the Window_vcp already defined? */
   if( Window_vcp == NULL ){

      /* Create a dialog ... ask if the VCP data is to be displayed. */
      popup = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
                                      "Do you want to display RDA VCP data?" );

      /* Run the dialog ... function returns when operator responds. */
      int result = gtk_dialog_run( GTK_DIALOG(popup) );
      switch( result ){

         /* On YES, set adaptation data to the selected value. */
         case GTK_RESPONSE_YES:
         {
            /* Current VCP. */
            if( Window_vcp == NULL )
               Write_vcp_data( vcp, FROM_RDA_RDACNT, 1 );
            break;

         }
         default:
         {
            /* On No, activate the radio button previously active. */
            break;
         }

      }

      /* Destory the popup widget. */
      gtk_widget_destroy( popup );

   }
   else if( refresh )
      Write_vcp_data( vcp, FROM_RDA_RDACNT, 0 );

   return TRUE;

}

