/********************************************************************************

     Description: This file contains the TCP/IP socket server routines used by 
                  the rms_interface task

********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/30 19:50:55 $
 * $Id: rms_socket_mgr.c,v 1.7 2013/04/30 19:50:55 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */


#include "orpgevt.h"
#include <orpgtask.h>
#include <orpgsite.h> 
#include <fcntl.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h> 
#include <unistd.h>

#include <errno.h>

#include <rms_ptmgr.h>


#define MAX_NAME_LEN           256
#define BUF_BLOCK_SIZE        4000        /* read/write block increment size */
#define RMS_PORT             49500        /* RPG RMS app server port number */
#define RPG_RMS_MSG_HDR_SIZE    11
#define MAX_MSG_LEN              MAX_NAME_LEN

#define RMS_READ                 0
#define RMS_WRITE                1

#define SOCKET_ERROR            -1
#define INVALID_SOCKET          -1

   /* ICD msg types */

#define RMS_STATUS_MSG           2
#define RDA_STATE_CMD            4
#define RDA_CHANNEL_CNTRL_CMD    5
#define WIDEBAND_LOOPBACK_CMD   12
#define RMS_FTM                 14
#define RMS_ACK_MSG             37


typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

   /* task level globals */

ushort reject_flag = 0; /* Reason msg rejected from being sent to the FAA/RMMS */

static int   Msg_write_pending = 0;
static int   Input_lbd;
static int   Output_lbd;
static pid_t Ch_pid;
static int   Reset_connection = 0;
static int   Terminate_process = 0;


    /* local routines */
    
static void Byte_swap_msg (char *msg);
static void Byte_swap_msg_hdr (char *msg);
static void Get_server_ip (uint *ip_addr, int channel_num);
static int  Initialize_socket (int *sd);
static void Log_msg_recvd (char *buffer);
static int  Process_input_data (char *read_buf, int bytes_in_buffer);
static int  Poll_socket (int io_type, int sd);
static void Populate_socket_struc (int server_addr, struct sockaddr_in *sock_struct);
static int  Read_data (int sd);
static int  Read_socket (int sd, char **read_buffer); 
static void Reset_connection_callback (en_t evtcd, void *msg, size_t msglen);
static void Run_rms_server (void);
static int  Set_sock_opts (int *sd);
static void Service_conn (int sock_d);
       void Write_msg_callback (en_t evtcd, void *msg, size_t msglen);
static int  Termination_handler (int signal, int sig_type);
static int Write_socket (int sd);


/********************************************************************************

    Description: This routine returns the pid of the child process

         Return: pid of child

********************************************************************************/

pid_t RMS_get_child_pid (void) {
   return (Ch_pid);
}


/********************************************************************************

    Description: This routine forks the child responsible for performing 
                 socket I/O

          Input:    argc, argv: command line arguments
                      input_lb: LB used to store the data read from the socket
                     output_lb: LB used to retrieve the data to be written out
                                the socket
                 input_lb_attr: attributes of the input LB
                output_lb_attr: attributes of the output LB 

         Return: 0 on success; -1 on error

 ********************************************************************************/

int RMS_socket_mgr (int argc, char **argv, char *input_lb, char *output_lb,
                    LB_attr input_lb_attr, LB_attr output_lb_attr) {
                    
   int abort_process = 0;
   int ret;

   if ((Ch_pid = fork()) == 0) {  /* cleanup child before launching the server */

         /* Register the termination handler */

      if (ORPGTASK_reg_term_handler(Termination_handler) != 0){
         LE_send_msg( GL_ERROR, "Unable to register termination handler.\n");
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
      }

      if ((Input_lbd = LB_open (input_lb, LB_WRITE, &input_lb_attr)) < 0) {
         LE_send_msg (GL_ERROR, "Failure opening input LB (err: %d)",
                      Input_lbd);
         abort_process = 1;
      }

      if ((Output_lbd = LB_open (output_lb, LB_WRITE, &output_lb_attr)) < 0) {
         LE_send_msg (GL_ERROR, "Failure opening output LB (err: %d)",
                      Input_lbd);
         abort_process = 1;
      }

      if (( ret = EN_register 
            (ORPGEVT_RMS_SEND_MSG, (void *) Write_msg_callback )) < 0) {
         LE_send_msg(GL_ERROR, 
        "Failed to register event ORPGEVT_RMS_SEND_MSG (ret: %d)",
        ret );
        abort_process = 1;
      }

      if (abort_process) {
         EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
      }

      sleep (1); /* give the parent time to initialize */
      Run_rms_server ();
   }
   return (0);
}


