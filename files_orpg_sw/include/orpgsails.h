/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/08 21:29:25 $
 * $Id: orpgsails.h,v 1.1 2014/07/08 21:29:25 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef ORPGSAILS_H
#define	ORPGSAILS_H

#include <infr.h>
#include <orpg.h>
#include <orpgda.h>
#include <gen_stat_msg.h>
#include <orpgvcp.h>

/* SAILS state values. */
#define GS_SAILS_DISABLED       0
#define GS_SAILS_INACTIVE       1
#define GS_SAILS_ACTIVE         2

/*  	Report the status of the last SAILS status I/O operation */
int 	ORPGSAILS_io_status();

/*	The following functions handle SAILS status and request I/O. */
int	ORPGSAILS_status_read();
int	ORPGSAILS_status_write();
int	ORPGSAILS_request_read();
int	ORPGSAILS_request_write();

/*	The following functions report various status/values. */

int	ORPGSAILS_get_status();
int	ORPGSAILS_get_num_cuts();
int	ORPGSAILS_get_req_num_cuts();
int	ORPGSAILS_get_max_cuts();
int	ORPGSAILS_get_site_max_cuts();
int	ORPGSAILS_allowed();
int	ORPGSAILS_set_num_cuts( int num_cuts );
int	ORPGSAILS_set_req_num_cuts( int num_cuts );

/*      The following funciton initializes SAILS status. */
void    ORPGSAILS_init();

#endif
