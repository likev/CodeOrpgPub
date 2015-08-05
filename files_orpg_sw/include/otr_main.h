/**************************************************************************
   
   Module:  otr_main.h
   
   Description: 
      This contains the constant OTR_PSERVER_NUMBER that is used
      by both ps_onetime and ps_routine.  This is the number of
      the simulated p_server that passes one time generation requests
      to ps_routine just as a normal p_server would.  When the
      requests are fulfilled, ps_onetime automatically sends a
      message deleting the requests to ps_routine, again with this
      p_server id.

   Assumptions:
      This uses the maximum LB message ID as the p_server id.  This
      assumes that no p_server is running with this ID.  This is safe
      as p_server ids start at 0, and will have at least one comm channel
      per p_server.
   
   **************************************************************************/
/*
* RCS info
* $Author: hoytb $
* $Locker:  $
* $Date: 2001/05/04 15:36:34 $
* $Id: otr_main.h,v 1.3 2001/05/04 15:36:34 hoytb Exp $
* $Revision: 1.3 $
* $State: Exp $
*/

#ifndef OTR_MAIN_H

#define OTR_MAIN_H
/*
* System Include Files/Local Include Files
*/
#include <lb.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
/* the p_server number for one time requests sent to ps_routine*/
#define OTR_PSERVER_NUMBER LB_MAX_ID

/*
* Static Globals
*/
/*
* Static Function Prototypes
*/

#endif
