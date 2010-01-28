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

#include <assert.h>
#include <object.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include "mesharp.h"
#include <ethernet.h>
#include <arp/if_arp.h>
#include <ip.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <maptable.h>
#include <exportStr.h>
#include <packet.h>
#include <mbinder.h>

#include <fstream>
#include <stdlib.h>
#include <string.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(MeshArp);



ether_mac_addr_type::ether_mac_addr_type() {

    bzero( addr , mac_address_size );
   // printf("Call by compile time!!!!!\n");

}

int ether_mac_addr_type::show() {

/*
    for(unsigned int i=0 ; i<mac_address_size ;++i) {
        printf(" %d ", addr[i]);
    }
*/

    return 1;
}


MappingTableEntry::MappingTableEntry() {

     status_ = VALID; 
     bzero( src_mac_.addr , mac_address_size);
     bzero( dst_mac_.addr , mac_address_size);
     inport_mbinder_   = NULL;
     outport_mbinder_  = NULL;

     timeout_ = 0; /* disabled */
     prev    = NULL;
     next    = NULL;

}

MappingTableEntry::MappingTableEntry(ether_maddr_t* src_mac , ether_maddr_t* dst_mac , MBinder* inport_mbinder, MBinder* outport_mbinder) {

     /* port module binder pointer validation checking */
     ASSERTION(src_mac,"MappingTableEntry::MappingTableEntry(): src_mac is NULL.\n");
     ASSERTION(src_mac,"MappingTableEntry::MappingTableEntry(): dst_mac is NULL.\n");
     ASSERTION(inport_mbinder,"MappingTableEntry::MappingTableEntry(): inport_mbinder is NULL.\n");
     ASSERTION(outport_mbinder,"MappingTableEntry::MappingTableEntry(): outport_mbinder is NULL.\n");

     /* assignment */
     status_ = VALID; 
     memcpy(src_mac_.addr  , src_mac->addr , mac_address_size);
     memcpy(dst_mac_.addr  , dst_mac->addr , mac_address_size);
     timeout_ = 0; /* disabled */
     inport_mbinder_   = inport_mbinder;
     outport_mbinder_  = outport_mbinder;
     
     prev    = NULL;
     next    = NULL;

}

int MappingTableEntry::update( ether_maddr_t* src_mac, ether_maddr_t*dst_mac, MBinder* inport_mbinder, MBinder* outport_mbinder) {

     /* port module binder pointer validation checking */
     ASSERTION(src_mac,"MappingTableEntry::update(): src_mac is NULL.\n");
     ASSERTION(src_mac,"MappingTableEntry::update(): dst_mac is NULL.\n");
     ASSERTION(inport_mbinder,"MappingTableEntry::update(): inport_mbinder is NULL.\n");
     ASSERTION(outport_mbinder,"MappingTableEntry::update(): outport_mbinder is NULL.\n");

     /* assignment */
     status_ = VALID;
     memcpy(src_mac_.addr  , src_mac->addr , mac_address_size);
     memcpy(dst_mac_.addr  , dst_mac->addr , mac_address_size);
     timeout_ = 0; /* disabled */
     inport_mbinder_   = inport_mbinder;
     outport_mbinder_  = outport_mbinder;

     return 1;

}


status_t MappingTableEntry::get_status() {
    return status_;
}

MappingTableEntry*  MappingTableEntry::get_next() {
    return next;
}

MappingTableEntry*  MappingTableEntry::get_prev() {
    return prev;
}

ether_maddr_t* MappingTableEntry::get_src_mac() {
    return &src_mac_;
}

ether_maddr_t* MappingTableEntry::get_dst_mac() {
    return &dst_mac_;
}


int MappingTableEntry::set_next(MappingTableEntry* ptr) {
    next = ptr;
    return 1;
}

int MappingTableEntry::set_prev(MappingTableEntry* ptr) {
    prev = ptr;
    return 1;
}

int MappingTableEntry::set_inport_mbinder(MBinder* new_inport_mbinder) {

    inport_mbinder_ = new_inport_mbinder;
    return 1;

}


int MappingTableEntry::set_outport_mbinder(MBinder* new_outport_mbinder) {

    outport_mbinder_ = new_outport_mbinder;
    return 1;
  
}

MBinder* MappingTableEntry::get_inport_mbinder() {
    return inport_mbinder_;
}

MBinder* MappingTableEntry::get_outport_mbinder() {
    return outport_mbinder_;
}

MacPortMappingTable::MacPortMappingTable() {
     head = NULL;
     tail = NULL;
     item_num = 0;
}


MBinder*    MacPortMappingTable::get_inport_mbinder(ether_maddr_t* dst_mac) {

    ASSERTION( dst_mac , "MacPortMappingTable::get_inport_mbinder(): dst_mac is NULL.\n");

    MTEntry* ptr = head;

    while( ptr ) {

        if ( !strcmp( ((ptr->get_src_mac())->addr ), dst_mac->addr) ) {

             return ptr->get_inport_mbinder();
 
        }

        ptr = ptr->get_next();
    }

    return NULL;

}

MBinder*    MacPortMappingTable::get_outport_mbinder(ether_maddr_t* dst_mac) {

    ASSERTION( dst_mac , "MacPortMappingTable::get_outport_mbinder(): dst_mac is NULL.\n");
    MTEntry* ptr = head;

    while( ptr ) {


        if ( !strcmp( ((ptr->get_dst_mac())->addr), dst_mac->addr) ) {

             return ptr->get_outport_mbinder();

        }

        ptr = ptr->get_next();
    }

    return NULL;

}


status_t  MacPortMappingTable::get_status_by_src_mac(ether_maddr_t* src_mac) {

    ASSERTION( src_mac , "MacPortMappingTable::get_status_by_src_mac(): src_mac is NULL.\n");
    MTEntry* ptr = head;

    while( ptr ) {

        if ( !memcmp( ((ptr->get_src_mac())->addr), src_mac->addr, mac_address_size) ) {
        
            return ptr->get_status();

        }

        ptr = ptr->get_next();
    }

    return NO_PORTINFO;

   

}

MTEntry* MacPortMappingTable::get_entry_by_src_mac(ether_maddr_t* src_mac) {

    ASSERTION( src_mac , "MacPortMappingTable::get_entry_by_src_mac(): src_mac is NULL.\n");
    MTEntry* ptr = head;

    //printf("src_mac = "); showmac(src_mac->addr);printf("\n");

    while( ptr ) {

        //printf("compared mac = ");showmac((ptr->get_src_mac())->addr);printf("\n");
        if ( !memcmp( ((ptr->get_src_mac())->addr), src_mac->addr, mac_address_size) ) {

             return ptr;

        }

        ptr = ptr->get_next();
    }

    return NULL;

}


int MacPortMappingTable::insert(ether_maddr_t* src_mac , ether_maddr_t* dst_mac , MBinder* inport_mbinder, MBinder* outport_mbinder) {

    MTEntry* ptr = new MTEntry(src_mac, dst_mac, inport_mbinder, outport_mbinder);
    if (!ptr) {
        perror("IpPortMappingTable::insert()");
        exit(1);
    }
    
    if ( head == NULL ) {
        head = ptr;
        tail = ptr;
        ++item_num;
    }
    else {
        tail->set_next(ptr);
        ptr->set_prev(tail);
        tail = tail->get_next();
        ++item_num;
    }
    return 1;
}

int MacPortMappingTable::del(ether_maddr_t* src_mac) {


    ASSERTION( src_mac, "MacPortMappingTable::del() src_mac is NULL.\n" );
    
    MTEntry* ptr = get_entry_by_src_mac(src_mac);
    if (!ptr) {
        printf("IpPortMappingTable::del(): entry is not found.");
        src_mac->show();
        printf("\n");
        return 0;
    }
   
    MTEntry* prev_ptr = ptr->get_prev();
    MTEntry* next_ptr = ptr->get_next();
   
    if ( prev_ptr == NULL ) {
        if ( head == ptr ) {
             head = head->get_next();
        }
        else {
            printf("MacPortMappingTable::del(): List_head corrupted.\n");
            exit(1);
        }
    }
    else {
        prev_ptr->set_next(next_ptr);
    }
  
    if ( next_ptr )
        next_ptr->set_prev(prev_ptr); 
    else {
       if ( tail == ptr ) {
           tail = tail->get_prev();
       }
       else {
            printf("MacPortMappingTable::del(): List_tail corrupted.\n");
            exit(1);

       }
    }
   
    delete ptr;
    --item_num;
    return 1;
}


