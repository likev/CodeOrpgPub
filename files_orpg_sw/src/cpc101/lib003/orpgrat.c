/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2012/11/07 15:38:12 $ 
 * $Id: orpgrat.c,v 1.7 2012/11/07 15:38:12 steves Exp $ 
 * $Revision: 1.7 $ 
 * $State: Exp $ 
 */ 
#include <orpgrat.h>
#include <orpgdat.h>
#include <orpg.h>

/* Number of defined alarms in each table */
static int Alarm_tbl_num[ORPGRAT_NUM_TABLES] = { 0, 0 };

/* Pointers to each of the alarms table */
static char *Alarm_tbl[ORPGRAT_NUM_TABLES] = { NULL, NULL };

/* Lookup table to each entry in the RDA alarms table. */
static int Alarm_tbl_ptr[ORPGRAT_NUM_TABLES][ORPGRAT_MAX_RDA_ALARMS+1];

/* alarm code to index table. */
static short Code_to_index[ORPGRAT_NUM_TABLES][ORPGRAT_MAX_RDA_ALARMS+1];

/* alarms table re-read needed */
static int Need_update[ORPGRAT_NUM_TABLES] = { 1, 1 };

/* alarms table in use. */
static int Table_in_use = ORPGRAT_ORDA_TABLE;

/* Static function prototypes. */
static char* Get_alarm_data( int table, int code );



/************************************************************************
 *  Description:  The following module clears the RDA alarms		*
 *  		  table and sets the number of alarms to 0. 		*
 *									*
 *  Return:	  If the table was previously initialized then 1 is	*
 *		  retuned.  On error, -1 is returned.  Otherwise,	*
 *                0 is returned.	 				*
 *									*
 ************************************************************************/
int ORPGRAT_clear_rda_alarms_tbl( int table ){

   int	ret;
   int	i;

   if( (table < 0) || (table >= ORPGRAT_NUM_TABLES) ){

      LE_send_msg( GL_ERROR, "Invalid Table Number (%d)\n", table );
      return(-1);

   }

   Need_update[table] = 1;

   for( i = 0; i <= ORPGRAT_MAX_RDA_ALARMS; i++ )
      Code_to_index[table][i] = ORPGRAT_DATA_NOT_FOUND;

   if( Alarm_tbl[table] != (char *) NULL ){

      Alarm_tbl_num[table]  = 0;

      free (Alarm_tbl[table]);
      Alarm_tbl[table] = (char *) NULL;

      for( i = 0; i <= ORPGRAT_MAX_RDA_ALARMS; i++ )
         Alarm_tbl_ptr[table][i] = -1;

      ret = 1;

   }
   else{

    ret = 0;

   }

   return ret;

/* ORPGRAT_clear_rda_alarms_tbl() */
}

/************************************************************************
 *  Description:  The following module reads the RDA alarms table	*
 *  		  from the RDA alarms definition lb.  The size of	*
 *		  the message containing the data is determined first 	*
 *  		  and memory is dynamically allocated to store the	*
 *  		  message in memory.  A pointer is kept to the start of	*
 *		  the data.						*
 *									*
 *  Return:	  On success 0 is returned, otherwise 			*
 *		  ORPGRAT_DATA_NOT_FOUND is retuned.			*
 *									*
 ************************************************************************/
int ORPGRAT_read_rda_alarms_tbl( int table ){

   int status;
   RDA_alarm_entry_t *alarm;
   int offset;
   int i;

   Alarm_tbl_num[table]  = 0;

   /* Initialize lookup table for mapping alarm code to table index. */
   for( i = 0; i <= ORPGRAT_MAX_RDA_ALARMS; i++ )
      Code_to_index[table][i] = ORPGRAT_DATA_NOT_FOUND;

   /* Make a pass through the RDA alarms message and create a lookup table
      to the start of each alarm definition.  We maintain lookup tables 
      referenced by alarm code.	*/

   /* Check to see if the table has already been initialized.  If so, 
      free up the memory associated with it.	*/
   Alarm_tbl_num[table]  = 0;

   if( Alarm_tbl[table] != (char *) NULL )
      free (Alarm_tbl[table]);


   /* Now read the RDA alarms table into the buffer just created.	*/ 
   if( table == ORPGRAT_RDA_TABLE )
      status = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL, &Alarm_tbl[table], 
                            LB_ALLOC_BUF, ORPGRAT_ALARMS_TBL_MSG_ID );

   else
      status = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL, &Alarm_tbl[table], 
                               LB_ALLOC_BUF, ORPGRAT_ORDA_ALARMS_TBL_MSG_ID);
   if( status <= 0 ){

      LE_send_msg( GL_INPUT, "ORPGDA_read of Alarm Table (%d) Failed (%d)\n",
		   table, status);
      return (ORPGRAT_DATA_NOT_FOUND);

   }

   offset = 0;
   while (offset < status) {

      short code;

      if( Alarm_tbl_num[table] > ORPGRAT_MAX_RDA_ALARMS ){

         LE_send_msg( GL_INPUT, "Too Many Alarms (%d)\n", Alarm_tbl_num);
         return (ORPGRAT_ERROR);

      }

      /* Alarm_tbl_ptr contains the offset, in bytes, to the start of each 
         alarm table record.	*/
      Alarm_tbl_ptr[table][Alarm_tbl_num[table]] = offset;

      /* Cast the current buffer pointer to the main alarms table structure.
         The size of the record can be determined from the size element
         in the main structure.  The start of the next record is the current	
	 offset plus the current record size. */
      alarm = (RDA_alarm_entry_t *) (Alarm_tbl[table]+offset);
      offset += ALIGNED_SIZE(sizeof(RDA_alarm_entry_t));

      /* Update the reference record by alarm code lookup table */
      code = alarm->code;
      if( code <= ORPGRAT_MAX_RDA_ALARMS ){

         if( Code_to_index[table][code] < 0 )
            Code_to_index[table][code] = Alarm_tbl_num[table];

         else if( code != 0 ){

            /* non-zero duplicated Alarm Code */
            LE_send_msg( GL_INPUT, "Duplicated Alarm Code (%d) in RDA Alarms Table\n",
                         code );

         }

      }

      Alarm_tbl_num[table]++;

   }

   Need_update[table] = 0;

   return 0;

