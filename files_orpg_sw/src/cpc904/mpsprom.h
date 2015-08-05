/*   @(#) mpsprom.h 99/12/23 Version 1.2   */
/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1992 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 
 
       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
   
       UconX Corporation
       San Diego, California

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAY-97  LMM  Obsoleted defs for P131SLAVESIZE & P330SLAVESIZE
*/

#ifndef	_mpsprom_h
#define	_mpsprom_h


/* Starting execution address for this board */

struct  X_ADDR
{
   bit32        valid_word;
   bit32        ( *exec_addr ) ( );
};

struct DNL_TYPE
{
   struct       X_ADDR  x_addr;
   bit32        dxr;            /* download exchange region */
#define         ilevel  dxr     /* interrupt the host at this level */
#define         laddr   dxr     /* begin loading at this on-board address */
#define         lsize   dxr     /* size of this download block */
#define         iaddr   dxr     /* init the on-board system at this address */
#define         breply  dxr     /* board's reply to the command */
#define         ixchg   dxr     /* IN HOST'S MEMORY ONLY */
#define PROM_ACK        1
#define PROM_NAK        2
   bit32        ivect;          /* interrupt the host with this vector */
   bit32        naddr;          /* address where next block of data goes */
   char         dblock [ 1024 ];/* buffer where data is host data is found */
};

#define         DNL_OFFSET      0x1200

/* Commands for the download isr from the  host */

#define         SET_ILEVEL      1
#define         SET_LOAD        2
#define         PUT_BLOCK       3
#define         INIT_SLAVE      4
#define         SELF_INT       -1


/* Slave addressing information */

/* This parameter actually specifies the distance between VMEbus addresses
 * for a UconX Controller, beginning at bus address zero.  In other words,
 * Controller N appears at bus address (0 + SLAVESIZE * CONTROLLER_NUMBER).
 * In order to deal with all Controllers in an orthogonal manner, and
 * because the SLAVESIZE indicated below allows 128 Mb for each Controller,
 * we will restrict all SLAVESIZEs to be the same value.
 */

#define		SLAVESIZE	0x08000000

#ifdef OBSOLETE
#define		P131SLAVESIZE	SLAVESIZE	/* PTI-VME131 slave bus size */
#define		P330SLAVESIZE	SLAVESIZE	/* PTI-VME330 slave bus size */
#endif

#endif	/* _mpsprom_h */
