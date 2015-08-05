/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 17:11:24 $
 * $Id: seq_cs.h,v 1.1 2013/07/22 17:11:24 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


#ifndef VCP_CS_H

#define VCP_CS_H

#include <cs.h>
#include <prfselect_buf.h>

/* Macro definitions for CS format */

#define VCP_CS_DFLT_FNAME		"vcp_sequence_table"
#define VCP_CS_COMMENT			'#'
#define VCP_CS_MAXLEN			80

/* Macro definitions for VCP_attr section. */
#define VCP_SEQ_CS_ATTR_KEY		"VCP_Seq"
#define VCP_SEQ_CS_NUM_SEQ_KEY		"num_seqs"
#define VCP_SEQ_CS_NUM_SEQ_TOK		(1 | (CS_SHORT))

#define VCP_SEQ_CS_NUM_VCPS_KEY		"num_in_seq"
#define VCP_SEQ_CS_NUM_VCPS_TOK		(1 | (CS_SHORT))
#define VCP_SEQ_CS_VCPS_KEY		"vcps"

#endif
