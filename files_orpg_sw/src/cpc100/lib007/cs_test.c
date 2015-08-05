/****************************************************************
		
    Module: cs_test.c	
		
    Description: This is the test program for the SC module.

*****************************************************************/

/* 
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/05/14 18:51:49 $
 * $Id: cs_test.c,v 1.17 2002/05/14 18:51:49 eddief Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */  


#include <stdio.h>

#include <infr.h>


/********************************************************************
			
    Description: Test set 1
    
********************************************************************/

void cs_test_1()
{
    char buf [128];
    int ret;

    CS_error ((void (*)())printf);
    
/*    CS_cfg_name("/users/jing/rpg/lib/supp/tcfg1"); */
    CS_cfg_name("/users/jing/cfg/user_profiles");
    CS_control (CS_COMMENT | '#');

    /*  Get the key token from a multiple word key item */
    printf ("CS_entry ret = %d\n", (ret = CS_entry ("Test", 1, 128, buf)));
    if (ret > 0)
	printf ("    %s\n", buf);
    
}

int main ()
{
    cs_test_1();		
    exit (0);
}
