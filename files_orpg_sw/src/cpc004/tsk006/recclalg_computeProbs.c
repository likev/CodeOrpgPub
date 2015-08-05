/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/15 21:25:05 $
 * $Id: recclalg_computeProbs.c,v 1.11 2006/06/15 21:25:05 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */
/*  compute_probs.c

  Defines the SCALED CHARACTERISTIC values (SCV) for each radar bin by applying
  the TARGET CATEGORY SCALING Function to the PATTERN CHARACTERISTIC values.
  The SCV for reflectivity bins uses reflectivity PATTERN CHARACTERISTIC values
  and spatially averaged Doppler PATTERN CHARACTERISTIC values.  The SCV for
  Doppler bins uses Doppler PATTERN CHARACTERISTIC values and spatially averaged
  reflectivity PATTERN CHARACTERISTIC values.  Then defines the TARGET LIKELIHOOD
  values for all TARGET Classes by applying TARGET CATEGORY weights to the SCALED
  CHARACTERISTIC values.

  By Tim O'Bannon, NWS WSR-88D ROC
  October, 2001

  Modified December 2001 to include SPIN logic
  Modified January 2002 by Brian Klein for orpg (CODE) implementation

*/

/*** System Include Files ***/
#include <rpgc.h>
#include <rpg_globals.h>
#include <a309.h>
#include <math.h>
#include <recclalg_arrays.h>
#include <recclalg_adapt.h>

/*** Local Include Files ***/
#define EXTERN extern       /* Prevents multiple adaptation data object instantiation */
#include "recclalg_adapt_object.h"
#include "recclalg_constants.h"
#include "recclalg_codedV2V.h"
#include "recclalg_codedW2W.h"


/*  Global variables        */
const static short  debugit = FALSE;  /* controls debug output in this file only */
const static int    MISSING_PAIR_FLAG = -999;
const static float  NEAR_FULL_OVERLAP = 0.1; /* 90% overlap of 1 degree radials  */


extern  float   Ztxtr[MAX_RADIALS][MAX_1KMBINS],
                Zsign[MAX_RADIALS][MAX_1KMBINS],
                Zspin[MAX_RADIALS][MAX_1KMBINS],
                VstDv[MAX_RADIALS][MAX_1KMBINS][MAX_DOP_BIN];
                
extern  int     numDBZGates;
extern  int     numDopGates;
extern  int 	RDA_type;    /* Parameter that holds rda type (0=legacy,1=open) */

float   scaleRngZt,
        scaleRngZs,
        scaleRngZp,
        scaleRngVm,
        scaleRngVs,
        scaleRngWm;

float   avgZtxtr,
        avgZsign,
        avgZspin,
        avgVmean,
        avgVstdv,
        avgWmean;

int ZPair[400], DPair[400];
int DClosest[400];  /*  New array added for ORDA radial mapping for Build 9  */


/* Prototypes */
int     Mesh_radials (void);
void    AP_scale_range(void);
void    AP_lkli_Z (void);
void    AP_lkli_Dop (void);
float   AP_likelihood(float, float, float, float, float, float);
void    Compute_mean_Dop (int, int);
float   AP_scaling_function (float, float, float);
void    Compute_mean_Z (int, int);

void Compute_probs()  {
/** Compute the likelihood that the radar bin belongs to the TARGET CATEGORY.  **/

/*  Determine which radials to use for spatial averaging  */
    if (Mesh_radials() != 0)
        if (debugit) fprintf(stderr,"Mesh_radials: Unable to find matching radial\n");

/*  Compute likelihood of radar bin being AP/Clutter  */
    AP_scale_range();
    if (debugit) fprintf(stderr,"Calling AP_lkli_Z\n");
    if (RPGC_check_data(RECCLDIGREF) == NORMAL) AP_lkli_Z();
    if (debugit) fprintf(stderr,"Calling AP_lkli_Dop\n");
    if (RPGC_check_data(RECCLDIGDOP) == NORMAL) AP_lkli_Dop();
    if (debugit) fprintf(stderr,"Returning to main\n");
}/* end of Compute_probs() */

