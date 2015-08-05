/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/11/24 23:40:16 $
 * $Id: epre_alg.c,v 1.8 2009/11/24 23:40:16 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/******************************************************************************
* Routine c_epre performs the actions necessary to generate Hybrid Scan
* reflectivity and elevation angle data files.  Inputs to this program
* include base reflectivity data, clutter/AP identification from the Radar
* Echo Classifier (REC), and beam blockage estimates from Blockage.c.
* By Tim O'Bannon, January 2002.
    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    01/15/02      0000       Tim O'Bannon       CCR
    10/26/05      0001       C. Pham            CCR NA05-21401
    02/17/06      0001       D. Zittel          CCR NA05-31201
******************************************************************************/

#include <math.h>
#include "epre_main.h"

/****  Global variables *****************************************************/

int    hysBinCount,       /*  Counts number of filled Hybrid Scan bins      */
       numberRadials,     /*  Input number of radials of base reflectivity  */
       tiltNumber,        /*  Tilt number                                   */
       fullHys,           /*  Flag that defines a full Hybrid Scan          */
       volumeNumber,      /*  Radar volume number                           */
       current_vcp;       /*  vcp number                                    */

double lowZLimit_val;     /*  Low DBZ threshold converted to power          */
double vcpElAngle;        /*  VCP elevation angle                           */

short  biasedDBZdata[MAX_RAD][MAX_RNG],  /* Input reflectivity data         */
       clutterAP[MAX_RAD][MAX_RNG];      /* Input AP/clutter probability    */

char   beamBlockage[BLOCK_RAD][MAX_RNG];     /* Input BeamBlockage data     */

double real_azimuth[MAX_RAD];
double sumWts[MAX_AZM][MAX_RNG],
       sumWtdZ[MAX_AZM][MAX_RNG];             /* Weights and Averged Power  */

static short hybridScanZ[MAX_AZM][MAX_RNG],   /* Output Hybrid Scan Refl    */
              hyScanElAng[MAX_AZM][MAX_RNG];  /* Output Hybrid Scan EleAngle*/

static unsigned long lasTimeRain;             /* last time for raining      */
static int prevResetSTP;                      /* flag for reset STP         */

static int  debugit = FALSE;                  /* For debugging use          */

static int  LRDA    = 0;                      /* Defined constant for Legacy RDA     */
      char *rda_ptr = 0;		      /* Null pointer for determining if data
						 is from legacy or open RDA, used by 
						 c_map_to_hybrid_scan                */
       int  rda_type;                         /* Indicates which RDA type is providing
                                                 data; 0 = legacy RDA, 1 = ORDA      */

/*** The Declaration for Each Function **************************************/

void c_initialize_hys(void);       /* Initialize the Hybrid Scan reflectivity
                                    and elevation angle files               */

void c_init_wt_arrays(void);       /* Initialize the azimuth weighting info */

void c_map_to_hybrid_scan(int);    /* Map the input reflectivity data to the
                                      whole degree (azimuth) Hybrid Scan.
                                      for temporary testing                 */
int  c_rain_detection(void);       /* Precip Detection Function             */

double biasedDBZ2power(int, double, int);
                                   /* Converts from biased dBZ to power
                                      accounting for beam blockage.         */
int power2biasedDBZ (double);      /* Converts from equivalent power to biased
                                      dBZ.                                  */

void c_read_blockage_file(int);    /* Read blockage data                    */

/****************************************************************************/

