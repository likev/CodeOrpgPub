/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/21 19:41:21 $
 * $Id: rpg_adpt_data_elems.c,v 1.11 2005/10/21 19:41:21 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#define RPG_ADAPTATION_DATA_ELEMENTS

#include <rpg.h>
#include <orpg.h>


#define MAXN_ADE		32	/* max number of ADE callback registrations */
#define MAX_VAR_NAME_LEN	256	/* max string length of variable name */
                                                            /* to DEAU f(x).  */

typedef struct {		/* adaptation block registration */

    int (*f_ptr)();		/* receiving buffer for data. */
    int timing;			/* update timing. */
    en_t event_id;		/* event id if timing is ON_EVENT */
    void *block_address;	/* address of adaptation data block/struct. */
    char *group_name;		/* name of adaptation data group. */
    int dea_modified;		/* has dea data been modified? */

} Adapt_data_elements_t;

static int N_ade = 0;		/* number of adapt data element callback registrations */
static Adapt_data_elements_t Adapt [MAXN_ADE];
				/* adapt data elements registered */

static int Events_registered = 0; 
				/* flag indicating adaptation data element updates 
                                   are tied to events */
static char *Task_name = NULL;	/* Holds the task name string. */
static char Variable_name[MAX_VAR_NAME_LEN];
				/* Holds the variable name .... includes prepended 
                                   "alg." and Task_name strings. */


/* local function prototypes */

void Report_DEAU_error( char * );


/* local functions. */

/********************************************************************

    Description:
       This function adds a new adaptation callback function to the 
       registered adaptation callback function list.

    Inputs:
       update - pointer to user-defined callback function which accesses
                the adaptation data.
       blk_ptr - pointer to adaptation data common block or struct
       grp_name - name of adaptation data group
       timing - update timing (ADPT_UPDATE_BOE, ADPT_UPDATE_BOV,
		ADPT_UPDATE_ON_CHANGE, ADPT_UPDT_WITH_CALL, or
                ADPT_UPDATE_ON_EVENT).
       event_id - event id if timing is ADPT_UPDATE_ON_EVENT (optional)

    Return:	
       Returns an identifier for this registration.  This is to be
       used in subsequent calls.  If error occurs, returns -1. 

    Notes:	

********************************************************************/
int RPG_reg_ade_callback( int (*update) (),
                          void *blk_ptr,
                          char *grp_name,
                          fint *timing, ...)
{

    int status = 0;
    int ret = 0;
    int pos = 0;
    va_list ap;
    char *group_name;

    /* Init deau data base */
    ORPGMISC_deau_init();

    /* Make sure grp_name isn't null. If it is, bail. */
    if( grp_name == NULL )
    {
      LE_send_msg( GL_ERROR, "variable grp_name cannot be NULL\n" );
      RPG_abort_task();
    }

    group_name = strdup(grp_name); 
    /*
     Make sure group_name is null terminated. Fortran strings may end
     in a dollar sign. The number MAX_VAR_NAME_LEN is arbitrary. If no
     end of string character is found before the MAX_VAR_NAME_LEN
     character is looked at, then the string is assumed not to have an end.
    */
    while( pos < MAX_VAR_NAME_LEN )
    {
      if( group_name[ pos ] == '$' )
      {
        group_name[ pos ] = '\0';
        break;
      }
      else if( group_name[ pos ] == '\0' )
      {
        break;
      }
      else
      {
        pos++;
      }
    }

    /* Make sure error didsn't occur above. If it did, bail. */
    if( pos == MAX_VAR_NAME_LEN )
    {
      LE_send_msg( GL_ERROR, "variable grp_name is too long\n" );
      RPG_abort_task();
    }

    /* Make sure group_name doesn't end in a period */
    if( (strlen( group_name ) > 0) 
                     && 
        (group_name[ strlen( group_name ) - 1 ] == '.' ))
    {
      group_name[ strlen( group_name ) - 1 ] = '\0';
    }
   
    /* Check if ADE callback function already registered. */
    if( (ret = RPG_is_ade_callback_reg( update, &status )) >= 0 )
       return (ret);

    if( N_ade >= MAXN_ADE ){	/* This should never happen */

	N_ade++;		/* This will cause later abort */
	return (-1);

    }

    /* Save the callback function address. */
    Adapt [N_ade].f_ptr = update;

    /* Validate the timing. */
    if( (*timing != ADPT_UPDATE_BOE) 
                 && 
        (*timing != ADPT_UPDATE_BOV) 
                 &&
        (*timing != ADPT_UPDATE_ON_CHANGE) 
                 && 
        (*timing != ADPT_UPDATE_WITH_CALL)
                 && 
        (*timing != ADPT_UPDATE_ON_EVENT) )
       PS_task_abort( "Invalid Timing In Adapt Data Element Callback Registration (%d)\n",
                      *timing );

    /* The timing is valid, so save in struct. */
    Adapt [N_ade].timing = *timing;
    
    /* Save common block/struct address. */
    Adapt [N_ade].block_address = blk_ptr;
    
    /* Is update associated with an event? */
    if(*timing == ADPT_UPDATE_ON_EVENT){

       va_start(ap, timing);		/* Start the variable argument
					   list */
       Adapt [N_ade].event_id = (en_t) *(va_arg(ap, int *));
					/* Get the event_id argument */ 

       va_end(ap);			/* End the variable argument list */

       Events_registered = 1;		/* Set flag indicating this adaptation
					   data update is tied to an event */

    }
    else
       Adapt [N_ade].event_id = -1;

    LE_send_msg( GL_INFO, "Adaptation Data Element Callback Function (ID: %d) Registered\n", N_ade );

    /* Register group name with DEAU library. If dea elements in */
    /* the group name change, then RPG_DEAU_callback_fx() is called. */

    Adapt[N_ade].group_name = group_name;
 
    EN_control( EN_PUSH_ARG, Adapt[N_ade].group_name );
    ret = DEAU_UN_register( Adapt[N_ade].group_name, (void *) RPG_DEAU_callback_fx );

    if( ret < 0 )
      LE_send_msg( GL_INFO, "DEAU_UN_register failed for Group: %s ID: %d\n", group_name, N_ade );
    
    /* The return value is a identifier for this data. */
    ret = N_ade;

    LE_send_msg( GL_INFO, "Successfully Registered Adaptation Group %s\n",
                 Adapt[N_ade].group_name );

    /* Increment the number of adaptation data callback functions registered. */
    N_ade++;

    return (ret);

/* End of RPG_reg_ade_callback() */
}

