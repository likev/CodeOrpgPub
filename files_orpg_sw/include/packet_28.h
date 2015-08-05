#ifndef PACKET_28_H
#define PACKET_28_H
/*******************************************************************************
   Filename: packet_28.h
   Author: Brian Klein
   Created: 09/08/2003
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 28 (generic product format)
   
   Change History
   ==============
   Brian Klein; November 04, 2002; CCR NA02-20601 Mesocyclone Rapid Update
*******************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 18:07:05 $
 * $Id: packet_28.h,v 1.1 2004/01/07 18:07:05 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_28          28

typedef struct
{
   short  code;         /* Packet code                                   */
   short  align;
   int    num_bytes;    /* Byte length not including self or packet code */
} packet_28_t;

#endif /* PACKET_28_H */
