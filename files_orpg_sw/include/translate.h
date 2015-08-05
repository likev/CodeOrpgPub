/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/11/21 22:30:02 $
 * $Id: translate.h,v 1.1 2005/11/21 22:30:02 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

# ifndef TRNS_H
# define TRNS_H

#define USER_COMMANDED_VCP_MSG_ID    1
#define TRANS_ACTIVE_MSG_ID          2
#define TRANS_TBL_MSG_ID             3

typedef struct trans_tbl_t {

   int incoming_vcp;
   int translate_to_vcp;
   int elev_cut_map_size;
   int elev_cut_map[ VCP_MAXN_CUTS ];

} Trans_tbl_t;

typedef struct trans_list_t {

   Trans_tbl_t table;
   struct trans_list_t *next;

} Trans_list_t;

typedef struct trans_tbl_inst_t {

   int installed;
   int num_tbls;
   struct trans_tbl_t table[1];

} Trans_tbl_inst_t;

typedef struct user_commanded_vcp_t {

   int last_vcp_commanded;

} User_commanded_vcp_t;

typedef struct trans_info_t {

   int active;
   int vcp_internal;
   int vcp_external;

} Trans_info_t;

/* Macro definitions for VCP_attr section. */
#define TRNS_CS_ATTR_KEY         "VCP_translation_table"
#define TRNS_CS_INCOMING_KEY     "incoming_vcp"
#define TRNS_CS_INCOMING_TOK     (1 | (CS_INT))
#define TRNS_CS_TRANSLATE_KEY    "translate_to_vcp"
#define TRNS_CS_TRANSLATE_TOK    (1 | (CS_INT))
#define TRNS_CS_MAP_SIZE_KEY     "elev_cut_map_size"
#define TRNS_CS_MAP_SIZE_TOK      (1 | (CS_INT))
#define TRNS_CS_CUT_MAP_KEY      "elev_cut_mapping"
#define TRNS_CS_CUT_MAP_TOK      (1 | (CS_INT))

#endif

