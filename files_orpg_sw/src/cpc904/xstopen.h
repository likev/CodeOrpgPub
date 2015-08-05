/*   @(#) xstopen.h 99/12/23 Version 1.1   */
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
** Open Request structure 
**
** The first 11 characters of the protocal name are used.
*/

#define	MAXSERVERLEN	64
#define MAXPROTOLEN	11
#define MAXSERVNAMELEN	15

typedef struct oreq
{
   char serverName[MAXSERVERLEN+1];	/* Logical name of UconX Server     */
   char serviceName[MAXSERVNAMELEN+1];	/* Name of service to connect to    */
   unsigned short port;			/* Bind port (or 0 )		    */
   unsigned short pad;
   int  ctlrNumber;			/* Logical num of target controller */
   char protoName[MAXPROTOLEN+1];	/* Logical name of protocol Stream  */
   int  dev;
   int  flags;
   int  openTimeOut;			/* Timeout value in seconds */
} OpenRequest;

typedef int MPSsd;

