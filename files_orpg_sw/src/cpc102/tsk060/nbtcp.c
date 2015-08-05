/*********************************************************************************

      File: nbtcp.c
            This file contains the main routines for the TCP-interfaced
             narrowband test tool.

      Note(s) from the original author: New version implementation of client 
                                        side for TCM ICD 2/5/01

 ********************************************************************************/


/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/20 21:25:47 $
 * $Id: nbtcp.c,v 1.19 2014/03/20 21:25:47 steves Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */


#include <ctype.h>     /*     isdigit (...)     */
#include <errno.h>
#include <fcntl.h>     /*     file system calls */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>     /* file, directory system calls */
#include <sys/stat.h>      /*             "                */

#include <comm_manager.h>
#include <cs.h>            /* RPG lib functions            */
#include <nbtcp.h>


   /* process level globals */

int  PVC0_sock, PVC1_sock; /* virtual circuit socket descriptors */
int  PIDS[MAXRPS];         /* product ids array */
char MNE[MAXRPS][4];       /* product mnemonic array */
/* One Time Request specific variables */
char Userpass[10];         /* Class 2 user password */
char Portpass[10];         /* Class 2 port password */
int  Otrs_pending = 0;     /* number of OTRs remaining to be received */
u_short RequestInterval = 6; /* minutes between repeating class 2 requests */
int LoopOTRs = FALSE;      /* True when sending repeated class 2 OTRs */
int Otr_only = FALSE;      
int Otr_script = FALSE;
int Ignore_special_codes = 0; /* Ignore all product codes < 16 expect GSM */
int Put_in_background = 0; /* Put nbtcp in the background. */
int Remove_products;	   /* If not 0, the age of products to delete from directory.  
			      Only applies when products written to directory. */
int  WMO_AWIPS_header_added = FALSE;	/* TRUE when WMO/AWIPS header added. */


   /* file scope globals */

static int     Logfd;                  /* task log file descriptor */
static char    Log_name [128];         /* tool log file */
static char    Log_buf [256];                 /* task msg buffer */
static short   Orpg_port = -1;         /* tcp port number to orpg */
static char    Orpg_host[MAX_NAME_LEN];/* orpg host name */
static char    Site_id[5];             /* product data source id */
static char    Password[10];           /* orpg logon password */
static int  RPS_list_required = FALSE; /* flag specifying when the RPS List
                                          needs to be sent to the RPG */
static char *File_dir;                 /* directory where the tool files are located */
static char Rps_filename  [FILENAME_MAX]; /* user supplied RPS file name */
static int  Socket_read_delay = FALSE; /* test flag used to delay the socket reads */
static int    Link_number = -1;        /* the link number used for cm_tcp */
static int    Interface_selected;      /* the comms interface selected */
static int    User_id = 454;           /* id of user */
static int    Link_connected;        /* state of the connection */
static int    Port_specified = FALSE; /* TRUE when user enters -p option */



   /* local function prototypes */

static void Check_cmdline_args ();
static void Get_file_naming_convention ();
static void Init_arrays ();
static void Print_cs_error ( char *msg);
static void Print_help (char **argv);
static int  Read_options (int argc, char **argv);
static void Read_rps_filename ();
static void Run_cmtcp_interface ();
static void Run_native_interface ();
static void Run_otr_interface ();
static int  Daemonize_nbtcp();


/********************************************************************************

     main routine

 ********************************************************************************/

