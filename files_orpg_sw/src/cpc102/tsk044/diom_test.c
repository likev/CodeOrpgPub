/********************************************************************************
    
    File: diom_test.c

     Description: This file contains the routines used to test the D/I and D/O
                  functionality of the Advantech ADAM 6051 Digital Input/Output 
                  (DIO) Module.  All commands/responses in this program are 
                  sent/received using the MODBUS Application Protocol over TCP/IP.


 ********************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:49:30 $
 * $Id: diom_test.c,v 1.4 2014/03/18 18:49:30 jeffs Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define DIO_MODBUS_PORT       502
#define DIO_NO_CMD_PENDING     -1
#define BUFFER_SIZE          1000

   /* ADAM 6051 Digital I/O module commands */

#define DIO_READ_DO_STATE    0x01
#define DIO_READ_DI          0X02
#define DIO_SET_DO           0x05
#define DIO_RESET_ALL_DOS    0x0f
#define DIO_CYCLE_DO         0X7F    /* internally defined command */


#define DIO_DO_START_ADDR      16    /* D/O start address space */
#define DIO_DI_START_ADDR       0    /* D/I start address space */
#define DIO_COMMS_CONTROLLER_DO 0    /* D/O used to perform channel switching */

   /* size of command messages in bytes */

#define DIO_READ_DO_CMD_LEN    6
#define DIO_SET_DO_CMD_LEN     6
#define DIO_RESET_DOS_CMD_LEN  9
#define DIO_READ_DI_CMD_LEN    6


#define ushort unsigned short
#define uchar  unsigned char


   /* global variable declarations */

typedef struct {
   ushort xaction_id;
   ushort protocol_id;
   ushort msg_len; 
}  cmd_hdr_t;


typedef struct {
   uchar  unit_id;
   uchar  function_code;
   ushort parameter_1;
   ushort parameter_2;
   uchar  parameter_3[4];
}   cmd_body_t;


typedef struct {
   cmd_hdr_t  hdr;
   cmd_body_t command;
} modbus_msg_t;


static modbus_msg_t Message;
static struct sockaddr_in  Sinaddr;
static int Verbose;
static int Socket_closed;
static int Reset_module_ip_addr;


   /* local function prototypes */

static int  Check_cmd_ack (int sd, int cmd_pending);
static int  Connect_socket (int sock_descriptor);
static int  Construct_cmd (uchar cmd);
static void Initialize_fixed_msg_fields ();
static int  Interrogate_response (int sd, int last_cmd_sent);
static void Menu_selection (int *command, int sd);
static int  Open_socket (char *ip_address);
static int  Poll_socket (int sd);
static void Print_usage (char **argv);
static int  Read_dio_msg (int sock_desc, char *buf, int buf_size);
static int  Read_options (int argc, char **argv);
static void Reset_conn (int signal); 
static void Select_channel (int *channel_selected);
static int  Send_command (int sd);
static int  Set_properties (int sd);
static void Terminate_proc (int signal);


/********************************************************************************

          main

 ********************************************************************************/

