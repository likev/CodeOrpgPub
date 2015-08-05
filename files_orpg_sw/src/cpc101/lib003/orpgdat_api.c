/************************************************************************
 *									*
 *	Module:  orpgpat.c						*
 *		This module contains a collection of routines to	*
 *		access and modify the product attribute table.		*
 *									*
 ************************************************************************/


/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/03/08 20:32:07 $
 * $Id: orpgdat_api.c,v 1.8 2005/03/08 20:32:07 jing Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*	System include files					*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include <orpg.h>
#include <orpgdat_api.h>
#include <mrpg.h>
#include <rss_replace.h>

/* Static Globals. */
static int Attr_tbl_num  = 0;			/* Number of data attribute entries.	*/
static Mrpg_data_t **Attr_tbl_ptr = NULL;	/* Lookup table to each entry in the	*/
				 	 	/* data attributes table.		*/
static int Reading_table = 0;			/* Flag set when reading DAT from LB.	*/
static char *Attr_tbl = NULL; 			/* Pointer to data attributes table	*/
static int Need_update = 1;			/* attribute table read needed 		*/
static void (*User_exception_callback)() = NULL;/* users exception call back function 	*/

/* Function Prototypes. */
static void Process_exception ();


/************************************************************************
 *									*
 *  Description: This function registers a call back function, which 	*
 *		 will be called when an exception condition is 		*
 *		 encountered.						*
 *									*
 *  Input:	user_exception_callback - the user's callback function.	*
 *									*
 ************************************************************************/
void ORPGDAT_error (void (*user_exception_callback)()){

    User_exception_callback = user_exception_callback;
    return;

}

/************************************************************************
 *									*
 *  Description: This function is called when an exception condition	*
 *		condition is detected. 					*
 *									*
 *  Input:	 None							*
 *									*
 ************************************************************************/
static void Process_exception (){

    if (User_exception_callback == NULL)
	return;
     
    else
	User_exception_callback ();

    return;

}

/************************************************************************
 *  Description:  The following module reads the data attributes from	*
 *  		  table from the ORPGDAT_TASK_STATUS attributes lb.  	*
 *		  The size of the message containing the data is 	*
 *  		  determined and memory is dynamically allocated to 	*
 *  		  store the message in memory.  A pointer is kept to 	*
 *		  the start of the data.				*
 *									*
 *  Return:	  On success 0 is returned, otherwise negative error	*
 *		  is retuned.						*
 *									*
 ************************************************************************/
int ORPGDAT_read_tbl(){

   int size = 0, offset, max_entries;
   Mrpg_data_t	*data_attr;

   /* Set flag indicating the table is being read. */
   Reading_table = 1;

   Attr_tbl_num  = 0;

   if( Attr_tbl != (char *) NULL )
      free (Attr_tbl);

   /* Now read the data attributes table                	*/
   size = ORPGDA_read( ORPGDAT_TASK_STATUS, (char *) &Attr_tbl,
                       LB_ALLOC_BUF, MRPG_RPG_DATA_MSGID );

   /* Close the LB and reset flag. */
   ORPGDA_close( ORPGDAT_TASK_STATUS ); 
   
   /* Check status of read.  If bad, return error. */
   if (size <= 0){

      /* This is a special case .... the first time called, this message may not
         be available. */
      if( (size != LB_NOT_FOUND) && (size != 0) ){

         LE_send_msg( GL_INPUT, 
		"ORPGDA_read MRPG_RPG_DATA_MSGID Failed (%d)\n", size );
         Process_exception ();

      }
      return( size );

   }

   /* Estimate the maximum number of data attribute messages so *
    * we can allocate enough memory for it.			*/
   max_entries = size / sizeof( Mrpg_data_t );
   Attr_tbl_ptr = calloc( max_entries*sizeof(Mrpg_data_t *), 1);
   if( Attr_tbl_ptr == NULL ){

      LE_send_msg( GL_MEMORY, "ORPGDAT: calloc Failed (Size %d)\n", 
                   max_entries*sizeof(Mrpg_data_t));
      Process_exception ();
      return (ORPGDAT_CALLOC_FAILED);

   }

   offset = 0;
   while (offset < size) {

      /* Cast the current buffer pointer to the main attributes	*
       * table structure.  The size of the record can be 	*
       * determined from the entry_size element in the main	*
       * structure.  The start of the next record is the current*
       * offset plus the current record size.			*/

      data_attr = (Mrpg_data_t *) (Attr_tbl+offset);
      Attr_tbl_ptr[ Attr_tbl_num ] = data_attr;

      offset = offset + data_attr->size;
      Attr_tbl_num++;

   }

   Need_update = 0;
   Reading_table = 0;
   return 0;

}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to record in the	*
 *		  data attributes table conaining a specified 		*
 *		  data id.						*
 *									*
 *  Inputs:	  data_id - The data id to find.			*
 *									*
 *  Outputs:      size - pointer to int holding sizeof table entry      *
 *									*
 *  Return:	  A pointer to the record containing the data id 	*
 *		  on success, a NULL pointer on failure.		*
 *									*
 ************************************************************************/
