/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 17:11:24 $
 * $Id: mnttsk_vcp_seq.h,v 1.1 2013/07/22 17:11:24 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef MNTTSK_VCP_SEQ_DEF_H
#define MNTTSK_VCP_SEQ_DEF_H

#include <seq_cs.h>
#include <orpg.h>
#include <prfselect_buf.h>


#define STARTUP   1
#define RESTART   2
#define CLEAR     3

static int MNTTSK_VCP_Seq_table( int startup_action );

#endif 
