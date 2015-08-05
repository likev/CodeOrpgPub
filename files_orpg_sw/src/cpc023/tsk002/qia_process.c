/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/06 18:56:31 $
 * $Id: qia_process.c,v 1.12 2014/10/06 18:56:31 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
*/
/******************************************************************************
     Filename: qia_process.c
     Author:   Brian Klein
     Created:  13 September 2007

     Description
     ===========
     This is the processing module for the Quality Index Algorithm.
     This algorithm was spun off of the original Hydrometeor Classification
     algorithm as a front-end processor for HCA input.  It ingests radials
     of DualPol data from the Preprocessor (dpprep). It outputs the same
     input data appending to it the quality index for each input data
     field.         

     Initial implementation is a "simple" version that is based mostly on
     differential phase (PhiDP) and does not require gradient processing.
     The "complex" version does require gradient processing and therefore
     introduces a significant latency in the processing stream.     

     Change History
     ==============
     Brian Klein;   13 September 2007;  CCR TBD;  Initial implementation
	
******************************************************************************/
#include <rpgc.h>
#include <rpgcs.h>
#include "qia.h"

/* File scope data */
#define DEBUG_AZ    -1    /* Set to positive azimuth value for debug output */
#define DEBUG_BIN   40    /* If DEBUG_AZ is positive, this is the first bin output */
#define N_IN        9      /* Number of input data fields */
#define N_QF        15     /* Number of output data fields */
#define KM_TO_M     1000.f /* Kilometers to meters conversion */
#define WORD_16     16     /* Size of a 16 bit word */
#define WORD_8      8      /* Size of an 8 bit word */
#define LARGEST_16  0xFFFF /* Largest non-flag value in a 16 bit word */
#define SMALLEST_16 2      /* Smallest non-flag value in a 16 bit word */ 
#define LARGEST_8   0xFF   /* Largest non-flag value in an 8 bit word */
#define SMALLEST_8  2      /* Smallest non-flag value in an 8 bit word */


/* Input data field indicies */
enum {IN_SMZ, IN_SMV, IN_ZDR, IN_PHI, IN_RHO, IN_KDP, IN_SDZ, IN_SDP, IN_SNR};

/* Input data field names */
static char *F_name[] = {"DSMZ", "DSMV", "DZDR", "DPHI", "DRHO",
                         "DKDP", "DSDZ", "DSDP", "DSNR"         };

/* Pointers to input data fields */
static Generic_moment_t *Zdr_hd, *Phi_hd, *Rho_hd, *Smz_hd, 
                        *Smv_hd, *Kdp_hd, *Sdz_hd, *Sdp_hd, *Snr_hd;

/* Function prototypes */
static int Get_data_fields (Qia_params_t *params, Base_data_header *bh, 
                            Qia_data_fields_t *data);
static int Verify_gm_hd (Base_data_header *bh, Generic_moment_t *gm, 
					int field);
static void Print_unique_msg (char *msg);
static int Format_output_radial (Base_data_header *bh, Qia_params_t *params,
		                       Qia_data_fields_t *data, char **out_radial);
static void Add_moment (char *buf, char *name, char f, Qia_params_t *params, 
	                   int word_size, unsigned short tover,
                        short SNR_threshold, float scale, float offset,
                        float *data);

