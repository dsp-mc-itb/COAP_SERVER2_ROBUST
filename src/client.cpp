/* minimal CoAP server
 *
 * Copyright (C) 2018-2023 Olaf Bergmann <bergmann@tzi.org>
 */
extern "C" {
  
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <coap3/coap.h>
#include "config.h"
}

extern "C" {
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
}
// #endif
#include "common.c"
#include "camera.cpp"

raspicam::RaspiCam_Cv camera2;
int flags = 0;

static unsigned char _token_data[24]; /* With support for RFC8974 */
coap_binary_t the_token = { 0, _token_data };

typedef struct {
  coap_binary_t *token;
  int observe;
} track_token;

track_token *tracked_tokens = NULL;
size_t tracked_tokens_count = 0;

#define FLAGS_BLOCK 0x01
static coap_optlist_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
const char *server_uri = "coap://192.168.1.150"; // alamat server coap Raspi DDWRT

static coap_uri_t proxy = { {0, NULL}, 0, {0, NULL}, {0, NULL}, (coap_uri_scheme_t)0 };
static int proxy_scheme_option = 0;
static int uri_host_option = 0;
static unsigned int ping_seconds = 0;
static char *root_ca_file = NULL; /* List of trusted Root CAs in PEM */
static char *ca_file = NULL;   /* CA for cert_file - for cert checking in PEM,
                                  DER or PKCS11 URI */
static char *cert_file = NULL; /* certificate and optional private key in PEM,
                                  or PKCS11 URI*/
static size_t repeat_count = 1;
static int ready = 0;
static int doing_getting_block = 0;
static int single_block_requested = 0;
//static uint32_t block_mode = COAP_BLOCK_USE_LIBCOAP; //COAP_BLOCK_USE_LIBCOAP;
//static uint32_t block_mode = COAP_BLOCK_TRY_Q_BLOCK | COAP_BLOCK_USE_LIBCOAP ;
#if CONFIG_COAP_METHOD == 0
  unsigned char msgtype = COAP_MESSAGE_CON;
  static uint32_t block_mode = COAP_BLOCK_USE_LIBCOAP;

#elif CONFIG_COAP_METHOD == 1
  unsigned char msgtype = COAP_MESSAGE_NON;
  static uint32_t block_mode = COAP_BLOCK_TRY_Q_BLOCK | COAP_BLOCK_USE_LIBCOAP ;
#endif
//   //BLock wise
//   block_mode = COAP_BLOCK_USE_LIBCOAP ;
//   msgtype = COAP_MESSAGE_CON; 
// } else if (true){
//   //Robust
//   block_mode = COAP_BLOCK_TRY_Q_BLOCK | COAP_BLOCK_USE_LIBCOAP ;
//   msgtype = COAP_MESSAGE_NON; 
// }

static coap_string_t output_file = { 0, NULL };   /* output file name */
static FILE *file = NULL;   

#define DEFAULT_WAIT_TIME 90

typedef unsigned char method_t;
method_t method = COAP_REQUEST_CODE_PUT; 
coap_block_t block = { .num = 0, .m = 0, .szx = 6 };

unsigned int wait_seconds = DEFAULT_WAIT_TIME; /* default timeout in seconds */
unsigned int wait_ms = 0;
int obs_started = 0;
unsigned int obs_seconds = 30;          /* default observe time */
unsigned int obs_ms = 0;                /* timeout for current subscription */
int obs_ms_reset = 0;
int doing_observe = 0;
static int reliable = 0;
static int add_nl = 0;
static int is_mcast = 0;

//char *image_path = "aaaaa";
char image_path[] = "aaaaa";

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

static coap_oscore_conf_t *oscore_conf = NULL;

static int quit = 0;

static coap_string_t payload = { 12330, (uint8_t*)"BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1BLOCK1----BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2BLOCK2----BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3BLOCK3----BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4BLOCK4----BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5BLOCK5----BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6BLOCK6----BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7BLOCK7----BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8BLOCK8----BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9BLOCK9----BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10BLOCK10--BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11BLOCK11--BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12BLOCK12--BLOCK13 BLOCK 13 BLOCK13 BLOCK 13 BLOCK13-" }; 