int MeshArp::update_src_mac_mbinder_mapping( ether_maddr_t* src_mac, MBinder* mb_p) {
 

    ASSERTION( src_mac , "MeshArp::update_src_mac_mbinder_mapping(): src_mac is NULL.\n");
    ether_maddr_t* dst_mac = new ether_maddr_t;

    MTEntry* mte_p = mac_port_map_table->get_entry_by_src_mac(src_mac);
    if ( mte_p ) {
        mte_p->update( src_mac , dst_mac , mb_p , recvtarget_ );
        return 1;
    }

    
    int res = mac_port_map_table->insert(src_mac , dst_mac , mb_p , recvtarget_);

    delete dst_mac;
    return res;
    
}

MBinder* MeshArp::find_inport_mbinder(ether_maddr_t* dst_mac) {
   
   ASSERTION( dst_mac , "MeshArp::find_inport_mbinder(): dst_mac is NULL.\n");

   MTEntry* mte_p = mac_port_map_table->get_entry_by_src_mac(dst_mac);

   if ( mte_p )
       return mte_p->get_inport_mbinder();
   else 
       return NULL;

}

MeshArp::MeshArp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{

	/* initialize arp table */
        arpTable = (arpTbl *)malloc(sizeof(arpTbl));
        arpTable->entrys = 0;
        arpTable->head = arpTable->tail = 0;

	ifaces_num = 0;

        
        mac_port_map_table = new MacPortMappingTable();
 
        hello_timer = new timerObj;
 
	/* bind variable */
	vBind("arpMode", &ARP_MODE);
	vBind("flushInterval", &flushInterval);
	vBind("ArpTableFileName", &fileName);
        vBind("ip", &ip_str );
        bzero(router_mac,9);
        vBind_mac("mac",router_mac);
        bzero(ospf_port_flag , 32);
}


MeshArp::~MeshArp() {

       if ( ARP_MODE ) 
           free(ARP_MODE);

}

int MeshArp::select_mesh_port() {

	for ( int i = 0 ; i < ifaces_num; i++){
		if ( !ifaces[i]->conn_fixed_network_flag ){
                    return i;
                }
	}
        return -1;

}

struct mesh_if* MeshArp::get_mesh_if_by_mbinder(MBinder* mb) {

    if (!mb) {

        printf("get_mesh_if_by_mbinder(): mb is NULL.\n");
        exit(1);

    }

    for (int i=0 ; i<ifaces_num ; ++i) {

        if (ifaces[i]->ptr == mb ) 
            return ifaces[i];        

    }

    return NULL;

}


