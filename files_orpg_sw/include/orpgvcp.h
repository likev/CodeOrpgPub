/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/15 20:08:17 $
 * $Id: orpgvcp.h,v 1.25 2012/10/15 20:08:17 steves Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */

/************************************************************************
 *	orpgvcp.h - This header file defines constants and functions	*
 *	used to access rda adaptation and current vcp data.		*
 *									*
 *	Developer: David Priegnitz - CIMMS/NSSL				*
 *	Reference: RDA/RPG ICD message type 7				*
 ************************************************************************/


#ifndef ORPGVCP_H
#define	ORPGVCP_H

/*	Include files needed.						*/

#include <time.h>
#include <infr.h>
#include <a309.h>
#include <orpgdat.h>
#include <rdacnt.h>
#include <vcp.h>

/*	The following defines the phase and channel	*/

#define ORPGVCP_LINEAR_CHANNEL			0x00
#define ORPGVCP_LOG_CHANNEL			0x80
#define ORPGVCP_RANDOM_PHASE			0x00
#define ORPGVCP_CONSTANT_PHASE			0x01
#define ORPGVCP_SZ2_PHASE			0x02

/*	The following define the channel type		*/

enum {ORPGVCP_SURVEILLANCE=0,	/* surveillance channel */
      ORPGVCP_DOPPLER1,		/* Doppler channel (sector 1) */
      ORPGVCP_DOPPLER2,		/* Doppler channel (sector 2) */
      ORPGVCP_DOPPLER3		/* Doppler channel (sector 3) */
};

/*	The following defines the moments	*/

enum {ORPGVCP_REFLECTIVITY=0,
      ORPGVCP_VELOCITY,
      ORPGVCP_SPECTRUM_WIDTH,
      ORPGVCP_DIFFERENTIAL_Z,
      ORPGVCP_CORRELATION_COEF,
      ORPGVCP_DIFFERENTIAL_PHASE
};

/*	The following define the pulse width	*/

#define	ORPGVCP_SHORT_PULSE			  2
#define	ORPGVCP_LONG_PULSE			  4

/*	The following define the Doppler velocity resolution */

#define ORPGVCP_VEL_RESOLUTION_LOW		  4
#define ORPGVCP_VEL_RESOLUTION_HIGH	 	  2

#define ORPGVCP_PULSE_COUNT_MAX			999

/*	The following defines the VCP pattern type	*/

#define ORPGVCP_PATTERN_TYPE_CONSTANT		  2
#define ORPGVCP_PATTERN_TYPE_HORIZONTAL		  4
#define ORPGVCP_PATTERN_TYPE_VERTICAL		  8
#define ORPGVCP_PATTERN_TYPE_SEARCHLIGHT	 16

/*	The following defines the along radial sample resolution */

#define	ORPGVCP_SAMPLE_RESOLUTION_LOW             0	/* 250 m */
#define	ORPGVCP_SAMPLE_RESOLUTION_HIGH            1	/*  50 m */

/*	The following defines the reflectivity range resolution */

#define	ORPGVCP_DBZ_RANGE_RESOLUTION_LOW          0	/* 1.0  km */
#define	ORPGVCP_DBZ_RANGE_RESOLUTION_HIGH         1	/* 0.25 km */

/*	The following defines the Doppler range resolution	*/

#define	ORPGVCP_DOPL_RANGE_RESOLUTION_LOW         1	/* 1.0  km */
#define	ORPGVCP_DOPL_RANGE_RESOLUTION_HIGH        0	/* 0.25 km */

/*	The following defines the azimuthal resolution		*/

#define	ORPGVCP_RADIAL_RESOLUTION_LOW             0	/* 1.0 deg */
#define	ORPGVCP_RADIAL_RESOLUTION_HIGH            1	/* 0.5 deg */

/*	The following are used for converting azimuth/elevation angles
	to BAMS (and vice versa). */
#define ORPGVCP_AZIMUTH_RATE_FACTOR	     (45.0/32768.0)
#define ORPGVCP_ELVAZM_BAMS2DEG             (180.0/32768.0)
#define ORPGVCP_ELVAZM_DEG2BAMS             (32768.0/180.0)
#define ORPGVCP_HALF_BAM                    (0.043945/2.0)
#define ORPGVCP_ELEVATION_ANGLE                   0x0001
#define ORPGVCP_AZIMUTH_ANGLE                     0x0002
#define ORPGVCP_ANGLE_FULL_PRECISION		  0x8000

/* 	The following are used for converting azimuth/elevation rate data
	to BAMS and (vice versa). */
#define ORPGVCP_RATE_BAMS2DEG              (22.5/16384.0)
#define ORPGVCP_RATE_DEG2BAMS              (16384.0/22.5)
#define ORPGVCP_RATE_HALF_BAM              (0.010986328125/2.0)

/*	The following defines where the VCP is defined		*/

