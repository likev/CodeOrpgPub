/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:11:11 $
 * $Id: rda_performance_maintenance.h,v 1.9 2002/12/11 22:11:11 nolitam Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

/***************************************************************************

	Header defining the data structures and constants used for
	RDA Performance/Maintenance Data messages.
	
***************************************************************************/


#ifndef RDA_PERFORMANCE_MESSAGE_H
#define RDA_PERFORMANCE_MESSAGE_H

#include <rda_rpg_message_header.h>

typedef	struct {

    short	dcu_status;		/*  Bit  Micro   Fiber  Direct  *
					 *  --- ------- ------- ------- *
					 *   9   Spare   Minor   Spare  *
					 *  10   Spare   Major   Spare  *
					 *  14   Spare   Remote  Spare  *
					 *  15   Spare   Spare   Spare  *
					 *				*
					 *  Bit set indicates alarm.	*/

    short	general_error;		/*  Hex code 0 < X < 1F		*/

    short	svc_15_error;		/*  Hex code 0 < X < 27		*/

    unsigned short outgoing_frames;	/*  Integer 0 < X < 2**16 -1	*/

    unsigned short frames_fcs_errors;	/*  Integer 0 < X < 2**16 -1	*/

    unsigned short retrans_i_frames;	/*  Integer 0 < X < 2**16 -1	*/

    unsigned short polls_sent_recvd;	/*  Integer 0 < X < 2**16 -1	*/

    short	poll_timeout_exp;	/*  Integer 0 < X < 10		*/

    unsigned short min_buf_read_pool;	/*  Integer 0 < X < 2**16 -1	*/

    unsigned short max_buf_read_done;	/*  Integer 0 < X < 2**16 -1	*/

    short	loopback_test;		/*  0 = pass;     1 = fail	*
					 *  2 = timeout;  3 =Not Tested	*/
    short	spare12;
    short	spare13;
    short	spare14;
    short	spare15;

} wideband_t;

typedef struct {

    float	agc_step1_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step2_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step3_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step4_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step5_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step6_amp;		/*  0.0 <= X <= 99.9 dB		*/
    float	agc_step1_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_step2_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_step3_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_step4_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_step5_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_step6_phase;	/*  -180 <= X <= 180 Deg	*/
    float	agc_iq_amp_bal;		/*  0.0 <= X <= 0.000		*/
    float	agc_iq_phase_bal;	/*  0.0 <= X <= 360.0		*/
    short	spare181;
    short	spare182;
    short	spare183;
    short	spare184;
    short	spare185;
    short	spare186;
    short	spare187;
    short	spare188;
    short	spare189;
    short	spare190;
    short	spare191;
    short	spare192;
    short	spare193;
    short	spare194;
    short	spare195;
    short	spare196;
    short	spare197;
    short	spare198;
    short	spare199;
    short	spare200;
    float	cw_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd1_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd2_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd3_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	cw_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd1_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd2_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd3_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	cw_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd1_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd2_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd3_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	cw_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd1_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd2_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	rfd3_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	short_pulse_lin_syscal;	/*  -99.9 <= X <= 99.9 dB	*/
    float	short_pulse_log_syscal;	/*  -99.9 <= X <= 99.9 dB	*/
    float	long_pulse_lin_syscal;	/*  -99.9 <= X <= 99.9 dB	*/
    float	long_pulse_log_syscal;	/*  -99.9 <= X <= 99.9 dB	*/
    float	phase_ram1_exp_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram2_exp_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram3_exp_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram4_exp_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram1_mea_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram2_mea_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram3_mea_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram4_mea_vel;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram1_exp_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram2_exp_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram3_exp_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram4_exp_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram1_mea_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram2_mea_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram3_mea_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    float	phase_ram4_mea_wid;	/*  -99.9 <= X <= 99.9 m/s	*/
    short	spare273;
    short	spare274;
    short	spare275;
    short	spare276;
    short	spare277;
    short	spare278;
    short	spare279;
    short	spare280;
    short	spare281;
    short	spare282;
    short	spare283;
    short	spare284;
    short	spare285;
    short	spare286;
    short	spare287;
    short	spare288;
    short	spare289;
    short	spare290;
    float	kd1_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd2_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd3_lin_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd1_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd2_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd3_log_tgt_exp_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd1_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd2_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd3_lin_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd1_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd2_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    float	kd3_log_tgt_mea_amp;	/*  -99.9 <= X <= 99.9 dBZ	*/
    short	spare315;
    short	spare316;
    short	spare317;
    short	spare318;
    short	spare319;
    short	spare320;
    short	spare321;
    short	spare322;
    short	spare323;
    short	spare324;
    short	spare325;
    short	spare326;
    short	spare327;
    short	spare328;
    short	spare329;
    short	spare330;

} calibration_t;

