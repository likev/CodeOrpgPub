/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/12 14:11:25 $
 * $Id: hci_read_l2.c,v 1.6 2014/03/12 14:11:25 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <hci_read_l2.h>

/* Global variables. */
#define ICAO_SIZE		5
#define STATE_SIZE		32
#define RADAR_NAME_SIZE		(ICAO_SIZE+STATE_SIZE)
#define NUM_STATES		50
#define NUM_TERRITORIES		2
static int Read_local;
static char Icao[ICAO_SIZE];
static char Radar_name[RADAR_NAME_SIZE];
static char State[STATE_SIZE];
static int Radar_node[NUM_STATES+NUM_TERRITORIES];

/* Static Global variables. */
static GtkWidget *Window;
static GtkWidget *Box;
static GdkColor Canvas;
static GdkColor Steelblue;
static GdkColor White;
static GdkColor Darkgreen;
static GdkColor Green;
static GdkColor Yellow;
static GtkTreeSelection *Selection;

/* Support for read_l2. */
#define MAX_LINES		50
#define LINE_SIZE		132
#define TRUNC_LINE_SIZE		80 
#define MIN_LINES		35
static int Alert_user = 0;
static int Process_radar = 0;
static int Change_radar = 0;
static int Start_rpg = 0;
static int Wait_rpg = 0;
static int Terminate_cp = 0;
static char Buffer[MAX_LINES][LINE_SIZE+1];
static int Current_line = -1;
static int First_line = -1;
static int Number_lines = 0;
static void *Cp = NULL;

/* Button and label for displaying playback status. */
static GtkWidget *Status_label1 = NULL;
static GtkWidget *Status_label2 = NULL;

/* Radio buttons for Level II source. */
static GtkWidget *Button1 = NULL;
static GtkWidget *Button2 = NULL;
static GtkWidget *Active_button = NULL;
static GtkCellRenderer *Renderer1 = NULL;
static GtkCellRenderer *Renderer2 = NULL;

/* Used for displaying read_l2 output. */
static GtkWidget *Window_l2 = NULL;
static GtkWidget *Box_l2 = NULL;
static GtkWidget *Frame_l2 = NULL;
static GtkWidget *Scrolled_l2 = NULL;
static GtkWidget *Label_l2 = NULL;
static GtkWidget *Close_l2 = NULL;

/* Check Button for Change_radar. */
static GtkWidget *Check = NULL;

/* Function Prototypes. */
static int Read_options( int argc, char **argv );
static void Init_buffer();
static int Add_line( char *buf, int len );
static char* Get_text();
static void Setup_tree_view( GtkWidget *widget );
static void Failure_and_exit();
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer data );
static gboolean Radio_action_callback( gpointer data );
static void On_changed( GtkWidget *widget, gpointer data ); 
static gboolean Service_timeout();
static gboolean Read_level2();
static gboolean Quit_l2( GtkWidget *widget, GdkEvent  *event,
                          gpointer data );


