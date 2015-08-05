
/*
 * $revision$
 * $state$
 * $Logs$
 */

/************************************************************************
Module:         superob_build_symb_layer.h

Description:    include file for the symbology block and packet layer
                construction module.
                
*************************************************************************/

#ifndef SUPEROB_BUILD_SYMB_LAYER_H
#define SUPEROB_BUILD_SYMB_LAYER_H

/* system includes ---------------------------------------------------- */
#include <stdio.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <basedata.h>

/* local  includes ---------------------------------------------------- */
#include "superob.h"
#include "superob_path.h"


/* symbology block structure (minus data layer)                         */
typedef struct {
  short divider;
  short block_id;
  int block_length;
  short num_layers;
  short divider2;
  int layer_length;
  } sym_block;


/* function prototypes                                                  */
void finish_pdb(int*,short,int,int,int,int,int,int,int,int);


#endif