#define ORPGVCP_RDA_DEFINED_VCP		  1		/* VCPs defined at the RDA. */
#define ORPGVCP_RPG_DEFINED_VCP		  2		/* VCPs defined at the RPG (by
							   configuration file located in
							   $CFG_DIR/vcp. */
#define ORPGVCP_VCP_MASK                  0x0fff        
#define ORPGVCP_SITE_SPECIFIC_RPG_VCP     0x1000        /* Site-specified RPG VCP. */ 
#define ORPGVCP_SITE_SPECIFIC_RDA_VCP     0x2000        /* Site-specified VCP. */ 

#define ORPGVCP_DATA_DEFINED_VCP	  0x4000	/* VCP defined by VCP message 
							   received at the RPG.
							   (Note: Assumes not defined by
							   config file.) */
#define ORPGVCP_EXPERIMENTAL_VCP	  0x8000	/* Experimental VCP .... */

/* For "get" functions passing in an elevation cut number, the 
   vcp_num argument can be OR'd with the following macro to 
   return the information from the VCP definition provided by
   the RDA. */
#define ORPGVCP_RDAVCP			  0x40000000
#define ORPGVCP_VOLNUM			  0x20000000

#define ORPGVCP_CLEAR_FLAGS		  (~(ORPGVCP_RDAVCP|ORPGVCP_VOLNUM))

/*  	Report the status of the last RDA adapatation data function */

int 	ORPGVCP_io_status();

/*	The following functions handle RDA adaptation data I/O.		*/

int	ORPGVCP_read ();
int	ORPGVCP_read_rdavcp ();
int	ORPGVCP_write ();

/*	The following functions are used to add/remove VCPs from the	*
 *	VCP adaptation data LB and from the other internal tables	*
 *	(i.e., wx mode, allowable PRF, volume time).			*/

int	ORPGVCP_write ();

/*	The following functions are used to add/remove VCPs from the	*
 *	VCP adaptation data LB and from the other internal tables	*
 *	(i.e., wx mode, allowable PRF, volume time).			*/

int	ORPGVCP_add    (int vcp_num, int wx_mode, int where_defined );
int	ORPGVCP_delete (int vcp_num);

/*	The following functions deal with the wx mode table.		*/

short	*ORPGVCP_wxmode_tbl_ptr ();
int	ORPGVCP_get_wxmode_vcp (int mode, int pos);
int	ORPGVCP_set_wxmode_vcp (int mode, int pos, int vcp);
int	ORPGVCP_get_where_defined_vcp (int where_defined, int pos);
int	ORPGVCP_get_where_defined (int vcp);
int	ORPGVCP_is_vcp_data_defined (int vcp);
int	ORPGVCP_set_vcp_is_data_defined (int vcp);
int	ORPGVCP_is_vcp_experimental (int vcp);
int	ORPGVCP_set_vcp_is_experimental (int vcp);
int	ORPGVCP_is_vcp_site_specific (int vcp, int where_defined);
int	ORPGVCP_set_vcp_is_site_specific (int vcp, int where_defined);
int	ORPGVCP_set_where_defined_vcp (int where_defined, int pos, int vcp);

/*	The following function definitions deal with the VCP table.	*/

short	*ORPGVCP_ptr (int pos);

int	ORPGVCP_get_vcp_num         (int pos);
int	ORPGVCP_set_vcp_num         (int pos, int vcp);
int	ORPGVCP_get_pulse_width     (int vcp_num, ...);
int	ORPGVCP_set_pulse_width     (int vcp_num, int width);
int	ORPGVCP_get_clutter_map_num (int vcp_num, ...);
int	ORPGVCP_set_clutter_map_num (int vcp_num, int width);
int	ORPGVCP_get_num_elevations  (int vcp_num, ...);
int	ORPGVCP_set_num_elevations  (int vcp_num, int width);
int	ORPGVCP_get_pattern_type    (int vcp_num, ...);
int	ORPGVCP_set_pattern_type    (int vcp_num, int pattern);
int	ORPGVCP_get_vel_resolution  (int vcp_num, ...);
int	ORPGVCP_set_vel_resolution  (int vcp_num, int res);

int	ORPGVCP_index (int vcp); /* returns table index for specified VCP */
int	ORPGVCP_get_num_vcps (); /* returns number of VCPs defined in table */

/*	The following functions deal with each cut.		*/