/*\////////////////////////////////////////////////////////////////////

   Description:
      GUI interface for read_l2

////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   GtkWidget *treeview, *scrolled_win, *table, *button;
   GtkWidget *separator, *label, *frame, *box;
   GSList *group;
   GtkTreeStore *radars;
   GtkTreeIter iter, child;
   guint i = 0;
   int j, node = 0, ret;

   static Mrpg_state_t rpg_state;

   /* Do some intialization. */
   Process_radar = 0;
   Change_radar = 0;
   Alert_user = 0;
   Start_rpg = 0;
   Wait_rpg = 0;
   Read_local = 1;
   memset( &Icao[0], 0, ICAO_SIZE );
   memset( &State[0], 0, STATE_SIZE );
   memset( &Radar_name[0], 0, RADAR_NAME_SIZE );

   for( j = 0; j < (NUM_STATES + NUM_TERRITORIES); j++ )
      Radar_node[j] = -1;

   /* Read command line options.  Task exits on failure. */
   if( Read_options (argc, argv) != 0 )
      exit( 0 ); 

   /* Set up the RPG environment. */
   if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){
    
      LE_send_msg( GL_ERROR, "ORPGMISC_init Failed\n" );
      exit(0); 

   }

   /* Initialize data for Level II playback output display. */
   Init_buffer();

   /* Initialize GTK. */
   gtk_init( &argc, &argv );

   /* Check the RPG state. */
   ret = ORPGMGR_get_RPG_states( &rpg_state );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "ORPGMGR_get_RPG_states Returned Error.\n" );
      Failure_and_exit(); 

   }

   /* Define some colors. */
   gdk_color_parse( "steel blue", &Steelblue );
   gdk_color_parse( "white", &White );
   gdk_color_parse( "#cdaf95", &Canvas );
   gdk_color_parse( "dark green", &Darkgreen );
   gdk_color_parse( "green", &Green );
   gdk_color_parse( "yellow", &Yellow );

   /* Create a top level window. */
   Window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
   {
      gtk_window_set_title( GTK_WINDOW(Window), "hci_read_l2" );
      gtk_container_set_border_width( GTK_CONTAINER(Window), 10 );

      /* We don't want the maximum button ... just minimize and 
         close. */
      gtk_window_set_resizable( GTK_WINDOW( Window ), FALSE );

      /* Set the background color to "peachpuff3" */
      gtk_widget_modify_bg( Window, GTK_STATE_NORMAL, &Canvas );

      /* Set up the close application callback.  This 
         function is called when the X button in the 
         upper right corner is pressed. */
      g_signal_connect( Window, "delete-event",
                        G_CALLBACK (Close_application),
                        NULL );
   }

   /* Add a close button. */
   Box = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER( Window ), Box );
   gtk_widget_show( Box );

   table = gtk_table_new( 1, 4, TRUE );
   {
      /* Make the "Close" button blue with white text. */
      button = gtk_button_new_with_label( "  Close  " );
      gtk_widget_modify_bg( button, GTK_STATE_NORMAL, &Steelblue );
      label = gtk_bin_get_child( GTK_BIN(button) );
      gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &White );
      gtk_button_set_relief( GTK_BUTTON( button ), GTK_RELIEF_HALF );

      /* Attach the button to the table. */
      gtk_table_attach( GTK_TABLE(table), button, 0, 1, 0, 1,
                        GTK_EXPAND, GTK_FILL, 3, 3 );

      /* Define signal handler for the "Close" button. */
      g_signal_connect( button, "clicked",
                        G_CALLBACK (Close_application),
                        Window );

      /* Add the State button and entry. */
      Status_label1 = gtk_label_new( "Status: ");
      gtk_label_set_justify( GTK_LABEL(Status_label1), GTK_JUSTIFY_LEFT );

      /* Create an Entry widget for the Status.  Make this Entry 
         widget not editable with a limit of 20 characters of text. */
      Status_label2 = gtk_entry_new( );
      gtk_editable_set_editable( GTK_EDITABLE( Status_label2 ), FALSE );
      gtk_entry_set_width_chars( GTK_ENTRY( Status_label2 ), 20 );

      /* Sets the background color of the Entry widget.  Note:  You can
         use get_widget_modify_text to change the text color ... for now
         we use the default text color of black. */
      gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );

      gtk_table_attach( GTK_TABLE(table), Status_label1, 1, 2, 0, 1,
                        GTK_EXPAND, GTK_SHRINK, 3, 3 );
      gtk_table_attach( GTK_TABLE(table), Status_label2, 2, 4, 0, 1,
                        GTK_EXPAND, GTK_SHRINK, 3, 3 );

      /* Attach the table to the box. */
      gtk_container_add( GTK_CONTAINER (Box), table );

      /* Initialize the status entry text. */
      gtk_entry_set_text( GTK_ENTRY( Status_label2 ),
                          " Waiting Selection " );

   }

   /* Add a separator. */
   separator = gtk_hseparator_new ();
   {
      gtk_box_pack_start (GTK_BOX (Box), separator, FALSE, TRUE, 0);
   }

   /* Add the "Level II Source" frame. */
   frame = gtk_frame_new( "Level II Source" );
   {
      gtk_box_pack_start( GTK_BOX(Box), frame, TRUE, TRUE, 10);
      box = gtk_hbox_new (FALSE, 0);
      gtk_container_set_border_width( GTK_CONTAINER (box), 10 );
      gtk_container_add( GTK_CONTAINER( frame ), box );

      /* Add the radio buttons for the various parts of General Status. */
      Button1 = gtk_radio_button_new_with_label( NULL, "ROC Eng LAN" );
      gtk_box_pack_start( GTK_BOX (box), Button1, TRUE, TRUE, 5 );
      group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( Button1 ) );

      Button2 = gtk_radio_button_new_with_label( group, "Iowa State" );
      gtk_box_pack_start( GTK_BOX(box), Button2, TRUE, TRUE, 5 );
      group = gtk_radio_button_get_group( GTK_RADIO_BUTTON (Button2) );

      /* Make toggle button 1 active. */
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( Button1 ), TRUE );
   }

   /* Add the "Level II Source" frame. */
   frame = gtk_frame_new( "Change_radar" );
   {
      gtk_box_pack_start( GTK_BOX(Box), frame, TRUE, TRUE, 10);
      box = gtk_hbox_new (FALSE, 0);
      gtk_container_set_border_width( GTK_CONTAINER (box), 10 );
      gtk_container_add( GTK_CONTAINER( frame ), box );

      /* Add a check button for Change Radar. */
      Check = gtk_check_button_new_with_label( "Change Radar" );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(Check), FALSE );
      gtk_box_pack_start( GTK_BOX (box), Check, TRUE, TRUE, 5 );
      
   }

   /* Create a tree view widget. */
   frame = gtk_frame_new( "WSR-88D Radars" );
   {
      gtk_box_pack_start( GTK_BOX(Box), frame, TRUE, TRUE, 10 );

      treeview = gtk_tree_view_new();
      Setup_tree_view( treeview );
      radars = gtk_tree_store_new( COLUMNS, G_TYPE_STRING, G_TYPE_STRING );

      /* Populate the tree view. */
      node = 0;
      i = 0;
      while( Radar_sites[i].radar != NULL ){

         /* If the type is a CATEGORY (i.e., State) ... */
         if( Radar_sites[i].type == CATEGORY ){

            /* Add the category as a new root element. */
            gtk_tree_store_append( radars, &iter, NULL );
            gtk_tree_store_set( radars, &iter, STATE, Radar_sites[i].radar, 
                                RADAR, "", -1 );

            if( node < (NUM_STATES + NUM_TERRITORIES) ){

               Radar_node[node] = i;
               node++;

            }

         }/* Otherwise, add the radar site name as a child of the category. */
         else {

            gtk_tree_store_append( radars, &child, &iter );
            gtk_tree_store_set( radars, &child, STATE, "", 
                                RADAR, Radar_sites[i].radar, -1 );

         }

         i++;

      }

   }

   gtk_tree_view_set_model( GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(radars) );
   gtk_tree_view_expand_all( GTK_TREE_VIEW (treeview) );
   g_object_unref( radars );

   /* Create a scrolled window to hold all the radars.  Attach
      the treeview to the scrolled window. */
   scrolled_win = gtk_scrolled_window_new( NULL, NULL);
   {
      gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_win ),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
      gtk_widget_set_size_request( scrolled_win, 500, 600 );
      gtk_container_add( GTK_CONTAINER( scrolled_win ), treeview );
   }

   /* Attach the scrolled window to the top level window. */
   gtk_container_add( GTK_CONTAINER( frame ), scrolled_win );

   /* Make sure only one row can be selected at a time ... also define 
      signal handler for tree view row selection. */
   Selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( treeview ) );
   gtk_tree_selection_set_mode( Selection, GTK_SELECTION_SINGLE );

   /* Define callback functions. */
   g_signal_connect( Selection, "changed", G_CALLBACK(On_changed), NULL );
   g_signal_connect_swapped( Button1, "clicked",
                             G_CALLBACK( Radio_action_callback ),
                             "ROC ENG LAN" );
   g_signal_connect_swapped( Button2, "clicked",
                             G_CALLBACK( Radio_action_callback ),
                             "Iowa State" );


   /* Show all widgets and proceed to the "main loop". */
   gtk_widget_show_all( Window );
   g_timeout_add( 1000, Service_timeout, NULL );
   gtk_main();

   return 0;

}

