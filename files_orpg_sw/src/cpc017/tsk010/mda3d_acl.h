/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:20:13 $
 * $Id: mda3d_acl.h,v 1.4 2005/03/03 22:20:13 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda3d_acl.h

Description:    include file for the mda3d_acl.c.  
                This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef MDA3D_ACL_H 
#define MDA3D_ACL_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include "mda3d_parameter.h" 


/* type defined */

  typedef struct {
  float ca;
  float cr;
  float cx;
  float cy;
  float ht;
  float dia;
  float rot_vel;
  float shr;
  float gtgmax;
  float rank;
  float avg_conv;
  float max_conv;
  } feature2d_t;

  typedef struct {
  float ca;
  float cr;
  float cx;
  float cy;
  float ht;
  float dia;
  float rot_vel;
  float shr;
  float gtgmax;
  float rank;
  float avg_conv;
  float max_conv;
  float elevation;
  int couplet_id;
  int sweep_num;
  } cplt_t;

  typedef struct {
  int tilt_num;
  int couplet_id;
  int ht;
  int dia;
  int rot_vel;
  int shr;
  int gtgmax;
  int rank;
  float ca;
  float cr;
  } mda_th_xs_t;

  typedef struct {
  int   meso_id;
  float base;
  float top;
  float depth;
  float low_level_dia;
  float max_level_dia;
  float low_level_rot_vel;
  float max_level_rot_vel;
  float low_level_shear;
  float max_level_shear;
  float low_level_gtg_vel_diff;
  float max_level_gtg_vel_diff;
  int   cir_type;
  float height_max_level_dia;
  float height_max_level_rot_vel;
  float height_max_level_shear;
  float height_max_level_gtg_vel_diff;
  float ll_center_azm;
  float ll_center_rng;
  float ll_center_x;
  float ll_center_y;
  float core_base;
  float core_top;
  float core_depth;
  int ll_elev_sweep;
  float msi;
  int msi_rank;
  float nssl_base;
  float nssl_top;
  float nssl_depth;
  float storm_relative_depth;
  int low_toped_meso;
  float user_defined_base;
  float user_defined_top;
  float user_defined_depth;
  int user_meso;
  float center_azm;
  float center_rng;
  float low_level_convergence;
  float mid_level_convergence;
  int strength_rank;
  float vert_integ_rot_vel;
  float vert_integ_shear;
  float vert_integ_gtg_vel_diff;
  float delt_v_slope;
  float trend_delt_v_slope;
  int topped;
  } new_cplt_t;

/* declare functions */
void mda3d_attributes(int num_elev, float elev_val[], 
                int *nbr_new_cplts, new_cplt_t new_cplt[]);

#endif
