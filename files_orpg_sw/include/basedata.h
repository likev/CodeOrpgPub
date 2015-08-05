
/*********************************************************************

	Header file defining the data structures and constants used 
	for radar basedata.

*********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/08 21:02:55 $
 * $Id: basedata.h,v 1.85 2014/07/08 21:02:55 steves Exp $
 * $Revision: 1.85 $
 * $State: Exp $
 * $Log: basedata.h,v $
 * Revision 1.85  2014/07/08 21:02:55  steves
 * add surveillance cut radial times to RPG internal radial message
 *
 * Revision 1.84  2013/06/05 21:10:19  steves
 * CCR NA13-00112
 *
 * Revision 1.83  2012/10/15 21:23:58  steves
 * issue 4-062
 *
 * Revision 1.82  2012/05/03 20:37:53  steves
 * issue 3-985
 *
 * Revision 1.81  2012-05-03 09:38:34-05  steves
 * issue 3-985
 *
 * Revision 1.80  2011/03/16 15:07:23  steves
 * issue 3-858
 *
 * Revision 1.79  2011/02/24 14:30:39  steves
 * issue 3-851
 *
 * Revision 1.77  2011-02-17 15:43:54-06  steves
 * issue 3-844
 *
 * Revision 1.78  2010/12/15 14:38:53  steves
 * issue 3-821
 *
 * Revision 1.76  2009-07-28 16:08:24-05  ccalvert
 * krause part tres
 *
 * Revision 1.73  2009-07-28 08:38:42-05  steves
 * issue 3-580
 *
 * Revision 1.72  2009-02-26 14:24:58-06  steves
 * issue 3-580
 *
 * Revision 1.71  2009/02/17 14:41:17  steves
 * Issue 3-580
 *
 * Revision 1.70  2008/02/04 19:54:52  steves
 * Issue 3-411
 *
 * Revision 1.69  2007/09/18 13:47:08  cmn
 * Check in fix for ROT issue 1754 - cjh for ss
 *
 * Revision 1.68  2007/07/03 15:25:06  steves
 * issue 3-046
 *
 * Revision 1.67  2007/06/22 18:18:03  steves
 * issue 3-046
 *
 * Revision 1.66  2007-06-19 09:03:42-05  steves
 * issue 3-050
 *
 * Revision 1.65  2007/06/07 22:10:36  steves
 * issue 3-046
 *
 * Revision 1.64  2007/04/20 19:01:48  jing
 * Update
 *
 * Revision 1.63  2007/03/13 18:46:18  jing
 * Update
 *
 * Revision 1.62  2007/03/10 00:21:04  steves
 * issue 3-046
 *
 * Revision 1.61  2007-03-09 18:20:19-06  steves
 * Reverted from version 1.59
 *
 * Revision 1.59  2007-03-05 16:44:24-06  ryans
 * Add max radial macros for std and super res
 *
 * Revision 1.58  2006/09/22 15:42:39  steves
 * issue 3-046
 *
 * Revision 1.57  2006-09-22 08:36:19-05  steves
 * issue 3-046
 *
 * Revision 1.56  2006/09/15 20:25:31  steves
 * issue 3-046
 *
 * Revision 1.55  2006/09/08 13:14:44  steves
 * Support for Super Res (Issue 3-046)
 *
 * Revision 1.53  2006/07/13 22:32:46  steves
 * issue 2-960
 *
 * Revision 1.52  2006-07-13 17:06:38-05  steves
 * issue 2-960
 *
 * Revision 1.51  2006/02/27 19:32:31  steves
 * issue 2-960
 *
 * Revision 1.47  2006-02-24 11:35:02-06  steves
 * issue 2-960
 *
 * Revision 1.42  2005/03/12 00:04:15  steves
 * issue 2-622
 *
 * Revision 1.40  2005/02/28 16:49:10  steves
 * issue 2-622
 *
 * Revision 1.38  2005/01/11 21:30:17  steves
 * issue 2-622
 *
 * Revision 1.37  2004/07/14 19:43:57  ryans
 * Fix error
 *
 * Revision 1.35  2004/02/19 18:39:01  ryans
 * Need ORDA macro for clutter map
 *
 * Revision 1.34  2003/10/24 15:51:01  steves
 * issue 2-259
 *
 * Revision 1.33  2003/09/02 15:18:28  steves
 * Reverted from version 1.31
 *
 * Revision 1.31  2003/07/23 19:06:57  steves
 * issue 2-220
 *
 * Revision 1.30  2003/07/23 19:01:13  steves
 * issue 2-220
 *
 * Revision 1.29  2003/06/23 14:43:22  steves
 *  issue 2-200
 *
 * Revision 1.28  2003/06/13 14:23:29  ryans
 * Macro defs and structures need updated to support ORDA.
 *
 * Revision 1.27  2002/05/31 22:06:45  steves
 * issue 1-950
 *
 * Revision 1.26  2001/03/26 19:29:36  davep
 * Issue 1-380
 *
 * Revision 1.25  2000/06/22 21:37:19  jkrause
 * adding struct to basedata.h
 *
 * Revision 1.24  2000/05/01 19:01:26  steves
 * add new msg_type bits to RPG radial header
 *
 * Revision 1.23  2000/03/18 14:43:44  steves
 * fix
 *
 * Revision 1.22  1999/06/04 21:14:57  steves
 * NO COMMENT SUPPLIED
 *
 * Revision 1.21  1999/05/26 21:03:40  steves
 * NO COMMENT SUPPLIED
 *
 * Revision 1.20  1998/11/17 19:46:58  steves
 * NO COMMENT SUPPLIED
 *
 * Revision 1.19  1998/08/01 17:06:49  steves
 * modefy
 *
 * Revision 1.16  1998/07/31 14:19:12  steves
 * modefy
 *
 * Revision 1.15  1998/07/06 22:02:02  steves
 * modefy
 *
 * Revision 1.14  1998/02/27 21:56:06  steves
 * modefy
 *
 * Revision 1.13  1997/12/31 15:21:50  steves
 * modefy
 *
 * Revision 1.12  1997/08/22 09:48:59  eforren
 * Solaris x86 port
 *
 * Revision 1.11  97/04/02  22:38:24  22:38:24  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.10  1997/01/08 23:42:36  steves
 *
 * Revision 1.9  1997/01/03 14:34:40  steves
 *
 * Revision 1.8  1997/01/02 20:00:31  steves
 *
 * Revision 1.7  96/09/16  21:09:46  21:09:46  steves (Steve Smith)
 * 
 * Revision 1.5  1996/09/13 20:57:59  jing
 *
 * Revision 1.4  1996/09/13 19:54:42  steves
 *
 * Revision 1.3  1996/06/27 16:39:52  jing
 *
 * Revision 1.2  1996/06/25 14:38:52  dodson
 * Build9/PORT update
 *
 */

