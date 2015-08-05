/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:34 $
 * $Id: orpgctl.h,v 1.37 2002/12/11 22:10:34 nolitam Exp $
 * $Revision: 1.37 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgctl.h

 Description: ORPG Control (ORPGCTL_) public header file.

 Assumptions:

 **************************************************************************/



#ifndef ORPGCTL_H
#define ORPGCTL_H

/* #if !defined(GCC) || !defined(SLRS_X86) */
#include <limits.h>            /* _POSIX_ARG_MAX, LOGNAME_MAX             */
/* #endif */

#ifndef LOGNAME_MAX
#define LOGNAME_MAX 128
#endif

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 *
 */

#define ORPGCTL_MAXCMDLEN	(_POSIX_ARG_MAX)

/*
 * RPG Start Modes
 */
#define ORPGCTL_NUM_START_MODES	8
#define ORPGCTL_CLEAN_START		0
#define ORPGCTL_CLEAR_START		1
#define ORPGCTL_QUICK_START		2
#define ORPGCTL_AUTO_START	    3
#define ORPGCTL_SPARE3_START	4
#define ORPGCTL_SPARE2_START	5
#define ORPGCTL_SPARE1_START	6
#define ORPGCTL_ADMIN_START		7

#define ORPGCTL_START_MODE_LEN	12
#define ORPGCTL_START_MODE_SIZ	((ORPGCTL_START_MODE_LEN) + 1)
#define ORPGCTL_CLEAN_START_STRING "clean"
#define ORPGCTL_CLEAR_START_STRING "clear"
#define ORPGCTL_QUICK_START_STRING "quick"
#define ORPGCTL_AUTO_START_STRING "auto"
#define ORPGCTL_SPARE3_START_STRING "spare3"
#define ORPGCTL_SPARE2_START_STRING "spare2"
#define ORPGCTL_SPARE1_START_STRING "spare1"
#define ORPGCTL_ADMIN_START_STRING "admin"

#define ORPGCTL_MIN_START_MODE_NDX	(ORPGCTL_CLEAN_START)
#define ORPGCTL_MAX_START_MODE_NDX	(ORPGCTL_ADMIN_START)
#define ORPGCTL_DFLT_START_MODE_NDX	(ORPGCTL_CLEAR_START)

/*
 * RPG Message Types ...
 */
typedef enum {ORPGCTL_MSGTYPE_RPGCMD=1} Orpgctl_msgtype_t ;

/*
 * RPG Message Sender IDs ...
 * NOTE: Sender IDs must be sequential (we loop on these)
 */
typedef enum {ORPGCTL_SENDER_ANY=0,
              ORPGCTL_SENDER_MSCF,
              ORPGCTL_SENDER_RMMS,
              ORPGCTL_SENDER_MRPG,
              ORPGCTL_SENDER_AGENT,
              ORPGCTL_SENDER_MNGRED,
              ORPGCTL_SENDER_ADMIN} Orpgctl_sender_t ;

#define ORPGCTL_SENDER_MIN (ORPGCTL_SENDER_MSCF)
#define ORPGCTL_SENDER_MAX (ORPGCTL_SENDER_ADMIN)


/*
 * RPG Message
 *
 * NOTE: Since each sender has their own LB message ID, we do not need
 *       a "sender ID" ...
 */

/*
 * At this time, we limit ourselves to 1024 bytes of data ...
 */
#define ORPGCTL_MAXMSG_DATASIZ	1024

typedef unsigned int Orpgctl_seqnum_t ;

typedef struct {
    Orpgctl_msgtype_t type ;
                               /** sequence number is per-sender          */
    Orpgctl_seqnum_t seqnum ;
                               /** POSIX message timestamp                */
    time_t time_psx ;
                               /** LOGNAME of user that sent message      */
    char logname[(LOGNAME_MAX)+1] ;
                               /** size of message data (bytes)
                                * message data (when present) appear after
                                * the size field ...
                                */
    size_t data_size_bytes ;
} Orpgctl_msg_t ;


/*
 * We use the LB message "tag" to ensure that no pending RPG Commands are
 * overwritten ...
 */
enum {ORPGCTL_RPGCMD_READ_TAG=1,
      ORPGCTL_RPGCMD_WRITE_TAG} ;

/*
 * ORPG RPG Command Event Message
 */

#define ORPGCTL_RPGCMD_MSG_SIZ (sizeof(Orpgctl_msg_t) + sizeof(Orpgctl_rpgcmd_t))

/*
 * ORPGCTL_pend_ (success) return values ...
 */
enum {ORPGCTL_PEND_NO=0,
      ORPGCTL_PEND_YES} ;

/*
 * Function Prototypes:
 */
void ORPGCTL_close(void) ;
Orpgctl_msg_t *ORPGCTL_recv_msg(Orpgctl_sender_t sender) ;
 int ORPGCTL_send_msg(Orpgctl_msgtype_t msgtype, Orpgctl_sender_t sender, ...) ;
 int ORPGCTL_pend_msg(Orpgctl_sender_t sender, Orpgctl_msgtype_t *msgtype,
                      Orpgctl_seqnum_t *seqnum) ;
                                  

#endif /* #ifndef ORPGCTL_H */
