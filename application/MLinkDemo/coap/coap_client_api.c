/* mlcoap_client_api.c -- ml mlcoap_client_api header file for ML CoAP client api interfaces 
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */


#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#if 0
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif


#include "config.h"
#include "resource.h"


#include "debug_coap.h"
#include "ml_coap.h"
#include "coap.h"
#include "coap_client_api.h"
#include "coap_crypto.h"

#include "../MLinkAppDef.h"

#define MLCOAP_CLIENT_NUM_MAX	4

static unsigned char _token_data[8];
static unsigned char _uriclient[128];

static coap_context_t *g_context_client =NULL;
typedef struct{
	coap_tid_t tid ;
	coap_context_t *ctx;
	unsigned char msgtype;
	unsigned char method;
	coap_address_t dstAddr;
	coap_uri_t uri;
	str payload;
	str proxy;
	coap_list_t *optlist;
	str the_token;
	coap_block_t block;
	int ready;
	unsigned char isRevDeal;
	int wait_seconds;
	coap_tick_t max_wait;
	unsigned int obs_seconds;
	coap_tick_t obs_wait;
	int clear_obs;
	char *group;
	int flags;
	unsigned int timeCount;
	mlcoap_client_ack_handler ackhandle;
	mlcoap_client_subscribe_handler subs_handle;
}mlcoap_client_api_t; 


mlcoap_client_api_t *g_mlcoap_client[MLCOAP_CLIENT_NUM_MAX]=
        {NULL,NULL,NULL,NULL};


int flags = 0;

str the_token = { 0, _token_data };

#define FLAGS_BLOCK 0x01

static coap_list_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static str proxy = { 0, NULL };
static unsigned short proxy_port = COAP_DEFAULT_PORT;

/* reading is done when this flag is set */
static int ready = 0;

static str output_file = { 0, NULL }; /* output file name */
static FILE *file = NULL;	/* output file stream */

static str payload = { 0, NULL }; /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

typedef unsigned char method_t;
method_t method = 1;		/* the method we are using in our requests */

coap_block_t block = { .num = 0, .m = 0, .szx = 6 };

unsigned int wait_seconds =  90;	/* default timeout in seconds */
coap_tick_t max_wait;		/* global timeout (changed by set_timeout()) */

unsigned int obs_seconds = 30;	/* default observe time */
coap_tick_t obs_wait = 0;	/* timeout for current subscription */

#define min(a,b) ((a) < (b) ? (a) : (b))

static int st_error_coap_send =0;// add by 2018.3.3

inline unsigned char * _getClientUri(unsigned char *hostname,unsigned char *uri)
{
	sprintf(_uriclient,"coap://%s/%s",hostname,uri);
//	debug("uri: %s %s \n",_uriclient,hostname);
	return _uriclient;
}

static int _getClientIndexFromTid(coap_tid_t id)
{

	int index = -1;//COAP_INVALID_TID;
	int i;
	for (i=0;i<MLCOAP_CLIENT_NUM_MAX;i++){
		if (g_mlcoap_client[i])
		if ( g_mlcoap_client[i]->tid ==id)
		{
		  index =i;
		  break;	
		}
	}
	return index;
}

static inline void
set_timeout(coap_tick_t *timer, const unsigned int seconds) {
  coap_ticks(timer);
  *timer += seconds * COAP_TICKS_PER_SECOND;
}

int
append_to_output(const unsigned char *data, size_t len) {
  size_t written;

  if (!file) {
    if (!output_file.s || (output_file.length && output_file.s[0] == '-')) {
    //  file = stdout;
      printf("[test] rev output data[%d] %s \n",len,data);
    }
    else {
      if (!(file = fopen((char *)output_file.s, "w"))) {
	perror("fopen");
	return -1;
      }
    }
  }
 if (file){
  do {
    written = fwrite(data, 1, len, file);
    len -= written;
    data += written;
  } while ( written && len );
  fflush(file);
 }
  return 0;
}

void
close_output() {
  if (file) {

    /* add a newline before closing in case were writing to stdout */
    if (!output_file.s || (output_file.length && output_file.s[0] == '-')) 
      fwrite("\n", 1, 1, file);

    fflush(file);
    fclose(file);
  }
  
}

coap_pdu_t *
new_ack( coap_context_t  *ctx, coap_queue_t *node ) {
  coap_pdu_t *pdu = coap_new_pdu();

  if (pdu) {
    pdu->hdr->type = COAP_MESSAGE_ACK;
    pdu->hdr->code = 0;
    pdu->hdr->id = node->pdu->hdr->id;
  }

  return pdu;
}

coap_pdu_t *
new_response( coap_context_t  *ctx, coap_queue_t *node, unsigned int code ) {
  coap_pdu_t *pdu = new_ack(ctx, node);

  if (pdu)
    pdu->hdr->code = code;

  return pdu;
}


static coap_pdu_t * _coap_new_request(mlcoap_client_api_t *clientapi){
  coap_pdu_t *pdu;
  coap_list_t *opt;
	
  if ( clientapi==NULL || ! ( pdu = coap_new_pdu() ) )
    return NULL;
  pdu->hdr->type = clientapi->msgtype;
  pdu->hdr->id = coap_new_message_id(clientapi->ctx);
  pdu->hdr->code = clientapi->method;


  pdu->hdr->token_length = clientapi->the_token.length;
  if ( !coap_add_token(pdu, clientapi->the_token.length, clientapi->the_token.s)) {
    debug("cannot add token to request\n");
  }
//  coap_show_pdu(pdu);

  for (opt = clientapi->optlist; opt; opt = opt->next) {
//    coap_list_t *optTemp = NULL;
    coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *)opt->data),
		    COAP_OPTION_LENGTH(*(coap_option *)opt->data),
		    COAP_OPTION_DATA(*(coap_option *)opt->data));	
//    optTemp = opt;
//    opt = opt->next;
//    coap_delete(optTemp);
	  }

  
#if IS_USE_MLCOAP_Authorized
  
	 coap_register_option(clientapi->ctx, ML_COAP_OPTION_Authorized);
	 unsigned char authorized[8]={0};
	 int authorlen = mlcoap_get_authorized(pdu->hdr->id,clientapi->the_token.s,clientapi->payload.s,authorized);	 
	 coap_add_option(pdu,ML_COAP_OPTION_Authorized,(unsigned int)authorlen,authorized);
