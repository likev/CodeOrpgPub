/********************************************************************

   This module contains the functions that support A31211__GET_INBUF 
   and A31212__REL_INBUF.

   Note:
      File scope global variables begin with a capital letter.

********************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:36:34 $
 * $Id: rpg_inbuf.c,v 1.53 2012/03/12 13:36:34 ccalvert Exp $
 * $Revision: 1.53 $
 * $State: Exp $
 */

#include <rpg.h>
#include <prod_status.h>

/* File scope global variables. */
static char *Task_name = NULL;
                        /* Task Name (not necessarily executable name). */

static In_data_type Inp_list [MAXN_INPS];
			/* list of input products */

static int N_inps = 0;	/* number of useful elements in Inp_list */

static int Driving_read = FALSE;
			/* TRUE/FALSE: flag indicating that the driving input
			   has been read */

static Product_time_t Other_input_time_list[MAXN_INPS][TIME_LIST_SIZE];
			/* list of times for TIME_LIST_SIZE most recent 
                           driving inputs */ 

static int Check_elevation_index = 0;
			/* If this flag is set, we need to check the elevation 
                           index of non-driving input against elevation index
                           of driving input for match.  This allows tasks which 
                           consume REFLDATA_ELEV to also consume data generated 
                           on the second half of split cut.  Also consumers of
                           COMBBASE_ELEV can consume data from first half of split 
                           cut.  At some future date, may want to extend this is
                           radial data (i.e., BASEDATA) as well. */
static int Use_direct = 1;	
                        /* Use ORPGDA_direct flag.  This option only works for 
                           the LB_MEMORY LB. When LB_DIRECT flag is set, 
                           the user can use function LB_direct to get the 
                           pointer pointing to the message in the shared 
                           memory.  This increases, when used for accessing 
                           the message, the efficiency by eliminating a message
                           copy, which ORPGDA_read requires. */

static int Input_stream = PGM_REALTIME_STREAM;
			/* Indicates the stream from which this process gets
			   its inputs. */ 

typedef struct Inp_buf_info {

   int     data_store_id;		/* Product ID of data to monitor. */
   LB_id_t last_msg_read;		/* Message ID of last successful 
					   message read. */
   int     read_returned;		/* Return value from read. */

} Inp_buf_info_t;

static Inp_buf_info_t Radial_info;
static int Monitor_radial_input_load = 0; /* Flag, if set, indicates the
                                             input is monitored for load shed
                                             purposes. */

static int Message_mode = 0;		/* Flag, if set, indicate process is
					   in additional message mode. */

static int Check_radial_input_load = 0;

#define RPG_IB_CHECK_INTERVAL		5
#define INVALID_MSG_ID			0xffffffff

/* Local static functions */
static int Read_other_input( int ind, int *buffer_ind );
static int Check_prod_status( char *prod_status_hdr, int prod_stat_len, 
                              unsigned int volume, int prod_id );
static int Read_other_input_replay_stream( int ind, int *buffer_ind );
static int Get_inbuf_any( int *mem, int *bufptr, int *datatype, 
                           int *opstat );
static time_t Get_product_time( LB_id_t id, int ind, int timing,
                                unsigned int *vol_seq_num,
                                int *elev_index );
static int Read_product( char **buf, LB_id_t msg_id, int ind, 
                         int *buffer_ind, int *return_status );
static void Register_input_buffer( char *buf, int size, int ind, 
                                   int *buffer_ind );
static void Set_buffer_pointer( int *mem, int ind, int buffer_ind,
                                int *bufptr );
static int Check_for_abort_message( Prod_header *phd, int ind );
static int Check_moments( int ind, Base_data_header *bdh );
void Check_radial_ib_callback( malrm_id_t timer_id );
static int Check_radial_size( int size, char *radial_msg );

/* Public Functions. */

/********************************************************************

    Description: 
        This function sets up the input data list and opens
	the input product LBs.

	Duplicated data type specifications are ignored.

    Input:
        data_type - The input data type (RPG buffer type)
	timing - OBSOLETE INPUT .... uses value from PAT!

    Outputs:

    Return:	
        Return value is 0 if successfully registered, or -1 on error.

    Notes:
	Registering too many data types causes task termination.
  
********************************************************************/
int RPG_in_data( fint *data_type, fint *timing ){

   int class_id, class_mask, i;

   /* Make sure the product is defined in the PAT. */
   if( ORPGPAT_prod_in_tbl( *data_type ) <= 0 )
      PS_task_abort( "Invalid Product ID %d, [1, %d)\n", *data_type, 
                     ORPGDAT_BASE );

   /* Check if input data type already registered.  If so, do nothing. */
   for( i = 0; i < N_inps; i++ ){

      if( Inp_list [i].type == *data_type )	
         return (0);

   }

   /* If too many input data types already registered, ignore input. */
   if( N_inps >= MAXN_INPS ){		

      LE_send_msg ( GL_ERROR, "More than %d Inputs Specified.\n", MAXN_INPS );
      return (-1);

   }

   /* Register the input data type and data_id.   These will be the same unless
      an alias is defined for this type. */
   Inp_list [N_inps].type = *data_type;
   Inp_list [N_inps].data_id = *data_type;
   Inp_list [N_inps].type_mask = 0xffff;
   Inp_list [N_inps].name = NULL;

   if( (class_id = ORPGPAT_get_class_id( *data_type )) != ORPGPAT_ERROR ){

      Inp_list [N_inps].data_id = class_id;

      /* Set the class mask, if one is defined. */
      if( (class_mask = ORPGPAT_get_class_mask( *data_type )) != ORPGPAT_ERROR )
         Inp_list [N_inps].type_mask = class_mask;

   }

   if( Inp_list [N_inps].data_id == BASEDATA_ELEV )
      Check_elevation_index = 1;

   Inp_list [N_inps].timing = ORPGPAT_get_type( *data_type );
   Inp_list [N_inps].len = 0;
   Inp_list [N_inps].time = 0;
   Inp_list [N_inps].elev_time = 0;
   Inp_list [N_inps].rpg_elev_index = 0;
   Inp_list [N_inps].vol_time = 0;
   Inp_list [N_inps].vol_num = 0;
   Inp_list [N_inps].must_read = 0;

   /* Do for maximum number of buffers of this type.  Initialize
      buffer pointer and buffer index. */
   for( i = 0; i < MAXN_IBUFS; i++ ){

      Inp_list [N_inps].buf[i] = NULL;
      Inp_list [N_inps].bufptr[i] = 0;

   }

   Inp_list [N_inps].buffer_count = -1;
   Inp_list [N_inps].requested = FALSE;
   Inp_list [N_inps].wait_for_data = TRUE;
   Inp_list [N_inps].moments = UNSPECIFIED_MOMENTS;
   Inp_list [N_inps].block_time = -1;

   N_inps++;
   return(0);

/* End of RPG_in_data */
}

/********************************************************************

    Description: 
      This function specifies an input as optional.  Optional inputs
      must specify a block time.  This is the maximum amount of time
      to wait for the data to become available.

    Input:	
      data_name - The input data name
      block_time - block time.

    Outputs:

    Globals:	
      Inp_list, N_inps

    Return:	
      Return -1 is invalid block time or input not previously
      registered.  Otherwise, return 0 if all normal.
      On error, this function has no effect.

    Notes:
      In order to register by name, the command line arguments
      must already be processed.  This is accomplished by
      call RPG_init_log_services().

********************************************************************/
int RPG_in_opt_by_name( char *data_name, fint *block_time ){

   int data_id;

   IB_get_id_from_name( data_name, &data_id );
   if( data_id <= 0 )
      return(-1);

   return( RPG_in_opt( &data_id, block_time ) );

/* End of RPG_in_opt_by_name() */
}

/********************************************************************

    Description: 
      This function specifies an input as optional.  Optional inputs
      must specify a block time.  This is the maximum amount of time
      to wait for the data to become available.

    Input:	
      data_type - The input data type (RPG buffer type)
      block_time - block time.

    Outputs:

    Globals:	
      Inp_list, N_inps

    Return:	
      Return -1 is invalid block time or input not previously
      registered.  Otherwise, return 0 if all normal.
      On error, this function has no effect.

********************************************************************/
int RPG_in_opt( fint *data_type, fint *block_time ){

    int i;

    /* The timing value must be positive and less than or equal to 
       32000 secs.  These limits are rather arbitrary.  */
    if (*block_time > 32000 || *block_time < 0){

	LE_send_msg( GL_ERROR, 
                     "Bad Block Time [%d] For Optional Input.\n",
                     *block_time );
	return (-1);

    }

    /* Find a match of the data type, and record this input as optional. */
    for( i = 0; i < N_inps; i++ ){

	if( Inp_list [i].type == *data_type){

	    if( i == DRIVING ){

		LE_send_msg ( GL_ERROR, "Driving Input Can Not Be Optional.\n");
                return(-1);

            }

	    Inp_list [i].block_time = *block_time;
	    return (0);

	}

    }

    /* If falls through to here, then the input was not registered.  
       Must first register input, then specify input as optional. */
    LE_send_msg ( GL_ERROR, "Unknown Optional Input Specified (%d).\n",
                  *data_type );

    return (-1);

/* End of RPG_in_opt() */
}

/********************************************************************

    Description: 
      This function specifies which moments user wants enabled.  
      This is only applicable to radial-type inputs or the radial
      elevation products.

    Input:	
      user_moments - moments wanting enabled.

    Outputs:

    Globals:	
      Inp_list, N_inps

    Return:	
      Return -1 if not radial-type inputs registered or moments
      incompatible with registered input.  Otherwise, return 0 
      if all normal.  On error, this function has no effect.

********************************************************************/
int RPG_reg_moments( fint *user_moments ){

    unsigned char moments = (*user_moments & 0xf); 
    int i;

    /* Verify that the specified moments are valid. */ 
    if( moments == 0 || (     !(moments | REF_MOMENT)
                          ||  !(moments | VEL_MOMENT)
                          ||  !(moments | WID_MOMENT) ) ){

	LE_send_msg( GL_ERROR, 
                     "Moments Specification (%x) Invalid.\n", moments );
	return (-1);

    }

    /* Find the radial input type. */
    for( i = 0; i < N_inps; i++ ){

        if( (ORPGPAT_get_class_id( Inp_list[i].type ) == BASEDATA) 
                         || 
            (ORPGPAT_get_class_id( Inp_list[i].type ) == RAWDATA) 
                         ||
            (ORPGPAT_get_class_id( Inp_list[i].type ) == BASEDATA_ELEV) ){

            /* For REFLDATA or REFLDATA_ELEV type, must have REF_MOMENT specified 
               as desired moment. */
	    if( (Inp_list[i].type == REFLDATA) || (Inp_list[i].type == REFLDATA_ELEV) 
                          ||
	        (Inp_list[i].type == REFL_RAWDATA) ){

               if( moments & REF_MOMENT ){

                  Inp_list[i].moments |= (unsigned char) REF_MOMENT;
                  return (0);

               }
               else{

                  LE_send_msg( GL_ERROR, 
                               "REFLDATA or REFLDATA_ELEV Registered, REF_MOMENT Not Specified\n" );
                  return (-1);

               }

            }

            /* For COMBBASE, COMBBASE_ELEV, or BASEDATA_ELEV type, any or all moments 
               must be specified as desired moments. */
            else if( (Inp_list[i].type == COMBBASE) || (Inp_list[i].type == BASEDATA) 
                                 || 
                     (Inp_list[i].type == COMB_RAWDATA) || (Inp_list[i].type == RAWDATA) 
                                 || 
                     (Inp_list[i].type == COMBBASE_ELEV) || (Inp_list[i].type == BASEDATA_ELEV) ){

               if( moments & REF_MOMENT )
                  Inp_list[i].moments |= (unsigned char) REF_MOMENT;

               if( moments & VEL_MOMENT )
                  Inp_list[i].moments |= (unsigned char) VEL_MOMENT;

               if( moments & WID_MOMENT )
                  Inp_list[i].moments |= (unsigned char) WID_MOMENT;
     
               return (0);
             
            }
            else{

               LE_send_msg( GL_ERROR, "Not Valid \"type\" For Basedata\n" );
               return (-1);

            }

        }

    }

    /* Should never get here .... */
    return (-1);

/* End of RPG_reg_moments() */
}

