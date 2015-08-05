/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:14 $
 * $Id: mpda_vcp_info.h,v 1.2 2003/07/17 15:08:14 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*  define a structure for mpda vcps  */

#include <vcp.h>

typedef struct {
    unsigned short cuts_per_elv[ECUTMAX];
    unsigned short cut_mode[ECUTMAX];
    unsigned short vel_num[ECUTMAX];
    unsigned short last_scan_flag[ECUTMAX];
    unsigned short prf_num[ECUTMAX];
} mpda_vcp_t;

mpda_vcp_t vcp;

int range_table[DELPRI_MAX][PRFMAX];   /* holds radar unambiguous ranges*/
