/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/02/07 19:31:11 $
 * $Id: layer_info.h,v 1.3 2003/02/07 19:31:11 christie Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* layer_info.h */

/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/02/07 19:31:11 $
 * $Id: layer_info.h,v 1.3 2003/02/07 19:31:11 christie Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef _LAYER_INFO_H
#define _LAYER_INFO_H

/**Solaris 8 change**/
#include <stdlib.h>

#include "global.h"

#define FALSE 0
#define TRUE 1

/* prototypes */

void delete_layer_info(layer_info *linfo, int size);
layer_info *copy_layer_info(layer_info *linfo, int size);


#endif