/*\///////////////////////////////////////////////////////////////////

   Description:
      Add column to the GtkTreeView. The column will be displayed 
      as text. 

///////////////////////////////////////////////////////////////////\*/
static void Setup_tree_view( GtkWidget *treeview ){

   GtkTreeViewColumn *column;

   /* Create a new GtkCellRendererText, add it to the tree view 
      column and append the column to the tree view. */
   Renderer1 = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes( "State", 
                    Renderer1, "text", STATE, NULL);
   gtk_tree_view_append_column( GTK_TREE_VIEW (treeview), column );
   g_object_set( Renderer1, "weight", "bold", NULL );

   Renderer2 = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes( "Radar", 
                    Renderer2, "text", RADAR, NULL);
   gtk_tree_view_append_column( GTK_TREE_VIEW (treeview), column );
   g_object_set( Renderer2, "font", "Italic", NULL );

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
      Callback function for Closing the application.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Close_application( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data ){

   /* Stop the co-process. */
   if( Cp != NULL ){

      MISC_cp_close( Cp );
      Cp = NULL;

   }

   /* Close this application. */
   gtk_main_quit ();
   return FALSE;

}


/*\/////////////////////////////////////////////////////////////////

   Descriptin: 
      Callback function for tree view row selection.

/////////////////////////////////////////////////////////////////\*/
static void On_changed( GtkWidget *widget, gpointer data ){

   GtkTreeIter iter;
   GtkTreeIter parent;
   GtkTreeModel *model = NULL;
   gboolean ret;
   static char *text = "You Selected a STATE.  Select a RADAR within a STATE and try again.\n";
   GtkWidget *popup_window = NULL;
   char *radar_name = NULL;

   int node;

   /* Determine which row is selected ... this determines the ICAO. */
   if(  gtk_tree_selection_get_selected( GTK_TREE_SELECTION(widget), 
                                        &model, &iter ) ){

      ret = gtk_tree_model_iter_parent( model, &parent, &iter );
      if( ret == FALSE ){

         popup_window = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                "%s", text );

         /* Run the dialog ... function returns when operator responds. */
         gtk_dialog_run( GTK_DIALOG(popup_window) );
         gtk_widget_destroy( popup_window );

         return; 

      }

      gtk_tree_model_get( model, &iter, RADAR, &radar_name,  -1 );

      /* Get the State name. */
      node = -1;
      if( (ret = gtk_tree_model_iter_parent( model, &parent, &iter )) == TRUE ){

         gchar *str = gtk_tree_model_get_string_from_iter( model, &parent );
         if( (str != NULL) 
                  && 
             (strstr( str, ":" ) == NULL) ){

            /* Convert to index. */
            node = atoi( str );
            if( (node == 0)
                      &&
                (strcmp(str, "0") != 0) )
               node = -1;

         }
          
      }

      /* Set global flag indicating a radar was selected. */
      if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(Check) ) ){

         LE_send_msg( GL_INFO, "change_radar will be called\n" );
         Alert_user = 1;

      }
      else{
 
         LE_send_msg( GL_INFO, "read_l2 will be called\n" );
         Process_radar = 1;

      }

      /* Extract the ICAO. */
      if( radar_name != NULL ){

         memcpy( &Icao[0], radar_name, 4 );
         Icao[4] = '\0';
         g_free( radar_name );

      }

      /* Get the State. */
      strcpy( &State[0], "" );
      if( (node >= 0) && (Radar_node[node] >= 0) )
         strcpy( &State[0], Radar_sites[Radar_node[node]].radar );

      if( (strlen( &State[0] ) + strlen( &Icao[0] ) + 1) <= RADAR_NAME_SIZE ){

         sprintf( &Radar_name[0], "%s: %s", &State[0], &Icao[0] );
         LE_send_msg( GL_INFO, "Radar Name: %s, len: %d\n", &Radar_name[0], strlen(&Radar_name[0]) );

      }

   }

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for closing a top-level window.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Quit_l2( GtkWidget *widget, GdkEvent  *event,
                          gpointer   data ){

   /* Stop the co-process. */
   if( Cp != NULL ){

      MISC_cp_close( Cp );
      Cp = NULL;

   }
 
   /* Initial Radar_name and Icao. */
   memset( Radar_name, 0, RADAR_NAME_SIZE );
   memset( Icao, 0, ICAO_SIZE );
   
   /* Initialize the Terminate_cp flag. */
   Terminate_cp = 0;

   /* Allow Selections. */
   gtk_tree_selection_set_mode( Selection, GTK_SELECTION_SINGLE );
   
   /* Destroy this widget. */
   gtk_widget_destroy( Window_l2 );

   /* Set the widget to NULL. */
   gtk_widget_destroyed( Window_l2, &Window_l2 );
   if( Window_l2 != NULL )
      g_print( "Window L2 GtkWidget pointer NOT NULL" );

   /* Do some initialization. */
   Init_buffer();

   gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );
   gtk_entry_set_text( GTK_ENTRY( Status_label2 ), 
                       " Waiting Selection " );

   /* Return to caller. */
   return FALSE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for Level II Source Radio Buttons.

   Inputs: 
      data - value passed is defined in signal "connect". 

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Radio_action_callback( gpointer data ){

   int ret = 0;

   /* Check the passed data. */
   if( strstr( (gchar *) data, "ROC ENG LAN" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button1 ) );
      if( ret == 0 )
         return FALSE;

      else{

         /* Display Volume Status. */
         Active_button = Button1;
         Read_local = 1;

      }

   }
   else if( strstr( (gchar *) data, "Iowa State" ) ){

      ret = (int) gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( Button2 ) );
      if( ret == 0 )
         return FALSE;

      else{

         Active_button = Button2;
         Read_local = 0;

      }

   }

   return TRUE;

}


