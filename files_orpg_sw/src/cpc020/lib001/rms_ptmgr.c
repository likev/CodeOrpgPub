/**************************************************************************

   Module:  rms_ptmgr_main.c

   Description:
   This is the main module for the terminal emulator to
   establish asynchronous communication.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 18:34:15 $
 * $Id: rms_ptmgr.c,v 1.33 2005/12/27 18:34:15 steves Exp $
 * $Revision: 1.33 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <rms_message.h>
#include <rms_util.h>
#include <rms_ptmgr.h>

#include <orpg.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define _POSIX_SOURCE   1
#define BUFFSIZE        256
#define ACK_MSB         32768

/* Task Level Globals */
extern int    errno;
extern int in_lbfd;             /* Input LB id */
extern int rms_connection_down; /* Flag to indicate whether the interface is down or up */
extern int out_lbfd;            /* Output LB id */
       char   ttyport[80];      /* Pathname for the port being opened */
       ushort reject_flag = 0;  /* Resaon msg rejected from being sent to the FAA/RMMS */

/*
* Static Globals
*/

static int   port_fd = -1;        /* I/O descriptor for the terminal port */
static int   Reset_comm_port = 0; /* flag to reset the serial comm port */
static pid_t pid;                 /* PID of the child port manager */

/* Static Function Prototypes */

static int comm_init ();
static int reinit_comm_port (int buffer);

/**************************************************************************
   Notes:  Incoming messages are written to the input linear buffer and an
   event posted to activate the interface routine to handle the message type.
   Outgoing messages are written to the ouput linear buffer by the interface
   routine and an event posted that calls the port manager send routine.

   **************************************************************************/


/**************************************************************************
   Description:  This function initializes the comm port.

   Input:    None

   Output:  None

   Returns: Initialized = 1, Not initialized = -1

   Notes:

   **************************************************************************/
int comm_init () {

   struct termios tty_attr;   /* Terminal structure containing the initialization info */

   /* Initialize comm port */

   /* Put in code to access CS function to read configuration files.
     Part of configuration is to get input and output LB names.  For later development!  */

   /* Open the port for reading and writing */
   port_fd = open(ttyport, O_RDWR | O_NOCTTY | O_NONBLOCK);

   /* Unable to open port return error code */
   if (port_fd == -1) {
      LE_send_msg (RMS_LE_ERROR, "Error = %s", strerror(errno));
      LE_send_msg (RMS_LE_ERROR, "Unable to open port (%s)", ttyport);
      return (-1);
   } /* End if */
   else{
      LE_send_msg(RMS_LE_LOG_MSG,"%s port opened (fd: %d)", ttyport, port_fd);
   } /* End else */

   /* Get the current attributes of the port */
   if (tcgetattr(port_fd, &tty_attr) != 0 ) {
      LE_send_msg (RMS_LE_ERROR, "Unable to get port settings\n");
      return (-1);
   } /* End if */

   tty_attr.c_cc[VMIN] = 18; /* Wake up after minimum number of characters arrive */
   tty_attr.c_cc[VTIME] = 1; /* Wake up after minimum number of seconds ( 1 = 0.01 ) */
   tty_attr.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | ISTRIP | INPCK | INLCR | 
                         IGNCR |ICRNL | IXON | IXOFF); /* Input modes */
   tty_attr.c_iflag |= IGNBRK;    /* Input modes */
   tty_attr.c_oflag &= ~(OPOST); /* Output modes */
   tty_attr.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | ISIG | 
                         NOFLSH | TOSTOP | IEXTEN); /* Line discipline modes */
   tty_attr.c_cflag &= ~(CSTOPB | HUPCL);
   tty_attr.c_cflag &= (CSIZE); /* Setup options */
   tty_attr.c_cflag |= CREAD | CLOCAL | CS8;    /* Setup options */

   /* Set input and output speeds */
   if (cfsetispeed (&tty_attr, B9600) == -1) {
      LE_send_msg (RMS_LE_ERROR, "Unable to set input speed to %d \n", 9600);
      return (-1);
   } /* End if */

   if (cfsetospeed (&tty_attr, B9600) == -1) {
      LE_send_msg (RMS_LE_ERROR, "Unable to set output speed to %d \n", 9600);
      return (-1);
   } /* End if */

   /* Throw away any noise */
   if (tcflush(port_fd, TCIFLUSH) == -1) {
      LE_send_msg (RMS_LE_ERROR, "Unable to flush channel\n");
      return (-1);
   } /* End if */

   /* Set terminal attributes */
   if (tcsetattr (port_fd, TCSANOW, &tty_attr) == -1) {
      LE_send_msg (RMS_LE_ERROR, "Unable to get set attributes\n");
      return (-1);
   } /* End if */

   return (0);

}  /* End init */


