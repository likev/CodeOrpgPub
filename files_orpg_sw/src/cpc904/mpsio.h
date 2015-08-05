/*   @(#) mpsio.h 99/12/23 Version 1.14   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1993 by
|
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ +++   ++ ++++ ++++ ++++  +++ ++ +++  
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    +++ +++   ++ ++++ ++++ +++  ++++ ++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|                ++++++         Corporation          +++    +++
|                 ++++   All the right connections  ++++    ++++
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
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  13-MAR-97  LMM  Added CTRL define (required for DG/UX driver only)
2.  14-MAR-97  LMM  Added mps_ctl_t struct (for DG/UX driver only)
3.  13-OCT-97  mpb  Reset structure for embedded NT driver.
4.  11-AUG-98  lmm  Added defines for HOST_BNUM and SOFTRESET
5.  01-JUN-99  mpb  Added defines and structs to support controller info.
*/

#define HOST_BNUM 15	/* Host board number on server */	/* #4 */ 

/* #5 */

typedef struct
{
   int ctlr_num;
   int dev_id;
} board_info;

typedef struct
{
   int total;
   board_info info [ 32 ];  /* MPSNDEV is 32 in mps.h */
} mps_ctlr_t;

/*
** Raw mode structures
*/
typedef struct 
{
   int bnum;
} mps_nmi_t;

typedef struct 
{
   int bnum;
   char btype[20];
} mps_reset_t;

/* #3 */
typedef struct 
{
   int bnum;
} mps_ptbug_t;

typedef struct
{
   int bnum;
   int dladdr;
   int dsize;
#ifdef NO_STREAMS
   char data[1024];
#endif
} mps_load_t;

typedef struct
{
   int bnum;
   int iaddr;
} mps_exec_t;

typedef struct
{
   int bnum;
} mps_ctl_t;			/* #2 */

typedef struct
{
   int   devnum;
   char  version[8];
   u_int write;
   u_int read;
   u_int max_read;		/* max # of descriptors read at one time */
   u_int upstream;
   u_int wrblock;
   u_int rdblock;
   u_int nostrbufs;
   u_int nodesc;
   u_int intr_unclaimed;
   u_int wrenable;
   u_int wrunblock;
} mps_stats_t;

/* Per stream structure containing stream level information */

#ifdef NO_STREAMS
/*
 * Struct for clone raw device.
 */
typedef struct
{
   int minor_num;
   char name[20];
} mps_clone_t;
#endif

/*
** Raw mode (download and statistics) commands
*/
#ifdef NO_STREAMS
#define RESET		_IOW('U', 0, mps_reset_t)
#define PUTBLOCK	_IOW('U', 1, mps_load_t)
#define EXEC		_IOW('U', 2, mps_exec_t)
#define GETSTATS	_IOWR('U', 3, mps_stats_t)
#define CLEARSTATS	_IOWR('U', 4, mps_stats_t)
#define CLONE		_IOWR('U', 5, mps_clone_t)
#else

/* 
 * ioctl values must be shorts.  TCP API passes this via the uclid flags 
 * field which is a short.
 */

#define MPS		('U' << 8)

/* normal */
#define RESET		(MPS | 0x00)
#define PUTBLOCK	(MPS | 0x01)
#define EXEC		(MPS | 0x02)
#define PTBUG		(MPS | 0x03)
#define SOFTRESET	(MPS | 0x04)  /* #4 */

/* special */
#define GETSTATS	(MPS | 0x80)
#define CLEARSTATS      (MPS | 0x81)
#define NMI		(MPS | 0x82)
#define CTRL		(MPS | 0x83)	/* #1 - DG/UX Driver only */

#define CTLR_INFO       (MPS | 0x85)    /* #5 */

#endif
