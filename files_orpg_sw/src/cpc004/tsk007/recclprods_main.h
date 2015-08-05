/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:27 $
 * $Id: recclprods_main.h,v 1.2 2002/11/26 22:07:27 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/************************************************************************
Module:        recclprods_main.h

Description:   include file for the main driving function of cpc004/tsk007
               which creates output for the Radar Echo Classifier. This file
               contains module specific include file listings, function 
               prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:       
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_main.h,v 1.2 2002/11/26 22:07:27 nolitam Exp $
************************************************************************/
#ifndef _RECCLPRODS_MAIN_H_
#define _RECCLPRODS_MAIN_H_

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
#include <recclalg_arrays.h>  /* this is taken directly from cpc004/tsk006 */
                              /* and contains the structures included in   */
                              /* both incoming intermediate products       */

/* the estimated max buffer size consists of the following data (in bytes):
   Msg Hdr/PDB        120
   Symb Block          10
   Layer Header         6
   Packet Header       14
   Packet Data      86612 ((230 bins+6 header bytes) x 367 radials)
   subtotal         86762
   allocated size  100000 allow for overflow, can handle 400+ radials   */

#define BUFSIZE  100000     /* estimated output size for ICD product    */
                            /* run length encoded products will vary    */
                            /* in size, this must be larger than any    */
                            /* product produced by the algorithm        */
   
/* function prototypes -------------------------------------------------*/
extern short build_symbology_layer(short* buffer,char **radial,int count,
    int* length);
extern void finish_pdb(short*,short,short,short,short,short,int);

extern int generate_refl_output(char *, char *,int);
extern int generate_dop_output(char *, char *,int);

#endif

/* ----------------------------------------
Product Codes can be found in a309.h
for REC algorithm  cpc004/tsk006
#define RECCLREF    132
#define RECCLDOP    133
#define RECCLDIGREF 298
#define RECCLDIGDOP 299
#define RECCLDIGREFTAB 110 
#define RECCLDIGDOPTAB 111
--------------------------------------------*/

