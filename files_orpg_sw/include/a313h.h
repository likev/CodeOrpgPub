/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:13:03 $
 * $Id: a313h.h,v 1.5 2014/03/18 14:13:03 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef A313H_H
#define A313H_H

/*********************************************************************************

If wanting to increase the number of rate scans in an hour,
this file, a313h.inc, prcprtac_file_io.h and prcprate_initialize_file_io.c
need to change.

**********************************************************************************/
/*/**a3133p2 -----------------------------------------------*/
/*//version: 1*/

/* For scan arrays*/
#define MAX_BDBZ  256

/* For converting between dbz and its biased internal representation */
#define bdbz_indx_offset   2
#define bdbz_const_offset 32.
#define bdbz_cftr          2.
#define rinit    0.
#define iinit    0
#define ibeg     1

/*/**a3133c7 -----------  equivalent C file epre_main.h --------------*/
/*/**a3133c8 ---------------------------------------------------------*/
/*//version: 0*/

/* Refecivity index (0-256) conversion to equivalent reflectivity (z)
   lookup table*/
typedef struct {
  double zrefl[MAX_BDBZ];
}a3133c8_t;

/* Declare global object struct */
 a3133c8_t a3133c8;

/*/**a3134ca ----------------------------------------*/
/*//version: 0*/

/* Table for converting (indexed) reflectivity to precip rate (mmx10/hr)*/
typedef struct {
  short rate_table[MAX_BDBZ+1];  /*Corrected an array bounds error  */
}a3134ca_t;

/* Declare global object struct */
 a3134ca_t a3134ca;

/*/**a313hgsp ---------------------------------------*/
/*//version: 0*/

/* General sizing parameters local to rate*/
/* Rate scan bin sizing parameters...     */
#define r_bin_size 2
#define r_mid_ofs  1

/* Rate header record sizing parameters...*/
#define RAZ_HDR_FLDS 6

/* Constant value definitions...*/
#define INIT_VALUE 0


/*/**a313hbad ---------------------------------------*/
/*//version: 1*/

/* Bad scan array sizing parameters...*/
#define MAX_TSTAMPS 22		/* Set the the maximum rate scans in an hour. */
#define DT_AND_TM   2

/* Structure for bad scan array...*/
typedef struct {
  int ndate;
  int ntime;
}Bad_Tstamp_t;

/* Declare global object struct */
 Bad_Tstamp_t BadTstamp[MAX_TSTAMPS];

/* Constant values related to bad scan array...*/
/* Hourly conversion factors [seconds per hour; seconds per minute;
   seconds per day; minutes per hour; and hours per day*/
#define SEC_P_DAY 86400
#define SEC_P_HR  3600
#define SEC_P_MIN 60
#define MIN_P_HR  60
#define HR_P_DAY  24

/* Common area definition to contain time stamp array...*/
#define idate 0
#define itime 1

typedef struct {
  int date;
  int time;
}a313hbsc_t;

/* Declare global object struct */
 a313hbsc_t a313hbsc[MAX_TSTAMPS];  

/*/**a313hlfp ------------Repeated in A3146.H --------------*/
/*//version: 0*/

/* LFM flag value parameters...*/
#define BEYOND_RANGE -99
#define WITHIN_RANGE 0

/*/**a313hlfm -----------------------------------*/
/*//version: 0*/

#define MAX_RABINS 115

/* Structure of LFM area definition...*/
typedef struct {
  double bin_area[MAX_RABINS];
  double bin_range[MAX_RABINS];
}a313hlfm_t;

/* Declare global object struct */
 a313hlfm_t a313hlfm;  

/*/**a313hadp ----------------------------------*/
/*//version: 1*/

/* Adaptation data structure*/
typedef struct {
  double ad_cutoff_rng;
  double ad_c01;
  double ad_c02;
  double ad_c03;
  int   ad_minrat;
  int   ad_maxrat;
}a313hadp_t;

/* Declare global object struct */
 a313hadp_t a313hadp;

/*/**a313hgnd -----------------------------------*/
/*//version: 1*/

/* Rate and area array sizing parameters...*/
typedef struct {
  int flag_zero;
  int ref_flag_zero;
  int flag_bad;
  int ref_sc_good;
  int scan_date;
  int scan_time;
  int ref_sc_date;
  int ref_sc_time;
  int nbr_badscns;
  int ptr_badscn;
  int time_dif;
  int first_scan;
}a313hgen_t;

/* Declare global object struct */
 a313hgen_t a313hgen;

/*/**a313hhdr -------------------------------------*/
/*//version: 1*/

/* Header record i/o buffer definition and positional parameters...*/
/* Structure for rate header record...*/
typedef struct {
  int hdr_rflagz;
  int hdr_rfscdt;
  int hdr_rfsctm;
  int hdr_ptbdsc;
  int hdr_nobdsc;
  int hdr_rfscgd;
}Rate_Header_t;

/* Declare global object struct */
 Rate_Header_t RateHdr;

/*/**a3135h ---------------------------------------------*/
/*//version: 0002*/
/* Precipitation accumulation parameter file */
#define ACZ_PRD_FLDS   9  /* Total number of fields in each header of the
                             period header array. */
#define ACZ_HRLY_FLDS 11  /* Total number of fields in each header of the
                             hourly array.*/
