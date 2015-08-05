/*
 * RCS info
 * $Author: dzittel $
 * $Locker:  $
 * $Date: 2005/02/17 15:49:53 $
 * $Id: saaaccum_usp.h,v 1.3 2005/02/17 15:49:53 dzittel Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaaccum_usp.h

Description:   include file for the accumulator function of cpc014/tsk014 which
	       creates User Selectable Water Equivalent and Depth products for 
	       the Snow Accumulation Algorithm. This file contains module specific
	       include file listings, function prototypes and constant definitions.
                
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:       
               Initial implementation 8/08/03 - Zittel
               11/05/2004	SW CCR NA04-30810	Build8 changes 
               
*******************************************************************************/

#ifndef _SAAACCUM_USP_H_
#define _SAAACCUM_USP_H_

#include <stdio.h>
#include <stdlib.h>
#include <a309.h>  /* NOTE: Required to access product buffer numbers! */
#include <rpgc.h>

/* local include - must be the same as in cpc004/tsk006                       */
#include <saa_arrays.h>

#define FALSE 0
#define TRUE 1

/* success/failure return values */
#define SAA_SUCCESS 0
#define SAA_FAILURE -1

/* local prototypes --------------------------------------------------------- */
int read_SAAUSERSEL_lb(int);

/* prototype for library function                                             */ 
extern int compute_area(short array[],int x, int y, int z);     /* Build8 name of function changed  
							    		and moved to saalib             */
int saa_max_value(short array[],int* x, int* y);   
#endif