int main (int argc, char **argv) {

   int sock_d = -1;  /* socket descriptor */
   int socket_connected = 0;
   char module_ip_address [INET_ADDRSTRLEN];

   Socket_closed = 1;
   Verbose = 0;
   Reset_module_ip_addr = 0;

      /* get the module's ip address from the command line */

   if (argc < 2) {
      fprintf (stdout, "Error: IP address not entered on command line\n");
      exit (1);
   }

   strncpy (module_ip_address, argv[argc - 1], 15);
   module_ip_address [15] = '\0';

   if (Read_options (argc, argv) == -1) {
      fprintf (stderr, "Error parsing command line options\n");
      exit (1);
   }

      /* set up the signal handlers */

   sigset (SIGPIPE, Reset_conn);

   sigset (SIGTERM, Terminate_proc);
   sigset (SIGHUP,  Terminate_proc);
   sigset (SIGINT,  Terminate_proc);

      /* select the channel number to test...obsolete code but leave in for
         future reference */

/*   Select_channel (&channel_to_test); */

      /* initialize the fixed message fields */

   Initialize_fixed_msg_fields ();

      /* infinite loop testing the module I/O */

   while (1) {
      int command_pending = DIO_NO_CMD_PENDING + 1;  /* set to something other
                                                        than the macro */

         /* open the socket */

      if (Socket_closed) {
         fprintf (stdout, "Attempting to open the socket");
         socket_connected = 0;

         while (Socket_closed) {
            if ((sock_d = Open_socket (module_ip_address)) < 0) { 
               fprintf (stdout, ".");
               sleep (1);
            } else {
               Socket_closed = 0;
               fprintf (stdout, "\nSocket opened\n");
            }
         }
      }

         /* attempt to connect to the server */

      if (!socket_connected)
         fprintf (stdout, "Attempting to connect to DIO module (IP address: %s)",
                  module_ip_address);

      while (!socket_connected) {
         fflush (stdout);   /* print the dots */

         if ((Connect_socket (sock_d)) < 0) {
            sleep (1);
            fprintf (stdout, ".");
         } else {
           socket_connected = 1;
           fprintf (stdout, "\nSocket connected\n");
         }
      }

         /* get the menu selection */

      Menu_selection (&command_pending, sock_d);

         /* process command */

      fprintf (stdout, "\n");
      Construct_cmd (command_pending);

      if (Send_command (sock_d) != 0) {
         fprintf (stderr, "Error sending command %d\n", command_pending);
         continue;
      } else if (Verbose)
         fprintf (stdout, "Command %d sent\n", command_pending);

         /* check for command ack from the DIO module */

      if (Check_cmd_ack (sock_d, command_pending) < 0) 
         fprintf (stderr, "Command ack failed for command %d\n",
                  command_pending);
      else if (Verbose)
         fprintf (stdout, "Command %d acknowledged\n", command_pending);

   }

   exit (0);
}


/********************************************************************************

     Description: Get the menu selection from the user

 ********************************************************************************/

static void Menu_selection (int *command, int sd) {

   static int selection = 0;

   int cmd [5] = {0,
                  DIO_SET_DO,
                  DIO_RESET_ALL_DOS,
                  DIO_CYCLE_DO,  /* used to automate setting, resetting the D/O */
                  DIO_READ_DI};

      /* if the last command was "cycle D/O", then wait 5 seconds and 
         reset the D/O */

   if (cmd [selection] == DIO_CYCLE_DO) {
      sleep (5);
      selection = 0;
      *command = DIO_RESET_ALL_DOS;
      return;
   }

      /* display the menu and wait for a selection */

   fprintf (stdout, "\n");
   fprintf (stdout, "1. Set D/O\n");
   fprintf (stdout, "2. Reset D/O\n");
   fprintf (stdout, "3. Cycle D/O on/off (5 second cyle time)\n");
   fprintf (stdout, "4. Read D/I\n");
   fprintf (stdout, "5. Exit and do not reset the DIO module IP address\n");
   fprintf (stdout, "6. Exit and reset the DIO module IP address. ");
   fprintf (stdout, "Use this option before \n");
   fprintf (stdout, "   returning the module to service\n");

   do {
      fprintf (stdout, "Enter selection: ");

      fscanf (stdin, "%d", &selection);

   } while ((selection < 1) || (selection > 6));

   if (cmd [selection] == DIO_CYCLE_DO)
      *command = DIO_SET_DO;
   else if ((selection == 5) || (selection == 6)) {

        if (selection == 5)
           Reset_module_ip_addr = 0;

        if (selection == 6)
           Reset_module_ip_addr = 1;

        Terminate_proc(0);
   } else {
   Reset_module_ip_addr = 1;
      *command = cmd [selection];
   }

   return;

}


/********************************************************************************

     Description: Verify the command acknowledgement received from the DIO module

 ********************************************************************************/

static int Check_cmd_ack (int sd, int cmd_pending) {

   int ret;

   if (Poll_socket (sd) < 0) {
      if (Verbose)
         fprintf (stderr, "socket poll failed for command %d (errno: %d)\n", 
                  cmd_pending, errno);
      perror ("socket poll failed");
      ret = -1;
   } else {   
         /* there's something to read */
      if ((Interrogate_response (sd, cmd_pending)) < 0) {
         fprintf (stderr, "Interrogation of message response failed\n");
         ret = -1;
      }
      else
         ret = 0;
   }

   return (ret);
}


