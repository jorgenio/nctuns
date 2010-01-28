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

#include "ospf.h"
#include <stdlib.h>
#include <netinet/in.h>
#include <net/ethernet.h>

using namespace std; 

int use_etxlib_flag; 


int get_version() { return use_etxlib_flag;}


int set_version(int value) { 
    
    use_etxlib_flag = value; 
    return 1;

}

ospf_d_etx* ospf_init_etx(keytype id, unsigned short interval, struct mesh_if** ifaces, int n){
	
	int i ;
	ospf_d_etx *od;

        use_etxlib_flag = OSPF_ETX;
	od = (ospf_d_etx*)malloc(sizeof(ospf_d_etx));
	
	if (!od) {
		merror("can't allocate ospf_d\n");
	}

	od->tree = NULL;
	od->header.ver= 1;
	keycopy(od->header.rid, id);
	od->helo_interval = interval;
	init_list(&od->self_lsas);
	init_list(&od->lsa_lists);
	neigh_init(&od->neighbors);
	ospf_new_neighbor_etx( od, id, NULL);

	od->seq = 1;
	od->bseq = 1;

	od->adhoc_iface = 1;

	for ( i = 0; i < n; i++)
		od->ifaces[i] = ifaces[i];

	od->ifaces_num = n;

	od->state = OSPF_HELO_STATE;
	
	return od;
}

ospf_d_ori* ospf_init_ori(keytype id, unsigned short interval, struct mesh_if** ifaces, int n){
	
	int i ;
	ospf_d_ori *od;
	
        use_etxlib_flag = OSPF_ORI;
        od = (ospf_d_ori*)malloc(sizeof(ospf_d_ori));
	
	if (!od) {
		merror("can't allocate ospf_d\n");
	}

	od->tree = NULL;
	od->header.ver= 1;
	keycopy(od->header.rid, id);
	od->helo_interval = interval;
	init_list(&od->self_lsas);
	init_list(&od->lsa_lists);
	neigh_init(&od->neighbors);
	ospf_new_neighbor_ori( od, id, NULL);

	od->seq = 1;
	od->bseq = 1;   

	od->adhoc_iface = 1;

	for ( i = 0; i < n; i++)
		od->ifaces[i] = ifaces[i];

	od->ifaces_num = n;

	od->state = OSPF_HELO_STATE;
	
	return od;
}


/*
 *
 * OSPF Network functions
 * 
 */

void ospf_helo_etx(ospf_d_etx* od){


	struct lsa_entry_etx* le;
        int bufsize = sizeof(struct ospf_header)+sizeof(struct helo_header)+800;
	char buf[bufsize];
        bzero( buf , bufsize );
	char* p = buf;
	struct ospf_header* oh = (struct ospf_header*)p;
	struct helo_header* hh = (struct helo_header*)(p + sizeof(struct ospf_header));

        
        //printf("VER=%d \n\n", get_version());
        
        if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_helo_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	memcpy( oh, &od->header, sizeof(struct ospf_header));
	oh->type = OSPF_HELO;
	oh->len = sizeof(buf);

	hh->interval = od->helo_interval;

	broadcast_packet(ospf_adhoc_iface(od), buf,sizeof(buf));

	/* check the delivery rate */
	WALK_LIST(le, od->self_lsas, struct lsa_entry_etx*){

		if(le->body.drate > 0 && le->body.type == NEIGH_AP){
			le->body.drate--;
                 

                /* the following disabled codes are marked by jclin to prevent from
                 * segmentation faults for some reason.
                 *
                 * enable the following piece of codes to test
                 */
               
   	                if (le->timeout == 0){
				if( (le->state == OSPF_DRATE_HIGH) && (le->body.drate < OSPF_DRATE_HALF) )
					le->timeout++;
				else if( (le->state == OSPF_DRATE_HALF) && 
                                         (le->body.drate < OSPF_DRATE_LOW || le->body.drate > OSPF_DRATE_HIGH) )
					le->timeout++;

				else if(le->state == OSPF_DRATE_LOW && le->body.drate > OSPF_DRATE_HALF)
					le->timeout++;
				else
				    ;
			}else {
				
				le->timeout++;
				if (le->timeout >= OSPF_HELO_WINDOW){

					if ( (le->state == OSPF_DRATE_HIGH && le->body.drate < OSPF_DRATE_HALF) ||
					     (le->state == OSPF_DRATE_HALF && 
						(le->body.drate < OSPF_DRATE_LOW || le->body.drate > OSPF_DRATE_HIGH) ) ||
					     (le->state == OSPF_DRATE_LOW && le->body.drate > OSPF_DRATE_HALF)) {

						if (le->body.drate > OSPF_DRATE_HIGH) 
						    le->state = OSPF_DRATE_HIGH;

						else if (le->body.drate > OSPF_DRATE_LOW) 
                                                    le->state = OSPF_DRATE_LOW;

						else 
                                                    le->state = OSPF_DRATE_HALF;
						
						//oh->type = OSPF_UPDATE_LSA;
						// it should update lsa here, but i'm lazy.............
						ospf_flood_lsa_etx(od);
					}
				}
			}//*/
		}
	}
}

void ospf_helo_ori(ospf_d_ori* od){
	
	char buf[sizeof(struct ospf_header)+sizeof(struct helo_header)];
	char* p = buf;
	struct ospf_header* oh = (struct ospf_header*)p;
	struct helo_header* hh = (struct helo_header*)(p + sizeof(struct ospf_header));


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_helo_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	memcpy( oh, &od->header, sizeof(struct ospf_header));
	oh->type = OSPF_HELO;
	oh->len = sizeof(buf);

	hh->interval = od->helo_interval;

	broadcast_packet(ospf_adhoc_iface(od), buf,sizeof(buf));
}

void ospf_flood_packet_etx(ospf_d_etx* od, char* buf, int s){
	
	char*p = buf + sizeof(struct ether_header);
	struct ospf_header* oh = (struct ospf_header*)p;
	struct lsa_header* lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;
	int i;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_flood_packet_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)return;
	
	keycopy(from, oh->rid);
	
	memcpy( oh, &od->header,sizeof(struct ospf_header));
	oh->type = OSPF_BROADCAST;
	oh->len = s - sizeof(struct ether_header);

	keycopy( lh->aid ,od->header.rid);
	lh->seq = od->bseq++;

	p += sizeof(struct ospf_header) + sizeof(struct lsa_header);

	for ( i = 0; i < od->ifaces_num; i++){
		if ( i != od->adhoc_iface){
			send_packet( od->ifaces[i], p , s - sizeof(struct ospf_header) - sizeof( struct lsa_header) - sizeof(struct ether_header) );
		}
	}

	//broadcast_packet( ospf_adhoc_iface(od), buf,oh->len);
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}


