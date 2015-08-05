/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/03/08 21:11:46 $
 * $Id: prod_deserialize.h,v 1.1 2006/03/08 21:11:46 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <orpg.h>
#include <infr.h>
#include <misc.h>
#include <orpg_product.h>

/* Locally defined macros */

/* the following are for files with prod desc blocks (outgoing prods) */
#define OPCODOFF 8  /* HW offset to packet code from start of sym blk */
#define ODLENOFF 10 /* HW offset to serialized data len from start of sym blk */
#define OSDATOFF 12 /* HW offset to serialized data len from start of sym blk */

/* the following are for files without prod desc blocks (incoming prods) */
#define IPCODOFF 6  /* HW offset to packet code starting after msg hdr */
#define IDLENOFF 8  /* HW offset to serialized data length starting after msg hdr */
#define ISDATOFF 10 /* HW offset to serialized data starting after msg hdr */

static int  Read_options(int argc, char** argv);
static int  ReadFileInfo();
static int  DeserializeProd();
static int  WriteDeserializedProd();
static void Print_usage (char **argv);
static void Print_RPGP_product (void *prodp);
static void Print_components (int n_comps, char **comps);
static void Print_area (RPGP_area_t *area);
static void Print_text (RPGP_text_t *text);
static void Print_grid (RPGP_grid_t *grid);
static void Print_table (RPGP_table_t *table);
static void Print_event (RPGP_event_t *event);
static void Print_params (int n_params, RPGP_parameter_t *params);
static void Print_points (int n_points, RPGP_location_t *points);
static void Print_xy_points (int n_points, RPGP_xy_location_t *points);
static void Print_azran_points (int n_points, RPGP_azran_location_t *points);
static int Get_data_type (char *attrs, char *buf, int buf_size);
static char *Get_token (char *text, char *buf, int buf_size);