int MeshArp::get(ePacket_ *pkt, MBinder *frm){
	
        struct  mesh_if* iface = NULL;
        keytype		srcMac, dstMac;
	struct neighbor *ne;
	Packet		*pk;
	
	pk = (Packet*) pkt->DataInfo_;


        ASSERTION(pk,"MeshArp::get(): get a packet with empty DataInfo_ field. \n");
        

        int ifaces_index = -1;
	for (int i = 0 ; i < ifaces_num; i++){
		if (((MBinder*)ifaces[i]->ptr)->bindModule() == frm->myModule()){
			iface = ifaces[i];
                        ifaces_index = i;
			break;
		}
	}


	if (!iface) {
	
             printf("MeshArp::get(): Abnormal case: cannot find the record for the incoming mbinder.\n");
             exit(1);
 	     //return NslObject::get(pkt,frm);
        }
     

	if (pk->pkt_getflags() == PF_SEND) {
	
             printf("MeshArp::get(): Abnormal case: MeshArp module is at the highest level in a protocol stack chain.\n");
             printf("It is buggy that the MeshArp module receives a packet with PF_SEND flag.\n");
             exit(1);
 	     //return NslObject::get(pkt,frm);
        }
     
        //printf("MeshArp::get(): packets from fixed_network_port = %d from %s\n" ,
        //    iface->conn_fixed_network_flag, frm->myModule()->get_name() );
 
        /* The following part deals with received packets from lower-level modules. */

	struct ether_header *eh = (struct ether_header *)pk->pkt_get();

	/* get ether src and ether dst */
	bcopy(eh->ether_shost, srcMac.key, 6);
	bcopy(eh->ether_dhost, dstMac.key, 6);
        
        //printf("\n");
        //showmac(srcMac.key);
        //showmac(dstMac.key);
        //printf("\n");

	ne = neigh_find( &neighbors, srcMac);
	if (!ne){

	    ne = (struct neighbor*)malloc( sizeof(struct neighbor));
	    keycopy( ne->id , srcMac);
	    ne->iface = iface;
	    ne->state = 1;
            ne->type = NEIGH_MOBILE;
	    ne->snr = 0;
	    neigh_add( &neighbors, ne);
	    printf("%s neighbor add %x:%x:%x:%x:%x:%x %s\n",get_name(),srcMac.key[0],srcMac.key[1],srcMac.key[2],
		srcMac.key[3],srcMac.key[4],srcMac.key[5],frm->myModule()->get_name());

	}

	if ((unsigned int)pk->pkt_getlen() <= sizeof (struct ether_header) + 1) {
		freePacket(pkt);
		return 1;
	}

        ether_maddr_t srcmac;
        memcpy( &srcmac.addr , &srcMac.key , 6 ); 
        update_src_mac_mbinder_mapping( &srcmac, (MBinder*)iface->ptr); 


         /* Procedures of receiving side are moved from recv() and performed here.*/
         /* Process ARP-protocol packets.
          * There are several rules to make the connectivity of mesh network correct.
          * A port connecting to a fixed network link is called as "fixed-network port."
          * A port connecting to a mesh network radio link is called as "mesh-ap port."
          *
          *
          * (1) If receiving an ARP-REQUEST packet from a fixed-network port, 
          *     MeshARP module do the following procedures:
          *
          *     1. Overwrite the source mac address (one of router's mac address ) of ARP REQUEST as one of
          *        mesh-ap ports' mac addresses. Note that if the mac address of one mesh-ap port for an 
          *        ARP-REQUEST packet  is selected, the ARP-REPLY packet  will be sent to the same mesh-ap 
          *        port later.
          *        
          *
          *     2. Send this ARP-REQUEST packet through the selected mesh-ap port.
          *
          *
          * (2) If receiving an ARP-REPLY packet from a fixed network port, do nothing.
          *     This is a unusual case because the ARP REQUEST for mesh client nodes are usually used to 
          *     inquire the mac address of a gateway router. In this case, the ARP-REQUEST packet is replied
          *     by MeshARP module. And so, it is rarely that ARP-REQUEST packets are really sent out through
          *     MeshARP's fixed network ports.
          *
          * 
          *
          * (3) If receiving an ARP-REQUEST packet from a mesh-ap port, then the MeshARP module should reply 
          *     this request packet with one of its mesh-ap ports' mac addresses.
          *
          *  
          * (4) If receiving an ARP-REPLY from a mesh-ap port, then the MeshARP module should do the following
          *     tasks:
          *
          *     1. Record this IP-MAC mapping.
          *     2. Overwrite the destination mac address as a gateway router's mac address.
          */

         int mergeFlag = false;

         if ( iface->conn_fixed_network_flag ) {
             /* In the case that packets are from a fixed network */
 
             if (pktIsArp(pkt)) {
      
                 printf("MeshArp::get(): receive an ARP packet.\n");
 
                 mergeFlag = freeArpUpdate(pkt);

		 if (iAmTpa(pkt)) {

		    if ( !mergeFlag )
                        freeArpLearning(pkt);

                    u_short ar_op = getArpOp(pkt);

                    if ( ar_op == ARPOP_REQUEST )
                        return(arpReply(pkt, (MBinder*)iface->ptr) );

                    else if ( ar_op == ARPOP_REPLY ) {
                        //printf("MeshArp::get(): prepare resumeSend(). \n");
                        return(resumeSend(pkt,(MBinder*)iface->ptr));
                    }

                    else
                        assert(0);

                 }
                 else {
 
                    //printf("MeshArp::get(): receive an ARP packet but the module is not the target..\n");
                   
                    u_short ar_op = getArpOp(pkt);

                    if ( ar_op == ARPOP_REQUEST ) {
                        
                         /* modify source address fields for this packet.*/
                         if (ifaces_index < 0) {
                             printf("MeshArp::get(): ifaces_index is negative.\n");
                             exit(1);
                         }

                         /* get arp request pkt */
                         Packet* req_pkt = (Packet *)pkt->DataInfo_;
                         arpPkt* Req_arpPkt_ = (arpPkt *)req_pkt->pkt_get(sizeof(struct ether_header));

                         u_long ipDst;
                         memcpy(&ipDst, Req_arpPkt_->ar_tpa, 4);
                         /*printf("MeshArp::get(): target_ip = ");
                         showip(ipDst);
                         printf("\n");*/
 
                         u_char* target_mac = get_mac(ipDst);
                         
                         /*printf("MeshArp::get(): target_mac = ");
                         showmac(target_mac);
                         printf("\n");*/

                         ether_maddr_t dst_mac;
                         MBinder* inport_mb = NULL;
                         if ( target_mac ) {
                             memcpy( dst_mac.addr , target_mac , 6);
        	             inport_mb = find_inport_mbinder(&dst_mac);
                         }
                         
                         int chosen_mesh_port = 0;

        	         if ( !inport_mb ) {
                             printf("MeshArp::get(): inport_mb is not found and select a default port.\n");
                             chosen_mesh_port = select_mesh_port();
                             inport_mb = (MBinder*)ifaces[chosen_mesh_port]->ptr;

                         }
                        
                         struct mesh_if* outif = get_mesh_if_by_mbinder(inport_mb); 
			 bcopy(outif->name.key, eh->ether_shost, 6);
                         bcopy(outif->name.key, Req_arpPkt_->ar_sha, 6);
                         /*printf("MeshArp::get(): send ARP request with new source mac = ");
                         showmac(eh->ether_shost);
                         printf("\n");
                         
                         printf("MeshArp::get(): send ARP request with new destination mac = ");
                         showmac(eh->ether_dhost);
                         printf("\n");*/

                         pk->pkt_setflow(PF_SEND);
                         //printf("MeshArp::get(): send to %s \n", ((MBinder*)(inport_mb))->bindModule()->get_name());
                         return(put(pkt,inport_mb)); 
                    }

                    else if ( ar_op == ARPOP_REPLY ) {
                         //printf("MeshArp::get(): receive an ARP reply from a fixed-network-link port.\n");
                         return(resumeSend(pkt,(MBinder*)iface->ptr));
                    }

                    else
                        assert(0);

                 }      

             }
             else {
        
                // printf("MeshArp::get(): receive a data packet.\n");

                 ip* iphdr = (ip*) pk->pkt_sget();

                 if (!iphdr) {
                     printf("MeshArp::get(): iphdr is null.\n");
                     return 1;
                     exit(1);
                 }

                 unsigned long ip_dst;
                 unsigned long ip_src;
		 IP_DST(ip_dst,iphdr);
                 IP_SRC(ip_src,iphdr);

                 //printf("get packet with DstIp = ");showip(ip_dst);printf(" SrcIp = ");showip(ip_src);printf("\n");
          
                 /* The mac address fields of mac header of a data packet should be modified.*/
                 u_char* target_mac = get_mac(ip_dst);

                 ASSERTION(target_mac,"MeshArp::get(): target_mac is NULL.\n"); 

                 ether_maddr_t dst_mac;
                 memcpy ( dst_mac.addr , target_mac , 6 ); 
                 dst_mac.show();

                 MBinder* inport_mb = find_inport_mbinder(&dst_mac);

                 if ( !inport_mb ) {
                     printf("MeshArp::get(): inport_mbinder is NULL.\n");
                     printf("dst mac = "); dst_mac.show(); 
                     exit(1);
                 }

                 struct mesh_if* outif = get_mesh_if_by_mbinder(inport_mb);

                 if (!outif) {

                     printf("MeshArp::get() outgoing interface structure is NULL.\n");
                     exit(1);
                 }

                 bcopy(outif->name.key , eh->ether_shost , 6 );
                
                 u_char* target_mac_addr = get_mac(ip_dst); 
                 if ( !target_mac_addr ) {

                     printf("MeshArp::get() target_mac_addr is NULL.\n");
                     exit(1);

                 }

                 bcopy(target_mac_addr  , eh->ether_dhost , 6 );
                 atchMacHdr(pkt, outif->name.key , target_mac_addr, ETHERTYPE_IP);
                 //printf("send the data packet with smac = ");showmac(eh->ether_shost);printf("\n");
                 //printf("send the data packet with dmac = ");showmac(eh->ether_dhost);printf("\n");
                 
                 
                 pk->pkt_setflow(PF_SEND);
                 //printf("send this data packet to %s \n", ((MBinder*)outif->ptr)->bindModule()->get_name());
                 return(put(pkt, (MBinder*)outif->ptr) );

             }

         }
         else {

             if ( pktIsArp(pkt) ) {

                  //printf("receive an arp packet.\n");

		  if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {

			mergeFlag = freeArpUpdate(pkt);

			if ( iAmTpa(pkt) ) {

                            //printf("MeshArp::get(): I am the target .\n");
		
          		    if ( !mergeFlag ) 
                                freeArpLearning(pkt);
		
			    u_short ar_op = getArpOp(pkt);
				
                            if ( ar_op == ARPOP_REQUEST ) {
                                //printf("it's arp request\n");
			        return(arpReply(pkt,(MBinder*)iface->ptr));
                            }
			
                            else if ( ar_op == ARPOP_REPLY ) {
                                 //printf("it's arp reply\n");
                                 /* redirect this arp reply packet to the gateway router */
                                
                                 /* get arp reply pkt */
                                 Packet* rep_pkt = (Packet *)pkt->DataInfo_;
                                 arpPkt* Rep_arpPkt_ = (arpPkt *)rep_pkt->pkt_get(sizeof(struct ether_header));

                                 u_long ipDst;
                                 memcpy(&ipDst, Rep_arpPkt_->ar_tpa, 4);
                                /* printf("MeshArp::get(): target_ip = ");
                                 showip(ipDst);
                                 printf("\n");*/
                                 
                                 struct mesh_if* outif = get_fixed_lan_if(); 
			         bcopy(outif->name.key, eh->ether_shost, 6);
                                 bcopy(outif->name.key, Rep_arpPkt_->ar_sha, 6);
                                 bcopy(router_mac, eh->ether_dhost , 6);
                                 bcopy(router_mac, Rep_arpPkt_->ar_tha , 6);
                                 /*printf("MeshArp::get(): send ARP reply with new source mac = ");
                                 showmac(eh->ether_shost);
                                 printf("\n");
                         
                                 printf("MeshArp::get(): modify ARP sha =  ");
                                 showmac(Rep_arpPkt_->ar_sha);
                                 printf("\n");

                                 printf("MeshArp::get(): send ARP reply with new destination mac = ");
                                 showmac(eh->ether_dhost);
                                 printf("\n");

                                 printf("MeshArp::get(): modify ARP tha = ");
                                 showmac(Rep_arpPkt_->ar_tha);
                                 printf("\n");
                                 */
                                 rep_pkt->pkt_setflow(PF_SEND);
                                 //printf("MeshArp::get(): send to %s \n", ((MBinder*)(outif->ptr))->bindModule()->get_name());
                                 return(put(pkt,(MBinder*)outif->ptr)); 
                                 
                            }

                            else {
                                printf("%s::get(): it's something else \n", get_name() );
			        assert(0);
                            }
			}
			else {
                            
                           // printf("MeshArp::get(): I am not the target.\n");   
			    
                           // if ( !mergeFlag ) 
                           //     freeArpLearning(pkt);
		
			    u_short ar_op = getArpOp(pkt);
				
                            if ( ar_op == ARPOP_REQUEST ) {
			        //return(arpReply(pkt,(MBinder*)iface->ptr));
                                //printf("it's arp request. free this packet.\n");
                                //freePacket(pkt);
                            }
			
                            else if ( ar_op == ARPOP_REPLY ) {
			        //return(resumeSend(pkt,(MBinder*)iface->ptr));
                                //printf("it's arp reply free this packet.");
                                //freePacket(pkt);
                            }

                            else {
                                printf("%s::get(): it's something else.\n", get_name() );
			        assert(0);
                            }
		           
                            freePacket(pkt);
			    return 1;
			}
		}
		else {
			freePacket(pkt);
                        printf("MeshArp module does not support modes other than \"RunARP\" \n");
                        exit(1);
			return 1;
		}
            }
            else {
          
                 //printf("MeshArp::get(): receive a data packet.\n");
 
                 ip* iphdr = (ip*) pk->pkt_sget();

                 //printf("MeshArp:: pkt_len = %d bytes. \n" , pk->pkt_getlen() );

                 if (!iphdr) {
                     printf("MeshArp::get(): iphdr is null.\n");
  		     freePacket(pkt);
                     return 1;
                 }

                 unsigned long ip_dst;
                 unsigned long ip_src;
		 IP_DST(ip_dst,iphdr);
                 IP_SRC(ip_src,iphdr);

                 //printf("get packet with DstIp = ");showip(ip_dst);
                 //printf(" SrcIp = ");showip(ip_src);printf("\n");
          
                 /* The mac address fields of mac header of a data packet should be modified.*/
                 ether_maddr_t dst_mac;
                 memcpy ( dst_mac.addr , eh->ether_dhost , 6 ); 

                 struct mesh_if* outif = get_fixed_lan_if();
                 //printf("%s packet is sent to outif %s ", get_name() , ((MBinder*)(outif->ptr))->bindModule()->get_name()  );
                 bcopy(outif->name.key , eh->ether_shost , 6 );
                

                 bcopy(router_mac  , eh->ether_dhost , 6 );
                 
                 atchMacHdr(pkt, outif->name.key , router_mac , ETHERTYPE_IP);
                 pk->pkt_setflow(PF_SEND);
                 //return(put(pkt, (MBinder*)(outif->ptr)) );
                 return (send_fixed_port(pkt,(MBinder*)outif->ptr));


            }
	}
    
        /* determine the outgoing port for this packet */
        
        pk->pkt_setflow(PF_SEND);
	return put(pkt, ((MBinder*)iface->ptr) );

	//return NslObject::get(pkt,frm);

}

