/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/13 23:16:48 $
 * $Id: packet_16.h,v 1.2 2007/02/13 23:16:48 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PACKET_16_H
#define PACKET_16_H

#define PACKET_16       16

typedef struct{

   short  code;         /* Packet code */
   short  first_bin;    /* Index of first range bin */
   short  num_bins;	/* Number of range bins */
   short  icenter;	/* I center of sweep */
   short  jcenter;	/* J center of sweep */
   short  scale_factor;	/* Range scale factor */
   short  num_radials;	/* Number of radials */

} Packet_16_hdr_t;

typedef struct{

   short  size_bytes;   /* Number of "data" bytes in radial */
   short  start_angle;  /* Radial start angle */
   short  delta_angle;  /* Radial delta angle */
   unsigned short data; /* data */

} Packet_16_data_t;

#endif /* PACKET_16_H */
