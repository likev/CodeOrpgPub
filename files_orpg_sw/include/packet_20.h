#ifndef PACKET_20_H
#define PACKET_20_H
/*******************************************************************************
   Filename: packet_20.h
   Author: Brian Klein
   Created: 11/04/2002
   
   Description
   ===========
   Header for RPG To Class 1 ICD defined packet code 20 (Point Feature)
   
   Change History
   ==============
   Brian Klein; November 04, 2002; CCR NA02-20601 Mesocyclone Rapid Update
*******************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2003/07/07 21:03:48 $
 * $Id: packet_20.h,v 1.2 2003/07/07 21:03:48 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#define PACKET_20        20
#define MESO_EXTRAP_20   1    /* Extrapolated meso (from previous vol)     */
#define CORSHR_EXTRAP_20 2    /* Extrapolated 3D shear (from previous vol) */
#define MESO_UPDATE_20   3    /* Updated or new meso                       */
#define CORSHR_UPDATE_20 4    /* Updated or new 3D shear                   */
#define MDA_STRONG_LOW   9    /* MDA with high strenth and low base        */
#define MDA_STRONG_HIGH  10   /* MDA with high strength and high base      */
#define MDA_WEAK         11   /* MDA with low strength rank                */
#define BYTES_PER_FEAT   8    /* Number of bytes needed per feature        */

typedef struct
{
   short  code;         /* Packet code                                   */
   short  num_bytes;    /* Byte length not including self or packet code */
} packet_20_hdr_t;

typedef struct
{
   short  pos_i;        /* I starting coordinate                         */
   short  pos_j;        /* J starting coordinate                         */
   short  type;         /* Point feature type (See above)                */
   short  attrib;       /* For features 1-4: radius in km/4              */
} packet_20_point_t;

typedef struct
{
   packet_20_hdr_t   hdr;
   packet_20_point_t point;
} packet_20_t;

#endif /* PACKET_20_H */
