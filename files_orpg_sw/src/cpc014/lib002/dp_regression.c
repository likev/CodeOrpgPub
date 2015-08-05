/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:05:26 $
 * $Id: dp_regression.c,v 1.4 2009/10/27 23:05:26 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <stdio.h>                    /* FILE                   */
#include <time.h>                     /* localtime()            */
#include "qperate_Consts.h"           /* MAX_BINS               */
#include "qperate_types.h"            /* Moments_t              */
#include "hca.h"                      /* ML_r_t                 */
#include "dp_lib_func_prototypes.h"   /* Hca_beamMLintersection */

/* This is a stub for the real Hca_beamMLintersection(), which *
 * is not included in the normal distribution.                 */

void Hca_beamMLintersection(float elev, short aznum, float bin_size)
{
   return;
}

/* hydro_convert converts Krause's hydroclasses to Klein's.
 * from: lib/HydroConst.cpp and ~/include/hca.h
 *
 * Krause's  Klein's        Meaning
 * --------- -------  ---------------------------------------------
 *   0  lr     0      Light rain
 *   1  mr     0      Moderate rain -or- (Light Rain/Moderate rain)
 *   2  hr     1      Heavy Rain
 *   3  rh     2      Rain/Hail mix
 *   4  bd     3      Big Drops
 *   5  ap     5      AP
 *   6  bs     4      Biological Birds/Insects
 *   7  uk    10      Unknown
 *   8  ne    11      No Echo
 *   9  ds     6      Dry Snow
 *  10  ws     7      Wet Snow
 *  11  hc     8      Horizontal Crystals (Crystals)
 *  12  vc     8      Vertical Crystals
 *  13  gr     9      Graupel
 */

short hydro_convert[14] = {0, 0, 1, 2, 3, 5, 4, 10, 11, 6, 7, 8, 8, 9};

int times_called = 0; /* number of times called, used to pick next filetime */
int num_bins     = 0; /* bins/radial, which changes on each run.            */

/* Some convenience globals */

long  num_matched;
long  num_not_matched;

float largest_abs_diff;
int   largest_abs_diff_azm;
int   largest_abs_diff_bin;
float largest_abs_diff_qpe;
float largest_abs_diff_krause;
char  largest_abs_diff_type[40];

#define REGRESSION_LINE_MAX 200 /* most I've seen is 174 */

/* These next 4 structures are needed for the melting layer calculation. */

ML_r_t   ML_r;
ML_bin_t Melting_layer;

float    ML_top[MAX_ML_AZ];
float    ML_bottom[MAX_ML_AZ];

char* filedir = "testdata_OUN20040621.Jan1408";

#define NUM_DATAFILES 330

char* filetimes[NUM_DATAFILES] =
{
"040621080002",
"040621080045",
"040621080109",
"040621080149",
"040621080211",
"040621080232",
"040621080253",
"040621080315",
"040621080336",
"040621080358",
"040621080420",
"040621080441",
"040621080503",
"040621080558",
"040621080638",
"040621080721",
"040621080745",
"040621080803",
"040621080825",
"040621080846",
"040621080907",
"040621080929",
"040621080950",
"040621081012",
"040621081034",
"040621081055",
"040621081117",
"040621081138",
"040621081233",
"040621081313",
"040621081356",
"040621081421",
"040621081439",
"040621081500",
"040621081522",
"040621081605",
"040621081626",
"040621081648",
"040621081709",
"040621081731",
"040621081752",
"040621081814",
"040621081909",
"040621081949",
"040621082032",
"040621082056",
"040621082114",
"040621082136",
"040621082157",
"040621082218",
"040621082240",
"040621082302",
"040621082323",
"040621082345",
"040621082406",
"040621082428",
"040621082449",
"040621082544",
"040621082624",
"040621082707",
"040621082731",
"040621082750",
"040621082811",
"040621082832",
"040621082854",
"040621082915",
"040621082937",
"040621083020",
"040621083041",
"040621083103",
"040621083125",
"040621083219",
"040621083300",
"040621083343",
"040621083407",
"040621083425",
"040621083446",
"040621083508",
"040621083529",
"040621083551",
"040621083612",
"040621083634",
"040621083655",
"040621083717",
"040621083739",
"040621083800",
"040621083855",
"040621083935",
"040621084018",
"040621084042",
"040621084101",
"040621084122",
"040621084144",
"040621084205",
"040621084227",
"040621084248",
"040621084310",
"040621084331",
"040621084353",
"040621084436",
"040621084531",
"040621084611",
"040621084654",
"040621084718",
"040621084737",
"040621084758",
"040621084819",
"040621084841",
"040621084902",
"040621084924",
"040621084945",
"040621085007",
"040621085029",
"040621085050",
"040621085112",
"040621085207",
"040621085247",
"040621085443",
"040621085842",
"040621085922",
"040621090005",
"040621090029",
"040621090048",
"040621090109",
"040621090130",
"040621090152",
"040621090213",
"040621090235",
"040621090256",
"040621090318",
"040621090339",
"040621090423",
"040621090517",
"040621090558",
"040621090640",
"040621090705",
"040621090723",
"040621090744",
"040621090806",
"040621090827",
"040621090849",
"040621090910",
"040621090931",
"040621090953",
"040621091015",
"040621091036",
"040621091058",
"040621091153",
"040621091233",
"040621091316",
"040621091340",
"040621091358",
"040621091420",
"040621091441",
"040621091503",
"040621091524",
"040621091546",
"040621091607",
"040621091629",
"040621091650",
"040621091712",
"040621091734",
"040621091829",
"040621091952",
"040621092016",
"040621092034",
"040621092055",
"040621092117",
"040621092139",
"040621092200",
"040621092222",
"040621092243",
"040621092305",
"040621093816",
"040621093856",
"040621093939",
"040621094003",
"040621094021",
"040621094042",
"040621094104",
"040621094125",
"040621094147",
"040621094208",
"040621094230",
"040621094251",
"040621094313",
"040621094335",
"040621094356",
"040621094451",
"040621094531",
"040621094614",
"040621094638",
"040621094656",
"040621094718",
"040621094739",
"040621094822",
"040621094844",
"040621094905",
"040621094927",
"040621094948",
"040621095010",
"040621095032",
"040621095127",
"040621095207",
"040621095250",
"040621095314",
"040621095332",
"040621095354",
"040621095415",
"040621095436",
"040621095458",
"040621095520",
"040621095541",
"040621095602",
"040621095624",
"040621095646",
"040621095707",
"040621095802",
"040621095842",
"040621100039",
"040621100438",
"040621100518",
"040621100601",
"040621100625",
"040621100643",
"040621100704",
"040621100726",
"040621100809",
"040621100830",
"040621100852",
"040621100913",
"040621100935",
"040621100956",
"040621101018",
"040621101113",
"040621101153",
"040621101236",
"040621101300",
"040621101318",
"040621101340",
"040621101401",
"040621101423",
"040621101444",
"040621101506",
"040621101527",
"040621101549",
"040621101610",
"040621101632",
"040621101653",
"040621101748",
"040621101828",
"040621101912",
"040621101936",
"040621101954",
"040621102015",
"040621102037",
"040621102058",
"040621102120",
"040621102141",
"040621102224",
"040621102246",
"040621102307",
"040621102329",
"040621102424",
"040621102504",
"040621102547",
"040621102611",
"040621102629",
"040621102651",
"040621102712",
"040621102734",
"040621102755",
"040621102817",
"040621102838",
"040621102900",
"040621102921",
"040621102943",
"040621103004",
"040621103059",
"040621103140",
"040621103223",
"040621103247",
"040621103305",
"040621103326",
"040621103348",
"040621103409",
"040621103431",
"040621103452",
"040621103514",
"040621103535",
"040621103557",
"040621103640",
"040621103735",
"040621103815",
"040621103858",
"040621103922",
"040621103940",
"040621104002",
"040621104023",
"040621104045",
"040621104106",
"040621104128",
"040621104149",
"040621104211",
"040621104232",
"040621104254",
"040621104315",
"040621104410",
"040621104450",
"040621104533",
"040621104557",
"040621104616",
"040621104637",
"040621104658",
"040621104720",
"040621104742",
"040621104803",
"040621104824",
"040621104846",
"040621104908",
"040621104929",
"040621104951",
"040621105046",
"040621105323",
"040621105722",
"040621105802",
"040621105845",
"040621105909",
"040621105927",
"040621105949"
};

