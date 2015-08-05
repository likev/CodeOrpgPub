/*   @(#) gdbuconx.h 99/12/23 Version 1.1   */
/*****************************************************************************

    @@@@      @@@@@             Copyright 1991 by          @@@@@       @@@@@
    @@@@      @@@@@                                        @@@@@       @@@@@
    @@@@      @@@@@                                          @@@@     @@@@
    @@@@      @@@@@    @@@@@@@@       @@ @@      @@@@     @@@@@@@     @@@@
    @@@@      @@@@@   @@@@@@@@@@   @@@@@ @@@@@   @@@@     @@@@@@@@@ @@@@@
    @@@@      @@@@@ @@@@@@@@@@@@  @@@@@@ @@@@@@  @@@@@@   @@@@@@@@@ @@@@@
    @@@@      @@@@@@@@@@@    @@@  @@@@@@ @@@@@@  @@@@@@   @@@@  @@@@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@@@@  @@@@  @@@@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@@@@  @@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@     @@@@@@@@@@@@@  @@@@   @@@@@
    @@@@      @@@@@@@@@                                            @
    @@@@      @@@@@@@@@         @@@@@@     @@@@@@@@@@  @@@@@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@  @@@@@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@  @@@@@@@  @@@@@@@
    @@@@      @@@@@@@@@@@    @@@  @@@@@@ @@@@@@  @@@@   @@@@@@  @@@@@@@
    @@@@      @@@@@ @@@@@@@@@@@@  @@@@@@ @@@@@@  @@@@   @@@@@@@@@@@@@@@@@
    @@@@      @@@@@   @@@@@@@@@@   @@@@@ @@@@@   @@@@     @@@@@@@@@ @@@@@
     @@@@@   @@@@      @@@@@@@@       @@ @@      @@@@     @@@@@@@     @@@@
     @@@@@@@@@@@@                                            @@@@     @@@@
       @@@@@@@@@                   Corporation             @@@@@       @@@@@
        @@@@@@               San Diego, California         @@@@@       @@@@@


       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.

       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.


*****************************************************************************/

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: gdb.h                                                                ::
 * ::                                                                      ::
 * ::               GDB Interface Description Includes                     ::
 * ::                                                                      ::
 * :: This module provides the include file definitions used for the code  ::
 * :: which provides the GDB interface between the Server/Controller and   ::
 * :: the Host system.                                                     ::
 * ::                                                                      ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 *
 * $Source: /import/razor_db_cmlnxsvr/orpg_database/RAZOR_UNIVERSE/DOMAIN_01/cpc904%build11/Archive/RZ_VCS/gdbuconx.h,v $
 *
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

#ifndef _gdb_h
#define	_gdb_h



/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: Include files needed: xstypes.h                                      ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */



/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: Macros and definitions                                               ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */


#define	ERRPKT(P,N)	{			\
	(P)->g_op = UC_GDBR_ERR;		\
	(P)->g_size = 0; 			\
	(P)->g_addr = (N); }

#define	OKPKT(P)	{			\
	(P)->g_op = UC_GDBR_OK;		\
	(P)->g_size = 0;			\
	(P)->g_addr = 0; }

#define	SIGPKT(P,S)	{			\
	(P)->g_op = UC_GDBR_SIG;		\
	(P)->g_size = 0; 			\
	(P)->g_addr = (S); }

#define	WHATPKT(P)	{			\
	(P)->g_op = UC_GDBC_WHAT;		\
	(P)->g_size = 0;			\
	(P)->g_addr = 0; }

#define	KILLPKT(P)	{			\
	(P)->g_op = UC_GDBC_KILL;		\
	(P)->g_size = 0;			\
	(P)->g_addr = 0; }

#define	RGETPKT(P,N)	{			\
	(P)->g_op = UC_GDBC_RGET;		\
	(P)->g_size = 0;			\
	(P)->g_addr = N; }

#define	STEPPKT(P,A)	{			\
	(P)->g_op = UC_GDBC_STEP;		\
	(P)->g_size = 0;			\
	(P)->g_addr = (A); }

#define	CONTPKT(P,A)	{			\
	(P)->g_op = UC_GDBC_CONT;		\
	(P)->g_size = 0;			\
	(P)->g_addr = (A); }

/*
 * GDB support packet types
 */

typedef struct UC_GDBInitS {	/* Structure passed for UC_GDBINIT */
    bit32     boardnum;		/* Board number to init GDB on */
    bit32     gdboutput;	/* Location to send GDB output to */
#define UC_GDBCONSOLE	0x1	/* the console port */
#define	UC_GDBBUS	0x2	/* the backplane bus */
}         UC_GDBInitT;

typedef struct UC_GDBmsgS {	/* Control region structure for GDB msgs */
    bit32     gdb_type;		/* Indicates disposition of msg */
#define	UC_GDB		(('G'<<24)|('D'<<16)|('B'<<8))
#define	UC_GDBINIT	(UC_GDB|'I')	/* Initialize GDB on cntlr */
#define	UC_GDBCMND	(UC_GDB|'C')	/* command going downstream */
#define	UC_GDBRESP	(UC_GDB|'R')	/* response going upstream */
#define	UC_GDBERR	(UC_GDB|'E')	/* error in command relaying */
    bit32     boardnum;		/* Board number for GDB operation */
    bit32     gdb_op;		/* GDB command or response */
#define	UC_GDBC_RGET	(UC_GDB|'g')	/* get value of registers */
#define	UC_GDBC_RSET	(UC_GDB|'G')	/* set value of registers */
#define	UC_GDBC_MGET	(UC_GDB|'m')	/* get memory contents */
#define	UC_GDBC_MSET	(UC_GDB|'M')	/* set memory contents */
#define	UC_GDBC_CONT	(UC_GDB|'c')	/* resume (w/optional addr) */
#define	UC_GDBC_STEP	(UC_GDB|'s')	/* step 1 instr (from optional addr) */
#define	UC_GDBC_KILL	(UC_GDB|'k')	/* kill */
#define	UC_GDBC_WHAT	(UC_GDB|'?')	/* what was last sigval? */
#define	UC_GDBR_OK	(UC_GDB|'o')	/* command completed ok */
#define	UC_GDBR_ERR	(UC_GDB|'e')	/* command completed w/error */
#define	UC_GDBR_SIG	(UC_GDB|'S')	/* execution halted with signal */
#define	UC_GDBR_RVAL	(UC_GDB|'r')	/* register values */
#define	UC_GDBR_MVAL	(UC_GDB|'v')	/* memory values */
#define	UC_GDBO_NONE	(UC_GDB|0)	/* no operation, just raw data */
    bit32     gdb_addr;		/* Address for command or response */
    bit32     gdb_size;		/* Size for command or response */
}         UC_GDBmsgT;


/*
 * Internal GDB command/response packet storage
 */

#define GDBBUFMAX       400	/* max data size of GDB buffer. There are 2. */
 /* must be at least NUMREGBYTES*2 */
 /* recommend lw-aligned size (X&3 == 0) */
 /* must be <= GDBBUFSIZE in x_if_int.h */

typedef struct GDBbufS {	/* buffer pointed to be gdb_cmnd/gdb_resp */
    bit32     g_valid;		/* set if buffer contains valid info */
    bit32     g_op;		/* set to gdb_op from UC_GDBmsgT */
    bit32     g_addr;		/* address of gdb operation (gdb_addr) */
    bit32     g_size;		/* size of gdb operation (gdb_size) */
    bit8      g_data[GDBBUFMAX];/* GDB data for operation */
}         GDBbufT;

#endif				/* _gdb_h */
