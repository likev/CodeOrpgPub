/********************************************************************************

   file: diom_config.c

   Description: This file contains all the routines needed to query and configure
                the ADAM Digital Input/Output (DIO) control module. The query 
                command interrogates the network and reports all DIO modules
                detected on the network. The configure command configures the
                network parameters of the DIO module according to the IP
                address or channel number entered by the user.

 *********************************************************************************/


/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/24 16:21:24 $
 * $Id: diom_config.c,v 1.1 2012/01/24 16:21:24 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <fcntl.h>
#include <stdio.h>
#include <curses.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stropts.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifdef SUNOS
#include <sys/sockio.h>
#endif

#ifdef LINUX
#include <string.h>
#endif


#define MAX_BUFFER_SIZE       5000
#define MAX_RETRIES             10

   /* DIO module message header macros */

#define DIO_MODULE_PORT_NUM     5048
#define HEADER                "MADA"
#define MSG_VERSION           0x0100


   /* DIO module queries */

#define DEVICE_INFORMATION_QUERY  0x0000
#define NETWORK_CONFIG_QUERY      0x0010
#define PORT_CONFIG_QUERY         0x0020

   /* DIO module commands */

#define DEVICE_CONFIG_SETUP       0x4000
#define NETWORK_CONFIG_SETUP      0x4010
#define RESET_SYSTEM_SETUP        0x40f0


#define uchar unsigned char


   /* file scope variables */

typedef struct {
   char  header [4];
   int   msg_id;
   short msg_version;
   short device_type;
   int   nodeid_high;   /* high mac address  */
   int   nodeid_low;    /* low mac address   */
   uchar user_id [32];
   ushort command;
   short data_len;
} msg_header_t; 

  
typedef struct {
   short firmware_version_major;
   short firmware_version_minor;
   short hardware_version;
   short port_number;
   uchar mac_address [6];
   short reserved;
   int   device_name_len;
   int   device_name_offset;
   int   device_description_len;
   int   device_description_offset;
   char  device_name [64];
   char  device_description [128];
}   device_query_response_t;

typedef struct {
   msg_header_t msg_header;
   short ethernet_speed_flag;
   short ethernet_mode_flag;
   int   network_config_protocol_flag;
   short ethernet_speed;
   short ethernet_mode;
   uchar mac_address[6];
   short reserved;
   int   network_config_protocol;
   u_char ip_address[4];
   int    subnet_mask;
   int    default_gateway_ip_addr;
}   network_query_response_t;

typedef struct {
   short next_port;
   short port_id;
   short port_type_flag;    /* 0x0001 - RS-232
                               0x0002 - RS-485
                               0x0004 - RS-422 */
   short data_bits_flag;    /* 0x0001 - 4 data bits
                               0x0002 - 5 data bits
                               0x0004 - 6 data bits
                               0x0008 - 7 data bits
                               0x0010 - 8 data bits */
   short parity_flag;       /* 0x0001 - no parity
                               0x0002 - odd parity
                               0x0004 - even parity
                               0x0008 - mark parity
                               0x0010 - space parity */
   short stop_bits_flag;    /* 0x0001 - 1 stop bit
                               0x0002 - 1.5 sopt bits
                               0x0004 - 2 stop bits */
   short flow_control_flag; /* 0x0001 - no flow control
                               0x0002 - Xon/Xoff flow control
                               0x0004 - RTS/CTS flow control
                               0x0008 - DTR/DSR flow control */
   short port_type;         /* 0x0001 - RS-232
                               0x0002 - RS-485
                               0x0004 - RS-422 */
   short data_bits;         /* 0x0001 - 4 data bits
                               0x0002 - 5 data bits
                               0x0004 - 6 data bits
                               0x0008 - 7 data bits
                               0x0010 - 8 data bits */
   short parity;            /* 0x0001 - no parity
                               0x0002 - odd parity
                               0x0004 - even parity
                               0x0008 - mark parity
                               0x0010 - space parity */
   short stop_bits;         /* 0x0001 - 1 stop bit
                               0x0002 - 1.5 sopt bits
                               0x0004 - 2 stop bits */
   short flow_control;      /* 0x0001 - no flow control
                               0x0002 - Xon/Xoff flow control
                               0x0004 - RTS/CTS flow control
                               0x0008 - DTR/DSR flow control */
   short port_state;        /* 0x0001 - port enabled
                               0x0100 - port disabled */
   short reserved_1;
   int max_baud_rate;       /* 0x00038400 */
   int min_baud_rate;       /* 0x00000032 */
   int baud_rate;
   int reserved_2;
   int port_name_len;
   char port_name [512];    /* size defined by port_name_len */
}   port_query_response_t;