/********************************************************************************

    Description: This routine cleans up and terminates the child process

         Return: 0 on success

 ********************************************************************************/

 int RMS_terminate_server (void) {


      /* Kill the server/child process */

   if (Ch_pid == 0)
      LE_send_msg(GL_ERROR, "RMS child process not found (Ch_pid == 0)");
   else {
      kill(Ch_pid, 1);
      LE_send_msg(GL_INFO, "RMS child process terminating");
   }
   return (0);
}


/********************************************************************************

    Description: This routine bytes swaps the msg fields depending on msg type

          Input: buffer containing the msg

 ********************************************************************************/

 static void Byte_swap_msg (char *msg) {

   ushort *short_ptr;
   ushort temp;
   ushort temp2;
   ushort msg_type;
   ushort msg_size;
   int    i;

   msg_size = *(ushort *)(msg);
   msg_size -= (RPG_RMS_MSG_HDR_SIZE + 2);  /* 2 == termination chars */
   msg_type = *(ushort *)(msg + 8);

   short_ptr = (ushort *)msg;
   short_ptr += RPG_RMS_MSG_HDR_SIZE;

   switch (msg_type) {

      case RMS_ACK_MSG:
         for (i = 0; i < 3; i++) {
            temp = (0xff00 & ((*short_ptr) << 8));
            temp += (0x00ff & ((*short_ptr) >> 8));
            *short_ptr = temp;
            ++short_ptr;
         }
      break;

      case RMS_FTM:
         for (i = 0; i < 2; i++) {
            temp = (0xff00 & ((*short_ptr) << 8));
            temp += (0x00ff & ((*short_ptr) >> 8));
            *short_ptr = temp;
            ++short_ptr;
         }
      break;

      case RDA_STATE_CMD: /* byte swap state cmd only and not the character data */
      case RDA_CHANNEL_CNTRL_CMD: /* byte swap only the channel control state field */
         temp = (0xff00 & ((*short_ptr) << 8));
         temp += (0x00ff & ((*short_ptr) >> 8));
         *short_ptr = temp;
      break;

      case WIDEBAND_LOOPBACK_CMD:
         temp = (0xff00 & ((*short_ptr) << 8));
         temp += (0x00ff & ((*short_ptr) >> 8));
         temp2 = (0xff00 & ((*(short_ptr + 1)) << 8));
         temp2 += (0x00ff & ((*(short_ptr + 1)) >> 8));
         *short_ptr = temp2;
         ++short_ptr;
         *short_ptr = temp;
      break;

      default:  /* byte swap all H/Ws */
         for (i = 0; i < msg_size; i++) {
            temp = (0xff00 & ((*short_ptr) << 8));
            temp += (0x00ff & ((*short_ptr) >> 8));
            *short_ptr = temp;
            ++short_ptr;
         }
      break;
   }   
   return;
}


/********************************************************************************

    Description: This routine bytes swaps the msg header fields

          Input: buffer containing the msg

         Return: 

 ********************************************************************************/

 static void Byte_swap_msg_hdr (char *msg) {

   int    i;
   ushort *short_ptr;
   ushort temp;

   short_ptr = (ushort *)msg;

      /* byte swap the msg hdr */

   for (i = 0; i < RPG_RMS_MSG_HDR_SIZE; i++) {
      temp = (0xff00 & ((*short_ptr) << 8));
      temp += (0x00ff & ((*short_ptr) >> 8));
      *short_ptr = temp;
      ++short_ptr;
   }
   return;
}


