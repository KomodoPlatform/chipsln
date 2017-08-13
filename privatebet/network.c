/******************************************************************************
 * Copyright © 2014-2018 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


void BET_message_send(char *debugstr,int32_t sock,cJSON *msgjson,int32_t freeflag)
{
    int32_t sendlen,len; char *msg;
    if ( jobj(msgjson,"sender") != 0 )
        jdelete(msgjson,"sender");
    jaddbits256(msgjson,"sender",Mypubkey);
    if ( jobj(msgjson,"node_id") != 0 )
        jdelete(msgjson,"node_id");
    jaddstr(msgjson,"node_id",LN_idstr);
    msg = jprint(msgjson,freeflag);
    len = (int32_t)strlen(msg) + 1;
    if ( (sendlen= nn_send(sock,msg,len,0)) != len )
        printf("%s: sendlen.%d != recvlen.%d for %s, pushsock.%d\n",debugstr,sendlen,len,msg,sock);
    //else printf("SEND.[%s]\n",msg);
    free(msg);
}

void BET_broadcast(int32_t pubsock,uint8_t *decoded,int32_t maxsize,bits256 *playerprivs,bits256 *playerpubs,int32_t numplayers,int32_t numcards,uint8_t *sendbuf,int32_t size,bits256 deckid)
{
    int32_t i,slen; bits256 checkdeckid,cardpubs[CARDS777_MAXCARDS];
    slen = (int32_t)strlen((char *)sendbuf);
    if ( pubsock >= 0 )
    {
        nn_send(pubsock,sendbuf,size,0);
    }
    else if ( slen+1+sizeof(bits256) < size && playerprivs != 0 )
    {
        for (i=0; i<numplayers; i++)
        {
            if ( BET_process_packet(cardpubs,&checkdeckid,GENESIS_PUBKEY,playerprivs[i],decoded,maxsize,playerpubs[i],sendbuf,size,numplayers,numcards) != 0 )
                break;
        }
    }
}

void BET_roundstart(int32_t pubsock,cJSON *deckjson,int32_t numcards,bits256 *privkeys,bits256 *playerpubs,int32_t numplayers,bits256 privkey0)
{
    static uint8_t *decoded; static int32_t decodedlen;
    int32_t i,n,len=0,slen,size=0; bits256 deckid; uint8_t *sendbuf=0; cJSON *array; char *deckjsonstr,*msg; cJSON *sendjson;
    deckid = jbits256(deckjson,"deckid");
    if ( (array= jarray(&n,deckjson,"ciphers")) != 0 && n == numplayers )
    {
        sendjson = jduplicate(deckjson);
        jdelete(sendjson,"ciphers");
        jdelete(sendjson,"result");
        deckjsonstr = jprint(sendjson,1);
        //printf("deckjsonstr.(%s)\n",deckjsonstr);
        slen = (int32_t)strlen(deckjsonstr);
        for (i=0; i<numplayers; i++)
        {
            fprintf(stderr,"%d ",i);
            msg = jstri(array,i);
            if ( sendbuf == 0 )
            {
                len = (int32_t)strlen(msg) >> 1;
                size = slen + 1 + sizeof(playerpubs[i]) + len;
                sendbuf = malloc(size);
                memcpy(sendbuf,deckjsonstr,slen+1);
            }
            else if ( (strlen(msg) >> 1) != len )
            {
                printf("[%d of %d] unexpected mismatched len.%d vs %d\n",i,numplayers,(int32_t)strlen(msg),len);
                continue;
            }
            memcpy(&sendbuf[slen+1],playerpubs[i].bytes,sizeof(playerpubs[i]));
            decode_hex(&sendbuf[slen+1+sizeof(playerpubs[i])],len,msg);
            if ( decodedlen < size )
            {
                decoded = realloc(decoded,size);
                decodedlen = size;
                printf("alloc decoded[%d]\n",size);
            }
            BET_broadcast(pubsock,decoded,decodedlen,privkeys,playerpubs,numplayers,numcards,sendbuf,size,deckid);
        }
        free(deckjsonstr);
    }
}

char *BET_transportname(int32_t bindflag,char *str,char *ipaddr,uint16_t port)
{
    sprintf(str,"tcp://%s:%u",bindflag == 0 ? ipaddr : "*",port); // ws is worse
    return(str);
}

int32_t BET_nanosock(int32_t bindflag,char *endpoint,int32_t nntype)
{
    int32_t sock,timeout;
    if ( (sock= nn_socket(AF_SP,nntype)) >= 0 )
    {
        if ( bindflag == 0 )
        {
            if ( nn_connect(sock,endpoint) < 0 )
            {
                printf("connect to %s error for %s\n",endpoint,nn_strerror(nn_errno()));
                nn_close(sock);
                return(-1);
            } else printf("nntype.%d connect to %s connectsock.%d\n",nntype,endpoint,sock);
        }
        else
        {
            if ( nn_bind(sock,endpoint) < 0 )
            {
                printf("bind to %s error for %s\n",endpoint,nn_strerror(nn_errno()));
                nn_close(sock);
                return(-1);
            } else printf("(%s) bound\n",endpoint);
        }
        timeout = 1;
        nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout));
        timeout = 100;
        nn_setsockopt(sock,NN_SOL_SOCKET,NN_SNDTIMEO,&timeout,sizeof(timeout));
        //maxsize = 2 * 1024 * 1024;
        //nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVBUF,&maxsize,sizeof(maxsize));
        if ( nntype == NN_SUB )
            nn_setsockopt(sock,NN_SUB,NN_SUB_SUBSCRIBE,"",0);
    }
    return(sock);
}

void BET_mofn_send(struct privatebet_info *bet,struct privatebet_vars *vars,int32_t cardi,int32_t playerj,int32_t encryptflag)
{
    bits256 shard; cJSON *reqjson; int32_t msglen; uint8_t encoded[sizeof(bits256) + 1024]; char cipherstr[sizeof(encoded)+2+1];
    shard = bet->MofN[cardi*bet->numplayers + playerj];
    if ( bits256_nonz(shard) != 0 )
    {
        reqjson = cJSON_CreateObject();
        jaddstr(reqjson,"method","MofN");
        jaddnum(reqjson,"playerj",playerj);
        jaddnum(reqjson,"cardi",cardi);
        if ( encryptflag == 0 )
            jaddbits256(reqjson,"shard",shard);
        else
        {
            msglen = BET_ciphercreate(Myprivkey,bet->playerpubs[playerj],encoded,shard.bytes,sizeof(shard));
            init_hexbytes_noT(cipherstr,encoded,msglen);
            jaddstr(reqjson,"cipher",cipherstr);
            //char str[65]; printf("%s -> cipherstr.(%s)\n",bits256_str(str,shard),cipherstr);
        }
        //char str[65]; fprintf(stderr,"{j%d c%d %s} ",j,cardi,bits256_str(str,shard));
        BET_message_send("BET_mofn_send",bet->pubsock>=0?bet->pubsock:bet->pushsock,reqjson,1);
    }
    else
    {
        int32_t i; char str[65]; for (i=0; i<bet->numplayers * bet->range; i++)
            printf("%s ",bits256_str(str,bet->MofN[i]));
        printf("MofN.%p\n",bet->MofN);
        printf("null shard cardi.%d/%d j.%d/%d MofN.%p\n",cardi,bet->range,playerj,bet->numplayers,bet->MofN);
    }
}

int32_t BET_client_deali(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars,int32_t senderid)
{
    int32_t deali,cardi,j;
    deali = jint(argjson,"deali");
    cardi = jint(argjson,"cardi");
    if ( (j= jint(argjson,"playerj")) < 0 )
    {
        for (j=0; j<bet->numplayers; j++)
            BET_mofn_send(bet,vars,cardi,j,0);
    } else BET_mofn_send(bet,vars,cardi,j,1);
    //printf("client deali.%d cardi.%d j.%d\n",deali,cardi,j);
    return(0);
}

