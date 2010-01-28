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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <object.h>
#include <event.h>
#include <regcom.h>
#include <scheduler.h>
#include <nodetype.h>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <nctuns_api.h>
#include <ethernet.h>
#include <ip.h>
#include <packet.h>
#include "dsdvd.h"

using namespace DSDV;

MODULE_GENERATOR(DSDVd);
 
#ifdef DUMPROUTINGTABLE

char *itoa(u_int64_t a){
    char *buf; 
    u_int64_t base = 10;
    int i = 1;
    for( ;; ){        
        if( a< base)
             break; 
        i++;
        base = base*10; 
    }
    buf = new char[++i];
    buf[i] = '\0';
    base = base /10;
    int quotient, remainder;
    i = 0;
    do{
        quotient = a / base;
        switch( quotient ){
            case 0: 
                buf[i] = '0'; break;
            case 1:
                buf[i] = '1'; break;    
            case 2:
                buf[i] = '2'; break;
            case 3:
                buf[i] = '3'; break;
            case 4:
                buf[i] = '4'; break;
            case 5:
                buf[i] = '5'; break;
            case 6:
                buf[i] = '6'; break;
            case 7:
                buf[i] = '7'; break;
            case 8:
                buf[i] = '8'; break;
            case 9:
                buf[i] = '9'; break;
        }
        remainder = a % base;
        
        a = remainder;
        i++;
        base = base /10 ;            
    }while( remainder != 0 );

    return buf;   
}
#endif
 
 
RtTable::RtTable(){	
	len = 0;
	maxlen = 10;
	now_counter = 0;
	//regular_counter =0;
	rtptr = new rt_unit[maxlen];
} 

int RtTable::addUnit(const rt_unit *unit)
{
	//assert( unit->metric <= BIG );

        // check to see if the dst is already exist
	for( int i=0; i<len; i++ ){
		if( rtptr[i].dst == unit->dst ){
			memcpy( &rtptr[i], unit, sizeof(rt_unit) );
			return 1;
		}
	}
	// it is a new unit, add it 
	if( len == maxlen ){
		// routing table is full
		rt_unit *tmp = rtptr;
		maxlen *= 2;
		rtptr = new rt_unit[maxlen];
		memcpy( rtptr, tmp, sizeof(rt_unit)*len );
		delete [] tmp;
	}
	memcpy( &rtptr[len], unit, sizeof(rt_unit) );
	len++;
	
	return 1;
}

int RtTable::getIndex(u_long dst){

	for(int i=0; i<len; i++){
		if( rtptr[i].dst == dst ){
			return i;    // return index value
		}
	}
	return(-1);	
}

int RtTable::updateUnit(int index , rt_unit *r){
	// according to index, update the data that rtptr point to
	if( index >=0 && index < len ){
		memcpy( &rtptr[index] ,r, sizeof(rt_unit) );
		return 1;
	}
	return -1;
}  

int RtTable::rebuild(){
	
	rt_unit *tmp = rtptr;
	int 	j = 0;
	
	rtptr = new rt_unit[maxlen];
	for(int i=0; i<len; i++){
			if( tmp[i].expire_flag != 1){
			memcpy(&rtptr[j], &tmp[i], sizeof(rt_unit));
			j++;
		}
	}
	len = j;
	delete [] tmp;
	return len;	
}

#ifdef DUMPROUTINGTABLE

void RtTable::dump(FILE *fp){
	
	char	dst[20], nextHop[20], sign_host[20];
	u_int64_t	time;	
	SEC_TO_TICK(time, 1);

	rt_unit	 *r = rtptr;
	
	fprintf(fp,"dump routing table information\n");
	fprintf(fp,"     ===============================================================\n");
	fprintf(fp,"    %8s | %8s | %7s | %8s | %8s | %10s\n", "dst", "nextHop", "metric", "seq_num", "sign_","install"); 
	for(int i=0; i<len; i++){
		ipv4addr_to_str( r[i].dst, dst );
		ipv4addr_to_str( r[i].nextHop, nextHop );
		ipv4addr_to_str( r[i].seq_num.sign_host, sign_host );
		
		fprintf(fp,"    %8s | %8s | %7d | %8d | %8s | %10.3f\n", dst, nextHop, r[i].metric, r[i].seq_num.num, sign_host, r[i].install_time/(float)time); 
	}	
	fprintf(fp,"     ===============================================================\n");
	return ;
}

