/**************************************************************************
   
   Module:  otr_find_product.c
   
   Description:
   This finds the product for a given time within the
   database, if it exists. If the time specified indicates
   it is a request for the latest product, then there is
   no time constraint on getting the product.


   Assumptions:
   
**************************************************************************/
/*
* RCS info
* $Author: steves $
* $Locker:  $
* $Date: 2009/09/03 21:13:34 $
* $Id: otr_find_product.c,v 1.25 2009/09/03 21:13:34 steves Exp $
* $Revision: 1.25 $
* $State: Exp $
*/
/*
* System Include Files/Local Include Files
*/

#include <prod_user_msg.h>
#include <prod_distri_info.h>
#include <stdlib.h>
#include <otr_find_product.h>
#include <orpgdbm.h>
#include <orpgerr.h>
#include <orpgdat.h>
#include <orpgpat.h>


/* Constant Definitions/Macro Definitions/Type Definitions */
/* permit a difference of 15 minutes from the time
   requested for the volume, and the actual volume time */
/* this number came from the legacy code a302a3.ftn */
#define PERMITTED_TIME_DIFFERENCE 900
#define MAX_PRODUCTS_IN_LIST 1000 

/* permit a difference of 0.5 degrees from the elevation requested
   and the actual elevation parameter (if elevation-based) */
#define PERMITTED_ELEV_DIFFERENCE   5

/* Static Globals */

/*
* Static Function Prototypes
*/

