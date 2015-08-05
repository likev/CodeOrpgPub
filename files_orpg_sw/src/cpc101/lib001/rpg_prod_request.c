
/***********************************************************************

	This module contains functions supporting product request access.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/07/19 22:34:00 $
 * $Id: rpg_prod_request.c,v 1.37 2012/07/19 22:34:00 steves Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 */  



#include <rpg.h>
#include <orpgadpt.h>


#define MAX_SLOTS 10            /* max number of requests for an elevation index and 
                                   product ID combination. */

typedef struct {		/* struct for request check list */

    int type;			/* product type number */
    int timing;			/* product timing */
    int *requested;		/* whether or not requested (TRUE/FALSE) */
    int in_or_out;              /* Input or Output */
    int n_requests;             /* number of requests in buffer that follows. */
    Prod_request *request;	/* product request information for this product. */

} Prod_check_list;

#define INPUT_DATA              1
#define OUTPUT_DATA             2

extern int Task_type;

static Prod_check_list Rc_list [MAXN_OUTS + MAXN_INPS];
				/* request check list - includes
				   all outputs */

static Prod_request *Replay_req = NULL;
				/* Replay Request buffer. */

static Prod_request *Outstanding_replay_req = NULL;
                                /* Outstanding Replay Request buffer. */

static int N_replay_req = 0; 	/* Number of replay requests. */

static int N_outstanding_replay_req = 0; 	
                                /* Number of outstanding replay requests. */

static int N_rc = 0;		/* number of items in Rc_list */

static Out_data_type *Out_list = NULL;	
				/* output data type list */

static int N_outs = 0;		/* number of outputs */

static In_data_type  *Inp_list = NULL;	
				/* input data type list */

static int N_inps = 0;		/* number of outputs */

static int Prq_lbid = -1;	/* data id of the product request LB */

static LB_check_list Cklist [MAXN_OUTS + MAXN_INPS];
				/* message status check list */

static LB_status Prq_status;	/* product request msg update status */

static int Input_stream = PGM_REALTIME_STREAM;
				/* Task's input data stream. */

static unsigned int Vol_seq = 0;
                                /* Volume scan sequence number. */

static int Vs_num = 0;          /* Volume scan number. */

#define MAX_VOLS                2

/* The Req_buf stores the complete product request message as read from
   ORPGDAT_PROD_REQUESTS.

   Rc_list also holds request information, but for elevation based 
   products, this information is likely only for a single elevation. */
static char *Req_buf [ MAX_VOLS] [MAXN_OUTS + MAXN_INPS] =  { { NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL },
                                                              { NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL,
                                                                NULL, NULL, NULL, NULL } };
                                /* the COMPLETE request messages */

static int Req_msg_len [MAX_VOLS] [MAXN_OUTS + MAXN_INPS] = { { 0, 0, 0, 0, 0, 0, 0, 0,
                                                                0, 0, 0, 0, 0, 0, 0, 0 },
                                                              { 0, 0, 0, 0, 0, 0, 0, 0,
                                                                0, 0, 0, 0, 0, 0, 0, 0 } };
                                /* length of the COMPLETE request messages */


/* local functions */
static int Read_product_requests( char **buf, int prod_id );
static int Get_replay_requests( char **buf, int prod_id );


/***********************************************************************

   Description: 
      This function returns the product requests sent to the 
      product "buf_type". The product request info is returned in the 
      array "uarray". This array is considered as a two dimensional
      (array of 10 shorts). All requests are read from the 
      prod_request LB. Only those that matches elevation index 
      "elev_index" are returned if "elev_index" >= 0 and elev_ind is 
      specified in the request (elev_ind >= 0). The product code 
      number "p_code" is put in the first element of the uarray

      For the realtime data stream, the product request LB is a 
      replaceable LB and the buffer type 
      numbers are used for identifying the msg containing the requests 
      for a product. If there is no request for a product the message may 
      not exist. The request list in a message is considered 
      terminated if the request pid is different from the LB message id.

      For the replay data stream, the product requests are read from
      a message queue.  As in the case of realtime products, the request
      list in a message is considered terminated if the request pid is
      different from the LB message id.

   Inputs:
      elev_index - the RPG elevation index to match; If 
	           elev_index < 0 or the elev_ind in the request is 
		   undefined (< 0), it matches all elevations.
      prod_id - the product identifier (product id)
      p_code - product code number used by old RPG;
      index - index for next request in uarray.  This value
              is 1 more than the total number of requests
              for the product;

   Outputs:	
      index - index for next request in uarray;
      uarray - array returning the requests;

   Return:
      The return value is meaningless; If the function fails
      because of a program error or a run time configuration
      error, it will call PS_task_abort to terminate the task.

************************************************************************/
int RPG_get_request (
    fint elev_index,	/* elevation index */
    fint prod_id,	/* product ID */
    fint p_code,	/* product code number */
    fint *index,	/* index for next request in uarray */
    fint2 *uarray	/* for returning the list of requests */ ) {

    char *buf;
    int num_reqs;

    buf = NULL;

    /* Read product requests. */
    if( Input_stream == PGM_REALTIME_STREAM )
       num_reqs = Read_product_requests( &buf, prod_id );

    else if( Input_stream == PGM_REPLAY_STREAM )
       num_reqs = Get_replay_requests( &buf, prod_id );

    else
       num_reqs = 0;

    /* The following function populates the user array and returns
       the number of requests. */
    PRQ_populate_user_array( elev_index, prod_id, p_code, buf, num_reqs,
                             index, uarray );

    /* Free buffer containing product requests. */
    if( buf != NULL )
       free( buf );

    return(0);

/* End of RPG_get_request(). */
}

