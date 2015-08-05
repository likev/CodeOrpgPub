/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/12 18:06:10 $
 * $Id: mnttsk_vcp.h,v 1.5 2012/09/12 18:06:10 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef MNTTSK_VCP_DEF_H
#define MNTTSK_VCP_DEF_H

#include <vcp_cs.h>
#include <orpg.h>
#include <dirent.h>
#include <translate.h>
#include <prfselect_buf.h>
#include <orpgsite.h>


#define STARTUP   1
#define RESTART   2
#define CLEAR     3

static int MNTTSK_VCP_tables( int startup_action, int vcp_num );
static int MNTTSK_VCP_translation_tables( int startup_action );
static int MNTTSK_PRF_commands( int startup_action );
static int MNTTSK_init_RDA_RDACNT( int startup_action );

#endif 