/******************************************************************************
     Description:  This is the process radial function for the dual-pol
     radar data Quality Index Algorithm. The Quality Index is used by
     the Hydrometeor Classification Algorithm to weight the HCA's fuzzy logic
     input variables in its classification processing.
     Input:    input - Pointer to input linear buffer containing one radial
                       of dual pol data from the preprocessor.
     Output:   output - Pointer to pointer of processed DP and quality
                        index data ready for output.
     Returns:  Base_data_header status or -1 if no dual pol data
     Notes:    
******************************************************************************/
int Qia_process_radial(char *input, char **output, int *length)
{
    int i;
    double Ac,Bc,Cc,Dc,Fc,Ec,Gc,Hc,Ic,Jc,Kc,Lc,Mc,Nc;
    double linear_snr;
    int blockage_rng, blockage_azm, blocked_percent;
    Base_data_header *bh;
    Qia_params_t params;   /* Items extracted from preprocessor output */
    char  *out_radial;
    static Qia_data_fields_t data;	/* Structure for all output fields */

    /* Algorithm constants (see AEL section 2.1) */
    const float PHI_DP_Z_THRESH = 600.0;   /* units: deg/km (was 300 before attenuation fix)*/
    const float PHI_DP_ZDR_THRESH = 300.0; /* units: deg/km */
    const float PHI_DP_PHI_THRESH = 100.0; /* units: deg/km (was 200 before attenuation fix)*/
    const float PHI_DP_KDP_THRESH = 100.0; /* units: deg/km (was 200 before attenuation fix)*/

    const float LINEAR_SNR_Z_THRESH = 1;   /* pow(10.0,(0.1*0.0));  0.0db */
    const float LINEAR_SNR_ZDR_THRESH = 3.16228; /* pow(10,(0.1*5.0)); 5.0db */

    const float DELTA_RHO_1_THRESHOLD = 0.5; /* unitless */
    const float RHO_MIN_THRESH = 0.8;      /* unitless ratio */

    const float LINEAR_SNR_KDP_THRESH = 1; /* pow(10.0,(0.1*0.0)); 0.0db */
    const float LINEAR_SNR_SDZ_THRESH = 1; /* pow(10.0,(0.1*0.0)); 0.0db */
    const float BLOCKAGE_THRESH = 50;      /* unitless */

    const float C = -0.69;                 /* exponent */

    bh = (Base_data_header *)input;

    /* Reuse the input buffer for output */
    *output = input;
    *length = bh->msg_len * sizeof (short);

    if (bh->n_surv_bins == 0){			/* spot blanked radial */
       Print_unique_msg ("Spot Blanked Radial");
	  return (bh->status);
    }

    if (bh->azi_num == 1)             /* Reset the unique message tracker */
       Print_unique_msg ("");

    /* Get the input data, if -1 returned then abort */
    if (Get_data_fields(&params, bh, &data) < 0) {
   
       /* Free memory allocated on our behalf by RPGCS_radar_data_conversion */
       if (data.smz != NULL) { free(data.smz); data.smz = NULL; }
       if (data.smv != NULL) { free(data.smv); data.smv = NULL; }
       if (data.rho != NULL) { free(data.rho); data.rho = NULL; }
       if (data.phi != NULL) { free(data.phi); data.phi = NULL; }
       if (data.zdr != NULL) { free(data.zdr); data.zdr = NULL; }
       if (data.snr != NULL) { free(data.snr); data.snr = NULL; }
       if (data.kdp != NULL) { free(data.kdp); data.kdp = NULL; }
       if (data.sdz != NULL) { free(data.sdz); data.sdz = NULL; }
       if (data.sdp != NULL) { free(data.sdp); data.sdp = NULL; }
       return (-1);
    }

    /* Loop for all dual pol gates.  (CPT&E label C) */
    for (i = 0; i < params.n_dgates; i++)
    {
      /* Compute the components of the Quality Index equations */
      /* AEL section 3.2.2 (CPT&E label D) */
      linear_snr = pow(10.0, (0.1*data.snr[i]));
      Ac = data.phi[i]/PHI_DP_Z_THRESH;
      Bc = LINEAR_SNR_Z_THRESH/linear_snr;
      Cc = data.phi[i]/PHI_DP_ZDR_THRESH;
      Dc = (1.0-data.rho[i])/DELTA_RHO_1_THRESHOLD;
      Ec = LINEAR_SNR_ZDR_THRESH/linear_snr;
      Fc = data.phi[i]/PHI_DP_PHI_THRESH;
      Gc = Dc;
      Hc = LINEAR_SNR_KDP_THRESH/linear_snr;
      Ic = data.phi[i]/PHI_DP_KDP_THRESH;
      Jc = Dc;
/*      Kc = LINEAR_SNR_KDP_THRESH/linear_snr;*//*From NSSL code*/
      Kc = Hc;    /* changed for efficiency */
      Lc = LINEAR_SNR_SDZ_THRESH/linear_snr;
      Mc = Kc;

      /* Added to address Partial Beam Blockage.                              */
      /* Get the beam blockage percentage.                                    */
      /* NOTE: blockage_azm is deliberately rounded to the nearest integer    */
      /* to ensure that the range of values falls within the array            */
      /* limits of 0 to 3599.                                                 */

      blockage_azm = (int) RPGC_NINT (bh->azimuth * 10.0);

      /* Make sure array index not over boundary */
      if(blockage_azm > 3599)
         blockage_azm = 3599;

      /* Convert range bin index from 250m to 1km */
      blockage_rng = (int) (i / 4);

      /* Blockage data only goes out to 230 km so keep using the old value beyond that */
      if (blockage_rng >= BLOCK_RNG) blockage_rng = BLOCK_RNG-1;

      /* Obtain the beam blockage value for each bin, convert from char to int.
       * NOTE: blocked_percent is an integer percentage (i.e 4 = 4%). */
      blocked_percent = (int) Beam_Blockage[blockage_azm][blockage_rng];

      if(blocked_percent <= Min_blockage_thresh) {
         Nc = 0.;          /* No blockage */
      }
      else {
         Nc = blocked_percent/BLOCKAGE_THRESH; 
      }
/*    if (bh->azi_num == 233 && i > 290 && i < 298)
    RPGC_log_msg(GL_INFO,"blocked_percent[%d]=%d i=%d Nc = %f",blockage_rng, blocked_percent, i, Nc);*/

      if (data.rho[i] < RHO_MIN_THRESH && data.smz[i] < Z_atten_thresh) {
        Dc = 0.0;
        Gc = 0.0;
        Jc = 0.0;
      }
  
      /* Quality index for smoothed reflectivity */
      data.q_smz[i] = exp( C * ((Ac*Ac) + (Bc*Bc) + (Nc*Nc) ));
    
      /* Quality index for differential reflectivity */
      data.q_zdr[i] = exp( C * ((Cc*Cc) + (Dc*Dc) + (Ec*Ec) + (Nc*Nc) ));

      /* Quality index for correlation coefficient */
      data.q_rho[i] = exp( C * ((Fc*Fc) + (Gc*Gc) + (Hc*Hc) ));

      /* Quality index for specific differential phase */
      data.q_kdp[i] = exp( C * ((Ic*Ic) + (Jc*Jc) + (Kc*Kc) ));

      /* Quality index for texture of reflectivity */
      data.q_sdz[i] = exp( C * (Lc*Lc));
   
      /* Quality index for texture of total differential phase */
      data.q_sdp[i] = exp( C * (Mc*Mc));

      /* handle finite problem */
      if ( !finite(data.q_smz[i]) || data.q_smz[i] == QIA_NO_DATA ) {
         data.q_smz[i] = 0.0;
      }
      if ( !finite(data.q_zdr[i]) || data.q_zdr[i] == QIA_NO_DATA ) {
         data.q_zdr[i] = 0.0;
      }
      if ( !finite(data.q_rho[i]) || data.q_rho[i] == QIA_NO_DATA ) {
         data.q_rho[i] = 0.0;
      }
      if ( !finite(data.q_kdp[i]) || data.q_kdp[i] == QIA_NO_DATA ) {
         data.q_kdp[i] = 0.0;
      }
      if ( !finite(data.q_sdz[i]) || data.q_sdz[i] == QIA_NO_DATA ) {
         data.q_sdz[i] = 0.0;
      }
      if ( !finite(data.q_sdp[i]) || data.q_sdp[i] == QIA_NO_DATA ) {
         data.q_sdp[i] = 0.0;
      }
    } /* End for all dual pol gates */

    /* Build the output radial */
    *length = Format_output_radial (bh, &params, &data, &out_radial);

/* DEBUG CODE FOLLOWS: */
/*    if (bh->azi_num == DEBUG_AZ){
     int start = DEBUG_BIN;
	RPGC_log_msg(GL_INFO,"Radial %d of elevation, bins %d to %d  rpg_elev_ind=%d elev_num=%d range_beg_surv=%d",
                           DEBUG_AZ,start,start+4, bh->rpg_elev_ind, bh->elev_num,bh->range_beg_surv );
     RPGC_log_msg(GL_INFO,"smz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.smz[0+start], data.smz[1+start], data.smz[2+start],
        data.smz[3+start], data.smz[4+start]);
     RPGC_log_msg(GL_INFO,"smv:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.smv[0+start], data.smv[1+start], data.smv[2+start],
        data.smv[3+start], data.smv[4+start]);
     RPGC_log_msg(GL_INFO,"rho:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.rho[0+start], data.rho[1+start], data.rho[2+start],
        data.rho[3+start], data.rho[4+start]);
     RPGC_log_msg(GL_INFO,"phi:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.phi[0+start], data.phi[1+start], data.phi[2+start],
        data.phi[3+start], data.phi[4+start]);
     RPGC_log_msg(GL_INFO,"zdr:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.zdr[0+start], data.zdr[1+start], data.zdr[2+start],
        data.zdr[3+start], data.zdr[4+start]);
     RPGC_log_msg(GL_INFO,"snr:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.snr[0+start], data.snr[1+start], data.snr[2+start],
        data.snr[3+start], data.snr[4+start]);
     RPGC_log_msg(GL_INFO,"kdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.kdp[0+start], data.kdp[1+start], data.kdp[2+start],
        data.kdp[3+start], data.kdp[4+start]);
     RPGC_log_msg(GL_INFO,"sdz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.sdz[0+start], data.sdz[1+start], data.sdz[2+start],
        data.sdz[3+start], data.sdz[4+start]);
     RPGC_log_msg(GL_INFO,"sdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.sdp[0+start], data.sdp[1+start], data.sdp[2+start],
        data.sdp[3+start], data.sdp[4+start]);
     RPGC_log_msg(GL_INFO,"q_smz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_smz[0+start], data.q_smz[1+start], data.q_smz[2+start],
        data.q_smz[3+start], data.q_smz[4+start]);
     RPGC_log_msg(GL_INFO,"q_rho:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_rho[0+start], data.q_rho[1+start], data.q_rho[2+start],
        data.q_rho[3+start], data.q_rho[4+start]);
     RPGC_log_msg(GL_INFO,"q_zdr:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_zdr[0+start], data.q_zdr[1+start], data.q_zdr[2+start],
        data.q_zdr[3+start], data.q_zdr[4+start]);
     RPGC_log_msg(GL_INFO,"q_kdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_kdp[0+start], data.q_kdp[1+start], data.q_kdp[2+start],
        data.q_kdp[3+start], data.q_kdp[4+start]);
     RPGC_log_msg(GL_INFO,"q_sdz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_sdz[0+start], data.q_sdz[1+start], data.q_sdz[2+start],
        data.q_sdz[3+start], data.q_sdz[4+start]);
     RPGC_log_msg(GL_INFO,"q_sdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data.q_sdp[0+start], data.q_sdp[1+start], data.q_sdp[2+start],
        data.q_sdp[3+start], data.q_sdp[4+start]);
     RPGC_log_msg(GL_INFO,"Output radial length: %d",*length);
    }**/
/* END DEBUG CODE */

    *output = out_radial;
    bh = (Base_data_header *)out_radial;

    /* Free memory allocated on our behalf by RPGCS_radar_data_conversion */
    if (data.smz != NULL) { free(data.smz); data.smz = NULL; }
    if (data.smv != NULL) { free(data.smv); data.smv = NULL; }
    if (data.rho != NULL) { free(data.rho); data.rho = NULL; }
    if (data.phi != NULL) { free(data.phi); data.phi = NULL; }
    if (data.zdr != NULL) { free(data.zdr); data.zdr = NULL; }
    if (data.snr != NULL) { free(data.snr); data.snr = NULL; }
    if (data.kdp != NULL) { free(data.kdp); data.kdp = NULL; }
    if (data.sdz != NULL) { free(data.sdz); data.sdz = NULL; }
    if (data.sdp != NULL) { free(data.sdp); data.sdp = NULL; }

    return (bh->status);
}/*End qia_process_radial() */

