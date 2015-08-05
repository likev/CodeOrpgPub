/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/27 17:46:56 $
 * $Id: prcpuspt_fill_pgctable.c,v 1.9 2002/11/27 17:46:56 nolitam Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/***********************************************************************

	This module contains function prcpuspt_fill_pgctable.

***********************************************************************/


#ifdef SUNOS
#define prcpuspt_fill_pgctable prcpuspt_fill_pgctable_
#endif

#ifdef  LINUX
#define prcpuspt_fill_pgctable prcpuspt_fill_pgctable__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <orpg.h>
#include <rpg.h>
#include <prod_distri_info.h>


/***********************************************************************

    Description: This function fills elements of the PGCTABLE to support
                 legacy algorithm USP.  (The PGCTABLE is the Default 
                 product generation and distribution control table). 

    Inputs:	prod_code - the product code 
      		weather_mode - the weather mode (Clear Air or 
      		Precipitation).
		pgctable - address of table to fill.

                The pgctable format is as follows:

   			pgctable[0]  - product code
			pgctable[1]  - undefined
			pgctable[2]  - end time (hours) 
			pgctable[3]  - duration (hours)
			pgctable[4]  - generation frequency
			pgctable[5]  - archive frequency
			pgctable[6]  - storage frequency
			pgctable[7]  - storage time
			pgctable[8]  - NA PUP distribution
			pgctable[9]  - PUES distribution
			pgctable[10] - RFC distribution
			pgctable[11] - Other User distribution
			pgctable[12] - pgctable[19] are undefined
			               for the USP product


    Outputs:	pgctable - The pgctable is filled with the default 
            	settinhg for the specified product code and weather
                mode.

    Notes:      The storage, archive, and distribution data are not set 
  		within pgctable since this information is not needed by 
		the USP algorithms.

    Return:	The return value is not used.

************************************************************************/
int prcpuspt_fill_pgctable( int *prod_code, 
                            int *weather_mode, 
                            short *pgctable ){

   int buffer_num;
   int ret;
   int num_entries;
   int i;
   Pd_prod_entry *def_table;
   LB_info list;

   /*
     Find buffer number associated with this product code.
   */
   buffer_num = OB_get_buffer_number( *prod_code );
   if( buffer_num < 0 )
      PS_task_abort( "Unable to Fill PGCTABLE.  Invalid Product Code %d.\n",
                     *prod_code );

   /*
     Find out how large the default generation table is before reading
     the data.
   */
   ret = ORPGDA_info( ORPGDAT_PROD_INFO, PD_CURRENT_PROD_MSG_ID, &list ); 
   if( ret < 0 )
      PS_task_abort( "ORPGDA_info on ORPGDAT_PROD_INFO Failed.  Ret = %d.\n", 
                     ret );

   /*
     Allocate a block of memory for the default product generation list, then
     read the data in.
   */
   def_table = (Pd_prod_entry *) malloc( list.size );
   if( def_table == (Pd_prod_entry *) NULL )
      PS_task_abort( "Malloc Failed For Default PGC Table.\n" );
   
   ret = ORPGDA_read( ORPGDAT_PROD_INFO, (char *) def_table, list.size, 
                      PD_CURRENT_PROD_MSG_ID );
   if( ret < 0 )  
      PS_task_abort( "ORPGDAT_PROD_INFO Read Failed.  Ret = %d.\n", ret );

   /*
     Initialize the pgctable to 0.
   */
   for( i = 0; i < 20; i++ )
      pgctable[i] = 0;

   /*
     We must search the table for the product of interest.
   */
   num_entries = list.size / sizeof( Pd_prod_entry );
   for( i = 0; i < num_entries; i++ ){

      if( def_table[i].prod_id != (prod_id_t) buffer_num )
         continue;
      
      /*
        Found a match.  See if this product is to be generated for
        this weather mode.
      */
      if( ((*weather_mode+1) & (int) def_table[i].wx_modes) 
                          &&
          (def_table[i].gen_pr > 0) ){

         /*
           Product scheduled for this weather mode. 
         */
         pgctable[0] = *prod_code;		/* set product code */
         pgctable[2] = def_table[i].params[0];	/* set end time */
         pgctable[3] = def_table[i].params[1];	/* set duration */
         pgctable[4] = def_table[i].gen_pr;	/* set generation period */

         break;

      }

   }

   /*
     Free memory associated with default generation table.
   */
   if( def_table != (Pd_prod_entry *) NULL )
      free( def_table );
   return(0);
}