inline u_long MeshArp::getDstIp(ePacket_ *pkt) {

	/* get next hop ip */
	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	return(pkt_->rt_gateway());
}


u_char *MeshArp::findArpTbl(u_long ipDst, int &recordExistButNoMac) {

	/* first check if ipDst is broadcast ip */
	u_char	*ip = (u_char *)&ipDst;
	if ( ip[3] == 255 ) {
		return((u_char *)ETHER_BROADCAST);
	}
	
	/* default situation is that record doesn't exist */
	recordExistButNoMac = 0;

	/* first, find normal arp table */
	u_int64_t currentTime = GetCurrentTime();
	for ( mapTbl *mt = arpTable->head; mt; mt = mt->next ) {
		if ( mt->ip == ipDst ) {
			if ( bcmp(mt->mac, ETHER_NULLADDR, 6) ) {
				/* this record has mac */
				mt->timestamp = currentTime + flushInterval_;
				return mt->mac;
			}
			else {
				/* this record has no mac */
				recordExistButNoMac = 1;
				return 0;
			}
		}
	}

	return 0;
}


int MeshArp::addArpTbl(u_long ip, u_char *mac, ePacket_ *pkt, arpTbl *targetArpTbl) {

        //printf("addcase\n");
 
	if ( targetArpTbl->entrys >= ARP_TBL_MAX_ENTRYS ) {
                printf("Arp table is full.\n");
                return -1;
        }

        if ( !mac )
            mac = (u_char*) "000000";
        //printf("addcase1 \n"); showip(ip); showmac((char*)mac); 
	/* establish this record */
	mapTbl *newRecord = (mapTbl *)malloc(sizeof(mapTbl));
	newRecord->ip = ip;
	if ( mac )
		bcopy(mac, newRecord->mac, 6);
	else
		bzero(newRecord->mac, 6);
        newRecord->timestamp = GetCurrentTime() + flushInterval_;
	/* pkt buffer - arp table maintains a pkt buffer for every
	 * entry(only one buffer) for re-sending. */
	newRecord->pkt = pkt;
        newRecord->next = 0;

        /* insert the record */
        if ( targetArpTbl->head && targetArpTbl->tail ) {
                /* switch table already has records */
                targetArpTbl->tail->next = newRecord;
                targetArpTbl->tail = newRecord;
        } else {
                /* this is the first record inserting into switch table */
                targetArpTbl->head = newRecord;
                targetArpTbl->tail = newRecord;
        }
        targetArpTbl->entrys++;

	return 1;
}


int MeshArp::delArpTbl(mapTbl *lastEntry, mapTbl *delEntry) {

        if ( arpTable->entrys <= 0 ) {
                printf("normal arp table is empty.\n");
                return -1;
        } else
        if ( arpTable->entrys == 1 ) {
                arpTable->head = 0;
                arpTable->tail = 0;
                arpTable->entrys = 0;
        } else {
                if ( delEntry == arpTable->head ) {
                        /* delete entry is head of arp table */
                        arpTable->head = arpTable->head->next;
                        delEntry->next = 0;
                } else
                if ( delEntry == arpTable->tail ) {
                        /* delete entry is tail of arp table */
                        arpTable->tail = lastEntry;
                        arpTable->tail->next = 0;
                } else {
                        /* delete entry is in one middle site */
                        lastEntry->next = delEntry->next;
                        delEntry->next = 0;
                }
                arpTable->entrys--;
        }

	if ( delEntry->pkt ) {
		Packet *p;
		ePacket_ *_pkt = pkt_copy(delEntry->pkt);
		GET_PKT(p, _pkt);
		if (-1 == p->pkt_callout(_pkt)) {
			// if failed, free the duplicated packet
//			printf("arp::delArpTbl: pkt_callout failed\n");
			freePacket(_pkt);
			_pkt = NULL;
		}
		freePacket(delEntry->pkt);
	}

        free(delEntry);

        return 1;
}


