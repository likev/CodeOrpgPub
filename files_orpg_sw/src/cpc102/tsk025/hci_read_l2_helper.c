#include <stdlib.h>
#include <infr.h>
#include <orpg.h>
#include <mrpg.h>
#include <glib.h>
#include <gtk/gtk.h>

#define COMMAND_SIZE		128
#define BUFSIZE			1024
#define CHANGE_RADAR_FAILED	-1
#define MRPG_STARTUP_FAILED	-2
#define MRPG_SHUTDOWN_FAILED	-3

enum { CHANGE_RADAR, SHUTDOWN_RPG, START_RPG, WAIT_RPG, PROCESS_RADAR, DONE };
static int State = CHANGE_RADAR;
static Mrpg_state_t Rpg_state;
static int Clean_startup = 0;

#define ICAO_SIZE               5
#define STATE_SIZE              32
#define RADAR_NAME_SIZE         (ICAO_SIZE+STATE_SIZE)
static char Icao[ICAO_SIZE];
static char Radar_name[RADAR_NAME_SIZE];

static GdkColor Canvas;
static GdkColor Steelblue;
static GdkColor White;
static GtkWidget *Window = NULL;
static GtkWidget *Box = NULL;
static GtkWidget *Frame = NULL;
static GtkWidget *Scrolled = NULL;
static GtkWidget *Label = NULL;
static GtkWidget *Close = NULL;

#define MAX_LINES               50
#define LINE_SIZE               132
#define TRUNC_LINE_SIZE         100 
#define MIN_LINES               35
static char Buffer[MAX_LINES][LINE_SIZE+1];
static int Current_line = -1;
static int First_line = -1;
static int Number_lines = 0;

/* Function Prototypes. */
static int Read_options( int argc, char **argv );
static void Init_buffer();
static int Set_up();
static int Add_line( char *buf, int len );
static char* Get_text();
static gboolean Service_timeout();
static gboolean Quit( GtkWidget *widget, GdkEvent  *event,
                      gpointer data );

/*\/////////////////////////////////////////////////////////////

   Description:
      hci_read_l2 helper program for changing radar prior
      to playback.

/////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int ret;

   /* Initialize GTK. */
   gtk_init( &argc, &argv );

   /* Set up GUI. */
   ret = Set_up();

   /* Do some initialization. */
   Init_buffer();

   /* Set up the RPG environment. */
   if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGMISC_init Failed\n" );
      exit(0);

   }

   /* Process the command line arguments. */
   if( (ret = Read_options( argc, argv )) != 0 ){

      LE_send_msg( GL_INFO, "Command Line Parsing Failed\n" );
      exit(0);
   
   }   
   
   /* Show all widgets and proceed to the "main loop". */
   gtk_widget_show_all( Window );
   g_timeout_add( 1000, Service_timeout, NULL );
   gtk_main();

   return 0;

}

