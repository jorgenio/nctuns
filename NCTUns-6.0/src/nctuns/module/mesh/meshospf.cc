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
#include <mesh/meshospf.h>
#include <ethernet.h>
#include <random.h>
#include <netinet/in.h>
#include <packet.h>
#include <mbinder.h>

#include <fstream>
#include <stdlib.h>
#include <string>

extern RegTable                 RegTable_;
extern typeTable                *typeTable_;
//#define Debug 0

//#define MESH_CCLIN_DEBUG__ 0

#define MOSPFprintf if (0)printf
#define IS_ADHOC_PORT( a) (a->myModule() == ((MBinder*)ifaces[1]->ptr)->bindModule())

MODULE_GENERATOR(MeshOSPF);

MeshOSPF::MeshOSPF(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
        use_etx_flag = 1;
	ifaces_num   = 0;
	helocount    = 0;
        ospf_etx     = 0;
        ospf_ori     = 0;

        tmp_etx_flag_str = NULL;
        vBind("use_etx_flag", &tmp_etx_flag_str);

}

MeshOSPF::~MeshOSPF() {

}


/* main behaviors of switch are in send function */
int MeshOSPF::send(ePacket_ *pkt, MBinder *frm) {

        keytype		srcMac, dstMac,sendMac,recvMac;
	Packet		*pk = (Packet*)pkt->DataInfo_;
	struct mesh_rte* rte;
	struct ether_header *eh;
	char* p;

//	printf("\t%s: MeshOSPF send\n",get_name());

	getSrcDstMac(&sendMac, &recvMac, pkt);
	if (*(pk->pkt_get()+sizeof(struct ether_header)) == MAGICN){
		pk->pkt_seek(sizeof(struct ether_header) + 1);
		getSrcDstMac(&srcMac, &dstMac, pkt);
	}else{
		keycopy(srcMac,sendMac);
		keycopy(dstMac,recvMac);
	}

	if (pk->pkt_getlen() == 0){
		freePacket(pkt);
  //              printf("%s: pkt len = 0 \n", get_name() );
		return 1;
	}
	rte = rtb_find( &route_table, dstMac);

	if (!rte){
                
                printf("%s: rte is null.\n", get_name() );

		if (memcmp(dstMac.key, "\xff\xff\xff\xff\xff\xff", 6) == 0){
                
                    if (!use_etx_flag ) {
                        MOSPFprintf("%s flood send \n",get_name());
                    }
                
		    pk->pkt_malloc(sizeof(struct ospf_header)+sizeof(struct   lsa_header)+sizeof(struct ether_header));
                    
                    if ( use_etx_flag )
	                ospf_flood_packet_etx( ospf_etx, pk->pkt_get(), pk->pkt_getlen());
                    else 
                        ospf_flood_packet_ori( ospf_ori, pk->pkt_get(), pk->pkt_getlen());
                    
		} else {
                    printf("%s %d not know %x:%x:%x:%x:%x:%x ><(from %x:%x:%x:%x:%x:%x, %s) \n",get_name(), pk->pkt_getlen(),                    
                    dstMac.key[0], dstMac.key[1], dstMac.key[2], 
                    dstMac.key[3], dstMac.key[4], dstMac.key[5],                    
                    srcMac.key[0], srcMac.key[1], srcMac.key[2],
                    srcMac.key[3], srcMac.key[4], srcMac.key[5],
                    frm->myModule()->get_name());

                }

                //printf("%s: pkt is freed. \n", get_name() );                
		freePacket(pkt);
		return 1;
	}else if (rte->iface == ifaces[1]){
           
                
		p = pk->pkt_malloc(sizeof(struct ether_header) + 1);
		eh = (struct ether_header*)p;
		memcpy(eh->ether_dhost, rte->nexthop.key, 6);
		memcpy(eh->ether_shost, rte->iface->name.key, 6);
		eh->ether_type = htons(1);
		p += sizeof(struct ether_header);
		*p = MAGICN;
		MOSPFprintf("%s adhoc send pkt %p dst = %x:%x:%x:%x:%x:%x\n",get_name(), pkt,
			dstMac.key[0],dstMac.key[1],dstMac.key[2],dstMac.key[3],
			dstMac.key[4],dstMac.key[5]);

//		printf("%s nexthop %x:%x:%x:%x:%x:%x hop=%d\n",get_name(),
//			rte->nexthop.key[0],rte->nexthop.key[1],rte->nexthop.key[2],rte->nexthop.key[3],
//			rte->nexthop.key[4],rte->nexthop.key[5],rte->hop);

		put( pkt, (MBinder*)rte->iface->ptr);
	}else{
		MOSPFprintf("%s normal send pkt %p dst = %s\n",
		    get_name(), pkt, ((MBinder*)rte->iface->ptr)->bindModule()->get_name());
  //              showmac( (char*)dstMac.key );
   //             printf("\n");   
		return put(pkt, (MBinder*)rte->iface->ptr);
	}

        //printf("%s: do nothing.\n",get_name() );
        return 1;
}


