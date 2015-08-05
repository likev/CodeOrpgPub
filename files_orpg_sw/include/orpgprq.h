/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/03 14:38:34 $
 * $Id: orpgprq.h,v 1.5 2012/05/03 14:38:34 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/************************************************************************

    orpgprq.h - Public header file for the product request processing 
    ORPG library module.

************************************************************************/

#ifndef ORPGPRQ_H
#define	ORPGPRQ_H

#include <rpg_port.h>

/*  The elevation parameter in a product request shall be used for
specifying either a single elevation or multiple elevations. The
paramter is defined in the following:

        bit 15 is reserved for future use (should not be set)

        bit 14 and 13 are used as follows:

        10 - all elevation cuts of the VCP are requested (NOTE: 
             if an angle is specified, then all elevations cuts
             of the VCP matching the requested angle are requested.
             The match is closest angle.  All non-positive angles
             must be specified according to rule for negative angles
             provide below.  No elevation is specified by 
             clearing bits 0 - 12.)

        01 - bits 0 - 12 specify an angle. All elevations at and 
             below the specified angle are requested.

        11 - bits 0 - 12 specify a number. The lowest number of cuts
             specified by the number are requested.

        00 - A single elevation, as specified by bits 0 - 12, is
             requested.

        When bits 0 - 12 are used for specifying an elevation angle, they
        denote (degree * 10) for positive angles or (3600 + degree * 10) for
        negative angles.
*/

#define ORPGPRQ_ALL_ELEVATIONS		0x4000
#define ORPGPRQ_LOWER_ELEVATIONS	0x2000
#define ORPGPRQ_LOWER_CUTS		0x6000
#define ORPGPRQ_SINGLE_ELEVATION	0x0
#define ORPGPRQ_ELEV_FLAG_BITS		0x6000
#define ORPGPRQ_RESERVED_BIT		0x8000


int ORPGPRQ_get_requested_elevations( int vcp, short ele_req, int elevs_size, 
                                      int vs_num, short *elevs, short *elev_inds );


#endif			/* #ifndef ORPGPRQ_H */