/******************************************************************************
     Description:  Extracts input data from radial and converts to floating
                   point values in physical units.  Also allocates memory
                   for output.
     Input:    None.
     Output:   None.
     Returns:  0 on success, -1 on error.
     Notes:    Error returned if all required input DP fields are not found.
******************************************************************************/
static int Get_data_fields (Qia_params_t *params, Base_data_header *bh,
                            Qia_data_fields_t *data) {
    static int buf_size = 0;
    static float *buf = NULL;
    int n_all_gates, cnt, i;
    int ngates = 0, range = 0, bin_size = 0;
    Generic_moment_t *gm;

    /* Extract number of bins, gate size and range to center of first gate
       for the survellance and Doppler moments.                            */
    params->n_zgates = bh->n_surv_bins;
    params->zg_size = (float)(.001) * bh->surv_bin_size;
    params->zr0 = (float)(.001) * bh->range_beg_surv + 
                  (float)(.5) * params->zg_size;

    params->n_vgates = bh->n_dop_bins;
    params->vg_size = (float)(.001) * bh->dop_bin_size;
    params->vr0 = (float)(.001) * bh->range_beg_dop +
                  (float)(.5) * params->vg_size;

    params->n_dgates = 0;

    /* Next, extract ZDR, PHI, RHO, KDP and SDP, gate size and range to
       center of first gate for the dual pol data                       */
    cnt = 0;
    for (i = 0; i < bh->no_moments; i++) {	/* get DP field gate info */

	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);

     /* RPG header file generic_basedata.h hardcodes a 4 character name. */
	if (strncmp (gm->name, F_name[IN_SMZ], 4) == 0 ||
            strncmp (gm->name, F_name[IN_ZDR], 4) == 0 ||
            strncmp (gm->name, F_name[IN_PHI], 4) == 0 ||
            strncmp (gm->name, F_name[IN_RHO], 4) == 0 ||
            strncmp (gm->name, F_name[IN_KDP], 4) == 0 ||
            strncmp (gm->name, F_name[IN_SNR], 4) == 0 ||
            strncmp (gm->name, F_name[IN_SDP], 4) == 0) {

	  if (cnt == 0) {
              ngates = gm->no_of_gates;
              range = gm->first_gate_range;
              bin_size = gm->bin_size;
              params->dr0 = M_TO_KM * range;
              params->n_dgates = ngates;
              params->dg_size = M_TO_KM * bin_size;
	    } /* end if */

	    cnt++;  /* Count the generic moments */

	} /* end if */
    } /*end for all generic moments */

    if (cnt == 7) {
       Print_unique_msg ("Dual-pol fields found-Processing");
    }
    else if (cnt == 0) {
	  Print_unique_msg ("No dual-pol field found-Output without processing");
	  return (-1);
    }
    else {
	  Print_unique_msg
        ("Unexpected number of dual-pol fields found-Output without processing");
       RPGC_log_msg(GL_INFO,"# Dual Pol Fields = %d",cnt);
	  return (-1);
    }

    /* Allocate buffers for the six output fields computed by this algorithm.
       All fields have n_dgates number of bins.  Because QIA cannot determine
       a quality value for bins beyond the other DP data fields, the two
       reflectivity-based fields (q_smz and q_sdz) are truncated to n_dgates */

    n_all_gates = 6 * (params->n_dgates + 1);

    if (n_all_gates > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (n_all_gates * sizeof (float));
	buf_size = n_all_gates;
    }

    /* Set all the pointers to their allocated memory */
    data->q_rho = buf;
    data->q_zdr = data->q_rho + (params->n_dgates + 1);
    data->q_kdp = data->q_zdr + (params->n_dgates + 1);
    data->q_sdp = data->q_kdp + (params->n_dgates + 1);
    data->q_smz = data->q_sdp + (params->n_dgates + 1);
    data->q_sdz = data->q_smz + (params->n_dgates + 1);

    /* The preprocessor sets the last bin to a flag value. 
       This is needed only for the case when n_gates is zero.  */
    data->q_rho[params->n_dgates] = QIA_NO_DATA;
    data->q_zdr[params->n_dgates] = QIA_NO_DATA;
    data->q_kdp[params->n_dgates] = QIA_NO_DATA;
    data->q_sdp[params->n_dgates] = QIA_NO_DATA;
    data->q_smz[params->n_dgates] = QIA_NO_DATA;
    data->q_sdz[params->n_dgates] = QIA_NO_DATA;

    /* Extract and convert all fields received from the preprocessor */
    for (i = 0; i < bh->no_moments; i++) {

	gm = (Generic_moment_t *)(((char *)bh) + bh->offsets[i]);
     /* RPG header file generic_basedata.h hardcodes a 4 character name. */
	if (strncmp (gm->name, F_name[IN_ZDR], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_ZDR) < 0)
		return (-1);
	    Zdr_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Zdr_hd->gate, Zdr_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->zdr)) != 1) {
            Print_unique_msg ("Missing ZDR field");
            return (-1);
         }           
         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Zdr scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Zdr_hd->scale, Zdr_hd->offset, Zdr_hd->no_of_gates, Zdr_hd->data_word_size,
            Zdr_hd->first_gate_range, *((data->zdr)+DEBUG_BIN));
	}
	else if (strncmp (gm->name, F_name[IN_PHI], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_PHI) < 0)
		return (-1);
	    Phi_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Phi_hd->gate, Phi_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->phi)) != 1) {
            Print_unique_msg ("Missing PHI field");
            return (-1);
         }
         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Phi scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Phi_hd->scale, Phi_hd->offset, Phi_hd->no_of_gates, Phi_hd->data_word_size,
            Phi_hd->first_gate_range, *((data->phi)+DEBUG_BIN));