int MeshOSPF::recv(ePacket_ *pkt) {
	return 1;
}

int MeshOSPF::get(ePacket_ *pkt, MBinder *frm) {

	Packet			*p;
	struct ether_header	*eh;
	u_char			*data;
	keytype			sid;
	struct mesh_if		*iface;
	int			i;
        int    fixed_lan_port_flag = false;
        int    arp_pkt_flag = false;

      	GET_PKT(p, pkt);
        eh = (ether_header *)p->pkt_get();
	data = (u_char*)eh + sizeof(ether_header);
	memcpy(sid.key, eh->ether_shost, 6);


        //printf("%s::get(): srcmac = ",get_name());showmac(eh->ether_shost);printf("  dstmac = ");showmac(eh->ether_dhost);printf("\n");

        if ( ntohs(eh->ether_type) == ETHERTYPE_ARP ) {
            /* arp pkt */
            arp_pkt_flag = true;
        }


	for ( i = 0 ; i < ifaces_num; i++){
		if (((MBinder*)ifaces[i]->ptr)->bindModule() == frm->myModule()){
			iface = ifaces[i];
			break;
		}
	}

	if (p->pkt_getlen() == sizeof(struct ether_header)){
		freePacket(pkt);
		return 0;
	}

#ifdef MESH_CCLIN_DEBUG__        
         printf("OSPF %d: get from %x:%x:%x:%x:%x:%x to %x:%x:%x:%x:%x:%x\n", get_nid(),
                    eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
                    eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5],
                    eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],
                    eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5] );
#endif

        if ( use_etx_flag ) {
            
            if (!IS_ADHOC_PORT(frm) && !neigh_find(&ospf_etx->neighbors, sid)){
                    
                    MOSPFprintf("%s net neighbor %d , len = %d,  %x:%x:%x:%x:%x:%x\n",
                    get_name(), sid.key[5], p->pkt_getlen(),
                    eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],
                    eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5] );
                
                    ospf_new_neighbor_etx( ospf_etx, sid, iface);
                    
            }
        }
	else {
            
            if (!IS_ADHOC_PORT(frm) && !neigh_find(&ospf_ori->neighbors, sid)){
                    
                    printf("%s net neighbor %d , len = %d,  %x:%x:%x:%x:%x:%x\n",
                    get_name(), sid.key[5], p->pkt_getlen(),
                    eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
                    eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5] );
                
                    ospf_new_neighbor_ori( ospf_ori, sid, iface);

            }
        }
        
	if (p->pkt_getlen() == sizeof(struct ether_header) + 1){
		freePacket(pkt);
		return 0;
	}
#ifdef Debug__
	printf("OSPF %d: get from %x:%x:%x:%x:%x:%x to %x:%x:%x:%x:%x:%x\n", get_nid(),
                    eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
                    eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5],
                    eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],
                    eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5] );
#endif
        if( IS_ADHOC_PORT(frm) && data[0] != MAGICN)
        {   
              	//p->pkt_seek(sizeof(struct ether_header));

                if ( use_etx_flag )
		    ospf_recv_etx(ospf_etx, p->pkt_get(), p->pkt_getlen());
                else
                    ospf_recv_ori(ospf_ori, p->pkt_get(), p->pkt_getlen());
              	freePacket(pkt);
              	return 0;
        } 