#endif

DSDVd::DSDVd(u_int32_t type, u_int32_t id, struct plist* pl , const char *name)
		: NslObject(type, id, pl, name)
{
       	s_flowctl = DISABLED;
	r_flowctl = DISABLED; 
	
	// initial sequence count
	seq = 0;

	// bind vabiable
	vBind("adv_interval", &adv_interval);
	vBind("expire_timeout", &expire_time_out);

#ifdef DUMPROUTINGTABLE
	char *nodeid = itoa(get_nid());
        string file_name(nodeid);
        file_name += ".log";
    
        if( (fp=fopen(file_name.c_str(), "a+"))==NULL ){
   	    printf("open file error\n");
            exit(1);
        }
	fprintf(fp,"This is node %s rorting information log file\n", nodeid);
        delete nodeid;
#endif
}


DSDVd::~DSDVd() {
#ifdef DUMPROUTINGTABLE
	fclose(fp);
#endif
}


int DSDVd::init() {

	printf("	init()\n"); 	
	
	/* get my ip address */
	mip = GET_REG_VAR(get_port(), "IP", u_long *);  	 
	
	
	// fill local information in routing table
     	rt_unit  r ;
     	r.dst = *mip;
     	r.nextHop = *mip;
     	r.metric = 0;
     	r.seq_num.num = seq;
     	r.seq_num.sign_host = *mip;     	
     	r.install_time = GetNodeCurrentTime(get_nid());
     	r.need_adv_now = 0;
	//r.need_adv_regular = 0;
	r.expire_flag = 0;
     	seq +=2;
     
     	rt_table.addUnit(&r);
     	
    
	/* transfer expire time out unit from second to tick */
	MILLI_TO_TICK(expire_time_out_, (u_int64_t)expire_time_out);
      	
     	long _rand, _timer;

/*
     	srandom( GetNodeCurrentTime(get_nid()) );
*/
     	_rand = random() % 1000;
	_rand -= 500;
	_timer = adv_interval + _rand; 
       	/* transfer adv interval unit from second to tick */
        MILLI_TO_TICK(adv_interval_, (u_int64_t)_timer);

     	// set timer function to regularly broadcast advertisment msg
     	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(DSDVd, sendAdvPktRegular);
	IF_timer.setCallOutObj(this, type);
	IF_timer.start(adv_interval_, 0);	
     
	return(1); 
}

int DSDVd::sendAdvPktRegular() {
	
	rt_unit *r = rt_table.getHead();
	int len_;
	
	ePacket_	*pkt;
	CREATE_EVENT(pkt);
	
	Packet *pkt_ = new Packet();
	pkt_->rt_setgw(inet_addr("1.0.1.255"));
	
	len_ = rt_table.getLen();
	
	int data_len_ = sizeof(struct ip) 
			+ sizeof(dsdv_hdr) + sizeof(rt_unit)*len_ ;
	char *data = pkt_->pkt_sattach(data_len_);
	if(pkt_->pkt_sprepend(data, data_len_) < 0){
		// make sure sattach is success, and plus p_len
		return(-1);
	}	
	char *ptr = data;
	
	// alloc a ip_header fill in the ip data
	struct ip ip_data_ ;
	ip_data_.ip_src = *mip;
	ip_data_.ip_dst = inet_addr("1.0.1.255");
	memcpy(ptr, &ip_data_ , sizeof(struct ip));
        ptr += sizeof(struct ip);

 	// alloc a dsdv hdr and fill data in
 	dsdv_hdr dsdv_hdr_;
 	dsdv_hdr_.len = len_;
 	memcpy(ptr, &dsdv_hdr_, sizeof(dsdv_hdr));	
	ptr += sizeof(dsdv_hdr);
			
	// first, add seq_num, then fill it in the adv_msg
	r[0].dst = *mip;
	r[0].nextHop = *mip;
	r[0].metric = 0;
	r[0].seq_num.num = seq;
	seq +=2;
	r[0].seq_num.sign_host = *mip;
	r[0].install_time = GetNodeCurrentTime(get_nid());
	r[0].need_adv_now = 0;
	//r[0].need_adv_regular = 0;
	r[0].expire_flag = 0;
	
	memcpy( ptr, &r[0], sizeof(rt_unit) );
	ptr += sizeof(rt_unit);
	
	// fill in other routing information
		// send full dump pkt 
	for( int i=1; i<rt_table.getLen(); i++ ){
		memcpy( ptr, &r[i], sizeof(rt_unit) );
		//r[i].need_adv_regular = 0;
		ptr += sizeof(rt_unit);
	}	
	
	// attach packet(pkt_) to event(pkt) 
	SET_EVENT_CALLOUTOBJ(pkt, NULL, NULL, (void *)pkt_);


     	long _rand, _timer;
/*
     	srandom( GetNodeCurrentTime(get_nid()) );
*/
     	_rand = random() % 1000;
	_rand -= 500;
	_timer = adv_interval + _rand; 
       	/* transfer adv interval unit from second to tick */
        MILLI_TO_TICK(adv_interval_, (u_int64_t)_timer);

	// set timer function to regularly broadcast advertisment msg
     	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(DSDVd, sendAdvPktRegular);
	IF_timer.setCallOutObj(this, type);
	IF_timer.start(adv_interval_, 0);

 	selfCheck();
	return(put(pkt, sendtarget_));
}

