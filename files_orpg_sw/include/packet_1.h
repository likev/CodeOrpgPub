#ifndef PACKET_1_H
#define PACKET_1_H
/*******************************************************************************
   Filename: packet_1.h
   Author: Brian Klein
   Created: 06/17/2003
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 1 (text & special symbols)
   NOTE:  This version only handles up to 4 characters.
   
   Change History
   ==============
   Brian Klein; June  17, 2003; CCR NA95-21502 MDA
*******************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2003/07/03 20:42:06 $
 * $Id: packet_1.h,v 1.1 2003/07/03 20:42:06 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PACKET_1       1
#define BYTES_PER_ID   8

typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
} packet_1_hdr_t;

typedef struct
{
   short  pos_i;        /* I starting coordinate                         */
   short  pos_j;        /* J starting coordinate                         */
   char   char1;        /* character 1                                   */
   char   char2;        /* character 2                                   */
   char   char3;        /* character 3                                   */
   char   char4;        /* character 4                                   */
} packet_1_data_t;

typedef struct
{
   packet_1_hdr_t   hdr;
   packet_1_data_t  data;
} packet_1_t;

#endif /* PACKET_1_H */