int main(int argc, char **argv)
{
   char rps_list_path [FILENAME_MAX]; /* fully qualified RPS file name */

   sigset(SIGINT,  MA_terminate);
   sigset(SIGHUP,  MA_terminate);
   sigset(SIGTERM, MA_terminate);
   sigset(SIGHUP,  MA_terminate);
   sigset(SIGPIPE, SIG_IGN);

      /* register for CS services error reporting */

   CS_error ((void (*)())Print_cs_error);
    
      /* initialize the global variables */

   Password[0] ='\0';
   Orpg_host[0] ='\0';

      /* read options */

   if (Read_options (argc, argv) != 0)
        exit (-1);

      /* Check the command line arguments */

   Check_cmdline_args ();

      /* get the RPS file directory */

   File_dir = SM_init_data_dir ();

   if (!Otr_only) {
      /* build the master RPS file */

     if (BMF_init_prod_file (File_dir) < 0) {
        fprintf (stderr, "Error Initializing product file");
        exit (-1);
     }

        /* read the RPS list filename from the user */

     Read_rps_filename ();

        /* construct the path name of the file that contains the RPS list
           to be sent to the ORPG */

     strcpy (rps_list_path, File_dir);
     strcat (rps_list_path, Rps_filename);

        /* see if file exists */

     if (access(rps_list_path, 0) != 0) {  
        printf ("File error: %s -- %s\n", strerror (errno), rps_list_path);
        exit (-1);
     }
   }

/* printf("orpgname = %s\npassword = %s\npath = %s\nlinkindex = %d\nport = %d\n",
orpgname,password,filepath,linkindex,orpgport);     */
    
       /* do task logging to "nbtcp.log.port#" file */

    if (Interface_selected == CM_TCP_INTERFACE)
       sprintf (Log_name, "nbtcp.log.%d", Link_number);
    else
       sprintf (Log_name, "nbtcp.log.%d", Orpg_port);

    if ((Logfd = creat(Log_name, 0664)) == -1) {
        printf("Unable to create %s (%s)\n", Log_name, strerror (errno));
        exit(-1);
    }

       /* get the file naming convention for the product files */

    if (PROC_get_product_dir () != NULL) {
       if ( (!Otr_script) && (!WMO_AWIPS_header_added) )
          Get_file_naming_convention ();
       else {
          if( WMO_AWIPS_header_added )
             PROC_publish_naming_convention (8);
          else
             PROC_publish_naming_convention (1);
       }
    }

      /* initialize the global arrays */

   Init_arrays ();

      /* initialize the user terminal */

   if (TERM_init_terminal () == -1) {
      printf ("Error intializing the terminal\n");
      printf ("Program is aborting\n");
      exit (-1);
   }

      /* check the endianess of the local host machine */

   SM_set_byte_swap_flag ();

   if (Otr_only)
      sprintf(Log_buf, "Program started as WAN OTR only.\n");
   else {
      if (Interface_selected == NATIVE_INTERFACE) {
         sprintf (Log_buf, "Program started, connecting to %s\n",
                 Orpg_host);
         printf ("Connecting to %s\n", Orpg_host);
      } else
         sprintf (Log_buf, "Program started, connecting to %d\n",
                 Link_number);
   }

   MA_printlog (Log_buf);


   if (Otr_only)
      Run_otr_interface ();
   else {
      if (Interface_selected == NATIVE_INTERFACE)
         Run_native_interface ();
      else
         Run_cmtcp_interface ();
   }

   exit (0);
}


/******************************************************************************** 

     Description: This routine prints an error message and aborts the program

           Input: msgbuf - the msg to write to the log

          Output:

          Return:

 ********************************************************************************/

void MA_abort (char *msg)
{
   int ret;
   long timeval;
   char msgbuf[80];

   time( &timeval);
   sprintf(msgbuf, "%s  %s (err: %s), aborting execution\n",
          ctime(&timeval), msg, strerror (errno));

   printf ("\nProgram aborting; see log %s for details\n",
           Log_name);

   if((ret = write(Logfd, msgbuf, strlen(msgbuf))) < 0){
       printf("write: Unable to log message %d %d\n",ret, errno);
       exit(-1);
   }
   
   if ((Interface_selected == CM_TCP_INTERFACE) && CMT_lbs_created())
       CMT_write (CM_DISCONNECT, Link_number, NULL, 0);

   close(Logfd);
   exit(2);
}


/********************************************************************************

     Description: This routine returns the connection state when cm_tcp is used

           Input: state - the connection state reported by cm_tcp

          Output: 

          Return:

 ********************************************************************************/