# ifndef BASEDATA_HEADER_H
# define BASEDATA_HEADER_H


#include <rda_rpg_message_header.h>


/*  The message type and rda channel are not byte swapped for
    Little Endian machines  -
    This macro prevented making changes in some source code
    It would be better to declare rda_channel and msg type
    as unsigned chars as they are in rda_rpg_message_header.h
*/    
#ifdef LITTLE_ENDIAN_MACHINE
#define BASEDATA_MSG_TYPE(a) ((a->msg_type&0xff00) >> 8)
#define BASEDATA_RDA_CHANNEL(a) (a->msg_type & 0x00ff)
#else
#define BASEDATA_MSG_TYPE(a) (a->msg_type & 0x00ff)
#define BASEDATA_RDA_CHANNEL(a) ((a->msg_type&0xff00) >> 8)
#endif

#define MAX_VSCAN    80
#define MAX_ELEVS    25
#define MAX_SECTS     3

#define MAX_RDA_MESSAGE_SIZE    2416 /* 1208 halfwords */
#define MAX_RDA_PAYLOAD_SIZE    (MAX_RDA_MESSAGE_SIZE-sizeof(RDA_RPG_message_header_t))

typedef struct nx_msg_hdr {
  short msg_len;		/* Message size in halfwords measured from 
			   	 * this halfword to end of record BDMSIZ=1 */

  short msg_type;		/*  Lower byte  (Regardless of endianess of the machine)  
			 	 *  Message type BDMTYP=2, where:
	   		 	 *  1 = digital radar data
		   		 *  2 = rda status data
		   		 *  3 = performance/maintenance data
	   			 *  4 = console message - rda to rpg
	   			 *  5 = volume coverage pattern - rda to rpg
	   		 	 *  6 = rda control commands
   		   		 *  7 = volume coverage pattern - rpg to rda
	  	 		 *  8 = clutter sensor zones
	   			 *  9 = request for data
				 * 10 = console message - rpg to rda
		   		 * 11 = loop back test - rda to rpg
		   		 * 12 = loop back test - rpg to rda
	   			 * 13 = clutter filter bypass map - rda to rpg
				 * 18 = RDA adaptation data
	   			 * 31 = digital radar data generic format rda to rpg
				 *
				 *  Higher byte: RDA channel number 0, 1 or 2 for legacy
                       	         *               8, 9, 10 for orda (regardless of machine's endianess)
	   			 */

  short seq_num; 		/* I.D. sequence = 0 to 7fff, then to 0. 
				 * BDIDSEQ=3 */

  short msg_date;		/* Modified julian date starting from * 1/1/70 
				 * BDJDAT=4 */ 

  long msg_time;		/* Generation time of messages in millisecs of 
			   	 * day past midnight (GMT) BDMILTIM=5.  
		    	 	 * This may be different than data time bellow */

  short num_msg_segs;		/* Number of message segments. BDTOTSEG=7 */

  short msg_seg_num;		/* Message segment number BDSEGNUM=8 */

  long time;			/* Collection time for this radial in millisecs 
			    	 * of day past midnight (GMT). BDZULU=9 */

  short date;           	/* Modified julian date from 1/1/70 BDCDAT=11 */

  short unamb_range;    	/* Unambiguous range (scaled: val/10 = KM) 
			   	   BDUNAMBI=12 */

  unsigned short azimuth;       /* Azimuth angle BDAZA=13
				 * (coded: (val/8)*(180/4096) = DEG).  An azimuth
				 * of "0 degs" points to true north while 
				 * "90 degs" points east.  Rotation is always 
			 	 * counterclockwise as viewed from above the 
			 	 * radar. */

  short azi_num;		/* Radial number within elevation scan (1, 2, ...)
			   	 * BDAZN=14 */

  short status;			/* Radial status where: BDRSTAT=15
			    	 * 0 = start of new elevation
			    	 * 1 = intermediate radial
			    	 * 2 = end of elevation
			    	 * 3 = beginning of volume scan
			    	 * 4 = end of volume scan */

  unsigned short elevation;     /* Elevation angle  BDEANG=16
			         * (coded: (val/8)*(180/4096) = DEG). An elevation
			         * of '0 degs' is parallel to the pedestal base
			         * while '90 degs' is perpendicular to the
			         * pedestal base. */

  short elev_num;       	/* RDA elevation number within VCP (1, 2, ...)
			   	 * scan BDENUM=17 */

  short surv_range;		/* Range to first gate of reflectivity data
			   	 * METERS BDSRANG=18 */

  short dop_range;      	/* Range to first gate of Doppler data.
			   	 * Doppler data - velocity and spectrum width
			   	 * METERS BDDRANG=19 */

  short surv_bin_size;		/* Reflectivity data gate size METERS 
			   	 * BDSINT=20 */

  short dop_bin_size;		/* Doppler data gate size METERS BDDINT=21 */ 

  short n_surv_bins;		/* Number of reflectivity gates BDNSB=22;
			   	 * n_bins' reduce when elevation increases; */

  short n_dop_bins;		/* Number of velocity and/or spectrum width
			   	 * data gates BDNDB=23; 
			         * n_bins = 0 if the field is missing; */

  short sector_num;	 	/* Sector number within cut (1 - 3) BDSPP=24; */

  float calib_const;		/* System gain calibration constant. */

  short ref_ptr;		/* Reflectivity data pointer (byte # from
				 *  start of digital radar message header.)
			 	 *  Set to 0 if reflectivity moment disabled. 
                         	 *  BDSP=27. */

  short vel_ptr;		 /* Velocity data pointer (byte # from
				  * start of digital radar message header). This
				  * pointer locates the beginning of velocity
				  * data. Set to 0 if velocity moment disabled.
				  * BDVP=28. */

  short spw_ptr;		 /* Spectrum width pointer (byte # from
			 	  * start of digital radar message header). This
				  * pointer locates the beginning of spectrum 
				  * width data. Set to 0 is spectrum width moment
        	                  * disabled. BDSWP=29 */

  short vel_resolution;		 /* Doppler velocity resolution BDDVR=30
				  * 2 = 0.5 m/s
				  * 4 = 1.0 m/s */

  short vcp_num; 	  	 /* Volume coverage pattern BDVCP=31. */

  short word_32;	 	 /* unused. */
  short word_33;	 	 /* unused. */
  short word_34;	 	 /* unused. */
  short word_35;		 /* unused */

  short ref_data_playback;	 /* Reflectivity data pointer for
                           	    Archive II playback */

  short dop_data_playback;	 /* Velocity data pointer for
                           	    Archive II playback */

  short sw_data_playback;	 /* Spectrum width data pointer for
                           	    Archive II playback */

  short nyquist_vel;		 /* Nyquist velocity BDNQVEL=39 (scaled: 
                                  * val/100 = m/s) */

  short atmos_atten;		 /* Atmospheric attenuation factor. 
			          * BDATMOS=40 (scaled: val/1000 = dB/KM) */

  short threshold_param;	 /* Overlaied threshold in .1; Range 0 - 200;
			   	  * Threshold parameter for minimum 
			   	  * difference in echo power between two 
			  	  * resolution volumes for them not to be 
			  	  * labeled range ambiguous (i.e. overlaid). 
			 	  * BDTOVER=41 */

  short spot_blank_flag;	 /* BDSPTBLNK spot blank flag: 0 - none; 
				  * 1 - for radial; 2 - for elevation; 
				  * 4 - for volume */

  short word_43;		 /* unused */
  short word_44;		 /* unused */

  float fazimuth;		/* Temporary storage for floating point 
                           	   azimuth (used to support Message 31). */

  float felevation;		/* Temporary storage for floating point 
                            	   elevation (used to support Message 31). */

  short word_49;		 /* unused */
  short word_50;		 /* unused */
  short word_51;		 /* unused */
  short word_52;		 /* unused */
  short word_53;		 /* unused */
  short word_54;		 /* unused */
  short word_55;		 /* unused */
  short word_56;		 /* unused */
  short word_57;		 /* unused */
  short word_58;   		 /* unused */

} RDA_basedata_header;


