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
Module:         s3_symb_layer.h

Description:    include file for the symbology block and packet layer
                construction module.
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.3,  June 2004    T. Ganger
              Linux upgrade
              
Version 1.4   March 2006   T. Ganger
              Changed type of 'buffer' array
              
Version 1.5   August 2007  T. Ganger
              Separated first layer header from the symbology block
                 structure for clarity              
              Added defined offsets
              
Version 1.6   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Expanded explanation of using 230 for BPR (bytes per radial).
              Modified to use a dynamic number of bins rather than a 
                  statically declared number of bins.
                  
Version 2.0   November 2008    T. Ganger  (Sample Algorithm ver 1.20)
              Used a new structure for internal data array.
              Used the data structures defined in packet_16.h.
              
FUTURE TO DO: Use the data structures defined in packet_16.h.

$Id$
*************************************************************************/

#ifndef _S3_SYMB_LAYER_H_
#define _S3_SYMB_LAYER_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <string.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <basedata.h>
#include <rpgc.h>
#include <rpgp.h>
#include <rpgcs_data_conversion.h>
/* Sample Alg 1.20 */
#include <packet_16.h>

/* local  includes ---------------------------------------------------- */
/* #include "s3f_cvg_struct.h" */
#include "s3_t2_prod_struct.h"


/* constants/definitions ---------------------------------------------- */
/* the number of bytes per radial in the product                        */

/* in bytes */
#define SYMB_OFFSET 120
#define SYMB_SIZE 10
#define LYR_HDR_SIZE 6
#define PKT_HDR_SIZE 14

/* added Sample Alg 1.20 - currently not used */
#define THR_01_OFFSET 60
#define THR_02_OFFSET 62
#define THR_03_OFFSET 64
#define THR_04_OFFSET 66
#define THR_05_OFFSET 68


/* symbology block structure (header portion)  per ICD format          */
/* does not include layer divider and layer length as does structure in product.h  */
typedef struct {
  short divider;
  short block_id;
  int block_length;
  short num_layers;

  } sym_block_hdr;

/* symbology data layer header                                          */
typedef struct {
  short divider2;    /* layer divider (always -1)                       */
  int layer_length;  /* length of first (upcoming) data layer in bytes  */
  } layer_hdr;


/* NOTE: STANDARD STRUCTURES FOR PACKET 16 ARE NOW PROVIDED IN packet_16.h */

/* digital radial data array (packet code 16) header structure  */
/* Sample Alg 1.20 - removed locally defined packet 16 header structure */


/* The following packet 16 header structure is defined in packet_16.h */
/*  NOTE: This structure is used with RPGC_digital_radial_data_hdr()  */
/* -------------------------------------------------------------- */
/* typedef struct{         */
/*                         */
/*   short  code;          */   /* Packet code */
/*   short  first_bin;     */   /* Index of first range bin */
/*   short  num_bins;      */   /* Number of range bins */
/*   short  icenter;       */   /* I center of sweep */
/*   short  jcenter;       */   /* J center of sweep */
/*   short  scale_factor;  */   /* Range scale factor */
/*   short  num_radials;   */   /* Number of radials */
/*                         */
/* } Packet_16_hdr_t;      */
/* -------------------------------------------------------------- */


/* packet 16 data layer structure                                       */
/* Sample Alg 1.20 - removed locally defined packet 16 data structure */


/* the following packet 16 data structure is defined in packet_16.h      */
/* Note: the 'data' field is a short rather than a char.  The data array */
/*       in packet 16 is actually an array of char.  Therefore the       */
/*       the element 'data' is not used as is.  It is just a place holder*/
/* -------------------------------------------------------------- */
 /* typedef struct{         */ 
 /*                         */ 
 /*   short  size_bytes;    */   /* Number of "data" bytes in radial */
 /*   short  start_angle;   */   /* Radial start angle */
 /*   short  delta_angle;   */   /* Radial delta angle */
 /*   unsigned short data;  */   /* data */
 /*                         */
 /* } Packet_16_data_t;     */
/* -------------------------------------------------------------- */


/* function prototypes ------------------------------------------------ */
/* Sample Alg 1.20 - replaced CVG_radial with s3_t2_internal_rad */
short build_symbology_layer(char* buffer, s3_t2_internal_rad *rad_ptr, int rad_count,
                                                  int bin_count, int* length);
                                                  
void finish_pdb(char* buffer, short elev_index, short target_elev,
                       short max_refl, int prod_len);

extern int print_symbology_header(char *c_ptr);


#endif