int Mesh_radials () {
/** Find the Doppler and reflectivity radial pairing.  This is based on the Doppler
    radials surrounding or encompassing the reflectivity radial, i.e., the first
    Doppler radial azimuth angle being less than (CCW of) the second reflectivity
    radial and the second Doppler radial azimuth angle being equal to or greater
    than (CW of) the second reflectivity radial.  For each reflectivity radial this
    module determines the Doppler radial immediately CCW.  Same for each Doppler radial.
    The paired radial information is used to spatially average the PATTERN CHARACTESTIC
    values.  **/

    int		radial,
	        Zradial,
	        Dradial;
    int         DNextRadial,
                ZNextRadial;
    float	lZaz,
        	rZaz,
        	lDaz,
        	rDaz,
                az_diff,left_az_diff, rt_az_diff;

     int start_radial;  /*  First radial to be processed;  Added for Build 9 */
     int end_Zradial;   /*  Last Refl radial to be processed; Added for Build 9 */
     int end_Dradial;   /*  Last Dopp radial to be processed; Added for Build 9 */

/*  Initialize pair arrays  */
    for (radial = 0; radial <= (MAX_RADIALS-1); radial++) {
        ZPair[radial] = MISSING_PAIR_FLAG;
        DPair[radial] = MISSING_PAIR_FLAG;
        DClosest[radial] = MISSING_PAIR_FLAG;
    }

/*  For ORDA use the boundary radials, these have valid information;
    else use the 2nd and penultimate radials for legacy RDA.  Logic added
    for Build 9 to test for rda type.                                       */

    if (RDA_type == ORPGRDA_LEGACY_CONFIG){
      start_radial = 1;
      end_Zradial = Z_array.pHeader.num_radials - 1;
      end_Dradial = D_array.pHeader.num_radials - 1;
    }
    else{
      start_radial = 0;
      end_Zradial = Z_array.pHeader.num_radials;
      end_Dradial = D_array.pHeader.num_radials;
    }      

    for (Zradial = start_radial; Zradial < end_Zradial; Zradial++) {

        lZaz = Z_array.rHeader[Zradial].azimuth;

        for (Dradial = start_radial; Dradial < end_Dradial; Dradial++) {

            lDaz = D_array.rHeader[Dradial].azimuth;
            DNextRadial = Dradial+1;
            if (DNextRadial == D_array.pHeader.num_radials) {
                DNextRadial = 0;
            }
            
            rDaz = D_array.rHeader[DNextRadial].azimuth;

            if (lDaz > rDaz)
                rDaz = rDaz + 360.0;    /* Correct for zero crossing  */

            if (((lDaz <= lZaz) && (rDaz > lZaz)) 
                                ||
                ((lDaz <= (lZaz + 360.0)) && (rDaz > (lZaz + 360.0)))) {

                DPair[Zradial] = Dradial;

                /* Check for near overlap. */
                left_az_diff = lZaz - lDaz;
                if( left_az_diff < 0.0 ) left_az_diff += 360.0;

	        rt_az_diff = rDaz - lZaz;
                if( rt_az_diff > 180. ) rt_az_diff -= 360.0;

                if( left_az_diff <= NEAR_FULL_OVERLAP ) {
                   DPair[Zradial] = -Dradial;
                }

                else{
                   if( rt_az_diff <= NEAR_FULL_OVERLAP ) {
                      DPair[Zradial] = -(DNextRadial);
                   }

                }
		if (left_az_diff < rt_az_diff) {
		    DClosest[Zradial] = Dradial;
		} else{
		    DClosest[Zradial] = DNextRadial;
		}
                break;      /*  Found a pair, don't need to process any further */

            }
        }
    }

    for (Dradial = start_radial; Dradial < end_Dradial; Dradial++) {

        lDaz = D_array.rHeader[Dradial].azimuth;

        for (Zradial = start_radial; Zradial < end_Zradial; Zradial++) {

            lZaz = Z_array.rHeader[Zradial].azimuth;
            ZNextRadial = Zradial+1;

            if (ZNextRadial == Z_array.pHeader.num_radials) {
                ZNextRadial = 0;
            }
            
            rZaz = Z_array.rHeader[ZNextRadial].azimuth;

            if (lZaz > rZaz)
                rZaz = rZaz + 360.0;    /* Correct for zero crossing  */
 
            if (((lZaz <=  lDaz) &&  (rZaz >  lDaz))
                                ||
                ((lZaz <=  (lDaz + 360.0)) &&  (rZaz >  (lDaz + 360.0)))) {

                ZPair[Dradial] = Zradial;

                /* Check for near azimuth overlap. */
                az_diff = lDaz - lZaz;
                if( az_diff < 0. ) az_diff += 360.0;

                if( az_diff <= NEAR_FULL_OVERLAP )
                   ZPair[Dradial] = -Zradial;

                else{

                   az_diff = rZaz - lDaz;
                   if( az_diff > 180. ) az_diff -= 360.0;

                   if( az_diff <= NEAR_FULL_OVERLAP )
                      ZPair[Dradial] = -(ZNextRadial);
                }
                break;      /*  Found a pair, don't need to process any further */
            }
        }
    }
    return(0);
}/* end of Mesh_radials() */

