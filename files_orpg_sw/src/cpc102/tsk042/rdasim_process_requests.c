/******************************************************************

        file: rdasim_process_requests.c

        This module contains the comm manager functionality that 
        processes requests & messages to/from the rpg.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/16 18:44:54 $
 * $Id: rdasim_process_requests.c,v 1.9 2007/03/16 18:44:54 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <orpgevt.h>
#include <rdasim_simulator.h>
#include <rdasim_externals.h>


static int Cr_time;             /* current time */

static int N_req_processed = 0; /* number of requests processed */


/* local functions */

static void Process_connect (int ind);
static void Process_disconnect (int ind);
static int  Pending_request (int type, int ind);
static void Process_status (int ind);
static void Process_write (int ind);
static int  Search_req ();
static void Send_event_response (int ret_code);


/**************************************************************************

    Description: This routine is called when the link is disconnected. It
                 terminates all unfinished write requests and discards all 
                 incomplete incoming data.

    Inputs:      

**************************************************************************/

void PR_disconnect_cleanup ()
{
    int ind;


    while ((ind = Pending_request (CM_WRITE, -1)) >= 0) {
        if (LINk.req[ind].state > CM_DONE)
            PR_send_response (&(LINk.req[ind]), CM_DISCONNECTED);
        LINk.req[ind].state = CM_DONE;
    }

    LINk.r_cnt = 0;

    return;
}


/**************************************************************************

    Description: This routine processes the CM_CANCEL request. It removes
                 those requests that match req->parm and are not yet being 
                 processed. The cancelled requests are removed from the "req" 
                 list.

    Inputs:      new_req - the CM_CANCEL request;

**************************************************************************/

void PR_process_cancel (Req_struct *new_req)
{
    int ind, i, cnt;

    cnt = 0;
    ind = LINk.st_ind - 1;

    for (i = 0; i < LINk.n_reqs; i++) {
        Req_struct *req;  /* request message structure */

        ind = (ind + 1) % MAX_N_REQS;
        req = &(LINk.req[ind]);

        if ((new_req->parm == -1 || new_req->parm == req->req_num) &&
             req->state == CM_NEW)  {    /* match the req number */ 
                                         /* not started */
            int rm_ind,  /* index of the request to remove */
                k;

            if (req->type == CM_WRITE) { /* free the data buffer */
                LINk.w_buf = NULL;

                if (req->data != NULL) {
                    free (req->data);
                    req->data = NULL;
                }
            }
        
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
            cnt++;
        }
    }
       /* send a response */

    PR_send_response (new_req, cnt);

    return;
}


/**************************************************************************

    Description: This routine processes lost connections and other
                 exception conditions.

    Inputs:      

**************************************************************************/

void PR_process_exception ()
{
    int i;  /* pending request index */


    if (LINk.conn_activity == NO_ACTIVITY &&
                LINk.link_state == LINK_CONNECTED)
         Send_event_response (CM_LOST_CONN);

    LINk.link_state = LINK_DISCONNECTED;

       /* stop all pending connect request */

    while ((i = Pending_request (CM_CONNECT, -1)) >= 0)
            PR_send_response (&(LINk.req[i]), CM_TERMINATED);

    PR_disconnect_cleanup ();

       /* flag the request currently being processed as "failed" */

    if (LINk.conn_activity != NO_ACTIVITY)
        PR_send_response (&(LINk.req[(int)LINk.conn_req_ind]), 
                          CM_FAILED);

    LINk.conn_activity = NO_ACTIVITY;

    return;
}

/**************************************************************************

    Description: This routine processes all current and pending requests.

    Input:       

    Output:

    Return:

**************************************************************************/

void PR_process_requests ()
{
    Req_struct *req;  /* request message structure */
    int cnt, ind;

    cnt = 0;
    Cr_time = time (NULL);

    if ((ind = Search_req ()) < 0)
         return;

    cnt++;
    req = &(LINk.req[ind]);

    switch (req->type) {
        case CM_CONNECT:
            Process_connect (ind);
            break;

        case CM_DIAL_OUT:
            PR_send_response (req, CM_NOT_CONFIGURED);
            break;

        case CM_DISCONNECT:
            Process_disconnect (ind);
            break;

        case CM_WRITE:
            Process_write (ind);
            break;

        case CM_STATUS:
            Process_status (ind);
            break;

        default:
            break;
    }

    return;
}


