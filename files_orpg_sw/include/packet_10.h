#ifndef PACKET_10_H
#define PACKET_10_H
/*******************************************************************************
   Filename: packet_10.h
   Author: Brian Klein
   Created: 11/04/2002
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 10 (unlinked vector,
    uniform value)
   
   Change History
   ==============
   Brian Klein; November 04, 2002; CCR NA02-20601 Mesocyclone Rapid Update
*******************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/01/16 18:06:42 $
 * $Id: packet_10.h,v 1.1 2003/01/16 18:06:42 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_10       10
#define BYTES_PER_VEC    8

typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
   short  val;          /* Color value                                   */
} packet_10_hdr_t;

typedef struct
{
   short  beg_i;        /* Beginning I position                          */
   short  beg_j;        /* Beginning J position                          */
   short  end_i;        /* Ending I position                             */
   short  end_j;        /* Ending J position                             */
} packet_10_data_t;

#endif /* PACKET_10_H */