/*************************************************************************
   Description:
      This function checks if adapt data callback function identified
      by "callback_id" has been registered.

   Inputs:
      update - pointer to callback function.
      status - pointer to int to receive status of operation.  
               This is used to provide a FORTRAN interface.

   Outputs:
      status - receives the status of the operation.  If
               already registered, status is set to 1; otherwise
               status is set to 0.

   Returns:
      Callback ID if adaptation data elements callback already registered,
      or -1 if it is not.

   Notes:

*************************************************************************/
int RPG_is_ade_callback_reg( int (*update) (), fint *status ){

   int ind = 0;

   /* Initialize status to not registered. */
   *status = 0;

   /* Check all previously registered callback functions. */
   for( ind = 0; ind < N_ade; ind++ ){

      /* Check for match on block IDs. */
      if( update == Adapt[ind].f_ptr ){

         /* Block is registered. */
         *status = 1;
         return (ind);

      }

   /* End of "for" loop. */
   }

   /* Block is not registered. */
   return (-1);

/* End of RPG_is_ade_callback_reg() */
}

/*************************************************************************
   Description:
      This function is called by the DEAU library if dea elements in a
      registered group change. Any callbacks that registered the changed
      group are flagged to be called during the next "update". If the
      callback registered with a timing of "ON_CHANGE", then the callback
      is called immediately.

   Inputs:
      grp_name - name of adaptation data group
      Other inputs are required for a callback function and aren't used. Check man pages of deau, lb, and en for details.

   Outputs:

   Returns:
      Returns -1 on error, or 0 on success.

   Notes:

*************************************************************************/
int RPG_DEAU_callback_fx( int lb_fd, LB_id_t msg_id, int msg_len, char *grp_name )
{
  int ind = 0;
  int ret = 0;

  /* Do for all registered blocks. */
  for( ind = 0; ind < N_ade; ind++ )
  {
    /* If dea elements in group have changed, set flag. */
    if( strcmp( Adapt[ind].group_name, grp_name ) == 0 )
    {
        LE_send_msg( GL_INFO, "Adapt[ind].group %s has been modified\n", Adapt[ind].group_name );
        Adapt[ind].dea_modified = 1;
    }

  }

  return ret;

/* End of RPG_DEAU_callback_fx() */
}

