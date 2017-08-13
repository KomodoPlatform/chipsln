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


#include "bet.h"

int32_t Gamestart,Gamestarted,Lastturni,Maxrounds = 3,Maxplayers = 2;
uint8_t BET_logs[256],BET_exps[510];
bits256 *Debug_privkeys;
struct BET_shardsinfo *BET_shardsinfos;
portable_mutex_t LP_peermutex,LP_commandmutex,LP_networkmutex,LP_psockmutex,LP_messagemutex,BET_shardmutex;
int32_t LP_canbind,IAMLP;
struct LP_peerinfo  *LP_peerinfos,*LP_mypeer;
bits256 Mypubkey,Myprivkey;

#include "gfshare.c"
#include "cards777.c"
#include "commands.c"
#include "gameloop.c"
#include "network.c"
#include "states.c"

int cli_main(char *buffer,int32_t maxsize,int argc, char *argv[]);

// original shuffle with player 2 encrypting to destplayer
// autodisconnect
// payments/bets
// virtualize games
// privatebet host -> publish to BET chain
// tableid management -> leave, select game, start game

int main(int argc,const char *argv[])
{
    char connectaddr[128],bindaddr[128],*modestr,*hostip="127.0.0.1",*retstr; cJSON *argjson,*reqjson,*deckjson; bits256 pubkeys[64],privkeys[64]; uint32_t i,n,range,numplayers; int32_t testmode=0,pubsock=-1,subsock=-1,pullsock=-1,pushsock=-1; long fsize; uint16_t tmp,rpcport=7797,port = 7797+1;
    libgfshare_init();
    OS_init();
    portable_mutex_init(&LP_peermutex);
    portable_mutex_init(&LP_commandmutex);
    portable_mutex_init(&LP_networkmutex);
    portable_mutex_init(&LP_psockmutex);
    portable_mutex_init(&LP_messagemutex);
    portable_mutex_init(&BET_shardmutex);
    if ( argc > 1 )
    {
        if ( (argjson= cJSON_Parse(argv[1])) != 0 )
        {
            hostip = jstr(argjson,"hostip");
            if ( (tmp= juint(argjson,"hostport")) != 0 )
                port = tmp;
            if ( (tmp= juint(argjson,"rpcport")) != 0 )
                rpcport = tmp;
            if ( (modestr= jstr(argjson,"mode")) != 0 )
            {
                if ( strcmp(modestr,"host") == 0 )
                {
                    if ( hostip == 0 && system("curl -s4 checkip.amazonaws.com > /tmp/myipaddr") == 0 )
                    {
                        if ( (hostip= OS_filestr(&fsize,"/tmp/myipaddr")) != 0 && hostip[0] != 0 )
                        {
                            n = (int32_t)strlen(hostip);
                            if ( hostip[n-1] == '\n' )
                                hostip[--n] = 0;
                        } else printf("error getting myipaddr\n");
                    }
                    BET_transportname(1,bindaddr,hostip,port);
                    pubsock = BET_nanosock(1,bindaddr,NN_PUB);
                    BET_transportname(1,bindaddr,hostip,port+1);
                    pullsock = BET_nanosock(1,bindaddr,NN_PULL);
                    if ( OS_thread_create(malloc(sizeof(pthread_t)),NULL,(void *)stats_rpcloop,(void *)&rpcport) != 0 )
                    {
                        printf("error launching stats rpcloop for port.%u\n",port);
                        exit(-1);
                    }
                }
            }
            if ( hostip == 0 || hostip[0] == 0 )
                hostip = "127.0.0.1";
            BET_transportname(0,connectaddr,hostip,port);
            printf("connect %s\n",connectaddr);
            subsock = BET_nanosock(0,connectaddr,NN_SUB);
            BET_transportname(0,connectaddr,hostip,port+1);
            pushsock = BET_nanosock(0,connectaddr,NN_PUSH);
            sleep(1);
            printf("BET API running on %s:%u pub.%d sub.%d; pull.%d push.%d ipbits.%08x\n",hostip,port,pubsock,subsock,pullsock,pushsock,(uint32_t)calc_ipbits("5.9.102.210"));
            BET_mainloop(pubsock,pullsock,subsock,pushsock,jstr(argjson,"passphrase"));
        }
    }
    else
    {
        printf("no argjson, default to testmode\n");
        while ( testmode != 0 )
        {
            OS_randombytes((uint8_t *)&range,sizeof(range));
            OS_randombytes((uint8_t *)&numplayers,sizeof(numplayers));
            range = (range % CARDS777_MAXCARDS) + 1;
            numplayers = (numplayers % (CARDS777_MAXPLAYERS-1)) + 2;
            for (i=0; i<numplayers; i++)
                privkeys[i] = curve25519_keypair(&pubkeys[i]);
            //Debug_privkeys = privkeys;
            deckjson = 0;
            if ( (reqjson= BET_createdeck_json(pubkeys,numplayers,range)) != 0 )
            {
                if ( (retstr= BET_command("createdeck",reqjson)) != 0 )
                {
                    if ( (deckjson= cJSON_Parse(retstr)) != 0 )
                    {
                        printf("BET_roundstart numcards.%d numplayers.%d\n",range,numplayers);
                        BET_roundstart(-1,deckjson,range,privkeys,pubkeys,numplayers,privkeys[0]);
                        printf("finished BET_roundstart numcards.%d numplayers.%d\n",range,numplayers);
                    }
                    free(retstr);
                }
                free_json(reqjson);
            }
            if ( deckjson != 0 )
                free_json(deckjson);
            {
                int32_t permi[CARDS777_MAXCARDS],permis[CARDS777_MAXPLAYERS][CARDS777_MAXCARDS];
                memset(permi,0,sizeof(permi));
                memset(permis,0,sizeof(permis));
                for (i=0; i<numplayers; i++)
                    BET_permutation(permis[i],range);
                BET_permutation_sort(permi,permis,numplayers,range);
            }
        }
    }
    sleep(1);
    return 0;
}