void    AP_scale_range() {
/** Compute the range of values for the TARGET CATEGORY SCALING FUNCTION for each AP/clutter
    PATTERN CHARACTERISTIC.  **/

    if (debugit) fprintf(stderr,"In AP_scale_range\n");
    scaleRngZt = adapt_cl_target.Ztxtr1 - adapt_cl_target.Ztxtr0,
    scaleRngZs = adapt_cl_target.Zsign1 - adapt_cl_target.Zsign0,
    scaleRngZp = adapt_cl_target.Zspin1 - adapt_cl_target.Zspin0,
    scaleRngVm = adapt_cl_target.Vmean1 - adapt_cl_target.Vmean0,
    scaleRngVs = adapt_cl_target.Vstdv1 - adapt_cl_target.Vstdv0,
    scaleRngWm = adapt_cl_target.Wmean1 - adapt_cl_target.Wmean0;
    if (debugit) fprintf(stderr,"Zt=%7.2f, Zs=%7.2f, Zp=%7.2f, Vm=%7.2f, Vs=%7.2f, Wm=%7.2f\n",
            scaleRngZt, scaleRngZs, scaleRngZp, scaleRngVm, scaleRngVs, scaleRngWm);
}/* end of AP_scale_range */



void AP_lkli_Z() {
/**  Compute the AP/Clutter TARGET LIKELIHOOD value for each reflectivity bin.  **/

    int radial;
    int range;

    float likelihood;
    
    for (radial = 0; radial <= (MAX_RADIALS-1); radial++) {

        for (range = 0; range < numDBZGates; range ++) {

            if(Z_array.Z_data[radial][range] <= REC_RANGE_FOLDED_CODE || Ztxtr[radial][range] <= REC_NO_DATA_FLAG)
                Z_array.Z_clut[radial][range] = REC_BELOW_SNR_CODE;     
            else
            {

                /*  Spatially average the Doppler PATTERN CHARACTERISTIC value.  */
                Compute_mean_Dop(radial, range);

                likelihood = AP_likelihood(Ztxtr[radial][range],
                           Zsign[radial][range],
                           Zspin[radial][range],
                           avgVmean,
                           avgVstdv,
                           avgWmean);

                if(likelihood < 0.0)
                    /*  Set array value to missing  */
                    Z_array.Z_clut[radial][range] = REC_BELOW_SNR_CODE;
                else
                    /*  Store clutter likelihood internally as a short integer noting the percent 
                        likelihood plus 2 (values between 2 and 102 represent likelihoods between 
                        0 and 100 percent).   Note, coded WSR-88D base data reserves the number 0 
                        for missing and 1 for rangefolded.  */
                    Z_array.Z_clut[radial][range] = ((short) RPGC_NINT(likelihood * 100.0)) + 2;

            }/* end if */
        }/* end for range */                           

    }/* end for radial */
}/* end of AP_likli_Z() */



