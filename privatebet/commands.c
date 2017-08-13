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

char *chipsln_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    int32_t n,numargs,maxsize = 1000000; char *args[16],*cmdstr,*buffer = malloc(maxsize);
    numargs = 0;
    args[numargs++] = "chipsln";
    args[numargs++] = jstr(argjson,"method");
    args[numargs] = 0;
    cmdstr = jprint(argjson,0);
    cli_main(buffer,maxsize,numargs,args,cmdstr);
    free(cmdstr);
    n = (int32_t)strlen(buffer);
    if ( buffer[n-1] == '\n' )
        buffer[n-1] = 0;
    buffer = realloc(buffer,n+1);
    return(buffer);
}

cJSON *chipsln_issue(char *buf)
{
    char *retstr; cJSON *retjson,*argjson;
    argjson = cJSON_Parse(buf);
    if ( (retstr= chipsln_command(0,argjson,"127.0.0.1",0)) != 0 )
    {
        retjson = cJSON_Parse(retstr);
        free(retstr);
    }
    free_json(argjson);
    return(retjson);
}

cJSON *chipsln_noargs(char *method)
{
    char buf[1024];
    sprintf(buf,"{\"method\":\"%s\"}",method);
    return(chipsln_issue(buf));
}

cJSON *chipsln_strarg(char *method,char *str)
{
    char buf[4096];
    sprintf(buf,"{\"method\":\"%s\",\"params\":[\"%s\"]}",method,str);
    return(chipsln_issue(buf));
}

cJSON *chipsln_strnum(char *method,char *str,uint64_t num)
{
    char buf[4096];
    sprintf(buf,"{\"method\":\"%s\",\"params\":[\"%s\", %llu]}",method,str,(long long)num);
    return(chipsln_issue(buf));
}

cJSON *chipsln_numstr(char *method,char *str,uint64_t num)
{
    char buf[4096];
    sprintf(buf,"{\"method\":\"%s\",\"params\":[%llu, \"%s\"]}",method,(long long)num,str);
    return(chipsln_issue(buf));
}

cJSON *chipsln_getinfo() { return(chipsln_noargs("getinfo")); }
cJSON *chipsln_help() { return(chipsln_noargs("help")); }
cJSON *chipsln_stop() { return(chipsln_noargs("stop")); }
cJSON *chipsln_newaddr() { return(chipsln_noargs("newaddr")); }
cJSON *chipsln_getnodes() { return(chipsln_noargs("getnodes")); }
cJSON *chipsln_getpeers() { return(chipsln_noargs("getpeers")); }
cJSON *chipsln_getchannels() { return(chipsln_noargs("getchannels")); }
cJSON *chipsln_devblockheight() { return(chipsln_noargs("dev-blockheight")); }

cJSON *chipsln_listinvoice(char *label) { return(chipsln_strarg("listinvoice",label)); }
cJSON *chipsln_delinvoice(char *label) { return(chipsln_strarg("delinvoice",label)); }
cJSON *chipsln_waitanyinvoice(char *label) { return(chipsln_strarg("waitanyinvoice",label)); }
cJSON *chipsln_waitinvoice(char *label) { return(chipsln_strarg("waitinvoice",label)); }

cJSON *chipsln_getlog(char *level) { return(chipsln_strarg("getlog",level)); }
cJSON *chipsln_close(char *idstr) { return(chipsln_strarg("close",idstr)); }
cJSON *chipsln_devrhash(char *secret) { return(chipsln_strarg("dev-rhash",secret)); }
cJSON *chipsln_addfunds(char *rawtx) { return(chipsln_strarg("addfunds",rawtx)); }

cJSON *chipsln_getroute(char *idstr,uint64_t msatoshi)
{
    return(chipsln_strnum("getroute",idstr,msatoshi));
}

cJSON *chipsln_fundchannel(char *idstr,uint64_t satoshi)
{
    return(chipsln_strnum("fundchannel",idstr,satoshi));
}

