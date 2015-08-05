/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/05/21 19:59:35 $ */
/* $Id: clutprod_handle_volstart.c,v 1.2 2007/05/21 19:59:35 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <clutprod.h>

/* Function prototypes. */
static int Clutprod_query_dbm();

/***************************************************************************

   Description:
      Check product database for number of unique CFC products.  If less
      than the number expected, generate all the products.

   Inputs:
      num_products - number of unique products (in terms on parameters)
                     that there can be. 

   Returns:
      Number of unique products found.

***************************************************************************/
int Clutprod_handle_volstart( int num_products ){

    /* Local variables */
    int num_cfc_prods;

    num_cfc_prods = 0;

    /* Call function to check product database for CFC products and return 
       number found. */
    num_cfc_prods = Clutprod_query_dbm();

    /* If the number of unique CFC prods in the DB is less than "num_products", 
       we need to regenerate the CFC product. */
    if( num_cfc_prods < num_products ){

	RPGC_log_msg( GL_INFO, "Number of CFC Products (%d) < Expected (%d)\n",
                      num_cfc_prods, num_products );

        /* Need to set flag to generate all CFC prods */
	Clutinfo.generate_all_products = 1;

        /* Call routine to generate CFC products */
	Clutprod_generation_control();

    }

    return num_cfc_prods;

/* End of Clutprod_handle_volstart() */
} 

#define MAX_NUM_CFC_PRODS 	30


/************************************************************

      Description:
         This module queries the product database and returns
         the number of unique CFC products in the database.
         The number returned is obtained by examining all
         CFC products and counting how many different product
         dependent parameter values are found.

      Returns:
         Number of unique CFC products in the database
 
*************************************************************/
static int Clutprod_query_dbm(){

   int			stat;
   int			first_time;
   int			prod_index;
   int			num_unique_vals = 0;
   int			unique_val_index;
   int			unique_val_flag;
   int			unique_vals [ MAX_NUM_CFC_PRODS ] = {0};
   RPGC_prod_rec_t	db_info [ MAX_NUM_CFC_PRODS ];

   /* query the product database, searching for CFC prods */
   stat = RPGC_find_pdb_product ( "CFCPROD", db_info, MAX_NUM_CFC_PRODS, 
                                  QUERY_END_LIST );
   if( stat < 0 )
      RPGC_log_msg ( GL_INFO, "cfcprd__query_dbm: problem in ORPGDBM_query()\n");
    
   else if ( stat > 0 ){

      /* examine params[0] for each prod and count unique values */
      unique_val_index = 0;
      num_unique_vals = 0;
      first_time = 1;
     
      for ( prod_index = 0; prod_index < stat; prod_index++ ){

         unique_val_flag = 1;  /* assume unique product */
         if ( first_time ){

            first_time = 0;
            num_unique_vals++;  /* must be unique if first time */
            unique_vals[ num_unique_vals - 1 ] = db_info[prod_index].params[0];

         }
         else{

            for ( unique_val_index = 0; unique_val_index < num_unique_vals; unique_val_index++ ){

               if ( db_info[prod_index].params[0] == unique_vals[unique_val_index] )
                  unique_val_flag = 0;

            }
            if ( unique_val_flag == 1 ){

               num_unique_vals++;
               unique_vals[ num_unique_vals - 1 ] = db_info[prod_index].params[0];

            }

         }

      }

   }

   return num_unique_vals;

/* End of Clutprod_query_dbm() */
} 
