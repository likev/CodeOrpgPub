/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/13 21:13:01 $
 * $Id: orpgsun.h,v 1.1 2013/06/13 21:13:01 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef ORPGSUN_H
#define ORPGSUN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <novas.h>
#include <solarsystem.h>

/* Macro definitions. */
#define ORPGSUN_DELTAT		67.4 /* This may need to change periodically. */

/* Prototypes for ORPGSUN functions. */
void ORPGSUN_NovasComputePos( site_info here, double *SunAz, double *SunEl );
void ORPGSUN_NovasComputePosAtTime( site_info here, double *SunAz, double *SunEl,
                                    time_t time );

#ifdef __cplusplus
}
#endif

#endif