/* The following structure was added to support the new ORDA
   message formats. */
typedef struct orda_nx_msg_hdr {

  RDA_RPG_message_header_t	msg_hdr; 

  long time;			/* Collection time for this radial in millisecs 
			    	   of day past midnight (GMT). BDZULU=9 */

  short date;           	/* Modified julian date from 1/1/70 BDCDAT=11 */

  short unamb_range;    	/* Unambiguous range (scaled: val/10 = KM) 
			   	   BDUNAMBI=12 */

  unsigned short azimuth;       /* Azimuth angle BDAZA=13
			   	   (coded: (val/8)*(180/4096) = DEG).  An azimuth
			   	   of "0 degs" points to true north while 
			   	   "90 degs" points east.  Rotation is always 
			   	   counterclockwise as viewed from above the 
			   	   radar. */

  short azi_num;		/* Radial number within elevation scan (1, 2, ...)
			   	   BDAZN=14 */

  short status;			/* Radial status where: BDRSTAT=15
			    	   0 = start of new elevation
			    	   1 = intermediate radial
			    	   2 = end of elevation
			    	   3 = beginning of volume scan
			    	   4 = end of volume scan */

  unsigned short elevation;     /* Elevation angle  BDEANG=16
			   	   (coded: (val/8)*(180/4096) = DEG). An elevation
			   	   of '0 degs' is parallel to the pedestal base
			   	   while '90 degs' is perpendicular to the
			   	   pedestal base. */

  short elev_num;       	/* RDA elevation number within volume (1, 2, ...)
			   	   scan BDENUM=17 */

  short surv_range;		/* Range to first gate of reflectivity data
			   	   METERS BDSRANG=18 */

  short dop_range;      	/* Range to first gate of Doppler data.
			   	   Doppler data - velocity and spectrum width
			   	   METERS BDDRANG=19 */

  short surv_bin_size;		/* Reflectivity data gate size METERS 
			   	   BDSINT=20 */

  short dop_bin_size;		/* Doppler data gate size METERS BDDINT=21 */ 

  short n_surv_bins;		/* Number of reflectivity gates BDNSB=22;
			   	   n_bins = 0 if field is missing; */

  short n_dop_bins;		/* Number of velocity and/or spectrum width
			   	   data gates BDNDB=23; 
			   	   n_bins = 0 if the field is missing; */

  short sector_num;     	/* Sector number within cut (1 - 3) BDSPP=24;
			   	   This rotates in (1, 2, 3) as I observed; */

  float calib_const;    	/* System gain calibration constant (-99.0 to +99.0)
			   	   (dB biased). BDCC=25 */

  short ref_ptr;		/* Reflectivity data pointer (byte # from
			   	   start of digital radar message header - the 
			   	   "time" field in this structure). This
			   	   pointer locates the beginning of reflectivity
			   	   data. Set to 0 if reflectivity moment disabled. 
                           	   BDSP=27; The value used is 100. */

  short vel_ptr;		/* Velocity data pointer (byte # from
			   	   start of digital radar message header). This
			   	   pointer locates the beginning of velocity
			   	   data. Set to 0 if velocity moment disabled.
			   	   BDVP=28. */

  short spw_ptr;		/* Spectrum width pointer (byte # from
			   	   start of digital radar message header). This
			   	   pointer locates the beginning of spectrum 
			   	   width data. Set to 0 is spectrum width moment
                           	   disabled. BDSWP=29 */

  short vel_resolution;		/* Doppler velocity resolution BDDVR=30
			   	   2 = 0.5 m/s
			   	   4 = 1.0 m/s */

  short vcp_num;   		/* Volume coverage pattern BDVCP=31 */

  short word_32;		/* unused */
  short word_33;		/* unused */
  short word_34;		/* unused */
  short word_35;		/* unused */
  short word_36;		/* unused */
  short word_37;		/* unused */
  short word_38;		/* unused */

  short nyquist_vel;		/* Nyquist velocity BDNQVEL=39 
			   	   (scaled: val/100 = m/s) 
			   	   0 if Dop data is missing */

  short atmos_atten;		/* Atmospheric attenuation factor. 
			   	   range -2 - -20;
			   	   BDATMOS=40 (scaled: val/1000 = dB/KM) */

  short threshold_param;	/* Overlaied threshold in .1; Range 0 - 200;
			   	   Threshold parameter for minimum 
			   	   difference in echo power between two 
			   	   resolution volumes for them not to be 
			   	   labeled range ambiguous (i.e. overlaid). 
			   	   BDTOVER=41 */

  short spot_blank_flag;	/* BDSPTBLNK spot blank flag: 0 - non; 
			   	   1 - for radial; 2 - for elevation; 
			   	   4 - for volume */

  short word_43;		/* unused */
  short word_44;		/* unused */

  float fazimuth;		/* Temporary storage for floating point 
                           	   azimuth (used to support Message 31). */

  float felevation;		/* Temporary storage for floating point 
                            	   elevation (used to support Message 31). */

  short word_49;		/* unused */
  short word_50;		/* unused */
  short word_51;        	/* unused */
  short word_52;        	/* unused */
  short word_53;        	/* unused */
  short word_54;        	/* unused */
  short word_55;        	/* unused */
  short word_56;        	/* unused */
  short word_57;        	/* unused */
  short word_58;        	/* unused */

} ORDA_basedata_header;