/*         if (bh->azimuth == 324.5 || bh->azimuth == 2.5 || bh->azimuth == 193.5){
           fprintf(stdout,"PHI azimuth=%f\n", bh->azimuth);
         for (j=0;j< params->n_dgates; j++)
           fprintf(stdout,"%d  %f\n",j,data->phi[j]);
         }*/

	}
	else if (strncmp (gm->name, F_name[IN_RHO], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_RHO) < 0)
		return (-1);
	    Rho_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Rho_hd->gate, Rho_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->rho)) != 1) {
            Print_unique_msg ("Missing RHO field");
            return (-1);
         }
         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Rho scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Rho_hd->scale, Rho_hd->offset, Rho_hd->no_of_gates, Rho_hd->data_word_size,
            Rho_hd->first_gate_range, *((data->rho)+DEBUG_BIN));
	}
	else if (strncmp (gm->name, F_name[IN_SMZ], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_SMZ) < 0)
		return (-1);
	    Smz_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Smz_hd->gate, Smz_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->smz)) != 1) {
            Print_unique_msg ("Missing SMZ field");
            return (-1);
         }
/*         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Smz scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Smz_hd->scale, Smz_hd->offset, Smz_hd->no_of_gates, Smz_hd->data_word_size,
            Smz_hd->first_gate_range, *((data->smz)+DEBUG_BIN));*/
	}
	else if (strncmp (gm->name, F_name[IN_SMV], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_SMV) < 0)
		return (-1);
	    Smv_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Smv_hd->gate, Smv_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->smv)) != 1) {
            Print_unique_msg ("Missing SMV field");
            return (-1);
         }
