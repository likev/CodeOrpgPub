/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/14 21:01:15 $
 * $Id: orpgred.h,v 1.38 2013/05/14 21:01:15 steves Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgred.h

 Description: liborpg.a RED(redundant orpg command/data access) module 
              include file.

       Notes: This is included by orpg.h.

 **************************************************************************/

#ifndef ORPGRED_H
#define ORPGRED_H

#include <clutter.h>
#include <orpgda.h>
#include <orpgdat.h>
#include <rda_status.h>
#include <siteadp.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define ORPGRED_RDA_CONTROLLING              0
#define ORPGRED_RDA_NON_CONTROLLING          1
#define ORPGRED_RDA_CONTROL_STATE_UNKNOWN    2


enum {                   /* ORPG-to-Redundant manager commands */
   ORPGRED_UPDATE_ALL_MESSAGES = 0, /* Cmd to update all redundant channel's 
                                       adaptation data LBs */
   ORPGRED_DOWNLOAD_BYPASS_MAP,     /* Cmd redundant channel to download Bypass 
                                       Map to its RDA */
   ORPGRED_DOWNLOAD_CLUTTER_ZONES,  /* Cmd redundant channel to download 
                                       Suppression Zones to its RDA */
   ORPGRED_VCP_RDA_CONTROL};        /* VCP RDA control commands (either
                                       change VCP or download VCP to RDA */


enum {         /* redundant channel manager LB message ids */
   ORPGRED_CHANNEL_STATUS_MSG = 1,      /* this channel's status msg */
   ORPGRED_REDUN_CHANL_STATUS_MSG,      /* redundant channel's status msg */
   ORPGRED_PING_CHANNEL_LINK,           /* channel link ping msg */
   ORPGRED_PING_RESPONSE,               /* channel link ping response msg */
   ORPGRED_DNLOAD_BYPASS_MAP,           /* cmd msg to download bypass map to RDA */
   ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES, /* cmd msg to download clutter censor 
                                           zones to RDA */
   ORPGRED_SELECT_VCP,                  /* cmd msg to select a VCP */
   ORPGRED_DOWNLOAD_VCP,                /* cmd msg to download a VCP */
   ORPGRED_SEND_CHANNEL_STATUS,         /* cmd msg to send channel status */
   ORPGRED_UPDATE_ADAPT_DATA_TIME,      /* cmd msg to update this channel's Adapt 
                                           Data time stamp */
   ORPGRED_UPDATE_SPOT_BLANKING,        /* cmd msg to update the spot blanking 
                                           state */
   ORPGRED_UPDATE_CMD,                  /* cmd msg to update the Clutter Mitigation
                                           Decision */
   ORPGRED_UPDATE_SR,                   /* cmd msg to update Super Res */
   ORPGRED_UPDATE_SAILS,                /* cmd to update SAILS */
   ORPGRED_UPDATE_AVSET,                /* cmd to update AVSET */
   ORPGRED_PREVIOUS_CHANL_STATE};       /* previous channel state msg */

typedef enum {      /* states of the channel */
   ORPGRED_CHANNEL_INACTIVE,
   ORPGRED_CHANNEL_ACTIVE
} Channel_state_t;

typedef enum {      /* states of the RPG-RPG link */
   ORPGRED_CHANNEL_LINK_UP = 1,
   ORPGRED_CHANNEL_LINK_DOWN
} Channel_link_state_t;

typedef enum {          /* states of the NB users comms relay */
   ORPGRED_COMMS_RELAY_UNKNOWN,    /* the state of the comms relay is unknown */
   ORPGRED_COMMS_RELAY_ASSIGNED,   /* the comms relay is assigned to this RPG */
   ORPGRED_COMMS_RELAY_UNASSIGNED, /* the comms relay is not assigned to this RPG */
} Comms_relay_state_t;


/* Redundant Manager Messages */

   /* ORPGDAT_REDMGR_CMDS */

typedef struct {       /* orpg-to-redundant mgr command msg */
   short   cmd;            /* redundant manager command to process */
   int     lb_id;          /* data id of the cmd msg lb */
   LB_id_t msg_id;         /* message id of the command message */
   int     parameter1;      /* optional command parameters */
   int     parameter2;
   int     parameter3;
   int     parameter4;
   int     parameter5;
} Redundant_cmd_t;


   /* ORPGDAT_REDMGR_CH_MSGS */

      /* msg 1 & 2 -- local and redundant channel status messages */

#define MAX_HOSTNAME_LEN         32

