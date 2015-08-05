/*
 */

#ifndef DP_PRECIP_8BIT_CONST_H
#define DP_PRECIP_8BIT_CONST_H

#include <dp_Consts.h>

/* Product sizes in bytes:
 *
 * sizeof(Prod_header)     =  96
 * sizeof(Graphic_product) = 120
 * sizeof(Sym_block_t)     =  16
 * sizeof(Packet16_hdr_t)  =  14
 *
 * sizeof(Packet16_data_t) = 926 = 6 byte radial header + 920 byte bins
 * for all radials         = 926 * 360 = 333360
 *
 * 333606 = 96 + 120 + 16 + 14 + 333360.
 *
 * The DSA (172) includes a layer 2, which bumps it up to 335000 bytes */

#define SIZE_P170     335000 /* DAA */
#define SIZE_P172     335000 /* DSA */
#define SIZE_P174     335000 /* DOD */
#define SIZE_P175     335000 /* DSD */

/* Product codes are used to switch symbology block parameters */

#define CODE170          170 /* DAA */
#define CODE172          172 /* DSA */
#define CODE174          174 /* DOD */
#define CODE175          175 /* DSD */

/* The following constants are used in the packet 16 encoding.
 * See the ICD for the RPG to Class 1 User,
 * "Digital Radial Data Array Packet" for more info */

#define ICENTER            0 /* screen coords in pixels */
#define JCENTER            0 /* screen coords in pixels */
#define RANGE_SCALE_FACT 250 /* ? meters                */
#define SCALE_FACTOR      10 /* in tenths of a degree   */
#define DELTA_ANGLE       10 /* in tenths of a degree   */

/* The following constants give the number of values
 * displayed in each of the 3 layer 2 subsections */

#define NUM_ADAP          36 /* adaptable parameters */
#define NUM_SUPL          11 /* supplemental data    */
#define NUM_BIAS          13 /* bias values          */

#define DP_PRECIP_8BIT_DEBUG FALSE

/* 16 Aug 2010 Dan Stein - Added the following #define for the DSA product.
 * We'll encode the DSA using the product max value until that max exceeds
 * 2 times the maximum of the 4-bit storm-total products (STA, STP). We
 * use this color table to get that user-defined 4-bit max.
 */
#define COLOR_INDEX_171    22


#endif /* DP_PRECIP_8BIT_CONST */