/*         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Smv scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Smv_hd->scale, Smv_hd->offset, Smv_hd->no_of_gates, Smv_hd->data_word_size,
            Smv_hd->first_gate_range, *((data->smv)+DEBUG_BIN));*/
	}
	else if (strncmp (gm->name, F_name[IN_SNR], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_SNR) < 0)
		return (-1);
	    Snr_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Snr_hd->gate, Snr_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->snr)) != 1) {
            Print_unique_msg ("Missing SNR field");
            return (-1);
         }
/*         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Snr scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Snr_hd->scale, Snr_hd->offset, Snr_hd->no_of_gates, Snr_hd->data_word_size,
            Snr_hd->first_gate_range, *((data->snr)+DEBUG_BIN));*/
	}
	else if (strncmp (gm->name, F_name[IN_KDP], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_KDP) < 0)
		return (-1);
	    Kdp_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Kdp_hd->gate, Kdp_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->kdp)) != 1) {
            Print_unique_msg ("Missing KDP field");
            return (-1);
         }
         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Kdp scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Kdp_hd->scale, Kdp_hd->offset, Kdp_hd->no_of_gates, Kdp_hd->data_word_size,
            Kdp_hd->first_gate_range, *((data->kdp)+DEBUG_BIN));
/*         if (bh->azimuth == 324.5 || bh->azimuth == 2.5 || bh->azimuth == 193.5){
           fprintf(stdout,"KDP azimuth=%f\n", bh->azimuth);
         for (j=0;j< params->n_dgates; j++)
           fprintf(stdout,"%d  %f\n",j,data->kdp[j]);
         }*/
     }
	else if (strncmp (gm->name, F_name[IN_SDZ], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_SDZ) < 0)
		return (-1);
	    Sdz_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Sdz_hd->gate, Sdz_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->sdz)) != 1) {
            Print_unique_msg ("Missing SDZ field");
            return (-1);
         }
