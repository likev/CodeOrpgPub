/* 
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/11/20 16:22:20 $
 * $Id: rpg_clutter_censor_zones.h,v 1.10 2007/11/20 16:22:20 cmn Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

/*************************************************************************

	Header defining the data structures and constants used for
	Clutter Sensor Zones messages.
	
        This file contains structures for both the Legacy RDA message
        and the Open RDA message.
*************************************************************************/


#ifndef RPG_CLUTTER_ZONES_MESSAGE_H
#define RPG_CLUTTER_ZONES_MESSAGE_H

					/* Max number of clutter zones */
#define	MAX_NUMBER_CLUTTER_ZONES	25
#define CLCZ_MAXCZ                      25
#define	B9_MAX_NUMBER_CLUTTER_ZONES	15


#define CLCZ_ZNHW	8 /* Num halfwords per clutter zone */

 			/* Halfword locations */
#define CLCZ_SRNG	0 /* Starting range */
#define CLCZ_ERNG       1 /* Ending range */
#define CLCZ_SAZ        2 /* Starting azimuth */
#define CLCZ_EAZ        3 /* Ending azimuth */
#define CLCZ_ELSEG      4 /* Elevation segment */
#define CLCZ_OPSEL      5 /* Operator select code */
#define CLCZ_CDW        6 /* Doppler suppression level - Legacy RDA only */
#define CLCZ_CSW        7 /* Surveillance suppression level - Legacy RDA only */


typedef	struct {

    short	start_range;		/*  Range of sector boundary closest
    					    to the radar, in km.
    					    Range 2 to 510 (< stop_range)
    					*/
    short	stop_range;		/*  Range of sector boundary farthest
    					    from the radar, in km.
    					    Range 2 to 510 (> start_range)
    					*/
    short	start_azimuth;		/*  Azimuth angle defining the left
    					    side of sector, in degrees.
    					    Range 0 to 360.
    					*/
    short	stop_azimuth;		/*  Azimuth angle defining the right
    					    side of sector, in degrees.
    					    Range 0 to 360 (clockwise from
                                            start_azimuth.)
    					*/
    short	segment;		/*  Elevation segment number.  Segment
    					    1 is closest to the ground;
    					    increasing segment number denotes
    					    increasing elevation.
    					    Range 1 to 5.
					*/
    short	select_code;		/*  Operator Select Code; determines
    					    the type of filtering to perform.
    					    
    					    	0 - No filtering
    					    	1 - Bypass Map in control
    					    	2 - Clutter Filtering forced
    					    	    on all gates in sector
    					*/
    short	doppl_level;		/*  Doppler Channel suppression level.
    
    						1 - Minimum
    						2 - Medium
    						3 - Maximum
    					*/
    short	surv_level;		/*  Surveillance Channel suppression
    					    level.
    					    
    					    	1 - Minimum
    					    	2 - Medium
    					    	3 - Maximum
    					*/
} RPG_clutter_region_data_t;


typedef	struct {

    short	start_range;		/*  Range of sector boundary closest
    					    to the radar, in km.
    					    Range 0 to 511 (< stop_range)
    					*/
    short	stop_range;		/*  Range of sector boundary farthest
    					    from the radar, in km.
    					    Range 0 to 511 (> start_range)
    					*/
    short	start_azimuth;		/*  Azimuth angle defining the left
    					    side of sector, in degrees.
    					    Range 0 to 360.
    					*/
    short	stop_azimuth;		/*  Azimuth angle defining the right
    					    side of sector, in degrees.
    					    Range 0 to 360 (clockwise from
                                            start_azimuth.)
    					*/
    short	segment;		/*  Elevation segment number.  Segment
    					    1 is closest to the ground;
    					    increasing segment number denotes
    					    increasing elevation.
    					    Range 1 to 5.
					*/
    short	select_code;		/*  Operator Select Code; determines
    					    the type of filtering to perform.
    					    
    					    	0 - No filtering
    					    	1 - Bypass Map in control
    					    	2 - Clutter Filtering forced
    					    	    on all gates in sector
    					*/
} ORPG_clutter_region_data_t;


/* This structure represents Msg Type 8 in the RDA<->RPG (Lgcy RDA) ICD */
typedef	struct {

    short	regions;		/*  Number of clutter map override
    					    regions.
    					*/
    RPG_clutter_region_data_t	data [B9_MAX_NUMBER_CLUTTER_ZONES];
    					/*  Clutter sensor zone data for
    					    each region.
    					*/
} RPG_clutter_regions_t;


/* This structure represents Msg Type 8 in the ORDA<->RPG (Open RDA) ICD */
typedef	struct {

    short	regions;		/*  Number of clutter map override
    					    regions.
    					*/
    ORPG_clutter_region_data_t	data [MAX_NUMBER_CLUTTER_ZONES];
    					/*  Clutter sensor zone data for
    					    each region.
    					*/
} ORPG_clutter_regions_t;

typedef struct {

    short       regions;                /*  Number of clutter map override
                                            regions.
                                        */
    ORPG_clutter_region_data_t  data [B9_MAX_NUMBER_CLUTTER_ZONES];
                                        /*  Clutter sensor zone data for
                                            each region.
                                        */
} B9_ORPG_clutter_regions_t;




#endif
