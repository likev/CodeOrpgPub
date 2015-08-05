/*   @(#) sysmacros.h 99/12/23 Version 1.1   */
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

#ifndef	_xstra_sysmacros_h
#define	_xstra_sysmacros_h



/*
 * Some macros for units conversion
 */

/* a click is a page.  A segment is area mapped by a click of ptes (2Meg)*/
/* Core clicks to segments and vice versa */

#define ctos(x)		(((x) + (NCPS-1)) / NCPS)
#define	ctost(x)	((x) / NCPS)
#define	stoc(x)		((x) * NCPS)

/* XXX - what does this stand for? (byte to table physical?)*/
#define btotp(x) ((x)>>BPTSHFT)

#define stob(x)	(((unsigned)(x) & 0x7ff) << 21)
#define btos(x)	((unsigned)(x)>>21 & 0x7ff)

/* Core clicks to disk blocks */
#define	ctod(x) ((x)*NDPC)

/* inumber to disk address */
#ifdef INOSHIFT
#define	itod(x)	(daddr_t)(((unsigned)(x)+(2*INOPB-1))>>INOSHIFT)
#else
#define	itod(x)	(daddr_t)(((unsigned)(x)+(2*INOPB-1))/INOPB)
#endif

/* inumber to disk offset */
#ifdef INOSHIFT
#define	itoo(x)	(int)(((unsigned)(x)+(2*INOPB-1))&(INOPB-1))
#else
#define	itoo(x)	(int)(((unsigned)(x)+(2*INOPB-1))%INOPB)
#endif

/* clicks to bytes */
#ifdef BPCSHIFT
#define	ctob(x)	((x)<<BPCSHIFT)
#else
#define	ctob(x)	((x)*NBPC)
#endif

/* bytes to clicks */
#ifdef BPCSHIFT
#define	btoc(x)	(((unsigned)(x)+(NBPC-1))>>BPCSHIFT)
#define	btoct(x)	((unsigned)(x)>>BPCSHIFT)
#else
#define	btoc(x)	(((unsigned)(x)+(NBPC-1))/NBPC)
#define	btoct(x)	((unsigned)(x)/NBPC)
#endif

#ifdef INKERNEL

/* major part of a device internal to the kernel */

extern unsigned char MAJOR[256];
#undef major
#define	major(x)	(unsigned int)(MAJOR[(unsigned)((x)>>8)&0xFF])
#define	bmajor(x)	(unsigned int)(MAJOR[(unsigned)((x)>>8)&0xFF])

#else

/* major part of a device external from the kernel */
#undef major
#define	major(x)	(int)(((unsigned)x>>8)&0xFF)

#endif	/* INKERNEL */

/* minor part of a device */
#undef minor
#define	minor(x)	(int)(x&0xFF)

/* make a device number */
#undef makedev
#define	makedev(x,y)	(dev_t)(((x)<<8) | (y))

/*
 *   emajor() allows kernel/driver code to print external major numbers
 *   eminor() allows kernel/driver code to print external minor numbers
 */

#define emajor(x)	(int)(((unsigned)(x)>>8)&0xFF)
#define eminor(x)	(int)((x)&0xFF)

/*
 *  Evaluate to true if the process is a server - Distributed UNIX
 */
#define	server()	(u.u_procp->p_sysid != 0)

/*
 * bcd macros -- these macros only operate on 2 bcd digits (8 bits)
 */

#define bcd_to_dec(x)	( ((((x) >> 4) & 0xf) * 10) + ((x) & 0xf) )
#define dec_to_bcd(x)	( (((x) / 10) << 4) | ((x) % 10) )

/*
 * process priority calculation.  macro'ed for performance
 */
#define BOUND(x, min, max) \
	( (x)<(min) ? (min):  ((x)>(max)?(max):(x))  )

#define PRI(pp) \
	((((unsigned char)pp->p_cpu) >> 3) + \
	 (((unsigned char)pp->p_cpu) >> 4) + \
	 PUSER + pp->p_nice - NZERO)

/* USED TO BE:			(BOUND(pri(pp), 0, 127)) */
#define CALCCPRI(pp)		(PRI(pp) & 0x7f)

#endif	/* _xstra_sysmacros_h */