int MeshArp::dumpArpTable() {

	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	char			tmpBuf[30];

	ExpStr = new ExportStr(2);
	sprintf(tmpBuf, "IP\t\tMAC\n");
	ExpStr->Insert_comment(tmpBuf);

        for ( mapTbl *mt = arpTable->head; mt; mt = mt->next ) {

		row = ExpStr->Add_row();
		column = 1;

                ipv4addr_to_str(mt->ip, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t\t");

		macaddr_to_str(mt->mac, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\n");

        }

	EXPORT_GET_SUCCESS(ExpStr);
        return 1;
}


int MeshArp::atchMacHdr(ePacket_ *pkt, u_char* macSrc_  , u_char *macDst_, u_short frameType_) {

	keytype dst;
	struct neighbor *ne;
        Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);
	
	bcopy(macDst_, dst.key, 6);
	ne = neigh_find( &neighbors, dst);
	/* construct ether header */
        struct ether_header	*eh = 
	  (struct ether_header *)pkt_->pkt_malloc(sizeof(struct ether_header));

	/* directly filling the header space of the pkt */
	if (ne){
		//printf("found %x\n", ne->iface->name.key[0]);
		bcopy( ne->iface->name.key, eh->ether_shost, 6);
	}else{ 
		//printf("not found\n");
            
		bcopy(macSrc_, eh->ether_shost, 6);
	}
	bcopy(macDst_, eh->ether_dhost, 6);
	eh->ether_type = htons(frameType_);
	
	return 1;
}


int MeshArp::updatePktBuf(u_long ipDst, ePacket_ *pkt) {

/* arp table maintains a buffer space for each ip. so if
 * another pkt comes and has the same ip as the record that
 * already in arp table(ex.record is waiting for arp reply)
 * , we take new incoming pkt replacing old one cz we only
 * maintain one buffer space for each ip.
 */
	mapTbl *mt;

    for ( mt = arpTable->head; mt; mt = mt->next )
    	if ( mt->ip == ipDst ) break;

	if ( mt->pkt ) {
		Packet *p;
		ePacket_ *_pkt = pkt_copy(mt->pkt);
		GET_PKT(p, _pkt);
		if (-1 == p->pkt_callout(_pkt)) {
			// if failed, free the duplicated packet
//			printf("arp::updatePktBuf, pkt_callout failed\n");
			freePacket(_pkt);
			_pkt = NULL;
		}
		freePacket(mt->pkt);
	}
	mt->pkt = pkt;

	return 1;
}


int MeshArp::arpRequest(u_long ipDst, MBinder* mb) {

	/* construct arp pkt, attach it to event */
	ePacket_ *pkt = createEvent();
	Packet	 *pkt_ = new Packet;
	assert(pkt_);
	arpPkt	*arpPkt_ = (arpPkt *)pkt_->pkt_malloc( sizeof(arpPkt) );
	assert(arpPkt_);
	ATTACH_PKT(pkt_, pkt);

	/* fill arp header */
	arpPkt_->ar_hrd = htons(ARPHRD_ETHER);
	arpPkt_->ar_pro = htons(ETHERTYPE_IP);
	arpPkt_->ar_hln = ETHER_ADDR_LEN;
	arpPkt_->ar_pln = 4;
	arpPkt_->ar_op = htons(ARPOP_REQUEST);

        struct mesh_if* outif = get_mesh_if_by_mbinder(mb);
        if (!outif) {
           printf("MeshArp::arpRequest(): outif is null.\n");
        }
	bcopy(outif->name.key, arpPkt_->ar_sha, ETHER_ADDR_LEN);
        // printf("%s creates arp reply with sha = ");showmac(arpPkt_->ar_sha);printf("\n");
	bcopy(ip_, arpPkt_->ar_spa, 4);
	bcopy(&ipDst, arpPkt_->ar_tpa, 4);

	/* attach mac header */
	atchMacHdr(pkt, (u_char*)outif->name.key , (u_char *)ETHER_BROADCAST, ETHERTYPE_ARP);

        /* modified by Chih-che Lin 08/28/05 to make the sending process of packets correct.
         * ARP Request should be broadcasted.
         */


        //printf("%s send arpRequest.\n", get_name() );

        /* C.C. Lin: disable this code to avoid the degree of collisions in Mesh network. 
         *
         */
        /*int i;
        for ( i = 0; i < ifaces_num; i++){


                if (! ifaces[i])
                    continue;
nder*)ifaces[i]->ptr)
                Event* e_dup = pkt_copy(pkt);
                put( e_dup , (MBinder*)ifaces[i]->ptr);

        }*/
	
        Event* e_dup = pkt_copy(pkt);

        int port_id =0;
        char* ip4 = (char*)&ipDst;
        ip4+=3;
        int ip4_int = (int) (*ip4);
        if ( ((ip4_int + 24) < 35) || ((ip4_int+24) == 39) || ((ip4_int+24) == 40) )
            port_id = 0 ;
        else {
            port_id = 0;
        }
        printf("ipDst[4]+24 = %d port id = %d \n", ip4_int+24 , port_id);
        
        if ( !mb )
            put( e_dup , (MBinder*)ifaces[port_id]->ptr);
        else
            put( e_dup , mb);

        freePacket(pkt);     
	
        return 1;
}


u_char* MeshArp::get_mac(u_long ipDst) {

   for (int i=0 ; i<25 ; ++i) {

       if ( ipmac_table[i].ip == ipDst ) {
    
          return ipmac_table[i].mac;

       }

   } 

   return NULL;
}


int MeshArp::send_fixed_port(ePacket_ *pkt, MBinder* mb) {

	keytype dst;
	struct neighbor *ne;
  

	/* first get destination ip address */
	u_long ipDst = getDstIp(pkt);

	//printf("%s send\n",get_name());
	/* search arp table, if found, just send the pkt. if
	 * not, send arp request.
	 */
        struct mesh_if* outif = get_mesh_if_by_mbinder(mb);
        
        if (!outif) {
           printf("MeshArp::arpRequest(): arp is null.\n");
        }

	int	recordExistButNoMac = 0;
	if ( u_char *macDst = findArpTbl(ipDst, recordExistButNoMac) ) {
        
                //show_arptable();
 
                //printf("MeshArp::send_fixed_port(): ipdst =");showip(ipDst);printf("\n"); 
                //printf("MeshArp::send_fixed_port(): macdst = ");showmac(macDst);printf("\n");
		atchMacHdr(pkt, outif->name.key , macDst, ETHERTYPE_IP);
		bcopy(macDst, dst.key, 6);
		ne = neigh_find( &neighbors, dst);
		if (ne) {
		  //   printf("MeshARP send by the old arptable. ---- \n");
                     put(pkt, (MBinder*)ne->iface->ptr);
                     

                }
		else {
		     /*printf("MeshArp::send(): strange case!!!!\n");
        	     
                     printf("\nIP= ");showip(ipDst);printf("\n");
                     printf("NO MAC flag value = %d ", recordExistButNoMac);
                     printf("Entry Mac addr = ");
                     showmac((char*)macDst);*/
                     u_char* dst_mac = get_mac(ipDst);
                    

                     if ( !dst_mac ) {
                         printf("abnormal setting.\n"); 
                         printf("MeshArp::send(): strange case!!!!\n");

                         printf("\nIP= ");showip(ipDst);printf("\n");
                         printf("NO MAC flag value = %d ", recordExistButNoMac);
                         printf("Entry Mac addr = ");
                         //showmac((char*)macDst);

                         exit(1);
                     }
                     else {
                         macDst = dst_mac;
                         atchMacHdr(pkt, outif->name.key , macDst, ETHERTYPE_IP);
                    //     printf("filled target MAC = ");showmac(macDst);printf("\n");
                        // printf("fill in the mac by pre-defined table and send this packet in the first mbinder.\n");
                         put(pkt, (MBinder*)mb);
                         return 1;

                     }

                }

	} else {

		if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
			/* if record exists but has no mac, we only
			 * need to update buffer space.
			 */
			if ( recordExistButNoMac ) {
				updatePktBuf(ipDst, pkt);
			} 
			else {
				addArpTbl(ipDst, 0, pkt, arpTable);
				return(arpRequest(ipDst,mb));
			}
		}
		else {
			freePacket(pkt);
			return (1);
		}		
	}

	return 1;
}

int MeshArp::send(ePacket_ *pkt,MBinder* mb) {

	keytype dst;
	struct neighbor *ne;
  


	/* first get destination ip address */
	u_long ipDst = getDstIp(pkt);

	//printf("%s send\n",get_name());
	/* search arp table, if found, just send the pkt. if
	 * not, send arp request.
	 */
	int	recordExistButNoMac = 0;

        struct mesh_if* outif = get_mesh_if_by_mbinder(mb);
        if (!outif) {
            printf("MeshArp::arpRequest(): arp is null.\n");
        }


	if ( u_char *macDst = findArpTbl(ipDst, recordExistButNoMac) ) {

		atchMacHdr(pkt, outif->name.key , macDst, ETHERTYPE_IP);
		bcopy(macDst, dst.key, 6);
		ne = neigh_find( &neighbors, dst);
		if (ne) {
		     //printf("MeshARP send by the old arptable. ---- \n");
                     put(pkt, (MBinder*)ne->iface->ptr);

                }
		else {
		     /*printf("MeshArp::send(): strange case!!!!\n");
        	     
                     printf("\nIP= ");showip(ipDst);printf("\n");
                     printf("NO MAC flag value = %d ", recordExistButNoMac);
                     printf("Entry Mac addr = ");
                     showmac((char*)macDst);*/
                     u_char* dst_mac = get_mac(ipDst);
                    

                     if ( !dst_mac ) {
                         printf("abnormal setting.\n"); 
                         printf("MeshArp::send(): strange case!!!!\n");

                         printf("\nIP= ");showip(ipDst);printf("\n");
                         printf("NO MAC flag value = %d ", recordExistButNoMac);
                         printf("Entry Mac addr = ");
                         showmac((char*)macDst);

                         exit(1);
                     }
                     else {
                         macDst = dst_mac;
                         atchMacHdr(pkt, outif->name.key , macDst, ETHERTYPE_IP);
                         //printf("filled target MAC = ");showmac(macDst);printf("\n");
                        // printf("fill in the mac by pre-defined table and send this packet in the first mbinder.\n");
                       
                         int port_id =0;
                         char* ip4 = (char*)&ipDst;
                         ip4+=3;
        		 int ip4_int = (int) (*ip4);
        		 if ( ((ip4_int + 24) < 35) || ((ip4_int+24) == 39) || ((ip4_int+24) == 40) )
            		     port_id = 0 ;
        		 else
            		     port_id = 1;
        		
                         /* there are no good methods for selecting downlink port in a smart way without 
                          * an auxliary protocol for maintaining node location.
                          */

                         if (!mb)
                             mb = (MBinder*)ifaces[0]->ptr;
                         put(pkt, (MBinder*)mb);
                         return 1;

                     }

                }

	} else {

		if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
			/* if record exists but has no mac, we only
			 * need to update buffer space.
			 */
			if ( recordExistButNoMac ) {
				updatePktBuf(ipDst, pkt);
			} 
			else {
				addArpTbl(ipDst, 0, pkt, arpTable);
                                if (!mb) {
                                    printf("MeshArp::send(): mb is null.\n");
                                    exit(1);
                                }
				return(arpRequest(ipDst,mb));
			}
		}
		else {
			freePacket(pkt);
			return (1);
		}		
	}

	return 1;
}


int MeshArp::pktIsArp(ePacket_ *pkt) {

	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	
	struct ether_header *eh = 
		(struct ether_header *)pkt_->pkt_get();
	
	if ( ntohs(eh->ether_type) == ETHERTYPE_ARP ) {
		/* arp pkt */
		return 1;
	}
	
	/* not arp pkt, strip ether header */
	pkt_->pkt_seek(sizeof(struct ether_header));
	return 0;
}

/* free arp update: look up the incoming arp pkt, check spa and sha.
 * if my arp table has the entry whose ip equal spa, I can update
 * my table freely.
 */
