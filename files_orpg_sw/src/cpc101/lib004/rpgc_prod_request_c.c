/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/16 17:33:58 $
 * $Id: rpgc_prod_request_c.c,v 1.23 2012/11/16 17:33:58 ccalvert Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <orpgadpt.h>

extern int Task_type;		/* Set in rpgc_init_c.c */

/* max number of requests for an elevation index and product ID combination. */
#define MAX_SLOTS 		10            

typedef struct {                /* struct for request check list */

    int type;                   /* product type number */
    int timing;                 /* product timing */
    int *requested;             /* whether or not requested (TRUE/FALSE) */
    int in_or_out;              /* Input or Output */
    int n_requests;             /* number of requests in buffer that follows. */
    Prod_request *request;      /* product request information for this product. */

} Prod_check_list;

#define INPUT_DATA              1
#define OUTPUT_DATA             2
static Prod_check_list Rc_list [MAXN_OUTS + MAXN_INPS];
                                /* request check list - includes
                                   all outputs */

typedef struct {		/* struct for validating product request. */

   char *prod_name;		/* Product name. */
   int prod_code;		/* Code associated with name. */
   int (*fn)( User_array_t *user_array );
				/* Validation function. */

} Prod_validator_t;

static Prod_validator_t Validator_list [MAXN_OUTS];
				/* Register product request validation functions. */

static int Num_validation_fns = 0;
				/* Number of validation functions registered. */

static Prod_request *Replay_req = NULL;
                                /* Replay Request buffer. */

static Prod_request *Outstanding_replay_req = NULL;
                                /* Outstanding Replay Request buffer. */

static int N_replay_req = 0;    /* Number of replay requests. */

static int N_outstanding_replay_req = 0;
                                /* Number of outstanding replay requests. */

static int N_rc = 0;            /* number of items in Rc_list */

static Out_data_type *Out_list = NULL;
                                /* output data type list */

static int N_outs = 0;          /* number of outputs */

static In_data_type  *Inp_list = NULL;
                                /* input data type list */

static int N_inps = 0;          /* number of outputs */

static int Prq_lbid = -1;       /* data id of the product request LB */

static LB_check_list Cklist [MAXN_OUTS + MAXN_INPS];
                                /* message status check list */

static LB_status Prq_status;    /* product request msg update status */

static int Input_stream = PGM_REALTIME_STREAM;
                                /* Task's input data stream. */

static int Max_slots = MAX_SLOTS;
                                /* Maximum number of requests allowed. */

static unsigned int Vol_seq = 0;
				/* Volume scan sequence number. */

static int Vs_num = 0;		/* Volume scan number. */
				
#define MAX_VOLS		2

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
static int Validate_request( User_array_t *user_array );

/**************************************************************
   Description:
      C/C++ interface for returning product request customizing
      data.

   Inputs:
      elev_index - RPG elevation index.  For volume-based 
                   requests, value should be -1.  

   Outputs:
      n_requests - number of product requests found

   Returns:
      Returns pointer to product request data (structure of 
      type User_array_t) or NULL if no requests.

   Notes:
      Since this function allocates memory, the application
      programmer is responsible for freeing the memory.

**************************************************************/
void *RPGC_get_customizing_data( int elev_index, int *n_requests ){
   
   int stat;
   User_array_t *user_array;

   /* Set Max_slots to default. */
   Max_slots = MAX_SLOTS;

   /* If *n_requests > 0 and less than some reasonable limit, set
      the new limit. */
   if( (*n_requests > 0) && (*n_requests < SLOTS_CAP) )
      Max_slots = *n_requests;

   /* Initialize the number of requests to 0. */
   *n_requests = 0;

   /* Allocate space for requests ... number of requests limited 
      to Max_slots. */
   user_array =  (User_array_t *) 
                 calloc( 1, Max_slots*sizeof(User_array_t) );
   if( user_array == NULL ){

      PS_task_abort( "calloc Failed in RPGC_get_customizing_data()\n" );
      return (NULL);

   }

   /* Get the product request data. */
   stat = RPGC_get_customizing_info( elev_index, (void *) user_array,
                                     n_requests );

   /* If there are no user requests, free memory reserved for requests
      and return NULL pointer. */
   if( (*n_requests == 0) || (stat != NORMAL) ){

      free( user_array );
      *n_requests = 0;
      return( NULL );

   }

   return( (void *) user_array );
   
/* End of RPGC_get_customizing_data( ) */
}

