/*
 * RCS info
 * $Author: 
 * $Locker:  
 * $Date: 
 * $Id: 
 * $Revision: 
 * $State: 
 */

#ifndef DP_ELEV_FUNC_PROTOYPES_H_
#define DP_ELEV_FUNC_PROTOYPES_H_

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

#include <dp_precip_adapt.h>
#include <dp_lib_func_prototypes.h>

void init_bad_radial( Compact_dp_radial* radial );
void init_elevation( Compact_dp_basedata_elev* out, 
                     Compact_dp_radial* bad_radial );
void DPE_build_product( void* input, void* output );
void extract_momentData( Generic_moment_t* output, Generic_moment_t* input );
void Add_moment_dfhc( Generic_moment_t* hd, short first_gate_range, 
                      short bin_size, unsigned short no_of_gates, float* data );
int  dp_precip_callback_fx( void* struct_address );
void save_radial( char* inbuf );

#endif /* DP_ELEV_FUNC_PROTOYPES_H_ */
