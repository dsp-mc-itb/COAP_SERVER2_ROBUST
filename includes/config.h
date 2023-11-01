
#define REPEAT_DELAY_MS 100
#define MAX_USER 128 /* Maximum length of a user name (i.e., PSK
                      * identity) in bytes. */
#define MAX_KEY   64 /* Maximum length of a key (i.e., PSK) in bytes. */
#define BUFSIZE 100
#define CONFIG_COAP_LOG_DEFAULT_LEVEL COAP_LOG_EMERG

// #define _COAP_LOG_EMERG  0
// #define _COAP_LOG_ALERT  1
// #define _COAP_LOG_CRIT   2
// #define _COAP_LOG_ERR    3
// #define _COAP_LOG_WARN   4
// #define _COAP_LOG_NOTICE 5
// #define _COAP_LOG_INFO   6
// #define _COAP_LOG_DEBUG  7
// #define _COAP_LOG_OSCORE 8

#define CONFIG_COAP_METHOD 1
// 0 = BLOCK WISE
// 1 = ROBUST

#define ACK_TIMEOUT ((coap_fixed_point_t){2,0})
#define ACK_RANDOM_FACTOR ((coap_fixed_point_t){1,0})
#define MAX_RETRANSMIT 4

#define PACKET_LOSS "0%%" //Example "5%%" "2,3,5,8" "1,2,23,34,78"

#define NON_TIMEOUT (coap_fixed_point_t){2,0}
#define NON_RECEIVE_TIMEOUT ((coap_fixed_point_t){4,0})