/***********************************************************************
   
   Description:
      Register for monitoring input buffer load for data ID "data_id".

   Inputs:
      data_id - Data ID to monitor.

   Outputs:

   Returns:

   Notes:
      "data_id" must be either RAWDATA or BASEDATA, it must be the 
      driving input, and the input timing must be RADIAL_DATA.

***********************************************************************/
int RPG_monitor_input_buffer_load( int *data_id ){


   /* Check that this "data_id" is valid (must be RAWDATA or BASEDATA),
      this "data_id" is also the driving input, and the input's timing is
      RADIAL_DATA. */
   if( (*data_id != RAWDATA) && (*data_id != BASEDATA) )
      return 0;

   if( (Inp_list[DRIVING].data_id != *data_id)
                   &&
       (Inp_list[DRIVING].timing != RADIAL_DATA) )
      return 0;

   /* Set the monitoring flag. */
   Monitor_radial_input_load = 1;

   /* Initialize the monitoring data. */
   Radial_info.data_store_id = Inp_list[DRIVING].data_id;
   Radial_info.last_msg_read = INVALID_MSG_ID;
   Radial_info.read_returned = LB_TO_COME;

   /* Register timer for input buffer load monitoring. */
   if( MALRM_register( (malrm_id_t) RESERVED_TIMER_ID, Check_radial_ib_callback ) < 0 )
      LE_send_msg( GL_ERROR, "Unable to Monitor RPG RADIAL Input Buffer Load\n" );

   return 0;

/* End of RPG_monitor_input_buffer_load() */
}

/***********************************************************************

    Description: 

	Returns the length of the input data given "bufptr".

    Inputs:	

        bufptr - the index into the array MEM of the starting 
	 	 position of the buffer to be rleased

    Outputs:
       len - on successful, receives the len of buffer, in bytes.

    Return:	

        Returns 0 if buffer found, returns -1 otherwise.

    Notes:

***********************************************************************/
int RPG_get_inbuf_len( int *bufptr, int *len ){

    int ind, num;

    /* Initialize the length to -1. */
    *len = -1;

    /* Search the input list for match on buffer pointer. */
    for( ind = 0; ind < N_inps; ind++ ){

       for( num = 0; num <= Inp_list[ind].buffer_count; num++ ) {
       
	  if( Inp_list[ind].bufptr[num] == *bufptr ){

             if( Inp_list[ind].timing != RADIAL_DATA )
                *len = Inp_list[ind].len - sizeof(Prod_header);
             else
                *len = Inp_list[ind].len;

	     return (0);

          }

       }

    }

    /* If no match found, programming error. */
    LE_send_msg( GL_ERROR, "Unknown Input Buffer (%d).  Length?????\n", *bufptr );
    return (-1);

/* End of RPG_get_inbuf_len() */
}

/* Private Functions. */
/***********************************************************************

   Description:
      Given data name, returns the Product ID associated with the name.
      The association is defined in the Task Table entry for the
      calling task.

   Inputs:
      data_name = data name string.

   Ouputs:
      data_id - data (product) ID.

***********************************************************************/
void IB_get_id_from_name( char *data_name, int *data_id ){
    
    int ind, i;
    char str[ORPG_TASKNAME_SIZ];

    /* Initialize the data_id to invalid value. */
    *data_id = -1;

    /* Check for valid string pointer. */
    if( data_name == NULL )
       return;

    /* Check for any $ in the name ... this is a reserved character. */
    i = 0;
    while( i < ORPG_TASKNAME_SIZ ){

       /* We need to make a copy of the string while we validate.
          If this string is a parameter (constant), modifying it
          will cause segmentation violation. */
       str[i] = data_name[i];

       if( str[i] == '$' )
         str[i] = '\0';

       if( str[i] == '\0' )
          break;

       i++;

    }

    /* Check to insure string length is valid */
    if( i >= ORPG_TASKNAME_SIZ )
       return;

    /* Find the data_name in the list of registered inputs. */
    for (ind = 0; ind < N_inps; ind++) {

       if( (Inp_list[ind].name != NULL)
                    &&
           (strcmp( (char *) &str[0], Inp_list[ind].name ) == 0) ){

          *data_id = Inp_list[ind].type;
          return;

       }

    }

    /* If the data_id is still undefined, check the task_attr_table entry.
       If Task_name hasn't been defined, then the argument list hasn't been
       processed yet. */
    if( Task_name != NULL ){

       int num_inputs, *input_ids = NULL;
       Orpgtat_entry_t *task_entry = NULL;
       char *input_names = NULL;

       task_entry = ORPGTAT_get_entry( (char *) &(Task_name[0]) );
       if( task_entry == NULL )
          PS_task_abort( "Task_name %s Not in TAT\n", (char *) &Task_name[0] );

       num_inputs = task_entry->num_input_dataids;
       input_ids = (int *) (((char *) task_entry) + task_entry->input_data);
       if( task_entry->input_names > 0 )
          input_names = (char *) (((char *) task_entry) + task_entry->input_names);

       if( input_names != NULL ){

          /* Do For All TAT input entries. */
          for( i = 0; i < num_inputs; i++ ){

             /* Check for match on input data name. */
             if( strcmp( input_names, (char *) &str[0] ) == 0){

                *data_id = input_ids[i];
                break;

             }

             if( input_names != NULL )
                input_names += (strlen(input_names) + 1);

          }

       }

       if( task_entry != NULL )
          free( task_entry );

    }

    return;

/* End of IB_get_id_from_name() */
}

/***********************************************************************

    Description: 
        This function is the emulated A31211__GET_INBUF function.
	The following is the original description of the function:

	Supply the calling task with the MEM pointer for a buffer of
	of input data of the requested type.  The request may be for a
	specific type or for any of the types the task processes.  A
	request for a specific type must also be one that the task
	processes.  For realtime input streams, if a specific request 
        cannot be immediately filled the task will be suspended until 
        the data is available.  For non-realtime input streams, if the
        data is not immediately available, NO_DATA is returned as the 
        "opstat".  However, for a non-realtime input which is not radial
        data and is not warehoused, then the task will suspend until
        the data is available just as in the realtime case.

	If the request is for ANY_TYPE, a NO_DATA "opstat" status is 
        returned if the task has no input data currently available.

	Further info was found in "nexrad software maintenance training",
	page RPG-SG-04-02-3.

    Inputs:	
        mem - the MEM array
	reqdata - data type as defined in rpg_globals.h; If ANY_TYPE,
	          it will match the first input data listed on the 
		  input queue of the calling task.

    Outputs:
        bufptr - index in MEM for the first element of the buf;
	datatype - returns the data type (the same as reqdata if
		   reqdata != ANY_TYPE).
	opstat - status as defined in a309.h.

    Return:	
        Return value is 0 if buffer acquired successfully.  Otherwise,
        -1 is returned (such as for an abort condition).

    Notes:	
        When an fatal error is detected, this function aborts 
	the task. Refer to support.doc for a detailed description
	of this function.

***********************************************************************/
int IB_get_inbuf( int *mem, int *reqdata, int *bufptr, 
         	  int *datatype, int *opstat ){

    int ind, alias_class_id, alias_id, buffer_ind;
    Prod_header *phd = (Prod_header *) NULL;
    Base_data_header *bhd = NULL;

    /* Initialize status of get_inbuf operation to NORMAL. */
    *opstat = NORMAL;

    /* If requested type is ANY_TYPE, then wait for any of the inputs to 
      become available. */
    if (*reqdata == ANY_TYPE) {

        /* If task activation is based on WAIT_DRIVING_INPUT, can not request 
           data for ANY_TYPE.  Only tasks whose activation is based on  
           WAIT_ANY_INPUT can request data of ANY_TYPE. */
	if (WA_wait_driving ())
	    PS_task_abort ("Illegal Call To IB_get_inbuf With ANY_TYPE\n");

        /* Check if any of the registered inputs are available. */
	return (Get_inbuf_any (mem, bufptr, datatype, opstat));

    }

    *datatype = *reqdata;

    /* Search for a match of the requested data type with the 
       registered data types.  The match can be for registered 
       type or data ID (class ID if different) associated with 
       registered type. */
    for (ind = 0; ind < N_inps; ind++) {

       if( (*reqdata == Inp_list[ind].type) 
                     ||
           (*reqdata == Inp_list[ind].data_id) )
          break;

    }

    /* In the typical case, the first pass through the inputs
       would find a match.   This next pass is needed in case
       aliases were defined. (NOTE: A separate pass was defined
       for efficiency reasons .... otherwise this pass and the
       previous could have been accomplished in a single pass.) */
    if( ind >= N_inps ){

       /* For a requested type that is an alias for another type, 
          check the alias's type and class. */
       for (ind = 0; ind < N_inps; ind++) {

          alias_id = ORPGPAT_get_aliased_prod_id( Inp_list[ind].type );
          alias_class_id = ORPGPAT_get_class_id( alias_id );
          if( ((alias_id > 0) && (*reqdata == alias_id))
                              ||
              ((alias_class_id > 0) && (*reqdata == alias_class_id)) )
             break;

       }

       if (ind >= N_inps){

           /* No match found. */ 
	   PS_task_abort ("Unknown Input Data Request %d\n", *reqdata);

       }

    }

    /* If requested data type is the driving input, read the input, then 
       validate the input via call to WA_check_data.  If data is out of 
       sequence, then WA_check_data will return TERMINATE.   Otherwise, 
       check if the requested data is needed. (e.g., This is required for 
       RADIAL_TYPE consumers who request BASEDATA but require either 
       REFLDATA or COMBBASE data.  It is also required for consumers 
       of the BASEDATA_ELEV data.) */
    if (ind == DRIVING) {

        while(1){

           /* Check if radial monitoring is active. */
           if( Monitor_radial_input_load ){

              if( Check_radial_input_load ){

                 time_t timer_expires_at;

                 /* Check the current radial input load. */
                 Check_radial_input_load = 0;
                 IB_check_input_buffer_load();

                 /* Reset timer for next check. */
                 timer_expires_at = time( (time_t *) NULL ) +
                                    (time_t) RPG_IB_CHECK_INTERVAL;

                 if( MALRM_set( (malrm_id_t) RESERVED_TIMER_ID, timer_expires_at,
                                MALRM_ONESHOT_NTVL ) < 0 )
                    LE_send_msg( GL_ERROR, 
                                 "Unable To Set RPG RADIAL Input Buffer Check Timer\n" );
                 
              }

           }

           *opstat = IB_read_driving_input ( &buffer_ind );
	   Driving_read = TRUE;

           /* Break out of "while" loop if acquisition of driving input
              fails. */
	   if( *opstat != NORMAL )
              break;

           *opstat = WA_check_data ( buffer_ind );
        
           /* For non-RADIAL_DATA driving input, check if the abort 
              flag is set in the orpg product header.  If so, set the
              status of operation depending on the abort flag value. */
           phd = (Prod_header *) NULL;
           if( (Inp_list[DRIVING].timing != RADIAL_DATA)
                            &&
               (*opstat == NORMAL) ){

              phd = (Prod_header *) Inp_list[DRIVING].buf[buffer_ind];
              *opstat = Check_for_abort_message( phd, DRIVING );

           }
         
           /* Non-NORMAL status causes while loop exit. */
	   if ( *opstat != NORMAL )
              break;

           /* For radial and radial elevation inputs, check need to process 
              this buffer.  All other input types will return DATA_NEEDED. */
           if( WA_data_filtering( Inp_list + DRIVING, buffer_ind ) == DATA_NEEDED ){

              /* Check if appropriate moments are enabled ... need only do this at
                 beginning of volume. */
              if( Inp_list[DRIVING].moments != UNSPECIFIED_MOMENTS ){

                 /* We need only do this check at beginning of volume. */
                 if( (ORPGPAT_get_class_id( Inp_list[DRIVING].type ) == BASEDATA)
                                           ||
                     (ORPGPAT_get_class_id( Inp_list[DRIVING].type ) == RAWDATA)
                                           ||
                     (ORPGPAT_get_class_id( *reqdata ) == BASEDATA) )
                    bhd = (Base_data_header *) Inp_list[DRIVING].buf[buffer_ind];

                 else if( (ORPGPAT_get_class_id( Inp_list[DRIVING].type ) == BASEDATA_ELEV) 
                                           ||
                          (ORPGPAT_get_aliased_prod_id( *reqdata ) == BASEDATA_ELEV) ){

                    /* For the BASEDATA_ELEV product, need to account for the 
                       ORPG product header prepended to the product. */
                    Compact_basedata_elev *cbe;

                    cbe = (Compact_basedata_elev *) (Inp_list[DRIVING].buf[buffer_ind]
                                                  + sizeof(Prod_header));
                    bhd = &cbe->radial[0].bdh;

                 }
                 else
                    bhd = NULL;
                 
                 /* If at beginning of elevation/volume and moments have been, 
                    specified, check if the data has the specified moments. */
                 if( bhd != NULL ){

                    if( ((bhd->status & 0xf) == GOODBEL) 
                                          || 
                        ((bhd->status & 0xf) == GOODBVOL) ){ 

                       /* If specified moments not available, return NO_DATA. */
                       if( Check_moments( DRIVING, bhd ) ){

                          AP_set_abort_reason( PGM_DISABLED_MOMENT );
                          *opstat = NO_DATA;

                       }

                    }

                 }

              }

              break;

           }

           /* Data not needed.  Release buffer and go to top of while loop. */
           else
              IB_release_input_buffer( DRIVING, buffer_ind );

       /* End of "while" loop. */
       }

    }
    else{ 
       
       /* Requested type is the non-driving input, so read other 
          input. */
       *opstat = Read_other_input( ind, &buffer_ind );

       /* A NORMAL return indicates an input buffer was successfully read. */
       if( (*opstat == NORMAL) 
                  &&  
           (Inp_list[ind].timing != RADIAL_DATA) ){

          In_data_type *idata = &Inp_list[ind]; 
          int is_abort_msg = NORMAL;

          /* Check for abort condition. */   
          phd = (Prod_header *) Inp_list[ind].buf[buffer_ind];

          is_abort_msg = Check_for_abort_message( phd, ind );

          if( (idata->block_time >= 0) && (is_abort_msg != NORMAL) )
             *opstat = NO_DATA;

          else
             *opstat = is_abort_msg;

       }

    }

    /* If operation status is NORMAL, then .... */ 
    if ( *opstat == NORMAL ){

       /* Set and register the buffer pointer. */
       Set_buffer_pointer( mem, ind, buffer_ind, bufptr );

       /* Read all required ITCs which were registered to be updated
          with receipt of "reqdata". */
       ITC_read_all (*reqdata);

       /* We can return to caller at this point. */
       return (0);

    }
    else if( (*opstat == TERMINATE) || (*opstat == FAULT_ME) 
                         || 
             (*opstat == NO_DATA && (Inp_list[ind].block_time < 0))
                         ||
             (*opstat < 0) ){

       /* Abort processing for this volume scan.

         NOTE:  If product header pointer is not NULL, then an abort condition
                has occurred in response to an input with the abort flag set
                or NO_DATA.  Otherwise, the abort condition was triggered by the 
                algorithm control flag within a radial message since radial messages
                have no header.  In the first case, RPG_abort_processing must be 
                called to set the Abort flag in the wait_act modules.

                Normally we would expect the aborting process to call 
                A31145_ABORT_ME or A31168_ABORT_ME_BECAUSE.  Since there is 
                no guarantee the task aborts itself, we abort here!  */

       if( phd != (Prod_header *) NULL ){

          /* Set the aborted volume sequence number. */
          AP_set_aborted_volume( phd->g.vol_num );

          /* The "len" field contains the abort reason code. */
          if( phd->g.len < 0 )
             RPG_abort_processing( &phd->g.len );

       }
       else{

          int reason = AP_get_abort_reason();
             
          /* Check if the abort reason is defined and valid.  Valid abort
             reasons are in the range:

                   PGM_MAX_GEN_FAILURE <= reason <= PGM_MIN_GEN_FAILURE.

             If valid, use that reason.  Otherwise the abort reason is 
             PGM_SCAN_ABORT by default. */ 
         if( (reason < PGM_MIN_GEN_FAILURE) || (reason > PGM_MAX_GEN_FAILURE) )
             reason = PGM_SCAN_ABORT;

         RPG_abort_processing( &reason );

       }

       /* Release input buffer on an abort condition. */
       IB_release_input_buffer( ind, buffer_ind );

       return (-1);

    }
    else if( *opstat == NO_DATA ){

       /* If NO_DATA, this may mean that an ABORT_MSG indicated disabled
          moments or no memory.  In this case, we need to release the
          input buffer. */
       IB_release_input_buffer( ind, buffer_ind );

       return (-1);

    }

    /* Control should never get to this point. */
    LE_send_msg( GL_ERROR, "Control Reached End of Function Unexpectedly\n" );
    return (0);

/* End of IB_get_inbuf() */
}

