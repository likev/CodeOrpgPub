#ifndef PACKET_29_H
#define PACKET_29_H
/*******************************************************************************
   Header for RPG To Class 1 ICD defined packet code 29 (generic product format)
   
*******************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/03/17 20:56:31 $
 * $Id: packet_29.h,v 1.1 2006/03/17 20:56:31 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_29          29

typedef struct
{
   short  code;			/* Packet code */
   short  align;
   short  num_bytes_msw;	/* Byte length not including self or packet code, MSW */
   short  num_bytes_lsw;	/* Byte length not including self or packet code, LSW */

} packet_29_t;

#endif /* PACKET_29_H */
