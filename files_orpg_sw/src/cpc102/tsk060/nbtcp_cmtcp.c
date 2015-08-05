/*********************************************************************************

      File: nbtcp_cmtcp.c
            This file contains the routines needed to use the cm_tcp comm
            manager.

 ********************************************************************************/


/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/04/21 14:16:23 $
 * $Id: nbtcp_cmtcp.c,v 1.2 2008/04/21 14:16:23 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include <lb.h>
#include <nbtcp.h>

#define COMMS_CONFIG_FILE   "/comms_link.conf\0"
#define TCP_CONFIG_FILE     "/tcp.conf\0"
#define REQUEST_QUE_SIZE     8


static int Req_fd  = -1;   /* request LB file descriptor */
static int Resp_fd = -1;   /* response LB file descriptor */
static char Log_buf[256];  /* tmp buffer used to construct log msgs */
static CM_req_struct *Req_que[REQUEST_QUE_SIZE];

static int  Add_request_to_que (CM_req_struct *cm_req);
static void Flush_request_que (void);
static int  Get_instance_number (int link_num);
static void Initialize_request_que ();
static int  Interrogate_response (void *buf, int msg_len);
static void Process_event (CM_resp_struct *event);
static void Process_exception (CM_resp_struct *resp_hdr);
static void Remove_msg_from_req_que (CM_resp_struct *hdr);


/********************************************************************************

 ********************************************************************************/
 
 int CMT_create_lbs (int link_num) {

   char request_lb [128];
   char response_lb [128];
   LB_attr lb_attr;
   int lb_flags;
   char *home_dir;
   char lb_dir [MAX_NAME_LEN];
   char buf[256];
   int  instance_num;

   
   if ((instance_num = Get_instance_number (link_num)) == -1) {
      fprintf (stdout, 
        "Error reading comm manager number from configuration file for line %d\n",
        link_num);
      return (-1);
   }

   if ((home_dir = getenv ("HOME")) == NULL) {
      printf ("Error reading \"HOME\" env variable. Please set this env variable \n");
      printf ("and re-run the tool\n");
      return (-1);
   }

   strcpy (lb_dir, home_dir);
   strcat (lb_dir, "/nbtcp\0");

      /* create the directory if it does not exist */

   if (opendir (lb_dir) == NULL) {
       printf ("Creating Linear Buffer directory \"%s\"\n",
               lb_dir);

      if ((mkdir (lb_dir, S_ISUID | S_ISGID | S_ISVTX | S_IRWXU |
                          S_IRWXG | S_IROTH | S_IXOTH)) == -1) {
           perror ("Error creating Linear Buffer directory -- ");
           return (-1);
      }
   }

      /* populate the LB attributes structure */

   strncpy (lb_attr.remark, "Request LB for nbtcp", LB_REMARK_LENGTH);
   lb_attr.remark [LB_REMARK_LENGTH - 1] = '\0';
   lb_attr.msg_size = 0;  /* force dynamic msg sizing */
   lb_attr.mode = 0666;
   lb_attr.maxn_msgs = 100;
   lb_attr.types = 0;
   
      /* assign some nra size so that LB Notification services 
         are available...these services are not currently being used */

   lb_attr.tag_size = LB_DEFAULT_NRS << NRA_SIZE_SHIFT;

   lb_flags = LB_WRITE;

      /* Open the request LB or create the LB if it does not exist */

   sprintf(request_lb, "%s/%s%d", lb_dir, "req.", instance_num);

   if ((Req_fd = LB_open (request_lb, lb_flags, &lb_attr)) == LB_OPEN_FAILED) {

      lb_flags = LB_CREATE;

      if ((Req_fd = LB_open (request_lb, lb_flags, &lb_attr)) < 0) {
         sprintf (buf, "%s \"%s\" (err: %d errno: %d)", "LB_open failed for", 
                  request_lb, Req_fd, errno);
         MA_printlog (buf);
         printf ("LB_open failed for \"%s\" (err: %d;   errno: %d)\n",
                 request_lb, Req_fd, errno);
         return (-1);
      }
   }

   sprintf (buf, "LB %s opened (fd: %d)", request_lb, Req_fd);
   MA_printlog (buf);

      /* Open the response LB or create the LB if it does not exist */

   strncpy (lb_attr.remark, "Response LB for nbtcp", LB_REMARK_LENGTH);
   lb_attr.remark [LB_REMARK_LENGTH - 1] = '\0';

   lb_flags = LB_READ;

   sprintf(response_lb, "%s/%s%d", lb_dir, "resp.", link_num);

   if ((Resp_fd = LB_open (response_lb, lb_flags, &lb_attr)) == LB_OPEN_FAILED) {

      lb_flags = LB_CREATE;

      if ((Resp_fd = LB_open (response_lb, lb_flags, &lb_attr)) < 0) {
         sprintf (buf, "%s \"%s\" (err: %d errno: %d)", "LB_open failed for", 
                  response_lb, Resp_fd, errno);
         MA_printlog (buf);
         printf ("LB_open failed for \"%s\" (err: %d;   errno: %d)\n",
                 response_lb, Resp_fd, errno);
         return (-1);
      }
         /* now close the lb and re-open it as read only */

      if (LB_close (Resp_fd) < 0) {
         printf ("Error initializing response LB \"%s\" (err: %d)\n",
                 response_lb, Resp_fd);
         return (-1);
      }

      lb_flags = LB_READ;

      if ((Resp_fd = LB_open (response_lb, lb_flags, &lb_attr)) < 0) {
         sprintf (buf, "%s \"%s\" (err: %d errno: %d)", "LB_open failed for", 
                  response_lb, Resp_fd, errno);
         MA_printlog (buf);
         printf ("LB_open failed for \"%s\" (err: %d;   errno: %d)\n",
                 response_lb, Resp_fd, errno);
         return (-1);
      }
   }

   sprintf (buf, "LB %s opened (fd: %d)", response_lb, Resp_fd);
   MA_printlog (buf);

      /* initialize the request queue */
      
   Initialize_request_que ();

   return (0);
}


