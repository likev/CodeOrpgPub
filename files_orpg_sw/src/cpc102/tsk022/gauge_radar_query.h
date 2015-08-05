/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:07 $
 * $Id: gauge_radar_query.h,v 1.3 2011/04/13 22:53:07 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef GAUGE_RADAR_QUERY_H
#define GAUGE_RADAR_QUERY_H

#include <time.h> /* time_t */

/******************************************************************************
    Filename: gauge_radar_query.h

    Description:
    ============
    This is the header for for the DualPol gauges DB query.
    Put all the SQL query fields here.

    To support the SQL DB query, 3 system files need to be configured:

    1. The GAUGE_RADAR database id must be added to ~/include/orpgdat.h

    #define GAUGE_RADAR                 300007 -* 24 hour gauge database *-

    2. The query fields must be added to ~/include/rpgdbm.h

    #include "gauge_radar_query.h"

    3. The new query structure must be added to:

    ~/src/cpc002/tsk006/rpg_sdqs.conf

    and a 'make install' done. I don't know how to do these via snippets.

    rpg_gauges_query {
      src_struct     gauge_radar_query_t # source (msg in LB) data struct
      sdb_struct     gauge_radar_query_t # SDB struct
      src_data_store GAUGE_RADAR         # data store ID for messages.
      data_endian    local

      index_trees {
          start_time
          end_time
      }
    }

    Nathan Bain <nebain@mesonet.org> on Mon, 25 Jan 2010, said:

    "From the time that data is observed to the time an MDF file appears
     on the web (rounded up to whole minutes)...

     Little Washita ARS:  3 minutes
     Fort Cobb ARS:       3 minutes
     Oklahoma Mesonet:    5 minutes
     OKC Micronet:       35 seconds

     There will be some variance. If communications are unusually good,
     the files will become available sooner. If communications are poor
     to some parts of the state, the files will become available later.
     Here are the same numbers again, in expected "worst case" scenarios:

     Little Washita ARS:  4 minutes
     Fort Cobb ARS:       4 minutes
     Oklahoma Mesonet:    6 minutes
     OKC Micronet:        1 minute, 10 seconds

     Also note that with every product cycle, we regenerate files from
     the previous few cycles if new data has come in for those times
     (i.e., records were missing from the previous cycle)."

    Dan Berkowitz says the files should be current to:

     Little Washita ARS:  1994 03/01
     Fort Cobb ARS:       2005 12/24
     Oklahoma Mesonet:    1994 01/01
     OKC Micronet:        2008 05/01

    Change History
    ==============
    DATE      VERSION    PROGRAMMER         NOTES
    --------  -------    ----------         ----------------
    20091204  0000       James Ward         Initial version
******************************************************************************/

typedef struct {
  time_t start_time; /* start time of gauges record */
  time_t end_time;   /* end   time of gauges record */
} gauge_radar_query_t;

#endif /* GAUGE_RADAR_QUERY_H */
