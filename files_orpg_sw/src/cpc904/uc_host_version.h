/*   @(#) uc_host_version.h 99/12/23 Version 1.67   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1996 by
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
** Versions for UconX Host Drivers and API 
*/

#ifndef _uc_host_version_h
#define _uc_host_version_h

/* Windows NT PCI334 Host Driver */
#define MPS_NT_33X_VERSION  L"UconX Windows NT PCI Driver Version 2.4.2"

/* Solaris SBus/PCI 332/334/370 Host Driver */
#define MPS_33X_VERSION "UconX Sbus/PCIbus Driver Version 3.4.3"

/* VME Host Drivers */
#define MPS_SOLARIS_VERSION "UconX Solaris/SunOS VME Driver Version 3.0.3"
#define MPS_DGUX_VERSION    "UconX DGUX VME Driver Version 1.2" 
#define MPS_HPUX_VERSION    "UconX HPUX VME Driver Version 2.2"

/* Misc Drivers */
#define NIF_VERSION "UconX Network Interface (NIF) Driver Version 1.1"

/* API */
#define API_VERSION "UconX API Version 3.2.7"

#endif	/* _uc_host_version_h */