/********************************************************************************

     Description: This routine gets the local host IP address to use for 
                  servicing the RMS connection
           Input: channel_num - the channel number of this channel

          Output: ip_addr - the IP address interface to use for this connection
          
 ********************************************************************************/

static void Get_server_ip (uint *ip_addr, int channel_num) {

   struct hostent *localhosts;
   struct in_addr *addr_ptr;
   uint   ip_int_addr;
   char   local_hostname [2][10] = {{ "rpg1-eth1\0"},
                                    {"rpg2-eth1\0"}};
   if ((localhosts = gethostbyname (local_hostname[channel_num - 1])) == NULL) {
       LE_send_msg (GL_ERROR, "gethostbyname() failed (h_errno: %d)\n", h_errno);
       EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
       ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   addr_ptr = (struct in_addr *) *localhosts->h_addr_list;

   memcpy (&ip_int_addr, (char *) *&addr_ptr, 4);

   LE_send_msg (GL_INFO, "local ip address: %s (hex: 0x%x)", 
                inet_ntoa(*addr_ptr), htonl (ip_int_addr));

   return;

}


/********************************************************************************

    Description: This routine initializes the socket structure

          Input: sd - the descriptor of the socket to initialize

         Return: 0 on success; -1 on error

 ********************************************************************************/

#define MAX_CONNS_ALLOWED 2  /* apparently Linux is not padding this number for
                                open/close pending connections */

static int Initialize_socket (int *sd) {

   uint server_ip_addr;
   int channel_number;
   struct sockaddr_in server_addr;

   channel_number = ORPGSITE_get_int_prop(ORPGSITE_CHANNEL_NO);

      /* get the server/RPG IP address */

   Get_server_ip (&server_ip_addr, channel_number);

      /* pouplate the socket structure */

   Populate_socket_struc (server_ip_addr, &server_addr);

     /* initialize the socket */
 
   if ((*sd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      LE_send_msg (GL_ERROR, "socket () err (err: %d)", errno);
      return (-1);
   }

   Set_sock_opts (sd); 

   if ((bind (*sd, (struct sockaddr_in *)&server_addr, sizeof (server_addr))) == -1) {
      LE_send_msg (GL_ERROR, "bind() failed (err: %d: sd: %d)", errno, *sd);
      return (-1);
   }

   if (listen(*sd, MAX_CONNS_ALLOWED) == -1) {
      LE_send_msg (GL_ERROR, "listen() failed (err: %d)", errno);
      return (-1);
   }

   return (0);
}


/**************************************************************************

    Description: This function hex dumps the message read to the log file

          Input: buffer - the buffer containing the msg to dump

**************************************************************************/

static void Log_msg_recvd (char *buffer) {

   int i;
   char log_msg [MAX_MSG_LEN];
   char msg_segment [MAX_MSG_LEN];
   int  print_msg = 0;

   ushort msg_type;
   ushort msg_size;
   ushort short_wd;
   uchar  byte1;
   uchar  byte2;

   msg_size = *(ushort *)buffer;
/*   msg_size *= 2; */ /* convert size to bytes */
   msg_type = *(ushort *)(buffer + 8);

   if (msg_type == RMS_ACK_MSG)  /*don't log ack's...they're filling up the log */
     return;

   LE_send_msg (GL_INFO, "Msg contents:  ");

   strcpy (&log_msg[0], "\0");

   short_wd = *(ushort *)buffer;

   byte1 = *(uchar *)buffer;
   byte2 = *(uchar *)(buffer + 1);

   for (i = 0; i < msg_size; i++) {
      print_msg = 1;
      if ((i % 4) == 0) {
         LE_send_msg (GL_INFO, "%s", log_msg);
         print_msg = 0;
         strcpy (&log_msg[0], "\0");
         sprintf (msg_segment, "  %04d   %04hx", i, short_wd);
      } else
         sprintf (msg_segment, "  %04hx", short_wd);

      strcat (log_msg, msg_segment);
      buffer = buffer + 2;
      short_wd = *(ushort *)buffer;
   }

   if (print_msg)
      LE_send_msg (GL_INFO, "%s", log_msg);


      /* this code will print the data to the log in actual byte order
         (i.e. Little Endian order for the Linux PC)
         

   for (i = 0; i < msg_size; i++) {
      print_msg = 1;
      if ((i % 4) == 0) {
         LE_send_msg (GL_INFO, "%s", log_msg);
         print_msg = 0;
         strcpy (&log_msg[0], "\0");
         sprintf (msg_segment, "  %04d   %02hx%02hx", i, byte1, byte2);
      } else
         sprintf (msg_segment, "  %02hx%02hx", byte1, byte2);

      strcat (log_msg, msg_segment);
      buffer = buffer + 2;
      byte1 = *(uchar *)buffer;
      byte2 = *(uchar *)(buffer + 1);
   }

   if (print_msg)
      LE_send_msg (GL_INFO, "%s", log_msg);
*/

   return;
}


/********************************************************************************

    Description: This routine performs the polling functionality to determine
                 if data can be read from or written to the socket

          Input: action - type of poll to perform (read or write)
                     sd - the socket decsriptor to poll

         Return: -1 if I/O can not be performed (i.e. socket is closed, etc.)
                  0 if there is no I/O to perform;
                  1 if there is data to read, or if data can be written without
                    blocking

 ********************************************************************************/

static int Poll_socket (int action, int sd) {

   int    ret = 0;
   int    err;
   fd_set fd_ioset;
   struct timeval timeout;

      /* initialize the socket if needed */

   if ( sd == INVALID_SOCKET)
      return (-1);

      /* set select call to non-blocking  */

   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

      /* specify the descriptors to poll */

   FD_ZERO (&fd_ioset);
   FD_SET (sd, &fd_ioset);

      /* check for something to read from the socket */

   switch (action) {
      case RMS_READ:
         if ((ret = select ((int)(sd + 1), &fd_ioset, NULL, NULL, &timeout)) ==
                                  SOCKET_ERROR) {
              err = errno;
              LE_send_msg (GL_ERROR, "select () read error (errno: %d)", err);
              if ((err == EINTR) || (err == EAGAIN))
                 ret = 0;
         }
      break;

      case RMS_WRITE:
         if ((ret = select ((int)(sd + 1), NULL, &fd_ioset, NULL, &timeout)) ==
                                  SOCKET_ERROR) {
              err = errno;
              LE_send_msg (GL_ERROR, "select () write error (errno: %d)", err);
              if ((err == EINTR) || (err == EAGAIN))
                 ret = 0;
         }
      break;

      default:
         LE_send_msg (GL_ERROR, 
                      "Invalid socket status check received (rec'd: %d)\n",
                      action);
         return (0);
      break;
   }

   return (ret);
}


/********************************************************************************

    Description: This function populates the socket structure

          Input: server_addr - the IP address this host

         Output: sock_struct - the socket strcuture

 ********************************************************************************/

 static void Populate_socket_struc (int server_addr, struct sockaddr_in *sock_struct) {

   memset ((void *) sock_struct, '\0', sizeof (struct sockaddr_in)); 
   sock_struct->sin_family = AF_INET;
   sock_struct->sin_port = htons(RMS_PORT);
   sock_struct->sin_addr.s_addr = htonl(server_addr);

   return;
}


/********************************************************************************

    Description: This function parses the data block read from the socket into
                 ICD messages then calls the routine to process the messages

          Input:        read_buf - the buffer containing the data read
                 bytes_in_buffer - the number of bytes read by the last socket
                                   read

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Process_input_data (char *read_buf, int bytes_in_buffer) {

   int ret;
   int bytes_parsed = 0;
   ushort msg_size = 0;
   ushort msg_type = 0;
   int halfwds_in_buf;

   halfwds_in_buf = (bytes_in_buffer / 2) + (bytes_in_buffer % 2);

   while (bytes_parsed < bytes_in_buffer) {

         /* byte swap the message */

      Byte_swap_msg_hdr (read_buf + bytes_parsed); 
      Byte_swap_msg (read_buf + bytes_parsed); 

      msg_size = *(ushort *)(read_buf + bytes_parsed);
      msg_size *= 2; /* convert size to bytes */
      msg_type = *(ushort *)(read_buf + 8 + bytes_parsed);

      LE_send_msg (GL_INFO, "Msg type %d recv'd (msg size: %d bytes)\n",
                   msg_type, msg_size);

         /* see if a partial message was received at the end of the buffer.
            if it was then discard the partial message...the client will resend 
            the message due to not receiving an ACK msg. This is not a pretty 
            solution but it is convenient and effective for now
            ****This needs to be changed when time permits ***** */

      if ((bytes_parsed + msg_size) > bytes_in_buffer) {
         LE_send_msg (GL_INFO, "Partial msg received...discard it\n");
         Log_msg_recvd (read_buf + bytes_parsed);
         break;
      } else { /* write msg to the LB for the parent to process */
      
         Log_msg_recvd (read_buf + bytes_parsed);

         ret = LB_write (Input_lbd, read_buf + bytes_parsed, msg_size, LB_ANY);

         if (ret < 0) {
            LE_send_msg (GL_ERROR, "Error writing to LB (err: %d)", ret);
            return (-1);
         }

         if (ret != msg_size)
            LE_send_msg (GL_ERROR, 
              "LB_write () failed to write complete msg (ret: %d;  msg_size: %d)",
              ret, msg_size);

         bytes_parsed += msg_size;
      }
      LE_send_msg (GL_INFO, "# bytes processed: %d (# bytes in buffer: %d)\n",
                   bytes_parsed, bytes_in_buffer);
   }

      /* Post the event for RMS message received */

   if ((ret = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0)) < 0) {
        LE_send_msg (GL_ERROR,
            "Failed to Post ORPGEVT_RMS_MSG_RECEIVED (%d).\n", ret );
        return (-1);
   } else
      return (0);
}


