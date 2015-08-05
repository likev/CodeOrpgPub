/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 21:32:36 $
 * $Id: saausers_main.h,v 1.5 2008/01/04 21:32:36 aamirn Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */


/************************************************************************
Module:        saausers_main.h

Description:   Include file for the main driving function of cpc014/tsk014
               which creates output for the Snow Accumulation Algorithm's 
               User Selectable Snow Accumulation products. This file
               contains module specific include file listings, function 
               prototypes and constant definitions.
                
CCR#:          NA98-35001 - Initial implementation

               
Author:        David Zittel, Meterologist, Applications Branch, ROC
                   Walter.D.Zittel@noaa.gov
               Version 0, December 2003
               
History:       
               Initial implementation 12/01/04 - Zittel
               Build 9                04/19/06 - Zittel 
                                      Change BUFSIZE from 40K to 90K bytes
               
************************************************************************/
#ifndef _SAAUSERS_MAIN_H_
#define _SAAUSERS_MAIN_H_

/* system includes ---------------------------------------------------- */


/* ORPG   includes ---------------------------------------------------- */
#include "rpg_globals.h"

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
#include <saa_arrays.h>    /* this is taken directly from cpc013/tsk008 */
                           /* and contains the structures included in   */
                           /* both incoming intermediate products       */
                           
#include <saausers_arrays.h>  /*  This file contains structures and arrays */
                              /*  unique to the saausers task.                 */

/* the estimated max buffer size consists of the following data (in bytes):
   Msg Hdr/PDB        120
   Symb Block          10
   Layer Header         6
   Packet Header       14
   Packet Data      86612 ((230 bins+6 header bytes) x 367 radials)
   subtotal         86762
   allocated size   90000 allow for overflow, can handle 400+ radials   */

#define BUFSIZE  90000      /* estimated output size for ICD product    */
                            /* run length encoded products will vary    */
                            /* in size, this must be larger than any    */
                            /* product produced by the algorithm        */
                            
#define SAAUSER_BUFSIZ 331200   /* MAX_SAA_RADIALS*MAX_SAA_BINS*sizeof(short)*2  */
   
/* function prototypes -------------------------------------------------*/
extern short build_symbology_layer(short *buffer,
   int count,int* length,short radial_data[][MAX_SAA_BINS],int prod_id,int);
extern void finish_SDT_pdb(short*,short,short,int,int,int);
extern void build_saa_color_tables(void);
extern int generate_sdt_output(char *, int, int, int, int);
extern int get_hourly_index(void);
extern int init_user_lbs(int);
extern int get_next_lb(void);

int write_SAAUSERSEL_lb(int msg_id);
int write_USRSELHDR_lb(int msg_id);
extern int read_SAAUSERSEL_lb(int msg_id);
int read_USRSELHDR_lb(int msg_id);
int get_saa_usp_requests(int,int);
int generate_usp_output(char *, int, int, int);

int hi_sf_flg;

/*#endif */

#endif
