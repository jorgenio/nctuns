/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#include <stdlib.h>
#include "mesh_route.h"

#include <nctuns_api.h>

#include "mac802_16_mesh_mode_sys_param.h"
#include "mac802_16_meshss.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"

#define __ENABLED__ACK_OBSERVATION 0

MODULE_GENERATOR(MeshRoute);

MeshRoute::MeshRoute(uint32_t type, uint32_t id, plist* pl, const char* name)
: NslObject(type, id, pl, name)
, path_setup_flag(0)
, local_ip(0)
, netmask(0)
, meshRtFilename(NULL)
, cwnd_observe_flag(0)
{

    vBind("MeshRoutingTableFilename", &meshRtFilename);

    REG_VAR("80216_MESHSS_ROUTE_MODULE", this);
}

MeshRoute::~MeshRoute()
{
}

int
MeshRoute::init()
{
    readConfig();

    mac802_16_MeshSS* mac;

    local_ip    = GET_REG_VAR1(get_portls(), "IP", uint32_t*);
    netmask     = GET_REG_VAR1(get_portls(), "NETMASK", uint32_t*);
    mac         = GET_REG_VAR1(get_portls(), "80216_MESHSS_MAC_MODULE", mac802_16_MeshSS*);

    assert(local_ip);
    assert(netmask);
    assert(mac);

    return 1;

}

int
MeshRoute::recv(Event* ep)
{
    Packet *Pkt;
    ClassifierRule *MatchRule = NULL;
    struct ip *ih;

    GET_PKT(Pkt, ep);

    //printf("%d MeshRoute::%s() len = %d\n", get_nid(), __FUNCTION__, Pkt->pkt_getlen());
    //printf("%d MeshRoute::%s() len = %d %llu \n", get_nid(), __FUNCTION__, Pkt->pkt_getlen(), GetCurrentTime());

    ih = (struct ip*)(Pkt->pkt_sget());

/*
    {
        char *buf=Pkt->pkt_sget();
        int l=Pkt->pkt_getlen();
        for(int z=0;z<l;z++)
        printf("%02x%c", buf[z]&0xff, (z+1)%32?' ':'\n');
    }
    fflush(stdout);
*/

/*
    struct ether_header *etherhdr;
    short               ether_type;

    etherhdr = (struct ether_header*)Pkt->pkt_get();
    ether_type = ntohs(etherhdr->ether_type);

    if( ether_type == ETHERTYPE_IP ){

        ih = (struct ip*)(Pkt->pkt_sget());
    }
    else if( ether_type == ETHERTYPE_ARP ) {
        printf("ARP Packet, but we should run Know In Advance Mode\n");
        exit(1);
    }
*/
    MatchRule = CrTable.Find(ih);

    if (MatchRule) {

        Pkt->pkt_addinfo("nextHop", (char*)&MatchRule->nextHop, sizeof(int));
        Pkt->pkt_setflow(PF_SEND);
        IP_DEC_TTL(Pkt->pkt_sget());
        TEST_FUNC((MatchRule->Dump()));
#if 0
        NSLOBJ_INFO("ip srt = ");INFO_FUNC(showip(ih->ip_src));
        INFO_FUNC(printf(", ip dst = "));INFO_FUNC(showip(ih->ip_dst));
        INFO_FUNC(printf(", nextHop = %d\n", MatchRule->nextHop));
#endif

        put(ep, sendtarget_);
    }
    else {

#if __ENABLED__ACK_OBSERVATION

        if (get_nid() == 1) {

            static u_int64_t ACK_timestamp = 0;
            static u_int32_t ACK_cnt = 0;

            struct tcphdr *tcphdr;

            tcphdr = reinterpret_cast<struct tcphdr*>(Pkt->pkt_sget() + (ih->ip_hl << 2));
            //printf("sport = %d\n", ntohs(tcphdr->source));
            //printf("dport = %d\n", ntohs(tcphdr->dest));

            printf("[%u]MeshRoute::recv(): Receive TCP seq=%u ack=%u (",
            get_nid(), ntohl(tcphdr->seq), ntohl(tcphdr->ack_seq));

            if( tcphdr->syn ) printf("SYN ");

            if( tcphdr->rst ) printf("RST ");

            if( tcphdr->ack ) {

                printf("ACK ");
                u_int64_t cur_ack_timestamp = GetCurrentTime();

                if ( cur_ack_timestamp == ACK_timestamp) {

                    ++ACK_cnt;

                }
                else {

                    printf("the size of previous ACK bulk is %d ",
                    ACK_cnt );

                    ACK_timestamp = cur_ack_timestamp;
                    ACK_cnt=0;

                }
            }

            printf(") @ %lld, pktlen=%d \n", GetCurrentTime(), Pkt->pkt_getlen());

            if (cwnd_observe_flag) {

                tcp_cwnd_estimator_ie.update_recv_seq(ntohl(tcphdr->seq));
                tcp_cwnd_estimator_ie.print_estimated_cwnd_size();

            }
        }
#endif

        put(ep, recvtarget_);

    }

    return 1;
}