/*\/////////////////////////////////////////////////////////////

   Description:
      Service timeout routine that handles changing radar
      and restarting the RPG.

/////////////////////////////////////////////////////////////\*/
static gboolean Service_timeout(){

   int ret, nbytes = 0;
   char cmd[COMMAND_SIZE];
   char o_cmd[BUFSIZE];
   char buf[BUFSIZE];

   static int count = 0;
   static int exit_code = 0;

   /* Nothing more to do ... start cleaning up. */
   if( State == DONE ){

      count++;

      /* Let the screen display for 5 passes. */
      if( count == 5 ){

         gtk_button_clicked( GTK_BUTTON(Close) );
         exit( exit_code );

      }

      return TRUE;

   }

   /* Go through all the various processing states. */

   switch( State ){

      case CHANGE_RADAR:
      {
         sprintf( cmd, "sh -c \"change_radar -r %s -R -S\"", &Icao[0] );
         sprintf( buf, "Executing %s\n", cmd );
         Add_line( buf, strlen(buf) );

         ret = MISC_system_to_buffer( cmd, o_cmd, BUFSIZE, &nbytes );
         if( ret == 0 ){

            char blockage_name[128];
            char path[128];
            char *loc = NULL;
            char *home = NULL;

            /* Initialize the variables. */
            memset( &blockage_name[0], 0, 128 );
            memset( &path[0], 0, 128 );
            home = getenv( "HOME" );

            /* Is the "BLOCKAGE_DIR" environment variable defined? */
            if( (loc = getenv( "BLOCKAGE_DIR" )) == NULL )
               strcpy( &blockage_name[0], "/import/apps/data/blockage_B12" ); 
 
            else
               strcpy( &blockage_name[0], loc );

            sprintf( &path[0], "%s/%s_blockage.lb", &blockage_name[0], &Icao[0] );
            sprintf( cmd, "cp %s %s/cfg/bin/blockage.lb", &path[0], home );
            sprintf( buf, "Executing %s\n", cmd );
            Add_line( buf, strlen(buf) );

            ret = MISC_system_to_buffer( cmd, o_cmd, BUFSIZE, &nbytes );
            if( ret != 0 ){

               sprintf( buf, "Copy of Site-Specific Blockage File Failed: %d\n", ret );
               Add_line( buf, strlen(buf) );
               if( nbytes > 0 )
                  Add_line( o_cmd, strlen(o_cmd) );

            }
            else{

               sprintf( buf, "Copy of Site-Specific Blockage File Succeeded\n" );
               Add_line( buf, strlen(buf) );

           }

            sprintf( buf, "Successfully changed radar to %s\n", &Icao[0] );
            Add_line( buf, strlen(buf) );

            
            State = START_RPG;

            if( Clean_startup ){


               sprintf( cmd, "sh -c \"mrpg -p startup\"" );

            }
            else
               sprintf( cmd, "sh -c \"mrpg startup\"" );

            sprintf( buf, "Executing %s\n", cmd );
            Add_line( buf, strlen(buf) );

         }
         else{

            sprintf( buf, "Error changing radar.\n" );
            Add_line( buf, strlen(buf) );
            State = DONE;
            exit_code = CHANGE_RADAR_FAILED;

         }

         break;

      }

      case START_RPG:
      {
         if( Clean_startup )
            sprintf( cmd, "sh -c \"mrpg -p startup\"" );

         else
            sprintf( cmd, "sh -c \"mrpg startup\"" );

         ret = MISC_system_to_buffer( cmd, o_cmd, BUFSIZE, &nbytes );
         if( ret >= 0 ){

            sprintf( buf, "Starting the RPG.\n" );
            Add_line( buf, strlen(buf) );
            
            State = WAIT_RPG;

         }
         else{

            sprintf( buf, "Error starting RPG.\n" );
            Add_line( buf, strlen(buf) );
            exit_code = MRPG_STARTUP_FAILED;

         }

         break;
   
      }

      case WAIT_RPG:
      {
         ret = ORPGMGR_get_RPG_states( &Rpg_state );

         if( ret < 0 )
            LE_send_msg( GL_INFO, "ORPGMGR_get_RPG_states Returned Error.\n" );

         else if( Rpg_state.state == MRPG_ST_OPERATING ){

            /* If the RPG is opertional .... */
            sprintf( buf, "The RPG is Operational\n" );
            Add_line( buf, strlen(buf) );

            /* Let's process the level 2 data if an ICAO has been defined. */
            if( strlen( &Icao[0] ) > 0 )
               State = PROCESS_RADAR;

         }

         break;

      }

      case PROCESS_RADAR:
      {
         /* Need to start this process again. */
         sprintf( cmd, "hci_read_l2 -r \"%s\" &", Radar_name );
         sprintf( buf, "Process Radar: cmd->%s\n", cmd );
         Add_line( buf, strlen(buf) );

         if( (ret = MISC_system_to_buffer( cmd, o_cmd, BUFSIZE, &nbytes )) < 0 ){

            sprintf( buf, "hci_read_l2 Failed: %d\n", ret );
            Add_line( buf, strlen(buf) );

         }

         State = DONE;
         break;
      }
       
      default:
      case DONE:
         break;
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
   Clean_startup = 0;

   /* Process command line arguments. */
   while ((c = getopt (argc, argv, "cr:")) != EOF) {

      switch (c) {

         case 'c':
         {

            Clean_startup = 1;
            break;
 
         }

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

                  }

               }
               else
                  memcpy( &Icao[0], radar_name, 4 );

            }

            /* Set the processing state. */
            State = CHANGE_RADAR;

            break;

         }

         default:
            break;
      }

   }

   return (0);

/* End of Read_options() */
}

