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
Module:         sample4_t2.h

Authors:        Steve Smith, Software Engineer, NWS ROC
                    steve.smith@noaa.nssl.gov
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                Version 1.0, January 2002
 
Version 1.1   April 2005   T. Ganger
              Modified to use new product ID's 

Version 1.2   April 2006   T. Ganger
              Added a packet 1 header structure and a symbology block 
                   header structure and included product.h.
              Added two new external print diagnostic functions.  

Version 1.3   August 2007  T. Ganger
              Separated first layer header from the symbology block
                 structure for clarity 
              Added defined offsets 
              
Version 1.4   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Eliminated definition of output buffer ids.
                
$Id$
************************************************************************/

#ifndef _SAMPLE4_T2_H_
#define _SAMPLE4_T2_H_

/* ORPG   includes ----------------------------------------------------*/
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
#include <product.h>

#include <rpgc.h>
#include "rpgcs.h"



/* local defines -----------------------------------------------------*/

/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define SAMPLE4_IP1                 1998  */
/* #define SAMPLE4_IP2                 1997  */
/* #define SAMPLE4_FP1                 1993  */
/* #define SAMPLE4_FP2                 1994  */


/* estimated size for the products */
#define SAMPLE4_FP_SIZE           1000
#define SAMPLE4_IP_SIZE           1000



/* added Sample Alg 1.17 */
/* in bytes */
#define SYMB_OFFSET 120
#define SYMB_SIZE 10
#define LYR_HDR_SIZE 6
#define PKT_HDR_SIZE 8


/* symbology header block structure (header portion)  per ICD format    */
typedef struct {
  short divider;
  short block_id;
  int block_length;
  short num_layers;
/*  short divider2;  */
/*  int layer_length;*/
  } sym_block_hdr;

/* added Sample Alg 1.17 */
/* symbology data layer header                                          */
typedef struct {
  short divider2;    /* layer divider (always -1)                       */
  int layer_length;  /* length of first (upcoming) data layer in bytes  */
  } layer_hdr;
  

/* data packet 1 header structure (minus character data)                 */
typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
   short  pos_i;        /* I starting coordinate                         */
   short  pos_j;        /* J starting coordinate                         */   
   
} pkt_1_hdr;



extern int print_message_header(char* buffer);

extern int print_pdb_header(char* buffer);

extern int print_symbology_header(char *c_ptr);

extern void product_to_disk(char * output_buf, int outlen, char *pname, 
                            short ele_idx);
  
#endif