/********************************************************************************

 ********************************************************************************/
int CMT_lbs_created () {

   return ((Req_fd > 0) && (Resp_fd > 0));
}


/********************************************************************************

 ********************************************************************************/

int CMT_read (int link_num) {

   int ret;
   char **msg = NULL;

   while ((ret = LB_read (Resp_fd, &msg, LB_ALLOC_BUF, LB_NEXT)) >= 0) {
      CM_resp_struct *hdr = (CM_resp_struct *) msg;

         /* interrogate only the responses for this link */

      if (hdr->link_ind == link_num)
         Interrogate_response (msg, ret);
   }

   if (ret == LB_TO_COME)
      return (0);
   else
      return (ret);
}


/********************************************************************************

 ********************************************************************************/

int CMT_write (int cm_type, int link_num, void *msg, int msg_len) {

   CM_req_struct *cm_hdr;
   int ret;
   static int seq_num = 0;
   char *msg_buf;
   int  msg_size;

   if (msg == NULL) {  /* this is a cm_tcp msg only */
      msg_size = sizeof (CM_req_struct);

      if ((msg_buf = (char *) malloc(msg_size)) == NULL) {
        perror ("malloc failed");
        MA_abort ("CM_write(): malloc failed");
      }
   } else {  /* allocate space in msg for the cm_tcp hdr */

      msg_size = msg_len + sizeof (CM_req_struct);

      if ((msg_buf = (char *) malloc(msg_size)) == NULL) {
        perror ("malloc failed");
        MA_abort ("CM_write(): malloc failed");
      }

         /* copy the msg to the new buffer */

      memcpy (msg_buf + sizeof(CM_req_struct), msg, msg_len);
   }

   cm_hdr = (CM_req_struct *) msg_buf;

   cm_hdr->type = cm_type;
   cm_hdr->req_num = seq_num++;
   cm_hdr->link_ind = link_num;
   cm_hdr->time = (uint) (time (NULL));
   cm_hdr->parm = 1;  /* always send data on PVC 1 */

   if (msg == NULL)
      cm_hdr->data_size = 0;
   else
      cm_hdr->data_size = msg_len;

      /* add request to request queue...all writes are comm mgr requests */

   if (Add_request_to_que (cm_hdr) < 0) {
      printf ("Error adding msg to the request queue\n");
      free (msg_buf);
      return (-1);
   }

/* printf ("LB_write of cm->type: %d\n", cm_hdr->type); */

      /* there's a major problem if the LB write fails */

   if ((ret = LB_write (Req_fd, (char *) msg_buf, msg_size, LB_ANY)) < 0) {
      sprintf (Log_buf, "%s %d%s", "LB_write error occurred (err: ", ret, ")");
      MA_abort (Log_buf);
   } 
   
   free (msg_buf);

   return (ret);
}


