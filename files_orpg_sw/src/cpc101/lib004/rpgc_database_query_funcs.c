#include <rpgc.h>

#define RPGC_QUERY_RETRIES_MAX 		2
#define RPGC_MAX_LIST_SIZE		1000

/*\//////////////////////////////////////////////////////////////

   Description:
      ORPGC_DB_select queries the DB of "data_id". "where" 
      specifies what kind of records we want to search for. It 
      is similar to the WHERE field in the SQL SELECT statement. 

   Inputs:
      where  - a text string containing query field names,
      operators and constants (values). The acceptable operators 
      are "=" or "==" (equal), "<>" or "!=" (not equal), "<" 
      (less than), "<=" (less than or equal), ">" (greater than), 
      ">=" (greater than or equal), "and", "or", "not", 
      "(" and ")". 

   Output:
      result - the address of a pointer for returning the query
      result. The caller should not try to interpret the returned
      "result". It should be only used for passing to other
      functions such as RPGC_DB_get_record.  If it is not NULL, 
      it must be freed when it is no longer needed. 

   Returns: 
      RPGC_DB_select returns the number of records found on 
      success or a negative error code.

//////////////////////////////////////////////////////////////\*/
int RPGC_DB_select( int data_id, char *where, void **result ){

   int ret, retries, maxn, records_found, records_returned;
   char *lb_name = NULL;
   LB_status status;
   LB_attr attr;

   /* Do some error checking ..... */
   if( where == NULL ){

      LE_send_msg( GL_INFO, "Invalid where argument to RPGC_DB_select.\n" );
      return -1;

   }

   /* Get the LB name of the data base.   This name is used
      in most of the SDQ functions. */
   lb_name = ORPGDA_lbname( data_id );
   if( lb_name == NULL ){

      LE_send_msg( GL_INFO, "RPGC_DB_select - Data Base Not Found\n" );
      return -1;

   }
   
   /* Get the LB_status to determine the maximum records for the query. */
   maxn = RPGC_MAX_LIST_SIZE;
   status.n_check = 0;
   status.attr = &attr;
   ret = ORPGDA_stat( data_id, &status );
   if( ret == LB_SUCCESS )
      maxn = attr.maxn_msgs;

   /* Now query the database.  Do Until retries exhausted .... */
   retries = 0;
   while( retries < RPGC_QUERY_RETRIES_MAX ){

      SDQ_set_maximum_records( maxn );
      ret = SDQ_select( lb_name, where, result );

      /* Check if negative return value ... means error occurred. */
      if( ret < 0 ){

         LE_send_msg( GL_INFO, "RPGC_DB_select Failed: %d\n", ret );
         return -1;

      }
      else
         break;

   }

   /* Check to see if there were any query errors. */
   ret = SDQ_get_query_error( *result );

   if( ret < 0 ){

      LE_send_msg( GL_INFO, "RPGC_DB_select Query Error: %d\n", ret );
      free( *result );
      return -1;

   }

   /* At this point it is assumed that the query was a success so now we 
      need to determine the number of records which matched the query 
      criteria. */ 
   records_found = SDQ_get_n_records_found( *result );
   records_returned = SDQ_get_n_records_returned( *result );

   /* Lets check to see if the number of records returned is not equal 
      to the number of records matching the query criteria. If they are 
      different, log a warning message and continue. */
   if( records_found != records_returned )
      LE_send_msg( GL_INFO, "RPGC_DB_select Query Matched Records: %d, Returned: %d\n",
                   records_found, records_returned);

   /* Now we need to allocate memory for the query data which are to be 
      returned to the caller.  It is the responsibility of the caller to 
      free this memory after use. */
   if( records_returned <= 0 ){

      if( *result != NULL )
         free( *result );

      return (0);

   }

   return records_returned;

/* End of RPGC_DB_select() */
}


