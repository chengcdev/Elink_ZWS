#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>

#if 0

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>

#endif

#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "resource.h"
#include "coap.h"
#include "ml_coap.h"
#include "debug_coap.h"
#include "coap_client.h"
#include "../queue/queue.h"

coap_context_t  *g_svrctx;

typedef struct { 
  char *uri;
  coap_resource_t *resource;
  unsigned int obs;
  unsigned char *obsNotifyData;
  unsigned int obsDataLen;
  coap_method_handler_t coap_handler;
  ml_coap_restful_deal_handler_t restfull_handler;
} mlcoap_service_t;


//unsigned char g_obsdataTemp[1024]={0};
static QUEUE g_notifyQueue = {0};

void hnd_service_getpost(coap_context_t  *ctx, struct coap_resource_t *resource, 
	     coap_address_t *peer, coap_pdu_t *request, str *token,
	     coap_pdu_t *response);


#define ML_COAP_SERVICE_MAX_NUM		5



static mlcoap_service_t g_coap_service[]={
	{
		URI_SERVICE_SYS,
		NULL,
		0,
		NULL,
		0,
		hnd_service_getpost,
		NULL,
	},
	{
		URI_SERVICE_MANAGER,
		NULL,
		0,
		NULL,
		0,
		hnd_service_getpost,
		NULL,
	},
	{
		URI_SERVICE_CTRL,
		NULL,
		0,
		NULL,
		0,
		hnd_service_getpost,
		NULL,
	},
	{
		URI_SERVICE_MSG,
		NULL,
		1,
		NULL,
		0,
		hnd_service_getpost,
		NULL,
	},
	{
		URI_SERVICE_DISCOVER,
		NULL,
		1,
		NULL,
		0,
		hnd_service_getpost,//hnd_service_discover,
		NULL,
	},

};

static int get_servicetype(const char *uri)
{
	int ret = -1;
	if (strcmp(uri,URI_SERVICE_SYS)==0)
		ret = ML_COAP_URITYPE_SYS;
	else if (strcmp(uri,URI_SERVICE_MANAGER)==0)
		ret = ML_COAP_URITYPE_MANAGER;
	else if (strcmp(uri,URI_SERVICE_CTRL)==0)
		ret = ML_COAP_URITYPE_CTRL;
	else if (strcmp(uri,URI_SERVICE_MSG)==0)
		ret = ML_COAP_URITYPE_MSG;
	else if (strcmp(uri,URI_SERVICE_DISCOVER)==0)
		ret = ML_COAP_URITYPE_DISCOVER;

	return (int)ret;
}

/**
 *  初始化通知信息队列
 *
 */
static void coap_init_notify_queue( void )
{
    init_queue(&g_notifyQueue, sizeof(ml_coap_notifyMsg_t));
}

/**
 *  将通知数据放入队列中
 *
 */
static void coap_push_notify( ml_coap_notifyMsg_t *msg )
{
    int ret = 0;
    ret = push_node(&g_notifyQueue, msg);
    if (ret == FALSE)
    {
        debug("push notify msg node fail !!!");
    }
//    debug("push notify msg node ok[%d] !!!",g_notifyQueue.count);
}

/**
 *  将通知数据放入队列中
 *
 */
static ml_coap_notifyMsg_t *coap_pop_notify( void )
{
    return pop_node(&g_notifyQueue);
}


/**
 *  获取通知数据
 *
 */