void AP_lkli_Dop() {
/**  Compute the AP/Clutter TARGET LIKELIHOOD value for each Doppler bin.  **/

    int radial,
        range,
        bin,
        Vm,         /* Doppler velocity value (coded)   */
        Wm;         /* Spectrum width value (coded)     */

    float   floatV,         /* Doppler velocity value (m/s)     */
            floatW,         /* Spectrum width value (m/s)       */
            likelihood;

    for (radial = 0; radial <= (MAX_RADIALS-1); radial++)
    {
        for (range = 0; range < numDopGates/4; range++)
        {

            /*  Spatially average the reflectivity PATTERN CHARACTERISTIC value.  */
            Compute_mean_Z(radial, range);

            for (bin = 0; bin <= 3; bin++)
            {
                
                Vm = D_array.V_data[radial][range][bin];
                
                /* Set nodata and range folded codes as needed */
                if (Vm == REC_RANGE_FOLDED_CODE)
                    D_array.D_clut[radial][range][bin] = REC_RANGE_FOLDED_CODE;
                    
                else if ((Vm == REC_BELOW_SNR_CODE) || (VstDv[radial][range][bin] == REC_NO_DATA_FLAG))
                    D_array.D_clut[radial][range][bin] = REC_BELOW_SNR_CODE;
                    
                else
                {
                    floatV = codedV2V (Vm);
                    Wm = D_array.W_data[radial][range][bin];
                    floatW = codedW2W (Wm);

/*                  if (debugit) printf("\nIn AP_lkli_Dop: txt %f, sign %f, Vm %f, Vs %f, Wm %f",
                        avgZtxtr,
                        avgZsign,
                        avgZspin,
                        floatV,
                        VstDv[radial][range][bin],
                        floatW);  */

                    likelihood = AP_likelihood(avgZtxtr,
                                   avgZsign,
                                   avgZspin,
                                   floatV,
                                   VstDv[radial][range][bin],
                                   floatW);
/*                  if (debugit) printf("\nIn AP_lkli_Dop: range %d, likelihood %f",
                        range, likelihood);  */

                    if(likelihood < 0.0)
                        /*  Set array value to missing  */
                        D_array.D_clut[radial][range][bin] = REC_BELOW_SNR_CODE;
                    else
                        /*  Store clutter likelihood internally as a short integer noting the percent 
                            likelihood plus 2 (values between 2 and 102 represent likelihoods between 
                            0 and 100 percent).   Note, coded WSR-88D base data reserves the number 0 
                            for missing and 1 for rangefolded.  */
                        D_array.D_clut[radial][range][bin] = ((short) RPGC_NINT(likelihood * 100.0)) + 2;    

                }/* end if */
            }/* end for bins */
        }/* end for range */
    }/* end for radials */
}/* end of AP_likli_Dop() */