#define BUFSIZE		1024

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function contains a list of functions to be called by gtk_main()
      whenever a timeout occurs.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Service_timeout(){

   int cp_status, len, ret, nbytes;
   GtkWidget *popup = NULL;

   static int clean_startup = 0;
   static char *text = NULL, buf[BUFSIZE];
   static char cmd[BUFSIZE];
   static char o_cmd[BUFSIZE];

   static char *error = "You have selected the Change Radar Option.  The following\nactions will take place:\n\n1) change_radar will set ICAO\n2) Site-specific blockage data will be copied to $CFG_DIR/bin\n3) The RPG application software will be restarted.\n4) hci_read_l2 will be re-spawned to start playback\n\nThis might take a short while.\n\nIf you want an RPG clean startup, answer \"Yes\". Otherwise\nanswering \"No\" will perform a normal startup.\n";

   /* Do we need to alert the user? */
   if( Alert_user ){

      int result = 0;

      popup = gtk_message_dialog_new( NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                      "%s", error );

      Alert_user = 0;
      Change_radar = 1;
      clean_startup = 0;

      /* Run the dialog ... function returns when operator responds. */
      result = gtk_dialog_run( GTK_DIALOG(popup) );
      switch( result ){

         /* On YES, set adaptation data to the selected value. */
         case GTK_RESPONSE_YES:
         {

            clean_startup = 1;
            break;

         }

         case GTK_RESPONSE_NO:
         default:
         {
            clean_startup = 0;
            break;
         }

      }

      /* Destroy the popup widget. */
      gtk_widget_destroy( popup );

      /* Return. */
      return TRUE;

   }

   /* Check if change_radar should be called. */
   if( Change_radar ){
      
      /* Update status widget. */
      gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Yellow );
      gtk_entry_set_text( GTK_ENTRY( Status_label2 ),
                          " Changing Radar    " );

      /* Uncheck the Change Radar check box. */
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(Check), FALSE );

      /* Use change radar to change the ICAO ... don't shutdown or 
         restart the RPG. */
      if( clean_startup == 1 )
         sprintf( cmd, "hci_read_l2_helper -c -r \"%s\"", Radar_name );

      else
         sprintf( cmd, "hci_read_l2_helper -r \"%s\"", Radar_name );

      LE_send_msg( GL_INFO, "Executing %s\n", cmd );
      ret = MISC_system_to_buffer( cmd, o_cmd, BUFSIZE, &nbytes );
      if( ret >= 0 ){

         LE_send_msg( GL_INFO, "Successfully changed radar to %d\n", Icao );
         exit(0);

      }

   }

   /* Check if a radar has been selected and if so, process the data. */
   if( Process_radar ){

      LE_send_msg( GL_INFO, "Servicing ICAO: %s\n", (char *) &Icao[0] );
      Process_radar = 0;
      if( Window_l2 == NULL )
         Read_level2();

      /* Set the background color to green. */
      gtk_widget_modify_base( Status_label2, GTK_STATE_NORMAL, &Green );

      /* Construct and display text. */
      sprintf( buf, "Ingesting --> %s", Icao );
      gtk_entry_set_text( GTK_ENTRY( Status_label2 ), buf );

   }

   /* Is Co-process running.  If so, process output. */
   Terminate_cp = 0;
   if( Cp != NULL ){

      if( (cp_status = MISC_cp_get_status( Cp )) != 0 ){
      
         LE_send_msg( GL_INFO, "read_l2 failed Unexpectedly: %d\n", cp_status );
         Terminate_cp = 1;

      }

   }
      
   /* Even if the Co-process failed, continue to attempt to read in
      order to catch any possible error message. */
   while( (Cp != NULL) 
              &&
          (ret = MISC_cp_read_from_cp( Cp, buf, BUFSIZE )) != 0 ){

      len = strlen( buf );
      if( len > 0 )
         Add_line( (char *) &buf[0], len );

      /* Check if the return code indicates the Co-process failed. */
      if( ret == MISC_CP_DOWN ){

         /* No need to continue reading. */
         Terminate_cp = 1;
         break;

      }
 
   }

   /* Did the Co-process terminate? */
   if( Terminate_cp ){

      Terminate_cp = 0;
      LE_send_msg( GL_INFO, "Terminating Co-process: %s\n", buf );
      gtk_button_clicked( GTK_BUTTON( Close_l2 ));

   }

   /* If Output window widget exists, write output. */
   if( Window_l2 != NULL ){

      text = Get_text();
      if( text != NULL ){

         /* The output is presented as a label. */
         gtk_label_set_markup( GTK_LABEL( Label_l2 ), text );
         free(text);
         text = NULL;

      }

   }

   return TRUE;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Initializes data used for Level 2 playback output display.

