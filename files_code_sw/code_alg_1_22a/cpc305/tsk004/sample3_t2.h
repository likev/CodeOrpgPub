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
Module:         sample3_t2.h

Description:    include file for the sample3_t2 demonstration program. 
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.3,  April 2005   T. Ganger
              Modified to use the new product IDs

Version 1.4   March 2007   T. Ganger  
              Added warning that the standard 400 radial limit must be
                  increased to 800 radials if registering for the
                  new SR_ data types.
              Changed NUM_ELEV from 6 to 5.  This is a hard coded limit
                  of max number of elevations to process.  Without 
                  testing for last elevations (number in current VCP),
                  the limit should not exceed the smallest VCP.
                  
Version 1.5   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated definition of output buffer ids.
              New output buffer size calculated.
      
Version 1.6   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Eliminated artificial limit on number of elevations to read.
              Included new structure for intermediate product input and
                  internal data array.
      
$Id$
************************************************************************/
#ifndef _SAMPLE3_T2_H_
#define _SAMPLE3_T2_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <math.h>

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
#include "rpgcs.h"

/* The contents of Base_data_header is defined in basedata.h  */ 


/* Local Includes ----------------------------------------------------- */
/* #include "s3f_cvg_struct.h" */
#include "s3_t2_prod_struct.h"

/* Algorithm specific constants/definitions --------------------------- */

/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define SAMPLE3_FP 1992  */    /* Output Linear Buffer ID Code       */
/* #define SAMPLE3_IP 1999  */    /* Input Linear Buffer ID Code        */


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


/* Sample Alg 1.20 - eliminated artificial limit on number of elevations to read */
/*                   replaced with MAX_ELEVS defined in basedata.h               */
/* #define NUM_ELEV 5  */


/* function prototypes ------------------------------------------------ */
void clear_buffer(char*);

/* Sample Alg 1.20 - replaced CVG_radial* with unsigned char* */
int translate_product(unsigned char *ptr,char *buffer);

/* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
int assemble_product(char *buffer, int number_of_bins, s3_t2_internal_rad *rad_data);

extern int print_message_header(char* buffer);

extern int print_pdb_header(char* buffer);

/* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
extern short build_symbology_layer(char* buffer, s3_t2_internal_rad *rad_ptr, 
                                   int rad_count, int bin_count, int* length);

extern void finish_pdb(char* buffer, short elev_index, short target_elev,
                       short max_refl, int prod_len);

extern void product_to_disk(char * output_buf, int outlen, char *pname, 
                                   short ele_idx);


#endif


