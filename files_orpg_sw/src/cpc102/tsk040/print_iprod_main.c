/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/05/23 21:02:47 $
 * $Id: print_iprod_main.c,v 1.1 2007/05/23 21:02:47 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <print_iprod.h>

/*******************************************************************************
* Description:
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int main( int argc, char *argv[] )
{
   int ret_val = PRINT_IPROD_SUCCESS; /* This function's return value */
   int ret; /* generic function call return value */
   unsigned short prod_indexes[PRINT_IPROD_NUM_PRODS] = {0}; /* product indexes,
                                                        corresponding to the
                                                        user's menu selections */
   int prod_types[PRINT_IPROD_NUM_PRODS] = {0}; /* product data types
                                                   corresponding to the chosen
                                                   products */


   /* Register for input and output data */
   RPGC_reg_io(argc, argv);

   /* Initialize task and register for log services. */
   RPGC_task_init( ELEVATION_BASED, argc, argv );

   /* Print main menu and get user's menu selections */
   ret = Get_main_menu_selects( prod_indexes );
   if ( ret == PRINT_IPROD_SUCCESS )
   {
      /* Fill the prod types array */
      ret = Get_prod_types(prod_indexes, prod_types);
      if ( ret != PRINT_IPROD_SUCCESS )
      {
         LE_send_msg(GL_INFO,
            "main: Error returned from Get_prod_types (%d)\n", ret);
         return (PRINT_IPROD_FAILURE);
      }

      /* Wait for activation. */
      while(1)
      {
         RPGC_wait_for_any_data( WAIT_ANY_INPUT );
         ret = Process_input_data( prod_types );
         if ( ret != PRINT_IPROD_SUCCESS )
         {
            LE_send_msg(GL_INFO, "main: Error in Process_input_data (%d)\n",
               ret);
            ret_val = PRINT_IPROD_FAILURE;
            break;
         }
      }
   }
   else
   {
      LE_send_msg(GL_INFO, "ERROR in main: Get_print_selections returned %d\n",
         ret);
      ret_val = PRINT_IPROD_FAILURE;
   }

   return (ret_val);

} /* end main */