/*\///////////////////////////////////////////////////////////////////

   Description:
      RPGC_DB_insert adds a new record "record" of size "rec_len" to 
      the data base "data_id". 

   Inputs:
      data_id - Data ID of the database.
      record - Record to be inserted into the data base.
      rec_len - length of the record to be inserted.

   Returns:
      Returns the size of the record inserted on success or a negative 
      error code on failure.

///////////////////////////////////////////////////////////////////\*/
int RPGC_DB_insert( int data_id, void *record, int rec_len ){

   char *lb_name = NULL;

   /* Get the LB name of the data base.   This name is used
      in most of the SDQ functions. */
   lb_name = ORPGDA_lbname( data_id );
   if( lb_name == NULL ){

      LE_send_msg( GL_INFO, "RPGC_DB_select - Data Base Not Found\n" );
      return -1;

   }

   /* Write the record to the data base. */
   return( SDQ_insert( lb_name, record, rec_len ) );
 
/* End of RPGC_DB_insert(). */
}


/*\///////////////////////////////////////////////////////////////

   Description:
      Deletes all records that match expression "where" in DB
      "data_id". 

   Inputs:
      data_id - Data ID of the database.
      where - query string used for the deletion.

   Returns:
      Returns the number of records deleted on success or a
      negative error code on failure. 

   Note:
      Be careful in using this function because any record 
      deleted cannot be recovered. One may call RPGC_DB_select 
      first to check the records that match "where" before 
      calling RPGC_DB_delete. It is the user's responsibility 
      to maintaining the DB by calling RPGC_DB_delete to prevent 
      DB from full in which case RPGC_DB_insert will fail.

///////////////////////////////////////////////////////////////\*/
int RPGC_DB_delete( int data_id, char *where ){

   char *lb_name = NULL;

   /* Get the LB name of the data base.   This name is used
      in most of the SDQ functions. */
   lb_name = ORPGDA_lbname( data_id );
   if( lb_name == NULL ){

      LE_send_msg( GL_INFO, "RPGC_DB_select - Data Base Not Found\n" );
      return -1;

   }
   
   return( SDQ_delete( lb_name, where ) );

/* End of RPGC_DB_delete */
}

/*\///////////////////////////////////////////////////////////////////

   Description:
      Retrieves the "ind"-th record in the query result "result",
      returned from RPGC_DB_select, from the DB. RPGC_DB_get_record 
      allocates the memory for holding the record and returns the 
      pointer with "record". The caller must free this pointer when 
      the record is no longer needed. 

   Inputs:
      result - Query result from RPGC_DB_select call.
      ind - Record number in the query result "result".

   Outputs:
      record - The "ind"-th record of the query result.

   Returns:
      On success, returns the size of the record. It returns a 
      negative error code on failure.

   Notes:
      The caller must free this pointer when the record is no 
      longer needed. 

///////////////////////////////////////////////////////////////////\*/
int RPGC_DB_get_record( void *result, int ind, char **record ){

   int ret;

   /* Read the record from the data base. */
   ret = SDQ_get_message( result, ind, record );

   /* Negative return value indicates error. */
   if( ret < 0 )
      LE_send_msg( GL_INFO, "RPGC_DB_get_record Failed: %d\n", ret );

   return ret;

/* End of RPGC_DB_get_record() */
}

/*\//////////////////////////////////////////////////////////////////

   Description:
      RPGC_DB_get_header is similar to RPGC_DB_get_record except that 
      it returns the query header only. It is thus more efficient. If 
      one needs only to look at the query header, RPGC_DB_get_header 
      should be used. 

   Inputs:
      result - Query result from RPGC_DB_select call.
      ind - Record number in the query result "result".

   Outputs:
      hd - Header of the "ind"-th query in result.      

   Returns:
      The function returns the size of the hd on success or a 
      negative error code.

   Notes:
      The returned pointer of "hd" must NOT be freed. 

//////////////////////////////////////////////////////////////////\*/
int RPGC_DB_get_header( void *result, int ind, char **hd ){

   int ret;

   /* Read the record from the data base. */
   ret = SDQ_get_record( result, ind, hd );

   /* Negative return value indicates error. */
   if( ret < 0 )
      LE_send_msg( GL_INFO, "RPGC_DB_get_record Failed: %d\n", ret );

   return ret;

/* End of RPGC_DB_get_header() */
}