void ospf_flood_packet_ori(ospf_d_ori* od, char* buf, int s){
	
	char*p = buf + sizeof(struct ether_header);
	struct ospf_header* oh = (struct ospf_header*)p;
	struct lsa_header* lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;
	int i;

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_flood_packet_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)return;
	
	keycopy(from, oh->rid);
	
	memcpy( oh, &od->header,sizeof(struct ospf_header));
	oh->type = OSPF_BROADCAST;
	oh->len = s - sizeof(struct ether_header);

	keycopy( lh->aid ,od->header.rid);
	lh->seq = od->bseq++;

	p += sizeof(struct ospf_header) + sizeof(struct lsa_header);

	for ( i = 0; i < od->ifaces_num; i++){
		if ( i != od->adhoc_iface){
			send_packet( od->ifaces[i], p , s - sizeof(struct ospf_header) - sizeof( struct lsa_header) - sizeof(struct ether_header) );
		}
	}

	//broadcast_packet( ospf_adhoc_iface(od), buf,oh->len);
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_flood_lsa_etx(ospf_d_etx* od){
	
	char buf[2048];
        bzero ( buf , 2048 );
	char *p;
	struct lsa_entry_etx* lsae = NULL;
	struct ospf_header* oh = NULL;
	struct lsa_header* lh = NULL;
	struct neighbor* ne = NULL;
	struct ether_header *eh = (struct ether_header*)buf;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_flood_lsa_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)
	    return;

	p = buf + sizeof(struct ether_header);
	oh = (struct ospf_header*)p;
	lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	memcpy( oh, &od->header,sizeof(struct ospf_header));
	oh->type = OSPF_LSA;
	oh->len = sizeof(buf);

	keycopy( lh->aid ,od->header.rid);
	lh->seq = od->seq++;

	OSPF_DEBUG("%d flood lsa\n", od->header.rid.key[5]);
	p += sizeof(struct ospf_header)+sizeof(struct lsa_header);
	WALK_LIST( lsae, od->self_lsas, struct lsa_entry_etx*){
		memcpy(p, &lsae->body, sizeof(struct lsa_body_etx));
		p += sizeof(struct lsa_body_etx);
	}
	lh->len = p - (char*)lh;
	oh->len = p - (char*)oh;

	//broadcast_packet( ospf_adhoc_iface(od), buf,oh->len);
	/*
	 * we should not use broadcast here
	 * because lsa packet it's not allow to be missed
	 * and broadcast packet is not safe
	 */
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od)){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_flood_lsa_ori(ospf_d_ori* od){
	
	char buf[2048];
        bzero( buf, 2048 );
	char *p;
	struct lsa_entry_ori* lsae;
	struct ospf_header* oh;
	struct lsa_header* lh;
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_flood_lsa_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)return;

	p = buf + sizeof(struct ether_header);
	oh = (struct ospf_header*)p;
	lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	memcpy( oh, &od->header,sizeof(struct ospf_header));
	oh->type = OSPF_LSA;
	oh->len = sizeof(buf);

	keycopy( lh->aid ,od->header.rid);
	lh->seq = od->seq++;

	OSPF_DEBUG("%d flood lsa\n", od->header.rid.key[5]);
	p += sizeof(struct ospf_header)+sizeof(struct lsa_header);
	WALK_LIST( lsae, od->self_lsas, struct lsa_entry_ori*){
		memcpy(p, &lsae->body, sizeof(struct lsa_body_ori));
		p += sizeof(struct lsa_body_ori);
	}
	lh->len = p - (char*)lh;
	oh->len = p - (char*)oh;

	//broadcast_packet( ospf_adhoc_iface(od), buf,oh->len);
	/*
	 * we should not use broadcast here
	 * because lsa packet it's not allow to be missed
	 * and broadcast packet is not safe
	 */
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od)){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}


