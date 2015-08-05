/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:28 $
 * $Id: recclprods_symb_layer.h,v 1.3 2002/11/26 22:07:28 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/************************************************************************
Module:        recclprods_symb_layer.h

Description:   include file for the symbology block and packet layer
               construction module for 16-level radial RLE radial
               
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_symb_layer.h,v 1.3 2002/11/26 22:07:28 nolitam Exp $
*******************************************************************************/

#ifndef _RECCLPRODS_SYMB_LAYER_H_
#define _RECCLPRODS_SYMB_LAYER_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <a309.h>   /* needed for intermediate and final product codes  */
#include <rpgc.h>

/* local  includes ---------------------------------------------------- */
#include <recclalg_arrays.h>

/* constants/definitions ---------------------------------------------- */
#define FALSE 0
#define TRUE  1
#define BPR 230             /* number of bytes per radial               */
#define ICENTER 256         /* i center of display (ramtec values)      */
#define JCENTER 280         /* j center of display (ramtec values)      */
#define REC_SUCCESS 0
#define REC_FAILURE -1


/* symbology block structure (minus data layer) per ICD format          */
typedef struct {
  short divider;     /* block divider (always -1)                       */
  short block_id;    /* block ID (always 1)                             */
  int block_length;  /* length of symbology block and packets in bytes  */
  short num_layers;  /* number of data layers in this block (1-15)      */
  short divider2;    /* layer divider (always -1)                       */
  int layer_length;  /* length of first (upcoming) data layer in bytes  */
  } sym_block;

/* Radial Data Packet (16-level) header structure                       */
typedef struct {
  short pcode;            /* Packet Type 0xAF1F (hex)                   */
  short first_range_bin;  /* Location of first range bin                */
  short num_bins;         /* number of range bins in radial             */
  short icenter;          /* I coordinate of center of sweep            */
  short jcenter;          /* J coordiante of center of sweep            */
  short scalefactor;      /* number of pixels per range bin             */
  short num_radials;      /* total number of radials */
  } packet_AF1F_hdr;

/* packet AF1F data layer structure                                     */
typedef struct {
  short num_halfwords;    /* Number of RLE halfwords per radial         */
  short start_angle;      /* starting angle of radial data              */
  short angle_delta;      /* radial angle data                          */
  unsigned char data[BPR];/* 4-bit run code and color levels            */
  } packet_AF1F_layer;


/* function prototypes                                                  */
short build_symbology_layer(short *buffer,char *inbuf,
   int count,int* length,short radial_data[][MAX_1KMBINS],int prod_id,int);
void finish_pdb(short*,short,short,int,int,int);
extern int radial_run_length_encode(int start_angle, int angle_delta, 
   short *inbuf, int start_index, int end_index, int num_data_bins, 
   int buff_step, int ctable_index, int buf_index, short *outbuf,
   int prod_id);


#endif