#define RDA_MSG_HD_SIZE		16
				/* size of the RDA msg header */
#define RDA_DATA_HD_SIZE	100
				/* size of the RDA basedata header */

/* Header for RPG internel base data. */
typedef struct {

    unsigned short msg_len;	/* message size in ushorts. */
				/* In RPG this is assigned a value of the 
				   RDA message size and never used; We
				   use this for the size of this message */

    short msg_type;		/* bit flags: (bit 0 LSB)
				   0-th (REF_INSERT_BIT) bit: refl. inserted
				   1-st bit: flags dealiased velocity
                                   2-nd bit: reflectivity enabled
                                   3-rd bit: velocity enabled
                                   4-th bit: spectrum width enabled 
				   5-th bit: surveilance cut
				   6-th bit: combined cut (0.5 and 1.5)
				   7-th bit: batch and cuts above 1.5 
				   8-th bit: Super Resolution cut
				   9-th bit: Dual-Pol cut        
				   10-th bit: Super Resolution Recombined cut
                                   11-th bit: Preprocessed Dual-Pol cut  
                                   12-th bit: High Resolution Reflectivity
                                   13-th bit: High Attenuation Radial 
                                   14-th bit: Supplemental Scan VCP (SAILS) */


    short version;              /* Version number for radial format. */

    char radar_name[6];         /* Radar name string, 4 characters, NULL terminated. */

    int time;			/* Radial time, in millisecs since midnight. */

    int begin_vol_time;		/* Volume scan start time, in millisecs since midnight. */

    unsigned short date;	/* Radial date, modified Julian. */

    unsigned short begin_vol_date; /* Volume start date, modified Julian */

    float latitude;             /* Latitude of RDA. */

    float longitude;            /* Longitude of RDA. */

    unsigned short height;      /* Height of radar, in meters MSL. */

    unsigned short feedhorn_height; /* Height of feedhorn, in meters MSL. */  

    short weather_mode;		/* 1 (clear air) or 2 (convective) */

    short vcp_num;		/* Volume coverage pattern. */

    short volume_scan_num;	/* Volume scan number, 1 - 80. */   
                                /* Recycles to 1 after 80. */

    short vol_num_quotient;	/* Quotient for dividing volume sequence number
				   by MAX_VSCAN. */

    float azimuth;		/* Radial azimuth in degrees */

    float elevation;		/* Radial elevation in degrees */

    short azi_num;		/* Radial number within elevation sweep. */

    short elev_num;		/* RDA elevation number within VCP. */

    short rpg_elev_ind;		/* RPG elevation index corresponding to 
                                   RDA elevation number. */

    short target_elev;		/*  Target elevation, in .1 degrees.  From VCP
                                    definition. */

    short last_ele_flag;	/* = 1 if this is the last cut 
				   = 0 otherwise. */

    short start_angle;		/* Radial start azimuth, in .1 degrees. */

    short delta_angle;		/* Radial width, in .1 degrees. */

    unsigned char azm_index;	/* Azimuth Index value (deg *100). */

    unsigned char azm_reso;	/* Azimuth Resolution: 1 = 1/2 deg, 2 = 1 deg. */

    float sin_azi;		/* Sine of azimuth. */

    float cos_azi;		/* Cosine of azimuth. */

    float sin_ele;		/* Sine of the elevation. */

    float cos_ele;		/* Cosine of the elevation. */

    short status;		/* Radial status.  Psuedo end of volume
                                   and psuedo end of elevation are added. */ 

    char pbd_alg_control;	/* Used for algorithm processing control.
				   PBD_ABORT_FOR_NEW_EE
				   PBD_ABORT_FOR_NEW_EV
				   PBD_ABORT_FOR_NEW_VV 

                                   bits 0-2: processing control flag
                                   bits 3-7: processing control abort reason. */

    char pbd_aborted_volume;    /* Set in conjuction with pbd_alg_control,
                                   this is the volume scan number to abort. */

    short atmos_atten;		/* Atmospheric attenuation, in dB/km*1000. */

    short spot_blank_flag;	/* Spot blanking bit map. */

    float horiz_noise;		/* Horizontal Noise, in dBm.  (Dual Pol only) */

    float vert_noise;		/* Vertical Noise, in dBm.  (Dual Pol only) */

    float calib_const_vol;	/* Blue sky dBZ0, in dBZ. */

    float calib_const_elev;	/* Elevation-scaled blue sky dBZ0, in dBZ. */

    float calib_const;          /* Horizontal dBZ0 measured for this radial, in dBZ. */

    float v_calib_const;        /* Vertical dBZ0 measure for this radial, in dBZ. */

    float horiz_shv_tx_power;   /* Horizontal channel power, in kW. (Dual Pol only) */

    float vert_shv_tx_power;    /* Veritical channel power, in kW. (Dual Pol only) */

    float sys_diff_refl;        /* Calibration of system ZDR. (Dual Pol only) */

    float sys_diff_phase; 	/* Differential phase, in deg*182.049882. (Dual Pol
                                      only) */

    short sector_num;           /* Doppler PRF Sector Number. */

    short vel_offset;		/* Byte offset to start of velocity data. 
                                   The velocity data must be word aligned. */

    short n_dop_bins;		/* Number of Doppler bins. */

    short dop_bin_size;		/* Doppler bin size in meters. */

    short dop_range;		/* Range to start of Doppler data, 
                                   in number of bins. */

    short range_beg_dop;        /* Range to beginning of 1st Doppler bin, 
                                   in meters. */

    short dop_resolution;	/* = 1 if vel_resolution = 2 (.5 m/s); 
				   = 2 otherwise. */

    short unamb_range;		/* Unambiguous range. */

    short nyquist_vel;		/* Nyquist velocity. */

    short vel_snr_thresh;       /* SNR applied to velocity, in dB*8. */

    short vel_tover;            /* Tover applied to velocity. */

    short ref_offset;		/* Byte offset to start of reflectivity data. 
                                   The reflectivity data must be word aligned. */

    short n_surv_bins;		/* Number of surveilance bins in the msg */;

    short surv_bin_size;	/* Bin size in meters. */

    short surv_range;		/* Range to start of surveillance data,
                                   in number of bins. */

    short range_beg_surv;       /* Range to beginning of 1st surveillance bin, 
                                   in meters. */

    short sc_azi_num;           /* Split cut azimuth number of reflectivity 
                                   radial. */

    short surv_snr_thresh;      /* SNR applied to reflectivity moment, in dB*8. */

    short spw_offset;		/* Byte offset to start of spectrum width data.  
                                   The spectrum width data must be word aligned. */

    short spw_snr_thresh;       /* SNR applied to spectrum width, in dB*8. */

    short spw_tover;            /* Tover applied to spectrum width. */

    unsigned short suppl_flags; /* Supplemental flags.  See rdacnt.h for 
                                   defined flags. */

    short sig_proc_states;      /* Signal Processing States ... see RDA/RPG ICD. */

    unsigned short spare[7];	/* Spare placeholders. */

    int surv_time;		/* Used primarily for split cuts ... time of the 
                                   matching surveillance radial. */

    unsigned short surv_date;   /* Used primarily for split cuts ... date of the 
                                   matching surveillance radial. */

    short no_moments;           /* Number of offsets to additional moments
                                   that follow. */

    unsigned int offsets[20];   /* Offsets to additional moments.  Offsets are 
                                   the number of bytes from the begining of the
                                   Base_data_header structure.  The offset points
                                   to a Generic_moment_t data structure. 

                                   Note:  Offset values should be a multiple of 
                                   sizeof(word). */ 

} Base_data_header;



