/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:47:58 $
 * $Id: recclalg_classifyEcho.c,v 1.12 2014/11/07 21:47:58 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/*  classify_echo.c

  Computes the PATTERN CHARACTERISTIC values for the reflectivity
  and Doppler data, then calls compute_probs to generate the bin
  by bin TARGET LIKELIHOOD values.

  Includes Princeton University "SPIN" function

  By Tim O'Bannon, NWS WSR-88D ROC
  December, 2001
  
  Modified January 2002 by Brian Klein for orpg (CODE) implementation

*/

/*** System Include Files ***/
#include <rpg_globals.h> 
#include <a309.h>
#include <math.h>
#include <recclalg_arrays.h>
#include <recclalg_adapt.h>

#include <assert.h>

/*** Local Include Files ***/
#define EXTERN extern       /* Prevents multiple adaptation data object instantiation */
#include "recclalg_adapt_object.h"
#include "recclalg_constants.h" 


/*  Global variables        */
const static short debugit = FALSE;  /* controls debug output in this file only */
const static float ZRES = 2.0;       /* Convert from coded reflectivity value to dBZ  */

float   Ztxtr[MAX_RADIALS][MAX_1KMBINS];
float   Zsign[MAX_RADIALS][MAX_1KMBINS];
float   Zspin[MAX_RADIALS][MAX_1KMBINS];
float   VstDv[MAX_RADIALS][MAX_1KMBINS][MAX_DOP_BIN];

extern float  DopplerRes;
extern int     numDBZGates;
extern int     numDopGates;
extern int     RDA_type;	    /* Variable that shows type of RDA (0=legacy, 1=open) 
			       Added for Build 9                                  */

#define MAX_1KMRANGE      (MAX_1KMBINS-1)
static int     codedV[MAX_RADIALS][MAX_QTRKMBINS];  /* Coded velocity.  Has values 0 to 255. */
static float   Vsqr[MAX_RADIALS][MAX_QTRKMBINS];    /* Coded velocity squared.  Has values 0 to 255^2. */
static float   dBZdifSq[MAX_RADIALS][MAX_1KMRANGE]; /* Difference squared array */
static int     sign[MAX_RADIALS][MAX_1KMRANGE];     /* Sign of difference.  Has values -1, 0, or 1. */
static int     spinFlag[MAX_RADIALS][MAX_1KMRANGE]; /* Flag to count SPIN difference.  Has values 0, 1, or 2 */


/*** Function Prototypes ***/
void Compute_Z_texture(void);
void Compute_V_stddev(void);
extern void Compute_probs(void);
static float StdDev(int radial, int Vrange);
static float MeandBZdifSq(int, int);


void Classify_echo() {
/** Classifies the echoes based on TARGET CLASS.
 **/

/*  Build the PATTERN CHARACTERISTIC values used to classify the radar echo. */
    Compute_Z_texture();
    Compute_V_stddev();

/*  Compute the TARGET LIKELIHOOD values  */
    Compute_probs();
}/* end classify_echo() */



