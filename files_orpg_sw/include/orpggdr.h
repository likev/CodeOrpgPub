/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/08 17:26:31 $
 * $Id: orpggdr.h,v 1.6 2007/06/08 17:26:31 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef ORPGGDR_H
#define ORPGGDR_H

#include <generic_basedata.h>

#define ORPGGDR_RVOL        1
#define ORPGGDR_RELV        2
#define ORPGGDR_RRAD        3
#define ORPGGDR_DREF        4
#define ORPGGDR_DVEL        5
#define ORPGGDR_DSW         6
#define ORPGGDR_DRHO        7
#define ORPGGDR_DPHI        8
#define ORPGGDR_DZDR        9
#define ORPGGDR_DSNR        10
#define ORPGGDR_DRFR        11
#define ORPGGDR_DRF2	    12

/* The following types are derived fields from the dual pol preprocessor.  
   These are defined here in order to reserve the values.  Current the 
   ORPGGDR library does not recognize these are valid fields. */
#define ORPGGDR_DSMZ	    13
#define ORPGGDR_DSMV	    14
#define ORPGGDR_DKDP	    15
#define ORPGGDR_DSDP	    16
#define ORPGGDR_DSDZ	    17

/* The following type is user-defined type. */
#define ORPGGDR_DANY	    -1

/* Library function prototypes. */
char* ORPGGDR_get_data_block( char *rda_msg, int type );

#endif
