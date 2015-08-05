
/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:40 $
 * $Id: packet_0e03.h,v 1.2 2009/03/03 18:48:40 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PACKET_0E03_H
#define PACKET_0E03_H

typedef struct{

   short code;			/* Packet code */
   short initial_pt_ind;	/* Initial point indicator */
   short i_start;		/* I start of vector */
   short j_start;		/* J start of vector */
   short length;		/* byte length */

} packet_0e03_hdr_t;

#endif /* PACKET_0E03_H */