/******************************************************************************
    Filename: dp_regression.c

    Description:
    ============
      read_RegressionData() reads the regression data from the next file in
    the list (based on filetimes[]) into reg_moments[][] and rates.

    Inputs: none

    Outputs: Moments_t    reg_moments - filled reg_moments[][] array
             Regression_t rate        - filled rates

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    20070123      0000       Ward               Initial implementation
******************************************************************************/

void read_RegressionData(Moments_t** reg_moments, Regression_t* rates)
{
   char  datadir[100];
   char  datafile[100];
   char  filename[200];
   char  system_line[200];
   FILE* fp;
   char  line[REGRESSION_LINE_MAX];
   int   start_reading;
   float az, elev;
   float mlt, mlb;
   int   az_i = 0;
   int   bin;
   /* V, SPW, PhiDP, SNR, KDP2, STDZ, STDPhiDP, are in the data,
    * but they are unused. */
   float V, SPW, PhiDP, SNR, KDP2, STDZ, STDPhiDP;
   char  toss1[20], toss2[20];
   int   toss3;
   int   krause_hydro;
   /* long  lineno; */

   #ifndef REGRESSION_TESTING
      return;
   #endif

   /* Set the directory we'll be reading from */

   sprintf(datadir, "%s/%s",
                     "/home/wardj/tmp/regression",
                     filedir);

   /* Get the PreProOut data */

   sprintf(datafile, "PreProOut_%s.txt",
           filetimes[times_called % NUM_DATAFILES]);

   memset(filename, 0, 200);
   sprintf(filename, "%s/%s", datadir, datafile);

   sprintf(system_line, "/bin/gunzip %s.gz >& /dev/null", filename);
   system(system_line);
   sleep(5); /* to give gunzip time to work */

   fp = fopen(filename, "r");

   for(az_i = 0; az_i < MAX_AZM; az_i++)
   {
      for(bin = 0; bin < MAX_BINS; bin++)
      {
         memset(&(reg_moments[az_i][bin]), 0, sizeof(Moments_t));
      }
   }

   start_reading = FALSE;
   /* lineno = 1; */
   memset(line, 0, REGRESSION_LINE_MAX);
   while(fgets(line, REGRESSION_LINE_MAX, fp) != NULL)
   {
     /* fprintf(stderr, "%ld %s", lineno, line); lineno++; */

     if(strncmp(line, "Az:", 3) == 0)
     {
        sscanf(line,"%3s%f%9s%d", toss1, &az, toss2, &num_bins);
        az_i = (int) RPGC_NINT(az);
        if((az_i <= 0) || (az_i >= 360))
           az_i = 0;
        start_reading = TRUE;
     }
     else if(start_reading)
     {
        /* Scan the line to get the bin number */

        sscanf(line, "%d", &bin);
        if(bin >= MAX_BINS)
           continue;

        /* Scan the line to get the data */

        sscanf(line, "%d%f%f%f%f%f%f%f%f%f%f%f%f",
               &toss3,
               &elev,
               &(reg_moments[az_i][bin].Z),
               &V,
               &SPW,
               &(reg_moments[az_i][bin].Zdr),
               &(reg_moments[az_i][bin].CorrelCoef), /* RhoHV */
               &PhiDP,
               &SNR,
               &(reg_moments[az_i][bin].Kdp),        /* KDP1 */
               &KDP2,
               &STDZ,
               &STDPhiDP);

        /* Convert PROTOTYPE_NODATA to QPE_NODATA */

        if(reg_moments[az_i][bin].Z          == PROTOTYPE_NODATA)
           reg_moments[az_i][bin].Z           = QPE_NODATA;
        if(reg_moments[az_i][bin].Zdr        == PROTOTYPE_NODATA)
           reg_moments[az_i][bin].Zdr         = QPE_NODATA;
        if(reg_moments[az_i][bin].CorrelCoef == PROTOTYPE_NODATA)
           reg_moments[az_i][bin].CorrelCoef  = QPE_NODATA;
        if(reg_moments[az_i][bin].Kdp        == PROTOTYPE_NODATA)
           reg_moments[az_i][bin].Kdp         = QPE_NODATA;

        rates->rhohv[az_i][bin] = reg_moments[az_i][bin].CorrelCoef;
     }
     memset(line, 0, REGRESSION_LINE_MAX);
   }

   fclose(fp);

   sprintf(system_line, "/bin/gzip %s >& /dev/null", filename);
   system(system_line);
   sleep(5); /* to give gzip time to work */

   /* Get the Rate data. */

   sprintf(datafile, "QPEv2_%s.txt",
           filetimes[times_called % NUM_DATAFILES]);

   memset(filename, 0, 200);
   sprintf(filename, "%s/%s", datadir, datafile);

   sprintf(system_line, "/bin/gunzip %s.gz >& /dev/null", filename);
   system(system_line);
   sleep(5); /* to give gunzip time to work */

   fp = fopen(filename, "r");

   start_reading = FALSE;
   /* lineno = 1; */
   memset(line, 0, REGRESSION_LINE_MAX);
   while(fgets(line, REGRESSION_LINE_MAX, fp) != NULL)
   {
     /* fprintf(stderr, "%ld %s", lineno, line); lineno++; */

     if(strncmp(line, "MLT:", 4) == 0)
     {
        sscanf(line,"%4s%f%4s%f", toss1, &mlt, toss2, &mlb);
     }
     else if(strncmp(line, "Az:", 3) == 0)
     {
        sscanf(line,"%3s%f%9s%d", toss1, &az, toss2, &num_bins);
        az_i = (int) RPGC_NINT(az);
        if((az_i <= 0) || (az_i >= 360))
           az_i = 0;
        start_reading = TRUE;
     }
     else if(start_reading)
     {
        /* Scan the line to get the bin number */

        sscanf(line, "%d", &bin);
        if(bin >= MAX_BINS)
           continue;

        /* Scan the line to get the data */

        sscanf(line, "%d%f%d%f%f%f%f",
               &toss3,
               &elev,
               &krause_hydro,
               &(rates->Rz[az_i][bin]),
               &(rates->Rkdp[az_i][bin]),
               &(rates->Rec[az_i][bin]),
               &(rates->Rzdr[az_i][bin]));

        reg_moments[az_i][bin].HydroClass = hydro_convert[krause_hydro];

        ML_bottom[az_i] = mlb; /* km */
        ML_top[az_i]    = mlt; /* km */

        Hca_beamMLintersection(elev, az_i, 0.25);

        /* Fix later ...
        reg_moments[az_i][bin].Mlt[3] = Melting_layer.bin_bb;
        reg_moments[az_i][bin].Mlt[2] = Melting_layer.bin_b;
        reg_moments[az_i][bin].Mlt[1] = Melting_layer.bin_t;
        reg_moments[az_i][bin].Mlt[0] = Melting_layer.bin_tt;
        */
     }
     memset(line, 0, REGRESSION_LINE_MAX);
   }

   fclose(fp);

   sprintf(system_line, "/bin/gzip %s >& /dev/null", filename);
   system(system_line);
   sleep(5); /* to give gzip time to work */

} /* end read_RegressionData() ========================================== */

