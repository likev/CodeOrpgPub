/********************************************************************************

       File: socket_io.c
             This file contains all the routines used to open, close, 
             read, and write to/from the sockets IAW ORPG narrowband 
             TCP requirements.

             The following functions are performed in this file:
             1. open, close, connect to server (ie. ORPG)
             3. set TCP socket properties 
             3. log on
             4. send and receive orpg messages/products

      Note(s) from original author: New version implementation of client side
                                    for TCM ICD 2/5/01

  ********************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:49:30 $
 * $Id: nbtcp_socket_mgr.c,v 1.12 2014/03/18 18:49:30 jeffs Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */


#include <errno.h>
#include <sys/types.h> /* fcntl(...)  */
#include <unistd.h>    /*      "      */
#include <fcntl.h>     /*      "      */
#include <signal.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>    /* free(...)   */
#include <string.h>
#include <strings.h>   /* bzero(...)
                          bcopy(...)  */
#include <bzlib.h>     /* bzip2 compression/decompression routines */
#include <zlib.h>      /* glib compression/decompression routines */

#include <sys/socket.h>    /* TCP system/lib calls  */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <netdb.h>

#include "nbtcp.h"


#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif


extern int errno; 


   /* process level globals */

extern int    PVC0_sock, PVC1_sock;
extern char   Password [10];

typedef unsigned char uchar;

   /* ICD defined message compression header format */
   
typedef struct {
   char method;
   char multi_msg_flag;
   char spare [2];
   int  uncompressed_len;
} Compression_hdr_t;


   /* file scope globals */

static char	      Hd[TCP_HDR_LEN];
static tcp_header *Tcp_hdr;
static struct     sockaddr_in Sinaddr;
static char       Log_msg [128];
static int        Class;  /* CLASS_1 or CLASS_2 */


   /* local function prototypes */
        
static int Create_socket (char *server, u_short port);
static int Decompress_msg (int method, char *src_buf, unsigned int src_len, 
                           char *dest_buf, unsigned int dest_len);
static int Pvc_login_ack ();
static int Pvc_login (int sock, int pvc, char *passwd);
static int Read_sock (int pvc);
static int Read_tcp_header (int sd);
static int Set_tcp_properties (int fd);
static int Sock_read (int, char *buffer, int);


/********************************************************************************

     Description: This routine closes the sockets

           Input:

          Output:

          Return:

 ********************************************************************************/

void SOC_close_sockets ()
{
   close (PVC0_sock);
   close (PVC1_sock);
   return;
}


/********************************************************************************

     Description: This routine returns the internet address of the host we're 
                  connected to

           Input:

          Output:

          Return: The internet address of the host we-re connected to 

 ********************************************************************************/

char *SOC_get_inet_addr()
{
   return (inet_ntoa(Sinaddr.sin_addr));
}


/********************************************************************************

     Description: This routine initializes and connects the sockets

           Input: server_name - the name of the server we're connecting to
                  port_number - port number of the socket we're connecting to
                  password    - login password

          Output:

          Return: sockets_connected - TRUE if the sockets are connected;
                                      FALSE if the sockets are not conencted

 ********************************************************************************/

