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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nctuns_api.h>
#include <time.h>
#include <ip.h>
#include <ethernet.h>
#include <mbinder.h>

#include <ps/DS/ds_i.h>
#include "debug.h"

#include <netinet/in.h>

MODULE_GENERATOR(ds_i);

const char* ds_i::className = "ds_i";

ds_i::ds_i(u_int32_t type, u_int32_t id,struct plist *pl, const char *name)
                : NslObject(type, id, pl, name)
{

	/* disable flow control */
	s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	vBind("ds_domain", &ds_domain);
	vBind("log_qlen", &log_qlen);
	vBind("log_qdrop", &log_qdrop);
	vBind("qlen_option", &qlen_option);
	vBind("qlen_samRate", &qlen_samRate);
	vBind("qdrop_samRate", &qdrop_samRate);

	vBind("log_AF1_QueLen", &log_qlen_file[0]);
	vBind("log_AF2_QueLen", &log_qlen_file[1]);
	vBind("log_AF3_QueLen", &log_qlen_file[2]);
	vBind("log_AF4_QueLen", &log_qlen_file[3]);
	vBind("log_BE_QueLen", &log_qlen_file[4]);
	vBind("log_NC_QueLen", &log_qlen_file[5]);
	vBind("log_EF_QueLen", &log_qlen_file[6]);

	vBind("log_AF11_Drop", &log_drop_file[0]);
	vBind("log_AF12_Drop", &log_drop_file[1]);
	vBind("log_AF21_Drop", &log_drop_file[2]);
	vBind("log_AF22_Drop", &log_drop_file[3]);
	vBind("log_AF31_Drop", &log_drop_file[4]);
	vBind("log_AF32_Drop", &log_drop_file[5]);
	vBind("log_AF41_Drop", &log_drop_file[6]);
	vBind("log_AF42_Drop", &log_drop_file[7]);
	vBind("log_BE_Drop", &log_drop_file[8]);
	vBind("log_NC_Drop", &log_drop_file[9]);
	vBind("log_EF_Drop", &log_drop_file[10]);

	log_qlen = NULL;
	log_qdrop = NULL;
	qlen_option = NULL;
	qlen_samRate = 1;
	qdrop_samRate = 1;
	
	qlen_full_log = true;
	
	af1 = NULL;
	af2 = NULL;
	af3 = NULL;
	af4 = NULL;
	be = NULL;
	nc = NULL;
	ef = NULL;
	cntQue = NULL;
	
	int i;
	// ef doesn't have rate limit
	for(i=0;i<6;i++)
		qRate[i]  = 0;
	
	for(i=0;i<7;i++){
		qArray[i] = NULL;
		log_qlen_file[i] = NULL;
	}
	
	for(i=0;i<11;i++)
		log_drop_file[i] = NULL;

	/* debug */
	DDDshowMe = false;
	debug = 0;
}




ds_i::~ds_i() {

	cntQue = NULL;

	// It is safe to delete a NULL point
	delete af1;
	delete af2;
	delete af3;
	delete af4;
	delete be;
	delete nc;
	delete ef;
}


