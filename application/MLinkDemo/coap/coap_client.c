#include "coap_client.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include "config.h"
#include "resource.h"
#include "coap_crypto.h"
#include "ml_coap.h"
#include "ml_coap.h"



extern	coap_context_t	*g_svrctx;
static unsigned char _token_data[8];
str the_token = { 0, _token_data };
static str payload = { 0, NULL }; /* optional payload to send */
static int flags;

coap_block_t block = { .num = 0, .m = 0, .szx = 6 };

/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static str proxy = { 0, NULL };
static unsigned short proxy_port = COAP_DEFAULT_PORT;

/* reading is done when this flag is set */
static int ready = 0;

#define FLAGS_BLOCK 0//0x01

static coap_list_t *optlist = NULL;

static coap_address_t dstAddr;

extern int  check_segment(const unsigned char *s, size_t length);
extern void decode_segment(const unsigned char *seg, size_t length, unsigned char *buf);


static int  resolve_address(const str *server, struct sockaddr *dst) {
  
  struct addrinfo *res, *ainfo;
  struct addrinfo hints;
  static char addrstr[256];
  int error, len=-1;

  memset(addrstr, 0, sizeof(addrstr));
  if (server->length)
    memcpy(addrstr, server->s, server->length);
  else
    memcpy(addrstr, "localhost", 9);

  memset ((char *)&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;

  error = getaddrinfo(addrstr, "", &hints, &res);

  if (error != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    return error;
  }

  for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
    switch (ainfo->ai_family) {
    case AF_INET6:
    case AF_INET:
      len = ainfo->ai_addrlen;
      memcpy(dst, ainfo->ai_addr, len);
      goto finish;
    default:
      ;
    }
  }

 finish:
  freeaddrinfo(res);
  return len;
}

static int _order_opts(void *a, void *b) {
  if (!a || !b)
    return a < b ? -1 : 1;

  if (COAP_OPTION_KEY(*(coap_option *)a) < COAP_OPTION_KEY(*(coap_option *)b))
    return -1;

  return COAP_OPTION_KEY(*(coap_option *)a) == COAP_OPTION_KEY(*(coap_option *)b);
}


static coap_list_t * new_option_node(unsigned short key, unsigned int length, unsigned char *data) {
  coap_option *option;
  coap_list_t *node;

  option = coap_malloc(sizeof(coap_option) + length);
  if ( !option )
    goto error;

  COAP_OPTION_KEY(*option) = key;
  COAP_OPTION_LENGTH(*option) = length;
  memcpy(COAP_OPTION_DATA(*option), data, length);

  /* we can pass NULL here as delete function since option is released automatically  */
  node = coap_new_listnode(option, NULL);

  if ( node )
    return node;

 error:
  perror("new_option_node: malloc");
  coap_free( option );
  return NULL;
}

static int _get_payload_from_text(char *text, str *buf) {
  int len;
  unsigned char *textData;
  int textlen;
#if IS_USE_MLCOAP_ENCRYPT
	textData = mlcoap_data_encrypt(ML_COAP_ENCRYPT_TYPE,text);
#else
	textData = text;
#endif 
  textlen = strlen(textData);
  len = check_segment((unsigned char *)textData,textlen );
  if (len < 0)
    return 0;

  buf->s = (unsigned char *)coap_malloc(len+1);
  if (!buf->s)
    return 0;

memset(buf->s,0,len+1);
  buf->length = len;
  decode_segment((unsigned char *)textData, textlen, buf->s);

  return 1;
}

static int _get_payload_from_file(char *filename, str *buf) {
  FILE *inputfile = NULL;
  ssize_t len;
  int result = 1;
  struct stat statbuf;

  if (!filename || !buf)
    return 0;

  if (filename[0] == '-' && !filename[1]) { /* read from stdin */
    buf->length = 20000;
    buf->s = (unsigned char *)coap_malloc(buf->length);
    if (!buf->s)
      return 0;

    inputfile = stdin;
  } else {
    /* read from specified input file */
    if (stat(filename, &statbuf) < 0) {
      perror("cmdline_input_from_file: stat");
      return 0;
    }

    buf->length = statbuf.st_size;
    buf->s = (unsigned char *)coap_malloc(buf->length);
    if (!buf->s)
      return 0;

    inputfile = fopen(filename, "r");
    if ( !inputfile ) {
      perror("cmdline_input_from_file: fopen");
      coap_free(buf->s);
      return 0;
    }
  }

  len = fread(buf->s, 1, buf->length, inputfile);

  if (len < buf->length) {
    if (ferror(inputfile) != 0) {
      perror("cmdline_input_from_file: fread");
      coap_free(buf->s);
      buf->length = 0;
      buf->s = NULL;
      result = 0;
    } else {
      buf->length = len;
    }
  }

  if (inputfile != stdin)
    fclose(inputfile);

  return result;
}

