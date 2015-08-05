/*   @(#)nbshare.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * nbshare.h of snet module
 *
 * SpiderX25
 * @(#)$Id: nbshare.h,v 1.1 2000/02/25 17:14:46 john Exp $
 * 
 * SpiderX25 Release 8
 */

/* Magic number */
#define IFMAGIC 0x12345678          /* Access check code */

/* Interface version */
#define IFVERSION        3          /* Interface version number */

/* Length of software name field */
#define IFNAMESZ	64          /* Name field length */

/* Attention ring special entries */
#define TBACKID       0xFE          /* Back-enable transfer ring */
#define STARTID       0xFF          /* Initiate scan of transfer ring */

/* Reasons for blocking */
#define FCBLOCK       0x01          /* STREAMS flow control */
#define TFBLOCK       0x02          /* Transfer queue blockage */

/* Stage values for 'started' and 'backed' fields */
#define ATDONE		 0
#define ATSENT		 1
#define ATRCVD		 2
#define ATDEAD         255

/* Transfer ring entries */
#define ENDMSG      0x8000          /* Sub-field of 'te_length' */
#define LENMSK      0x7FFF          /* Sub-field of 'te_length' */

struct tfentry
{
    uint32          te_daddr;       /* 00: Address of data start */
    uint16          te_length;      /* 04: Endmsg flag/block length */
    uint8           te_sid;         /* 06: Destination stream ID */
    uint8           te_type;        /* 07: Message block type */
    uint32          te_blkid;       /* 08: Data block identity */
};

/* Stream control */
struct isqctl
{
    uint32          is_hoststrm;    /* 00: Host's stream handle */
    uint32          is_facility;    /* 04: Facility code */

    /* Down flow control pseudo-ring */
    uint8           is_fdhseq;      /* 08: Down flow host sequence number */
    uint8           is_fdsseq;      /* 09: Down flow slave sequence number */
    uint8           is_fdhiwat;     /* 0A: Down flow high-water mark */
    uint8           is_fdlowat;     /* 0B: Down flow low-water mark */

    /* Up flow control pseudo-ring */
    uint8           is_fuhseq;      /* 0C: Up flow host sequence number */
    uint8           is_fusseq;      /* 0D: Up flow slave sequence number */
    uint8           is_fuhiwat;     /* 0E: Up flow high-water mark */
    uint8           is_fulowat;     /* 0F: Up flow low-water mark */

    /* Flow control blocked status */
    uint8           is_fdblocked;   /* 10: Down blocked status */
    uint8           is_fublocked;   /* 11: Up blocked status */

    /* Flow control back-enable stages */
    uint8           is_fdbkstage;   /* 12: Down back-enable up-attn stage */
    uint8           is_fubkstage;   /* 13: Up back-enable down-attn stage */
};

/* Root shared structure (length 128 bytes) */
/* Note: 32-bit addresses in the interface are all relative to the */
/*       start of this structure.				   */
struct shared
{
    /* Identification */
    uint8           if_ifversion;   /* 00: Interface version code */
    uint8           if_swversion;   /* 01: Software version */
    uint8           if_nbslot;      /* 02: Backplane slot number */
    uint8           if_nbident;     /* 03: Board type<<4 | sequence number */

    /* Access check */
    uint32          if_magic;       /* 04: "Magic number" */

    /* Host private info */
    uint32          if_hostbrd;     /* 08: Host's board handle */

    /* Diagnostic address block */
    uint32          if_diagaddr;    /* 0C: Address of diagnostic info */

    /* ASCII software name (padded with blanks) */
    uint8           if_swname[IFNAMESZ];
				    /* 10: Software name */

    /* Panic or general error message */
    uint32          if_syscall;     /* 50: System call to be called */

    /* Stream control array */
    uint32          if_scaddr;      /* 54: Stream control array address */

    /* Ring bodies */
    uint32          if_tdaddr;      /* 58: Down transfer ring address */
    uint32          if_tuaddr;      /* 5C: Up transfer ring address */
    uint32          if_adaddr;      /* 60: Down attention ring address */
    uint32          if_auaddr;      /* 64: Up attention ring address */

    /* Static administrative values */
    uint8           if_nstreams;    /* 68: Number of streams supported */
    uint8           if_nfacils;     /* 69: Number of facilities supported */
    uint8           if_hmpcid;      /* 6A: Host bus ID */
    uint8           if_smpcid;      /* 6B: Slave bus ID */

    /* Transfer ring parameters */
    uint8           if_tdsize;      /* 6C: Down tfer ring size */
    uint8           if_tdhiwat;     /* 6D: Down tfer ring high-water mark */
    uint8           if_tusize;      /* 6E: Up tfer ring size */
    uint8           if_tulowat;     /* 6F: Up tfer ring low-water mark */

    /* Watchdog sequence numbers */
    uint8           if_wdhseq;      /* 70: Watchdog host sequence number */
    uint8           if_wdsseq;      /* 71: Watchdog slave sequence number */

    /* Down transfer ring controls */
    uint8           if_tdsseq;      /* 72: Down tfer slave sequence number */
    uint8           if_tdhseq;      /* 73: Down tfer host sequence number */
    uint8           if_tdblocked;   /* 74: Down tfer blocked status */
    uint8           if_tdststage;   /* 75: Down tfer start down-attn stage */
    uint8           if_tdbkstage;   /* 76: Down tfer back-enbl up-attn stage */

    /* Up transfer ring controls */
    uint8           if_tusseq;      /* 77: Up tfer slave sequence number */
    uint8           if_tuhseq;      /* 78: Up tfer host sequence number */
    uint8           if_tublocked;   /* 79: Up tfer blocked status */
    uint8           if_tuststage;   /* 7A: Up tfer start up-attn stage */
    uint8           if_tubkstage;   /* 7B: Up tfer back-enbl down-attn stage */

    /* Attention ring controls */
    uint8           if_adpseq;      /* 7C: Down attn put sequence number */
    uint8           if_addseq;      /* 7D: Down attn done sequence number */
    uint8           if_aupseq;      /* 7E: Up attn put sequence number */
    uint8           if_audseq;      /* 7F: Up attn done sequence number */
    uint8           if_close_strm;  /* 80: handle to board stream which is *
                                     *     being closed from kernel.       */
    uint8           if_close;
};