int SOC_initiate_sock_connections (char *server_name, u_short port_number,
                                    char *password, int user_class)
{
   int  sockets_connected = FALSE;

   Class = user_class;   /* Set the file scope variable */

   if((PVC0_sock = Create_socket(server_name, port_number)) < 0)
          MA_abort("CreateSocket() pvc0 failed");
        
   if(connect(PVC0_sock, (struct sockaddr *)&Sinaddr, sizeof(Sinaddr)) == -1)
   {
      if((errno == ECONNREFUSED) || (errno == EPIPE))
      {
         close(PVC0_sock);
         sleep(10); 
         return (sockets_connected);
      }
      else  MA_abort("connect() pvc0 failed");
   }

   if (Class == CLASS_1) {
      if((PVC1_sock = Create_socket(server_name, port_number)) < 0)
          MA_abort("CreateSocket() pvc1 failed");
        
      if(connect(PVC1_sock, (struct sockaddr *)&Sinaddr, sizeof(Sinaddr)) == -1)
      {
         if((errno == ECONNREFUSED) || (errno == EPIPE))
         {
            close(PVC0_sock);
            close(PVC1_sock);
            sleep(10);
            return (sockets_connected);
         }
         else 
         {
            close(PVC0_sock);
            close(PVC1_sock);
            MA_abort("connect() pvc1 failed");
         }

         if(Set_tcp_properties(PVC0_sock) < 0)
            MA_abort("set properties pvc0 failed");
   
         if(Set_tcp_properties(PVC1_sock) < 0)
             MA_abort("set properties pvc1 failed");  
      }
   }  /* end if Class */

   PROC_new_conn ();  /* set the new connection made flag */

   sprintf(Log_msg, "Sockets connected to ORPG at %s\n",
           inet_ntoa(Sinaddr.sin_addr));
   MA_printlog(Log_msg);
       
      /* log on with login message (link id, #pvcs, pvc#, password) 
         on each pvc */
      /* should receive acknowledgement message on pvc0 */
	
      /* send the login */	

   if (Pvc_login(PVC0_sock, 0, password) == -1) 
         MA_abort("pvc_login() pvc0 failed");

   if (Class == CLASS_1) {
      if (Pvc_login(PVC1_sock, 1, password) == -1) 
            MA_abort("pvc_login() pvc1 failed");
   } /* end if Class */
	   
      /* wait for ack from ORPG */

   if (Pvc_login_ack() == -1) MA_abort("pvc_login_ack failed");

   sockets_connected = TRUE;

   sprintf(Log_msg, "Narrowband login to ORPG %s Successful\n",
           inet_ntoa(Sinaddr.sin_addr));
   MA_printlog(Log_msg);   
        
   return (sockets_connected);
}

 
/********************************************************************************

     Description: This routine polls the sockets for activity and services
                  them as needed

           Input:

          Output:

          Return: -1 if a polling error occurred; otherwise, 0 is returned

 ********************************************************************************/

int SOC_pollsocks()
{
    struct pollfd psock[2];
    int poll_ret;
    
    psock[0].fd = PVC0_sock;	/* build poll struct */
    if (Class == CLASS_1)
       psock[1].fd = PVC1_sock;
    psock[0].events = POLLIN | POLLPRI;
    if (Class == CLASS_1)
       psock[1].events = POLLIN | POLLPRI;
    
    if (Class == CLASS_1)
       poll_ret = poll(psock, 2, 500); /* poll with .5 sec delay */
    else
       poll_ret = poll(psock, 1, 500); /* poll with .5 sec delay */
    
    if(poll_ret < 0) {
       if( errno != EINTR && errno != EAGAIN) {
          return (-1);
       }
       return(0);
    }

    if(poll_ret == 0) return(0);

    if(psock[0].revents != 0) {
       if((psock[0].revents) & (POLLERR | POLLHUP | POLLNVAL)) return(-1);
       if ((Read_sock (PVC0)) ==-1) return (-1);
    }

    if (Class == CLASS_1) {
       if(psock[1].revents != 0) {
          if((psock[1].revents) & (POLLERR | POLLHUP | POLLNVAL)) return(-1);    
          if ((Read_sock (PVC1)) == -1) return (-1);
       }   
    }  /* end if Class */
    return(0);
}


/********************************************************************************

     Description: This routine writes data to a socket

           Input: fd     - socket descriptor to write to
                  buffer - data buffer containing the data to write
                  size   - size in bytes of the data buffer to write

          Output:

          Return: number of bytes written, or -1 if socket is
                  disconnected or error

          Note: If the socket is blocked, it will retry until the socket
                is available for write.

 ********************************************************************************/

int SOC_sock_write(int fd, char *buffer, int size)
{
   int nwritten;
   int k;

   nwritten = 0;

   while (1) {

      errno = 0;
      k = write(fd, &buffer[nwritten], size - nwritten);
		
      if (k < 0) {
         if (errno == EWOULDBLOCK || errno == EAGAIN) {
            poll (0,0,100);
            continue;
         }

         if (errno != EINTR) {	/* fatal error */
            return (-1);  /* no fatal error - caller will reopen socket */
         }

         continue;
      }

      if (k > 0) nwritten += k;
      if (nwritten >= size) return (nwritten);
   }
}


/********************************************************************************

     Description: get a socket and set socket options, socket address structure

           Input: server - name of the server connecting to
                  port   - port number connecting to

          Output:

          Return: socket decriptor on success; -1 on error

 ********************************************************************************/

static int Create_socket(char *server, u_short port)
{
    int    sock, i;
    struct hostent *hp, *gethostbyname();
    
    i=1;
    bzero(&Sinaddr, sizeof(Sinaddr));
    Sinaddr.sin_family=AF_INET;;    
    if( (Sinaddr.sin_addr.s_addr = inet_addr(server)) == INADDR_NONE) {
            
       hp=gethostbyname(server);

       if(!hp) {
          sprintf(Log_msg, "Could not resolve %s, in hosts table?\n",
                  server);
          MA_printlog(Log_msg);
          return(-1);
       }
       bcopy(hp->h_addr, (char *)&Sinaddr.sin_addr, hp->h_length);        
    }

    Sinaddr.sin_port=htons(port);
    if((sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        close(sock);
        sprintf(Log_msg, "Get socket failed, errno =  %d\n",
	             errno);
        MA_printlog(Log_msg);

        return(-1);
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i));

    return(sock);
}