int c_epre(int *startup,
           int *newHysFlag,
           int passedVcpNum,
           int passedTilt,
           int nextLastElev,
           int passedVolNum,
           unsigned long daTimeSecs,
           int elevangle,
           int elevind,
           int radialNumber,
           int *rainDetected,
           short passed_ref[][MAX_RNG],
           short clut_ap[][MAX_RNG],
           double passed_azimuth[],
           short hys[][MAX_RNG],
           short hys_el[][MAX_RNG])
{

  int i, j,
      azm,
      rng,
      noRainDuration,
      durationThreshold=hyd_epre.rain_time_thresh * SEC_IN_MIN;/* In seconds*/

  double filledPortion;

  current_vcp = passedVcpNum;
  tiltNumber =  passedTilt;
  volumeNumber = passedVolNum;
  numberRadials = radialNumber;

  if ( debugit ) {
   fprintf(stderr,"newHysFlag=%d\tpassedVcpNum=%d\n",*newHysFlag,passedVcpNum);
   fprintf(stderr,"passedVolNum=%d\tpassedTilt=%d\tdaTimeSecs=%lu\n\n",
                  passedVolNum, passedTilt, daTimeSecs);
  }
  if ( *startup ){
                                 /* Initialize rain detection date + time    */
     lasTimeRain = daTimeSecs - SEC_IN_HOUR;
     prevResetSTP = 1;           /* as if it has not been raining for an hour*/
     *startup = FALSE;
    }
 
 /*  Added to support radial mapping with ORDA data   WDZ 02/17/2006         */ 
  rda_type = ORPGRDA_get_rda_config( rda_ptr );  /* Find out which type of RDA
						    is providing the data    */
  if( debugit)
    fprintf(stderr,"\n rda_type = %d\n",rda_type);
  fullHys = 0;                   /* Initialize full hybrid scan flag         */
   
  vcpElAngle = (double)elevangle / 10; /* Convert the elevation angle of
                                         an integer type into float type     */

/* If this is a new hybrid scan */
  if ( *newHysFlag ) {
      *newHysFlag = 0;              /*  Reset flag                           */
      a3133c7.rej_blkg_cnt=0;
      a3133c7.rej_cltr_cnt=0;
      c_initialize_hys( );          /*  Initialize hybrid scan arrays        */
   }

/* Convert low dbz threshold to power */
   lowZLimit_val = pow(10.0, hyd_epre.low_dbz_thresh / 10.);

   if ( debugit) fprintf(stderr,"\n=========> Begin C_EPRE Module <============");

/* Get azimuth angle, ap/clutter data and base reflectivity data */

   for ( i=0; i<MAX_RAD; i++ ){
       real_azimuth[i] = passed_azimuth[i];
       for ( j=0; j<MAX_RNG; j++ ) {
           biasedDBZdata[i][j] = passed_ref[i][j];
           clutterAP[i][j] = clut_ap[i][j];
          }
      }

/* Get Blockage data */

   c_read_blockage_file(elevangle);

/* Map the reflectivity data into the hybrid scan.  This is the core module
   of the EPRE.  It performs the data quality tests (BLOCKAGE THRESHOLD and
   CLUTTER THRESHOLD) and checks for hybrid scan exclusions zones to determine
   whether the input bin of reflectivity data should be mapped, and then uses
   an azimuthal weighting scheme to remap the data.  The reflectivity from the
   lowest radar bin that passes the tests is used in the hybrid scan. */
 
   c_map_to_hybrid_scan(elevind);

/* Copy hybrid Scan containing 2 dimensional arrays to one-dimension array */
   for ( azm=0; azm < MAX_AZM; azm++ )
       for( rng=0; rng < MAX_RNG; rng++ ) {
          hys[azm][rng] = hybridScanZ[azm][rng];
          hys_el[azm][rng] = hyScanElAng[azm][rng];
         }

/* The hybrid scan is considered full when the percentage of bins filled
   exceeds FULL_HYS_THRESHOLD. */

   filledPortion = (double) hysBinCount / (double) (MAX_AZM * MAX_RNG) * 100.0;

   if (debugit) fprintf( stderr, "\nepre: Filled portion of HyScan = %6.4f\n",
                       filledPortion );
 
   if ( filledPortion >= hyd_epre.full_hys_thresh )
      fullHys = 1;
 
   /* The following logic implements a rain detection functionality.  First the
   area of hybrid scan reflectivity exceeding a rain reflectivity threshold
   is computed.  If that area exceeds a rain area threshold, rain is
   considered to have been detected.  If no rain is detected for a threshold
   duration, the Storm Total Product is reset to zero. */

   if ( fullHys || elevind == nextLastElev ) {
      *rainDetected = c_rain_detection();
      a3133c7.rain_detec_flg = *rainDetected;

      if (debugit) fprintf( stderr,"epre: Rain detected = %d", *rainDetected );
      a3133c7.reset_stp_flg = 0;
      a3133c7.prcp_begin_flg = 0;
   
      if ( *rainDetected ){
         lasTimeRain = daTimeSecs;
         noRainDuration = 0;
        }
      else {
         noRainDuration = daTimeSecs - lasTimeRain;
         if (noRainDuration >= durationThreshold)
              a3133c7.reset_stp_flg=1;
        }
      /* If it has not been raining for an hour and it is presently raining,
         set the prcp_begin_flg as true */
      if (debugit)
        fprintf( stderr,"\n\nepre: SUPL prcp_begin_flg  = %d prevResetSTP = %d\n",
                 a3133c7.prcp_begin_flg, prevResetSTP );

      if ( prevResetSTP == 1 && a3133c7.reset_stp_flg == 0 )
         a3133c7.prcp_begin_flg = 1;

      prevResetSTP = a3133c7.reset_stp_flg;
      a3133c7.pct_hys_filled = filledPortion;
      a3133c7.highest_elang = vcpElAngle;

      /* the following is to print out the supplemental variable and others*/
      if ( debugit ){
        fprintf( stderr, "the next 9 lines are when the fullhys is full");

        fprintf( stderr,"\n duration = %d, threshold = %d",
                      noRainDuration, durationThreshold);
        fprintf( stderr, "\nlasTimeRain = %lu", lasTimeRain);
        fprintf( stderr,"\n TOT Number of blocked bins = %d",
                      a3133c7.rej_blkg_cnt );
        fprintf( stderr,"\n TOT number of clutter bins = %d",
                      a3133c7.rej_cltr_cnt );
        fprintf( stderr,"\nepre: SUPL pct_hys_filled = %6.4f",
                      a3133c7.pct_hys_filled );
        fprintf( stderr,"\nepre: SUPL higest_elang = %8.4f",
                      a3133c7.highest_elang );
        fprintf( stderr,"\nepre: SUPL prevResetSTP  = %d",
                      prevResetSTP );
        fprintf( stderr,"\nepre: SUPL reset_stm_total = %d",
                      a3133c7.reset_stp_flg );
        fprintf( stderr,"\nepre: SUPL prcp_begin_flg  = %d",
                      a3133c7.prcp_begin_flg );
        fprintf( stderr,"\n---------------------------------------\n\n");
       } /* end of debugit */
    } /* end of fullhys */
   if (debugit) {
     fprintf( stderr,"\nepre: fullHys = %d at ELEV=%d VOL=%d\n",
                   fullHys, tiltNumber, volumeNumber);
     fprintf( stderr, "=========> End C_EPRE Module <============\n");
    }
   return(fullHys);
}