int MeshArp::freeArpUpdate(ePacket_ *pkt) {

	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

   
        /* prevent from updateing incorrect ip-mac mapping */
        for (int i=0 ; i<ifaces_num ; ++i ){
	  
            if (!memcmp(arpPkt_->ar_sha,ifaces[i]->name.key, 6)) {
                 printf("MeshArp::freeArpUpdate(): ignoring this updating.\n");
                 return 1;
            }

        }

	
        mapTbl *mt;
        for ( mt = arpTable->head; mt; mt = mt->next )
                if ( !bcmp(&mt->ip, arpPkt_->ar_spa, 4) ) break;

	if ( mt ) {
		bcopy(arpPkt_->ar_sha, mt->mac, 6);
                //printf("MeshArp::freeupdate: mt->ip = ");showip(mt->ip);printf(" mt->mac = ");showmac(mt->mac);printf("\n");
                
		mt->timestamp = GetCurrentTime() + flushInterval_;
		return 1;
	}

	return 0;
}


int MeshArp::iAmTpa(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

	//return(bcmp(arpPkt_->ar_tpa, ip_, 4) == 0);
        if ( bcmp(arpPkt_->ar_tpa, ip_, 4) == 0 ) {
            //printf("MeshArp::recv an arp_protocol packet. pkt = %u \n", pkt);
	    /*
            unsigned char* ip_p = (unsigned char*)ip_;
            char* tip_p = (char*)arpPkt_->ar_tpa;
   
            printf("ip_ = %d %d %d %d  arpPkt_->ar_tpa = %d %d %d %d \n", ip_p[0], ip_p[1] , ip_p[2] , ip_[3] ,
                     tip_p[0] , tip_p[1] , tip_p[2] , (int)tip_p[3] );
	    */
            return 1;
        }
        else {
            //printf("MeshArp::recv a non-arp_protocol packet. pkt = %u \n", pkt );
            unsigned char* ip_p = (unsigned char*)ip_;
            char* tip_p = (char*)arpPkt_->ar_tpa;
   
            printf("ip_ = %d %d %d %d  arpPkt_->ar_tpa = %d %d %d %d \n", ip_p[0], ip_p[1] , ip_p[2] , ip_p[3] ,
                     tip_p[0] , tip_p[1] , tip_p[2] , tip_p[3] );
            

            
            return 0;
        } 
}

/* if no free arp update, do free arp learning. */
int MeshArp::freeArpLearning(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

        
        /* prevent from updateing incorrect ip-mac mapping */
        for (int i=0 ; i<ifaces_num ; ++i ){
	  
            if (!memcmp(arpPkt_->ar_sha,ifaces[i]->name.key, 6)) {
                 printf("MeshArp::freeArpLearning(): ignoring this updating.\n");
                 return 1;
            }

        }


	u_long spaUlong;
	bcopy(arpPkt_->ar_spa, &spaUlong, 4);
        //printf("MeshArp::freeArpLearning: add mt->ip = ");showip(spaUlong);
        //printf(" mt->mac");showmac(arpPkt_->ar_sha);printf("\n");
	return addArpTbl(spaUlong, arpPkt_->ar_sha, 0, arpTable);
}


u_short MeshArp::getArpOp(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

	return ntohs(arpPkt_->ar_op);
}


int MeshArp::arpReply(ePacket_ *pkt, MBinder* mb) {
	ePacket_	*reply_ep;
	Packet		*reply_pkt;
	Packet		*req_pkt;
	arpPkt		*Reply_arpPkt_;
	arpPkt		*Req_arpPkt_;
	struct neighbor	*ne;
	keytype		srcMac;

//        printf("%s send arpReply.\n", get_name() );

        struct mesh_if* outif = get_mesh_if_by_mbinder(mb);
        
        if (!outif) {
            printf("MeshArp::arpRequest(): arp is null.\n");
        }


	/* get arp request pkt */
        req_pkt = (Packet *)pkt->DataInfo_;
	Req_arpPkt_ = (arpPkt *)req_pkt->pkt_get(sizeof(struct ether_header));

	/* create reply pkt */
	reply_ep = createEvent();
	reply_pkt = new Packet;
	assert(reply_pkt);

       // char *testptr = reply_pkt->pkt_getinfo("PADDING");

	Reply_arpPkt_ = (arpPkt *)reply_pkt->pkt_malloc( sizeof(arpPkt) );
	assert(Reply_arpPkt_);
	
        //char *testptr = reply_pkt->pkt_getinfo("PADDING");
        ATTACH_PKT(reply_pkt, reply_ep);
        reply_pkt->pkt_setflow(PF_SEND);
	/* fill arp header */
	Reply_arpPkt_->ar_hrd = htons(ARPHRD_ETHER);
	Reply_arpPkt_->ar_pro = htons(ETHERTYPE_IP);
	Reply_arpPkt_->ar_hln = ETHER_ADDR_LEN;
	Reply_arpPkt_->ar_pln = 4;
	Reply_arpPkt_->ar_op = htons(ARPOP_REPLY);
	bcopy(Req_arpPkt_->ar_spa, Reply_arpPkt_->ar_tpa, 4);

	bcopy(Req_arpPkt_->ar_tpa, Reply_arpPkt_->ar_spa, 4);
	bcopy(Req_arpPkt_->ar_sha, Reply_arpPkt_->ar_tha, 6);
	
	bcopy(Reply_arpPkt_->ar_tha, srcMac.key, 6);
	
        ne = neigh_find( &neighbors, srcMac);
        
        if (!ne){
		bcopy(mac_, Reply_arpPkt_->ar_sha, 6);
	}else{
		bcopy(ne->iface->name.key, Reply_arpPkt_->ar_sha, 6);
	}

        //printf("ARPREQ target mac address = ");showmac((char*)Reply_arpPkt_->ar_sha);printf("\n");

	/* process ether header */
	atchMacHdr(reply_ep, outif->name.key  , (u_char *)Reply_arpPkt_->ar_tha, ETHERTYPE_ARP);


        /* modified by Chih-che Lin 08/28/05 to make the sending process of packets correct */

        ether_maddr_t dst_mac;
        //memcpy ( dst_mac.addr , Reply_arpPkt_->ar_tha , 6 ); 
        memcpy ( dst_mac.addr , Req_arpPkt_->ar_sha , 6 ); 


        MBinder* inport_mb = find_inport_mbinder(&dst_mac);

        if ( !inport_mb ) {
             printf("MeshArp::arpReply(): inport_mbinder is NULL.\n");
             printf("dst mac = "); dst_mac.show(); 
             printf("arp carried addresses: arpReq->srcmac = ");
             for ( int i= 0 ; i<6 ; ++i ) {
                 printf(" %d " , Req_arpPkt_->ar_sha[i] ) ; 
             }
             exit(1);
        }

        if (!mb)
            return put( reply_ep , inport_mb );
        else
            return put( reply_ep , mb );


}

int MeshArp::resumeSend(ePacket_ *pkt, MBinder* mb) {

        Packet  *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt	*arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));
	u_long	spaUlong;
	bcopy(arpPkt_->ar_spa, &spaUlong, 4);
	
        mapTbl *mt;
        for ( mt = arpTable->head; mt; mt = mt->next )
                if ( mt->ip == spaUlong ) break;

	freePacket(pkt);

        struct mesh_if* outif = get_mesh_if_by_mbinder(mb);
        if (!outif) {
            printf("MeshArp::arpRequest(): outif is null.\n");
            exit(1);
        }


	if ( mt->pkt ) {
		
		ePacket_ *pktTmp = mt->pkt;
                Packet* p = (Packet *)pktTmp->DataInfo_;
                p->pkt_setflow(PF_SEND);
		mt->pkt = 0;
		atchMacHdr(pktTmp, outif->name.key , mt->mac, ETHERTYPE_IP);

                /* modified by Chih-che Lin 08/28/05 to make the sending process of packets correct */

                ether_maddr_t dst_mac;
                memcpy( dst_mac.addr , mt->mac , 6);

                if (mb) {
                    printf("MeshArp::resumeSend(): send to %s \n", mb->bindModule()->get_name());
                    return put(pktTmp,mb);
                }

                exit(1); 
        	MBinder* inport_mb = find_inport_mbinder(&dst_mac);
        	if ( !inport_mb ) {
            		printf("MeshArp::resumeSend(): inport_mbinder is NULL.\n");
            		exit(1);
        	}

        	
                printf("MeshArp::resumeSend(): send to %s \n", inport_mb->bindModule()->get_name());
                return put( pktTmp , inport_mb );


	}

	return 1;
}


