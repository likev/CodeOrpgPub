/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/21 17:03:59 $
 * $Id: nexrad.h,v 1.1 2005/06/21 17:03:59 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*************************************************************************
 *
 *      File: nexrad.h
 *
 *      Description: nexrad header file
 *
 *************************************************************************/

/*
 */

# ifndef _NEXRAD_H
# define _NEXRAD_H

/*
 * Definitions / macros / types
 */

/* ---- message types  ---- */
#define DIGITAL_RADAR_DATA               1
#define RDA_STATUS_DATA                  2
#define PERFORMANCE_MAINTENANCE_DATA     3
#define CONSOLE_MESSAGE_A2G              4
#define MAINTENANCE_LOG_DATA             5
#define RDA_CONTROL_COMMANDS             6
#define VOLUME_COVERAGE_PATTERN          7
#define CLUTTER_SENSOR_ZONES             8
#define REQUEST_FOR_DATA                 9
#define CONSOLE_MESSAGE_G2A              10
#define LOOPBACK_TEST_RDA_RPG            11
#define LOOPBACK_TEST_RGD_RPA            12
#define CLUTTER_FILTER_BYPASS_MAP        13
#define EDITED_CLUTTER_FILTER_MAP        14

/* required for receiving data through the NSSL RPG */
#define BEGIN_NEW_VOLUME				201

/* ---- radial status  ---- */
#define START_OF_NEW_ELEVATION 0
#define INTERMEDIATE_RADIAL    1
#define END_OF_ELEVATION       2
#define BEGINNING_OF_VOL_SCAN  3
#define END_OF_VOL_SCAN        4
#define END_OF_TILT_FLAG       999
#define BAD_AZI_FLAG       	   888


/* ---- doppler velocity resolution  ---- */
#define POINT_FIVE_METERS_PER_SEC 2
#define ONE_METER_PER_SEC 4

/* ---- volume coverage patterns  ---- */
#define SCAN_16_PER_5_MIN 11
#define SCAN_11_PER_5_MIN 21
#define SCAN_8_PER_10_MIN 31
#define SCAN_7_PER_10_MIN 32

#define NEX_HORIZ_BEAM_WIDTH 0.95
#define NEX_VERT_BEAM_WIDTH  0.95
#define NEX_NUM_DELTAS       5
#define NEX_NUM_UNAMB_RNGS   8
#define NEX_FIXED_ANGLE      0.005493164063
#define NEX_AZ_RATE          0.001373291016
#define NEX_NOMINAL_WAVELN   0.1053
#define NEX_PACKET_SIZE      2432
#define NEX_MAX_REC_LEN      145920
#define NEX_MAX_FIELDS       8
#define NEX_MAX_GATES        960  
#define NEX_DZ_ID            1
#define NEX_VEL_ID           2
#define NEX_SW_ID            4
#define DZ_ONLY              1
#define VE_AND_SW_ONLY       6
#define DZ_VE_SW_ALL         7

typedef struct nx_id_rec {
  char filename[8];
} NEXRAD_id_rec;

typedef struct nx_vol_number {
  unsigned short	vol_number;
} NEXRAD_vol_number;

typedef struct nx_vol_title {
  char filename[9];              /* Filename (root) - "ARCHIVE2." */
  char extension[3];             /* Filename (extension) - "1", "2", etc. */
  long julian_date;              /* Modified julian date referenced from 1/1/70 */
  long millisecs_past_midnight;  /* Time - Millisecs of day from midnight
				  * (GMT) when file was created */
  long filler1;                  /* unused */
} NEXRAD_vol_title;

typedef struct nx_ctm_info {
  short word1;  /* This stuff is to be ignored.  It is used for checking */
  short word2;  /* data integrity during the transmission process */ 
  short word3;
  short word4;
  short word5;
  short word6;
} NEXRAD_ctm_info;

typedef struct nx_msg_hdr {
  short message_len;    /* (7) Message size in halfwords measured from 
				   	 	 * this halfword to end of record */

  short message_type;  /* (8) Message type, where:
			*  1 = digital radar data
			*  2 = rda status data
			*  3 = performance/maintenance data
			*  4 = console message - rda to rpg
			*  5 = maintenance log data
			*  6 = rda control commands
			*  7 = volume coverage pattern
			*  8 = clutter sensor zones
			*  9 = request for data
			* 10 = console message - rpg to rda
			* 11 = loop back test - rda to rpg
			* 12 = loop back test - rpg to rda
			* 13 = clutter filter bypass map - rda -> rpg
			* 14 = edited clutter filter map rpg -> rda
			*/
  short seq_num;        /* (9) I.D. sequence = 0 to 7fff, then to 0. */
  short julian_date;    /* (10) Modified julian date starting from * 1/1/70 */ 
  long millisecs_past_midnight;    /* (11-12) Generation time of messages in 
				    	* millisecs of day past midnight (GMT).  
				    	* This time may be different than time listed
				    	* in halfwords 15-16 defined below */
  short num_message_segs;	/* (13) Number of message segments. */
  short message_seg_num; /* (14) Message segment number */
} NEXRAD_msg_hdr;


