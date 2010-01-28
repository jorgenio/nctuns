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

#ifndef __NCTUNS_MeshArp_h__
#define __NCTUNS_MeshArp_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#include "meshlib/neighbor.h"
#include "meshlib/ospf.h"
#include "meshospf.h"

#define ARP_TBL_MAX_ENTRYS	256

/* MacPort_MappingTable is added by Chih-che Lin on 08/28/2005 
 * to make packets that enters and leaves a multi-gateway
 * mesh system through correct APs.
 */ 

#define NO_PORTINFO_VALUE   100000

#define ASSERTION(exp, str) do { \
    if ( !(exp) ) {   \
        printf("<Assertion failed>" );  \
        printf("%s", (str) );   \
        exit(1);    \
    }\
} while(0)

#define IS_NULL_STR(ep, str, ret_val) do {      \
        if ( !(ep) ) {                          \
                printf("%s\n",(str));           \
                return (ret_val);               \
            }                                   \
    }while(0)

#define IS_NULL(ep, ret_val) do {     \
            if ( !(ep) ) {            \
                return (ret_val);     \
            }                         \
    }while(0)



typedef u_int32_t ip_type ;
const u_int32_t mac_address_size = 6;

enum status_t { INVALID = 0 , VALID = 1 , NO_PORTINFO = NO_PORTINFO_VALUE}; 


typedef class ether_mac_addr_type {
    public:

    
    char addr[mac_address_size];

    ether_mac_addr_type();
    int show();

} ether_maddr_t;


typedef class MappingTableEntry {

     private:
 
     status_t       status_;
     ether_maddr_t  src_mac_;
     ether_maddr_t  dst_mac_;
     MBinder*       inport_mbinder_;
     MBinder*       outport_mbinder_;
     u_int64_t      timeout_;
     MappingTableEntry* prev;
     MappingTableEntry* next;
 
     public:
 
     MappingTableEntry();
     MappingTableEntry( ether_maddr_t* src_mac, ether_maddr_t*dst_mac, MBinder* inport_mbinder, MBinder* outport_mbinder);

     int update( ether_maddr_t* src_mac, ether_maddr_t*dst_mac, MBinder* inport_mbinder, MBinder* outport_mbinder );
 
     status_t get_status();
     
     ether_maddr_t* get_src_mac();
     ether_maddr_t* get_dst_mac();

     MappingTableEntry* get_next();
     MappingTableEntry* get_prev();
     
     int set_next(MappingTableEntry* ptr);
     int set_prev(MappingTableEntry* ptr);
   
     MBinder* get_inport_mbinder();
     MBinder* get_outport_mbinder();

     int  set_inport_mbinder(MBinder* new_inport_mbinder);
     int  set_outport_mbinder(MBinder* new_outport_mbinder);
    

} MTEntry;

typedef class MacPortMappingTable {

     private:
     MTEntry*  head;
     MTEntry*  tail;
     u_int32_t item_num; /* for debugging */
     public:
     MacPortMappingTable();
     MBinder*     get_inport_mbinder(ether_maddr_t* dst_mac);
     MBinder*     get_outport_mbinder(ether_maddr_t* dst_mac);
     status_t     get_status_by_src_mac(ether_maddr_t* src_mac);
     MTEntry*     get_entry_by_src_mac(ether_maddr_t* src_mac);
     
     int insert(ether_maddr_t* src_mac , ether_maddr_t* dst_mac , MBinder* inport_mbinder, MBinder* outport_mbinder);
     int del(ether_maddr_t* src_mac);
     
     

} MacMapTable;


typedef struct mapTbl {
	u_long		ip;
	u_char		mac[6];
	u_int64_t	timestamp;
	ePacket_	*pkt;
	struct mapTbl	*next;
} mapTbl;

typedef struct arpTbl {
	mapTbl		*head, *tail;
	int		entrys;
} arpTbl;

class IpMacEntry {
 
    public:
    u_long  ip;
    char*   ip_str;
    u_char* mac;

};


class MeshArp: public NslObject {

 private:
	u_char*			mac_;
 	u_long*			ip_;
        u_long                  node_ip_;
        char*                   ip_str;
        u_char			router_mac[9]; 
        struct in_addr		ip_addr;
	arpTbl*			arpTable;
	timerObj		flushTimer;
        MacPortMappingTable*    mac_port_map_table;

	char			*ARP_MODE;//know_in_advance or run_arp
	int			flushInterval;	// flush interval(unit:ms)
	u_int64_t		flushInterval_; // flush interval(unit:tick)
	char			*fileName;
	struct mesh_if		*ifaces[32];
	int			ifaces_num;
        char			ospf_port_flag[32]; 

	list			neighbors;
        IpMacEntry*		ipmac_table;
        int			iptable_max_index;    
        timerObj*		hello_timer; /* hello_timer is used to control the frequency of sending
                                              * hello messages to neighbor OSPF module.
                                              */

        int select_mesh_port();

 public:
 	MeshArp(u_int32_t type, u_int32_t id, struct plist * pl, const char *name); 
 	~MeshArp();   

	int 			init(); 
	int                     recv(ePacket_ *pkt);
	int                     send(ePacket_ *pkt,MBinder* mb);
	int 			send_fixed_port(ePacket_ *pkt, MBinder* mb);
        int                     get(ePacket_ *pkt, MBinder*);
	int 			command(int argc, const char *argv[]); 
	
	inline u_long		getDstIp(ePacket_ *pkt);
	u_char			*findArpTbl(u_long ipDst, int &recordExistButNoMac);
	int			addArpTbl(u_long ip, u_char *mac, ePacket_ *pkt, arpTbl *arpTable);
	int			delArpTbl(mapTbl *lastEntry, mapTbl *delEntry);
        int                     flushArpTbl();
	int			atchMacHdr(ePacket_ *pkt, u_char* macSrc , u_char *macDst, u_short frameType);
	int			updatePktBuf(u_long ipDst, ePacket_ *pkt);
	int			arpRequest(u_long ipDst, MBinder* mb);
	int			pktIsArp(ePacket_ *pkt);
	int			freeArpUpdate(ePacket_ *pkt);
	int			iAmTpa(ePacket_ *pkt);
	int			freeArpLearning(ePacket_ *pkt);
	u_short			getArpOp(ePacket_ *pkt);
	int			arpReply(ePacket_ *pkt, MBinder* mb);
	int			resumeSend(ePacket_ *pkt, MBinder* mb);
	
	int			dumpArpTable();
	int			parseLine(char *line, char *ip, char *mac);
	int			StrToIP(char *str, u_long &ip);

        int 			update_src_mac_mbinder_mapping(ether_maddr_t* src_ip , MBinder* mb_p);
        MBinder* 		find_inport_mbinder(ether_maddr_t* dst_ip);
        int 			getSrcDstMac(ether_maddr_t* src, ether_maddr_t* dst, ePacket_ *pkt);
        int 			getSrcMac(ether_maddr_t* src, ePacket_ *pkt);
        int 			getDstMac(ether_maddr_t* dst, ePacket_ *pkt);
        int			show_arptable();
        int			send_hello_msg();
	u_char*			get_mac(u_long ipDst);
        struct mesh_if*		get_mesh_if_by_mbinder(MBinder* mb);
        struct mesh_if*         get_fixed_lan_if();
        
}; 
 

#endif /* __NCTUNS_MeshArp_h__ */