/***********************************************************************

   Description:
      Takes request information as input and fills the User_array_t
      structure as output.

   Input:
      elev_index - product request elevation index
      prod_id - product ID
      p_code - product code
      buf - buffer containing product request information.
      num_reqs - number of requests in buf.

   Outputs:
      uarray - user array containing product request information 
      index - number of requests in uarray

   Returns:
      Number of requests in user array, uarray.

   Notes:
      If buf is NULL, num_reqs holds the aborted volume scan number.

***********************************************************************/
int PRQ_populate_user_array( int elev_index, int prod_id, int p_code, 
                             char *buf, int num_reqs, int *index, 
                             short *uarray ){

    Prod_request *pr = NULL;
    short *ua = NULL;
    int i, ind;

    /* If the passed buffer is NULL, use the Req_buf address as 
       the buffer. */
    if( buf == NULL ){

       /* Go through Rc_list trying to find match on product ID. */
       for( i = 0; i < N_rc; i++ ){

          if( prod_id == Rc_list[i].type ){

             /* Determine the index into Req_buf for the volume 
                scan number.  Note: When buf is passed in as NULL
                pointer, num_reqs passes the volume scan number. */
             ind = num_reqs % 2;

             /* Match found ... set buffer address and number of 
                requests. */
             buf = Req_buf[ind][i];
             num_reqs = Req_msg_len[ind][i];
             break;

          }

       }

    }

    /*  Get the appropriate requests. */
    pr = (Prod_request *) buf;
    ua = uarray + ((*index - 1) * PREQ_SIZE);
    for (i = 0; i < num_reqs; i++) {

        /* Check for product ID mismatch. */
	if (pr->pid != prod_id){

            /* If realtime input stream, break out of "for" loop.  Otherwise,
               continue processing all requests. */
            if( Input_stream != PGM_REALTIME_STREAM )
               continue;

            else
	       break;

        }

	if( (pr->elev_ind == elev_index) 
                          ||
            (elev_index < 0) 
                          || 
	    (pr->elev_ind == REQ_ALL_ELEVS) ){

            /* Only MAX_SLOTS number of products are allowed for this
               elev_ind and prod_id combination. */
            if( *index < (MAX_SLOTS+1) ){

               /* Update user array with request information. */
	       ua [PREQ_PID] = p_code;
	       ua [PREQ_WIN_AZI] = pr->param_1;
	       ua [PREQ_WIN_RANGE] = pr->param_2;
	       ua [PREQ_ELAZ] = pr->param_3;
	       ua [PREQ_STORM_SPEED] = pr->param_4;
	       ua [PREQ_STORM_DIR] = pr->param_5;
	       ua [PREQ_SPARE] = pr->param_6;
	       ua [PREQ_ELEV_IND] = pr->elev_ind;
	       ua [PREQ_REQ_NUM] = pr->req_num;	
	       ua [PREQ_RESERVED] = 1;	/* not used */

               /* Prepare for next request.  PREQ_SIZE fields in the array uarray */
	       ua += PREQ_SIZE;	
	       *index = *index + 1;

            }
            else
               AP_abort_request( pr, PGM_SLOT_UNAVAILABLE );

	}

        /* Prepare for next request. */
	pr++;

    /* End of "for" loop. */
    }
   
    return (0);

/* End of RPG_get_request() */
}