/*         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Sdz scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Sdz_hd->scale, Sdz_hd->offset, Sdz_hd->no_of_gates, Sdz_hd->data_word_size,
            Sdz_hd->first_gate_range, *((data->sdz)+DEBUG_BIN));*/
     }
	else if (strncmp (gm->name, F_name[IN_SDP], 4) == 0) {
	    if (Verify_gm_hd (bh, gm, IN_SDP) < 0)
		return (-1);
	    Sdp_hd = gm;
         if (RPGCS_radar_data_conversion((void*)&Sdp_hd->gate, Sdp_hd,
                                     QIA_NO_DATA, QIA_RF_DATA, &(data->sdp)) != 1) {
            Print_unique_msg ("Missing SDP field");
            return (-1);
         }
/*         if (bh->azi_num == DEBUG_AZ)
          RPGC_log_msg(GL_INFO,
            "Sdp scale:%f offset:%f #gates:%d size:%d first gate rng:%d gate[DEBUG_BIN]:%f",
            Sdp_hd->scale, Sdp_hd->offset, Sdp_hd->no_of_gates, Sdp_hd->data_word_size,
            Sdp_hd->first_gate_range, *((data->sdp)+DEBUG_BIN));*/
	} /* end if */
    } /*end for all generic moments */
    return (0);
} /* end of Get_data_fields() */


/******************************************************************************
     Description:  Prints "msg" if it is not the same as the previous one.
     Input:    msg - message string to be printed.
     Output:   None.
     Returns:  Nothing.
     Notes:    Prints to task's log file.
******************************************************************************/
static void Print_unique_msg (char *msg) {
    static char *prev = NULL;

    if (prev != NULL && strcmp (msg, prev) == 0)
	return;
    if (msg[0] != '\0')
	LE_send_msg (GL_INFO, "%s", msg);
    prev = STR_copy (prev, msg);
} /* end of Print_unique_msg */

/******************************************************************************
     Description:  Verifies if the generic moment header "gm", of field type
                   "field", matches the radial header "bh" in terms of word
                   size.
     Input:    bh - pointer to basedata header.
               gm - pointer to generic moment header.
               field - name of the data field being verified.
     Output:   None.
     Returns:  O is header verifies; -1 if header does not verify.
     Notes:    
******************************************************************************/
static int Verify_gm_hd (Base_data_header *bh, Generic_moment_t *gm, 
						int field) {
    short range_beg;

    range_beg = gm->first_gate_range - gm->bin_size / 2;
    if (/*gm->no_of_gates != bh->n_dop_bins ||
	range_beg != bh->range_beg_dop || */
	 gm->bin_size != 250 || 
	(gm->data_word_size != 8 && gm->data_word_size != 16)) {
	LE_send_msg (GL_ERROR, 
	  "Header of field %s inconsistent with radial header bin size=%d word size = %d",
               F_name[field],gm->bin_size,gm->data_word_size);
	return (-1);
    }
    return (0);
}


