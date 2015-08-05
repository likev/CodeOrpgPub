/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:44:04 $
 * $Id: prcprate_rate_scan_ctrl.h,v 1.1 2005/03/09 15:44:04 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef RATE_SCAN_CTRL_H
#define RATE_SCAN_CTRL_H

/* Declare function prototypes */

void init_precip_table(void);
void init_rate_adapt(void);
void read_header_recd(int *iostatus);
void avg_hybscn_pairs(void);
void update_bad_scans(int *iostatus);
void range_effect_correc(void);
void lfm4_map(void);

#endif