/***********************************************************************

   Description: 
      This function checks whether a product is requested to be generated.

   Inputs:
      data_type - the product identifier (buffer type number)

   Outputs:	
      user_array - product request data in the following format:
  
         user_array[0] = product_code
         user_array[1] = product dependent parm 1
         user_array[2] = product dependent parm 2
         user_array[3] = product dependent parm 3
         user_array[4] = product dependent parm 4
         user_array[5] = product dependent parm 5
         user_array[6] = product dependent parm 6
         user_array[7] = elevation index
         user_array[8] = request number
         user_array[9] = not used
  
      status - NORMAL indicating that "data_type" is requested or the 
               timing is specified as DEMAND_DATA; otherwise NOT_REQD 
               indicating it is not requested or not an output product 
               of this task.

   Return:
       The return value is not used.

   Notes:
       If Rc_list[].request points to multiple requests, the first 
       request is returned. 

************************************************************************/
int PRQ_check_data (int *data_type, short *ua, int *status){

    User_array_t *user_array = (User_array_t *) ua;
    short *parm;
    int i;

    /* Initialize the need for this product to be generated to NOT_REQD. */
    *status = NOT_REQD;

    /* Initialize the user_array elements. */
    memset( user_array, 0, sizeof(User_array_t) );

    /* Make all product dependent parameters initially unused. */
    parm = &user_array->ua_dep_parm_0;
    for( i = 0; i < UA_NUM_PARMS; i++ ){

       *parm = PARAM_UNUSED;
       parm++;

    }

    /* If there is a request for this product or the product timing is
       DEMAND_TYPE (which means a request is not necessary), return
       NORMAL. */
    for( i = 0; i < N_rc; i++){

	if( Rc_list[i].type == *data_type ){
 
	   if( (*(Rc_list[i].requested) == TRUE)
                          ||
               (Rc_list[i].timing == DEMAND_DATA) ){

	      *status = NORMAL;

              if( *(Rc_list[i].requested) == TRUE ){

                 if( Rc_list[i].request != NULL ){

                     user_array->ua_prod_code = ORPGPAT_get_code( Rc_list[i].request->pid ); 

                     user_array->ua_dep_parm_0 = Rc_list[i].request->param_1; 
                     user_array->ua_dep_parm_1 = Rc_list[i].request->param_2; 
                     user_array->ua_dep_parm_2 = Rc_list[i].request->param_3; 
                     user_array->ua_dep_parm_3 = Rc_list[i].request->param_4; 
                     user_array->ua_dep_parm_4 = Rc_list[i].request->param_5; 
                     user_array->ua_dep_parm_5 = Rc_list[i].request->param_6; 

                     user_array->ua_elev_index = Rc_list[i].request->elev_ind; 
                     user_array->ua_req_number = Rc_list[i].request->req_num; 

                 }

              }

	      return (0);

           }
           /* For the replay stream and warehoused data .....*/
           else if( (Input_stream ==  PGM_REPLAY_STREAM) 
                                  &&
                    (ORPGPAT_get_warehoused( *data_type ) > 0) ){

              /* If the data is an input, since it is replay stream and 
                 warehoused data, do not require a request.  If not 
                 warehoused data, we expect a request. */
              if( Rc_list[i].in_or_out == INPUT_DATA ){
           
                 if( Rc_list[i].request != NULL ){

                     user_array->ua_prod_code = ORPGPAT_get_code( Rc_list[i].request->pid ); 

                     user_array->ua_dep_parm_0 = Rc_list[i].request->param_1; 
                     user_array->ua_dep_parm_1 = Rc_list[i].request->param_2; 
                     user_array->ua_dep_parm_2 = Rc_list[i].request->param_3; 
                     user_array->ua_dep_parm_3 = Rc_list[i].request->param_4; 
                     user_array->ua_dep_parm_4 = Rc_list[i].request->param_5; 
                     user_array->ua_dep_parm_5 = Rc_list[i].request->param_6; 

                     user_array->ua_elev_index = Rc_list[i].request->elev_ind; 
                     user_array->ua_req_number = Rc_list[i].request->req_num; 

                 }

                 *status = NORMAL;
                 return(0);

              }

           }

	}

    }

    return (0);

/* End of PRQ_check_data() */
}

/***********************************************************************

   Description: 
      This function checks whether a product is requested to be generated.

   Inputs:
      data_type - the product identifier (buffer type number)
      user_array - product request data to check, in the following format:
  
         user_array[0] = product_code
         user_array[1] = product dependent parm 1
         user_array[2] = product dependent parm 2
         user_array[3] = product dependent parm 3
         user_array[4] = product dependent parm 4
         user_array[5] = product dependent parm 5
         user_array[6] = product dependent parm 6
         user_array[7] = elevation index
         user_array[8] = request number
         user_array[9] = not used
  
   Outputs:	
      status - NORMAL indicating that "data_type" is requested or the 
               timing is specified as DEMAND_DATA; otherwise NOT_REQD 
               indicating it is not requested or not an output product 
               of this task.

   Return:
       The return value is not used.

   Notes:

************************************************************************/
int PRQ_check_req (int *data_type, short *ua, int *status){

    User_array_t *user_array = (User_array_t *) ua;
    int i, j;

    /* Initialize the need for this product to be generated to NOT_REQD. */
    *status = NOT_REQD;

    /* If there is a request for this product or the product timing is
       DEMAND_TYPE (which means a request is not necessary), return
       NORMAL. */
    for( i = 0; i < N_rc; i++){

       if( Rc_list[i].type == *data_type ){
 
          if( Rc_list[i].timing == DEMAND_DATA ){

	     *status = NORMAL;
             return (0);

          }

          /* If the product is requested, ....... */
          else if( *(Rc_list[i].requested) == TRUE ){

             /* Go through all requests, trying to find match. */
             if( Rc_list[i].request != NULL ){

                Prod_request *temp_array = (Prod_request *) Rc_list[i].request;

                for( j = 0; j < Rc_list[i].n_requests; j++ ){

                   short prod_code = ORPGPAT_get_code( temp_array->pid ); 

                   if( (user_array->ua_prod_code == prod_code)
                                           && 
                       (user_array->ua_dep_parm_0 == temp_array->param_1)
                                           && 
                       (user_array->ua_dep_parm_1 == temp_array->param_2)
                                           &&
                       (user_array->ua_dep_parm_2 == temp_array->param_3) 
                                           &&
                       (user_array->ua_dep_parm_3 == temp_array->param_4) 
                                           &&
                       (user_array->ua_dep_parm_4 == temp_array->param_5) 
                                           &&
                       (user_array->ua_dep_parm_5 == temp_array->param_6) ){

                       *status = NORMAL;

                       return (0);

                   } 

                   /* Check the next product request. */
                   temp_array++;

                /* End of "for" loop. */
                }

                /* This should not happen ... if does, application programming error. */
                LE_send_msg( GL_INFO, "PRQ_check_req(): Request Not Found\n" );

             }

          }

       }

    }

    return (0);

/* End of PRQ_check_data() */
}

