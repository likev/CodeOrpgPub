/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/13 14:20:19 $
 * $Id: packet_4.h,v 1.1 2009/03/13 14:20:19 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef PACKET_4_H
#define PACKET_4_H

/*****************************************************************************
   Description
      Header for RPG To Class 1 ICD defined packet code 4 (wind barb)
   
*****************************************************************************/
#define PACKET_4       4

typedef struct packet_4_data {

   short code;		/* Packet code. */

   short num_bytes;	/* Byte length not including self or packet code */

   short color_val;	/* Color value for wind barb. */

   short pos_i;		/* I starting coordinate. */

   short pos_j;		/* J starting coordinate. */

   short wind_dir;	/* Wind direction, deg. */

   short wind_spd;	/* Wind speed, knts. */

} packet_4_data_t;


#endif /* PACKET_4_H */