int MA_get_connection_state (void)
{
   if (Link_connected == TRUE)
      return (CM_CONNECT);
   else
      return (CM_DISCONNECT);
}

/********************************************************************************

     Description: This routine returns the default RPS filename uspplied by
                  the user at tool startup

           Input:

          Output:

          Return: filename of the default rps file

 ********************************************************************************/

char *MA_get_default_rps_file ()
{
   return (Rps_filename);
}


/********************************************************************************

     Description: This routine returns the comms interface used

           Input: 

          Output: 

          Return: the comms interface used (i.e. cm_tcp or the native interface)

 ********************************************************************************/

int MA_get_interface () {
   return (Interface_selected);       
}


/********************************************************************************

     Description: This routine returns the link number - this is only used
                  with the cm_tcp interface

           Input: 

          Output: 

          Return: the link number 

 ********************************************************************************/

int MA_get_link_number () {
   return (Link_number);
}


/********************************************************************************

     Description: This routine returns the mnemonic of the RPG we're connected to

           Input:

          Output:

          Return: Site_id - The ID of the product data source

 ********************************************************************************/

char *MA_get_site_id ()
{
   return (Site_id);
}


/********************************************************************************

     Description: This routine returns the user id

           Input:

          Output:

          Return: Site_id - The ID of the product data source

 ********************************************************************************/

int MA_get_user_id ()
{
   return (User_id);
}

/********************************************************************************

     Description: This routine prints messages to the reporting log

           Input: msgbuf - the msg to write to the log

          Output:

          Return:

 ********************************************************************************/

void MA_printlog(char *msg)
{
   int ret;
   long timeval;
   char msg_buf [1024];

   time (&timeval);

   strcpy (msg_buf, ctime (&timeval));

      /* get rid of the newline char and add some spaces */

   strncpy (&msg_buf [strlen (msg_buf) - 1], "  \0",  3);

   strcat (msg_buf, msg);  /* cat the time stamp and log message */

   if((ret = write(Logfd, msg_buf, strlen(msg_buf))) < 0) {
       printf("write: Unable to log message %d %d\n",ret, errno);
       exit(-1);
    }

    return;       
}


/********************************************************************************

     Description: This routine assigns the "rps list required" flag

           Input: state - state (true/false) to set the "rps list required"
                          flag to

          Output: RPS_list_required

          Return:

 ********************************************************************************/

void MA_rps_required (int state)
{
   RPS_list_required = state;
   return;       
}


/********************************************************************************

     Description: This routine perfroms the normal process termination duties

           Input:

          Output:

          Return:

 ********************************************************************************/

void MA_terminate(int sig) {

   if (Interface_selected == CM_TCP_INTERFACE) {
       printf ("\nDisconnecting link: %d\n\n", Link_number);
       CMT_write (CM_DISCONNECT, Link_number, NULL, 0);
    }

   printf ("Task terminated (signal: %d)\n", sig);
   exit(0);
}


/********************************************************************************

     Description: This routine updates the connection state when cm_tcp is used

           Input: state - the connection state reported by cm_tcp

          Output: 

          Return:

 ********************************************************************************/

void MA_update_connection_state (int state)
{
   if (state == CM_CONNECT)
      Link_connected = TRUE;
   else
      Link_connected = FALSE;
   return;       
}


/********************************************************************************

     Description: This routine does basic checking for the comms interface 
                  selected

           Input: argv - command line argument list

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

static void  Check_cmdline_args () {

   if (Interface_selected == CM_TCP_INTERFACE) {
      if (Orpg_port != -1)  
         fprintf (stderr, 
          "Error: Port # option cannot be used with cm_tcp...option ignored\n");

      if (Password[0] != '\0')
            fprintf (stderr, 
              "Error: Password option cannot be used with cm_tcp...option ignored\n");

   }
   if (Otr_only && !Port_specified)
      Orpg_port = ORPGPORT_C2;
}


/********************************************************************************

     Description: This routine reads the RPS list filename from the terminal

           Input:

          Output: filename - name of the RPS list file

          Return:

 ********************************************************************************/