/**************************************************************************
   Description: 
      This finds if a specific product exists in the product data base.
   
   Input:  
      request_ptr - The request message for the product.
   
   Output:
      lb_return - is a pointer to the place to return the LB number.
      msg_return - is a pointer to the place to return the message number in the LB.
      volume_number - is a pointer to the place to return the volume number the product
                      was found in. 
   
   Returns: 
      The return value is 0 for no product found, or 1 if a product was
      found.  If a product was found, the LB id and the message id for 
      the product is returned through calling arguments along with the
      volume number the product was found in.
   
   Notes:  

**************************************************************************/
int OTR_find_product(Pd_request_products *request_ptr, int *lb_return, 
		      int *msg_return, int *volume_number){

   int product_id;    	/* ID of product to search for */
   int product_code;  	/* product code of product to search for */
   int time = 0;      	/* time requested for the product */
   int ret;           	/* number returned from query */
   int time_difference; /* smallest time difference found in the loop so far */
   int t_diff;          /* time difference for the current product */
   int elev_difference; /* smallest elev difference found in the loop so far */
   int elev;            /* current elevation */
   int e_diff;          /* elev difference for the current product */
   int i,j,k;         	/* indexes for for loops */
   int elev_based = -1; /* index for elevation parameter */
   int product_found; 	/* indicates the parameters are the same for the product */
   int q_data_field;  	/* index for the data fields being set for the query */

   static ORPGDBM_query_data_t query_data [4];      /* data controling what is returned 
                                                       from the query */
   static RPG_prod_rec_t db_info [MAX_PRODUCTS_IN_LIST];   /* database info returned 
                                                              from the query */

   LE_send_msg(GL_INFO | LE_VL3, "Attempting to Find Product in Products Database \n");    
   product_id = request_ptr->prod_id;
   product_code = ORPGPAT_get_code(product_id);
   
   q_data_field = 0;
   query_data [q_data_field].field = ORPGDBM_MODE;
   query_data [q_data_field++].value = ORPGDBM_PARTIAL_SEARCH | ORPGDBM_HIGHEND_SEARCH;
   			      
   query_data [q_data_field].field = RPGP_PCODE;
   query_data [q_data_field++].value = product_code;

   /* If volume scan start time is non-negative, then the user asked for a 
      particular date/time.   Otherwise, the user asked for latest product
      available. */
   if (request_ptr->VS_start_time >= 0){

      /* if negative time, then latest product will do, but this is for a 
         specific time, so specify the time * range that is acceptable */
      /* convert request time to Julian time */
      time = request_ptr->VS_start_time + ((request_ptr->VS_date - 1) * 86400) ;

      query_data [q_data_field].field = ORPGDBM_VOL_TIME_RANGE;
      query_data [q_data_field].value = time - PERMITTED_TIME_DIFFERENCE;
      query_data [q_data_field++].value2 = time + PERMITTED_TIME_DIFFERENCE;

   }

   query_data [q_data_field].field = ORPGDBM_ELEV_RANGE;
   elev_based = ORPGPAT_elevation_based (product_id);
   if (elev_based < 0)
      query_data [q_data_field++].value = SDQM_UNDEF_SHORT;
   else{

      /* Check if elevation angle is encoded as negative angle.  if so, decode it
         into a negative angle. */
      if( request_ptr->params[elev_based] > 1800 ){

         LE_send_msg( GL_INFO, "Convert Prod Req Elev Param to Neg Ang for DB Query: %d\n",
                      request_ptr->params[elev_based] );
         request_ptr->params[elev_based] -= 3600;

      }
      
      /* If the product is elevation-based (i.e., has an elevation parameter), then
         the search is over an elevation range. */
      query_data [q_data_field].value =  (short) request_ptr->params[elev_based] - 
                                                 PERMITTED_ELEV_DIFFERENCE;
      if( query_data [q_data_field].value < ORPGDBM_MIN_ELEV )
         query_data [q_data_field].value = (short) ORPGDBM_MIN_ELEV;

      query_data [q_data_field].value2 =  (short) request_ptr->params[elev_based] + 
                                                  PERMITTED_ELEV_DIFFERENCE;
      if( query_data [q_data_field].value2 > ORPGDBM_MAX_ELEV )
         query_data [q_data_field].value2 = (short) ORPGDBM_MAX_ELEV;

      q_data_field++;

   }
   
   ret = ORPGDBM_query( db_info, query_data, q_data_field , MAX_PRODUCTS_IN_LIST );

   if( ret <= 0 ){

      LE_send_msg( GL_INFO | LE_VL3, 
                   "--->ORPGDBM_queury Returned %d .... No Product(s) Found\n", ret );
      return(0);

   }
   else{

      /* search the products returned to see if the parameters match */
      time_difference = ORPGDBM_MAX_VOLT;
      elev_difference = ORPGDBM_MAX_ELEV;
      elev = ORPGDBM_MAX_ELEV;
        
      for( i=0; i < ret; i++ ){

         LE_send_msg(GL_INFO | LE_VL3, 
             "--->Checking Candidate %d of %d, msg: %d vol_t: %d, prod: %d, elev: %d\n",
             (i+1), ret, db_info[i].msg_id, db_info[i].vol_t, db_info[i].prod_code, 
             db_info[i].elev );
         LE_send_msg(GL_INFO | LE_VL3, "--->params: %6d %6d %6d %6d %6d %6d\n",
             db_info[i].params[0], db_info[i].params[1], db_info[i].params[2], 
             db_info[i].params[3], db_info[i].params[4], db_info[i].params[5]);                       

         /* If product code matches, then ..... */
         if( (db_info[i].msg_id >= 0) && (product_code == db_info[i].prod_code) ){

            product_found = 1;

            /* found a product, now see if parameters match */
            for( j = 0; j < ORPGPAT_get_num_parameters (product_id); j++ ){

               /* note: this should be similar to the OTR_compare_parameters
                        function in otr_new_volume_product_request. 
                        The same function is not used here because the
                        elevation index cannot be used - the VCP may have
                        changed after the product was stored. This also
                        needs to check both reply parameters and request
                        parameters e.g. a stored storm motion product
                        that was generated with request parameters of
                        PARAM_ALG_SET and replies with 10 knots, should
                        match request for both PARAM_ALG_SET requests, and requests
                        for 10 knots for the parameter. */
               k = ORPGPAT_get_parameter_index (product_id, j);
               if( (PARAM_ANY_VALUE != request_ptr->params[k]) &&
                   (PARAM_ANY_VALUE != db_info[i].params[k]) &&
                   (PARAM_UNUSED    != request_ptr->params[k]) &&
                   (PARAM_UNUSED    != db_info[i].params[k]) &&
                   ((k != elev_based) && (db_info[i].params[k] != request_ptr->params[k])) &&
                   (PARAM_ANY_VALUE != db_info[i].req_params[k]) &&
                   (PARAM_UNUSED    != db_info[i].req_params[k]) &&
                   ((k != elev_based) && (db_info[i].req_params[k] != request_ptr->params[k])) ){

                  product_found = 0; 
                  break;

               } 

            }

            /* If the product not matched, check next product returned from query. */
            if( !product_found )
               continue;

            /* Product match found ... */ 
            if( request_ptr->VS_start_time < 0 ){

               LE_send_msg(GL_INFO | LE_VL3, "--->Match on %d of %d \n",i, ret);  

               /* If product request is for latest available, then this product is 
                  the one to return since we did a high end search of the data base. */

               time_difference = 0;

               *lb_return = ORPGDAT_PRODUCTS;
               *msg_return = db_info[i].msg_id;
               *volume_number = 0;  

               break;

            }
            else{

               t_diff = abs(db_info[i].vol_t - time);

               /* If the time difference for the current candidate matches the time
                  difference for another candidate product, then .... */
               if( t_diff == time_difference ){

                  /* If product is elevation-based, we want product with closest
                     elevation match.  In case of a tie, take the lower elevation. */
                  if( elev_based ){

                     e_diff = abs(db_info[i].params[elev_based] - request_ptr->params[elev_based]);
                     if( e_diff <= elev_difference ){

                        if( (elev_difference != e_diff) 
                                      ||
                            (db_info[i].params[elev_based] < elev) ){

                           elev_difference = e_diff;
                           elev = db_info[i].params[elev_based];

                           *lb_return = ORPGDAT_PRODUCTS;
                           *msg_return = db_info[i].msg_id;
                           *volume_number = 0;  

                        }

                     }
                        
                  } 

               }
               else if( t_diff < time_difference ){

                  /* Save the time difference and keep looking for one that's closer */
                  time_difference = t_diff;

                  /* If elevation based, save the elevation and elevation difference for
                     possible future comparison. */
                  if( elev_based ){

                     elev_difference = 
                        abs(db_info[i].params[elev_based] - request_ptr->params[elev_based]);
                     elev = db_info[i].params[elev_based];

                  }

                  *lb_return = ORPGDAT_PRODUCTS;
                  *msg_return = db_info[i].msg_id;
                  *volume_number = 0;  

               }

               /* If time difference is 0, no need to look any further. */             
               if( time_difference == 0 ){

                  LE_send_msg(GL_INFO | LE_VL3, "--->Match on %d of %d \n",i, ret);  
                  break;

               }

            }
                                          
         }

      }     

      /* If here and the time difference is less than the initialized value,
         we must have found a matching product. */
      if( time_difference < ORPGDBM_MAX_VOLT ){

         LE_send_msg( GL_INFO | LE_VL2, "--->Found Product (msg_id: %d) \n",
                      *msg_return );
         return(1);

      }
      else
         LE_send_msg( GL_INFO | LE_VL2, "--->Product Not Found in Database (ret: %d)\n",
                      ret);
   }

   return (0); 

}
