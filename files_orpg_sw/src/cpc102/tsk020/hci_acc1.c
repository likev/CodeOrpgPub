#include <glib.h>
#include <gtk/gtk.h>
#include <infr.h>
#include <orpg.h>
#include <orpgrda.h>
#include <vcp.h>

/* Macros for identifying the 2 types of Accounting Data. */
#define ELEVATION_ACCT_DATA 	1
#define VOLUME_ACCT_DATA 	2
#define SCAN_SUMMARY_DATA 	3

/* Global Variables. */
static char *Buffer = NULL;
static int Buf_size = 0;
static int Text_size = 0;
static int Service_request = 0;
static unsigned int Volume_num = (unsigned int) -1;
static int Volume_scan = 0;
static Radial_accounting_data Accdata;
static GtkWidget *Window = NULL;

/* Used for selecting radio buttons. */
GtkWidget *Button1 = NULL;
GtkWidget *Button2 = NULL;
GtkWidget *Button3 = NULL;
GtkWidget *Prev_button = NULL;
GtkWidget *Next_button = NULL;
GtkWidget *Volume_label = NULL;
GtkWidget *Volume_entry = NULL;
GtkWidget *Active_button = NULL;
GtkWidget *Info_label = NULL;
GtkWidget *Box1 = NULL;

/* Used for displaying Accouting Information. */
GtkWidget *Window_acct = NULL;
GtkWidget *Box_acct = NULL;
GtkWidget *Frame_acct = NULL;
GtkWidget *Scrolled_acct = NULL;
GtkWidget *Label_acct = NULL;

/* Used for providing colors. */
GdkColor Yellow;
GdkColor Green;
GdkColor Canvas;
GdkColor Steelblue;
GdkColor White;

/* Function Prototypes and Definitions. */
static void Failure_and_exit();
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer data );
static gboolean Quit_acct( GtkWidget *widget, GdkEvent  *event,
                           gpointer data );
static gboolean Radio_action_callback( gpointer data );
static int Add_text( char *text );
static gboolean Display_volume_acct();
static gboolean Display_elev_acct();
static gboolean Display_scan_summary();
static gboolean Prev_callback( gpointer data );
static gboolean Next_callback( gpointer data );
static void Enter_callback( GtkWidget *widget, GtkWidget *entry );
static void Changed_callback( GtkWidget *widget, GtkWidget *entry );
static int Service_timeout();
static void Write_acct_data();
static void Write_scan_sum_data();
static void Convert_time( long timevalue, char *hours, char *minutes, 
                          char *seconds );
static void To_ASCII( int value, char *string, int type );
static void Calendar_date( short date, char *day, char *month, char *year );


