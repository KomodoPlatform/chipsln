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

bits256 *BET_process_packet(bits256 *cardpubs,bits256 *deckidp,bits256 senderpub,bits256 mypriv,uint8_t *decoded,int32_t maxsize,bits256 mypub,uint8_t *sendbuf,int32_t size,int32_t checkplayers,int32_t range)
{
    int32_t j,k,i,n,slen,recvlen,numplayers,numcards,myid=-1; uint8_t *recv; bits256 *MofN,deckid,checkpub,playerpubs[CARDS777_MAXPLAYERS]; cJSON *deckjson,*array; char str[65],str2[65];
    slen = (int32_t)strlen((char *)sendbuf);
    if ( (deckjson= cJSON_Parse((char *)sendbuf)) == 0 )
    {
        printf("couldnt parse sendbuf\n");
        return(0);
    }
    if ( (numplayers= jint(deckjson,"numplayers")) <= 0 || numplayers != checkplayers )
    {
        printf("no numplayers\n");
        return(0);
    }
    if ( (numcards= jint(deckjson,"numcards")) <= 0 || numcards != range )
    {
        printf("no numcards or numcards.%d != range.%d (%s)\n",numcards,range,jprint(deckjson,0));
        return(0);
    }
    deckid = jbits256(deckjson,"deckid");
    *deckidp = deckid;
    if ( (array= jarray(&n,deckjson,"players")) != 0 && n == numplayers )
    {
        for (i=0; i<numplayers; i++)
        {
            playerpubs[i] = jbits256i(array,i);
            if ( bits256_cmp(mypub,playerpubs[i]) == 0 )
                myid = i;
        }
        if ( myid < 0 )
        {
            printf("mismatched playerpub[%d of %d]\n",i,numplayers);
            return(0);
        }
    }
    if ( (array= jarray(&n,deckjson,"cardpubs")) != 0 && n == numcards )
    {
        for (i=0; i<numcards; i++)
            cardpubs[i] = jbits256i(array,i);
        checkpub = cards777_deckid(cardpubs,numcards,deckid);
        if ( bits256_cmp(checkpub,deckid) != 0 )
        {
            printf("error comparing deckid %s vs %s\n",bits256_str(str,checkpub),bits256_str(str2,deckid));
            return(0);
        } else printf("verified deckid %s\n",bits256_str(str,deckid));
    }
    if ( memcmp(&sendbuf[slen+1],playerpubs[myid].bytes,sizeof(playerpubs[myid])) == 0 )
    {
        recv = &sendbuf[slen+1+sizeof(bits256)];
        recvlen = size - (int32_t)(slen+1+sizeof(bits256));
        if ( (MofN= (bits256 *)BET_decrypt(decoded,maxsize,senderpub,mypriv,recv,&recvlen)) != 0 )
        {
            if ( recvlen/sizeof(bits256) == numplayers*numcards )
            {
                for (k=0; k<numcards; k++)
                {
                    for (j=0; j<numplayers; j++)
                    {
                        //fprintf(stderr,"[c%d p%d %s].%d ",k,j,bits256_str(str,MofN[k*numplayers + j]),myid);
                        BET_MofN_item(deckid,k,cardpubs,numcards,j,numplayers,MofN[k*numplayers + j],myid);
                    }
                }
                //printf("new recvlen.%d -> %d\n",recvlen,(int32_t)(recvlen/sizeof(bits256)));
                return(MofN);
            } else printf("recvlen %d mismatch p%d c%d\n",recvlen,numplayers,numcards);
        } else printf("decryption error\n");
        return(0);
    }
    return(0);
    // packet for different node
    /*for (i=0; i<32; i++)
        printf("%02x",sendbuf[slen+1+i]);
    printf(" sent pubkey\n");
    for (i=0; i<32; i++)
        printf("%02x",playerpubs[myid].bytes[i]);
    printf(" myid.%d pubkey\n",myid);
    printf("memcmp playerpubs error\n");
    return(-1);*/
}