float AP_likelihood (float Zt, float Zs, float Zp, float Vm, float Vs, float Wm)  {
/** Define the AP/Clutter TARGET LIKELIHOOD.  First step is to compute the TARGET WEIGHTED
    SCALED CHARACTERISTIC value (SCV) by applying the AP/clutter TARGET CATEGORY SCALING
    function and the AP/Clutter TARGET CATEGORY WEIGHTS.  Then sum the weighted SCVs to and
    normalize to generate a the TARGET LIKELIHOOD value between 0 and 1.  **/

    float   scaledZtxtr,
            scaledZsign,
            scaledZspin,
            scaledVmean,
            scaledVstdv,
            scaledWmean,
            sumWts,
            likelihood;

    sumWts = likelihood = 0.0;

    /*  Compute the TARGET WEIGHTED SCV  */
    if (Zt <= REC_NO_DATA_FLAG) {

        scaledZtxtr = 0.0;
        scaledZsign = 0.0;
        scaledZspin = 0.0;
    }
    else {

        scaledZtxtr = AP_scaling_function(Zt, scaleRngZt, adapt_cl_target.Ztxtr0);
        scaledZsign = AP_scaling_function(fabs((double)Zs), scaleRngZs, adapt_cl_target.Zsign0); 
        sumWts = adapt_cl_target.ZtxtrWt + adapt_cl_target.ZsignWt;

        /* It is possible for Zp to be missing yet Zt is valid. */
        if( Zp > REC_NO_DATA_FLAG ){

           scaledZspin = AP_scaling_function(fabs((double)(Zp - 50.)), scaleRngZp, adapt_cl_target.Zspin0);
           sumWts = sumWts + adapt_cl_target.ZspinWt;

        }
        else
           scaledZspin = 0.0;
    }

    if (Vs <= REC_NO_DATA_FLAG) {

	scaledVmean = 0.0;
	scaledVstdv = 0.0;
	scaledWmean = 0.0;
    }
    else {

	scaledVmean = AP_scaling_function(fabs((double)Vm), scaleRngVm, adapt_cl_target.Vmean0); /* use absolute value of velocity */
    	scaledVstdv = AP_scaling_function(Vs, scaleRngVs, adapt_cl_target.Vstdv0);
    	scaledWmean = AP_scaling_function(Wm, scaleRngWm, adapt_cl_target.Wmean0);
        sumWts = sumWts + adapt_cl_target.VmeanWt + adapt_cl_target.VstdvWt + adapt_cl_target.WmeanWt;
    }

    /* Sum the WEIGHTED SCV  */
    likelihood =  adapt_cl_target.ZtxtrWt * scaledZtxtr
               +  adapt_cl_target.ZsignWt * scaledZsign
               +  adapt_cl_target.ZspinWt * scaledZspin
               +  adapt_cl_target.VmeanWt * scaledVmean
               +  adapt_cl_target.VstdvWt * scaledVstdv
               +  adapt_cl_target.WmeanWt * scaledWmean;

    /* Normalize the TARGET LIKELIHOOD based on TARGET CATEGORY WEIGHTS  */
    if (sumWts > 0.0){

        likelihood = likelihood / sumWts;
 
 	/* Ensure the returned value is valid */
        if (likelihood < 0.0){

	    fprintf(stderr,"In AP_likelihood: **ERROR** underflow\n");
 	    likelihood = REC_NO_DATA_FLAG;
	}

 	if (likelihood > 1.0){

	    fprintf(stderr,"In AP_likelihood: **ERROR** overflow\n");
 	    likelihood = 1.0;
	}
       
    }
    else
        likelihood = REC_NO_DATA_FLAG;
        
    return (likelihood);
}/* end of AP_likelihood() */


float AP_scaling_function (float value, float range, float zero_level) {
/** Compute the AP/Clutter SCALED CHARACTERISTIC value.  Note, this function is
    equivalent to the AP/Clutter TARGET CATEGORY SCALING FUNCTION described in
    Appendix B of the AEL.
 **/
    float   scale;
    
    scale = 0.0;
    
    if (fabs((double)range) > 0.01)
        scale = (value - zero_level) / range;
    else
    {
        fprintf(stderr,"***ERROR*** division by zero/overflow potential in AP_scaling_function\n");
        return (0.0);
    }

/*      Constrain output to range from 0 to 1  */
    if (scale < 0.0)
        scale = 0.0;
    else if (scale > 1.0)
        scale = 1.0;

    return (scale);
} /* end of AP_scaling_function() */