cJSON *chipsln_invoice(uint64_t msatoshi,char *label)
{
    return(chipsln_numstr("invoice",msatoshi,label));
}

cJSON *chipsln_withdraw(uint64_t satoshi,char *address)
{
    return(chipsln_numstr("withdraw",satoshi,address));
}

cJSON *chipsln_connect(char *ipaddr,uint16_t port,char *destid)
{
    char buf[4096];
    sprintf(buf,"{\"method\":\"connect\",\"params\":[\"%s\", %u, \"%s\"]}",ipaddr,port,destid);
    return(chipsln_issue(buf));
}

cJSON *chipsln_sendpay(cJSON *routejson,bits256 rhash)
{
    char buf[16384];
    sprintf(buf,"{\"method\":\"sendpay\",\"params\":[%s, \"%s\"]}",jprint(routejson,0),bits256_str(str,rhash));
    return(chipsln_issue(buf));
}

char *privatebet_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *method;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        if ( strcmp(method,"createdeck") == 0 )
            return(BET_createdeck(argjson));
        else return(clonestr("{\"error\":\"missing method\"}"));
    } else return(clonestr("{\"error\":\"missing method\"}"));
}

char *pangea_command(void *ctx,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *method;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        return(clonestr("{\"result\":\"success\", \"agent\":\"pangea\"}"));
    } else return(clonestr("{\"error\":\"missing method\"}"));
}

char *stats_JSON(void *ctx,char *myipaddr,int32_t mypubsock,cJSON *argjson,char *remoteaddr,uint16_t port)
{
    char *agent;
    agent = jstr(argjson,"agent");
    if ( (agent= jstr(argjson,"agent")) != 0 )
    {
        if ( strcmp(agent,"bet") == 0 )
            return(privatebet_command(ctx,argjson,remoteaddr,port));
        else if ( strcmp(agent,"chipsln") == 0 )
            return(chipsln_command(ctx,argjson,remoteaddr,port));
        else if ( strcmp(agent,"pangea") == 0 )
            return(pangea_command(ctx,argjson,remoteaddr,port));
        else return(clonestr("{\"error\":\"invalid agent\"}"));
    }
    printf("stats_JSON.(%s)\n",jprint(argjson,0));
    return(clonestr("{\"result\":\"success\"}"));
}

char *BET_command(char *method,cJSON *reqjson)
{
    static int32_t maxlen;
    char *retstr,*params; int32_t n;
    params = jprint(reqjson,0);
    if ( (retstr= bitcoind_passthrut("bet","127.0.0.1:7797","",method,params,30)) != 0 )
    {
        if ( (n= (int32_t)strlen(retstr)) > 0 && retstr[n-1] == '\n' )
            retstr[n-1] = 0;
        if ( n > maxlen )
            maxlen = n;
        printf("%s %s -> (%d) max.%d\n",method,params,n,maxlen);
    } else printf("null return from %s %s\n",method,jprint(reqjson,0));
    free(params);
    return(retstr);
}

cJSON *BET_createdeck_json(bits256 *pubkeys,int32_t numplayers,int32_t range)
{
    int32_t i; cJSON *pubs,*reqjson;
    pubs = cJSON_CreateArray();
    for (i=0; i<numplayers; i++)
        jaddibits256(pubs,pubkeys[i]);
    reqjson = cJSON_CreateObject();
    jaddstr(reqjson,"agent","bet");
    jaddstr(reqjson,"method","createdeck");
    jaddnum(reqjson,"range",range);
    jadd(reqjson,"pubkeys",pubs);
    return(reqjson);
}

void BET_shardkey(uint8_t *key,bits256 deckid,int32_t cardi,int32_t playerj)
{
    memcpy(key,deckid.bytes,sizeof(deckid));
    memcpy(&key[sizeof(deckid)],&cardi,sizeof(cardi));
    memcpy(&key[sizeof(deckid)+sizeof(cardi)],&playerj,sizeof(playerj));
}

