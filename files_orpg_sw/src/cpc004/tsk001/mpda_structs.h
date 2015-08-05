/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:21:00 $
 * $Id: mpda_structs.h,v 1.5 2009/10/27 22:21:00 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*******************************************************************************

   mpda_structs.h

   PURPOSE:

   Contains globals for mpda code.

   DEFINITIONS:

   struct mpda_data - data structure containing the following

   short  final_vel       - array of final dealiased velocities (m/s*10)
   short  vel1            - array containing prf1 velocity values (m/s*10)
   short  vel2            - array containing prf2 velocity values (m/s*10)
   short  vel3            - array containing prf3 velocity values (m/s*10)
   unsigned short sw1     - array containing prf1 spectrum width values (UIF)
   unsigned short sw2     - array containing prf2 spectrum width values (UIF)
   unsigned short sw3     - array containing prf3 spectrum width values (UIF)
   unsigned short final_sw- array of final spectrum width values (UIF)
   short  ref_buff        - array holding ref data for output buffer
   short  hskp_buff       - array of header data for output
   short  prf2            - array to hold prf2 radial before alignment
   short  prf3            - array to hold prf3 radial before alignment
   unsigned short sw_rad  - array to hold radial of spectrum width before align
   unsigned short vel_limit-array to hold position of last vel bin on each rad
   int    az1             - array containing prf1 azimuth angle values (deg*100)
   int    az2             - value of prf2 azimuth angle values (deg*100)
   int    az3             - value of prf3 azimuth angle values (deg*100)
   unsigned short az_flag2- array to hold flag of whether radial of prf2 had
                            been aligned with prf1
   unsigned short az_flag3- array to hold flag of whether radial of prf3 had
                            been aligned with prf1
   short  prf1_nyq        - Nyquist vel of prf1 (m/s*10)
   short  prf1_twice_nyq  - Twice the nyquist vel of prf1 (m/s*10)
   int    prf1_unab_gates - number of prf1 unambigious gates
   int    prf1_n_dop_bins - number of doppler bins in prf1 radial
   short  prf2_nyq        - Nyquist vel of prf2 (m/s*10)
   short  prf2_twice_nyq  - Twice the nyquist vel of prf2 (m/s*10)
   int    prf2_unab_gates - number of prf2 unambigious gates
   short  prf3_nyq        - Nyquist vel of prf3 (m/s*10)
   short  prf3_twice_nyq  - Twice the nyquist vel of prf3 (m/s*10)
   int    prf3_unab_gates - number of prf3 unambigious gates
   int    ps_tot_prf1_rads- number of radials in 360 degrees of prf1
   int    rad_offset      - number of radials that overlap in prf1(past 360 deg)
   int    tot_prf1_rads   - number of radials for prf1
   int    prf1_number     - the number of the PRF used for prf1
   int    prf2_number     - the number of the PRF used for prf2
   int    prf3_number     - the number of the PRF used for prf3
   int    init_prf_arry   - Flag when TRUE indicates vol/elv restart
                            occurred and arrays must be reset

   unsigned short dp_data - array to hold extra base data fields (e.g., dual pol
   int dp_start		  - the starting memory location for the extra fields
   int dp_end		  - the ending memory location for the extra fields

*******************************************************************************/

/*
  We define the following macro to improve array index efficiency
*/
#define MAX_GATES_O   (MAX_GATES+40)
#include <basedata.h>
struct mpda_data 
  {
  short  final_vel[MAX_RADS][MAX_GATES_O];
  short  vel1[MAX_RADS][MAX_GATES_O];
  short  vel2[MAX_RADS][MAX_GATES_O];
  short  vel3[MAX_RADS][MAX_GATES_O];
  unsigned short sw1[MAX_RADS][MAX_GATES_O];
  unsigned short sw2[MAX_RADS][MAX_GATES_O];
  unsigned short sw3[MAX_RADS][MAX_GATES_O];
  unsigned short final_sw[MAX_RADS][MAX_GATES_O];
  unsigned short ref_buff[MAX_RADS][MAX_REF_GATES];
  short  hskp_buff[MAX_RADS][BASEDATA_HD_SIZE];
  short  prf2[MAX_GATES_O];
  short  prf3[MAX_GATES_O];
  unsigned short sw_rad[MAX_GATES_O];
  unsigned short vel_limit[MAX_RADS];
  int    az1[MAX_RADS];
  unsigned short az_flag2[MAX_RADS];
  unsigned short az_flag3[MAX_RADS];
  int    az2;
  int    az3;
  short  prf1_nyq;
  short  prf1_twice_nyq;
  int    prf1_unab_gates;
  int    prf1_n_dop_bins;
  short  prf2_nyq;
  short  prf2_twice_nyq;
  int    prf2_unab_gates;
  short  prf3_nyq;
  short  prf3_twice_nyq;
  int    prf3_unab_gates;
  int    ps_tot_prf1_rads;
  int    rad_offset;
  int    tot_prf1_rads;
  short  prf1_number;
  short  prf2_number;
  short  prf3_number;
  }save;

int        num_prfs;        /* num of vel cuts so far in current elev   */
int        init_prf_arry[4];  /*  Flag indicating if arrays need 
                                  reinitializing due to an elevation restart  */

/*
  Structure for lookup table values
*/

struct tab
  {
  short  value[LOOKUP_TABLE_SQR];
  }lookup_table[MAX_LOOKUP_TABLES];

struct dp_data_t
  {
  unsigned short dp_data[MAX_RADS][4096];
  int dp_start[MAX_RADS];
  int dp_end[MAX_RADS];
  } save_dp;


short    uif_table[MAX_NUM_UIF+1];          /* lookup table to convert UIF */
int      prf_pairs_table[MAX_PRFS_SQR];     /* linear array to hold PRF pairs*/
short    old_resolution;                    /* holds old doppler resolution*/
short    scale_factor;                      /* scales vel from resolution */
float    max_uif_vel;                       /* maximum UIF velocity */
float    min_uif_vel;                       /* minimum UIF velocity */
float    sf_velocity;                       /* scales vel from resolution/FLOAT_10 */
short    sounding_for_use;		    /* Flag for if sounding is available
                                               and should be used */
short    vel_max_nyq;                       /* The number of the velocity cut
                                               with the highest nyquist */
