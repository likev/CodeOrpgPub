/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/04/27 15:48:03 $
 * $Id: orpgumc_rda.h,v 1.8 2007/04/27 15:48:03 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
#ifndef ORPGUMC_RDA_H
#define ORPGUMC_RDA_H

#include <orpg.h>
#include <comm_manager.h>
#include <rda_rpg_message_header.h>
#include <basedata.h>
#include <generic_basedata.h>
#include <rda_performance_maintenance.h>
#include <orda_pmd.h>
#include <rda_rpg_console_message.h>
#include <rda_rpg_loop_back.h>
#include <clutter.h>
#include <rpg_request_data.h>
#include <misc.h>
#include <orpgrda.h>
#include <rpg_vcp.h>
#include <orda_adpt.h>
 

#define UMC_RDA_SUCCESS		0
#define UMC_RDA_FAILURE		-1
 
                                                                                        
#define MSG_HDR_BYTES		sizeof(RDA_RPG_message_header_t)
#define MSG_HDR_SHORTS		((MSG_HDR_BYTES + 1)/sizeof(short))


int UMC_from_ICD_RDA (void *msg);
int UMC_to_ICD_RDA (void *msg);
int UMC_RPGtoRDA_message_convert_to_external(int data_type, char* data);
int UMC_RDAtoRPG_message_convert_to_internal(int data_type, char* data);
int UMC_RDAtoRPG_message_header_convert(char* data);
int UMC_generic_basedata_header_convert_to_internal(char* data);
int UMC_generic_basedata_header_convert_to_external(char* data);
int UMC_floats_to_icd( int nooffloats, float* buf );
int UMC_floats_from_icd( int nooffloats, float* buf );
char *UMC_convert_to_31 (RDA_RPG_message_header_t *msg_header, char *radar_id);

#endif
