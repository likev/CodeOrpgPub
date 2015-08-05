/******************************************************************

	file: rms_util.h

	This is the main module for utility library functions.
	
******************************************************************/
#include <prod_distri_info.h>
#include <rms_ptmgr.h>
#include <orpg.h>

  
#ifndef RMS_UTIL_H
#define RMS_UTIL_H


/* Defines for RMS */

#define PLUS_INT		4

#define PLUS_SHORT 		2

#define PLUS_BYTE  		1

#define PLUS_BYTE_2 		2

#define SECURITY_LEVEL_RMS	0x00000020

#define	SECURITY_LEVEL_ZERO	0x00000001

#define MAX_N_USERS	50	/* max number of users served by a p_server 
				   instance */
				   
#define RMS_PASSWORD1  "WXMAN1"
#define RMS_PASSWORD2  "WXMAN2"
#define RMS_PASSWORD3  "WXMAN3"

#define	RMS_CLEAR_LOCK	0
#define	RMS_SET_LOCK	1

#define	RMS_RPG_LOCK	0
#define	RMS_RDA_LOCK	1

#define	RMS_STATUS	0

#define	RMS_COMMAND_UNLOCKED	0
#define	RMS_COMMAND_LOCKED	1


/* Globals for RMS util */

enum	{PROD_DISTRI_METHOD_NONE=0,
	 PROD_DISTRI_METHOD_SSET,
	 PROD_DISTRI_METHOD_RSET,
	 PROD_DISTRI_METHOD_1TIM,
	 PROD_DISTRI_METHOD_COMB
};

enum {				/* p_server states; psv_state in User_struct */
    ST_DISCONNECTED,		/* state disconnected */
    ST_CONNECT_PENDING,		/* state connect pending */
    ST_AUTHENTICATION,		/* state authentication */
    ST_INITIAL_PROCEDURE,	/* state initial procedure */
    ST_ROUTINE,			/* state routine procedure */
    ST_TERMINATION		/* state termination */
};

enum { EDIT, WRITE };

typedef struct{
	char password[8];
	char name [8];
	int rate;
	int distri_type;
	int line_class;
	int line_type;
	int connect_time;
}Nb_cfg;

typedef struct {
	int line_num; 
	ushort nb_status; /* narrowband status */
	ushort nb_utilization; /* utilization percentage */
	short user_id;
}Nb_status;

typedef struct {
	int rms_rpg_locked_hci;
	int rms_rda_locked_hci;
	int rms_rpg_locked_rms;
	int rms_rda_locked_rms;
	} Rms_status;
	
/* Public functions */

/* rms_conv.c */
int conv_intgr (UNSIGNED_BYTE *buf_ptr);

ushort conv_ushrt (UNSIGNED_BYTE *buf_ptr);
	
short conv_shrt (UNSIGNED_BYTE *buf_ptr);

float conv_real(UNSIGNED_BYTE *buf_ptr);
	
void conv_char (UNSIGNED_BYTE *buf_ptr, char *temp_char, int num_bytes);

void conv_int_unsigned(UNSIGNED_BYTE *buf_ptr,int *num);

void conv_ushort_unsigned(UNSIGNED_BYTE *buf_ptr,ushort *num);

void conv_short_unsigned(UNSIGNED_BYTE *buf_ptr,short *num);

void conv_char_unsigned (UNSIGNED_BYTE *buf_ptr, char *temp_char, int num_bytes);

void conv_real_unsigned(UNSIGNED_BYTE *buf_ptr, float *in_flt);

void init_buf(UNSIGNED_BYTE in_buf[]);

int swap_bytes(UNSIGNED_BYTE *in_buf, int msg_size);

/* rms_nb_status.c */
int rms_get_user_status(Nb_status *user_ptr);

short rms_get_user_id(int line_num);

short rms_get_line_index(int line_num);

int rms_read_user_info ();

/* rms_nb_cfg.c */
int rms_get_nb_cfg (Nb_cfg* cfg_ptr,short line_num, short uid);

/*rms_security.c */
int rms_validate_password (char* password);

int rms_get_lock_status( int command);

int rms_rda_rpg_lock(int command, int state);
#endif
	