void coap_get_notify( void )
{
    ml_coap_notifyMsg_t *pnotifyMsg = NULL;
    pnotifyMsg = coap_pop_notify();
    if (pnotifyMsg != NULL)
    {
//        debug("g_coap_service[uritype].obsNotifyData: %x",g_coap_service[pnotifyMsg->uriType].obsNotifyData);
        if (g_coap_service[pnotifyMsg->uriType].obsNotifyData == NULL)
        {
            g_coap_service[pnotifyMsg->uriType].obsNotifyData = coap_malloc(ML_COAP_OBS_MAXLEN);
        }

        memset(g_coap_service[pnotifyMsg->uriType].obsNotifyData,0,ML_COAP_OBS_MAXLEN);
        memcpy(g_coap_service[pnotifyMsg->uriType].obsNotifyData,pnotifyMsg->notifyData, pnotifyMsg->notifySize);
//        warn("notifyData: %s", g_coap_service[pnotifyMsg->uriType].obsNotifyData);
        g_coap_service[pnotifyMsg->uriType].obsDataLen = pnotifyMsg->notifySize;
        g_coap_service[pnotifyMsg->uriType].resource->dirty=1;
        free(pnotifyMsg);
    }
}
static coap_context_t *get_context(const char *node, const char *port) {
  coap_context_t *ctx = NULL;  
  int s;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
//  hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
  
  s = getaddrinfo(node, port, &hints, &result);

//  debug("svr node: %s %s \n",node,port);

  if ( s != 0 ) {
//    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
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
  
  fprintf(stderr, "no context available for interface '%s'\n", node);

 finish:
  freeaddrinfo(result);
  return ctx;
}

static void _get_pdu_option_value(coap_pdu_t *pdu,int optionfilter,
	coap_opt_iterator_t *opt_iter,unsigned char *data,int maxlen)
{
	coap_opt_t *option;
	int len=maxlen;
	if (data==NULL || pdu ==NULL ||opt_iter==NULL)
		return;
	memset(data,0,maxlen);
	option = coap_check_option(pdu,(unsigned char) optionfilter, opt_iter);
	
	if (option) {
		 unsigned char *p = COAP_OPT_VALUE(option);
		 if (COAP_OPT_LENGTH(option)<maxlen){
			len = COAP_OPT_LENGTH(option);
		 }
		 memcpy(data,p,len);
	}	
}

//////////////////////////// coap service hnd deal   ////////////////////

void hnd_service_getpost(coap_context_t  *ctx, struct coap_resource_t *resource, 
	     coap_address_t *peer, coap_pdu_t *request, str *token,
	     coap_pdu_t *response) 
{
	int res,buflen,result =0;
	char buf[40] = {0};
	
	coap_opt_iterator_t opt_iter;
	coap_subscription_t *subscription = NULL;
	unsigned char addr[48];
	ml_coap_reqPacket_t reqPacket;
	ml_coap_ackPacket_t ackPacket;
	int uritype =0;
	coap_opt_t *option;
	unsigned char observeState = 0xff;  // 0: cancel observe
	
    coap_block_t block;
    static char *recvBuff = NULL;
    static char *respBuff = NULL;
    static int respBuffLen = 0;
    static int responsedLen = 0;

	memset(&reqPacket, 0, sizeof(ml_coap_reqPacket_t));
	memset(&ackPacket, 0, sizeof(ml_coap_ackPacket_t));

	coap_print_addr(peer,addr,48);

  //  res = coap_split_path(resource->uri.s, resource->uri.length, buf, &buflen);
	if (request){
		debug("[token:%d  %s] id:%d  %s   %s \n",
		      token->length,token->s,request->hdr->id,request->data,addr);
	}

	uritype = get_servicetype(resource->uri.s);
//	debug("uritype: %d, optype: %d %d ",  uritype, request->hdr->type,ctx->observe);
	if (uritype<0){
		response->hdr->code = COAP_RESPONSE_CODE(500);
		return;
	}
	response->hdr->code = 
				 g_coap_service[uritype].restfull_handler ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(503);
	if (request && g_coap_service[uritype].restfull_handler ==NULL){
		
		return;
	}
	if ( g_coap_service[uritype].obs ){

		if (request != NULL){
			char  *obsStatus;
			int observer_temp=0;

			option = coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter);

			if (option && COAP_OPT_LENGTH(option)>0){
				obsStatus = COAP_OPT_VALUE(option);

				memcpy(&observer_temp,obsStatus,COAP_OPT_LENGTH(option));
				if ( observer_temp >0 ){

					coap_queue_t *sent = NULL;
					/* find transaction in sendqueue to stop retransmission */
					coap_remove_from_queue(&ctx->sendqueue, request->hdr->id, &sent);
					if (sent)
			    	  coap_cancel(ctx, sent);
                    debug("=====  [hxf][test] coap_remove_observers_1   =====");
                    coap_remove_observers_1(ctx,resource,peer,token);
                    observeState = 0;
				}
			}
		//if (request != NULL &&
		//  coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter))

			if (request != NULL && option && COAP_OPT_LENGTH(option) ==0 )
			{
				subscription = coap_add_observer(resource, peer, token);
				coap_print_addr(&subscription->subscriber,addr,48);

				debug("=====  [hxf][test] coap_add_observer [%x] %s   =====",
				      subscription ,addr);
//printf("=======[test] obsStatus[%x] %s ======\n",subscription ,token->s);
				if (subscription) {
					//	          subscription->non = COAP_MESSAGE_NON;

					subscription->non = request->hdr->type == COAP_MESSAGE_NON;
					coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
				}
			}
		}
		else
		{
		    debug("request is NULL");
		}

//debug("[hxf tst] %s %d %d [hdrtype:%d %d %d ] \n",
//      g_coap_service[uritype].obsNotifyData, ctx->observe,resource->dirty,
//      response->hdr->type,response->hdr->id ,ctx->message_id);
//

		if ( resource->dirty == 1 ){
//            debug("COAP_OPTION_OBSERVE : %d, %d", (unsigned char)buf[0], coap_encode_var_bytes(buf, ctx->observe));

			coap_add_option(response, COAP_OPTION_OBSERVE, 
				coap_encode_var_bytes(buf, ctx->observe+1), buf);
//			debug("COAP_OPTION_OBSERVE : %d, %d", ctx->observe, coap_encode_var_bytes(buf, ctx->observe));
			//COAP_MEDIATYPE_TEXT_PLAIN,COAP_MEDIATYPE_APPLICATION_JSON
			  coap_add_option(response, COAP_OPTION_CONTENT_FORMAT,
					  coap_encode_var_bytes(buf,COAP_MEDIATYPE_TEXT_PLAIN  ), buf);

	//		coap_add_option(response, COAP_OPTION_MAXAGE,
	//			coap_encode_var_bytes(buf, 0x01), buf);
			
			coap_add_data(response, g_coap_service[uritype].obsDataLen,
						g_coap_service[uritype].obsNotifyData);


			return;
		}
	}

	if (request->data==NULL && g_coap_service[uritype].obs==0){
		response->hdr->code = COAP_RESPONSE_CODE(400);
//		return;
	}

#if 1              // modify by chenb 20180628. support reciving block

    if (request) {
      int res;
      if (coap_get_block(request, COAP_OPTION_BLOCK1, &block)){
            static int recvBuffLen = 0;
            int recvLen = 0;
            if ((block.num==0)&&(recvBuffLen != 0))
            {
                recvBuffLen = 0;
                response->hdr->code = COAP_RESPONSE_CODE(400);
                return;
            }
            else if ((block.num != 0)&&(recvBuffLen == 0))
            {
                response->hdr->code = COAP_RESPONSE_CODE(400);
                return;
            }
       coap_add_option(response, COAP_OPTION_BLOCK1,
                  coap_encode_var_bytes(buf, ((block.num << 4) |
                              (block.m << 3) |
                              block.szx)), buf);

           if (recvBuff == NULL)
           {
               recvBuff = malloc(2048);
               memset(recvBuff, 0, 2048);
           }
           recvLen = (unsigned char *)request->hdr + request->length - request->data;
           recvBuffLen = recvBuffLen + recvLen;
           if (recvBuffLen < 2048)
           {
               memcpy(recvBuff+recvBuffLen-recvLen, request->data, recvLen);
               *(recvBuff+recvBuffLen) = 0;
           }

           if(block.m){
               response->hdr->code = COAP_RESPONSE_CODE(231);
               return ;
           }

           reqPacket.data = recvBuff;
           reqPacket.length =  recvBuffLen;
           recvBuffLen = 0;
           printf("recvice data is %s\r\n", reqPacket.data);
      }
      else if (coap_get_block(request, COAP_OPTION_BLOCK2, &block))
      {
         if (block.num && (respBuff != NULL))
         {
            char ackData[1028] = {0};
            int ackLen = 0;
            if (respBuffLen == 0)
            {
                response->hdr->code = COAP_RESPONSE_CODE(400);
                return;
            }
            if ((respBuffLen-responsedLen)>1024)
            {
                ackLen = 1024;
                memcpy(ackData, respBuff+responsedLen, ackLen);
                block.m = 1;
            }
            else
            {
                ackLen = respBuffLen-responsedLen;
                memcpy(ackData, respBuff+responsedLen, ackLen);
                block.m = 0;
                respBuffLen = 0;
            }

            coap_add_option(response, COAP_OPTION_BLOCK2,
           coap_encode_var_bytes(buf, ((block.num << 4) |
                       (block.m << 3) |
                       block.szx)), buf);
            response->hdr->code = COAP_RESPONSE_CODE(205);
            result = coap_add_data(response, ackLen, ackData);

            return;
         }
         else
         {
             reqPacket.data = request->data;
             reqPacket.length =  (unsigned char *)request->hdr + request->length - request->data;
         }
      }
      else
      {
          reqPacket.data = request->data;
          reqPacket.length =  (unsigned char *)request->hdr + request->length - request->data;
      }
    }
    else
    {
        reqPacket.data = request->data;
        reqPacket.length =  (unsigned char *)request->hdr + request->length - request->data;
    }
    _get_pdu_option_value(request,ML_COAP_OPTION_Encrypt,&opt_iter,reqPacket.encrypt,sizeof(reqPacket.encrypt));
    _get_pdu_option_value(request,ML_COAP_OPTION_Authorized,&opt_iter,reqPacket.verify,sizeof(reqPacket.verify));

    ackPacket.echoValue = response->hdr->code;

    memset(reqPacket.token,0,TOKEN_LENGTH_MAX);
    if ( token && (token->length>0) && token->s && (token->length<TOKEN_LENGTH_MAX))
        memcpy(reqPacket.token ,token->s,token->length);

    reqPacket.id = request->hdr->id;
    reqPacket.srcaddr = addr;
#else
    _get_pdu_option_value(request,ML_COAP_OPTION_Encrypt,&opt_iter,reqPacket.encrypt,sizeof(reqPacket.encrypt));
    _get_pdu_option_value(request,ML_COAP_OPTION_Authorized,&opt_iter,reqPacket.verify,sizeof(reqPacket.verify));

    ackPacket.echoValue = response->hdr->code;

    memset(reqPacket.token,0,TOKEN_LENGTH_MAX);
    if ( token && (token->length>0) && token->s && (token->length<TOKEN_LENGTH_MAX))
        memcpy(reqPacket.token ,token->s,token->length);

    reqPacket.id = request->hdr->id;
    reqPacket.data = request->data;
    reqPacket.srcaddr = addr;
    reqPacket.length =  (unsigned char *)request->hdr + request->length - request->data;

#endif


	if (request && g_coap_service[uritype].restfull_handler && (observeState != 0)){
		result = g_coap_service[uritype].restfull_handler(&reqPacket, &ackPacket);

#if 1       // modify 20180628. in order to suport transporting block
        if ((reqPacket.data == recvBuff)&&(reqPacket.data != NULL))
        {
            response->hdr->code = COAP_RESPONSE_CODE(204);
        }
        else
        {
            response->hdr->code = ackPacket.echoValue;
        }
#else
        response->hdr->code = ackPacket.echoValue;
#endif
		if (result<0)
		{
		    if (ackPacket.data !=  NULL)
		    {
		        free(ackPacket.data);
		        ackPacket.data = NULL;
		    }
//		    warn("ackPacket echoValue is error");
            return;
		}

		if ((ackPacket.datalen>0)  && (ackPacket.data!=NULL)){
#if 1       // support response block
		    // when response data length greater than 1024 . we will send data with block
            if (ackPacket.datalen > 1024)
            {
                if (respBuffLen == 0)
                {
                    // add block2 option
                    block.num = 0;
                    block.m = 1;
                    block.szx = 6;
                    coap_add_option(response, COAP_OPTION_BLOCK2,
                               coap_encode_var_bytes(buf, ((block.num << 4) |
                                           (block.m << 3) |
                                           block.szx)), buf);

                    if (respBuff == NULL)
                    {
                        respBuff = malloc(2048);
                    }
                    memset(respBuff, 0, 2048);
                    // record ack length to respBuffLen
                    respBuffLen = ackPacket.datalen;
                    // copy ackPacket data to respBuff
                    memcpy(respBuff, ackPacket.data, respBuffLen);
                    // subcontract data
                    ackPacket.datalen = 1024;
                    *(ackPacket.data+1024) = '\0';
                    responsedLen = 1024;
                }
                else
                {
                    respBuffLen = 0;
                    ackPacket.datalen = 0;
                    response->hdr->code = COAP_RESPONSE_CODE(400);
                }
            }
#endif
			result = coap_add_data(response, ackPacket.datalen, ackPacket.data);
		}
		else if (request->hdr->type == COAP_MESSAGE_CON)
		{
            coap_add_data(response, 0, NULL);
		}

        if (ackPacket.data !=  NULL)
        {
            free(ackPacket.data);
            ackPacket.data = NULL;
        }
//	    coap_send_confirmed(ctx,peer,response);

	}
	else if (request->hdr->type == COAP_MESSAGE_CON)
	{
	    // if message's type is COAP_MESSAGE_CON, it need be acked
        coap_add_data(response, 0, NULL);
	}
}