void ospf_forward_lsa_etx(ospf_d_etx* od, char* buf, int s){
	struct ospf_header* oh = NULL;
	struct lsa_header* lh = NULL;
	char*p = buf + sizeof(struct ether_header);
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_forward_lsa_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)return;

	oh = (struct ospf_header*)p;
	lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	
	keycopy(from, oh->rid);
	
	keycopy( oh->rid, od->header.rid);
	//broadcast_packet( ospf_adhoc_iface(od), buf, oh->len);

	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_forward_lsa_ori(ospf_d_ori* od, char* buf, int s){
	
	struct ospf_header* oh = NULL;
	struct lsa_header* lh = NULL;
	char*p = buf + sizeof(struct ether_header);
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_forward_lsa_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if (od->state != OSPF_NORMAL_STATE)return;

	oh = (struct ospf_header*)p;
	lh = (struct lsa_header*)(p + sizeof(struct ospf_header));
	
	keycopy(from, oh->rid);
	
	keycopy( oh->rid, od->header.rid);
	//broadcast_packet( ospf_adhoc_iface(od), buf, oh->len);

	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors, struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_recv_etx( ospf_d_etx *od, char* buf, int size){
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_recv_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	//OSPF_DEBUG("%d recv from%d\n", od->header.rid.key[5], oh->rid.key[5]);
	switch (oh->type){
		case OSPF_HELO:
			ospf_recv_helo_etx(od, buf, size);
			break;
		case OSPF_LSA:
			ospf_recv_lsa_etx(od, buf, size);
			break;
		case OSPF_BROADCAST:
			ospf_recv_broadcast_etx( od, buf, size);
			break;
	};
}

void ospf_recv_ori( ospf_d_ori *od, char* buf, int size){
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	
	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_recv_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	//OSPF_DEBUG("%d recv from%d\n", od->header.rid.key[5], oh->rid.key[5]);
	switch (oh->type){
		case OSPF_HELO:
			ospf_recv_helo_ori(od, buf, size);
			break;
		case OSPF_LSA:
			ospf_recv_lsa_ori(od, buf, size);
			break;
		case OSPF_BROADCAST:
			ospf_recv_broadcast_ori( od, buf, size);
			break;
	};
}

void ospf_recv_broadcast_etx( ospf_d_etx *od, char* buf, int size){
	
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_header* lh = (struct lsa_header*)(buf + sizeof(struct ether_header) + sizeof(struct ospf_header));
	struct lsa_list *lsas;
	char* p = NULL;
	int i = 0;
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_recv_broadcast_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if ( keycmp( od->header.rid, lh->aid ) == 0) return;
	lsas = ospf_get_lsa_list_etx( od, lh->aid);
	if (!lsas)return;
	
	if (lh->seq <= lsas->bseq)return;
	lsas->bseq = lh->seq;
	
	keycopy(from, oh->rid);

	p = buf + sizeof(struct ospf_header) + sizeof( struct lsa_header) + sizeof(struct ether_header);

	for ( i = 0; i < od->ifaces_num; i++){
		if ( i != od->adhoc_iface){
			send_packet( od->ifaces[i], p , size - sizeof(struct ospf_header) - sizeof( struct lsa_header) - sizeof(struct ether_header) );
		}
	}
	//broadcast_packet(ospf_adhoc_iface(od),buf,size);
	
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors,struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_recv_broadcast_ori( ospf_d_ori *od, char* buf, int size){
	
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_header* lh = (struct lsa_header*)(buf + sizeof(struct ether_header) + sizeof(struct ospf_header));
	struct lsa_list *lsas;
	char* p = NULL;
	int i = 0;
	struct neighbor* ne;
	struct ether_header *eh = (struct ether_header*)buf;
	keytype from;

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_recv_broadcast_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if ( keycmp( od->header.rid, lh->aid ) == 0) return;

	lsas = ospf_get_lsa_list_ori( od, lh->aid);
	if (!lsas)return;

	//printf("%x:%x:%x:%x:%x:%x lh->seq=%d, lsas->bseq=%d\n",lh->aid.key[0],lh->aid.key[1],lh->aid.key[2],
	//		lh->aid.key[3],lh->aid.key[4],lh->aid.key[5],lh->seq,lsas->bseq);
	
	if ((lh->seq <= lsas->bseq ))  
	{
	//	printf("break return%d\n");
		return;  //doe???
	}
	lsas->bseq = lh->seq;
	
	keycopy(from, oh->rid);

	p = buf + sizeof(struct ospf_header) + sizeof( struct lsa_header) + sizeof(struct ether_header);

	for ( i = 0; i < od->ifaces_num; i++){
		if ( i != od->adhoc_iface){
			send_packet( od->ifaces[i], p , size - sizeof(struct ospf_header) - sizeof( struct lsa_header) - sizeof(struct ether_header) );
		}
	}
	//broadcast_packet(ospf_adhoc_iface(od),buf,size);
	
	eh->ether_type = htons(0x0001);
	memcpy( eh->ether_shost, ospf_adhoc_iface(od)->name.key,6);

	WALK_LIST( ne, od->neighbors,struct neighbor*){
		if (ne->iface == ospf_adhoc_iface(od) && keycmp(ne->id, from) != 0){
			memcpy( eh->ether_dhost, ne->id.key ,6);
			send_packet(ospf_adhoc_iface(od),buf, oh->len + sizeof(struct ether_header));
		}
	}
}

void ospf_recv_lsa_etx( ospf_d_etx *od, char* buf,int size){

	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_header* lh = (struct lsa_header*)(buf + sizeof(struct ether_header) + sizeof(struct ospf_header));
	struct lsa_list *lsas = NULL, *old_lsas = NULL, *tlsas=NULL;
	unsigned int s =0;
	struct lsa_body_etx* lb = NULL;
	struct lsa_entry_etx* le = NULL, *old_le,*tle = NULL;
	char* p;
	int i = 0;
	struct mesh_rte* rte = NULL;
	unsigned char tdrate;
	
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_recv_lsa_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	if ( keycmp( od->header.rid, lh->aid ) == 0) return;
	
	s = oh->len - sizeof(struct ospf_header);
	
	if ( oh->len != size - sizeof(struct ether_header) || lh->len != (unsigned int)s)
		return ;

	lsas = ospf_get_lsa_list_etx( od, lh->aid);
	
	if (lsas && lh->seq <= lsas->seq)
        	return;

	lsas = ospf_flush_lsa_list_etx( od, lh->aid);

	lsas->seq = lh->seq;
	p = (char*)lh;
	p += sizeof(struct lsa_header);
	lb = (struct lsa_body_etx*)p;
	s -= sizeof(struct lsa_header);
	
	while ( s ){
		le = (struct lsa_entry_etx*) malloc( sizeof(struct lsa_entry_etx));
                bzero( le , sizeof(struct lsa_entry_etx));
		memcpy(&le->body, lb, sizeof(struct lsa_body_etx));
		add_head( &lsas->lsa, NODE le);

		tle = NULL;
		/* caculate the etx ( etx = 1 / dr*dt) */
		tdrate = 0;
		
                if (le->body.type == NEIGH_AP){
			tlsas = ospf_get_lsa_list_etx(od, le->body.id);
			if (tlsas){
				tle = ospf_get_lsa_entry_etx(od, tlsas, lh->aid);
				if (tle) tdrate = tle->body.drate;
			}
			else OSPF_DEBUG("%d not find %d's lsas\n", od->header.rid.key[5], le->body.id.key[5]);
		}
		else 
                     tdrate = OSPF_HELO_WINDOW;

		if (tdrate > OSPF_HELO_WINDOW) 
			tdrate = OSPF_HELO_WINDOW;

		if (le->body.drate > OSPF_HELO_WINDOW) 
			le->body.drate = OSPF_HELO_WINDOW;
		
		if (tdrate == 0 || le->body.drate == 0)
			le->etx = 1000;
		else
			le->etx = OSPF_HELO_WINDOW * OSPF_HELO_WINDOW / (tdrate * le->body.drate);
		
		if (tle) {
			tle->etx = le->etx;
			OSPF_DEBUG("%d get %d's etx %f\n", od->header.rid.key[5], le->body.id.key[5],le->etx);
		}
		/* if lsa's type is NEIGH_MOBILE, check the other ap's lsa list */
		
		
		if (le->body.type == NEIGH_MOBILE){
			rte = rtb_find(od->route, le->body.id);
			if (rte && keycmp(rte->nexthop, lsas->id) != 0 && keycmp( lsas->id, le->body.id) != 0){
				old_lsas = ospf_get_lsa_list_etx(od, rte->nexthop);
				if (old_lsas){
					old_le = ospf_get_lsa_entry_etx(od, old_lsas, le->body.id);
					if (old_le){
						rem_node(NODE old_le);
						free(old_le);
					}
				}
				if ( keycmp( rte->nexthop, rte->target) == 0){
					struct neighbor *ne;
				
                                        if (old_le) {
	
					   for( old_le = (struct lsa_entry_etx*) HEAD(od->self_lsas);
						(NODE (old_le)); 
                                                   old_le =(struct lsa_entry_etx*) ((NODE (old_le))->next)) {

                                                  if ( !old_le )
 						      break;

                                                  if ( keycmp(old_le->body.id, le->body.id) == 0) 
                                                      break;
                                           
                                           }



					    if (old_le){
						rem_node(NODE old_le);
						//free(old_le);
					    }

					    ne = neigh_find(&od->neighbors, le->body.id);
					    if (ne) {
						rem_node(NODE ne);
						free(ne);
					    } 
                                        }
				}
			}
		}
		
	    //if (od->header.rid.key[5] == 3)
	    OSPF_DEBUG("%d recv mobile from %d\n", od->header.rid.key[5], le->body.id.key[5]);
		
		p += sizeof(struct lsa_body_etx);
		lb = (struct lsa_body_etx*)p;
		s -= sizeof(struct lsa_body_etx);
		i++;
	}

	//if (od->header.rid.key[5] == 3)
	OSPF_DEBUG("%d recv lsa from %d : %d entries\n", od->header.rid.key[5], lh->aid.key[5], i);
	ospf_forward_lsa_etx(od,buf,size);
	ospf_build_tree_etx(od);
}

void ospf_recv_lsa_ori( ospf_d_ori *od, char* buf,int size){

	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_header* lh = (struct lsa_header*)(buf + sizeof(struct ether_header) + sizeof(struct ospf_header));
	struct lsa_list *lsas = NULL, *old_lsas = NULL;
	int s;
	struct lsa_body_ori* lb = NULL;
	struct lsa_entry_ori* le = NULL, *old_le = NULL;
	char* p = NULL;
	int i = 0;
	struct mesh_rte* rte = NULL;


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_recv_lsa_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	

	if ( keycmp( od->header.rid, lh->aid ) == 0) return;
	
	s = (int)oh->len - sizeof(struct ospf_header);
	
	if ( oh->len != size - sizeof(struct ether_header) || lh->len != (unsigned int)s)
            return ;

	lsas = ospf_get_lsa_list_ori( od, lh->aid);
	
	if (lsas && lh->seq <= lsas->seq)return;

	lsas = ospf_flush_lsa_list_ori( od, lh->aid);

	lsas->seq = lh->seq;
	p = (char*)lh;
	p += sizeof(struct lsa_header);
	lb = (struct lsa_body_ori*)p;
	s -= sizeof(struct lsa_header);
	
	while ( s ){
		le = (struct lsa_entry_ori*) malloc( sizeof(struct lsa_entry_ori));
                bzero(le , sizeof(struct lsa_entry_ori));
		memcpy(&le->body, lb, sizeof(struct lsa_body_ori));
		add_head( &lsas->lsa, NODE le);

		/* if lsa's type is NEIGH_MOBILE, check the other ap's lsa list */
		
		
		if (le->body.type == NEIGH_MOBILE){
			rte = rtb_find(od->route, le->body.id);
			if (rte && keycmp(rte->nexthop, lsas->id) != 0 && keycmp( lsas->id, le->body.id) != 0){
				old_lsas = ospf_get_lsa_list_ori(od, rte->nexthop);
				if (old_lsas){
					old_le = ospf_get_lsa_entry_ori(od, old_lsas, le->body.id);
					if (old_le){
						rem_node(NODE old_le);
						free(old_le);
					}
				}
				if ( keycmp( rte->nexthop, rte->target) == 0){
					struct neighbor *ne;
					
					WALK_LIST ( old_le, od->self_lsas, struct lsa_entry_ori*) 
						if ( keycmp(old_le->body.id, le->body.id) == 0) break;
					if (old_le){
						rem_node(NODE old_le);
						free(old_le);
					}

					ne = neigh_find(&od->neighbors, le->body.id);
					if (ne) {
						rem_node(NODE ne);
						free(ne);
					}
				}
			}
		}
		
	    //if (od->header.rid.key[5] == 3)
	    OSPF_DEBUG("%d recv mobile from %d\n", od->header.rid.key[5], le->body.id.key[5]);
		
		p += sizeof(struct lsa_body_ori);
		lb = (struct lsa_body_ori*)p;
		s -= sizeof(struct lsa_body_ori);
		i++;
	}

	//if (od->header.rid.key[5] == 3)
	OSPF_DEBUG("%d recv lsa from %d : %d entries\n", od->header.rid.key[5], lh->aid.key[5], i);
	ospf_forward_lsa_ori(od,buf,size);
	ospf_build_tree_ori(od);
}


void ospf_recv_helo_etx( ospf_d_etx *od, char* buf,int size){
	
	struct neighbor* ne = NULL;
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_entry_etx* le = NULL;
	
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_recv_helo_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	//if (od->header.rid.key[5] == 11)
	//OSPF_DEBUG("%d recv helo from %d\n", od->header.rid.key[5], oh->rid.key[5]);
	ne = (struct neighbor*)neigh_find( &od->neighbors, oh->rid);
	if (!ne){
		ne = (struct neighbor*) malloc( sizeof(struct neighbor));
                bzero(ne, sizeof(struct neighbor));
		keycopy( ne->id, oh->rid);
		ne->iface = ospf_adhoc_iface(od);
		ne->state = 1;
		ne->type = NEIGH_AP;
		ne->snr = 0;
		neigh_add( &od->neighbors, ne);

		/* here need to add lsa */
		le = (struct lsa_entry_etx*) malloc( sizeof(struct lsa_entry_etx));
                bzero(le,sizeof(struct lsa_entry_etx));
		keycopy(le->body.id, oh->rid);
		le->body.drate = 1;
		/* disabled by jclin to prevent from segmenation faults
                   le->timeout = 0;
                */
		le->body.type = NEIGH_AP;
		add_head( &od->self_lsas, NODE le);
		
		ospf_flood_lsa_etx(od);
		ospf_build_tree_etx(od);
	}else{

		WALK_LIST(le, od->self_lsas, struct lsa_entry_etx*){

			if (keycmp(le->body.id, oh->rid) == 0){
				if(le->body.drate < OSPF_HELO_WINDOW) le->body.drate += 2;
				if (le->body.drate > OSPF_HELO_WINDOW) le->body.drate = OSPF_HELO_WINDOW;
			}
		}
	}
		
	
	ne->timestamp = 0; /* now */
	
}

void ospf_recv_helo_ori( ospf_d_ori *od, char* buf,int size){
	
	struct neighbor* ne;
	struct ospf_header* oh = (struct ospf_header*)(buf + sizeof(struct ether_header));
	struct lsa_entry_ori* le;
	
	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_recv_helo_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	printf("ospf_recv_helo_ori key = ");showmac(oh->rid.key);printf("\n");

	//if (od->header.rid.key[5] == 11)
	//OSPF_DEBUG("%d recv helo from %d\n", od->header.rid.key[5], oh->rid.key[5]);
	ne = (struct neighbor*)neigh_find( &od->neighbors, oh->rid);
	if (!ne){
		ne = (struct neighbor*) malloc( sizeof(struct neighbor));
		keycopy( ne->id, oh->rid);
		ne->iface = ospf_adhoc_iface(od);
		ne->state = 1;
		ne->type = NEIGH_AP;
		ne->snr = 0;
		neigh_add( &od->neighbors, ne);

		/* here need to add lsa */
		le = (struct lsa_entry_ori*) malloc( sizeof(struct lsa_entry_ori));
		keycopy(le->body.id, oh->rid);
		le->body.state = 1;
		le->body.type = NEIGH_AP;
		add_head( &od->self_lsas, NODE le);
		
		ospf_flood_lsa_ori(od);
		ospf_build_tree_ori(od);
	}
	
	ne->timestamp = 0; /* now */
	
}

/*
 *
 * OSPF LSA functions
 *
 */


struct lsa_list* ospf_get_lsa_list_etx( ospf_d_etx* od, keytype k){

	struct lsa_list* lsal;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_get_lsa_list_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	WALK_LIST( lsal, od->lsa_lists , struct lsa_list*){
		
		if (keycmp(lsal->id,k) == 0){
		
			struct lsa_entry_etx* le;
			int i = 0;

			WALK_LIST( le, lsal->lsa, struct lsa_entry_etx*) i++;
			//if (od->header.rid.key[5] == 4)
			//OSPF_DEBUG("%d get lsa list %d : %d entries\n", od->header.rid.key[5], k.key[5], i);
			return lsal;
		}
	}
	
	
			//if (od->header.rid.key[5] == 4)
			//OSPF_DEBUG("%d get lsa list %d : 0 entries\n", od->header.rid.key[5], k.key[5]);
	return NULL;
}

struct lsa_list* ospf_get_lsa_list_ori( ospf_d_ori* od, keytype k){

	struct lsa_list* lsal;


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_get_lsa_list_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	WALK_LIST( lsal, od->lsa_lists , struct lsa_list*){
		if (keycmp(lsal->id,k) == 0){
			struct lsa_entry_ori* le;
			int i = 0;

			WALK_LIST( le, lsal->lsa, struct lsa_entry_ori*) i++;
			//if (od->header.rid.key[5] == 4)
			//OSPF_DEBUG("%d get lsa list %d : %d entries\n", od->header.rid.key[5], k.key[5], i);
			return lsal;
		}
	}
	
	
			//if (od->header.rid.key[5] == 4)
			//OSPF_DEBUG("%d get lsa list %d : 0 entries\n", od->header.rid.key[5], k.key[5]);
	return NULL;
}

struct lsa_list* ospf_flush_lsa_list_etx( ospf_d_etx* od, keytype k){
	
	struct lsa_entry_etx *le = NULL,*ln = NULL;
	struct lsa_list *lsal = NULL;
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_flush_lsa_list_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	lsal = ospf_get_lsa_list_etx(od,k);

	/* if not exists, add it*/
	//if (od->header.rid.key[5] == 4)
	OSPF_DEBUG("%d flush lsa list %d\n", od->header.rid.key[5], k.key[5]);
	if ( lsal == NULL){
		lsal = (struct lsa_list*)malloc(sizeof(struct lsa_list));
                bzero(lsal, sizeof(struct lsa_list));
		keycopy(lsal->id,k);
		lsal->seq = 0;
		lsal->bseq = 0;
		init_list(&lsal->lsa);
		add_head( &od->lsa_lists, NODE lsal);
		return lsal;
	}

	WALK_LIST_DELSAFE( le, ln , lsal->lsa, struct lsa_entry_etx*){
		rem_node( NODE le);
		free(le);
	}
	return lsal;
}

struct lsa_list* ospf_flush_lsa_list_ori( ospf_d_ori* od, keytype k){

	struct lsa_entry_ori *le,*ln;
	struct lsa_list *lsal;
	
	lsal = ospf_get_lsa_list_ori(od,k);


	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_flush_lsa_list_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }	

	/* if not exists, add it*/
	//if (od->header.rid.key[5] == 4)
	OSPF_DEBUG("%d flush lsa list %d\n", od->header.rid.key[5], k.key[5]);
	if ( lsal == NULL){
		lsal = (struct lsa_list*)malloc(sizeof(struct lsa_list));
		keycopy(lsal->id,k);
		lsal->seq = 0;
		lsal->bseq = 0;
		init_list(&lsal->lsa);
		add_head( &od->lsa_lists, NODE lsal);
		return lsal;
	}

	WALK_LIST_DELSAFE( le, ln , lsal->lsa, struct lsa_entry_ori*){
		rem_node( NODE le);
                bzero( le, sizeof(le));
		free(le);
	}
	return lsal;
}

struct lsa_entry_etx* ospf_get_lsa_entry_etx( ospf_d_etx* od, struct lsa_list* l, keytype k){
	
	struct lsa_entry_etx *le;
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_get_lsa_entry_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	WALK_LIST(le, l->lsa, struct lsa_entry_etx*){
	
		if (keycmp(le->body.id, k) == 0){
			return le;
		}
	}
	
	return NULL;
}

struct lsa_entry_ori* ospf_get_lsa_entry_ori( ospf_d_ori* od, struct lsa_list* l, keytype k){
	
	struct lsa_entry_ori *le;
	
	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_get_lsa_entry_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	
	WALK_LIST(le, l->lsa, struct lsa_entry_ori*){
		if (keycmp(le->body.id, k) == 0){
			return le;
		}
	}
	return NULL;
}

/*
 *
 * OSPF Tree System
 * 
 */


void ospf_build_tree_etx( ospf_d_etx* od){
	
	struct lsa_list used; /* mark which node is already in the tree */
	struct lsa_entry_etx *le = NULL,*te = NULL,*tle = NULL;
	struct ospf_tnode_etx *root = NULL,*tp = NULL,*ts = NULL,*ots = NULL,*otp = NULL,*tptr = NULL;
	struct lsa_list *lsas = NULL,*tlsas = NULL;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_build_tree_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	ospf_flush_tree_etx(od);
	init_list(&used.lsa);

	//if (od->header.rid.key[5] == 3)
	//OSPF_DEBUG("%d build tree\n", od->header.rid.key[5]);

	/* add root entry first */
	le = (struct lsa_entry_etx*)malloc(sizeof(struct lsa_entry_etx));
        bzero( le , sizeof(struct lsa_entry_etx));
	keycopy( le->body.id, od->header.rid);
	add_head(&used.lsa, NODE le);

	root = (struct ospf_tnode_etx*)malloc(sizeof(struct ospf_tnode_etx));
        bzero( root , sizeof(struct ospf_tnode_etx));
	memcpy( &root->body, &le->body, sizeof(struct lsa_body_etx));
	root->parent   = NULL;
	root->child    = NULL;
	root->sibling  = NULL;
	root->rsibling = NULL;
	root->etx      = 0;

	le->tnode = root;

   
	tp = root;
//        printf("tp_root=%u child=%u \n",tp,tp->child);
	/* make first level tnode */

	WALK_LIST(te, od->self_lsas, struct lsa_entry_etx*){

                if (!te)
                    continue;

		if (ospf_get_lsa_entry_etx( od, &used, te->body.id))
		    continue;

		tlsas = ospf_get_lsa_list_etx(od,te->body.id);
		
		te->etx = 1000;
		
		if (tlsas){
		
			tle = ospf_get_lsa_entry_etx(od, tlsas, od->header.rid);
			
			if (tle){
			
				if (te->body.drate > OSPF_HELO_WINDOW)  
				    te->body.drate = OSPF_HELO_WINDOW;
				
				if (tle->body.drate > OSPF_HELO_WINDOW) 
				    tle->body.drate = OSPF_HELO_WINDOW;

				if (te->body.drate == 0 || tle->body.drate == 0)
					te->etx = 1000;
				else
					te->etx = OSPF_HELO_WINDOW * OSPF_HELO_WINDOW / (te->body.drate * tle->body.drate);
			}
		}

		if (te->etx == 0) 
		    te->etx = 1;

		//if (te->etx == 1000)continue;

		le = (struct lsa_entry_etx*)malloc(sizeof(struct lsa_entry_etx));
                bzero(le,sizeof(struct lsa_entry_etx));
		keycopy( le->body.id, te->body.id);
		add_head(&used.lsa, NODE le);
		
		ts = (struct ospf_tnode_etx*)malloc( sizeof(struct ospf_tnode_etx));
                bzero(ts,sizeof(struct ospf_tnode_etx));
		memcpy( &ts->body, &te->body, sizeof(struct lsa_body_etx));
		
		le->tnode = ts;
		le->etx   = te->etx;

		ts->etx   = te->etx;
		OSPF_DEBUG("%d first set %d %.3f %d\n", od->header.rid.key[5],te->body.id.key[5], te->etx,te->body.drate);
		
		ts->parent     = tp;
		ts->child      = NULL;
		ts->sibling    = tp->child;
		ts->rsibling   = NULL;
		
		if (tp->child) 
		    tp->child->rsibling = ts;
		
		tp->child = ts;
		//printf("tp_enter_walk_list_loop_indication.\n");
	}

        //printf("tp_0=%u child=%u\n", tp , tp->child);
	tp = tp->child;

        /*cclin: correct the possible buggy tree building process */
        //if ( (unsigned int)tp <= 0x1000 || (unsigned int)tp >= 0x1013000 ) 
        //     tp = NULL;
        
  //     printf("tp_1=%u child=%u \n", tp , tp?tp->child:0);

	while (tp != NULL){
	
		ots = NULL;
		otp = tp->parent;
		//printf("tp_2=%u parent=%u\n", tp , tp->parent);
		
		while (tp != NULL){

			//printf("tp_3=%u child=%u sibling=%u rsibling=%u \n" , tp , tp->child , tp->sibling , tp->rsibling);
                        //fflush(stdout);
			
			lsas = ospf_get_lsa_list_etx(od, tp->body.id);
			
			if (lsas){
		
			    WALK_LIST(te, lsas->lsa, struct lsa_entry_etx*){
					
			        double etx = 0.0;

			        if ( keycmp(te->body.id , tp->body.id) == 0 || keycmp(te->body.id , root->body.id) == 0  )
			            continue;
					
			        /* The etx of a used LSA list is the sum of every hop's etx. */
			        etx  = te->etx;
			        tptr = tp;
					
			        while(tptr){
				    etx += tptr->etx;
				    tptr = tptr->parent;
			        }

			        if (etx == 0.0)
			            etx = 1.0;
					
			        le = ospf_get_lsa_entry_etx( od, &used, te->body.id) ;
					
			        if (le && (etx < le->etx) && (le->tnode != root) ) {
					    
				    if (le->tnode->parent->child == le->tnode){
					
				        if( (!le->tnode->sibling) || (le->tnode->sibling->parent != le->tnode->parent) )
						    
					    le->tnode->parent->child = NULL;
					else
						    
					    le->tnode->parent->child = le->tnode->sibling;
			            }
						
				    if (ots == le->tnode)
				        ots = le->tnode->rsibling;
						
				    OSPF_DEBUG("%d remove %d %.3f < %.3f sibling = %d, rsibling = %d\n", 
					od->header.rid.key[5],le->body.id.key[5], etx , le->etx, 
					le->tnode->sibling? le->tnode->sibling->body.id.key[5]:0, 
					le->tnode->rsibling? le->tnode->rsibling->body.id.key[5]:0);
				

                                    if ( le->tnode == otp )
					otp = NULL;
	    
				    //ospf_free_tree_etx(le->tnode,&used, &ots);
				    //rem_node(NODE le);
				    //free(le);
				    //le = NULL;
					
				}

				if (!le){

				    le = (struct lsa_entry_etx*)malloc(sizeof(struct lsa_entry_etx));
                                    bzero(le, sizeof(struct lsa_entry_etx));
				    keycopy( le->body.id, te->body.id);
				    add_head(&used.lsa, NODE le);
				    le->etx = etx;	


				    ts = (struct ospf_tnode_etx*)malloc( sizeof(struct ospf_tnode_etx));
				    bzero(ts, sizeof(struct ospf_tnode_etx) );		
				    memcpy( &ts->body, &te->body, sizeof(struct lsa_body_etx));
					    
				    ts->parent = tp;
				    ts->child = NULL;
				    ts->sibling = NULL; 
				    ts->rsibling = ots;
				    ts->etx = te->etx;

				    le->tnode = ts;

				    if(ots != NULL){
					ots->sibling = ts;
				    }
				
				    if (tp->child == NULL)
				        tp->child = ts;
					
				    OSPF_DEBUG("%d set %d %.3f parent = %d , sibling %d, rsibling %d\n", 
				            od->header.rid.key[5],le->body.id.key[5], le->etx, tp->body.id.key[5],
					    ts->sibling? ts->sibling->body.id.key[5]:0, 
					    ts->rsibling? ts->rsibling->body.id.key[5]:0);
				
				    ots = ts;
				} 
			    }
		        }

		    tp = tp->sibling;
    //                printf("tp = tp->sibling %u \n" , (unsigned int)tp );
	        }

		/* find next level */
                if ( otp ) {
		    tp = otp->child;
            //        printf("otp = %u , otp->child = %u , tp = %u", otp , otp->child , tp );
                }
                else 
	            tp = NULL;
	
                
          //      printf("tp = %u \n" , (unsigned int)tp );
	
		while (tp && tp->child == NULL && tp->sibling != NULL) {
 
		    tp = tp->sibling;
                     
      //              printf("tp = tp->sibling =  %u \n" , (unsigned int)tp );
                }

		if (tp) { 
		    tp = tp->child;
                    
        //            printf("tp = tp->child =  %u \n" , (unsigned int)tp );
                }
		
	}

	WALK_LIST_DELSAFE( le, te, used.lsa, struct lsa_entry_etx*){
		
		rem_node( NODE le);
                bzero(le,sizeof(le));
           //     printf(" le = %u \n", le);
		free(le);
	
	}

	od->tree = root;
	
	ospf_make_route_etx(od);

	//if (od->header.rid.key[5] == 1)
	//OSPF_DEBUG("%d build tree end\n", od->header.rid.key[5]);
}
	//
void ospf_build_tree_ori( ospf_d_ori* od){
	
        struct lsa_list used; /* mark which node is already in the tree */
	struct lsa_entry_ori *le,*te;
	struct ospf_tnode_ori *root,*tp,*ts,*ots,*otp;
	struct lsa_list *lsas;
	
	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_build_tree_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }	

	ospf_flush_tree_ori(od);
	init_list(&used.lsa);

	//if (od->header.rid.key[5] == 3)
	//OSPF_DEBUG("%d build tree\n", od->header.rid.key[5]);

	/* add root entry first */
	le = (struct lsa_entry_ori*)malloc(sizeof(struct lsa_entry_ori));
        bzero(le,sizeof(struct lsa_entry_ori));
	keycopy( le->body.id, od->header.rid);
	add_head(&used.lsa, NODE le);

	root = (struct ospf_tnode_ori*)malloc(sizeof(struct ospf_tnode_ori));
        bzero(root, sizeof(struct ospf_tnode_ori));
	memcpy( &root->body, &le->body, sizeof(struct lsa_body_ori));
	root->parent  = NULL;
	root->child   = NULL;
	root->sibling = NULL;

	tp = root;

	/* make first level tnode */

	WALK_LIST(te, od->self_lsas, struct lsa_entry_ori*){
		//if(root->body.id.key[1]==0xc)
		//	printf("build tree:sibiling [%x]\n",te->body.id.key[1]);

		if (ospf_get_lsa_entry_ori( od, &used, te->body.id))continue;

		le = (struct lsa_entry_ori*)malloc(sizeof(struct lsa_entry_ori));
                bzero(le,sizeof(struct lsa_entry_ori));
		keycopy( le->body.id, te->body.id);
		add_head(&used.lsa, NODE le);
		
		ts = (struct ospf_tnode_ori*)malloc( sizeof(struct ospf_tnode_ori));
                bzero(ts,sizeof(struct ospf_tnode_ori));
		memcpy( &ts->body, &te->body, sizeof(struct lsa_body_ori));
		ts->parent = tp;
		ts->child = NULL;
		ts->sibling = tp->child;
		tp->child = ts;
	}

	tp = tp->child;

	while (tp != NULL){
		ots = NULL;
		otp = tp;
		while (tp != NULL){
			
			lsas = ospf_get_lsa_list_ori(od, tp->body.id);
			if (lsas){
				WALK_LIST(te, lsas->lsa, struct lsa_entry_ori*){
				
					if (ospf_get_lsa_entry_ori( od, &used, te->body.id) == NULL){
					
						le = (struct lsa_entry_ori*)malloc(sizeof(struct lsa_entry_ori));
						bzero(le,sizeof(struct lsa_entry_ori));
						keycopy( le->body.id, te->body.id);
						add_head(&used.lsa, NODE le);
						
						ts = (struct ospf_tnode_ori*)malloc( sizeof(struct ospf_tnode_ori));
						bzero(ts,sizeof(struct ospf_tnode_ori));
						memcpy( &ts->body, &te->body, sizeof(struct lsa_body_ori));
						ts->parent = tp;
						ts->child = NULL;
						ts->sibling = NULL; 
						if(ots != NULL)ots->sibling = ts;
						if (tp->child == NULL)tp->child = ts;
						ots = ts;
					}
				}
			}
			
			tp = tp->sibling;
		}

		/* find next level */
		tp = otp->parent->child;
		while (tp->child == NULL && tp->sibling != NULL) tp = tp->sibling;

		if (tp) tp = tp->child;
		
	}

	WALK_LIST_DELSAFE( le, te, used.lsa, struct lsa_entry_ori*){
		rem_node( NODE le);
		free(le);
	}

	od->tree = root;
	
	ospf_make_route_ori(od);

	//if (od->header.rid.key[5] == 1)
	//OSPF_DEBUG("%d build tree end\n", od->header.rid.key[5]);
}