/******************************************************************************
     Description:  Formats the output radial.
     Input:    bh - pointer to basedata header.
               params - pointer to structure contianing processing 
                        parameters.  See Qia_params_t.
               data - pointer to the output data fields.  See Qia_data_fields.
     Output:   out_radial - pointer to pointer of formatted output radial.
     Returns:  Size of output radial in bytes.
     Notes:    
******************************************************************************/
static int Format_output_radial (Base_data_header *bh, Qia_params_t *params,
		Qia_data_fields_t *data, char **out_radial) {
    static int buf_size = 0;
    static char *buf = NULL;
    int size1, out_size, z_size, v_size, d_size, phi_size;
    int kdp_size, zdr_size, rho_size;

    Base_data_header *ohd;

    /* size of the first part - basic 3 moments */
    size1 = ALIGNED_SIZE (bh->offsets[0]);

    /* the following field sizes are slightly larger than needed. But it is 
       necessary if n_gates = 0 */
    z_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_zgates);
    v_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_vgates);
    d_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
						params->n_dgates);
    rho_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Rho_hd->data_word_size / 8) * params->n_dgates);
    kdp_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Kdp_hd->data_word_size / 8) * params->n_dgates);
    phi_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Phi_hd->data_word_size / 8) * params->n_dgates);
    zdr_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Zdr_hd->data_word_size / 8) * params->n_dgates);

    /* Twelve 8-bit fields and three 16-bit fields */
    out_size = size1 + (3 * z_size) + v_size + (7 * d_size) +
               rho_size + kdp_size + phi_size + zdr_size;

    if (out_size > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (out_size);
	buf_size = out_size;
    }

    memcpy (buf, (char *)bh, bh->offsets[0]);
    ohd = (Base_data_header *)buf;
    ohd->msg_type |= PREPROCESSED_DUALPOL_TYPE;
    ohd->no_moments = (short)N_QF;

    ohd->offsets[DRHO_IDX] = size1;
    Add_moment (buf + size1, "DRHO", 'd', params, Rho_hd->data_word_size, Rho_hd->tover,
                Rho_hd->SNR_threshold, Rho_hd->scale, Rho_hd->offset, data->rho);
    size1 += rho_size;

    ohd->offsets[DPHI_IDX] = size1;
    Add_moment (buf + size1, "DPHI", 'd', params, Phi_hd->data_word_size, Phi_hd->tover,
                Phi_hd->SNR_threshold, Phi_hd->scale, Phi_hd->offset, data->phi);
    size1 += phi_size;

    ohd->offsets[DZDR_IDX] = size1;
    Add_moment (buf + size1, "DZDR", 'd', params, Zdr_hd->data_word_size, Zdr_hd->tover,
                Zdr_hd->SNR_threshold, Zdr_hd->scale, Zdr_hd->offset, data->zdr);
    size1 += zdr_size;

    ohd->offsets[DSMZ_IDX] = size1;
    Add_moment (buf + size1, "DSMZ", 'z', params, 8, Smz_hd->tover,
                Smz_hd->SNR_threshold, Smz_hd->scale, Smz_hd->offset, data->smz);
    size1 += z_size;

    ohd->offsets[DSNR_IDX] = size1;
    Add_moment (buf + size1, "DSNR", 'z', params, 8, Snr_hd->tover,
                Snr_hd->SNR_threshold, Snr_hd->scale, Snr_hd->offset, data->snr);
    size1 += z_size;

    ohd->offsets[DSMV_IDX] = size1;
    Add_moment (buf + size1, "DSMV", 'v', params, 8, Smv_hd->tover,
                Smv_hd->SNR_threshold, Smv_hd->scale, Smv_hd->offset, data->smv);
    size1 += v_size;

    ohd->offsets[DKDP_IDX] = size1;
    Add_moment (buf + size1, "DKDP", 'd', params, Kdp_hd->data_word_size, Kdp_hd->tover,
                Kdp_hd->SNR_threshold, Kdp_hd->scale, Kdp_hd->offset, data->kdp);
    size1 += kdp_size;

    ohd->offsets[DSDP_IDX] = size1;
    Add_moment (buf + size1, "DSDP", 'd', params, 8, Sdp_hd->tover,
                Sdp_hd->SNR_threshold, Sdp_hd->scale, Sdp_hd->offset, data->sdp);
    size1 += d_size;

    ohd->offsets[DSDZ_IDX] = size1;
    Add_moment (buf + size1, "DSDZ", 'z', params, 8, Sdz_hd->tover,
                Sdz_hd->SNR_threshold, Sdz_hd->scale, Sdz_hd->offset, data->sdz);
    size1 += z_size;

    ohd->offsets[DQSZ_IDX] = size1;
    Add_moment (buf + size1, "DQSZ", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_smz);
    size1 += d_size;

    ohd->offsets[DQZD_IDX] = size1;
    Add_moment (buf + size1, "DQZD", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_zdr);
    size1 += d_size;

    ohd->offsets[DQRO_IDX] = size1;
    Add_moment (buf + size1, "DQRO", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_rho);
    size1 += d_size;

    ohd->offsets[DQKD_IDX] = size1;
    Add_moment (buf + size1, "DQKD", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_kdp);
    size1 += d_size;

    ohd->offsets[DQTZ_IDX] = size1;
    Add_moment (buf + size1, "DQTZ", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_sdz);
    size1 += d_size;

    ohd->offsets[DQTP_IDX] = size1;
    Add_moment (buf + size1, "DQTP", 'd', params, 8, 0,
                0, Q_scale, Q_offset, data->q_sdp);
    size1 += d_size;