#define ACZ_TOT_PRDS  26  /* Total number of headers in the period header 
                             array. (Max rate scan/hour + 4)*/
#define ACZ_TOT_HOURS  2  /* Total number of hours in the hourly array. */

/** Structure for Period Header array */
typedef struct {
  int p_beg_date;   /* Julian date for begin of accumulation period */
  int p_beg_time;   /* Begin time of accumulation period in seconds */
  int p_flag_zero;  /* Set - indicates no rain for accum period     */
  int p_flag_miss;  /* Set - if accum period is missing             */
  int p_end_date;   /* Julian date for end of accumulation period   */
  int p_end_time;   /* End time of accumulation period in seconds   */
  int p_delt_time;  /* Time difference between current and previous 
                       scans                                        */
  int p_frac;       /* Fractional part of the period that lies within
                       the hour [from 0 to 1000]                    */
  int p_spot_blank; /* Set - indicated spot blanked for accum perio */
}Period_Header_t;

/* Declare global object struct */
 Period_Header_t PerdHdr[ACZ_TOT_PRDS]; 
 
/* Structure for Hourly Headers array */
typedef struct {
  int h_beg_date;     /* Julian date for beginning of the hour      */
  int h_beg_time;     /* Beginning of hour in seconds from midnight */
  int h_flag_nhrly;   /* Set -  indicates no hourly totals          */
  int h_flag_zero;    /* Set -  indicates no rain within hour       */
  int h_end_date;     /* Julian date for ending of the hour         */
  int h_end_time;     /* End of hour in seconds from midnight       */
  int h_scan_type;    /* Defines how hour ends                      */
  int h_curr_prd;     /* Current index into period header array     */
  int h_max_hrly;
  int h_flag_case;    /* Type accum - extrapolate or interpolate    */
  int h_spot_blank;   /* Set - indicates one of the periods in hour
                         is spot blanked                            */
}Hour_Header_t;

/* Declare global object struct */
 Hour_Header_t HourHdr[ACZ_TOT_HOURS], HourlyHdr;
 
/* Definitions for current and previous hourly header*/
#define prev_hour 0
#define curr_hour 1

/* Value definitions to be used in concert with p_frac of period header array*/
#define MAX_PER_CENT 1000
#define MIN_PER_CENT    0

/* Parameters for updating scan data 115 x 360*/
#define FIRST_RADIAL 0  
#define FIRST_BIN    0

/* Hourly conversion factors [seconds per hour; seconds per minute;
   seconds per day; minutes per hour; and hours per day*/

/* Flags for accumulation method either extrapolate or interpolate*/
#define INTERP  0
#define EXTRAP  1

/* Flags for time conversions: hms_sec [hour, min, sec to sec]
   and sec_hms [sec to hour, min, sec] */
#define SEC_HMS  0
#define HMS_SEC  1

/* Value definitions for number of new periods added depending on whether
   acumulation method is for extrapolation or interpolation.  If interpolation,
   then only one new period added; else for extrapolation three new periods
   are added.
   1) interpolation
        a) n1 [23] added
   2) extrapolation
        a) n1 [23] added first
        b) n2 [24] added second
        c) n3 [25] added third
*****************************************************************************/
#define  n1 23				/* 1 + Max rate scans/hour */
#define  n2 24				/* 2 + Max rate scans/hour */
#define  n3 25				/* 3 + Max rate scans/hour */

/* Value definition for number of previous periods kept on disk */
#define NUM_PREV_PRD 22			/* Max rate scans/hour */

/* Flags for method of accumulating hourly totals either
   by addition or subtraction. */
#define ADD_HRLY_FLG 0
#define SUB_HRLY_FLG 1

/* Other needed values*/
#define INCR        1
#define DECR       -1
#define NULL0       0  
#define AVG_FACTOR  2
#define NPREV_PRD  23	/* 1 + Max rate scans/hour */

/* Structure for accumulation algorithm */
typedef struct {
  int cases;             /* Define method for period accumulations as
                            clear- method is interpolation
                            set  - method is extrapolation */
  int method;            /* Defines the method for hourly accumulations as 
                            clear- to add period accumulations 
                            set  - to add new periods and subtract 
                                  out old */
  int max_interp_tim;    /* Maximum time allowed for for interpolation
                            in seconds. */
  int gage_accum_tim;    /* Gage end accumulation [integer minutes]*/ 
  int min_tim_hrly;      /* Minimum time required in hour in order to
                            accumulate hourly totals. */
  int max_hrly_poss;     /* Maximum accumulation in hour. */
  int frst_in_outbuf; 
  int max_prcp_rate;     /* Maximum precipitation rate. */ 
  int max_acum_hrly;
  int thrsh_hrly_outli;  /* Maximum accumulation allowed in hour. */ 
  int max_day_no; 
  int min_day_no;
  int time_last_prcp; 
  int thrsh_restart;     /* Maximum time allowed between scans before
                            restart of accumulations. */  
  int scn_flag;          /* Defines rain state as 
                            clear - rain for current scan and past hour 
                            set   - no rain for current scan and past hour*/
  int acumnam; 
  int hbuf_empty;
  int current_index; 
  int new_frac[NPREV_PRD]; /* New fraction */
}BLKA_t;

/* Declare global object struct */
 BLKA_t blka;

/* Parameter for determining if i/o was successful...*/
#define IO_OK 0

#endif        /* #ifndef A313H_H */