typedef struct {
   ushort ethernet_speed;
   ushort ethernet_mode;
   uchar  mac_address[6];
   ushort spare;
   int    network_config_protocol;
   uint   ip_address; 
   uint   subnet_mask;
   uint   default_gateway;
} network_config_cmd_t;

typedef struct {
   uchar mac_address[6];
   uchar ip_address [INET_ADDRSTRLEN];
   uint  subnet_mask;
   uint  default_gateway;
} device_info_t;

static int    Sd;               /* socket descriptor */
static struct sockaddr_in Sin;  /* socket address structure */
static int    Last_cmd_sent;
static int    Query_network;    /* flag to query network for DIO modules */
static char   Module_ip_addr [INET_ADDRSTRLEN]; /* module IP address returned
                                                   in the Network Config query */
static int Verbose;
static int Channel_number;
static char IP_address[INET_ADDRSTRLEN]; /* IP addr supplied as a cmd line arg */
static network_query_response_t Network_query_response;


   /* local functions prototypes */

static int  Execute_cmd_sequence (int cmd);
static void Flush_read_buffer (void);
static void Get_channel_number ();
static int  Interrogate_response (char *buf, int len, struct sockaddr *response);
static int  Open_socket (void);
static void Print_usage (char **argv);
static int  Read_options (int argc, char **argv);
static int  Read_socket (void *buffer, struct sockaddr *response_addr);
static int  Send_device_msg (int msg_type);
static int  Set_socket_properties (int fd);
static int  Update_msg_header (int msg_type, msg_header_t *msg_hdr, int msg_len);
static int  Validate_response (void *msg_buf);
static int  Write_module_msg (char *msg_buffer, int msg_size);


/********************************************************************************

    Description: main

 ********************************************************************************/

int main (int argc, char **argv)
{

   Query_network = 0;
   IP_address[0] = '\0';

   if (Read_options (argc, argv) < 0) {
      fprintf (stderr, "Failure parsing command line options\n");
      fprintf (stderr, "Program aborting...\n");
      exit(1);
   }

      /* get the channel number from the user if an IP address was not
         supplied as a command line argument */

   if (!Query_network)
      if (IP_address[0] == '\0')
         Get_channel_number ();

      /* Open the socket */

   if (Open_socket () < 0) {
      fprintf (stderr, "Error intializing socket\n");
      exit (1);
   }
      /* set the socket properties */

   if (Set_socket_properties (Sd) < 0 ) {
       fprintf (stderr, "Error setting socket properties\n");
       exit (1);
   } 

      /* Broadcast the query for the module's network 
         configuration */

   if (Execute_cmd_sequence (NETWORK_CONFIG_QUERY) != 0) {
      fprintf (stderr, "Failure executing command: 0x%04x\n", Last_cmd_sent);
      fprintf (stderr, "Program is aborting...\n");
      exit (1);
   }

      /* flush the read buffer */
    
   Flush_read_buffer ();

      /* if this was just a device query, then exit */

   if (Query_network)
      exit (0);

      /* setup the module's network configuration */

   if (Execute_cmd_sequence (NETWORK_CONFIG_SETUP) != 0) {
      fprintf (stderr, "Failure executing command: 0x%04x\n", Last_cmd_sent);
      fprintf (stderr, "Program is aborting...\n");
      exit (1);
   }

      /* flush the read buffer */
    
   Flush_read_buffer ();

      /* query the module's network configuration again to 
         confirm successful reconfguration */

   if (Execute_cmd_sequence (NETWORK_CONFIG_QUERY) != 0) {
      fprintf (stderr, "Failure executing command: 0x%04x\n", Last_cmd_sent);
      fprintf (stderr, "Program is aborting...\n");
      exit (1);
   } else
       fprintf (stdout, 
               "\nCycle power to the DIO module to complete the re-configuration sequence\n\n");

      /* flush the read buffer */
    
   Flush_read_buffer ();

      /* reset the module to save the new configuration */
/***** the reset command does not work 
   if (Execute_cmd_sequence (RESET_SYSTEM_SETUP) != 0) {
      fprintf (stderr, "Failure executing command: 0x%04x\n", Last_cmd_sent);
      fprintf (stderr, "Program is aborting...\n");
      exit (1);
   }
*/
   exit (0);
}


