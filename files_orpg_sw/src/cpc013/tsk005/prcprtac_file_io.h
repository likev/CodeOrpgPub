/*
 * RCS info
 * $Author: cm $
 * $Locker:  $
 * $Date: 2010/09/14 21:39:28 $
 * $Id: prcprtac_file_io.h,v 1.3 2010/09/14 21:39:28 cm Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
#ifndef PRCPRTAC_FILE_IO_H
#define PRCPRTAC_FILE_IO_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <rpgc.h>
#include <lb.h>
#include <a313hparm.h>

#define ACUM_FN "HYACCUMS.DAT"             /* hydromet filename */       
char *LB_name;
LB_attr attr;
int fdlb; 

/* Header pointer flag values...*/
#define rathdr 1
#define prdhdr 2
#define hlyhdr 3

/* Reference rate scan and hourly accum. scan index pointer parameters */
#define ratscn 0
#define hlyscn 23	/* 1 + Max rate scans/hour */

#define orpg_rathdr_rec 1
#define orpg_prdhdr_rec 2
#define orpg_hlyhdr_rec 3
#define orpg_badscn_rec 4
#define orpg_ratscn_rec 5

/* IO function codes */
#define readLB  0
#define writeLB 1

typedef struct {
 short scan_data[360][115];
}ScanData_t;

/************* FUNCTION DECLARATIONS *********************/
int open_disk_file(void);
int Header_IO (int, int);
int Scan_IO (int, int, short[MAX_AZMTHS][MAX_RABINS]);
int Badscan_IO (int);

#endif
