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
#include "dsqueue.h"
#include <time.h>
#include <ethernet.h>

#ifdef LINUX
#include <netinet/in.h>
#endif

DSQueue::DSQueue(const char* name,int maxLen,int rate,int nid,int port){

	assert(name);
	_name = strdup(name);

	_maxLength = maxLen;
	_rate = rate;
	_length = 0 ;

	_account = 0 ;

	_head = NULL;
	_tail = NULL;

	_logLength = false;
	_logDrop = false;
	_drops = 0 ;
	_pass = 0 ;
	logLenFile = NULL;
	logDropFile = NULL;
	
	DDDshowIt = false;
	_DDDdebug = 0;
	_DDDnid = nid;
	_DDDport = port;
}

DSQueue::~DSQueue(){
	if (logLenFile)
		fclose(logLenFile);
	if (logDropFile)
		fclose(logDropFile);
	if (_name)
		free(_name);
}

void DSQueue::enableLogLength(char* const filePath){
	_logLength = true;
	logLenFile = fopen(filePath,"w+");
	assert(logLenFile);
}

void DSQueue::logLength(){
	
	if(_logLength == false )
		return;
	
	double		current_time;
	current_time = GetCurrentTime() * TICK / 1000000000.0;
	fprintf(logLenFile, "%.3f\t%d\n", current_time, getLength() );

}

void DSQueue::enableLogDrop(char* filePath){
	_logDrop = true;

	logDropFile = fopen(filePath,"w+");

	assert(logDropFile);
}

void DSQueue::logDrop(){
	
	if(_logDrop == false)
		return;

	double		current_time;
	current_time = GetCurrentTime() * TICK / 1000000000.0;
	int	dropRate = 0 ;
	if(_pass != 0)
		dropRate = (int)((_drops * 100) / _pass) ;
	
	fprintf(logDropFile, "%.3f\t%d\t%d\t%d\n", current_time, dropRate, _drops, _pass);

	// reset
	_drops = 0 ;
	_pass = 0 ;
}

int DSQueue::enqueue(ePacket_* pkt){
        
	_pass++;
	
	if(isFull()){
		_drops++;
		return -1;
	}

	_length++;

	pkt->next_ep = NULL;
	if( _head == NULL)
		_head = pkt;
	else
		_tail -> next_ep = pkt; 
	_tail = pkt;
				
	return 0;	

}

ePacket_* DSQueue::getNext(){
	return _head;
}


ePacket_* DSQueue::dequeue(){
		

	//double		DDDcurrent_time;
	//DDDcurrent_time = GetCurrentTime() * TICK / 1000000000.0;

	if(_length == 0){
		_account = 0;
		return NULL;
	}
	
    struct ether_header *eh;
	ePacket_* pkt;
	struct ip *iph;
	Packet *p;
	u_short ip_len ;

	pkt = _head;
    GET_PKT(p,pkt);							//	assert((pkt)&&((p)=(Packet *)(pkt)->DataInfo_));


    eh=(struct ether_header *)p->pkt_get();
    /* non-ip packet in NC queue, dequeue it directly */
    if(ntohs(eh->ether_type)!=0x0800){
		_length --;
		_head = _head -> next_ep;
		pkt -> next_ep=NULL;
		return pkt;
    }

	iph = (struct ip*) p->pkt_sget();
	ip_len = ntohs(iph->ip_len); 
	if(ip_len > _account ){
		return NULL;
	}

	_length --;
	_account -= ip_len;

	_head = _head -> next_ep;
	pkt -> next_ep=NULL;
	
	return pkt;
}

void DSQueue::addAccount(){
	if(_account < _accountUnit * 10)
		_account += _accountUnit;
}
	
void  DSQueue::show(){
	printf("DSQueue - name(%s), MaxLen(%d), rate(%d), Len(%d)\n"
					,_name, _maxLength,_rate,_length);
}

void DSQueue::showName(){
	printf("DSQueue - name(%s)\n",_name);
}
//-------------------------------------------------------

AFQueue::AFQueue(char* name,int maxLen,int rate, int ts1, int ts2, int maxDR,int nid,int port)
					: DSQueue(name,maxLen,rate,nid,port)
{
	_ts1 = ts1;
	_ts2 = ts2;
	_maxDR = maxDR;

	_drop1 = 0;
	_drop2 = 0;
	_pass1 = 0;
	_pass2 = 0;

	logDrop1File = NULL;
	logDrop2File = NULL;
	
}

int AFQueue::enqueue(ePacket_* pkt){
	return enqueue(pkt,1);
}

/*
 * type = 0(low drop precedence), >=1(high drop precedence) 
 * */
int AFQueue::enqueue(ePacket_* pkt,int type){
		
	if(type == 0){
		_pass1++;
	}
	else{
		_pass2++;
	}


	if(isFull()){
		if(type == 0)
			_drop1++;
		else
			_drop2++;
		
		return -1;
	}
	
	float dropR = 0;
	
	if(_length < _ts1){
		return DSQueue::enqueue(pkt);
	}
	
	if((_length < _ts2) && (type == 0)){
		return DSQueue::enqueue(pkt);
	}

	int temp;	
	if(type > 0){
		dropR = (_length - _ts1) / (float)(_maxLength - _ts1);
		temp = random() % 10000 ;

		if( temp < dropR * 10000){
			_drop2++;
			return -1;	
		}
		else{
			return DSQueue::enqueue(pkt);
		}
	}
	
	dropR = ((_length - _ts2) / (float)(_maxLength - _ts2)) * _maxDR / 100;
	temp = random() % 10000;
	
	if( temp < dropR * 10000 ){
		_drop1++;
		return -1;		// drop it
	}else{
		return DSQueue::enqueue(pkt);
	}
}
 
void AFQueue::show(){
	printf("AFQueue - name(%s), MaxLen(%d), rate(%d), Len(%d), ts1(%d), ts2(%d), maxDR(%d)\n"
					,_name,_maxLength,_rate,_length,_ts1,_ts2,_maxDR);
}


void AFQueue::showName(){
	printf("AFQueue - name(%s)\n",_name);
}


/*
 * d0_filePath : drop rate log file for low drop precedence phb, Ex. AF11 
 * d1_filePath : drop rate log file for high drop precedence phb, Ex. AF12 
 * */

void AFQueue::enableLogDrop(char* const d0_filePath, char* const d1_filePath){
	_logDrop = true;

	logDrop1File = fopen(d0_filePath,"w+");
	logDrop2File = fopen(d1_filePath,"w+");

	assert(logDrop1File);
	assert(logDrop2File);
}

void AFQueue::logDrop(){
	
	if(_logDrop == false)
		return;

	double		current_time;
	current_time = GetCurrentTime() * TICK / 1000000000.0;
	int	dropRate = 0 ;

	if(_pass1 != 0)
		dropRate = (int)((_drop1 * 100) / _pass1) ;
	fprintf(logDrop1File, "%.3f\t%d\n", current_time, dropRate);

	dropRate = 0;
	if(_pass2 != 0)
		dropRate = (int)((_drop2 * 100) / _pass2) ;
	fprintf(logDrop2File, "%.3f\t%d\n", current_time, dropRate);
	
	// reset
	_drops = 0 ;
	_pass = 0 ;
}