/********************************************************************************

    Description: This function handles the command to send to the DIO module

          Input: cmd - Command to send

         Output: Last_cmd_sent - global var that identifies the last command 
                                 sent to the DIO module

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Execute_cmd_sequence (int cmd) {
   struct sockaddr read_response;
   int    read_attempts = 0;
   int    dio_datagrams_read = 0;
   int    ret = 0;
   char   read_buffer [MAX_BUFFER_SIZE];

      /* send the message to the module */

   Send_device_msg (cmd);
   Last_cmd_sent = cmd;

      /* try MAX_RETRIES times to send a msg and receive a valid
         response before giving up */

   for (read_attempts = 1; read_attempts <= MAX_RETRIES; read_attempts++) {

      sleep (1);  /* wait for a response */

      if ((ret = Read_socket (read_buffer, &read_response)) > 0) {
         if (Interrogate_response (read_buffer, ret, &read_response) == 0)
            ++dio_datagrams_read;
      } else {
         if (dio_datagrams_read == 0)
            Send_device_msg (cmd);     /* the last query failed...try again */
         else
             break;
      }
   }

      /* if this was only a query, then report the number of modules detected
         and return */

   if (Query_network) {
      fprintf (stdout, "%d DIO module(s) detected on the network\n\n", 
               dio_datagrams_read);
      return (0);
   }

   if ((dio_datagrams_read == 0) && (ret < 0)) {
       fprintf (stderr, "\nError: No DIO modules were detected on the network.\n");
       fprintf (stderr, "       Verify the module's network and power");
       fprintf (stderr, " connections then re-run this utility\n\n");
      return (-1);
   }

   if (dio_datagrams_read > 1) {
      fprintf (stderr, "\nWarning: \"%d\" DIO modules", dio_datagrams_read);
      fprintf (stderr, " were detected on the network.\n");
      fprintf (stderr, "      Only one DIO module can be on the network");
      fprintf (stderr, " at a time during configuration.\n");
      fprintf (stderr, "      Disconnect the other channel's DIO module");
      fprintf (stderr, " then re-run this utility\n");
      fprintf (stderr, "      to configure this channel's module\n\n");
      return (-1);
   }

   return (0);
}


/********************************************************************************

    Description: This function flushes the read buffer. Since this is a UDP 
                 socket and the messages are being broadcast, we have no 
                 assurances that the DIO module(s) sent only one response per
                 query/command (we may have sent multiple queries/commands if
                 a response was not received in a timely fashion), or that all 
                 responses were sent by a DIO module.
 
          Input:

         Output:

         Return:

 ********************************************************************************/

static void Flush_read_buffer () {
   int  ret;
   int  loop_cnt = 0;
   char buffer[MAX_BUFFER_SIZE];
   struct sockaddr read_response;
   socklen_t response_len = sizeof (read_response); 
 
   while ((ret = recvfrom (Sd, buffer, MAX_BUFFER_SIZE, 0, 
                           &read_response, &response_len)) > 0) {
       fprintf (stdout, "\nsocket read flush returned %d bytes\n", ret);
       sleep (1);
       ++loop_cnt;
       
       if (loop_cnt > 10) {  /* arbitrary limit to prevent infinite looping */
          fprintf (stderr, "Error flushing socket read buffer\n");
          break;
       }
   }

   return;
}


/********************************************************************************

    Description: This function gets the channel number from the user

          Input:

         Output:

         Return:

 ********************************************************************************/

static void Get_channel_number () {

   while (1) {
      int  buf_size = 128;
      char buf [buf_size];

      fprintf (stdout, "Enter channel # to configure (1 or 2): "); 
      fgets (buf, buf_size, stdin);
      Channel_number = atoi (buf);

      if (((Channel_number != 1) && (Channel_number != 2)) ||
          ((strlen(buf) - 1) > 1))  /* disregard the carriage return */
         fprintf (stdout, "Channel number is invalid\n");
      else
         return;
   }
}