/* ORPGRAT_read_rda_alarms_tbl() */
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to an RDA alarm	*
 *		  text string.						*
 *									*
 *  Inputs:	  code - alarm code 					*
 *									*
 *  Return:	  A NULL pointer on failure or a pointer to the		*
 *		  specified text on success.				*
 *									*
 ************************************************************************/
char* ORPGRAT_get_alarm_text( int code ){


   RDA_alarm_entry_t *entry;

   /* Get the alarm table entry, then return the alarm text. */
   entry = (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data( code );
   if( entry == NULL )
      return( NULL );

   return( entry->alarm_text );

}
   
/************************************************************************
 *									*
 *  Description:  This function returns a pointer to an alarms		*
 *		  table record.	 This is the Public Interface.		*
 *									*
 *  Inputs:	  code - alarm code 					*
 *									*
 *  Return:	  A NULL pointer on failure or a pointer to the		*
 *		  specified record on success.				*
 *									*
 ************************************************************************/
char* ORPGRAT_get_alarm_data( int code ){

   int config;

   /* Get the table to access from ORPGRDA. */
   config = ORPGRDA_get_rda_config( NULL );
   if( config == ORPGRDA_LEGACY_CONFIG )
      Table_in_use = ORPGRAT_RDA_TABLE;

   else 
      Table_in_use = ORPGRAT_ORDA_TABLE;
   
   if( Need_update[Table_in_use] )
      ORPGRAT_read_rda_alarms_tbl( Table_in_use );
   if( Need_update[Table_in_use] )
      return (NULL);

   return( Get_alarm_data( Table_in_use, code ) );

/* End of ORPGRAT_get_alarm_data() */
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to an alarms		*
 *		  table record.  This is the Public Interface.		*
 *									*
 *  Inputs:	  code - alarm code 					*
 *                table - table number (ORPGRAT_RDA_TABLE or		*
 *                        ORPGRAT_ORDA_TABLE				* 
 *									*
 *  Return:	  A NULL pointer on failure or a pointer to the		*
 *		  specified record on success.				*
 *									*
 ************************************************************************/
char* ORPGRAT_get_table_alarm_data( int table, int code ){

   if( Need_update[table] )
      ORPGRAT_read_rda_alarms_tbl( table );
   if( Need_update[table] )
      return (NULL);

   return( Get_alarm_data( table, code ) );

/* End of ORPGRAT_get_table_alarm_data() */
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to an alarms		*
 *		  table record.  This is the Private Interface.		*
 *									*
 *  Inputs:	  code - alarm code 					*
 *                table - table number (ORPGRAT_RDA_TABLE or		*
 *                        ORPGRAT_ORDA_TABLE				* 
 *									*
 *  Return:	  A NULL pointer on failure or a pointer to the		*
 *		  specified record on success.				*
 *									*
 ************************************************************************/
static char* Get_alarm_data( int table, int code ){

   static RDA_alarm_entry_t spare = { ORPGRAT_UNDEFINED_CODE,
                                      ORPGRAT_NOT_APPLICABLE,
                                      ORPGRAT_NOT_APPLICABLE,
                                      ORPGRAT_NOT_APPLICABLE,
                                      ORPGRAT_NOT_APPLICABLE,
                                      ORPGRAT_NOT_APPLICABLE,
                                      "SPARE" };
   char *ret;
   int ndx = 0;

   ret = (char *) NULL;
   if( (Alarm_tbl[Table_in_use] != (char *) NULL) 
                  && (code >= 0) 
                  && (code <= ORPGRAT_MAX_RDA_ALARMS) ){

      ndx = Code_to_index[Table_in_use][code];

      if( ndx == ORPGRAT_DATA_NOT_FOUND ){

         /* Fill in undefined fields. */
         spare.type = ORPGRAT_NOT_APPLICABLE,
         spare.device = ORPGRAT_NOT_APPLICABLE,
         spare.sample = ORPGRAT_NOT_APPLICABLE,
         spare.state = ORPGRAT_NOT_APPLICABLE,

         /* List it as a "spare" alarm. */
         strcpy( spare.alarm_text, "SPARE" );
         spare.code = code; 

         ret = (char *) &spare;

      }
      else
         ret = (char *) (Alarm_tbl[Table_in_use] + Alarm_tbl_ptr[Table_in_use][ndx]);

   }

   return ret;

/* End of Get_alarm_data() */
}