/***********************************************************************

   Description: 
      This function returns the product request data.

   Inputs:
      data_type - the product identifier (buffer type number)

   Outputs:	
      n_requests - stores the number of product requests in buffer
                   returned.

   Return:
       Pointer to product request data.

   Notes:

************************************************************************/
Prod_request* PRQ_get_prod_request( int data_type, int *n_requests ){

    int i;

    /* If there is a request for this product or the product timing is
       DEMAND_TYPE (which means a request is not necessary), return
       request data. */
    for( i = 0; i < N_rc; i++ ){

	if( Rc_list[i].type == data_type 
                       &&
	    (*(Rc_list[i].requested) == TRUE
                       ||
            Rc_list[i].timing == DEMAND_DATA) ){

           *n_requests = Rc_list[i].n_requests;
	   return ( Rc_list[i].request );

        }

    }

    *n_requests = 0;
    return (NULL);

/* End of PRQ_get_prod_request() */
}

/********************************************************************

   Description: 
      This function checks whether output products are requested and 
      updates the requested field of Out_data_type in the output list. 
      In the case of the real-time data stream, we update requests for 
      volume based products only at the beginning of a volume. This 
      function may be called repeatedly.  Thus we need to check the 
      previous update time to eliminate duplicated update.  For the
      replay data stream, we bypass the beginning of volume check.

   Input:	
      new_vol - 1: this is called at beginning of a volume;
		0: this is called at beginning of an elevation;
      elev_ind - current elevation index.
      time - the current data time;

   Outputs:

   Returns:	
      Returns 0 which is not used.

   Notes:

********************************************************************/
int PRQ_update_requests (int new_vol, int elev_ind, int time){

   static int update_time = 0;		/* previous update time */
   static int old_elev_ind = -1;

   int index, ret, i;

   /* If update time is same as last and input stream is the real-time
      input stream, just return.  Otherwise, update the update time. */
   if( (Input_stream == PGM_REALTIME_STREAM)
                &&
      ((update_time == time) 
                && 
       (elev_ind == old_elev_ind)) ){

      LE_send_msg( GL_INFO, "Request Data Not Updated !!!!\n" );
      return (0);

   }

   update_time = time;
   old_elev_ind = elev_ind;

   /* For realtime input stream, check the request LB. */
   if( Input_stream == PGM_REALTIME_STREAM ){

      /* Find out if messages in LB have changed. */
      if ((ret = ORPGDA_stat (Prq_lbid, &Prq_status)) != LB_SUCCESS)
         PS_task_abort ("ORPGDA_stat (Product Request LB) Failed (%d)\n", ret);

   }

   /* Derive the index that will be used in the rest of this module. */
   index = Vs_num % 2;

   /* Update product request information. */
   for (i = 0; i < N_rc; i++) {

      int msg_st;

      /* For realtime input stream only ... */
      if( Input_stream == PGM_REALTIME_STREAM ){

         /* Do not read product request information if new volume scan
            has not occurred and either:

            1) output timing is VOLUME_DATA or ELEVATION_DATA. */
         if (new_vol == 0){

            if( (Rc_list[i].timing == VOLUME_DATA) 
                               ||
                (Rc_list[i].timing == ELEVATION_DATA) )
            continue;		/* we update every volume */

         }

         msg_st = Cklist[i].status;

         /* Check if particular LB message has been update.  If updated
            or new volume, re-read requests. */
         if( (msg_st == LB_MSG_UPDATED)
                     ||
                  (new_vol) ){

            /* Free the previously allocated buffer. */
            if( Req_buf[index][i] != NULL ){

               free( Req_buf[index][i] );
               Req_buf[index][i] = NULL;

            }

            /* Requests updated ... read real-time product requests for 
               this product. */
            Req_msg_len[index][i] = Read_product_requests( &Req_buf[index][i], Rc_list[i].type );
	 
         }
         else if (msg_st == LB_MSG_NOT_FOUND)
            Req_msg_len[index][i] = 0;

         /* If there is no request message for this product, it is always
            requested.  DEMAND_DATA is also always requested. */
         if ( (Req_msg_len[index][i] <= 0) || (Rc_list[i].timing == DEMAND_DATA) ) {

            *(Rc_list[i].requested) = TRUE;

            if( Rc_list[i].request != NULL )
               free( Rc_list[i].request );

            Rc_list[i].request = NULL;
            Rc_list[i].n_requests = 0;
	    continue;

         }

      }
      else if( Input_stream == PGM_REPLAY_STREAM ){

         /* Free the previously allocated buffer. */
         if( Req_buf[index][i] != NULL ){

            free( Req_buf[index][i] );
            Req_buf[index][i] = NULL;

         }

         /* Get the replay product requests for this product. */
         Req_msg_len [index][i] = Get_replay_requests( &Req_buf[index][i], Rc_list[i].type );

      }

   }

   /* Update product request information. */
   for (i = 0; i < N_rc; i++) {

      int k;
      Prod_request *pr;

      /* Initialially set all requests to not requested. */
      *(Rc_list[i].requested) = FALSE;

      if( Rc_list[i].request != NULL )
         free( Rc_list[i].request );

      Rc_list[i].request = NULL;
      Rc_list[i].n_requests = 0;

      /* Go through all requests in this product request message. */
      pr = (Prod_request *) Req_buf[index][i];

      for (k = 0; k < Req_msg_len[index][i]; k++) {

         /* Check for end of product requests. */
	 if (pr->pid != Rc_list[i].type)
	    break;

         /* For elevation based outputs, must match elevation index
            or elevation index must be REQ_ALL_ELEVS for product request. */
	 if (Rc_list[i].timing == ELEVATION_DATA) {

	    if (pr->elev_ind == elev_ind || pr->elev_ind == REQ_ALL_ELEVS) {

               /* Set the "requested" flag to TRUE */
	       *(Rc_list[i].requested) = TRUE;

               /* If the "request" buffer is NULL, allocate storage. */ 
               if( Rc_list[i].request == NULL ){

                  /* Allocate space for product requests. */
                  Rc_list[i].request = (Prod_request *) calloc( 1, 
                                        Req_msg_len[index][i]*sizeof(Prod_request) );
                  if( Rc_list[i].request == NULL )
                     PS_task_abort( "calloc Failed in PRQ_update_requests For %d Bytes\n",
                                    Req_msg_len[index][i]*sizeof(Prod_request) );

                  Rc_list[i].n_requests = 0;

               }

	       memcpy( (Rc_list[i].request + Rc_list[i].n_requests), pr,
                       sizeof(Prod_request) );

               Rc_list[i].n_requests++;

	    }

	 }
	 else {

            /* For other output timings, elevation index must be REQ_ALL_ELEVS. */
	    if (pr->elev_ind == REQ_ALL_ELEVS) {

	       *(Rc_list[i].requested) = TRUE;

               /* If the "request" buffer is NULL, allocate storage. */ 
               if( Rc_list[i].request == NULL ){

                  /* Allocate space for product requests. */
                  Rc_list[i].request = (Prod_request *) calloc( 1, 
                                       Req_msg_len[index][i]*sizeof(Prod_request) );
                  if( Rc_list[i].request == NULL )
                     PS_task_abort( "calloc Failed in PRQ_update_requests For %d Bytes\n",
                                    Req_msg_len[index][i]*sizeof(Prod_request) );

                  Rc_list[i].n_requests = 0;

               }

	       memcpy( (Rc_list[i].request + Rc_list[i].n_requests), pr,
                       sizeof(Prod_request) );

               Rc_list[i].n_requests++;

	    }

	 }

         /* Prepare for next product request. */
	 pr++;

      }

   }

   return (0);

/* End of PRQ_update_requests () */
}

