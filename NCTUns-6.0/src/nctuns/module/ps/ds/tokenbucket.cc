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
#include <ip.h>
#include <nctuns_api.h>

#include "tokenbucket.h"
#include "tc.h"
#include "debug.h"

#ifdef LINUX
#include <netinet/in.h>
#endif
/*
 * rate : Mbps
 * size : Mbit
 * ql :	  packets
 * */

Tokenbucket::Tokenbucket(float rate,float size,int maxIN,char* phb,TrafCond* owner){

	_tokenUnit = (unsigned int)((rate * 100 * 1000) / 8);	/* rate(Mbps) -> byte/0.1sec */
	_size = (unsigned int)(size * 125000);					/* Mbit -> byte */	
	_maxIN = maxIN;
	_maxQL = _maxIN;
	_in = 0 ;
	_ql = 0 ;
	_owner = owner;

	_tokens = 0 ;
	_head = NULL;
	_tail = NULL;

	_2ndCodePoint = 0;
#ifdef REMARK
	_remark = true;
#else
	_remark = false;
#endif


	if(_remark == true){
		_2ndCodePoint = phbTo2ndDSCP(phb);
		if(phb[0]=='A' && phb[1]=='F'){
			_maxQL = 4 * _maxIN;
		}else if(phb[0]=='B' && phb[1]=='E'){
			_maxQL = 2 * _maxIN;
		}
		sprintf(_ofp_tag,"N%dP%d",_owner->get_nid(),_owner->get_port());
	}

	_debug=0;

}

Tokenbucket::~Tokenbucket(){

}
	
/*
 * return value:
 * 		 false)		can't put pkt in
 * 		  true)		ok
 * */
bool Tokenbucket::input(ePacket_* pkt){
	assert(pkt&&pkt->DataInfo_);
	if(enqueue(pkt) == -1)
		return false;
	
	tryOutput();
	return true;
}

void Tokenbucket::getToken(){
	if(_tokens + _tokenUnit <= _size)
		_tokens += _tokenUnit;
	tryOutput();
}


void Tokenbucket::show(){
	
	if( DEBUG_LEVEL == MSG_OFF )
		return;

	if(_remark == true)
		printf("Tokenbucket: size(%d byte), tokenUnit(%d byte), tokens(%d), maxQL(%d), QL(%d), maxIn(%d), IN(%d), debug(%d)\n"
						,_size,_tokenUnit,_tokens,_maxQL,_ql,_maxIN,_in,_debug);
	else
		printf("Tokenbucket: size(%d byte), tokenUnit(%d byte), tokens(%d), maxIn(%d), IN(%d), debug(%d)\n"
						,_size,_tokenUnit,_tokens,_maxIN,_in,_debug);
}

void Tokenbucket::show(char* message){

	if( DEBUG_LEVEL == MSG_OFF )
		return;

	if(_remark == true)
			printf("Tokenbucket: [%s] size(%d byte), tokenUnit(%d byte), tokens(%d), maxQL(%d), QL(%d), maxIn(%d), IN(%d), debug(%d)\n"
						,message,_size,_tokenUnit,_tokens,_maxQL,_ql,_maxIN,_in,_debug);
	else
			printf("Tokenbucket: [%s] size(%d byte), tokenUnit(%d byte), tokens(%d), maxIn(%d), IN(%d), debug(%d)\n"
						,message,_size,_tokenUnit,_tokens,_maxIN,_in,_debug);
}

void Tokenbucket::tryOutput(){

	if( _head == NULL )
		return;

	ePacket_* pkt;
	struct ip *iph;
	Packet *p;
	u_short ip_len ;

	pkt = _head;
	GET_PKT(p,pkt);
	iph = (struct ip*) p->pkt_sget();
	ip_len = ntohs(iph->ip_len); 
	if( _remark && ((p->pkt_getinfo(_ofp_tag))!=NULL) ){
		ip_len = 0;
	}

	while(ip_len <= _tokens){
		_tokens -= (ip_len);
		pkt = dequeue();
		_owner->sendPkt(pkt);	
		
		if(_head == NULL)
			break;

		pkt = _head;
		GET_PKT(p,pkt);
		iph = (struct ip*) p->pkt_sget();
		ip_len = ntohs(iph->ip_len);
		if( _remark && ((p->pkt_getinfo(_ofp_tag))!=NULL) ){
			ip_len = 0;
		}

	}
}



int Tokenbucket::enqueue(ePacket_* pkt){
	assert(pkt&&pkt->DataInfo_);

	if(_ql >= _maxQL)
		return -1;

	if(_in >= _maxIN){
		// out-of-profile
		Packet *p;
		GET_PKT(p,pkt);
		int tmp =1;
		p->pkt_addinfo(_ofp_tag,(char*)&tmp, sizeof(int)); //arrive time of DS module
		remark(pkt);	
	}else{
		_in++;
	}
	_ql++;

	pkt->next_ep = NULL;
	if( _head == NULL)
		_head = pkt;
	else
		_tail -> next_ep = pkt; 
	_tail = pkt;

	return 0;
}

ePacket_* Tokenbucket::dequeue(){
	if(_ql == 0)
		return NULL;
	_ql--;

	ePacket_* pkt = _head;
	_head = _head->next_ep;
	pkt -> next_ep = NULL;

	Packet *p;
	GET_PKT(p,pkt);
	if((p->pkt_getinfo(_ofp_tag))==NULL)
		_in--;

	return pkt;
}


unsigned char Tokenbucket::phbTo2ndDSCP(const char* phb){

	unsigned char codepoint;			// for one byte
	if(strcmp(phb,"AF11")==0)
		codepoint = 48;		// '001100'00 AF12
	else if(strcmp(phb,"AF21")==0)
		codepoint = 80;		// '010100'00 AF22
	else if(strcmp(phb,"AF31")==0)
		codepoint = 112;	// '011100'00 AF32
	else if(strcmp(phb,"AF41")==0)
		codepoint = 144;	// '100100'00 AF42
	else{
		codepoint = 0;		// '000000'00 BE
	}
	return codepoint;	
}

void Tokenbucket::remark(ePacket_* pkt){

	assert(pkt&&pkt->DataInfo_);
	struct ip *iph;
	Packet *p;
	GET_PKT(p,pkt);
	iph = (struct ip *)p->pkt_sget();

    iph -> ip_tos = _2ndCodePoint;
}

