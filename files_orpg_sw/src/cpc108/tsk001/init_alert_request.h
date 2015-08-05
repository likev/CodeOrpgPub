/**************************************************************************

      Module: init_alert_request.h

 **************************************************************************/

/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/02/09 23:27:35 $
 * $Id: init_alert_request.h,v 1.1 2005/02/09 23:27:35 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef INIT_ALERT_REQUEST_H
#define INIT_ALERT_REQUEST_H

#include <orpg.h>

#define STARTUP   1
#define RESTART   2
#define CLEAR     3

/*
 * Function Prototypes
 */
int Init_alert_request(int maint_type) ;


#endif /* #ifndef INIT_ALERT_REQUEST_H */