/********************************************************************************

    Description: This routine reads data from the socket

          Input: sd - socket descriptor to read from

         Return: -1 if the socket poll or read failed, or if interrogating
                    the msg failed
                  0 if there is nothing to read or if the data read was
                  successfully parsed

 ********************************************************************************/

static int Read_data (int sd) {
   int  bytes_read;
   char *read_buf = NULL;
   int  ret;

      /* poll the socket to check for something to read */

   ret = Poll_socket (RMS_READ, sd);

      /* return if there was an error, or if there is nothing to read */

   if ((ret == -1) || (ret == 0))
      return (ret);

      /* read the data if there's input data in the queue */

   if ((bytes_read = Read_socket (sd, &read_buf)) == SOCKET_ERROR) {
       LE_send_msg (GL_ERROR, "Socket read error (errno: %d)\n", errno);
       return (-1);
   } else if (bytes_read == 0) { 
        return (0);
   } else {  /* parse the input data */
        ret = Process_input_data (read_buf, bytes_read);
        return (ret);
   }
}


/********************************************************************************

    Description: This routine reads all the data currently in the TCP buffer

          Input: sd - the socket descriptor to read from

         Output: read_buffer - buffer containing the data read

         Return: bytes_read: # of bytes read, or SOCKET_ERROR on read error

 ********************************************************************************/


