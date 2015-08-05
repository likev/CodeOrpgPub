/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/03 22:50:16 $
 * $Id: saaprods_main.h,v 1.4 2008/01/03 22:50:16 aamirn Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


/************************************************************************
Module:        saaprods_main.h

Description:   include file for the main driving function of cpc014/tsk013
               which creates output for the Snow Accumulation Algorithm.
               This file contains module specific include file listings,
               function prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
Adapted by:    Dave Zittel, Meteorologist, ROC/Applications
               for the Snow Algorithm
               
History:       
               Initial implementation 1/31/02 - Stern
               Adapted for SAA 8/30/03 - Zittel
                              04/19/06 - Zittel
                                 Changed BUFSIZE from 40K to 90K
               
************************************************************************/
#ifndef _SAAPRODS_MAIN_H_
#define _SAAPRODS_MAIN_H_

/* system includes ---------------------------------------------------- */


/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>

/*** the following are included in rpg_globals.h */
/***                                             */
/***  #include <stdio.h>                         */
/***  #include <stdlib.h>                        */
/***  #include <time.h>                          */
/***  #include <ctype.h>                         */
/***  #include <string.h>                        */
/***                                             */
/***  #include <basedata.h>                      */
/***  #include <rpg_port.h>                      */
/***  #include <prod_gen_msg.h>                  */
/***  #include <prod_request.h>                  */
/***  #include <prod_gen_msg.h>                  */
/***  #include <a309.h>                          */
/***  #include <rpg_vcp.h>                       */
/***  #include <orpg.h>                          */
/***  #include <rpg.h>                           */
/***  #include <mrpg.h>                          */
/*************************************************/

#include <rpgc.h>


/* local  includes ---------------------------------------------------- */
#include <saa_arrays.h>    /* this is taken directly from cpc004/tsk006 */
                           /* and contains the structures included in   */
                           /* both incoming intermediate products       */

#define BUFSIZE  90000      /* estimated output size for ICD product    */
                            /* run length encoded products will vary    */
                            /* in size, this must be larger than any    */
                            /* product produced by the algorithm        */
                            
/* function prototypes -------------------------------------------------*/

extern short build_symbology_layer(short *buffer,
   int count,int* length,short radial_data[][MAX_SAA_BINS],int prod_id,int);
extern void finish_SDT_pdb(short*,short,short,int,int,int);
extern void build_saa_color_tables(void);
 int generate_sdt_output(char *, int, int);


#endif