/******************************************************************************** 

     Description: Decompress the message according to the compression method

           Input: method    - compression method used to compress the message
                  *src_buf  - ptr to the source/compressed message buffer 
                  src_len   - length of the compressed message
                  dest_len  - length of the uncompressed message

          Output: *dest_buf - ptr to the destination/uncompressed message buffer

          Return: number of bytes of the uncompressed message, or 
                  -1 if message decompression failed

 ********************************************************************************/

static int Decompress_msg (int method, char *src_buf, unsigned int src_len, 
                           char *dest_buf, unsigned int dest_len) {

   int ret = 0;
   uint len = 0;
   ulong l_dest_l;

   switch (method) {

      case BZIP2_COMPRESSION:

         len = dest_len;
         ret = BZ2_bzBuffToBuffDecompress (dest_buf, &len, src_buf, src_len, 0, 0);

         if (ret != BZ_OK) {
            printf ("BZ2 Decompress error (err: %d)\n", ret);
            return (-1);
         }
      break;

      case GZIP_COMPRESSION:

         l_dest_l = dest_len;
         ret = uncompress ((uchar *) dest_buf, &l_dest_l, (uchar *) src_buf,
                           (ulong) src_len);

         if (ret != Z_OK) {
            printf ("GZIP Decompress error (err: %d)\n", ret);
            return (-1);
         }
         len = l_dest_l;
      break;

      default:
         fprintf (stderr, 
                  "Decompress_msg (): Invalid decompression method (%d)\n",
                  method);
         return (-1);
      break;
   }
   return (len);
}


/********************************************************************************

     Description: send login message to the server

           Input: sock   - socket descriptor to write the login msg to
                  pvc    - virtual circuit being logged in to
                  passwd - login password

          Output:

          Return: 0 on successful login; -1 on unsuccessful login

 ********************************************************************************/

static int Pvc_login (int sock, int pvc, char *passwd)
{
   char msg[MSG];
   tcp_header *header;
   int noofpvcs = 2;     /* # pvcs on link */
   
   if (Class == CLASS_1)
      noofpvcs = 2;     /* # pvcs on link */
   else if (Class == CLASS_2)
      noofpvcs = 1;     /* # pvcs on link */
   sprintf(msg + sizeof(tcp_header),"%d %d %d %s %s",
           0, noofpvcs, pvc, passwd,
           "UNIX ORPG Narrowband TCP client connect request"); 
   header = (tcp_header *)msg;
   header->type = htonl(LOGIN); /* message type = 0 */
   header->id = htonl(TCP_ID);  /* ORPG connection ID */
   header->length = htonl(MSG - sizeof (tcp_header)); /* msg length excluding header */
   if (SOC_sock_write(sock, msg, MSG) != MSG) return (-1);
   return(0);
}


/********************************************************************************

     Description: receive a login acknowledgement from the ORPG

           Input:

          Output:

          Return: 0 on success; -1 on error

 ********************************************************************************/

static int Pvc_login_ack()
{
    char       msg[MSG];
    tcp_header *header;
    int        ret, len;

    if ((ret = Sock_read(PVC0_sock, Hd, sizeof(tcp_header))) < 0) return(-1);    
    
    header = (tcp_header *)Hd;
    if (ntohl(header->id) != TCP_ID || ntohl(header->type) != LOGIN_ACK) {
/*        printf("incorrect login ack msg ID %d, or type %d\n", 
                 ntohl(header->id), ntohl(header->type));*/
        return(-1);    
    }
    len = ntohl(header->length);
    if (len != 0){
    if((ret = Sock_read(PVC0_sock, msg, len))!= len) return(-1);
    /* could verify ack message contains:
          linkindex, number of pvcs = 2, text="connected" */
    }      

    sprintf(Log_msg, "Login ack: %s\n",
	         msg);
    MA_printlog(Log_msg);
    return(0);
}


/********************************************************************************

     Description: Read TCP msg header from the socket

           Input: sd - socket descriptor msg received on

          Output:

          Return: len - length of the message specified in the TCP header,
                        or -1 on error

 ********************************************************************************/

