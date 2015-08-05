/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/07/02 21:51:05 $
 * $Id: rda_rpg_clutter_map.h,v 1.9 2004/07/02 21:51:05 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  
/****************************************************************************

	Header defining the data structures and constants used for
	Clutter Filter Bypass Map messages.

	These definitions are valid for message types 13 and 14,
	MESSAGE_TYPE_CLUTTER_FILTER_RDA_TO_RPG and
	MESSAGE_TYPE_CLUTTER_FILTER_RPG_TO_RDA, respectively.
	
        There are nearly duplicate sets of structures.  One set pertains to
        the legacy RDA Clutter Filter Bypass Map containing 256 radials.
        The other set pertains to the Open RDA Clutter Filter Bypass Map
        containing 360 radials.	
****************************************************************************/


#ifndef RDA_RPG_BYPASS_MAP_MESSAGE_H
#define RDA_RPG_BYPASS_MAP_MESSAGE_H

#include "rda_rpg_message_header.h"

#define	MAX_BYPASS_MAP_SEGMENTS	        2  /* Max num elev segs */
#define	ORDA_MAX_BYPASS_MAP_SEGMENTS	5  /* Max num elev segs */
#define	BYPASS_MAP_RADIALS	      256  /* Num radials (legacy) */
#define	ORDA_BYPASS_MAP_RADIALS       360  /* Num radials (orda) */
#define	BYPASS_MAP_BINS		      512  /* Num range bins */
#define	HW_PER_RADIAL		       32  /* Num halfwords per radial */



typedef	struct {

    short	seg_num;		/*  Segment Number:  Range 1 to 5
    					    Elevation segment 1 is closest to
    					    the ground and increasing segment
    					    numbers denote increasing elevation.
    					*/
    
    short	data [BYPASS_MAP_RADIALS][HW_PER_RADIAL];
    					/*  Clutter filter bypass map data.
    					    Each elevation segment contains
                                            BYPASS_MAP_RADIALS radials (~1.4
                                            degrees) and BYPASS_MAP_BINS bins
                                            (1 kilometer resolution).
    					    Each bin is represented by a bit
    					    in HW_PER_RADIAL short integers per radial.
    					    The first bin of the first short
    					    word is the MSB of the first
    					    short.  Radials are ordered with
    					    0 degrees (359.3 to 0.7) first and
    					    358.6 degrees (357.9 to 359.3)
    					    last.  Increasing angles are taken
    					    to be clockwise relative to true
    					    north.
    					    
    					    	Unset (0) = perform clutter
    					    		    filtering
    					   	Set   (1) = bypass the clutter
    					   		    filters
    					 */

} RDA_bypass_map_segment_t;


typedef	struct {

    short	seg_num;		/*  Segment Number:  Range 1 to 5
    					    Elevation segment 1 is closest to
    					    the ground and increasing segment
    					    numbers denote increasing elevation.
    					*/
    
    short	data [ORDA_BYPASS_MAP_RADIALS][HW_PER_RADIAL];
    					/*  Clutter filter bypass map data.
    					    Each elevation segment contains
                                            ORDA_BYPASS_MAP_RADIALS radials (1
                                            degree) and BYPASS_MAP_BINS bins
                                            (1 kilometer resolution).
    					    Each bin is represented by a bit
    					    in HW_PER_RADIAL short integers per radial.
    					    The first bin of the first short
    					    word is the MSB of the first
    					    short. Increasing angles are taken
    					    to be clockwise relative to true
    					    north.
    					    
    					    	Unset (0) = perform clutter
    					    		    filtering
    					   	Set   (1) = bypass the clutter
    					   		    filters
    					 */
} ORDA_bypass_map_segment_t;


typedef	struct {

    unsigned short            date;     /*  Bypass map generation date.
                                            Value is in julian days beginning
                                            with Jan 1, 1970 at 0000 UT (=1).
                                            Range 1 to 65535.  */

    short                     time;     /*  Number of minutes past midnight
                                            UT.  Range 0 to 1440. */

    short	             num_segs;	/*  Number of Elevation Segments. */

    RDA_bypass_map_segment_t segment [MAX_BYPASS_MAP_SEGMENTS];
					/*  Clutter filter bypass map for
                                            up to MAX_BYPASS_MAP_SEGMENTS
                                            elevation segments. */
} RDA_bypass_map_t;


typedef	struct {

    unsigned short            date;     /*  Bypass map generation date.
                                            Value is in julian days beginning
                                            with Jan 1, 1970 at 0000 UT (=1).
                                            Range 1 to 65535.  */

    short                     time;     /*  Number of minutes past midnight
                                            UT.  Range 0 to 1440. */

    short	             num_segs;	/*  Number of Elevation Segments. */

    ORDA_bypass_map_segment_t segment [ORDA_MAX_BYPASS_MAP_SEGMENTS];
					/*  Clutter filter bypass map for
                                            up to ORDA_MAX_BYPASS_MAP_SEGMENTS
                                            elevation segments. */
} ORDA_bypass_map_t;


typedef struct {

    RDA_RPG_message_header_t msg_hdr;     /*  Message header. */
    
    RDA_bypass_map_t         bypass_map;  /*  Bypass Map.     */

} RDA_bypass_map_msg_t;

 
typedef struct {

    RDA_RPG_message_header_t msg_hdr;     /*  Message header. */
    
    ORDA_bypass_map_t         bypass_map;  /*  Bypass Map.     */

} ORDA_bypass_map_msg_t;

#endif