/********************************************************************************

     Description: Establish a connection with the DIO module

 ********************************************************************************/

static int Connect_socket (int sock_descriptor) {
   int ret;
   int err = errno;

      /* connect to the server */

   ret = connect(sock_descriptor, (struct sockaddr *)&Sinaddr, sizeof(Sinaddr));

   if (ret < 0) {

      err = errno;

      if (err == EISCONN)
         return (0);
      else {
         if ((err != EALREADY) && (err != EINPROGRESS))
             fprintf (stdout, "socket connect error (errno: %d)\n", errno);
/*            perror ("connect () error"); */
         return (-1);
      }
   }
   else
      return (0);
}


/********************************************************************************

     Description: Construct the command in MODBUS format.

 ********************************************************************************/

static int Construct_cmd (uchar cmd) {

   static ushort transaction_id = 0;

      /* increment xaction id */

   Message.hdr.xaction_id = htons (++transaction_id);

      /* specify which D/O to control */

   Message.command.parameter_1 = htons (DIO_COMMS_CONTROLLER_DO + DIO_DO_START_ADDR);

   switch (cmd) {

      case DIO_SET_DO:
         Message.command.function_code = cmd;

            /* assign the value to set the D/O */

         Message.command.parameter_2 = htons (0xff00);

            /* specify the message length */

         Message.hdr.msg_len = DIO_SET_DO_CMD_LEN;
      break;

      case DIO_READ_DO_STATE:
         Message.command.function_code = cmd;

            /* specify number of coils to read */

         Message.command.parameter_2 = htons (0x0001);

            /* specify the message length */

         Message.hdr.msg_len = DIO_READ_DO_CMD_LEN;
      break;

      case DIO_RESET_ALL_DOS:
         Message.command.function_code = cmd;

            /* specify the start address of the D/Os */

         Message.command.parameter_1 = htons (DIO_DO_START_ADDR);

            /* specify number of D/Os to reset */

         Message.command.parameter_2 = htons (0x0002);

            /* define the number of bytes required to hold the
               bit pattern and the bit values (ie 0) to force the 
               D/Os off  */

         Message.command.parameter_3[0] = 1; /* # bytes used for bit pattern */
         Message.command.parameter_3[1] = 0; /* first 8 D/Os...we only have 2 */
         Message.command.parameter_3[2] = 0; /* second 8 D/Os...is not used */

            /* specify the message length */

         Message.hdr.msg_len = DIO_RESET_DOS_CMD_LEN;
      break;

      case DIO_READ_DI:
         Message.command.function_code = cmd;

            /* specify the start address of the D/Is */

         Message.command.parameter_1 = htons (DIO_DI_START_ADDR);

            /* specify number of D/Is to read */

         Message.command.parameter_2 = htons (0x0001);

            /* specify the message length */

         Message.hdr.msg_len = DIO_READ_DI_CMD_LEN;

      break;

   }

   return (0);
}


/********************************************************************************

     Description: Initialize the fixed message fields

 ********************************************************************************/

static void Initialize_fixed_msg_fields () {

   Message.hdr.protocol_id = 0;
   Message.command.unit_id = 1;

   return;

}


/********************************************************************************

     Description: Interrogate the message received from the DIO module

 ********************************************************************************/
 
