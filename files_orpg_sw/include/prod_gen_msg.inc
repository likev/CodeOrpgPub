C
C
C RCS info
C $Author: nolitam $
C $Locker:  $
C $Date: 2002/12/11 22:24:59 $
C $Id: prod_gen_msg.inc,v 1.6 2002/12/11 22:24:59 nolitam Exp $
C $Revision: 1.6 $
C $State: Exp $
C

/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

	The contents in this file are derived from prod_gen_msg.h. The 
        macros must be consistent with those defined there. Thus if 
        prod_gen_msg.h is modified, this file has to be updated 
        accordingly.

***********************************************************************/


**PROD_GEN_MSG

C	/* the following is used by FORTRAN programs */

      integer*4 PGM_CPU_LOADSHED
      integer*4 PGM_MEM_LOADSHED
      integer*4 PGM_SCAN_ABORT
      integer*4 PGM_DISABLED_MOMENT
      integer*4 PGM_ABORT_REMAIN_SCAN
      integer*4 PGM_TASK_FAILURE
      integer*4 PGM_REPLAY_DATA_UNAVAILABLE
      integer*4 PGM_INVALID_REQUEST
      integer*4 PGM_SLOT_UNAVAILABLE
      integer*4 PGM_INPUT_DATA_ERROR
      integer*4 PGM_TASK_SELF_TERMINATED

      parameter( PGM_CPU_LOADSHED = -1 )
      parameter( PGM_MEM_LOADSHED = -2 )
      parameter( PGM_SCAN_ABORT = -3 )
      parameter( PGM_DISABLED_MOMENT = -4 )
      parameter( PGM_ABORT_REMAIN_SCAN = -5 )
      parameter( PGM_TASK_FAILURE = -6 )
      parameter( PGM_REPLAY_DATA_UNAVAILABLE = -7 )
      parameter( PGM_INVALID_REQUEST = -8 )
      parameter( PGM_SLOT_UNAVAILABLE = -9 )
      parameter( PGM_INPUT_DATA_ERROR = -10 )
      parameter( PGM_TASK_SELF_TERMINATED = -11 )

/* */