/* SIGINT handler: set quit to 1 for graceful termination */
static void
handle_sigint(int signum COAP_UNUSED) {
  quit = 1;
}

static void
free_xmit_data(coap_session_t *session COAP_UNUSED, void *app_ptr) {
  coap_free(app_ptr);
  return;
}

static int
append_to_output(const uint8_t *data, size_t len) {
  size_t written;

  if (!file) {
    if (!output_file.s || (output_file.length && output_file.s[0] == '-')) {
      file = stdout;
    } else {
      if (!(file = fopen((char *)output_file.s, "w"))) {
        perror("fopen");
        return -1;
      }
    }
  }

  do {
    written = fwrite(data, 1, len, file);
    len -= written;
    data += written;
  } while (written && len);
  fflush(file);

  return 0;
}

/* Called after processing the options from the commandline to set
 * Block1, Block2, Q-Block1 or Q-Block2 depending on method. */
static void
set_blocksize(void) {
  static unsigned char buf[4];        /* hack: temporarily take encoded bytes */
  uint16_t opt;
  unsigned int opt_length;

  if (method != COAP_REQUEST_DELETE) {
    if (method == COAP_REQUEST_GET || method == COAP_REQUEST_FETCH) {
      if (coap_q_block_is_supported() && block_mode & COAP_BLOCK_TRY_Q_BLOCK){
        coap_log_info("Using Q BLOCK 2\n");
        opt = COAP_OPTION_Q_BLOCK2;
      }
      else{
        opt = COAP_OPTION_BLOCK2;
        coap_log_info("Using BLOCK 2\n");
      }
    } else {
      coap_log_notice("Q-Block Suppoert : %d\n",coap_q_block_is_supported());
      if (coap_q_block_is_supported() && block_mode & COAP_BLOCK_TRY_Q_BLOCK){
        opt = COAP_OPTION_Q_BLOCK1;
        coap_log_info("Using Q BLOCK 1\n");
      }
      else {
        coap_log_info("Using BLOCK 1\n");
        opt = COAP_OPTION_BLOCK1;
      }
    }

    // block.m = (opt == COAP_OPTION_BLOCK1 || opt == COAP_OPTION_Q_BLOCK1) &&
    //           ((1ull << (block.szx + 4)) < payload.length);

    opt_length = coap_encode_var_safe(buf, sizeof(buf),
                                      (block.num << 4 | block.m << 3 | block.szx));

    coap_insert_optlist(&optlist, coap_new_optlist(opt, opt_length, buf));
  }
}

static void
track_new_token(size_t tokenlen, uint8_t *token) {
  track_token *new_list = (track_token*)realloc(tracked_tokens,
                                  (tracked_tokens_count + 1) * sizeof(tracked_tokens[0]));
  if (!new_list) {
    coap_log_info("Unable to track new token\n");
    return;
  }
  tracked_tokens = new_list;
  tracked_tokens[tracked_tokens_count].token = coap_new_binary(tokenlen);
  if (!tracked_tokens[tracked_tokens_count].token)
    return;
  memcpy(tracked_tokens[tracked_tokens_count].token->s, token, tokenlen);
  tracked_tokens[tracked_tokens_count].observe = doing_observe;
  tracked_tokens_count++;
}

static int
track_check_token(coap_bin_const_t *token) {
  size_t i;

  for (i = 0; i < tracked_tokens_count; i++) {
    if (coap_binary_equal(token, tracked_tokens[i].token)) {
      return 1;
    }
  }
  return 0;
}

static void
track_flush_token(coap_bin_const_t *token, int force) {
  size_t i;

  for (i = 0; i < tracked_tokens_count; i++) {
    if (coap_binary_equal(token, tracked_tokens[i].token)) {
      if (force || !tracked_tokens[i].observe || !obs_started) {
        /* Only remove if not Observing */
        coap_delete_binary(tracked_tokens[i].token);
        if (tracked_tokens_count-i > 1) {
          memmove(&tracked_tokens[i],
                  &tracked_tokens[i+1],
                  (tracked_tokens_count-i-1) * sizeof(tracked_tokens[0]));
        }
        tracked_tokens_count--;
      }
      break;
    }
  }
}

