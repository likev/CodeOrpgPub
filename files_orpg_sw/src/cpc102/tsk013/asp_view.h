#include <orpg.h>
#include <prod_gen_msg.h>
#include <packet_28.h>
#include <orpg_product.h>
#include <gtk/gtk.h>

/* General compression macro definitions. */
#define MIN_BYTES_TO_COMPRESS    1000

/* Macro defintions used by the bzip2 compressor. */
#define RPG_BZIP2_MIN_BLOCK_SIZE_BYTES   100000  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MIN_BLOCK_SIZE              1  /* corresponds to 100 Kbytes */
#define RPG_BZIP2_MAX_BLOCK_SIZE              9  /* corresponds to 900 Kbytes */
#define RPG_BZIP2_WORK_FACTOR                30  /* the recommended default */
#define RPG_BZIP2_NOT_VERBOSE                 0  /* turns off verbosity */
#define RPG_BZIP2_NOT_SMALL                   0  /* does not use small version */
   
#define MAX_PROD_SIZE                    300000
#define FILE_NAME_SIZE                      128
#define MSG_PRODUCT_LEN                      60
#define WMO_HEADER_SIZE                      30
   
#define STATUS_PROD_CODE                    152  /* product code for ASP 
                                                    (Archive III Status Product). */
typedef struct wmo_header {
    char form_type[2];
    char data_type[2];
    char distribution[2];
    char space1;
    char originator[4];
    char space2;
    char date_time[6];
    char extra;
} WMO_header_t;

typedef struct awips_header {
    char category[3];
    char product[3];
    char eoh[2];
} AWIPS_header_t;

/* Function Prototypes. */
GtkWidget* _create_filter_window(void);
int generate_packet(char ***myPacket, char **radarName, int *numPackets , char* filename);

