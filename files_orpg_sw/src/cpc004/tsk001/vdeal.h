
/***********************************************************************

    Description: Internal include file for vdeal.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/22 21:58:14 $
 * $Id: vdeal.h,v 1.12 2014/08/22 21:58:14 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */  

#ifndef VDEAL_H
#define VDEAL_H

#define MAX_ALTITUDE 18000.	/* max altitude, in meters, processed in
				   estimating EW */
#define MAX_EW_NRS 200		/* max number of EW grid points in range
				   direction */
#define MAX_N_PRF 3		/* max number of PRF sectors */
#define MAX_PARTITION_SIZE 56	/* max number of radials in a partition - 56 
				   is 13 partions for 720 radials*/
#define OVERLAP_SIZE 8		/* maximum partition one-side overlap size */

typedef struct {
    short x, y;
} Point_t;

enum {GATE_OTHER, GATE_ISOLATE};	/* for Gate_t.type */

typedef struct gate_t {	/* record special gate */
    short x;		/* x location */
    short y;		/* y location */
    unsigned char v;	/* value */
    unsigned char type;	/* type flag */
} Gate_t;

typedef struct {
    short nyq;		/* Nyquest v of PRF section */
    short azi;		/* start azi number of PRF section */
    int size;		/* number of radials of PRF section */
} Prf_sec_t;

typedef struct params {	/* algorithm paramters - may be optimized for VCP,
			   location and time of the year */
    int max_shear;	/* The maximum normal shear between neighboring gates.
			   This and the next are used in identifying connected
			   regions and 2D border type identification. */
    int am_shear;	/* The aliased maximum normal shear between
			   neighboring gates. */
    float weight_factor;/* weighting factor used for 2D weighting (.1). 0 for
			   linearly weighting. */
    float has_weight;	/* factor for shear-based azi weighting - improve 
			   tornado cases. >= 1 - disabled. */
    float bh_thr;	/* histogram analysis threshold for identifying failed
			   gates in 2D dealiasing (VDEAL_BH) (.35) */
    int small_gc;	/* small gate count used in histogram analysis */
    int data_off;	/* v offset (data value for v = 0) */

    float r_w_ratio;	/* factor for range border weight ratio */
    int r0;		/* (r0 + x) * r_w_ratio is gate width / gate size */
    int g_size;		/* gate size in range direction in meters */
    float data_scale;	/* data scale (number of levels per m/s) */
    int xs, ys;		/* region location */
    int xr, yr;		/* subsample ratio */
    void *vdv;

    int nyq;		/* Nyqust velocity */
    char strict_bh;	/* Sets _BH in terms of bh_thr (true), or not for
			   regions well connected to the good region */
    char fppi;		/* the region is full ppi */
    char tp_quant;	/* use two-peak quantization */
    char use_bc;	/* apply boundary condition */

    int stride;		/* buffer stride for dmap */
    unsigned char *dmap;/* region pointer to the dmap buffer */
} Params_t;

/* bit flags for Ew_struct_t.rfs */
#define RF_CLEAR_AIR 0x8
#define RF_HIGH_VS 0x2		/* high vertical shear - must be 2 */
#define RF_LOW_VS 0x1	/* low VS. If both set, very high VS - must be 1 */
#define RF_NONUNIFORM 0x20	/* non-uniform wind range */
#define RF_EW_UNAVAILABLE 0x4	/* EW not available */
#define RF_2SIDE_VAD 0x200	/* VAD on two sides exists */
#define RF_HIGH_WIND 0x10	/* high VAD wind detected */
#define RF_BV2 0x40		/* VAD failed because of bad data */

/* bit flags for Ew_struct_t.efs */
#define EF_HIGH_SHEAR 0x1	/* high shear area */
#define EF_STORM 0x4		/* storm area */
#define EF_CLEAR_AIR 0x8	/* clear air area */
#define EF_SECOND_TRIP 0x10	/* second trip in v */
#define EF_FRONT 0x20		/* front area */
#define EF_LOW_ELE_EW 0x40	/* use ew of lower elevation cut */
#define EF_HS_GRID 0x2		/* high shear grid detected by median inp */

typedef struct Ew_struct {
    int n_rgs;		/* number of EW grid points in range direction */
    int rz;		/* EW grid's range step size, in number of gates */
    int n_azs;		/* number of EW grid points in azimuthal direction */
    float az;		/* EW grid's azimuthal step size, in degrees */
    short *ews;		/* array of current EWs. n_rgs by n_azs */
    short *ewm;		/* pointer to EW map. n_rgs by n_azs */
    unsigned short *rfs;/* flags for each range */
    unsigned char *eww;	/* ewm weighting for each ew grid */
    unsigned char *efs;	/* flags for each ew grid */
    void *ups;		/* array of EW updating info */
} Ew_struct_t;

