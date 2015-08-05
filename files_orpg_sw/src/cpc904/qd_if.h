/*   @(#) qd_if.h 99/12/23 Version 1.1   */
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


#ifndef	_qd_if_h
#define	_qd_if_h


/*****************************************************************************
 * qd_if.h
 *
 * Include file describing the UC driver interface and control operations.
 * The UC driver may be used for various support functions including download
 * and reset of Controller boards.  This include file describes the functions
 * and definitions that are passed between the QD driver and the controlling
 * applications or modules.
 * 
 *****************************************************************************/

/****
***** Include files
****/


/****
***** Definitions
****/



/****
***** Forward declarations
****/

/****
***** Global declarations
****/


/****
***** Local declarations
****/


/****
***** Definitions and code
****/

#define QD_IFMARK       (('Q'<<24)|('C'<<16)|('T'<<8)|'L')

#define	HOST_SLOT	(('H'<<24)|('O'<<16)|('S'<<8)|'T')


 /* Various structures specific to the individual commands that can be
  * performed using the QD control interface.  These structures form
  * the secondary part of the QD_Ctl structure defined below.
  */

typedef struct QD_SetloadS {	/* Structure passed for Setload operation */
    bit8  boardtype[20];	/*   Board typecode name (from qd_known_ltis) */
    bit32 text_addr;		/*   0 or addr for text rgn; returns actual */
    bit32 text_size;		/*   Size of text region or 0 if no text */
    bit32 data_addr;		/*   0 or addr for data rgn; returns actual */
    bit32 data_size;		/*   Size of data region or 0 if no data */
} QD_Setload;

typedef struct QD_PutblockS {	/* Structure passed for Putblock operation */
    bit32 textblock;		/*   Set to download to text region */
    bit32 blocksize;		/*   Size of this download block */
				/*   QD_Ctl struct followed by the data to
				 *      be downloaded... */
} QD_Putblock;

typedef struct QD_EndloadS {	/* Structure passed for Endload operation */
    bit32 valid;		/*   Set if dwnld is valid, clear to abort */
    bit32 xSTRinfo;		/*   Address of module's xSTRinfo structure */
    bit8  name[8];		/*   Module's name to be used for Streams */
} QD_Endload;

typedef struct QD_IntrS {	/* Structure passed for Interrupt operation */
    bit32 intlevel;		/*   Level to interrupt at */
} QD_Intr;

typedef struct QD_InitS {	/* Structure passed for Init operation */
    bit32 startaddr;		/*   Address to begin execution at */
} QD_Init;

typedef struct QD_ResetS {	/* Structure passed for Reset operation */
    bit8  boardtype[20];	/*   Board typecode name (from qd_known_ltis) */
} QD_Reset;

enum QD_dbg_op { dbg_enable, dbg_disable, dbg_set, dbg_clr };
typedef struct QD_DebugS {	/* Structure passed for SetDebug operation */
    enum QD_dbg_op dbg_cmnd;
    bit32 dbg_param;
} QD_Debug;

 /* Defines the various QD control commands that can be performed.  Each
  * command is usually associated with a command specific control buffer
  * as well, as found above.
  * The various commands are:
  *     Reset   - resets the specified remote Partner
  *     Setload - Starts download of specified remote Partner
  *     Putblock - Writes download block to specified remote Partner
  *     Endload - Finishes download of specified remote Partner
  *     Init - Starts execution of downloaded remote Partner
  *     Interrupt - Generates interrupt on remote Partner
  *	SetDebug - Controls internal debugging operations
  *	DriverId - Causes the driver to output current ID and config to stdout
  *     Response - Response to a previously requested command
  */

enum ctl_cmds { Reset, Setload, Putblock, Endload, Init, Interrupt, Response,
		SetDebug, DriverId };

typedef struct QD_Ctl_s
{
    bit32 ctl_mark;             /* mark start of this struct (mb QD_IFMARK) */
    enum ctl_cmds ctl_cmd;      /* Command to perform */
    bit32 ctl_rval;             /* Return value (default: 0 == success) */
    bit32 ctl_bnum;		/* Board # to be controlled (or HOST_SLOT) */
    union {
        QD_Setload   ctl_setload;
        QD_Putblock  ctl_putblock;
        QD_Endload   ctl_endload;
        QD_Reset     ctl_reset;
        QD_Init      ctl_init;
        QD_Intr      ctl_intr;
	QD_Debug     ctl_debug;
    } ctl_op;			/* Operation specific data */
} QD_Ctl;



#endif	/* _qd_if_h */