/*\///////////////////////////////////////////////////////////////////

   Description:
      Main function for hci_acc1 gui.

///////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

    GtkWidget *box2;
    GtkWidget *table;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *separator;
    GtkWidget *frame;
    GtkWidget *child;
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

    gtk_window_set_title (GTK_WINDOW (Window), "hci_acc1");
    gtk_container_set_border_width (GTK_CONTAINER (Window), 0);

    Box1 = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER (Window), Box1 );
    gtk_widget_show( Box1 );

    /* Add the close and help buttons. */
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

       gtk_container_add( GTK_CONTAINER (Box1), table );

    }

    /* Add a separator. */
    separator = gtk_hseparator_new ();
    {
       gtk_box_pack_start (GTK_BOX (Box1), separator, FALSE, TRUE, 0);
    }

    /* Add the "Accounting Data View" frame. */
    frame = gtk_frame_new( "Accounting Data View" );
    {
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);
       box2 = gtk_hbox_new (FALSE, 0);
       gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
       gtk_container_add( GTK_CONTAINER( frame ), box2 );

       /* Add the radio buttons for Volume and Elevation Accounting. */
       Button1 = gtk_radio_button_new_with_label( NULL, "Volume Accounting" );
       gtk_box_pack_start( GTK_BOX (box2), Button1, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( Button1 ) );

       Button2 = gtk_radio_button_new_with_label( group, "Elevation Accounting" );
       gtk_box_pack_start( GTK_BOX (box2), Button2, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button2) );

       Button3 = gtk_radio_button_new_with_label( group, "Scan Summary" );
       gtk_box_pack_start( GTK_BOX (box2), Button3, TRUE, TRUE, 5 );
       group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button3) );

       gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button1 ), TRUE );
       Active_button = Button1;

    }

    /* Add the "Volume Scan" frame.  Contains a "<--Prev" and "Next-->"
       for cycling through volume scans. */
    frame = gtk_frame_new( "Volume Scan" );
    {

       table = gtk_table_new( 1, 4, TRUE );
       {
          /* Add the Volume scan label and entry widget. */
          Volume_label = gtk_label_new( "Volume Scan: ");
          gtk_label_set_justify( GTK_LABEL(Volume_label), GTK_JUSTIFY_LEFT );

          /* Create an Entry widget for the Volume Scan Number. */
          Volume_entry = gtk_entry_new( );
          gtk_entry_set_width_chars( GTK_ENTRY( Volume_entry ), 10 );

          gtk_table_attach( GTK_TABLE(table), Volume_label, 1, 2, 0, 1,
                            GTK_EXPAND, GTK_SHRINK, 2, 2 );
          gtk_table_attach( GTK_TABLE(table), Volume_entry, 2, 3, 0, 1,
                            GTK_EXPAND, GTK_SHRINK, 2, 2 );

          /* Define signal handlers for the "Entry" widget. */
          g_signal_connect( Volume_entry, "activate",
                            G_CALLBACK (Enter_callback),
                            Volume_entry );

/*
          g_signal_connect( Volume_entry, "changed",
                            G_CALLBACK (Changed_callback),
                            Volume_entry );
*/

          /* Make the "Prev" button blue with white text. */
          Prev_button = gtk_button_new_with_label( " <--Prev " );
          gtk_widget_modify_bg( Prev_button, GTK_STATE_NORMAL, &Steelblue );
          label = gtk_bin_get_child( GTK_BIN(Prev_button) );
          gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
          gtk_button_set_relief( GTK_BUTTON( Prev_button ), GTK_RELIEF_HALF );

          /* Attach the button to the table. */
          gtk_table_attach( GTK_TABLE(table), Prev_button, 0, 1, 0, 1,
                            GTK_EXPAND, GTK_FILL, 2, 2 );

          /* Define signal handler for the "Prev" button. */
          g_signal_connect_swapped( Prev_button, "clicked",
                            G_CALLBACK (Prev_callback),
                            "Prev" );

          /* Make the "Next" button blue with white text. */
          Next_button = gtk_button_new_with_label( " Next--> " );
          gtk_widget_modify_bg( Next_button, GTK_STATE_NORMAL, &Steelblue );
          label = gtk_bin_get_child( GTK_BIN(Next_button) );
          gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
          gtk_button_set_relief( GTK_BUTTON( Next_button ), GTK_RELIEF_HALF );

          /* Attach the button to the table. */
          gtk_table_attach( GTK_TABLE(table), Next_button, 3, 4, 0, 1,
                            GTK_EXPAND, GTK_FILL, 2, 2 );

          /* Define signal handler for the "Next" button. */
          g_signal_connect_swapped( Next_button, "clicked",
                            G_CALLBACK (Next_callback),
                            "Next" );

          /* Attach the table to the frame. */
          gtk_container_add( GTK_CONTAINER (frame), table );

       }

       /* Add the frame to the box container. */
       gtk_box_pack_start( GTK_BOX(Box1), frame, TRUE, TRUE, 10);

    }

    gtk_widget_show_all( Window );
     
    /* Define callback functions for the radio buttons. */
    g_signal_connect_swapped( Button1, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "Volume Accounting" );
    g_signal_connect_swapped( Button2, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "Elevation Accounting" );
    g_signal_connect_swapped( Button3, "clicked",
                              G_CALLBACK( Radio_action_callback ),
                              "Scan Summary" );

    /* Set a period timer to check whether data needs to be displayed. */
    g_timeout_add( 1000, Service_timeout, NULL );

    /* Do GTK Main Loop. */
    gtk_main( ); 
    return 0;
}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for whenever the operator enters a volume scan 
      number.

////////////////////////////////////////////////////////////////////////////\*/
static void Enter_callback( GtkWidget *widget, GtkWidget *entry ){

   const gchar *entry_text = gtk_entry_get_text( GTK_ENTRY (entry) );
   int vs = 0;

   char text[128];
   GtkWidget *popup_window = NULL;

   /* Convert this number to an integer. */
   vs = atoi( entry_text );

   /* Validate the number. */
   if( vs <= 0 ){

      sprintf( text, "Volume Scan Must Be > 0\n" );
      popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                             "%s", text );

      /* Run the dialog ... function returns when operator responds. */
      gtk_dialog_run( GTK_DIALOG(popup_window) );
      gtk_widget_destroy( popup_window );

      /* Set the text in the Entry Widget to blank. */
      gtk_entry_set_text( GTK_ENTRY( Volume_entry ), "          " );

      return;

   }

   /* Set the volume number and convert to volume scan. */
   Volume_num = (unsigned int) vs;
   Volume_scan = ORPGMISC_vol_scan_num( Volume_num );
   
   LE_send_msg( GL_INFO,  "Entry contents: %s (%d)\n", 
                entry_text, Volume_scan );

   gtk_editable_select_region( GTK_EDITABLE( Volume_entry ), 0, 
                               GTK_ENTRY (entry)->text_length );

   /* Set the "Prev" and "Next" buttons to sensitive. */
   gtk_widget_set_sensitive( Prev_button, TRUE );
   gtk_widget_set_sensitive( Next_button, TRUE );

   /* Determine which radio button is selected and service 
      the request. */
   if( Active_button == Button1 )
      Service_request = VOLUME_ACCT_DATA;

   else if( Active_button == Button2 )
      Service_request = ELEVATION_ACCT_DATA;

   else if( Active_button == Button3 )
      Service_request = SCAN_SUMMARY_DATA;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for whenever the operator starts to change a 
      volume scan number.

