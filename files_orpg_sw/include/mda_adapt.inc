C
C RCS info
C $Author: ryans $
C $Locker:  $
C $Date: 2006/02/28 16:55:42 $
C $Id: mda_adapt.inc,v 1.1 2006/02/28 16:55:42 ryans Exp $
C $Revision: 1.1 $
C $State: Exp $
C


/****************************************************
 *  Mesocyclone Detection Algorithm Adaptation Data *
 *   THIS MUST MATCH mda_adapt.h!!!!!               *
 ****************************************************/
 
/*** Refer to file dea/mda.alg for descriptions of these structure members ***/    

**mda_adapt

      integer*4    min_refl
      integer*4    overlap_filter_on
      integer*4    min_filter_rank
      real*4       beam_width
      integer*4    meso_min_nsv
      integer*4    meso_vs_rng_1
      integer*4    meso_vs_rng_2
      integer*4    meso_max_vect_len
      integer*4    meso_max_core_len
      real*4       meso_max_ratio
      real*4       meso_min_ratio
      real*4       meso_min_radim
      integer*4    meso_max_dia
      real*4       meso_2d_dist
      integer*4    meso_min_rng
      integer*4    v_d_th_lo
      integer*4    v_d_th_hi
      integer*4    conv_max_lookahd
      real*4       conv_rng_dist
      integer*4    meso_conv_buff
      integer*4    meso_ll_conv_ht
      integer*4    meso_ml_conv_ht1
      integer*4    meso_ml_conv_ht2
      integer*4    mda_no_shallow
      integer*4    meso_min_rank_shal
      real*4       meso_min_depth_shal
      integer*4    meso_max_top_shal

      common /mda_adapt_c/ min_refl,overlap_filter_on,
     1 min_filter_rank,beam_width,meso_min_nsv,
     2 meso_vs_rng_1,meso_vs_rng_2,meso_max_vect_len,
     3 meso_max_core_len,meso_max_ratio,meso_min_ratio,
     4 meso_min_radim,meso_max_dia,meso_2d_dist,
     5 meso_min_rng,v_d_th_lo,v_d_th_hi,conv_max_lookahd,
     6 conv_rng_dist,meso_conv_buff,meso_ll_conv_ht,
     7 meso_ml_conv_ht1,meso_ml_conv_ht2,mda_no_shallow,
     8 meso_min_rank_shal,meso_min_depth_shal,
     9 meso_max_top_shal
      
/*