#ifdef Debug
	printf("OSPF %d: get from %x:%x:%x:%x:%x:%x to %x:%x:%x:%x:%x:%x\n", get_nid(),
                    eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
                    eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5],
                    eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],
                    eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5] );
#endif

	 /* Note by C.C. Lin on 09/23/2005.
          * release helo_msg from neighbors in fixed network:
          * Only hello message is associated with destination mac address
          * 0xffffffffffff. Assume that there are no broadcast packets from 
          * fixed network. If a broadcast from fixed networks is necessary,
          * this blocking mechanism should be refined.
          *
          *  
          */
         int bcast_flag = true;
         for ( int i=0 ; i<6 ;++i ) {
            
             if ( eh->ether_dhost[i] != 0xff ) {
                 bcast_flag = false;
             }
             
         } 

         if ( determine_fixed_lan_port(frm) ) 
	     fixed_lan_port_flag = true;
         else
	     fixed_lan_port_flag = false;

         if ( bcast_flag && fixed_lan_port_flag && (!arp_pkt_flag) ) {

             /*printf(" myname= %s adhoc_port_modulename= %s \n",
		get_name(), ((MBinder*)ifaces[1]->ptr)->bindModule()->get_name());*/
#ifdef MESH_CCLIN_DEBUG__
	     printf("%s free pkt bflag = %d flan_flag = %d sid %d , len = %d,  %x:%x:%x:%x:%x:%x\n",
            		get_name(), bcast_flag, fixed_lan_port_flag , sid.key[5], p->pkt_getlen(),
               		eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
               		eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5] );
#endif

             freePacket(pkt);
             return 0;                    

        }
      
//        printf("MeshOSPF fixed_lan_port_flag = %d \n", fixed_lan_port_flag);
	
 	Packet *pkt_ = (Packet *)pkt->DataInfo_;
 	pkt_->pkt_setflow(PF_SEND);
        //printf("%s send to the module %s \n", get_name() , frm->bindModule()->get_name());
 	return send(pkt, frm);

}


int MeshOSPF::init() {
	unsigned char* mac;
	u_int64_t HelloTimeTick;
	int i;

	for ( i = 0; i < ifaces_num; i++){
		NslObject* mbptr = (((MBinder*)ifaces[i]->ptr)->bindModule());
                u_int32_t nm_portid = mbptr->get_port();
		mac = GET_REG_VAR( nm_portid,"MAC",u_char*);
		if (i == 1)memcpy(ospf_id.key, mac, 6);
		memcpy(ifaces[i]->name.key, mac,6);
	}


	printf( "MESHOSPF_ETX_FLAG= %s \n" , tmp_etx_flag_str);
   
        if ( !tmp_etx_flag_str ) {
            printf("Warning: ETX flag is not set.\n Set it as off by default.\n");
            tmp_etx_flag_str = strdup("off");
        }
        if ( !strcmp(tmp_etx_flag_str, "yes") ) {
            use_etx_flag = 1;
            set_version(OSPF_ETX);
            printf("MeshOSPF runs with ETX.\n");
        }
        else {
            use_etx_flag = 0;
            set_version(OSPF_ORI);
            printf("MeshOSPF runs without ETX.\n");
        }

        if ( use_etx_flag ) {
        
	    ospf_etx = ospf_init_etx( ospf_id, 3, ifaces, ifaces_num);
            rtb_init(&route_table);
            ospf_etx->route = &route_table;    
        }
        else {
            ospf_ori = ospf_init_ori( ospf_id, 3, ifaces, ifaces_num);
            rtb_init(&route_table);
            ospf_ori->route = &route_table;
        }
	
	SEC_TO_TICK(HelloTimeTick, 1);
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(MeshOSPF, helo);
	heloTimer.setCallOutObj(this,  type);
        
        if ( use_etx_flag ) {
	    heloTimer.start(GetNodeCurrentTime(get_nid())+100+Random()%1000, HelloTimeTick/10);
        }
        else {
            heloTimer.start(GetNodeCurrentTime(get_nid())+100+Random()%1000, HelloTimeTick);
        }
	return(1);  

}

  
int MeshOSPF::command(int argc, const char *argv[]) {

    NslObject                       *obj;
    struct mesh_if* iface;

    if ( argc == 4 ) {

        bool uncanonical_module_name_flag = 0;

        /* The "." sign is not allowed as part of a module name. */
        if ( strstr(argv[3],".") ) {
            uncanonical_module_name_flag = 1;
        }
        /* The RegTable->lookup_Instance() with two parameter version
        * should use the canonical name of a module. If an illegal
        * module name is found, we should return immediately.
        * A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */
        if (strncasecmp("NODE", argv[3], 4))
            uncanonical_module_name_flag = 1;

        /* A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */

        /* Connectivity */
        if (!uncanonical_module_name_flag) {
            obj = RegTable_.lookup_Instance(get_nid(), argv[3]);
            if (!obj) {
                printf("No %s this Instance!\n\n", argv[3]);
                return(-1);
            }
        }
    }

    /* support port should be added here */
    MBinder		*tmpMBinder;
    u_int32_t	portNum;

    if (!strncmp(argv[1], "port", 4)) {
        sscanf(argv[1], "port%d", &portNum);

        iface = (struct mesh_if*) malloc( sizeof(struct mesh_if));

        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);

        /*
        unsigned char* mac;
        mac = GET_REG_VAR(obj->get_port(),"MAC",u_char*);
        memcpy( iface->name.key, mac, 6);
        */
        iface->ptr = tmpMBinder;
        iface->state = 1;

        ifaces[portNum - 1] = iface;
        ifaces_num++;

    }
    else if (!strcmp(argv[1], "sendtarget"))
        sendtarget_->bind_to(obj);
    else if(!strcmp(argv[1], "recvtarget"))
        recvtarget_->bind_to(obj);
    else {
        printf("MeshOSPF::command(): Invalid command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
        return(-1);
    }

    return(1);
}