static coap_pdu_t *
coap_new_request(coap_context_t *ctx,
                 coap_session_t *session,
                 method_t m,
                 coap_optlist_t **options,
                 unsigned char *data,
                 size_t length) {
  coap_pdu_t *pdu;
  uint8_t token[8];
  size_t tokenlen;
  (void)ctx;

  if (!(pdu = coap_new_pdu((coap_pdu_type_t)msgtype, (coap_pdu_code_t)m, session))) {
    free_xmit_data(session, data);
    return NULL;
  }

  /*
   * Create unique token for this request for handling unsolicited /
   * delayed responses.
   * Note that only up to 8 bytes are returned
   */
  if (the_token.length > COAP_TOKEN_DEFAULT_MAX) {
    coap_session_new_token(session, &tokenlen, token);
    /* Update the last part 8 bytes of the large token */
    memcpy(&the_token.s[the_token.length - tokenlen], token, tokenlen);
  } else {
    coap_session_new_token(session, &the_token.length, the_token.s);
  }
  track_new_token(the_token.length, the_token.s);
  if (!coap_add_token(pdu, the_token.length, the_token.s)) {
    coap_log_debug("cannot add token to request\n");
  }

  if (options){
    coap_add_optlist_pdu(pdu, options);
    
  }
    coap_add_option(pdu, COAP_OPTION_URI_PATH, 5, (uint8_t *)image_path);
  if (length) {
    /* Let the underlying libcoap decide how this data should be sent */
    coap_add_data_large_request(session, pdu, length, data,
                                free_xmit_data, data);
  }

  return pdu;
}

static int
event_handler(coap_session_t *session COAP_UNUSED,
              const coap_event_t event) {
  coap_log_notice("EVENT TRIGGER\n");
    
  switch (event) {
  case COAP_EVENT_DTLS_CLOSED:
  case COAP_EVENT_TCP_CLOSED:
  case COAP_EVENT_SESSION_CLOSED:
  case COAP_EVENT_OSCORE_DECRYPTION_FAILURE:
  case COAP_EVENT_OSCORE_NOT_ENABLED:
  case COAP_EVENT_OSCORE_NO_PROTECTED_PAYLOAD:
  case COAP_EVENT_OSCORE_NO_SECURITY:
  case COAP_EVENT_OSCORE_INTERNAL_ERROR:
  case COAP_EVENT_OSCORE_DECODE_ERROR:
  case COAP_EVENT_WS_PACKET_SIZE:
  case COAP_EVENT_WS_CLOSED:
    quit = 1;
    break;
  case COAP_EVENT_DTLS_CONNECTED:
  case COAP_EVENT_DTLS_RENEGOTIATE:
  case COAP_EVENT_DTLS_ERROR:
  case COAP_EVENT_TCP_CONNECTED:
  case COAP_EVENT_TCP_FAILED:
  case COAP_EVENT_SESSION_CONNECTED:
  case COAP_EVENT_SESSION_FAILED:
  case COAP_EVENT_PARTIAL_BLOCK:
  case COAP_EVENT_XMIT_BLOCK_FAIL:
  case COAP_EVENT_SERVER_SESSION_NEW:
  case COAP_EVENT_SERVER_SESSION_DEL:
  case COAP_EVENT_BAD_PACKET:
  case COAP_EVENT_MSG_RETRANSMITTED:
  case COAP_EVENT_WS_CONNECTED:
  case COAP_EVENT_KEEPALIVE_FAILURE:
  default:
    break;
  }
  return 0;
}

static void
nack_handler(coap_session_t *session COAP_UNUSED,
             const coap_pdu_t *sent,
             const coap_nack_reason_t reason,
             const coap_mid_t mid COAP_UNUSED) {
  coap_log_notice("NACK TRIGGER\n");
  if (sent) {
    coap_bin_const_t token = coap_pdu_get_token(sent);

    if (!track_check_token(&token)) {
      coap_log_err("nack_handler: Unexpected token\n");
    }
  }

  switch (reason) {
  case COAP_NACK_TOO_MANY_RETRIES:
  case COAP_NACK_NOT_DELIVERABLE:
  case COAP_NACK_RST:
  case COAP_NACK_TLS_FAILED:
  case COAP_NACK_WS_FAILED:
  case COAP_NACK_TLS_LAYER_FAILED:
  case COAP_NACK_WS_LAYER_FAILED:
    coap_log_err("cannot send CoAP pdu\n");
    quit = 1;
    break;
  case COAP_NACK_ICMP_ISSUE:
  case COAP_NACK_BAD_RESPONSE:
  default:
    ;
  }
  return;
}