int DSDVd::sendAdvPktNow(){
	
	ePacket_ 	*pkt;
	CREATE_EVENT(pkt); 
	
	Packet *pkt_ = new Packet();
	pkt_->rt_setgw(inet_addr("1.0.1.255"));
	
	int len = sizeof(struct ip) + sizeof(dsdv_hdr) +
			sizeof(rt_unit) * (rt_table.getNowCounter() + 1);

	char *data = pkt_->pkt_sattach(len); 
	if(pkt_->pkt_sprepend(data, len) < 0){
		// make sure sattach is success, and plus p_len
		return(-1);
	}	
	char *ptr = data;
	
	// alloc a ip_header fill in the ip data
	struct ip ip_data_ ;
	ip_data_.ip_src = *mip;
	ip_data_.ip_dst = inet_addr("1.0.1.255");
	
	memcpy(ptr, &ip_data_ , sizeof(struct ip));
        ptr += sizeof(struct ip);
	
	// fill in len and data
	dsdv_hdr dsdv_hdr_;
	dsdv_hdr_.len = (rt_table.getNowCounter() + 1);
	memcpy(ptr, &dsdv_hdr_, sizeof(dsdv_hdr));
	ptr += sizeof(dsdv_hdr);
	
	rt_unit *r = rt_table.getHead();
	memcpy(ptr, r, sizeof(rt_unit));
	ptr += sizeof(rt_unit);
	
	for(int i=1; i<rt_table.getLen(); i++){
		if( r[i].need_adv_now ==1 ){
			memcpy(ptr , &r[i], sizeof(rt_unit));
			r[i].need_adv_now = 0;
			ptr += sizeof(rt_unit);
		}
	}
	
	// attach packet(pkt_) to event(pkt) 
	SET_EVENT_CALLOUTOBJ(pkt, NULL, NULL, (void *)pkt_);

	return(put(pkt, sendtarget_));
}

