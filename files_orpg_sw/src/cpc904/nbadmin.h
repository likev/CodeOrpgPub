/*   @(#)nbadmin.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * nbadmin.h of snet module
 *
 * SpiderX25
 * @(#)$Id: nbadmin.h,v 1.1 2000/02/25 17:14:44 john Exp $
 * 
 * SpiderX25 Release 8
 */

/* Admin message type */
#define NBADMIN     0x99

/* Values for 'na_command' */
#define NBASGNSVC   0x00    /* Assign facility on board to service  */
#define NBRAMREAD   0x01    /* Read from board RAM		    */
#define NBRAMWRITE  0x02    /* Write to board RAM		    */
#define NBGETDET    0x03    /* Return network board details         */
#define NBACQNET    0x04    /* Acquire board for networrk           */
#define NBRESET     0x05    /* Reset board			    */
#define NBEXEC      0x06    /* Execute loaded code		    */
#define NBBREAK     0x07    /* Break into loaded code		    */
#define NBISREADY   0x08    /* Board ready for access		    */
#define NBWDOGON    0x09    /* Switch Watchdog ON		    */

/* Board type codes (in board identity top byte) */
#define ENPTYPE        1
#define BARTYPE        2
#define GATTYPE        3

/* Maximum buffer size for NBRAMREAD and NBRAMWRITE */
#define MAXTFER     2048

/*  General admin message */
struct nbadmin
{
    unchar  na_type;        /* Message type: always NBADMIN         */
    unchar  na_command;     /* Command: see list above		    */
    ushort  na_board;       /* Board: (type<<8 | seqno) or slot     */
    ulong   na_address;     /* Address for operation, if relevant   */
    ushort  na_length;      /* Lenth of transfer, if relevant       */
};

/* Alternative use of fields for service definition message */
#define na_facility na_address
#define na_service  na_length

/* Flags in 'nb_flags': these define properties of board RAM as seen by CPU */
#define NBSWAPPED    0x01   /* Board values swapped w.r.t. CPU values   */
#define NBMSISLOW    0x02   /* MS of value on board is in lower address	*/
#define NBINTEL86    0x04   /* Board has 8086-type segmentation		*/

/* Values for 'nb_state' */
#define NBNOTNETBD	0   /* Dummy value: doesn't occur in real entry */
#define NBNOTINUSE 	1   /* Is network board, but has not been used  */
#define NBACQUIRED	2   /* Board has been acquired for network      */
#define NBPREPARED	3   /* Board is reset or being loaded           */
#define NBRUNNING	4   /* Board is running (without a watchdog)    */
#define NBWATCHED	5   /* Board is running with its watchdog ON    */
#define NBWFAILED	6   /* Board watchdog failed: unable to restart */
#define NBSTOPPED	7   /* Board has stopped (all streams closed)   */

/*  Network board details */
struct nboard
{
    ulong   nb_vaddr;       /* Kernel virtual address		    */
    ulong   nb_ramstart;    /* Board address of RAM window start    */
    ulong   nb_roffset;     /* Interface root offset		    */
    ulong   nb_size;        /* Size of RAM in bytes		    */
    ushort  nb_ident;       /* Board type<<8 | sequence number      */
    ushort  nb_slot;        /* Backplane slot number		    */
    unchar  nb_flags;	    /* Property flags			    */
    unchar  nb_state;       /* Current start-up state (see above)   */
    unchar  nb_nstreams;    /* Number of streams supported          */
    unchar  nb_nfacils;     /* Number of facilities supported       */
};

#define nb_type nb_ident>>8