/*
 * Response handler used for coap_send() responses
 */
static coap_response_t
message_handler(coap_session_t *session COAP_UNUSED,
                const coap_pdu_t *sent,
                const coap_pdu_t *received,
                const coap_mid_t id COAP_UNUSED) {

  coap_opt_t *block_opt;
  coap_opt_iterator_t opt_iter;
  size_t len;
  const uint8_t *databuf;
  size_t offset;
  size_t total;
  coap_pdu_code_t rcv_code = coap_pdu_get_code(received);
  coap_pdu_type_t rcv_type = coap_pdu_get_type(received);
  coap_bin_const_t token = coap_pdu_get_token(received);

  coap_log_info("** process incoming %d.%02d response:\n",
                 COAP_RESPONSE_CLASS(rcv_code), rcv_code & 0x1F);
  if (coap_get_log_level() < COAP_LOG_DEBUG)
    coap_show_pdu(COAP_LOG_INFO, received);

  /* check if this is a response to our original request */
  if (!track_check_token(&token)) {
    /* drop if this was just some message, or send RST in case of notification */
    if (!sent && (rcv_type == COAP_MESSAGE_CON ||
                  rcv_type == COAP_MESSAGE_NON)) {
      /* Cause a CoAP RST to be sent */
      return COAP_RESPONSE_FAIL;
    }
    return COAP_RESPONSE_OK;
  }

  if (rcv_type == COAP_MESSAGE_RST) {
    coap_log_info("got RST\n");
    return COAP_RESPONSE_OK;
  }

  /* output the received data, if any */
  if (COAP_RESPONSE_CLASS(rcv_code) == 2) {

    /* set obs timer if we have successfully subscribed a resource */
    if (doing_observe && !obs_started &&
        coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
      coap_log_debug("observation relationship established, set timeout to %d\n",
                     obs_seconds);
      obs_started = 1;
      obs_ms = obs_seconds * 1000;
      obs_ms_reset = 1;
    }

    if (coap_get_data_large(received, &len, &databuf, &offset, &total)) {
      append_to_output(databuf, len);
      if ((len + offset == total) && add_nl)
        append_to_output((const uint8_t *)"\n", 1);
    }

    /* Check if Block2 option is set */
    block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
    if (!single_block_requested && block_opt) { /* handle Block2 */

      /* TODO: check if we are looking at the correct block number */
      if (coap_opt_block_num(block_opt) == 0) {
        /* See if observe is set in first response */
        ready = doing_observe ? coap_check_option(received,
                                                  COAP_OPTION_OBSERVE, &opt_iter) == NULL : 1;
      }
      if (COAP_OPT_BLOCK_MORE(block_opt)) {
        doing_getting_block = 1;
      } else {
        doing_getting_block = 0;
        if (!is_mcast)
          track_flush_token(&token, 0);
      }
      return COAP_RESPONSE_OK;
    }
  } else {      /* no 2.05 */
    /* check if an error was signaled and output payload if so */
    if (COAP_RESPONSE_CLASS(rcv_code) >= 4) {
      fprintf(stderr, "%d.%02d", COAP_RESPONSE_CLASS(rcv_code),
              rcv_code & 0x1F);
      if (coap_get_data_large(received, &len, &databuf, &offset, &total)) {
        fprintf(stderr, " ");
        while (len--) {
          fprintf(stderr, "%c", isprint(*databuf) ? *databuf : '.');
          databuf++;
        }
      }
      fprintf(stderr, "\n");
      track_flush_token(&token, 1);
    }

  }
  if (!is_mcast)
    track_flush_token(&token, 0);

  /* our job is done, we can exit at any time */
  ready = doing_observe ? coap_check_option(received,
                                            COAP_OPTION_OBSERVE, &opt_iter) == NULL : 1;
  return COAP_RESPONSE_OK;
}