/**************************************************************
   Description:
      RPG interface for allowing all input products to be
      registered with a single call.

   Returns:
      Returns -1 on error, or 0 otherwise.

**************************************************************/
void IB_reg_inputs( int *status ){

   Orpgtat_entry_t *tat = NULL;
   int timing, num_inputs, i, *input_ids = NULL;
   int done, num_opt_inputs, num_outputs, j, *output_ids = NULL;

   *status = 0;

   /* Register for LE services if not already registered. */
   RPG_init_log_services_c( );

   /* Get my task_name ... may be different than executable name.
      Output registration are based on task_name, not executable
      name. */
   Task_name = INIT_task_name();
   IB_set_task_name( Task_name );

   /* Get the TAT entry for this task name. */
   if( (tat = ORPGTAT_get_entry( Task_name )) == NULL ){

      LE_send_msg( GL_ERROR, 
              "Unable to Get Task Entry in RPGC_reg_inputs\n" );
      *status = -1;
      return;

   }

   /* Get the number of inputs and the input data IDs (i.e.,
      the product IDs) */
   num_inputs = tat->num_input_dataids;
   input_ids = (int *) (((char *) tat) + tat->input_data);

   /* Register each input. */
   for( i = 0; i < num_inputs; i++ ){

      LE_send_msg( GL_INFO, "Registering Input (product ID) %d\n", 
                   input_ids[i]);
      RPG_in_data( &input_ids[i], &timing );

      /* Get the number of outputs and the output data IDs (i.e.,
         the product IDs).  We need this in order to determine if the
         input registered is optional or mandatory. */
      num_outputs = tat->num_output_dataids;
      output_ids = (int *) (((char *) tat) + tat->output_data);
      done = 0;

      for( j = 0; j < num_outputs; j++ ){

         /* Check if there are any optional inputs which match this input. */
         if( (num_opt_inputs = ORPGPAT_get_num_opt_prods( output_ids[j] )) > 0 ){

            int index, opt_prod, block_time = 5;
            for( index = 0; index < num_opt_inputs; index++ ){

                opt_prod = ORPGPAT_get_opt_prod( output_ids[j], index );
                if( opt_prod == input_ids[i] ){

                   LE_send_msg( GL_INFO, "--->Registering Input As Optional (Block Time = 5)\n" ); 
                   RPG_in_opt( &input_ids[i], &block_time );
                   done = 1;
                   break;

                }
                else if( opt_prod == ORPGPAT_ERROR ){

                  LE_send_msg( GL_ERROR, "ORPGPAT_get_num_opt_prods() Failed For Product %d\n",
                               output_ids[j] );
                  done = 1;
                  break;

               }

            }

            if( done )
               break;

         }
         else if( num_opt_inputs == ORPGPAT_ERROR ){

            LE_send_msg( GL_ERROR, "ORPGPAT_get_num_opt_prods() Failed For Product %d\n",
                         input_ids[i] );
            break;

         }

      }

   }

   if( tat != NULL )
      free( tat );

   return;

}

/**************************************************************************************

   Description:
   
      This functions checks the number of unread messages in monitored data store 
      and sets the ratio (number of unread messages) / ( maximum number of messages).  
      This number is then reported as a percent to ORPGLOAD for 
      LOAD_SHED_CATEGORY_RPG_RADIAL.
   
   Inputs:
      
   Outputs:

   Returns:

   Notes:
      If last_msg_read equals INVALID_MSG_ID, it is assume unavailable and the load 
      is reported as 0.

*************************************************************************************/
void IB_check_input_buffer_load( ){

   int ret;
   LB_status status;
   LB_attr attr;
   LB_info info;

   int max_msgs = 0;
   int load = 0; 
   int number_unread = 0;

   /* Ensure that monitoring is activated. */
   if( !Monitor_radial_input_load )
      return;

   /* If "msg_id" is valid, do the following. */
   if( Radial_info.last_msg_read != INVALID_MSG_ID ){

      /* Get maximum number of messages in LB. */
      status.n_check = 0;
      status.attr = &attr;
      if( (ret = ORPGDA_stat( Radial_info.data_store_id, &status )) < 0 ){

         LE_send_msg( GL_ERROR, "ORPGDA_stat Failed (%d)\n", ret );
         return;

      }
      max_msgs = attr.maxn_msgs;

      /* Find the number of unread messages in the LB. */
      if( (ret = ORPGDA_msg_info( Radial_info.data_store_id, LB_LATEST, &info )) < 0 ){

         LE_send_msg( GL_ERROR, "ORPGDA_msg_info Failed (%d)\n", ret ); 
         return;

      }

      number_unread = info.id - Radial_info.last_msg_read;
      if( number_unread < 0 )
         number_unread = -number_unread;

   }

   /* Determine load (in percent) */
   if( number_unread > 0 ){

      if( max_msgs > 0 )
         load = (int) ((float) (number_unread * 100) / ((float) max_msgs));

   }
   else{

      /* If "read_returned" value is LB_EXPIRED, then set load level to 100. 
         Otherwise, set to 0. */
      if( Radial_info.read_returned == LB_EXPIRED )
         load = 100;

      else
         load = 0;

   }

   /* Report the load. */
   ORPGLOAD_set_data( LOAD_SHED_CATEGORY_RPG_RADIAL,
                      LOAD_SHED_CURRENT_VALUE, load );
 
/* End of RPG_check_inbut_buffer_load() */
}

/***********************************************************************

    Description: 

        This function is the emulated A31212__REL_INBUF function.
	The following is the original description of the function:

	Release a buffer of input data.  This routine is used by a task
	to signal completion of processing of the data contained in
	the input buffer.

	Further info was found in "nexrad software maintenance training",
	page RPG-SG-04-02-3.

    Inputs:	

        bufptr - the index into the array MEM of the starting 
	 	 position of the buffer to be rleased

    Return:	

        Returns 0 if buffer successfully released, returns -1 otherwise.

    Notes:

***********************************************************************/
int IB_rel_inbuf( int *bufptr ){

    int ind, num;

    /* Search the input list for match on buffer pointer. */
    for (ind = 0; ind < N_inps; ind++) {

       for(num = 0; num <= Inp_list[ind].buffer_count; num++ ) {
       
	  if (Inp_list[ind].bufptr[num] == *bufptr){

             /* Match of buffer pointer with registered buffer pointer.
                Free space allocated to buffer and return to caller. */
             IB_release_input_buffer( ind, num );
	     return (0);

          }

       }

    }

    /* If no match found, programming error. */
    LE_send_msg( GL_INFO, "Releasing Unknown Input Buffer Pointer (%d)\n", 
                 *bufptr );

    return (-1);

/* End of IB_rel_inbuf() */
}

/***********************************************************************

    Description: 
       Releases all input buffers acquired.

    Inputs:	
       None

    Returns:	
       Currently always returns 0.

    Notes:

***********************************************************************/
int IB_rel_all_inbufs( ){

    int ind, num;

#ifdef DEBUG 
    LE_send_msg( GL_ERROR, "--->Releasing all input buffers\n" );
#endif

    /* Search the input list for match on buffer pointer. */
    for (ind = 0; ind < N_inps; ind++) {

#ifdef DEBUG 
       LE_send_msg( GL_INFO, "------>For input ind %d, there are %d buffers\n",
                    ind, Inp_list[ind].buffer_count );
#endif

       for(num = 0; num <= Inp_list[ind].buffer_count; num++ ){

#ifdef DEBUG 
          LE_send_msg( GL_INFO, "------>Releasing input buffer @ %p\n",
                       Inp_list[ind].buf[num] );
#endif
          IB_release_input_buffer( ind, num );

       }

    }

    return(0);

/* End of IB_rel_all_inbufs() */
}