/*****************************************************************************
 Routine c_initialize_hys initializes the Hybrid Scan reflectivity arrays to
 the missing data flag (RDMSNG = 256).
*****************************************************************************/

void c_initialize_hys( )
{

 int azm, rng;

 for ( azm = 0; azm < MAX_AZM; azm++ )
     for ( rng = 0; rng < MAX_RNG; rng++ ) {
         hybridScanZ[azm][rng] = RDMSNG;
         hyScanElAng[azm][rng] = -999;
       }
}

/*****************************************************************************
  Routine c_map_to_hybrid_scan maps the base reflectivity data to the fixed
  grid Hybrid Scan data file.  This mapping is performed by determining the
  portion of each incoming beam that contributes to each fixed whole degree
  radial of the Hybrid Scan.  That portion of the beam is then used as a
  weighting factor to allocate a weighted reflectivity value to each Hybrid
  Scan radial.  Finally, the weighted reflectivity values are summed for each
  radial and range bin, and if a significant portion of a bin is filled, then
  its average weighted reflectivity is assigned to the Hybrid Scan at that loc.

  This module builds the Hybrid Scan data file based on the philosophy that
  the lowest unblocked, uncontaminated reflectivity data should be used for
  estimating precipitation.  The base reflectivity data is not mapped to the
  Hybrid Scan reflectivity data file if;
        1.  the beam is blocked - use blockage data from Blockage Algorithm
        2.  the bin is within an exclusion zone - user selectable bounds
        3.  the bin is probably AP/clutter - use AP/clutter id from REC
            Algorithm
        4.  the Hybrid Scan data file already contains a valid answer in that
            range bin.

    By Tim O'Bannon, January 2002
    
    
    07/12/09    0000       Zhan Zhang        Add special handling for 0 crossing
                                             exclusion zones (CCR NA09-00174)                                             
*****************************************************************************/