typedef struct {

   /* The following items vary during the course of the elevation
      scan. */
 
   short accunrng[MAX_ELEVS][MAX_SECTS];  /* Unambiguous range, in
                                             km*10. */
 
   short accunvel[MAX_ELEVS][MAX_SECTS];  /* Unambiguous velocity,
                                             in m/s*100. */
 
   float accsecsaz[MAX_ELEVS][MAX_SECTS];  /* PRF sector start
                                             azimuth, in degrees. */

   /* The following items are considered constant during the 
      elevation scan. */

   long acceltms[MAX_ELEVS];   /* Starting elevation time, in
                                  milliseconds. */

   long acceltme[MAX_ELEVS];   /* Ending elevation time, in
                                  milliseconds. */

   int  acceldts[MAX_ELEVS];   /* Starting elevation date, in
                                  modified Julian. */

   int  acceldte[MAX_ELEVS];   /* Ending elevation date, in
                                  modified Julian. */

   float accbegaz[MAX_ELEVS];  /* Beginning azimuth for this
                                  elevation, in degrees. */

   float accbegel[MAX_ELEVS];  /* Beginning elevation angle, 
                                  in degrees. */

   float accendaz[MAX_ELEVS];  /* Ending azimuth for this 
                                  elevation, in degrees. */

   float accpendaz[MAX_ELEVS]; /* Azimuth for pseudo end of 
                                  elevation, in degrees. */

   short accnumrd[MAX_ELEVS];  /* Number of radials this 
                                  elevation. */

   short accatmos[MAX_ELEVS];  /* Atmospheric attenuation, in
                                  dB/km*1000. */

   /* The following items are considered constant during the volume
      scan. */

   short accnumel;             /* Number of elevation cuts 
                                  this volume scan. */

   short accthrparm;           /* Threshold parameter TOVER. */

   short accwxmode;            /* Weather Mode. */

   short accvcpnum;            /* Volume Coverage Pattern Number. */

   short accdopres;            /* Doppler data resolution. */

   float acccalib;             /* Calibration constant. */

   short accvsdate;             /* Volume scan start date, modified
                                  Julian. */

} Radial_accounting_data;