///////   ml coap external restfull api interface  //////////////////////		
int ml_coap_init()
{
	coap_context_t  *ctx;
	coap_resource_t *r;
	int i;
	
	
    char addr_str[32] = ML_COAP_DEFAULT_ADDR;//0.0.0.0
    char port_str[8] = ML_COAP_DEFAULT_PORT;//"5683";
	
	coap_log_t log_level = LOG_DEBUG;// LOG_WARNING;
	coap_set_log_level(log_level);
	
	ctx = get_context(addr_str, port_str);
	
	debug (" ==coap init== addr [%s %s ]  %d\n",addr_str,port_str,ctx);
  	if (!ctx)
    	return -1;

	int err,yes,one;
	int loop= 0; 

//	uint32_t mcast_addr = inet_addr(ML_COAP_SYS_MCAST_ADDR);
//	uint8_t mcast_mac[6];

//	wifi_get_ipv4_multicast_mac(ntohl(mcast_addr), mcast_mac);
//	wifi_add_mcast_filter(mcast_mac);
	yes = 1;
	err = setsockopt(ctx->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));
	one = 0;
	err = setsockopt(ctx->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&one, sizeof(one));
	one = 255;
	err = setsockopt(ctx->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&one, sizeof(one));

	 if(err< 0) 
	  perror("setsockopt():IP_MULTICAST_LOOP");

     struct ip_mreq mreq; /*加入广播组*/
     mreq.imr_multiaddr.s_addr= inet_addr(ML_COAP_SYS_MCAST_ADDR); /*广播地址*/
     mreq.imr_interface.s_addr= htonl(INADDR_ANY); /*网络接口为默认*/
                                                              /*将本机加入广播组*/
	 err = setsockopt(ctx->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
	 if (err< 0) 
	  perror("setsockopt():IP_ADD_MEMBERSHIP");
	i=0;
	for (i=0;i<ML_COAP_SERVICE_MAX_NUM;i++)
	{
	  	
		r = coap_resource_init((unsigned char *)g_coap_service[i].uri, strlen(g_coap_service[i].uri), 0);
		
		r->observable = g_coap_service[i].obs;

		coap_register_handler(r, COAP_REQUEST_GET, g_coap_service[i].coap_handler);
		coap_register_handler(r, COAP_REQUEST_POST, g_coap_service[i].coap_handler);
		coap_add_resource(ctx, r);

		g_coap_service[i].resource = r;
	}
//	mlcoap_set_token(ML_COAP_DEFAULT_TOKEN);  note by 2017.5.15
	coap_register_option(ctx, ML_COAP_OPTION_Authorized);
	coap_register_option(ctx, ML_COAP_OPTION_Encrypt);
	g_svrctx = ctx;
	coap_init_notify_queue();
	return 0;
}
void ml_exit_coap(void)
{
	int i;
	for (i=0;i<ML_COAP_SERVICE_MAX_NUM;i++)
	{
		if (g_coap_service[i].obs)
		{
			coap_free(g_coap_service[i].obsNotifyData);
			g_coap_service[i].obsNotifyData = NULL;
		}
	}
	coap_free_context( g_svrctx );
}


