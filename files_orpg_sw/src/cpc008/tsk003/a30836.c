/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2006/12/18 21:52:59 $ */
/* $Id: a30836.c,v 1.2 2006/12/18 21:52:59 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <combattr.h>

/* Function Prototypes. */
static float El_az_ran( float range, float height );

/**************************************************************************

   Description:
      This subroutine packs the Combined Attributes data for a storm 
      cell into the apropriate positions of the output buffer. It 
      receives as input the index to the storm cell in question 
      (SIDX), the information regarding the most severe kinematic 
      phenomenon associated with that storm  (i.e. TVS, Meso)
      and assorted storm attribute information, acquired 
      and the SCIT and Hail algorithms.  It outputs a Feature table 
      (i.e., CAT_FEAT) of severe phenomena associated with each 
      cell, anttributes table (i.e. COMB_ATT) of additional storm 
      cell descriptors, a Forecast table (i.e. FORCST_POSITS) of 
      predicted storm cell positions, and a Header containing 
      information such as the number of storm cells in the current 
      volume scan, and the number of forecast positions per storm 
      cell. 

   Inputs:
      sidx - input index for current storm.
      cat_idx - category index.
      stormain - 2D array of storm attributes.
      ntotpred - total number of storms this scan.
      stormidtyp - table of storm ID-indices and types.
      stormotion - table of storm motion data.
      stormforw - table of storm forecasted positions.
      forcadap - forecasting algorithm adaptation data.
      hailabel - table of hail labels.
      hailstats - 2D array of hail evaluation statistics with
                  the storm cells sorted by cell-based VIL.
     
   Outputs:
      cat_num_storms - number of storms processed for the CAT.
      cat_feat - feature table of severe phenomenon associated with
                 each storm cell.
      comb_att - table of combined attributes.
      forcst_posits - table of storm forecasted positions.

   Returns:
      Currently always returns 0.

**************************************************************************/
int A30836_pack_storm( int sidx, int cat_idx, float stormain[][NSTM_CHR], 
                       int ntotpred, int stormidtyp[][NSTF_IDT], 
                       float stormotion[][NSTF_MOT], float stormforw[][NSTF_INT][NSTF_FOR], 
                       int *forcadap, int *cat_num_storms, int cat_feat[][CAT_NF], 
                       float comb_att[][CAT_DAT], int *num_fposits, 
                       float forcst_posits[][MAX_FPOSITS][FOR_DAT], 
                       int *hailabel, float hailstats[][NHAL_STS] ){

    /* Initialized data */
    static char *charidtable[260] = { "A0", "B0", "C0", "D0", "E0", 
                                      "F0", "G0", "H0", "I0", "J0",
                                      "K0", "L0", "M0", "N0", "O0",
                                      "P0", "Q0", "R0", "S0", "T0", 
                                      "U0", "V0", "W0", "X0", "Y0", 
                                      "Z0", "A1", "B1", "C1", "D1",
                                      "E1", "F1", "G1", "H1", "I1",
                                      "J1", "K1", "L1", "M1", "N1", 
                                      "O1", "P1", "Q1", "R1", "S1", 
                                      "T1", "U1", "V1", "W1", "X1", 
                                      "Y1", "Z1", "A2", "B2", "C2", 
                                      "D2", "E2", "F2", "G2", "H2", 
	                              "I2", "J2", "K2", "L2", "M2", 
                                      "N2", "O2", "P2", "Q2", "R2", 
                                      "S2", "T2", "U2", "V2", "W2", 
                                      "X2", "Y2", "Z2", "A3", "B3", 
                                      "C3", "D3", "E3", "F3", "G3", 
                                      "H3", "I3", "J3", "K3", "L3", 
                                      "M3", "N3", "O3", "P3", "Q3", 
                                      "R3", "S3", "T3", "U3", "V3", 
                                      "W3", "X3", "Y3", "Z3", "A4", 
                                      "B4", "C4", "D4", "E4", "F4", 
                                      "G4", "H4", "I4", "J4", "K4", 
                                      "L4", "M4", "N4", "O4", "P4", 
                                      "Q4", "R4", "S4", "T4", "U4", 
	                              "V4", "W4", "X4", "Y4", "Z4", 
                                      "A5", "B5", "C5", "D5", "E5", 
                                      "F5", "G5", "H5", "I5", "J5", 
                                      "K5", "L5", "M5", "N5", "O5", 
                                      "P5", "Q5", "R5", "S5", "T5", 
                                      "U5", "V5", "W5", "X5", "Y5", 
                                      "Z5", "A6", "B6", "C6", "D6", 
                                      "E6", "F6", "G6", "H6", "I6",
                                      "J6", "K6", "L6", "M6", "N6", 
                                      "O6", "P6", "Q6", "R6", "S6", 
                                      "T6", "U6", "V6", "W6", "X6", 
                                      "Y6", "Z6", "A7", "B7", "C7", 
                                      "D7", "E7", "F7", "G7", "H7", 
	                              "I7", "J7", "K7", "L7", "M7", 
                                      "N7", "O7", "P7", "Q7", "R7", 
                                      "S7", "T7", "U7", "V7", "W7", 
                                      "X7", "Y7", "Z7", "A8", "B8", 
                                      "C8", "D8", "E8", "F8", "G8", 
                                      "H8", "I8", "J8", "K8", "L8", 
                                      "M8", "N8", "O8", "P8", "Q8", 
                                      "R8", "S8", "T8", "U8", "V8", 
                                      "W8", "X8", "Y8", "Z8", "A9", 
                                      "B9", "C9", "D9", "E9", "F9", 
                                      "G9", "H9", "I9", "J9", "K9", 
                                      "L9", "M9", "N9", "O9", "P9", 
                                      "Q9", "R9", "S9", "T9", "U9", 
	                              "V9", "W9", "X9", "Y9", "Z9" };

    /* Local variables */
    int i, j, top;
    float elev_ang_deg;
    int charix;

    /* STORE NUMBER OF STORMS. */
    *cat_num_storms = ntotpred;

    /* BUILD CAT_FEATURES BUFFER.  NOTE:  IF AND WHEN THE IDS BECOME
       0 INDEXED, THE FOLLOWING LINE OF CODE WILL NEED TO REMOVE "-1". */
    charix = stormidtyp[sidx][STF_ID] - 1;
    memcpy( &cat_feat[cat_idx][CAT_SID], charidtable[charix], sizeof(int) );

    /* STORE THE STORM CELL TYPE ("NEW" OR "CONTINUING") */
    cat_feat[cat_idx][CAT_TYPE] = stormidtyp[sidx][STF_TYP];

    /* STORE TVS DATA, IF ANY. */
    cat_feat[cat_idx][CAT_TVS] = Storm_feats[sidx][CAT_TVS_TYPE];

    /* STORE MDA DATA */
    cat_feat[cat_idx][CAT_MDA] = Storm_feats[sidx][CAT_MDA_TYPE];

    /* STORE HAIL DATA. */

    /* HAIL LABEL. */
    cat_feat[cat_idx][CAT_HAIL] = hailabel[sidx];

    /* PROBABILITY OF HAIL, PROBABILITY OF SEVERE HAIL, MAXIMUM 
       EXPECTED HAIL SIZE.  HAIL SIZE IS SCALED TO UNITS OF 0.25 
       INCHES. */
    cat_feat[cat_idx][CAT_POH] = (int) hailstats[sidx][H_POH];
    cat_feat[cat_idx][CAT_POSH] = (int) hailstats[sidx][H_PSH];
    cat_feat[cat_idx][CAT_MEHS] = (int) (hailstats[sidx][H_MHS] * 4.f);

    /* BUILD COMBINED_ATTRIBUTES BUFFER. */

    /* DETERMINE ELEVATION ANGLE OF STORM CELL CENTROID FROM ITS 
       ITS HEIGHT AND RANGE. */
    elev_ang_deg = El_az_ran( stormain[sidx][STM_RAN], stormain[sidx][STM_ZCN] );

    /* STORE AZIMUTH, RANGE, HEIGHT & ELEVATION ANGLE OF STORM CELL 
       CENTROID */
    comb_att[cat_idx][CAT_AZ] = stormain[sidx][STM_AZM];
    comb_att[cat_idx][CAT_RNG] = stormain[sidx][STM_RAN];
    comb_att[cat_idx][CAT_HCN] = stormain[sidx][STM_ZCN];
    comb_att[cat_idx][CAT_ELCN] = elev_ang_deg;

    /* DETERMINE ELEVATION OF CENTER OF MAX REFLECTIVITY. */
    elev_ang_deg = El_az_ran( stormain[sidx][STM_RAN], stormain[sidx][STM_RFH] );

    /* STORE CELL-BASED VIL, THE CELLS MAXIMUM REFLECTIVITY, ELEVATION 
       AND HEIGHT OF OCCURRENCE. */
    comb_att[cat_idx][CAT_VIL] = stormain[sidx][STM_VIL];
    comb_att[cat_idx][CAT_MXZ] = stormain[sidx][STM_MRF];
    comb_att[cat_idx][CAT_HMXZ] = stormain[sidx][STM_RFH];
    comb_att[cat_idx][CAT_ELVXZ] = elev_ang_deg;

    /* STORE HEIGHT OF STORM CELL BASE */
    comb_att[cat_idx][CAT_SBS] = stormain[sidx][STM_BAS];

    /* RETRIEVE COMPONENT WHICH IS THE STORM CELL TOP */
    top = stormain[sidx][STM_LCT];

    /* IF INDEX IS NEGATIVE, THE STORM TOP WAS LOCATED ON THE HIGHEST 
       ELEVATION CUT. */
    if( (float) top < 0.f ) 
	comb_att[cat_idx][CAT_STP] = -stormain[sidx][STM_TOP];
    else
	comb_att[cat_idx][CAT_STP] = stormain[sidx][STM_TOP];
    

    /* STORE STORM'S FORECAST DIRECTION (DEGREES FROM) & SPEED (M/SEC). */
    comb_att[cat_idx][CAT_FDIR] = abs(stormotion[sidx][STF_DIR] );
    comb_att[cat_idx][CAT_FSPD] = abs(stormotion[sidx][STF_SPD]);

    /* STORE THE AZIMUTH, ELEVATION INDEX, RANGE & STRENGTH RANK OF AN 
       MDA FEATURE ASSOCIATED WITH THE STORM CELL (IF ANY). */
    comb_att[cat_idx][CAT_AZMDA] = Basechars[sidx][MDAB_AZ];
    comb_att[cat_idx][CAT_RNMDA] = Basechars[sidx][MDAB_RN];
    comb_att[cat_idx][CAT_ELMDA] = Basechars[sidx][MDAB_EL];
    comb_att[cat_idx][CAT_SRMDA] = Basechars[sidx][MDA_SR];

    /* STORE THE AZIMUTH, ELEVATION & RANGE OF THE BASE OF A TVS 
       ASSOCIATED WITH THE STORM (IF ANY). */
    comb_att[cat_idx][CAT_AZTVS] = Basechars[sidx][TVSB_AZ]; 
    comb_att[cat_idx][CAT_RNTVS] = Basechars[sidx][TVSB_RN];
    comb_att[cat_idx][CAT_ELTVS] = Basechars[sidx][TVSB_EL];

    /* BUILD FORECAST_POSITIONS BUFFER. */
    *num_fposits = i = forcadap[STA_NFOR];
    for( j = 0; j < i; ++j ){

	forcst_posits[cat_idx][j][CAT_FX] = stormforw[sidx][j][STF_XF];
	forcst_posits[cat_idx][j][CAT_FY] = stormforw[sidx][j][STF_YF];

    }

    /* RETURN TO CALLER ROUTINE */
    return 0;

} 