static void Get_file_naming_convention ()
{
   int selection;

   do {
     printf ("\n\nSelect the product files naming convention:\n");

     printf ("1. ProdMne_MsgCode_ElevCut_Site_id-Day-Year:Hr:Min:Sec\"Z\"");
     printf (" (original naming convention)\n");
     printf ("2. ProdMne_MsgCode_ElevCut_VSDate_VSTime_VS#_Site_id\n");
     printf ("3. MsgCode_ProdMne_VS#_ElevCut_Site_id\n");
     printf ("4. VS#_ElevCut_MsgCode_ProdMne_Site_id-Day-Year:Hr:Min:Sec\"Z\"\n");
     printf ("5. VS#_MsgCode_ProdMne_ElevCut_Site_id\n");
     printf ("6. \"b\"VS#_ProdMne_MsgCode_ElevCut_Site_id\n");
     printf ("7. PS.PUPID_SC.U_DI.C_DC_RADARi.AR_VICINITY_PA.RADAR-ICAO-PCODE-...\n");
   
     scanf ("%d", &selection);

   } while ((selection < 0) || (selection > 7));

   PROC_publish_naming_convention (selection);

   return;
}


/********************************************************************************

     Description: This routine initializes the PIDS and MNE (ie. product id and
                  product mnenomic) arrays

           Input:

          Output: PIDS - Product id array
                  MNE  - Product mnemonic array

          Return:

 ********************************************************************************/

static void Init_arrays ()
{
      /* fill the pid, mnemonic arrays for pid's always received 
         but not in the RPS list */
   
   PIDS[0] = 2;              /* msg code */
   strcpy(MNE[0],"GSM");     /* gen status msg */
   PIDS[1] = 3;
   strcpy(MNE[1],"PRR");     /* request response msg */
   PIDS[2] = 6;
   strcpy(MNE[2],"AAP");     /* alert adapt parameter msg */
   PIDS[3] = 9;
   strcpy(MNE[3],"AM");      /* alert msg */
   PIDS[4] = -1;

   return;
}


/********************************************************************************

    Description: This routine prints Configuration Support (CS) errors to the
                 std err and to the process log. The CS service performs the
                 reading/parsing of the message files.

          Input: The error msg to print (msg supplied by CS)

         Output:

         Return:

 ********************************************************************************/

void Print_cs_error ( char *msg)
{
   fprintf (stderr, "\n%s\n", msg);

   if(write(Logfd, msg, strlen(msg)) < 0)
       printf("write: Unable to log error: %s\n", strerror (errno));

   sleep (2);
   return;
}


/********************************************************************************

     Description: This routine prints the help information to the stdout

           Input: argv - command line argument list

          Output:

          Return:

 ********************************************************************************/

static void Print_help (char **argv)
{
   printf ("\nUsage: %s [options] orpg_host | link number\n", argv[0]);
   printf ("      options:\n");
   printf ("         -A Add WMO/AWIPS header (dft = do not add)\n");
   printf ("         -C Country of Origin for use in the WMO/AWIPS header (dft = US)\n");
   printf ("         -R Remove products from directory older than X minutes (dft = 0 (Never))\n");
   printf ("         -P Purge directory at startup (dft = No)\n");
   printf ("         -d Directory where product files are written\n");
   printf ("         -D Same as \"-d\" except any files in directory will be deleted\n");
   printf ("         -p Orpg port number (dft = %d)\n", ORPGPORT);
   printf ("         -s Site_id ID of the product data (dft = %s)\n", ORPGNAME);
   printf ("         -t Test flag used to set 1 second delay between socket reads (dft = not set)\n");
   printf ("         -u User id (dft = %d)\n", User_id);
   printf ("         -w Orpg logon password (dft = %s)\n", PASSWORD);
   printf ("         -c Make script-based Class 2 one time request via WAN (no RPS list)\n");
   printf ("         -o Make interactive Class 2 one time requests via WAN (no RPS list)\n");
   printf ("            -j Class 2 User password (dft = %s)\n",Userpass);
   printf ("            -l Class 2 Port password (dft = %s)\n",Portpass);
   printf ("            -r Class 2 Request repeat interval in minutes, (dft = %d)\n",RequestInterval);
   printf ("         -i If -d used, will not store products with codes < 16 (dft = Do Not Ignore)\n");
   printf ("         -b Make nbtcp go to background ... only works with TCP and native interface\n");
   printf ("         -h Help, these messages\n");
   printf ("\n");
   printf ("The last argument can either be a hostname/IP address, or a link\n");
   printf ("number referenced in the comms_link.conf file. If a link number\n");
   printf ("is entered (range of 0-99), the tool will use cm_tcp to perform\n");
   printf ("the communications services; otherwise, the tool will use its\n");
   printf ("native interface to perform the communications services\n");
   exit (0);
}


