/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/08 16:33:23 $
 * $Id: pcipdalg.h,v 1.21 2006/06/08 16:33:23 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */
#ifndef PCIPCALG_H
#define PCIPDALG_N

#include <alg_adapt.h>
#include <mode_select.h>
#include <hydromet_prep.h>
#include <stdlib.h>
#include <rpgc.h>
#include <a309.h>
#include <math.h>
#include <vcp.h>

#define CTGRY0             0
#define CTGRY1             1

#define PFWXCLA            1
#define PFWXCONV           2

#define SEC_IN_DAY     86400
#define SEC_IN_HOUR     3600
#define SEC_IN_MIN        60

typedef struct {

   int rpgvcp;
   int rpgwmode;
   int rpgvsnum;
   Vcp_struct current_vcp_table; 

} Vcpinfo_t;

typedef struct {

   int current_date;
   int current_time;
   int precip_cat;
   int last_precip_detect_date;
   int last_precip_detect_time;
   int weather_mode;
   int inop_flag;

} Prcipmsg_t;

#define MODE_SELECT_UNKNOWN      -1
#define MODE_SELECT_NORMAL        0
#define MODE_SELECT_CONFLICT      1

typedef struct {

   int wxstatus_status;
   int current_wxstatus;
   int current_vcp;
   int recommended_wxstatus;
   time_t recommended_wxstatus_start_time;
   time_t current_wxstatus_time;
   time_t conflict_start_time;
   A3052t a3052t;

} Mode_select_status_t;

#ifdef PCIPDALG_C
int Hybrscan_id = 0;
int Prcipmsg_id = 0;
int Prfsel_id = 0;

/* Adaptation data. */
Mode_select_entry_t Mode_select;
hydromet_prep_t Hydromet_prep;
int Msf_ade_id;
#endif

#ifdef A3052A_C
extern int Hybrscan_id;
extern int Prcipmsg_id;
extern int Prfsel_id;
extern Mode_select_status_t Mode_select_status;
extern Mode_select_entry_t Newtbl;
extern int Msf_ade_id;
#endif

#ifdef A3052H_C
Mode_select_status_t Mode_select_status;
Mode_select_entry_t Newtbl;
int Mode_B_selection_time;

/* Adaptation data. */
extern Mode_select_entry_t Mode_select;
extern hydromet_prep_t Hydromet_prep;
#endif

#ifdef WRITE_GAGEDB_C
extern Mode_select_status_t Mode_select_status;
extern Mode_select_entry_t Newtbl;
extern Mode_select_entry_t Mode_select;
extern int Mode_B_selection_time;
#endif

/* Function prototypes. */
void A3052A_buffer_control( int event );
void A3052C_start_of_volume( int event );
int A3052E_init_get_adapt( );
int A3052F_comp_tables( );
int A3052G_chekrate_sumareas( int *hybrscan, float *area );
int A3052H_precip_cats( int *catflag );
int A3052I_setmode_setobuf( Prcipmsg_t *outbuf, int catflag, Vcpinfo_t *vcp_info,
                            int *mode_deselect );

void Write_gagedb( int *precip_status );
void Write_wx_status( float area, int wxstatus_deselect );
void Read_wx_status( );

#endif
