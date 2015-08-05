/****************************************************************
		
	File: rmtd_user_func_set.c	
				
	2/24/94

	Purpose: This module contains functions that must be
	compiled when generating a RMT server. The first is a
	shell main function and the second is a function that reads
	the user function pointers. A main function should not
	be compiled and put in a library. The second function
	relies on the C preprocessor and the user provided information
	in rmt_user_def.h.

	Files used: rmt_user_def.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/03/05 22:08:04 $
 * $Id: rmtd_user_func_set.c,v 1.22 2009/03/05 22:08:04 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <stdio.h>
#include <stdlib.h>


/*** Local include files ***/

#include <rmt.h>
/* #include "rmt_def.h" */
#include "rmt_user_def.h"

static void Set_up_user_functions (
      int (**user_func) (int, char *, char **)	);
			/* returns the user function pointers */


/****************************************************************
			
	main()				Date: 2/24/94

	This is the main function for the RMT server. It calls 
	RMTD_server_main, which is the true main function, and never
	returns. We use this shell to avoid to put the main in 
	the RMT library.
*/

int
main 
  (
      int argc,
      char **argv,
      char **envp		/* The Unix c function call arguments */
  )
{

    /* Initialize the server */
    if (RMTD_server_init (argc, argv, envp, 
				Set_up_user_functions) != 0)
        exit (1);

#ifdef RMT_SECURITY_ON
    /* Initialize the security check module */
    if (SECD_initialize_sec_rmtd () == RMT_ABORT)
	exit (1);
#endif

#ifdef RMT_USER_FUNCS_INIT_CALLED
    /* Initialize the user server functions */
    if (RMT_initialize_user_funcs () == RMT_ABORT)
	exit (1);
#endif

    /* call the main program */
    RMTD_server_main ();

    exit (0);
}

/****************************************************************

	UFSD_get_client ()

	This function calls SECD_get_client if RMT_SECURITY_ON
	is set.
*/

int 
  UFSD_get_client
(
    int fd
)
{

#ifdef RMT_SECURITY_ON
    return (SECD_get_client (fd));
#else
    return (0);
#endif

}

/****************************************************************
			
	Set_up_user_functions()	Date: 2/24/94

	This function sets up the user defined remote function
	pointers. Because this function uses C preprocessor and
	the user provided information in rmt_user_def.h, it can 
	not be precompiled and put in the RMT library.

	This function returns the user function pointers in the 
	function pointer array of "user_func".

*/


static void 
Set_up_user_functions 
(
      int (**user_func) (int, char *, char **)	/* returns the user function pointers */
)
{
    extern int Rmt_user_func_1_server (int, char *, char **);
    extern int Rmt_user_func_2_server (int, char *, char **);
    extern int Rmt_user_func_3_server (int, char *, char **);
    extern int Rmt_user_func_4_server (int, char *, char **);
    extern int Rmt_user_func_5_server (int, char *, char **);
    extern int Rmt_user_func_6_server (int, char *, char **);
    extern int Rmt_user_func_7_server (int, char *, char **);
    extern int Rmt_user_func_8_server (int, char *, char **);
    extern int Rmt_user_func_9_server (int, char *, char **);
    extern int Rmt_user_func_10_server (int, char *, char **);
    extern int Rmt_user_func_11_server (int, char *, char **);
    extern int Rmt_user_func_12_server (int, char *, char **);
    extern int Rmt_user_func_13_server (int, char *, char **);
    extern int Rmt_user_func_14_server (int, char *, char **);
    extern int Rmt_user_func_15_server (int, char *, char **);
    extern int Rmt_user_func_16_server (int, char *, char **);
    extern int Rmt_user_func_17_server (int, char *, char **);
    extern int Rmt_user_func_18_server (int, char *, char **);
    extern int Rmt_user_func_19_server (int, char *, char **);
    extern int Rmt_user_func_20_server (int, char *, char **);
    extern int Rmt_user_func_21_server (int, char *, char **);
    extern int Rmt_user_func_22_server (int, char *, char **);
    extern int Rmt_user_func_23_server (int, char *, char **);
    extern int Rmt_user_func_24_server (int, char *, char **);

#if Rmt_def_user_func >= 1
    user_func[1 - 1] = Rmt_user_func_1_server;
#endif

#if Rmt_def_user_func >= 2
    user_func[2 - 1] = Rmt_user_func_2_server;
#endif

#if Rmt_def_user_func >= 3
    user_func[3 - 1] = Rmt_user_func_3_server;
#endif

#if Rmt_def_user_func >= 4
    user_func[4 - 1] = Rmt_user_func_4_server;
#endif

#if Rmt_def_user_func >= 5
    user_func[5 - 1] = Rmt_user_func_5_server;
#endif

#if Rmt_def_user_func >= 6
    user_func[6 - 1] = Rmt_user_func_6_server;
#endif

#if Rmt_def_user_func >= 7
    user_func[7 - 1] = Rmt_user_func_7_server;
#endif

#if Rmt_def_user_func >= 8
    user_func[8 - 1] = Rmt_user_func_8_server;
#endif

#if Rmt_def_user_func >= 9
    user_func[9 - 1] = Rmt_user_func_9_server;
#endif

#if Rmt_def_user_func >= 10
    user_func[10 - 1] = Rmt_user_func_10_server;
#endif

#if Rmt_def_user_func >= 11
    user_func[11 - 1] = Rmt_user_func_11_server;
#endif

#if Rmt_def_user_func >= 12
    user_func[12 - 1] = Rmt_user_func_12_server;
#endif

#if Rmt_def_user_func >= 13
    user_func[13 - 1] = Rmt_user_func_13_server;
#endif

#if Rmt_def_user_func >= 14
    user_func[14 - 1] = Rmt_user_func_14_server;
#endif

#if Rmt_def_user_func >= 15
    user_func[15 - 1] = Rmt_user_func_15_server;
#endif

#if Rmt_def_user_func >= 16
    user_func[16 - 1] = Rmt_user_func_16_server;
#endif

#if Rmt_def_user_func >= 17
    user_func[17 - 1] = Rmt_user_func_17_server;
#endif

#if Rmt_def_user_func >= 18
    user_func[18 - 1] = Rmt_user_func_18_server;
#endif

#if Rmt_def_user_func >= 19
    user_func[19 - 1] = Rmt_user_func_19_server;
#endif

#if Rmt_def_user_func >= 20
    user_func[20 - 1] = Rmt_user_func_20_server;
#endif

#if Rmt_def_user_func >= 21
    user_func[21 - 1] = Rmt_user_func_21_server;
#endif

#if Rmt_def_user_func >= 22
    user_func[22 - 1] = Rmt_user_func_22_server;
#endif

#if Rmt_def_user_func >= 23
    user_func[23 - 1] = Rmt_user_func_23_server;
#endif

#if Rmt_def_user_func >= 24
    user_func[24 - 1] = Rmt_user_func_24_server;
#endif

    return;
}
