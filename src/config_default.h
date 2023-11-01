

#define CONFIG_COAP_LOG_DEFAULT_LEVEL COAP_LOG_EMERG

#define ACK_TIMEOUT ((coap_fixed_point_t){2,0})
#define ACK_RANDOM_FACTOR ((coap_fixed_point_t){1,0})
#define MAX_RETRANSMIT 4

#define PACKET_LOSS "3,6,7,8" //Example "5%%" "2,3,5,8" "1,2,23,34,78"

#define NON_TIMEOUT ((coap_fixed_point_t){2,0})
#define NON_RECEIVE_TIMEOUT ((coap_fixed_point_t){4,0})
