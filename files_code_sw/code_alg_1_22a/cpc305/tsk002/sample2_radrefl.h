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
Module:         sample2_radrefl.h

Description:    include file for the 16-level reflectivity algorithm
                sample2_radrefl.c. This file contains module specific
		          include file listings, function prototypes and 
		          constant definitions.
----------------------------------------------------------------------------
          Modified to reflect the new size of the base data header
          
----------------------------------------------------------------------------                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.5,  April 2005   T. Ganger
              Reflect the new size of the base data header (Build 5)
              Modified to use the new product IDs
              
Version 1.6   March 2006   T. Ganger
              Modified to reflect the new base data message, larger
                   header, new order of moment data, possible 
                   larger reflectivity array - Build 8

Version 1.7   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
                  
Version 1.8   February 2008    T. Ganger    (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated definition of output buffer ids.
              Changed reflectivity product from maximum 230 bins to a
                  maximum 460 bins.
              Used globally defined constants for maximum number of bins
                  and maximum number of radials.
              Passed calibration constant as a float.

Version 1.9   February 2009    T. Ganger    (Sample Algorithm ver 1.21)
              Added function to print dual pol data parameters.
              Added max size estimate for SR output data.

$Id$
************************************************************************/
#ifndef _RAD_REFL_H_
#define _RAD_REFL_H_

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

#include <rpgc.h>
#include "rpgcs.h"


/* -------------------------------------------------------------------- */
/* the structure of a base data radial message, which is ingested by the*/
/* algorithm, is defined in  basedata.h                                 */

/* 
 * The order and size of the base data data arrays are no longer fixed.
 *
 * The offsets in the base data header or the API access function must
 * be used to read the basic moments: reflectivity, velocity, and
 * spectrum width.
 *
 * Fields in the base data header are used to determine array size.
 */

/* The contents of Base_data_header is defined in basedata.h  */ 
/* -------------------------------------------------------------------- */



/* local  includes ---------------------------------------------------- */


/* ORPG constants not defined in include files ------------------------ */

/* number of bins per radial (BPR) (can be set to 230 up_to 460)        */
/* if using the new SR_ data types, this could be set to 1840           */
/* this is used to set the value of a single    */
/*             variable in the main module      */
#define PROD_RANGE_NUM_BINS 460  /* WARNING - This must correspond to   */
                     /* the value set for BUFSIZE below                 */
                     


/* Algorithm specific constants/definitions --------------------------- */

/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define RADREFL 1991 */  /* ORPG Linear Buffer Code, normally */
                            /* defined in a309.h , must match    */
                            /* the product_tables configuration  */
                            /* file entry for this product       */



#define BUFSIZE  92000      /* estimated output size for ICD product    */
                            /* run length encoded products will vary    */
                            /* in size, this must be larger than any    */
                            /* product produced by the algorithm        */

/* The following is estimated mas size for SR_ data of at least 720     */
/* radials and 1840 bins                                                */
#define SR_BUFSIZE 735000

/* function prototypes -------------------------------------------------*/
void clear_buffer(char*,int);

extern short build_symbology_layer(short* buffer,char **radial,int rad_count,
                                                  int bin_count, int* length);

extern void finish_pdb(short* buffer, short elev_ind, short elevation,
                      float calib_const, short max_refl, int prod_len);

extern int print_message_header(char* buffer);

extern int print_pdb_header(char* buffer);

/* Sample Alg 1.21 */
extern void test_read_dp_moment_params(Base_data_radial *radialPtr);

#endif