int DSDVd::recvAdvPkt(Packet *p) {
	// recv DSDV advertisement pkt, update routing table
	
	int _flag;
	assert(p);
	char *p_data = p->pkt_sget();
		
	rt_unit *r = (rt_unit *)(p_data + sizeof(struct ip) + sizeof(dsdv_hdr));	
	dsdv_hdr *dsdv_hdr_ = (dsdv_hdr*)(p_data + sizeof(struct ip));
	int len_ = dsdv_hdr_->len;;
	
	u_long from ;
	IP_SRC(from, p_data);

	for(int i=0; i<len_; i++){
		update(from, r);
		r++;
	}
	
	// go through the whole routing table, label stale entry
	u_int64_t  now = GetNodeCurrentTime(get_nid()) ;
	u_int64_t  diff;
	r = rt_table.getHead();

	_flag = 0;
	for(int i=0; i<rt_table.getLen(); i++){
		diff = now - r[i].install_time; 
		if( diff > expire_time_out_ ){
			if( r[i].dst == r[i].nextHop ){
				// this dst is our neighbor, 
				// keep this entry and bcast its dead, we only bcast once to prevent bcast storm
				if( r[i].already_bcast == 0){
					r[i].metric = BIG;
					r[i].seq_num.num ++;
					r[i].seq_num.sign_host = *mip;
					r[i].need_adv_now =1;
					r[i].already_bcast = 1;
					r[i].install_time = GetNodeCurrentTime(get_nid());
					rt_table.addNowCounter(); 
				}else{
					// we have already bcast this bad news once,
					// and it is too old, so we decide to delete this entry
					_flag = 1;
					r[i].expire_flag = 1;				
				}
			}else{
				_flag = 1;
				r[i].expire_flag = 1;
			}	
		}
	}
	// if our neighbor is dead, we need to bcast its death immediately
	if( rt_table.getNowCounter() > 0){
		sendAdvPktNow();
	}
	rt_table.flushNowCounter();
	
	// delete stale entry
	if( _flag == 1 )
		rt_table.rebuild();
	
	return 1;
}

#ifdef DUMPROUTINGTABLE

int DSDVd::dumpAdvPkt(Packet *p){

	char src[20], dst[20], nextHop[20], sign_host[20];
	rt_unit *r;
	u_int64_t	time;	
	SEC_TO_TICK(time, 1);
	
	assert(p);
	u_long src_ ;
	IP_SRC(src_, p->pkt_sget());
	ipv4addr_to_str(src_, src);
	
	dsdv_hdr *dsdv_hdr_ = (dsdv_hdr*)(p->pkt_sget() + sizeof(struct ip));	
	int len = dsdv_hdr_->len;
						
	r = (rt_unit *)(p->pkt_sget() + sizeof(struct ip) + sizeof(dsdv_hdr));
	
	
	fprintf(fp,"dump adv pkt msg,");
	fprintf(fp,"this pkt's src ip is %s, and it has %d rt_unit\n", src, len);
	fprintf(fp,"    -----------------------------------------------------------------\n");
	fprintf(fp,"    %8s | %8s | %7s | %8s | %8s | %10s\n", "dst", "nextHop", "metric", "seq_num", "sign_", "install"); 
	
	
	for(int i=0; i<len ;i++){
		ipv4addr_to_str(r->dst, dst);
		ipv4addr_to_str(r->nextHop, nextHop);
		ipv4addr_to_str(r->seq_num.sign_host, sign_host);
		fprintf(fp,"    %8s | %8s | %7d | %8d | %8s | %10.3f\n", dst, nextHop, r->metric, r->seq_num.num, sign_host, r->install_time/(float)time); 
		r++;
	}
	fprintf(fp,"    -----------------------------------------------------------------\n");
	return 1; 
}
#endif

