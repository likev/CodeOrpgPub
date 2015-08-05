/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * x25_monitor.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25_monitor.h,v 1.2 2000/11/27 20:51:07 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history.

Chg Date	Init Description
 1.  8-Nov-00   djb  Initial version
 2. 21-Nov-00   djb  Added declarations for user-initiated restarts
*/

#ifdef NOTIFY_STATUS

#ifdef USER_RESTART /* #2 */
#define UREV_TRANSIENT_DXFER_OFF 0x00 
#define UREV_TRANSIENT_DXFER_ON  0x01
#define UREV_PVC_CAN_ATTACH      0x02
#endif /* USER_RESTART */

struct x25_statusmon {
  unsigned long snid;
  unsigned long state;
};

/* extern void x25_notify_status (struct lsformat *lsp); */
#ifdef NEED_THIS
extern void x25_notify_status(struct lsformat *lsp, int new_state, int update);
#endif /* NEED_THIS */

#define X25_STATMON_REQ   0x584D4F4E

#define SET_DLPI_STATE(a, b) {x25_notify_status (a, b, 1);}

#else

#define SET_DLPI_STATE(a, b) a->dlpi_state = b;

#endif /* NOTIFY_STATUS */
