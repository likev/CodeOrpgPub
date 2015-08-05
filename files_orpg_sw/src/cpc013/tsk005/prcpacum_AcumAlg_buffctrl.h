/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:43:39 $
 * $Id: prcpacum_AcumAlg_buffctrl.h,v 1.2 2008/01/04 20:43:39 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PRCPACUM_ACUMALG_BUFFCTRL_H
#define PRCPACUM_ACUMALG_BUFFCTRL_H

/* Global include file */
#include <prcprtac_main.h>
#include <a313hparm.h>

/* Local include file */
#include "prcprtac_file_io.h"

/* Declare function prototypes */
void init_acum_adapt(void);
void copy_in2out_buffer(PRCPRTAC_smlbuf_t*);
void fill_supl_missing(void);
void write_rate_hdr(int*);
void set_up_hdrs(int*);
void buffer_needs(int*);
void fill_supl_array(void);
void determine_period_acums(short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS],short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS]);
void determine_hourly_acum(short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS],short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS],int*);
void prepare_outputs(short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS],short[MAX_AZMTHS][MAX_RABINS],
       short[MAX_AZMTHS][MAX_RABINS],int*);

#endif
