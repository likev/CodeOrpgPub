/*********************************************************************************

      File: nbtcp_terminal.c
            This file contains the terminal interface routines for the tool

 ********************************************************************************/


/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/11/17 18:33:05 $
 * $Id: nbtcp_terminal.c,v 1.6 2009/11/17 18:33:05 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <fcntl.h>     /*    fcntl (...)   */
#include <stdio.h>
#include <time.h>

#include <nbtcp.h>

   /* Globals */
extern int PVC0_sock, PVC1_sock;  /* virtual circuit socket descriptors */
extern int Otrs_pending;          /* number of OTRs remaining to be received */
extern u_short RequestInterval;
extern int LoopOTRs;
extern int Otr_only;
extern int Otr_script;

   /* File scope */
static time_t start_time = 0;
static time_t next_time = 0;
static char  *str_start_time;


   /* local funtion prototypes */

static int Send_rps_list ();

/********************************************************************************

     Description: This routine checks for user input

           Input: link_state - the connected state of the link
                  interface  - the comms interface being used (cm_tcp or native)
                  connection - the link number or port number connected to 
                               (depends on the interface used)

          Output:

          Return:

 ********************************************************************************/

#define NUMBER_OF_SELECTIONS  5
#define NUMBER_OF_WAN_OTR_SELECTIONS 3
#define TERMINATION_FLAG      9
#define PROD_SAVE_FLAG_INDX   NUMBER_OF_SELECTIONS
#define PROD_SAVE_FLAG_OTR_INDX NUMBER_OF_WAN_OTR_SELECTIONS


void TERM_check_input (int link_state, int interface, int connection)
{
          char buf[BUFSIZ];  /* BUFSIZ defined in stdio.h */
          int  i;
   static int  number_active_items = NUMBER_OF_SELECTIONS;
   static int  selection = 0;
   static int  selection_pending = FALSE;
   static int  ret = 0;
   static int  n_1_connection_state = FALSE;
   static int  n_1_selection_pending = FALSE;
   static char text_selections [NUMBER_OF_SELECTIONS+1] [BUFSIZ] = {
       {""},
       {"Send One Time Request to Dedicated RPG\0"},
       {"Send Bias Table\0"},
       {"Send Alert Request\0"},
       {"Send RPS List\0"},
       {"Toggle product save flag"}};  /* Note: keep this as the last entry in the
                                                array so the "toggle" logic is
                                                managed properly */

   if (link_state == TRUE) {

         /* refresh the main menu */

      if (((ret != -1) && (selection_pending == FALSE))  
                           ||
           ((n_1_selection_pending == TRUE) && (selection_pending == FALSE)) 
                           ||
           (n_1_connection_state != link_state))
      {
         if ((system ("clear")) >= 0)
         {
            fprintf (stdout, "******************************\n");
            fprintf (stdout, "*                            *\n");
            fprintf (stdout, "*    NBTCP Tool Interface    *\n");
            if (interface == CM_TCP_INTERFACE)
               fprintf (stdout, "*          Link: %d          *\n", connection);
            else
               fprintf (stdout, "*         Port: %d         *\n", connection);
            fprintf (stdout, "*                            *\n");
            fprintf (stdout, "******************************\n");

            fprintf (stdout, "\n\n");

            for (i=1; i<= NUMBER_OF_SELECTIONS-1; i++) 
               fprintf (stdout, " %d - %s\n", i, text_selections[i]);

               /* display the current state of the "product save" flag
                  if products can be saved */
            if (PROC_get_product_dir () != NULL) {
               fprintf (stdout, " %d - %s ", 
                  PROD_SAVE_FLAG_INDX,
                  text_selections[PROD_SAVE_FLAG_INDX]);
               if (PROC_get_prod_save_flag () == TRUE)
                  fprintf (stdout, "(current state: save products)\n");
               else
                  fprintf (stdout, 
                           "(current state: products not saved)\n");
            }
            else
               number_active_items = NUMBER_OF_SELECTIONS - 1;

                        
            fprintf (stdout, " 9 - Terminate Tool\n");

            fprintf(stdout, "\nEnter selection: ");
            fflush (stdout);
         }
      }

      n_1_selection_pending = selection_pending;

         /* if selection_pending is true, then a lower level menu selection
            is still being processed. Continue servicing that request until
            it has been completely processed */

      if (selection_pending == FALSE) { 
  
            /* read stdin...-1 is returned if the user has not
               hit the <enter> key */

         ret = read (STDIN_FILENO, buf, FILENAME_MAX); 
 
         if (ret != -1) {
            fprintf(stdout, "\n");

            if ((selection = atoi(buf)) == 0) {
                  printf ("Input is invalid\n");
                  sleep (1); 
            }
            else if ((selection > 0) && (selection <= number_active_items)) {
                  fprintf (stdout, "Processing \"%s\"\n", 
                           text_selections[selection]);
                  sleep (1); 
            }
         } 
         else
            selection = 0;
      }

      switch (selection) {
         case 0:   /* nothing to do, so break */
            break;

         case 1:
            if ((SM_send_product_request_msg (PVC1_sock)) == -1)
               fprintf (stderr, 
                        "Error sending Onetime Request Message\n");
         break;
  
         case 2:
            if ((SM_send_bias_table_msg ()) == -1)
               fprintf (stderr, 
                        "Error sending Bias Table Message\n");
            break;

         case 3:
            if ((SM_alert_request_msg ()) == -1)
               fprintf (stderr, 
                        "Error sending Alert Request Message\n");
            break;

         case 4:
            ret = Send_rps_list ();
            if (ret == 0) {
               selection_pending = FALSE;
               sleep (1);
            }
            else if (ret == 1)
               selection_pending = TRUE;
            else if (ret == -1) {
               fprintf (stderr, "Error sending RPS List\n");
               selection_pending = FALSE;
               sleep (2);
            }
            break;

         case 5:
            if (PROC_get_product_dir () == NULL)
               fprintf (stdout, "Invalid selection\n");
            else
               PROC_toggle_prod_save_flag ();
            break;

         case TERMINATION_FLAG:
            MA_terminate(0);
            break;

         default:
            fprintf (stdout, "Invalid selection\n");
            sleep (1); 
            break;
      }
   }

   n_1_connection_state = link_state;

   return;
}

