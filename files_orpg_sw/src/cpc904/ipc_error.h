/*   @(#)ipc_error.h	1.1	07 Jul 1998	*/
/*
 *  SpiderSTREAMS IPC Library - Error Interface
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  @(#)$Id: ipc_error.h,v 1.2 2000/07/14 15:56:55 jing Exp $
 */

#ifndef _ipc_error_h
#define _ipc_error_h

extern void ipc_set_program_name(char *progname);
extern void ipc_warning(char *format, ...);
extern void ipc_system_error(char *format, ...);
extern void ipc_memory_error(char *format, ...);
extern void ipc_user_error(int error, char *format, ...);

#endif /* _ipc_error_h */