void Compute_Z_texture() {
/** Generates the reflectivity PATTERN CHARACTERISTIC values, including the texture
    of reflectivity (Ztxtr) and mean sign of reflectivity change (Zsign) for each radar
    bin with valid reflectivity data.

    Ztxtr is defined as the mean of the differences in reflectivity squared in
    the surrounding bins.  AP/clutter is normally associated with high values
    of Ztxtr, however high Ztxtr values may also indicate strong thunderstorm
    reflectivity gradients.

    Zsign is defined as the average sign of reflectivity change in the surrounding bins.
    Zsign is intended to help discriminate between the irregular/random reflectivity
    distributions in clutter (absolute value of Zsign low) and the strongly increasing
    or decreasing reflectivities in thunderstorms (absolute value of Zsign high).

    Zspin is defined as the average percentage of reflectivity differences that exceed
    a threshold difference.
 **/

    float dBZdif;     /* Reflectivity difference  */
    int start_radial;  /*  First radial to be processed;  Added for Build 9 */
    int end_radial;    /*  Last radial to be processed;   Added for Build 9 */

    int     radial,
            range,      /* Range bin  */
            Zp0,        /* Coded reflectivity at current range  */
            Zp1;        /* Coded reflectivity at current range + 1 bin  */

/*  Radial computations.  Note, uses the radial dBZ difference between bins.  This software
    assigns the difference between bin i and bin i+1 to bin i.  Therefore no value can be
    assigned to the last range bin (229).  */
    for (radial = 0; radial < MAX_RADIALS; radial++)
    {

        for (range = 0; range < numDBZGates - 1; range++)
        {
            Zp0 = Z_array.Z_data[radial][range];
            Zp1 = Z_array.Z_data[radial][range+1];

            if((Zp0 <= REC_RANGE_FOLDED_CODE) || (Zp1 <= REC_RANGE_FOLDED_CODE))
            {
/**             if (debugit) fprintf(stderr,"radial = %d, range = %d, setting dBZdifSq to NO_DATA\n",radial,range);**/
                dBZdifSq[radial][range] = REC_NO_DATA_FLAG;
                sign[radial][range] = REC_NO_DATA_FLAG;
            }
            else
            {
                dBZdif = (Zp1 - Zp0) / ZRES;  /* ZRES converts to dBZ  */
                if (dBZdif > 0){

                    sign[radial][range] = 1;
                    dBZdifSq[radial][range] = dBZdif * dBZdif; /* dBZ**2 */
/*                  if (debugit) fprintf(stderr,"Plus %*.*f\n",5,2,dBZdifSq[radial][range]);*/
                }
                else if (dBZdif == 0){

                    sign[radial][range] = 0;
                    dBZdifSq[radial][range] = 0;
/*                  if (debugit) fprintf(stderr,"Zero\n");*/
                }
                else{

                    sign[radial][range] = -1;
                    dBZdifSq[radial][range] = dBZdif * dBZdif; /* dBZ**2 */
/*                  if (debugit) fprintf(stderr,"Minus %*.*f\n",5,2,dBZdifSq[radial][range]);*/
                }

                /*  Flag ZSPIN differences for counting */
                if (fabsf(dBZdif) > adapt_cl_target.ZspinThr) 
                    spinFlag[radial][range] = 2;    /*  Count high values   */
                else
                    spinFlag[radial][range] = 1;    /*  Count all samples */

            }/* end ifelse */

        }/* end for range */

/*      if (debugit) fprintf(stderr,"END OF RADIAL\n");*/

    }/* end for radial */


        
/*  2D (tilt) computations.  Note, this software does not attempt to generate reflectivity
    difference squared for the first or last deltaAz-1 radials, nor for the first or last
    deltaRng-1 range bins.  Rather, it leaves these boundaries uncomputed for legacy RDA
    data.  For open systems RDA, the azimuthal boundaries are computed (Added for Build 9).  */
    if (RDA_type == ORPGRDA_LEGACY_CONFIG){

      start_radial = adapt_cl_target.deltaAz;
      end_radial = MAX_RADIALS - adapt_cl_target.deltaAz - 1;

    }
    else{

      start_radial = 0;
      end_radial = 359;

    }      
    for (radial = start_radial; radial <= end_radial; radial++){

        for (range = adapt_cl_target.deltaRng; range < numDBZGates - adapt_cl_target.deltaRng; range++){

            if (Z_array.Z_data[radial][range] == REC_MISSING_FLAG){

                if (debugit) fprintf(stderr,"radial=%d, range=%d, setting texture to REC_NO_DATA_FLAG\n",radial,range);
                Ztxtr[radial][range] = REC_NO_DATA_FLAG;
                Zsign[radial][range] = REC_NO_DATA_FLAG;
                Zspin[radial][range] = REC_NO_DATA_FLAG;
            }
            else {

                Ztxtr[radial][range] = MeandBZdifSq(radial, range);

            }

        }/* end for range */
        if (debugit) fprintf(stderr,"rad=%d,rg=50,Ztxtr=%5.1f,Zsign=%2.3f,Zspin=%5.1f\n",radial,Ztxtr[radial][50],Zsign[radial][50],Zspin[radial][50]);
    }/* end for radial */

}/* end Compute_Z_texture() */

#define UNDEFINED -2