/* bit flags for Base_data_header.msg_type */
#define REF_INSERT_BIT			1
#define VEL_DEALIASED_BIT               2
#define REF_ENABLED_BIT                 4
#define VEL_ENABLED_BIT                 8
#define WID_ENABLED_BIT                16

/* cont'd .... The following are cut types. */ 
#define REFLDATA_TYPE                  32
#define COMBBASE_TYPE                  64
#define BASEDATA_TYPE                 128

/* cont'd .... The following are radial types. */
#define SUPERRES_TYPE                 256
#define DUALPOL_TYPE                  512
#define RECOMBINED_TYPE              1024
#define PREPROCESSED_DUALPOL_TYPE    2048
#define HIGHRES_REFL_TYPE            4096

/* cont'd .... Special flags               */
#define HIGH_ATTENUATION_TYPE        8192
#define SUPPLEMENTAL_CUT_TYPE       16384


/* various masks for Base_data_header.msg_type. */
#define CUT_TYPE_MASK              0x00e0
#define RADIAL_TYPE_MASK           0x1f00
#define BASEDATA_TYPE_MASK         0x0fe0   /* Includes Cut Type and Radial Type. */
#define ALL_TYPES            	   0x0fe0

/* values for the spot_blank_flag field */
#define 	SPOT_BLANK_RADIAL	1
#define 	SPOT_BLANK_ELEVATION	2
#define 	SPOT_BLANK_VOLUME	4