int MeshArp::recv(ePacket_ *pkt) {
	int	mergeFlag;
        
	
 //        printf("%s recv packet\n",get_name());
	

         /* Process ARP-protocol packets.
          * There are several rules to make the connectivity of mesh network correct.
          * A port connecting to a fixed network link is called as "fixed-network port."
          * A port connecting to a mesh network radio link is called as "mesh-ap port."
          *
          *
          * (1) If receiving an ARP-REQUEST packet from a fixed-network port, 
          *     MeshARP module do the following procedures:
          *
          *     1. Overwrite the source mac address (one of router's mac address ) of ARP REQUEST as one of
          *        mesh-ap ports' mac addresses. Note that if the mac address of one mesh-ap port for an 
          *        ARP-REQUEST packet  is selected, the ARP-REPLY packet  will be sent to the same mesh-ap 
          *        port later.
          *        
          *
          *     2. Send this ARP-REQUEST packet through the selected mesh-ap port.
          *
          *
          * (2) If receiving an ARP-REPLY packet from a fixed network port, do nothing.
          *     This is a unusual case because the ARP REQUEST for mesh client nodes are usually used to 
          *     inquire the mac address of a gateway router. In this case, the ARP-REQUEST packet is replied
          *     by MeshARP module. And so, it is rarely that ARP-REQUEST packets are really sent out through
          *     MeshARP's fixed network ports.
          *
          * 
          *
          * (3) If receiving an ARP-REQUEST packet from a mesh-ap port, then the MeshARP module should reply 
          *     this request packet with one of its mesh-ap ports' mac addresses.
          *
          *  
          * (4) If receiving an ARP-REPLY from a mesh-ap port, then the MeshARP module should do the following
          *     tasks:
          *
          *     1. Record this IP-MAC mapping.
          *     2. Overwrite the destination mac address as a gateway router's mac address.
          */

         MBinder* mb = NULL;

         if ( pktIsArp(pkt) ) {

             Packet *pkt_p = (Packet *)pkt->DataInfo_;

             struct ether_header *eh =
                (struct ether_header *)pkt_p->pkt_get();

             /* C.C. Lin: filter ARP packets broadcasted by the node itself. */
             if ( !memcmp( mac_ , eh->ether_shost , 6 ) ) {

                    printf("%s MeshArp::recv(): Bingo! this is a packet that I broadcasted. \n", get_name());
                    //rxBuf->DataInfo_ = NULL;
                    //freePacket(rxBuf);
                    return 0;

                }
	

		if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {

			mergeFlag = freeArpUpdate(pkt);

			if ( iAmTpa(pkt) ) {
		
          		    if ( !mergeFlag ) 
                                freeArpLearning(pkt);
		
			    u_short ar_op = getArpOp(pkt);
				
                            if ( ar_op == ARPOP_REQUEST )
			        return(arpReply(pkt,mb));
			
                            else if ( ar_op == ARPOP_REPLY )
			        return(resumeSend(pkt,mb));

                            else
			        assert(0);
			}
			else {
				freePacket(pkt);
				return 1;
			}
		}
		else {
			freePacket(pkt);
			return 1;
		}
	}
	return put(pkt, recvtarget_);
}