void c_map_to_hybrid_scan(int eleN)
{

  int radial,
      rng,
      azm,
      intAzB,
      intAzA,
      blockageAzm,
      biasedDBZ,
      zone,
      clutterCounter,         /*  Debug information   */
      blockedCounter,         /*  Debug information   */
      inputZHistogram[257],   /*  Debug information   */
      hybridZHistogram[257],  /*  Debug information   */
      prefilled,              /*  Debug information   */
      skinnyRadial,           /*  Debug information   */
      goodData,               /*  Debug information   */
      clutterProb,            /*  In whole percent    */
      clutterOffset = 2;      /*  REC AP/clutter probability values are
                                  offset by 2 to accomodate missing and
                                  range folded data.  0% likelihood is
                                  represented by code value 2  */
  int    converted_beam;
  double azAngle,
         rElAngle,
         eqvPower,
         weightB,
         weightA,
         meanZ,
         blockage,
         fract_blothrsh,
         fract_weighthresh,
         BEAM_WDTH =  hyd_epre.beam_width,
         halfBeamWidth = BEAM_WDTH / 2.;
 

  if (debugit) fprintf( stderr,"Start remap hybridscan ............\n" );

  rElAngle = vcpElAngle;                  /* Define real elevation angle */
/* Initialize the histograms and the counters used in debugging the EPRE */
  for ( biasedDBZ = 0; biasedDBZ <= RDMSNG; biasedDBZ++ ) {
      inputZHistogram[biasedDBZ] = 0;
      hybridZHistogram[biasedDBZ] = 0;
    }
 
  clutterCounter = 0;
  blockedCounter = 0;

/* Initialize the azimuthal weighting information */
  c_init_wt_arrays();

/* Process each incoming radial of base data.  This loop defines the
   information needed to remap the reflectivity data to the whole degree
   Hybrid Scan, tests to see if the incoming radial is blocked or
   contaminated by AP/clutter, and implements the user selectable exclusion
   zones (if any have been defined). */

  for ( radial = 0; radial < numberRadials; radial++ ) {
      azAngle = real_azimuth[radial];

/* checking azimuth angle range */
      if ( azAngle > 360. || azAngle < 0. ) {
         printf("\nepre: Unreal azimuth angle %f at %d", azAngle, radial);
         continue;
      }

/* Determine the whole degree azimuth angles that overlap with this base data
   azimuth.  The whole degree angles will be used to store weighted
   reflectivity information into the temporary tilt scan.  Note, this
   technique is only valid for BEAM_WDTH <= 1.0 degrees. */
      
      
/* Integerize the index to the radial based on the RDA type: 0 = legacy RDA
   1 = open RDA.  Open RDA radials map precisely to the HSR radials. WDZ 12/12/2005 */
      
      if( rda_type == LRDA)
        intAzB = (int) ( azAngle + 0.5 );
      else
        intAzB = (int)( azAngle );

      if ( intAzB > 359 ) intAzB = 0;
      if ( ( intAzA = intAzB - 1 ) < 0 ) intAzA = 359;

/* Calculate the relative contribution (weight) of the beam for each of the
     overlapping whole degree azimuths. */

      if ( rda_type == LRDA){
        if( ( weightB =  azAngle + halfBeamWidth ) >= 360.0 )
           weightB = weightB - 360.0;
        weightB = weightB - intAzB;
        if ( weightB > BEAM_WDTH ) weightB = BEAM_WDTH;
        if ( weightB < ZERO ) weightB = ZERO;
        weightA = BEAM_WDTH - weightB;
     }
     else{
 /*  rda_type is ORDA  */
       weightB = BEAM_WDTH;
       weightA = 0.0;
     }

     if ( debugit )
        printf( "\n azA=%d, azB=%d, wtA=%5.2f, wtB=%5.2f",
               intAzA, intAzB, weightA, weightB );

/* Compute blockage azimuth angle (in tenths of a degree).  The Blockage
   Algorithm computes beam blockage for every 1 km range bin and for every 0.1
   deg.  Note, this number is deliberately truncated to ensure that the range
   of values falls within the array limits of 0 to 3599. */

      blockageAzm = (int) ( azAngle * 10. );

/* For each 1 km range bin, ensure that the Hybrid Scan uses the lowest
   elevation reflectivity data that is uncontaminated and unblocked. */
      for ( rng = 0; rng < MAX_RNG; rng++ )  {

/* Obtain base reflectivity data in Level II biased format, i.e. values range
    from 0 to 255. */

         biasedDBZ = (int)biasedDBZdata[radial][rng];

/* Count biasedDBZ distribution for inputZHistogram.  Note, this is used for
   debugging, not for operational code. */

         inputZHistogram[biasedDBZ]++;

/* Check that biasdDBZ value is normal */

         if ( biasedDBZ < ZERO || biasedDBZ >= RDMSNG ){
            if ( debugit )
               fprintf( stderr, "\nbad data at %d, %d, %d",
                                radial, rng, biasedDBZ );
            goto nextRangeBin;
           }

/* Obtain the beam blockage value for each bin, convert to integer from char,
   then convert to float from integer*/
         converted_beam = (int)beamBlockage[blockageAzm][rng];
         blockage = (double) converted_beam / 100.;

/* Check to be sure no illegal values of blockage are passed to the EPRE.
    Hopefully this won't be a problem in the operational code, but who can tell.
*/
         if ( blockage < 0.0 ) {
            fprintf( stderr,"\nepre: Illegal blockage value %f at az %d ran %d",
                                    blockage, blockageAzm, rng);
            goto nextRangeBin;
          }

/* If range bin is blocked, go to the next radial (because the remainder of
    the bins in this radial must also be blocked).  For debug software,
    increment the blocked data counter by the number of blocked bins. */

         fract_blothrsh = hyd_epre.block_thresh / 100.;
         if ( blockage >=  fract_blothrsh || blockage >= 1.0 ){

            blockedCounter += ( MAX_RNG - rng );

            goto nextRadial;
          }

/* No reflectivity data should be used from user selected exclusion zones.
   Note, the exclusion zones cannot cross 360 degrees in this prototype
   implementation. */
   
/* the above presumption has been changed. We add specially handling for 0 degree
   crossing exclusion zones */
         if ( hyd_epre.num_zone > 0 )
            for ( zone = 0; zone < hyd_epre.num_zone; zone++ )
             {
               /* handling normally defined exclusion zones */
                /* note that we use AND in azimuth term */
               if ( exzone[zone][0] < exzone[zone][1] )
               {

                    if ( rElAngle <= (double)exzone[zone][4] &&
                          azAngle >= (double)exzone[zone][0] &&
                          azAngle <= (double)exzone[zone][1] &&
                          rng >= (int)exzone[zone][2] &&
                          rng <= (int)exzone[zone][3]){
                        
                          /*RPGC_log_msg(GL_INFO, "inside No. %d Normal exclusion zone \n", zone);
                          RPGC_log_msg(GL_INFO, "elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f\n",
                                     exzone[zone][4],exzone[zone][0],exzone[zone][1],
                                     exzone[zone][2],exzone[zone][3]);*/
                  
                          
                          goto nextRangeBin;  }
               
                }
                else
                { /* special handling for 0-degree-crossing  exclusion zone 
                     (CCR NA09-00174) */
                  /* note that we use OR in azimuth term */
                    
                    if ( rElAngle <= (double)exzone[zone][4] &&
                          (azAngle >= (double)exzone[zone][0] ||
                          azAngle <= (double)exzone[zone][1] )&&
                          rng >= (int)exzone[zone][2] &&
                          rng <= (int)exzone[zone][3]){

                          /*RPGC_log_msg(GL_INFO, "inside No. %d 0 degree crossing exclusion zone \n", zone);
                          RPGC_log_msg(GL_INFO, "elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f\n",
                                     exzone[zone][4],exzone[zone][0],exzone[zone][1],
                                     exzone[zone][2],exzone[zone][3]);*/
                          goto nextRangeBin;  }                
                
                 }
              
              
              }
 
/* Obtain AP/clutter probability and convert to percent. */

         clutterProb = (int)( clutterAP[radial][rng] - clutterOffset );

/* If range bin is clutter/AP, go to next range bin.  For debug software,
    increment clutter counter. */

         if ( clutterProb > hyd_epre.clutter_thresh ) {
            clutterCounter++;
            goto nextRangeBin;
          }

/* If all the tests have been passed for this range bin, compute the equivalent
   power (in mm^6/m^3) and correct for partial beam blockage. */

         eqvPower = biasedDBZ2power( biasedDBZ, blockage, rng );

/* Store the weighted power and the weights for each overlapping whole
   degree azimuth. */

         sumWts[intAzA][rng]+= weightA;
         sumWts[intAzB][rng]+= weightB;
         sumWtdZ[intAzA][rng]+= ( weightA * eqvPower );
         sumWtdZ[intAzB][rng]+= ( weightB * eqvPower );

         nextRangeBin:     continue;
      }/* End bin for loop */

      nextRadial:     continue;
   }

 a3133c7.rej_blkg_cnt += blockedCounter;
 a3133c7.rej_cltr_cnt += clutterCounter;

  if ( debugit ){
    fprintf( stderr,"\n  Number of clutter bins = %d", clutterCounter );
    fprintf( stderr,"\n  Number of blocked bins = %d", blockedCounter );
   }

/* Initialize bin counter and debug counters */
  hysBinCount = 0;
  prefilled = 0;
  skinnyRadial = 0;
  goodData = 0;

/* Obtain WEIGHT_THRESHOLD probability and convert to float */
  fract_weighthresh = hyd_epre.weight_thresh / 100.;
 
/* For each whole degree and range bin, this loop checks to see if the Hybrid
    Scan has already been filled with data from a lower elevation.  If the bin
    is empty and a sufficient portion of input reflectivity data exists to fill
    the bin, then this loop computes the average power, converts to biased
    reflectivity, puts the answer into the Hybrid Scan Reflectivity array,
    and puts the elevation angle (index) of that bin into the Hybrid Scan
    Elevation array.  */

  for (azm = 0; azm < MAX_AZM; azm++) {
     for (rng = 0; rng < MAX_RNG; rng++) {

/* If Hybrid Scan file already contains an answer from a lower tilt, go to the
   next range bin.  Count the number of Hybrid Scan bins filled.  For debug,
   count the number of bins previously filled */

        if ( hybridScanZ[azm][rng] < RDMSNG ) {
           hysBinCount++;
           prefilled++;
           goto nextRangeBin2;
         }
/* If there is insufficient data in the whole degree radial to generate a
   reasonable average power, go to the next range bin.  For debug, count the
   number of bins with insufficient data */

        else if ( sumWts[azm][rng] < fract_weighthresh ) {
             skinnyRadial++;
             goto nextRangeBin2;
         }
        else {

/* Compute the average power */

             meanZ = sumWtdZ[azm][rng] / sumWts[azm][rng];

/* Convert power to biased reflectivity and put the value into the Hybrid Scan
   Reflectivity array, put the elevation index into the Hybrid Scan Elevation
   array, and count the number of Hybrid Scan bins filled.  For debug, count
   the number of bins with good data */

             hybridScanZ[azm][rng] = (short)power2biasedDBZ(meanZ);
             hyScanElAng[azm][rng] = (short)eleN;
             hysBinCount++;
             goodData++;

         }/*end of else*/

        nextRangeBin2:          continue;
      }/*end of (rng = 0; rng < MAX_RNG; rng++)*/

/* Increment the Hybrid Scan histogram counter */

      for ( rng = 0; rng < MAX_RNG; rng++ )
          hybridZHistogram[hybridScanZ[azm][rng]]++;
   }

 if ( debugit ){
    fprintf( stderr,
                 "\nNumber of filled bins = %d\nNumber previously filled = %d",
                  hysBinCount, prefilled );

    fprintf( stderr,
                  "\nNumber of new good bins = %d\nNumber of skinny bins = %d",
                  goodData, skinnyRadial );

    fprintf( stderr,"\n  Histograms\n  dbz  inputZ   hys\n" );

    for ( biasedDBZ = 0; biasedDBZ <= RDMSNG; biasedDBZ++ )
        fprintf( stderr,"\n  %3d %7d %7d ",
          biasedDBZ, inputZHistogram[biasedDBZ], hybridZHistogram[biasedDBZ] );
   } /* end of debugit */

}