int DSDVd::update(u_long from, const rt_unit *a){
	
	rt_unit  b;
 	int 	e,f,g,h;
	char 	dst[20];
	int 	index ;
	
	ipv4addr_to_str( a->dst, dst );
	sscanf(dst,"%d.%d.%d.%d", &e, &f, &g, &h);
		
	if( a->dst == *mip ){
		return 1; 
	}	
	if( a->dst == a->seq_num.sign_host ){
		// path is ok.			
		if( (index = rt_table.getIndex(a->dst)) < 0 ){
			// we have no path to dst, add it to routing table
 			b.dst = a->dst;
			b.nextHop = from;
			b.metric = a->metric+1;
			b.seq_num.num = a->seq_num.num;
			b.seq_num.sign_host = a->seq_num.sign_host;
			b.install_time = GetNodeCurrentTime(get_nid());
			b.need_adv_now = 0;
			b.expire_flag = 0;
			rt_table.addUnit(&b);
			return(1);
		}
	 	// we already has a path to dst.
	 	
		b = rt_table.getUnit(index);
		// check our path seq fresh
		if( a->seq_num.num > b.seq_num.num ){
			// incoming is more fresh, get it.
			b.dst = a->dst;
			if( b.nextHop != from){
				// nextHop will change, adv it now
				b.need_adv_now = 1;
				rt_table.addNowCounter();
			}else{ 
				b.need_adv_now = 0;
			}
			b.nextHop = from;
			b.metric = a->metric+1;
			b.seq_num.num = a->seq_num.num;
			b.seq_num.sign_host = a->seq_num.sign_host;
			b.install_time = GetNodeCurrentTime(get_nid());
			b.expire_flag = 0;
			rt_table.updateUnit(index, &b);
			return(1);
		}else if( a->seq_num.num == b.seq_num.num ){
			// fresh is the same, check metric
			if( (a->metric+1) < b.metric ){
				// it is a shorter path, get it
				b.dst = a->dst;
				if( b.nextHop != from ){
					b.need_adv_now = 1;
					rt_table.addNowCounter();
				}else{
					b.need_adv_now = 0;
				}
				b.nextHop = from;
				b.metric = a->metric+1;
				b.seq_num.num = a->seq_num.num;
				b.seq_num.sign_host = a->seq_num.sign_host;
				b.install_time = GetNodeCurrentTime(get_nid());
				b.expire_flag = 0;
				rt_table.updateUnit(index, &b);
			}
		}
	}else if( a->dst != a->seq_num.sign_host ){
		// from adv pkt, we know a broken path,
		// check to see if we use this path	
		if( (index = rt_table.getIndex(a->dst)) > 0){ 	
			// the path to the dst is in our routing path
			b = rt_table.getUnit(index);	
			if( b.nextHop == from ){
				// we use this path, we should mark this way is down
				// and bcast this bad news.
				// remember , we just bcst once to prevent bcats storm
				if( b.already_bcast == 0 ){
					b.dst = a->dst;
					b.nextHop = from;
					b.metric = BIG;
					b.seq_num.num = a->seq_num.num;
					b.seq_num.sign_host = a->seq_num.sign_host;
					b.install_time = a->install_time;
					b.need_adv_now = 1;
					b.expire_flag = 1;
					b.already_bcast = 1;
					rt_table.addNowCounter();
					rt_table.updateUnit(index, &b);
				}	
			}	
		}
	}			
	return 1;		
}

int DSDVd::send(ePacket_ *pkt) {

 	Packet			*p; 
	u_long			dst;

	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	/* get destination ip address */
	IP_DST(dst, p->pkt_sget()); 

	/* filter destination address */
	if((*mip == dst)||is_ipv4_broadcast(get_nid() , dst)) {
		/* if the destination == myip, pass
		 * to upper layer
		 */
		return(put(pkt, recvtarget_));   
	}
	else {	/* otherwise, we should modify the flow 
		 * direction and do routing.
		 */
		p->pkt_setflow(PF_SEND); 
  		if (routing(dst, p) < 0) {
			freePacket(pkt);
			return(1); 
		}

		return(put(pkt, sendtarget_));  
	}  
}