static int Read_tcp_header (int sd) {

   int hdr_length, len;
   int msg_type;
   
   hdr_length = Sock_read (sd, Hd, sizeof (tcp_header));

   if (hdr_length < 0 || hdr_length != sizeof (tcp_header)) {
/*       printf("TCP hdr len = %d, errno = %d\n",hdr_length, errno); */
       return (-1);
    }

   Tcp_hdr = (tcp_header *) Hd;

   len = ntohl (Tcp_hdr->length);
   msg_type = ntohl (Tcp_hdr->type);

   if ((msg_type != KEEPALIVE) && (msg_type != DATAACK) && (msg_type != DATA)) {
     /* printf("Bad header: id: %d, msg_type: %d, len: %d\n",
                ntohl (Tcp_hdr->id), msg_type, len)); */
         return (-1);
   }
   return (len);
}


/********************************************************************************

     Description: Read PVCx socket msg. This function validates the TCP header, 
                  then decompresses and unpacks the products from the message if
                  the message has been compressed and/or the products packed 
                  within the message.
     
           Input: pvc - the pvc socket (i.e. PVC0, PVC1) to read from

          Output:

          Return: 0 on success; -1 on error


          Note:  Refer to ICD 2620041 for more information on how this
                 function processes the incoming messages. 

 ********************************************************************************/

static int Read_sock (int pvc) {

   int  i;
   int  ret;
   int  pvc_sd;
   int  pvc_ack = 0;
   int  msg_len;
   char *buf;
   int  number_packed_msgs = 1;
   int  product_size;
   char *msg_size_ptr;
   int  tmp_int;
   int  buf_offset = 0;
   char *uncompressed_buf;
   int  uncompressed_buf_len;
   int  compressed_msg_len;
   int tcp_hdr_msg_len;
   Compression_hdr_t compression_hdr;

      /* determine which socket/pvc to read from */

   if (pvc == PVC0) {
      pvc_sd = PVC0_sock;
/*      printf ("Reading PVC0 socket data\n"); */
   } else if (pvc == PVC1) {
      pvc_sd = PVC1_sock;
/*      printf ("Reading PVC1 socket data\n"); */
   } else {
      printf ("Invalid pvc_sd (%d) passed to socket read routine\n", pvc);
      return (-1);
   }
   
      /* extract the TCP header from the message */

   if((ret = Read_tcp_header(pvc_sd)) == -1) return (-1);

   if(ret == 0) {
/*      printf("TCP hdr type: %d\n", ntohl(Tcp_hdr->type));*/

      if((ntohl(Tcp_hdr->type)) == DATAACK) return (0); /* ignore an ack */

      Tcp_hdr->id = 0; /* otherwise prepare a keep-alive */

      if(SOC_sock_write(pvc_sd, Hd, TCP_HDR_LEN) != TCP_HDR_LEN) return (-1);

/*        printf("pvc KA response\n");*/

      return (0);  /* send a keep-alive */
   }

   if (ntohl (Tcp_hdr->id) != 0) pvc_ack = ntohl (Tcp_hdr->id); /*save ack #*/

      /* obtain the message length and malloc space for the message */

   tcp_hdr_msg_len = ntohl (Tcp_hdr->length);
   msg_len = tcp_hdr_msg_len;

   msg_len &= 0x7fffffff;

   buf = malloc (msg_len);

   if (buf == NULL) {
      MA_abort("Read_sock () malloc() failed");
   }

      /* read the message of msg_len as defined in the TCP header */

   ret = Sock_read (pvc_sd, buf, msg_len);

   if ((ret < 0)  || (ret != msg_len)) {
      printf ("Read_sock (): Error reading product msg (msg_len: %d;  ret: %d)\n",
              msg_len, ret);
      free (buf);
      return (-1);
   }

      /* Initialize the variables needed for further processing */

   product_size = msg_len;
   buf_offset = 0;
   number_packed_msgs = 1;

      /* if msg length is < 0, then the message is compressed and/or packed */

   if (tcp_hdr_msg_len < 0) {

      memcpy (&compression_hdr, buf, sizeof (compression_hdr));

         /* Double the uncompressed length buffer size even though the
            uncompressed length is specified in the compression hdr - make
            sure we have enough space to uncompress the message */

      compressed_msg_len = msg_len;
      msg_len = ntohl (compression_hdr.uncompressed_len);
      uncompressed_buf_len = msg_len * 2;
      uncompressed_buf = malloc (uncompressed_buf_len);

      if (uncompressed_buf == NULL) {
         MA_abort("Read_sock () malloc() failed");
      }

         /* if a compression method is specified, then decompress the message */

      if ((compression_hdr.method == 0) || (compression_hdr.method == 1)) {

         buf_offset += sizeof (compression_hdr);
         compressed_msg_len -= buf_offset;
         msg_len -= buf_offset;

         ret = Decompress_msg (compression_hdr.method, buf + buf_offset,
                               compressed_msg_len, uncompressed_buf, 
                               uncompressed_buf_len);

         if ((ret <= 0) || (ret != msg_len)) {
            printf ("Msg decompression error (msg_len: %d;  ret: %d)\n", 
                    msg_len, ret);
            return (-1);
         }

            /* reassign *buf to the uncompressed data buffer */

         free (buf);   /* free the ptr to the compressed msg buffer */
         buf = uncompressed_buf;
         buf_offset = 0;
      }
    
         /* See if multiple products are embedded within this message */

      if (compression_hdr.multi_msg_flag) {

            /* extract the number of products in this message */

         memcpy (&tmp_int, buf + buf_offset, sizeof (int));
         number_packed_msgs = ntohl (tmp_int);
         buf_offset = sizeof (int);

            /* set the msg size ptr to the multi-msg size array */

         msg_size_ptr = buf + buf_offset + msg_len - 
                        (number_packed_msgs * sizeof (int));
         memcpy (&tmp_int, msg_size_ptr, sizeof (int));
         product_size = ntohl (tmp_int);
         msg_size_ptr += 4;   /* increment to the next product msg size */

      } else  /* there is only one product in this message */
         product_size = msg_len;
   }

      /* Process all the products received/embedded in this message */

   for (i = 1; i <= number_packed_msgs; i++) {
      PROC_process_msg (buf + buf_offset, product_size);
      buf_offset += product_size;

         /* Get the size of the next product to process */

      if (number_packed_msgs > 1) {
         memcpy (&tmp_int, msg_size_ptr, sizeof (int));
         product_size = ntohl (tmp_int);
         msg_size_ptr += 4;   /* increment to the next product msg size */
      }
   }

   free (buf);

      /* Send a msg Ack if required */

   if (pvc_ack != 0) {
      Tcp_hdr->type = htonl(DATAACK);
      Tcp_hdr->id = htonl(pvc_ack);
      Tcp_hdr->length = 0;
/*         printf("pvc ACK\n");*/
      if (SOC_sock_write(pvc_sd, Hd, TCP_HDR_LEN) != TCP_HDR_LEN) return (-1);
   }

   return (0);
}