#endif 
  
#if IS_USE_MLCOAP_ENCRYPT	  
	  coap_register_option(clientapi->ctx, ML_COAP_OPTION_Encrypt);
	  coap_add_option(pdu,ML_COAP_OPTION_Encrypt,1,ML_COAP_ENCRYPT_TYPE);
#endif 



  if (clientapi->payload.length) {
    if ((clientapi->flags & FLAGS_BLOCK) == 0){
//    	debug("clientapi len[%d] %d  \n",clientapi->payload.length,pdu->length);
    	coap_add_data(pdu, clientapi->payload.length, clientapi->payload.s);
    }
    else
      coap_add_block(pdu, clientapi->payload.length,clientapi->payload.s, 
      		clientapi->block.num, clientapi->block.szx);
  }

  return pdu;
}

//coap_tid_t clear_obs(coap_context_t *ctx, const coap_address_t *remote) {
coap_tid_t _clear_obs(mlcoap_client_api_t *clientapi)
{
  coap_pdu_t *pdu;
  coap_tid_t tid = COAP_INVALID_TID;
  if (clientapi==NULL)
  	return tid;
  coap_context_t *ctx = clientapi->ctx;
  coap_address_t *remote = &clientapi->dstAddr;
  
  /* create bare PDU w/o any option  */
  info("response code 7.31 is %d\n", COAP_RESPONSE_CODE(731));
  
  pdu = coap_pdu_init(clientapi->msgtype, COAP_RESPONSE_CODE(731), 
		      coap_new_message_id(ctx),
		      sizeof(coap_hdr_t) + clientapi->the_token.length);

  if (!pdu) {
    return tid;
  }

  if (!coap_add_token(pdu, clientapi->the_token.length, clientapi->the_token.s)) {
	  info( "cannot add token");
    coap_delete_pdu(pdu);
    return tid;
  }
//  coap_show_pdu(pdu);

  if (pdu->hdr->type == COAP_MESSAGE_CON)
    tid = coap_send_confirmed(ctx, remote, pdu);
  else 
    tid = coap_send(ctx, remote, pdu);
    
  if (tid == COAP_INVALID_TID) {
    debug("clear_obs: error sending new request");
    coap_delete_pdu(pdu);
  } else if (pdu->hdr->type != COAP_MESSAGE_CON)
    coap_delete_pdu(pdu);

  return tid;
}