/********************************************************************************

    Description: This function interrogates the datagram read from the socket. 
                 The datagram is first validated to ensure it has been sent by 
                 a DIO module, then it is evaluated to determine what type
                 message the DIO module sent.

          Input: buf           - the socket read buffer
                 msg_len       - the length of the message read
                 response_addr - the response socket structure

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Interrogate_response (char *buf, int msg_len, 
                                 struct sockaddr *response_addr)
{
   const u_char *addr_ptr;
   unsigned int ip_addr_int;
   int response_val;
   msg_header_t *msg_hdr;

   msg_hdr = (msg_header_t *) buf;

      /* ensure the response was sent by a DIO module */

   if ((Validate_response (buf)) < 0) {
      fprintf (stderr, "Device response validation failed\n");
      return (-1);
   }
   else
      if (Verbose)
         fprintf (stdout, "Device response validated for command 0x%04x\n\n", 
                  Last_cmd_sent);

   switch (Last_cmd_sent) {

      case NETWORK_CONFIG_QUERY:

        memcpy (&Network_query_response, buf, sizeof (Network_query_response));

        memcpy (&ip_addr_int, Network_query_response.ip_address, 4);
        ip_addr_int = ntohl (ip_addr_int);
        Module_ip_addr[0] = '\0';

        addr_ptr = (const u_char *) Network_query_response.ip_address;

        snprintf (Module_ip_addr, sizeof (Module_ip_addr), "%d.%d.%d.%d", 
                  addr_ptr[0], addr_ptr[1], addr_ptr[2], addr_ptr[3]);

        fprintf (stdout, "   Device IP addr: %-15s", Module_ip_addr);

        fprintf (stdout, "  (MAC addr: %02x:%02x:%02x:%02x:%02x:%02x)", 
               Network_query_response.mac_address[0],
               Network_query_response.mac_address[1],
               Network_query_response.mac_address[2],
               Network_query_response.mac_address[3],
               Network_query_response.mac_address[4],
               Network_query_response.mac_address[5]);

        if (Verbose)
           fprintf (stdout, "  (IP int addr:  0x%x)", ip_addr_int);

        fprintf (stdout, "\n");


      break;

      case NETWORK_CONFIG_SETUP:
      case RESET_SYSTEM_SETUP:

         if (msg_len <= sizeof (msg_header_t)) {
            fprintf (stderr, 
              "Setup cmd response msg size invalid (cmd: %d;  expected: %d; actual: %d)\n",
              Last_cmd_sent, sizeof (msg_header_t) + 4, msg_len);
            return (-1);
         }

         memcpy ((char *)&response_val, (char *)(buf + sizeof (msg_header_t)), 4);

         if (response_val != 0) {
            fprintf (stderr, 
                  "Error returned from device setup command (cmd: %x;  ret: %d)\n",
                     msg_hdr->command, response_val);
            return (-1);
         }
         else
            if (Verbose)
               fprintf (stdout, 
                       "Command 0x%04x was successful\n", Last_cmd_sent);
      break;

      default:
         fprintf (stderr, "Unknown device response received\n");
      break; 
   }

   return (0);
}


