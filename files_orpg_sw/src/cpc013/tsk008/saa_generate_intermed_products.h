/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:43 $
 * $Id: saa_generate_intermed_products.h,v 1.1 2004/01/21 20:12:43 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/*******************************************************************
File    : saa_generate_intermed_products.h
Created : Sept. 15,2003
Details : Header file for saa_generate_intermed_products.c, which
          generates the SAA intermediate products.
Author  : Reji Zachariah
Modification History: Khoi Dang added functions(October 10,2003)
			1. createLB
			2. write_LB
			3. write_LB_Header
			4. read_LB
			5. read_LB_Header
			6. init_LB
			7. get_nextLB
			8. get_hourly_index
			9. reset_usr_data
			10.generate_intermediate_usp_product

*********************************************************************/

#ifndef SAA_GENERATE_INTERMED_PRODUCTS_H
#define SAA_GENERATE_INTERMED_PRODUCTS_H

#include <stdlib.h>
#include <stdio.h>

/**********************CONSTANTS         *****************************/
static const float OHP_SWE_SCALING_FACTOR 	= 1000/25.4;
static const float OHP_SD_SCALING_FACTOR  	= 100/25.4;
static const float TOTAL_SWE_SCALING_FACTOR	= 100/25.4;
static const float TOTAL_SD_SCALING_FACTOR 	= 10/2.54;
static const short MAX_ACCUM_CAP		= 32767; /* 2^15-1 */
short data_valid_flag	;
void generate_intermediate_products(char* outbuf,int max_bufsize);


#endif