typedef struct {

    float	unfilt_lin_chan_pwr;	/*  -99.9 <= X <= 99.9 dB	*/
    float	filt_lin_chan_pwr;	/*  -99.9 <= X <= 99.9 dB	*/
    float	unfilt_log_chan_pwr;	/*  -99.9 <= X <= 99.9 dB	*/
    float	filt_log_chan_pwr;	/*  -99.9 <= X <= 99.9 dB	*/
    short	spare339;
    short	spare340;
    short	spare341;
    short	spare342;
    short	spare343;
    short	spare344;
    short	spare345;
    short	spare346;
    short	spare347;
    short	spare348;
    short	spare349;
    short	spare350;
    short	spare351;
    short	spare352;
    short	spare353;
    short	spare354;
    short	spare355;
    short	spare356;
    short	spare357;
    short	spare358;
    short	spare359;
    short	spare360;

} clutter_check_t;

typedef	struct {

    short	state_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	state_fil_wr_stat;	/*  1 = OK; 2 = ERROR		*/
    short	by_map_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	by_map_fil_wr_stat;	/*  1 = OK; 2 = ERROR		*/
    short	rdasc_cal_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	rdasc_cal_fil_wr_stat;	/*  1 = OK; 2 = ERROR		*/
    short	rdasot_cal_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	mod_adap_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	spare369;		/*  1 = SPARE			*/
    short	cen_zone_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	cen_zone_fil_wr_stat;	/*  1 = OK; 2 = ERROR		*/
    short	rem_vcp_fil_wr_stat;	/*  1 = OK; 2 = ERROR		*/
    short	rem_vcp_fil_rd_stat;	/*  1 = OK; 2 = ERROR		*/
    short	spare374;
    short	spare375;
    short	spare376;
    short	spare377;
    short	spare378;
    short	spare379;
    short	spare380;
    short	spare381;
    short	spare382;
    short	spare383;
    short	spare384;
    short	spare385;
    short	spare386;
    short	spare387;
    short	spare388;
    short	spare389;
    short	spare390;
    short	spare391;
    short	spare392;
    short	spare393;
    short	spare394;
    short	spare395;
    short	spare396;
    short	spare397;
    short	spare398;
    short	spare399;
    short	spare400;

} disk_file_status_t;

typedef	struct {

					/*  NO = Not Cofigured or not	*
					 *  initialized.		*/
    short	dau_init_stat;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	maint_con_init_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	ped_init_stat;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	self_tst1_stat;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	self_tst2_stat;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	self_tst2_data;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	sps_init_stat;		/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	sps_download_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	sps_dim_loop_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	sps_smi_loop_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	sps_hsp_loop_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	rpg_link_init_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	user_link_init_stat;	/*  1 = NO; 2 = OK; 3 = FAIL	*/
    short	spare414;
    short	spare415;
    short	spare416;
    short	spare417;
    short	spare418;
    short	spare419;
    short	spare420;
    short	spare421;
    short	spare422;
    short	spare423;
    short	spare424;
    short	spare425;
    short	spare426;
    short	spare427;
    short	spare428;
    short	spare429;
    short	spare430;

} device_init_t;

