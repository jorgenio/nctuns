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
//#include <ethernet.h>
#include <ip.h>
#include <udp.h>
#include <tcp.h>
#include <packet.h>

#include "ruleMgr.h"
#include "tokenbucket.h"
#include "debug.h"

const char* Rule::className = "Rule";
const char* RuleMgr::className = "RuleMgr";

Rule::Rule(char* sip,char* dip, char* srcPort,char* dstPort, char* ptc, char* phb){
	
	// parsing string ip address into u_long form
	strcpy(_sip,sip);
	strcpy(_dip,dip);
	strcpy(_srcPort,srcPort);
	strcpy(_dstPort,dstPort);
	strcpy(_ptc,ptc);
	strcpy(_phb,phb);

	_codepoint = phbToDSCP(_phb);

	debug = 0;	
}


Rule::~Rule(){
}

void Rule::show(){
	if(DEBUG_LEVEL == MSG_OFF )
		return;

	printf("Rule: %s %s %s %s %s %s %d\n", _sip, _dip, _srcPort, _dstPort, _ptc, _phb, _codepoint);	
}

unsigned char Rule::phbToDSCP(const char* phb){

	unsigned char codepoint;			// for one byte
	if(strcmp(phb,"EF")==0)
		codepoint = 184;	// '101110'00
	else if(strcmp(phb,"AF11")==0)
		codepoint = 40;		// '001010'00
	else if(strcmp(phb,"AF12")==0)
		codepoint = 48;		// '001100'00
	else if(strcmp(phb,"AF13")==0)
		codepoint = 56;		// '001110'00
	else if(strcmp(phb,"AF21")==0)
		codepoint = 72;		// '010010'00
	else if(strcmp(phb,"AF22")==0)
		codepoint = 80;		// '010100'00
	else if(strcmp(phb,"AF23")==0)
		codepoint = 88;		// '010110'00
	else if(strcmp(phb,"AF31")==0)
		codepoint = 104;	// '011010'00
	else if(strcmp(phb,"AF32")==0)
		codepoint = 112;	// '011100'00
	else if(strcmp(phb,"AF33")==0)
		codepoint = 120;	// '011110'00
	else if(strcmp(phb,"AF41")==0)
		codepoint = 136;	// '100010'00
	else if(strcmp(phb,"AF42")==0)
		codepoint = 144;	// '100100'00
	else if(strcmp(phb,"AF43")==0)
		codepoint = 152;	// '100110'00
	else if(strcmp(phb,"NC")==0)
		codepoint = 224;	// '111000'00
	else if(strcmp(phb,"BE")==0)
		codepoint = 0;	// '000000'00
	else{
		showMsg(MSG_WARNING,"phdToDSCP","WARNING Rule, phbToDSCP(): no matched!! \n");
		if(DEBUG_LEVEL >= MSG_WARNING)
			printf("\tphb=%s\n",phb);

		codepoint = 0;		// '000000'00 Best Effort and other else.
	}
	return codepoint;	
}


u_long Rule::transformIP(const char* ipstr,unsigned& mask){
	
	char* _ipstr=(char*)malloc(strlen(ipstr)+1);
	strcpy(_ipstr,ipstr);
	
	u_long ipval = 0;
	mask =0;
	char* ipItem =(char*)&ipval;
	char* maskItem = (char*)&mask;


	int i;
	char* token;
	for(token = strtok(_ipstr,"."), i = 0 ; token != NULL ; token = strtok(NULL,"."), i++){
		if( strcmp(token,"*") == 0)
			break;
		ipItem[i] = atoi(token);
		maskItem[i] = 0xFF;
	}
		
	free(_ipstr);
	
	return ipval;
}

bool Rule::match(ePacket_* pkt){

	struct ip* iph;
	Packet *p;
	unsigned short sport;
	unsigned short dport;
	unsigned sip;
	unsigned dip;
	unsigned short ptc;

	
	GET_PKT(p,pkt);
	
	iph = (struct ip*) p->pkt_sget();
	sip = iph->ip_src;
	dip = iph->ip_dst;
	ptc = iph->ip_p;


	// TCP / UDP or neither
	if(iph->ip_p==6){
	  	struct tcphdr* tcph=(struct tcphdr *)((char *)iph+sizeof(struct ip));
		sport = tcph->th_sport;
		dport = tcph->th_dport;
	}else if((int)iph->ip_p==17){
		struct udphdr* udph=(struct udphdr *)((char *)iph+sizeof(struct ip));
		sport = udph->uh_sport;
		dport = udph->uh_dport;
	}else{
		sport = 0 ;
		dport = 0 ;
	}
	
	return match(sip,dip,sport,dport,ptc);
	 
}