/********************************************************************

   Description: 
      This function initializes this module.

   Inputs:

   Outputs:

   Returns:
      There is no return value define for this function.

   Notes:	
      This function must be called before any other functions in 
      this module can be called.

********************************************************************/
void PRQ_initialize (){
 
    int ret, i;

    /* Set the task's input stream. */
    Input_stream = INIT_get_task_input_stream();

    /* Set the product request LB file data ID. */
    if( Input_stream == PGM_REALTIME_STREAM )
       Prq_lbid = ORPGDAT_PROD_REQUESTS;

    else
       Prq_lbid = ORPGDAT_REPLAY_REQUESTS;

    /* Open the LB to set the read pointer.  This is very important
       since if this is not done, the read pointer is set the first
       time it is accessed.  This may cause problems with replay requests. */
    if( Input_stream == PGM_REPLAY_STREAM ){

       if( (ret = ORPGDA_lbfd( Prq_lbid )) < 0 )
          LE_send_msg( GL_INFO, "ORPGDA_lbfd Returned Error (%d)\n", ret );

    }

    /* Set up the message status check list */
    N_outs = OB_out_list (&Out_list);
    N_inps = IB_inp_list (&Inp_list);
    N_rc = 0;

    /* Do for all outputs. */
    for (i = 0; i < N_outs; i++) {
	Rc_list [N_rc].type = Out_list[i].type;
	Rc_list [N_rc].timing = Out_list[i].timing;
        Rc_list [N_rc].in_or_out = OUTPUT_DATA;
	Rc_list [N_rc].requested = &(Out_list[i].requested);
        Rc_list [N_rc].n_requests = 0;
        Rc_list [N_rc].request = NULL;
	N_rc++;
    }

    /* Do for all inputs. */
    for (i = 0; i < N_inps; i++) {
	Rc_list [N_rc].type = Inp_list[i].type;
	Rc_list [N_rc].timing = Inp_list[i].timing;
        Rc_list [N_rc].in_or_out = INPUT_DATA;
	Rc_list [N_rc].requested = &(Inp_list[i].requested);
        Rc_list [N_rc].n_requests = 0;
        Rc_list [N_rc].request = NULL;
	N_rc++;
    }

    /* Do for all outputs and inputs. */
    for (i = 0; i < N_rc; i++)
	Cklist[i].id = Rc_list[i].type;

    /* Product request message update status */
    Prq_status.attr = NULL;
    Prq_status.n_check = N_rc;
    Prq_status.check_list = Cklist;

    /* Read the requests initially */
    PRQ_update_requests (1, 0, 1);

/* End of PRQ_initialize () */
}

