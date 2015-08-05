
/*********************************************************************

	Header file defining the data structures and constants used 
	for radar basedata.

*********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/06 18:11:27 $
 * $Id: generic_basedata.h,v 1.13 2013/06/06 18:11:27 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 * $Log: generic_basedata.h,v $
 * Revision 1.13  2013/06/06 18:11:27  steves
 * CCR NA13-00112
 *
 * Revision 1.11  2012/10/15 21:26:19  steves
 * issue 4-062
 *
 * Revision 1.10  2007/03/21 14:37:13  steves
 * issue 3-046
 *
 * Revision 1.9  2007/02/28 23:47:11  steves
 * issue 3-183
 *
 * Revision 1.7  2007/02/07 22:52:29  ccalvert
 * conform to finalized msg 31 format
 *
 * Revision 1.6  2006/10/18 22:22:48  ryans
 * Modify to conform to latest super-res ICD
 *
 * Revision 1.5  2006/09/12 15:07:09  steves
 * issue 3-046
 *
 * Revision 1.4  2006-09-08 08:15:31-05  steves
 * Support for Super Res (Issue 3-046)
 *
 * Revision 1.2  2006/02/17 20:22:18  steves
 * issue 2-960
 *
 * Revision 1.00  2004/02/19 18:39:01  eforren
 */

#ifndef GENERIC_BASEDATA_H
#define GENERIC_BASEDATA_H

#include <basedata.h>

#define HALF_DEGREE_AZM     	1
#define ONE_DEGREE_AZM      	2

#define BYTE_MOMENT_DATA    	8
#define SHORT_MOMENT_DATA  	16

#define M31_BZIP2_COMP		1
#define M31_ZLIB_COMP		2

/* 
   The Generic Format Basedata Radial message consists of the following componments:

   Message Header
   Generic Basedata Radial Header
   Data Block Header
   Optional Data Moment Unique Parameter block.

*/

/* Bit Definitions for Signal Processing States */
#define SPS_RXRN_STATE		0x0001
#define SPS_CBT_STATE		0x0002

/* Radar information that can change every volume. */
typedef struct {

    char type[4];                       /* Data type: RVOL */

    unsigned short len;                 /* Length of this record, in bytes. */

    unsigned char major_version;        /* Major version number of the entire
                                           generic moment record. */

    unsigned char minor_version;        /* Minor version number of the entire
                                           generic moment record. */

    float lat;                          /* Radar latitude, in degrees. */

    float lon;                          /* Radar longitude, in degrees. */

    short height;                       /* Radar height, in feet MSL. */

    unsigned short feedhorn_height;     /* Height of feedhorn, in feet AGL. */

    float calib_const;                  /* System gain calibration constant, in dB. */

    float horiz_shv_tx_power;           /* Horizontal channel transmitter power, in kW. */

    float vert_shv_tx_power;            /* Veritical channel transmitter power, in kW. */

    float sys_diff_refl;                /* Calibration of system ZDR. */

    float sys_diff_phase;               /* Initial System Differential phase, in deg. */

    unsigned short vcp_num;             /* Volume coverage pattern. */

    unsigned short sig_proc_states;	/* Bit field providing states of various Signal Processing
                                           Algorithms.  See RDA/RPG ICD for definition. */

} Generic_vol_t;


/* Radar information that can change every elevation. */
typedef struct {

    char type[4];                       /* Data type:  RELV */

    unsigned short len;                 /* Length of this record, in bytes. */

    short atmos;                        /* Atmospheric attenuation factor, in dB/km*1000. */

    float calib_const;                  /* Calibration constant in dB. */

} Generic_elev_t;


/* Additional radar information that can change every radial.  */
typedef struct {

    char type[4];                       /* Data type: RRAD */

    unsigned short len;                 /* Length of this record, in bytes. */

    unsigned short unamb_range;         /* Unambiguous range, in km*10. */

    float horiz_noise;                  /* Horizontal Noise, in dBm. */

    float vert_noise;                   /* Vertical Noise, in dBm. */

    unsigned short nyquist_vel;         /* Nyquist velocity, in m/s*100. */

    short spare;			/* Needed to force 4 byte alignment. */

} Generic_rad_t;

/* Added (appended) to Radial Block header with Major Version 2 of Message 31. 
   The "len" field also indicates whether this information is present. */
#define GENERIC_RAD_DBZ0_MAJOR		2
typedef struct {

   float h_dBZ0;                      /* Horizontal dBZ0. */

   float v_dBZ0;                      /* Vertical dBZ0. */

} Generic_rad_dBZ0_t;