static float MeandBZdifSq (int radial, int range) {
/** For a given radar bin, computes the average dBZ difference squared (dBZdifSq) and the
    average sign (Zsign) of the small area bounded by +/- deltaAz and +/- deltaBin.  **/

    int     rBin,
            azim,
            counter,
            spinCounter,
            spinThreshCounter;

    float   sumDiff,
            sumSign,
            floatCount,
            meandBZdifSqr;

    int previous_sign;
    int curr_radial;  /*  Computed index for handling ORDA indexed radials  */

    counter = 0;
    spinCounter = 0;
    spinThreshCounter = 0;
    sumDiff = 0.0;
    sumSign = 0;

    for (azim = -adapt_cl_target.deltaAz; azim <= adapt_cl_target.deltaAz; azim++)
    {
        previous_sign = UNDEFINED;

        /*  Following code added for Build 9 to handle ORDA indexed radials  5/10/06 WDZ */
        curr_radial = radial + azim;
        if(RDA_type != ORPGRDA_LEGACY_CONFIG){
          if(curr_radial < 0){
            curr_radial += 360;
          }
          if(curr_radial > 359){
            curr_radial -= 360;
          }
        }
        for (rBin = -adapt_cl_target.deltaRng; rBin <= adapt_cl_target.deltaRng; rBin++)
        {
            if (dBZdifSq[curr_radial][range+rBin] >= 0.0)
            {
                counter++;
                sumDiff += dBZdifSq[curr_radial][range+rBin];
                sumSign += sign[curr_radial][range+rBin];

                /* Initialize the dBZ difference sign for this radial. */
                if( previous_sign == UNDEFINED )
                   previous_sign = sign[curr_radial][range+rBin];

                /* Count all possible dBZ differences. */
                if (spinFlag[curr_radial][range+rBin] >= 1) spinCounter++;

                /* If dBZ difference is greater than a threshold value,
                   increment the spinThreshCounter counter. */
                if (spinFlag[curr_radial][range+rBin] == 2){

                   if( sign[curr_radial][range+rBin] != previous_sign ){

                      spinThreshCounter++;
                      previous_sign = sign[curr_radial][range+rBin];

                   }

                }
            }/* end if */
        }/* end for bin */
    }/* end for azimuth */

/*  if (debugit) fprintf(stderr,"counter=%d, spinCounter=%d, spinThreshCounter=%d\n",counter,spinCounter,spinThreshCounter);*/
    if (counter > 0)
    {
        floatCount = counter;
        meandBZdifSqr = sumDiff / floatCount;  /* in dBZ**2 */
        Zsign[radial][range] = sumSign / floatCount;

/*      if (debugit) fprintf(stderr,"floatCount=%f, sumDiff=%f, Zsign=%f\n",floatCount,sumDiff,Zsign[radial][range]);*/
    }
    else
    {
        meandBZdifSqr = REC_NO_DATA_FLAG;
        Zsign[radial][range] = REC_NO_DATA_FLAG;
        if (debugit) fprintf(stderr,"***ERROR (NO_DATA)***  setting sign to -1\n");
    }

    if( spinCounter > 0 )
        Zspin[radial][range] = ((float) spinThreshCounter / (float) spinCounter) * 100.;

    else
        Zspin[radial][range] = REC_NO_DATA_FLAG;

    return (meandBZdifSqr);
}/* end MeandBZdifSq() */



