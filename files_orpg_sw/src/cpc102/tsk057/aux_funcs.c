/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:31:31 $
 * $Id: aux_funcs.c,v 1.4 2005/06/02 19:31:31 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <orpg.h>
#include <time.h>

/* Function Prototypes. */
char* Get_date_time_str( int date, int time );
char* Get_product_mnemonic_str( int prod_code );

/*************************************************************************************

   Description:  Generates a date/time string.

   Inputs:       date - Modified Julian Date.
                 time - Number of seconds since midnight.

   Returns:      date/time string in format mm/dd/yy hr:mm:ss

*************************************************************************************/
char* Get_date_time_str( int date, int time ){

   int ret, len;
   int year = 0;
   int month = 0;
   int day = 0;
   int hours = 0;
   int minutes = 0;
   int seconds = 0;
   time_t time_value = ((date-1)*86400) + time;

   static char date_time[20];

   memset( date_time, 0, 20 );

   ret = unix_time( &time_value, &year, &month, &day,
                    &hours, &minutes, &seconds );
   if( ret >= 0 ){
 
      if( year >= 2000 )
         year = year - 2000;
      else
         year = year - 1900;    

   }

   len = sprintf( date_time, "%02d/%02d/%02d %02d:%02d:%02d", 
                  month, day, year, hours, minutes, seconds );

   return ( (char*) date_time);

/* End of Get_date_time_str() */
}

/***********************************************************************************

   Description:  Returns Product mnemonic.

   Inputs:       prod_code - Product code.

***********************************************************************************/
char* Get_product_mnemonic_str( int prod_code ){

   int prod_id;
   static char *mnemonic;
   static char mne[4] = "   ";

   /* Initialize the mnemonic string to NULL string. */
   mnemonic = NULL;

   /* Get the product ID from product code. */
   prod_id = ORPGPAT_get_prod_id_from_code( prod_code );


   /* If valid product ID, get product mnemonic. */
   if( prod_id > 0 )
      mnemonic = ORPGPAT_get_mnemonic( prod_id );

   if( mnemonic == NULL )
      mnemonic = mne;

   return (mnemonic);

/* End of Get_product_mnemonic_str() */
}