/********************************************************************************

    Description: This function reads command line arguments.

    Inputs:      argc - number of command line arguments
                 argv - the list of command line arguments

    Return:      It returns 0 on success or -1 on failure.

 ********************************************************************************/

static int Read_options (int argc, char **argv)
{
   extern char *optarg;    /* used by getopt */
   extern int optind;
   int err;                /* error flag */
   int number_args = 1;    /* # cmdline args parsed (used for error checking) */
   int c;
   char last_arg [MAX_NAME_LEN];
   int  i;
   int  add_wmo_awips_header = 0;
   int  icao_defined = 0;
   int  delete_files = 0;

   err = 0;

   strcpy (Site_id, ORPGNAME);
   strcpy (last_arg, argv[argc -1]);

      /* detemine which comms interface to use (the max line number allowed 
         is 99) */
  
   if (strlen(last_arg) > 2)
      Interface_selected = NATIVE_INTERFACE;
   else {
      for (i = 0; i < strlen (last_arg); i++) {
         if (!isdigit((int)last_arg[i])) {
            printf ("Invalid link number entered (arg: %s)\n",
                    last_arg);
            Print_help(argv);
         }
      }
      Interface_selected = CM_TCP_INTERFACE;
   }

   if (Interface_selected == CM_TCP_INTERFACE)
       Link_number = atoi(last_arg);
   else { /* last arg must be either an IP address or host name */
      strcpy (Orpg_host, last_arg);
         /* initialize the other globals to their default values */
      strcpy (Password, PASSWORD);
      Orpg_port = ORPGPORT;
   }

   Remove_products = 0;
   while ((c = getopt (argc, argv, "AC:R:D:bid:p:s:u:w:j:l:r:coht?")) != EOF) {
	    switch (c) {

         case 'A':
            add_wmo_awips_header = 1;
            break;

         case 'C':
            WAH_add_country_of_origin(optarg);
            break;

         case 'R':
            if(sscanf (optarg, "%d", &Remove_products) != 1)
              err = 1;

            /* Convert minutes to seconds. */
            Remove_products *= 60;
            break;

         case 'b':
            Put_in_background = 1;
            break;

         case 'i':
            Ignore_special_codes = 1;
            break;

         case 'D':
            delete_files = 1;
         case 'd':  /* product log directory name */
            number_args +=2;

            if (PROC_init_product_dir (optarg, delete_files) == -1) {
               printf ("Error initializing Product directory, ");
               printf ("program is aborting\n");
               err = -1;
            }
            break;

         case 'o':   /* WAN OTR interactive mode only */
            ++number_args;
            Otr_only = TRUE;
            Otr_script = FALSE;
            break;

         case 'c':   /* WAN OTR script mode only */
            ++number_args;
            Otr_only = TRUE;
            Otr_script = TRUE;
            break;

          case 'j':
            number_args +=2;
            strncpy (Userpass, optarg,10);
            Userpass[9] = '\0';
            break;

         case 'l':
            number_args +=2;
            strncpy (Portpass, optarg,10);
            Portpass[9] = '\0';
            break;

         case 'r':
            number_args +=2;
            if(sscanf (optarg, "%hu", &RequestInterval) != 1)
              err = 1;
            break;


         case 's':
            number_args += 2;
	    strncpy (Site_id, optarg, 4);
	    Site_id[4] = '\0'; 
            icao_defined = 1;
	    break;
	    
         case 'p':
            number_args += 2;
	    if(sscanf (optarg, "%hd", &Orpg_port) != 1)
	       err = 1;
            else
               Port_specified = TRUE;
            break;

         case 't':
            ++number_args;
            Socket_read_delay = TRUE;
	          break;
	    
         case 'u':
            number_args += 2;
	          if(sscanf (optarg, "%d", &User_id) != 1)
	             err = 1;
            break;

         case 'w':
            number_args += 2;
	    strncpy (Password, optarg, 10);
	    Password[9] = '\0';
	    break;

	 case 'h':
	 case '?':
	    Print_help (argv);
	    break;

	 default:
	    printf ("Unexpected option - %c\n", c); 
	    err = 1;
	    break;
      }

   }

      /* do basic cmd line argument error checking */

   if ((argc < 2) || (argc <= number_args)) {
      printf ("\nError: Command line argument list is invalid\n");
      Print_help (argv);
   }

   if( (add_wmo_awips_header) && (!icao_defined) ) {
      printf ("\nAdd WMO/AWIPS header requires Site ID\n" );
      err = 1;
   }
  
   if (err == 1) 		/* print usage message */
      Print_help (argv);

   if( (add_wmo_awips_header) && (icao_defined) )         
      WMO_AWIPS_header_added = WAH_add_awips_wmo_hdr(Site_id);

	return (0);
}