int MeshArp::parseLine(char *line, char *ip, char *mac) {

        if ( line[0] == '#' ) return(-1);

        char    *tmp;

        tmp = strtok(line, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
        strcpy(ip, tmp);

        tmp = strtok(NULL, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
        strcpy(mac, tmp);

        return(1);
}


int MeshArp::StrToIP(char *str, u_long &ip) {

        u_char          *p = (u_char *)&ip;

        char *tmp;
        tmp = strtok(str, ". \n\r\t\b");
        p[0] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[1] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[2] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[3] = (u_char)atoi(tmp);

        return(1);
}


int MeshArp::init() {

        NslObject::init();

	/* export variable */
	EXPORT("arp-table", E_RONLY);

	/* get MAC address */
	
	mac_ = GET_REG_VAR(get_port(), "MAC" , u_char* );
       
        if ( mac_ ) {
            printf("Marp mac_addr = ");
            for ( int i=0 ; i<6 ; ++i) {
                 printf(" %d ", mac_[i] );
            }
            printf("\n");
        }
        else
	    printf("Marp mac_addr is NULL \n");

	/* get IP address */
        
        assert(inet_aton( ip_str , &ip_addr) != 0);
        ip_ =(u_long*) &(ip_addr.s_addr);

        //char* print_ip_ = (char*)ip_;
        //printf("MeshARP get IP= %d %d %d %d\n", print_ip_[0] , print_ip_[1] , print_ip_[2] , print_ip_[3] );
        //printf("MeshARP get mac= ");showmac(router_mac);printf("\n");

	/*
	 *  <ARP_MODE>	
	 *
	 *	RunARP:
	 *		Capability
	 *		1.Send ARP reauest.
	 *		2.Gratuitous ARP.
	 *		3.Flush ARP table.
	 *
	 *	KnowInAdvance:
	 *		Capability
	 *		1.No ARP reauest .
	 *		2.No gratuitous ARP.
	 *		3.No ARP table flush.			
	 */

	for (int i = 0; i < ifaces_num; i++){
		
		u_char* mac = NULL;
		
		if (! ifaces[i])
		    continue;
		
		mac = GET_REG_VAR(((MBinder*)ifaces[i]->ptr)->bindModule()->get_port(),"MAC",u_char*);
		
		memcpy(ifaces[i]->name.key, mac,6);

                printf("MeshArp::init(): port[%d] mac= ", i);showmac(ifaces[i]->name.key);printf("\n");
	}

	neigh_init(&neighbors);	

	if ( !ARP_MODE ) {
		ARP_MODE = (char *)malloc( 16*sizeof(char) );
		strcpy(ARP_MODE , "RunARP");
	}

	if ( !flushInterval ) flushInterval = 1000000; // 3000 ms

        /* transfer flush interval unit from millisecond to tick */
        MILLI_TO_TICK(flushInterval_, (u_int64_t)flushInterval);

	if( ARP_MODE && !strcmp(ARP_MODE, "KnowInAdvance") ) {
		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(fileName) + 1);
		sprintf(FILEPATH,"%s/%s",GetConfigFileDir(),fileName);

                ifstream tblFile(FILEPATH);
                if ( !tblFile ) {
                        printf("Can't read in specified arp table <%s>.\n"
				,FILEPATH);
                        printf("Automatically turn arpMode to normal arp\n");
                        strcpy(ARP_MODE, "RunARP");
                }
		else {
                	char    line[128];
                	u_char  mac__[6] = {0};
			u_long	ip__ = 0;
                	char    ip[20] = {0}, mac[20] = {0};
                	for ( tblFile.getline(line, 128); strlen(line) > 0;
                        	tblFile.getline(line, 128) )
                	{
                        	if ( parseLine(line, ip, mac) < 0 )
					continue;

				StrToIP(ip, ip__);
                       		str_to_macaddr(mac, mac__);
				addArpTbl(ip__, mac__, 0, arpTable);
                	}
                	tblFile.close();
			return(1);
		}
		free(FILEPATH);		
	}
	
	if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
		/* set timer function to regularly check normal arp table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MeshArp, flushArpTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
	}
	else {
		ARP_MODE = (char *)malloc( 16*sizeof(char) );
		strcpy(ARP_MODE , "RunARP");

		/* set timer function to regularly check normal arp table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MeshArp, flushArpTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
	}

        /* initialize_hello_timer */ 
        hello_timer->init();
        hello_timer->setCallOutObj(reinterpret_cast<NslObject*>(this),(int (NslObject::*)(Event*)) &MeshArp::send_hello_msg);
        
        u_int64_t sec_tick;
        SEC_TO_TICK(sec_tick,1);
        
        int meshospf_used_flag = true;
        
        if ( meshospf_used_flag )
            hello_timer->start( sec_tick,sec_tick);
        else {
            delete hello_timer;
            hello_timer = NULL;
        }

        /* pre-set IP-MAC mapping */
        ipmac_table = new IpMacEntry[100];
        struct in_addr ip_addr;
        ip_addr.s_addr = 0;
       
        /* open ipmac table file, named as $NID.ipmac */
        char filename[20];
        bzero(filename,20);
        printf("NID = %d \n", get_nid());
        sprintf(filename,"%d.ipmac", get_nid());
        int filepathlen = strlen(GetConfigFileDir())+strlen(filename) + 1;
        char* filepath= (char *)malloc(filepathlen);
        bzero(filepath,filepathlen);
        sprintf(filepath,"%s/%s",GetConfigFileDir(), filename);


        FILE* iptablefile_handle = fopen( filepath, "r");
        
        if ( !iptablefile_handle ) {
  
            printf("MESHARP::init(): open ipmac table file failed. The expected filename is %s \n", filepath);
            exit(1);

        } 

        char linebuf[70];
        int  linenum = 0;
        while (1) {

            if ( feof(iptablefile_handle) ) 
		break;

            if ( ferror(iptablefile_handle) )
                break;

            bzero(linebuf,70);

            fgets(linebuf, 70 , iptablefile_handle ); 

            char node_ip[20];
            char node_mac[20];
            bzero(node_ip,20);
            bzero(node_mac,20);
            sscanf( linebuf , "%s %s" , node_ip , node_mac  ); 
            
            if ( feof(iptablefile_handle) ) 
		break;

            if ( ferror(iptablefile_handle) )
                break;

            printf("node_mac = %s \n",node_mac);
            char* new_node_ip = strdup(node_ip);
            u_char* mac_addr = new u_char[6];
            bzero( mac_addr , 6);
            /* simple translate mac address string into 6-byte format */
            for (int i=0 ; i<6 ; ++i) {
                char* token = NULL;
                if ( !i)
                    token = strtok(node_mac, ":; \0\n");
                else
                    token = strtok(NULL,":; \0\n");

                mac_addr[i] = strtol(token,NULL,16); 
                printf("tok = %s ",token);
            }
            
            ipmac_table[linenum].ip_str = new_node_ip;
            ipmac_table[linenum].mac = mac_addr;
            
            //printf("Initialize ip_str = %s \n", new_node_ip);
            inet_aton( new_node_ip , &ip_addr );
            ipmac_table[linenum].ip = (u_long) (ip_addr.s_addr);
   
            //printf("Init IP = "); showip(ipmac_table[linenum].ip);
            //printf("\n Init MAC = "); showmac((char*)ipmac_table[linenum].mac);printf("\n");
 

            linenum++;

        }
 
	fclose(iptablefile_handle);
        iptable_max_index = linenum;

        return(1);
}	


int MeshArp::send_hello_msg() {

    for ( int i=0 ; i < ifaces_num ; i++ ) {
        if (!ifaces[i]->conn_fixed_network_flag && ospf_port_flag[i])
            mesharp_ospf_helo_ori( ifaces[i] );
    }

    show_arptable();
    return 1;

}

int MeshArp::flushArpTbl() {

        printf("arptable entry number = %d \n" , arpTable->entrys );
        show_arptable();
        /* take away entry which expires */
	u_int64_t currentTime = GetCurrentTime();
        for ( mapTbl *mt = arpTable->head, *mtLast = 0, *mtNext = 0; mt; ) {
                if ( currentTime >= mt->timestamp ) {
                        mtNext = mt->next;
                        delArpTbl(mtLast, mt);
                        mt = mtNext;
                        //printf("case1 \n");
                } else {
                        mtLast = mt;
                        mt = mt->next;
                        //printf("case2\n");
                }
        }

        /* set timer function to regularly check normal arp table */
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(MeshArp, flushArpTbl);
        flushTimer.setCallOutObj(this, type);
        flushTimer.start(flushInterval_, 0);

        return 1;
}

struct mesh_if* MeshArp::get_fixed_lan_if() {

    for (int i=0 ; i<ifaces_num ; ++i) {

          if (ifaces[i]->conn_fixed_network_flag == 1 )
             return ifaces[i]; 

    }

    return NULL;

}
 

int MeshArp::show_arptable() {

        printf("MeshArp::show_arptable(): Dump entries.............. \n");
        printf("     ip         mac          timestamp         pkt     next    \n");

        /* take away entry which expires */
        for ( mapTbl *mt = arpTable->head, *mtLast = 0; mt; ) {
                       
                        showip(mt->ip);printf("   ");
                        showmac((char*)mt->mac);
                        printf("  %llu  ", mt->timestamp);
                        printf(" %p ", mt->pkt);
                        printf(" %p  \n" , mt->next);
                               

                        mtLast = mt;
                        mt = mt->next;
        }


        printf("\n....End of arpTable.\n\n");
        return 1;

}
  
int MeshArp::command(int argc, const char *argv[]) {

        NslObject*		obj;
	struct mesh_if*		iface = NULL;
        int    port_conn_fixed_net_flag_int_t = false;

        printf("Mesharp::init(): argc = %d \n", argc);
    
               
	if (!strncmp(argv[1], "port", 4)) {
            return 1; /* do nothing. Ignore this entry. */
	}
 
	if ( argc == 4 ) {
           
                if (argv[1]) {

                    if ( !strncmp(argv[1], "ospfport" , 8) ) {

			 u_int32_t	portNum = 0;
                         printf("MeshArp::init(): argv[1] = %s \n",argv[1] );
		         sscanf(argv[1], "ospfport%d", &portNum);

                         if (argv[3]) {
                            
                             if (!strncmp(argv[3],"yes",3)) 
                                 ospf_port_flag[portNum-1] = true;
                             else
				 ospf_port_flag[portNum-1] = false;

                         }


                        
                    }
                    else if (!strncmp(argv[1], "fixed_net", 9)) {

                       	/* support port should be added here */
			MBinder*	tmpMBinder = NULL;
			u_int32_t	portNum = 0;



                        if (argv[3]) {

			    char *module_name = strdup(argv[3]);
                            char* next_module_name = strtok(module_name,". \n\t");
                            char* port_conn_fixed_network_flag = strtok(NULL,". \n\t");
                            printf("next_module_name = %s \n", next_module_name);
                            
                            if ( !next_module_name )  {
                     
                                printf("MeshArp::command(): next_module_name is null");
                                //exit(1);
 
                            }

                            if ( !port_conn_fixed_network_flag ) {

                                printf("MeshArp::command(): port_conn_fixed_network_flag is null");
                                //exit(1);
                            }

                            printf("Name = %s conn_fixed_network_flag = %s \n",
                                 next_module_name , port_conn_fixed_network_flag );

                            if ( next_module_name ) {            
		       
                                obj = RegTable_.lookup_Instance(get_nid(), next_module_name);
		         
                                if (!obj) {
				    free(module_name);
			            return(-1);
	 	                }

                            }
                            else
			        obj = NULL;

			    free(module_name);
                         if (port_conn_fixed_network_flag) {

                             if ( !strncmp("yes" , port_conn_fixed_network_flag , 3) )
                                 port_conn_fixed_net_flag_int_t = true; 

                             else
                                 port_conn_fixed_net_flag_int_t = false; 
                     
                         }
                         else {
                             port_conn_fixed_net_flag_int_t = false;
                         }


                         printf("MeshArp::init(): argv[1] = %s \n",argv[1] );
		         sscanf(argv[1], "fixed_net%d", &portNum);

                         printf("portnum = %d" , portNum);

		         iface = (struct mesh_if*) malloc( sizeof(struct mesh_if));
		
		         tmpMBinder = new MBinder(this);
		         assert(tmpMBinder);
		         tmpMBinder->bind_to(obj);
		         iface->ptr = tmpMBinder;
		         iface->state = 1;
                         iface->conn_fixed_network_flag = port_conn_fixed_net_flag_int_t;
		         ifaces[portNum - 1] = iface;
		         ifaces_num++;

                         printf("MeshArp::init(): Set port connecting to obj whose name is %s as fixed_network_port = %d \n",
                             obj->get_name() , iface->conn_fixed_network_flag );             



                    }
                    else {;}

                }

            }

                
                
	}
	else {
                printf("MeshArp::init(): argv[1] = %s \n",argv[1] );
                return NslObject::command( argc, argv);
        }

        return(1);
}

int MeshArp::getSrcDstMac(ether_maddr_t* src, ether_maddr_t* dst, ePacket_ *pkt) {

        Packet  *pkt_ = (Packet *)pkt->DataInfo_;
        assert(pkt_);

        ASSERTION( src, "MeshArp::getSrcDstMac(): src is NULL.\n");
        ASSERTION( dst, "MeshArp::getSrcDstMac(): src is NULL.\n");
 
        /* decapsulate ether header */
        struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

        /* get ether src and ether dst */
        bcopy(eh->ether_shost, src->addr, 6);
        bcopy(eh->ether_dhost, dst->addr, 6);

        return 1;
}

int MeshArp::getSrcMac(ether_maddr_t* src, ePacket_ *pkt) {

        Packet  *pkt_ = (Packet *)pkt->DataInfo_;
        assert(pkt_);

        ASSERTION( src, "MeshArp::getSrcDstMac(): src is NULL.\n");
        /* decapsulate ether header */
        struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

        /* get ether src and ether dst */
        bcopy(eh->ether_shost, src->addr, 6);

        return 1;
}


int MeshArp::getDstMac(ether_maddr_t* dst, ePacket_ *pkt) {

        Packet  *pkt_ = (Packet *)pkt->DataInfo_;
        assert(pkt_);

        ASSERTION( dst, "MeshArp::getSrcDstMac(): src is NULL.\n");
 
        /* decapsulate ether header */
        struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

        /* get ether src and ether dst */
        bcopy(eh->ether_dhost, dst->addr, 6);

        return 1;
}
