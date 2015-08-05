
/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:39 $
 * $Id: packet_0802.h,v 1.2 2009/03/03 18:48:39 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PACKET_0802_H
#define PACKET_0802_H

typedef struct{

   short code;			/* Packet code */
   short indicator;  	/* Color value indicator */
   short value;		/* Color value */

} packet_0802_t;

#endif /* PACKET_0802_H */