/*\/////////////////////////////////////////////////////////////////

   Description:
      Setup widget for displaying progress messages. 

/////////////////////////////////////////////////////////////////\*/
static int Set_up(){

   GtkWidget *child = NULL;
   GtkWidget *table = NULL;
   GtkWidget *separator = NULL;

   /* Define some colors. */
   gdk_color_parse( "steel blue", &Steelblue );
   gdk_color_parse( "white", &White );
   gdk_color_parse( "#cdaf95", &Canvas );

   /* Create the top-level window. */
   Window = gtk_window_new( GTK_WINDOW_TOPLEVEL );

   /* If we don't want the maximum button, set the last argument
      to FALSE. */
   gtk_window_set_resizable( GTK_WINDOW( Window ), TRUE );

   /* Set the background color to "peachpuff3" */
   gtk_widget_modify_bg( Window, GTK_STATE_NORMAL, &Canvas );

   /* Set up the close application callback.  This function is called 
      when the X button in the upper right corner is pressed. */
   g_signal_connect( Window, "delete-event",
                     G_CALLBACK (Quit),
                     NULL );

   gtk_window_set_title( GTK_WINDOW (Window), "hci_read_l2_helper" );
   gtk_container_set_border_width ( GTK_CONTAINER(Window), 0 );

   Box = gtk_vbox_new( FALSE, 0 );
   gtk_container_add( GTK_CONTAINER(Window), Box );

   /* Add the close button. */
   table = gtk_table_new( 1, 4, TRUE );

   /* Add a Close button. */
   Close = gtk_button_new_with_label( "  Close  " );
   {
      gtk_widget_modify_bg( Close, GTK_STATE_NORMAL, &Steelblue );
      child = gtk_bin_get_child( GTK_BIN(Close) );
      gtk_widget_modify_fg( child, GTK_STATE_NORMAL, &White );
      gtk_button_set_relief( GTK_BUTTON( Close ), GTK_RELIEF_HALF );

      /* Attach the table to the Box. */
      gtk_box_pack_start( GTK_BOX(Box), table, FALSE, TRUE, 0);

      /* Attach the button to the table. */
      gtk_table_attach( GTK_TABLE(table), Close, 0, 1, 0, 1,
                        GTK_EXPAND, GTK_FILL, 3, 3 );

      /* Define signal handler for the "Close" button. */
      g_signal_connect( Close, "clicked",
                        G_CALLBACK (Quit),
                        Window );
   }

   /* Add a separator. */
   separator = gtk_hseparator_new();
   {
      gtk_box_pack_start( GTK_BOX(Box), separator, FALSE, TRUE, 0 );
   }

   /* Add the "Playback Output" frame. */
   Frame = gtk_frame_new( "Helper Output" );

  {
      gtk_box_pack_start( GTK_BOX(Box), Frame, TRUE, TRUE, 20);

      /* Create a scrolled window. */
      Scrolled = gtk_scrolled_window_new( NULL, NULL );
      gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( Scrolled ),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );

      /* Create a label containing the playback information and 
         add to the scrolled window. */
      Label = gtk_label_new( "" );
      gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( Scrolled ),
                                             GTK_WIDGET( Label ) );
      gtk_widget_set_size_request( Scrolled, 500, 500 );

      /* Add the scrolled window to the frame container. */
      gtk_container_add( GTK_CONTAINER(Frame), Scrolled );

   }

   gtk_widget_show_all( Window );

   return 0;

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Initializes data used for output display.

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

   static char temp[LINE_SIZE];
   char *text = NULL;

   /* Check that the line length is less than LINE_SIZE.  If not, clip it. */
   if( len > TRUNC_LINE_SIZE ){

      strcpy( &buf[TRUNC_LINE_SIZE-5], "....\n" );
      len = strlen( buf );

   }

   temp[0] = '\0';
   strcat( temp, buf );

   /* Increment Current_line. */
   Current_line++;
   if( Current_line > MAX_LINES )
      Current_line = 0;

   /* Set First_line. */
   if( First_line < 0 )
      First_line = 0;

   else if( Number_lines > MAX_LINES )
      First_line++;
   if( First_line > MAX_LINES )
      First_line = 0;

   /* Move the data to Buffer. */
   sprintf( &Buffer[Current_line][0], "%s", temp );

   /* Increment the total number of lines. */
   Number_lines++;

   /* Write output. */
   if( Window != NULL ){

      text = Get_text();
      if( text != NULL ){

         /* The output is presented as a label. */
         gtk_label_set_text( GTK_LABEL( Label ), text );
         free(text);
         text = NULL;

      }

   }

   return 0;

}
/*\/////////////////////////////////////////////////////////////////////////////

   Description:
      Builds the text for the label used for displaying output.

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
      if( j > MAX_LINES )
         j = 0;

   }

   /* Return text. */
   return text;

}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Callback function for closing a top-level window.

   Notes: 
      See GTK+ documentation for callback function format.

////////////////////////////////////////////////////////////////////////////\*/
static gboolean Quit( GtkWidget *widget, GdkEvent  *event,
                      gpointer   data ){

   /* Destroy this widget. */
   gtk_widget_destroy( Window );

   /* Set the widget to NULL. */
   gtk_widget_destroyed( Window, &Window );
   if( Window != NULL )
      g_print( "Window GtkWidget pointer NOT NULL" );

   /* Return to caller. */
   return FALSE;

}