/******************************************************************************
    Filename: dp_regression.c

    Description:
    ============
      compare_QPE_Prototype() compares QPE's rate with Prototype rate for 1 bin.

    This is a helper function for compareRates().

    diff_percent is no longer used, kept here in case it
    gets used again.

    Inputs: int   azm    - azimuth
            int   bin    - bin
            float qpe    - qpe rate
            float krause - Prototype's rate
            char* type   - type for possible error message.

    Outputs: A comparison

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    20070123      0000       Ward               Initial implementation
******************************************************************************/

void compare_QPE_Prototype(int azm, int bin, char* type,
                           Stats_t* stats, Regression_t* rates)
{
   float  diff, abs_diff;
   float  diff_percent, abs_diff_percent;

   #ifndef REGRESSION_TESTING
      return;
   #endif

   diff = stats->rate[azm][bin] - rates->Rec[azm][bin];
   if(fabsf(stats->rate[azm][bin]) > 0.0)
      diff_percent = (diff * 100.0) / stats->rate[azm][bin];
   else /* can't do the ratio */
      diff_percent = QPE_NODATA;

   abs_diff         = fabsf(diff);
   abs_diff_percent = fabsf(diff_percent);

   if(abs_diff > MAX_ABS_DIFF)
   {
      fprintf(stderr,
        "[%d][%d] %s QPE %f != Prototype %f diff %f\n",
              azm, bin,
              type,
              stats->rate[azm][bin],
              rates->Rec[azm][bin],
              diff);
      num_not_matched++;
   }
   else
      num_matched++;

   if(abs_diff > largest_abs_diff)
   {
      largest_abs_diff        = abs_diff;
      largest_abs_diff_azm    = azm;
      largest_abs_diff_bin    = bin;
      largest_abs_diff_qpe    = stats->rate[azm][bin];
      largest_abs_diff_krause = rates->Rec[azm][bin];
      strcpy(largest_abs_diff_type, type);
   }
}

