/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:55 $
 * $Id: res.c,v 1.6 2009/05/15 17:52:55 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* res.c */
/* functions dealing with product resolutions */

#include "res.h"


/* converts between resolution index and actual resolution */
float res_index_to_res(int res)
{
  return resolution_number_list[res];
}





/*********************************************************/
/* TEMPORARY KLUDGE FUNCTION - A workaround for generic  */ 
/* radial products having inherent resolution rather than*/
/* a configured resolution.  This artificially sets the  */
/* sd->resolution field so product overlayed on top can  */
/* read the resolution.  A permanent fix is to place the */
/* actual resolution in the screen data                  */
/*********************************************************/
int temporary_set_screen_res_index(float res)
{
    
    if( (res > 149.0) && (res < 151.0)  )   /* res == 150 */
       return(1);
    else if( (res > 249.0) && (res < 251.0) )  /* res == 250 */
       return(2);
    else if( (res > 299.0) && (res < 301.0) )  /* res == 300 */
       return(3);
    else if( (res > 462.0) && (res < 464.0) )  /* res == 463   */
       return(4);
    else if( (res > 499.0) && (res < 501.0) )  /* res == 500   */
       return(5);
    else if( (res > 599.0) && (res < 601.0) )  /* res == 600  */
       return(6);
    else if( (res > 925.0) && (res < 927.0) ) /* res == 926  */
       return(7);
    else if( (res > 999.0) && (res < 1001.0) )  /* res == 1000  */
       return(8);
    else if( (res > 1999.0) && (res < 2001.0) )  /* res == 2000  */
       return(9);
    else if( (res > 3999.0) && (res < 4001.0) )  /* res == 4000  */
       return(10);
    else
       return(0);
    
}













/******************************************************
 * THE FOLLOWING FUNCTIONS ARE NO LONGER USED
 *****************************************************/

/* uses the resolution info above to figure out the range of a product 
 * we return an int, since that's what people usually refer to the ranges
 * as, and these calculations only have about 2 sig figs
 */
int calc_prod_range(int prod_id, int num_bins)
{
    float the_res;
    int  *res_index;

    res_index = assoc_access_i(product_res, prod_id);
    if(res_index == NULL)
      the_res = 0.0;
    else
      the_res = res_index_to_res(*res_index);
    return (int)(the_res * (float)(num_bins));
} 

/* uses the resolution info above to figure out the range
 * of a particular bin
 */
float calc_range(int prod_id, int bin)
{
    float the_res, out;
    int  *res_index;

    res_index = assoc_access_i(product_res, prod_id);
    if(res_index == NULL) {
      the_res = 0.0f;
      if(verbose_flag)
  	  printf("Could not find product ID %d\n", prod_id);
    } else {
      the_res = res_index_to_res(*res_index);
    }

    out = the_res * bin;

    return out;
} 