/*******************************************************************

   Description:
      Calls the adaptation data callback function based on 
      ID of registered callback.

   Inputs:
      callback_id - pointer to int containing callback ID.
      status - pointer to int to receive status of operation.

   Outputs:
      status - contains status of operation:  0 if unsuccessful 
               or 1 if successful.

   Returns:
     Returns 0 on failure (callback not registered) or 1 on 
     successful read.  If the callback function returns an error,
     this error will be propagated along.

   Notes:

********************************************************************/
int RPG_read_ade( fint *callback_id, fint *status ){

   int success = 0;
   int ind = 0;

   /* Initialize the status to unsuccessful. */
   *status = 0;

   /* Is the callback function already registered? */
   if( *callback_id >= N_ade )
      return -1;

   if( (ind = RPG_is_ade_callback_reg( Adapt[*callback_id].f_ptr,
                                       &success )) < 0 )
   {
      LE_send_msg( GL_ERROR, "Callback Function With ID %d Not Registered\n",
                   ind );
      return -1;
   }

   /* Has any dea elements been modified? */
   if( Adapt[*callback_id].dea_modified == 0 )
     return -1;

   /* Unset dea flag. */
   Adapt[*callback_id].dea_modified = 0;

   /* Call the callback function. */
   if( Adapt[*callback_id].f_ptr( Adapt[*callback_id].block_address ) >= 0 )
   {
      /* Normal return. */
      *status = 1;
      return (0);
   }

   return (0);

/* End of RPG_read_ade() */
}

/********************************************************************

    Description: 
       This function calls all the registered callback functions.

    Return:	
       This function returns a negative value (-1) if any of the
       callback functions returns an error.

    Notes:	

********************************************************************/
int RPG_update_all_ade(){

  int ret = 0;
  int ind = 0;

  /* Do for all registered blocks. */
  for( ind = 0; ind < N_ade; ind++ )
  {
    /* Has any dea elements been modified? */
    if( Adapt[ind].dea_modified == 0 )
      continue;

    /* Unset dea flag. */
    Adapt[ind].dea_modified = 0;

    /* Call the callback function.   We ignore specific errors that
       may occur but report that an error has occurred.. */
    if( Adapt[ind].f_ptr( Adapt[ind].block_address ) < 0 )
    {
      LE_send_msg( GL_ERROR, "Callback Function With ID %d Failed\n", ind );
      ret = -1;
    }

  /* End of "for" loop. */
  }

   return (ret);

/* End of RPG_update_all_ade() */
}

/********************************************************************

   Description:
      Retrieves value(s) from the database given "alg_name", 
      "valid_id", and a receiving buffer.

   Inputs:
      alg_name - string containing the algorithm name
      value_id - string containing the variable name (how it appears
                 in the DEAU file.
      values - pointer to receiving buffer for the data.

   Outputs:
      values - contains the data value(s) from the data base.

   Returns:
      Returns -1 on error, or 0 on success.

   Notes:

********************************************************************/
int RPG_ade_get_values( char *alg_name, char *value_id, double *values ){

   int ret = 0;        /* Return code. */
   int num_values = 0; /* Number of elements in "values". */

   strcpy( Variable_name, alg_name );
   strcat( Variable_name, value_id );

   /* Determine number of elements in "values". */
   num_values = DEAU_get_number_of_values( Variable_name );

   /* Call the DEAU library to retrieve the value(s). */
   if( (ret = DEAU_get_values( Variable_name, values, num_values )) < 0 ){

      LE_send_msg( GL_ERROR, "DEAU_get_values Returned Error: %d\n", ret );
      LE_send_msg( GL_ERROR, "--->Variable Name: %s, Num values: %d\n",
                   Variable_name, num_values );
      return(-1);

   }

   return(0);

}