/**
 *  锟斤拷询锟斤拷锟斤拷coap sock
 * 
 */
void ml_poll_coap(void)
{
    fd_set readfds;
    struct timeval tv, *timeout;
    int result;
    coap_tick_t now;
    coap_queue_t *nextpdu;
	coap_context_t  *ctx;
	ctx = g_svrctx;

    FD_ZERO(&readfds);
    FD_SET( ctx->sockfd, &readfds );

    nextpdu = coap_peek_next( ctx );

    coap_ticks(&now);
    while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
        coap_tid_t tid=coap_retransmit( ctx, coap_pop_next( ctx ) );
      debug("==== [test] send server coap_retransmit [%d] =====\n",tid);

      nextpdu = coap_peek_next( ctx );
    }
#if 0
    if ( nextpdu && nextpdu->t <= COAP_RESOURCE_CHECK_TIME ) {
      /* set timeout if there is a pdu to send before our automatic timeout occurs */
      tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
      tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
      timeout = &tv;
    } else {
      tv.tv_usec = 0;
      tv.tv_sec = COAP_RESOURCE_CHECK_TIME;
      timeout = &tv;
    }
#else
    tv.tv_usec =50;
    tv.tv_sec =0;
    timeout = &tv;
#endif
    result = select( FD_SETSIZE, &readfds, 0, 0, timeout );
    if ( result < 0 ) {		/* error */
      if (errno != EINTR)
	perror("select");
    } else if ( result > 0 ) {	/* read from socket */
      if ( FD_ISSET( ctx->sockfd, &readfds ) ) {
          coap_read( ctx );	/* read received data */
          coap_dispatch( ctx );	/* and dispatch PDUs from receivequeue */
      }
    } else {			/* timeout */
      // if (time_resource) {
	// time_resource->dirty = 1;
      //}
    }

