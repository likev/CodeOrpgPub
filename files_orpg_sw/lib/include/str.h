
/*******************************************************************

    Description: Public header file for STR string manipulation 
    functions

*******************************************************************/

/* 
 * RCS info
 * $Author: eforren
 * $Locker:  $
 * $Date: 2011/05/19 19:35:13 $
 * $Id: str.h,v 1.6 2011/05/19 19:35:13 jing Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#ifndef STR_H
#define STR_H

#ifdef __cplusplus
extern "C"
{
#endif


char *STR_grow (void *str, int length);
char *STR_create (const void *orig_str);
char *STR_copy (char *dest_str, const char *src_str);
char *STR_cat (char *dest_str, const char *src_str);
void STR_free (void *str);
char *STR_append (void *dest_str, const void *src_str, int size);
char *STR_replace (char *str, int offset, int len, char *c_str, int size);
int STR_size (void *str);
char *STR_reset (void *str, int length);
char *STR_gen (char *str, ...);


#ifdef __cplusplus
}
#endif

#endif		