static int Interrogate_response (int sd, int last_cmd_sent) {
 
   modbus_msg_t *msg_read;
   char  buffer[BUFFER_SIZE];
   uchar temp;
   char  *buffer_ptr;
   short *short_ptr;
   int   bytes_read;
   uchar response_byte_cnt;
   int   i;

      /* read the message received from the DIO module */

   if ((bytes_read = Read_dio_msg (sd, buffer, BUFFER_SIZE)) == -1) {
     perror ("");
     if (Verbose)
        fprintf (stderr,"socket read failed (errno: %d)\n",
                 errno);
      return (-1);
   }

   if (Verbose)
      fprintf (stdout, 
               "Message response received for command %d (msg len: %d)\n", 
               last_cmd_sent, bytes_read);

      /* set the pointer to the function code (ie command)
         in the read buffer */

   buffer_ptr =  buffer + 1 + sizeof (cmd_hdr_t);

      /* if the MSB is set in the function code, then
         the command failed */

   if ((char ) *buffer_ptr < 0) {
      fprintf (stderr, "Function Code MSB is set; command %d failed\n",  
               last_cmd_sent);
      return (-1);
   }

   switch (last_cmd_sent) {

      case DIO_SET_DO:
         msg_read = (modbus_msg_t *) buffer;

/* this code is probably not needed since both the outgoing command and 
   incoming response should be in big endian format. 

         msg_read->xaction_id = ntohs (msg_read->xaction_id);
         msg_read->protocol_id = ntohs (msg_read->protocol_id);
         msg_read->msg_length = ntohs (msg_read->msg_length);
         msg_read->parameter_1 = ntohs (msg_read->parameter_1);
         msg_read->parameter_2 = ntohs (msg_read->parameter_2);
*/

         if ((memcmp((const void *)msg_read, (const void *)&Message,
                      bytes_read)) == 0) {
        
            if (Verbose) 
               fprintf (stdout, "Command %d validated....cmd succeeded\n", 
                        last_cmd_sent);
/*            fprintf (stdout, "Wait 5 seconds then turn D/O off\n");
            sleep (5); */
            return (0);
         } else {
             fprintf (stdout, "Message command/response compare failed\n");
             fprintf (stdout, "   Module command header: %2x %2x %2x\n",
                      Message.hdr.xaction_id,
                      Message.hdr.protocol_id,
                      Message.hdr.msg_len);
             fprintf (stdout, "   Module command body: %2x %2x %2x %2x\n",
                      Message.command.unit_id,
                      Message.command.function_code,
                      Message.command.parameter_1,
                      Message.command.parameter_2);
             fprintf (stdout, "   Module response: %2x %2x %2x %2x\n",
                      msg_read->command.unit_id,
                      msg_read->command.function_code,
                      msg_read->command.parameter_1,
                      msg_read->command.parameter_2);
             return (-1);
         }
      break;

      case DIO_READ_DO_STATE:

         if (Verbose)
            fprintf (stdout, "Function code returned:    %2x\n", 
                     (uchar) (*buffer_ptr));

         response_byte_cnt = (uchar) *(buffer_ptr + 1);

         if (Verbose)
            fprintf (stdout, "Byte count:                 %d\n", 
                     response_byte_cnt);

         for (i = 0; i < response_byte_cnt; i++) {
            fprintf (stdout, "Coils state for byte # %d: x%02x\n", 
                     i + 1, (uchar) *(buffer_ptr + i));
         }
         
      break;

      case DIO_RESET_ALL_DOS:

         if (Verbose) {
            fprintf (stdout, "Function code returned: %02x\n", 
                     (uchar) (*buffer_ptr));

            ++buffer_ptr;
            short_ptr = (short *) buffer_ptr; 
            fprintf (stdout, "D/O start address: %2x\n", 
                     ntohs((ushort) (*short_ptr)));

            buffer_ptr = buffer_ptr + 2;
   
            fprintf (stdout, "Requested number of coils to force: %x\n", 
                     ntohs((ushort) (*buffer_ptr)));
         }

      break;

      case DIO_READ_DI:

         if (Verbose)
            fprintf (stdout, "Function code returned: %02x\n", 
                     (uchar) (*buffer_ptr));

         buffer_ptr = buffer_ptr + 1;
  
         response_byte_cnt = (uchar) *(buffer_ptr);

         if (Verbose)
            fprintf (stdout, "Byte count: %d\n", 
                     response_byte_cnt);

         buffer_ptr = buffer_ptr + 1;

         for (i = 0; i < response_byte_cnt; i++) {

               /* mask all bits but bit 1, then complement the 
                  bit so that "1" = D/I is set and "0" = D/I 
                  not set */

            temp = ~(0xfe | ((uchar)*(buffer_ptr + i)));

            fprintf (stdout, "D/I state for bit # 1: %01x\n", 
                     temp);
         }

      break;

     default:
        fprintf (stderr, 
                 "Invalid command_pending command received for interrogation\n");
        return (-1);
     break;
   }
   return (0);
}