static int Read_socket (int sd, char **read_buffer) {

   static int  buf_size = BUF_BLOCK_SIZE; 
   static char *buffer = NULL;
   char        *new_buf_ptr = NULL;
   int         buf_offset = 0;
   int         buf_read_size;
   int         len_ret = 0;
   int         bytes_read = 0;


      /* this executes only on the first pass */

   if (buffer == NULL) {
      if ((buffer = (char *) malloc (buf_size)) == NULL) {
         LE_send_msg (GL_ERROR, "Read_socket: malloc failed\n");
         EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
      }
   }

   buf_read_size = buf_size;
   *read_buffer = buffer;

      /* read all the data in the TCP queue */

   while ((len_ret = 
           recv (sd, buffer + buf_offset, buf_read_size, 0)) == buf_read_size) {

         /* increase buffer size by BUF_BLOCK_SIZE bytes */

      new_buf_ptr = (char *) realloc (buffer, buf_size + BUF_BLOCK_SIZE);

      if (new_buf_ptr == NULL) {
          LE_send_msg (GL_ERROR, 
                   "realloc failed increasing buffer size to %d bytes\n",
                   buf_size + BUF_BLOCK_SIZE);
         EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
      }

      LE_send_msg (GL_INFO, "read buffer realloc'ed (new size: %d bytes)\n",
                   buf_size + BUF_BLOCK_SIZE);

         /* update the buffer to the new memory space */

      buffer = new_buf_ptr;
      *read_buffer = buffer;

         /* update buffer parameters for the next pass of the loop */

      buf_offset += buf_read_size;
      buf_read_size = BUF_BLOCK_SIZE;
      buf_size += BUF_BLOCK_SIZE;  /* update the size of the mem buffer */

      bytes_read += len_ret;
   }

      /* account for residual bytes from last read, or determine if an 
         error occurred */

   if (len_ret > 0)
      bytes_read += len_ret;
   else if (len_ret == 0)  /* socket has been closed by peer */
        bytes_read = SOCKET_ERROR;
   else if ((len_ret < 0) && (bytes_read == 0)) {
      if ((errno != EAGAIN) && (errno != EINTR))
         bytes_read = SOCKET_ERROR;
   }
      
   return (bytes_read);
}


