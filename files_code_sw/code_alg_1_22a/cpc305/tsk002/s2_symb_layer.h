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
Module:         symb_layer.h

Description:    include file for the symbology block and packet layer
                construction module for 16-level RLE radial reflectivity
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.2, June 2004     T. Ganger
             Linux upgrade

Version 1.3, July 2006     T. Ganger
             Modified to use RPGC_run_length_encode rather than local 
                  function.

Version 1.4  March 2007     T. Ganger
             Included note that super res data could have 1840 bins
             
Version 1.5  August 2007   T. Ganger
             Separated first layer header from the symbology block
                 structure for clarity
             Added defined offsets
             
Version 1.6   February 2008    T. Ganger    (Sample Algorithm ver 1.18)
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique file names with respect to other algorithms.
              Modification to handle variable array sizes (used variable 
                   bin count).
              Passed calibration constant as a float.
              
Version 1.7   November 2008    T. Ganger    (Sample Algorithm ver 1.20)
              Used the data structures defined in packet_af1f.h.
              

$Id$
*************************************************************************/

#ifndef _SYMB_LAYER_H_
#define _SYMB_LAYER_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <string.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <basedata.h>
#include <rpgc.h>

#include <rpgcs_data_conversion.h>
#include <packet_af1f.h>

/* local  includes ---------------------------------------------------- */


/* constants/definitions ---------------------------------------------- */


#define SYMB_OFFSET 120
#define SYMB_SIZE 10
#define LYR_HDR_SIZE 6
#define PKT_HDR_SIZE 14



#define FALSE 0
#define TRUE  1

/* symbology block structure (header portion) per ICD format           */
/* does not include layer divider and layer length as does structure in product.h  */
typedef struct {
  short divider;     /* block divider (always -1)                       */
  short block_id;    /* block ID (always 1)                             */
  int block_length;  /* length of symbology block and packets in bytes  */
  short num_layers;  /* number of data layers in this block (1-15)      */

  } sym_block_hdr;


/* symbology data layer header                                          */
typedef struct {
  short divider2;    /* layer divider (always -1)                       */
  int layer_length;  /* length of first (upcoming) data layer in bytes  */
  } layer_hdr;

/* Sample Alg 1.20 - eliminated locally defined af1f header structure */
/* Radial Data Packet (16-level) header structure                       */


/* the following packet AF1F header structure is defined */
/* in packet_af1f.h                                      */
/* -------------------------------------------------------------------------- */
/* typedef struct{              */  
/*                              */  
/*    short code;               */  /* Packet code */
/*    short index_first_range;  */  /* Index of first range bin */
/*    short num_range_bins;     */  /* Number of range bins. */
/*    short i_center;           */  /* I center of sweep. */
/*    short j_center;           */  /* J center of sweep. */
/*    short scale_factor;       */  /* Scale Factor. */
/*    short num_radials;        */  /* Number of radials in sweep. */
/*                              */  
/* } packet_af1f_hdr_t;         */ 
/* -------------------------------------------------------------------------- */

/* the following packet AF1F data structure is defined */
/* in packet_af1f.h                                    */
/* NOTE: This structure does not actually contain the data.  It only   */
/*       represents the 6 byte radial header within packet AF1F.       */
/* -------------------------------------------------------------------------- */
/* typedef struct{               */
/*                               */
/*    short num_rle_hwords;      */  /* Number RLE halfwords in radial. */
/*    short start_angle;         */  /* Radial start angle (deg*10). */
/*    short delta_angle;         */  /* Radial delta angle (deg*10). */
/*                               */
/* } packet_af1f_radial_data_t;  */
/* -------------------------------------------------------------------------- */

 
/* function prototypes                                                  */

short build_symbology_layer(short* buffer,char **radial,int rad_count,
                                                int bin_count, int* length);

int get_max_value(short* sptr,int num_bins);

void finish_pdb(short* buffer, short elev_ind, short elevation,
               float calib_const, short max_refl, int prod_len);

extern void create_color_table(short *clr);

extern int print_symbology_header(char *c_ptr);

#endif