typedef	struct {

    int		dau_io_err_stat;	/*  SVCI Error Code		*/
    int		dau_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	dau_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		mc_io_err_stat;		/*  SVCI Error Code		*/
    int		mc_io_err_date;		/*  Julian Date from 1/1/1970	*/
    char	mc_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		ped_io_err_stat;	/*  SVCI Error Code		*/
    int		ped_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	ped_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		sps_io_err_stat;	/*  SVCI Error Code		*/
    int		sps_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	sps_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		arch2_io_err_stat;	/*  SVCI Error Code		*/
    int		arch2_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	arch2_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		disk_io_err_stat;	/*  SVCI Error Code		*/
    int		disk_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	disk_io_err_time [8];	/*  ASCII (hh:mm:ss)		*/
    int		arch2_sum_err_stat;	/*  Error Code 0-99		*/
    int		red_chan_io_err_stat;	/*  SVCI Error Code		*/
    int		red_chan_io_err_date;	/*  Julian Date from 1/1/1970	*/
    char	red_chan_io_err_time [8];	/*  ASCII (hh:mm:ss)	*/
    short	spare487;
    short	spare488;
    short	spare489;
    short	spare490;
    short	spare491;
    short	spare492;
    short	spare493;
    short	spare494;
    short	spare495;
    short	spare496;
    short	spare497;
    short	spare498;
    short	spare499;
    short	spare500;

} device_io_error_t;

