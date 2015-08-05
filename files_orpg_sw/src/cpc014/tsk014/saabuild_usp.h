/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 21:32:35 $
 * $Id: saabuild_usp.h,v 1.2 2008/01/04 21:32:35 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saabuild_usp.h

Description:   include file for the Snow Accumulation Algorithm product
	       generator of cpc014/tsk014 which creates output for the
	       User Selectable Storm Water and Depth Accumulation products. 
               This file contains module specific include file 
               listings, function prototypes and constant definitions.
                
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:       
               Initial implementation 8/08/03 - Zittel
               
*******************************************************************************/

#ifndef _SAABUILD_USP_H_
#define _SAABUILD_USP_H_

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
int generate_sdt_output(char *, int, int, int, int);
int saa_max_value(short array[],int* x, int* y);
/*int generate_usp_output(char *, int, int); */
/* extern prototypes -------------------------------------------------------- */
extern short build_saa_symbology_layer(short *buffer,int msg_type,
              int* length,short radial_data[MAX_SAA_RADIALS][MAX_SAA_BINS],int prod_id,int);
extern short build_nullsymbology_layer(short *buffer,int msg_type,int* length,int prod_id,int);
extern void finish_SDT_pdb(short*,short,short,int,int,int);
extern int generate_TAB(char *,int,int,int);

    
#endif