/********************************************************************************

    Description: This function opens the socket as a UDP socket, sets the
                 parameters to the network broadcast address and assigns the 
                 port number to the applicable DIO module UDP port.

          Input:

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Open_socket (void) {

      /* open the socket */

   if ((Sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { 
      fprintf (stderr, "Error opening socket (sd: %d; errno: %d)\n", Sd, errno);
      return (-1);
   }

   if (Verbose)
      fprintf (stdout, "socket opened (fd: %d)\n", Sd);

   Sin.sin_addr.s_addr = htonl(INADDR_BROADCAST);
   Sin.sin_family = AF_INET;
   Sin.sin_port = htons(DIO_MODULE_PORT_NUM);

   return (0);
}


/**************************************************************************

    Description: This function reads the command line arguments.

         Inputs: argc - number of command arguments
                 argv - the list of command arguments

         Output: Query_network - global flag 

         Return: 0 on success or -1 on failure

**************************************************************************/

static int Read_options (int argc, char **argv)
{
   extern char *optarg;    /* used by getopt */
   extern int  optind;
   int         c;          /* used by getopt */


   while ((c = getopt (argc, argv, "a:hvq")) != EOF) {
      switch (c) {
         case 'a':
            strncpy (IP_address, optarg, INET_ADDRSTRLEN);
            IP_address[INET_ADDRSTRLEN - 1] = '\0';
         break;

         case 'v':
            Verbose = 1;
         break;

         case 'q':
            Query_network = 1;
         break;

         case 'h':
            Print_usage (argv);
         default:
            return (-1);
         break;
      }
   }

   return (0);
}


/********************************************************************************

    Description: This function prints the usage message to the terminal
                 and exits.

          Input:

         Output:

         Return:

 ********************************************************************************/

static void Print_usage (char **argv) {

   printf ("\nUsage: %s [options]\n", argv[0]);
   printf ("     options:\n");
   printf ("        -a IP address to assign to the DIO module\n");
   printf ("        -h print usage message\n");
   printf ("        -q query network for DIO modules (print results to stdout)\n");
   printf ("        -v verbose mode\n");
  
   exit (0);
}


/********************************************************************************

    Description: This function reads the message from the socket.

          Input: buffer        - the buffer to read the message into
                 response_addr - the socket response structure

         Output:

         Return:  0 on success; -1 on error

 ********************************************************************************/

static int Read_socket (void *buffer, struct sockaddr *response_addr)
{
   int       ret;
   socklen_t response_len = sizeof (response_addr); 

      /* read data from the socket */

   if ((ret = recvfrom (Sd, buffer, MAX_BUFFER_SIZE, 0, 
                        response_addr, &response_len)) < 0) {
       return (-1);
   }

   if (Verbose)
      printf ("\nBytes read from socket: %d\n\n", ret);

   return (ret);
}


/********************************************************************************

    Description: This function writes the message to the socket.

          Input: msg_type - the type of msg to send (ie. query, cmd, etc)

         Output:

         Return: 0 on success, or -1 on error. 

 ********************************************************************************/

static int Send_device_msg (int msg_type) {

   char *msg_buffer = NULL;
   int   buffer_size = 0;
   short data_len = 0;
   int   ret;
   char dio_module[2][12] = {{"diomodule\0"},
                             {"diomodule2\0"}};
   char default_gateway_addr[] = "0.0.0.0";
   char subnet_mask[] = "255.255.255.0";
   msg_header_t *msg_hdr = NULL;
   network_config_cmd_t *network_config_ptr;
   struct hostent *hp;      /* pointer to the host file entry structure */
   struct in_addr address;

   if (msg_type == NETWORK_CONFIG_SETUP)
       data_len = 28;  /* struc size for network config setup cmd */
   else
       data_len = 0;

   buffer_size = sizeof (msg_header_t) + data_len;
   msg_buffer = calloc (buffer_size, 1);
   msg_hdr = (msg_header_t *)msg_buffer;

   if (msg_buffer == NULL) {
      fprintf (stderr, 
         "Error calloc'ing %d bytes of memory\n", buffer_size);
      return (-1);
   }

   switch (msg_type) {
      case NETWORK_CONFIG_QUERY:

         fprintf (stdout, "\nQuery network for DIO module(s):\n");

         Update_msg_header (NETWORK_CONFIG_QUERY, msg_hdr, data_len);
         
      break;

      case NETWORK_CONFIG_SETUP:

         fprintf (stdout, "\nRe-configure the DIO module's network parameters\n");

         network_config_ptr = (network_config_cmd_t *)
                              (msg_buffer + sizeof (msg_header_t));

            /* upate the message header */

         Update_msg_header (NETWORK_CONFIG_SETUP, msg_hdr, data_len);

            /* assign the field values as specified in the vendor
               documentation */

         network_config_ptr->ethernet_speed = 0x0000;
         network_config_ptr->ethernet_mode = 0x0000;
         network_config_ptr->spare = 0x0000;
         network_config_ptr->network_config_protocol = 0x00000000;

            /* assign the MAC address that was passed back from the DIO 
               module network configuration query */

         memcpy (network_config_ptr->mac_address, 
                 &Network_query_response.mac_address, 6);

            /* assign the module's IP address.

               use the command line option IP address if one was entered;
               otherwise, use the channel number to determine the IP
               address */

         if (IP_address[0] != '\0') {
            if ((ret = inet_pton (AF_INET, IP_address, &address)) != 1) {
               if (ret == 0)
                  fprintf (stdout,
                      "\nInvalid dotted-decimal IP address entered: %s\n",
                      IP_address);
            else
               fprintf (stdout, "IP address unknown: %s  (errno: %d)\n",
                        IP_address, errno);

               exit (1);
            }

            memcpy ((char *)&network_config_ptr->ip_address, 
                    (char *)&address, sizeof (struct in_addr));  
         } else {

               /* assign the DIO module's IP address via DNS lookup
                  according to the channel number selected */

            if ((hp = gethostbyname(dio_module[Channel_number - 1])) == NULL) {
               fprintf (stderr, "Error getting hostname \"%s\" (h_errno: %d)\n", 
                        dio_module[Channel_number - 1], h_errno);
               fprintf (stderr, "Program aborting...\n");
               exit(1);
            }

            memcpy ((char *)&network_config_ptr->ip_address, 
                    (char *)*hp->h_addr_list, 4);  
         }

         memcpy (&address.s_addr, &network_config_ptr->ip_address, 4);

         fprintf (stdout, "   New IP addr:         %s\n", inet_ntoa(address)); 

            /* assign the module's subnet mask */

         if (inet_pton (AF_INET, subnet_mask, &address) != 1) {
            fprintf (stderr, 
                     "Error converting subnet mask IP addr: %s (errno: %d)\n", 
                     subnet_mask, errno);
            exit (1);
         }

         memcpy ((char *)&network_config_ptr->subnet_mask, 
                 (char *) &address, 4); 

         fprintf (stdout, "   New subnet mask:     %s\n", inet_ntoa (address));

            /* assign the module's default gateway address */

         if (inet_pton (AF_INET, default_gateway_addr, &address) != 1) { 
            fprintf (stderr, 
                     "Error converting default gateway IP addr: %s (errno: %d)\n", 
                     default_gateway_addr, errno);
            exit (1);
         }

         memcpy ((char *)&network_config_ptr->default_gateway, 
                 (char *) &address, 4); 

         fprintf (stdout, "   New default_gateway: %s\n\n",  inet_ntoa (address));

      break;

      case RESET_SYSTEM_SETUP:  /* this command always failed during testing */

         fprintf (stdout, "\nReset DIO Module Settings\n");

         if (Verbose)
            fprintf (stdout, "%d bytes calloc'ed\n", buffer_size);

         Update_msg_header (RESET_SYSTEM_SETUP, msg_hdr, data_len);
      break;

      default:
         fprintf (stdout, 
                  "Invalid Device command received (msg_type: %d)\n", 
                  msg_type);
         free (msg_buffer);
         return (0);
      break;
   }

      /* write the message to the socket */

   if ((Write_module_msg (msg_buffer, buffer_size)) != 0) {
       fprintf (stderr, "Error writing command to DIO device\n");
       free (msg_buffer);
       return (-1);
   }

   free (msg_buffer);

   return (0);
}


/********************************************************************************

    Description: This function sets the UDP socket properties. Broadcasting
                 is enabled because we don't know what the IP address of the
                 DIO module is initially set to. All queries and commands are 
                 broadcast for convenience. Non-blocking mode is set so we don't 
                 block on I/O operations.

          Input: fd - the socket descriptor to set the properties for

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

int Set_socket_properties (int fd) {
   int flag = 1;
   int val;

   errno = 0;

      /* set the broadcast socket option...used for resource discovery */

   if (setsockopt (fd, SOL_SOCKET, SO_BROADCAST, (char *)&flag, 
       sizeof (flag)) < 0) {
          fprintf (stderr, 
              "Error setting \"Broadcast\" socket option (errno: %d)\n", errno);
          return (-1);
   }

      /* set non-block I/O */

   if((val = fcntl(fd, F_GETFL, 0)) < 0) {
       fprintf (stderr, "Error reading socket status flags (errno: %d)\n",
                errno);
       return (-1);
   }

#ifdef HPUX
    val |= O_NONBLOCK;
#else
    val |= O_NDELAY;
#endif

   if((fcntl(fd, F_SETFL, val)) < 0) {
       fprintf (stderr, "Error setting non-blocking option (errno: %d)\n",
                errno);
       return (-1);
   }

   return (0);
}