/*****************************************************************************

     Description: This routine checks for user input when the -o option was 
                  entered

           Input: orpg_host - IP of RPG
                  orpg_port - TCP port number

          Output:

          Return: 

 *****************************************************************************/

void TERM_check_WAN_OTR_input (char *orpg_host, ushort orpg_port, char *password)
{
          char buf[BUFSIZ];  /* BUFSIZ defined in stdio.h */
          int  i;
          int  connected = 0;
          time_t clock, current_time;
   static int  number_active_items = NUMBER_OF_WAN_OTR_SELECTIONS;
   static int  selection = 0;
   static int  ret = 0;
   static char text_selections [NUMBER_OF_WAN_OTR_SELECTIONS+1] [BUFSIZ] = {
       {""},
       {"Send WAN One Time Request Once\0"},
       {"Send WAN One Time Request Repeatedly\0"},
       {"Toggle product save flag"}};  /* Note: keep this as the last entry in the
                                                array so the "toggle" logic is
                                                managed properly */
   
      selection = 0;
      /* refresh the main menu if no outstanding OTRs and we are not */
      /* making multiple OTRs (looping) and not running from a script */

      if ((ret != -1) && (Otrs_pending == 0) && !LoopOTRs && !Otr_script)  
      {
         if ((system ("clear")) >= 0)  
         {
            fprintf (stdout, "******************************\n");
            fprintf (stdout, "*                            *\n");
            fprintf (stdout, "*    NBTCP Tool Interface    *\n");
            fprintf (stdout, "*       WAN OTR Mode         *\n");
            fprintf (stdout, "*    RPG: %s   port:%d *\n",orpg_host,orpg_port);
            fprintf (stdout, "******************************\n");

            fprintf (stdout, "\n\n");

            for (i=1; i<= NUMBER_OF_WAN_OTR_SELECTIONS-1; i++) 
               fprintf (stdout, " %d - %s\n", i, text_selections[i]);

               /* display the current state of the "product save" flag
                  if products can be saved */
            if (PROC_get_product_dir () != NULL) {
               fprintf (stdout, " %d - %s ", 
                  PROD_SAVE_FLAG_OTR_INDX,
                  text_selections[PROD_SAVE_FLAG_OTR_INDX]);
               if (PROC_get_prod_save_flag () == TRUE)
                  fprintf (stdout, "(current state: save products)\n");
               else
                  fprintf (stdout, 
                           "(current state: products not saved)\n");
            }
            else
               number_active_items = NUMBER_OF_WAN_OTR_SELECTIONS - 1;

                        
            fprintf (stdout, " 9 - Terminate Tool\n");

            fprintf(stdout, "\nEnter selection: ");
            fflush (stdout);
         }
      }

      if (Otrs_pending == 0) {
  
         if (!Otr_script) {
            /* read stdin...-1 is returned if the user has not
               hit the <enter> key */
            ret = read (STDIN_FILENO, buf, 1); 
            if (ret != -1) {
               fprintf(stdout, "\n");

               if ((selection = atoi(buf)) == 0) {
                     printf ("Input is invalid\n");
                     sleep (1); 
               }
               else if ((selection > 0) && (selection <= number_active_items)) {
                     fprintf (stdout, "Processing \"%s\"\n", 
                              text_selections[selection]);
                     sleep (1); 
               }
            } 
            else
               selection = 0;
         }
         else
            selection = 1; /* When running from script, issue OTRs once.  */
      }
      
      switch (selection) {
         case 0:  /* If looping it may be time to re-issue the OTRs */

            current_time = time(&clock);

            if (LoopOTRs && Otrs_pending == 0 && current_time >= next_time){

               next_time = current_time + (RequestInterval*60);
              
               /* create and connect socket for one virtual circuit */
               connected = SOC_initiate_sock_connections (orpg_host, orpg_port, password, CLASS_2);

               if ((SM_send_sign_on_msg ()) == -1) {
                  fprintf (stderr,
                           "Error sending sign on message\n");
                  return;
               }
               
               system ("clear");
               printf ("Connected to %s on port %d since %s\n",
                                orpg_host,orpg_port,str_start_time);
               printf("**Enter control-c to stop.** \n\n");

               /* RPG should send general status message then a product list, get them */
            
               if ((SOC_pollsocks()) == -1) {
                   SOC_close_sockets ();
                   printf ("\nConnection lost. Returning...\n");
                   fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                   return;
               }
            
               if ((SOC_pollsocks()) == -1) {
                   SOC_close_sockets ();
                   printf ("\nConnection lost. Returning...\n");
                   fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                   return;
               }
 
               if ((SM_send_product_request_msg (PVC0_sock)) == -1){
                  fprintf (stderr, "Error sending WAN OTR Message\n");
                  return;
               }
            }
            else {
               if (LoopOTRs && Otrs_pending == 0)
               printf("\rNext OTRs will be issued in %d seconds...",
                   (int)(next_time - current_time)); fflush(stdout);
            }

            break;

         case 1:
         
            /* Send OTRs once */
               
            LoopOTRs = FALSE;
               
            /* create and connect socket for one virtual circuit */
            connected = SOC_initiate_sock_connections (orpg_host, orpg_port, password, CLASS_2);

            if ((SM_send_sign_on_msg ()) == -1) {
               fprintf (stderr,
                        "Error sending sign on message\n");
               return;
            }
            
            str_start_time = ctime(&clock);
            start_time = time (&clock);
        
            system ("clear");
            printf ("Connected to %s on port %d since %s\n",
                             orpg_host,orpg_port,str_start_time);
            printf("**Enter control-c to stop.** \n\n");

            /* RPG should send general status message then a product list, get them */
         
            if ((SOC_pollsocks()) == -1) {
                SOC_close_sockets ();
                printf ("\nConnection lost. Returning...\n");
                fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                return;
            }
            
            if ((SOC_pollsocks()) == -1) {
                SOC_close_sockets ();
                printf ("\nConnection lost. Returning...\n");
                fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                return;
            }
            
            if ((SM_send_product_request_msg (PVC0_sock)) == -1){
               fprintf (stderr, 
                        "Error sending WAN OTR Message\n");
               return;
            }
            
            break;

         case 2:
               
            /*Set looping flag */
               
            LoopOTRs = TRUE;
               
            /* create and connect socket for one virtual circuit */
            connected = SOC_initiate_sock_connections (orpg_host, orpg_port, password, CLASS_2);

            if ((SM_send_sign_on_msg ()) == -1) {
               fprintf (stderr,
                        "Error sending sign on message\n");
               return;
            }
 
            str_start_time = ctime(&clock);
            start_time = current_time = time (&clock);
            next_time = current_time + (RequestInterval*60);
        
            system ("clear");
            printf ("Connected to %s on port %d since %s\n",
                             orpg_host,orpg_port,str_start_time);
            printf("**Enter control-c to stop.** \n\n");

            /* RPG should send general status message then a product list, get them */
          
            if ((SOC_pollsocks()) == -1) {
                SOC_close_sockets ();
                printf ("\nConnection lost. Returning...\n");
                fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                return;
            }
            
            if ((SOC_pollsocks()) == -1) {
                SOC_close_sockets ();
                printf ("\nConnection lost. Returning...\n");
                fprintf(stderr, "Connection to ORPG %s Failed.\n", SOC_get_inet_addr()); 
                return;
            }
            
            if ((SM_send_product_request_msg (PVC0_sock)) == -1){
               fprintf (stderr, "Error sending WAN OTR Message\n");
               return;
            }
                  
            break;
             
         case 3:
            if (PROC_get_product_dir () == NULL){
               fprintf (stdout, "Invalid selection\n");
               sleep(1);
            }
            else
               PROC_toggle_prod_save_flag ();
            break;

         case TERMINATION_FLAG:
            printf ("Tool is terminating\n\n");
            sleep (1); 
            exit (0);
            break;

         default:
            fprintf (stdout, "Invalid selection\n");
            sleep (1); 
            break;
      }

   return;
}

