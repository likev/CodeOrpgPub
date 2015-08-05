/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/18 18:23:20 $
 * $Id: rpgcs_miscellaneous.h,v 1.5 2007/06/18 18:23:20 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef RPGCS_MISCELLANEOUS_H
#define RPGCS_MISCELLANEOUS_H

#include <stdlib.h>
#include <rpgc.h>

#define RPGCS_CHAR_BIT		8

#ifdef __cplusplus
extern "C"
{
#endif

int RPGCS_bit_test( unsigned char *data, int off );
int RPGCS_bit_test_short( unsigned char *data, int off );
int RPGCS_bit_clear( unsigned char *data, int off );
int RPGCS_bit_clear_short( unsigned char *data, int off );
int RPGCS_bit_set( unsigned char *data, int off );
int RPGCS_bit_set_short( unsigned char *data, int off );
int RPGCS_define_clist( int *list, int size );
int RPGCS_add_top_clist( int value, int *list );
int RPGCS_add_bottom_clist( int value, int *list );
int RPGCS_remove_top_clist( int *list, int *value );
int RPGCS_remove_bottom_clist( int *list, int *value );
unsigned int RPGC_set_mssw_to_uint( unsigned int value );
void RPGCS_full_name( char *file_name, char *full_name );

#ifdef __cplusplus
}
#endif

#endif
