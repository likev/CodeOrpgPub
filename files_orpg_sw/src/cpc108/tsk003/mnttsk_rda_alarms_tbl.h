/**************************************************************************

 Module: mnttsk_pd_def.h

 Description: Product Distribution MNTTSK header file

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/02/05 22:34:36 $
 * $Id: mnttsk_rda_alarms_tbl.h,v 1.2 2004/02/05 22:34:36 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef MNTTSK_RDA_ALARMS_TBL_H
#define MNTTSK_RDA_ALARMS_TBL_H

#include <orpg.h>
#include <orpgrat.h>

#define STARTUP   1
#define RESTART   2
#define CLEAR     3


/* Function Prototypes */
int MNTTSK_RDA_ALARMS_TBL_maint( int maint_type ) ;


#endif /* #ifndef MNTTSK_RDA_ALARMS_TBL_H */
