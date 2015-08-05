 /**************************************************************************

      Module: mda3d_input.h

 Description: The new_cplt_t structure MUST match that defined
              in the mda3d header file.
 **************************************************************************/


#ifndef MDA3D_INPUT_H
#define MDA3D_INPUT_H

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

typedef struct {
   int   tilt_num;
   int   event_id;
   int   height;
   int   diam;
   int   rot_vel;
   int   shear;
   int   gtgmax;
   int   rank;
   float ca;
   float cr;
} time_height_xs_t;

#endif