void ospf_flush_tree_etx( ospf_d_etx *od){

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_flush_tree_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	if (od->tree)ospf_free_tree_etx(od->tree,NULL,NULL);
	od->tree = NULL;
}

void ospf_flush_tree_ori( ospf_d_ori *od){

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_flush_tree_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }	

	if (od->tree)ospf_free_tree_ori(od->tree);
	od->tree = NULL;
}

void ospf_free_tree_etx(struct ospf_tnode_etx* tn, struct lsa_list* l, struct ospf_tnode_etx** ots){
	
        struct ospf_tnode_etx *tc = tn->child, *ts = NULL;
	struct lsa_entry_etx* le = NULL;
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_free_tree_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	if (ots && *ots == tn) 
	    *ots = tn->rsibling;
		
	while ( tc != NULL && tc->parent == tn) {
		ts = tc->sibling;
		ospf_free_tree_etx(tc,l,ots);
		tc = ts;
	}

	if (tn->rsibling) tn->rsibling->sibling = tn->sibling;
	if (tn->sibling) tn->sibling->rsibling = tn->rsibling;

		
	if (l){
		le = ospf_get_lsa_entry_etx( NULL, l, tn->body.id);
		if(le){
			rem_node(NODE le);
                        bzero(le,sizeof(le));
			free(le);
		}
		
	}

        bzero(tn,sizeof(tn));
        //printf("free tn (1) = %u \n",tn);
	free(tn);
}