/***********************************************************************

    Description: 
       Release an input buffer of data.  Sets buffer pointer to NULL and
       frees memory allocated to buffer.  

    Inputs:	
       ind - Inp_list index for this data type.
       buffer_ind - the index of the input data buffer to be 
		    released.

    Outputs:

    Returns:
       There is no return value for this function.

    Globals:
       Inp_list, Use_direct, N_inps

    Notes:

***********************************************************************/
void IB_release_input_buffer ( int ind, int buffer_ind ){

    /* If buffer to be released has already been released or if the
       buffer is invalid, just return. */
    if( buffer_ind < 0 
               ||
        (Inp_list[ind].buf[buffer_ind] == NULL 
                       &&
        Inp_list[ind].bufptr[buffer_ind] == 0) ){

#ifdef DEBUG
       if( buffer_ind < 0 )
          LE_send_msg( GL_INFO, "--->buffer_ind < 0: %d\n", buffer_ind );

       else if( Inp_list[ind].buf[buffer_ind] == NULL )
          LE_send_msg( GL_INFO, "--->Inp_list[ind].buf[buffer_ind] == NULL\n" );

       else if( Inp_list[ind].bufptr[buffer_ind] == 0 )
          LE_send_msg( GL_INFO, "--->Inp_list[ind].bufptr[buffer_ind] == 0\n" );

#endif

       return;

    }

    /* Free the buffer corresponding to ind and buffer_ind. Cannot free 
       space if direct access is used and buffer has timing RADIAL_DATA. */
    if( Inp_list[ind].buf[buffer_ind] != NULL ){

	if( (Inp_list[ind].timing != RADIAL_DATA) 
                           ||
                    (!Use_direct) ){

	    free (Inp_list[ind].buf[buffer_ind]);

        }

	Inp_list[ind].buf[buffer_ind] = NULL;

    }

    Inp_list[ind].bufptr[buffer_ind] = 0;

    /* Decrement the count of allocated buffers, if necessary. */
    if( buffer_ind == Inp_list[ind].buffer_count )
       Inp_list[ind].buffer_count--;

/* End of IB_release_input_buffer() */
}

/***********************************************************************

   Description:
      Sets Task_name from task_name.

***********************************************************************/
void IB_set_task_name( char *task_name ){

   Task_name = task_name;

/* End of IB_set_task_name. */
}

/***********************************************************************

    Description: 
       This function initializes this module.

          The input timing values are checked for valid timing.  
          Invalid timing values caused task termination.

          The Other_input_time_list and Driving_input_time_list 
          structures are initialized.

          Set the task's input stream.

    Inputs:
       task_table - task table entry

    Outputs:

    Returns:

    Globals:
       Inp_list, N_inps, Driving_input_time_list, Other_input_time_list

    Notes:
 
***********************************************************************/
void IB_initialize ( Orpgtat_entry_t *task_table ){

   int aliased_id, i, j;
   int num_inputs = 0, *input_ids = NULL;
   int class_id, class_mask;
   char *input_names = NULL;

   /* Make sure the task_table is valid. */
   if( task_table == NULL )
      PS_task_abort( "IB_initialize Failed ... task_table is NULL" );

   /* Do for all inputs in the task table entry ..... */
   num_inputs = task_table->num_input_dataids;
   input_ids = (int *) (((char *) task_table) + task_table->input_data);
   if( task_table->input_names > 0 )
      input_names = (char *) (((char *) task_table) + task_table->input_names);

   /* If one of the task table inputs has an alias defined as a registered
      input, then:

      1) set the type to the task_table input id.
      2) set the data_id to the class id of the task_table input id.
      3) set the class mask to be used for this input. 

      The result is we change the registered inputs table to recognize
      the aliased input.
   */
   for( i = 0; i < num_inputs; i++ ){

      aliased_id = ORPGPAT_get_aliased_prod_id( input_ids[i] );
      if( aliased_id == ORPGPAT_ERROR )
         PS_task_abort( "ORPGPAT_get_aliased_prod_id Failed for input_id[%d]: %d\n",
                        i, input_ids[i] );

      /* If no aliased ID defined, go to next input in list. */
      if( aliased_id < 0 ){

         if( input_names != NULL ){

            for( j = 0; j < N_inps; j++ ){

               if( input_ids[i] == Inp_list[j].type ){

                  /* The input name may have already been set during registration.
                     If so, the name will point to a valid string.  If not, the
                     name will be a NULL pointer.  In this event, we need to set
                     the name. */
                  if( Inp_list[j].name == NULL ){

                     Inp_list[j].name = calloc( 1, strlen(input_names)+1 );
                     if( Inp_list[j].name == NULL )
                        PS_task_abort( "calloc Failed for %d Bytes\n",
                                       strlen(input_names)+1 );

                     strcpy( Inp_list[j].name, input_names );

                  }

                  break;

               }

            }

         }

      }
      else{

         for( j = 0; j < N_inps; j++ ){

            if( aliased_id == Inp_list[j].type ){

               Inp_list[j].type = input_ids[i];
               Inp_list[j].data_id = input_ids[i];

               if( (class_id = ORPGPAT_get_class_id( input_ids[i] )) != ORPGPAT_ERROR ){

                  Inp_list[j].data_id = class_id;
                  Inp_list[j].type_mask = 0xffff;

                  if( (class_mask = ORPGPAT_get_class_mask( input_ids[i] )) != ORPGPAT_ERROR )
                     Inp_list[j].type_mask = class_mask;

               }

               if( input_names != NULL ){

                  /* The input name may have already been set during registration.
                     If so, the name will point to a valid string.  If not, the
                     name will be a NULL pointer.  In this event, we need to set
                     the name. */
                  if( Inp_list[j].name == NULL ){
 
                     Inp_list[j].name = calloc( 1, strlen(input_names)+1 );
                     if( Inp_list[j].name == NULL )
                        PS_task_abort( "calloc Failed for %d Bytes\n",
                                       strlen(input_names)+1 );

                     strcpy( Inp_list[j].name, input_names );

                  }

               }

               LE_send_msg( GL_INFO, "Input Type %d Has an Alias\n", Inp_list[j].type );
               LE_send_msg( GL_INFO, "--->type: %d data_id: %d, task_mask: %d\n", 
                            Inp_list[j].type, Inp_list[j].data_id, Inp_list[j].type_mask );
               if( Inp_list[j].name != NULL )
                   LE_send_msg( GL_INFO, "--->name: %s\n", Inp_list[j].name );

               break;

            }

         }

      } 

      if( input_names != NULL )
         input_names += (strlen(input_names) + 1);
   }

   /* Set the tasks input stream. */
   Input_stream = INIT_get_task_input_stream();

   /* Do for all registered inputs in Inp_list .... */
   for (i = 0; i < N_inps; i++) {

      /* If replay task and input data timing is RADIAL_DATA, register
         the data.  Also clear the wait for data flag. */
      if( (Input_stream == PGM_REPLAY_STREAM)
                        &&
          (Inp_list [i].timing == RADIAL_DATA)
                        &&
          (ORPGPAT_get_warehoused( Inp_list [i].data_id ) > 0) ){

         int warehouse_id = ORPGPAT_get_warehouse_id( Inp_list[i].data_id );
         int warehouse_acct_id = ORPGPAT_get_warehouse_acct_id( Inp_list[i].data_id );
         int ret = -1;

         if( (warehouse_id > 0 ) && (warehouse_acct_id > 0) )
            ret = ORPGBDR_reg_radial_replay_type( Inp_list[i].data_id, 
                                                  warehouse_id,
                                                  warehouse_acct_id );

         if( ret > 0 ){

             LE_send_msg( GL_INFO, "ORPGBDR_reg_radial_replay_type Successful\n" );
             LE_send_msg( GL_INFO, "--->warehouse ID: %d, warehouse acct ID: %d\n",
                          warehouse_id, warehouse_acct_id );

         }

         Inp_list [i].wait_for_data = FALSE;

      }
      else if( Input_stream == PGM_REPLAY_STREAM ){

         /* Open the LB to set the read pointer. */
         if( ORPGDA_lbfd( Inp_list[i].data_id ) < 0 )
            LE_send_msg( GL_ERROR, "ORPGDA_lbfd Failed For %d\n", Inp_list[i].data_id );

         /* Check if the data is warehoused ..... */
         if( ORPGPAT_get_warehoused( Inp_list [i].data_id ) > 0 ){

            /* Data is warehoused, so clear the wait for data flag. */
            Inp_list [i].wait_for_data = FALSE;

            /* Close the input LB. */
            ORPGDA_close( Inp_list[i].data_id );

         }

      }

   }

   /* Initialize the Other_input_time and Driving_input_time list 
       structures. */
   for(i = 0; i < TIME_LIST_SIZE; i++){

      for(j = 0; j < MAXN_INPS; j++)
         Other_input_time_list[j][i].id = -1;

   }

   /* If monitoring radial input load, set timer now. */
   if( Monitor_radial_input_load ){

      time_t timer_expires_at;

      timer_expires_at = time( (time_t *) NULL ) + (time_t) RPG_IB_CHECK_INTERVAL;
      if( MALRM_set( (malrm_id_t) RESERVED_TIMER_ID, timer_expires_at,
                     MALRM_ONESHOT_NTVL ) < 0 )
         LE_send_msg( GL_ERROR, 
                      "Unable To Set RPG RADIAL Input Buffer Check Timer\n" );

   }

   /* Check if in additional message mode. */
   Message_mode = PS_in_message_mode();

   for( j = 0; j < N_inps; j++ ){

      LE_send_msg( GL_INFO, "Attributes for Registered Input %d\n", j );
      LE_send_msg( GL_INFO, "--->Type: %8d, Mask: %8x, Timing: %4d, Data ID: %8d\n",
                   Inp_list[j].type, Inp_list[j].type_mask, 
                   Inp_list[j].timing, Inp_list[j].data_id );
      if( Inp_list[j].name != NULL )
         LE_send_msg( GL_INFO, "--->Name: %s\n", Inp_list[j].name );
      if( Inp_list[j].block_time >= 0 )
         LE_send_msg( GL_INFO, "--->This Input Is Optional.  Block Time: %d\n",
                      Inp_list[j].block_time );
      
   }

/* End of IB_initialize() */
}

/***********************************************************************

    Description: 

       This function returns Inp_list, the input data type table, and 
       N_inps, the number of inputs.

    Outputs:	

       ilist - Inp_list

    Globals:

       Inp_list, N_inps

    Return:	

       N_inps
 
***********************************************************************/
int IB_inp_list( In_data_type **ilist ){

    *ilist = Inp_list;
    return (N_inps);

/* End of IB_inp_list() */
}

/***********************************************************************

    Description: 

       This function is called by RPG_wait_act to reset Driving_read 
       flag before a new processing session starts.

    Globals:
 
       Driving_read
 
***********************************************************************/
void IB_new_session (){

    Driving_read = FALSE;

    return;

/* End of IB_new_session() */
}

