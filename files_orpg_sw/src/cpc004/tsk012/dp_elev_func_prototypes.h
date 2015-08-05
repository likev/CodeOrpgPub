/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

#ifndef DP_ELEV_FUNC_PROTOYPES_H
#define DP_ELEV_FUNC_PROTOYPES_H

/******************************************************************************
    Filename: dp_elev_func_prototypes.h

    Description:
    ============
    Declare function prototypes for the DualPol Elevation algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    10/15/2007  0000       James Ward         Initial version.
******************************************************************************/

#include "dp_lib_func_prototypes.h"

void init_elevation(Compact_dp_basedata_elev* out,
                    int* num_out_of_range, int* num_duplicates);

void Add_radial(char* inbuf, Compact_dp_basedata_elev* cdbe,
                int* num_out_of_range,   int* num_duplicates);

/* void Add_moment_dehc (Generic_moment_t* moment, float* data); */

int  dp_precip_callback_fx( void* struct_address );

void save_radial(Base_data_header* inbuf);

int  dp_elev_prod_terminate( int signal, int flag );

#endif /* DP_ELEV_FUNC_PROTOYPES_H */