int
MeshRoute::send(Event* ep)
{
    Packet *Pkt;
    ClassifierRule *MatchRule = NULL;
    struct ip *ih;

    GET_PKT(Pkt, ep);

    ih = (struct ip *) (Pkt->pkt_sget());

#if 0
    NSLOBJ_INFO("ip srt = ");INFO_FUNC(showip(ih->ip_src));
    INFO_FUNC(printf(", ip dst = "));INFO_FUNC(showip(ih->ip_dst));
    INFO_FUNC(printf("\n"));
#endif

    MatchRule = CrTable.Find(ih);

    if (MatchRule) {

#if __ENABLED__ACK_OBSERVATION

        if (get_nid() == 1) {

            static u_int64_t ACK_timestamp = 0;
            static u_int32_t ACK_cnt = 0;

            struct tcphdr *tcphdr;

            tcphdr = reinterpret_cast<struct tcphdr*>(Pkt->pkt_sget() + (ih->ip_hl << 2));
            //printf("sport = %d\n", ntohs(tcphdr->source));
            //printf("dport = %d\n", ntohs(tcphdr->dest));

            printf("[%u]MeshRoute::send(): Sending TCP seq=%u ack=%u (",
                get_nid(), ntohl(tcphdr->seq), ntohl(tcphdr->ack_seq));

            if (cwnd_observe_flag)
                tcp_cwnd_estimator_ie.update_send_seq(ntohl(tcphdr->seq));

            if( tcphdr->syn ) printf("SYN ");

            if( tcphdr->rst ) printf("RST ");

            if( tcphdr->ack ) {

                printf("ACK ");
                u_int64_t cur_ack_timestamp = GetCurrentTime();

                if ( cur_ack_timestamp == ACK_timestamp) {

                ++ACK_cnt;

                }
                else {

                printf("[%u]MeshRouteSend():: the size of previous ACK bulk is %d \n",
                get_nid(), ACK_cnt );

                ACK_timestamp = cur_ack_timestamp;

                }
            }

            printf(") @ %lld, pktlen=%d \n", GetCurrentTime(), Pkt->pkt_getlen());

        }

#endif

        Pkt->pkt_addinfo("nextHop", (char*)&MatchRule->nextHop, sizeof(int));
        put(ep, sendtarget_);
    }
    else {

        printf("ip src=");
        showip(ih->ip_src);
        printf("ip dst=");
        showip(ih->ip_dst);
        printf("\n");

        freePacket(ep);
        printf("MeshRoute::No suitable link was found...\n");

        if (path_setup_flag)
            assert(0);

    }

    return 1;
}

int MeshRoute::showip(ulong ip) {

    char* str= reinterpret_cast<char*> (&ip);

    for ( u_int32_t i=0 ; i<4 ; ++i)
        printf(" %u ", static_cast<u_int32_t> (str[i]) );

    return 1;

}

uint32_t
MeshRoute::query_route(in_addr_t dest_ip_net_order)
{
    ClassifierRule* rule = CrTable.Find(dest_ip_net_order);

    NSLOBJ_DEBUG("find rule as follows:\n");

    if (rule) {

        DEBUG_FUNC((rule->Dump()));
        return rule->nextHop;

    }
    else {

        struct in_addr in_addr_s;
        in_addr_s.s_addr = dest_ip_net_order;
        NSLOBJ_ERROR("query_ip = %s.\n", inet_ntoa(in_addr_s));
        CrTable.Dump();
        NSLOBJ_ASSERT(0, "cannot find rules for paths to BS nodes.\n");

    }

    return 0;

}

