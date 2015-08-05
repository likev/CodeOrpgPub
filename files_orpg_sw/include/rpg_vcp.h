/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/11 21:53:50 $
 * $Id: rpg_vcp.h,v 1.14 2012/05/11 21:53:50 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */  
/**************************************************************************

	Header defining the data structures and constants used for
	Volume Coverage Pattern Data messages.
	
**************************************************************************/


#ifndef	VCP_MESSAGE_H
#define VCP_MESSAGE_H

/* Note:  This needs to kept consistent with the VCP definition in
          vcp.h, in particular Vcp_struct. */
#define	MAX_ELEVATION_CUTS	25

/* macros for pattern types */
#define VCP_CONST_ELEV_CUT_PT	2
#define VCP_HORIZ_RAST_SCAN_PT	4
#define VCP_VERT_RAST_SCAN_PT	8
#define VCP_SEARCHLIGHT_PT	16


/*	Volume Coverage Pattern Data messages can be one of four	*
 *	different types: Constant Elevation Cut, Horizontal Raster	*
 *	Scan, Vertical Raster Scan, and Searchlight.  The messages	*
 *	consist of a 6 byte header (VCP_message _header_t) and data	*
 *	for one of the four types.					*/
 
typedef	struct {

    short	msg_size;		/*  Size of message (in halfwords)
    					    Range 23 to 594. */

    short	pattern_type;		/*  2 = Constant Elevation Cut. */

    short	pattern_number;		/*  Pattern number from:
    
    						<= 255 - Operational/Constant
    							 Elevation Types
    						 > 255 - Maintenance/Test. */
    
} VCP_message_header_t;


/*	Header for Constant Elevation Cut Pattern type			*/


/*	Definition for the data structure VCP_elevation_cut_data_t	*/

typedef	struct {

    unsigned short	angle;		/*  Elevation angle.
    					    (coded: (val/8)*(180/4096) = deg) */

    unsigned char	phase;		/*  Phase.
    					    	0 - Random Phase
    					    	1 - Constant Phase
    					    	2 - SZ2 Phase */

    unsigned char 	waveform;	/* Waveform.
    					    	1 - Contiguous Surveillance
    					    	2 - Contiguous Doppler
    					    	    w/ ambiguity resolution
    					    	3 - Contiguous Doppler
    					    	    w/o ambiguity resolution
    					    	4 - Batch
    					    	5 - Staggered pulse pair. */

    unsigned char	super_res;	/*  1 - 1/2 degree azimuth
    					    2 - 1/4 km reflectivity
    					    4 - 300 km Doppler     
                                            8 - Dual Pol to 300 km. */

    unsigned char	surv_prf_num;	/*  Surveillance PRF number
    					    Range 1 to 8. */

    short	surv_prf_pulse;		/*  Surveillance PRF Pulse Count/
    					    Radial.
    					    Range 1 - 999. */

    short	azimuth_rate;		/*  Azimuth Rate.
    					    (coded: (val/8)*(45/4096) deg). */

    short	refl_thresh;		/*  Scaled Reflectivity Thresold
    					    dB = val/8
    					    Range -12 to 20 dB. */

    short	vel_thresh;		/*  Scaled Velocity Threshold
    					    dB = val/8
    					    Range -12 to 20 dB. */

    short	sw_thresh;		/*  Scaled Spectrum Width Threshold
    					    dB = val/8
    					    Range -12 to 20 dB. */

    short	diff_refl_thresh;	/*  SNR threshold for differential
					    reflectivity.
    					    dB = val/8
    					    Range -12 to 20 dB. */ 

    short	diff_phase_thresh;	/*  SNR threshold for differential
					    phase.
    					    dB = val/8
    					    Range -12 to 20 dB. */ 

    short	corr_coeff_thresh;	/*  SNR threshold for correlation
					    coefficient.
    					    dB = val/8
    					    Range -12 to 20 dB. */ 

    short	edge_angle1;		/*  Segment 1 Azimuth Clockwise
    					    Edge Angle
    					    (coded: (val/8)*(180/4096) dBZ)
    					    Range 0 to 359.956. */

    short	dopp_prf_num1;		/*  Segment 1 Doppler PRF Number
    					    Range 1 to 8. */

    short	dopp_prf_pulse1;	/*  Segment 1 Doppler PRF Pulse
    					    Count/Radial
    					    Range 1 to 999. */
    short	spare15;

    short	edge_angle2;		/*  Segment 2 Azimuth Clockwise
    					    Edge Angle
    					    (coded: (val/8)*(180/4096) dBZ)
    					    Range 0 to 359.956. */

    short	dopp_prf_num2;		/*  Segment 2 Doppler PRF Number
    					    Range 1 to 8. */

    short	dopp_prf_pulse2;	/*  Segment 2 Doppler PRF Pulse
    					    Count/Radial
    					    Range 1 to 999. */
    short	spare19;

    short	edge_angle3;		/*  Segment 3 Azimuth Clockwise
    					    Edge Angle
    					    (coded: (val/8)*(180/4096) dBZ)
    					    Range 0 to 359.956. */

    short	dopp_prf_num3;		/*  Segment 3 Doppler PRF Number
    					    Range 1 to 8. */

    short	dopp_prf_pulse3;	/*  Segment 3 Doppler PRF Pulse
    					    Count/Radial
    					    Range 1 to 999. */

    short	spare23;
    
} VCP_elevation_cut_data_t;

typedef	struct {

    short	number_cuts;		/*  Number of elevation cuts.
    					    Range 1 to 25. */

    short	group;			/*  Clutter Map group number.
    					    Legacy RDA Range: 1 to 99.
    					    Open RDA Range: 1 to 2. */

    unsigned char	doppler_res;	/*  Doppler Velocity Resolution.
    					    2 = 0.5 m/s
    					    4 = 1.0 m/s. */

    unsigned char	pulse_width;	/*  Pulse Width.
    					    2 = Short
    					    4 = Long. */

    short		spare7;		/* Used for sample_resolution????. */

    short		spare8;		/* Used to support VCP Translation. */

    short		spare9;

    short		spare10;

    short		spare11;

    VCP_elevation_cut_data_t	data [MAX_ELEVATION_CUTS];
    					/*  Pointer to elevation cut pattern
    					    data.  Up to 25 elevation cuts can
    					    be defined.
    					*/
    
} VCP_elevation_cut_header_t;

typedef struct {
    VCP_message_header_t       vcp_msg_hdr;    /* VCP message body header */
    VCP_elevation_cut_header_t vcp_elev_data;  /* VCP elevation cut data */
} VCP_ICD_msg_t;


#endif