int 
resolve_address(const str *server, struct sockaddr *dst) {
  
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

  error = getaddrinfo(addrstr, addrstr, &hints, &res);

  if (error != 0) {
    warn("getaddrinfo: [error: %d ] %s \n", error,addrstr);
    return error;
  }

  for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
    switch (ainfo->ai_family) {
//    case AF_INET6:
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

static inline coap_opt_t *
get_block(coap_pdu_t *pdu, coap_opt_iterator_t *opt_iter) {
  coap_opt_filter_t f;
  
  assert(pdu);

  memset(f, 0, sizeof(coap_opt_filter_t));
  coap_option_setb(f, COAP_OPTION_BLOCK1);
  coap_option_setb(f, COAP_OPTION_BLOCK2);

  coap_option_iterator_init(pdu, opt_iter, f);
  return coap_option_next(opt_iter);
}

#define HANDLE_BLOCK1(Pdu)						\
  ((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) &&	\
   ((flags & FLAGS_BLOCK) == 0) &&					\
   ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) ||			\
    (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

static inline int
_check_token(coap_pdu_t *received,str token) 
{
  return received->hdr->token_length == token.length &&
    memcmp(received->hdr->token, token.s, token.length) == 0;
}

static void
message_handler(struct coap_context_t  *ctx, 
		const coap_address_t *remote, 
		coap_pdu_t *sent,
		coap_pdu_t *received,
		const coap_tid_t id) {

  coap_pdu_t *pdu = NULL;
  coap_opt_t *block_opt;
  coap_opt_iterator_t opt_iter;
  unsigned char buf[12];
  coap_list_t *option;
  size_t len;
  unsigned char *databuf;
  coap_tid_t tid;
  int clientindex =-1;
  mlcoap_client_api_t *clientapi =NULL;
  coap_opt_t  *optSubs =NULL;// hasoptionSubs =0;

#ifndef NDEBUG
  if (LOG_DEBUG <= coap_get_log_level()) {
    debug("** process incoming %d.%02d response: host[%s]  \n",
	  (received->hdr->code >> 5), received->hdr->code & 0x1F,inet_ntoa(remote->addr.sin.sin_addr));
    coap_show_pdu(received);
  }

  // recive the coap data and distribute here .
#endif

//  warn("** process incoming %d.%02d response: host[%s]  \n",
//     (received->hdr->code >> 5), received->hdr->code & 0x1F,inet_ntoa(remote->addr.sin.sin_addr));

//printf("** process incoming %d.%02d response: [%d] id[%d,%d] token[%d %s]\n",
//  (received->hdr->code >> 5), received->hdr->code & 0x1F,sent,id,received->hdr->id,
//  received->hdr->token_length,received->hdr->token);

 optSubs= coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter) ;
 clientindex = _getClientIndexFromTid(received->hdr->id);
 
 if (clientindex <0){
	//if (optSubs && !sent)
	if (!sent && optSubs ){
		memset(buf,0,12);
		memcpy(buf,received->hdr->token,received->hdr->token_length);
		clientindex =atoi(buf);
		if (clientindex>=0 && clientindex<MLCOAP_CLIENT_NUM_MAX){
			clientapi =g_mlcoap_client[clientindex];
			if (coap_get_data(received, &len, &databuf)){		
			if (clientapi->subs_handle)
				clientapi->subs_handle(databuf,len);
			}
		}
	}
	//return;
 }else 
	clientapi =g_mlcoap_client[clientindex];

  
#ifndef NDEBUG
  if (LOG_DEBUG <= coap_get_log_level()) {
    debug("** process incoming %d.%02d response:\n",
	  (received->hdr->code >> 5), received->hdr->code & 0x1F);
    coap_show_pdu(received);
  }
#endif

	if (clientapi==NULL){
	    char addr[48];
	    coap_print_addr(remote,addr,48);
		warn(" recevice error remote[%s] len[%d] \n",addr,received->length);
		return ;
	}

  /* check if this is a response to our original request */
  if (!_check_token(received,clientapi->the_token)) {
    /* drop if this was just some message, or send RST in case of notification */
    if (!sent && (received->hdr->type == COAP_MESSAGE_CON || 
		  received->hdr->type == COAP_MESSAGE_NON))
	  info("coap_send_rst\n");
      coap_send_rst(ctx, remote, received);	  
    return;
  }

  if (received->hdr->type == COAP_MESSAGE_RST) {
    info("got RST\n");
    return;
  }
  
  /* output the received data, if any */
//  optSubs =  coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter);
  if (received->hdr->code == COAP_RESPONSE_CODE(205)) {

    /* set obs timer if we have successfully subscribed a resource */
    if (sent && optSubs) {		
      debug("observation relationship established, set timeout to %d\n", clientapi->obs_seconds);
      set_timeout(&clientapi->obs_wait, clientapi->obs_seconds);
    }
    
    /* Got some data, check if block option is set. Behavior is undefined if
     * both, Block1 and Block2 are present. */
    block_opt = get_block(received, &opt_iter);

    if (!block_opt) {
      /* There is no block option set, just read the data and we are done. */
      if (coap_get_data(received, &len, &databuf)){
		//append_to_output(databuf, len);
//debug("[client:%d ] sent[%d] len %d \n",clientapi->ackhandle,sent,len);

//		if (clientapi->ackhandle && sent)
		if (clientapi->ackhandle)
		{
			ml_coap_revMsg_t *msg = coap_malloc(sizeof(ml_coap_revMsg_t));
			msg->echoValue = MLCOAP_ACK_OK;
			msg->msgData = databuf;
			msg->msgLen = len;
			msg->srcAddr =remote->addr.sin.sin_addr.s_addr;
			clientapi->ackhandle(clientapi->tid,msg);
			coap_free(msg);
//			clientapi->ackhandle(clientapi->tid,MLCOAP_ACK_OK,databuf,len);
		}
		if (!optSubs)
			clientapi->isRevDeal =1;
	  }
    } else {
      unsigned short blktype = opt_iter.type;

      /* TODO: check if we are looking at the correct block number */
      if (coap_get_data(received, &len, &databuf))
		append_to_output(databuf, len);

      if (COAP_OPT_BLOCK_MORE(block_opt)) {
	/* more bit is set */
	debug("found the M bit, block size is %u, block nr. %u\n",
	      COAP_OPT_BLOCK_SZX(block_opt), coap_opt_block_num(block_opt));

	/* create pdu with request for next block */
	//pdu = _coap_new_request(ctx, method, NULL); /* first, create bare PDU w/o any option  */
	pdu = _coap_new_request(clientapi);
	
	if ( pdu ) {
	  /* add URI components from optlist */
	  for (option = clientapi->optlist; option; option = option->next ) {
	    switch (COAP_OPTION_KEY(*(coap_option *)option->data)) {
	    case COAP_OPTION_URI_HOST :
	    case COAP_OPTION_URI_PORT :
	    case COAP_OPTION_URI_PATH :
	    case COAP_OPTION_URI_QUERY :
	      coap_add_option ( pdu, COAP_OPTION_KEY(*(coap_option *)option->data),
				COAP_OPTION_LENGTH(*(coap_option *)option->data),
				COAP_OPTION_DATA(*(coap_option *)option->data) );
	      break;
	    default:
	      ;			/* skip other options */
	    }
	  }

	  /* finally add updated block option from response, clear M bit */
	  /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
	  debug("query block %d\n", (coap_opt_block_num(block_opt) + 1));
	  coap_add_option(pdu, blktype, coap_encode_var_bytes(buf, 
	      ((coap_opt_block_num(block_opt) + 1) << 4) | 
              COAP_OPT_BLOCK_SZX(block_opt)), buf);

	  if (received->hdr->type == COAP_MESSAGE_CON)
	    tid = coap_send_confirmed(ctx, remote, pdu);
	  else 
	    tid = coap_send(ctx, remote, pdu);

	  if (tid == COAP_INVALID_TID) {
	    debug("message_handler: error sending new request");
            coap_delete_pdu(pdu);
	  } else {
	    set_timeout(&clientapi->max_wait, clientapi->wait_seconds);
            if (received->hdr->type != COAP_MESSAGE_CON)
              coap_delete_pdu(pdu);
          }
	  return;
	}
      }
    }
  } else {			/* no 2.05 */
    /* check if an error was signaled and output payload if so */
    if (COAP_RESPONSE_CLASS(received->hdr->code) >= 4) {
	  if (clientapi->ackhandle){
			ml_coap_revMsg_t *msg = coap_malloc(sizeof(ml_coap_revMsg_t));
			msg->echoValue = received->hdr->code;
			msg->msgData = NULL;
			msg->msgLen = 0;
			msg->srcAddr =remote->addr.sin.sin_addr.s_addr;
			clientapi->ackhandle(clientapi->tid,msg);
			coap_free(msg);
//		  clientapi->ackhandle(clientapi->tid,received->hdr->code,NULL,0);
	  }
    }
  }

  /* finally send new request, if needed */
  if (pdu && coap_send(ctx, remote, pdu) == COAP_INVALID_TID) {
    debug("message_handler: error sending response");
  }
  coap_delete_pdu(pdu);

  /* our job is done, we can exit at any time */
  clientapi->ready = coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter) == NULL;

debug("[test]clientapi->ready[%d] [%d]\n ",clientapi->ready,clientindex);
}

int
join( coap_context_t *ctx, char *group_name ){
//  struct ipv6_mreq mreq;
  struct addrinfo   *reslocal = NULL, *resmulti = NULL, hints, *ainfo;
  int result = -1;

  /* we have to resolve the link-local interface to get the interface id */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;//AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;

  result = getaddrinfo("::", NULL, &hints, &reslocal);
  if ( result < 0 ) {
//    fprintf(stderr, "join: cannot resolve link-local interface: %s\n",
//	    gai_strerror(result));
    goto finish;
  }
#ifdef IS_USE_AF_INET6
  /* get the first suitable interface identifier */
  for (ainfo = reslocal; ainfo != NULL; ainfo = ainfo->ai_next) {
    if ( ainfo->ai_family == AF_INET6 ) {
      mreq.ipv6mr_interface =
	      ((struct sockaddr_in6 *)ainfo->ai_addr)->sin6_scope_id;
      break;
    }
  }
#endif
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //AF_INET6
  hints.ai_socktype = SOCK_DGRAM;

  /* resolve the multicast group address */
  result = getaddrinfo(group_name, NULL, &hints, &resmulti);

  if ( result < 0 ) {
//    fprintf(stderr, "join: cannot resolve multicast address: %s\n",
//	    gai_strerror(result));
    goto finish;
  }
#ifdef   IS_USE_AF_INET6
  for (ainfo = resmulti; ainfo != NULL; ainfo = ainfo->ai_next) {
    if ( ainfo->ai_family == AF_INET6 ) {
      mreq.ipv6mr_multiaddr =
	((struct sockaddr_in6 *)ainfo->ai_addr)->sin6_addr;
      break;
    }
  }
  result = setsockopt( ctx->sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		       (char *)&mreq, sizeof(mreq) );
  if ( result < 0 )
    perror("join: setsockopt");
#endif

 finish:
  freeaddrinfo(resmulti);
  freeaddrinfo(reslocal);

  return result;
}

int
order_opts(void *a, void *b) {
  if (!a || !b)
    return a < b ? -1 : 1;

  if (COAP_OPTION_KEY(*(coap_option *)a) < COAP_OPTION_KEY(*(coap_option *)b))
    return -1;

  return COAP_OPTION_KEY(*(coap_option *)a) == COAP_OPTION_KEY(*(coap_option *)b);
}


coap_list_t *
new_option_node(unsigned short key, unsigned int length, unsigned char *data) {
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

typedef struct { 
  unsigned char code;
  char *media_type;
} content_type_t;

void
cmdline_content_type(char *arg, unsigned short key) {
  static content_type_t content_types[] = {
    {  0, "plain" },
    {  0, "text/plain" },
    { 40, "link" },
    { 40, "link-format" },
    { 40, "application/link-format" },
    { 41, "xml" },
    { 42, "binary" },
    { 42, "octet-stream" },
    { 42, "application/octet-stream" },
    { 47, "exi" },
    { 47, "application/exi" },
    { 50, "json" },
    { 50, "application/json" },
    { 255, NULL }
  };
  coap_list_t *node;
  unsigned char i, value[10];
  int valcnt = 0;
  unsigned char buf[2];
  char *p, *q = arg;

  while (q && *q) {
    p = strchr(q, ',');

    if (isdigit(*q)) {
      if (p)
	*p = '\0';
      value[valcnt++] = atoi(q);
    } else {
      for (i=0; content_types[i].media_type &&
	     strncmp(q,content_types[i].media_type, p ? p-q : strlen(q)) != 0 ;
	   ++i)
	;
      
      if (content_types[i].media_type) {
	value[valcnt] = content_types[i].code;
	valcnt++;
      } else {
	warn("W: unknown content-type '%s'\n",arg);
      }
    }

    if (!p || key == COAP_OPTION_CONTENT_TYPE)
      break;
    
    q = p+1;
  }

  for (i = 0; i < valcnt; ++i) {
    node = new_option_node(key, coap_encode_var_bytes(buf, value[i]), buf);
    if (node)
      coap_insert( &optlist, node, order_opts );
  }
}

static void _parse_uri(char *arg,mlcoap_client_api_t *clientapi) {
  unsigned char portbuf[2];
#define BUFSIZE 64
  unsigned char _buf[BUFSIZE];
  unsigned char *buf = _buf;
  size_t buflen;
  int res;

  if (clientapi == NULL ||arg ==NULL)
  	return;
  if (clientapi->proxy.length) {		/* create Proxy-Uri from argument */
    size_t len = strlen(arg);
    while (len > 270) {
      coap_insert(&clientapi->optlist, 
		  new_option_node(COAP_OPTION_PROXY_URI,
				  270, (unsigned char *)arg),
		  order_opts);
      len -= 270;
      arg += 270;
    }

    coap_insert(&clientapi->optlist, 
		new_option_node(COAP_OPTION_PROXY_URI,
				len, (unsigned char *)arg),
		order_opts);
  } else {			/* split arg into Uri-* options */
  

	
    coap_split_uri((unsigned char *)arg, strlen(arg), &clientapi->uri );
	

    if (clientapi->uri.port != COAP_DEFAULT_PORT) {
      coap_insert( &clientapi->optlist, 
		   new_option_node(COAP_OPTION_URI_PORT,
				   coap_encode_var_bytes(portbuf, clientapi->uri.port),
				 portbuf),
		   order_opts);    
    }


    if (clientapi->uri.path.length) {
      buflen = BUFSIZE;
      res = coap_split_path(clientapi->uri.path.s, clientapi->uri.path.length, buf, &buflen);

      while (res--) {
	coap_insert(&clientapi->optlist, new_option_node(COAP_OPTION_URI_PATH,
					      COAP_OPT_LENGTH(buf),
					      COAP_OPT_VALUE(buf)),
		    order_opts);

	buf += COAP_OPT_SIZE(buf);      
      }
    }

    if (clientapi->uri.query.length) {
      buflen = BUFSIZE;
      buf = _buf;
      res = coap_split_query(clientapi->uri.query.s, clientapi->uri.query.length, buf, &buflen);

      while (res--) {
	coap_insert(&clientapi->optlist, new_option_node(COAP_OPTION_URI_QUERY,
					      COAP_OPT_LENGTH(buf),
					      COAP_OPT_VALUE(buf)),
		    order_opts);

	buf += COAP_OPT_SIZE(buf);      
      }
    }
  }
  
}

int
cmdline_blocksize(char *arg) {
  unsigned short size;

 again:
  size = 0;
  while(*arg && *arg != ',')
    size = size * 10 + (*arg++ - '0');
  
  if (*arg == ',') {
    arg++;
    block.num = size;
    goto again;
  }
  
  if (size)
    block.szx = (coap_fls(size >> 4) - 1) & 0x07;

  flags |= FLAGS_BLOCK;
  return 1;
}

/* Called after processing the options from the commandline to set 
 * Block1 or Block2 depending on method. */
void _set_blocksize(mlcoap_client_api_t *clientapi) {
  static unsigned char buf[4];	/* hack: temporarily take encoded bytes */
  unsigned short opt;
  if (clientapi==NULL)
  	return;
  if (clientapi->method != COAP_REQUEST_DELETE) {
    opt = clientapi->method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

    coap_insert(&clientapi->optlist, new_option_node(opt,
                coap_encode_var_bytes(buf, (clientapi->block.num << 4 | clientapi->block.szx)), buf),
		order_opts);
  }
}

void
_parse_subscribe(unsigned int arg,mlcoap_client_api_t *clientapi) {

  if (clientapi==NULL)
  	return;
  if (clientapi->max_wait <arg)
  	clientapi->max_wait =arg+MLCOAP_CLIENT_WAIT_MAX;
  
  clientapi->obs_seconds = arg;

  coap_insert(&clientapi->optlist, new_option_node(COAP_OPTION_SUBSCRIPTION, 0, NULL),
	      order_opts);
}

int
cmdline_proxy(char *arg) {
  char *proxy_port_str = strrchr((const char *)arg, ':'); /* explicit port ? */
  if (proxy_port_str) {
    char *ipv6_delimiter = strrchr((const char *)arg, ']');
    if (!ipv6_delimiter) {
      if (proxy_port_str == strchr((const char *)arg, ':')) {
        /* host:port format - host not in ipv6 hexadecimal string format */
        *proxy_port_str++ = '\0'; /* split */
        proxy_port = atoi(proxy_port_str);
      }
    } else {
      arg = strchr((const char *)arg, '[');
      if (!arg) return 0;
      arg++;
      *ipv6_delimiter = '\0'; /* split */
      if (ipv6_delimiter + 1 == proxy_port_str++) {
        /* [ipv6 address]:port */
        proxy_port = atoi(proxy_port_str);
      }
    }
  }

  proxy.length = strlen(arg);
  if ( (proxy.s = coap_malloc(proxy.length + 1)) == NULL) {
    proxy.length = 0;
    return 0;
  }

  memcpy(proxy.s, arg, proxy.length+1);
  return 1;
}

inline void _parse_token(char *arg,mlcoap_client_api_t *clientapi) {
  strncpy((char *)clientapi->the_token.s, arg, min(sizeof(_token_data), strlen(arg)));
  clientapi->the_token.length = strlen(arg);
}

void
cmdline_option(char *arg) {
  unsigned int num = 0;

  while (*arg && *arg != ',') {
    num = num * 10 + (*arg - '0');
    ++arg;
  }
  if (*arg == ',')
    ++arg;

  coap_insert( &optlist, new_option_node(num,
					 strlen(arg),
					 (unsigned char *)arg), order_opts);
}

extern int  check_segment(const unsigned char *s, size_t length);
extern void decode_segment(const unsigned char *seg, size_t length, unsigned char *buf);

int _parse_payload(char *text, str *buf) {
  int len;
  if (text==NULL){
	return 0;
  }
  
  unsigned char *textData;
  int textlen;
  
#if IS_USE_MLCOAP_ENCRYPT
	  textData = mlcoap_data_encrypt(ML_COAP_ENCRYPT_TYPE,text);
#else
	  textData = text;
#endif 
  textlen = strlen(textData);

  len = check_segment((unsigned char *)text, textlen);
//  debug("len:%d %d \n",textlen,len);
  if (len < 0)
    return 0;

  buf->s = (unsigned char *)coap_malloc(len+5);
  memset(buf->s,0,len+5);
  if (!buf->s)
    return 0;

  buf->length = len;
  decode_segment((unsigned char *)text, textlen, buf->s);
  return 1;
}

#if 0
int
cmdline_input_from_file(char *filename, str *buf) {
  FILE *inputfile = NULL;
  ssize_t len;
  int result = 1;
#if 0

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
#endif
  return result;
}

method_t
cmdline_method(char *arg) {
  static char *methods[] =
    { 0, "get", "post", "put", "delete", 0};
  unsigned char i;

  for (i=1; methods[i] && strcasecmp(arg,methods[i]) != 0 ; ++i)
    ;

  return i;	     /* note that we do not prevent illegal methods */
}
#endif

coap_context_t *
get_context(const char *node, const char *port) {
  coap_context_t *ctx = NULL;  
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
//  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;
  
  s = getaddrinfo(node, port, &hints, &result);

  if ( s != 0 ) {
	  warn( "getaddrinfo: port:%s \n",port);
    return NULL;
  } 

  /* iterate through results until success */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    coap_address_t addr;

    if (rp->ai_addrlen <= sizeof(addr.addr)) {
      coap_address_init(&addr);
      addr.size = rp->ai_addrlen;
      memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);

      ctx = coap_new_context(&addr);
      if (ctx) {
	/* TODO: output address:port for successful binding */
	goto finish;
      }
    }
  }
  
  warn("no context available for interface '%s'\n", node);

 finish:
  freeaddrinfo(result);
  return ctx;
}