/********************************************************************************

 ********************************************************************************/

 static int Add_request_to_que (CM_req_struct *cm_req) {

    int i;
    char *buf;

    for (i = 0; i < REQUEST_QUE_SIZE; i++) {
       if (Req_que[i] == NULL) 
          break;
    }   
    
    if (i == REQUEST_QUE_SIZE) {
       printf ("Error: the request message queue is full, can not send message\n");
       Flush_request_que ();
       return (-1);
    }

    if ((buf = malloc (sizeof (CM_req_struct))) == NULL) {
       printf ("malloc failed (errno: %d)\n", errno);
       MA_abort ("Add_request_to_que(): malloc failed\n");
    }

    memcpy (buf, cm_req, sizeof (CM_req_struct));
    Req_que[i] = (CM_req_struct *) buf;

    return (0);
}


/********************************************************************************

 ********************************************************************************/

void Flush_request_que (void) {
   int index;

   printf ("Flushing the request queue\n");

   for (index = 0; index < REQUEST_QUE_SIZE; index++) {
      if (Req_que[index] != NULL) {
         free (Req_que[index]);
         Req_que[index] = NULL; 
      }
   }

   return;
}


/********************************************************************************

 ********************************************************************************/

static int Get_instance_number (int link_num) {

   char *cm_dir;
   char config_file [MAX_NAME_LEN];
   FILE *file;
   char buf[256];

   if ((cm_dir = getenv ("CFG_DIR")) == NULL) {
      printf ("\n\nThe $CFG_DIR environment variable is not setup in your\n");
      printf ("environment. This env variable is used by cm_tcp to specify\n");
      printf ("the location of the comms configuration files. Tool nbtcp\n");
      printf ("also uses this env variable to locate the configuration\n");
      printf ("files. Please set the CFG_DIR env variable and re-run\n");
      printf ("the tool\n\n");

      return (-1);
   }

   strcpy (config_file, cm_dir);
   strcat (config_file, COMMS_CONFIG_FILE);

   if ((file = fopen (config_file, "rt")) == NULL) {
       fprintf (stderr, "Error opening configuration file %s (err: %d)\n", 
                config_file, errno);
       return (-1);
   }

      /* parse the config file looking for the link number */

   while (!feof(file)) {
      if ((fgets (buf, 255, file)) == NULL) {
         fprintf (stderr, "Error reading file (errno: %d)\n", errno);
         fclose (file);
         break;
      } else {
         char tmp1[100];
         char tmp2[100];
         int first_arg;
         int instance_num;

         sscanf (buf, "%s", tmp1);
         first_arg = atoi(tmp1);

            /* see if we found the link entries section */

         if ((first_arg != 0) || (first_arg == 0 && (strcmp(tmp1, "0\0") == 0))) {
            if (first_arg == link_num) {
               sscanf (buf, "%s %s %d", tmp1, tmp2, &instance_num);
               return (instance_num);
            }
         }
      }
   }

      /* if we made it here, the link number wasn't found */

   return(-1);
}


