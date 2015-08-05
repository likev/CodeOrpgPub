#include <rpgc.h>
#include <orpg.h>
#include <orpgdbm.h>
#include <stdarg.h>

#define MAX_FIELDS		20

/*\//////////////////////////////////////////////////////////////////////////////////

   Description:
      Queries the product database for product code associated with "name".

   Inputs:
      max_results - maximum number of query results.

   Outputs:
      results - contains the query results.

   Returns:
      The number of products found in the query.

   Notes:
      Products are queried by product code or product ID if the product is 
      warehoused.  The user can optionally specify query by volume time, in 
      UTC (RPGC_VOL_TIME), elevation angle, in deg*10 (RPGC_ELEV), volume time 
      range (RPGC_VOL_TIME_RANGE) and elevation range (RPGC_ELEV_RANGE).  

      Each optional query field is a tuple.  RPGC_VOL_TIME and RPGC_ELEV are 
      2-tuples, RPGC_VOL_TIME_RANGE and RPGC_ELEV_RANGE are 3-tuples.  These
      are specified in the following format:

      RPGC_VOL_TIME, volume time in UTC
      RPGC_ELEV, elevation angle in deg*10
      RPGC_VOL_TIME_RANGE, begining volume time in UTC, ending volume time in UTC.
      RPGC_ELEV_RANGE, begining elevation in deg*10, ending elevation in deg*10.

      The argument list must be terminated with RPGC_END_LIST.

/////////////////////////////////////////////////////////////////////////////////\*/
int RPGC_find_pdb_product( char *name, RPGC_prod_rec_t *results, int max_results, ... ){
   
   int                  prod_code, prod_id, field, value, nresults, is_warehoused, i;
   ORPGDBM_query_data_t *query_data = NULL;
   RPG_prod_rec_t	*db_info = NULL;
   va_list              ap;

   /* Convert the data name into product code. */
   prod_code = RPGC_get_code_from_name( name );
   prod_id = RPGC_get_id_from_name( name );
   if( prod_id < 0 )
      return 0;

   is_warehoused = ORPGPAT_get_warehoused( prod_id );
   if( (prod_code <= 0) && (is_warehoused <= 0) )
      return 0;

   /* Allocate storage for the query data. */
   query_data = (ORPGDBM_query_data_t *) 
                        calloc( 1, sizeof(ORPGDBM_query_data_t)*MAX_FIELDS );

   if( query_data == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes.\n",
                   sizeof(ORPGDBM_query_data_t)*max_results );
      return 0;

   }
   
   /* Establish the query data */
   query_data[0].field = ORPGDBM_MODE;
   query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH;

   if( prod_code > 0 ){

      query_data[1].field = RPGP_PCODE;
      query_data[1].value = prod_code;

   }
   else{ 

      query_data[1].field = RPGP_WAREHOUSED;
      query_data[1].value = prod_id;

   }

   /* Parse the argument list. */
   field = 2;
   va_start( ap, max_results );

   while( field < MAX_FIELDS ){

      /* A field value of QUERY_END_LIST terminates the argument list. */
      value = va_arg( ap, int );
      if( value == QUERY_END_LIST )
         break;

      field++;

      switch( value ){

         case RPGP_VOLT:
         {
            query_data[field].field = RPGP_VOLT;
            query_data[field].value = va_arg( ap, int );
            break;
         }

         case RPGP_ELEV:
         {
            query_data[field].field = RPGP_ELEV;
            query_data[field].value = va_arg( ap, int );
            break;
         }

         case ORPGDBM_VOL_TIME_RANGE:
         {
            query_data[field].field = ORPGDBM_VOL_TIME_RANGE;
            query_data[field].value = va_arg( ap, int );
            query_data[field].value2 = va_arg( ap, int );
            break;
         }

         case ORPGDBM_ELEV_RANGE:
            query_data[field].field = ORPGDBM_ELEV_RANGE;
            query_data[field].value = va_arg( ap, int );
            query_data[field].value2 = va_arg( ap, int );
            break;

         default:
            break;

      }

   }

   /* Allocate space for the query results. */
   db_info = (RPG_prod_rec_t *) calloc( 1, sizeof(RPG_prod_rec_t)*max_results );
   if( db_info == NULL ){

      free( query_data );
      LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes.\n", 
                   sizeof(RPG_prod_rec_t)*max_results );

      return 0;

   }

   /* Query the product database, searching for matching products. */
   nresults = ORPGDBM_query ( db_info, query_data, field, max_results );

   /* Query buffer no longer needed. */
   free( query_data );

   /* Transfer the query results to user-supplied buffer. */
   for( i = 0; i < nresults; i++ ){

      results[i].msg_id = db_info[i].msg_id;
      results[i].vol_time = db_info[i].vol_t;
      results[i].elev = db_info[i].elev;
      results[i].elev_ind = db_info[i].elev_ind;
      memcpy( &results[i].params[0], &db_info[i].params[0], 6*sizeof(short) );

   }

   /* Free the query results. */
   free( db_info );

   /* Return the number of query results. */
   return nresults;

/* End of RPGC_find_a_product() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Reads product from Product Database located at message "msg_id".

   Inputs:
      name - name of product (defined in TAT).
      msg_id - message ID of product in Product Database.

   Returns:
      Pointer to product data.  Product is in ICD format.

   Notes:
      The caller must use RPGC_release_pdb_product() to release the
      the product when no longer needed. 

//////////////////////////////////////////////////////////////////////////\*/
void* RPGC_get_pdb_product( char *name, LB_id_t msg_id ){

   int prod_id, ret;
   char *buf = NULL, *cpt = NULL;
   Prod_header *phd = NULL;

   /* Read the product from the product data base. */
   ret = ORPGDA_read( ORPGDAT_PRODUCTS, &buf, LB_ALLOC_BUF, msg_id );
   
   if( ret <= 0 ){

      LE_send_msg( GL_INFO, "ORPGDA_read Failed (%d)\n", ret );
      return NULL;

   }

   /* Convert name to product id and verify product read has the same id. */
   prod_id = RPGC_get_id_from_name( name );

   phd = (Prod_header *) buf;
   if( prod_id != phd->g.prod_id ){

      LE_send_msg( GL_INFO, "Prod ID: %d Mismatch with Product Header ID: %d\n",
                   prod_id, phd->g.prod_id );
      free( buf );
      return NULL;

   }
   
   /* Check if the product was compressed.  If so, decompress it. */
   if( ORPGPAT_get_compression_type( prod_id ) > 0 )
      RPGC_decompress_product( buf );

   /* Strip off the product header and return the product to caller. */
   cpt = buf + sizeof(Prod_header);
   return( cpt );
   
/* End of RPGC_get_a_product() */
}

/*\//////////////////////////////////////////////////////////////////////////

   Description:
      Used to release (free) the product obtained using RPGC_get_pdb_product.

   Inputs:
      buf - pointer to product buffer data.

//////////////////////////////////////////////////////////////////////////\*/
void RPGC_release_pdb_product( void *buf ){

   char *cpt = NULL;

   if( buf == (void *) NULL )
      return;

   cpt = (char *) buf - sizeof(Prod_header);

   free( cpt );

   return;

/* End of RPGC_release_a_product() */
}
