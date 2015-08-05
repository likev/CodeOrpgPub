/*	hci_vcp_data.h - This header file defines			*
 *	functions used to access current VCP data.			*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/15 12:52:17 $
 * $Id: hci_vcp_data.h,v 1.16 2012/10/15 12:52:17 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

#ifndef HCI_VCP_DATA_DEF
#define	HCI_VCP_DATA_DEF

/*	Include files needed.						*/

#include <time.h>
#include <infr.h>
#include <orpgdat.h>
#include <itc.h>
#include <vcp.h>

#define	VCP_SNR_SCALE		  8.0
#define	VCP_AZIMUTH_SCALE	 10.0
#define	AZI_RATE_SCALE		(22.5/16384)

#define	VCP_SNR_MIN		-12.0
#define	VCP_SNR_MAX		 20.0
#define	VCP_AZIMUTH_MIN		  0.0
#define	VCP_AZIMUTH_MAX		359.9

#define	WAVEFORM_CONTIGUOUS_SURVEILLANCE	VCP_WAVEFORM_CS
#define	WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB	VCP_WAVEFORM_CD
#define	WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB	VCP_WAVEFORM_CDBATCH
#define	WAVEFORM_BATCH				VCP_WAVEFORM_BATCH
#define	WAVEFORM_STAGGERED_PULSE_PAIR		VCP_WAVEFORM_STP

#define PHASE_SZ2                               VCP_PHASE_SZ2

#define	SPEED_OF_LIGHT		300000000.0	/* meters per second */

#define	HCI_VELOCITY_RESOLUTION_LOW	ORPGVCP_VEL_RESOLUTION_LOW
#define HCI_VELOCITY_RESOLUTION_HIGH	ORPGVCP_VEL_RESOLUTION_HIGH

#define	FIRST_PRF			1
#define	LAST_PRF			8

#define	FIRST_DELTA_PRI			1
#define	LAST_DELTA_PRI			5

int	hci_read_current_vcp_data       ();
int	hci_write_current_vcp_data      ();
int	hci_current_vcp_update_flag     ();
void	hci_lock_current_vcp_data       ();
void	hci_unlock_current_vcp_data     ();
int	hci_current_vcp                 ();
int	hci_current_vcp_id              ();
int	hci_current_vcp_wxmode          ();
int	hci_current_vcp_seqnum          ();
int	hci_current_vcp_type            ();
int	hci_current_vcp_num             ();
int	hci_current_vcp_num_elevations  ();
int	hci_current_vcp_clutter_map_num ();
int	hci_current_vcp_pulse_width     ();

float	hci_current_vcp_elevation_angle  (int cut);
int	hci_current_vcp_wave_type        (int cut);
int	hci_current_vcp_phase            (int cut);
int	hci_current_vcp_dual_pol         (int cut);
int	hci_current_vcp_super_res        (int cut);
int	hci_current_vcp_surv_prf_number  (int cut);
int	hci_current_vcp_surv_pulse_count (int cut);
int	hci_current_vcp_azimuth_rate     (int cut);

int	hci_current_vcp_get_vel_resolution ();
void	hci_current_vcp_set_vel_resolution (int num);

float	hci_current_vcp_get_ref_noise_threshold (int cut);
float	hci_current_vcp_get_vel_noise_threshold (int cut);
float	hci_current_vcp_get_spw_noise_threshold (int cut);

void	hci_current_vcp_set_ref_noise_threshold (int cut, float num);
void	hci_current_vcp_set_vel_noise_threshold (int cut, float num);
void	hci_current_vcp_set_spw_noise_threshold (int cut, float num);

float	hci_current_vcp_get_sector_azimuth (int sector, int cut);
int	hci_current_vcp_get_sector_prf_num (int sector, int cut);
int	hci_current_vcp_get_sector_pulse_cnt (int sector, int cut);

void	hci_current_vcp_set_sector_azimuth   (int sector, int cut, float angle);
void	hci_current_vcp_set_sector_prf_num   (int sector, int cut, int num);
void	hci_current_vcp_set_sector_pulse_cnt (int sector, int cut, int num);

Vcp_struct	*hci_current_vcp_data_ptr ();

int	hci_get_unambiguous_range (int delta_pri, int prf_num);
int	hci_get_unambiguous_range_sprt (int delta_pri, int prf_num);
float	hci_get_prf_value (int prf_num);

#endif
