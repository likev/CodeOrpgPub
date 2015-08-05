/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/03/08 21:03:08 $
 * $Id: prod_decompress.h,v 1.2 2006/03/08 21:03:08 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <orpg.h>
#include <infr.h>
#include <misc.h>
#include <mnttsk_pat_def.h>

static int  Read_options(int argc, char** argv);
static int  ReadFileInfo();
static int  DecompressProd();
static int  WriteDecompProd();
static void Print_usage (char **argv);

/* Externally defined functions */
int Init_attr_tbl( char *file_name, int clear_table );
void* Decompress_product_w_PDB( void *bufptr, int *size );
void* Decompress_product_wo_PDB( void *bufptr, int *size );