void BET_clientloop(void *_ptr)
{
    int32_t nonz,recvlen; void *ptr; cJSON *msgjson; struct privatebet_vars *VARS; struct privatebet_info *bet = _ptr;
    VARS = calloc(1,sizeof(*VARS));
    printf("client loop: pushsock.%d subsock.%d\n",bet->pushsock,bet->subsock);
    while ( bet->subsock >= 0 && bet->pushsock >= 0 )
    {
        nonz = 0;
        if ( (recvlen= nn_recv(bet->subsock,&ptr,NN_MSG,0)) > 0 )
        {
            nonz++;
            if ( (msgjson= cJSON_Parse(ptr)) != 0 )
            {
                if ( BET_clientupdate(msgjson,ptr,recvlen,bet,VARS) < 0 )
                    printf("unknown clientupdate msg.(%s)\n",jprint(msgjson,0));
                free_json(msgjson);
            }
            nn_freemsg(ptr);
        }
        if ( nonz == 0 )
            usleep(10000);
    }
}

void BET_host_gamestart(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    cJSON *deckjson,*reqjson; char *retstr;
    reqjson = cJSON_CreateObject();
    jaddstr(reqjson,"method","start0");
    jaddnum(reqjson,"numplayers",bet->numplayers);
    jaddnum(reqjson,"numrounds",bet->numrounds);
    jaddnum(reqjson,"range",bet->range);
    BET_message_send("BET_start",bet->pubsock,reqjson,1);
    deckjson = 0;
    if ( (reqjson= BET_createdeck_json(bet->playerpubs,bet->numplayers,bet->range)) != 0 )
    {
        if ( (retstr= BET_command("createdeck",reqjson)) != 0 )
        {
            if ( (deckjson= cJSON_Parse(retstr)) != 0 )
            {
                printf("BET_roundstart numcards.%d numplayers.%d\n",bet->range,bet->numplayers);
                BET_roundstart(bet->pubsock,deckjson,bet->range,0,bet->playerpubs,bet->numplayers,Myprivkey);
                printf("finished BET_roundstart numcards.%d numplayers.%d\n",bet->range,bet->numplayers);
            }
            free(retstr);
        }
        free_json(reqjson);
    }
    printf("Gamestart.%u vs %u Numplayers.%d ",Gamestart,(uint32_t)time(NULL),bet->numplayers);
    printf("gamestart Range.%d numplayers.%d numrounds.%d\n",bet->range,bet->numplayers,bet->numrounds);
    BET_tablestatus_send(bet,vars);
    Lastturni = (uint32_t)time(NULL);
    vars->turni = 0;
    vars->round = 0;
    reqjson = cJSON_CreateObject();
    jaddstr(reqjson,"method","start");
    jaddnum(reqjson,"numplayers",bet->numplayers);
    jaddnum(reqjson,"numrounds",bet->numrounds);
    jaddnum(reqjson,"range",bet->range);
    BET_message_send("BET_start",bet->pubsock,reqjson,1);
}