float	ORPGVCP_get_elevation_angle (int vcp_num, int cut, ...);
float	ORPGVCP_set_elevation_angle (int vcp_num, int cut, float angle);
int	ORPGVCP_get_waveform      (int vcp_num, int cut, ...);
int	ORPGVCP_set_waveform      (int vcp_num, int cut, int waveform);
int	ORPGVCP_get_super_res     (int vcp_num, int cut, ...);
int	ORPGVCP_set_super_res     (int vcp_num, int cut, int dual_pol);
int	ORPGVCP_get_dual_pol     (int vcp_num, int cut, ...);
int	ORPGVCP_set_dual_pol     (int vcp_num, int cut, int dual_pol);
float	ORPGVCP_get_azimuth_rate  (int vcp_num, int cut, ...);
float	ORPGVCP_set_azimuth_rate  (int vcp_num, int cut, float rate);
int	ORPGVCP_get_configuration (int vcp_num, int cut, ...);
int	ORPGVCP_set_configuration (int vcp_num, int cut, int configuration);
int	ORPGVCP_get_phase_type    (int vcp_num, int cut, ...);
int	ORPGVCP_set_phase_type    (int vcp_num, int cut, int type);
int	ORPGVCP_is_SZ2_vcp        (int vcp_num);
int	ORPGVCP_get_prf_num       (int vcp_num, int cut, int prf_type, ...);
int	ORPGVCP_set_prf_num       (int vcp_num, int cut, int prf_type, int prf_num);
int	ORPGVCP_get_pulse_count   (int vcp_num, int cut, int prf_type, ...);
int	ORPGVCP_set_pulse_count   (int vcp_num, int cut, int prf_type, int count);
float	ORPGVCP_get_edge_angle    (int vcp_num, int cut, int prf_type, ...);
float	ORPGVCP_set_edge_angle    (int vcp_num, int cut, int prf_type, float angle);
float	ORPGVCP_get_threshold     (int vcp_num, int cut, int prf_type, ...);
float	ORPGVCP_set_threshold     (int vcp_num, int cut, int prf_type, float rate);

/*	The following functions deal with the allowable PRF table.	*
 *	There is a 1 to 1 mapping to each VCP in the RDA adaptation	*
 *	data.								*/

short	*ORPGVCP_allowable_prf_ptr        (int pos);
int	ORPGVCP_get_allowable_prf_vcp_num (int pos);
int	ORPGVCP_set_allowable_prf_vcp_num (int pos, int vcp);
int	ORPGVCP_get_allowable_prfs        (int vcp_num, ...);
int	ORPGVCP_set_allowable_prfs        (int vcp_num, int num);
int	ORPGVCP_get_allowable_prf         (int vcp_num, int indx, ...);
int	ORPGVCP_set_allowable_prf         (int vcp_num, int indx, int prf_num);
int	ORPGVCP_get_allowable_prf_default (int vcp_num, int elev_num, ...);
int	ORPGVCP_set_allowable_prf_default (int vcp_num, int elev_num,
					   int prf_num);
int	ORPGVCP_get_allowable_prf_pulse_count (int vcp_num, int elev_num,
					   int indx, ...);
int	ORPGVCP_set_allowable_prf_pulse_count (int vcp_num, int elev_num,
					   int indx, int count);

/*	The following functions deal with the PRF table.		*/

float	*ORPGVCP_prf_ptr ();
float	ORPGVCP_get_prf_value (int prf);

/*	The following functions deal with the VCP times table.		*/

short	*ORPGVCP_vcp_times_ptr ();
int	ORPGVCP_get_vcp_time (int vcp_num, ...);
int	ORPGVCP_set_vcp_time (int vcp_num, int vcp_time);

/*	The following functions deal with the VCP flags table.		*/

short	*ORPGVCP_vcp_flags_ptr ();
short	ORPGVCP_get_vcp_flags (int vcp_num, ...);
short	ORPGVCP_set_vcp_flags (int vcp_num, short vcp_flags);

/*	The following functions deal with the Unambiguous Range table.	*/

int	*ORPGVCP_unambiguous_range_ptr ();
int	*ORPGVCP_unambiguous_range_table_ptr (int delta_pri);

int*	ORPGVCP_delta_pri_ptr ();
int	ORPGVCP_get_delta_pri ();

/*	The following functions deal with the RPG elevation index	*
 *	table.								*/

short	*ORPGVCP_elev_indicies_ptr (int pos);
int	ORPGVCP_get_rpg_elevation_num (int vcp_num, int cut, ...);
int	ORPGVCP_set_rpg_elevation_num (int vcp_num, int cut, int num);

/*	The following functions are tools used in VCP property		*
 *	calculations.							*/

float	ORPGVCP_compute_azimuth_rate (int surv_prf_num, int surv_pulse_count,
				      int dopl_prf_num, int dopl_pulse_count);

/*	More functions */
int 	ORPGVCP_get_all_elevation_angles (int vcp_num, 
				int buffer_size, float *elev_angles, ...);
double           ORPGVCP_ICD_angle_to_deg( int type, unsigned short angle_bams );
float            ORPGVCP_BAMS_to_deg( int type, unsigned short angle_bams );
unsigned short   ORPGVCP_deg_to_BAMS( int type, float angle_deg );
float            ORPGVCP_rate_BAMS_to_degs( unsigned short rate_bams );
unsigned short   ORPGVCP_rate_degs_to_BAMS( float rate_deg );


#endif
