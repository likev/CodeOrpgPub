/**************************************************************************

      Module: mnttsk_pd_def.h

  Created by: Arlis Dodson

 Description: Product Distribution MNTTSK header file

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/26 20:56:41 $
 * $Id: mnttsk_pgt_def.h,v 1.3 2012/09/26 20:56:41 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MNTTSK__PGT_DEF_H
#define MNTTSK__PGT_DEF_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <orpg.h>

/**@#-*/ /*CcDoc Token Processing OFF*/

#define STARTUP   	1
#define RESTART   	2
#define CLEAR     	3

#define CURRENT    	1
#define DEFAULT_A  	2
#define DEFAULT_B  	4
#define ALL_TBLS  	16

#define CFG_NAME_SIZE	128
/*
 * Function Prototypes
 */
int MNTTSK_PGT_INFO_maint(int maint_type, int init_tables ) ;


#endif /* #ifndef MNTTSK_PGT_DEF_H */
/**@#+*/ /*CcDoc Token Processing ON*/