////////////////////////////////////////////////////////////////////////////\*/
static void Changed_callback( GtkWidget *widget, GtkWidget *entry ){

   /* Set the "Prev" and "Next" buttons to insensitive. */
   gtk_widget_set_sensitive( Prev_button, FALSE );
   gtk_widget_set_sensitive( Next_button, FALSE );

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for the "<--Prev" button.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Prev_callback( gpointer data ){

   /* Test the string to ensure the data was as expected. */
   if( strstr( (gchar *)data, "Prev" ) ){

      Volume_num--;

      if( Volume_num == 0 )
         Volume_num = 80;

      Volume_scan = ORPGMISC_vol_scan_num( Volume_num );

      LE_send_msg( GL_INFO, "Next Volume Selectied: %d (%d)\n",
                   Volume_scan, Volume_num );

      /* Determine which radio button is active and service the request. */
      if( Active_button == Button1 )
         Service_request = VOLUME_ACCT_DATA;

      else if( Active_button == Button2 )
         Service_request = ELEVATION_ACCT_DATA;

      else if( Active_button == Button3 )
         Service_request = SCAN_SUMMARY_DATA;

      LE_send_msg( GL_INFO, "Prev_data: Service_request set to: %d\n",
                   Service_request );

   }

   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for the "Next-->" button.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Next_callback( gpointer data ){

   /* Test the string to ensure the data was as expected. */
   if( strstr( (gchar *)data, "Next" ) ){

      Volume_num++;
      Volume_scan = ORPGMISC_vol_scan_num( Volume_num );

      LE_send_msg( GL_INFO, "Next Volume Selectied: %d (%d)\n",
                   Volume_scan, Volume_num );

      /* Determine which radio button is active and service the request. */
      if( Active_button == Button1 )
         Service_request = VOLUME_ACCT_DATA;

      else if( Active_button == Button2 )
         Service_request = ELEVATION_ACCT_DATA;

      else if( Active_button == Button3 )
         Service_request = SCAN_SUMMARY_DATA;

      LE_send_msg( GL_INFO, "Next_data: Service_request set to: %d\n",
                   Service_request );

   }

   return TRUE;

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function contains a list of functions to be called by gtk_main()
      whenever a timeout occurs.

////////////////////////////////////////////////////////////////////////////\*/
static int Service_timeout(){

   /* Check that a valid Volume Scan has been entered. */
   if( Service_request ){

      if( (Volume_num == (unsigned int) -1)
                      ||
          (Volume_scan < 1) || (Volume_scan > 80) ){


         LE_send_msg( GL_INFO, "In Service_timeout .. Invalid Vol #: %u OR Vol Scan #: %d\n",
                      Volume_num, Volume_scan );
         return TRUE;

      }

   }

   /* If here, a valid volume scan number has been entered. */
   if( Service_request ){

      LE_send_msg( GL_INFO, "In Service_timeout-->Service_request: %d\n",
                   Service_request );

      if( Service_request == ELEVATION_ACCT_DATA ){

         LE_send_msg( GL_INFO, "Display Elevation Accounting\n" );
         Display_elev_acct();

      }
      else if( Service_request == VOLUME_ACCT_DATA ){

         LE_send_msg( GL_INFO, "Display Volume Accounting.\n" );
         Display_volume_acct();

      }
      else if( Service_request == SCAN_SUMMARY_DATA ){

         LE_send_msg( GL_INFO, "Display Scan Summary.\n" );
         Display_scan_summary();

      }

      Service_request = 0;

   }

   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for Closing the application.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Close_application( GtkWidget *widget, GdkEvent  *event,
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
static gboolean Quit_acct( GtkWidget *widget, GdkEvent  *event,
                          gpointer data ){

  /* Destory this widget. */
  gtk_widget_destroy( Window_acct );

  /* Set the widget to NULL. */
  gtk_widget_destroyed( Window_acct, &Window_acct );
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
      Callback function for Radio Buttons.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Radio_action_callback( gpointer data ){

   int ret = 0;

   /* Check the passed data. */
   if( strstr( (gchar *) data, "Volume Accounting" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button1 ) );
      if( ret == 0 )
         return FALSE;

      else{

         /* Display Volume Status. */
         Active_button = Button1;
         Service_request = VOLUME_ACCT_DATA;
         return TRUE;

      }

   }
   else if( strstr( (gchar *) data, "Elevation Accounting" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button2 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button2;
         Service_request= ELEVATION_ACCT_DATA;
         return TRUE;

      }

   }
   else if( strstr( (gchar *) data, "Scan Summary" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button3 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button3;
         Service_request= SCAN_SUMMARY_DATA;
         return TRUE;

      }

   }
   else
     g_print( "Bad Value in radio_action: value = %s\n", (gchar *) data );

   return TRUE;

}

/*\///////////////////////////////////////////////////////////

   Description:
      Function that displays the volume accounting data.

///////////////////////////////////////////////////////////\*/
static gboolean Display_elev_acct(){

   int ret, elv, elev_num, num_sectors;
   char weather_mode;
   float dopres, overlay_margin;

   static char text[256];

   /* Read in the radial accounting data. */
   LE_send_msg( GL_INFO, "Read Accounting Data for Volume Scan: %d\n",
                Volume_scan );
   ret = ORPGDA_read( ORPGDAT_ACCDATA, (char *) &Accdata,
                      sizeof( Radial_accounting_data ), Volume_scan-1 );

   /* Display error message if data is available for this volume scan. */
   if( ret <= 0 ){

      sprintf( Buffer, 
          "<span face = \"Monspace\">Accounting Data Read Error For Volume Scan %d: (Code: %d)\n</span>",
          Volume_scan, ret );
      LE_send_msg( GL_INFO, "%s", Buffer );

      /* Write out the data. */
      Write_acct_data( );

      return TRUE;

   }

   /* Initialize buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Set weather mode. */
   if( Accdata.accwxmode == PRECIPITATION_MODE )
      weather_mode = 'A';
   
   else if (Accdata.accwxmode == CLEAR_AIR_MODE )
      weather_mode = 'B';

   else
      weather_mode = 'T';

   /* Set the Doppler data resolution. */
   if( Accdata.accdopres == 1 )
      dopres = 0.5;

   else if( Accdata.accdopres == 2 )
      dopres = 1.0;

   else{
   
      LE_send_msg( GL_INFO, "Unknown Doppler Resolution %d\n",
                   Accdata.accdopres );
      dopres = 0.0;

   }

   /* Set overlay margin. */
   overlay_margin = (float) Accdata.accthrparm/10.0;

   /* Write volume constants. */
   sprintf( text, "\n\n <span weight=\"bold\">SEQUENCE NUMBER: %2d</span>  VCP: %3d  WEATHER MODE: %c\n",
            Volume_scan, Accdata.accvcpnum, weather_mode );
   Add_text( text );

   sprintf( text, " DOPPLER RESOLUTION: %3.1f m/s   OVERLAY MARGIN: %4.1f dB\n",
            dopres, overlay_margin );
   Add_text( text );

   sprintf( text, " CALIBRATION CONSTANT: %5.2f dB\n", Accdata.acccalib );
   Add_text( text );

   /* Write header. */
   sprintf( text,
      "\n EL#   SAZM NYVEL UNRG   SAZM NYVEL UNRG   SAZM NYVEL UNRG   ATMOS\n");
   Add_text( text );

   sprintf( text,
      "             m/s   km          m/s   km          m/s   km    dB/km\n");
   Add_text( text );
   sprintf( text,
      "__________________________________________________________________\n");
   Add_text( text );

   /* Do for each of elevation cut. */
   for( elv = 0; elv < Accdata.accnumel; elv++ ){

      /* Set elevation number. */
      elev_num = elv + 1;

      /* Determine how many sectors are defined. */
      for( num_sectors = 0; num_sectors < MAX_SECTS; num_sectors++ )
         if( Accdata.accsecsaz[elv][num_sectors] == -1.0 )
            break;

      if( num_sectors > MAX_SECTS )
         num_sectors = MAX_SECTS;

      /* Write out accounting data. */

      if( num_sectors == 1 ){

         sprintf( text,
            " %2d     0.0 %5.2f  %3d                                      %5.4f\n",
            elev_num,
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10,
            Accdata.accatmos[elv]/1000.0 );

         Add_text( text );

      }
      else if( num_sectors == 2 ){

         sprintf( text,
            " %2d   %5.1f %5.2f  %3d  %5.1f %5.2f %3d                     %5.4f\n",
            elev_num,
            Accdata.accsecsaz[elv][0],
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10,
            Accdata.accsecsaz[elv][1],
            Accdata.accunvel[elv][1]/100.0,
            Accdata.accunrng[elv][1]/10,
            Accdata.accatmos[elv]/1000.0 );

         Add_text( text );

      }
      else if( num_sectors == 3 ){

         sprintf( text,
            " %2d   %5.1f %5.2f  %3d  %5.1f %5.2f  %3d  %5.1f %5.2f  %3d  %5.4f\n",
            elev_num,
            Accdata.accsecsaz[elv][0],
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10,
            Accdata.accsecsaz[elv][1],
            Accdata.accunvel[elv][1]/100.0,
            Accdata.accunrng[elv][1]/10,
            Accdata.accsecsaz[elv][2],
            Accdata.accunvel[elv][2]/100.0,
            Accdata.accunrng[elv][2]/10,
            Accdata.accatmos[elv]/1000.0 );

         Add_text( text );

      }

   }

   /* Put in some blank lines. */
   Add_text( "\n</span>" );

   /* Write out the data. */
   Write_acct_data( );

   return TRUE;

}


/*\///////////////////////////////////////////////////////////

   Description:
      Function that displays the volume accounting data.

///////////////////////////////////////////////////////////\*/
static gboolean Display_volume_acct(){

   int ret, elv, elev_num, duration = 0;
   short date;
   char weather_mode;
   float overlay_margin, dopres;
   long vol_start_time = 0, vol_end_time = 0;
   
   static char start_hours[3], start_minutes[3];
   static char end_hours[3], end_minutes[3];
   static char start_seconds[7], end_seconds[7];
   static char day[3], month[3], year[3];
   static int prev_start_time;

   static char text[256];

   /* Read in the radial accounting data. */
   LE_send_msg( GL_INFO, "Read Accounting Data for Volume Scan: %d\n",
                Volume_scan );
   ret = ORPGDA_read( ORPGDAT_ACCDATA, (char *) &Accdata,
                      sizeof( Radial_accounting_data ), Volume_scan-1 );

   /* Display error message if data is available for this volume scan. */
   if( ret <= 0 ){

      sprintf( Buffer, 
          "<span face = \"Monspace\">Accounting Data Read Error For Volume Scan %d: (Code: %d)\n</span>",
          Volume_scan, ret );
      LE_send_msg( GL_INFO, "%s", Buffer );

      /* Write out the data. */
      Write_acct_data( );

      return TRUE;

   }

   /* Initialize buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Set weather mode. */
   if( Accdata.accwxmode == PRECIPITATION_MODE )
      weather_mode = 'A';

   else if (Accdata.accwxmode == CLEAR_AIR_MODE )
      weather_mode = 'B';

   else
      weather_mode = 'T';

   /* Set the Doppler data resolution. */
   if( Accdata.accdopres == 1 )
      dopres = 0.5;

   else if( Accdata.accdopres == 2 )
      dopres = 1.0;

   else{

      LE_send_msg( GL_INFO, "Unknown Doppler Resolution %d\n", 
                   Accdata.accdopres );
      dopres = 0.0;

   }

   /* Set overlay margin. */
   overlay_margin = (float) Accdata.accthrparm/10.0;

   /* Write volume constants. */
   sprintf( text, "\n\n <span weight=\"bold\">SEQUENCE NUMBER: %2d</span>  VCP: %3d  WEATHER MODE: %c\n",
            Volume_scan, Accdata.accvcpnum, weather_mode );
   Add_text( text );

   sprintf( text, " DOPPLER RESOLUTION: %3.1f m/s   OVERLAY MARGIN: %4.1f dB\n",
            dopres, overlay_margin );
   Add_text( text );

   sprintf( text, " CALIBRATION CONSTANT: %5.2f dB\n", Accdata.acccalib );
   Add_text( text );

   /* Write header. */
   sprintf( text,
            "\n                   START         END        START    END                    \n");
   Add_text( text );
   sprintf( text,
            "EL#    DATE                TIME                AZIMUTH     PSEUDO ELANG RADS\n");
   Add_text( text );
   sprintf( text,
            "____________________________________________________________________________\n");
   Add_text( text );

   /* Set previous elevation start time as start time for first elevation cut. */
   prev_start_time = Accdata.acceltms[0];

   /* Do for each of elevation cut. */
   for( elv = 0; elv < Accdata.accnumel; elv++ ){

      /* Set elevation number. */
      elev_num = elv + 1;

      /* Convert the time in milliseconds to HH:MM:SS.sss format. */
      Convert_time( Accdata.acceltms[elv],
                    start_hours, start_minutes, start_seconds );
      Convert_time( Accdata.acceltme[elv],
                    end_hours, end_minutes, end_seconds );

      if( elv == 0 )
         vol_start_time = Accdata.acceltms[elv];

      if( elv == Accdata.accnumel - 1 );
         vol_end_time = Accdata.acceltme[elv];

      /* Convert the date to mm/dd/yy format. */
      date = Accdata.acceldts[elv];
      Calendar_date( date, day, month, year );

      /* Write out accounting data. */
      sprintf( text,
             " %2d  %s/%s/%s   %s:%s:%s %s:%s:%s  %6.2f  %6.2f  %6.2f  %4.1f  %3d\n",
             elev_num,
             month,
             day,
             year,
             start_hours,
             start_minutes,
             start_seconds,
             end_hours,
             end_minutes,
             end_seconds,
             Accdata.accbegaz[elv],
             Accdata.accendaz[elv],
             Accdata.accpendaz[elv],
             Accdata.accbegel[elv],
             Accdata.accnumrd[elv] );

      Add_text( text );

   }

   duration = (int) (vol_end_time - vol_start_time)/1000;
   if( duration < 0 )
      duration += 86400;

   sprintf( text, "\n<span weight=\"bold\">VOLUME DURATION: %3d</span>\n\n", duration );
   Add_text( text );
   Add_text( "</span>" );

   /* Write out the data. */
   Write_acct_data( );

   return TRUE;
}


/*\///////////////////////////////////////////////////////////

   Description:
      Function that displays the scan summary data.

///////////////////////////////////////////////////////////\*/
static gboolean Display_scan_summary(){

   short date;
   char weather_mode;
   long vol_start_time = 0;
   Scan_Summary *scan = NULL;

   static char hours[3], minutes[3], seconds[7];
   static char day[3], month[3], year[3];

   static char text[256];

   /* Read in the radial accounting data. */
   LE_send_msg( GL_INFO, "Read Scan Summary Data for Volume Scan: %d\n",
                Volume_scan );
   scan = ORPGSUM_get_scan_summary( Volume_scan );

   /* Display error message if data is available for this volume scan. */
   if( scan == NULL ){

      sprintf( Buffer,
          "<span face = \"Monspace\">Scan Summary Data Read Error For Volume Scan %d:\n</span>",
          Volume_scan );
      LE_send_msg( GL_INFO, "%s", Buffer );

      /* Write out the data. */
      Write_acct_data( );

      return TRUE;

   }

   /* Initialize buffer. */
   memset( Buffer, 0, Buf_size );
   Text_size = 0;

   /* Start populating the buffer. */
   sprintf( text, "<span face = \"Monospace\">\n" );
   Add_text( text );

   /* Convert the time in milliseconds to HH:MM:SS.sss format. */
   vol_start_time = scan->volume_start_time*1000;
   Convert_time( vol_start_time, hours, minutes, seconds );

   /* Convert the date to mm/dd/yy format. */
   date = scan->volume_start_date;
   Calendar_date( date, day, month, year );

   /* Write out accounting data. */
   sprintf( text,
            " VOLUME START DATE:    %s/%s/%s \n",
            month,
            day,
            year );
   Add_text( text );

   sprintf( text,
            " VOLUME_START_TIME:    %s:%s:%s \n",
            hours,
            minutes,
            seconds );
   Add_text( text );

   /* Set weather mode. */
   if( scan->weather_mode == PRECIPITATION_MODE )
      weather_mode = 'A';

   else if( scan->weather_mode == CLEAR_AIR_MODE )
      weather_mode = 'B';

   else
      weather_mode = 'T';

   sprintf( text, " <span weight=\"bold\">SCAN NUMBER:          %2d (%d)</span>\n",
            Volume_scan, Volume_num );
   Add_text( text );

   sprintf( text, " VCP:                  %3d\n", scan->vcp_number );
   Add_text( text );

   sprintf( text, " WEATHER MODE:         %c\n", weather_mode );
   Add_text( text );

   sprintf( text, " RDA ELEVATION CUTS:   %d\n", scan->rda_elev_cuts );
   Add_text( text );

   sprintf( text, " RPG ELEVATION CUTS:   %d\n", scan->rpg_elev_cuts );
   Add_text( text );

   sprintf( text, " SPOT BLANK STATUS:    %d\n", scan->spot_blank_status );
   Add_text( text );

   sprintf( text, " LAST RDA CUT:         %d\n", (int) scan->last_rda_cut );
   Add_text( text );

   sprintf( text, " LAST RPG CUT:         %d\n", (int) scan->last_rpg_cut );
   Add_text( text );

   if( scan->avset_status == AVSET_ENABLED )
      sprintf( text, " AVSET STATUS:         Enabled (%d)\n", (int) scan->avset_status );
   else if( scan->avset_status == AVSET_DISABLED )
      sprintf( text, " AVSET STATUS:         Disabled (%d)\n", (int) scan->avset_status );
   else
      sprintf( text, " AVSET STATUS:         Unknown (%d)\n", (int) scan->avset_status );
   Add_text( text );

   sprintf( text, " N SAILS CUTS:         %d\n", (int) scan->n_sails_cuts );
   Add_text( text );

   sprintf( text, " AVSET TERM ANGLE:     %d (deg*10)\n", (int) scan->avset_term_ang );
   Add_text( text );

   Add_text( "</span>" );

   /* Write out the data. */
   Write_scan_sum_data();

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
      Writes out the acct data.

********************************************************************************/
static void Write_acct_data(){

   GtkWidget *button = NULL;
   GtkWidget *label = NULL;
   GtkWidget *table = NULL;
   GtkWidget *child  = NULL;
   GtkWidget *separator  = NULL;

   char temp[32];

   /* Create the top-level window. */
   if( Window_acct == NULL ){

      Window_acct = gtk_window_new( GTK_WINDOW_TOPLEVEL );

      /* We don't want the maximum button ... just minimize and 
         close. */
      gtk_window_set_resizable( GTK_WINDOW( Window_acct ), FALSE );

      /* Set the background color to "peachpuff3" */
      gtk_widget_modify_bg( Window_acct, GTK_STATE_NORMAL, &Canvas );

      /* Set up the close application callback.  This function is called 
         when the X button in the upper right corner is pressed. */
      g_signal_connect( Window_acct, "delete-event",
                        G_CALLBACK (Quit_acct),
                        NULL );

      gtk_window_set_title (GTK_WINDOW (Window_acct), "Accounting Data");
      gtk_container_set_border_width (GTK_CONTAINER (Window_acct), 0);

      Box_acct = gtk_vbox_new( FALSE, 0 );
      gtk_container_add( GTK_CONTAINER (Window_acct), Box_acct );

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
         gtk_box_pack_start (GTK_BOX (Box_acct), table, FALSE, TRUE, 0);

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 0, 1, 0, 1,
                           GTK_EXPAND, GTK_FILL, 3, 3 );

         /* Define signal handler for the "Close" button. */
         g_signal_connect( button, "clicked",
                           G_CALLBACK (Quit_acct),
                           Window_acct );

      }
      
      /* Add the "Prev" button. */   /* Make the "Prev" button blue with white text. */
      button = gtk_button_new_with_label( " <--Prev " );
      {
         /* Add the "Prev" button. */   
         gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
         label = gtk_bin_get_child( GTK_BIN(button) );
         gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
         gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 2, 3, 0, 1,
                           GTK_EXPAND, GTK_FILL, 2, 2 );

         /* Define signal handler for the "Prev" button. */
         g_signal_connect_swapped( button, "clicked",
                           G_CALLBACK (Prev_callback),
                           "Prev" );

      }
      
      /* Add the "Next" button. */
      button = gtk_button_new_with_label( " Next--> " );
      { 
         /* Make the "Next" button blue with white text. */
         gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
         label = gtk_bin_get_child( GTK_BIN(button) );
         gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
         gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 3, 4, 0, 1,
                           GTK_EXPAND, GTK_FILL, 2, 2 );

         /* Define signal handler for the "Next" button. */
         g_signal_connect_swapped( button, "clicked",
                           G_CALLBACK (Next_callback),
                           "Next" );

      }

      /* Add a separator. */
      separator = gtk_hseparator_new ();
      {
         gtk_box_pack_start (GTK_BOX (Box_acct), separator, FALSE, TRUE, 0);
      }

      /* Add the "Accounting Data" frame. */
      Frame_acct = gtk_frame_new( "Accounting Data" );
      {
         gtk_box_pack_start( GTK_BOX(Box_acct), Frame_acct, TRUE, TRUE, 10);

         /* Create a scrolled window. */
         Scrolled_acct = gtk_scrolled_window_new( NULL, NULL );
         gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( Scrolled_acct ),
                                         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

         /* Create a label containing the VCP information and add to the scrolled window. */
         Label_acct = gtk_label_new( "Accounting data will be displayed here!" );
         gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( Scrolled_acct ),
                                                GTK_WIDGET( Label_acct ) );
         gtk_widget_set_size_request( Scrolled_acct, 775, 675 );

         /* Add the scrolled window to the frame container. */
         gtk_container_add( GTK_CONTAINER(Frame_acct), Scrolled_acct );

      }

      gtk_widget_show_all( Window_acct );

   }

   if( Window_acct != NULL ){

      /* Display the markup text. */
      gtk_label_set_markup( GTK_LABEL( Label_acct ), (gchar *) Buffer );

      /* Update the Volume_entry text. */
      sprintf( temp, "%d", Volume_num );
      gtk_entry_set_text( GTK_ENTRY(Volume_entry), temp );


   }

   return;

}


/********************************************************************************
   
   Description:
      Writes out the Scan Summary data.

********************************************************************************/
static void Write_scan_sum_data(){

   GtkWidget *button = NULL;
   GtkWidget *label = NULL;
   GtkWidget *table = NULL;
   GtkWidget *child  = NULL;
   GtkWidget *separator  = NULL;

   char temp[32];

   /* Create the top-level window. */
   if( Window_acct == NULL ){

      Window_acct = gtk_window_new( GTK_WINDOW_TOPLEVEL );

      /* We don't want the maximum button ... just minimize and 
         close. */
      gtk_window_set_resizable( GTK_WINDOW( Window_acct ), FALSE );

      /* Set the background color to "peachpuff3" */
      gtk_widget_modify_bg( Window_acct, GTK_STATE_NORMAL, &Canvas );

      /* Set up the close application callback.  This function is called 
         when the X button in the upper right corner is pressed. */
      g_signal_connect( Window_acct, "delete-event",
                        G_CALLBACK (Quit_acct),
                        NULL );

      gtk_window_set_title (GTK_WINDOW (Window_acct), "Scan Summary Data");
      gtk_container_set_border_width (GTK_CONTAINER (Window_acct), 0);

      Box_acct = gtk_vbox_new( FALSE, 0 );
      gtk_container_add( GTK_CONTAINER (Window_acct), Box_acct );

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
         gtk_box_pack_start (GTK_BOX (Box_acct), table, FALSE, TRUE, 0);

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 0, 1, 0, 1,
                           GTK_EXPAND, GTK_FILL, 3, 3 );

         /* Define signal handler for the "Close" button. */
         g_signal_connect( button, "clicked",
                           G_CALLBACK (Quit_acct),
                           Window_acct );

      }

      /* Add the "Prev" button. */   /* Make the "Prev" button blue with white text. */
      button = gtk_button_new_with_label( " <--Prev " );
      {
         /* Add the "Prev" button. */
         gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
         label = gtk_bin_get_child( GTK_BIN(button) );
         gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
         gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 2, 3, 0, 1,
                           GTK_EXPAND, GTK_FILL, 2, 2 );

         /* Define signal handler for the "Prev" button. */
         g_signal_connect_swapped( button, "clicked",
                           G_CALLBACK (Prev_callback),
                           "Prev" );

      }

      /* Add the "Next" button. */
      button = gtk_button_new_with_label( " Next--> " );
      {
         /* Make the "Next" button blue with white text. */
         gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
         label = gtk_bin_get_child( GTK_BIN(button) );
         gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
         gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

         /* Attach the button to the table. */
         gtk_table_attach( GTK_TABLE(table), button, 3, 4, 0, 1,
                           GTK_EXPAND, GTK_FILL, 2, 2 );

         /* Define signal handler for the "Next" button. */
         g_signal_connect_swapped( button, "clicked",
                           G_CALLBACK (Next_callback),
                           "Next" );

      }

      /* Add a separator. */
      separator = gtk_hseparator_new ();
      {
         gtk_box_pack_start (GTK_BOX (Box_acct), separator, FALSE, TRUE, 0);
      }

      /* Add the "Accounting Data" frame. */
      Frame_acct = gtk_frame_new( "Scan Summary Data" );
      {
         gtk_box_pack_start( GTK_BOX(Box_acct), Frame_acct, TRUE, TRUE, 10);

         /* Create a scrolled window. */
         Scrolled_acct = gtk_scrolled_window_new( NULL, NULL );
         gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( Scrolled_acct ),
                                         GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

         /* Create a label containing the Scan Summary information and add to the 
            scrolled window. */
         Label_acct = gtk_label_new( "Scan Summary data will be displayed here!" );
         gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( Scrolled_acct ),
                                                GTK_WIDGET( Label_acct ) );
         gtk_widget_set_size_request( Scrolled_acct, 500, 500 );

         /* Add the scrolled window to the frame container. */
         gtk_container_add( GTK_CONTAINER(Frame_acct), Scrolled_acct );

      }

      gtk_widget_show_all( Window_acct );

   }

   if( Window_acct != NULL ){

      /* Display the markup text. */
      gtk_label_set_markup( GTK_LABEL( Label_acct ), (gchar *) Buffer );

      /* Update the Volume_entry text. */
      sprintf( temp, "%d", Volume_num );
      gtk_entry_set_text( GTK_ENTRY(Volume_entry), temp );


   }

   return;

}