/* Structure definition for Generic Format Basedata Radial Moment header. */
typedef struct {

    char name[4];		 	/* Name of this moment.  Space padded on the right.
					   "DREF"  - Reflectivity 
					   "DVEL"  - Velocity
					   "DSW "  - Spectrum Width
					   "DZDR"  - Differential Reflectivity
					   "DPHI"  - Differential Phase
					   "DRHO"  - Differential Correlation
					   "DSNR"  - Signal-to-Noise Ratio */

    unsigned int info;			/* An offset to moment specific information for this
			   		   moment. */

    unsigned short no_of_gates;		/* Number of gates for this moment. */

    short first_gate_range;		/* Range to the center of the first gate, in m. */

    short bin_size;			/* Size of each gate for this moment in m. */

    unsigned short tover;		/* Threshold parameter which specifies the minimum
				   	   difference in echo power between two resolution
				   	   gates for them not to be labeled "overlayed", in dB*10. */

    short SNR_threshold;		/* SNR threshold for valid data, in dB*8. */

    unsigned char control_flag;		/* Special control features.
					   0 - None
					   1 - recombined azimuthal radials
					   2 - recombined range gates
					   3 - recombined azimuthal and range
					       gates to legacy resolution
					*/

    unsigned char data_word_size;	/* Number of bits used for each gate of data.
					   Possible_values 8 or 16. */

    float scale;			/* Scale factor used to quantize moment data.
					   0.0 means the moment data is floating point
					   and the the scale factor and offset are not
					   used.

					   To convert floating point moment data to 
					   integers of some size, the formula is:

						i = (f * scale) + offset

					   To convert integer moment data of some size to
					   floating point data, the formula is:

						f = (i - offset) / scale    
					 */


    float offset;			 /* Shift factor used to quantize moment data.
					    See the scale field above for the formulas to
					    quantize and unquantize moment data.  */

    union {				 /* Variable length array of moment data. "no_of_gates"
					    indicates the number of elements in the array
					    and "data_resolution" indicates the type and the 
					    size of the data. */

        unsigned char b[0];	 	/* Use this if data_word_size is 8 or 12. */

        unsigned short u_s[0];		/* Use this if data_word_size is 16. */

        unsigned int u_i[0];		/* Use this if data_word_size is 32 and scale 
					   is not 0.0.  */

        float f[0]; 			/* Use this if data_word_size is 32 and scale 
					   is 0.0. */

    } gate;

} Generic_moment_t;


/* Any type of record. */
typedef struct{

    char name[4];				/* Type of this record. */

} Generic_any_t;


/* Union of possible record types in the data pointer list
   of the Generic_basedata_header_t record */
typedef union gen_rec_t {

    Generic_any_t any;			/* Any type of record. */

    Generic_vol_t vol;			/* Volume based radar parameter record. Type 'RVOL' */

    Generic_elev_t elev;		/* Elevation based radar parameter record.  Type 'RELV' */

    Generic_rad_t radial;		/* Radial based radar parameter record.  Type 'RRAD'. */

    Generic_moment_t moment;		/* Generic moment record.  Type 'D'. */

} Generic_record_t;


/* Structure definition for Moment Unique parameters. */
typedef struct{

   short unique_param_size;		/* Size, in bytes, for this moments unique parameters. */
   char  unique_param;			/* Unique parameters. */

} Moment_unique_params_t;


/* Structure definition for Generic Basedata header. */
typedef struct{

    char radar_id[4]; 			/* ICAO Radar Identifier. */

    unsigned long time;			/* Collection time for this radial in
					   millisecs of day past midnight
					   (GMT). */

    unsigned short date;		/* Modified Julian date from 1/1/70. */

    short azi_num;			/* Radial number within elevation
					   scan. */

    float azimuth;			/* Azimuth angle in degree. "0 degs"
					   points true north while "90 degs"
					   points east. */

    unsigned char compress_type;	/* Type of compression used. */

    unsigned char spare_17;

    short radial_length;		/* Uncompressed length of the radial
					   in bytes including the Data Header
					   Block length. */

    unsigned char azimuth_res; 		/* Azimuth Resolution Spacing.Possible
					   values: HALF_DEGREE_AZM,
					   ONE_DEGREE_AZM. */


    unsigned char status;		/* Radial Status. */

    unsigned char elev_num;  		/* RDA elevation number within VCP. */

    unsigned char sector_num;		/* Scan sector number. */

    float elevation;			/* Elevation angle, in degrees. */

    unsigned char spot_blank_flag;	/* Spot blanking status for current
					   radial. */

    unsigned char azimuth_index;	/* Azimuth index value (defined for
					   indexed azimuths only). */

    unsigned short no_of_datum;		/* Number of data items in this
					   radial. */

    int data[0];			/* Variable length array of data
					   block offsets.  Note:  this 
					   doesn't actually reserve any 
					   storage for "data" since size i
					   is 0. */

} Generic_basedata_header_t;


/* Structure definition for Generic Basedata. */
typedef struct{

    RDA_RPG_message_header_t	msg_hdr;

    Generic_basedata_header_t   base;

} Generic_basedata_t;

#endif

