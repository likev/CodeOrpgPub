/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:38 $
 * $Id: mda1d_data_conversion.h,v 1.2 2003/07/11 19:17:38 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DATA_CONVERSION_H
#define DATA_CONVERSION_H

/* Enumerations. */
typedef enum { LOW_RES, HIGH_RES } RESO;

/* Function Prototypes */
RESO RPGCSX_get_velocity_reso( int reso );

double RPGCSX_velocity_to_ms( RESO reso, int value );

#endif
