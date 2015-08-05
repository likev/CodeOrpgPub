/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

#ifndef DP_S2S_ACCUM_FUNC_PROTOTYPES_H
#define DP_S2S_ACCUM_FUNC_PROTOTYPES_H

/******************************************************************************
    Filename: dp_s2s_accum_func_prototypes.h

    Description:
    ============
       Declare function prototypes for the Dual Pol Scan-to-scan accumulation
    algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    10/15/2007  0000       James Ward         Initial version.
******************************************************************************/

#include "dp_lib_func_prototypes.h" /* open_s2s_accum_data_store */
#include "dp_s2s_accum_Consts.h"    /* S2S_ACCUM_BUFSIZE         */
#include "dp_s2s_accum_types.h"     /* S2S_Accum_Buf_t           */

int copy_supplemental(Rate_Buf_t* old, Rate_Buf_t* new,
                      S2S_Accum_Buf_t* accum);

int   dp_compute_accum ( Rate_Buf_t* rate1, Rate_Buf_t* rate2,
                         S2S_Accum_Buf_t* accum_buf );

int   dp_s2s_accum_terminate(int signal, int flag);

#endif /* DP_S2S_ACCUM_FUNC_PROTOTYPES_H */