void ospf_free_tree_ori(struct ospf_tnode_ori* tn){

	struct ospf_tnode_ori *tc = tn->child, *ts;
        

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_free_tree_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }        
        
	while ( tc != NULL && tc->parent == tn) {
		ts = tc->sibling;
		ospf_free_tree_ori(tc);
		tc = ts;
	}
	free(tn);
}

void ospf_make_route_etx(ospf_d_etx *od){ 
	
        struct mesh_rte *rte;
	struct ospf_tnode_etx *tn = od->tree, *ts, *tc;
	struct mesh_rtb* rtb = od->route;
	struct neighbor* ne;
	
	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_make_route_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	ts = tn->child;
	while (ts != NULL){
		rte = rtb_find(rtb, ts->body.id);
		if (!rte){
			rte = (struct mesh_rte*) malloc( sizeof( struct mesh_rte));
			rtb_add(rtb, rte);
		}

		ne = neigh_find( &od->neighbors, ts->body.id);
		rte->type = ts->body.type;
		keycopy( rte->target ,ts->body.id);
		keycopy( rte->nexthop ,ts->body.id);
		rte->hop = 1;
		rte->timestamp = 0;
		rte->weight = 1;
		rte->iface = ne->iface;

		tc = ts->child;
		while ( tc != NULL && tc->parent == ts){
			ospf_set_route_etx( od, rtb, tc, rte);
			tc = tc->sibling;
		}
		ts = ts->sibling;
	}
}

