/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/09 21:44:01 $
 * $Id: orda_clutter_map.h,v 1.5 2007/01/09 21:44:01 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  
/***************************************************************************

	Header defining the data structures and constants used for
	Open RDA Clutter Filter Map messages as defined in the
	Interface Control Document (ICD) for the RDA/RPG.  The ICD message
	is message type 15.
	
***************************************************************************/

#ifndef ORDA_CLUTTER_MAP_MESSAGE_H
#define	ORDA_CLUTTER_MAP_MESSAGE_H

#define	MAX_RANGE_ZONES_ORDA	25
#define	NUM_AZIMUTH_SEGS_ORDA	360
#define	MAX_ELEVATION_SEGS_ORDA	5


/*
  Include RDA/RPG message header.
*/
#include "rda_rpg_message_header.h"


/*	The ORDA clutter filter map message consists of two
 *	short integers indicating the map generation date
 *	and time, respectively, and data making up the clutter map.
 *	The data portion consists of the filter definitions
 *      for each elevation segment (up to 5) starting with the lowest segment.
 *	Each elevation segment includes 360 azimuth radials, each of which
 *      consist of up to MAX_RANGE_ZONES_ORDA range zones.  Not all range zones
 *      of a radial need to be defined, only that the last defined range zone
 *      must have an end range of 511 km.  The first azimuth radial, R0,
 *	subtends the angle (0.0 <= R0 < 1.0) degrees.  The next azimuth radial,
 *	R1, subtends the angle (1.0 <= R1 < 2.0 ) degrees.  Increasing angles
 *	are taken to be clockwise relative to true north.
 */
 
 

typedef struct {

    unsigned short       op_code;        /*  Filter op code from:

                                                0 - Bypass Filter (None)
                                                1 - Bypass Map in control
                                                2 - Force Filter (all bins)
                                        */
    unsigned short       range;          /*  Stop range per zone.
                                            Range 0 to 511.
                                        */
} ORDA_clutter_map_filter_t;


typedef	struct {

    unsigned short      num_zones;    /* Number of range zones */
    ORDA_clutter_map_filter_t filter [MAX_RANGE_ZONES_ORDA];

} ORDA_clutter_map_segment_t;


typedef	struct {

    ORDA_clutter_map_segment_t segment [NUM_AZIMUTH_SEGS_ORDA];

} ORDA_clutter_map_data_t;


typedef	struct {

    unsigned short	      date;	/*  Clutter map generation date.
    					    Value is in julian days beginning
    					    with Jan 1, 1970 at 0000 UT (=1).
    					    Range 1 to 65535.  */

    short	              time;	/*  Number of minutes past midnight 
                                            UT.  Range 0 to 1440. */

    unsigned short	      num_elevation_segs;

    ORDA_clutter_map_data_t   data [MAX_ELEVATION_SEGS_ORDA];

} ORDA_clutter_map_t;


typedef	struct {

    RDA_RPG_message_header_t msg_hdr;  /* Message header. */

    ORDA_clutter_map_t       map;      /* Clutter map data. */

} ORDA_clutter_map_msg_t;

#endif