/********************************************************************************

   Description: This function re-initializes the comm port.

         Input: buffer - specifies which buffer to flush (input or output)

        Output: None

       Returns: 0 on success; -1 on error

 ********************************************************************************/

int reinit_comm_port (int buffer) {

   int ret;

   /* flush the buffers and close the comm port if it is open */
   if (port_fd >= 0) {
      tcflush (port_fd, buffer);

      if (buffer == TCIFLUSH)
         LE_send_msg (RMS_LE_LOG_MSG, "Comm Port read buffer flushed");
      else if (buffer == TCOFLUSH)
         LE_send_msg (RMS_LE_LOG_MSG, "Comm Port write buffer flushed");

      LE_send_msg (RMS_LE_LOG_MSG, "Closing comm port (fd: %d)", port_fd);

      close (port_fd);
   }

   /* open and re-initialize the comm port */
   if ((ret = comm_init ()) == 0)
      Reset_comm_port = 0;

   return (ret);
}

/**************************************************************************

   Description: This function reads the comm port.

         Input: None

        Output: Places the incomming message in the linear buffer and posts
                an event to call the interface for message handling.

        Return: None

       Notes: This routine is the forked child process that continuously 
              monitors the comm port for incoming messages.

   **************************************************************************/

void rms_listen() {

   UNSIGNED_BYTE buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE *buf_ptr;
   int           msg_size;   /* Message size */
   int           ret;
   int           eol_count;  /* End of line counter ( eol = 7F7F7F7F) */

   LB_NTF_control(LB_NTF_SIGNAL, SIGUSR2);

   /* Main routine to listen for incoming message at the port */
   while (1) {

      /* Get message */
      msg_size = 0;
      eol_count = 0;
      buf_ptr = buf;

      /* Read each byte as it comes across the port.  System set up for Non Canonical mode */

      do {
         /* reinitialize the comm port if required */
         if (Reset_comm_port || (port_fd < 0))  {
            if (reinit_comm_port (TCIFLUSH) == -1) {
               sleep (1);
               continue;
            }
            break;
         }

         /* Read one byte from the port */
         errno = 0;
         ret = read(port_fd, &buf_ptr[msg_size], 1);

         /* suspend if there is nothing to read */
         if (ret <= 0) {
            sleep (1);
            continue;
         } /* End if */

         /* If this is a 7F and the last byte was 7F then increment end of message */
         if (buf_ptr[msg_size] == 0x7f && buf_ptr[msg_size-1] == 0x7f)
            eol_count++;

         /* Increment the buffer counter by one byte */
         msg_size++;

      } while ( eol_count <3 && msg_size <= MAX_BUF_SIZE);

      /* If the message is bigger than the maximum buffer size then print error message */
      if (msg_size > MAX_BUF_SIZE)
         LE_send_msg (RMS_LE_ERROR, "Input buffer exceeded maximum size (size %d)\n", msg_size);

      /* Header is 18 bytes long so if message is less than 18 there is no reason to process
         the message and the message must be less than the maximum buffer size */
      if (msg_size > 18 && msg_size <= MAX_BUF_SIZE) {
         /* Write the message to the input LB */
         ret = LB_write (in_lbfd, (char*)buf, msg_size, LB_ANY);

         if ( ret < 0 || ret != msg_size ){
            LE_send_msg (RMS_LE_ERROR, 
                "Input LB write failed (ret %d message length %d)\n", ret, msg_size);
         } /* End if */
         else {
            /* EN_post for message received */
            if ((ret = EN_post (ORPGEVT_RMS_MSG_RECEIVED, NULL, 0, 0)) < 0)
                 LE_send_msg(RMS_LE_ERROR, 
                     "Failed to Post ORPGEVT_RMS_MSG_RECEIVED (%d).\n", ret );
         }/* End else */
      } /* End if statement */
   }  /* End while loop */
} /* End rms_listen */


