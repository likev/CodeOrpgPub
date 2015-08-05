/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:05 $
 * $Id: gauge_radar_consts.h,v 1.3 2011/04/13 22:53:05 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef GAUGE_RADAR_CONSTS_H
#define GAUGE_RADAR_CONSTS_H

/******************************************************************************
    Filename: gauge_radar_consts.h

    Description:
    ============
    Declare constants for the gauge radar comparison algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    20091204    0000       James Ward         Initial version.
******************************************************************************/

#define GAUGE_RADAR_DEBUG 0

#define GAUGE_RADAR_DEA_FILE "alg.gauge_radar."

#define ID_LEN        20 /* length of gauge id */
#define MAX_LINE_LEN 100
#define MAX_GAUGES   200

#define SECS_IN_5_MINS 300

/* A gauge line returned by the Python script looks like:
    REDR ,  1.27
*/

#define GAUGE_DATA_START 10

#define YYYYMMDD_LEN 8
#define HHMM_LEN     4

/* GAUGE_RADAR, the gauge database ID, is needed only for the gauge_radar_db.c
 * compile. Normally it would be in ~/include/orpgdat.h.
 * #define GAUGE_RADAR 300007 */

/*************************************************************************
 * We expect the 191 site IDs to be returned by the script in the
 * following order. 26 gauges are not under the KOUN umbrella, and their
 * values are not be saved. 'index' is the 0-based index into the gauges
 * data, 'index orig' is the index into the data returned by the script.
 *
 * 20100428 Chris Calvert confirms that (for gauge location) the range to
 * the first bin is 0. The first bin of actual data is 2 Km out,
 * so DP bins 0-7 (1/4 Km resolution) for all azimuths should always be
 * filled with 0. There is 1 gauge < 2 Km from the radar, NRMN at 0.26 Km,
 * and DP/PPS always show 0 precip. Normally the range to the first bin
 * is read from the Base_data_header:
 *
 *    short surv_range;           -* Range to start of surveillance data,
 *                                   in number of bins. *-
 *
 *    short range_beg_surv;       -* Range to beginning of 1st surveillance
 *                                   bin, in meters. *-
 *
 * These were being returned as:
 *
 *    surv_range 1, range_beg_surv 0 meters
 * -----------------------------------------------------------------------
 * OK Mesonet
 * http://mesonet.org/
 *
 * index index site id
 *       orig
 * ----- ----- -------
 *    0    0   ADAX
 *    1    1   ALTU
 *    2    2   ANTL
 *         3   ARNE - not under the KOUN umbrella
 *         4   BEAV - not under the KOUN umbrella
 *    3    5   BESS
 *    4    6   BIXB
 *    5    7   BLAC
 *         8   BOIS - not under the KOUN umbrella
 *    6    9   BOWL
 *    7   10   BREC
 *    8   11   BRIS
 *        12   BUFF - not under the KOUN umbrella
 *    9   13   BURB
 *   10   14   BURN
 *   11   15   BUTL
 *   12   16   BYAR
 *   13   17   CAMA
 *   14   18   CENT
 *   15   19   CHAN
 *   16   20   CHER
 *   17   21   CHEY
 *   18   22   CHIC
 *   19   23   CLAY
 *        24   CLOU - not under the KOUN umbrella
 *        25   COOK - not under the KOUN umbrella
 *        26   COPA - not under the KOUN umbrella
 *   20   27   DURA
 *   21   28   ELRE
 *   22   29   ERIC
 *   23   30   EUFA
 *   24   31   FAIR
 *   25   32   FORA
 *   26   33   FREE
 *   27   34   FTCB
 *        35   GOOD - not under the KOUN umbrella
 *   28   36   GUTH
 *   29   37   HASK
 *   30   38   HINT
 *   31   39   HOBA
 *   32   40   HOLL
 *        41   HOOK - not under the KOUN umbrella
 *   33   42   HUGO
 *        43   IDAB - not under the KOUN umbrella
 *        44   JAYX - not under the KOUN umbrella
 *        45   KENT - not under the KOUN umbrella
 *   34   46   KETC
 *   35   47   LAHO
 *   36   48   LANE
 *   37   49   MADI
 *   38   50   MANG
 *   39   51   MARE
 *        52   MAYR - not under the KOUN umbrella
 *   40   53   MCAL
 *   41   54   MEDF
 *   42   55   MEDI
 *        56   MIAM - not under the KOUN umbrella
 *   43   57   MINC
 *        58   MTHE - not under the KOUN umbrella
 *   44   59   NEWK
 *        60   NOWA - not under the KOUN umbrella
 *   45   61   OILT
 *   46   62   OKEM
 *   47   63   OKMU
 *   48   64   PAUL
 *   49   65   PAWN
 *   50   66   PERK
 *        67   PRYO - not under the KOUN umbrella
 *   51   68   PUTN
 *   52   69   REDR
 *   53   70   RETR
 *   54   71   RING
 *        72   SALL - not under the KOUN umbrella
 *   55   73   SEIL
 *   56   74   SHAW
 *   57   75   SKIA
 *        76   SLAP - not under the KOUN umbrella
 *   58   77   SPEN
 *   59   78   STIG
 *   60   79   STIL
 *   61   80   STUA
 *   62   81   SULP
 *        82   TAHL - not under the KOUN umbrella
 *        83   TALI - not under the KOUN umbrella
 *   63   84   TIPT
 *   64   85   TISH
 *        86   VINI - not under the KOUN umbrella
 *   65   87   WALT
 *   66   88   WASH
 *   67   89   WATO
 *   68   90   WAUR
 *   69   91   WEAT
 *        92   WEST - not under the KOUN umbrella
 *   70   93   WILB
 *        94   WIST - not under the KOUN umbrella
 *   71   95   WOOD
 *   72   96   WYNO
 *   73   97   NINN
 *   74   98   ACME
 *   75   99   APAC
 *   76  100   HECT
 *   77  101   VANO
 *   78  102   ALV2
 *   79  103   GRA2
 *   80  104   PORT
 *   81  105   INOL
 *       106   NRMN - under the KOUN umbrella, but too close
 *   82  107   CLRM
 *   83  108   NEWP
 *       109   BROK - not under the KOUN umbrella
 *   84  110   MRSH
 *   85  111   ARD2
 *   86  112   FITT
 *   87  113   OKCN
 *   88  114   OKCW
 *   89  115   OKCE
 *   90  116   CARL
 *   91  117   WEBR
 *   92  118   KIN2
 *   93  119   HOLD
 *
 * Little Washita Watershed
 * http://ars.mesonet.org/sites/
 *
 *   94  120   A121
 *   95  121   A124
 *   96  122   A131
 *   97  123   A132
 *   98  124   A133
 *   99  125   A134
 *  100  126   A135
 *  101  127   A136
 *  102  128   A144
 *  103  129   A146
 *  104  130   A148
 *  105  131   A149
 *  106  132   A150
 *  107  133   A152
 *  108  134   A153
 *  109  135   A154
 *  110  136   A156
 *  111  137   A159
 *  112  138   A162
 *  113  139   A182
 *
 * Fort Cobb
 * http://ars.mesonet.org/sites/
 *
 *  114  140   F115
 *  115  141   F108
 *  116  142   F105
 *  117  143   F102
 *  118  144   F113
 *  119  145   F101
 *  120  146   F104
 *  121  147   F114
 *  122  148   F111
 *  123  149   F107
 *  124  150   F106
 *  125  151   F109
 *  126  152   F110
 *  127  153   F112
 *  128  154   F103
 *
 * OKC Micronet
 * http://okc.mesonet.org/
 *
 *  129  155   KCB101
 *  130  156   KCB102
 *  131  157   KCB103
 *  132  158   KCB104
 *  133  159   KCB105
 *  134  160   KCB106
 *  135  161   KCB107
 *  136  162   KCB108
 *  137  163   KCB109
 *  138  164   KCB110
 *  139  165   KNE101
 *  140  166   KNE103
 *  141  167   KNE104
 *  142  168   KNE105
 *  143  169   KNE202
 *  144  170   KNW103
 *  145  171   KNW104
 *  146  172   KNW105
 *  147  173   KNW106
 *  148  174   KNW107
 *  149  175   KNW108
 *  150  176   KNW201
 *  151  177   KNW202
 *  152  178   KSE101
 *  153  179   KSE102
 *  154  180   KSW101
 *  155  181   KSW102
 *  156  182   KSW103
 *  156  183   KSW104
 *  158  184   KSW105
 *  159  185   KSW107
 *  160  186   KSW108
 *  161  187   KSW109
 *  162  188   KSW110
 *  163  189   KSW111
 *  164  190   KSW112

A useful script for counting non-zero gauges is:

cd ~/tmp/gauges;

cat *.csv | \
egrep -v 'ARNE|BEAV|BOIS|BUFF|CLOU|COOK|COPA|GOOD|HOOK|IDAB' | \
egrep -v 'JAYX|KENT|MAYR|MIAM|MTHE|NOWA|PRYO|SALL|SLAP|TAHL' | \
egrep -v 'TALI|VINI|WEST|WIST|BROK|0.0|-' | wc -l
*************************************************************************/

