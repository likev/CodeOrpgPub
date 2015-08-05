/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 15:06:55 $
 * $Id: dp_precip_8bit_types.h,v 1.2 2009/03/03 15:06:55 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DP_PRECIP_8BIT_TYPES_H
#define DP_PRECIP_8BIT_TYPES_H

#include <packet_16.h> /* PACKET_16 */

/* symbology block structure (minus data layer) */

typedef struct {
  short divider;
  short block_id;
  int   block_length;
  short num_layers;
  short divider2;
  int   layer_length;
} Sym_block_t;

/* digital radial data array (packet code 16) header structure   */

typedef struct {
  short pcode;
  short first_range_bin;
  short num_bins;
  short icenter;
  short jcenter;
  short scale_factor;
  short num_radials;
} Packet16_hdr_t;

/* packet 16 data layer structure  */

typedef struct {
  short num_bytes;
  short start_angle;
  short delta_angle;
  unsigned char data[MAX_BINS];
} Packet16_data_t;

#endif /* DP_PRECIP_8BIT_TYPES_H */
