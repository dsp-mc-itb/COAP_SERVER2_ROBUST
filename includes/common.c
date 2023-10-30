/* minimal CoAP functions
 *
 * Copyright (C) 2018-2023 Olaf Bergmann <bergmann@tzi.org>
 */
#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

#define REPEAT_DELAY_MS 1000
#define MAX_USER 128 /* Maximum length of a user name (i.e., PSK
                      * identity) in bytes. */
#define MAX_KEY   64 /* Maximum length of a key (i.e., PSK) in bytes. */
#define BUFSIZE 100

//int resolve_address(const char *host, const char *service, coap_address_t *dst);

static void
usage(const char *program, const char *version) {
  printf("Usage disable\n");
}
