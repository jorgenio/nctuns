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
#include <nctuns_api.h>
#include <ps/DS/tc.h>
#include "debug.h"

#ifdef LINUX
#include <netinet/in.h>
#endif

MODULE_GENERATOR(TrafCond);

const char* TrafCond::className = "TraifCond";

TrafCond::TrafCond(u_int32_t type, u_int32_t id,struct plist *pl, const char *name)
                : NslObject(type,id,pl, name) 
{

	/* disable flow control */
    s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	vBind("ds_domain", &ds_domain);


}

TrafCond::~TrafCond() {
	delete ruleMgr;
}


int TrafCond::init() {

	/* for debug */
	debug = 0;	
	DDDshowMe = false;
	if(DEBUG_LEVEL >= MSG_DEBUG && get_nid() == DDDEBUG_SHOW_NODE){
		DDDshowMe = true;
		int tmp = DDDEBUG_SHOW_NODE;
		printf("DEBUG TrafCond::init() Set show node  = N%d\n",tmp);
	}

	/* init RuleMgr */
	ruleMgr = new RuleMgr();

	char* dsFilePath = (char*) malloc(strlen(GetConfigFileDir()) + 50);
	sprintf(dsFilePath,"%sdsdmdf.txt",GetConfigFileDir());
	parseDSFile(dsFilePath);
	free(dsFilePath);

	/* init tokenGiver timer */
	
	u_int64_t period;
	MILLI_TO_TICK(period, 100);		// every 0.1 sec
	
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(TrafCond, giveToken);
	tokenGiver.setCallOutObj(this, type);
	tokenGiver.start(period, period);
	
	return 1;
}

void TrafCond::giveToken(){
	ruleMgr->giveToken();
}
void TrafCond::parseDSFile(char* dsFilePath){
 	
 	FILE* fp;
	fp = fopen(dsFilePath,"r");

	if(fp == NULL){
		char * msg = (char*) malloc(strlen(dsFilePath) + 100);
		sprintf(msg,"Can't open ds config file: \n\t\t %s",dsFilePath);
		showMsg(MSG_FATAL,"parseDSFile",msg);
		free(msg);
		exit(-1);
	}
	
	char line[128];
	char cmd[40];
	char sip[16];		// "xxx.xxx.xxx.xxx\0"
	char dip[16];
	char sport[6];		// source port 
	char dport[6];		// destination port 
	char ptc[4];		// protocol
	char phb[6];
	
	float tbRate;		// tokenbucket rate
	float tbSize;		// tokenbucket size
	unsigned int tbQL;		// tokenbucket waitqueue length

	bool start = false;	// if the domain information is what we wanted, set start true.
	char dsName[16] = "";
	
	int r;			// a temp value to record return value from 'sscanf'

	
	while(!feof(fp)){
		// reset
		line[0] = '\0';
		sip[0] = '\0';
		dip[0] = '\0';
		sport[0] = '\0';
		dport[0] = '\0';
		ptc[0] = '\0';
		phb[0] = '\0';
		tbRate = 0;
		tbSize = 0;
		tbQL = 50;
		
		
		while((line[0]=='\0' || line[0]=='#') && !feof(fp))
			fgets(line,127,fp);

		//r = sscanf(line,"%s",&cmd);
		r = sscanf(line,"%s",cmd);
		if(r < 0)
			continue;
		
		if( strcasecmp(cmd,"DefineDSDomain") == 0)
			start = false;

		else if( strcasecmp(cmd,"EndDomainDefine") == 0 )
			start = false;
		
		else if( strcasecmp(cmd,"Name") == 0 ){
			//sscanf(line,"%s %s",&cmd,&dsName);
			sscanf(line,"%s %s",cmd,dsName);

			if( strcasecmp(dsName,ds_domain) == 0)
				start = true;
		}
		
		else if( strcasecmp(cmd,"RuleAdd") == 0){
			if(start == false)
				continue;

			//	RuleAdd [sip] 	[dip] 	[sport]	[dport]	[ptc]	[PHB]	[R:Mbps][S:Mb]	(ql) 
			r = sscanf(line,"%s  %s  %s  %s  %s  %s  %s  %f  %f  %d",
					 cmd,sip, dip, sport, dport, ptc, phb, &tbRate, &tbSize, &tbQL);
			
			//pprintf("Debug:: %s  %s  %s  %s  %s  %s  %s  %d  %d  %d\n",
			//		 cmd,sip, dip, sport, dport, ptc, phb, tbRate, tbSize, tbQL);
			if(tbRate <= 0)
				ruleMgr -> addRule(sip,dip,sport,dport,ptc,phb,NULL);
			else{
				Tokenbucket* tb = new Tokenbucket(tbRate,tbSize,tbQL,phb,this);	
				ruleMgr -> addRule(sip,dip,sport,dport,ptc,phb,tb);
			}
		}
	}
	fclose(fp);
}


int TrafCond::send(ePacket_ *pkt) {

	assert(pkt&&pkt->DataInfo_);
	struct ether_header *eh;
	struct ip *iph;
	Packet *p;
	
	GET_PKT(p,pkt);
	eh=(struct ether_header *)p->pkt_get();

	/* not a ip packet  */
	if(ntohs(eh->ether_type)!=0x0800){
		return(NslObject::send(pkt)); 
	}


	Rule* rule = ruleMgr -> getRule(pkt);
	
	/* ip packet with matched rule  */
	if(rule != NULL){
		iph = (struct ip *)p->pkt_sget();
    	iph -> ip_tos = rule -> getCodepoint();

		Tokenbucket* tb;
		tb = ruleMgr->getTokenbucket(pkt);
		if(tb == NULL)
			return(NslObject::send(pkt)); 

		if(tb->input(pkt) == false){
			// tokenbucket queue is full
			freePacket(pkt);
		}
		// put packet into tokenbucket
		return 1;
	}

	/* ip packet without matched rule */
	return(NslObject::send(pkt)); 
}


	
int TrafCond::recv(ePacket_ *pkt) {

	/* Just by pass incoming packet */
        assert(pkt&&pkt->DataInfo_);
        return(NslObject::recv(pkt));
}


/*
 * sendPkt(), work with tokenbucket 
 * */
int TrafCond::sendPkt(ePacket_* pkt){
	return (NslObject::send(pkt));	
}

void TrafCond::showMsg(const int msgType,const char* funcName,
				const char* message){

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
			if( strlen(message)>40)
				printf(" :\n\t\"%s\".",message);
			else
				printf(" : \"%s\".",message);
		}
		printf("\n");
	}
}
