/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/13 23:16:05 $
 * $Id: packet_17.h,v 1.2 2007/02/13 23:16:05 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PACKET_17_H
#define PACKET_17_H

#define PACKET_17       17

typedef struct{

   short  code;         	/* Packet code */
   short  spare1;       	/* Spare */
   short  spare2;		/* Spare */
   short  lfm_boxes_in_row;	/* Number of LFM boxes per row */
   short  num_rows;		/* Number of rows */

} Packet_17_hdr_t;

typedef struct{

   short  bytes_per_row;   	/* Number of "data" bytes per row */
   unsigned short data; /* data */

} Packet_17_data_t;

#endif /* PACKET_17_H */