static int _get_freeclient_index(void)
{
	int i;
	int index =-1;
	for (i=0;i<MLCOAP_CLIENT_NUM_MAX;i++){
		if (g_mlcoap_client[i] ==NULL)
		{
			index =i;
			break;
		}
	}
	return index;
}

static  int _new_mlcoap_client(void)
{
	int clientIndex =-1;
	mlcoap_client_api_t *clientapi = NULL;
	
	clientIndex = _get_freeclient_index();
	if (clientIndex <0){
	debug("====clientIndex is full==========");
	    return (-1);
	}
	if (g_mlcoap_client[clientIndex])
		coap_free(g_mlcoap_client[clientIndex]);
	
	g_mlcoap_client[clientIndex] =coap_malloc(sizeof(mlcoap_client_api_t));
	memset(g_mlcoap_client[clientIndex],0,sizeof(mlcoap_client_api_t));

	clientapi = g_mlcoap_client[clientIndex];
	clientapi->wait_seconds = MLCOAP_CLIENT_WAIT_MAX;
	clientapi->obs_seconds = MLCOAP_CLIENT_DEFAULT_OBSSECOND;
	clientapi->the_token.s = _token_data;

	_parse_token(ML_COAP_DEFAULT_TOKEN,clientapi);
	return clientIndex;
}