/**************************************************************************

    Description: This routine writes messages to the rpg via the
                 response linear buffer.

    Inputs:      data - pointer to the data;
                 len - data length;

**************************************************************************/

#ifndef LINUX
#ifndef GCC 
#pragma align 4 (data_buffer)   /* align the data buffer on a 4 byte boundary */
#endif
#endif

void PR_send_data (char *data, int message_type, int len)
{
    CM_resp_struct *resp;    /* the comm manager response structure */
    int  cm_header_length = sizeof (CM_resp_struct);
    int  ctm_header_length = sizeof (CTM_header_t);
    int  total_message_length;  /* total message length written to the LB */
    char data_buffer [MAX_BUFFER_SIZE];
                             /* data buffer to construct the message in */
    char *cpt;               /* message data buffer pointer */
    int  ret;                /* length of message in bytes returned by 
                                LB_write */

       /* construct the comm manager response header */

    resp = (CM_resp_struct *)(data_buffer);

    resp->type = CM_DATA;
    resp->req_num = LINk.r_seq_num;
    LINk.r_seq_num++;
    resp->link_ind = LINk.link_ind;
    resp->ret_code = 1;
    resp->data_size = len + ctm_header_length;
    resp->time = time (NULL);

    if (message_type == GENERIC_DIGITAL_RADAR_DATA){

       if( len > MAX_MESSAGE_31_SIZE){
           fprintf (stderr, "message length (len = %d) received > MAX_MESSAGE_31_SIZE (size = %d)",
                    len, MAX_MESSAGE_31_SIZE);
           fprintf (stderr, "         message discarded\n");
           return;
       }

    }
    else if (len > MAX_MESSAGE_SIZE) {
       fprintf (stderr, "message length (len = %d) received > MAX_MESSAGE_SIZE (size = %d)",
                len, MAX_MESSAGE_SIZE);
       fprintf (stderr, "         message discarded\n");
       return;
    }

    total_message_length = cm_header_length + ctm_header_length + len;

    if( message_type == GENERIC_DIGITAL_RADAR_DATA ){

        Generic_basedata_t *gbt = (Generic_basedata_t *) data;

        len = SHORT_BSWAP_L( gbt->msg_hdr.size ) * sizeof(short);

        /* Compress the data if it needs compressing. */
        if( COmpress_radials ){

           static char *dest = NULL;
           static int dest_len = 0;

           if( (dest == NULL)   
                     ||
               (len > dest_len) )
              dest = realloc( dest, len );

           if( dest != NULL ){

              int src_len, cmp_len;
              int offset = SHORT_BSWAP_L( gbt->base.no_of_datum ) * sizeof(int);
              char *src = (char *) data + sizeof(Generic_basedata_t) + offset;

              src_len = len - offset;
              cmp_len = MISC_compress( MISC_BZIP2, src, src_len, dest, dest_len );

              if( cmp_len > 0 ){

                 memcpy( src, dest, cmp_len );

                 if( cmp_len % sizeof(short) )
                    cmp_len++;

                 len = cmp_len + offset;
                 gbt->msg_hdr.size = SHORT_BSWAP_L( len/sizeof(short) );

              }
                 
           }

        } 

        total_message_length = cm_header_length + ctm_header_length + len;

       /* copy the data to the data buffer compensating for the comm manager
          and CTM headers and the number of data fields available. */

       cpt = data_buffer;
       memcpy (cpt + cm_header_length + ctm_header_length, data, len);

       ret = LB_write (LINk.respfd, cpt, total_message_length, LB_NEXT);

    }
    else{

       /* copy the data to the data buffer compensating for the comm manager
          and CTM headers */

       cpt = data_buffer;
       memcpy (cpt + cm_header_length + ctm_header_length, data, len);

       ret = LB_write (LINk.respfd, cpt, total_message_length, LB_NEXT);

    }

    if (ret != total_message_length) {
        fprintf (stderr, "LB_write (send data) failed (ret = %d)\n", ret);
        MA_terminate ();
    }

/*    if (LINk.data_en)
        EN_post (ORPGEVT_NB_COMM_RESP + LINk.link_ind, NULL, 0, 1); */

    return;
}