/***********************************************************************

    Description: 
        This function reads the next data in the driving input.  If the 
        data is not available, then for realtime input streams it waits 
        until it becomes available.  Otherwise, return immediately.

    Input:	 
        buffer_ind - Pointer to int to receive the index of the input 
                     data buffer.

    Outputs:
        buffer_ind - Pointer to int receiving the index of the input 
                     data buffer.

    Returns:
       If input buffer successfully acquired, returns NORMAL.  For 
       non-realtime input streams, returns NO_DATA if data not yet
       available.

    Notes:
       If an error from LB read is something other than LB_EXPIRED or
       LB_TO_COME, the task terminates.

***********************************************************************/
int IB_read_driving_input ( int *buffer_ind ){

   char *bd_buf = NULL;	/* base data buffer */

   /* Initialize return value to NORMAL and buffer_ind to invalide value. */
   *buffer_ind = -1;

   /* If driving input is radial data, then ..... */
   if( Inp_list[DRIVING].timing == RADIAL_DATA ){

      while (1){

         int size;

         /* If task is waiting for activation and END_OF_VOLUME event has
            been received, service it now. */
         if( WA_waiting_for_activation() )
            INIT_process_eov_event();

	 /* Try ORPGDA_direct.  This is only applicable for LB's stored 
            as shared memory.  */
	 if (Use_direct){

            /* Access of driving input data depends on input data
               stream. */
            if( Input_stream == PGM_REALTIME_STREAM )
               size = ORPGDA_direct( Inp_list[DRIVING].data_id, &bd_buf, 
                                     LB_NEXT );
           
            else{

               size = ORPGBDR_read_radial_direct( Inp_list[DRIVING].data_id, 
                                                  &bd_buf, LB_NEXT );
               if( size == LB_NOT_FOUND )
                  PS_task_abort( "ORPGBDR_read_radial_direct Failed (data_id: %d, LB_NEXT)\n",
                                 Inp_list[DRIVING].data_id );

            }

	    if( size == LB_BAD_ACCESS || size == RSS_NOT_SUPPORTED) {

               /* Direct access not supported.  Must use ORPGDA_read. */
               Use_direct = 0;
               bd_buf = NULL;
               if( Message_mode )
                  PS_message( "Direct Access Not Supported\n" );

	       continue;

            }

         }

         /* Direct access not supported.  Use ORPGDA_read */
	 else{

            /* Access of driving input data depends on input data
               stream. */
            if( Input_stream == PGM_REALTIME_STREAM )
	       size = ORPGDA_read( Inp_list[DRIVING].data_id, &bd_buf, 
                                   LB_ALLOC_BUF, LB_NEXT );
            else 
	       size = ORPGBDR_read_radial( Inp_list[DRIVING].data_id, &bd_buf, 
                                           LB_ALLOC_BUF, LB_NEXT );

         }

         /* If real-time stream, get message ID of last sucessful read and
            save along with status of read.  For monitoring the radial 
            input load only. */
         if( (Monitor_radial_input_load) 
                           &&
             (Input_stream == PGM_REALTIME_STREAM) ){

            Radial_info.last_msg_read = INVALID_MSG_ID; 
            if( size > 0 )
               Radial_info.last_msg_read = ORPGDA_get_msg_id( ); 

            Radial_info.read_returned = size;

         }

         /* Check for success of acquiring driving input. */
         if( (size > 0) && (Check_radial_size( size, bd_buf ) == 0) ){
                 
            /* Successful! */
            Register_input_buffer( bd_buf, size, DRIVING, buffer_ind );

            /* Break out of "while" loop so we can return to caller. */
            break;

	 }

         /* Data not yet available.  For realtime data, try again.
            Otherwise check if we should wait for the data to become
            available.  If not, invalidate the buffer index and return 
            NO_DATA.  */
	 else if( size == LB_TO_COME ){	

            /* Free basedata buffer which may have been allocated. */
            if( bd_buf != NULL && !Use_direct )
               free( bd_buf );

            if( Input_stream != PGM_REALTIME_STREAM ){

               if( !Inp_list[DRIVING].wait_for_data ){

                  /* Do not wait for input to become available. */
                  *buffer_ind = -1;
                  return (NO_DATA);

               }

            }

            /* Wait for input to become available.  Wait a short time 
               before re-checking if input is available. */
	    msleep (WAIT_TIME);

            /* Go to top of "while" loop. */
	    continue;

         }

         /* Data has expired from LB.  For realtime data, try again.
            Otherwise check if we should wait for the data to become
            available.  If not, invalidate the buffer index and return 
            NO_DATA.  */
	 else if( size == LB_EXPIRED ){

            /* Free basedata buffer which may have been allocated. */
            if( bd_buf != NULL && !Use_direct )
               free( bd_buf );

            if( Input_stream != PGM_REALTIME_STREAM ){

               /* Should we wait for the data or just return NO_DATA. */
               if( !Inp_list[DRIVING].wait_for_data ){

                  /* Do not wait for input to become available. */
                  *buffer_ind = -1;
                  return (NO_DATA);

               }

            }

            /* Go to top of "while" loop. */
	    continue;

         }
	 else{

            if( size < 0 ) 
               PS_task_abort( "Driving Input Read Failed (%d) (data_id: %d, LB_NEXT)\n", 
                              size, Inp_list[DRIVING].data_id );

            else
               PS_task_abort( "Unexpected Driving Input Size: %d, Expected: %d\n",
                              size, BASEDATA_SIZE*sizeof(short) );

        }

      /* End of "while" loop. */
      }

      /* Return to caller. */
      return (NORMAL);

   }	

   /* Non-radial driving input data. */
   else {			

      /* Do Forever .....  */
      while (1){

	 int size, return_status;
         char *buf;

         /* If task is waiting for activation and END_OF_VOLUME event has
            been received, service it now. */
         if( WA_waiting_for_activation() )
            INIT_process_eov_event();

         /* Read the driving input. */
	 size = Read_product( &buf, LB_NEXT, DRIVING, buffer_ind, 
                              &return_status );

         /* A size greater than 0 indicates successful read. */
         if( size > 0 )
            break;

	 if( size == LB_TO_COME ){	

            /* The message is not currently available. */
	    msleep (WAIT_TIME);
	    continue;

	 }

	 /* The message expired.  Do not wait to read next input otherwise
            we may never catch up.  Seek to the first message in the LB. */
	 else if( size == LB_EXPIRED ){

            return_status = ORPGDA_seek( Inp_list[DRIVING].data_id, 0, 
                                         LB_FIRST, NULL );
            
            /* If seek is successful, attempt to re-read driving input.
               Otherwise, terminate. */
            if( return_status == LB_SUCCESS )
	       continue;

            PS_task_abort( "Driving Input (%d) Read Failed (%d) (data_id: %d, LB_NEXT)\n",
                         Inp_list[DRIVING].type, size, Inp_list[DRIVING].data_id );
         }

         /* Check if replay request wishing warehoused data out of 
            the product database. */
         else if( (size == LB_NOT_FOUND)
                   &&
             (Input_stream == PGM_REPLAY_STREAM)
                   && 
             (!Inp_list[DRIVING].wait_for_data) ){

            /* Requested data not in product database. */
            return (NO_DATA);

         }

         /* Some fatal error occurred. */
         else if( size <= 0 )
            PS_task_abort( "Driving Input (%d) Read Failed (%d) (data_id: %d, LB_NEXT)\n",
                         Inp_list[DRIVING].type, size, Inp_list[DRIVING].data_id );

      /* End of "while" loop. */
      }

      /* Input read, so return NORMAL to caller. */
      return (NORMAL);

   }

/* End of IB_read_driving_input() */
}

/***********************************************************************

   Description:
      This function is responsible for querying the product database
      for the input specified by "ind".  If the specified data is available,
      the message ID of the data is returned.

   Inputs:
      ind - index into Inp_list for the data type to be queried.

   Outputs:

   Returns:
      0 if data is not found in the database, or a valid message ID if
      data is found.

   Notes:
      The task terminates if queried data type has neither a product code
      associated with it or is not a warehoused type.

***********************************************************************/
LB_id_t IB_product_database_query( int ind ){

   RPG_prod_rec_t db_info;
   ORPGDBM_query_data_t query_data[4];
   int ret; 
   Replay_req_info_t *replay_info;
   unsigned int vol_seq_num;

   int query_fields = 0;
   int passes = 0;
   int prod_code = 0;
   int is_warehoused = 0;

   /* Get the database query information. */
   replay_info = WA_get_query_info();

   /* Verify that the replay data pointer is not NULL. */
   if( replay_info == NULL ){

      LE_send_msg( GL_INFO, "WA_get_query_info Returned NULL Pointer\n" );
      return (0);

   }

   /* Save the volume sequence number in request in the event the requested
      volume scan data is not available. */
   vol_seq_num = replay_info->replay_vol_seq_num;

   /* Initialize the query data. */
   query_data[query_fields].field = RPGP_VOLT;
   query_data[query_fields].value = replay_info->replay_vol_time;
   query_fields++;

   /* If the product is warehoused, set the warehoused query field. */
   if( (is_warehoused = ORPGPAT_get_warehoused( Inp_list[DRIVING].data_id )) > 0 ){

      query_data[query_fields].field = RPGP_WAREHOUSED;
      query_data[query_fields].value = Inp_list[ind].data_id;
      query_fields++;

   }

   /* If the product has an associated product code, set the product code
      query field. */
   if( (prod_code = ORPGPAT_get_code( Inp_list[ind].data_id )) > 0 ){

      query_data[query_fields].field = RPGP_PCODE;
      query_data[query_fields].value = prod_code;
      query_fields++;

   }

   if( replay_info->replay_elev_ang >= 0 ){

      query_data[query_fields].field = RPGP_ELEV;
      query_data[query_fields].value = replay_info->replay_elev_ang;
      query_fields++;

   }

   passes = 0;

   /* Do for the appropriate number of passes.  For alert-paired product
      requests, only one pass since the volume sequence number in the
      request matches the volume sequence number of the volume in which 
      the alert condition was detected.  Otherwise, make two passes.  Once 
      for the current volume scan and once for the previous volume scan. */
   while(1){

      unsigned int vol_time;

      /* Increment the number of passes. */
      passes++;

      /* Query the product database for the product. */
      ret = ORPGDBM_query( &db_info, query_data, query_fields, 1 );

      /* A return value > 0 indicates the query was successful (i.e., product
         found in the database. */
      if( ret > 0 ){

         /* Verify that the returned data is what we wanted. */
         if( db_info.vol_t != replay_info->replay_vol_time ){

            LE_send_msg( GL_ERROR, "Data Base Volume Time (%d) != Request Volume Time (%d)\n",
                         db_info.vol_t, replay_info->replay_vol_time ); 
            return (0);

         }

         if( (is_warehoused > 0) && (db_info.warehoused != Inp_list[ind].data_id) ){

            LE_send_msg( GL_ERROR, "Data Base Warehoused (%d) != Inp_list[ind].data_id\n",
                         db_info.warehoused, Inp_list[ind].data_id );
            return (0);

         }

         if( (prod_code > 0) && (db_info.prod_code != prod_code) ){

            LE_send_msg( GL_ERROR, "Data Base Product Code (%d) != Product Code (%d)\n",
                         db_info.prod_code, prod_code );
            return (0);

         }

         /* Data return. */
         return( db_info.msg_id );

      }

      if( replay_info->replay_type == ALERT_OT_REQUEST )
         break;
 
      /* For USER_OT_REQUEST and DRIVING input, we need to check the previous
         volume scan.  For USER_OT_REQUEST and non-DRIVING input, the volume
         sequence number has already been set correctly when the DRIVING input
         was acquired. */
      if( passes < 2 && ind == DRIVING ){

         /* Set the volume sequence number to the previous volume unless
            the volume sequence number becomes negative, then break out of while 
            loop. */
        replay_info->replay_vol_seq_num--;
        if( replay_info->replay_vol_seq_num < 0 )
           break;
 
        if( (vol_time = SS_get_volume_time( replay_info->replay_vol_seq_num )) == 0 )
           break;

        query_data[0].value = vol_time;
        replay_info->replay_vol_time = vol_time;

        /* Go to top of "while" loop. */
        continue;

      }
         
      /* Break out of "while" loop. */
      break;
 
   } /* End of "while" loop. */

   /* No data return. */
   replay_info->replay_vol_seq_num = vol_seq_num;
   return (0);

/* End of IB_product_database_query() */
}
 
/********************************************************************

   Description: 
      This function reads a data from an input other than the driving 
      input. For the PGM_REALTIME_stream input data stream, the data 
      read must be synchronized with the data time and volume sequence 
      number. If the data is not yet available, it waits until the data 
      is available. If the data is not going to be available, the function 
      returns either TERMINATE or NO_DATA.  We consider that the data 
      is not going to be available:

	 1) if input timing is VOLUME_BASED, ELEVATION_BASED, or
	    DEMAND_BASED and input is not optional
 
	   a)  data time does not match and 
           b)  volume sequence number is greater than the
               driving input volume sequence number.

	 2) if input is optional, we have waited a specific 
            period of time. 

         3) if input timing is TIME_BASED and the other input
            is not received within a time specific time window.

	 4) if a new driving input is received.

      For the PGM_REPLAY_STREAM, Read_other_input_replay_stream() is
      called.

      If the data timing between the driving input and other input
      are different, we synchronize the data time to the other
      inputs timing.  We assume (but not impose) that the driving
      input timing is either of the same update frequency as 
      the other input or has a greater update frequency.  For
      example, if one input is ELEVATION_DATA and the other input
      is VOLUME_DATA, then the ELEVATION_DATA should be the driving
      input.  The volume time and volume sequence number are 
      compared for match in this example. 

   Input:	
      ind - index into input data structure for this input
      buffer_ind - The index of the input data buffer

   Outputs:

   Return:	
      The function returns NORMAL on success, or either TERMINATE
      or NO_DATA on error. 

   Notes:

********************************************************************/

