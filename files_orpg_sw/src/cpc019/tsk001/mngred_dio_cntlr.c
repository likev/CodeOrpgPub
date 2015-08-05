/********************************************************************************
    
    File: mngred_dio_cntlr.c

     Description: This file contains the routines used to control the the 
                  Advantech ADAM 6051 Digital Input/Output (DIO) Ethernet 
                  Module.  The DIO module controls and monitors the state 
                  of the Comms control relay.


 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/12 16:21:52 $
 * $Id: mngred_dio_cntlr.c,v 1.6 2013/02/12 16:21:52 steves Exp $
 * $Revision: 1.6 $
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

#include <mngred_globals.h>
#include <orpgmisc.h>


#define DIO_MODBUS_PORT       502
#define DIO_NO_CMD_PENDING     -1
#define BUFFER_SIZE          1000
#define MAX_IO_RETRIES          4

#define IO_READ                 0
#define IO_WRITE                1

   /* ADAM 6051 Digital I/O module commands */

#define DIO_READ_DO_STATE    0x01
#define DIO_READ_DI          0X02
#define DIO_SET_DO           0x05
#define DIO_RESET_DOS        0x0f
#define DIO_CYCLE_DO         0X7F    /* internally defined command */


#define DIO_DO_START_ADDR      16    /* D/O start address space */
#define DIO_DI_START_ADDR       0    /* D/I start address space */
#define DIO_COMMS_CONTROLLER_DO 0    /* D/O used to perform channel switching */

   /* size of command messages in bytes */

#define DIO_READ_DO_CMD_LEN     6
#define DIO_SET_DO_CMD_LEN      6
#define DIO_RESET_DOS_CMD_LEN   9
#define DIO_READ_DI_CMD_LEN     6


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
static int Socket_closed = 1;
static int Sock_d;


   /* local function prototypes */

static int  Check_cmd_ack (int cmd_pending, void *state);
static int  Connect_socket (void);
static int  Construct_cmd (uchar cmd);
static int  Flush_read_buffer (void);
static void Initialize_fixed_msg_fields ();
static int  Interrogate_response (int last_cmd_sent, uchar *di_state);
static int  Open_socket (void);
static int  Poll_socket (int io_type);
static int  Read_dio_msg (char *buf, int buf_size);
static int  Send_command (void);
static int  Set_properties (void);


/********************************************************************************

   Description: Acquire the comms relay

        Return: 0 on success; -1 on failure

 ********************************************************************************/

int DIO_acquire_comms_relay (void) {

      /* return if this is a non-operational environment */

   if (!ORPGMISC_is_operational()) return(0);

      /* open the socket if it is closed */

   if (Socket_closed) {
      if (Open_socket () < 0) {
         LE_send_msg (MNGRED_DEBUG_VL, "Error opening socket");
         return (-1);
      }
   }

      /* ensure the D/O is reset before commanding it to set again */

   DIO_reset_dos ();

   Construct_cmd (DIO_SET_DO);

   if (Send_command () < 0) {
      LE_send_msg (MNGRED_DEBUG_VL, "Error sending DIO command %d", DIO_SET_DO);
      return (-1);
   } 
   
   LE_send_msg (MNGRED_DIOM_VL, "DIO command %d sent", DIO_SET_DO);

      /* check for command ack from the DIO module */

   if (Check_cmd_ack (DIO_SET_DO, NULL) < 0) {
      LE_send_msg (MNGRED_DIOM_VL, "DIO command ack failed for command %d",
                   DIO_SET_DO);
      return (-1);
   }

   LE_send_msg (MNGRED_DIOM_VL, "DIO command %d acknowledged", DIO_SET_DO);
      
   return (0);
}


/********************************************************************************

     Description: Initialize the digital I/O module

 ********************************************************************************/