int ds_i::init() {

	char* dsFilePath = (char*) malloc(strlen(GetConfigFileDir()) + 100);
	sprintf(dsFilePath,"%sdsdmdf.txt",GetConfigFileDir());
	parseDSFile(dsFilePath);
	free(dsFilePath);
	
	qArray[0] = af1;
	qArray[1] = af2;
	qArray[2] = af3;
	qArray[3] = af4;
	qArray[4] = be;
	qArray[5] = nc;
	qArray[6] = ef;

	
	if(DEBUG_LEVEL >= MSG_DEBUG && get_nid() == DDDEBUG_SHOW_NODE){
		DDDshowMe = true;
		
		int tmp = DDDEBUG_SHOW_NODE;
		printf("DEBUG ds_i::init() Set show node  = N%d\n",tmp);

		for(int i=0;i<6;i++){
			if(qArray[i] != NULL)
				qArray[i]->DDDshowIt = true;
		}
	}

	for(int i=0; i<6; i++)
		qRate[i]=0;

	if(qArray[0] != NULL)
		qRate[0] = qArray[0]->getRate();
	for(int i=1;i<6;i++){
		if(qArray[i] != NULL)
			qRate[i]=qArray[i]->getRate(); 
		qRate[i] += qRate[i-1];
	}


	if(DEBUG_LEVEL >= MSG_DEBUG){
		printf("DEBUG ds_i::init(), qRate = {%d,%d,%d,%d,%d,%d}\n",
					qRate[0],qRate[1],qRate[2],qRate[3],qRate[4],qRate[5]);
		for(int i=0;i<7;i++){
			if(qArray[i] != NULL){
				printf("%d :",i);
				qArray[i] -> show();
			}
		}
	}

	/* open queue length log file */
	if ( (log_qlen)&&(!strcasecmp(log_qlen, "on")) ){
		char* logFilePath;
		logFilePath = (char*) malloc(strlen(GetConfigFileDir()) + 100);
		for(int i=0; i<7; i++){
			printf("*");
			sprintf(logFilePath,"%s%s",GetConfigFileDir(),log_qlen_file[i]);
			if(qArray[i]!=NULL){
				qArray[i]->enableLogLength(logFilePath);
			}
		}
		free(logFilePath);

		if ( (qlen_option)&&(!strcasecmp(qlen_option, "FullLog")) ){
		//	full log
			qlen_full_log = true;
		}else{ 
		// sample rate
			qlen_full_log = false;
			u_int64_t samplerate_tick;
			MILLI_TO_TICK(samplerate_tick, qlen_samRate * 1000);
			
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(ds_i, logQueueLength);
			lenLogTimer.setCallOutObj(this, type);
			lenLogTimer.start(samplerate_tick, samplerate_tick);
		}

	}

	/* open queue drop log file */
	if ( (log_qdrop)&&(!strcasecmp(log_qdrop, "on")) ){
		char* logFilePath1;
		char* logFilePath2;
		logFilePath1 = (char*) malloc(strlen(GetConfigFileDir()) + 100);
		logFilePath2 = (char*) malloc(strlen(GetConfigFileDir()) + 100);

		int i;

		// af phb group
		for(i=0; i<4; i++){			
			sprintf(logFilePath1,"%s%s",GetConfigFileDir(),log_drop_file[2*i]);
			sprintf(logFilePath2,"%s%s",GetConfigFileDir(),log_drop_file[2*i + 1]);

			if(qArray[i]!=NULL){
				AFQueue* af = (AFQueue*) qArray[i];
				af->enableLogDrop(logFilePath1,logFilePath2);
			}
		}

		// others
		for(i=4; i<7; i++){			
			sprintf(logFilePath1,"%s%s",GetConfigFileDir(),log_drop_file[i+4]);
			if(qArray[i]!=NULL){
				qArray[i]->enableLogDrop(logFilePath1);
			}
		}

		free(logFilePath1);
		free(logFilePath2);
	
		char logPath[40];
		sprintf(logPath,"DSI_N%dP%d",get_nid(),get_port());
			
		for(i=0;i<7;i++)
			if(qArray[i] != NULL)
				qArray[i]->enableLogDrop(logPath);

		u_int64_t samplerate_tick;
		MILLI_TO_TICK(samplerate_tick, qdrop_samRate * 1000);
		
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(ds_i, logQueueDrop);
		dropLogTimer.setCallOutObj(this, type);
		dropLogTimer.start(samplerate_tick, samplerate_tick);
	}

	
	/* set upcall */
	int (NslObject::*upcall)(MBinder *);
	upcall = (int (NslObject::*)(MBinder *))&ds_i::intrq;
	sendtarget_->set_upcall(this, upcall);
	return(1);
}

void ds_i::logQueueLength(){
	for(int i=0;i<7;i++)
		if(qArray[i] != NULL)
			qArray[i]->logLength();
}

void ds_i::logQueueDrop(){
	for(int i=0;i<7;i++)
		if(qArray[i] != NULL)
			qArray[i]->logDrop();
}