#ifndef WITHOUT_ASYNC
    /* check if we have to send asynchronous responses */
    // check_async(ctx, now);
#endif /* WITHOUT_ASYNC */

#ifndef WITHOUT_OBSERVE
    // put notif data
    coap_get_notify();
//    debug("============");
    /* check if we have to send observe notifications */
    coap_check_notify(ctx);
#endif /* WITHOUT_OBSERVE */
  
}
/**
 *  coap 注锟斤拷涌诜锟斤拷锟截碉拷锟斤拷锟斤拷
 *
 * @param uritype	 锟斤拷锟侥观诧拷锟斤拷锟斤拷锟斤拷锟�
 * @param handle   锟斤拷应锟接口凤拷锟斤拷幕氐锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
 */
void ml_coap_reg_handler(ML_COAP_URITYPE_E uritype,ml_coap_restful_deal_handler_t *handle)
{
	g_coap_service[uritype].restfull_handler = handle;
}

int ml_coap_service_check( void )
{
    if (g_coap_service[0].restfull_handler && g_coap_service[1].restfull_handler &&
        g_coap_service[2].restfull_handler && g_coap_service[3].restfull_handler && g_coap_service[4].restfull_handler)
    {
        return 0;
    }
    return -1;
}

/**
 *   发送通知给观察者
 *
 * @param uritype	 服务类型
 * @param notifydata 通知内容
 * @param datalen  通知内容长度
 * @return >=0on success or <0 on error.
 */
