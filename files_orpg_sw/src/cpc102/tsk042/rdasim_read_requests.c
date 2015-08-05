/******************************************************************

   file: rdasim_read_request.c

   Description: This file contains the comm manager routines that 
                read the requests from the rpg.
   
******************************************************************/

/* 
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2004/01/21 22:58:41 $
 * $Id: rdasim_read_requests.c,v 1.5 2004/01/21 22:58:41 garyg Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <rdasim_simulator.h>
#include <rdasim_externals.h>


/* local functions */

static int  Check_request (CM_req_struct *req);
static void Rm_done_requests ();


/**************************************************************************

    Description: This function reads all new requests from the request LB.
                 The requests are sorted in terms of their priorities. If a 
                 request can not be read because the "req" array is full, this 
                 and all subsequent requests are left untouched in the LB. 
                 The CM_CANCEL request is processed here. If a read error occurs, 
                 the program will terminate.

    Inputs:      

    Return:      It returns the total number of requests read.

**************************************************************************/

void RR_get_requests ()
{
   int req_cnt;  /* total number of requests read */

   req_cnt = 0;

   while (1) {
      Req_struct req;   /* request message structure */
      CM_req_struct *cmreq;
      int ind, ret;

      cmreq = (CM_req_struct *)&(req.type);
      ret = LB_read (LINk.reqfd, (char *)cmreq, 
                      sizeof (CM_req_struct), LB_NEXT);

      if (ret == LB_TO_COME)
          break;

      if (ret < 0) {
         if (ret == LB_BUF_TOO_SMALL) {
            if (req.type != CM_WRITE || req.data_size <= 0) {
               fprintf (stderr, "LB_read incorrect request length\n");
               continue;
            }
         } else {
            fprintf (stderr, "LB_read request msg failed (ret = %d)\n", ret);
            MA_terminate ();
         }
      }

         /* check for a valid link index */

      ind = req.link_ind;
      if (ind != LINk.link_ind) {
         fprintf (stderr, "request with wrong link index (req.link_ind: %d;   LINk.link_ind: %d) - ignored\n", 
                  ind, LINk.link_ind);
         continue;
      }

      req.state = CM_NEW;
      req.data = NULL;
      req.msg_id = LB_previous_msgid (LINk.reqfd);

         /* check the request parameters */

      if (Check_request (cmreq) != 0) {
         PR_send_response (&req, CM_INVALID_PARAMETER);
         continue;
      }

         /* process CM_CANCEL */

      if (req.type == CM_CANCEL) {
         PR_process_cancel (&req);
         continue;
      }

         /* remove all finished requests */

      Rm_done_requests ();

         /* check if there is room for the request */

      if (LINk.n_reqs >= MAX_N_REQS) {

            /* too many pending requests */

         PR_send_response (&req, CM_TOO_MANY_REQUESTS);
         continue;
      }

         /* read the full message */

      if (req.type == CM_WRITE) {
         char *buf;
         int size, read_size;
         int offset;       /* offset for boundary alignment */

         size = sizeof (CM_req_struct) + req.data_size;
         read_size = size;

         if (LINk.n_added_bytes > 0)
             size += MA_align_bytes (LINk.n_added_bytes);

         buf = malloc (size);
         
         if (buf == NULL) {
            fprintf (stderr, "malloc failed (size = %d)\n", size);
            MA_terminate ();
         }

         LB_seek (LINk.reqfd, -1, LB_CURRENT, NULL);

         offset = MA_align_bytes (LINk.n_added_bytes) -
                  LINk.n_added_bytes;
          
         ret = LB_read (LINk.reqfd, buf + offset, read_size, LB_NEXT);

         if (ret != read_size) {
            fprintf (stderr, "Bad request (type CM_WRITE) - ignored\n");
            continue;
         }

         req.data = buf;

         if (LINk.n_added_bytes != 0)
             memcpy (buf, cmreq, sizeof (CM_req_struct));
      }

         /* put the request in the list */

      memcpy (&(LINk.req[(LINk.st_ind + LINk.n_reqs) % MAX_N_REQS]), 
              &req, sizeof (Req_struct));

      LINk.n_reqs++;
      req_cnt++;
   }
   return;
}


/**************************************************************************

    Description: This function checks whether or not parameters in a 
                 request are legal.

    Inputs:      req - the request to be checked;

    Return:      returns 0 if no error is found or -1 is an error is found.

**************************************************************************/

static int Check_request (CM_req_struct *req)
{
   if (req->type < CM_CONNECT || req->type >= CM_DATA)
       return (-1);

   if (req->type == CM_WRITE) {
      if (req->parm != 1 || req->data_size <= 0)
          return (-1);

      if (req->data_size > LINk.packet_size)
          return (-1);
   }
   return (0);
}


/**************************************************************************

    Description: This function removes all requests in the request list that
                 are marked as "CM_DONE".

    Inputs:      

**************************************************************************/

static void Rm_done_requests ()
{
   int ind, i, cnt;

      /* remove those that do not need copy */

   cnt = 0;
   ind = LINk.st_ind - 1;

   for (i = 0; i < LINk.n_reqs; i++) {
      Req_struct *req;

      ind = (ind + 1) % MAX_N_REQS;
      req = &(LINk.req[ind]);
      if (req->state == CM_DONE)       /* done */
          cnt++;
      else
          break;
   }

   LINk.st_ind = (LINk.st_ind + cnt) % MAX_N_REQS;
   LINk.n_reqs -= cnt;

       /* remove those that need copy */

   ind = LINk.st_ind - 1;

   for (i = 0; i < LINk.n_reqs; i++) {
      Req_struct *req;

      ind = (ind + 1) % MAX_N_REQS;
      req = &(LINk.req[ind]);

      if (req->state == CM_DONE)  {   /* done */
         int rm_ind, k;
   
             /* remove req from the list */
         rm_ind = ind;

         for (k = 1; k < LINk.n_reqs - i; k++) {
            memcpy (&(LINk.req[rm_ind]), 
                    &(LINk.req[(rm_ind + 1) % MAX_N_REQS]), 
                    sizeof (Req_struct));
            rm_ind = (rm_ind + 1) % MAX_N_REQS;
         }
       
         LINk.n_reqs--;
         i--;
      }
   }
   return;
}