void ospf_make_route_ori(ospf_d_ori *od){ 
	
	struct mesh_rte *rte;
	struct ospf_tnode_ori *tn = od->tree, *ts, *tc;
	struct mesh_rtb* rtb = od->route;
	struct neighbor* ne;
	
	
	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_make_route_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }
	
	ts = tn->child;

//	printf("\nroot=[%x]\n",tn->body.id.key[1]);	
	while (ts != NULL){
//		printf("neighbor=[%x]\n",ts->body.id.key[1]);	
		rte = rtb_find(rtb, ts->body.id);
		if (!rte){
			rte = (struct mesh_rte*) malloc( sizeof( struct mesh_rte));
			rtb_add(rtb, rte);
		}

		ne = neigh_find( &od->neighbors, ts->body.id);
		rte->type = ts->body.type;
		keycopy( rte->target ,ts->body.id);
		keycopy( rte->nexthop ,ts->body.id);
		rte->hop = 1;
		rte->timestamp = 0;
		rte->weight = 1;
		rte->iface = ne->iface;

		tc = ts->child;
		while ( tc != NULL && tc->parent == ts){
//			printf("set route: TN[%x] TS[%x] TC[%x] ",tn->body.id.key[1], ts->body.id.key[1],tc->body.id.key[1]);
			ospf_set_route_ori( od, rtb, tc, rte);
			tc = tc->sibling;
		}
		ts = ts->sibling;
	}
}

