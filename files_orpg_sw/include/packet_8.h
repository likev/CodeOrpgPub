#ifndef PACKET_8_H
#define PACKET_8_H
/*******************************************************************************
   Filename: packet_8.h
   Author: Brian Klein
   Created: 11/04/2002
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 8 (text/spcl sym w/color)
   
   Change History
   ==============
   Brian Klein; November 04, 2002; CCR NA02-20601 Mesocyclone Rapid Update
*******************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/01/16 18:06:44 $
 * $Id: packet_8.h,v 1.1 2003/01/16 18:06:44 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_8       8

typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
} packet_8_hdr_t;

typedef struct
{
   short  color_val;    /* Color value of text/special symbol            */
   short  pos_i;        /* I starting coordinate                         */
   short  pos_j;        /* J starting coordinate                         */
} packet_8_data_t;

typedef struct
{
   packet_8_hdr_t   hdr;
   packet_8_data_t  data;
} packet_8_t;

#endif /* PACKET_8_H */