static void _get_uri(char *arg) {
  unsigned char portbuf[2];
#define BUFSIZE 40
  unsigned char _buf[BUFSIZE];
  unsigned char *buf = _buf;
  size_t buflen;
  int res;

  if (proxy.length) {		/* create Proxy-Uri from argument */
    size_t len = strlen(arg);
    while (len > 270) {
      coap_insert(&optlist, 
		  new_option_node(COAP_OPTION_PROXY_URI,
				  270, (unsigned char *)arg),
		  _order_opts);
      len -= 270;
      arg += 270;
    }

    coap_insert(&optlist, 
		new_option_node(COAP_OPTION_PROXY_URI,
				len, (unsigned char *)arg),
		_order_opts);
  } else {			/* split arg into Uri-* options */

    coap_split_uri((unsigned char *)arg, strlen(arg), &uri );
	
    if (uri.port != COAP_DEFAULT_PORT) {
      coap_insert( &optlist, 
		   new_option_node(COAP_OPTION_URI_PORT,
				   coap_encode_var_bytes(portbuf, uri.port),
				 portbuf),
		   _order_opts);    
    }

    if (uri.path.length) {
      buflen = BUFSIZE;
      res = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);

      while (res--) {
	coap_insert(&optlist, new_option_node(COAP_OPTION_URI_PATH,
					      COAP_OPT_LENGTH(buf),
					      COAP_OPT_VALUE(buf)),
		    _order_opts);

	buf += COAP_OPT_SIZE(buf);      
      }
    }

    if (uri.query.length) {
      buflen = BUFSIZE;
      buf = _buf;
      res = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);

      while (res--) {
	coap_insert(&optlist, new_option_node(COAP_OPTION_URI_QUERY,
					      COAP_OPT_LENGTH(buf),
					      COAP_OPT_VALUE(buf)),
		    _order_opts);

	buf += COAP_OPT_SIZE(buf);      
      }
    }
  }
}

static int  _coap_get_addr_from_uri(char *coap_uri)
{
	static str server;
	unsigned short port = COAP_DEFAULT_PORT;
	int res;
	
	static char addr[INET6_ADDRSTRLEN];
	void *addrptr = NULL;
	char port_str[NI_MAXSERV] = "0";

	_get_uri(coap_uri);

	
	if (proxy.length) {
	  server = proxy;
	  port = proxy_port;
	} else {
	  server = uri.host;
	  port = uri.port;
	}
	
	/* resolve destination address where server should be sent */
	res = resolve_address(&server, &dstAddr.addr.sa);
	
	if (res < 0) {
	  fprintf(stderr, "failed to resolve address\n");
	  return res;
	}
	
	dstAddr.size = res;
	dstAddr.addr.sin.sin_port = htons(port);
	
	/* add Uri-Host if server address differs from uri.host */
	
	switch (dstAddr.addr.sa.sa_family) {
	case AF_INET: 
	  addrptr = &dstAddr.addr.sin.sin_addr;
	  break;
	case AF_INET6:
	  addrptr = &dstAddr.addr.sin6.sin6_addr;
	  break;
	default:
	  ;
	}	
	
}
	
static coap_pdu_t *_coap_new_request(coap_context_t *ctx, MLCOAP_METHOD_E m,int msgtype, coap_list_t *options ) {
  coap_pdu_t *pdu;
  coap_list_t *opt;
  char buf[2];

  if ( ! ( pdu = coap_new_pdu() ) )
    return NULL;

  pdu->hdr->type = msgtype;
  pdu->hdr->id = coap_new_message_id(ctx);
  pdu->hdr->code = m;


  pdu->hdr->token_length = the_token.length;
  if ( !coap_add_token(pdu, the_token.length, the_token.s)) {
    debug("cannot add token to request\n");
  }

  coap_show_pdu(pdu);


  for (opt = options; opt; opt = opt->next) {
    coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *)opt->data),
		    COAP_OPTION_LENGTH(*(coap_option *)opt->data),
		    COAP_OPTION_DATA(*(coap_option *)opt->data));
  }

#if IS_USE_MLCOAP_Authorized

   unsigned char authorized[8]={0};
   int authorlen = mlcoap_get_authorized(pdu->hdr->id,the_token.s,payload.s,authorized);   
   coap_add_option(pdu,ML_COAP_OPTION_Authorized,(unsigned int)authorlen,authorized);
#endif 

#if IS_USE_MLCOAP_ENCRYPT
	
	coap_add_option(pdu,ML_COAP_OPTION_Encrypt,1,ML_COAP_ENCRYPT_TYPE);