struct BET_shardsinfo *BET_shardsfind(bits256 deckid,int32_t cardi,int32_t playerj)
{
    uint8_t key[sizeof(bits256) + sizeof(cardi) + sizeof(playerj)]; struct BET_shardsinfo *shards;
    BET_shardkey(key,deckid,cardi,playerj);
    portable_mutex_lock(&BET_shardmutex);
    HASH_FIND(hh,BET_shardsinfos,key,sizeof(key),shards);
    portable_mutex_unlock(&BET_shardmutex);
    return(shards);
}

void BET_MofN_item(bits256 deckid,int32_t cardi,bits256 *cardpubs,int32_t numcards,int32_t playerj,int32_t numplayers,bits256 shard,int32_t shardi)
{
    void *G; struct BET_shardsinfo *shards; int32_t i,m; uint8_t sharenrs[255],recovernrs[255],space[4096];
    if ( (shards= BET_shardsfind(deckid,cardi,playerj)) == 0 )
    {
        //printf("create new (c%d p%d)\n",cardi,playerj);
        shards = calloc(1,sizeof(*shards) + (numplayers * sizeof(bits256)));
        BET_shardkey(shards->key,deckid,cardi,playerj);
        shards->numcards = numcards;
        shards->numplayers = numplayers;
        portable_mutex_lock(&BET_shardmutex);
        HASH_ADD_KEYPTR(hh,BET_shardsinfos,shards->key,sizeof(shards->key),shards);
        portable_mutex_unlock(&BET_shardmutex);
    } //else printf("extend new (c%d p%d)\n",cardi,playerj);
    if ( shards != 0 )
    {
        shards->data[shardi] = shard;
        for (i=m=0; i<shards->numplayers; i++)
            if ( bits256_nonz(shards->data[i]) != 0 )
                m++;
        if ( m >= (shards->numplayers >> 1) + 1 )
        {
            //printf("got m.%d numplayers.%d M.%d\n",m,shards->numplayers,(shards->numplayers >> 1) + 1);
            gfshare_calc_sharenrs(sharenrs,numplayers,deckid.bytes,sizeof(deckid));
            //for (i=0; i<numplayers; i++)
            //    printf("%d ",sharenrs[i]);
            //char str[65]; printf("recover calc_sharenrs deckid.%s\n",bits256_str(str,deckid));
            memset(recovernrs,0,sizeof(recovernrs));
            for (i=0; i<shards->numplayers; i++)
                if ( bits256_nonz(shards->data[i]) != 0 )
                    recovernrs[i] = sharenrs[i];
            G = gfshare_initdec(recovernrs,shards->numplayers,sizeof(shards->recover),space,sizeof(space));
            for (i=0; i<shards->numplayers; i++)
                if ( bits256_nonz(shards->data[i]) != 0 )
                    gfshare_dec_giveshare(G,i,shards->data[i].bytes);
            gfshare_dec_newshares(G,recovernrs);
            gfshare_decextract(0,0,G,shards->recover.bytes);
            gfshare_free(G);
            shards->recover = cards777_cardpriv(Myprivkey,cardpubs,numcards,shards->recover);
            char str2[65]; printf("recovered (c%d p%d).%d %s [%d] range.%d\n",cardi,playerj,shardi,bits256_str(str2,shards->recover),shards->recover.bytes[1],numcards);
            if ( shards->recover.bytes[1] >= numcards )
                exit(-1);
        }
    }
}

void BET_betinfo_set(struct privatebet_info *bet,char *game,int32_t range,int32_t numrounds,int32_t maxplayers)
{
    safecopy(bet->game,game,sizeof(bet->game));
    bet->range = range;
    bet->numrounds = numrounds;
    bet->maxplayers = maxplayers;
}