/* for Vdeal_t.data_type */
#define DT_SUB_IMAGE 0x1
#define DT_HSPW_SET 0x4
#define DT_VH_VS 0x8		/* the data is of very high vertical shear */
#define DT_NONUNIFORM 0x01	/* the data is non-uniform */
#define DT_NOVAD 0x02		/* VAD is not available */

enum {RT_DONE, RT_START_ELE, RT_PROCESS, RT_COMPLETED};
						/* for Vdeal_t.rt_state */
enum {RS_NONE, RS_START_ELE, RS_NORMAL, RS_END_ELE};
						/* for Vdeal_t.radial_status */

typedef struct vdeal_g_t {	/* vdeal global variables */
    int xz;		/* cut size in range, in number of bins */
    int yz;		/* cut size in azimuth, in number of bins */
    int data_off;	/* data offset in data levels (value for 0 m/s) */
    float data_scale;	/* data scale (number of levels per m/s) */
    float start_range;	/* starting range of the cut, in meters */
    int g_size;		/* gate size in meters */
    float start_azi;	/* starting azimuth of the cut, in degrees */
    float gate_width;	/* gate width of the radials, in degrees */
    int nyq;		/* Nyquist velocity in data levels. If multiple PRF
			   sections, the min nyq of all sections, */
    float elev;		/* elevation of the cut in degrees */
    char full_ppi;	/* this is 360 degree ppi */
    char low_prf;	/* this is a low PRF scan */
    short vcp;		/* VCP number */
    short unamb_range;
    short phase;	/* processing phase: 1 (EEW) or 2 */
    int vol_num;	/* volume number */
    int cut_num;	/* cut number in the current volume */
    time_t dtm;		/* data time */
    int data_type;	/* data type */
    int nonuniform_vol;	/* the latest vol_num of nonuniform data detected */

    int n_secs;		/* number of PRF sections */
    Prf_sec_t secs[MAX_N_PRF];	/* PRF sections */

    short realtime;	/* this is realtime processing */
    short rt_read;	/* number of redials read in this elevation */
    short rt_processed;	/* number of redials processed in this elevation */
    short rt_done;	/* number of redials comleted in this elevation */
    char rt_state;	/* real time state */
    char radial_status;	/* status of the latest radial */

    unsigned char *inp;	/* input image of the current cut, xz by yz */
    unsigned char *spw;	/* SPW image of the current cut, xz by yz. In m/s. 
			   Missing data is 0. 1 is 0 m/s. All > 63 is 63. */
    unsigned char *dbz;	/* DBZ image of the current cut, xz by yz */
    unsigned char *ew_aind;	/* EW azimuth index of each radial */
    unsigned short *ew_azi; /* azimuth angle (in .1 degrees) of each radial */

    short *out;		/* output image, xz by yz */
    unsigned char *dmap;/* dealiasing map, xz by yz */

    Ew_struct_t ew;	/* EW data */
} Vdeal_t;

typedef struct region_t {	/* struct for a region (connected area) */
    short *data;	/* the regions data. The stride is xz */
    int xs;		/* x global location */
    int ys;		/* y global location */
    int xz;		/* x size */
    int yz;		/* y size */
    int n_gs;		/* number of valid gates */
} Region_t;

typedef struct clump_df {	/* struct for a data filtering */
    unsigned char *map;		/* filtering map - The same stride as data */
    unsigned char yes_bits;	/* if non-0, any bit will enable data */
    unsigned char exc_bits;	/* if non-0, any bit will exclude data */
    unsigned char *omap;	/* output map - The same stride as data */
    unsigned char mapv;		/* value for assigning (ORed) to omap */
} Data_filter_t;

/* bit flag for Vdeal_t.dmap */
#define DMAP_BH 2	/* bit flag for bad gate based on histogram value */
#define DMAP_BE 4	/* bit flag for bad gate based on EW */
#define DMAP_NPRCD 16	/* bit flag for not processed data */
#define DMAP_HSPW 32	/* bit flag for high SPW data */
#define DMAP_NHSPW 64	/* bit flag for not high SPW data */
#define DMAP_FILL 1	/* bit flag for temporarily filled-in gates */
#define DMAP_LOCAL 8	/* bit flag for local use */

/* bit mask for Vdeal_t.spw */
#define SPW_MASK_value 0x3f	/* for spw value of 6 bits */
#define SPW_MASK_hw 0x40	/* for all high spw */
#define SPW_MASK_hsr 0x80	/* for high spw inside HSR */

#define pi 3.141592653589
#define deg2rad 0.017453293	/* pi / 180. */
#define rad2deg 57.295779513	/* 180. / pi */
#define SNO_DATA (2047)	/* gate is missing for short ((short)0x7ff) */
#define BNO_DATA 0	/* gate is missing for unsigned byte */