#define MAX_NUM_REQS        10
#define MSECS_PER_MINUTE    60000  /* milli-seconds per minute   */
#define MINS_PER_DAY	    1440
#define OUTBUF_SIZE         350000 /* 360 * 920 + header (bytes) */
#define SECS_PER_MINUTE	    60
#define MINS_PER_HOUR	    60
#define HOURS_PER_DAY	    24
#define SECONDS_PER_DAY     86400;
#define MIN_TIME_SPAN	    15      /* minimum time span is 15 minutes      */
#define MSG_HDR_SIZE        18      /* msg header size (bytes)              */
#define PDB_SIZE            102     /* prod description block size (bytes)  */
#define SYMB_HDR_SIZE       16      /* symb block header size (bytes)       */
#define PACKET_HDR_SIZE     14      /* packet 16 header size (bytes)        */
#define RADIAL_HDR_SIZE     6       /* packet 16 radial header size (bytes) */

#define SYMB_BLK_ENTRY      120     /* offset to symb block (bytes)      */
#define PCKT_HDR_ENTRY      136     /* offset to packet header (bytes)   */
#define PCKT_DATA_ENTRY     150     /* offset to 1st radial data (bytes) */

#define HYDRO_DEA_FILE  "alg.hydromet_adj."

#define DAA_PROD_ID    170
#define DSA_PROD_ID    172

