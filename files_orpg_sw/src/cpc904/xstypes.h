/*   @(#) xstypes.h 99/12/23 Version 1.8   */

/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1991 by
|
|              +++    +++                           +++     +++
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
|              +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
|              +++    ++++++      +++++ ++++++++++ +++ +++++   
|              +++    ++++++      +++++ ++++++++++ +++  +++    
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++ +++++   
|              +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
|              +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|               ++++++++                             +++    +++
|                ++++++         Corporation         ++++    ++++
|                 ++++   All the right connections  +++      +++
|
|
|      This software is furnished  under  a  license and may be used and
|      copied only  in  accordance  with  the  terms of such license and
|      with the inclusion of the above copyright notice.   This software
|      or any other copies thereof may not be provided or otherwise made
|      available to any other person.   No title to and ownership of the
|      program is hereby transferred.
|
|      The information  in  this  software  is subject to change without
|      notice  and  should  not be considered as a commitment by UconX
|      Corporation.
|  
|      UconX Corporation
|      San Diego, California
|                                               
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  10-FEB-96  LMM  Redefined bit32 from "long" to "int" 
2.  13-OCT-97  mpb  Windows NT needs a few types defined.
3.  05-nov-97  pmt  SNTP timestamping stuff has been dumped here.
4.  24-SEP-97  mpb  Windows NT needs a few more types defined.
*/

/*
** This header file contains types used by xSTRa internal sources and that
** are unique to that environment and will therefore not conflict with any
** host based types.h definitions.
**
** This include file is intended to be used when compiling code which will
** be run on the client platform and which intends to interface to the
** UconX server environment.
*/

#ifndef _xstypes_h
#define _xstypes_h



#ifndef NULL
#define	NULL 0
#endif


typedef unsigned char   bit8;
typedef unsigned short  bit16;
typedef unsigned int    bit32;		/* #1 */
#define BITS_DEFINED

/* #2, #4 */
#ifdef WINNT
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;
#endif /* WINNT */


/* The CP or "CopyPointer" macro is used to duplicate a pointer value into
 * another pointer, while suppressing any type conversion warnings by the
 * compiler.  The user of this macro assumes all responsibility for the
 * type equivalencies; this macro is intended only to save typing and compiler
 * or linter hassles.
 *
 * Pointer value S is copied to pointer value D.
 */

#ifdef typeof
#define CP(D,S) { (D) = (typeof(D))(S); }
#else
#define CP(D,S) { (char *)(D) = (char *)(S); }
#endif


/* Added for NTP support. On a Unix machine, this would be in systime.h 
   By the way, it's named utimeval because TCP has its own named timeval. pmt */
struct utimeval
{
    int tv_sec;   /* Seconds since Jan. 1, 1970 */
    int tv_usec;  /* Microseconds. Our clock resolution is 1 Millisec. */
};
 
/* Macros for adding or subtracting two timeval structures */
#define ADD_TIMEVAL(x, y)     x.tv_sec += y.tv_sec;  \
                              x.tv_usec += y.tv_usec; \
                              if (x.tv_usec >= 1000000) { \
                                 x.tv_sec++;  \
                                 x.tv_usec -= 1000000; \
                              }
#define SUBTRACT_TIMEVAL(x, y)   x.tv_sec -= y.tv_sec;  \
                              x.tv_usec -= y.tv_usec; \
                              if (x.tv_usec < 0) { \
                                 x.tv_sec--;  \
                                 x.tv_usec += 1000000; \
                              }
 

#endif	/* _xstypes_h */