void Compute_V_stddev() {
/** Computes the standard deviation (StdSqV) of the radial differences in velocity
    for the area bounded by +/- deltaAz and +/- deltaBin.  **/

    int radial,
        range,      /*  1 km range bin  */
        Vrange,     /*  250 meter range bin  */
        bin;        /*  250 meter range bin */
    int start_radial;  /*  First radial to be processed;  Added for Build 9 */
    int end_radial;    /*  Last radial to be processed;   Added for Build 9 */
        
        
/*  Radial computation of velocity squared (Vsqr).  */
    for (radial = 0; radial <= (MAX_RADIALS-1); radial++)
        for (range = 0; range < numDopGates/4; range++)
            for (bin = 0; bin <= 3; bin++) {
                Vrange = range * 4 + bin;

                codedV[radial][Vrange] = D_array.V_data[radial][range][bin];

                if (codedV[radial][Vrange] <= REC_RANGE_FOLDED_CODE)
                    Vsqr[radial][Vrange] = REC_NO_DATA_FLAG;
                else  {
/*  timo
    At the recommendation of Brian Klein, use coded V rather than V to save time,
    be sure to multiply answer by DopplerRes below to adjust for V.
 */
                    Vsqr[radial][Vrange] = codedV[radial][Vrange] * codedV[radial][Vrange];
                }
            }

/*  2D (tilt) computation of standard deviation.  Note, this software does not attempt to generate
    velocity standard deviations for the first or last deltaAz-1 radials, nor for the first or last
    deltaRng-1 range bins.  Rather, it leaves these boundaries uncomputed for legacy RDA data.  
    For ORDA the azimuthal boundaries are computed (added for Build 9).				*/

    if (RDA_type == ORPGRDA_LEGACY_CONFIG){
      start_radial = adapt_cl_target.deltaAz;
      end_radial = MAX_RADIALS - adapt_cl_target.deltaAz - 1;
    }
    else{
      start_radial = 0;
      end_radial = 359;
    }      

    for (radial = start_radial; radial <= end_radial; radial++)
        for (Vrange = adapt_cl_target.deltaBin; Vrange < numDopGates - adapt_cl_target.deltaBin; Vrange++) {
            range = Vrange / 4;
            bin = Vrange - (range * 4);

            if (Vsqr[radial][Vrange] <= REC_NO_DATA_FLAG)
                VstDv[radial][range][bin] = REC_NO_DATA_FLAG;
            else 
                VstDv[radial][range][bin] = StdDev(radial, Vrange);

/***            if (debugit) fprintf(stderr,"in compute_V_stddev: rad=%d rg=%d, VstDv = %5.3f\n",
                        radial,range,VstDv[radial][range][bin]);***/
        }
}


static float StdDev(int radial, int range) {
/** For a given bin, computes the standard deviation of velocity for area bounded by
    +/- deltaAz and +/- deltaBin.  **/

    int     bin,
            azim,
            counter;
    int curr_radial;   /*  Index added for build 9 to correct computation for ORDA
    			   along 0th/359th radial interface */

    float   floatCounter,
            sumV,           /*  Sum of coded velocities         */
            sumSqrs,        /*  Sum of coded velocity squared   */
            sum_of_Sqrs,    /* Sum of squared velocity.             */  
            sqr_of_Average, /*  Square of Average of velocity.      */
            stdV;           /*  Standard deviation of velocity      */

/*  if (debugit) printf("\nIn stdSq: ");  */

    counter = 0;
    sumV = 0;
    sumSqrs = 0;

/*  For each bin, variance is computed within +/- deltaAz and +/- deltaBin  */
    for (azim = radial - adapt_cl_target.deltaAz; azim <= radial + adapt_cl_target.deltaAz; azim++)
    {
        curr_radial = azim;
        if(RDA_type != ORPGRDA_LEGACY_CONFIG){
          if(curr_radial < 0){
            curr_radial += 360;
          }
          if(curr_radial > 359){
            curr_radial -= 360;
          }
        }
        for (bin = range - adapt_cl_target.deltaBin; bin <= range + adapt_cl_target.deltaBin; bin++)
        {
            if (Vsqr[curr_radial][bin] > REC_NO_DATA_FLAG)
            {
                counter++;
                sumV = sumV + codedV[curr_radial][bin];
                sumSqrs = sumSqrs + Vsqr[curr_radial][bin];
            }/* end if */
        }/* end for bin */
    }/* end for azimuth */

    if (counter <= 1)
        stdV = REC_NO_DATA_FLAG;

    else
    {
        floatCounter = counter;

        sum_of_Sqrs = sumSqrs / floatCounter;

        sqr_of_Average = sumV / floatCounter;
        sqr_of_Average *= sqr_of_Average;
            
        stdV = sqrt((double)(sum_of_Sqrs - sqr_of_Average)) * DopplerRes;
/***        if (debugit) fprintf(stderr,"stdV %lf\n",stdV);***/
    }/* end ifelse */
/*  if (debugit) fprintf(stderr,"cnt%2.2d DopRes%2.3f sumSq%7.1f sqSum%7.1f std%lf\n",
            counter, DopplerRes, sumSqrs, sqrSums, stdV);*/
    return (stdV);
}
