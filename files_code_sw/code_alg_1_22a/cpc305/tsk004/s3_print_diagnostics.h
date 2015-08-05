/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/************************************************************************
Module:         s3_print_diagnostics.h

Description:    include file for the print diagnostics module.
                which creates a formatted screen output for QC
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                    
Version 1.1,  June 2004    T. Ganger
              Misc cleanup
              
Version 1.2   April 2006   T. Ganger
              Added symbology block print function and a product to disk
                   output function.
              Renamed module and other misc cleanup 
              
Version 1.3   February 2008    T. Ganger
              Created unique filenames with respect to other algorithms.
              

$Id$
************************************************************************/

#ifndef _S3_PRINT_DIAGNOSTICS_H_
#define _S3_PRINT_DIAGNOSTICS_H_

/* system includes ---------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>

/* ORPG   includes ---------------------------------------------------- */
#include <product.h>
#include <rpgc.h>

/* constants/definitions ---------------------------------------------- */
#define FALSE 0
#define TRUE 1



/* symbology header block structure (includes first layer div & length) */
typedef struct {
  short divider;
  short block_id;
  int block_length;
  short num_layers;
  short divider2;
  int layer_length;
  } sym_block;




/* function prototypes ------------------------------------------------ */
int print_message_header(char* buffer);
int print_pdb_header(char* buffer);
int print_symbology_header(char *c_ptr);
void product_to_disk(char * output_buf, int outlen, char *pname, 
                            short ele_idx);


#endif

