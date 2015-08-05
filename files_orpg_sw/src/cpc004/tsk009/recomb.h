
/******************************************************************

    This is the private header file for recomb - the 
    super-resolution radar data recombination program.
	
******************************************************************/

/* RCS info */
/* $Author: jing $ */
/* $Locker:  $ */
/* $Date: 2008/03/14 18:51:45 $ */
/* $Id: recomb.h,v 1.3 2008/03/14 18:51:45 jing Exp $ */
/* $Revision:  */
/* $State: */

#ifndef SPRS2NRM_H
#define SPRS2NRM_H

void CR_combine_init (int range_only, int azi_only, int index_azi);
int CR_combine_radials (char *input, char **output, int *length);
int MAIN_output_radial (char *output, int length);
int RCDP_get_recombined_dp_fields (char *rad);
int RCDP_dp_recomb (char *rad1, char *rad2);

#endif