int ds_i::send(ePacket_ *pkt){

	assert(pkt&&pkt->DataInfo_);

	struct ether_header *eh;
	Packet *p;
	
	GET_PKT(p,pkt);
	eh=(struct ether_header *)p->pkt_get();

	/* insert timestamp into pkt->DataInfo_ */
	double time = GetCurrentTime() * TICK / 1000000000.0;
	p->pkt_addinfo("DS_AT", (const char *)&time, sizeof(double)); //arrive time of DS module


	/* not a ip packet, insert into NC queue!! */
	if(ntohs(eh->ether_type)!=0x0800){
		if( sendtarget_->qfull() ){
			if( nc -> enqueue(pkt) == -1 ){
				freePacket(pkt);
				return(1);
			}
			if(qlen_full_log == true)
				logQueueLength();
			return(1);	
		}else
			return(NslObject::send(pkt));  
	}

	/* ip packet, select queue by DHCP field */	
	if( sendtarget_->qfull() ){
		struct ip *iph= (struct ip *)p->pkt_sget();
		unsigned char codepoint = iph -> ip_tos;
		char phb[6];
		DSCPTophb(phb,codepoint);
	
		DSQueue* q = NULL;
		AFQueue* aq = NULL;
		bool isAF = false;

		if(strcmp(phb,"BE") == 0){
		 	q = be;
		}else if(strcmp(phb,"NC") == 0){
			q = nc;
		}else if(strcmp(phb,"EF") == 0){
			q = ef;
		}else if(phb[0]=='A' && phb[1]=='F'){
			isAF = true;
			aq = (AFQueue*) qArray[(phb[2]-'1')];
		}

		if(q == NULL && aq == NULL){
			if(DEBUG_LEVEL >= MSG_FATAL){
				printf("FATAL ERROR in ds_i::send() :\n");
				printf("\tNo matched queue, maybe a wrong dsdmdf file!!\n");
				printf("\tDEBUG INFO: codepoint(%d),phb(%s)\n",codepoint,phb);
				printf("EXIT(1)");
			}
			exit(1);
		}

		if(isAF == false){
			if( q -> enqueue(pkt) == -1 ){
				freePacket(pkt);
				return(1);
			}
		}else{
			if( aq -> enqueue(pkt, phb[3]-'1')  == -1){
				freePacket(pkt);
				return(1);
			}	
		}
				

		if(qlen_full_log == true)
			logQueueLength();

		return(1);	
	}else{
		return(NslObject::send(pkt)); 
	} 
}


int ds_i::recv(ePacket_ *pkt){
	/* Just by pass incoming packet */
        assert(pkt&&pkt->DataInfo_);
        return(NslObject::recv(pkt));
}

int ds_i::intrq(MBinder *port) {
	ePacket_	*pkt=NULL;

	// ef queue first
	if( (ef != NULL) && !(ef->isEmpty()) ){
		ef->addAccount();
		pkt = ef->dequeue();
		if(pkt != NULL){
			assert(sendtarget_->enqueue(pkt)== 0);
		}
		return 1;
	}

	// dequeue from current Queue, 
	// or set cntQue to NULL and select a new queue.
	if(cntQue != NULL)
		pkt = cntQue -> dequeue();

	if(pkt != NULL){
		assert(sendtarget_->enqueue(pkt) == 0);
	
		if( qlen_full_log == true )
			logQueueLength();	
		return(1);

	}else{
		cntQue = NULL;
	}

	
	// select a queue, try 30 times at most.
	DSQueue* q=NULL;
	int test_count=50;		// avoid infinite while loop
	int r,i;

	while((q==NULL || q->isEmpty()==true) && test_count >0){
		test_count--;
		r = random()%qRate[5];
		for(i=0;i<6;i++){
			if(r < qRate[i])
				break;
		}
		q = qArray[i];
	}

	if(test_count==0){
		q = NULL;
		for(i=0;i<6;i++){
			if(qArray[i] != NULL && !qArray[i]->isEmpty() ){
				q = qArray[i];
				break;
			}
		}
	}
	if( q==NULL || q->isEmpty()){
		return 1;
	}
	
	// BE / NC
	if(q == be && !nc->isEmpty() ){
	
		double be_time=0, nc_time=0;
	
		pkt = be->getNext();
		ePacket_* nc_pkt = nc->getNext();
		Packet* p;
		
		assert(pkt&&(p=(Packet*)pkt->DataInfo_));
		be_time = *((double*)p->pkt_getinfo("DS_AT"));
		
		assert(nc_pkt&&(p=(Packet*)nc_pkt->DataInfo_));
		nc_time = *((double*)p->pkt_getinfo("DS_AT"));
	
		if(be_time > nc_time){
			q = nc;	// give chance to nc queue
		}
	}
	
	// here, we make sure that q is not NULL , and is not empty
	cntQue = q ;
	cntQue -> addAccount();
	pkt = cntQue -> dequeue();

	if (pkt != NULL){
		assert(sendtarget_->enqueue(pkt) == 0);
	}
	
	if( qlen_full_log == true )
		logQueueLength();	

	return(1);

}