#define LIST_SIZE	20	/* the number of latest data we check */
#define MAX_PASSES      20      /* Number of passes to wait for non-DRIVING
                                   input for Replay Requests. */

static int Read_other_input( int ind, int *buffer_ind ){

   time_t driving_time;
   unsigned int cum_t, driving_volume;
   int driving_elev_index;

   /* Initialize pointers used in this module. */
   In_data_type *idata = &Inp_list[ind]; 

   /* Driving input must always be the first input read. */
   if (Driving_read == FALSE)
      PS_task_abort( "GET_INBUF Must First Read The Driving Input.\n");

   /* Initialize buffer_ind to invalid value. */
   *buffer_ind = -1;

   /* For the replay data stream, ....... */
   if( Input_stream == PGM_REPLAY_STREAM )
      return( Read_other_input_replay_stream( ind, buffer_ind ) );

   /* If here, assumes the data stream is real-time. */

   /* Get the driving input volume sequence number. */
   driving_volume = Inp_list[DRIVING].vol_num;

   /* Calculate driving time for comparison with other input.  
      NOTE:  The driving and test time are based on the timing
             of the other input in order that we perform the 
             correct time match when the data timings are not
             the same. */
   driving_time = Inp_list[DRIVING].time + 1;
   if( idata->timing == ELEVATION_DATA )
      driving_time = Inp_list[DRIVING].elev_time;

   else if( idata->timing == VOLUME_DATA )
      driving_time = Inp_list[DRIVING].vol_time;

   driving_elev_index = Inp_list[DRIVING].rpg_elev_index;

#ifdef DEBUG
   LE_send_msg( GL_INFO, "--->Driving Volume: %d, Driving Time: %d, Driving Elevation Index: %d\n",
                driving_volume, driving_time, driving_elev_index );
   if( idata->timing == ELEVATION_DATA )
      LE_send_msg( GL_INFO, "------>Driving Time is Elevation Start Time\n" );
   else
      LE_send_msg( GL_INFO, "------>Driving Time is Volume Scan Start Time\n" );
#endif

   /* Initialize cumulative time.  This is used for optional 
      inputs. */
   cum_t = 0;	

   /* Do Forever ... */
   while (1) {

      LB_info other_list [LIST_SIZE];
      unsigned int other_volume;
      int other_nlist, other_elev_index, i;
      time_t other_time = 0;
      LB_id_t id = 0xffffffff;

      /* Get a list of the latest available data.  Abort on error. */
      other_nlist = ORPGDA_list (idata->data_id, other_list, LIST_SIZE);
      if (other_nlist < 0)
         PS_task_abort( "ORPGDA_list Failed (%d) (data_id: %d)\n", 
                        other_nlist, idata->data_id );

      /* Do For All available inputs of this type ..... */
      for( i = other_nlist - 1; i >= 0; i-- ){

         /* Get the product time and volume scan sequence number
            for this message id */
         other_time = Get_product_time( other_list[i].id, ind, 
                                        idata->timing, &other_volume,
                                        &other_elev_index );
                
         /* If other input timing is not time based), verify other input is 
            from the correct elevation/volume. */
         if( idata->timing != TIME_DATA ){

            /* This test will fail if the product is not currently 
               available.  Note:  The volume scan number must match.
               If an elevation product, the elevation index must match.  
               If not an elevation product, the times must match.  */
            if( other_volume == driving_volume ){

               if( Check_elevation_index ){

                  if( other_elev_index == driving_elev_index )
                     id = other_list[i].id;

               }
               else if( other_time == driving_time )
                  id = other_list[i].id;

               if( id != 0xffffffff ){
#ifdef DEBUG
                  LE_send_msg( GL_INFO, "--->Match on input -- ID: %d\n", id );
#endif 
                  /* Break out of "for" loop. */
	          break;

               }

	    }

#ifdef DEBUG
            LE_send_msg( GL_INFO, "--->No Match Yet -- other_time: %d, driving_time: %d\n",
                         other_time, driving_time );
            LE_send_msg( GL_INFO, "------>other_volume: %d, driving_volume: %d\n",
                         other_volume, driving_volume );
            LE_send_msg( GL_INFO, "------>other_elev_index: %d, driving_elev_index: %d\n",
                         other_elev_index, driving_elev_index );
#endif 

         }

         /* If other input timing is TIME_DATA, timing value specifies 
            a time window.  For time window, we currently ignore the 
            volume scan sequence number. */
         else{	

            /* Only data with earlier time and within time window 
               is accepted. */
	    if( (other_time >= 0) && (other_time <= driving_time)
                                  && 
		(other_time >= (driving_time - idata->time)) ){

	       id = other_list[i].id;

               /* Break out of "for" loop. */
	       break;

	    }

	 }

      /* End of "for" loop. */
      }

      /* No data is found, so either wait for data to arrive or 
         return error. */
      if( id == 0xffffffff ){	

         /* Check the latest message.  Give up if new data has come. */
         if( other_nlist > 0 ){

            other_time = Get_product_time( other_list[other_nlist-1].id, 
                                           ind, idata->timing, &other_volume,
                                           &other_elev_index );

            /* If data timing is not TIME_DATA, must match time and volume 
               sequence number of driving input. */
            if( idata->timing != TIME_DATA ){

               if( ((other_time != driving_time)
                                ||
                    (Check_elevation_index && (other_elev_index != driving_elev_index)))
                               && 
                    (other_volume > driving_volume) ){

                  /* Block time >= 0 only for optional inputs. */
                  if( idata->block_time < 0 ){

                     /* Data with a later time and volume scan sequence
                        number has arrived.  Return NO_DATA. */
                     LE_send_msg( GL_INFO, "Newer Other Input %d Arrived!\n",
                               idata->type );

#ifdef DEBUG
                     LE_send_msg( GL_INFO, "--->other_time: %d, driving_time: %d\n",
                                  other_time, driving_time ); 
                     LE_send_msg( GL_INFO, "--->other_volume: %d, driving_volume: %d\n",
                                  other_volume, driving_volume ); 
#endif

		     return (NO_DATA);

                  }

               }

            }

         }

         /* If input is optional, and the cumulative time exceeds the
            time period specified for waiting, return NO_DATA. */
	 if( idata->block_time >= 0 ){

            if( cum_t >= idata->block_time*1000 ){

               LE_send_msg( GL_INFO, "Optional Input %d Abandoned\n", idata->type); 
               return (NO_DATA);

            }
            else if( cum_t == 0 ){

               int status, prod_stat_len = 0;
               char *prod_status_hdr = NULL;

               /* Check the product status.  If this product is not coming owing to task
                  failure or not being configured, then return NO_DATA immediately instead
                  of waiting for block_time to expire. */
               prod_stat_len = ORPGDA_read( ORPGDAT_PROD_STATUS, (char *) &prod_status_hdr,
                                            LB_ALLOC_BUF, PROD_STATUS_MSG );
               if( prod_stat_len >=  ALIGNED_SIZE(sizeof(Prod_gen_status_header)) ){

                  status = Check_prod_status( prod_status_hdr, prod_stat_len,
                                              driving_volume, (int) idata->type );
                  free( prod_status_hdr ); 

                  if( (status == PGS_TASK_FAILED) 
                              || 
                      (status == PGS_TASK_SELF_TERM) 
                              || 
                      (status == PGS_TASK_NOT_CONFIG) ){

                     
                     LE_send_msg( GL_INFO, "Optional Input %d Abandoned (Status = %d)\n", 
                                  idata->type, status ); 
                     return (NO_DATA);

                  }

               } 

            }

         }

         /* Wait a short time before trying again. */
	 msleep (WAIT_TIME);

         /* If input is mandatory (i.e., block time < 0 ), then 
            block until the next input is available. */
	 if( (idata->block_time < 0) && (other_nlist > 0) ){

	    LB_info other_info;

            /* Required data is not yet available so move LB read
               pointer to 1 message past latest available message. */
	    ORPGDA_seek( idata->data_id, 1, 
                         other_list[other_nlist-1].id, &other_info );

	 }

	 cum_t += WAIT_TIME;
	 continue;

      }

      /* Data is available, so read it. */
      else{	

	 int ret, return_status;
         char *buf;

         /* Read the product. */
	 if( (ret = Read_product( &buf, id, ind, buffer_ind, &return_status )) <= 0 ){

            /* Some error occurred. */
            if( ret == LB_NOT_FOUND ){

               /* Buffer not found. This should never happen unless the product
                  was shed out of the product data base.  Return NO_DATA. */
               return( NO_DATA );

            }
            else
               PS_task_abort( "Other Input Read Of %d Failed (%d) (data_id: %d, msg_id: %d)\n", 
                              idata->type, ret, Inp_list[ind].data_id, id );

	 }
         else
            return (return_status);

      }

   /* End of "while" loop. */
   }

   return(NORMAL);

/* End of Read_other_input() */
}

/******************************************************************************

   Description:
      Searches the product generation status for match on Product ID and 
      volume scan sequence number.  If match found, returns the status.

   Inputs:
      prod_status_hdr - pointer to the product status header.
      prod_status_len - length of product status, in bytes.
      volume - volume scan sequence number for the product status.
      prod_id - product ID of product we wish to find in product status.

   Outputs:

   Returns:
      Product generation status for volume scan of interest, if product found,
      or PGS_GEN_OK otherwise.

   Notes:

******************************************************************************/
static int Check_prod_status( char *prod_status_hdr, int prod_stat_len, 
                              unsigned int volume, int prod_id ){

   int v_index, p_index, num_prods, status = PGS_GEN_OK;
   Prod_gen_status_header *pgs_hdr = (Prod_gen_status_header *) prod_status_hdr;
   char *buf = prod_status_hdr + ALIGNED_SIZE(sizeof(Prod_gen_status_header));
   Prod_gen_status *pgs = (Prod_gen_status *) buf;

   /* Search for match on volume number. */
   for( v_index = 0; v_index < pgs_hdr->vdepth; v_index++ ){

      if( pgs_hdr->vnum[v_index] == volume )
         break;

   }

   /* No match found.... */
   if( v_index >= pgs_hdr->vdepth )
      return( status );

   /* Determine the number of products advertised in Product Status. */
   num_prods = (prod_stat_len - ALIGNED_SIZE(sizeof(Prod_gen_status_header)))/
               ALIGNED_SIZE(sizeof(Prod_gen_status));

   /* Search product status for match on product ID.  If match found,
      return the status. */
   for( p_index = 0; p_index < num_prods; p_index++ ){

      if( pgs[p_index].prod_id == prod_id )
         return( pgs[p_index].msg_ids[v_index] );

   }

   return( status );

/* End of Check_prod_status() */
}