int MeshOSPF::getSrcDstMac(keytype *src, keytype *dst, ePacket_ *pkt) {

	Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);

	/* decapsulate ether header */
	struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

	/* get ether src and ether dst */
	bcopy(eh->ether_shost, src->key, 6);
	bcopy(eh->ether_dhost, dst->key, 6);

        return 1;
}

int MeshOSPF::helo()
{
	
    if ( use_etx_flag ) {
        
        if (helocount  < OSPF_HELO_WINDOW * 2){
		ospf_helo_etx(ospf_etx);
	}else if (helocount == OSPF_HELO_WINDOW * 2){
		u_int64_t HelloTimeTick;
		SEC_TO_TICK(HelloTimeTick, 1);
		
		ospf_etx->state = OSPF_NORMAL_STATE;
		ospf_flood_lsa_etx(ospf_etx);
		ospf_helo_etx(ospf_etx);
		heloTimer.cancel();
		heloTimer.start(GetNodeCurrentTime(get_nid())+HelloTimeTick, HelloTimeTick);
	}else 
		ospf_helo_etx(ospf_etx);

	helocount++;
	return 0;
    }
    else {
    
        if (helocount  < 5){
		ospf_helo_ori(ospf_ori);
	}else if (helocount == 5){
		ospf_ori->state = OSPF_NORMAL_STATE;
		ospf_flood_lsa_ori(ospf_ori);
		ospf_helo_ori(ospf_ori);
	}//else 
		//ospf_helo(ospf);
	helocount++;
	return 0;
    }
}

int MeshOSPF::determine_fixed_lan_port(MBinder* frm) {

   MBinder* mb_ptr = frm;
  
   NslObject* obj_ptr = mb_ptr->myModule();
   if ( !obj_ptr )
       return 0;
  
   mb_ptr = obj_ptr->sendtarget_;

   while( obj_ptr ) {
       
       if ( strstr( obj_ptr->get_name() , "Phy" ) ) {
           return 1;
       }

       /* for safety! prevent from traversing other nodes' protocol modules */
       if ( strstr( obj_ptr->get_name() , "Wphy" ) ) {
           return 0;
       }

       if ( strstr( obj_ptr->get_name() , "AWphy" ) ) {
           return 0;
       }

 

       if ( mb_ptr )
           obj_ptr = mb_ptr->bindModule();
       else
	   obj_ptr = NULL;
       mb_ptr  = obj_ptr->sendtarget_;   

   } 

   return 0;

}