/*\///////////////////////////////////////////////////////////////////////
  
   Description:
      Converts time, in milliseconds since midnight, into hrs,
      min, secs strings.

///////////////////////////////////////////////////////////////////////\*/
static void Convert_time( long timevalue, char *hours, char *minutes, 
                          char *seconds ){

   int hrs, mins, secs;

   /* Extract the number of hours. */
   hrs = timevalue/3600000;

   /* Extract the number of minutes. */
   timevalue = timevalue - hrs*3600000;
   mins = timevalue/60000;

   /* Extract the number of seconds. */
   secs = timevalue - mins*60000;

   /* Convert numbers to ASCII. */
   To_ASCII( hrs, hours, 0 );
   To_ASCII( mins, minutes, 0 );
   To_ASCII( secs, seconds, 1 );

/* End of Convert_time(). */
}


/*\///////////////////////////////////////////////////////////////////////

   Description:
      Takes value (number) and converts to string.

   Inputs:
      value - value to convert.
      type - type of value to convert.

   Outputs:
      string - converted string.

///////////////////////////////////////////////////////////////////////\*/
static void To_ASCII( int value, char *string, int type ){

   int i;
   unsigned char digit;

   /* Process integer value. */
   if( type == 0 ){

      for( i = 1; i >= 0; i-- ){

         /* Produce the text string. */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10;

      }

      /* Pad the string with string terminator. */
      string[2] = '\0';

   }
   else{

      for( i = 5; i >= 3; i-- ){

         /* Produce the fractional portion of text string. */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10;

      }

      /* Put in the decimal point. */
      string[2] = '.';

      for( i = 1; i >= 0; i-- ){

         /* Produce the fractional portion of text string. */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10;

      }

      /* Pad the string with string terminator. */
      string[6] = '\0';

   }

/* End of To_ASCII(). */
}


/*\/////////////////////////////////////////////////////////////////////////

    Description:
       Given the modified Julian date, returns day, month and year strings.

////////////////////////////////////////////////////////////////////////\**/
static void Calendar_date( short date, char *day, char *month, char *year ){

   int l,n, julian;
   int dd, dm, dy;

   /* Convert modified julian to type integer */
   julian = date;

   /* Convert modified julian to year/month/day */
   julian += 2440587;
   l = julian + 68569;
   n = 4*l/146097;
   l = l -  (146097*n + 3)/4;
   dy = 4000*(l+1)/1461001;
   l = l - 1461*dy/4 + 31;
   dm = 80*l/2447;
   dd= l -2447*dm/80;
   l = dm/11;
   dm = dm+ 2 - 12*l;
   dy = 100*(n - 49) + dy + l;
   dy = dy - 1900;

   /* Convert numbers to ASCII. */
   To_ASCII( dd, day, 0 );
   To_ASCII( dm, month, 0 );
   To_ASCII( dy, year, 0 );

   return;

/* End of Calendar_date(). */
}