/******************************************************************************

   Description:
      For the PGM_REPLAY_STREAM, if the data is warehoused (i.e.,
      Inp_list[ind].wait_for_data == FALSE), then the data is read and 
      if not available, the module returns to caller immediately with
      NO_DATA.  If the data is not warehoused, then we continue to try
      and read the data until available or a fatal read error occurs.

      If data is available, compares the the "driving_time" and other
      inputs time "test_time" to see if the inputs are from the same
      elevation or volume.  Also validates the volume sequence number.

   Input:	
      ind - index into input data structure for this input
      buffer_ind - The index of the input data buffer

   Outputs:

   Return:	
      The function returns NORMAL on success, or negative value on
      error. 

   Notes:

******************************************************************************/
static int Read_other_input_replay_stream( int ind, int *buffer_ind){

   In_data_type *idata = &Inp_list[ind];
   int size, passes, return_status;
   char *buf;

   passes = 0;
   while(1){

      /* Read the product.  If data resides in the product database,
         the message ID is ignored. */
      size = Read_product( &buf, LB_NEXT, ind, buffer_ind, &return_status );
      if( (size == LB_NOT_FOUND) || (size == LB_TO_COME) ){

         if( !idata->wait_for_data ) 
            break;

         else{

            /* We need a way to break out of this loop in case the product
               never arrives.  Thus we make at most MAX_PASSES before we
               quit. */
            passes++;
            if( passes >= MAX_PASSES )
               break;
 
            msleep(500);
            continue;
 
         }

      }
      else if( size == LB_EXPIRED ){

         int ret;

         LE_send_msg( GL_ERROR, "Other Input %d at Read Pointer Expired!\n", 
                      idata->data_id );
         LE_send_msg( GL_INFO, "--->Seek Read Pointer To First Message in LB\n" );

         /* Position read pointer to first message in LB.  Start reading from there!! */
         if( (ret = ORPGDA_seek( idata->data_id, 0, LB_FIRST, NULL )) == LB_SUCCESS )
            continue;

         /* Terminate if "seek" fails. */
         PS_task_abort( "Other Input Seek Failed (%d) (data_id: %d, LB_FIRST)\n", 
                        ret, Inp_list[DRIVING].data_id );

      }
      else if( size < 0 )
         PS_task_abort( "Other Input Read Failed (%d) (data_id: %d, LB_NEXT)\n", 
                        size, Inp_list[DRIVING].data_id );

      /* If here and return status is NORMAL, verify the other input is for the 
         correct volume and elevation, if elevation-based. */ 
      if( return_status == NORMAL ){

         Prod_header *phd = (Prod_header *) idata->buf[*buffer_ind]; 
         time_t driving_time = Inp_list[DRIVING].time;
         unsigned int driving_volume = Inp_list[DRIVING].vol_num;
         time_t test_time = 0;
         int driving_elev_index, test_elev_index = phd->g.elev_ind;


         /* Set the other input "test_time" */
         if( idata->timing == VOLUME_DATA )
            test_time = phd->g.vol_t;

         else if( idata->timing == ELEVATION_DATA )
            test_time = phd->elev_t;

         /* Set the "driving_time" */
         if( Inp_list[DRIVING].timing == ELEVATION_DATA ){

            if( idata->timing == VOLUME_DATA )
               driving_time = Inp_list[DRIVING].vol_time;

            else if( idata->timing == ELEVATION_DATA )
               driving_time = Inp_list[DRIVING].time;

         } 
         else if( Inp_list[DRIVING].timing == VOLUME_DATA ){

            if( idata->timing == VOLUME_DATA )
               driving_time = Inp_list[DRIVING].time;

            else if( idata->timing == ELEVATION_DATA )
               driving_time = Inp_list[DRIVING].elev_time;

         }

         driving_elev_index = Inp_list[DRIVING].rpg_elev_index;

         /* Validate whether data are from the correct volume/elevation. */
         if( (((test_time != 0) && (test_time != driving_time)) 
                               ||
               (Check_elevation_index && (test_elev_index != driving_elev_index)))
                               ||
             (phd->g.vol_num != driving_volume) ){

            LE_send_msg( GL_INFO, 
               "test_time != driving_time (%d, %d) or test_vol != driving_volume (%d, %d)\n",
               test_time, driving_time, phd->g.vol_num, driving_volume );

            IB_release_input_buffer( ind, *buffer_ind );
            continue;

         }

      }

      break;

   /* End of "while" loop */
   }

   return (return_status);

/* End of Read_other_input_replay_stream() */
}

/********************************************************************

   Description: 
      This function reads the next available data from any of its 
      input. 

    Inputs:	 mem - the MEM array

    Outputs:	 bufptr - index in MEM for the first element of the buf;
		 datatype - returns the data type.
		 opstat - status as defined in a309.h.

    Return:	 The function returns 0 which is not used.

********************************************************************/
static int Get_inbuf_any (int *mem, int *bufptr, int *datatype, int *opstat){

   int ind, size, buffer_ind;
   Prod_header *phd = NULL;
   char *buf;

   static unsigned int last_vol_num_processed = 0;

   /* Initialize "opstat" to NO_DATA. */
   *opstat = NO_DATA;

   /* Get next available input. */
   if( (ind = WA_get_next_avail_input()) < 0 ){

      /* If return value is negative, search Inp_list for available input.
         Process prior volume scans first. */
      for( ind = 0; ind < N_inps; ind++ ){

         if( (Inp_list[ind].must_read > 0)
                        && 
             (Inp_list[ind].vol_num < last_vol_num_processed) )
            break;

      }
      
      /* No inputs from previous volume, so process current volume. */
      if( ind >= N_inps ){

         for( ind = 0; ind < N_inps; ind++ ){

            if( Inp_list[ind].must_read > 0 )
               break;

         }

      }

   }
 
   /* If no input found, programming error. */
   if( ind >= N_inps )
      PS_task_abort( "Get_input_any Could Not Find Input\n" );

   /* Set the volume scan processed. */
   if( Inp_list[ind].vol_num > last_vol_num_processed ){

      last_vol_num_processed = Inp_list[ind].vol_num;
      if( Message_mode )
         PS_message( "Processing Data For Volume Scan %d\n", 
                     last_vol_num_processed );

   }

   /* Read the input buffer. */
   size = Read_product( &buf, LB_NEXT, ind, &buffer_ind, opstat );

   /* Set flag indicating this input was read. */
   Inp_list[ind].must_read--;
   *datatype = Inp_list[ind].type;

   /* If error returned from read indicates LB message no longer available
      (LB_EXPIRED) or LB not available (LB_TO_COME), then return NO_DATA.
      The first error condition might happen if the LB is sized too small,
      the second might arise if the LB message read was not of the same
      input data stream. */
   if( (size == LB_EXPIRED) || (size == LB_TO_COME) )
      return( NO_DATA );

   else if( size < 0 )
      PS_task_abort( "ORPGDA_read (ANY_TYPE) Failed (%d) (data_id: %d, LB_NEXT)\n", 
                      size, Inp_list[ind].data_id );


   /* Check if input is an abort message. */
   if( *opstat == NORMAL ){	

      phd = (Prod_header *) buf;
      *opstat = Check_for_abort_message( phd, ind );
         
      /* If size of buffer read is size of product header, and *opstat
         comes back as NORMAL, report error and terminate. */
      if( (size == sizeof(Prod_header)) && (*opstat == NORMAL) )
         PS_task_abort( "Product Length Received %d But Not Abort Message\n",
                        size );

      /* Abort processing if return status is not NORMAL. */
      if( *opstat != NORMAL ){

         if( ((Input_stream == PGM_REALTIME_STREAM) && (*opstat != NO_DATA))
                                      ||
             (Input_stream == PGM_REPLAY_STREAM) ){

            /* Set the volume scan sequence number for the aborted volume
               scan and then abort outputs. */
            AP_set_aborted_volume( phd->g.vol_num );
            RPG_abort_processing( &phd->g.len );

         }

         /* Release the acquired input buffer. */
         IB_release_input_buffer( ind, buffer_ind );

         /* Return status of operation. */
         return( *opstat );

      }

   }

   /* Check if this task needs this data.  If data is not needed, return
      NO_DATA. */
   if( WA_data_filtering( Inp_list + ind, buffer_ind ) != DATA_NEEDED ){

      if( Message_mode )
         PS_message( GL_INFO, "Data Type %d Not Needed By This Application\n", 
                     *datatype );
      IB_release_input_buffer( ind, buffer_ind );
      return(NO_DATA);

   }

   /* If all is still NORMAL, then set buffer pointer, set the 
      data to be returned, and return NORMAL. */
   if( *opstat == NORMAL ){            

      /* This is the all normal case.  Set and register the buffer
         pointer (index). */
      Set_buffer_pointer( mem, ind, buffer_ind, bufptr );

      /* Read all input ITCs which were registered to be updated
         with the receipt of "datatype" */
      ITC_read_all (*datatype);

      if( Message_mode )
         PS_message( "Product %d For Volume %d Returned To Application\n",
                      phd->g.prod_id, phd->g.vol_num );
      return( NORMAL );

   }

   /* If control reaches here, return NO_DATA. */
   return (NO_DATA);

/* End of Get_inbuf_any() */
}

/********************************************************************

    Description: This function returns the product time and volume 
		 sequence number based on message id.  The product times 
		 and volume sequence numbers are stored in the
		 product_time_list.  If the id is not found in the
                 list, the product header must be read from the 
                 linear buffer to get the time.

    Inputs:	id - the message id of the message in question
                ind - index into Inp_list associated with input
                timing - the other input timing 

    Outputs:    vol_seq_num - volume sequence number
                elev_index - elevation index

    Return:	The function returns the generation time of the 
                product.  This is either the elevation time or the
                volume time.  On error, the invalid time -1 is 
                returned.

    Notes:      The timing of the other input is needed when "ind"
                corresponds to the DRIVING input.  In this case,
                we need to pass the appropriate time back to the 
                caller in order to perform time match.  For example,
                if other input is VOLUME_DATA but the driving input
                is ELEVATION_DATA, we want to return the volume time
                of the driving input.

********************************************************************/
static time_t Get_product_time( LB_id_t id, int ind, int timing,
                                unsigned int *vol_seq_num,
                                int *elev_index ){

   int ret, slot, i;
   Prod_header phdr;
   LB_id_t minimum_id;

   Product_time_t *product_time_list;
   In_data_type *idata = &Inp_list[ind];

   /* Set pointer to input time list used in this module. */
   product_time_list = Other_input_time_list[ind];

   /* Search the LB info list and the product time list for a 
      match on id.  If id match found, return time. */ 
   for( i = 0; i < TIME_LIST_SIZE; i++ ){
     
      if( id == product_time_list[i].id ){

         *vol_seq_num = product_time_list[i].vol_num;
         *elev_index = product_time_list[i].rpg_elev_index;
         return( product_time_list[i].time );

      }

   }

   /* No match found.  Must read the product header in the linear
      buffer message. */
   ret = ORPGDA_read( idata->data_id, (char *) &phdr, 
                      sizeof( Prod_header ), id );
   if( ret < 0 && ret != LB_BUF_TOO_SMALL )
      return(-1);

   /* Search the product_time_list for a slot to place the id and 
      time of the linear buffer just read. */
   minimum_id = product_time_list[0].id;
   slot = 0;
   for( i = 0; i < TIME_LIST_SIZE; i++ ){

      /* Found empty slot. */
      if( product_time_list[i].id == -1 )
         break;

      if( product_time_list[i].id < minimum_id ){

         /* Save slot number with smallest id so far. */
         minimum_id = product_time_list[i].id;
         slot = i;

      }

   } 

   /* There must have been an empty slot. */
   if( i < TIME_LIST_SIZE )
      slot = i;

   /* Save the id in the product_time_list slot. */
   product_time_list[slot].id = id;
         
   /* Set product time based on whether the input timing is
      elevation data or other. */
   if( timing == ELEVATION_DATA )
      product_time_list[slot].time = phdr.elev_t;
     
   else if( timing == VOLUME_DATA )
      product_time_list[slot].time = phdr.g.vol_t;

   /* Set the product volume scan sequence number. */
   product_time_list[slot].vol_num = phdr.g.vol_num;
   *vol_seq_num = product_time_list[slot].vol_num;
   
   /* Set the RPG elevation index. */
   product_time_list[slot].rpg_elev_index = phdr.g.elev_ind;
   *elev_index = product_time_list[slot].rpg_elev_index;
   
   return( product_time_list[slot].time );

/* End of Get_product_time() */
}

