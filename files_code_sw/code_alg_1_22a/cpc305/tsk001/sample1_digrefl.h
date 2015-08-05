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
Module:         sample1_digrefl.h

Description:    include file for the digital reflectivity algorithm
                digital_reflectivity.c. This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
               
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org

Version 1.6,  March 2005   T. Ganger
              Reflect the new size of the base data header (Build 5)
              Modified to use the new product IDs
              
Version 1.7   March 2006   T. Ganger
              Modified to reflect the new base data message, larger
                   header, new order of moment data, possible 
                   larger reflectivity array - Build 8
              External product_to_disk function added.

Version 1.8   June 2006   T. Ganger  
              Modified to reflect the multiple outputs for differently 
                   named tasks  

Version 1.9   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
                  
Version 2.0   February 2008    T. Ganger    (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated definition of output buffer ids.
              New output buffer size calculated.
              
Version 2.1   February 2009    T. Ganger    (Sample Algorithm ver 1.21)
              Added function to print dual pol data parameters.
              
              

$Id$
************************************************************************/
#ifndef _DIGITAL_REFLECTIVITY_H_
#define _DIGITAL_REFLECTIVITY_H_

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

#include <packet_16.h>

/* include files for adaptation data demo */
#include <siteadp.h>


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


/* The contents of Base_data_header is defined in basedata.h  */ 
/* -------------------------------------------------------------------- */


/* local  includes ---------------------------------------------------- */
/* #include "s1_symb_layer.h" */


/* ORPG constants not defined in include files ------------------------ */



/* Algorithm specific constants/definitions --------------------------- */

/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define SR_DIGREFLBASE 1990  */
/* #define SR_DIGREFLRAW 1995   */


/* the estimated buffer size for non Super Res data (in bytes):
 * (Pre ICD header     96 not included)
 * Msg Hdr/PDB        120
 * Symb Block          10
 * Layer Header         6
 * Packet Header       14
 * Packet Data     171022 ((460 bins+6 header bytes) x 367 radials)
 * TOTAL           171172 
 */
 
/* to allow for 400 radials, the following can be used if not in a       */
/* Super Res elevation, the output buffer must be reallocated otherwise. */
#define BUFSIZE 184250

/* the algorithm will re-allocate the output buffer for larger size if    */
/* the basedata radial has increase surveillance resolution or 0.5 degree */
/* radials. For reference, the following buffer size are calculated needed.*/
/*    1472250 for 800 radials and 1840 bins (full super resolution)       */
/*     736250 for 400 radials and 1840 bins                               */
/*     368250 for 800 radials and  460 bins                               */
/*     184250 for 400 radials and  460 bins (original resolution )        */


/* function prototypes ------------------------------------------------ */
void clear_buffer(char*);

extern short build_symbology_layer(char* buffer, char **radial, 
                            int rad_count, int bin_count, int* length);

extern void finish_pdb(char* buffer, short elev_ind, short elevation, 
                                         short max_refl, int prod_len);

extern int print_message_header(char* buffer);

extern int print_pdb_header(char* buffer);

extern void product_to_disk(char * output_buf, int outlen, char *pname, 
                            short ele_idx);

/* Sample Alg 1.21 */
extern void test_read_dp_moment_params(Base_data_radial *radialPtr);

#endif