/* values for the azm_reso field */
#define BASEDATA_HALF_DEGREE		1
#define BASEDATA_ONE_DEGREE		2

/* ---- ICD defined message types  ---- */
#define DIGITAL_RADAR_DATA                 1
#define RDA_STATUS_DATA                    2
#define PERFORMANCE_MAINTENANCE_DATA       3
#define CONSOLE_MESSAGE_A2G                4
#define RDA_RPG_VCP			   5
#define RDA_CONTROL_COMMANDS               6
#define RPG_RDA_VCP			   7
#define CLUTTER_SENSOR_ZONES               8
#define REQUEST_FOR_DATA                   9
#define CONSOLE_MESSAGE_G2A               10
#define LOOPBACK_TEST_RDA_RPG             11
#define LOOPBACK_TEST_RPG_RDA             12
#define CLUTTER_FILTER_BYPASS_MAP         13
#define EDITED_CLUTTER_FILTER_MAP         14
#define NOTCHWIDTH_MAP_DATA               15 /* Legacy RDA */
#define CLUTTER_MAP_DATA		  15 /* ORDA */
#define ADAPTATION_DATA			  18
#define GENERIC_DIGITAL_RADAR_DATA        31


/* ---- radial status  ---- */
/* see a309.h */
/* This can be "OR"ed in the status field to indicate a bad radial */
#define BAD_RADIAL	       0x80	


#define END_OF_TILT_FLAG       999
#define BAD_AZI_FLAG           888


/* ---- doppler velocity resolution  ---- */
#define POINT_FIVE_METERS_PER_SEC 2
#define ONE_METER_PER_SEC 4

/* ---- values for pbd_alg_control ---- */
/* The first 4 bits set the pbd_alg_control flag, the last 4 bits provide the reason. */
/* ---- processing control flag ---- */
#define PBD_ABORT_UNKNOWN       7
#define PBD_ABORT_NO		0
#define PBD_ABORT_FOR_NEW_EE	1	/* both elevation and volume based 
					   products abort and wait for new 
					   elevation scan */