#define UNDEFINED_ELEV		-999

/***********************************************************************
   Description:
      This module takes an input product request list and determines
      the number of requests which can be satisfied by this task.  
      Any requests which can be satisfied by this task are saved in
      Replay_req.
      
   Inputs:
      req - pointer to Prod_request structure containing product 
            requests.
      length - length in bytes, of the product request list.
      number_requests - pointer to int to receive the number of
                        product requests which can be satisfied by
                        this task.

   Outputs:
      number_requests - pointer to int receiving the number of 
                        product requests which can be satisfied by this
                        task.

   Returns:
      Returns pointer to Replay_req.

   Notes:
      Task processing terminates on a bad product request list.

***********************************************************************/
Prod_request* PRQ_replay_product_requests( Prod_request *req, int length,
                                           int *number_requests ){

   int i, j, req_elev_param, requested_elev;
   int target_elev, process_now, num_reqs;
   Prod_request *tmp_req = NULL, *prod_request = NULL;
   short *params = NULL;

   /* Ensure that Out_list is defined.  This should never happen. */
   if( Out_list == NULL )
      PS_task_abort( "Out_list Is Not Defined\n" );

   /* If the one-time request list is not NULL, free it now. */
   if( Replay_req != NULL ){

      free( Replay_req );
      Replay_req = NULL;

   }
   
   /* Set the number of replay requests to 0. */
   N_replay_req = 0;

   /* If the one-time request list is not NULL, free it now. */
   if( Outstanding_replay_req != NULL ){

      free( Outstanding_replay_req );
      Outstanding_replay_req = NULL;

   }
   
   /* Set the number of outstanding replay requests to 0. */
   N_outstanding_replay_req = 0;
   
   if( (length % sizeof(Prod_request)) != 0 )
      PS_task_abort( "Bad One-Time Prod Request Msg (%d Not Multiple of %d)\n",
                      length, sizeof(Prod_request) );

   /* Determine the number of requests in this request message. */
   num_reqs = length / sizeof(Prod_request);

   /* No requests found. */
   if( num_reqs == 0 ){

      *number_requests = 0;
      return NULL;

   }

   /* Allocate space for one-time requests.  */
   Replay_req = (Prod_request *) calloc( 1, (unsigned int) length );
   if( Replay_req == NULL )
      PS_task_abort( "calloc Failed for %d Bytes\n", length ); 

   prod_request = Replay_req;

   tmp_req = req;
   target_elev = UNDEFINED_ELEV;

   /* Do For All requests .... */
   for( i = 0; i < num_reqs; i++ ){

      requested_elev = UNDEFINED_ELEV;

      /* If this product has an elevation parameter, save it for future comparison. */
      if( (req_elev_param = ORPGPAT_elevation_based( tmp_req->pid )) >= 0 ){

         /* The "target_elev" is the elevation parameter for the first request.
            All other requests must match this request in order to be serviced
            together. */
         if( target_elev == UNDEFINED_ELEV ){

            params = &tmp_req->param_1;

            /* Get the elevation parameter. */
            target_elev = params[req_elev_param];

         }

         /* Get the requested elevation parameter. */
         params = &tmp_req->param_1;
         requested_elev = params[req_elev_param];

      }

      process_now = 0;
      for( j = 0; j < N_outs; j++ ){

         /* Does the product ID match? */
         if( tmp_req->pid == Out_list[j].type ){

            /* If this is ELEVATION_DATA, all elevations must match. */
            if( Out_list[j].timing == ELEVATION_DATA ){

               if( (target_elev == UNDEFINED_ELEV) 
                                ||
                   ((target_elev != UNDEFINED_ELEV)
                                &&
                    (requested_elev == target_elev)) )
                  process_now = 1;

            }
            else
               process_now = 1;

            if( process_now ){

               /* Transfer data to One-time request structure. */
               memcpy( prod_request, tmp_req, sizeof( Prod_request ) );

               /* Increment the number of requests and prepare for next request. */
               N_replay_req++;
               prod_request++;

               /* Break out of inner "for" loop. */
               break;

            }
            else{

               /* Must save this request.  Add it to the list of outstanding requests. */
               if( Outstanding_replay_req == NULL ){

                  Outstanding_replay_req = (Prod_request *) calloc( 1, sizeof(Prod_request) );
                  if( Outstanding_replay_req == NULL ){

                     LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                                  sizeof(Prod_request) );
                     ORPGTASK_exit(GL_MEMORY);

                  }

                  N_outstanding_replay_req = 1;
                  memcpy( Outstanding_replay_req, tmp_req, sizeof(Prod_request) );

               }
               else{

                  Prod_request *req = Outstanding_replay_req + N_outstanding_replay_req;

                  Outstanding_replay_req = (Prod_request *) realloc( Outstanding_replay_req,
                                                (N_outstanding_replay_req+1)*sizeof(Prod_request) );
                  req = Outstanding_replay_req + N_outstanding_replay_req;

                  memcpy( req, tmp_req, sizeof(Prod_request) );
                  N_outstanding_replay_req++;

               }

               /* Tell the operator all about it. */
               LE_send_msg( GL_INFO, "Can Not Service Prod %d Request At This Time\n",
                            req->pid );
               LE_send_msg( GL_INFO, "--->Target Elev: %d != Requested Elev: %d\n",
                            target_elev, requested_elev );
               LE_send_msg( GL_INFO, "--->Add To Outstanding Replay Reqs (Size: %d)\n",
                            N_outstanding_replay_req );

               /* Break out of inner "for" loop. */
               break;

            }

         }

      /* Check the next output for match. */
      }

      /* Go to next request. */
      tmp_req++;

   }

   *number_requests = N_replay_req;
   return( Replay_req );