typedef struct {
  unsigned char code;
  const char *media_type;
} content_type_t;

static coap_session_t *
open_session(coap_context_t *ctx,
             coap_proto_t proto,
             coap_address_t *bind_addr,
             coap_address_t *dst,
             const uint8_t *identity,
             size_t identity_len,
             const uint8_t *key,
             size_t key_len) {
  coap_session_t *session;

  if (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS ||
      proto == COAP_PROTO_WSS) {
    /* Encrypted session */
    if (root_ca_file || ca_file || cert_file) {
      /* Setup PKI session */
        //coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
      
        // session = coap_new_client_session_pki(ctx, bind_addr, dst, proto,
        //                                       dtls_pki);
    } else if (identity || key) {
      /* Setup PSK session */
        // coap_dtls_cpsk_t *dtls_psk = setup_psk(identity, identity_len,
        //                                      key, key_len);
        // session = coap_new_client_session_psk2(ctx, bind_addr, dst, proto,
        //                                        dtls_psk);
    } else {
      /* No PKI or PSK defined, as encrypted, use PKI */
        //coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
        // session = coap_new_client_session_pki(ctx, bind_addr, dst, proto,
        //                                       dtls_pki);
    }
  } else {
    /* Non-encrypted session */
      session = coap_new_client_session(ctx, bind_addr, dst, proto);
  }
  if (session && (proto == COAP_PROTO_WS || proto == COAP_PROTO_WSS)) {
    coap_ws_set_host_request(session, &uri.host);
  }
  return session;
}


static coap_session_t *
get_session(coap_context_t *ctx,
            const char *local_addr,
            const char *local_port,
            coap_uri_scheme_t scheme,
            coap_proto_t proto,
            coap_address_t *dst,
            const uint8_t *identity,
            size_t identity_len,
            const uint8_t *key,
            size_t key_len) {
  coap_session_t *session = NULL;

  is_mcast = coap_is_mcast(dst);
  if (local_addr || coap_is_af_unix(dst)) {
    if (coap_is_af_unix(dst)) {
      coap_address_t bind_addr;

      if (local_addr) {
        if (!coap_address_set_unix_domain(&bind_addr,
                                          (const uint8_t *)local_addr,
                                          strlen(local_addr))) {
          fprintf(stderr, "coap_address_set_unix_domain: %s: failed\n",
                  local_addr);
          return NULL;
        }
      } else {
        char buf[COAP_UNIX_PATH_MAX];

        /* Need a unique address */
        snprintf(buf, COAP_UNIX_PATH_MAX,
                 "/tmp/coap-client.%d", (int)getpid());
        if (!coap_address_set_unix_domain(&bind_addr, (const uint8_t *)buf,
                                          strlen(buf))) {
          fprintf(stderr, "coap_address_set_unix_domain: %s: failed\n",
                  buf);
          remove(buf);
          return NULL;
        }
        (void)remove(buf);
      }
      session = open_session(ctx, proto, &bind_addr, dst,
                             identity, identity_len, key, key_len);
    } else {
      coap_addr_info_t *info_list = NULL;
      coap_addr_info_t *info;
      coap_str_const_t local;
      uint16_t port = local_port ? atoi(local_port) : 0;

      local.s = (const uint8_t *)local_addr;
      local.length = strlen(local_addr);
      /* resolve local address where data should be sent from */
      info_list = coap_resolve_address_info(&local, port, port, port, port,
                                            AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL,
                                            1 << scheme,
                                            COAP_RESOLVE_TYPE_LOCAL);
      if (!info_list) {
        fprintf(stderr, "coap_resolve_address_info: %s: failed\n", local_addr);
        return NULL;
      }

      /* iterate through results until success */
      for (info = info_list; info != NULL; info = info->next) {
        session = open_session(ctx, proto, &info->addr, dst,
                               identity, identity_len, key, key_len);
        if (session)
          break;
      }
      coap_free_address_info(info_list);
    }
  } else if (local_port) {
    coap_address_t bind_addr;

    coap_address_init(&bind_addr);
    bind_addr.size = dst->size;
    bind_addr.addr.sa.sa_family = dst->addr.sa.sa_family;
    coap_address_set_port(&bind_addr, atoi(local_port));
    session = open_session(ctx, proto, &bind_addr, dst,
                           identity, identity_len, key, key_len);
  } else {
    session = open_session(ctx, proto, NULL, dst,
                           identity, identity_len, key, key_len);
  }
  return session;
}

