/*   @(#) xstpoll.h 99/12/23 Version 1.1   */
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
|
v                                                
-------------------------------------------------------------------------->*/


/*
 * Structure of file descriptor/event pairs supplied in
 * the poll arrays.
 */

struct xpollfd 
{
   MPSsd sd;                       /* STREAM desc to poll */
   short events;                   /* events of interest on fd */
   short revents;                  /* events that occurred on fd */
};

/*
 * Testable select events
 */
#define POLLSYS         0x80              /* fd is non-MPS */
#ifndef POLLIN
#define POLLIN          0x01              /* fd is readable */
#define POLLPRI         0x02              /* priority info at fd */
#define POLLOUT         0x04              /* fd is writeable (won't block) */

/*
 * Non-testable poll events (may not be specified in events field,
 * but may be returned in revents field).
 */

#define POLLERR         0x08             /* fd has error condition */
#define POLLHUP         0x10             /* fd has been hung up on */
#define POLLNVAL        0x20             /* invalid pollfd entry */
#endif
