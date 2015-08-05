/*$<
 *======================================================================= 
 * 
 *   (c) Copyright, 2012 Massachusetts Institute of Technology.
 *       This material may be reproduced by or for the 
 *       U.S. Government pursuant to the copyright license 
 *       under the clause at 252.227-7013 (Jun. 1995).
 *
 *=======================================================================
 *
 *
 *   FILE: build_model_grids.h
 *
 *   AUTHOR:  Michael F Donovan
 *
 *   CREATED:  Nov 15, 2011; Initial Version
 *
 *   DESCRIPTION:
 *
 *   This file contains module specific include file listings, function 
 *   prototypes and constant definitions for building the model derived
 *   grid data stores.
 *
 *
 *=======================================================================
 *$>
 */

#ifndef BUILD_MODEL_GRIDS_C
#define BUILD_MODEL_GRIDS_C

#include <pack_grid_data.h>
#include <create_grid_lb.h>
#include <rpgcs_latlon.h>

#define MISSING_HGT     -999999.0f
#define ZDR_TEMP1       -10.0
#define ZDR_TEMP2       -16.0

#define NUM_COMPONENTS  20
#define MDL_RANGE	0
#define MDL_AZ		1
#define MDL_ZERO_X      2
#define MDL_ZDR_TEMP1   9
#define MDL_ZDR_TEMP2   10
#define MDL_MINUS_20    11
#define MDL_MAX_TEMP_WL 12
#define MDL_MIN_TEMP_CL 15
#define MDL_SPARE1      17
#define MDL_SPARE2      18
#define MDL_SPARE3      19

/* Function Prototypes. */
void Open_lb_data_store();

void Create_FreezingHeight_Grid( int model, char *buf,
                                 RPGCS_model_attr_t *model_attrs,
                                 int site_i_index, int site_j_index );

void Create_CIP_Grid( int model, char *buf, RPGCS_model_attr_t *model_attrs,
                      int site_i_index, int site_j_index );

float T_lookup( float tmp );
float RH_lookup( float rh );
void create_cipLookup();

#endif
