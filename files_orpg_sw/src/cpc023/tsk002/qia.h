/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:12:01 $
 * $Id: qia.h,v 1.5 2012/03/12 13:12:01 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
*/

#ifndef _QIA_H
#define _QIA_H

#define QIA_NO_DATA -1.e20f
#define QIA_RF_DATA -2.e20f
#define C0 0.
#define Cp5 0.5
#define Q_scale  100.f
#define Q_offset 2.f
#define MAXRADIALS	360
#define BLOCK_RNG       230

/* Indicies into output buffer for each generic moment output */
#define DRHO_IDX  0
#define DPHI_IDX  1
#define DZDR_IDX  2
#define DSMZ_IDX  3
#define DSNR_IDX  4
#define DSMV_IDX  5
#define DKDP_IDX  6
#define DSDP_IDX  7
#define DSDZ_IDX  8
#define DQSZ_IDX  9
#define DQZD_IDX  10
#define DQRO_IDX  11
#define DQKD_IDX  12
#define DQTZ_IDX  13
#define DQTP_IDX  14

/* Configuration data from qia.alg */
    float Z_atten_thresh;
/* Configuration data from dp_precip.alg */
    int Min_blockage_thresh;
/* Beam Blockage array.  NA11-00386  */
   char  Beam_Blockage[MAXRADIALS*10][BLOCK_RNG];

typedef struct {	  /* parameters extracted from input basedata header */
    float   zr0;  	  /* range of the center of the first sample volume (km) */
    int     n_zgates; /* number of Z sample volumes in the radial */
    float   zg_size;  /* Z sample volume size (km) */
    float   vr0;	  /* Doppler range 0 (km) */ 
    int     n_vgates; /* number of V sample volumes in the radial */
    float   vg_size;  /* V sample volume size (km) */
    float   dr0;	  /* DP range 0 (km) */ 
    int     n_dgates; /* number of DP sample volumes in the radial */
    float   dg_size;  /* DP sample volume size (km) */
} Qia_params_t;

typedef struct {	  /* structure for Quality Index Algorithm output */
    float *smz;       /* Smoothed reflectivity */
    float *q_smz; 	  /* Quality index for smoothed reflectivity */
    float *zdr;       /* Differential reflectivity */
    float *q_zdr; 	  /* Quality index for differential reflectivity */
    float *rho;       /* Correlation coefficient */
    float *q_rho;	  /* Quality index for correlation coefficient */
    float *kdp;       /* Specific differential phase */
    float *q_kdp; 	  /* Quality index for specific differential phase */
    float *sdz;       /* Texture of reflectivity */
    float *q_sdz; 	  /* Quality index for texture of smoothed reflectivity */
    float *sdp;       /* Texture of differential phase */
    float *q_sdp; 	  /* Quality index for texture of differential phase */
    float *phi;       /* Differential phase */
    float *snr;       /* Signal-to-noise ratio */
    float *smv;       /* Smoothed velocity */
} Qia_data_fields_t;

int Qia_acl(void);
int Qia_process_radial(char *input, char **output, int *length);
void read_Blockage(int elev_angle_tenths, char Beam_Blockage[MAXRADIALS*10][BLOCK_RNG]);

#endif           