/********************************************************************************

     Description: This routine reads the RPS list filename from the terminal

           Input:

          Output: filename - name of the RPS list file

          Return:

 ********************************************************************************/

static void Read_rps_filename ()
{
   printf ("Enter RPS list filename: ");
   scanf ("%s", Rps_filename);
   strncpy (&Rps_filename[FILENAME_MAX -1], "\0", 1);
   return;
}


/********************************************************************************

     Description: This is the main routine that executes when the cm_tcp
                  comms manager is selected as the comms interface. Using this
                  interface, nbtcp reads and writes to/from a Linear Buffer(LB)
                  instead of directly to/from the sockets. The system must be 
                  setup to use cm_tcp (i.e. the comms_link.conf and tcp.conf 
                  files must be properly configured, and the "CFG_DIR" 
                  environment variable must be set to the directory where these
                  configuration filea are located. For more information on cm_tcp,
                  refer to the cm_tcp API and the TCP/IP ICD.

           Input: 

          Output:

          Return:

 ********************************************************************************/

#define CONNECT_RETRY_TIME  15

static void Run_cmtcp_interface () {

   time_t connect_start_time;
   int    ret;
   int    n_1_connect_state;
   static int in_background = 0;

      /* create the request and response LBs */

   if (CMT_create_lbs (Link_number) == -1) {
      fprintf (stderr, "Error creating LBs\n");
      MA_abort ("Tool is aborting");
   }

   Link_connected = FALSE;

   n_1_connect_state = -1;

   connect_start_time = 0;

      /**************
       *             *
       *  main loop  *
       *             *
       ***************/
           
   for(;;) {

         /* if we're not connected, command cm_tcp to connect */
      
      if (Link_connected == FALSE) {
         if (Link_connected != n_1_connect_state) {
            printf ("\n\nConnecting to RPG...\n");
            n_1_connect_state = Link_connected;
         }
 
         if ((time(NULL) - connect_start_time) > CONNECT_RETRY_TIME) {
            CMT_write (CM_CONNECT, Link_number, NULL, 0);
            connect_start_time = time (NULL);
         }
      } else {  /* link is connected */

         if (RPS_list_required == TRUE) {
            if (SM_sendrps (Rps_filename) == 0)
                RPS_list_required = FALSE;
         }

         n_1_connect_state = Link_connected;
      }

         /* Do we need to put in background? */

      if( Put_in_background && !in_background) 
         in_background = Daemonize_nbtcp();
  
         /* check for user input */

      if( !in_background )
          TERM_check_input (Link_connected, Interface_selected, Link_number);
      
         /* see if there's anything to read */

      if ((ret = CMT_read (Link_number)) < 0)
         printf ("LB_read error (err: %d)\n", ret);

      sleep (1);
/* printf ("Link state: %d\n", Link_connected); */
   }
}


