/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:40:38 $
 * $Id: saausers_nullsymb_layer.h,v 1.2 2014/03/18 14:40:38 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/************************************************************************
Module:        saausers_nullsymb_layer.h

Description:   include file for the symbology block and packet layer
               construction module for 16-level radial RLE radial
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03 - Zittel
               
*******************************************************************************/

#ifndef _SAAUSERS_NULLSYMB_LAYER_H_
#define _SAAUSERS_NULLSYMB_LAYER_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <a309.h>   /* needed for intermediate and final product codes  */
#include <rpgc.h>

/* local  includes ---------------------------------------------------- */
#include <saa_arrays.h>

/* constants/definitions ---------------------------------------------- */
#define FALSE 0
#define TRUE  1
#define BPR  44             /* number of bytes per line of text         */
#define ICENTER 256         /* i center of display (ramtec values)      */
#define JCENTER 280         /* j center of display (ramtec values)      */
#define SAA_SUCCESS 0
#define SAA_FAILURE -1

/* symbology block structure (minus data layer) per ICD format          */
typedef struct {
  short divider;     /* block divider (always -1)                       */
  short block_id;    /* block ID (always 1)                             */
  int block_length;  /* length of symbology block and packets in bytes  */
  short num_layers;  /* number of data layers in this block (1-15)      */
  short divider2;    /* layer divider (always -1)                       */
  int layer_length;  /* length of first (upcoming) data layer in bytes  */
  } sym_block;

/* packet 1 text data layer structure                                   */
typedef struct {
  short pcode;            /* Packet Type 0x0001 (hex)                         */
  short num_bytes;        /* Length of data block in bytes (see ICD fig 3-8b) */
  short istart;           /* I coordinate of center of sweep                  */
  short jstart;           /* J coordinate of center of sweep                  */
  char data[BPR];
  } packet_1_layer_t;

/* Error Messages                                                        */
  char *err =               "Data not available because:                 ";
  char *no_data_msg[4] =   {"No buffer space for product                 ",
                            "Product too big for existing buffer         ",
                            "Insufficient number of hourly accumulations ",
                            "Current hour is not the requested end hour  "};

/* function prototypes                                                  */
short build_nullsymbology_layer(short *buffer,int msg_type,int* length,
                                int prod_id,int max_bufsize);
void finish_SDT_pdb(short*,short,short,int,int,int);


#endif