/********************************************************************

   Description:
      Allows the user to register a request validator. 

   Inputs:
      prod_name - product name string (same as the name used for 
                  output registration).
      fn - pointer to user-supplied registration function.

   Returns:
      A negative number if registration fails or 0 otherwise.

********************************************************************/
int RPGC_register_req_validation_fn( char *prod_name, 
                                     int (*fn)(User_array_t *user_array) ){

   int prod_code, i;

   /* Add this validation function to the list of registered 
      functions. */
   prod_code = RPGC_get_code_from_name( prod_name );
   
   if( prod_code <= 0 ){

      LE_send_msg( GL_INFO, "Request Validation Registration Failed (Code: %d).\n",
                   prod_code );
      return -1;

   }

   /* Search list ... if duplicate registration, overwrite 
      previous registration. */
   for( i = 0; i < Num_validation_fns; i++ ){

      if(  prod_code == Validator_list[i].prod_code ){

         Validator_list[i].fn = fn;
         return 0;

      }

  } 

  /* Register the function. */
  strcpy( Validator_list[i].prod_name, prod_name ); 
  Validator_list[i].prod_code = prod_code;
  Validator_list[i].fn = fn;

  Num_validation_fns++;

  return 0;

/* End of RPGC_register_validation_fn(). */
}


/********************************************************************

   Description:
      This function is used to get product customizing data for all
      products generated by the calling task.

   Inputs:
       elev_index - elevation index of product

   Outputs:
      user_array - structure containing customizing data.
      num_requests - number of product requests at elevation index
      status - status of operation (0 - normal, 1 - failure )

   Notes:
      The user_array elements are defined as follows:

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

         The product dependent parameters are defined in
         the RPG/Associated PUP ICD.  See RPG_get_request
         for more information.

   Returns:
      There is no return value defined for this module.

********************************************************************/
int RPGC_get_customizing_info( int elev_index, User_array_t *user_array,
                               int *num_requests ){

   int buf_type, status, prod_code, el_ind, product_found, i;
   User_array_t *user_array_p = user_array;

   /* Initialize status to normal. */
   status = 0;

   /* Examine all outputs registered by this task.  If the
      output type is associated with a product code, get the
      customizing data for that product. */
   *num_requests = 0;
   product_found = 0;
   for( i = 0; i < N_outs; i++ ){

      int j, t_num_reqs = 0;

      if( Out_list [i].int_or_final != INT_PROD ){

         product_found = 1;

         buf_type = Out_list [i].type;          /* Buffer or product id */
         prod_code = Out_list [i].int_or_final; /* Product code */
         el_ind = elev_index;                   /* Elevation index */
         t_num_reqs = RPGC_get_request( el_ind, buf_type, user_array_p, Max_slots );
         LE_send_msg( GL_INFO, "There are %d Requests for Product Code %d\n",
                      t_num_reqs, prod_code );

         /* There are requests for this product. */
         if( (t_num_reqs > 0) && ((*num_requests + t_num_reqs) <= Max_slots) ){

            /* Check to see if the user has registered a request validator. */
            for( j = 0; j < t_num_reqs; j++ ){

               if( Validate_request( user_array_p ) ){

                  (*num_requests)++;
                  user_array_p++;

               }

            }

         }

      }

   }

   /* If no product codes registered for this task, return status of 1. */
   if( !product_found ) status = 1;

   return( status );

/*End of RPGC_get_customizing_info() */
}