typedef double Banbks_t;

typedef double Spmcg_t;

typedef struct {		/* sparse matrix struct */
    int *ne;			/* # non-zero elements for each row */
    int *c_ind;			/* list of non-zero element column indeces */
    Spmcg_t *ev;		/* element values corresponding to c_ind */
} Sp_matrix;

/* values for parameters of VDC functions */
enum {VDC_IDR_XS, VDC_IDR_SIZE, VDC_IDR_XZ};
#define VDC_IDR_SORT 0xff
#define VDC_IDR_WRAP 0x100
#define VDC_PARM_ARRAY 0x200
#define VDC_NEXT 0x10000000
#define VDC_BIN 0x80000000

typedef struct part_t {	/* partitions struct */
    int ys;		/* starting y in the cut */
    int yz;		/* partition size in number of gates */
    int yoff;		/* offset of the first partition radial */
    int eyz;		/* extended partition y size */
    int nyq;
    int depend[2];	/* dependent partition indexes */
    int state;		/* current processsing state */
    unsigned char *inp;	/* partition data input */
    unsigned char *dmap;/* partition dmap */
} Part_t;

typedef struct {	/* high shear feature struct */
    short type;		/* rec type: 0 for dealiasing error; 1 for HS */
    short x, y;		/* feature localtion, gate index and azi index */
    short xz, yz;	/* feature size */
    short maxs, mins;	/* max and min shears in the feature in .5 m/s */
    short n;		/* number of high shear borders */
    short nyq, unamb_range;
    short min_size;
    float threshold;
} Hs_feature_t;

int VD2D_2d_dealiase (short *carea, Params_t *parms, int n_gates,
				int xsize, int ysize, short *bd_dfs);
int VDB_solve (int n, Sp_matrix *a, Banbks_t *b);
int VDB_linear_fit (int n, double *x, double *y, double *ap, double *bp);
void VDB_check_timeout (int seconds);

int VDE_initialize (Vdeal_t *vdv);
int VDE_global_dealiase (Vdeal_t *vdv, Region_t *region,
				unsigned char *dmap, int stride, int *qerr);
int VDE_update_ew (Vdeal_t *vdv, Region_t *region);
void VDE_print_ew_flags (Vdeal_t *vdv);
int VDE_update_ew_data (Vdeal_t *vdv, Region_t *region);
short VDE_get_ew_value (Vdeal_t *vdv, int x, int y);

int VDD_get_nyq (Vdeal_t *vdv, int ys);
int VDD_process_image (Vdeal_t *vdv);
int VDD_apply_gd_copy_to_out (Vdeal_t *vdv, Region_t *region, int gd);
int VDD_init_vdv (Vdeal_t *vdv);
int VDD_process_realtime (Vdeal_t *vdv);
void VDD_log (const char *format, ...);
void VDD_set_parameters (Vdeal_t *vdv, Params_t *parms, int nyq);

void VDC_reset_next_region (void *rgsp);
int VDC_get_next_region (void *rgsp, int ind, Region_t *out);
int VDC_identify_regions (unsigned char *img, Data_filter_t *dmap, int stride, 
	int xst, int yst, int xsize, int ysize, 
	void *parms, int sort, void **rgsp);
void VDC_free (void *rgsp);

int VDE_get_azi_ind (Vdeal_t *vdv, double azimuth);
void VDE_ew_deal_area (Vdeal_t *vdv, int xs, int ys, int xz, int yz);
int VDE_check_global_deal (Vdeal_t *vdv, Region_t *region, int gd);
int VDE_reset (Vdeal_t *vdv, int all);
int VDE_generate_ewm (Vdeal_t *vdv);
void VDE_set_ew_flags (Vdeal_t *vdv);
int VDE_quantize_gd (int nyq, int diff, int *qerr);

int EE_get_eew (Vdeal_t *vdv);
int EE_estimate_ew (Vdeal_t *vdv);
short EE_get_eew_value (int x, int y);
int Myround (double x);
int EE_read_data (Vdeal_t *vdv, FILE *fl, char *fname, int new_ver, int ops);
int EE_save_data (Vdeal_t *vdv, FILE *fl, char *fname, int ops);
int EE_get_near_elev_ew (Vdeal_t *vdv, double r, double azi);
int EE_get_previous_ew (Vdeal_t *vdv, int x, int y);

int VDR_realtime_process (int argc, char *argv[], Vdeal_t *vdv);
int VDR_output_processed_radial (Vdeal_t *vdv);
int VDR_get_volume_time (time_t *v_st);
char *VDR_get_image_label ();
int VDR_get_ext_wind (int alt, int up, double *speed, double *dir);
void VDR_status_log (const char *msg);

