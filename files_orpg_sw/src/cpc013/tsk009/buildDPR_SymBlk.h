/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:12:06 $
 * $Id: buildDPR_SymBlk.h,v 1.2 2009/03/03 18:12:06 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef PRODSYMBLK_H
#define PRODSYMBLK_H

/*** System Include Files ***/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <rpgp.h>
#include <assert.h>
#include <rpc/xdr.h>
#include <orpg_product.h>

/*** ORPG System Include Files for generic product format ***/

#include <packet_28.h>
#define FLOAT
#include "rpgcs_latlon.h"

#include <dp_Consts.h> /* MAX_BINS */

/* DPRSIZE 1346872 + 96 = buf size of 1346968 */

#define DPRCODE            176   /* Product code/id                          */
#define DPRSIZE        1346872   /* Output buffer size in bytes, no compress */
#define NUM_COMP             1   /* Number of component                      */
#define NUM_PROD_PARAM       0   /* Number of product parameters             */
#define NUM_COMP_PARAM       0   /* Number of product component parameters   */

#endif /* PRODSYMBLK_H */
