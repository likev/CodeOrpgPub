#ifndef PACKET_15_H
#define PACKET_15_H
/*******************************************************************************
   Filename: packet_15.h
   Author: Brian Klein
   Created: 11/04/2002
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 15 (storm ID)
   
   Change History
   ==============
   Brian Klein; November 04, 2002; CCR NA02-20601 Mesocyclone Rapid Update
*******************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/01/16 18:06:42 $
 * $Id: packet_15.h,v 1.1 2003/01/16 18:06:42 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_15       15
#define BYTES_PER_ID    6

typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
} packet_15_hdr_t;

typedef struct
{
   short  pos_i;        /* I starting coordinate                         */
   short  pos_j;        /* J starting coordinate                         */
   char   char1;        /* character 1, (alphabetic character)           */
   char   char2;        /* character 2, (numeric character)              */
} packet_15_data_t;

typedef struct
{
   packet_15_hdr_t   hdr;
   packet_15_data_t  data;
} packet_15_t;

#endif /* PACKET_15_H */
