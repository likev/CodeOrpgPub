/*
 * RCS info
 * $Author: dzittel $
 * $Locker:  $
 * $Date: 2005/02/17 15:37:16 $
 * $Id: saaprods_sdt.h,v 1.2 2005/02/17 15:37:16 dzittel Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saaprods_sdt.h

Description:   include file for the product generator portion of task
               cpc014/tsk013 which creates output for the Snow Accumulation
               Algorithm. This file contains module specific include file 
               listings, function prototypes and constant definitions.
                
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:       
         Initial implementation 8/08/03 - Zittel
	11/04/2004	SW CCR NA04-30810		Build8 changes cleanup

               
*******************************************************************************/

#ifndef _SAAPRODS_SDT_H_
#define _SAAPRODS_SDT_H_

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
int generate_sdt_output(char *, int, int);
int saa_max_value(short array[],int* x, int* y);

/* extern prototypes -------------------------------------------------------- */
extern short build_saa_symbology_layer(short *buffer,int msg_type,
              int* length,short radial_data[360][MAX_SAA_BINS],int prod_id,int);
extern short build_nullsymbology_layer(short *buffer,int msg_type,int* length,int prod_id,int);
extern void finish_SDT_pdb(short*,short,short,int,int,int);
extern int generate_TAB(char *,int,int,int);

    
#endif