typedef	struct {

    short	data31;			/*  Bit Data: 			*
					 *  0 = +5V VDC PS		*
					 *	set = FAULT		*
					 *  1 = +15V VDC PS		*
					 *	set = FAULT		*
					 *  2 = +28V VDC PS		*
					 *	set = FAULT		*
					 *  3 = -15V VDC PS   		*
					 *	set = FAULT		*
					 *  4 = +45V VDC PS		*
					 *	set = FAULT		*
					 *  5 = Filament PS Voltage	*
					 *	set = FAULT		*
					 *  6 = Vacuum Pump PS Voltage	*
					 *	set = FAULT		*
					 *  7 = Focus Coil PS Voltage	*
					 *	set = FAULT		*
					 *  8 = Filament PS 		*
					 *	set = OFF		*
					 *  9 = Klystron Warmup		*
					 *	set = PREHEAT		*
					 * 10 = Transmitter Available	*
					 *	set = NOT AVAIL		*
					 * 11 = WG Switch Position	*
					 *	set = DUMMY LOAD	*
					 * 12 = WG/PFN Transfer Interlk	*
					 *	set = OPEN		*
					 * 13 = Maintenance Mode	*
					 *	set = MAINTENANCE	*
					 * 14 = Maintenance Required	*
					 *      set = WORK REQUIRED	*
					 * 15 = PFN Switch Position	*
					 *	set = LONG PULSE	*/

    short	data32;			/*  0 = Modulator Overload	*
					 *	set = FAULT		*
					 *  1 = Modulator Inv. Current	*
					 *	set = FAULT		*
					 *  2 = Modulator Switch Fail	*
					 *	set = FAULT		*
					 *  3 = Main Power Voltage	*
					 *	set = FAULT		*
					 *  4 = Flyback Charger		*
					 *	set = FAULT		*
					 *  5 = Inverse Diode Curr.	*
					 *	set = FAULT		*
					 *  6 = Trigger Amplifier	*
					 *	set = FAULT		*
					 *  7 = spare			*
					 *	set to 0		*
					 *  8 = Circulator Temperature	*
					 *	set = FAULT		*
					 *  9 = Spectrum Filter Press.	*
					 *	set = FAULT		*
					 * 10 = WG ARC/VSWR		*
					 *	set = FAULT		*
					 * 11 = Cabinet Interlock	*
					 *	set = FAULT		*
					 * 12 = Cabinet Ait Temperature	*
					 *	set = FAULT		*
					 * 13 = Cabinet Airflow		*
					 *	set = FAULT		*
					 * 14 = Spare			*
					 *	set to 0		*
					 * 15 = Xmtr Spare		*
					 *	set to 0		*/

    short	data33;			/*  0 = Klystron Current	*
					 *	set = FAULT		*
					 *  1 = Klystron Fil. Current	*
					 *	set = FAULT		*
					 *  2 = Klystron Vacion Current	*
					 *	set = FAULT		*
					 *  3 = Klystron Air Temp.	*
					 *	set = FAULT		*
					 *  4 = Klystron Airflow	*
					 *	set = FAULT		*
					 *  5 = One Test Bit 5		*
					 *	1 = NORMAL		*
					 *  6 = One Test Bit 6		*
					 *	1 = NORMAL		*
					 *  7 = One Test Bit 7		*
					 *	1 = NORMAL		*
					 *  8 = Transmitter O.V.	*
					 *	set = FAULT		*
					 *  9 = Transmitter O.C.	*
					 *	set = FAULT		*
					 * 10 = Focus Coil Current	*
					 *	set = FAULT		*
					 * 11 = Focus Coil Airflow	*
					 *	set = FAULT		*
					 * 12 = Oil Temperature		*
					 *	set = FAULT		*
					 * 13 = PRF Limit		*
					 *	set = FAULT		*
					 * 14 = Oil Level		*
					 *	set = FAULT		*
					 * 15 = Tranmitter Bat. Charg.	*
					 *	0 = CHARGING		*/

    short	data34;			/*  0 = Zero Test Bit 0		*
					 *	0 = NORMAL		*
					 *  1 = Zero Test Bit 1		*
					 *	0 = NORMAL		*
					 *  2 = Zero Test Bit 2		*
					 *	0 = NORMAL		*
					 *  3 = Zero Test Bit 3		*
					 *	0 = NORMAL		*
					 *  4 = Zero Test Bit 4		*
					 *	0 = NORMAL		*
					 *  5 = Zero Test Bit 5		*
					 *	0 = NORMAL		*
					 *  6 = Zero Test Bit 6 	*
					 *	0 = NORMAL		*
					 *  7 = Zero Test Bit 7		*
					 *	0 = NORMAL		*
					 *  8 = One Test Bit 0		*
					 *	1 = NORMAL		*
					 *  9 = One Test Bit 1		*
					 *	1 = NORMAL		*
					 * 10 = One Test Bit 2		*
					 *	1 = NORMAL		*
					 * 11 = One Test Bit 3		*
					 *	1 = NORMAL		*
					 * 12 = One Test Bit 4		*
					 *	1 = NORMAL		*
					 * 13 = Nod. Switch Maint.	*
					 *	set = MAINT. REQ.	*
					 * 14 = Post Chrg. Reg.		*
					 *	set = MAINT. REQ.	*
					 * 15 = WG Pressure/Humidity	*
					 *	set = FAULT		*/

    short	data35;			/*  0 = AC #1 Compress Shut Off	*
					 *	1 = SHUTOFF		*
					 *  1 = ac #2 compress Shut Off	*
					 *	1 = SHUTOFF		*
					 *  2 = Generator Maint. Reqd.	*
					 *	0 = MAINT. REQUIRED	*
					 *  3 = Power Source		*
					 *	0 = UTILITY POWER	*
					 *  4 = Gen Battery Voltage	*
					 *	1 = OK			*
					 *  5 = Gen. Engine		*
					 *	1 = OK			*
					 *  6 = Transitional Power TPS	*
						1 = OFF, 0 = OK		*
					 *  7 = Spare			*
						set to 1		*
					 *  8 = H.V.			*
					 *	0 = ON; 1 = OFF		*
					 *  9 = TX Recycling Summary	*
					 *	1 = RECYCLING		*
					 * 10 = Xmtr INOP		*
					 *	1 = INOPERATIVE		*
					 * 11 = COHO/CLOCK		*
					 *	set = FAULT		*
					 * 12 = DAU UART		*
					 *	set = FAULT		*
					 * 13 = Spare			*
					 *	set to 1		*
					 * 14 = Spare			*
					 *	set to 1		*
					 * 15 = Spare			*
					 *	set to 1		*/

    short	data36;			/*  0 = Gen. Shelter Fire/Smoke	*
					 *	1 = NORMAL		*
					 *  1 = Eq. Shelter Halon Sys.	*
					 *	0 = NORMAL		*
					 *  2 = Utility Volt/Freq.	*
					 *	1 = AVAILABLE		*
					 *  3 = +9V Receiver PS		*
					 *	1 = FAULT		*
					 *  4 = +/-15V A/D Conv. PS	*
					 *	1 = FAULT		*
					 *  5 = +5V A/D Conv. PS	*
					 *	1 = FAULT		*
					 *  6 = Spare			*
					 *	set to 1		*
					 *  7 = -5.2V A/D Conv. PS	*
					 *	1 = FAULT		*
					 *  8 = Pwr. Xfer Switch	*
					 *	1 = AUTO; 0 = MANUAL	*
					 *  9 = Gen. Volt/Freq. Avail.	*
					 *	1 = AVAILABLE		*
					 * 10 = Aircraft Lightning	*
					 *	1 = OK			*
					 * 11 = Eq. Shelter Fire Sys.	*
					 *	0 = NORMAL		*
					 * 12 = +5V Receiver PS		*
					 *	1 = FAULT		*
					 * 13 = Eq. Shelter Fire/Smoke	*
					 *	0 = NORMAL		*
					 * 14 = +/-18V Receiver PS	*
					 *	1 = FAULT		*
					 * 15 = -9V Receiver PS		*
					 *	1 = FAULT		*/

    short	data37;			/*  0 = AC #1 Filter Dirty	*
					 *	0 = DIRTY		*
					 *  1 = AC #2 Filter Dirty	*
					 *	0 = DIRTY		*
					 *  2 = Xmtr Air Filter		*
					 *	0 = DIRTY		*
					 *  3 = Spare			*
					 *	set to 1		*
					 *  4 = Spare			*
					 *	set to 1		*
					 *  5 = Spare			*
					 *	set to 1		*
					 *  6 = Spare			*
					 *	set to 1		*
					 *  7 = Spare			*
					 *	set to 1		*
					 *  8 = Site Security		*
					 *	0 = ALARM		*
					 *  9 = Security Equipment	*
					 *	0 = FAULT; 1 = OK	*
					 * 10 = Security System		*
					 *	0 = DISABLED; 1 = OK	*
					 * 11 = +5V Rec Prot PS		*
					 *	1 = FAULT		*
					 * 12 = Spare			*
					 *	set to 1		*
					 * 13 = Spare			*
					 *	set to 1		*
					 * 14 = Rec. not Con. to Ant.	*
					 *	1 = NOT CONNECTED	*
					 * 15 = Radome Hatch		*
					 *	0 = OPEN		*/

    unsigned char	out_temp;	/*  -50 <= X <= 50 Deg C	*
					 *  T = (val-51)*(100/204)	*/
    unsigned char	eqp_shelt_temp;	/*    0 <= X <= 50 Deg C	*  
					 *  T = (val-51)*(50/204)	*/
    unsigned char	ac1_air_temp;	/*    0 <= X <= 50 Deg C	*  
					 *  T = (val-51)*(50/204)	*/
    unsigned char	trans_air_temp;	/*  -10 <= X <= 60 Deg C	*  
					 *  T = (val-51)*(70/204)	*/
    unsigned char	radome_air_temp;/*  -50 <= X <= 50 Deg C	*  
					 *  T = (val-51)*(100/204)	*/
    unsigned char	gen_shelt_temp;	/*    0 <= X <= 50 Deg C	*  
					 *  T = (val-51)*(50/204)	*/
    unsigned char	gen_fuel_level; /*    0 <= X <= 100 %		*
					 *  T = (val-51)*(100/204)	*/
    unsigned char	spare41l;	/*  1 = SPARE			*/
    unsigned char	spare42h;	/*  1 = SPARE			*/
    unsigned char	spare42l;	/*  1 = SPARE			*/
    unsigned char	spare43h;	/*  1 = SPARE			*/
    unsigned char	spare43l;	/*  1 = SPARE			*/
    unsigned char	spare44h;	/*  1 = SPARE			*/
    unsigned char	spare44l;	/*  1 = SPARE			*/
    unsigned char	ac2_air_temp;	/*    0 <= X <= 50 Deg C	*  
					 *  T = (val-51)*(50/204)	*/
    unsigned char	spare45l;	/*  1 = SPARE			*/
    unsigned char	spare46h;	/*  1 = SPARE			*/
    unsigned char	xmtr_rf_pwr;	/*  -0.4 <= X <= 9.6 MN		*/
    unsigned char	ant_rf_pwr;	/*  -0.4 <= X <= 9.6 MN		*/
    unsigned char	spare47l;
    unsigned char	spare48h;	/*  1 = SPARE			*/
    unsigned char	spare48l;
    unsigned char	spare49h;
    unsigned char	ped_p28v_ps;	/*  0 <= X <= 40.8 Volts	*
					 *	V = 40.8/255		*/
    unsigned char	enc_p5v_ps;	/*  0 <= X <= 18.36 Volts       *
					 *      V = 18.36/255           */
    unsigned char	ped_p15v_ps;	/*  0 <= X <= 20.0 Volts	*
					 *	V = 20.0/255		*/
    unsigned char	spare51h;
    unsigned char	ped_p5v_ps;	/*  0 <= X <= 6.64 Volts	*
					 *	V = 6.64/255		*/
    unsigned char	spare52h;
    unsigned char	spare52l;
    unsigned char	sig_proc_p5v_ps;/*  0 <= X <= 6.64 Volts	*
					 *	V = 6.64/255		*/
    unsigned char	spare53l;
    unsigned char	mnt_con_p28v_ps;/*  0 <= X <= 37.4 Volts	*
					 *	V = 37.4/255		*/
    unsigned char	mnt_con_p15v_ps;/*  0 <= X <= 20.0 Volts	*
					 *	V = 20.0/255		*/
    unsigned char	mnt_con_p5v_ps;	/*  0 <= X <= 6.64 Volts	*
					 *	V = 6.64/255		*/
    unsigned char	spare55l;
    unsigned char	spare56h;
    unsigned char	spare56l;
    unsigned char	ped_n15v_ps;	/*  0 <= X <= 20.0 Volts	*
					 *	V = 20.0/255		*/
    unsigned char	spare57l;
    unsigned char	spare58h;
    unsigned char	mnt_con_n15v_ps;	/*  0 <= X <= 40.8 Volts	*
					 *	V = 40.8/255		*/
    unsigned char	spare59h;
    unsigned char	dau_test0;
    unsigned char	dau_test1;
    unsigned char	dau_test2;
    unsigned char	spare61h;
    unsigned char	spare61l;
    short		dau_interface;	/*  1 = OK; 0 = FAILED		*/
    short		xmtr_sum_stat;	/*  0 = OK; 1 = FAIL; 2 = MAINT	*
					 *  3 = RECYC.			*/
    short		spare64;
    short		spare65;
    short		spare66;
    short		spare67;
    short		spare68;
    short		spare69;
    short		spare70;
    short		spare71;
    short		spare72;
    short		spare73;
    short		spare74;
    short		spare75;
    short		spare76;
    short		spare77;
    short		spare78;
    short		spare79;
    short		spare80;
    short		spare81;
    short		spare82;
    short		spare83;
    short		spare84;
    short		spare85;
    short		spare86;
    short		spare87;
    short		spare88;
    short		spare89;
    short		spare90;
    short		spare91;
    short		spare92;
    short		spare93;
    short		spare94;
    short		spare95;
    short		data96;		/*  1 = Elev Servo Amp Inhibit	*
					 *	1 = INHIBIT		*
					 *  2 = Elev Servo Amp Short	*
					 *	1 = SHORT CIRCUIT	*
					 *  3 = Elev Servo Amp Overtemp	*
					 *	1 = OVERTEMP		*
					 *  4 = +15V Overvoltage	*
					 *	0 = NORMAL		*
					 *  5 = +150V Undervoltage	*
					 *	0 = NORMAL		*
					 *  6 = Elev Motor Overtemp	*
					 *	1 = OVERTEMP		*
					 *  7 = Elev Stow Pin		*
					 *	1 = ENGAGED		*
					 *  8 = Elev PCU Parity		*
					 *	1 = PARITY ERROR	*
					 *  9 = Elev Dead Limit		*
					 *	1 = IN DEAD LIMIT	*
					 * 10 = Spare			*
					 *	set to 0		*
					 * 11 = Elev + Normal Limit	*
					 *	1 = IN NORMAL + LIMIT	*
					 * 12 = Elev - Normal Limit	*
					 *	1 = IN NORMAL - LIMIT	*
					 * 13 = Spare			*
					 *	set to 0		*
					 * 14 = Elev Encoder Light	*
					 *	1 = FAILURE		*
					 * 15 = Elev Gearbox Oil	*
					 *	1 = OIL LEVEL LOW	*/

    short		data97;		/*  1 = Azi Servo Amp Inhibit	*
					 *	1 = INHIBIT		*
					 *  2 = Azi Servo Amp Short Cir	*
					 *	1 = SHORT CIRCUIT	*
					 *  3 = Azi Servo Amp Overtemp	*
					 *	1 = OVERTEMP		*
					 *  4 = Spare			*
					 *	set to 0		*
					 *  5 = Spare			*
					 *	set to 0		*
					 *  6 = Azi Motor Overtemp	*
					 *	1 = OVERTEMP		*
					 *  7 = Azi Stop Pin		*
					 *	1 = ENGAGED		*
					 *  8 = Azi PCU Parity		*
					 *	1 = PARITY ERROR	*
					 *  9 = Azi Encoder Light	*
					 *	1 = FAILURE		*
					 * 10 = Azi Gearbox Oil		*
					 *	1 = OIL LEVEL LOW	*
					 * 11 = Azi Bull Gear Oil	*
					 *	1 = OIL LEVEL LOW	*
					 * 12 = Spare			*
					 *	set to 0		*
					 * 13 = Spare			*
					 *	set to 0		*
					 * 14 = Azi Handwheel		*
					 *	1 = ENGAGED		*
					 * 15 = Spare			*
					 *	set to 0		*/

    short		data98;		/*  1 = SPARE			*
					 *	set to 0		*
					 *  2 = SPARE			*
					 *	set to 0		*
					 *  3 = SPARE			*
					 *	set to 0		*
					 *  4 = Azi Servo Amp PS	*
					 *	0 = NORMAL		*
					 *  5 = Elev Amp PS		*
					 *	0 = NORMAL		*
					 *  6 = Servo			*
					 *	1 = OFF; 0 = ON		*
					 *  7 = Ped Interlock Switch	*
					 *	1 = SAFE; 0 = OPER	*
					 *  8 = SPARE			*
					 *	set to 0		*
					 *  9 = SPARE			*
					 *	set to 0		*
					 * 10 = SPARE			*
					 *	set to 0		*
					 * 11 = SPARE			*
					 *	set to 0		*
					 * 12 = SPARE			*
					 *	set to 0		*
					 * 13 = SPARE			*
					 *	set to 0		*
					 * 14 = SPARE			*
					 *	set to 0		*
					 * 15 = SPARE			*
					 *	set to 0		*/

    short		az_pos_corr;	/*  0 <= X <= 360 Degrees	*
					 *  Coded the same as basedata	*
					 *  azimuth angles.		*/
    short		el_pos_corr;	/*  0 <= X <= 360 Degrees	*
					 *  Coded the same as basedata	*
					 *  azimuth angles.		*/
    short		spare101;
    short		spare102;
    short		spare103;
    short		spare104;
    short		spare105;
    unsigned char	spare106h;	/*  Spare			*/
    unsigned char	rf_gen_data;	/*  0 = Spare			*
					 *  1 = Spare			*
					 *  2 = Spare			*
					 *  3 = Spare			*
					 *  4 = Spare			*
					 *  5 = RF Gen Phase Shftd COHO	*
					 *	1 = FAIL		*
					 *  6 = RF Gen RF/Stalo		*
					 *	1 = FAIL		*
					 *  7 = RF Gen Freq Sel Oscil	*
					 *	1 = FAIL		*/

    short		spare107;
    short		spare108;
    unsigned char	spare109h;
    unsigned char	parity_alrm_cf;	/*  0 = Parity Alarm CF1	*
						1 = PARITY ERROR	*
					 *  1 = Parity Alarm CF2	*
						1 = PARITY ERROR	*
					 *  2 = Parity Alarm CF3	*
						1 = PARITY ERROR	*
					 *  3 = Parity Alarm CF4	*
						1 = PARITY ERROR	*
					 *  4 = Parity Alarm CF5	*
						1 = PARITY ERROR	*
					 *  5 = Parity Alarm CF6	*
						1 = PARITY ERROR	*
					 *  6 = Parity Test Gen RAM	*
						1 = PARITY ERROR	*
					 *  7 = Spare			*/

    short		spare110;
    short		spare111;
    unsigned short	prt1_int;	/*  9.6 MHz Clock Count		*
					 *  0 <= X <= 65535		*/

    unsigned short	prt2_int;	/*  9.6 MHz Clock Count		*
					 *  0 <= X <= 65535		*/

    short		spare114;
    short		spare115;
    short		spare116;
    short		spare117;
    short		spare118;
    short		spare119;
    short		spare120;
    short		spare121;
    short		spare122;
    float		ant_pk_pwr;		/*  0 <= X <= 999.9 w	*/
    float		xmtr_pk_pwr;		/*  0 <= X <= 999.9 Kw	*/
    float		ant_rf_ave_pwr;		/*  0 <= X <= 999.9 w	*/
    float		xmtr_rf_ave_pwr;	/*  0 <= X <= 999.9 w	*/
    float		mwave_loss;		/*  -99.9 <=X<= 99.9 dB	*/
    float		ant_pwr_mtr_zero;	/*  0.0 <=X<= 255.0	*/
    float		xmtr_pwr_mtr_zero;	/*  0.0 <=X<= 255.0	*/
    int			xmtr_rec_cnt;		/*  0 <=X<= 999999	*/
    short		spare139;
    short		spare140;
    float		sh_pulse_lin_chan_noise;/*			*/
    float		sh_pulse_log_chan_noise;/*			*/
    float		lo_pulse_lin_chan_noise;/*			*/
    float		lo_pulse_log_chan_noise;/*			*/
    float		system_noise_temp;	/*  0 <=X<= 9999 Deg K	*/
    short		idu_tst_detect;		/*  0 <=X<= 9999 	*/
    short		spare152;

} bitdata_t;

typedef	struct {

    RDA_RPG_message_header_t	msg_hdr;
    wideband_t		rpg;
    wideband_t		user;
    bitdata_t		data;
    calibration_t	calibration;
    clutter_check_t	clutter_check;
    disk_file_status_t	disk_status;
    device_init_t	device_init;
    device_io_error_t	device_error;
    short		interproc_chan_resp;	/*  0=NA; 1=NO; 2=YES	*/
    short		spare502;
    short		chan_in_cntrl;		/*  0=NA; 1=NO; 2=YES	*/
    short		spare504;
    short		spare505;
    short		spare506;
    short		spare507;
    short		spare508;
    short		spare509;
    short		spare510;
    short		spare511;
    short		spare512;
    short		spare513;
    short		spare514;
    short		spare515;
    short		spare516;
    short		spare517;
    short		spare518;
    short		spare519;
    short		spare520;

} rda_performance_t;

#endif