/********************************************************************************

    Description: This is the callback routine to notify the child process to
                 reset the connection...the parent timed out

          Input: 
          
         Output: Reset_connection - flag specifying the socket should be closed

 ********************************************************************************/

#define RMS_CONNECTION_TIMED_OUT 3

static void Reset_connection_callback (en_t evtcd, void *msg, size_t msglen) {
   int reset_type;

   reset_type = *(int *)msg;
  
   if (reset_type == RMS_CONNECTION_TIMED_OUT)
      Reset_connection = 1;

LE_send_msg (0, "Reset_conn_callback: reset_type: %d;  Reset_connection flag: %d",
             reset_type, Reset_connection);

   return;
}   


/********************************************************************************

    Description: This is the main server routine. It's only funtion is to process
                 I/O through the TCP/IP socket.

 ********************************************************************************/

 static void Run_rms_server (void) {

   int listen_sd;
   int conn_sd;
   int new_sd;
   int ret;
   int client_connected = 0;
   
   if ((Initialize_socket (&listen_sd)) == -1) {
         EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   while (1) {
      conn_sd = -1;
      new_sd = -1;
/*      conn_sd = accept (listen_sd, (struct sockaddr_in *) NULL, NULL);
      if ((conn_sd == -1) || (client_connected))
         continue;
*/
      new_sd = accept (listen_sd, (struct sockaddr_in *) NULL, NULL);
      if ((new_sd == -1) || (client_connected)) {
         if ((client_connected) && (new_sd > 0))
            close (new_sd);  /* allow only one connection */
         else if (!client_connected && Terminate_process)
            exit(0);
         continue;
      }

      conn_sd = new_sd;

      Set_sock_opts (&conn_sd);

      LE_send_msg (GL_INFO, "Client connected - servicing the connection");

      client_connected = 1;

      Service_conn (conn_sd);

      LE_send_msg (GL_INFO, "Closing client connection");
      close (conn_sd);
      client_connected = 0;

         /* De-register for the callback for the connection timeout event */

      if (( ret = EN_deregister 
            (ORPGEVT_RESET_RMS, (void *) Reset_connection_callback )) < 0) {
         LE_send_msg (GL_ERROR, 
           "Failed to de-register event ORPGEVT_RESET_RMS (ret: %d)", ret );
      }
   }

   exit(0);  /* never happens...is killed by the parent */
 }


/********************************************************************************

    Description: This routine services the socket connection after the client
                 connect request is accepted

          Input: sock_d - the socket descriptor
          
 ********************************************************************************/

#define RMS_CONNECTION_LOST 4

static void Service_conn (int sock_d) {
   int conn_state = RMS_CONNECTION_LOST;
   int ret;

      /* register for the callback for the connection timeout event */

   if (( ret = EN_register 
         (ORPGEVT_RESET_RMS, (void *) Reset_connection_callback )) < 0) {
      LE_send_msg (GL_ERROR, 
         "Failed to register event ORPGEVT_RESET_RMS (ret: %d)",
      ret );

      EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

      /* set the connected socket to non-blocking */

   if (fcntl (sock_d, F_SETFL, O_NONBLOCK) < 0) {
      LE_send_msg (GL_ERROR, "Error setting non-blocking mode on sock_d: %d",
                   sock_d);
      EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);
      ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
   }

   while (1) {

      if (Read_data (sock_d) == -1) {
         if ((ret = 
                EN_post (ORPGEVT_RESET_RMS, &conn_state, sizeof(conn_state), 0)) < 0)
             LE_send_msg(GL_ERROR, "Child: Failed to Post ORPGEVT_RESET_RMS (%d)\n", ret);
         Msg_write_pending = 0;
         return;
      }

      if (Msg_write_pending) {
         if (Write_socket (sock_d) == -1) {
            if ((ret = 
                 EN_post (ORPGEVT_RESET_RMS, &conn_state, sizeof(conn_state), 0)) < 0)
                LE_send_msg(GL_ERROR, "Child: Failed to Post ORPGEVT_RESET_RMS (%d)\n", ret);
            Msg_write_pending = 0;
            return;
         }
         Msg_write_pending = 0;
      }

      if (Terminate_process) {
         Write_socket (sock_d);
         exit (0);
      }

      sleep (1);

      if (Reset_connection) {
         Reset_connection = 0;
         return;
      }   
   }
   return;
}


/********************************************************************************

    Description: This routine sets the socket and TCP/IP options

          Input: sd - the socket descriptor
          
         Output: 

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Set_sock_opts (int *sd) {

  struct linger ling;
  int flag = 1;

      /* disable buffering of short data in TCP layer */

   if (setsockopt (*sd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                   sizeof (flag)) == SOCKET_ERROR) 
          /* complain but continue */
       LE_send_msg (GL_ERROR,
                "Error setting TCP_NODELAY (errno: %d)...continue", 
                errno);

      /* enable port reuse */

   if (setsockopt (*sd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag,
                   sizeof (flag)) == SOCKET_ERROR) 
          /* complain but continue */
       LE_send_msg (GL_ERROR,
                "Error setting \"REUSE ADDR\" (errno: %d)...continue", 
                errno);

      /* turn off lingering */

   ling.l_onoff = 0;
   ling.l_linger = 0;

   if (setsockopt (*sd, SOL_SOCKET, SO_LINGER, (char *)&ling,
                   sizeof (struct linger)) == SOCKET_ERROR) 
          /* complain but continue */
       LE_send_msg (GL_ERROR,
                "Error setting \"No Linger\" option (errno: %d)...continue", 
                errno);

      /* set non-blocking I/O */
      
/*   if (ioctlsocket (Sock_d, FIONBIO, (u_long FAR*) &non_block_mode) ==
                                                          SOCKET_ERROR) {
      closesocket (Sock_d);
      Sock_d = INVALID_SOCKET;
      LE_send_msg (GL_ERROR,
               "Error setting socket to non-blocking (errno: %d)\n",
                errno);
      return (-1);
   }
*/
   LE_send_msg (GL_INFO, "Socket %d initialized\n", *sd);

   return (0);

}


