
/*******************************************************************

    Module: cs.h

    Description: header file for the CS module.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:31 $
 * $Id: cs.h,v 1.25 2012/07/27 19:33:31 jing Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */  

#ifndef CS_H
#define CS_H

#define CS_UPDATED 1

/* values for the "action" argument of CS_control () */
#define CS_CLOSE	0x1	/* close the configuration file */
#define CS_UPDATE	0x2	/* force an update of the stored cfg text */
#define CS_RESET	0x4	/* resets the file parsing state to the newly 
				   open state */
#define CS_BACK_LINE	0x8	/* back up the CS_next_line pointer by one 
				   line */
#define CS_KEY_OPTIONAL	0x10	/* optional key mode */
#define CS_KEY_REQUIRED	0x20	/* key required mode */
#define CS_COMMENT	0x4000	/* set comment identification character */
#define CS_SINGLE_LEVEL 0x40
#define CS_DELETE	0x80	/* not used */ 
#define CS_NO_ENV_EXP	0x100	/* no environmental variable expansion */
#define CS_YES_ENV_EXP	0x200	/* resume environmental variable expansion */

#define CS_NEXT_LINE	(char *)0xffffffff
				/* values for the argument "key" of function 
				   CS_entry */
#define CS_THIS_LINE	(char *)0xfffffffe
				/* values for the argument "key" of function 
				   CS_entry */

/* flags used in the "tok" argument of CS_entry */
#define CS_UINT		0x40000
#define CS_INT_KEY	0x80000000
#define CS_INT		0x40000000
#define CS_FLOAT	0x20000000
#define CS_DOUBLE	0x10000000
#define CS_SHORT	0x8000000
#define CS_BYTE		0x4000000
#define CS_CHAR		CS_BYTE
#define CS_HEXINT	0x2000000
#define CS_UNREAD_KEYS	0x100000	/* for returning unaccessed keys */
#define CS_ALL_TOKENS	0x200000	/* Return all tokens for a token value 
					   starting at token number  */	
#define CS_FULL_LINE	0x80000	

#define CS_TOK_MASK	0xffff	/* mask retrieving the token number in the 
				   "tok" argument of CS_entry */

enum {CS_DOWN_LEVEL = 0, CS_UP_LEVEL, CS_TOP_LEVEL};
				/* values for argument "action" of function 
				   CS_level */

/* return error code */
#define CS_KEY_NOT_FOUND	-780
#define CS_TOK_NOT_FOUND	-781
#define CS_BUF_TOO_SMALL	-782
#define CS_TOO_MANY_EVENTS	-785

#define CS_OPEN_ERR		-787
#define CS_SEEK_ERR		-788
#define CS_READ_ERR		-789
#define CS_MALLOC_ERR		-790

#define CS_EV_REGISTER_ERR	-792
#define CS_ENV_VAR_ERR		-793
#define CS_END_OF_TEXT		-794
#define CS_KEY_AMBIGUOUS	-795

#define CS_BAD_ARGUMENT		-796
#define CS_CONVERT_ERROR	-797
#define CS_FORMAT_ERROR		-798
#define CS_NO_SUCH_LEVEL	-799

#ifdef __cplusplus
extern "C"
{
#endif

/* public functions */
int CS_event (int syscfg_ev, int *status);
int CS_int_entry (int key, int tok, int buf_size, char *buf);
int CS_name_entry (const char *key, int tok, int buf_size, char *buf);
int CS_entry (const char *key, int tok, int buf_size, char *buf);
int CS_next_line (int tok, int buf_size, char *buf);
char *CS_cfg_name (const char *name);
int CS_control (int action);
void CS_set_sys_cfg (char *sys_cfg_name);
void CS_error(void (*error_func)(char* buf));
void CS_report (const char *msg);
int CS_level (int action);
int CS_entry_int_key (int key, int tok, int buf_size, char *buf);
int CS_parse_control (int action);

void CS_set_single_level (int yes);

#ifdef __cplusplus
}
#endif

#endif			/* #ifndef CS_H */