int DIO_init_dio_module (void) {

   int ret = 0;

      /* return if this is a non-operational environment */

   if (!ORPGMISC_is_operational()) return(0);

      /* initialize the fixed message fields */

   Initialize_fixed_msg_fields ();

   if (Open_socket () < 0) {
      LE_send_msg (MNGRED_DEBUG_VL, "Error opening socket");
      ret = -1;
   }

   return (ret);
}


/********************************************************************************

     Description: Read the state of the control relay D/I

 ********************************************************************************/

Comms_relay_state_t DIO_read_comms_relay_state (void) {

   uchar di_state;
   static int di_alarm_set = 0;

      /* If this is a non-operational environment, return state to match 
         state of the RDA */

   if (!ORPGMISC_is_operational()) {
      if (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING)
         return (ORPGRED_COMMS_RELAY_ASSIGNED);
      else 
         return (ORPGRED_COMMS_RELAY_UNASSIGNED);
   }

      /* open the socket if it is closed */

   if (Socket_closed) {
      if (Open_socket () < 0) {
         LE_send_msg (MNGRED_DEBUG_VL, "Error opening socket");
         return (-1);
      }
   }

   if (Flush_read_buffer () < 0)
      return (ORPGRED_COMMS_RELAY_UNKNOWN);

   Construct_cmd (DIO_READ_DI);

   if (Send_command () < 0) {
      LE_send_msg (MNGRED_DIOM_VL, "Error sending command %d\n", DIO_READ_DI);
      return (ORPGRED_COMMS_RELAY_UNKNOWN);
   } 

   LE_send_msg (MNGRED_DIOM_VL, "Command %d sent\n", DIO_READ_DI);

      /* check for command ack from the DIO module */

   if (Check_cmd_ack (DIO_READ_DI, &di_state) < 0) {
/*   if ((di_state = Interrogate_response (DIO_READ_DI)) < 0) { */
      LE_send_msg (MNGRED_DIOM_VL, "Command ack failed for command %d\n",
                   DIO_READ_DI);
      if (di_alarm_set == 0) {
         LE_send_msg (GL_STATUS | GL_ERROR,
            "%s Failure reading NB/WB comms relay (relay state is \"Unknown\")",
            MNGRED_WARN_ACTIVE);
         di_alarm_set = 1;
      }
      return (ORPGRED_COMMS_RELAY_UNKNOWN);
   }   

   LE_send_msg (MNGRED_DIOM_VL, "D/I State: %d", di_state);

   if (di_alarm_set) {
      LE_send_msg (GL_STATUS | GL_ERROR,
         "%s Failure reading NB/WB comms relay (relay state is \"Unknown\")",
         MNGRED_WARN_CLEAR);
         di_alarm_set = 0;
   }

   if (di_state == 0)
      return (ORPGRED_COMMS_RELAY_UNASSIGNED);
   else  /* D/I is set */
      return (ORPGRED_COMMS_RELAY_ASSIGNED);
}


/********************************************************************************

     Description: Reset the D/Os...single D/O reset did not seem to work
                  properly but resetting all the D/Os does appear to work, 
                  so reset all the D/Os

 ********************************************************************************/

int DIO_reset_dos (void) {

      /* return if this is a non-operational environment */

   if (!ORPGMISC_is_operational()) return(0);

   Construct_cmd (DIO_RESET_DOS); 

   if (Send_command () < 0) {
      LE_send_msg (LE_CRITICAL_BIT, "Error sending DIO cmd %d\n", DIO_RESET_DOS);
      return (-1);
   }

   LE_send_msg (MNGRED_DIOM_VL, "Command %d sent\n", DIO_RESET_DOS);

      /* check for command ack from the DIO module */

   if (Check_cmd_ack (DIO_RESET_DOS, NULL) < 0) {
      LE_send_msg (LE_CRITICAL_BIT, "DIO cmd ack failed for cmd %d\n",
                   DIO_RESET_DOS);
      return (-1);
   }   

   LE_send_msg (MNGRED_DIOM_VL, "Command %d acknowledged\n", DIO_RESET_DOS);

   return (0);
}


