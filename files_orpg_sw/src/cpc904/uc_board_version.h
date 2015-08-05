/*   @(#) uc_board_version.h 99/12/23 Version 1.198   */

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
** Versions for UconX Board Side Drivers & Protocols 
*/

#ifndef _uc_board_version_h
#define _uc_board_version_h

/* BIF and HIF */
#define BIF_VERSION "UconX BIF Driver Version 4.1.1"
#define HIF_VERSION "UconX HIF Driver Version 4.2.1"
#define OLD_HIF_VERSION "UconX HIF Driver (DGUX) Version 1.1" 

/* TCP/IP */
#define TCP_VERSION "UconX TCP/IP Version 7.7.2"

/* xSTRa */
#define XSTRA_VERSION "UconX xSTRa Version 7.1.3"

/******************************************************************************/
/* PROTOCOLS & Drivers */
/******************************************************************************/

/* adt - CCS/ADDS protocol */
#define ADT_VERSION "UconX CCS/ADDS Protocol Version 2.1"

/* asy - Async Driver (ADX Protocol) */
#define ASY_IUSC_VERSION "UconX Async IUSC Driver Version 2.1"
#define ASY_QUICC_VERSION "UconX Async QUICC Driver Version 2.2.2"
#define ASY_SCC_VERSION "UconX Async SCC Driver Version 2.2"

/* bsc - Bisync Driver (BSC Protocol) */
#define BSC_SCC_VERSION "UconX Bisync SCC Driver Version 2.2"
#define BSC_QUICC_VERSION "UconX Bisync QUICC Driver Version 1.1"

/* ddcmp - DDCMP Protocol */
#define DDCMP_SCC_VERSION "UconX DDCMP SCC Driver Version 2.1.1"
#define DDCMP_QUICC_VERSION "UconX DDCMP QUICC Driver Version 1.4"

/* euromux - EUROCOM Multiplexor */
#define EUROMUX_VERSION "UconX EUROCOM Mux Version 1.1"

/* fdl - FAAD Data Link Protocol */
#define FDL_VERSION "UconX FDL Protocol Version 1.0"

/* frelay - Frame Relay Protocol */
#define FRELAY_VERSION "UconX Frame Relay Protocol Version 2.4"
#define SFRELAY_VERSION "UconX Frame Relay (Spider) Protocol Version 2.0.3"

/* habm - HDLC ABM Protocol */
#define HABM_VERSION "UconX HDLC ABM Protocol Version 2.4.1"

/* hdlc - HDLC Driver (HDLC DFX Protocol) */
#define HDLC_IUSC_VERSION "UconX HDLC IUSC Driver Version 3.2"
#define HDLC_QUICC_VERSION "UconX HDLC QUICC Driver Version 5.1.1"
#define HDLC_SCC_VERSION "UconX HDLC SCC Driver Version 3.3.1"
#define HDLC_T1_VERSION "UconX HDLC T1/E1 Driver Version 1.1.1"

/* hnrm - HDLC NRM Protocol */
#define NRM_VERSION "UconX HDLC NRM Protocol Version 2.2.3"

/* ibm3270 - IBM3270 Protocol */
#define IBM3270_VERSION "UconX IBM 3270 Protocol Version 1.0"

/* ifdt - IFDT Protocol */
#define IFDT_VERSION "UconX IFDT Protocol Version 1.0"

/* jplabm - HDLC ABM Protocol (JPL Version) */
#define JABM_VERSION "UconX HDLC ABM Protocol (JPL) Version 2.0.1"

/* jplhdlc - HDLC ABM Protocol (JPL Version) */
#define JHDLC_SCC_VERSION "UconX HDLC SCC Driver (JPL) Version 2.0.1"

/* lapb - HDLC LAPB Protocol */
#define LAPB_VERSION "UconX HDLC LAPB Protocol Version 4.3"

/* lapd - HDLC LAPD Protocol */
#define LAPD_VERSION "UconX HDLC LAPD Protocol Version 1.0"

/* mf2000 - Market Feed 2000 Protocol */
#define MF2000_VERSION "UconX Marketfeed 2000 Protocol Version 1.0"

/* msv2 - BSC Medium Speed Variant 2 (MSV2) Protocol */
#define MSV2_VERSION "UconX MSV2 Protocol Version 1.5"

/* rdr - Radar Receiver Protocol */
#define RDR_VERSION "UconX Radar Receiver Protocol Version 4.6"

/* sbsimux - SBSI Multiplexor */
#define SBSIMUX_VERSION "UconX SBSI Mux Version 3.2.1"

/* sbsi - SBSI Driver */
#define SBSI_IUSC_VERSION "UconX SBSI IUSC Driver Version 4.3"
#define SBSI_QUICC_VERSION "UconX SBSI QUICC Driver Version 5.3.1"
#define SBSI_SCC_VERSION "UconX SBSI SCC Driver Version 4.2"

/* sbsi protocol for Lockheed */
#define SBSI_LOCKHEED_VERSION "UconX Lockheed SBSI Protocol Version 1.0.1"

/* TADILB */
#define TADILB_VERSION "UconX TADILB Version 1.0"

/* LINK11 */
#define LINK11_VERSION "UconX LINK11 Version 1.0"

/* x25 - X.25 */
#define X25_VERSION "UconX.25 Version 3.1.1"
#define X25_V8_VERSION "UconX.25 Version 4.0.2"

/******************************************************************************/
/* PROM Versions */
/******************************************************************************/

#define ESCA131_PROM_VERSION "UconX Prom Version 1.2" 

#define PT131_PROM_VERSION "UconX Prom Version 1.5" 
#define PT151_PROM_VERSION "UconX Prom Version 1.5" 
#define PT161_PROM_VERSION "UconX Prom Version 1.0" 
#define PT330a_PROM_VERSION "UconX Prom Version 1.1" 
#define PT340_PROM_VERSION "UconX Prom Version 1.0" 
#define MPS300_600_PROM_VERSION "UconX Prom Version 3.7" 
#define PQ370_PROM_VERSION "UconX Prom Version 1.1" 
#define MPS800_PROM_VERSION "PTI Prom Version 1.1" 
#define PQBUG_PROM_VERSION "PQbug Version 1.1"

#endif	/* _uc_board_version_h */