/* For now, the features product is made as a storm product */

#define HOURLY_PROD_ID   179
#define STORM_PROD_ID    180
#define FEATURES_PROD_ID STORM_PROD_ID

#define ICENTER            0
#define JCENTER            0
#define RANGE_SCALE_FACT 250
#define SCALE_FACTOR      10
#define DELTA_ANGLE       10

#define SYMB_OFFSET      120

#define PROMPT1 "No precipitation detected during the specified time span"
#define PROMPT2 "No accumulation records available for the specified time span"

#define DEBUG              FALSE
#define DP_DUA_ACCUM_DEBUG FALSE

#define CDBE_BUF_NAME "DP_MOMENTS_ELEV"
#define DP_BUF_NAME   "DP_LT_ACCUM"
#define PPS_BUF_NAME  "HYADJSCN"

/* For now, the features product is made as a storm product */

#define HOURLY_PRODUCT_NAME   "HOURLY_GAUGE"
#define STORM_PRODUCT_NAME    "STORM_GAUGE"
#define FEATURES_PRODUCT_NAME  STORM_PRODUCT_NAME

#define GAUGE_MISSING -99999

#define NUM_MSGS   16
#define MSG_LEN    81 /* 80 chars + NULL byte */

#define HEADER_LEN MSG_LEN

/* TOLERANCE is used for float comparisons.
 * TRACE is the trace amount threshold */

#define TOLERANCE 0.00000001
#define TRACE     0.01

#define HOURLY   0
#define STORM    1
#define FEATURES 2

#endif /* GAUGE_RADAR_CONSTS_H */