/********************************************************************************

    Description: This function generates the message header according to the
                 type message being sent

          Input: msg_type - type of message being written to the socket
                 *msg_hdr - pointer to the message header
                 msg_len  - length of the message being sent

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Update_msg_header (int msg_type, msg_header_t *msg_hdr, int msg_len) {

static int msg_seq_num = 0;

   msg_seq_num++;

   switch (msg_type) {

         /* set the message header for a network settings query */

      case NETWORK_CONFIG_QUERY:
         strncpy (msg_hdr->header, HEADER, 4); 
         msg_hdr->msg_id = htonl(msg_seq_num);
         msg_hdr->msg_version  = htons(MSG_VERSION);
         msg_hdr->device_type = htons (0x5000);
         msg_hdr->nodeid_high = htonl(0x0000);
         msg_hdr->nodeid_low = htonl(0x0000);
         msg_hdr->command  = htons(NETWORK_CONFIG_QUERY);
         msg_hdr->data_len = htons(msg_len);
      break;

         /* set the message header for the command to reconfigure the
            module's network parameters */

      case NETWORK_CONFIG_SETUP:
         strncpy (msg_hdr->header, HEADER, 4);
         msg_hdr->msg_id = htonl(msg_seq_num);
         msg_hdr->msg_version  = htons(MSG_VERSION);
         msg_hdr->device_type = htons(0x5000);

         memcpy (&msg_hdr->nodeid_high, 
                 &Network_query_response.msg_header.nodeid_high, 8);
         msg_hdr->command  = htons(NETWORK_CONFIG_SETUP);
         msg_hdr->data_len = htons(msg_len);

      break;

      case RESET_SYSTEM_SETUP:
         strncpy (msg_hdr->header, HEADER, 4); 
         msg_hdr->msg_id = htonl(msg_seq_num);
         msg_hdr->msg_version  = htons(MSG_VERSION);
         msg_hdr->device_type = 0x5000;
         memcpy (&msg_hdr->nodeid_high, 
                 &Network_query_response.msg_header.nodeid_high, 8);
         msg_hdr->command  = RESET_SYSTEM_SETUP;
         msg_hdr->data_len = htons(msg_len);
      break;
   }

   return (0);
}


