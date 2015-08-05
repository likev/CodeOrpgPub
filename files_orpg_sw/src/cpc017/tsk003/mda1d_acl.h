/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/12/03 20:47:38 $
 * $Id: mda1d_acl.h,v 1.4 2010/12/03 20:47:38 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda1d_acl.h

Description:    include file for the mda1d_acl.c.  
                This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef MDA1D_ACL_H 
#define MDA1D_ACL_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>


/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <rpgc.h>
#include <basedata.h>

/* type defined */

  typedef struct {
  double range;
  double beg_azm;
  double end_azm;
  double beg_vel;
  double end_vel;
  double vel_diff;
  double shear;
  double maxgtgvd;
  double gtg_azm;
  double fvm;
  double rank;
  double cs_vec_flag;
  } Shear_vect;

 typedef struct {
 double azm;
 double range_max;
 double range_min;
 double max_vel;
 double min_vel;
 double vel_diff;
 } Conv_vect;

/* declare functions */
void mda1d_data_preprocessing(Base_data_radial *base_data, double *azimuth,
        double *elevation, int *swp_nbr, int *ng_ref, int *ng_vel,
        double vel[],
        int *radial_status, int *radial_time, int *radial_date,int *azm_reso);

void mda1d_find_shear(double azm, double vel[], double old_azm, 
		double old_vel[], double range_vel[], int ng_vel, int naz,int azm_reso);
void mda1d_set_table(double range_vel[]);
void mda1d_finish_vectors(double ng_vel, double old_vel[], double old_azm,
		     double range_vel[], int naz, int azm_reso);
void mda1d_conv_vector (double vel[], double azm, double range_vel[], int ng_vrl,
		double conv_max_rgn_th);
void mda1d_vect_sort();
void conv_core_vector(double *max_vel, int *max_vel_index,
                      double *min_vel, int *min_vel_index,
                      double *vel_diff, double *shear,
                      int *discard_this_vector,
                      double range_vel[], double smooth_vel[]);

void mda1d_find_max_min (double smooth_vel[], int start_ind, int end_ind,
                         double *max_vel, double *min_vel,
                         int *max_vel_index, int *min_vel_index);
void mda1d_clean_duplicate();

void mda1d_search_core_shear(double range_vel[], int range_index,
                                double *cs_beg_vel, double *cs_beg_azm,
                                double *cs_end_vel, double *cs_end_azm,
                                double *cs_vel_diff, double *cs_shear,
                                double *cs_rank, double *cs_cart_length);

void mda1d_shear_attributes(double beg_vel, double end_vel,
                               double beg_azm, double end_azm, double rang_vel,
                               double *vel_diff, double *arc_length, double *shear);


#endif