void ds_i::DSCPTophb(char* phb,const unsigned char codepoint){

	switch(codepoint){
	  case 184:		// '101110'00
		strcpy(phb,"EF");
		return;
	  case 40:		// '001010'00
		strcpy(phb,"AF11");
	  	return;
	  case 48:		// '001100'00
		strcpy(phb,"AF12");
	  	return;
	  case 56:		// '001110'00
		strcpy(phb,"AF13");
	  	return;
	  case 72:		// '010010'00
		strcpy(phb,"AF21");
	  	return;
	  case 80:		// '010100'00
		strcpy(phb,"AF22");
	  	return;
	  case 88:		// '010110'00
		strcpy(phb,"AF23");
	  	return;
	  case 104:		// '011010'00
		strcpy(phb,"AF31");
	  	return;
	  case 112:		// '011100'00
		strcpy(phb,"AF32");
	  	return;
	  case 120:		// '011110'00
		strcpy(phb,"AF33");
	  	return;
	  case 136:		// '100010'00
		strcpy(phb,"AF41");
	  	return;
	  case 144:		// '100100'00
		strcpy(phb,"AF42");
	  	return;
	  case 152:		// '100110'00
		strcpy(phb,"AF43");
	  	return;
	  case 224:		// '111000'00
	  case 192:		// '110000'00
	  case 160:		// '101000'00
	  case 128:		// '100000'00
		strcpy(phb,"NC");
		return;
	  default:
		strcpy(phb,"BE");
	  	return;	  
	}
}