void Compute_mean_Z (int DRadial, int range) {
/** Compute the SPATIALLY AVERAGED reflectivity PATTERN CHARACTERISTIC values (Ztxtr and Zsign) for the
    radials bounding the Doppler radial.  **/

    int     counter,
            Zspin_counter,
            num_pairs,
            pair,
            ZRadial;

    float   floatCounter,
            sumZtxtr,
            sumZsign,
            sumZspin;


    sumZtxtr = sumZsign = sumZspin = 0.0;
    counter = Zspin_counter = 0;

    /* If ZPair is not defined, set the average Z texture to the
       "No Data" flag and return. */
    if( ZPair[DRadial] == MISSING_PAIR_FLAG ){

        avgZtxtr = REC_NO_DATA_FLAG;
	avgZsign = REC_NO_DATA_FLAG;
        avgZspin = REC_NO_DATA_FLAG;  
        return;

    }

    if (ZPair[DRadial] < 0){

       /* This indicates only one paired radial is needed because the overlap */
       /* is so great.                                                        */
       ZPair[DRadial] = -ZPair[DRadial];
       num_pairs      = 1;
    }
    else {

       /* Use two paired radials.                                             */
       num_pairs      = 2;
    }

    for (pair = 0; pair < num_pairs; pair++) {

        ZRadial = ZPair[DRadial] + pair;
        if (Ztxtr[ZRadial][range] > REC_NO_DATA_FLAG) {

            sumZtxtr = sumZtxtr + Ztxtr[ZRadial][range];
            sumZsign = sumZsign + Zsign[ZRadial][range];
            counter++;
            
        }

        if( Zspin[ZRadial][range] > REC_NO_DATA_FLAG ){

            sumZspin = sumZspin + Zspin[ZRadial][range];
            Zspin_counter++;

        }

    }

    if (counter <= 0){

        avgZtxtr = REC_NO_DATA_FLAG;  
	avgZsign = REC_NO_DATA_FLAG;

    }
    else{

        floatCounter = counter;
        avgZtxtr = sumZtxtr / floatCounter;
        avgZsign = sumZsign / floatCounter;

    }

    if( Zspin_counter > 0 )
        avgZspin = sumZspin / (float) Zspin_counter;

    else
        avgZspin = REC_NO_DATA_FLAG;  

}/* end of Compute_mean_Z */


void Compute_mean_Dop (int ZRadial, int range) {
/** Compute the SPATIALLY AVERAGED Doppler PATTERN CHARACTERISTIC values (Vmean, Vstdv, and Wmean)
    for the radials bounding the reflectivity radial.  **/

    int     counter,
        num_pairs,
        pair,
        bin,
        DRadial;

    float  floatCounter,
        realV,
        realW,
        sumVm,
        sumVs,
        sumWm;

    sumVm = 0.0;
    sumVs = 0.0;
    sumWm = 0.0;
    counter = 0;

    /* If DPair is not defined, set the average Vstdv to the
       "No Data" flag and return. */
    if( DPair[ZRadial] == MISSING_PAIR_FLAG ){

        avgVstdv = REC_NO_DATA_FLAG;  
        avgVmean = REC_NO_DATA_FLAG;
        avgWmean = REC_NO_DATA_FLAG;
        return;

    }

    if (DPair[ZRadial] < 0){

/*     This indicates only one paired radial is needed because the overlap */
/*     is so great.                                                        */

       DPair[ZRadial] = -DPair[ZRadial];
       num_pairs      = 1;
    }
    else {

/*     Use two paired radials.                                             */

       num_pairs      = 2;
    }

    for (pair = 0; pair < num_pairs; pair++) {

        DRadial = DPair[ZRadial] + pair;
/***        if (debugit) fprintf(stderr,"In Compute_mean_Dop: ZRadial %d, DRadial %d\n", ZRadial, DRadial);***/
        for (bin = 0; bin <= 3; bin ++) {

            if( (D_array.V_data[DRadial][range][bin] > REC_RANGE_FOLDED_CODE)
                                          &&
                (VstDv[DRadial][range][bin] > REC_NO_DATA_FLAG) ){

                realV = codedV2V((int)D_array.V_data[DRadial][range][bin]);
                sumVm = sumVm + realV;

                sumVs = sumVs + VstDv[DRadial][range][bin];

                realW = codedW2W((int)D_array.W_data[DRadial][range][bin]);
                sumWm = sumWm + realW;

                counter++;
            }
        }
    }

    if (counter <= 0){

        avgVstdv = REC_NO_DATA_FLAG; 
        avgVmean = REC_NO_DATA_FLAG; 
        avgWmean = REC_NO_DATA_FLAG; 

    }
    else{

        floatCounter = counter;
        avgVmean = sumVm / floatCounter;
        avgVstdv = sumVs / floatCounter;
        avgWmean = sumWm / floatCounter;
    }
}/* end of Compute_mean_Dop() */