/********************************************************************************

    Description: This function verifies the message received was sent by a 
                 DIO module

          Input: msg_buf - pointer to the buffer containing the message to 
                           validate

         Output:

         Return: 0 if the message was sent by a DIO module; otherwise,
                 -1 is returned.

 ********************************************************************************/

static int Validate_response (void *msg_buf) {

   msg_header_t *hdr;

   hdr = (msg_header_t *)msg_buf;

      /* verify the response came from a DIO module */

   if (strncmp ((const char *)hdr, "MADA", 4) == 0)
      return (0);
   else
      return (-1);
}


/********************************************************************************

    Description: This function writes the message to the socket

          Input: msg_buffer - pointer to the message buffer to write
                 msg_size   - length of the message to write

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

int Write_module_msg (char *msg_buffer, int msg_size) {

   int bytes_written = 0;
   int loop_counter = 0;
   int ret;

      /* if the loop counter exceeds 5 passes, then something is wrong */

   while ((bytes_written < msg_size) && (loop_counter < 5)) {

      ++loop_counter;

      errno = 0;

         /* write the message out the socket */

      ret = sendto (Sd, msg_buffer + bytes_written, msg_size - bytes_written, 0, 
                    (const struct sockaddr *) &Sin, sizeof (Sin));

      if (ret < 0) {
         fprintf (stderr, "Socket write error (errno: %d)\n", errno);

         if (errno == EWOULDBLOCK || errno == EAGAIN) {
            sleep (1);
            continue;
         }

         if (errno != EINTR) {
            return (-1);
         }
         continue;
      }

      if (ret > 0)
          bytes_written += ret;
   }

   if (Verbose)
      fprintf (stdout, "\n%d bytes written to socket (msg_size: %d)\n", 
               bytes_written, msg_size);

   if (bytes_written == msg_size)
      return (0);
   else
      return (-1);

}