static int st_mlcoap_client_sendmsg(mlcoap_client_api_t *clientapi,unsigned char *struri,
	unsigned char *data)
{
	static str server;
	coap_context_t	*ctx = NULL;
	unsigned short port = COAP_DEFAULT_PORT;
	char port_str[32] = "0";
	void *addrptr = NULL;
	static char addr[64];

	int clientIndex =-1;
	int res;
	
	coap_pdu_t	*pdu;
	if (clientapi==NULL)
		return (-1);
    st_error_coap_send++;   // add by 2018.3.3

	_parse_uri(struri,clientapi);
	_parse_payload(data,&clientapi->payload);

	if (clientapi->proxy.length) {
	  server = clientapi->proxy;
	  port = COAP_DEFAULT_PORT;//clientapi->proxy_port;
	} else {
	  server = clientapi->uri.host;
	  port = clientapi->uri.port;
	}
	sprintf(port_str,"%d",port+clientIndex+10);
	res = resolve_address(&server, &clientapi->dstAddr.addr.sa);	
	if (res < 0) {
//	  fprintf(stderr, "failed to resolve address\n");
	  warn("failed to resolve address");
	  return (-1);
	}
//	debug("clientapi payload len is %d, addr : %x", clientapi->payload.length, clientapi->dstAddr.addr.sin.sin_addr);
	clientapi->dstAddr.size = res;
	clientapi->dstAddr.addr.sin.sin_port = htons(port);
//    debug("ctx now is null");
	/* add Uri-Host if server address differs from uri.host */
//	ctx = g_context_client;//note by 2018.3.20


	switch (clientapi->dstAddr.addr.sa.sa_family) {
	case AF_INET: 
	  addrptr = &clientapi->dstAddr.addr.sin.sin_addr;
	
	  /* create context for IPv4 */
	 // if (ctx==NULL)
	  	ctx = get_context("0.0.0.0", port_str);
	  break;
#if 0
	case AF_INET6:
	  addrptr = &clientapi->dstAddr.addr.sin6.sin6_addr;
	
	  /* create context for IPv6 */
	  if (ctx ==NULL){
	 	 ctx = get_context("::", port_str);
	  }
	  break;
#endif
	default:
	  ;
	}
	
	if (!ctx) {
	  warn( "cannot create context\n");
	  return -1;
	}

	
	clientapi->ctx = ctx;

	coap_register_option(ctx, COAP_OPTION_BLOCK2);
	coap_register_response_handler(ctx, message_handler);

	/* join multicast group if requested at command line */
	if (clientapi->group)
	  join(ctx, clientapi->group);
	
	/* construct CoAP message */
	
	if (!clientapi->proxy.length && addrptr
//		&& (inet_ntop(clientapi->dstAddr.addr.sa.sa_family, addrptr, addr, sizeof(addr)) != 0)
		&& (strlen(addr) != clientapi->uri.host.length 
		|| memcmp(addr, clientapi->uri.host.s, clientapi->uri.host.length) != 0)) {
		/* add Uri-Host */
	
	  coap_insert(&clientapi->optlist, new_option_node(COAP_OPTION_URI_HOST,
						clientapi->uri.host.length, clientapi->uri.host.s),
		  order_opts);
	}

	/* set block option if requested at commandline */
	if (clientapi->flags & FLAGS_BLOCK)
	  _set_blocksize(clientapi);

	if (! (pdu = _coap_new_request(clientapi)))
		return -1;
	
#ifndef NDEBUG
	  if (LOG_DEBUG <= coap_get_log_level()) {
		debug("sending CoAP request:\n");
		coap_show_pdu(pdu);
	  }
#endif
	if (pdu->hdr->type == COAP_MESSAGE_CON)
		clientapi->tid = coap_send_confirmed(ctx, &clientapi->dstAddr, pdu);
	else 
		clientapi->tid = coap_send(ctx, &clientapi->dstAddr, pdu);

	debug("clientapi tid is %d", clientapi->tid);
	if (pdu->hdr->type != COAP_MESSAGE_CON || clientapi->tid == COAP_INVALID_TID){
		coap_delete_pdu(pdu);
//        if (g_context_client != NULL)
//        {
//            free(g_context_client);
//            g_context_client = NULL;
//        }
//        if (g_context_client != NULL)
//		debug("test !COAP_MESSAGE_CON tid[%d] dstAddr ",clientapi->tid);
	}
	else{
	    debug("==== test COAP_MESSAGE_CON tid[%d] ===",clientapi->tid);
	}
	set_timeout(&clientapi->max_wait, clientapi->wait_seconds);

	clientapi->timeCount=0;
	clientapi->tid = pdu->hdr->id;

    st_error_coap_send=0;   // add by 2018.3.3
	return clientapi->tid;
}
static _mlcoap_client_free(mlcoap_client_api_t *clientapi)
{
	if (clientapi){
	    if (clientapi->optlist->data != NULL)
	    {
//	        debug("_mlcoap_client_free opt before");
	        coap_delete_list(clientapi->optlist);
	        clientapi->optlist = NULL;
//            debug("_mlcoap_client_free opt end");
	    }
//	    if (g_context_client != NULL)
//	    {
//	        free(g_context_client);
//	        g_context_client = NULL;
//	    }
	    if (clientapi->payload.s != NULL)
	    {
	        free(clientapi->payload.s);
	        clientapi->payload.s = NULL;
	    }
		coap_free(clientapi);
		clientapi = NULL;

	}
}


