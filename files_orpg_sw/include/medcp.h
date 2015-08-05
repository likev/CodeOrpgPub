/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/25 17:01:31 $
 * $Id: medcp.h,v 1.2 2009/09/25 17:01:31 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************
    Module: medcp.h
    Description: This is the header file to medcp tool.
****************************************************************/


#ifndef MEDCP_H
#define MEDCP_H

enum { MEDCP_SUCCESS=0,
       MEDCP_DEVICE_NOT_FOUND=100,
       MEDCP_CHECK_MOUNT_FAILED,
       MEDCP_DEVICE_MOUNT_FAILED,
       MEDCP_DEVICE_UNMOUNT_FAILED,
       MEDCP_DEVICE_MISMATCH,
       MEDCP_MEDIA_NOT_DETECTED,
       MEDCP_MEDIA_NOT_MOUNTED,
       MEDCP_MEDIA_NOT_BLANK,
       MEDCP_MEDIA_NOT_WRITABLE,
       MEDCP_MEDIA_NOT_REWRITABLE,
       MEDCP_BLANKING_MEDIA_FAILED,
       MEDCP_MEDIA_WRITE_FAILED,
       MEDCP_MEDIA_VERIFY_FAILED,
       MEDCP_MEDIA_TOO_SMALL,
       MEDCP_CREATE_CD_IMAGE_FAILED,
       MEDCP_EJECT_T_FAILED
}; /* Exit codes */



#endif