/********************************************************************************

     Description: Verify the command acknowledgement received from the DIO module

 ********************************************************************************/

static int Check_cmd_ack (int cmd_pending, void *state) {

   int ret;
   int retry_cnt = 0;

   while (retry_cnt < MAX_IO_RETRIES) {
      if ((ret = Poll_socket (IO_READ)) < 0) {
         LE_send_msg (MNGRED_DIOM_VL, "socket poll failed for command %d", cmd_pending);
         return (-1);
      }
      ++retry_cnt;

      if (ret == 1)
         break;
      else   
         msleep (500);
   }   

   if (retry_cnt >= MAX_IO_RETRIES) 
      ret = -1;
   else {   /* there's something to read */
      if ((Interrogate_response (cmd_pending, state)) < 0) {
         LE_send_msg (MNGRED_DIOM_VL, "Interrogation of message response failed\n");
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

static int Connect_socket (void) {
   int ret = -1;
   int err = 0;
   int retry_counter = 0;

   while ((retry_counter < MAX_IO_RETRIES) && (ret < 0)) {

      ++retry_counter;

         /* connect to the server */

      ret = connect(Sock_d, (struct sockaddr *)&Sinaddr, sizeof(Sinaddr));

      if (ret < 0) {

         err = errno;

         if (err == EISCONN) 
            return (0);

         msleep (500);
      }
   }

   if (ret < 0) {
      LE_send_msg (MNGRED_DEBUG_VL, "socket connect error (errno: %d)\n", err);
      return (-1);
   } 

   return (0);
}


/********************************************************************************

     Description: Construct the command in MODBUS format.

 ********************************************************************************/

static int Construct_cmd (uchar cmd) {

   static ushort transaction_id = 0;

      /* increment xaction id */

   Message.hdr.xaction_id = htons (++transaction_id);

      /* specify which D/O to control...all commands apply to D/Os
         but one, so we'll just set parameter 1 to the D/O start address
         and reset this parameter for the D/I command when necessary */

   Message.command.parameter_1 = htons (DIO_COMMS_CONTROLLER_DO + 
                                        DIO_DO_START_ADDR);

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

      case DIO_RESET_DOS:
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

     Description: Flush the read buffer of all received messages

 ********************************************************************************/

 static int Flush_read_buffer (void) {

   char buffer[BUFFER_SIZE];

   while (Poll_socket (IO_READ) == 1) {
      LE_send_msg (MNGRED_DIOM_VL, "flushing read buffer");    
      if (Read_dio_msg (buffer, BUFFER_SIZE) < 0)
         return (-1);
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
            
           Input:  the command that initiated the response

          Return: 0 on success; -1 on failure

 ********************************************************************************/
 
static int Interrogate_response (int last_cmd_sent, uchar *di_state) {
 
   modbus_msg_t *msg_read;
   char  buffer[BUFFER_SIZE];
   char  *buffer_ptr;
   short *short_ptr;
   short response;
   int   bytes_read;
   uchar response_byte_cnt;
   int   i;

      /* read the message received from the DIO module */

   if ((bytes_read = Read_dio_msg (buffer, BUFFER_SIZE)) == -1) {
        LE_send_msg (LE_CRITICAL_BIT, "socket read failed (errno: %d)\n",
                     errno);
        return (-1);
   }

   LE_send_msg (MNGRED_DIOM_VL,
      "DIO Message response received for command %d (msg len: %d)\n", 
      last_cmd_sent, bytes_read);

      /* set the pointer to the function code (ie command)
         in the read buffer */

   buffer_ptr =  buffer + 1 + sizeof (cmd_hdr_t);

      /* if the MSB is set in the function code, then
         the command failed */

   if ((char) *buffer_ptr < 0) {
/*      char tmp_buf[bytes_read + 1];
      int i;
      
      for (i = 0; i < bytes_read; i++) 
         sprintf (&tmp_buf[i], "%d", buffer_ptr + i);

      tmp_buf [bytes_read] = '\0';
         
      LE_send_msg (MNGRED_DIOM_VL, "Command %d failed (returned cmd string: %s)",  
               last_cmd_sent, tmp_buf);
      return (-1);
*/
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
        
               LE_send_msg (MNGRED_DIOM_VL, 
                            "Command %d validated....cmd succeeded\n", 
                            last_cmd_sent);
            return (0);

         } else {
             LE_send_msg (MNGRED_DEBUG_VL, 
                          "Message command/response compare failed\n");
             LE_send_msg (MNGRED_DEBUG_VL, 
                      "   Module command header: %2x %2x %2x\n",
                      Message.hdr.xaction_id,
                      Message.hdr.protocol_id,
                      Message.hdr.msg_len);
             LE_send_msg (MNGRED_DEBUG_VL, 
                      "   Module command body: %2x %2x %2x %2x\n",
                      Message.command.unit_id,
                      Message.command.function_code,
                      Message.command.parameter_1,
                      Message.command.parameter_2);
             LE_send_msg (MNGRED_DEBUG_VL, 
                      "   Module response: %2x %2x %2x %2x\n",
                      msg_read->command.unit_id,
                      msg_read->command.function_code,
                      msg_read->command.parameter_1,
                      msg_read->command.parameter_2); 
             return (-1);
         }
      break;

      case DIO_READ_DO_STATE: /* this command is buggy and is not used */

         if (Verbose)
            LE_send_msg (MNGRED_DIOM_VL, "Function code returned:    %2x\n", 
                     (uchar) (*buffer_ptr));

         response_byte_cnt = (uchar) *(buffer_ptr + 1);

         if (Verbose)
            LE_send_msg (MNGRED_DIOM_VL, "Byte count:                 %d\n", 
                     response_byte_cnt);

         for (i = 0; i < response_byte_cnt; i++) {
            LE_send_msg (MNGRED_DEBUG_VL, "Coils state for byte # %d: x%02x\n", 
                     i + 1, (uchar) *(buffer_ptr + i));
        
         }
      break;

      case DIO_RESET_DOS:

         LE_send_msg (MNGRED_DIOM_VL, 
                 "Function code returned: %02x", (uchar) (*buffer_ptr));

         ++buffer_ptr;
         short_ptr = (short *) buffer_ptr; 

         LE_send_msg (MNGRED_DIOM_VL, 
                  "D/O start address: %2x", ntohs((ushort) (*short_ptr)));

         buffer_ptr = buffer_ptr + 2;
         short_ptr = (short *) buffer_ptr; 

         response = (ushort) *short_ptr;
   
         LE_send_msg (MNGRED_DIOM_VL, "Requested number of coils to force: %x", 
                      ntohs((ushort) (*short_ptr)));

         if (response > 0) {
           LE_send_msg (MNGRED_OP_VL, " Reset D/Os command succeeded");
           return (0);
         } else {
           LE_send_msg (LE_CRITICAL_BIT, "D/Os failed to reset");
           return (-1);
         } 

      break;

      case DIO_READ_DI:

         LE_send_msg (MNGRED_DIOM_VL, "Function code returned: %02x", 
                     (uchar) (*buffer_ptr));

         buffer_ptr = buffer_ptr + 1;
  
         response_byte_cnt = (uchar) *(buffer_ptr);

         LE_send_msg (MNGRED_DIOM_VL, "Byte count: %d", response_byte_cnt);

         buffer_ptr = buffer_ptr + 1;

            /* mask all bits but bit 1, then complement the 
               bit so that "1" = D/I is set and "0" = D/I 
               not set */

         *di_state = ~(0xfe | ((uchar)*(buffer_ptr)));

         LE_send_msg (MNGRED_DIOM_VL, "D/I state for bit # 1: %01x", 
                      *di_state);

         return (0);

      break;

     default:
        LE_send_msg (MNGRED_OP_VL,
                 "Invalid command_pending command received for interrogation");
        return (-1);
     break;
   }
   return (0);
}


/********************************************************************************

     Description: Open the socket

 ********************************************************************************/ 

static int Open_socket (void) {

   char *module [] = {"diomodule",
                      "diomodule2"};
   struct hostent *hp;
   int channel_num;

   Socket_closed = 1;

   channel_num = CHAnnel_status.rpg_channel_number;

   if ((channel_num < 1) || (channel_num > 2)) {
      LE_send_msg (GL_ERROR,"Invalid channel number received (channel num: %d)\n",
               channel_num);
      return (-1);
   }

   hp = gethostbyname(module[channel_num - 1]);

   if(!hp) {
      LE_send_msg (GL_ERROR, "Could not resolve hostname \"%s\" in hosts table\n",
                   module[channel_num - 1]);
      return (-1);
   }

   bzero (&Sinaddr, sizeof(Sinaddr));
   bcopy(hp->h_addr, (char *)&Sinaddr.sin_addr, hp->h_length);

   Sinaddr.sin_family = AF_INET;
   Sinaddr.sin_port = htons((ushort) DIO_MODBUS_PORT);

   if ((Sock_d = socket (Sinaddr.sin_family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
       LE_send_msg (LE_CRITICAL_BIT, 
                    "call to \"socket(...)\" failed, errno = %d\n",
                    errno);
       return(-1);
   }

      /* set the connection properties */

   if ((Set_properties ()) < 0) {
       LE_send_msg (LE_CRITICAL_BIT, "Error setting socket/TCP properties\n");
       close (Sock_d);
       return (-1);
   }

      /* connect to the server */

   if (Connect_socket () < 0) {
      LE_send_msg (MNGRED_DIOM_VL, 
                   "Error connecting to server (errno: %d)", errno);
      close (Sock_d);
      return (-1);
   }

   LE_send_msg (MNGRED_OP_VL, "Socket opened");

   Socket_closed = 0;

   return (0);
}


/********************************************************************************

     Description: Poll the socket and wait for a response from the last
                  command sent

          Return: 0 on success; -1 on failure

 ********************************************************************************/

static int Poll_socket (int io_type) {
   int ret;
   fd_set fd_rset;
   fd_set fd_wset;
   struct timeval timeout;
   int err;
   int ret_code;
   static int err_cnt = 0;

      /* set response timeout to 1 second */

   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

      /* specify the descriptors to poll */

   switch (io_type) {
      case IO_WRITE:
         FD_ZERO (&fd_wset);
         FD_SET (Sock_d, &fd_wset);
         ret = select (Sock_d + 1, NULL, &fd_wset, NULL, &timeout);

         if (ret < 0) {  /* an error occurred */

            err = errno;
            ++err_cnt;

            if ((err == EINTR) || (err == EAGAIN))
               ret_code = 0;
            else {   
               LE_send_msg (MNGRED_DEBUG_VL, 
                            "select() failed (errno: %d)\n", errno);
               ret_code = -1;
            }

            if (err_cnt >= 5) {
              Socket_closed = 1;
              close (Sock_d);
              err_cnt = 0;
            }
            return (ret_code);
         }

         err_cnt = 0;

         if (FD_ISSET(Sock_d, &fd_wset))
            return (1);
         else
            return (0);
      break;

      case IO_READ:
         FD_ZERO (&fd_rset);
         FD_SET (Sock_d, &fd_rset);
         ret = select (Sock_d + 1, &fd_rset, NULL, NULL, &timeout);
         
         if (ret < 0) {  /* an error occurred */

            err = errno;
            ++err_cnt;

            if ((err == EINTR) || (err == EAGAIN))
               ret_code = 0;
            else {   
               LE_send_msg (MNGRED_DEBUG_VL, 
                            "select() failed (errno: %d)\n", errno);
               ret_code = -1;
            }
            if (err_cnt >= 5) {
              Socket_closed = 1;
              close (Sock_d);
              err_cnt = 0;
            }
            return (ret_code);
         }

         err_cnt = 0;

         if (FD_ISSET(Sock_d, &fd_rset))
            return (1);
         else
            return (0);
      break;
   }

   return (-1);

}


/********************************************************************************

     Description: Read the message received from the DIO module

 ********************************************************************************/ 

static int Read_dio_msg (char *buf, int buf_size) {

   int ret;

   ret = read (Sock_d, buf, buf_size);

   if (ret <= 0) {
      if (ret < 0)
         LE_send_msg (MNGRED_OP_VL, "socket read error occurred (errno: %d)",
                      errno);
      else
         LE_send_msg (MNGRED_OP_VL, "socket closed by peer");

      close (Sock_d);
      Socket_closed = 1;
      return (-1);
   }

   if (ret == buf_size) {
      LE_send_msg (MNGRED_OP_VL,
           "Warning: DIO msg size read == max buffer size (max buf size: %d)\n",
           buf_size); 
   }

   LE_send_msg (MNGRED_DIOM_VL, "socket read returned: %d\n", ret);

   return (ret);
}


/********************************************************************************

     Description: Send the command to the DIO module

 ********************************************************************************/

static int Send_command (void) {

   int ret;
   int err;

   if ((ret = Poll_socket (IO_WRITE)) < 0) {
      LE_send_msg (LE_CRITICAL_BIT, "socket poll failed for socket write");
      return (-1);
   } else if (ret == 1) {   /* we can write without blocking */

         /* send the command to the DIO module */

      if ((ret = write (Sock_d, &Message, 
                        Message.hdr.msg_len + sizeof (cmd_hdr_t))) <= 0) {
         err = errno;
         LE_send_msg (LE_CRITICAL_BIT, 
                      "socket write failed (ret: %d;  errno: %d)\n",
                      ret, err);
         if ((err == ECONNRESET) || (err == ENOTCONN) || (err == ETIMEDOUT)
                                 || (err == EPIPE))
              Socket_closed = 1;
              close (Sock_d);
      
         return (-1);
      }
      return (0);
   }
   return (0);
}


/********************************************************************************

     Description: Set the properties for the socket, protocol layers

 ********************************************************************************/

static int Set_properties (void) {

   int flag = 1;
   struct linger lig;

      /* enable address re-use */

   if (setsockopt(Sock_d, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, 
                  sizeof(flag)) < 0)
       LE_send_msg (LE_CRITICAL_BIT, 
                    "setting SO_REUSEADDR failed (errno: %d)\n", errno);

       /* disable buffering of short data in TCP layer */

    if (setsockopt (Sock_d, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, 
                    sizeof (flag)) < 0)
        LE_send_msg (LE_CRITICAL_BIT, 
                     "setting TCP_NODELAY failed (errno %d)", errno);

       /* turn off lingering */

    lig.l_onoff = 0;
    lig.l_linger = 0;

    if (setsockopt (Sock_d, SOL_SOCKET, SO_LINGER, (char *)&lig, 
                    sizeof (struct linger)) < 0)
        LE_send_msg (LE_CRITICAL_BIT, 
                     "setting SO_LINGER failed (errno %d)", errno);

   /* set non-blocking IO */

#ifdef HPUX
    if (fcntl (Sock_d, F_SETFL, O_NONBLOCK) < 0) {
#else
    if (fcntl (Sock_d, F_SETFL, O_NDELAY) < 0) {
#endif
        LE_send_msg (LE_CRITICAL_BIT, 
                     "fcntl O_NDELAY failed (errno %d)\n", errno);
        return (-1);
    }

   return (0);
}
