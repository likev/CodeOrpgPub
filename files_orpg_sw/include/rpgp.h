/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/05 19:19:51 $
 * $Id: rpgp.h,v 1.6 2007/02/05 19:19:51 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef RPGP_PROD_SUPPORT_H
#define RPGP_PROD_SUPPORT

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <rpc/xdr.h>
#include <orpg_product.h>

/* For optional arguments to RPGP_set_param..... first part of tuple. */
#define RPGP_ATTR_RANGE            -1
#define RPGP_ATTR_DEFAULT          -2
#define RPGP_ATTR_ACCURACY         -3
#define RPGP_ATTR_CONVERSION       -4
#define RPGP_ATTR_EXCEPTION        -5
#define RPGP_ATTR_NAME             -6
#define RPGP_ATTR_DESCRIPTION      -7
#define RPGP_ATTR_UNITS            -8

/* For type argument to RPGP_set_param. */
#define RPGP_TYPE_INT              -1
#define RPGP_TYPE_UINT             -2
#define RPGP_TYPE_SHORT            -3
#define RPGP_TYPE_USHORT           -4
#define RPGP_TYPE_BYTE             -5
#define RPGP_TYPE_UBYTE            -6
#define RPGP_TYPE_BIT              -7
#define RPGP_TYPE_FLOAT            -8
#define RPGP_TYPE_DOUBLE           -9

/* Function prototypes. */
int RPGP_build_RPGP_product_t( int prod_id, int vol_num, char *name, char *description,
                               RPGP_product_t *prod );
int RPGP_finish_RPGP_product_t( RPGP_product_t *prod, int numof_prod_params,
                                RPGP_parameter_t *prod_params, int numof_components,
                                void **components );
int RPGP_set_int_param( RPGP_parameter_t *param, char *id, char *name, int type, void *value, 
                        int size, int scale, ... );
int RPGP_set_float_param( RPGP_parameter_t *param, char *id, char *name, int type, void *value, int size, 
                          const int fld_width, const int precision, const double scale, ... );
int RPGP_set_string_param( RPGP_parameter_t *param, char *id, char *name, void *value, int size, ... );

/* These are the algorithm interface for generic product debugging routines. */
void RPGP_print_prod (RPGP_product_t *prod);
void RPGP_print_components (int n_comps, char **comps);
void RPGP_print_area (RPGP_area_t *area);
void RPGP_print_points (int n_points, RPGP_location_t *points);
void RPGP_print_params (int n_params, RPGP_parameter_t *params);
void RPGP_print_event (RPGP_event_t *event);

int RPGP_set_packet_16_radial( unsigned char *packet, short start_angle,
                               short angle_delta, unsigned char *data,
                               int num_values );
int RPGP_build_RPGP_product_t( int prod_id, int vol_num, char *name,
                               char *description, RPGP_product_t *prod );
int RPGP_finish_RPGP_product_t( RPGP_product_t *prod, int numof_prod_params,
                                RPGP_parameter_t *prod_params,
                                int numof_components, void **components );

#ifdef __cplusplus
}
#endif

#endif