/* End of PRQ_replay_product_requests() */
}

/****************************************************************************
   Description:
      This module reads the replay request LB to see if there are any
      outstanding requests.

   Inputs:
      num_reqs - pointer to int to receive the number of requests in
                 the replay request just read.

   Outputs:
      num_reqs - pointer to int receiving the number of replay requests.

   Returns:
      Pointer to the start of the replay requests.

   Notes:
      If the ORPGDA_read fails, the task terminates.

****************************************************************************/
Prod_request* PRQ_check_for_replay_requests( int *num_reqs ){

   int len;
   char *buf;
   Prod_request *requests = NULL;

   while( 1 ){

      /* If there were some replay requests which were previously read but
         could not be immediately serviced, then process these first. */
      if( N_outstanding_replay_req == 0 ){

         /* Initialize the number of requests to 0. */
         *num_reqs = 0;

         /* Read the replay (one-time) product request LB for a product
            request. */
         buf = NULL;
         len = ORPGDA_read( ORPGDAT_REPLAY_REQUESTS, &buf, LB_ALLOC_BUF,
                            LB_NEXT );

         /* If some abnormal error occurred, terminate processing. */
         if( len < 0 && len != LB_TO_COME ){

            /* If this replay task waits for data to activate versus waits for
               request, then ignore the LB_EXPIRED error return.  This error
               can occur because the request LB is only read when data is 
               available, not when a (or any) request is available. */
            if( (len == LB_EXPIRED) && (Inp_list[DRIVING].wait_for_data) )
               continue;

            PS_task_abort( "Replay Product Request LB Read Failed (%d)\n",
                           len );

         }

         /* If no new requests in Request LB, sleep a short period before 
            re-reading if we are to wait for a request (not wait for data). */ 
         if( len == LB_TO_COME ){

            if( ES_event_registered( EVT_CFCPROD_REPLAY_PRODUCT_REQUEST )
                                   ||
                Inp_list[DRIVING].wait_for_data ){
   
               /* Set the number of requests to 0 and NULL the request
                  pointer when returning. */
               *num_reqs = 0;
               return (NULL);

            }
            else{

               /* Service END_OF_VOLUME event if it has been received. */
               INIT_process_eov_event();

               sleep(2);
               continue;

            }

         }

      }
      else{

         LE_send_msg( GL_INFO, "There are %d Outstanding Requests ....\n",
                      N_outstanding_replay_req );

         /* Make copy of outstanding request list. */
         len = N_outstanding_replay_req*sizeof(Prod_request);
         buf = (char *) calloc( 1, len );
         if( buf == NULL ){

            LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n", len );
            ORPGTASK_exit(GL_MEMORY);

         }

         memcpy( (void *) buf, (void *) Outstanding_replay_req, len );

      }
     
      /* There are product requests outstanding.  Check if match on 
         any product that this task produces. */
      requests = PRQ_replay_product_requests( (Prod_request *) buf, 
                                              len, num_reqs );

      /* Free the buffer allocated by ORPGDA_read if all requests processed. */
      if( buf != NULL ){

         free( buf);
         buf = NULL;

      }

      /* Return the number of requests and the request buffer pointer. */
      if( *num_reqs > 0 ){

         Prod_request *req;
         int i;

         LE_send_msg( GL_INFO, "Replay Request Message Has %d Requests\n", 
                      *num_reqs );

         req = requests;
         for( i = 0; i < *num_reqs; i++ ){

            LE_send_msg( GL_INFO, "--->Request For Product ID %d For Vol Seq # %d, Elev Ind %d\n",
                         req->pid, req->vol_seq_num, req->elev_ind );
            LE_send_msg( GL_INFO, "------> Parameters: %d %d %d %d %d %d\n",
                         req->param_1, req->param_2, req->param_3,
                         req->param_4, req->param_5, req->param_6 );

            req++;

         }

         return( requests );

      }

   /* End of "while" loop. */
   }

   /* Set the default return values. */
   *num_reqs = 0;
   return( NULL );

/* End of PRQ_check_for_replay_requests() */
}