/******************************************************************************
    Filename: dp_regression.c

    Description:
    ============
      compareRates() our QPE rates->with the Prototype's rates

    Inputs: Stats_t* stats - QPE's rate data

    Outputs: A comparison printed to stderr.

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    20070123      0000       Ward               Initial implementation
******************************************************************************/

void compareRates(Stats_t* stats, Regression_t* rates)
{
  int    i, j, num_bins_filled;
  long   num_bi;
  long   num_ne;
  long   num_low_rhohv;
  long   num_low_rhohv_75;
  long   num_low_rhohv_65;
  long   num_low_rhohv_50;
  long   num_low_rhohv_other;
  long   num_low_rhohv_nodata;
  long   num_no_z;
  long   num_ra;
  long   num_hr;
  long   num_bd;
  long   num_rh_above_ml;
  long   num_rh_below_ml;
  long   num_ws;
  long   num_gr;
  long   num_ds_above_ml;
  long   num_ds_below_ml;
  long   num_ic_above_ml;
  long   num_ic_below_ml;
  long   num_gc;
  long   num_uk;
  long   num_case_not_matched;
  long   subtotal;
  long   total_bins;
  float  percent;
  time_t now;

  #ifndef REGRESSION_TESTING
     return;
  #endif

  fprintf(stderr, "\n----------------------------------------------------\n");

  fprintf(stderr, "compareRates %d) for %s/QPEv2_%s.txt\n",
          times_called + 1,
          filedir,
          filetimes[times_called % NUM_DATAFILES]);

  now = time(NULL);
  fprintf(stderr, "%s\n", asctime(localtime(&now)));

  /* read_RegressionData() is called every elevation, but compareRates()
   * is called once per volume, so we want to increment the counter here. */

  times_called++;

  num_bi               = 0;
  num_ne               = 0;
  num_no_z             = 0;
  num_ra               = 0;
  num_hr               = 0;
  num_bd               = 0;
  num_rh_above_ml      = 0;
  num_rh_below_ml      = 0;
  num_ws               = 0;
  num_gr               = 0;
  num_ds_above_ml      = 0;
  num_ds_below_ml      = 0;
  num_ic_above_ml      = 0;
  num_ic_below_ml      = 0;
  num_gc               = 0;
  num_uk               = 0;
  num_low_rhohv        = 0;
  num_low_rhohv_75     = 0;
  num_low_rhohv_65     = 0;
  num_low_rhohv_50     = 0;
  num_low_rhohv_other  = 0;
  num_low_rhohv_nodata = 0;
  num_case_not_matched = 0;

  num_matched          = 0;
  num_not_matched      = 0;

  largest_abs_diff        = 0.0;
  largest_abs_diff_azm    = 0;
  largest_abs_diff_bin    = 0;
  largest_abs_diff_qpe    = 0.0;
  largest_abs_diff_krause = 0.0;
  memset(largest_abs_diff_type, 0, 40);

  num_bins_filled = num_bins;
  if(num_bins_filled > MAX_BINS)
     num_bins_filled = MAX_BINS;

  for(i=0; i<MAX_AZM; i++)
  {
     for(j=0; j<num_bins_filled; j++)
     {
        if (stats->reason[i][j] == STATS_BI)
        {
           num_bi++;

           /* We always set biota to 0.0, while Prototype
            * sometimes calculates an Rz value, so we can't
            * compare them. */

           if (stats->rate[i][j] != 0.0)
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != %f\n",
                      i, j,
                      "BI",
                      stats->rate[i][j],
                      0.0);
              num_not_matched++;
           }
           else
              num_matched++;
        }
        else if (stats->reason[i][j] == STATS_NE)
        {
           num_ne++;

           /* We always set no echo to 0.0, while Prototype
            * sometimes calculates an Rz value, so we can't
            * compare them. John Krause says:
            *
            * "The computation of Rz is only dependent on Z not
            * on HydroClass so yes you are reading it properly.
            * In the current system that bin would produce precipitation,
            * conditioned by exclusion zones and terrain of course." */

           if (stats->rate[i][j] != 0.0)
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != %f\n",
                      i, j,
                      "NE",
                      stats->rate[i][j],
                      0.0);
           }
           else
              num_matched++;
        }
        else if (stats->reason[i][j] == STATS_NO_Z)
        {
           num_no_z++;

           /* John Krause says -91750.398438 is a missing sanity
            * check on the calcECRainRate subroutine. */

           if((fabs(rates->Rec[i][j] - PROTOTYPE_NODATA) > MAX_ABS_DIFF) &&
              (fabs(rates->Rec[i][j] - -91750.398438) > MAX_ABS_DIFF))
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != rates->Rec %f\n",
                      i, j,
                      "NO_Z",
                      stats->rate[i][j],
                      rates->Rec[i][j]);
              num_not_matched++;
           }
           else
              num_matched++;
        }
        else if (stats->reason[i][j] == STATS_RA)
        {
           num_ra++;
           compare_QPE_Prototype(i, j, "RA", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_HR)
        {
           num_hr++;
           compare_QPE_Prototype(i, j, "HR", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_BD)
        {
           num_bd++;
           compare_QPE_Prototype(i, j, "BD", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_RH_ABOVE_ML)
        {
           num_rh_above_ml++;
           compare_QPE_Prototype(i, j, "RH_ABOVE_ML", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_RH_BELOW_ML)
        {
           num_rh_below_ml++;

           /* RH_BELOW_ML depends on KDP, which may be missing. */

           if((stats->rate[i][j] == QPE_NODATA) &&
              (fabs(rates->Rec[i][j] - PROTOTYPE_NODATA) <= MAX_ABS_DIFF))
           {
              num_matched++;
           }
           else
           {
              compare_QPE_Prototype(i, j, "RH_BELOW_ML", stats, rates);
           }
        }
        else if (stats->reason[i][j] == STATS_WS)
        {
           num_ws++;
           compare_QPE_Prototype(i, j, "WS", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_GR)
        {
           num_gr++;
           compare_QPE_Prototype(i, j, "GR", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_DS_ABOVE_ML)
        {
           num_ds_above_ml++;
           compare_QPE_Prototype(i, j, "DS_ABOVE_ML", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_DS_BELOW_ML)
        {
           num_ds_below_ml++;
           compare_QPE_Prototype(i, j, "DS_BELOW_ML", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_IC_ABOVE_ML)
        {
           num_ic_above_ml++;
           compare_QPE_Prototype(i, j, "IC_ABOVE_ML", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_IC_BELOW_ML)
        {
           num_ic_below_ml++;
           compare_QPE_Prototype(i, j, "IC_BELOW_ML", stats, rates);
        }
        else if (stats->reason[i][j] == STATS_GC)
        {
           num_gc++;

           /* We call this ground clutter, while Prototype calls it
            * AP, anomalous propagation, and sometimes gets an
            * Rz value, so we can't compare them. */

           if (stats->rate[i][j] != QPE_NODATA)
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != %d\n",
                      i, j,
                      "GC",
                      stats->rate[i][j],
                      QPE_NODATA);
              num_not_matched++;
           }
           else
              num_matched++;
        }
        else if (stats->reason[i][j] == STATS_UK)
        {
           num_uk++;

           /* We always set unknown to QPE_NODATA, while Prototype
            * sometimes calculates an Rz value, so we can't
            * compare them. */

           if (stats->rate[i][j] != QPE_NODATA)
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != %d\n",
                      i, j,
                      "UK",
                      stats->rate[i][j],
                      QPE_NODATA);
              num_not_matched++;
           }
           else
              num_matched++;
        }
        else if (stats->reason[i][j] == STATS_LOW_RHOHV)
        {
           num_low_rhohv++;

           if(rates->rhohv[i][j] == PROTOTYPE_NODATA)
              num_low_rhohv_nodata++;
           else if(rates->rhohv[i][j] >= 0.75)
              num_low_rhohv_75++;
           else if(rates->rhohv[i][j] >= 0.65)
              num_low_rhohv_65++;
           else if(rates->rhohv[i][j] >= 0.50)
              num_low_rhohv_50++;
           else
              num_low_rhohv_other++;

           /* We always set bins with a correlation coefficient
            * < 0.85 to QPE_NODATA, while Prototype sometimes calculates
            * an Rz value, so we can't compare them. */

           if (stats->rate[i][j] != QPE_NODATA)
           {
              fprintf(stderr,
                "[%d][%d] %s QPE %f != %d\n",
                      i, j,
                      "LOW_RHOHV",
                      stats->rate[i][j],
                      QPE_NODATA);
              num_not_matched++;
           }
           else
              num_matched++;
        }
        else /* print a general message */
        {
           num_case_not_matched++;

           fprintf(stderr,
             "[%d][%d] %s QPE %f %f %f %f %f\n",
                   i, j,
                   "NOT MATCHED",
                   stats->rate[i][j],
                   rates->Rz[i][j],
                   rates->Rkdp[i][j],
                   rates->Rec[i][j],
                   rates->Rzdr[i][j]);
           num_not_matched++;
       }
     }
  }

  fprintf(stderr,
          "maximum absolute difference that triggers an error %9.5f mm/hr\n",
          MAX_ABS_DIFF);
  fprintf(stderr,
          "largest absolute difference                        %9.5f\n",
          largest_abs_diff);
  fprintf(stderr,
          "at: azm %d bin %d case %s, QPE %f Prototype %f\n\n",
          largest_abs_diff_azm,
          largest_abs_diff_bin,
          largest_abs_diff_type,
          largest_abs_diff_qpe,
          largest_abs_diff_krause);

  fprintf(stderr, "bins filled per radial   %6d\n\n",  num_bins_filled);
  total_bins = num_bins_filled * 360;

  subtotal = num_matched + num_not_matched;

  percent = (num_matched * 100.0) / subtotal;
  fprintf(stderr, "bins where QPE matches   %6ld %9.5f %%\n",
          num_matched, percent);
  percent = (num_not_matched * 100.0) / subtotal;
  fprintf(stderr, "bins where QPE NOT match %6ld %9.5f %%\n",
          num_not_matched, percent);
  fprintf(stderr, "                         ------\n");
  fprintf(stderr, "                         %6ld\n\n", subtotal);

  if(subtotal != total_bins)
  {
     fprintf(stderr, "subtotal %ld != total_bins %ld\n",
             subtotal, total_bins);
  }

  subtotal = num_bi + num_ne + num_no_z        + num_ra + num_hr + num_bd +
             num_rh_above_ml + num_rh_below_ml + num_ws + num_gr +
             num_ds_above_ml + num_ds_below_ml +
             num_ic_above_ml + num_ic_below_ml +
             num_gc + num_uk + num_low_rhohv;

  percent = (num_bi * 100.0) / subtotal;
  fprintf(stderr, "biota                            %6ld %9.5f %%\n",
          num_bi,  percent);
  percent = (num_ne * 100.0) / subtotal;
  fprintf(stderr, "no echo                          %6ld %9.5f %%\n",
          num_ne,  percent);
  percent = (num_no_z * 100.0) / subtotal;
  fprintf(stderr, "no Z                             %6ld %9.5f %%\n",
          num_no_z, percent);
  percent = (num_ra * 100.0) / subtotal;
  fprintf(stderr, "light/moderate rain              %6ld %9.5f %%\n",
          num_ra, percent);
  percent = (num_hr * 100.0) / subtotal;
  fprintf(stderr, "heavy rain                       %6ld %9.5f %%\n",
          num_hr, percent);
  percent = (num_bd * 100.0) / subtotal;
  fprintf(stderr, "big drops                        %6ld %9.5f %%\n",
          num_bd, percent);
  percent = (num_rh_above_ml * 100.0) / subtotal;
  fprintf(stderr, "rain/hail above melting layer    %6ld %9.5f %%\n",
          num_rh_above_ml, percent);
  percent = (num_rh_below_ml * 100.0) / subtotal;
  fprintf(stderr, "rain/hail below melting layer    %6ld %9.5f %%\n",
          num_rh_below_ml, percent);
  percent = (num_ws * 100.0) / subtotal;
  fprintf(stderr, "wet snow                         %6ld %9.5f %%\n",
          num_ws, percent);
  percent = (num_gr * 100.0) / subtotal;
  fprintf(stderr, "graupel                          %6ld %9.5f %%\n",
          num_gr, percent);
  percent = (num_ds_above_ml * 100.0) / subtotal;
  fprintf(stderr, "dry snow above melting layer     %6ld %9.5f %%\n",
          num_ds_above_ml, percent);
  percent = (num_ds_below_ml * 100.0) / subtotal;
  fprintf(stderr, "dry snow below melting layer     %6ld %9.5f %%\n",
          num_ds_below_ml, percent);
  percent = (num_ic_above_ml * 100.0) / subtotal;
  fprintf(stderr, "ice crystals above melting layer %6ld %9.5f %%\n",
          num_ic_above_ml, percent);
  percent = (num_ic_below_ml * 100.0) / subtotal;
  fprintf(stderr, "ice crystals below melting layer %6ld %9.5f %%\n",
          num_ic_below_ml, percent);
  percent = (num_gc * 100.0) / subtotal;
  fprintf(stderr, "ground clutter                   %6ld %9.5f %%\n",
          num_gc, percent);
  percent = (num_uk * 100.0) / subtotal;
  fprintf(stderr, "unknown                          %6ld %9.5f %%\n",
          num_uk, percent);
  percent = (num_low_rhohv * 100.0) / subtotal;
  fprintf(stderr, "low RhoHV                        %6ld %9.5f %%",
          num_low_rhohv,  percent);
  percent = (num_low_rhohv_75 * 100.0) / subtotal;
  fprintf(stderr, "  0.75 <= RhoHV < 0.85  %6ld %9.5f %%\n",
          num_low_rhohv_75,  percent);
  percent = (num_low_rhohv_65 * 100.0) / subtotal;
  fprintf(stderr, "%s              0.65 <= RhoHV < 0.75  %6ld %9.5f %%\n",
          "                                 ------",
          num_low_rhohv_65,  percent);
  percent = (num_low_rhohv_50 * 100.0) / subtotal;
  fprintf(stderr, "%s%6ld              0.50 <= RhoHV < 0.65  %6ld %9.5f %%\n",
          "                                 ",
          subtotal,
          num_low_rhohv_50,  percent);
  percent = (num_low_rhohv_other * 100.0) / subtotal;
  fprintf(stderr, "%sRhoHV < 0.50  %6ld %9.5f %%\n",
          "                                                             ",
          num_low_rhohv_other,  percent);
  percent = (num_low_rhohv_nodata * 100.0) / subtotal;
  fprintf(stderr, "%sRhoHV no data %6ld %9.5f %%\n",
          "                                                             ",
          num_low_rhohv_nodata,  percent);

  if(subtotal != total_bins)
  {
     fprintf(stderr, "subtotal %ld != total_bins %ld\n",
             subtotal, total_bins);
  }

  subtotal = num_low_rhohv_75 + num_low_rhohv_65 + num_low_rhohv_50 +
             num_low_rhohv_other + num_low_rhohv_nodata;
  if(subtotal != num_low_rhohv)
  {
     fprintf(stderr, "subtotal %ld != num_low_rhohv %ld\n",
             subtotal, num_low_rhohv);
  }

} /* end compareRates() =============================================== */

/******************************************************************************
    Filename: dp_regression.c

    Description:
    ============
      print_stats() prints QPE stats.

    Inputs: Stats_t* stats - QPE's rate data

    Outputs: A comparison printed to stderr.

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    ----------    -------    ---------------    ----------------------
    20080522      0000       Ward               Initial implementation
******************************************************************************/

void print_stats(Stats_t* stats)
{
  int   i, j;
  long  num_STATS_BI                = 0;
  long  num_STATS_NE                = 0;
  long  num_STATS_BLOCKED_USABILITY = 0;
  long  num_STATS_BLOCKED_NO_Z      = 0;
  long  num_STATS_BLOCKED_NO_ZPRIME = 0;
  long  num_STATS_BLOCKED_KDP_POS   = 0;
  long  num_STATS_BLOCKED_KDP_NEG   = 0;
  long  num_STATS_BLOCKED_ZPRIME    = 0;
  long  num_STATS_NO_Z              = 0;
  long  num_STATS_RA                = 0;
  long  num_STATS_HR                = 0;
  long  num_STATS_BD                = 0;
  long  num_STATS_RH_ABOVE_ML       = 0;
  long  num_STATS_RH_BELOW_ML       = 0;
  long  num_STATS_WS                = 0;
  long  num_STATS_GR                = 0;
  long  num_STATS_DS_ABOVE_ML       = 0;
  long  num_STATS_DS_BELOW_ML       = 0;
  long  num_STATS_IC_ABOVE_ML       = 0;
  long  num_STATS_IC_BELOW_ML       = 0;
  long  num_STATS_HC_UNMATCHED      = 0;
  long  num_STATS_NO_HC             = 0;
  long  num_STATS_GC                = 0;
  long  num_STATS_UK                = 0;
  long  num_STATS_LOW_RHOHV         = 0;
  long  num_STATS_HIGH_KDP_BLOCK    = 0;
  long  num_STATS_IS_EXCLUDED       = 0;
  long  num_not_checked             = 0;
  long  subtotal                    = 0;
  float percent                     = 0.0;

  #ifndef COLLECT_STATS
     return;
  #endif

  for(i=0; i<MAX_AZM; i++)
  {
     for(j=0; j<MAX_BINS; j++)
     {
        /* Uncomment this if you want to print the rate,
         * for example, if you are checking product 176
         *
         * fprintf(stderr, "[%d][%d] %f\n", i, j, stats->rate[i][j]);
         */

        switch(stats->reason[i][j])
        {
           case STATS_BI                : num_STATS_BI++;
                                          break;
           case STATS_NE                : num_STATS_NE++;
                                          break;
           case STATS_BLOCKED_USABILITY : num_STATS_BLOCKED_USABILITY++;
                                          break;
           case STATS_BLOCKED_NO_Z      : num_STATS_BLOCKED_NO_Z++;
                                          break;
           case STATS_BLOCKED_NO_ZPRIME : num_STATS_BLOCKED_NO_ZPRIME++;
                                          break;
           case STATS_BLOCKED_KDP_POS   : num_STATS_BLOCKED_KDP_POS++;
                                          break;
           case STATS_BLOCKED_KDP_NEG   : num_STATS_BLOCKED_KDP_NEG++;
                                          break;
           case STATS_BLOCKED_ZPRIME    : num_STATS_BLOCKED_ZPRIME++;
                                          break;
           case STATS_NO_Z              : num_STATS_NO_Z++;
                                          break;
           case STATS_RA                : num_STATS_RA++;
                                          break;
           case STATS_HR                : num_STATS_HR++;
                                          break;
           case STATS_BD                : num_STATS_BD++;
                                          break;
           case STATS_RH_ABOVE_ML       : num_STATS_RH_ABOVE_ML++;
                                          break;
           case STATS_RH_BELOW_ML       : num_STATS_RH_BELOW_ML++;
                                          break;
           case STATS_WS                : num_STATS_WS++;
                                          break;
           case STATS_GR                : num_STATS_GR++;
                                          break;
           case STATS_DS_ABOVE_ML       : num_STATS_DS_ABOVE_ML++;
                                          break;
           case STATS_DS_BELOW_ML       : num_STATS_DS_BELOW_ML++;
                                          break;
           case STATS_IC_ABOVE_ML       : num_STATS_IC_ABOVE_ML++;
                                          break;
           case STATS_IC_BELOW_ML       : num_STATS_IC_BELOW_ML++;
                                          break;
           case STATS_HC_UNMATCHED      : num_STATS_HC_UNMATCHED++;
                                          break;
           case STATS_NO_HC             : num_STATS_NO_HC++;
                                          break;
           case STATS_GC                : num_STATS_GC++;
                                          break;
           case STATS_UK                : num_STATS_UK++;
                                          break;
           case STATS_LOW_RHOHV         : num_STATS_LOW_RHOHV++;
                                          break;
           case STATS_HIGH_KDP_BLOCK    : num_STATS_HIGH_KDP_BLOCK++;
                                          break;
           case STATS_IS_EXCLUDED       : num_STATS_IS_EXCLUDED++;
                                          break;
           default                      : num_not_checked++;
                                          break;
        }
     }
  }

  subtotal = num_STATS_BI                +
             num_STATS_NE                +
             num_STATS_BLOCKED_USABILITY +
             num_STATS_BLOCKED_NO_Z      +
             num_STATS_BLOCKED_NO_ZPRIME +
             num_STATS_BLOCKED_KDP_POS   +
             num_STATS_BLOCKED_KDP_NEG   +
             num_STATS_BLOCKED_ZPRIME    +
             num_STATS_NO_Z              +
             num_STATS_RA                +
             num_STATS_HR                +
             num_STATS_BD                +
             num_STATS_RH_ABOVE_ML       +
             num_STATS_RH_BELOW_ML       +
             num_STATS_WS                +
             num_STATS_GR                +
             num_STATS_DS_ABOVE_ML       +
             num_STATS_DS_BELOW_ML       +
             num_STATS_IC_ABOVE_ML       +
             num_STATS_IC_BELOW_ML       +
             num_STATS_HC_UNMATCHED      +
             num_STATS_NO_HC             +
             num_STATS_GC                +
             num_STATS_UK                +
             num_STATS_LOW_RHOHV         +
             num_STATS_HIGH_KDP_BLOCK    +
             num_STATS_IS_EXCLUDED       +
             num_not_checked;

  if(subtotal != MAX_AZM_BINS)
  {
     fprintf(stderr, "subtotal %ld != MAX_AZM_BINS %d\n",
             subtotal, MAX_AZM_BINS);
     return;
  }

  /* If we got here, we filled all the bins */

  percent = (num_STATS_BI * 100.0) / subtotal;
  fprintf(stderr, 
          "biota                                           %6ld %9.5f %%\n",
          num_STATS_BI,  percent);

  percent = (num_STATS_NE * 100.0) / subtotal;
  fprintf(stderr, 
          "no echo (hydroclass)                            %6ld %9.5f %%\n",
          num_STATS_NE,  percent);

  percent = (num_STATS_LOW_RHOHV * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, RhoHV < RhoHV_Thresh (0.85)          %6ld %9.5f %%\n",
          num_STATS_LOW_RHOHV,  percent);

  percent = (num_STATS_BLOCKED_USABILITY * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, blocked %% > Usability_Blk (50 %%)     %6ld %9.5f %%\n",
          num_STATS_BLOCKED_USABILITY, percent);

  percent = (num_STATS_BLOCKED_NO_Z * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, blocked, no R(Z)                     %6ld %9.5f %%\n",
          num_STATS_BLOCKED_NO_Z, percent);

  percent = (num_STATS_BLOCKED_NO_ZPRIME * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, blocked, no R(Z')                    %6ld %9.5f %%\n",
          num_STATS_BLOCKED_NO_ZPRIME, percent);

  percent = (num_STATS_BLOCKED_KDP_POS * 100.0) / subtotal;
  fprintf(stderr, 
          "blocked, using positive KDP                     %6ld %9.5f %%\n",
          num_STATS_BLOCKED_KDP_POS, percent);

  percent = (num_STATS_BLOCKED_KDP_NEG * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, blocked, negative KDP                %6ld %9.5f %%\n",
          num_STATS_BLOCKED_KDP_NEG, percent);

  percent = (num_STATS_BLOCKED_ZPRIME * 100.0) / subtotal;
  fprintf(stderr, 
          "blocked, using R(Z')                            %6ld %9.5f %%\n",
          num_STATS_BLOCKED_ZPRIME, percent);

  percent = (num_STATS_NO_Z * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, no R(Z)                              %6ld %9.5f %%\n",
          num_STATS_NO_Z, percent);

  percent = (num_STATS_RA * 100.0) / subtotal;
  fprintf(stderr, 
          "light/moderate rain                             %6ld %9.5f %%\n",
          num_STATS_RA, percent);

  percent = (num_STATS_HR * 100.0) / subtotal;
  fprintf(stderr, 
          "heavy rain                                      %6ld %9.5f %%\n",
          num_STATS_HR, percent);

  percent = (num_STATS_BD * 100.0) / subtotal;
  fprintf(stderr, 
          "big drops                                       %6ld %9.5f %%\n",
          num_STATS_BD, percent);

  percent = (num_STATS_RH_ABOVE_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "rain/hail above melting layer                   %6ld %9.5f %%\n",
          num_STATS_RH_ABOVE_ML, percent);

  percent = (num_STATS_RH_BELOW_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "rain/hail below melting layer                   %6ld %9.5f %%\n",
          num_STATS_RH_BELOW_ML, percent);

  percent = (num_STATS_WS * 100.0) / subtotal;
  fprintf(stderr, 
          "wet snow                                        %6ld %9.5f %%\n",
          num_STATS_WS, percent);

  percent = (num_STATS_GR * 100.0) / subtotal;
  fprintf(stderr, 
          "graupel                                         %6ld %9.5f %%\n",
          num_STATS_GR, percent);

  percent = (num_STATS_DS_ABOVE_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "dry snow above melting layer                    %6ld %9.5f %%\n",
          num_STATS_DS_ABOVE_ML, percent);

  percent = (num_STATS_DS_BELOW_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "dry snow below melting layer                    %6ld %9.5f %%\n",
          num_STATS_DS_BELOW_ML, percent);

  percent = (num_STATS_IC_ABOVE_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "ice crystals above melting layer                %6ld %9.5f %%\n",
          num_STATS_IC_ABOVE_ML, percent);

  percent = (num_STATS_IC_BELOW_ML * 100.0) / subtotal;
  fprintf(stderr, 
          "ice crystals below melting layer                %6ld %9.5f %%\n",
          num_STATS_IC_BELOW_ML, percent);

  percent = (num_STATS_HC_UNMATCHED * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, hydroclass not matched               %6ld %9.5f %%\n",
          num_STATS_HC_UNMATCHED, percent);

  percent = (num_STATS_NO_HC * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, no hydroclass                        %6ld %9.5f %%\n",
          num_STATS_NO_HC,  percent);

  percent = (num_STATS_GC * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, ground clutter                       %6ld %9.5f %%\n",
          num_STATS_GC, percent);

  percent = (num_STATS_UK * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, unknown (hydroclass)                 %6ld %9.5f %%\n",
          num_STATS_UK, percent);

  percent = (num_STATS_HIGH_KDP_BLOCK * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, blocked %% >= Kdp_Block_Thresh (70 %%) %6ld %9.5f %%\n",
          num_STATS_HIGH_KDP_BLOCK,  percent);

  percent = (num_STATS_IS_EXCLUDED * 100.0) / subtotal;
  fprintf(stderr, 
          "try again, in an exclusion zone                 %6ld %9.5f %%\n",
          num_STATS_IS_EXCLUDED,  percent);

  percent = (num_not_checked * 100.0) / subtotal;
  fprintf(stderr, 
          "bins not checked                                %6ld %9.5f %%\n",
          num_not_checked,  percent);

  fprintf(stderr, "                                                ------\n");
  fprintf(stderr, "                                                %6ld\n",
          subtotal);

} /* end print_stats() =============================================== */
