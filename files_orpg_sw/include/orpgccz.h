/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/29 19:11:56 $
 * $Id: orpgccz.h,v 1.3 2009/05/29 19:11:56 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <orpg.h>
#include <deau.h>

#ifndef ORPGCCZ_H
#define ORPGCCZ_H

/* Macro Definitions. */
#define ORPGCCZ_LEGACY_ZONES      "ccz.legacy_zones"
#define ORPGCCZ_ORDA_ZONES        "ccz.orda_zones"
#define ORPGCCZ_DOWNLOAD_INFO     "ccz.download_info"
#define ORPGCCZ_DEFAULT           0
#define ORPGCCZ_BASELINE          1

/* Function Prototypes. */
int ORPGCCZ_get_censor_zones( char *id, char **buf, int baseline );
int ORPGCCZ_set_censor_zones( char *id, char *buf, int size, int baseline );
int ORPGCCZ_get_download_info( char **buf );
int ORPGCCZ_baseline_to_default( char *id );
int ORPGCCZ_default_to_baseline( char *id );
int ORPGCCZ_clear_edit_lock( char *id );
int ORPGCCZ_set_edit_lock( char *id );
int ORPGCCZ_get_edit_status( char *id );

#endif
