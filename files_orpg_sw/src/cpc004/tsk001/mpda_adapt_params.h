/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:06 $
 * $Id: mpda_adapt_params.h,v 1.2 2003/07/17 15:08:06 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*
   All parameter definitions are explained in the get_adapt_params
   routine
*/

int   th_overlap_size;
int   th_overlap_relax;
int   max_az_bin_cnt;
int   max_rad_bin_cnt;
float seed_unf;
int   th_seed_chk;
int   gates_back;
int   gates_for;
short th_qc_chk;
short th_fst_tplt_chk;
short max_rad_jmp;
float mpda_tover;
short min_trip_fix;
short max_trip_fix;
int   max_delta_az;
short use_sounding;

/*
  Derived adaptable parameters
*/
short seed_unf_prf1;
short seed_unf_prf2;
short seed_unf_prf3;
