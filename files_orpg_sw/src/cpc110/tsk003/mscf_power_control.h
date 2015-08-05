/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 21:36:37 $
 * $Id: mscf_power_control.h,v 1.2 2013/07/22 21:36:37 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef MSCF_POWER_CONTROL_H
#define MSCF_POWER_CONTROL_H

enum { PC_DEVICE_UNAVAILABLE, PC_DEVICE_AVAILABLE };
enum { PC_TURN_ON, PC_TURN_OFF, PC_REBOOT, NUM_PWRCTRL_OPTIONS, PC_NONE };
enum { PC_ON_STATE, PC_OFF_STATE, NUM_PWRCTRL_STATES };
enum { PC_SELECT_NO, PC_SELECT_YES, NUM_PWRCTRL_SELECTIONS };
enum { PC_ACTIVE_NO, PC_ACTIVE_YES, NUM_PWRCTRL_ACTIVE_STATES };
enum { DEFAULT_CURSOR, BUSY_CURSOR };
enum { PWRADM_DISCONNECTED, PWRADM_CONNECTED };


#define MAX_OUTLETS_PER_SWITCH  8
#define MAX_NUM_SWITCHES        2
#define MAX_NUM_OUTLETS         MAX_NUM_SWITCHES*MAX_OUTLETS_PER_SWITCH
#define MAX_CMD_LEN             256
#define MAX_ADDRESS_LEN         32
#define MAX_STRING_LEN          64
#define MAX_LABEL_LEN           18
#define MAX_OUT_TEXT_LEN        10000


typedef struct /* Structure for power outlet. */
{
  int active;                 /* boolean: yes/no */
  int on;                     /* boolean: on/off */
  int selected;               /* selected in GUI */
  char label[MAX_LABEL_LEN+1]; /* label string */
  int switch_index;           /* index of switch */
  int outlet_index;           /* index of outlet in switch */
  int enable_turn_on;         /* Enable Turn On of outlet */
  int enable_turn_off;        /* Enable Turn Off of outlet */
  int enable_reboot;          /* Enable Turn Reboot of outlet */
  char host_name[MAX_CMD_LEN];/* Host name of device (if applicable) */
  char shutdown_cmd[MAX_CMD_LEN];/* Cmd to run on host before PC action */
  int shutdown_delay;         /* Number of seconds to delay before PC action */
  Widget icon;                /* draw button */
  Widget label_widget;        /* label widget */
} Power_status_t;

/* Function Prototypes. */
void    Send_snmp_command( char * );
void    Popup_wait_for_command( char *, char *, int );
void    Sentry_get_outlet_names();
void    Sentry_get_outlet_status();
int     Sentry_get_next_result( char *, char **, int *, int * );
int     Sentry_PC_power_control( int, int );
void    APC_get_outlet_names();
void    APC_get_outlet_status();
int     APC_get_next_result( char *, char **, int *, int * );
int     APC_PC_power_control( int, int );

#endif
