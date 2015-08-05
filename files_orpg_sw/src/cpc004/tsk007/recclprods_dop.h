/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:25 $
 * $Id: recclprods_dop.h,v 1.3 2002/11/26 22:07:25 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_dop.h

Description:   include file for the Doppler product generator of 
               cpc004/tsk007 which creates output for the Radar Echo 
               Classifier. This file contains module specific include file 
               listings, function prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:       
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_dop.h,v 1.3 2002/11/26 22:07:25 nolitam Exp $
*******************************************************************************/

#ifndef _RECCLPRODS_DOP_H_
#define _RECCLPRODS_DOP_H_

#include <stdio.h>
#include <stdlib.h>
#include <a309.h>  /* NOTE: Required to access product buffer numbers! */
#include <rpgc.h>

/* local include - must be the same as in cpc004/tsk006                       */
#include <recclalg_arrays.h>

#define FALSE 0
#define TRUE 1

/* success/failure return values */
#define REC_SUCCESS 0
#define REC_FAILURE -1

/* local prototypes --------------------------------------------------------- */
int generate_dop_output(char *, char *,int);

/* extern prototypes -------------------------------------------------------- */
extern short build_symbology_layer(short *buffer,char *inbuf,
   int count,int* length,short radial_data[][MAX_1KMBINS],int prod_id,int);
extern int radials_to_process(char *);    
extern void finish_pdb(short*,short,short,int,int,int);
extern int generate_TAB(char *,char *,int,int,int);
    
#endif