int mlcoap_client_init(void)
{
	coap_log_t log_level =LOG_DEBUG;// LOG_DEBUG; LOG_WARNING;

	wait_seconds = MLCOAP_CLIENT_WAIT_MAX;
	
	coap_set_log_level(log_level);
}


/**  锟斤拷询锟斤拷锟斤拷coap client  锟斤拷锟秸碉拷 coap revice 锟斤拷锟斤拷
  * 	锟矫猴拷锟斤拷锟斤拷要锟斤拷应锟矫诧拷锟节碉拷锟斤拷锟竭筹拷锟叫碉拷锟斤拷
  *  timeout default  50us
  * @return >=0   or <0 on error.
  */
int mlcoap_client_poll_context(void)
{
	fd_set readfds;
	struct timeval tv;
	int result;
	coap_tick_t now;
	coap_queue_t *nextpdu;
	coap_pdu_t	*pdu;

	coap_context_t	*ctx = NULL;
	mlcoap_client_api_t *clientapi =NULL;
	
	int i;
	int isExitCoapClient =0;
	int timeout;
	
	for (i=0;i<MLCOAP_CLIENT_NUM_MAX;i++){
		clientapi = g_mlcoap_client[i];
		if (clientapi == NULL)
			continue;
		ctx = clientapi->ctx;
		if (ctx == NULL)
			continue;
	//while ( !(clientapi->ready && coap_can_exit(ctx)) )
	isExitCoapClient = clientapi->ready && coap_can_exit(ctx);
	if (!isExitCoapClient)
	{
	  clientapi->timeCount++;
	  tv.tv_usec =1000*10;//50; //edit by 2018.3.20 太快会导致sendto时候还未成功发送，就被销毁
	  tv.tv_sec =0;
	  
	  FD_ZERO(&readfds);
	  FD_SET( ctx->sockfd, &readfds );
	
	  nextpdu = coap_peek_next( ctx );

	  coap_ticks(&now);
	  while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
	      coap_tid_t tid = coap_retransmit( ctx, coap_pop_next( ctx ));
		
		nextpdu = coap_peek_next( ctx );

		debug("==== [test] send coap_retransmit [%d] =====\n",tid);

	  }

	  if (nextpdu && nextpdu->t < min(clientapi->obs_wait ? clientapi->obs_wait : clientapi->max_wait,
	  	clientapi->max_wait) - now) { 
		/* set timeout if there is a pdu to send */
		timeout =nextpdu->t;
//		tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
//		tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
	  } else {
		/* check if obs_wait fires before max_wait */
		if (clientapi->obs_wait && clientapi->obs_wait < clientapi->max_wait) {
			timeout =clientapi->obs_wait - now; 
//	  tv.tv_usec = ((clientapi->obs_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
//	  tv.tv_sec = (clientapi->obs_wait - now) / COAP_TICKS_PER_SECOND;
		} else {
			timeout = clientapi->max_wait - now;
//	  tv.tv_usec = ((clientapi->max_wait - now) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
//	  tv.tv_sec = (clientapi->max_wait - now) / COAP_TICKS_PER_SECOND;
		}
	  }

//	  printf("[test]obs_wait sock[%d][%d,%d] [%d %d]  [%d %d],now[%d] tv[%d %d] \n",ctx->sockfd,i,clientapi->tid,
//		clientapi->timeCount,timeout,clientapi->max_wait,clientapi->obs_wait,now,tv.tv_sec,tv.tv_usec);
	
	  result = select(ctx->sockfd + 1, &readfds, 0, 0, &tv);
	  if ( result < 0 ) {	  /* error */
		perror("select");
		continue;
		//break;
	  } else if ( result > 0 ) {  /* read from socket */
		if ( FD_ISSET( ctx->sockfd, &readfds ) ) {
			coap_read( ctx );   /* read received data */
			coap_dispatch( ctx );   /* and dispatch PDUs from receivequeue */
		}
	  } else { /* timeout */
		coap_ticks(&now);
		if (clientapi->isRevDeal)
		{		
			if (clientapi->ready)
				isExitCoapClient =1;
		}else
		if (clientapi->max_wait <= now ) {
//		  info("timeout[%d] [%d %d] \n",i,clientapi->max_wait,now);
		  	if (clientapi->ackhandle){
				ml_coap_revMsg_t *msg = coap_malloc(sizeof(ml_coap_revMsg_t));
				msg->echoValue = MLCOAP_ACK_TIMEOUT;
				msg->msgData = NULL;
				msg->msgLen = 0;
//				if (ctx->recvqueue->remote)
//				msg->srcAddr =0;//ctx->recvqueue->remote->addr.sin.sin_addr;
				clientapi->ackhandle(clientapi->tid,msg);
				coap_free(msg);
//				clientapi->ackhandle(clientapi->tid,MLCOAP_ACK_TIMEOUT,NULL,0);
			}
	  		//break;
	  		//if (clientapi->ready)
				isExitCoapClient =1;
		} 
		if (clientapi->obs_wait && (clientapi->obs_wait <= now) || clientapi->clear_obs) {
	 	 debug("clear observation relationship\n");
	
		/* make sure that the obs timer does not fire again */
		_clear_obs(clientapi);

	  	clientapi->obs_wait = 0; 
	  	clientapi->obs_seconds = 0;
		isExitCoapClient =1;
		} 
	  }
	}
	//close_output();
	if (isExitCoapClient && st_error_coap_send==0){
//		debug("== client poll exit[%d ] ===\n",i);
        coap_free_context( ctx );
        ctx = NULL;
        _mlcoap_client_free(clientapi);
        g_mlcoap_client[i] =NULL;

	}	
  }
  return 0;
}

