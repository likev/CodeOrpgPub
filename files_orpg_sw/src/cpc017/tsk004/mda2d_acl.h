/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:54 $
 * $Id: mda2d_acl.h,v 1.2 2003/07/11 19:17:54 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda2d_acl.h

Description:    include file for the mda2d_acl.c.  
                This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef MDA2D_ACL_H 
#define MDA2D_ACL_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include "mda2d_parameter.h" 


/* type defined */

  typedef struct {
  float range;
  float beg_azm;
  float end_azm;
  float beg_vel;
  float end_vel;
  float vel_diff;
  float shear;
  float maxgtgvd;
  float gtg_azm;
  float rank;
  float cs_vec_flag;
  } Shear_vect;

  typedef struct {
  float range;
  float beg_azm;
  float end_azm;
  float beg_vel;
  float end_vel;
  float vel_diff;
  float shear;
  float maxgtgvd;
  float gtg_azm;
  float rank;
  float cs_vec_flag;
  int id;
  int marked;
  } Shear_vect_2d;

  typedef struct {
  float ca;
  float cr;
  float cx;
  float cy;
  float ht;
  float dia;
  float rot_vel;
  float shr;
  float max_vd;
  float max_shr;
  float gtgmax;
  float rank;
  float avg_conv;
  float max_conv;
  float length;
  float length_org;
  float ratio;
  float azmin;
  float azmax;
  float lr;
  float hr; 
  } Feature_2D;

 typedef struct {
 float azm;
 float range_max;
 float range_min;
 float max_vel;
 float min_vel;
 float vel_diff;
 } Conv_vect;

 typedef struct note *NODEPTR;

 struct note {
 Shear_vect_2d data;
 NODEPTR next;
 };

/* declare functions */
 NODEPTR make_node(Shear_vect_2d);
 void print_list();
 int insert_list_in_order (Shear_vect_2d val, NODEPTR *L);
 void free_list(NODEPTR *L);
 void mda_features_compress(Feature_2D mda_sf[], Feature_2D mda_sf_com[],
             int mda_v_index[MESO_MAX_FEAT][MESO_MAX_NSV], 
	     int mda_v_index_com[MESO_MAX_FEAT][MESO_MAX_NSV], 
	     int nbr_mda_feat, int *nbr_mda_feat_com, int nbr_mda_rank);
 void mda2d_sort_byrank(Feature_2D mda_sf_com[], int nbr_mda_feat_com);
 void search_list (int k,
                 Shear_vect_2d feature[MESO_MAX_NSV],
                 int *length_feature);

#endif
