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
 *   FILE: pack_grid_data.h
 *
 *   AUTHOR:  Michael F Donovan
 *
 *   CREATED:  Nov 15, 2011; Initial Version
 *
 *   DESCRIPTION:
 *
 *   This file contains module specific include file listings, function 
 *   prototypes and constant definitions for the packing of external
 *   model derived grid data.
 *
 *
 *=======================================================================
 *$>
 */

#ifndef PACK_GRID_DATA_H
#define PACK_GRID_DATA_H

#include <rpgcs_model_data.h>
#include <rpgcs_time_funcs.h>
#include <rpgcs.h>
#include <rpgp.h>
#include <orpg_product.h>

#define PARAMETER_IDS    16
#define MAX_ATTR_LENGTH  512

#define FRZ_GRID 1
#define CIP_GRID 2

RPGP_ext_data_t * grid_ext_data;

/* Function Prototypes. */
int fill_rpgp_ext_data_t( int ext_grid_type, char *product_description, RPGCS_model_attr_t *model_attrs, 
                          RPGCS_model_grid_data_t *grid_data, RPGP_ext_data_t *ext_data );

int fill_grid_components( int comp_num, int ext_grid_type,
                          RPGCS_model_grid_data_t *grid_data );

int fill_grid_data( int comp_num, int ext_grid_type, RPGCS_model_grid_data_t *grid_data, RPGP_grid_t *grid );

int fill_product_params( int ext_grid_type, char *product_description, RPGCS_model_attr_t*model_attrs );

int set_int_param( RPGP_parameter_t *param, char *id, char *id_name, int value, char *units );
int set_float_param( RPGP_parameter_t *param, char *id, char *id_name, float value, char *units );
int set_string_param( RPGP_parameter_t *param, char *id, char *id_name, char *value, char *units );

void print_param_and_component_info( int site_i_index, int site_j_index );

#endif