////////////////////////////////////////////////////////////////////////////\*/
static void Init_buffer(){

   int i;

   /* Initialize for playback output. */
   Current_line = -1;
   First_line = -1;
   Number_lines = 0;

   for( i = 0; i < MAX_LINES; i++ )
      memset( &Buffer[i][0], 0, LINE_SIZE+1 );

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      This function adds the string "buf" of len "len" to Buffer. 

////////////////////////////////////////////////////////////////////////////\*/
static int Add_line( char *buf, int len ){

   char *str = NULL;
   static char temp[LINE_SIZE];

   /* Check that the line length is less than LINE_SIZE.  If not, clip it. */
   if( len > TRUNC_LINE_SIZE ){

      strcpy( &buf[TRUNC_LINE_SIZE-5], "....\n" );
      len = strlen( buf );

   }

   temp[0] = '\0';
   strcat( temp, buf );

   /* Check if buffer string contains "File header" */
   if( (str = strstr( buf, "File header" )) != NULL ){

      /* Remove the line feed because we are going to
         add it back in later. */
      if( (str = strstr( buf, "\n" )) != NULL )
         str = '\0';

      /* Make text bold. */
      sprintf( temp, "<span weight=\"bold\">%s</span>\n", buf );
      len = strlen( temp );      

      LE_send_msg( GL_INFO, "BOV: %s", temp );

   }

   /* Check if buffer string contains "End of volume detected" */
   if( (str = strstr( buf, "End of volume detected" )) != NULL ){

      /* Remove the line feed because we are going to
         add one in the sprintf. */
      if( (str = strstr( buf, "\n" )) != NULL )
         str = '\0';

      /* Make text bold. */
      sprintf( temp, "<span foreground=\"dark green\">%s</span>\n", buf );
      len = strlen( temp );      

      LE_send_msg( GL_INFO, "EOV: %s", temp );

   }

   /* Increment Current_line. */
   Current_line++;
   if( Current_line > (MAX_LINES-1) )
      Current_line = 0; 

   /* Set First_line. */
   if( First_line < 0 )
      First_line = 0;

   else if( Number_lines > MAX_LINES )
      First_line++;

   if( First_line > (MAX_LINES-1) )
      First_line = 0;

   /* Move the data to Buffer. */
   sprintf( &Buffer[Current_line][0], "%s", temp );

   /* Increment the total number of lines. */
   Number_lines++;

   return 0;

}


/*\/////////////////////////////////////////////////////////////////////////////

   Description:
      Builds the text for the label used for displaying read_l2 output.

/////////////////////////////////////////////////////////////////////////////\*/
static char* Get_text(){

   int num = Number_lines;
   int i, j, size = 0;

   char *text = NULL;

   /* Allocate a buffer big enough to hold the text. */
   if( num < MIN_LINES )
      num = MIN_LINES;

   if( num > MAX_LINES )
      num = MAX_LINES;

   size = num*(LINE_SIZE+1);
   text = calloc( 1, size );
   if( text == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n", size );
      return NULL;

   }

   /* Fill text. */
   j = First_line;
   for( i = 0; i < num; i++ ){

      if( i < Number_lines )
         strcat( text, Buffer[j] );

      /* Prepare for next line. */
      j++;

      /* If j goes beyond the array limit, set it 
         back to 1. */
      if( j > (MAX_LINES-1) )
         j = 0;

   }

   /* Return text. */
   return text;

}


/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Top level module for "read_l2" playback.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Read_level2(){

   GtkWidget *table = NULL;
   GtkWidget *child = NULL;
   GtkWidget *separator = NULL;

   char buf[128];
   int ret = 0;

   /* Do not allow selections anymore. */
   gtk_tree_selection_set_mode( Selection, GTK_SELECTION_NONE );
   
   /* Create the top-level window. */
   Window_l2 = gtk_window_new( GTK_WINDOW_TOPLEVEL );

   /* If we don't want the maximum button, set the last argument
      to FALSE. */
   gtk_window_set_resizable( GTK_WINDOW( Window_l2 ), TRUE );

   /* Set the background color to "peachpuff3" */
   gtk_widget_modify_bg( Window_l2, GTK_STATE_NORMAL, &Canvas );

   /* Set up the close application callback.  This function is called 
      when the X button in the upper right corner is pressed. */
   g_signal_connect( Window_l2, "delete-event",
                     G_CALLBACK (Quit_l2),
                     NULL );

   sprintf( buf, "%s", Radar_name );
   gtk_window_set_title( GTK_WINDOW (Window_l2), buf );
   gtk_container_set_border_width ( GTK_CONTAINER(Window_l2), 0 );

   Box_l2 = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER(Window_l2), Box_l2 );

   /* Add the close button. */
   table = gtk_table_new( 1, 4, TRUE );

   /* Add a Close button. */
   Close_l2 = gtk_button_new_with_label( "  Close  " );
   {
      gtk_widget_modify_bg( Close_l2, GTK_STATE_NORMAL, &Steelblue );
      child = gtk_bin_get_child( GTK_BIN(Close_l2) );
      gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );
      gtk_button_set_relief( GTK_BUTTON( Close_l2 ), GTK_RELIEF_HALF );

      /* Attach the table to the Box. */
      gtk_box_pack_start( GTK_BOX(Box_l2), table, FALSE, TRUE, 0);

      /* Attach the button to the table. */
      gtk_table_attach( GTK_TABLE(table), Close_l2, 0, 1, 0, 1,
                        GTK_EXPAND, GTK_FILL, 3, 3 );

      /* Define signal handler for the "Close" button. */
      g_signal_connect( Close_l2, "clicked",
                        G_CALLBACK (Quit_l2),
                        Window_l2 );
   }

   /* Add a separator. */
   separator = gtk_hseparator_new();
   {
      gtk_box_pack_start( GTK_BOX(Box_l2), separator, FALSE, TRUE, 0 );
   }

   /* Add the "Playback Output" frame. */
   Frame_l2 = gtk_frame_new( "Playback Output" );
   {
      gtk_box_pack_start( GTK_BOX(Box_l2), Frame_l2, TRUE, TRUE, 20);

      /* Create a scrolled window. */
      Scrolled_l2 = gtk_scrolled_window_new( NULL, NULL );
      gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( Scrolled_l2 ),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

      /* Create a label containing the playback information and 
         add to the scrolled window. */
      Label_l2 = gtk_label_new( "read_l2 output will displayed here!" );
      gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( Scrolled_l2 ),
                                             GTK_WIDGET( Label_l2 ) );
      gtk_widget_set_size_request( Scrolled_l2, 525, 575 );

      /* Add the scrolled window to the frame container. */
      gtk_container_add( GTK_CONTAINER(Frame_l2), Scrolled_l2 );

   }

   gtk_widget_show_all( Window_l2 );

   /* Construct the command. */
   memset( buf, 0, 128 );
   if( Read_local )
      sprintf( buf, "read_l2 -L %s", Icao );

   else 
      sprintf( buf, "read_l2 %s", Icao );

   /* Start the co-process. */
   LE_send_msg( GL_INFO, "cmd: %s\n", buf );
   if( (ret = MISC_cp_open( buf, 0, &Cp )) != 0 ){

      /* For some reason the co-process failed. */
      LE_send_msg( GL_ERROR, "MISC_cp_open Failed: %d\n", ret );
      gtk_button_clicked( GTK_BUTTON( Close_l2 ));
      Cp = NULL;

   }

   return TRUE;

}