Mrpg_data_t* ORPGDAT_get_entry ( int data_id, int *size ){

   Mrpg_data_t	*ptr = NULL;
   int index = 0;

   if( (Need_update) && (!Reading_table) )
      ORPGDAT_read_tbl ();

   if( (Need_update) || (Reading_table) ){
   
      *size = 0;
      return (NULL);

   }

   /* If the input data id is outside the allowed range of ids,	*
    * return a NULL pointer.					*/
   if( data_id  < ORPGDAT_BASE )
      return (NULL);

   /* The input data id is defined so return a pointer to the	*
    * main structure for record containing the input data id.	*/
   while( index < Attr_tbl_num ){

      ptr = Attr_tbl_ptr[index];
      if( ptr->data_id == data_id ){

         Mrpg_data_t *entry = (Mrpg_data_t *) malloc( ptr->size );
         if( entry != NULL ){

            memcpy( entry, ptr, ptr->size );
            return entry;

         }
         else{

            LE_send_msg( GL_ERROR, "malloc Failed (Size %d)\n",
                         ptr->size );
            break;

         }
         
      }
      index++;

   }

   return( NULL );

}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the mnemonic	*
 *		  string associated with an ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, a pointer to the mnemonic string is	*
 *		  returned.						*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/
Mrpg_wp_item* ORPGDAT_get_write_permission( int data_id, int *size ){

   Mrpg_data_t *attr_tbl = NULL;
   Mrpg_wp_item *wp_item = NULL;
   int entry_size = 0;

   attr_tbl = ORPGDAT_get_entry( data_id, &entry_size );
   if( attr_tbl != NULL ){

      if( attr_tbl->wp_size != 0 ){

         wp_item = (Mrpg_wp_item *) 
                   calloc( sizeof(Mrpg_wp_item)*attr_tbl->size, 1 );
         if( wp_item == NULL ){

            LE_send_msg( GL_INPUT, "calloc Failed (Size %d)\n",
	                 sizeof( Mrpg_wp_item) );
            Process_exception ();
            return( NULL );

         }

         *size = attr_tbl->wp_size;
         memcpy( wp_item, &attr_tbl->wp[0],
                 (attr_tbl->wp_size*sizeof(Mrpg_wp_item)) );
	 return( wp_item );

      }

   }

   *size = 0;
   return( NULL );

}

/************************************************************************
 *									*
 *  Description:  This function returns the compression type		*
 *		  associated with a specified ORPG data store.		*
 *									*
 *  Input:	  data_id - data id                  			*
 *									*
 *  Return:	  On success the compression type is returned.		*
 *		  On failure negative error is returned.		*
 *									*
 ************************************************************************/
int ORPGDAT_get_compression_type ( int data_id ){

   int ret, size = 0;
   Mrpg_data_t *data;

   data = ORPGDAT_get_entry ( data_id, &size );

   ret = ORPGDAT_ENTRY_NOT_FOUND;

   if ( data != NULL)
      ret = (int) data->compr_code;

   return ret;
}