/**************************************************************************

    Description: This routine forms a response message and sends it to the 
                 rpg. It frees the allocated memory segments for CM_WRITE 
                 requests. It also sets the req->state to CM_DONE.

    Inputs:      req - the original request;
                 ret_code - the return code;

**************************************************************************/

void PR_send_response (Req_struct *req, int ret_code)
{
    CM_resp_struct resp;    /* response data structure */
    int ret;

       /* clean up the resources allocated for this request */

    if (req->type == CM_WRITE) {
        if (req->data != NULL)
            free (req->data);
        req->data = NULL;
    }

    if ((req->type == CM_CONNECT || req->type == CM_DISCONNECT) &&
         ret_code != CM_IN_PROCESSING)
        LINk.conn_activity = NO_ACTIVITY;

       /* send the response message */

    resp.type = req->type;
    resp.req_num = req->req_num;
    resp.link_ind = req->link_ind;
    resp.ret_code = ret_code;
    resp.data_size = 0;
    resp.time = time (NULL);

    ret = LB_write (LINk.respfd, (char *)&resp, sizeof (CM_resp_struct), 
                    LB_NEXT);

    if (ret != sizeof (CM_resp_struct)) {
        fprintf (stderr, "LB_write (response) failed (ret = %d)\n", ret);
        MA_terminate ();
    }

/*    EN_post (ORPGEVT_NB_COMM_RESP + LINk.link_ind, NULL, 0, 1); */

    req->state = CM_DONE;
    N_req_processed++;

    return;
}


/**************************************************************************

    Description: This routine searches for any pending request of "type"
                 on "link" received before request req["ind"]. If ind = -1, 
                 all requests are searched.

    Inputs:      ind - link->req[ind] is the request in processing;

    Return:      The index in array link->req of the pending  request of 
                 "type" if it is found, or -1 if not found.

**************************************************************************/

static int Pending_request (int type, int ind)
{
    int req_ind, i;

    req_ind = LINk.st_ind - 1;

    for (i = 0; i < LINk.n_reqs; i++) {
        Req_struct *req;

        req_ind = (req_ind + 1) % MAX_N_REQS;
        if (req_ind == ind)
            break;

        req = &(LINk.req[req_ind]);
        if (req->type == type && req->state != CM_DONE)
            return (req_ind);
    }

    return (-1);
}


/**************************************************************************

    Description: This routine processes the wideband "CONNECT" request.

    Inputs:      ind - link->req[ind] is the CM_CONNECT request;

    Note:        There can only be one on-going connect/disconnect request
                 under processing. link->conn_activity indicates the
                 current connection activity: CONNECTING, DISCONNECTING
                 or NO_ACTIVITY. link->req[link->conn_req_ind] is the
                 current connect/disconnect request under processing.

**************************************************************************/

static void Process_connect (int ind)
{
    Req_struct *req;   /* the request Linear Buffer */

    req = &(LINk.req[ind]);

    if (LINk.conn_activity == CONNECTING ||
        LINk.conn_activity == DISCONNECTING) {
        PR_send_response (req, CM_IN_PROCESSING);
        return;
    }

    if (LINk.link_state == LINK_CONNECTED) {
        PR_send_response (req, CM_SUCCESS);
        return;
    }

    req->state = Cr_time;
    LINk.link_state = LINK_DISCONNECTED;
    LINk.conn_activity = CONNECTING;
    LINk.conn_req_ind = ind;

    RD_connect_link();

    return;
}


/**************************************************************************

    Description: This routine processes the wideband "DISCONNECT" request.

    Inputs:      ind - link->req[ind] is the CM_DISCONNECT request;

    Note:        Refer to Note in Process_connect.

**************************************************************************/

static void Process_disconnect (int ind)
{
    Req_struct *req;     /* the request Linear Buffer */
    int i;

    req = &(LINk.req[ind]);

       /* stop all pending connect request */

    while ((i = Pending_request (CM_CONNECT, ind)) >= 0)
        PR_send_response (&(LINk.req[i]), CM_TERMINATED);

    if (LINk.conn_activity != NO_ACTIVITY) {
        PR_send_response (req, CM_IN_PROCESSING);
        return;
    }

    if (LINk.link_state == LINK_DISCONNECTED) {
        PR_send_response (req, CM_SUCCESS);
        return;
    }

    req->state = Cr_time;
    LINk.conn_activity = DISCONNECTING;
    LINk.conn_req_ind = ind;

    RD_disconnect_link();

    return;
}


