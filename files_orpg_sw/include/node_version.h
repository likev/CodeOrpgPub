/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:25 $
 * $Id: node_version.h,v 1.2 2002/12/11 22:10:25 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/**************************************************************************

    Module: 
       node_version.h

    Description: 
       Contains node name and software version information for the current
       version of adaptation data.

 **************************************************************************/


#ifndef NODE_VERSION_H
#define NODE_VERSION_H

typedef struct{

   int version;       /* Software version number (Build # * 10) */
   char node_name[8]; /* Node name - either RPG1, RPG2, or MSCF */

} Node_version_t;

#define NODE_VERSION_MSGID   1

#endif /*DO NOT REMOVE!*/