#endif 

  if (payload.length) {
    if ((flags & FLAGS_BLOCK) == 0){
    	debug("payload len[%d] %d  \n",payload.length,pdu->length);
      coap_add_data(pdu, payload.length, payload.s);
    }
    else
      coap_add_block(pdu, payload.length, payload.s, block.num, block.szx);
  }

  return pdu;
}


void _message_handler(struct coap_context_t  *ctx, 
		const coap_address_t *remote, 
		coap_pdu_t *sent,
		coap_pdu_t *received,
		const coap_tid_t id) {

  coap_pdu_t *pdu = NULL;
  coap_opt_t *block_opt;
  coap_opt_iterator_t opt_iter;
  unsigned char buf[4];
  coap_list_t *option;
  size_t len;
  unsigned char *databuf;
  coap_tid_t tid;
#ifndef NDEBUG
  if (LOG_DEBUG <= coap_get_log_level()) {
    debug("** process incoming %d.%02d response:\n",
	  (received->hdr->code >> 5), received->hdr->code & 0x1F);
    coap_show_pdu(received);
  }
#endif

}

void mlcoap_set_option(char *arg) {
  unsigned int num = 0;

  while (*arg && *arg != ',') {
    num = num * 10 + (*arg - '0');
    ++arg;
  }
  if (*arg == ',')
    ++arg;

  coap_insert( &optlist, new_option_node(num,
					 strlen(arg),
					 (unsigned char *)arg), _order_opts);
}

inline void mlcoap_set_token(char *arg) {
	strncpy((char *)the_token.s, arg, min(sizeof(_token_data), strlen(arg)));
	the_token.length = strlen(arg);
}

inline str mlcoap_get_token(void)
{
	return the_token;
}

void mlcoap_client_init(void)
{
}
int mlcoap_send_data(char * coapuri,MLCOAP_METHOD_E m,unsigned int msgtype ,unsigned char * data,unsigned int datalen)
{
	coap_context_t *ctx;
	void *addrptr = NULL;
	char addr[INET6_ADDRSTRLEN];
	char buf[2];
	coap_pdu_t	*pdu;
	
	coap_tid_t tid = COAP_INVALID_TID;
	
	int ret =0;
	ctx = g_svrctx;


	 if (!ctx) {
		coap_log(LOG_EMERG, "cannot create context\n");
		return -1;
	  }
	
	 coap_register_option(ctx, COAP_OPTION_BLOCK2);
	 coap_register_response_handler(ctx, _message_handler);


	ret = _coap_get_addr_from_uri(coapuri);

	_get_payload_from_text(data,&payload);
	  /* join multicast group if requested at command line */
//	 if (group)
//		join(ctx, group);
	
	 /* construct CoAP message */
	
	if (!proxy.length && addrptr
	  && (inet_ntop(dstAddr.addr.sa.sa_family, addrptr, addr, sizeof(addr)) != 0)
	  && (strlen(addr) != uri.host.length 
	  || memcmp(addr, uri.host.s, uri.host.length) != 0)) {
	  /* add Uri-Host */
	
		coap_insert(&optlist, new_option_node(COAP_OPTION_URI_HOST,
			uri.host.length, uri.host.s),
			_order_opts);
	 }

	
	 
	/* set block option if requested at commandline */
  if (flags & FLAGS_BLOCK)
  	set_blocksize();
	
  if (! (pdu = _coap_new_request(ctx, m,msgtype, optlist)))
		return -1;


//coap_add_option(pdu, COAP_OPTION_CONTENT_FORMAT,
//	 coap_encode_var_bytes(buf,COAP_MEDIATYPE_APPLICATION_JSON ), buf);
  
#undef NDEBUG	
#ifndef NDEBUG
	  if (LOG_DEBUG <= coap_get_log_level()) {
		debug("sending CoAP request:\n");
		coap_show_pdu(pdu);
	  }
#endif
	
	
	if (pdu->hdr->type == COAP_MESSAGE_CON)
		tid = coap_send_confirmed(ctx, &dstAddr, pdu);
	  else 
		tid = coap_send(ctx, &dstAddr, pdu);
	
	if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
		coap_delete_pdu(pdu);

	return ret;
}

int mlcoap_send_data_con(char * coapuri,MLCOAP_METHOD_E m ,unsigned char * data,unsigned int datalen)
{
	return mlcoap_send_data(coapuri,m,COAP_MESSAGE_CON,data,datalen);
}

int mlcoap_send_data_non(char * coapuri,MLCOAP_METHOD_E m ,unsigned char * data,unsigned int datalen)
{
	return mlcoap_send_data(coapuri,m,COAP_MESSAGE_NON,data,datalen);
}