typedef struct nx_data_hdr {
  long millisecs_past_midnight; /* (15-16) Collection time for this radial in
				 * millisecs of day past midnight (GMT). */
  short julian_date;            /* (17) Modified julian date from 1/1/70 */
  short unamb_range_x10;        /* (18) Unambiguous range (scaled: val/10 = KM) */
  unsigned short azimuth;       /* (19) Azimuth angle
				 * (coded: (val/8)*(180/4096) = DEG).  An azimuth
				 * of "0 degs" points to true north while "90 degs"
				 * points east.  Rotation is always counterclockwise
				 * as viewed from above the radar. */
  short radial_num;             /* (20) Radial number within elevation scan */
  short radial_status;          /* (21) Radial status where:
				 * 0 = start of new elevation
				 * 1 = intermediate radial
				 * 2 = end of elevation
				 * 3 = beginning of volume scan
				 * 4 = end of volume scan */
  unsigned short elevation;     /* (22) Elevation angle
				 * (coded: (val/8)*(180/4096) = DEG). An elevation
				 * of '0 degs' is parallel to the pedestal base
				 * while '90 degs' is perpendicular to the
				 * pedestal base. */
  short elev_num;               /* (23) RDA elevation number within volume scan */
  short ref_gate1;              /* (24) Range to first gate of reflectivity data
				 * METERS */
  short vel_gate1;              /* (25) Range to first gate of Doppler data.
				 * Doppler data - velocity and spectrum width
				 * METERS */
  short ref_gate_width;         /* (26) Reflectivity data gate size METERS */
  short vel_gate_width;         /* (27) Doppler data gate size METERS */ 
  short ref_num_gates;          /* (28) Number of reflectivity gates */
  short vel_num_gates;          /* (29) Number of velocity and/or spectrum width
				 * data gates */
  short sector_num;             /* (30) Sector number within cut */
  float sys_gain_cal_const;     /* (31-32) System gain calibration constant
				 * (dB biased). */
  short ref_ptr;                /* (33) Reflectivity data pointer (byte # from
				 * start of digital radar message header). This
				 * pointer locates the beginning of reflectivity
				 * data. */
  short vel_ptr;                /* (34) Velocity data pointer (byte # from
				 * start of digital radar message header). This
				 * pointer locates the beginning of velocity
				 * data. */
  short sw_ptr;                 /* (35) Spectrum width pointer (byte # from
				 * start of digital radar message header). This
				 * pointer locates the beginning of spectrum width
				 * data. */
  short velocity_resolution;    /* (36) Doppler velocity resolution
				 * 2 = 0.5 m/s
				 * 4 = 1.0 m/s */
  short vol_coverage_pattern;   /* (37) Volume coverage pattern
				 * 11 = 16 elev scans / 5 mins.
				 * 21 = 11 elev scans / 6 mins.
				 * 31 = 8 elev scans / 10 mins.
				 * 32 = 7 elev scans / 10 mins. */
  short word_38;                /* unused */
  short word_39;                /* unused */
  short word_40;                /* unused */
  short word_41;                /* unused */
  short ref_data_playback;      /* (42) Reflectivity data pointer for Archive II
				 * playback */
  short vel_data_playback;      /* (43) Velocity data pointer for Archive II
				 * playback */
  short sw_data_playback;       /* (44) Spectrum width data pointer for Archive II
				 * playback */
  short nyquist_vel;            /* (45) Nyquist velocity (scaled: val/100 = m/s) */
  short atmos_atten_factor;     /* (46) Atmospheric attenuation factor.
				 * (scaled: val/1000 = dB/KM) */
  short threshold_param;        /* Threshold parameter for minimum difference in
				 * echo power between two resolution volumes for
				 * them not to be labeled range ambiguous (i.e.
				 * overlaid). */
  short word_48;                /* unused */
  short word_49;                /* unused */
  short word_50;                /* unused */
  short word_51;                /* unused */
  short word_52;                /* unused */
  short word_53;                /* unused */
  short word_54;                /* unused */
  short word_55;                /* unused */
  short word_56;                /* unused */
  short word_57;                /* unused */
  short word_58;                /* unused */
  short word_59;                /* unused */
  short word_60;                /* unused */
  short word_61;                /* unused */
  short word_62;                /* unused */
  short word_63;                /* unused */
  short word_64;                /* unused */
} NEXRAD_data_hdr;

typedef struct nex_hdr {
/*  Time6 time;*/
  long nazimuths;
  long ref_ngates;
  long vel_ngates;
  long ref_resolution;
  long vel_resolution;
  float elevation;
} NEXRAD_hdr;

# endif     /* _NEXRAD_H */