int  ml_coap_notify_obs(ML_COAP_URITYPE_E uritype , char *notifydata,unsigned int datalen)
{
	int ret =0;
	if (uritype>=ML_COAP_SERVICE_MAX_NUM || uritype<0)
		return -1;
	if (g_coap_service[uritype].obs<=0 || datalen>=ML_COAP_OBS_MAXLEN)
		return -2;
	if (MicoGetMemoryInfo()->free_memory <= 5120)
	{
	    warn("memory free scarce");
	    return -3;
	}
#if 0
    debug("g_coap_service[uritype].obsNotifyData: %x",g_coap_service[uritype].obsNotifyData);
	if (g_coap_service[uritype].obsNotifyData == NULL)
		g_coap_service[uritype].obsNotifyData = coap_malloc(ML_COAP_OBS_MAXLEN);
	{
		memset(g_coap_service[uritype].obsNotifyData,0,ML_COAP_OBS_MAXLEN);

		memcpy(g_coap_service[uritype].obsNotifyData,notifydata,datalen);
	}
	
	g_coap_service[uritype].obsDataLen = datalen;
	g_coap_service[uritype].resource->dirty=1;
#else
	ml_coap_notifyMsg_t notifyMsg = {0};
	memset(&notifyMsg, 0, sizeof(ml_coap_notifyMsg_t));
	notifyMsg.uriType = uritype;
	notifyMsg.notifySize = datalen;
	memcpy(notifyMsg.notifyData, notifydata, notifyMsg.notifySize);
	coap_push_notify(&notifyMsg);

#endif

	return ret;
}


