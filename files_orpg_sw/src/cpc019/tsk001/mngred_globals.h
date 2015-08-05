/********************************************************************************

      Module:  mngred_globals.h

      Description: This include file provides definitions and declarations 
                   of global variables.

 ********************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2001/02/08 21:26:09 $
 * $Id: mngred_globals.h,v 1.3 2001/02/08 21:26:09 garyg Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


#include <mngred.h>


#ifndef MNGRED_GLOBALS_H
#define MNGRED_GLOBALS_H

extern int errno;    /* system error number */


#ifdef MNGRED_MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN Channel_link_state_t CHAnnel_link_state; /* state of the RPG-RPG channel link         */
EXTERN Channel_state_t  CHAnnel_state;  /* RPG channel active/inactive state                 */
EXTERN Channel_status_t CHAnnel_status; /* the RPG & RDA status of this channel              */
EXTERN int INTerval_timer_expired;      /* flag denoting the interval timer expired          */
EXTERN int REDundant_channel_state;     /* active/inactive state of the redun channel        */
EXTERN int SENd_ipc_cmd;                /* flag specifying that an IPC command needs
                                           to be sent from the active channel to the 
                                           redundant channel                                 */
EXTERN int RDA_download_required;       /* inactive channel flag specifying that a RDA
                                           download is required                              */
EXTERN int CONfiguration_type;          /* the type of redundant configuration this site is
                                           configured as (ie. FAA redundant or NWS redundant)*/

#endif