/*****************************************************************

   Description:
      Set the volume sequence number .... 

*****************************************************************/
void PRQ_set_vol_seq_num( unsigned int vol_seq ){

   Vol_seq = vol_seq;
   Vs_num = Vol_seq % MAX_VSCAN;
   if( Vs_num == 0 )
      Vs_num = MAX_VSCAN;

/* End of PRQ_set_vol_seq_num() */
}

/**************************************************************************

   Description:
      This function reads the product request LB.

   Inputs:
      buf - buffer to receive product request data.
      prod_id - message ID to read.

   Outputs:
      buf - pointer to pointer to buffer receiving product request data.

   Returns:
      0 if no product requests, the number of product requests if read
      successful.

   Notes:
      Process aborts if read fails.

**************************************************************************/
static int Read_product_requests( char **buf, int prod_id ){

   int ret;

   /* Read the product request LB. */
   ret = ORPGDA_read( Prq_lbid, buf, LB_ALLOC_BUF, prod_id );
   
   /* If LB message not found, return no requests. */
   if( ret == LB_NOT_FOUND )
      return (0);

   /* On all other LB read failures or bad length, terminate task. */
   if( (ret < 0) || ((ret % sizeof(Prod_request)) != 0) ) 
      PS_task_abort( "Bad Product Request Message For Product %d (%d)\n",
                     prod_id, ret );

   /* Return the number of requests in the request message. */
   return( ret / sizeof(Prod_request) );

/* End of Read_product_requests() */
}

/************************************************************************ 

   Description:
      This function returns in buffer "buf" the replay requests 
      for "prod_id"

   Inputs:
      buf - pointer to pointer to char to receive replay requests.
      prod_id - the product ID of interest.

   Outputs:
      buf - pointer to pointer to char receiving replay requests.

   Returns:
      The number of replay requests matching "prod_id".

   Notes:

************************************************************************/
static int Get_replay_requests( char **buf, int prod_id ){

   Prod_request *tmp_req, term_req;
   int i, num_req = 0;

   /* Check if the replay request buffer is undefined.
      If yes, return 0 requests. */
   if( Replay_req == NULL ){

      LE_send_msg( GL_INFO, "Replay Request Buffer is Undefined\n" );
      return 0;

   }

   /* Allocate space to store replay requests returned to caller. */
   if( *buf != NULL )
      free( *buf );

   *buf = calloc( (unsigned int) 1, ((N_replay_req+1)*sizeof(Prod_request)) );
   if( *buf == NULL )
      PS_task_abort( "calloc Failed for %d Bytes\n",
                     N_replay_req*sizeof(Prod_request) );

   /* Go through all one-time requests.  Save requests which match 
      one prod_id. */
   tmp_req = Replay_req;
   for( i = 0; i < N_replay_req; i++ ){

      if( tmp_req->pid == prod_id ){

         memcpy( *buf+(num_req*sizeof(Prod_request)), tmp_req, 
                 sizeof(Prod_request) );
         num_req++; 

      }

      tmp_req++;

   }

   /* Terminate the requests. */
   term_req.pid = -1;
   memcpy( *buf+(num_req*sizeof(Prod_request)), &term_req, 
           sizeof(Prod_request) );

   num_req++;

   return (num_req);

/* End of Get_replay_requests() */
}