bool Rule::match(unsigned sip,unsigned dip, short srcPort, short dstPort, short ptc){

	// sip
	if(strcmp(_sip,"*") != 0){
		unsigned mask;
		unsigned i_sip = transformIP(_sip,mask);
		if( (i_sip & mask) != (sip & mask) ){
			return false;
		}
	}
	
	// dip
	if(strcmp(_dip,"*") != 0){
		unsigned mask;
		unsigned i_dip = transformIP(_dip,mask);
		if( (i_dip & mask) != (dip & mask) )
			return false;
	}

	// source port
	if(strcmp(_srcPort,"*") != 0 && srcPort != atoi(_srcPort))
		return false;
	
	// destination port
	if(strcmp(_dstPort,"*") != 0 && dstPort != atoi(_dstPort))
		return false;
	
	// protocol
	if(strcmp(_ptc,"*") != 0 && ptc != atoi(_ptc))
		return false;
	
	return true;
}

unsigned char Rule::getCodepoint(){
	return _codepoint;
}



void Rule::showMsg(int msgType, const char* funcName, const char* message){

	if(message == NULL)
		return;

	if(DEBUG_LEVEL >= msgType){
		switch(msgType){
		case MSG_DEBUG:
			printf("DEBUG");
			break;
		case MSG_WARNING:
			printf("WRANING");
			break;
		case MSG_ERROR:
			printf("ERROR");
			break;
		case MSG_FATAL:
			printf("FATAL ERROR");
			break;
		default:
			return;
		}

		printf(" %s",className);
		if(funcName)	printf("::%s()",funcName);

		if(message){
			if(strlen(message)>40 )
				printf(":\n\t%s",message);
			else
				printf(": %s",message);
		}
		printf("\n");
	}
}


//----------------------------------------------------------------------

Rule* RuleMgr::getRule(ePacket_* pkt){
	RuleLink* rl = rules_head;
	

	while(rl != NULL){
		if( rl->rule->match(pkt) == true )
			break;
		rl = rl->nextLink;
	}
	
	if(rl != NULL)
		return rl->rule;
	else
		return NULL;
}


Tokenbucket* RuleMgr::getTokenbucket(ePacket_* pkt){
		
	RuleLink* rl = rules_head;

	while(rl != NULL){
		if( rl->rule->match(pkt) == true )
			break;
		rl = rl->nextLink;
	}
	
	if(rl != NULL)
		return rl->tb;
	else
		return NULL;
}

Rule* RuleMgr::getRule(unsigned sip,unsigned dip, short srcPort, short dstPort, short ptc){

	RuleLink* rl = rules_head;
	while(rl != NULL){
		if( rl->rule->match(sip,dip,srcPort,dstPort,ptc) == true )
			break;
		rl = rl->nextLink;
	}

	if(rl != NULL)
		return rl->rule;
	else
		return NULL;
}

RuleMgr::RuleMgr(){
	debug = 0;	
	rules_head = NULL;
	rules_tail = NULL;
	_count = 0 ;
}

RuleMgr::~RuleMgr(){

	// free rules link space
	RuleLink* rl;
	while(rules_head != NULL){
		rl = rules_head;
		rules_head = rules_head -> nextLink;

		delete (rl -> rule) ;
		delete (rl -> tb);
		delete rl;
	}
	rules_tail = NULL;
}

void RuleMgr::addRule(char* sip,char* dip,char* sport,char* dport,char* ptc,char* phb,Tokenbucket* tb){
	
	RuleLink* rl = new RuleLink();
	rl -> rule = new Rule(sip,dip,sport,dport,ptc,phb);
	rl -> tb = tb;
	rl -> nextLink = NULL;
	
	if(rules_head == NULL){
		rules_head = rl;
		rules_tail = rl;
	}else{
		(rules_tail -> nextLink) = rl;
		rules_tail = rl;	
	}

	_count++;
}

int RuleMgr::getRuleNumber(){
	return _count;
}

void RuleMgr::giveToken(){
	RuleLink* rl = rules_head;

	while(rl != NULL){
		if(rl->tb != NULL)
			rl->tb->getToken();

		rl = rl -> nextLink;
	}
}

void RuleMgr::show(){

	if(DEBUG_LEVEL == MSG_OFF )
		return;

	RuleLink* rl = rules_head;

	printf("RuleMgr show()------------------------------------------\n");
	while(rl != NULL){
		rl -> rule -> show();

		if(rl->tb == NULL)
			printf("Tokenbucket: NULL\n");
		else
			rl->tb->show();

		rl = rl -> nextLink;
	}
	printf("End------------------------------------------------------\n");
}


void RuleMgr::showMsg(int msgType,char* funcName,char* message){

	if(message == NULL)
		return;

	char className[]="RuleMgr";

	if(DEBUG_LEVEL >= msgType){
		switch(msgType){
		case MSG_DEBUG:
			printf("DEBUG");
			break;
		case MSG_WARNING:
			printf("WRANING");
			break;
		case MSG_ERROR:
			printf("ERROR");
			break;
		case MSG_FATAL:
			printf("FATAL ERROR");
			break;
		default:
			return;
		}

		printf(" %s",className);
		if(funcName)	printf("::%s()",funcName);

		if(message){
			if( strlen(message) > 40 )
				printf(":\n\t%s",message);
			else
				printf(": %s",message);
		}
		printf("\n");
	}
}
//-------------------------------------------------------------------------------------------------