/********************************************************************************

     Description: set TCP properites on the socket TCP proto, linger off, 
                  non-blocking, no short buffering

           Input: fd - descriptor of socket to set properties for

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

static int Set_tcp_properties (int fd)
{
    int i;
    struct linger lig;		/* parameter for setting SO_LINGER */
    int val;

       /* disable buffering of short data in TCP level */

    i = 1;
    errno = 0;

    if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, (char *)&i, 
        sizeof (int)) < 0) return (-1);

    /* turn off linger */

    lig.l_onoff = 0;
    lig.l_linger = 0;

    if (setsockopt (fd, SOL_SOCKET, SO_LINGER, (char *)&lig, 
        sizeof (struct linger)) < 0) return (-1);

       /* set non-block IO */

    if((val = fcntl(fd, F_GETFL, 0)) < 0) {
       close(fd);
       return (-1);
    }
    
    val |= O_NDELAY;

    if((fcntl(fd, F_SETFL, val)) < 0) {
       close (fd);
       return (-1);
    }  

    return (0);
} 


/******************************************************************************** 

     Description: Read data from a socket

           Input: fd     - socket descriptor to read data from
                  buffer - msg buffer to read data to
                  size   - size of the msg buffer

          Output:

          Return: number of bytes read or -1 if socket is disconnected or error

 ********************************************************************************/

static int Sock_read (int fd, char *buffer, int size) 
{
    int nread;   /* number of bytes read */
    int k;

    nread = 0;

    while (1) {

       errno = 0;
       if ((k = read (fd, &buffer[nread], size - nread)) == 0)
       {
         fprintf(stderr, "Socket disconnected\n");
           return (-1);   /* socket disconnected */
       }
	     
       if (k < 0) {
          if (errno == ECONNRESET) return (-1);
       else if (errno == EWOULDBLOCK || errno == EAGAIN) {
          if (nread == 0) return (0);
             poll (0,0,500);
             continue;
          }
	    else if (errno != EINTR) {
             return (-1);  /* no fatal error - caller will reopen socket */
          }
          continue;
       }

       nread += k;
/*       printf("read port %d, bytes = %d, size= %d, read = %d\n",fd, k, size, nread);*/
       if (nread >= size) return (nread);
    }
}