void BET_betvars_parse(struct privatebet_info *bet,struct privatebet_vars *vars,cJSON *argjson)
{
    vars->turni = jint(argjson,"turni");
    vars->round = jint(argjson,"round");
    if ( bits256_cmp(bet->tableid,Mypubkey) != 0 )
    {
        Gamestart = juint(argjson,"gamestart");
        Gamestarted = juint(argjson,"gamestarted");
    }
    //printf("TURNI.(%s)\n",jprint(argjson,0));
}

cJSON *BET_betinfo_json(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    int32_t i,n; cJSON *array,*betjson = cJSON_CreateObject();
    jaddstr(betjson,"method","tablestatus");
    jaddstr(betjson,"game",bet->game);
    jaddbits256(betjson,"tableid",bet->tableid);
    jaddnum(betjson,"maxplayers",bet->maxplayers);
    array = cJSON_CreateArray();
    for (i=n=0; i<bet->maxplayers; i++)
    {
        if ( bits256_nonz(bet->playerpubs[i]) != 0 )
        {
            n++;
            jaddibits256(array,bet->playerpubs[i]);
            if ( bits256_cmp(bet->playerpubs[i],Mypubkey) == 0 )
                bet->myplayerid = i;
        }
    }
    bet->numplayers = n;
    jadd(betjson,"players",array);
    jaddnum(betjson,"numplayers",n);
    if ( bet->range == 0 )
        bet->range = (52 % CARDS777_MAXCARDS) + 2;
    if ( bet->numrounds == 0 )
        bet->numrounds = Maxrounds;
    jaddnum(betjson,"range",bet->range);
    jaddnum(betjson,"numrounds",bet->numrounds);
    jaddnum(betjson,"gamestart",Gamestart);
    //jaddnum(betjson,"gamestarted",vars->gamestarted);
    if ( Gamestarted != 0 )
    {
        jaddnum(betjson,"round",vars->round);
        jaddnum(betjson,"turni",vars->turni);
    }
    else if ( Gamestart != 0 )
    {
        jaddnum(betjson,"timestamp",time(NULL));
        jaddnum(betjson,"countdown",Gamestart - time(NULL));
    }
    return(betjson);
}

int32_t BET_betinfo_parse(struct privatebet_info *bet,struct privatebet_vars *vars,cJSON *msgjson)
{
    int32_t i,n; cJSON *players;
    memset(vars,0,sizeof(*vars));
    if ( (players= jarray(&n,msgjson,"players")) != 0 && n > 0 )
    {
        bet->myplayerid = -1;
        for (i=0; i<n; i++)
        {
            bet->playerpubs[i] = jbits256i(players,i);
            if ( bits256_cmp(bet->playerpubs[i],Mypubkey) == 0 )
                bet->myplayerid = i;
        }
    }
    if ( jstr(msgjson,"game") != 0 )
        safecopy(bet->game,jstr(msgjson,"game"),sizeof(bet->game));
    bet->maxplayers = jint(msgjson,"maxplayers");
    if ( (bet->numplayers= jint(msgjson,"numplayers")) != n )
    {
        bet->numplayers = 0;
        printf("Numplayers %d mismatch %d\n",bet->numplayers,n);
    }
    bet->tableid = jbits256(msgjson,"tableid");
    bet->numrounds = jint(msgjson,"numrounds");
    bet->range = jint(msgjson,"range");
    if ( (bet->numrounds= jint(msgjson,"numrounds")) == 0 )
        bet->numrounds = Maxrounds;
    BET_betvars_parse(bet,vars,msgjson);
    return(bet->numplayers);
}

void BET_tablestatus_send(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    cJSON *tablejson;
    tablejson = BET_betinfo_json(bet,vars);
    BET_message_send("BET_tablestatus_send",bet->pubsock>=0?bet->pubsock:bet->pushsock,tablejson,1);
}

void BET_status_disp(struct privatebet_info *bet,struct privatebet_vars *vars)
{
    char str[65];
    printf("%s: mypubkey.(%s) playerid.%d numplayers.%d max.%d numrounds.%d round.%d\n",bet->game,bits256_str(str,Mypubkey),bet->myplayerid,bet->numplayers,bet->maxplayers,bet->numrounds,vars->round);
}

