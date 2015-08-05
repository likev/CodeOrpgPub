/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/30 22:17:53 $
 * $Id: mdattnn.h,v 1.3 2011/09/30 22:17:53 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/************************************************************************
Module:         mdattnn.h

Description:    include file for the mdattnn process.  
      
************************************************************************/

#ifndef MDATTNN_H 
#define MDATTNN_H

#include "mdattnn_params.h"
#include "trfrcatr.h"
#include "mdattnn_data.h"
#include <stdio.h>

void readInput_at_firstTilt(int* num_sf, int* overflow_flg, int* elev_time,
                            feature2d_t* feature_2d, char* inbuf);
void tracking_at_firstTilt(int num_sf, feature2d_t* feature_2d,
			   int new_time, int new_date, float elevation,
			   float mda_def_u, float mda_def_v,
			   FILE* output_ttnn, FILE* output_fort34,
                           int* nbr_first_elev_newcplt,
			   cplt_t first_elev_newcplt[MAX_MDA_FEAT],
			   int* elev_time);
void mda_ru_sort(float n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT],
                 int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT], int num_features);
void mda_ru_extrapolate(int* iadd, int addnew[], 
			int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT],
			float elevation, int num_features, int first_tilt); 
void mda_ru_downgrading_prevention(cplt_t new_cplt[], int nbr_new_cplts,
                        cplt_t first_elev_newcplt[], int nbr_first_elev_newcplt);
void mda_sort(const int flag, cplt_t cplt[MAX_MDA_FEAT], const int n_cplts);
int  getTimeDiff(const int new_time,
                 const int new_date,
                 const int old_time,
                 const int old_date);
int  readMda3dInput(cplt_t* new_cplt, int* elev_time, 
                    int *vol_num, int* overflow_flg, char* inbuf);
int  readTdaInput(polar_loc_t* tvs);
int  makeTVSAssociation(const cplt_t*      new_cplt,
                        const polar_loc_t* tvs,
                        const int          nbr_tvs);
void makeMesoAssociations(const cplt_t   new_cplt,
                          const cplt_t   Old_cplt[],
                          const int      Nbr_Old_cplts,
                          const int      time_diff,
                                float    n2o_dist[],
                                int      n2o_order[]);
void getAverageMotion(const cplt_t*  Old_cplt,
                      const int      Nbr_Old_cplts,
                      const float*   avg_mu,
                      const float*   avg_mv);
void readSCITInput(const float* avg_SCIT_spd,
                   const float* avg_SCIT_dir);
void assignStormId(cplt_t* ptrCplt);
#endif