/********************************************************************************

   Description: This function is the event notification callback routine that
                sets the global flag directing the process(es) to reset 
                the comm port(s)

         Input:  None

        Output:  Reset_comm_port - global flag to reset the comm port

        Return:

 ********************************************************************************/

void rms_reset_port (en_t evtcd, void *msg, size_t msglen) {
   Reset_comm_port = 1;
   return;
}


/**************************************************************************
   Description:  This function is the main process for the comm port manager.

   Input:    None

   Output:     None

   Returns:    Comm port running = 1, Comm port not running = -1

   Notes:
   **************************************************************************/

int rms_ptmgr (int argc, char *argv[]) {

   int ret;

   /* Start comm port */
   if (comm_init () < 0 ) {
      LE_send_msg (0, "Unable to initialize port\n");
      return (-1);
   }

   /* Call listen as a new process and save pid */
   if ((pid = fork()) == 0) {
      /* Initialize the Log Error buffers */
      if ((ret = ORPGMISC_init (argc, argv, 1000, 0, -1, -1)) < 0) {
         LE_send_msg (RMS_LE_ERROR, "ORPGMISC_init failed: %d\n", ret);
         ORPGTASK_exit (ORPGTASK_EXIT_FAILURE);
      }

      LE_send_msg (RMS_LE_LOG_MSG, "child ORPGMISC_init %s registered", argv[0]);

      /* Register child for event ORPGEVT_RESET_COMM_PORT */

      if (( ret = EN_register (ORPGEVT_RMS_RESET_COMM_PORT, (void *) rms_reset_port )) < 0 ) {
          LE_send_msg(RMS_LE_ERROR, 
                 "Failed to register child event ORPGEVT_RMS_RESET_COMM_PORT (ret: %d)", ret );
      } 
      else {
          LE_send_msg(RMS_LE_LOG_MSG, "child event ORPGEVT_RMS_RESET_COMM_PORT registered");
      }

      rms_listen (); /* call the child routine */
   }

   sleep(2); /* give child time to register to avoid contention */

   /* Set the shutdown flag to zero */

   rms_shutdown_flag = 0;

   return (1);

} /* End main */

/**************************************************************************
   Description:  This function writes to the comm port.

   Input:    Event driven process that reads the message from the linear
         buffer and writes it to the comm port.

   Output:     None

   Returns:    None

   Notes:     Called by ORPGEVT_RMS_SEND_MSG.

   **************************************************************************/