/**************************************************************
   Description:
      C/C++ interface for checking if the data type "data_type"
      is needed.

   Inputs:
      data_type - data type (product ID) to check.

   Returns:
      Returns NORMAL if data is needed or needs to be produced.
      Returns NOT_REQD otherwise. 

**************************************************************/
int RPGC_check_data( int data_type ){
   
   int i, status;

   /* Initialize the need for this product to be generated to NOT_REQD. */
   status = NOT_REQD;

   /* If there is a request for this product or the product timing is
      DEMAND_TYPE (which means a request is not necessary), return
      NORMAL. */
   for( i = 0; i < N_rc; i++){

      if( Rc_list[i].type == data_type ){
 
         if( (*(Rc_list[i].requested) == TRUE)
                        ||
             (Rc_list[i].timing == DEMAND_DATA) )
	    return (NORMAL);

         /* For the replay stream and warehoused data .....*/
         else if( (Input_stream == PGM_REPLAY_STREAM) 
                                &&
                  (ORPGPAT_get_warehoused( data_type ) > 0) ){

            /* If the data is an input, since it is replay stream and 
               warehoused data, do not require a request.  If not 
               warehoused data, we expect a request. */
            if( Rc_list[i].in_or_out == INPUT_DATA )
               return(NORMAL);

           }

	}

    }

    return (status);

/* End of RPGC_check_data( ) */
}

/************************************************************************

   Description:
      Interface to RPGC_get_request() requiring data name.  See
      RPGC_get_request for details.

   Inputs:
      elev_index - the RPG elevation index to match; If 
	           elev_index < 0 or the elev_ind in the request is 
		   undefined (< 0), it matches all elevations.
      dataname - the product name string.
      max_requests - maximum number of requests allowed in uarray.

   Outputs:
      uarray - array containing the requests;

   Return:
      The number of product requests in uarray.

************************************************************************/
int RPGC_get_request_by_name( int elev_index, char *dataname, 
                              User_array_t *uarray, int max_requests ) {

   int prod_id = RPGC_get_id_from_name( dataname );

   return( RPGC_get_request( elev_index, prod_id, uarray, max_requests ) );

/* End of RPGC_get_request_by_name() */
}

/***********************************************************************

   Description: 
      This function returns the product requests sent to the product 
      "buf_type". The product request info is returned in the array 
      "uarray". This array is considered as a two dimensional (array of 
      PREQ_SIZE shorts).  Only those that matches elevation index 
      "elev_index" are returned if "elev_index" >= 0 and elev_ind is 
      specified in the request (elev_ind >= 0). The product code number 
      "p_code" is put in the first element of the uarray.

   Inputs:
      elev_index - the RPG elevation index to match; If 
	           elev_index < 0 or the elev_ind in the request is 
		   undefined (< 0), it matches all elevations.
      prod_id - the product identifier (product id).
      max_requests - maximum number of requests allowed in uarray.

   Outputs:	
      uarray - array containing the requests;

   Return:
      The number of product requests in uarray.

************************************************************************/
int RPGC_get_request( int elev_index, int prod_id, User_array_t *uarray,
                      int max_requests ) {

    char *buf = NULL;
    int num_reqs, index;

    /* Read product requests. */
    if( Input_stream == PGM_REALTIME_STREAM )
       num_reqs = Read_product_requests( &buf, prod_id );

    else if( Input_stream == PGM_REPLAY_STREAM )
       num_reqs = Get_replay_requests( &buf, prod_id );

    else
       return 0;

    /* The following function populates the user array and returns
       the number of requests. */
    index = PRQ_populate_user_array( elev_index, prod_id, buf, num_reqs, 
                                     uarray, max_requests );

    /* Free buffer containing product requests. */
    if( buf != NULL )
       free( buf );

    return( index );

/* End of RPGC_get_requests(). */
}
    