/********************************************************************

    Description: 
        This function reads the product with msg_id from. If product 
        is link to product data base, reads the product from the product 
        data base.  If some error occurs, returns status of ORPGDA_read call.
        The return_status contains some amplification data.

        If everything is normal, registers the buffer in Inp_list.

    Inputs:	
        buf - pointer to pointer to receive input buffer.
        msg_id - ID of message to read from product LB.
        idata - pointer to Inp_list entry for this product.
        ind - index into Inp_list for this product.

    Outputs:
        buf - pointer to pointer receiving input buffer.
        buffer_ind - index of input buffer in Inp_list.  This is the 
                     index of the buffer if multiple buffers of this
                     type are acquired by caller.
        return_status - amplification data for buffer read.  Possible 
                        values are TERMINATE, NO_DATA, or NORMAL.


    Return:	
        Returns value of ORPGDA_read.  If product in product database,
        returns and product header of product in product LB does not
        match fields of product header in product data base, returns
        LB_NOT_FOUND.       

********************************************************************/
static int Read_product( char **buf, LB_id_t msg_id, int ind, 
                         int *buffer_ind, int *return_status ){

   Prod_header *phd;  
   char *dest = NULL;
   int status, size, decompr_size = 0;

   In_data_type *idata = &Inp_list[ind];

   /* Initialize the buffer index to -1 and the "return status"
      to NORMAL. */
   *buffer_ind = -1;
   *return_status = NORMAL;

   while(1){

      /* Initialize buf pointer to NULL */
      *buf = NULL;

      /* For the real-time data stream or for the replay data stream 
         where the product is not warehoused, read the product from the
         product LB.  Otherwise, read the product from the product
         database. */
      if( Input_stream == PGM_REALTIME_STREAM 
                       ||
          (Input_stream == PGM_REPLAY_STREAM && Inp_list[ind].wait_for_data) ){
 
         /* Read the message pointed to by msg_id. */ 
         size = ORPGDA_read( idata->data_id, buf, LB_ALLOC_BUF, msg_id );

      }
      else{

         /* Query the database for the product. */
         msg_id = IB_product_database_query( ind );
         if( msg_id != 0 ){

            /* Read the message from the product database pointed to by msg_id. */ 
            size = ORPGDA_read( ORPGDAT_PRODUCTS, buf, LB_ALLOC_BUF, msg_id );

            /* NOTE: If size indicates message read, we may want to validate 
                     message. */

         }
         else{

            /* The product is not available.  Return NO_DATA. */
            *return_status = NO_DATA;
            return( LB_NOT_FOUND );

         }

      }

      /* A negative size value indicates the read failed. */
      if( size < 0 ){

         *return_status = NO_DATA;
         return ( size );

      }

      phd = (Prod_header *) *buf;  
      
      /* Check input stream in product header against task's input
         stream.  Ignore messages which have different input streams unless
         the product is a warehoused type.   Warehoused products are generated
         off the real-time stream.

         NOTE: In the future, may need to change this to accept product
               codes.  Products with product codes will not be generated
               with the correct input stream. */
      if( (phd->g.input_stream != Input_stream) 
                        &&
          (ORPGPAT_get_warehoused( phd->g.prod_id ) <= 0) ){

         free( *buf );
         continue;

      }

      /* Break out of "while" loop. */
      break;

   }

   /* Check if the product was compressed.  If compressed, need to decompress
      the product. */
   if( (ORPGPAT_get_compression_type( phd->g.prod_id ) > 0) 
                               &&
       (idata->timing != RADIAL_DATA) ){

      RPG_decompress_product( *buf, &dest, &decompr_size, &status ); 
      if( status < 0 )
         *return_status = NO_DATA;

      /* If the source and destination buffers have the same address,
         most likely the input buffer was an Abort Message. */
      if( *buf != dest ){

         free( *buf );
         *buf = dest;

      }

      size = decompr_size;

   }

   /* Product read successfully.  Update Inp_list. */
   Register_input_buffer( *buf, size, ind, buffer_ind );

   return( size );

/* End of Read_product() */
}

/*****************************************************************
   Description:
      Register an input buffer in Inp_list.

   Inputs:
      buf - address of input buffer.
      size - size, in bytes, of buffer.
      ind - index into Inp_list.
      buffer_ind - pointer to int to receive index
                   of buffer in Inp_list[ind].buf

   Outputs:
      buffer_ind - pointer to int receiving index
                   of buffer in Inp_list[ind].buf

   Returns:
  
   Notes:
      Process terminates if too many input buffers 
      acquired.

*****************************************************************/
static void Register_input_buffer( char *buf, int size,
                                   int ind, int *buffer_ind ){

   int i;

   /* Set buffer size, in bytes. */
   Inp_list[ind].len = size;
 
   /* Search for a free slot in Inp_list to track buffer. */
   for( i = 0; i < MAXN_IBUFS; i++ )
      if( Inp_list[ind].buf[i] == NULL )
         break;
 
   if( i < MAXN_IBUFS ){

      /* Free slot available.  Save the buffer pointer. */
      *buffer_ind = i;
      Inp_list[ind].buf[i] = buf;

      /* Increment the count of buffers of this type that are 
         being tracked. */
      if( i >= Inp_list[ind].buffer_count )
         Inp_list[ind].buffer_count = i; 
         
   }
   else
      PS_task_abort( "Too Many Input Buffers Of Type %d Acquired!\n",
                     Inp_list[ind].type );

/* End of Register_input_buffer() */
}

/***************************************************************
   Description:
      Set the buffer pointer (index into MEM) returned back to
      the application.

   Inputs:

   Outputs:

   Returns:

   Notes:

***************************************************************/
static void Set_buffer_pointer( int *mem, int ind, int buffer_ind,
                                int *bufptr ){

   /* Set the pointer relative to the start of mem where the data 
      resides.  Radial inputs do not have a orpg product header. */
   if (Inp_list [ind].timing == RADIAL_DATA)
      *bufptr = (int *)(Inp_list [ind].buf[buffer_ind]) - mem + 1;

   else
      *bufptr = (int *)(Inp_list [ind].buf[buffer_ind] + 
                sizeof (Prod_header)) - mem + 1;

   /* Register the buffer pointer in Inp_list. */
   Inp_list [ind].bufptr[buffer_ind] = *bufptr;

/* End of Set_buffer_pointer() */
}

/*****************************************************************

   Description:
      This module checks if the product just read is an abort 
      message.  If yes, then the abort reason is set, and an
      "opstat" value for the abort reason is returned to the caller.

   Inputs:
      phd - pointer to product header.
      ind - index into Inp_list for this data type.

   Outputs:

   Returns:
      "opstat" value for the abort reason, or NORMAL otherwise.

   Notes:

*****************************************************************/
static int Check_for_abort_message( Prod_header *phd, int ind ){

   int ret = NORMAL;

   /* A negative length in the product header indicates an
      abort message. */
   if( phd->g.len < 0 ){

      /* Set the abort reason code.  This is needed so when the 
         task receives the "opstat", the abort processing knows 
         the reason for the abort. */
      AP_set_abort_reason( phd->g.len );

#ifdef RPG_LIBRARY

      /* Set the return value based on abort reason. */ 
      switch( phd->g.len ){

         case PGM_CPU_LOADSHED:
         case PGM_SCAN_ABORT:
         case PGM_INPUT_DATA_ERROR:
         {
             ret = TERMINATE;
             break;
         }
         case PGM_MEM_LOADSHED:
         case PGM_DISABLED_MOMENT:
         {
             ret = NO_DATA;
             break;
         }
         case PGM_TASK_FAILURE:
         {
            ret = FAULT_ME;
            break;
         }
         default:
         {
            ret = TERMINATE;
            break;
         }

      /* End of "switch" statement. */
      }

#else
#ifdef RPGC_LIBRARY

      /* For processes which "WAIT_DRIVING_INPUT" and the input is driving 
         or non-driving not optional input, set the return values 
         such that most errors return TERMINATE. */
      if( WA_wait_driving() && 
          ((ind == DRIVING) || (Inp_list[ind].block_time < 0)) ){

         /* Set the return value based on abort reason. */ 
         switch( phd->g.len ){

            case PGM_CPU_LOADSHED:
            case PGM_SCAN_ABORT:
            case PGM_INPUT_DATA_ERROR:
            case PGM_TASK_FAILURE:
            case PGM_TASK_SELF_TERMINATED:
            default:
            {
                ret = TERMINATE;
                break;
            }
            case PGM_MEM_LOADSHED:
            case PGM_DISABLED_MOMENT:
            {
                ret = NO_DATA;
                break;
            }
   
         /* End of "switch" statement. */
         }

      }
      else{

         /* For Optional non-driving inputs, or inputs for WAIT_ANY_INPUT 
            type tasks, set the return value such that most errors return 
            NO_DATA. */ 
         switch( phd->g.len ){

            case PGM_SCAN_ABORT:
            case PGM_INPUT_DATA_ERROR:
            {
                ret = TERMINATE;
                break;
            }
            case PGM_MEM_LOADSHED:
            case PGM_DISABLED_MOMENT:
            case PGM_CPU_LOADSHED:
            case PGM_TASK_FAILURE:
            case PGM_TASK_SELF_TERMINATED:
            default:
            {
                ret = NO_DATA;
                break;
            }
   
         /* End of "switch" statement. */
         }

      }

#endif
#endif

      LE_send_msg( GL_INFO, 
              "Product Input Abort Flag (%d) Set For Product %d (Vol # %d)\n",
              phd->g.len, Inp_list[ind].type, phd->g.vol_num );

   }

   /* Return the "ret" value. */
   return (ret);

/* End of Check_for_abort_message() */
}   

/*******************************************************************************

   Description:
      Callback function for radial input buffer check.

   Inputs:
      alarm_id - Alarm ID.

   Outputs:

   Returns:

   Notes:

*******************************************************************************/
void Check_radial_ib_callback( malrm_id_t alarm_id ){

   /* Validate the alarm_id. */
   if( alarm_id != (malrm_id_t) RESERVED_TIMER_ID )
      return;

   /* Set the check flag. */
   Check_radial_input_load = 1;

/* End of Check_radial_ib_callback() */
}

/*******************************************************************************

   Description:
      Given the basedata header and the index of the input in the 
      Input list, check if the desired moments are available.

   Inputs:
      ind - index of input in Input List
      bdh - pointer to base_data_header structure

   Outputs:

   Returns:
      Returns 0 if desired moments available, or -1 otherwise.

   Notes:

*******************************************************************************/
static int Check_moments( int ind, Base_data_header *bdh ){

   int ref_flag, vel_flag, wid_flag;

   RPG_what_moments( bdh, &ref_flag, &vel_flag, &wid_flag );

   if( (Inp_list[ind].moments & REF_MOMENT) && !ref_flag )
      return(-1);
   
   if( (Inp_list[ind].moments & VEL_MOMENT) && !vel_flag )
      return(-1);
  
   if( (Inp_list[ind].moments & WID_MOMENT) && !wid_flag )
      return(-1);

   return(0);

/* End of Check_moments() */
}

/******************************************************************************

   Description:
      Validates the size of the RPG format radial message.

   Inputs:
      size - size of the message, in bytes, as from from input LB.
      radial_msg - pointer to the radial message.

   Returns:
      0 if size OK, -1 if not.

******************************************************************************/
static int Check_radial_size( int size, char *radial_msg ){

   Base_data_header *hdr = (Base_data_header *) radial_msg;
   int tsize;

   /* Check the size against the size in the message header. */
   if( size < (hdr->msg_len*sizeof(short)) ){

      LE_send_msg( GL_INFO, "Bad Radial Size in Radial Header: %d < %d\n",
                   size, hdr->msg_len*sizeof(short) );
      return -1;

   }

   /* Add the sizes of the standard moments. */
   tsize = 0;
   if( hdr->ref_offset > 0 ){

      if( hdr->n_surv_bins > 0 )
         tsize = hdr->ref_offset + (hdr->n_surv_bins*sizeof(Moment_t));

   }

   if( (hdr->vel_offset > hdr->ref_offset) || (hdr->spw_offset > hdr->ref_offset) ){

      if( hdr->n_dop_bins > 0 ){

         if( hdr->vel_offset > hdr->spw_offset )
            tsize = hdr->vel_offset + (hdr->n_dop_bins*sizeof(Moment_t));

         else
            tsize = hdr->spw_offset + (hdr->n_dop_bins*sizeof(Moment_t));

      }

   }

   /* Add the size of any additional moments. */
   if( hdr->no_moments > 0 ){

      Generic_moment_t *mom =  (Generic_moment_t *) (radial_msg + hdr->offsets[hdr->no_moments-1]);

      /* Note: Need to subtract sizeof(int) because of "union" definition in
               Generic_moment_t structure. */
      if( mom->data_word_size == BYTE_MOMENT_DATA )
         tsize = hdr->offsets[hdr->no_moments-1] + sizeof(Generic_moment_t) + 
                 mom->no_of_gates;

      else if( mom->data_word_size == SHORT_MOMENT_DATA )
         tsize = hdr->offsets[hdr->no_moments-1] + (sizeof(Generic_moment_t) + 
               mom->no_of_gates*sizeof(short) );

   }
  
   /* Check the size against what we expect the size to be. */
   if( size < tsize ){

      LE_send_msg( GL_INFO, "Bad Radial Size: %d < %d\n", size, tsize );
      return -1;

   }

   return 0;

/* End of Check_radial_size() */
}
