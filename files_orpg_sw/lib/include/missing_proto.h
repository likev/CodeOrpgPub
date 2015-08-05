
/***********************************************************************

    File: missing_proto.h

    Description: The file defines missing system function prototypes.

***********************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2003/11/14 16:08:41 $
 * $Id: missing_proto.h,v 1.11 2003/11/14 16:08:41 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#ifndef MISSING_PROTO_H

#define MISSING_PROTO_H


#ifdef GCC
/*
    #undef va_start
    #define va_start(a,b) (a = (void *)((char *)&b + \
	((sizeof (b) + (sizeof (int) - 1)) & ~(sizeof (int) - 1))));
    #undef va_arg
    #define va_arg(list, mode) \
	((mode *)(list = (void *)((char *)list + sizeof (mode))))[-1]
*/


extern int ftok(const char *path, int id);
extern int getpagesize(void);
/* extern int gethostname(char *name, size_t namelen); */
#endif


#endif  /* MISSING_PROTO_H */


