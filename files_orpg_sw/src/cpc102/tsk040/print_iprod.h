/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/05/23 21:02:45 $
 * $Id: print_iprod.h,v 1.1 2007/05/23 21:02:45 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef PRINT_IPROD_H
#define PRINT_IPROD_H

#include <stdlib.h>
#include <stdio.h>
#include <orpg.h>
#include <rpgc.h>
#include <a308buf.h>
#include <a313buf.h>
#include <prodsel.h>
#include <a317buf.h>

/* Macro for the number of prods and the global array for holding their ID
   names. For future add-ons, simply alter the macro and array as necessary. 
   Will also need to add the necessary logic to Print_prod_data(). */
#define PRINT_IPROD_NUM_PRODS      3

/* Miscellaneous program macros */
#define PRINT_IPROD_SUCCESS        0
#define PRINT_IPROD_FAILURE        1
#define PRINT_IPROD_EXIT_CODE      PRINT_IPROD_NUM_PRODS + 1

/* Function Prototypes */
void Print_menu();
int Get_main_menu_selects( unsigned short select_arr[PRINT_IPROD_NUM_PRODS]);
int Get_prod_types( unsigned short p_idx_arr[PRINT_IPROD_NUM_PRODS],
   int p_type_arr[PRINT_IPROD_NUM_PRODS]);
int Process_input_data( int data_types[PRINT_IPROD_NUM_PRODS] );
void Print_prod_data( int prod_type, void *buf);
void Print_combattr_data( void *combatt_buf );
void Print_ettab_data( void *ettab_buf );
void Print_vadtmhgt_data( void *vadtmhgt_buf );

#endif		/* #ifndef PRINT_IPROD_H */