int VDV_vad_analysis (Vdeal_t *vdv, int tmp);
int VDV_read_history (Vdeal_t *vdv, int ops);
int VDV_write_history (Vdeal_t *vdv, int ops);
int VDV_get_wind (Vdeal_t *vdv, int xs, double *spdp, double *azip);
void VDV_save_storm_distance (Vdeal_t *vdv);
int VDV_get_bw_vs_correction (Vdeal_t *vdv, int xi, int yi);
double VDV_range_to_alt (Vdeal_t *vdv, double R);
double VDV_alt_to_range (Vdeal_t *vdv, double alt);

void VD2D_realtime_processing (Vdeal_t *vdv);

void PP_fill_in_gaps (Vdeal_t *vdv, unsigned char *inp, int xz, int yz,
				int level, int fp, int max_gap, int nyq);
void PP_remove_high_shear_gates (unsigned char *inp, int xz, int yz, 
				int thr, int ethr, int fp, int nyq);
int PP_remove_noisy_data (unsigned char *inp, int xz, int yz, int level,
	int min_md, int thr, int fp, Data_filter_t *dft,
	unsigned char *outmap, unsigned char mapv, int nyq, int d_off);
void PP_convert_spw (unsigned char *spw, int n, int data_off);
short *PP_get_hz_cnt ();
int PP_analyze_z (Vdeal_t *vdv);
int PP_preprocessing_v (Vdeal_t *vdv, int ys, int yn);
void PP_set_front_ew (Vdeal_t *vdv);
void PP_setup_prf_sectors (Vdeal_t *vdv, int nyq, int n, int *secs);
unsigned char *PP_get_med_v ();
void PP_detect_fronts (Vdeal_t *vdv);

int VDA_search_median_value (int *d, int n, int nyq, int d_off, int *maxdp);
int VDA_Compute_shear_hist (unsigned char *inp, int xz, int yz, 
					int stride, int fp, int **histp);
void VDA_get_neighbor_offset (int n, int y, int xz, int yz, int fp, int *off);
int VDA_compute_data_hist (unsigned char *inp, int stride, int xs, 
					int xz, int yz, int **histp);
void VDA_set_constants (int nyq, int data_off);
int VDA_detect_false_shear (Vdeal_t *vdv, Region_t *reg, int thr, int *maxwp);
int VDA_check_fit_out (Vdeal_t *vdv, Region_t *reg, 
					int gd, int thr, int *bcntp);
int VDA_border_dealiase (Vdeal_t *vdv, Region_t *reg, Part_t *part, 
						int *conn, int *bcntp);
int VDA_check_border_conn (Vdeal_t *vdv, Region_t *reg, int use_out, int fppi);
void VDA_thin_and_dialate (unsigned char *mapbuf, int xz, int yz,
					int fp, int v, int level);
int VDA_find_thin_conn (unsigned char *inp, int xz, int yz, int level, int fp,
		Data_filter_t *dft, int nyq, int d_off, 
		unsigned char *outmap, unsigned char mapv);
int VDA_remove_single_gate_conn (unsigned char *inp, int xz, int yz, 
	    Data_filter_t *dft, unsigned char *outmap, unsigned char mapv);
int VDA_check_failed_gates (Vdeal_t *vdv, Part_t *parts, int ptind,
					Point_t *fp, int fp_bz);
unsigned char *VDA_get_border_map (Region_t *reg, int fppi);
int VDA_detect_false_hs (short *data, short *refd, int xz, int yz, int fp, 
						int thr, int *maxxy);
int VDA_get_reset_hsf (int set, void **hfs);
void VDA_detect_hs_features (Vdeal_t *vdv, int ys, int yz);

void CD_remove_ground_clutter (Vdeal_t *vdv);
void CD_spw_filter (Vdeal_t *vdv, int level, unsigned char *eew_inp);
int CD_read_gcc (Vdeal_t *vdv, FILE *fl, char *fname);
int CD_save_gcc (Vdeal_t *vdv, FILE *fl, char *fname);
int CD_get_saved_gcc_gate (Gate_t **saved_gates);
int CD_get_gcc_likely (char **p);

char *VDM_get_image_label ();
char *VDM_get_data_dir (char *buf, int buf_size);

/* debugging routines */
int dump_simage (char *name, short *image, int xsize, int ysize, int stride);
int dump_bimage (char *name, unsigned char *image, 
				int xsize, int ysize, int stride);
int Aliase_image (unsigned char *image, Vdeal_t *vdv, char *fname);
int VDT_dump_dmap (char *name, Vdeal_t *vdv);
int VDT_dump_ew (char *name, Vdeal_t *vdv, char *field);
int VDT_read_ew (char *name, Ew_struct_t *ew);
int VDT_dump_efs (char *name, Vdeal_t *vdv, unsigned char bit);

#endif		/* #ifndef VDEAL_H */