/*****************************************************************************
  c_rain_detection: Computes the area (in km^2) of the Hybrid Scan with
  reflectivity greater than or equal to a defined reflectivity
  (RAIN_DBZ_THRESHOLD).  If the area is greater than or equal to a defined
  threshold (RAIN_AREA_THRESHOLD), the rain detected flag is set.

  This module replaces a portion of the old Precipitation Detection Function
  (PDF).  The PDF time continuity is performed in the main program (c_epre)
  above.
******************************************************************************/
 
int c_rain_detection()
{

  int azm,
      rng,
      detected,
      biasedRainZthresh = 2 * hyd_epre.rain_dbz_thresh + 66;

  double sinBeamWth = sin(hyd_epre.beam_width * .01745329);

/* Initialize parameters */

  a3133c7.sum_area = 0.0;
  detected = 0;

/* For each azimuth and range bin */
  for ( azm = 0; azm < MAX_AZM; azm++ )
     for ( rng = 0; rng < MAX_RNG; rng++ )
        if ( hybridScanZ[azm][rng] >= biasedRainZthresh &&
           hybridScanZ[azm][rng] != RDMSNG )

/* Increment sum with the area of bin */
  a3133c7.sum_area += sinBeamWth * (rng + 0.5);

  if ( a3133c7.sum_area >= hyd_epre.rain_area_thresh )
     detected = 1;

  if (debugit) fprintf( stderr,"SUM_AREA = %f\n",a3133c7.sum_area );

  return(detected);
}
