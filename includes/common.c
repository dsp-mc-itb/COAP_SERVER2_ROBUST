/* minimal CoAP functions
 *
 * Copyright (C) 2018-2023 Olaf Bergmann <bergmann@tzi.org>
 */
#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

//int resolve_address(const char *host, const char *service, coap_address_t *dst);

static void
usage(const char *program, const char *version) {
  printf("Usage disable\n");
}
