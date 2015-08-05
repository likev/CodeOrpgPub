/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:26:53 $
 * $Id: saausers_tab.h,v 1.1 2004/01/21 20:26:53 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/************************************************************************
Module:        saausers_tab.h

Description:   include file for the tabular alphanumeric block function of 
               cpc014/tsk014 which creates output for the Snow Accumulation 
               Algorithm.
               This file contains module specific include file listings, 
               function prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:       
               Initial implementation 8/08/03 - Zittel
               
************************************************************************/
#ifndef _SAAUSERS_TAB_H_
#define _SAAUSERS_TAB_H_

#include <stdio.h>
#include <stdlib.h>

#include <rpgc.h>
#include <rpg_globals.h>  /* includes a309.h for prod_id definitions          */
#include <product.h>      /* for message header block structure               */

/* the following extern definition is needed for recclalg_adapt.h             */
#define EXTERN extern

#include "saa_adapt.h"
#include "saa_arrays.h"

#define NUM_TAB_PAGES 2        /* number of pages in the TAB block            */
#define LINE_WIDTH 80          /* width of each line of data in the TAB       */
#define SAA_SUCCESS 0
#define SAA_FAILURE -1
#define FALSE 0
#define TRUE 1

typedef struct {
   short divider;     /* block divider (always -1)                            */
   short block_id;    /* block ID (always 1)                                  */
   int block_length;  /* length of TAB block in bytes                         */
   } TAB_header;

typedef struct {
   short divider;    /* block divider (always -1)                             */
   short num_pages;  /* total number of pages in the TAB product              */
   } alpha_header;

typedef struct {
   short num_char;  /* number of characters per line                          */
   char data[80];   /* container for one line of TAB output                   */
   } alpha_data;

/* local prototypes                                                           */
int generate_TAB(char *,int,int,int);
void calendar_date (short date,int *dd,int *dm,int *dy);
char *msecs_to_string (int time);
char *hours_to_string( int hour);

#endif