int
MeshRoute::change_route(in_addr_t dest_ip_net_order, uint32_t nexthop_nid)
{
    NSLOBJ_ASSERT((nexthop_nid!=get_nid()), "detect nexthop is myself.\n");

    ClassifierRule* rule = CrTable.Find(dest_ip_net_order);

    NSLOBJ_DEBUG(("find rule as follows:\n"));

    if (rule) {

        DEBUG_FUNC((rule->Dump()));
        u_int32_t old_nexthop_nid = rule->nextHop;
        rule->nextHop = nexthop_nid;
        NSLOBJ_INFO("change next_hop from node %u to node %u.\n", old_nexthop_nid, nexthop_nid);

    }
    else {

        NSLOBJ_ASSERT(0, "cannot find rules for paths to BS nodes.\n");

    }

    return 1;

}

void MeshRoute::readConfig()
{
    char fn[256];
    FILE *fp = NULL;
    char buffer[1024];
    u_int nid, mask, nextHop;
    char ip[4];
    int in[4];
    ClassifierRule *rule;

    char* config_file_dir = GetConfigFileDir();

    snprintf(fn, sizeof(fn), "%s%s", config_file_dir, meshRtFilename);

    NSLOBJ_VINFO("GetScriptName=%s\n", GetScriptName());
    NSLOBJ_VINFO("GetConfigFileDir=%s\n", config_file_dir);
    NSLOBJ_VINFO("meshRtFilename=%s\n", meshRtFilename);
    NSLOBJ_VINFO("fn=%s\n", fn);

    //free(config_file_dir);

    if ((fp = fopen(fn, "r"))) {

        while (!feof(fp) && fgets(buffer, sizeof(buffer), fp)) {
            if (buffer[0] == '#') {
                continue;
            }

            sscanf(buffer, "$node_(%d) %d.%d.%d.%d/%d %d\n",
                &nid, &in[0], &in[1], &in[2], &in[3], &mask,
                &nextHop);

            ip[0] = in[0];
            ip[1] = in[1];
            ip[2] = in[2];
            ip[3] = in[3];

            if (nid != get_nid()) {
                continue;
            }

            rule = new ClassifierRule;
            rule->nextHop = nextHop;
            rule->f.InetDstAddr.s_addr =
                htonl(ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3]);

            rule->f.InetDstMask.s_addr = htonl(0xffffffff ^ ((1 << (32 - mask)) - 1));

            NSLOBJ_TEST(("Add rule as follows:\n"));
            TEST_FUNC((rule->Dump()));

            CrTable.AddItem(rule);
        }

        fclose(fp);
        NSLOBJ_VINFO("finish parsing meshrt file (num of added rules: %u).\n", CrTable.get_table_size());
    }
    else {

        NSLOBJ_FATAL("cannot open .meshrt file.\n");

    }
}


/**** TCP Congestion Window Estimator */

TcpCongestionWindowEstimator::TcpCongestionWindowEstimator() {

    tcp_cur_send_seq = 0;
    tcp_cur_recv_seq = 0;

}

int TcpCongestionWindowEstimator::update_send_seq(u_int32_t new_seqno) {

    if (tcp_cur_send_seq < new_seqno)
        tcp_cur_send_seq = new_seqno;

    return 1;

}

int TcpCongestionWindowEstimator::update_recv_seq(u_int32_t new_seqno) {

    if (tcp_cur_recv_seq < new_seqno)
        tcp_cur_recv_seq = new_seqno;

    return 1;

}

int TcpCongestionWindowEstimator::print_estimated_cwnd_size() {

    u_int32_t estimated_cwnd_size =
        tcp_cur_send_seq - tcp_cur_recv_seq;

    printf
        ("[%u]TcpCongestionWindowEstimator: Estimated TCP Cwnd Size = %u at %llu \n",
        1, estimated_cwnd_size, GetCurrentTime());

    return 1;

}

