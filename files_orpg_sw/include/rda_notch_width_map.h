/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/02/19 19:28:27 $
 * $Id: rda_notch_width_map.h,v 1.10 2004/02/19 19:28:27 ryans Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  
/***************************************************************************

	Header defining the data structures and constants used for
	Clutter Filter Notch Width Map messages as defined in the
	Interface Control Document (ICD) for the RDA/RPG (March 1, 1996)
	
***************************************************************************/


#ifndef RDA_NOTCH_WIDTH_MAP_MESSAGE_H
#define	RDA_NOTCH_WIDTH_MAP_MESSAGE_H

#define	NUM_RANGE_ZONES_LEGACY	 16
#define	NUM_AZIMUTH_SEGS_LEGACY	256
#define	NUM_ELEVATION_SEGS_LEGACY 2


/*
  Include RDA/RPG message header.
*/
#include "rda_rpg_message_header.h"


/*	The clutter filter notch width map message consists of two
 *	short integers indicating the notch width map generation date
 *	and time, respectively, and data making up the notch width map.
 *	The data portion consists of the filter/notch width definitions
 *	for each elevation segment starting with the lowest segment.
 *	Each elevation segment includes 256 azimuth radials
 *      (~1.4063 deg), each of which consist of 16 range zones.  Not all
 *      range zones of a radial need to be defined, only that the last
 *      defined range zone must have an end range of 510.  The first
 *	azimuth radial, R0, subtends the angle (359.2969 <= R0 < 0.7031)
 *	degrees.  The next azimuth radial, R1, subtends the angle
 *	(0.7031 <= R1 < 2.1094) degrees, etc.  
 *	Increasing angles are taken to be clockwise relative to true north.
 */
 
 
typedef	struct {

    unsigned char	op_code;	/*  Filter op code from:
    
    						0 - Bypass Filter (None)
    						1 - Bypass Map in control
    						2 - Force Filter (all bins)
    					*/
    unsigned char	range;		/*  Stop range per zone.
    					    Range 0 to 510.
    					*/

} RDA_notch_map_filter_t;


typedef struct {

    unsigned char	dplr_width;	/*  Doppler channel width code.
    
    						0 - No suppression
    						1 - Low supression
    						2 - Medium suppression
    						3 - High suppression
					*/
    unsigned char	surv_width;	/*  Surveillance channel width code.
    
    						0 - No suppression
    						1 - Low suppression
    						2 - Medium suppression
    						3 - High suppression
   					*/ 
} RDA_notch_map_suppr_t;


typedef	struct {

    RDA_notch_map_filter_t	filter [NUM_AZIMUTH_SEGS_LEGACY][NUM_ELEVATION_SEGS_LEGACY];
					/*  Data structure containing filter
					    op code and stop range.
					*/
    RDA_notch_map_suppr_t	suppr [NUM_AZIMUTH_SEGS_LEGACY][NUM_ELEVATION_SEGS_LEGACY];
					/*  Data structure containing notch
					    width code for each channel.
					*/
					
} RDA_notch_map_data_t;


typedef	struct {

    unsigned short	      date;	/*  Notch width map generation date.
    					    Value is in julian days beginning
    					    with Jan 1, 1970 at 0000 UT (=1).
    					    Range 1 to 65535.  */

    short	              time;	/*  Number of minutes past midnight 
                                            UT.  Range 0 to 1440. */

    RDA_notch_map_data_t      data [NUM_RANGE_ZONES_LEGACY];
 					/*  Clutter filter notch width data. */
   
} RDA_notch_map_t;


typedef	struct {

    RDA_RPG_message_header_t msg_hdr;  /* Message header. */

    RDA_notch_map_t          notchmap; /* Notchwidth map data. */

} RDA_notch_map_msg_t;


#endif