/********************************************************************************

    Description: This routine is the termination handler

          Input: signal  - the signal that caused the handler to be called
                 sigtype - the type of signal 
          
         Output: 

         Return: 1 - don't terminate now...the process will terminate itself later
                 after performing cleanup

 ********************************************************************************/

static int Termination_handler (int signal, int sigtype) {

   EN_post (ORPGEVT_RMS_SOCKET_MGR_FAILED, NULL, 0, 0);

   LE_send_msg (GL_INFO, "Child Termination Handler called (sig: %d;  sigtype: %d)",
                signal, sigtype);

   sleep (1);  /* give time for the event to be delivered */

   Terminate_process = 1;

   return (1);
}


/********************************************************************************

    Description: This is the callback routine to notify the process when
                 LB_writes occur

          Input: 
          
         Output: Msg_write_pending - file scope flag specifying a message write
                                     is ready to be serviced

 ********************************************************************************/

void Write_msg_callback (en_t evtcd, void *msg, size_t msglen) {
   Msg_write_pending = 1;
   return;
}   


/********************************************************************************

    Description: This routine writes data to the socket

          Input: sd - the socket descriptor

         Return:  0 on success;
                 -1 on error

 ********************************************************************************/

static int Write_socket (int sd) {

   int  buffer_len;
   char buffer[BUF_BLOCK_SIZE];
   int  write_err;
   int  msg_len;
   int  ret;
   int  bytes_written;
   char *msg;

      /* poll the socket to see if we can write data */

   ret = Poll_socket (RMS_WRITE, sd);

      /* return if there was an error */

   if (ret == -1)
      return (-1);

      /* flush the expired msgs */

    while ((ret = LB_read (Output_lbd, (char *) buffer, BUF_BLOCK_SIZE, LB_NEXT)) 
            == LB_EXPIRED);

      /* write the data to the socket */

   do {
      int number_halfwds = 0;
      int number_retries = 0;
/*      ushort *short_ptr;
      ushort temp;
      
      int    i;
      
      if (ret > 0) {
         LE_send_msg (GL_INFO, "Msg before byte swapping: ");
         for (i = 0; i < *(ushort *)buffer; i++) {
            LE_send_msg (GL_INFO, "%d %04x",i, *(ushort *)(buffer + (i * 2)));
         }
      }
*/
      if (ret == LB_TO_COME)
         return (0);

/*      LE_send_msg (GL_INFO, "LB_read returned: %d", ret); */

      if (ret < 0) {
         LE_send_msg (GL_ERROR, "LB_read error occurred (err: %d)", ret);
         return (-1);
      }
        /* get the actual size of the message...LB_read does not return this */

      number_halfwds = *(ushort *)buffer;
      msg_len = number_halfwds * 2;
      bytes_written = 0;
      buffer_len = msg_len;
      msg = buffer;

      LE_send_msg (GL_INFO, "Write msg: (Msg type: %d;  msg size: # bytes: %d)", 
                   *(ushort *)(buffer + 8), msg_len);

         /* byte swap the message header */

      Byte_swap_msg_hdr (buffer);

      while (bytes_written < msg_len) {

         if ((ret = send (sd, &buffer[bytes_written], buffer_len, 0)) == -1) {
             write_err = errno;

             if (((write_err == EINTR) || (write_err == EAGAIN )) && (number_retries < 3)) {
                msleep (1000);
                if (write_err == EAGAIN)
                   ++number_retries;
                continue;
             } else {
                LE_send_msg (GL_ERROR, "send() error (errno: %d)\n", 
                             write_err);
                return (-1);
             }
         } else {

            bytes_written += ret;
            buffer_len = msg_len - bytes_written;
         
            LE_send_msg (GL_INFO, 
                         "   send(): %d bytes written to socket (ret: %d)\n",
                         bytes_written, ret);
         }
      }
   } while ((ret = LB_read (Output_lbd, (char *) buffer, BUF_BLOCK_SIZE, LB_NEXT)) 
             != LB_EXPIRED);

   return (0);
}
