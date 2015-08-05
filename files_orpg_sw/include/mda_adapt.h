/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:46 $
 * $Id: mda_adapt.h,v 1.9 2007/01/30 22:56:46 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#ifndef MDA_ADAPT_H
#define MDA_ADAPT_H


#define MDA_DEA_NAME "alg.mda"


/****************************************************
 *  Mesocyclone Detection Algorithm Adaptation Data *
 ****************************************************/
 
/*** Refer to file dea/mda.alg for descriptions of these structure members ***/    

typedef struct
   {
   int    min_refl;    
   int    overlap_filter_on;				
   int    min_filter_rank;
   float  beam_width;
   int    meso_min_nsv;
   int    meso_vs_rng_1;
   int    meso_vs_rng_2;
   int    meso_max_vect_len;
   int    meso_max_core_len;
   float  meso_max_ratio;
   float  meso_min_ratio;
   float  meso_min_radim;
   int    meso_max_dia;
   float  meso_2d_dist;
   int    meso_min_rng;
   int    v_d_th_lo;
   int    v_d_th_hi;
   int    conv_max_lookahd;
   float  conv_rng_dist;
   int    meso_conv_buff;
   int    meso_ll_conv_ht;
   int    meso_ml_conv_ht1;
   int    meso_ml_conv_ht2;
   int    mda_no_shallow;
   int    meso_min_rank_shal;
   float  meso_min_depth_shal;
   int    meso_max_top_shal;
   } mda_adapt_t;
   
#endif /* MDA_ADAPT_H */