typedef struct {
   int    rpg_configuration;      /* specifies rpg hardware configuration.
                                    Macros are defined in siteadp.h */
   int    rpg_channel_number;     /* specifies the redundant rpg channel # */
   int    rpg_channel_state;      /* channel state: active, inactive */
   char   rpg_hostname[MAX_HOSTNAME_LEN]; /* RPG node hostname */
   int    rpg_state;              /* operate, standby, etc */
   int    rpg_mode;               /* maintenance, test, etc. */
   int    rda_control_state;      /* rda control state (controlling, non-ctl) */
   int    rda_status;             /* operate, standby, etc. */
   int    rda_operability_status; /* op status defined in the RDA-RPG ICD */
   int    rda_rpg_wb_link_state;  /* rda-rpg wb link state: 
                                     connected, disconnected */
   int    comms_relay_state;      /* state of the NB/WB comms relay */
   int    rpg_rpg_link_state;     /* state of link between RPG:
                                     ORPGRED_CHANNEL_LINK_UP = 1,
                                     ORPGRED_CHANNEL_LINK_DOWN = 2 */
   time_t adapt_dat_update_time;  /* last time the adapt dat was updated */
   int    rpg_sw_version_num;     /* RPG application s/w version number */
   int    adapt_data_version_num; /* Adaptation data version number */
   int    spare1;
   int    spare2;
   int    spare3;
   int    spare4;
   int    spare5;
   int    spare6;
   int    spare7;
   int    spare8;
   int    stat_msg_seq_num;       /* last read RDA status msg seq # */
              /* keep stat_msg_seq_num as the last element
                 in the structure. it's being stripped off
                 for a mem compare */
} Channel_status_t;

      /* msg 3 -- redundant mgr Inter-Process Communications (IPC) message */

typedef struct {
   int parameter1;      /* message parameters */
   int parameter2;
   int parameter3;
   int parameter4;
   int parameter5;
}  Redundant_channel_msg_t;

      /* msg 4 - the channel state at shutdown */

typedef struct {
   int  rda_control_state;       /* rda control state: controlling, 
                                    non-controlling */
   int  rpg_channel_state;       /* channel state: active, inactive */
}  Previous_channel_state_t;


/* public functions */

/*  Read functions */

LB_id_t ORPGRED_get_msg (Redundant_cmd_t *cmd);

int ORPGRED_send_msg (Redundant_cmd_t cmd);

LB_id_t ORPGRED_seek (LB_id_t seek_msg_id);

/*int ORPGRED_info (int lb_id);*/

/*   Convenience functions.               */

typedef enum {
   ORPGRED_MY_CHANNEL,
   ORPGRED_OTHER_CHANNEL
} Orpgred_chan_t;

time_t ORPGRED_adapt_dat_time       (Orpgred_chan_t which_channel);
int   ORPGRED_channel_num           (Orpgred_chan_t which_channel);
int   ORPGRED_channel_state         (Orpgred_chan_t which_channel);
int   ORPGRED_comms_relay_state     (Orpgred_chan_t which_channel);
int   ORPGRED_rda_control_status    (Orpgred_chan_t which_channel);
char *ORPGRED_get_hostname          (Orpgred_chan_t which_channel);
int   ORPGRED_rda_op_state          (Orpgred_chan_t which_channel);
int   ORPGRED_rda_status            (Orpgred_chan_t which_channel);
int   ORPGRED_rpg_mode              (Orpgred_chan_t which_channel);
int   ORPGRED_rpg_state             (Orpgred_chan_t which_channel);
int   ORPGRED_rpg_rpg_link_state    (void);
int   ORPGRED_update_adapt_dat_time (time_t update_time);
int   ORPGRED_version_numbers_match (void);
int   ORPGRED_wb_link_state         (Orpgred_chan_t which_channel);

typedef enum {
   ORPGRED_IO_NORMAL,
   ORPGRED_IO_ERROR,
   ORPGRED_IO_INVALID_MSG_ID
} Orpgred_io_status_t;

/*   Various redundant specific I/O routines         */

Orpgred_io_status_t ORPGRED_write_status (Orpgred_chan_t id);
Orpgred_io_status_t ORPGRED_read_status  (Orpgred_chan_t id);
Orpgred_io_status_t ORPGRED_io_status    (Orpgred_chan_t id);

char *ORPGRED_status_msg (Orpgred_chan_t id);

/*   LB notification callbacks for status messages      */

void   ORPGRED_en_local_status_callback (int fd, LB_id_t msgid,
                 int msg_info, void *arg);
void   ORPGRED_en_red_status_callback   (int fd, LB_id_t msgid,
                 int msg_info, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGRED_H DO NOT REMOVE! */