/********************************************************************************

     Description: This is the main routine that executes when the native
                  interface is selected. This interface manages the sockets, 
                  performs logon authentication, and directly reads/writes 
                  the messages to/from the RPG IAW the applicable ICD(s).

           Input: 

          Output:

          Return:


 ********************************************************************************/

static void Run_native_interface () {

   static int in_background = 0;

      /**************
       *             *
       *  main loop  *
       *             *
       ***************/
           
   for(;;) {
         /* create and connect sockets for all the virtual circuits */
      
      if(Link_connected == FALSE)  {
         Link_connected = SOC_initiate_sock_connections (Orpg_host, Orpg_port, 
                             Password, CLASS_1);
      }
      else { /* connected and log'ed on, start data transfers */

         if (RPS_list_required == TRUE) {
            if (SM_sendrps (Rps_filename) == 0)
                RPS_list_required = FALSE;
         }
		         
            /* poll the sockets to see if there's data to read */
 
         if ((SOC_pollsocks()) == -1) {
             SOC_close_sockets ();
             Link_connected = FALSE;
             printf ("\nConnection lost, attempting to reconnect\n");

             sprintf(Log_buf,
                 "Connection to ORPG %s Failed\n  Attempting to reconnect\n",
                 SOC_get_inet_addr()); 
             MA_printlog(Log_buf);
         }

            /* Do we need to put in background? */

         if( Put_in_background && !in_background) 
            in_background = Daemonize_nbtcp();

            /* check for user input */

         if( !in_background )
            TERM_check_input (Link_connected, Interface_selected, Orpg_port);

              /* this sleep throttles the rate at which products are sent 
                 to the tool. a one second delay is enough to cause the 
                 products to back up in the output queue on the RPG host */

           if (Socket_read_delay == TRUE)
              sleep (1);
      }
   }
}

/********************************************************************************

     Description: This routine daemonizes nbtcp.

          Input: 

          Output:

          Return: Flag to indicate if process in background.


********************************************************************************/
static int Daemonize_nbtcp(){

   pid_t pid, sid;

      /* Fork off the parent process. */

   pid = fork();
   if(pid < 0){

      Put_in_background = 0;
      return(0);

   }

      /* We got a good PID .. If I am the parent, I need to die ... */

   else if( pid > 0 )
      exit(0);

      /* All the rest is in the child process. */
      /* Change the file mode mask. */

   umask(0);

      /* Create a new SID for the child process. */

   sid = setsid();
   if( sid < 0 ){
      fprintf( stderr, "setsid() Failed.\n" );
      exit(0);
   }

      /* Close out the standard file descriptors. */

   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);

   return(1);
}

static void Run_otr_interface () {


   for(;;) {
      /* Processing only WAN OTRs */
      TERM_check_WAN_OTR_input (Orpg_host, Orpg_port, Password);

      /* poll the sockets to see if there's data to read */
 
      if ((SOC_pollsocks()) == -1 && !LoopOTRs) {
          SOC_close_sockets ();
          Link_connected = FALSE;
          printf ("\nConnection lost. Terminating...\n");

          sprintf(Log_buf,
              "Connection to ORPG %s Failed\n",
              SOC_get_inet_addr()); 
          MA_printlog(Log_buf);
          break; /* Just quit for now */
      }
   } /* end for loop */
}