/*  Update the message length field which is in units of shorts */
    ohd->msg_len = size1 / sizeof(short);

    *out_radial = buf;
    return (size1);
} /* End of Format_output_radial() */


/******************************************************************************
     Description:  Creates a generic moment field.
     Input:    name - string containing field name.
               f - character flag indicating data type.
               params - pointer to processing parameters.  See Qia_params_t.
               word_size - 8 or 16 bit word size of data.
               tover - RDA threshold value passed through by this algorithm.
               SNR_threshold - Threshold passed through by this algorithm.
               scale - scale factor to be applied to data before output.
               offset - offset to be applied to data before output.
               data - pointer to data to be output in generic moment format.
     Output:   buf - contains scaled and offset generic moment format data.
     Returns:  O is header verifies; -1 if header does not verify.
     Notes:    See RPG Class 1 to User ICD for application of scale and offset.
******************************************************************************/
static void Add_moment (char *buf, char *name, char f, Qia_params_t *params, 
	              int word_size, unsigned short tover, short SNR_threshold, 
	              float scale, float offset, float *data) {
    Generic_moment_t *hd;
    int i, up, low, n_gates;
    unsigned short *spt;
    unsigned char *cpt;
    float dscale, doffset;

    hd = (Generic_moment_t *)buf;
    strcpy (hd->name, name);
    hd->info = 0;
    if (f == 'd') {
	n_gates = params->n_dgates;
	hd->first_gate_range = RPGC_NINT((float)(params->dr0) * KM_TO_M);
	hd->bin_size = RPGC_NINT((float)(params->dg_size) * KM_TO_M);
    }
    else if (f == 'z') {
	n_gates = params->n_zgates;
	hd->first_gate_range = RPGC_NINT((float)(params->zr0) * KM_TO_M);
	hd->bin_size = RPGC_NINT((float)(params->zg_size) * KM_TO_M);
    }
    else {
	n_gates = params->n_vgates;
	hd->first_gate_range = RPGC_NINT((float)(params->vr0) * KM_TO_M);
	hd->bin_size = RPGC_NINT((float)(params->vg_size) * KM_TO_M);
    }

    hd->no_of_gates    = n_gates;
    hd->tover          = tover;
    hd->SNR_threshold  = SNR_threshold;
    hd->control_flag   = 0;
    hd->data_word_size = word_size;
    hd->scale          = scale;
    hd->offset         = offset;

    if (word_size == WORD_8) {
	up = 0xff;
	low = 2;
    }
    else {
	up = 0xffff;
	low = 2;
    }
    spt = hd->gate.u_s;
    cpt = hd->gate.b;
    dscale = scale;
    doffset = offset;
    for (i = 0; i < n_gates; i++) {
	int t;

	if (data[i] == QIA_NO_DATA)
	    t = 0;
        else if (data[i] == QIA_RF_DATA)
            t = 1;
	else {
	    float f;

	    f = data[i] * dscale + doffset;
	    if (f >= C0)
		t = (int)f + Cp5;
	    else
		t = (int)(-(-f + Cp5));

	    if (t > up)
		t = up;
	    if (t < low)
		t = low;
	}
	if (word_size == WORD_8)
	    cpt[i] = (unsigned char)t;
	else
	    spt[i] = (unsigned short)t;
    }
} /* End of Add_moment() */