/**************************************************************************

    Description: This routine returns the current state of the wideband link
                 to the rpg.

    Inputs:      ind - link->req[ind] is the CM_STATUS request;

**************************************************************************/

static void Process_status (int ind)
{
    Req_struct *req;

    req = &(LINk.req[ind]);

    if (LINk.link_state == LINK_CONNECTED)
        PR_send_response (req, CM_CONNECTED);
    else 
        PR_send_response (req, CM_DISCONNECTED);

    return;
}


/**************************************************************************

    Description: This routine writes messages to the RDA.

    Inputs:      ind - link->req[ind] is the CM_WRITE request;

**************************************************************************/

static void Process_write (int ind)
{
    Req_struct *req;
    char *data_buffer;


    req = &(LINk.req[ind]);

       /* make sure there is no unfinished write */

    if (LINk.w_buf != NULL)
        return;

    data_buffer = malloc(req->data_size + sizeof(CM_req_struct));

    if (data_buffer == NULL) {
       fprintf (stderr, "Process_write: malloc failed\n");
       MA_terminate();
    }
    else
       memcpy ((char *)data_buffer, req->data, req->data_size + sizeof(CM_req_struct));

    LINk.w_buf = data_buffer + sizeof (CM_req_struct);

    if (LINk.n_added_bytes != 0)
        LINk.w_buf += MA_align_bytes (LINk.n_added_bytes);

    LINk.w_size = req->data_size - LINk.n_added_bytes;
    LINk.w_cnt = 0;
    LINk.w_req_ind = ind;

    req->state = Cr_time;

      /* send a comm mgr response and process the RPG msg if the
         link is connected */

    if (LINk.link_state != LINK_CONNECTED)
        PR_send_response (&(LINk.req[(int)LINk.w_req_ind]), CM_DISCONNECTED);
    else {
        PR_send_response (&(LINk.req[(int)LINk.w_req_ind]), 
                         CM_SUCCESS);
        RRM_process_rpg_msg ();
    }

    free (data_buffer);
    LINk.w_buf = NULL;

    return;
}


/**************************************************************************

    Description: This routine searches for the next request to be 
                 processed in the "req" list.

    Inputs:      

    Return:      Returns the index in "req" of the request to be processed
                 or -1 if there is nothing to be processed.

**************************************************************************/

static int Search_req ()
{
    Req_struct *req;
    int i, 
        ind,      /* index to the first non-write request */
        wind;     /* index to the first write request */

    if (LINk.n_reqs <= 0)
        return (-1);

    req = LINk.req;
    ind = LINk.st_ind - 1;
    wind = -1;

    for (i = 0; i < LINk.n_reqs; i++)  {  /* go through all requests */
        ind = (ind + 1) % MAX_N_REQS;

        if (req[ind].state == CM_NEW) {
            if (req[ind].type != CM_WRITE)
                return (ind);        /* return the first non-write request */
            else {
                if (wind < 0)
                    wind = ind;
            }
        }
    }

    return (wind);
}


/**************************************************************************

    Description: This routine sends an event of type "ret_code" to the
                 rpg

    Inputs:      ret_code - event code.

**************************************************************************/

static void Send_event_response (int ret_code)
{
    CM_resp_struct resp; /* response message data structure */
    int ret;             /* length of message in bytes returned by LB_write */


       /* send the response message */

    resp.type = CM_EVENT;
    resp.link_ind = LINk.link_ind;
    resp.ret_code = ret_code;
    resp.time = time (NULL);

    ret = LB_write (LINk.respfd, (char *)&resp, 
                                sizeof (CM_resp_struct), LB_NEXT);
    if (ret != sizeof (CM_resp_struct)) {
        fprintf (stderr, "LB_write (response) failed (ret = %d)\n", ret);
        MA_terminate ();
    }

/*    EN_post (ORPGEVT_NB_COMM_RESP + LINk.link_ind, NULL, 0, 1); */

    return;
}