int32_t BET_pubkeyfind(struct privatebet_info *bet,bits256 pubkey)
{
    int32_t i;
    //char str[65]; printf("PUBKEY FIND.(%s)\n",bits256_str(str,pubkey));
    for (i=0; i<sizeof(bet->playerpubs)/sizeof(*bet->playerpubs); i++)
        if ( bits256_cmp(pubkey,bet->playerpubs[i]) == 0 )
            return(i);
    return(-1);
}

int32_t BET_pubkeyadd(struct privatebet_info *bet,bits256 pubkey)
{
    int32_t i,n = 0;
    //char str[65]; printf("PUBKEY ADD.(%s) maxplayers.%d\n",bits256_str(str,pubkey),bet->maxplayers);
    for (i=0; i<bet->maxplayers; i++)
    {
        if ( bits256_cmp(bet->tableid,Mypubkey) == 0 && i >= bet->maxplayers )
            break;
        if ( bits256_nonz(bet->playerpubs[i]) == 0 )
        {
            bet->playerpubs[i] = pubkey;
            for (i=0; i<sizeof(bet->playerpubs)/sizeof(*bet->playerpubs); i++)
                if ( bits256_nonz(bet->playerpubs[i]) != 0 )
                {
                    n++;
                    bet->numplayers = n;
                }
            return(n);
        }
        //char str[65]; printf("%s\n",bits256_str(str,bet->playerpubs[i]));
    }
    return(0);
}

int32_t BET_host_join(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars)
{
    bits256 pubkey; int32_t n;
    pubkey = jbits256(argjson,"pubkey");
    if ( bits256_nonz(pubkey) != 0 )
    {
        //printf("JOIN.(%s)\n",jprint(argjson,0));
        if ( bits256_nonz(bet->tableid) == 0 )
            bet->tableid = Mypubkey;
        if ( BET_pubkeyfind(bet,pubkey) < 0 )
        {
            if ( (n= BET_pubkeyadd(bet,pubkey)) > 0 )
            {
                if ( n > 1 )
                {
                    Gamestart = (uint32_t)time(NULL);
                    if ( n < bet->maxplayers )
                        Gamestart += BET_GAMESTART_DELAY;
                    printf("Gamestart in a %d seconds\n",BET_GAMESTART_DELAY);
                } else printf("Gamestart after second player joins or we get maxplayers.%d\n",bet->maxplayers);
                return(1);
            } else return(-2);
        }
    }
    return(-1);
}

int32_t BET_hostcommand(cJSON *argjson,struct privatebet_info *bet,struct privatebet_vars *vars)
{
    char *method;
    if ( (method= jstr(argjson,"method")) != 0 )
    {
        if ( strcmp(method,"join") == 0 )
            return(BET_host_join(argjson,bet,vars));
        else if ( strcmp(method,"tablestatus") == 0 )
            return(0);
        else return(1);
    }
    return(-1);
}

int32_t BET_havetable(bits256 pubkey,uint8_t *pubkey33,struct privatebet_info *bet)
{
    cJSON *reqjson;
    if ( bits256_nonz(bet->tableid) != 0 )
        return(1);
    else
    {
        reqjson = cJSON_CreateObject();
        jaddbits256(reqjson,"pubkey",pubkey);
        jaddstr(reqjson,"method","join");
        BET_message_send("BET_havetable",bet->pushsock,reqjson,1);
        return(-1);
    }
}

void BET_cmdloop(bits256 privkey,char *smartaddr,uint8_t *pubkey33,bits256 pubkey,struct privatebet_info *bet)
{
    while ( BET_havetable(pubkey,pubkey33,bet) < 0 )
        sleep(10);
    while ( 1 )
    {
        //BET_status_disp();
        sleep(10);
    }
}