#define INREF			1.21 
#define EARTH_RADIUS		6371.0 
#define RADFACT 		57.29578
#define TWOAE 			(2.0*INREF*EARTH_RADIUS)
#define ELEVMAX			90.0


/*********************************************************************
   Description:
      This subroutine computes the Elevation angle of a point when 
      given the {RNG, HGT} coordinates of that point. 

   Inputs:
      range - slant range of phenomenon.
      height - height of phenomenon.

   Returns:
      Elevation angle.

*********************************************************************/
static float El_az_ran( float range, float height ){

    /* Local variables */
    float sinelev, elevation;

    /* DETERMINE ELEVATION ANGLE, AND CONVERT TO DEGREES: */
    if( range != 0.f ){

	sinelev = height / range - range / TWOAE;

        /* LIMIT THE SINELEV VALUE THE INTERVAL [0.0, 1.0] */
	if( sinelev < 0.f )
	    sinelev = 0.f;

	else if (sinelev > 1.f) 
	    sinelev = 1.f;
	 
        /* NOW CALCULATE THE ELEVATION ANGLE (DEGREES) */
	elevation = asin(sinelev) * RADFACT;

    }
    else{

        /* ACCOUNT FOR SPECIAL CASE: */
	elevation = ELEVMAX;

    }

    /* RETURN TO CALLER */
    return elevation;

}

