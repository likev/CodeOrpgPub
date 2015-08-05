
/************************************************************************
Module:         mda3d_attributes.h

Description:    include file for the mda3d_attributes.c.  
                This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef MDA3D_ATTRIBUTES_H 
#define MDA3D_ATTRIBUTES_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include "mda3d_parameter.h" 


/* declare functions */
void mda3d_msi(int num_elev, float elev_val[], int i, int n, new_cplt_t new_cplt[]);
float mda3d_density(float input_height);
void mda3d_rank(int i, int n, int core_rank, float depth_thresh,
	float *base, float *top, float *depth, new_cplt_t new_cplt[]);

#endif