/********************************************************************************

     Description: This routine initializes the terminal for non-blocking I/O

           Input:

          Output:

          Return: 0 on success; -1 on error

 ********************************************************************************/

int TERM_init_terminal ()
{
   int status; 


   if ((status = fcntl(STDIN_FILENO, F_GETFL, 0))  < 0) {
      fprintf (stderr, "Error reading the status flags for stdin\n");
      return (-1);
   }

   if (!Otr_only) {
      status |= (O_NONBLOCK | O_NDELAY);

      if (fcntl (STDIN_FILENO, F_SETFL, status) < 0) {
          fprintf (stderr, "Error setting non-blocking mode for stdin\n");
          return (-1);
      }
   }
   return(0);
}


/********************************************************************************

     Description: This routine gets the RPS file name from the user then
                  calls the routine to send the list to the RPG.

           Input:

          Output:

          Return: transaction_not_completed - Return values:
                                        0 - menu selection has been completed
                                        1 - menu selection has not been completed
                                       -1 - error occurred processing selection

 ********************************************************************************/

#define RPS_LIST_ARRAY_SIZE  100

static int Send_rps_list ()
{
          int  ret;
          int  i;
          int  transaction_not_completed = 1;
          char buf[BUFSIZ];  /* BUFSIZ defined in stdio.h */
   static int  max_num_items = 0;
   static int  return_to_main_menu = 0;
   static int  number_defined_rps_lists = 0;
   static char *rps_list_names [RPS_LIST_ARRAY_SIZE];
   static int  level_one_pending = 0;
   static int  level_two_pending = 0;
   static int  menu_selection;
   static int  menu_level = 1;

   if (number_defined_rps_lists == 0) {
      rps_list_names[0] = "";   /* subscript 0 is a hassle, so don't use it */
      rps_list_names[1] = MA_get_default_rps_file ();
      ++number_defined_rps_lists;
   }
 
      /* read stdin...-1 is returned if the user has not
         hit the <enter> key */

   ret = read (STDIN_FILENO, buf, FILENAME_MAX); 

   if (menu_level == 1) {
      if (level_one_pending == 0) {

         level_one_pending = 1;
/*         system ("clear"); */

         fprintf (stdout, "\n\nEnter Selection From List:\n");

            /* print the rps array to choose from */

         for (i = 1; i <= number_defined_rps_lists; i++)
             fprintf (stdout, " %d - %s file\n", i, rps_list_names[i]);

            /* include menu item for the new filename */
         fprintf (stdout, " %d - Enter new RPS filename\n", 
               number_defined_rps_lists + 1);

         return_to_main_menu = number_defined_rps_lists + 2;
         max_num_items = return_to_main_menu;

            /* include menu item to cancel this selection */
         fprintf (stdout, " %d - Return to Main Menu\n", 
               return_to_main_menu);
      }
 
      if (ret != -1) {
         fprintf(stdout, "\n");

         menu_selection = atoi(buf);

         if ((menu_selection > max_num_items) || (menu_selection == 0))
             fprintf (stderr, "Selection is invalid\n");
         else if (menu_selection == return_to_main_menu)
             transaction_not_completed = 0;
         else {
             menu_level = 2;
             ret = -1;   /* reset the read state */
         }
      }
      else
         return (transaction_not_completed);
   }


   if ((menu_level == 2) && (menu_selection != return_to_main_menu)) {
      if (menu_selection <= number_defined_rps_lists) {
         fprintf (stdout, "Sending RPS List: %s\n", 
                  rps_list_names[menu_selection]);
         if ((ret = SM_sendrps (rps_list_names[menu_selection])) == -1) {
            fprintf (stderr, "Error processing request\n");
            level_one_pending = 0;
            menu_level = 1;
            return (-1); 
         }
         transaction_not_completed = 0;
      } else {
         if (level_two_pending == 0) {
            fprintf (stdout, "Enter new rps filename: ");
            fflush (stdout);
            level_two_pending = 1;
         }
 
         if (ret != -1) {
            char *ptr;

            strcpy (&buf[ret-1], "\0");
            fprintf (stdout, "Sending RPS List: %s\n", 
                     buf);
            if ((ret = SM_sendrps (buf)) == -1) {
               fprintf (stderr, "Error processing request\n");
               level_one_pending = 0;
               level_two_pending = 0;
               menu_level = 1;
               return (-1); 
            }
            transaction_not_completed = 0;
            if ((ptr = malloc (ret)) == NULL) {
               fprintf (stderr, 
                          "Error malloc'ing buffer space for new rps filename\n");
               fprintf (stderr, 
                          "RPS file \"%s\" not added to the RPS array\n", buf);
            } else {  /* add new rps file to the list */
               int file_exists = 0;
  
               strcpy (ptr, buf);

                  /* if the file already exists in the list of files previously
                     processed, then don't add it to the list again */

               for (i=1; i<= number_defined_rps_lists; i++) {
                   if (strcmp (rps_list_names[i], ptr) == 0)
                      file_exists = 1;
               }

               if (file_exists)
                   free (ptr);
               else {

                     /* limit the list to the size of the array */

                  if ( number_defined_rps_lists < (RPS_LIST_ARRAY_SIZE - 1))
                     ++number_defined_rps_lists;
                  else  /* if the list is max'ed out, then just cycle the last
                           array element with all subsequent entries */
                      free (rps_list_names[number_defined_rps_lists]);

                     /* add file to the processed file list */

                  rps_list_names[number_defined_rps_lists] = ptr;
               }
            }
         }
      }
   }

   if (transaction_not_completed == 0) {
      menu_level = 1;
      level_one_pending = 0;
      level_two_pending = 0;
   }

   return (transaction_not_completed);
}