void ds_i::parseDSFile(char* dsFilePath){
	
	FILE* fp;
	fp = fopen(dsFilePath,"r");

	if(fp == NULL){
		if(DEBUG_LEVEL >= MSG_FATAL){
			printf("FATAL ERROR in ds_i::parseDSFile() line %d:\n",__LINE__);
			printf("\tCan't open configure file, file name: %s \n",dsFilePath);
			printf("EXIT(1)\n");
		}
		exit(1);
	}
	
	char line[128];
	char cmd[20];
	char type[8];		// Ex. EF, BE, AF12
	char name[16];		// Ex. Best, Normal...
	char qName[24];		// Ex. N4P5_AF11
	int rate;
	int maxQL;
	int ts1;
	int ts2;
	int maxDR;
	
	bool start = false;	// if the domain information is what we wanted, set start true.
	char dsName[16];
	dsName[0]='\0';


	int r;			// a temp value to record return value from 'sscanf'
	while(!feof(fp)){
	
		// reset to default vaule
		line[0] = '\0';
		cmd[0] = '\0';
		type[0] = '\0';
		name[0] = '\0';
		qName[0] = '\0';
		
		rate = 1;
		maxQL = 50;
		ts1=20;
		ts2=40;
		maxDR = 80;
		
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
		else if( strcasecmp(cmd,"QueAdd") == 0){
			if(start == false)
				continue;
			
			
			// QueAdd	[type]	[name]	[rate]	[maxQL]	(ts1)	(ts2)	(masDR)
			r = sscanf(line,"%s  %s  %s  %d  %d  %d  %d  %d",
					 cmd, type, name, &rate, &maxQL, &ts1, &ts2, &maxDR);
					 
			if(strlen(name) > 0);
				sprintf(qName,"N%dP%d_%s",get_nid(),get_port(),name);
			
			if(type[0]=='A' && type[1]=='F'){
				if(r < 7){
					if(DEBUG_LEVEL >= MSG_WARNING){
						printf("WARNING in ds_i::parseDSFile() line %d:\n",__LINE__);
						printf("\tError ds define file format. Add AF queue with wrong parameter\n");
					}
					continue;
				}
				switch(type[2]){
				  case '1':
					if(af1!=NULL){
						if(DEBUG_LEVEL >= MSG_WARNING){
							printf("WARNING in ds_i::parseDSFile():\n");
							printf("\tError ds configure file format. Add AF1 queue more than once\n");
						}
						break;
					}
			  		af1 = new AFQueue(qName,maxQL,rate,ts1,ts2,maxDR,get_nid(),get_port());
					if(DEBUG_LEVEL >= MSG_DEBUG)	
						printf("Add DSQueue, AF1, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				  	break;

				  case '2':
					if(af2!=NULL){
						if(DEBUG_LEVEL >= MSG_WARNING){
							printf("WARNING in ds_i::parseDSFile():\n");
							printf("\tError ds configure file format. Add AF2 queue more than once\n");
						}
						break;
					}
			  		af2 = new AFQueue(qName,maxQL,rate,ts1,ts2,maxDR,get_nid(),get_port());
					if(DEBUG_LEVEL >= MSG_DEBUG)	
						printf("Add DSQueue, AF2, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				  	break;

				  case '3':
					if(af3!=NULL){
						if(DEBUG_LEVEL >= MSG_WARNING){
							printf("WARNING in ds_i::parseDSFile():\n");
							printf("\tError ds configure file format. Add AF3 queue more than once\n");
						}
						break;
					}
			  		af3 = new AFQueue(qName,maxQL,rate,ts1,ts2,maxDR,get_nid(),get_port());
					if(DEBUG_LEVEL >= MSG_DEBUG)	
						printf("Add DSQueue, AF3, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				  	break;
	
				  case '4':
					if(af4!=NULL){
						if(DEBUG_LEVEL >= MSG_WARNING){
							printf("WARNING in ds_i::parseDSFile():\n");
							printf("\tError ds configure file format. Add AF4 queue more than once\n");
						}
						break;
					}
			  		af4 = new AFQueue(qName,maxQL,rate,ts1,ts2,maxDR,get_nid(),get_port());
					if(DEBUG_LEVEL >= MSG_DEBUG)	
						printf("DEBUG Add DSQueue, AF4, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				  	break;

				  default:
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add AF queue with wrong type(%s)\n",type);
					}
				}
				continue;
			}

			if(type[0]=='B' && type[1]=='E'){
				if(r != 5){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add BE queue with wrong parameter\n");
					}
					continue;
				}
				if(be!=NULL){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add BE queue more than once\n");
					}
					continue;
				}
				be = new DSQueue(qName,maxQL,rate,get_nid(),get_port());
				if(DEBUG_LEVEL >= MSG_DEBUG)
					printf("DEBUG Add DSQueue, BE, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				continue;
			}
			if(type[0]=='N' && type[1]=='C'){
				if(r != 5){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add NC queue with wrong parameter\n");
					}
					continue;
				}
				if(nc!=NULL){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add NC queue more than once\n");
					}
					continue;
				}
				nc = new DSQueue(qName,maxQL,rate,get_nid(),get_port());
				if(DEBUG_LEVEL >= MSG_DEBUG)
					printf("DEBUG Add DSQueue, NC, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				continue;
			}
			
			if(type[0]=='E' && type[1]=='F'){
				if(r != 5){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add EF queue with wrong parameter\n");
					}
					continue;
				}
				if(ef!=NULL){
					if(DEBUG_LEVEL >= MSG_WARNING){
				  		printf("WARNING in ds_i::parseDSFile():\n");
						printf("\tError ds configure file format. Add EF queue more than once\n");
					}
					continue;
				}
				ef = new DSQueue(qName,maxQL,rate,get_nid(),get_port());
				if(DEBUG_LEVEL >= MSG_DEBUG)
					printf("DEBUG Add DSQueue, EF, qName(%s), maxQL(%d), rate(%d)\n",qName,maxQL,rate);
				continue;
			}
		}
	}

	if(be == NULL){
		be = new DSQueue("BE",50,1,get_nid(),get_port());
		if(DEBUG_LEVEL >= MSG_WARNING){
	  		printf("WARNING in ds_i::parseDSFile():\n");
			printf("\tNot find BE queue defination. Add BE queue automatically\n\t");
			be->show();
		}
	}
	if(nc == NULL){
		nc = new DSQueue("NC",50,1,get_nid(),get_port());
		if(DEBUG_LEVEL >= MSG_WARNING){
	  		printf("WARNING in ds_i::parseDSFile():\n");
			printf("\tNot find NC queue defination. Add NC queue automatically\n\t");
			nc->show();
		}
	}
}

void ds_i::showMsg(const int msgType,const char* funcName,
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