void send_image(coap_context_t *ctx,coap_session_t *session,raspicam::RaspiCam_Cv* camera) {
    
    coap_pdu_t *request = NULL;
    uint8_t token[8];
    size_t tokenlen; 
    payload.length = 0;
    payload.s = NULL; 
    struct timeval before;
    struct timeval after;
    long long time_elapsed_us;
    gettimeofday(&before, NULL);
    ready = 0;

    cv::Mat image;
    coap_log_notice("Taking Frame !\n");
    camera->grab();
    camera->retrieve (image);
     
    cv::imwrite("../output/image4.jpg", image, {cv::IMWRITE_JPEG_QUALITY, 60});
    gettimeofday(&after, NULL);
    
    time_elapsed_us = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
    coap_log_notice("Write to .jpeg !\n");
    FILE *file;
    char *filename = "../output/image4.jpg";  // Replace with your file name
    file = fopen(filename, "rb");

    if (file == NULL) {
        coap_log_err("Error opening file\n");
        return;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the uint8_t array
    uint8_t *data = (uint8_t *)malloc(file_size);

    if (data == NULL) {
        coap_log_err("Memory allocation error\n");
        fclose(file);
        return;
    }

    // Read the file into the uint8_t array
    size_t bytes_read = fread(data, 1, file_size, file);

    if (bytes_read != file_size) {
        coap_log_err("Error Reading file\n");
        free(data);
        fclose(file);
        return;
    }

    fclose(file);

    payload.s = data;
    payload.length = file_size;

    coap_log_notice("Value of myUInt8: %u\n", payload.s);
    coap_log_notice("Value of mySizeT: %zu\n", payload.length);

    coap_log_notice("Create New Request\n");
    if (!(request = coap_new_request(ctx, session, method, &optlist, payload.s,
                                payload.length))) {
      coap_log_err("Create New Request\n");
      return;
    }
  
    if (is_mcast && wait_seconds == DEFAULT_WAIT_TIME)
    /* Allow for other servers to respond within DEFAULT_LEISURE RFC7252 8.2 */
    wait_seconds = coap_session_get_default_leisure(session).integer_part + 1;

    wait_ms = wait_seconds * 1000;
    coap_log_debug("timeout is set to %u seconds\n", wait_seconds);
    coap_log_notice("sending CoAP request:\n");  
    coap_show_pdu(COAP_LOG_INFO, request);
    
    if (coap_send(session, request) == COAP_INVALID_MID) {
      coap_log_err("cannot send CoAP pdu\n");
      quit = 1; 
    }
   
    coap_log(LOG_NOTICE, "Clean Up\n");
    if (optlist) {
        coap_delete_optlist(optlist);
        optlist = NULL;
    }
    return;
} 

int main(int argc, char **argv) {
  coap_context_t  *ctx = NULL; //ok
  coap_session_t *session = NULL; //ok
  coap_address_t dst; //ok 
  int result = -1; //ok
  int exit_code = 0; //ok
  coap_pdu_t *pdu; //ok
  static coap_str_const_t server; //ok
  uint16_t port = COAP_DEFAULT_PORT; //ok
  char port_str[NI_MAXSERV] = ""; //ok
  char node_str[NI_MAXHOST] = ""; //ok
  int opt; //ok
  coap_log_t log_level = CONFIG_COAP_LOG_DEFAULT_LEVEL;//COAP_LOG_INFO; //ok
  int create_uri_opts = 1; //ok
  size_t i; //ok
  coap_uri_scheme_t scheme; //ok
  coap_proto_t proto; //ok
  uint32_t repeat_ms = REPEAT_DELAY_MS; //ok
  uint8_t *data = NULL; //ok
  size_t data_len = 0; //ok
  coap_addr_info_t *info_list = NULL; //ok
  static unsigned char buf[BUFSIZE];
  cv::Mat image2;
#ifndef _WIN32
  struct sigaction sa;
#endif
  /* Initialize libcoap library */
  
  coap_startup();
  coap_set_log_level(log_level);
  coap_log_notice("Coap_Startup\n");
  coap_log(LOG_NOTICE, "Cannot add debug packet loss!\n");
  
  if (!coap_debug_set_packet_loss(PACKET_LOSS)) {
        coap_log(LOG_NOTICE, "Cannot add debug packet loss!\n");
  };
  while ((opt = getopt(argc, argv,
                       "a:b:c:e:f:h:j:k:l:m:no:p:rs:t:u:v:wA:B:C:E:G:H:J:K:L:M:NO:P:R:T:UV:X:")) != -1) {
    switch (opt) {
    case 'a':
      strncpy(node_str, optarg, NI_MAXHOST - 1);
      node_str[NI_MAXHOST - 1] = '\0';
      break;
    case 'B':
      wait_seconds = atoi(optarg);
      break;
    
    case 'p':
      strncpy(port_str, optarg, NI_MAXSERV - 1);
      port_str[NI_MAXSERV - 1] = '\0';
      break;
    case 'w':
      add_nl = 1;
      break;
    case 'N':
      msgtype = COAP_MESSAGE_NON;
      break;
    case 'o':
      output_file.length = strlen(optarg);
      output_file.s = (unsigned char *)coap_malloc(output_file.length + 1);

      if (!output_file.s) {
        fprintf(stderr, "cannot set output file: insufficient memory\n");
        goto failed;
      } else {
        /* copy filename including trailing zero */
        memcpy(output_file.s, optarg, output_file.length + 1);
      }
      break;
    case 'U':
      create_uri_opts = 0;
      break;
    case 'l':
      if (!coap_debug_set_packet_loss(optarg)) {
        usage(argv[0], LIBCOAP_PACKAGE_VERSION);
        goto failed;
      }
      break;
    case 'r':
      reliable = coap_tcp_is_supported();
      break;
    case 'K':
      ping_seconds = atoi(optarg);
      break;
    
    case 'G':
      repeat_count = atoi(optarg);
      if (!repeat_count || repeat_count > 255) {
        fprintf(stderr, "'-G count' has to be > 0 and < 256\n");
        repeat_count = 1;
      }
      break;
    default:
      usage(argv[0], LIBCOAP_PACKAGE_VERSION);
      goto failed;
    }
  }

#ifdef _WIN32
  signal(SIGINT, handle_sigint);
#else
  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = handle_sigint;
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  /* So we do not exit on a SIGPIPE */
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, NULL);
#endif

  if (coap_split_uri((unsigned char *)server_uri, strlen(server_uri), &uri) == -1)
      { printf("CoAP server uri error"); }

    server = uri.host;
    port = proxy_scheme_option ? proxy.port : uri.port;
    scheme = proxy_scheme_option ? proxy.scheme : uri.scheme;

  /* resolve destination address where data should be sent */
  info_list = coap_resolve_address_info(&server, port, port, port, port,
                                        0,
                                        1 << scheme,
                                        COAP_RESOLVE_TYPE_REMOTE);

  if (info_list == NULL) {
    coap_log_err("failed to resolve address\n");
    goto failed;
  }
  proto = info_list->proto;
  memcpy(&dst, &info_list->addr, sizeof(dst));
  coap_free_address_info(info_list);

  ctx = coap_new_context(NULL);
  if (!ctx) {
    coap_log_emerg("cannot create context\n");
    goto failed;
  }

  coap_context_set_keepalive(ctx, ping_seconds);
  coap_context_set_block_mode(ctx, block_mode);
//   if (csm_max_message_size)
//     coap_context_set_csm_max_message_size(ctx, csm_max_message_size);
  coap_register_response_handler(ctx, message_handler);
  coap_register_event_handler(ctx, event_handler);
  coap_register_nack_handler(ctx, nack_handler);
  if (the_token.length > COAP_TOKEN_DEFAULT_MAX)
    coap_context_set_max_token_size(ctx, the_token.length);

  session = get_session(ctx,
                        node_str[0] ? node_str : NULL,
                        port_str[0] ? port_str : NULL,
                        scheme,
                        proto,
                        &dst,
                        NULL,
                        0,
                        NULL,
                        0
                       );

  if (!session) {
    coap_log_err("cannot create client session\n");
    goto failed;
  }
  /*
   * Prime the base token value, which coap_session_new_token() will increment
   * every time it is called to get an unique token.
   * [Option '-T token' is used to seed a different value]
   * Note that only the first 8 bytes of the token are used as the prime.
   */
  coap_session_init_token(session, the_token.length, the_token.s);

  /* Convert provided uri into CoAP options */
  if (coap_uri_into_options(&uri, !uri_host_option && !proxy.host.length ?
                            &dst : NULL,
                            &optlist, create_uri_opts,
                            buf, sizeof(buf)) < 0) {
    coap_log_err("Failed to create options for URI\n");
    goto failed;
  }

  /* set block option if requested at commandline */
  
  set_blocksize();
  
  if (initialize_camera(&camera2) == -1){
    coap_log_err("Error initialize Camera !\n");
    goto failed;
  }
  coap_log_notice("send first frame\n");
  send_image(ctx,session,&camera2);
 
  while (!quit &&                /* immediate quit not required .. and .. */
         (tracked_tokens_count || /* token not responded to or still observe */
          is_mcast ||             /* mcast active */
          repeat_count ||         /* more repeat transmissions to go */
          coap_io_pending(ctx))) { /* i/o not yet complete */
    uint32_t timeout_ms;
    /*
     * 3 factors determine how long to wait in coap_io_process()
     *   Remaining overall wait time (wait_ms)
     *   Remaining overall observe unsolicited response time (obs_ms)
     *   Delay of up to one second before sending off the next request
     */
    if (obs_ms) {
      timeout_ms = min(wait_ms, obs_ms);
    } else {
      timeout_ms = wait_ms;
    }
    if (repeat_count) {
      timeout_ms = min(timeout_ms, repeat_ms);
    }

    result = coap_io_process(ctx, timeout_ms);

    if (result >= 0) {
      if (wait_ms > 0) {
        if ((unsigned)result >= wait_ms) {
          coap_log_info("timeout\n");
          break;
        } else {
          wait_ms -= result;
        }
      }
      if (obs_ms > 0 && !obs_ms_reset) {
        if ((unsigned)result >= obs_ms) {
          coap_log_debug("clear observation relationship\n");
          for (i = 0; i < tracked_tokens_count; i++) {
            if (tracked_tokens[i].observe) {
              coap_cancel_observe(session, tracked_tokens[i].token, (coap_pdu_type_t)msgtype);
              tracked_tokens[i].observe = 0;
              coap_io_process(ctx, COAP_IO_NO_WAIT);
            }
          }
          doing_observe = 0;

          /* make sure that the obs timer does not fire again */
          obs_ms = 0;
          obs_seconds = 0;
        } else {
          obs_ms -= result;
        }
      }
      if (ready && repeat_count) {
        /* Send off next request if appropriate */
        if (repeat_ms > (unsigned)result) {
          repeat_ms -= (unsigned)result;
        } else {
          /* Doing this once a second */
          repeat_ms = REPEAT_DELAY_MS;
          coap_log_notice("Prepare send image\n");
          send_image(ctx,session,&camera2);
          
        }
      }
      obs_ms_reset = 0;
    }
  }

  exit_code = 0;

finish:
  
  /* Clean up library usage */
  coap_session_release(session); 
  coap_free_context(ctx);
  coap_cleanup();

  /* Clean up local usage */
  for (i = 0; i < tracked_tokens_count; i++) {
    coap_delete_binary(tracked_tokens[i].token);
  }
  free(tracked_tokens);
  coap_delete_optlist(optlist);
  camera2.release();
  return exit_code;

failed:
  exit_code = 1;
  coap_log_crit("FAILED EXIT CODE\n");
  goto finish;
}