/********************************************************************
   Description:
      Retrieves string(s) from the database given "alg_name",
      "valid_id", and a pointer to string to receive the string(s).

   Inputs:
      alg_name - string containing the algorithm name
      value_id - string containing the variable name (how it appears
                 in the DEAU file.
      values - pointer to string to receive the string(s).

   Outputs:
      values - contains the string(s) from the data base.

   Returns:
      Returns -1 on error, or 0 on success.

   Notes:
********************************************************************/
int RPG_ade_get_string_values( char *alg_name, char *value_id, char **values ){

   int ret = 0;

   strcpy( Variable_name, alg_name );
   strcat( Variable_name, value_id );

   /* Call the DEAU library to retrieve the value(s). */
   if( (ret = DEAU_get_string_values( Variable_name, values )) < 0 ){

      LE_send_msg( GL_ERROR, "DEAU_get_string_values Returned Error: %d\n", ret
);
      LE_send_msg( GL_ERROR, "--->Variable Name: %s\n",
                   Variable_name );
      return(-1);

   }

   return(0);

}

/********************************************************************

   Description:
      Retrieves number of value(s) for the "alg_name" and "valid_id" variables.

   Inputs:
      alg_name - string containing the algorithm name
      value_id - string containing the variable name (how it appears
                 in the DEAU file.

   Outputs:
      num_values - contains the number of value(s) in the variable.

   Returns:
      Returns the number of value(s) or error code if negative.

   Notes:

********************************************************************/
int RPG_ade_get_number_of_values( char *alg_name, char *value_id ){

   int num_values = 0; /* Number of elements associated with "value_id". */

   strcpy( Variable_name, alg_name );
   strcat( Variable_name, value_id );

   /* Determine number of elements in "values". */
   num_values = DEAU_get_number_of_values( Variable_name );

   return num_values;

}

/********************************************************************

   Description: 
      This function calls all the registered adaptation data 
      element callback functions.

   Inputs:

   Outputs:

   Returns:

   Notes:

********************************************************************/
void ADE_initialize(){

   int ind = 0;
   char *lb_name = NULL;


    /* Tell the DEAU module where the database is. There  */
    /* is no overhead in doing this automatically, in the */
    /* event the user never uses the DEAU service.        */

    lb_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
    if( lb_name != NULL )
    {
      DEAU_LB_name( lb_name );

      /* Register the DEAU error reporting function. */
      DEAU_set_error_func( Report_DEAU_error );
    }

   if (N_ade > MAXN_ADE)		
      PS_task_abort ("Too Many Adaptation Callbacks Specified\n");

   /* Get the task name.   Required for accessing adaptation data 
      elements. */
   Task_name = INIT_task_name();

   /* Do an initial read. */
   for( ind = 0; ind < N_ade; ind++ ){

      if( Adapt[ind].f_ptr( Adapt[ind].block_address ) < 0 )
         LE_send_msg( GL_ERROR, "Callback Function With ID %d Failed\n", ind );

   }

/* End of ADE_initialize() */
}