void BET_hostloop(void *_ptr)
{
    uint32_t lasttime = 0; uint8_t r; int32_t nonz,recvlen,sendlen; cJSON *argjson,*timeoutjson; void *ptr; struct privatebet_info *bet = _ptr; struct privatebet_vars *VARS;
    VARS = calloc(1,sizeof(*VARS));
    printf("hostloop pubsock.%d pullsock.%d range.%d\n",bet->pubsock,bet->pullsock,bet->range);
    while ( bet->pullsock >= 0 && bet->pubsock >= 0 )
    {
        nonz = 0;
        if ( (recvlen= nn_recv(bet->pullsock,&ptr,NN_MSG,0)) > 0 )
        {
            nonz++;
            if ( (argjson= cJSON_Parse(ptr)) != 0 )
            {
                if ( BET_hostcommand(argjson,bet,VARS) != 0 )
                {
                    if ( (sendlen= nn_send(bet->pubsock,ptr,recvlen,0)) != recvlen )
                        printf("sendlen.%d != recvlen.%d for %s\n",sendlen,recvlen,jprint(argjson,0));
                }
                free_json(argjson);
            }
            nn_freemsg(ptr);
        }
        if ( nonz == 0 )
            usleep(100000);
        if ( Gamestarted == 0 )
        {
            //printf(">>>>>>>>> t%u gamestart.%u numplayers.%d turni.%d round.%d\n",(uint32_t)time(NULL),Gamestart,bet->numplayers,VARS.turni,VARS.round);
            if ( time(NULL) > Gamestart && bet->numplayers > 1 && VARS->turni == 0 && VARS->round == 0 )
            {
                Gamestarted = (uint32_t)time(NULL);
                OS_randombytes(&r,sizeof(r));
                bet->range = (r % (CARDS777_MAXCARDS-1)) + 2;
                BET_host_gamestart(bet,VARS);
            }
            else if ( (0) && time(NULL) > lasttime+10 )
            {
                BET_tablestatus_send(bet,VARS);
                lasttime = (uint32_t)time(NULL);
            }
        }
        else if ( (0) && time(NULL) > Lastturni+BET_PLAYERTIMEOUT )
        {
            timeoutjson = cJSON_CreateObject();
            jaddstr(timeoutjson,"method","turni");
            jaddnum(timeoutjson,"round",VARS->round);
            jaddnum(timeoutjson,"turni",VARS->turni);
            jaddbits256(timeoutjson,"pubkey",bet->playerpubs[VARS->turni]);
            jadd(timeoutjson,"actions",cJSON_Parse("[\"timeout\"]"));
            BET_message_send("TIMEOUT",bet->pubsock,timeoutjson,1);
            //BET_host_turni_next(bet,&VARS);
        }
    }
}

void BET_mainloop(int32_t pubsock,int32_t pullsock,int32_t subsock,int32_t pushsock,char *passphrase)
{
    bits256 privkey,pubkey; uint64_t randvals; uint8_t pubkey33[33],taddr=0,pubtype=60; char smartaddr[64],randphrase[32]; long fsize; struct privatebet_info *BET,*BET2;
    BET = calloc(1,sizeof(*BET));
    BET2 = calloc(1,sizeof(*BET2));
    BET_betinfo_set(BET,"demo",52,0,Maxplayers);
    BET->pubsock = pubsock;
    BET->pullsock = pullsock;
    BET->subsock = subsock;
    BET->pushsock = pushsock;
    *BET2 = *BET;
    if ( passphrase == 0 || passphrase[0] == 0 )
    {
        passphrase = OS_filestr(&fsize,"passphrase");
        if ( passphrase == 0 || passphrase[0] == 0 )
        {
            OS_randombytes((void *)&randvals,sizeof(randvals));
            sprintf(randphrase,"%llu",(long long)randvals);
            printf("randphrase.(%s)\n",randphrase);
            passphrase = randphrase;
        }
    }
    printf("passphrase.(%s) pushsock.%d subsock.%d\n",passphrase,pushsock,subsock);
    conv_NXTpassword(privkey.bytes,pubkey.bytes,(uint8_t *)passphrase,(int32_t)strlen(passphrase));
    bitcoin_priv2pub(bitcoin_ctx(),pubkey33,smartaddr,privkey,taddr,pubtype);
    Mypubkey = pubkey;
    Myprivkey = privkey;
    if ( BET->pubsock >= 0 && BET->pullsock >= 0 )
    {
        if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)BET_hostloop,(void *)BET) != 0 )
        {
            printf("error launching BET_hostloop for pub.%d pull.%d\n",BET->pubsock,BET->pullsock);
            exit(-1);
        }
        //Tableid = pubkey;
    }
    if ( BET->subsock >= 0 )
    {
        if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)BET_clientloop,(void *)BET) != 0 )
        {
            printf("error launching BET_clientloop for sub.%d\n",BET->subsock);
            exit(-1);
        }
    }
    BET_cmdloop(privkey,smartaddr,pubkey33,pubkey,BET2);
}


