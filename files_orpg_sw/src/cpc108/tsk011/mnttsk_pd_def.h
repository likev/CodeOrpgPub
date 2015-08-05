
/*************************************************************************

    Product Distribution initialization task header file.

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/10/04 14:10:56 $
 * $Id: mnttsk_pd_def.h,v 1.9 2005/10/04 14:10:56 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#ifndef MNTTSK__PD_DEF_H
#define MNTTSK__PD_DEF_H


#define CFG_NAME_SIZE	128
#define CFG_SHORT_STR_SIZE	64

typedef struct {
    int line_ind;
    int user_ind;
    int cm_ind;
    int dev_n;
    int port_n;
    char line_type[CFG_SHORT_STR_SIZE];
    int line_rate;
    char cm_name[CFG_SHORT_STR_SIZE];
    int packet_size;
    int n_pvcs;
    int link_state;
    int en;
    int user_class;
    int time_out;
    char access_word[CFG_SHORT_STR_SIZE];
} Comms_link_t;

/*
 * Function Prototypes
 */
int IUD_init_user_db (Comms_link_t *Links, int N_links);

#endif /* #ifndef MNTTSK_PD_DEF_H */