void ospf_set_route_etx ( ospf_d_etx *od, struct mesh_rtb* rtb, struct ospf_tnode_etx *tn, struct mesh_rte* prte){

	struct ospf_tnode_etx *tc;
	struct mesh_rte *rte;


	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_set_route_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	rte = rtb_find(rtb, tn->body.id);
	if (rte){
		rtb_del(rte);
	}else
		rte = (struct mesh_rte*) malloc(sizeof(struct mesh_rte));

	memcpy(rte, prte, sizeof(struct mesh_rte));
	keycopy( rte->target, tn->body.id);
	rte->hop++;
	rtb_add(rtb,rte);

	tc = tn->child;
	while ( tc != NULL && tc->parent == tn ){
		ospf_set_route_etx(od, rtb, tc, rte);
		tc = tc->sibling;
	}
}

void ospf_set_route_ori ( ospf_d_ori *od, struct mesh_rtb* rtb, struct ospf_tnode_ori *tn, struct mesh_rte* prte){

	struct ospf_tnode_ori *tc;
	struct mesh_rte *rte;

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_set_route_ori(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	rte = rtb_find(rtb, tn->body.id);
	if (rte){
		rtb_del(rte);
	}else
		rte = (struct mesh_rte*) malloc(sizeof(struct mesh_rte));

	memcpy(rte, prte, sizeof(struct mesh_rte));
	keycopy( rte->target, tn->body.id);
	rte->hop++;
//	printf("\trte->hop=%d\n",rte->hop);
	rtb_add(rtb,rte);

	tc = tn->child;
	while ( tc != NULL && tc->parent == tn ){
//		printf("\tTN[%x] TC[%x] ",tn->body.id.key[1], tc->body.id.key[1]);
		ospf_set_route_ori(od, rtb, tc, rte);
		tc = tc->sibling;
	}
}

/*
 *
 * OSPF misc functions
 *
 */

struct neighbor* ospf_new_neighbor_etx( ospf_d_etx *od, keytype id, struct mesh_if* iface){

	struct neighbor*       ne;
	struct lsa_entry_etx*  le;
	struct mesh_rte*       rte;
	struct lsa_list*       lsas;

	if ( (get_version())!= OSPF_ETX ) {
            printf("ospf_new_neighbor_etx(): an etx version library function is called without correct flag value.\n");
            exit(1);
        }

	ne = neigh_find( &od->neighbors, id);
	if (!ne){
		ne = (struct neighbor*)malloc( sizeof(struct neighbor));
		keycopy( ne->id ,id);
		ne->iface = iface;
		ne->state = 1;
		ne->type = NEIGH_MOBILE;
		ne->snr = 0;
		neigh_add( &od->neighbors, ne);

		le = (struct lsa_entry_etx*) malloc( sizeof(struct lsa_entry_etx));
		keycopy(le->body.id, id);
		le->body.drate = OSPF_HELO_WINDOW;
		le->body.type = NEIGH_MOBILE;
		add_head( &od->self_lsas, NODE le);
		if ( keycmp ( id, od->header.rid) != 0){
			OSPF_DEBUG("%d new neighbor %d\n", od->header.rid.key[5], id.key[5]);
			ospf_flood_lsa_etx(od);
			rte = rtb_find(od->route, id);
			if (rte && keycmp(rte->nexthop, id) != 0){
				lsas = ospf_get_lsa_list_etx(od, rte->nexthop);
				le = NULL;
				if (lsas){
					le = ospf_get_lsa_entry_etx(od, lsas, id);
					if (le){
						rem_node(NODE le);
						bzero(le,sizeof(le));
						free(le);
					}
				}
			}
		}
		
			
		ospf_build_tree_etx(od);
	}
	return ne;
}

struct neighbor* ospf_new_neighbor_ori( ospf_d_ori *od, keytype id, struct mesh_if* iface){
	
	struct neighbor*        ne = NULL;
	struct lsa_entry_ori*   le = NULL;
	struct mesh_rte*        rte = NULL;
	struct lsa_list*        lsas = NULL;

	//printf("ospf_new_neighbor_ori:id[%x]\n",id.key[1]);

	if ( (get_version())!= OSPF_ORI ) {
            printf("ospf_new_neighbor_ori(): an etx version library function is called without correct flag value.\n");
            printf("version = %d \n", get_version() );
            exit(1);
        }

	ne = neigh_find( &od->neighbors, id);
	if (!ne ){
		ne = (struct neighbor*)malloc( sizeof(struct neighbor));
		keycopy( ne->id ,id);
		ne->iface = iface;
		ne->state = 1;
		ne->type = NEIGH_MOBILE;
		ne->snr = 0;
		neigh_add( &od->neighbors, ne);

		le = (struct lsa_entry_ori*) malloc( sizeof(struct lsa_entry_ori));
		keycopy(le->body.id, id);
		le->body.state = 1;
		le->body.type = NEIGH_MOBILE;
		add_head( &od->self_lsas, NODE le);
		if ( keycmp ( id, od->header.rid) != 0){
			OSPF_DEBUG("%d new neighbor %d\n", od->header.rid.key[5], id.key[5]);
			ospf_flood_lsa_ori(od);
			rte = rtb_find(od->route, id);
			if (rte && keycmp(rte->nexthop, id) != 0){
				lsas = ospf_get_lsa_list_ori(od, rte->nexthop);
				le = NULL;
				if (lsas){
					le = ospf_get_lsa_entry_ori(od, lsas, id);
					if (le){
						rem_node(NODE le);
						free(le);
					}
				}
			}
		}
		
			
		ospf_build_tree_ori(od);
	}
	return ne;
}

int mesharp_ospf_helo_ori( struct mesh_if* if_p){

        char buf[sizeof(struct ospf_header)+sizeof(struct helo_header)];
        char* ptr_p             = buf;
        struct ospf_header* oh  = (struct ospf_header*)ptr_p;
        struct helo_header* hh  = (struct helo_header*)(ptr_p + sizeof(struct ospf_header));
        int   buf_size = sizeof(buf);


        oh->ver  = 1;
        oh->type = OSPF_HELO;
        oh->len  = sizeof(buf);
        keycopy( oh->rid , if_p->name ); 

        hh->interval = 3;

        printf("mesharp_ospf_helo: bufsize = %d send to ", buf_size);showmac(if_p->name.key);printf("\n");

        return broadcast_packet( if_p , buf , buf_size );


}