#define PBD_ABORT_FOR_NEW_EV	2	/* Elevation based products abort and
					   wait for new elevation scan; Volume
					   based products abort and wait for
					   new volume scan */
#define PBD_ABORT_FOR_NEW_VV	3	/* both elevation and volume based 
					   products abort and wait for new 
					   volume scan */
#define PBD_ABORT_FLAG_MASK     0x7     /* mask for the processing control flag. */

/* ---- processing control abort reasons ---- */
#define PBD_ABORT_RADIAL_MESSAGE_EXPIRED    8   /* the input radial message expired */
#define PBD_ABORT_VCP_RESTART_COMMANDED    16   /* a VCP restart was commanded by operator */
#define PBD_ABORT_INPUT_DATA_ERROR         32   /* data consistancy check failed */
#define PBD_ABORT_UNEXPECTED_VCP_RESTART   48   /* unexpected start of volume scan */
#define PBD_ABORT_REASON_MASK             0xf8  /* mask for the processing control abort reason. */
				
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


/* ---- CONSTANTS used for data fields in the basedata structure ---- */
#define BASEDATA_HD_SIZE 	(sizeof(Base_data_header)/sizeof(short)) /* size of basedata header,
                                                                            in shorts. */
#define BASEDATA_VEL_OFF 	BASEDATA_HD_SIZE              /* offset of velocity, in shorts. */
#define BASEDATA_SPW_OFF 	(BASEDATA_VEL_OFF+1200)	      /* offset of spw, in shorts. */
#define BASEDATA_REF_OFF 	(BASEDATA_SPW_OFF+1200)	      /* offset of reflectivity, in shorts. */

/* Max number of radials per sweep */
#define BASEDATA_MAX_RADIALS	400
#define MAX_RADIALS		BASEDATA_MAX_RADIALS
#define BASEDATA_MAX_SR_RADIALS	800
#define MAX_SR_RADIALS		BASEDATA_MAX_SR_RADIALS

/* Maximum sizes of data fields. */
#define BASEDATA_REF_SIZE 	460	/* size of REF field, in shorts. */
#define BASEDATA_VEL_SIZE 	920	/* size of VEL field, in shorts. */
#define MAX_BASEDATA_REF_SIZE 	1840	/* maximum size of REF field, in shorts */
#define BASEDATA_DOP_SIZE 	1200 	/* size of VEL or SPW field, in shorts. */
#define BASEDATA_RHO_SIZE       1200	/* size of RHO field, in shorts. */
#define BASEDATA_PHI_SIZE       1200	/* size of PHI field, in shorts. */
#define BASEDATA_SNR_SIZE       1840	/* size of PHI field, in bytes. */
#define BASEDATA_ZDR_SIZE       1200	/* size of PHI field, in bytes. */
#define BASEDATA_RFR_SIZE        240	/* size of PHI field, in bytes. */

/* These sizes refer to radials constructed from RDA/RPG Message Type 1 and SPG. */
#define BASEDATA_SIZE		(BASEDATA_HD_SIZE+(2*BASEDATA_DOP_SIZE)+BASEDATA_REF_SIZE) 
                                        /* base data message size in shorts. */
#define MAX_BASEDATA_SIZE	(BASEDATA_HD_SIZE+(2*BASEDATA_DOP_SIZE)+MAX_BASEDATA_REF_SIZE)	
                                        /* maximum base data message size, in shorts. */

/* This following size refers to the maximum radial size constructed from RDA/RPG Message 
   Type 31.  Note: this value is computed from the above sizes so if any of the sizes
   change, this value will have to be manually changed. */
#define MAX_GENERIC_BASEDATA_SIZE  8460 /* maximim generic base data message size, in shorts. */

#define BASEDATA_INVALID 	256	/* value used for invalid data */
#define BASEDATA_RDBLTH 	0	/* value used for below SNR threshold */
#define BASEDATA_RDRNGF 	1	/* value used for range-folded data */

/* Used to define storage unit for moment data.  */
typedef unsigned short Moment_t;

typedef struct {

   Base_data_header hdr;
   Moment_t vel[BASEDATA_DOP_SIZE];
   Moment_t spw[BASEDATA_DOP_SIZE];
   Moment_t ref[BASEDATA_REF_SIZE];

} Base_data_radial;   


#define FIRST_MAINT_VCP 256	/* VCP numbers started here are 
				   maintainance vcp's */

/* ---- conversion constants ---- */
#define RDA_ANG_TO_DEGREE  ( 180.0 / 32768 )

/* Archive II .... */
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

typedef struct nx_id_rec {
  char filename[8];
} NEXRAD_id_rec;

#define NEX_MAX_PACKET_SIZE      (65535 * 2 + sizeof (NEXRAD_ctm_info))


#endif




