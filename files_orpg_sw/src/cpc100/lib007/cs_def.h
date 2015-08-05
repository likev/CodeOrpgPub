
/*******************************************************************

    Module: cs_def.h

    Description: private header file for the CS module.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:40:05 $
 * $Id: cs_def.h,v 1.5 2005/09/14 15:40:05 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#ifndef CS_DEF_H
#define CS_DEF_H

#define CS_NAME_SIZE	128	/* maximum name size */

#define AMBIGUOUS_KEY	0x1	/* This key is ambiguous - not used */
#define INT_KEY		0X2	/* this key is an integer key */
#define LINE_READ	0X4	/* this line is read - not used */
#define KEY_EXPANSION	0X8	/* the key contains environ. vars */

typedef struct {		/* struct for a SC entry */
    char *key;			/* configuration key */
    int ikey;			/* integer configuration key */
    short len;			/* length of the key */
    char flags;			/* flags: AMBIGUOUS_KEY, INT_KEY, LINE_READ */
    char level;			/* level number used for multi-level config */
    int line_number;		/* line number for error printing */
} Cs_entry;

#define SPACE_ESCAPE ((char)255)

typedef struct {
   char *cs_buf;   		/* pointer to cfg text */
   Cs_entry *cfg_list;		/* list of configuration entries */
   int n_cfgs;			/* Number of config entries */
   int line;			/* current line */
   int token;			/* current token */
   int level;			/* current level */
   int index;			/* current entry index */
   int start;			/* current search starting entry */
   char key_optional;		/* optional key mode */
   char single_level;		/* single level CS file - "{" and "}" are not 
				   treated as control characters. */
   char comment_char;		/* char leading comment lines */
   char need_cfg_read;		/* configuration update is needed */
   char *cfg_name;	        /* configuration source name */
} cs_cfg_t;


/* private functions */
int CS_update_cfg (cs_cfg_t *cfg);
void CS_print_err (int arg);
char *CS_err_buf (void);
int CS_gen_table (cs_cfg_t *cfg);
int CS_parse_control (int action);
void CS_set_current_cfg (cs_cfg_t *cfg);
cs_cfg_t *CS_get_current_cfg ();



#endif			/* #ifndef CS_DEF_H */