/**锟斤拷锟斤拷锟酵撅拷锟斤拷锟斤拷应应锟斤拷锟斤拷锟捷ｏ拷锟斤拷锟斤拷post 锟斤拷式
  * 	 unsigned char *strMethod,
  * @param struri 锟斤拷锟斤拷uri path eg: coap://192.168.1.12/time
  * @param data  锟斤拷锟斤拷锟斤拷锟捷碉拷payload 锟斤拷锟斤拷
  * @param handle  应锟斤拷锟斤拷应锟截碉拷锟斤拷锟斤拷
  * @return >=0  on success 锟斤拷锟斤拷锟斤拷锟较D   or <0 on error.
  */
int mlcoap_client_send_msg(unsigned char *hostname,unsigned char *struri,unsigned char *data ,
		mlcoap_client_ack_handler handle)
{
	int clientIndex =-1;
	mlcoap_client_api_t *clientapi = NULL;
	debug("clientapi size is %d", sizeof(mlcoap_client_api_t));
	clientIndex = _new_mlcoap_client();
	if (clientIndex<0)
		return (-1);
	clientapi = g_mlcoap_client[clientIndex];
	clientapi->msgtype =COAP_MESSAGE_CON;
	clientapi->method = COAP_REQUEST_POST;
	clientapi->ackhandle =handle;
	return st_mlcoap_client_sendmsg(clientapi,_getClientUri(hostname,struri),data);
}

/**锟斤拷锟斤拷锟斤拷锟斤拷应锟斤拷锟斤拷锟捷ｏ拷锟斤拷锟斤拷post 锟斤拷式
  * 	 unsigned char *strMethod,
  * @param struri 锟斤拷锟斤拷uri path eg: coap://192.168.1.12/time
  * @param data  锟斤拷锟斤拷锟斤拷锟捷碉拷payload 锟斤拷锟斤拷
  * @return >=0  on success 锟斤拷锟斤拷锟斤拷锟较D   or <0 on error.
  */
