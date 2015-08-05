#ifndef RPG_STATUS_PROD_H
#define RPG_STATUS_PROD_H

#include <rpgc.h>
#include <a309.h>
#include <orpg.h>
#include <orpg_product.h>
#include <status_prod.h>

/* RPG Messages. */
#define RPG_INFO_STATUS_MSG              0x0001 /* RPG informational message filter */
#define RPG_GEN_STATUS_MSG               0x0002 /* status type message filter */
#define RPG_WARN_STATUS_MSG              0x0004 /* warning/error type message filter */
#define RPG_NB_COMMS_STATUS_MSG          0x0008 /* Narrowband Communications message filter */

/* RPG Alarm Messages */
#define RPG_MAM_ALARM_ACTIVATED_MSG      0x0010 /* alarm set type message filter */
#define RPG_MAR_ALARM_ACTIVATED_MSG      0x0020 /* alarm set type message filter */
#define RPG_LS_ALARM_ACTIVATED_MSG       0x0040 /* alarm set type message filter */
#define RPG_ALARM_CLEARED_MSG            0x0080 /* alarm clear type message filter */

/* RDA Alarm Messages */
#define RDA_SEC_ALARM_ACTIVATED_MSG      0x0100 /* RDA Secondary alarm type message filter */
#define RDA_MAR_ALARM_ACTIVATED_MSG      0x0200 /* RDA MAR alarm type message filter */
#define RDA_MAM_ALARM_ACTIVATED_MSG      0x0400 /* RDA MAM alarm type message filter */
#define RDA_INOP_ALARM_ACTIVATED_MSG     0x0800 /* RDA INOP alarm type message filter */
#define RDA_NA_ALARM_ACTIVATED_MSG       0x1000 /* RDA Not Applicable alarm type message filter */
#define RDA_ALARM_CLEARED_MSG            0x2000 /* RDA alarm cleared type message filter */


typedef struct Linked_list {

   struct Linked_list *next;
   unsigned int code;
   char *text;

} Text_node_t; 

#ifdef RPG_STATUS_PROD_C
Text_node_t *List_head = NULL; 
Text_node_t *List_tail = NULL;
#endif

#ifdef BUILD_STATUS_PROD_C
extern Text_node_t *List_head; 
extern Text_node_t *List_tail;
#endif

#define ASP_STATEFILE "status_log_msg_id"

/* Function Prototypes. */
int Build_status_product( int num_messages );
int Write_to_state_file( LB_id_t msg_id );
int Read_from_state_file( LB_id_t *msg_id );

#endif 

