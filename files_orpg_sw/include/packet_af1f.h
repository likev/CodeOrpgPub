
/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/24 23:30:02 $
 * $Id: packet_af1f.h,v 1.1 2006/04/24 23:30:02 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef PACKET_AF1F_H
#define PACKET_AF1F_H

typedef struct{

   short code;			/* Packet code */
   short index_first_range;	/* Index of first range bin */
   short num_range_bins;	/* Number of range bins. */
   short i_center;		/* I center of sweep. */
   short j_center;		/* J center of sweep. */
   short scale_factor;		/* Scale Factor. */
   short num_radials;		/* Number of radials in sweep. */

} packet_af1f_hdr_t;

typedef struct{

   short num_rle_hwords;	/* Number RLE halfwords in radial. */
   short start_angle;		/* Radial start angle (deg*10). */
   short delta_angle;           /* Radial delta angle (deg*10). */

} packet_af1f_radial_data_t;

#endif /* PACKET_AF1F_H */