int DSDVd::recv(ePacket_ *pkt) {

	Packet			*p;
	u_long			dst;
	dsdv_hdr 		*dsdv_hdr_;
	struct ip		*ip;
	
	assert(pkt);
	p = (Packet *)pkt->DataInfo_;
	ip = (struct ip*)p->pkt_sget();
	dsdv_hdr_ = (dsdv_hdr*)(p->pkt_sget() + sizeof(struct ip));
	
	// check incoming pkt, if it is a dsdv pkt, we should do something.
	if( strcmp(dsdv_hdr_->name, "dsdv") == 0 ){
		
		u_int64_t time;
	        SEC_TO_TICK(time, 1);
	        
#ifdef DUMPROUTINGTABLE	        
		fprintf(fp,"Current Time is %10.3fsecs\n",GetCurrentTime()/(float)time );
		Debugger();
		dumpAdvPkt(p);
		rt_table.dump(fp);
#endif
		recvAdvPkt(p);
		freePacket(pkt);
		
#ifdef DUMPROUTINGTABLE	        
		fprintf(fp,"complete routing information change\n");
		rt_table.dump(fp);
		fprintf(fp,"\n\n\n");
#endif
		return(1);
	}

	// it is a normal pkt	
	/* get destination ip address */
	IP_DST(dst, p->pkt_sget()); 

        // recv a pkt , ttl-1 and check ttl value, 
        // if ttl == 0 , just send it to upper layer, 
        // kernel will create a icmp packet and send it back to the sender.
/* hwchu: moved below
        --ip->ip_ttl;
        
        if(ip->ip_ttl == 0){
        	return(put(pkt, recvtarget_));
        }
*/

	/* filter destination address */
	if((*mip == dst)||is_ipv4_broadcast(get_nid(), dst)) {
		/* if the destination == myip, pass
		 * to upper layer
		 */
		return(put(pkt, recvtarget_));   
	}
	else {	/* otherwise, we should modify the flow 
		 * direction and do routing.
		 */

		/* hwchu:
		 *   There is no chance for this packet to enter the kernel, 
		 *   so we decrement its TTL here.
		 */
		u_char ttl;

		GET_IP_TTL(ttl, p->pkt_sget());
		if (ttl <= 1) {
			return put(pkt, recvtarget_);
		}
		IP_DEC_TTL(p->pkt_sget());

		p->pkt_setflow(PF_SEND); 
  		if (routing(dst, p) < 0) {
			freePacket(pkt);
			return(1); 
		}

		return(put(pkt, sendtarget_));  
	}	
}

int DSDVd::selfCheck(){
	
	int _flag ;
	// go through the whole routing table, label stale entry
	rt_unit *r;
	u_int64_t  now = GetNodeCurrentTime(get_nid()) ;
	u_int64_t  diff;
	r = rt_table.getHead();

	_flag = 0;
	for(int i=0; i<rt_table.getLen(); i++){
		diff = now - r[i].install_time; 
		if( diff > expire_time_out_ ){
			// this entry is too old, expire it
			if( r[i].dst == r[i].nextHop ){
				// this dst is our neighbor, 
				// bcast its dead, we only bcast once to prevent bcast storm
				if( r[i].already_bcast == 0 ){
					r[i].metric = BIG;
					r[i].seq_num.num ++;
					r[i].seq_num.sign_host = *mip;
					r[i].need_adv_now =1;
					r[i].already_bcast = 1;
					r[i].install_time = GetNodeCurrentTime(get_nid());
					rt_table.addNowCounter(); 
				}else{
					// we have already bcast this bad news once,
					// and it is too old, so we decide to delete this entry
					_flag = 1;
					r[i].expire_flag = 1;		
				}
			}else{
				_flag = 1;
				r[i].expire_flag = 1;			
			}	
		}
	}
	
	// if our neighbor is dead, we need to bcast its death immediately
	if( rt_table.getNowCounter() > 0){
		sendAdvPktNow();
		rt_table.flushNowCounter();
	}
	
	// delete stale entry
	if( _flag == 1 ){
		rt_table.rebuild();
	}	
#ifdef DUMPROUTINGTABLE		
	u_int64_t time;
	SEC_TO_TICK(time, 1);	
	fprintf(fp,"	self check routing table\n");        
	fprintf(fp,"Current Time is %10.3fsecs\n",GetCurrentTime()/(float)time );
	Debugger();
	rt_table.dump(fp);	
#endif
	return 1;	
}

int DSDVd::routing(u_long dst, Packet *pkt) {
	
	rt_unit 		*r, tmp;
 	int 			index;
 	
	r = rt_table.getHead();

	if( (index = rt_table.getIndex(dst)) ){
 		tmp = rt_table.getUnit(index);
 		if( tmp.metric == BIG ){
 			return(-1);
 		}
 		pkt->rt_setgw(tmp.nextHop); 
   			return(1); 
	}  
	else{
		return(-1); 
	}
}


int DSDVd::Debugger() {
	char			ip[20];

	ipv4addr_to_str(*mip, ip); 
	
#ifdef DUMPROUTINGTABLE	        
	fprintf(fp,"	my ip: %s\n", ip);
#else	
	printf("	my ip: %s\n", ip);
#endif
	return(1);  
 
}
