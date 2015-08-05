/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:32:19 $
 * $Id: pulse_counts.h,v 1.2 2005/06/02 19:32:19 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
*/


#ifndef PULSE_COUNTS_H
#define PULSE_COUNTS_H

#include <stdio.h>

/* Global variables and macros. */
#define CLK_RATE      9600000.0
#define MIN_SURV_PRI  1 
#define MAX_SURV_PRI  3
#define MIN_DOP_PRI   3 
#define MAX_DOP_PRI   8
#define MIN_ALWB_PRI  4
#define MAX_ALWB_PRI  MAX_DOP_PRI

#define MAX_PRICNT    5
#define MIN_AZI_WIDTH 0.953
#define MAX_AZI_WIDTH 1.0555

#define BAMS_TO_DPS   (22.5/16384)

/* Function prototypes. */
int Pulse_cnt_CD( float az_rate, int pri );
int Pulse_cnt_CS( float az_rate, int pri );
int Pulse_cnt_BATCH( float az_rate, int spri, int spc, int dpri );

#endif