/***********************************************************************

   Description:
      Takes request information as input and fills the User_array_t
      structure as output.

   Input:
      elev_index - product request elevation index
      prod_id - product ID
      buf - buffer containing product request information.
      num_reqs - number of requests in buf.
      max_requests - size specifier for uarray.

   Outputs:
      uarray - user array containing product request information 

   Returns:
      Number of requests in user array, uarray.

   Note:  
      If buf is NULL, then num_reqs specifies the volume scan number.
      Data from Req_buf will be used to populate the user array.

***********************************************************************/
int PRQ_populate_user_array( int elev_index, int prod_id, char *buf, 
                             int num_reqs, User_array_t *uarray, 
                             int max_requests ){

    Prod_request *pr = NULL;
    int index, i;
    short p_code;
    User_array_t *ua = NULL;
    
    /* If the passed buffer is NULL, use the Req_buf address as 
       the buffer. */
    if( buf == NULL ){

       /* Go through Rc_list trying to find match on product ID. */
       for( i = 0; i < N_rc; i++ ){

          if( prod_id == Rc_list[i].type ){
             
             /* Determine the index into Req_buf for the volume 
                scan number.  Note: When buf is passed in as NULL
                pointer, num_reqs passes the volume scan number. */
             index = num_reqs % 2;

             /* Match found ... set buffer address and number of 
                requests. */
             buf = Req_buf[index][i];
             num_reqs = Req_msg_len[index][i];

             break;

          }

       }

    }
   
    /* If passed buffer is still NULL, return no requests. */
    if( buf == NULL )
       return 0;

    /* Initialization. */
    pr = (Prod_request *) buf;
    index = 0;
    ua = uarray;
    p_code = ORPGPAT_get_code( prod_id );
    if( p_code < 0 )
       p_code = 0;

    /* Get the appropriate requests. */
    for( i = 0; i < num_reqs; i++ ){

        /* Check for product ID mismatch. */
	if( pr->pid != prod_id ){

            /* If realtime input stream, break out of "for" loop.  Otherwise,
               continue processing all requests. */
            if( Input_stream != PGM_REALTIME_STREAM )
               continue;

            else
	       break;

        }

        /* If product request elevation index matches users elevation index or
           elevation index < 0 or product request elevation index specifies
           all elevations, then .... */
	if( (pr->elev_ind == elev_index)
                       || 
            (elev_index < 0)
                       || 
	    (pr->elev_ind == REQ_ALL_ELEVS) ){

            /* Only "max_requests" number of products are allowed for this
               elev_ind and prod_id combination. */
            if( index < max_requests ){

               /* Update user array with request information. */
	       ua->ua_prod_code = p_code;
	       ua->ua_dep_parm_0 = pr->param_1;
	       ua->ua_dep_parm_1 = pr->param_2;
	       ua->ua_dep_parm_2 = pr->param_3;
	       ua->ua_dep_parm_3 = pr->param_4;
	       ua->ua_dep_parm_4 = pr->param_5;
	       ua->ua_dep_parm_5 = pr->param_6;
	       ua->ua_elev_index = pr->elev_ind;
	       ua->ua_req_number = pr->req_num;	
	       ua->ua_spare = 1;	

               /* Prepare for next request. */
	       ua += 1;	
	       index += 1;

            }
            else
               AP_abort_request( pr, PGM_SLOT_UNAVAILABLE );

	}

        /* Prepare for next request. */
	pr++;

    /* End of "for" loop. */
    }
   
    return(index);

/* End of PRQ_populate_user_array() */
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
int PRQ_check_data (int data_type, User_array_t* user_array, int *status){

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

        if( Rc_list[i].type == data_type ){

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
                    (ORPGPAT_get_warehoused( data_type ) > 0) ){

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
int PRQ_check_req (int data_type, short *ua, int *status){

    User_array_t *user_array = (User_array_t *) ua;
    int i, j;

    /* Initialize the need for this product to be generated to NOT_REQD. */
    *status = NOT_REQD;

    /* If there is a request for this product or the product timing is
       DEMAND_TYPE (which means a request is not necessary), return
       NORMAL. */
    for( i = 0; i < N_rc; i++){

       if( Rc_list[i].type == data_type ){
 
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

/* End of PRQ_check_req() */
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
      volume based and elevation based products only at the beginning 
      of a volume. This function may be called repeatedly.  Thus we need 
      to check the previous update time to eliminate duplicated update.  
      For the replay data stream, we bypass the beginning of volume check.

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

   int ret, index, i;

   /* If update time is same as last and input stream is the real-time
      input stream, just return.  Otherwise, update the update time. */
   if( (update_time == time) 
                && 
       (Input_stream == PGM_REALTIME_STREAM )
                &&
       (elev_ind == old_elev_ind) )
      return (0);

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

            1) output timing is VOLUME_DATA or ELEVATION DATA. */
         if (new_vol == 0){

            if( (Rc_list[i].timing == VOLUME_DATA)
                               ||
                (Rc_list[i].timing == ELEVATION_DATA) )
            continue;           /* we update every volume */

         }

         msg_st = Cklist[i].status;

         /* Check if particular LB message has been update.  If updated
            or new volume, read product request information. */
         if( (msg_st == LB_MSG_UPDATED) 
                     ||
                  (new_vol) ){

            /* Free the previously allocated buffer. */
            if( Req_buf[index][i] != NULL ){

               free( Req_buf[index][i] );
               Req_buf[index][i] = NULL;

            }

            /* Requests need to be updated ... read real-time product requests for 
               this product. */
            Req_msg_len[index][i] = Read_product_requests( &Req_buf[index][i], 
                                                           Rc_list[i].type );
	 
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
         Req_msg_len[index][i] = Get_replay_requests( &Req_buf[index][i], Rc_list[i].type );

      }

   }

   /* Update product request information. */
   for( i = 0; i < N_rc; i++ ){

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

            /* For other output timings, elevation index must be REQ_ALL_ELEVS.  If
               output type is ON_REQUEST, then elevation index can be REQ_NOT_SCHEDLD. */
	    if( (pr->elev_ind == REQ_ALL_ELEVS) 
                              ||
                ((Rc_list[i].timing == TYPE_ON_REQUEST)
                               &&
                 (pr->elev_ind == REQ_NOT_SCHEDLD)) ){

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

        /* Assumes Out_list[i].data_name is a non-NULL string. */
        Validator_list [N_rc].prod_name = calloc( 1, (strlen(Out_list[i].data_name) + 1) );
        if( Validator_list [N_rc].prod_name == NULL )
           PS_task_abort( "calloc Failed for %d Bytes\n", strlen(Out_list[i].data_name) );

        strcpy( Validator_list [N_rc].prod_name, Out_list[i].data_name );
        Validator_list [N_rc].prod_code = RPGC_get_code_from_name( Out_list[i].data_name );
        Validator_list [N_rc].fn = NULL;

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

   int len, wait_for_data = 0;
   char *buf;
   Prod_request *requests = NULL;

   /* Check if this task has any inputs.   If so, set the "wait_for_data" 
      flag. */
   if( (N_inps > 0) && Inp_list[DRIVING].wait_for_data )
      wait_for_data = Inp_list[DRIVING].wait_for_data;

   while(1){

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
            if( (len == LB_EXPIRED) && (wait_for_data) )
               continue;

            PS_task_abort( "Replay Product Request LB Read Failed (%d)\n",
                           len );

         }

         /* If no new requests in Request LB, sleep a short period before 
            re-reading if we are to wait for a request (not wait for data). */ 
         if( len == LB_TO_COME ){

            if( !wait_for_data ){

               /* Service END_OF_VOLUME event if it has been received. */
               INIT_process_eov_event();

            }
   
            /* Set the number of requests to 0 and NULL the request
               pointer when returning. */
            *num_reqs = 0;
            return (NULL);

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


/**************************************************************

   Description:
      Calls user defined validation function for product
      request.  The validation function should return 1
      for a valid request or 0 if the request is invalid.

   Inputs:
      user_array - the product request.

   Notes:
      If no validation functions are registered or the
      validation function is a NULL pointer, the 
      request is assumed valid.

**************************************************************/
static int Validate_request( User_array_t *user_array ){

   int prod_code = user_array->ua_prod_code;
   int i, valid = 1;

   for( i = 0; i < Num_validation_fns; i++ ){

      if( prod_code == Validator_list[i].prod_code ){

         if( Validator_list[i].fn == NULL )
            return valid;

         valid = Validator_list[i].fn( user_array ); 
         return valid;

      }

   }

   return valid;

/* End of Validate_request(). */
}