void ptmgr_send_msg() {

   UNSIGNED_BYTE *buf_ptr;
   UNSIGNED_BYTE buf[MAX_BUF_SIZE];
   sigset_t      set;
   int           out_count, ret;   /* Number of bytes being sent */
   int           i = 0;
   ushort        message_size = 0;
   ushort        msg_seq_num, msg_type, return_code, rpg_state;

   /* if the port needs resetting, or the comm port is not open, then
      re-initialize the comm port */

   if (Reset_comm_port || (port_fd < 0))  {
      if (reinit_comm_port (TCOFLUSH) == -1)
         return;
   }

   /* Read the output buffer message */
   do {
     out_count = LB_read (out_lbfd, (char *) &buf, MAX_BUF_SIZE, LB_NEXT);
   } while (out_count == LB_EXPIRED);  /* flush all the expired msgs */

   /* Set the pointer at the beginning of the message*/
   buf_ptr = buf;

   if (out_count !=  MAX_BUF_SIZE) {
      if (out_count != LB_TO_COME)
         LE_send_msg (RMS_LE_ERROR, "Output LB read failed ptmgr (ret %d)\n", out_count);
   } /* End if */
   else {

      /* Read the message size in halfwords and multiply by 2 to get number of bytes */
      message_size = (conv_ushrt(buf_ptr) * 2);

      /* Place pointer at sequence number */
      buf_ptr += PLUS_SHORT;
      buf_ptr += PLUS_SHORT;
      buf_ptr += PLUS_SHORT;

      /* Get message sequence number */
      msg_seq_num = conv_ushrt (buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* Get message message_type */
      msg_type = conv_ushrt (buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* If the interface is down and the message is not an rms up message then exit */
      if(rms_connection_down == 1 && msg_type != 39)
         return;

      /* If the orpg is in shutdown no need to send anything other than an rpg state message */
      if (rms_shutdown_flag ==1 && msg_type != 1){
           LE_send_msg (RMS_LE_ERROR, "Unable to send message type %d RMS in shutdown mode", msg_type);
           return;
      }/* End if */

      /* If this is an acknowledgement message */
      if(msg_type == 37){

         /* Set pointer to beginning of message */
         buf_ptr = buf;

         /*Place buffer pointer at the return code */
         buf_ptr += MESSAGE_START;
         buf_ptr += PLUS_SHORT;
         buf_ptr += PLUS_SHORT;

         /* Get message return code*/
         return_code = conv_ushrt (buf_ptr);
         buf_ptr += PLUS_SHORT;

         /* Clear MSB to get sequence number of message being acknowledged */
         msg_seq_num &= ~ACK_MSB;

         LE_send_msg(RMS_LE_LOG_MSG,"Ack sent for RMMS message number %d.", msg_seq_num);

         if(return_code != 0){
            LE_send_msg(RMS_LE_LOG_MSG,"Acknowledgement return code =  %u.", return_code);
         } /* End if */

      } /* End if */

      /* if message is not rms up or acknowledgement message */
      else if (msg_type < 39){
            LE_send_msg(RMS_LE_LOG_MSG,"ORPG message %u sent (type %d).\n", msg_seq_num, msg_type);
      } /* End else */

      /* Fill the signal block set */
      sigfillset(&set);

      /* Block all signals except the kill signal */
      sigprocmask(SIG_BLOCK, &set,NULL);

      /* Write the outgoing message to the comm port */
      i = 0;

      do{
         /* Write message to the port byte by byte */
         write(port_fd,(unsigned char*) &buf[i],1);
         i++;

      } while (i < message_size);

      /*Wait for message to be sent */
      ret = tcdrain(port_fd);

      /* If message not sent print error message */
      if ( ret != 0)
         LE_send_msg (RMS_LE_ERROR, "Tcdrain interrupted Error = %s", strerror(errno));

      /* Unblock the signals */
      sigprocmask(SIG_UNBLOCK, &set,NULL);

      /* Check to see if the orpg is in shutdown */
      if (rms_shutdown_flag == 1){
         /* Check to see if this is an RPG state message */
         if(msg_type == 1){
            /* Set pointer to beginning of message */
            buf_ptr = buf;

            /*Place buffer pointer at the return code */
            buf_ptr += MESSAGE_START;

            /* Get message rpg state */
            rpg_state = conv_ushrt (buf_ptr);

            /* If the state is shutdown */
            if(rpg_state == 8){
               /* Clear the shutdown flag */
               rms_shutdown_flag = 0;

               /* Set the exit flag */
               Ev_exit = 1;

               /* Send the confirmation message to the LE buffer */
               LE_send_msg(RMS_LE_LOG_MSG,"RMS interface shutdown message sent.");
            }/* End if */
         }/* End if */
      }/* End if */
   } /* End else */
}/* End ptmgr_send_msg */

/**************************************************************************
   Description:  Kills the forked listen process upon shutdown.

   Input:    None

   Output: None

   Returns: Listen killed = 1, Listen not killed = -1

   Notes:     None

   **************************************************************************/
int rms_close_ptmgr() {

   /* Kill the forked listen routine */
   if ( pid == 0 )
      LE_send_msg(RMS_LE_LOG_MSG,
          "Port Manager listen routine not found (pid %d).\n", pid);
   else
      kill(pid, 1);

   return (0);
} /* End rms close ptmgr */
