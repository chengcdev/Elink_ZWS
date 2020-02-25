/* debug.c -- debug utilities
 *
 * Copyright (C) 2010--2012,2014 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include "config.h"

#if defined(HAVE_ASSERT_H) && !defined(assert)
# include <assert.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "debug_coap.h"
#include "net.h"

#define PRINTF   coap_printf

static coap_log_t maxlog = LOG_WARNING;	/* default maximum log level */

const char *coap_package_name() {
  return PACKAGE_NAME;
}

const char *coap_package_version() {
  return PACKAGE_STRING;
}

coap_log_t 
coap_get_log_level() {
  return maxlog;
}

void
coap_set_log_level(coap_log_t level) {
  maxlog = level;
}

/* this array has the same order as the type log_t */
static char *loglevels[] = {
  "EMRG", "ALRT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBG" 
};

#ifdef HAVE_TIME_H

static inline size_t
print_timestamp(char *s, size_t len, coap_tick_t t) {
  struct tm *tmp;
  time_t now = clock_offset + (t / COAP_TICKS_PER_SECOND);
  tmp = localtime(&now);
  return strftime(s, len, "%b %d %H:%M:%S", tmp);
}

#else /* alternative implementation: just print the timestamp */

static inline size_t
print_timestamp(char *s, size_t len, coap_tick_t t) {
#ifdef HAVE_SNPRINTF
  return snprintf(s, len, "%u.%03u", 
		  (unsigned int)(clock_offset + (t / COAP_TICKS_PER_SECOND)), 
		  (unsigned int)(t % COAP_TICKS_PER_SECOND));
#else /* HAVE_SNPRINTF */
  /* @todo do manual conversion of timestamp */
  return 0;
#endif /* HAVE_SNPRINTF */
}

#endif /* HAVE_TIME_H */

#ifndef NDEBUG

unsigned int
print_readable( const unsigned char *data, unsigned int len,
		unsigned char *result, unsigned int buflen, int encode_always ) {
  const unsigned char hex[] = "0123456789ABCDEF";
  unsigned int cnt = 0;
  assert(data || len == 0);

  if (buflen == 0 || len == 0)
    return 0;

  while (len) {
    if (!encode_always && isprint(*data)) {
      if (cnt == buflen)
	break;
      *result++ = *data;
      ++cnt;
    } else {
      if (cnt+4 < buflen) {
	*result++ = '\\';
	*result++ = 'x';
	*result++ = hex[(*data & 0xf0) >> 4];
	*result++ = hex[*data & 0x0f];
	cnt += 4;
      } else
	break;
    }

    ++data; --len;
  }

  *result = '\0';
  return cnt;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

size_t
coap_print_addr(const struct coap_address_t *addr, unsigned char *buf, size_t len) {
    const void *addrptr = NULL;
    in_port_t port;
    unsigned char *p = buf;

    switch (addr->addr.sa.sa_family) {
    case AF_INET:
      addrptr = &addr->addr.sin.sin_addr;
      port = ntohs(addr->addr.sin.sin_port);

      break;
//    case AF_INET6:
//      if (len < 7) /* do not proceed if buffer is even too short for [::]:0 */
//        return 0;
//
//      *p++ = '[';
//
//      addrptr = &addr->addr.sin6.sin6_addr;
//      port = ntohs(addr->addr.sin6.sin6_port);
//
//      break;
    default:
      memcpy(buf, "(unknown address type)", min(22, len));
      break; min(22, len);
    }
//    coap_printf("addr :%s \n",inet_ntoa(addr->addr.sin.sin_addr));
    snprintf(p,len,"%s:%d",inet_ntoa(addr->addr.sin.sin_addr),port);
    return buf + len - p;
}

void
coap_show_pdu(const coap_pdu_t *pdu) {
    return;
  unsigned char buf[80]; /* need some space for output creation */

  PRINTF("v:%d t:%d oc:%d c:%d id:%u", 
	  pdu->hdr->version, pdu->hdr->type,
	  pdu->hdr->token_length, pdu->hdr->code, pdu->hdr->id);

  /* show options, if any */
  if (pdu->hdr->token_length) {
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    coap_option_iterator_init((coap_pdu_t *)pdu, &opt_iter, COAP_OPT_ALL);

    PRINTF(" o:");
    while ((option = coap_option_next(&opt_iter))) {

      if (print_readable(COAP_OPT_VALUE(option), 
			 COAP_OPT_LENGTH(option), 
			 buf, sizeof(buf), 0))
	PRINTF(" %d:%s", opt_iter.type, buf);
    }
  }
  if (pdu->data){
  if (pdu->data < (unsigned char *)pdu->hdr + pdu->length) {
    print_readable(pdu->data,
		   (unsigned char *)pdu->hdr + pdu->length - pdu->data,
		   buf, sizeof(buf), 0 );
    PRINTF(" d:%s", buf);
  }
  }
  PRINTF("\r\n");
}

#endif /* NDEBUG */

void 
coap_log_impl(coap_log_t level, const char *format, ...) {
  char timebuf[32];
  coap_tick_t now;
  va_list ap;
//printf("coap_log_impl: [%d %d] \n",maxlog,level,LOG_DEBUG);
  if (maxlog < level)
    return;
  
  coap_ticks(&now);
  if (print_timestamp(timebuf,sizeof(timebuf), now))
    PRINTF("%s ", timebuf);

  if (level <= LOG_DEBUG)
    PRINTF("%s ", loglevels[level]);
//  PRINTF(format);

PRINTF("\n");
//  va_start(ap, format);
//  PRINTF(format, ap);
//  va_end(ap);
}