/********************************************************************************

 ********************************************************************************/

 static void Initialize_request_que () {

   int i;

   for (i = 0; i <= REQUEST_QUE_SIZE; i++) {
      Req_que[i] = NULL;
   }
   return;
}

/********************************************************************************

 ********************************************************************************/

static int Interrogate_response (void *buf, int msg_len) {

   CM_resp_struct *hdr;

   hdr = (CM_resp_struct *) buf;
/* printf ("response type: %d\n", hdr->type); */
   switch (hdr->type) {
  
     case CM_CONNECT:
     case CM_DISCONNECT:
     case CM_WRITE:
         switch (hdr->ret_code) {
            case CM_SUCCESS:
              if ((hdr->type == CM_CONNECT) || (hdr->type == CM_DISCONNECT)) {
                 PROC_new_conn ();
                 MA_update_connection_state (hdr->type);
              }
            break;

            case CM_TIMED_OUT:
            case CM_IN_PROCESSING:
            case CM_TERMINATED:
            case CM_DISCONNECTED:
            case CM_FAILED:
            case CM_INVALID_PARAMETER:

            default: /* process everything else as an exception */
               Process_exception (hdr);
            break;
         }

         Remove_msg_from_req_que (hdr);
     break;

     case CM_DATA:
        PROC_process_msg (buf + sizeof (CM_resp_struct), 
                          msg_len - sizeof(CM_resp_struct));
     case CM_EVENT:
        Process_event (hdr);
   }

   return (0);
}


/********************************************************************************

 ********************************************************************************/

 void Process_event (CM_resp_struct *event) {

   switch (event->ret_code) {
          /* we don't care about these events */
       case CM_STATISTICS:
       case CM_STATUS:
       break;

       case CM_START:
          printf ("\nThe instance of cm_tcp servicing this link has started\n");
       break;

       default: /* process all other events as exceptions */
          Process_exception (event);
/* printf ("event->return_code: %d\n", event->ret_code); */
       break;
   }
   return;
}


/********************************************************************************

 ********************************************************************************/

void Process_exception (CM_resp_struct *resp_hdr) {

   switch (resp_hdr->ret_code) {
         /* we don't care about these exceptions */
      case CM_IN_PROCESSING:
      case CM_TERMINATED:
      break;

      case CM_TIMED_OUT:
         if (resp_hdr->type == CM_WRITE)
            printf ("cm_tcp timed out reading/writing last msg...discard this msg\n");
      break;

      case CM_DISCONNECTED:
      case CM_LOST_CONN:
         if (MA_get_connection_state () == CM_CONNECT) {
            printf ("Unsolicited disconnect has occurred\n");
            MA_update_connection_state (CM_DISCONNECT);
         }

            /* delete any partially received msgs if the products are being
               saved to disk */

         PROC_process_msg (NULL, -1);

            /* flush the request queue */

         Flush_request_que ();

      break;

      case CM_TERMINATE:
         printf ("\nThe instance of cm_tcp servicing this link has terminated\n");

            /* delete any partially received products */

         PROC_process_msg (NULL, -1);
         MA_update_connection_state (CM_DISCONNECT);
      break;

      case CM_FAILED:
         printf ("cm_tcp request msg failed (CM_FAILED returned)...reason: unknown\n");
      break;

      case CM_INVALID_PARAMETER:
         printf ("Invalid parameter passed to cm_tcp in the request msg\n");
      break;
   }

   return;
}


/********************************************************************************

 ********************************************************************************/

void Remove_msg_from_req_que (CM_resp_struct *hdr) {
   int index;

   for (index = 0; index < REQUEST_QUE_SIZE; index++) {
      if (Req_que[index] != NULL)
         if (hdr->req_num == Req_que[index]->req_num)
           break;
   }
   if (index == REQUEST_QUE_SIZE) {  /* this shouldn't happen */
      printf ("Error: couldn't find request in request queue\n");
      return;
   }

   free ((char *)Req_que[index]);
   Req_que[index] = NULL; 
    
}