/********************************************************************************

     Description: Open the socket

 ********************************************************************************/ 

static int Open_socket (char *ip_address) {
/*   char *module [] = {"diomodule",
                      "diomodule2"};
   struct hostent *hp;
*/
   int sd;
   int ret;

/* NOTE: This code is left in for future reference to use the channel number 
         to obtain the hostname ip address. It was left in just in case
         we want to revert back to using this method one day. 

   if ((channel_num < 1) || (channel_num > 2)) {
      fprintf (stderr,"Invalid channel number received (channel num: %d)\n",
               channel_num);
      return (-1);
   }

   hp = gethostbyname(module[channel_num - 1]);

   if(!hp) {
      fprintf (stderr, "Could not resolve hostname \"%s\" in hosts table\n",
              module[channel_num - 1]);
      return(-1);
   }

   bcopy(hp->h_addr, (char *)&Sinaddr.sin_addr, hp->h_length);
*/

      /* assign the module's ip address */

   bzero (&Sinaddr, sizeof(Sinaddr));
   Sinaddr.sin_family = AF_INET;

   if ((ret = inet_pton (AF_INET, ip_address, &Sinaddr.sin_addr)) != 1) {
      if (ret == 0)
         fprintf (stdout,
                  "\nInvalid dotted-decimal IP address entered: %s\n",
                  ip_address);
      else
         fprintf (stdout, "IP address unknown: %s  (errno: %d)\n",
               ip_address, errno);
      exit (1);
   }

   Sinaddr.sin_port = htons((ushort) DIO_MODBUS_PORT);

   if ((sd = socket (Sinaddr.sin_family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
       fprintf(stderr, "call to \"socket\" failed, errno =  %d\n",
               errno);
       return(-1);
   }

      /* set the connection properties */

   if ((Set_properties (sd)) < 0) {
       fprintf (stderr, "Error setting connection properties\n");
       close (sd);
       return (-1);
   }

   return (sd);
}


/********************************************************************************

     Description: Poll the socket and wait for a response from the last
                  command sent

 ********************************************************************************/

static int Poll_socket (int sd) {
   int ret;
   fd_set fd_rset;
   struct timeval timeout;

      /* set response timeout to 10 seconds */

   timeout.tv_sec = 10;
   timeout.tv_usec = 0;

      /* specify the descriptors to poll */

   FD_ZERO (&fd_rset);
   FD_SET (sd, &fd_rset);

      /* check for something to read from the socket */

   if ((ret = select (sd + 1, &fd_rset, NULL, NULL, &timeout)) <= 0) {
      if (ret < 0)   /* an error occurred */
         fprintf (stderr, "select() failed (errno: %d)\n", errno);
      return (-1);
   } else
      return (0);
}


/********************************************************************************

     Description: Print the tool usage message

 ********************************************************************************/

static void Print_usage (char **argv) {

   fprintf (stdout, "\nUsage: %s [options] module_ip_address\n", argv[0]);
   fprintf (stdout, "      options:\n");
   fprintf (stdout, "         -h   Print usage message\n");
   exit (0);
}


/********************************************************************************

     Description: Read the message received from the DIO module

 ********************************************************************************/ 

static int Read_dio_msg (int sock_desc, char *buf, int buf_size) {

   int ret;
   int err;

   ret = read (sock_desc, buf, buf_size);

   if (ret < 0) {
      err = errno;
      if ((err == ECONNRESET) || (err == ENOTCONN) || (err == ETIMEDOUT))
           Socket_closed = 1;
      return (-1);
   }

   if (ret == buf_size) {
      fprintf (stderr, 
               "Warning: msg size read == max buffer size (max buf size: %d)\n",
               buf_size); 
   }

   if (Verbose)
      fprintf (stdout, "socket read returned: %d\n", ret);

   return (ret);
}


/********************************************************************************

     Description: Read the command line options

 ********************************************************************************/

static int Read_options (int argc, char **argv) {

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */

   while ((c = getopt (argc, argv, "hv")) != EOF) {
      switch (c) {
         case 'v':
            Verbose = 1;
         break;

         case 'h':
            Print_usage (argv);
         
         break;
      }
   }
   return(0);
}


/********************************************************************************

     Description: Reset the connection flag when any signal is caught that
                  indicates the connection has been lost/closed

 ********************************************************************************/

static void Reset_conn (int signal)  {
   fprintf (stderr, "SIGPIPE received\n");
   Socket_closed = 1;
   return;
}


/********************************************************************************

     Description: Get the channel number to test

     NOTE: This routime is not presently used. It was replaced with the IP 
           address argument supplied on the command line on tool startup.

 ********************************************************************************/

static void Select_channel (int *channel_selected) {

   while (1) {
      fprintf (stdout, "\nEnter channel # (1-2): ");
      fscanf (stdin, "%uh", channel_selected);

      if ((*channel_selected < 1) || (*channel_selected > 2))
         fprintf (stdout, "Invalid channel number entered.\n");
      else
         break;
   }
   return;
}


/********************************************************************************

     Description: Send the command to the DIO module

 ********************************************************************************/

static int Send_command (int sd) {

   int ret;
   int err;

      /* send the command to the DIO module */

   if ((ret = write (sd, &Message, Message.hdr.msg_len + sizeof (cmd_hdr_t))) <= 0) {
      err = errno;
      fprintf (stderr, "socket write failed (ret: %d;  errno: %d)\n",
               ret, err);
      if ((err == ECONNRESET) || (err == ENOTCONN) || (err == ETIMEDOUT))
           Socket_closed = 1;
      
      return (-1);
   }
   return (0);
}


/********************************************************************************

     Description: Set the properties for the socket, protocol layers

 ********************************************************************************/

static int Set_properties (int sd) {

   int flag = 1;
   struct linger lig;

      /* enable address re-use */

   if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, 
                  sizeof(flag)) < 0) {
       fprintf (stderr, "setting SO_REUSEADDR failed (errno: %d)\n", errno);
       return (-1);
   }

       /* disable buffering of short data in TCP layer */

    if (setsockopt (sd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, 
                    sizeof (flag)) < 0) {
        fprintf (stderr, "setting TCP_NODELAY failed (errno %d)", errno);
        return (-1);
    }

       /* turn off lingering */

    lig.l_onoff = 0;
    lig.l_linger = 0;

    if (setsockopt (sd, SOL_SOCKET, SO_LINGER, (char *)&lig, 
                    sizeof (struct linger)) < 0) {
        fprintf (stderr, "setting SO_LINGER failed (errno %d)", errno);
        return (-1);
    }

   /* set non-block IO */

    if (fcntl (sd, F_SETFL, O_NDELAY) < 0) {
        fprintf (stderr, "fcntl O_NDELAY failed (errno %d)\n", errno);
        return (-1);
    }

   return (0);
}


/********************************************************************************

     Description: If commanded by the user, reset the DIO module default IP 
                  address before terminating the tool...this assures a NEXRAD IP 
                  address is not assigned to the module before it is delivered 
                  to the field

 ********************************************************************************/

static void Terminate_proc (int signal) {
   
   fprintf (stdout, "Tool is terminating. ");
 
   if (Reset_module_ip_addr) {
      fprintf (stdout, "Executing termination sequence...\n\n");
      fprintf (stdout, "Resetting module IP address to: 10.0.0.1\n");

      if (system ("diom_config -a 10.0.0.1") < 0) {
          fflush (stdout);
          fprintf (stderr, "Error calling \"diom_config\" (errno: %d)\n",
                  errno);
          fprintf (stdout, 
               "\nNOTE: Run tool \"diom_config\" and reset the module IP address\n");
          fprintf (stdout, 
               "        to 10.0.0.1 before returning this module to service\n");
      }
   }

   fprintf (stdout, "\n");

   exit(0);
}