int mlcoap_client_send_msg_non_ack(unsigned char *hostname,unsigned char *struri,unsigned char *data,
		mlcoap_client_ack_handler handle)
{
	int clientIndex =-1;
	int ret=0;
	mlcoap_client_api_t *clientapi = NULL;

	clientIndex = _new_mlcoap_client();
	if (clientIndex<0)
		return (-1);
	clientapi = g_mlcoap_client[clientIndex];
	clientapi->msgtype =COAP_MESSAGE_NON;
	clientapi->method = COAP_REQUEST_POST;
	clientapi->ackhandle =handle;
	ret =st_mlcoap_client_sendmsg(clientapi,_getClientUri(hostname,struri),data);
	clientapi->ready =1;
	return ret;
}

int mlcoap_client_send_msg_non(unsigned char *hostname,unsigned char *struri,unsigned char *data)
{
	return mlcoap_client_send_msg_non_ack(hostname,struri,data,NULL);
}



/**目前锟捷诧拷支锟街讹拷锟侥筹拷时锟斤拷只锟斤拷通锟斤拷取锟斤拷锟接口斤拷锟叫讹拷锟斤拷取锟斤拷
*
* @param struri 锟斤拷锟斤拷锟侥碉拷uri path eg: coap://192.168.1.12/time
* @param data  锟斤拷锟斤拷锟斤拷锟捷碉拷payload 锟斤拷锟斤拷
* @param handle 锟斤拷锟斤拷应锟斤拷锟斤拷锟捷回碉拷
* @param subs_handle 锟斤拷锟斤拷通知锟斤拷锟捷回碉拷
* @return >=0  on success 锟斤拷锟斤拷锟斤拷锟较D   or <0 on error.
*/
int mlcoap_client_subscribe(unsigned char *struri,unsigned char *data,
		mlcoap_client_ack_handler handle,mlcoap_client_subscribe_handler subs_handle)
{
	int clientIndex =-1;
	mlcoap_client_api_t *clientapi = NULL;

	clientIndex = _new_mlcoap_client();
	if (clientIndex<0)
		return (-1);
	clientapi = g_mlcoap_client[clientIndex];
	clientapi->msgtype =COAP_MESSAGE_CON;
	clientapi->method = COAP_REQUEST_GET;
	clientapi->ackhandle =handle;
	clientapi->subs_handle = subs_handle;
	
	clientapi->obs_seconds=60*60*24*30;//
	clientapi->wait_seconds+=clientapi->obs_seconds;
	sprintf(clientapi->the_token.s,"%d",clientIndex);
	clientapi->the_token.length=strlen(clientapi->the_token.s);
	
	coap_insert(&clientapi->optlist, new_option_node(COAP_OPTION_SUBSCRIPTION, 0, NULL),
			order_opts);
	return st_mlcoap_client_sendmsg(clientapi,struri,data);
}

/** 取锟斤拷锟斤拷锟侥筹拷锟斤拷锟斤拷锟�
 * @param tid	 锟斤拷锟斤拷锟斤拷时锟斤拷锟截碉拷锟斤拷息ID
 * @return >=0 on success 锟斤拷锟斤拷锟斤拷锟较D  or <0 on error.
 */
int mlcoap_client_subscrible_clear(int tid)
{
	int clientindex = _getClientIndexFromTid(tid);
	if (clientindex<0 )
		return -1;
	mlcoap_client_api_t *clientapi = g_mlcoap_client[clientindex];
	if (clientapi==NULL)
		return (-1);
	clientapi->clear_obs =1;
	return clientapi->tid;
}

int mlcoap_client_send_discover_multicast(char *payload)
{
		return mlcoap_client_send_msg_non(Hostname_MulitCast, URI_SERVICE_DISCOVER, payload);
}

int mlcoap_client_send_ctrl_non(char *hostname, char *payload)
{
        return mlcoap_client_send_msg_non(hostname, URI_SERVICE_CTRL, payload);
}

int mlcoap_client_send_msg_multicast(char *payload)
{
//    static int count = 0;
//    warn("multcast num is %d", ++count);
#if 0
    IPStatusTypedef ipPara;
    char ipaddr[20] = {0};
    int len = 0;
    uint32_t addr = 0;
    uint32_t netmask = 0;
    char *ipStr = NULL;
    struct in_addr inaddr;
    micoWlanGetIPStatus(&ipPara, Station);

    addr = inet_addr(ipPara.ip);
    netmask = inet_addr(ipPara.mask);
    inaddr.s_addr = (~netmask) | addr;
    ipStr = inet_ntoa(inaddr);
    strcpy(ipaddr, ipStr);
//    warn("broadcast ipaddr: %s", ipaddr);

    return mlcoap_client_send_msg_non(ipaddr, URI_SERVICE_MSG, payload);
#else
    return mlcoap_client_send_msg_non(Hostname_MulitCast, URI_SERVICE_MSG, payload);
//    return mlcoap_client_send_msg_non("192.168.12.160", URI_SERVICE_MSG, payload);
#endif
}

int mlcoap_client_send_msg_ex(char *addr, char *payload)
{
        return mlcoap_client_send_msg_non(addr, URI_SERVICE_MSG, payload);
}
#if 0
int mlcoap_client_send_msg_terminal( unsigned char *payload)
{
    int addrIndex = 0;
    SENDADDR_PARAM_T sendAddrInfo  = {0};
    for (addrIndex=0; addrIndex < SEND_ADDR_NUM_MAX; addrIndex++)
    {
        mlink_sys_get_sendaddr(addrIndex, &sendAddrInfo);
        if (sendAddrInfo.addr[0] != 0)
        {
            warn("\n=====*********** send addr %s addrIndex:%d mac: %s*******=======", sendAddrInfo.addr, addrIndex, sendAddrInfo.srcmac);
            mlcoap_client_send_msg_non((unsigned char *)sendAddrInfo.addr, URI_SERVICE_MSG, payload);
        }
    }
    return kNoErr;
}
#else

#endif
// add by 2018.3.3
int mlcoap_get_error(void)
{
    int ret =0;
    if (st_error_coap_send>10)
        ret =1;
    return ret;
}