/*\/////////////////////////////////////////////////////////////////////

   Description:  
      This function reads command line arguments.

   Input:        
      argc - Number of command line arguments.
      argv - Command line arguments.

   Output:       
      Usage message

   Returns:      
      0 on success or -1 on failure

///////////////////////////////////////////////////////////////////\*/
static int Read_options( int argc, char **argv ){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   char *colon = NULL;
   char radar_name[RADAR_NAME_SIZE];

   /* Do some initialization. */
   memset( Icao, 0, ICAO_SIZE );
   memset( Radar_name, 0, RADAR_NAME_SIZE );
   memset( radar_name, 0, RADAR_NAME_SIZE );

   /* Process command line arguments. */
   while ((c = getopt (argc, argv, "r:")) != EOF) {

      switch (c) {

         case 'r':
         {
            strcpy( &radar_name[0], optarg );

            if( strlen( radar_name ) > ICAO_SIZE ){

               strcpy( &Radar_name[0], &radar_name[0] );
               Radar_name[strlen(radar_name)] = '\0';

               /* Does Radar_name contain the State? */
               colon = strstr( &radar_name[0], ":" );
               if( colon != NULL ){

                  /* Go past the colon. */
                  colon++;

                  /* Strip off leading blanks, if any. */
                  while( colon[0] == ' ' )
                     colon++;

                  /* Set ICAO and Process_radar flag. */
                  if( strlen( colon ) < ICAO_SIZE ){

                     memcpy( &Icao[0], colon, 4 );
                     Icao[4] = '\0';
                     Process_radar = 1;            

                  }

               }
               else
                  memcpy( &Icao[0], radar_name, 4 );

            }

            break;

         }

         default:
            break;
      }

   }

   return (0);

/* End of Read_options() */
}