/********************************************************************

   Description: 
      This function updates adaptation data elements by calling
      all registered callback functions.  The callback functions
      whose timing is beginning of volume are not called if 
      not beginning of volume.

   Inputs:
      new_vol - flag indicating whether a new volume scan has
                occurred.

   Outputs:

   Returns:
      Returns -1 if any of the callback functions return an error,
      or 0 otherwise.

   Notes:

********************************************************************/
int ADE_update_ade(int new_vol){

   int ret = 0;
   int ind = 0;

   /* Do for all registered callback functions. */
   for (ind = 0; ind < N_ade; ind++) {

      /* If not new volume scan and update timing is beginning of
         volume, continue.  A read of the data occurs when the new 
         volume scan occurs. */
      if( !new_vol && (Adapt[ind].timing == ADPT_UPDATE_BOV) )
         continue;

      /* Has any dea elements been modified? */
      if( Adapt[ind].dea_modified == 0 )
        continue;

      /* Unset dea flag. */
      Adapt[ind].dea_modified = 0;

      /* Call the adaptation data callback function. */
      if( Adapt[ind].f_ptr( Adapt[ind].block_address ) < 0 )
      {
         LE_send_msg( GL_ERROR, "Callback function ID %d Failed\n", ind );
         ret = -1;
      }

    /* End of "for" loop. */
    }

    return( ret );

/* End of ADE_update_ade() */
}


/********************************************************************

    Description: 
       This function calls those callback functions associated with
       event "event_id".

    Input:	
       event_id - event_id which triggers callback
 
    Return:	
       Returns -1 on error, or 0 on success. 

    Notes:	

********************************************************************/
int ADE_update_ade_by_event ( en_t event_id ){

   int ret = 0, ind;

   if( !Events_registered ){

      /* If no events registered, no need to go any further */
      return (-1);

   }

   /* Do for all registered callback functions. */
   for( ind = 0; ind < N_ade; ind++ ){

      /* If callback not associated with this event, continue */
      if(Adapt[ind].event_id != event_id)
         continue;

      /* Has any dea elements been modified? */
      if( Adapt[ind].dea_modified == 0 )
        continue;

      /* Unset dea flag. */
      Adapt[ind].dea_modified = 0;

      /* Call the registered callback function. */
      if( Adapt[ind].f_ptr( Adapt[ind].block_address ) < 0 ){

         ret = -1;
         LE_send_msg( GL_ERROR, 
                      "Callback Function With ID %d Associated With Event ID %d Failed\n",
                      ind, event_id );

      }

   /* End of "for" loop. */
   }

   return( ret );

/* End of ADE_update_ade_by_event() */
}


/**********************************************************************

   Description:
      DEAU error reporting function.

   Inputs:
      error - contains error text.

   Outputs:

   Returns:

   Notes:

**********************************************************************/
void Report_DEAU_error( char *error ){

   /* Send the message to log-error. */
   LE_send_msg( GL_INFO, "%s\n", error );

}

#define DEA_NAME_LENGTH     32

/**********************************************************************

   Description:
      Register for site adaptation data.

   Inputs:
      Pointer to struct holding site information adaptation data

   Outputs:

   Returns:
      Returns >=0 if successful, <0 if not

   Notes:

**********************************************************************/
int RPG_reg_site_info( void *blk_ptr )
{
   int temp = ON_CHANGE;
   int ret = 0;

   char Site_info_name[ DEA_NAME_LENGTH ];

   /* Set the Site_info_name to default. */
   strcpy( Site_info_name, "site_info" );

   /* Check if site information name macro is not NULL or empty string.
      If not NULL or empty, copy name defined by macro. */
   if( (SITE_INFO_DEA_NAME != NULL) 
                   && 
       (strlen( SITE_INFO_DEA_NAME ) > 0) ){

      /* Just to be sure, SITE_INFO_DEA_NAME must be less than 
         DEA_NAME_LENGTH characters in length. */  
      if( strlen( SITE_INFO_DEA_NAME ) < DEA_NAME_LENGTH )
         strcpy( Site_info_name, SITE_INFO_DEA_NAME ); 
         
      else
         ret = -1;
      
   }

   /* Register for site info adaptation data updates. */
   if( ret >= 0 )
      ret = RPG_reg_ade_callback( RPG_site_info_callback_fx,
                                  blk_ptr,
                                  Site_info_name,
                                  &temp );

   if( ret < 0 )
      LE_send_msg( GL_ERROR, "Unable to Register for site_info Updates (%d)\n", ret );

   return( ret );
}

