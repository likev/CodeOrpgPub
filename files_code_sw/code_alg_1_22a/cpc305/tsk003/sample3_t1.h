/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/************************************************************************
Module:         sample3_t1.h

Description:    include file for the sample3_t1 demonstration program. 
----------------------------------------------------------------------------
          Modified to reflect the new size of the base data header
          
---------------------------------------------------------------------------- 
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
              
Version 1.3,  April 2005   T. Ganger
              Modified to use the new product IDS                

Version 1.4   March 2006   T. Ganger
              Modified to reflect the new base data message, larger
                   header, new order or moment data, possible 
                   larger reflectivity array - Build 8

Version 1.5   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.               
                  
Version 1.6   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Eliminated definition of output buffer ids.
              New output buffer sizes calculated.
              
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Included new structure for intermediate product output.
                
$Id$
************************************************************************/
#ifndef _SAMPLE3_T1_H_
#define _SAMPLE3_T1_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>

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
/***  #include <a309.h>                          */
/***  #include <basedata.h>                      */
/***  #include <basedata_elev.h>                 */
/***  #include <rpg_port.h>                      */
/***  #include <prod_gen_msg.h>                  */
/***  #include <prod_request.h>                  */
/***  #include <prod_gen_msg.h>                  */
/***  #include <rpg_vcp.h>                       */
/***  #include <orpg.h>                          */
/***  #include <mrpg.h>                          */
/*************************************************/

/* required by rpgc after API enhancement patch */
#include <gen_stat_msg.h>

#include <rpgc.h>
#include <rpgcs.h>

/* -------------------------------------------------------------------- */
/* the structure of a base data radial message, which is ingested by the
   algorithm, is defined in  basedata.h  */

/* 
 * The order and size of the base data data arrays are no longer fixed.
 *
 * The offsets in the base data header or the API access function must
 * be used to read the basic moments: reflectivity, velocity, and
 * spectrum width.
 *
 * Fields in the base data header are used to determine array size.
 */

/* The contents of Base_data_header is also defined in basedata.h  */ 
/* -------------------------------------------------------------------- */



/* Local includes ----------------------------------------------------- */
/* #include "s3_cvg_struct.h" */
#include "s3_t1_prod_struct.h"

/* Algorithm specific constants/definitions --------------------------- */


/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define SAMPLE3_IP 1999  */


/* the estimated buffer size consists of the following data (in bytes):
 * (Pre ICD header     96 not included)
 * Header fields       24
 * Start Angles      1468 (4 x 367 radials)
 * Delta Angles      1468 (4 x 367 radials)
 * Base_data_header   200 (current size)
 * Radial Data Ptr      4
 * SUB TOTAL         3164
 * Radial Data      84410 (230 bins x 367 radials)
 * TOTAL            87574 (230 bins x 367 radials)
 * OR
 * Packet Data     168820 (460 bins x 367 radials)
 * TOTAL           171984 
 */

/* to allow for 400 radials, the following can be used if NOT using      */
/* one of the super resolution data types.                               */
#define BUFFSIZE 187164

/* the algorithm would need to calculate a different buffer size to       */
/* allocate if using one of the super resolution data types. For example  */
/*    1475164 for 800 radials and 1840 bins (full super resolution)       */
/*     739164 for 400 radials and 1840 bins                               */
/*     371164 for 800 radials and  460 bins                               */
/*     187164 for 400 radials and  460 bins (original resolution )        */

/* function prototypes ------------------------------------------------ */
void clear_buffer(char*);

#endif

