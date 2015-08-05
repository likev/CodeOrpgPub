/*   @(#) hdlcutils.h 99/11/02 Version 1.2   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1993 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 

   This software is furnished  under  a  license and may be used and
   copied only  in  accordance  with  the  terms of such license and
   with the inclusion of the above copyright notice.   This software
   or any other copies thereof may not be provided or otherwise made
   available to any other person.   No title to and ownership of the
   program is hereby transferred.
 
   The information  in  this  software  is subject to change without
   notice and should not  be  considered  as  a  commitment by UconX
   Corporation.
            UconX Corporation
          San Diego, California

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/
  
/*
Modification history:
 
Chg Date       Init  Description
1.  05-MAY-99  mpb   Create.
*/

#define MAX_DATASIZE   4000

typedef struct LinkControl
{
   MPSsd  sd;
   int    link;
   int    rcvcount;
   int    sndcount;
   int    bytecount;
#ifdef WINNT
   time_t start_time;
   time_t end_time;
#else
   struct timespec start_time;
   struct timespec end_time;
#endif /* WINNT */
} LCTRL;


#ifdef ANSI_C
int hdlc_close(MPSsd link);
int hdlc_cmd(MPSsd link, int command);
int hdlc_open(OpenRequest *p_oreq, LCTRL *p_lctl,
             int baud, int encoding, int maxframe, int mode, int modem);
int hdlc_response(MPSsd d, int response, int fail);
int hdlc_stats(LCTRL *in);
int hdlc_stop(MPSsd link);
#else
int hdlc_close();
int hdlc_cmd();
int hdlc_open();
int hdlc_response();
int hdlc_stats();
int hdlc_stop();
#endif /* ANSI_C */
