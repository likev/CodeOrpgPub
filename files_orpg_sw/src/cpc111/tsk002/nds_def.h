
/***********************************************************************

    Description: Internal include file for nds (node service).

***********************************************************************/

/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2007/03/07 22:36:14 $
* $Id: nds_def.h,v 1.3 2007/03/07 22:36:14 jing Exp $
* $Revision: 1.3 $
* $State: Exp $
*/

#ifndef NDS_DEF_H
#define NDS_DEF_H


#define NDS_NAME_SIZE 128

int NDS_get_nds_fd ();
void NDS_resume_mrpg ();

int NDSPS_init ();
void NDSPS_update ();

#endif		/* #ifndef NDS_DEF_H */
