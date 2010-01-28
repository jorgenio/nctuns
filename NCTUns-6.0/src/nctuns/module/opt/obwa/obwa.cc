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

#include "obwa.h"
#include <assert.h>
#include <object.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include <stdlib.h>
#include <gbind.h>
#include <packet.h>

extern ReadPath			*readpath_;

MODULE_GENERATOR(obwa);

u_long	allpacket =0;
obwa::obwa(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	/* bind variable */
	vBind("ProcessTime", &Process_time);
	
	vBind("TIMEOUT", &Timeout); 	
	vBind("Sendpacketlength", &Send_packet_length); 
	vBind("Droppacketlength", &Drop_packet_length); 

	vBind("MaxWave",&MAX_WAVE); 	
	vBind("ControlPacketWave",&ControlPacket_wave);
	vBind("ChooseWave",&choose_wave);

	vBind("ringfile",&ringfile);
	vBind("nodeconnectfile",&nodeconnectfile);
	vBind("nodekindfile",&nodekindfile);
	vBind("pathfile",&nodepathfile);

	/* initiate variables */	
	WAT = NULL;
	Send_seq = 0;
	AssembleTimer_seqnum = 0;
	Buffer_head = (struct buffer_head *)malloc(sizeof(struct buffer_head));
	Buffer_head->list_num = 0;
	Buffer_head->first = NULL;
	Buffer_head->last = NULL;

}


obwa::~obwa() {
}

int obwa::recv(ePacket_ *pkt){
	return NslObject::recv(pkt);
}


int obwa::send(ePacket_ *pkt) {
	u_long  		next;
	Packet			*packet = (Packet *)pkt->DataInfo_;
	int 			OBS;

	
	next = (u_long)packet->rt_gateway();
	
	if (Is_AssembleTimer_running_goallist (next) == 0){
		/* store first packet into buffer
		 */
		struct lheader	*lh;
		
		OBS = 2;	//represent it is data packet	 
		
		packet->pkt_addinfo("OBS",(char *)&OBS ,sizeof(int));
		lh = (struct lheader *)packet->pkt_malloc(sizeof(struct lheader));
		lh->dstIP = next;
		lh->option = 0;			//normal packet
		lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
		
		putinto_buffer(pkt,next);               
		Set_AssembleTimer_run_goallist (next);
		
	}
	else{
		/* queue in buffer until 
		 * 1.timeout 
		 * 2.reach segment size $$$$$$$nedd to adjust
		 * */
		struct lheader	*lh;
		OBS = 2;
		packet->pkt_addinfo("OBS",(char *)&OBS ,sizeof(int));
		lh = (struct lheader *)packet->pkt_malloc(sizeof(struct lheader));
		lh->dstIP = next;
		lh->option = 0;//data packet
		lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());	
		putinto_buffer(pkt,next);                 	
	}
	return(1);
}


int obwa::get(ePacket_ *pkt, MBinder *frm) {

	Packet	*p = (Packet *)pkt->DataInfo_;
	char    *tmp;
	int 	 *tmp1;
	tmp = p->pkt_getinfo("OBS");		//to check if receive packet is about obs

	tmp1 = (int *)p->pkt_getinfo("SEQ");		//to check if receive packet is about obs

	if(p->pkt_getflags() & PF_RECV){
		//normal packet received
		if (tmp!= NULL){
			if (*tmp == 1){
				freePacket(pkt);
				return(1);
			}
			else if(*tmp==3){
				freePacket(pkt);
				return(1);
			}
			else{
				return(recv(pkt));
			}
		}
		else{
			printf("%d obwa free pkt which is not obs pkt\n",get_nid());
			freePacket(pkt);
			return(1);
		}
	}
	else{
		return send(pkt);
	}
}

/* This function is used to calculate transmission time for send length packet
 * */

u_int64_t obwa::Calculate_trans_time(int length, int wave){

	struct plist * tmplist;
        tmplist =  (struct plist *)malloc(sizeof(struct plist));
	tmplist->pid = get_port();
	tmplist->next =  (struct plist *)malloc(sizeof(struct plist));
	tmplist->next->pid = wave;
	tmplist->next->next = NULL;

        double* bandwidth =(double *) get_regvar(get_nid(),tmplist,"BW");
	
	free(tmplist->next);
	free(tmplist);

 	/* set time to release wave  */
				 
	u_int64_t       onesec;
        SEC_TO_TICK(onesec,1);
	u_int64_t       delayTick ;
	double		delay_double,dt;
	
	delay_double = length * 8 * onesec / ((*bandwidth)  );
	delayTick = length * 8 * onesec / ((u_int64_t)(*bandwidth));
	dt = (delay_double- delayTick) *10;
	if(dt >= 5 )
		delayTick++;

	// Carefully !!! If delayTick ==0 then add 1 tick
	// But this will result in unaccurance	
	return delayTick;	
}


/* This function is used to calculate Propdelay  time for send length packet
 * */

int obwa::Calculate_Propdelay_time(int length, int wave){

	struct plist * tmplist;
        tmplist =  (struct plist *)malloc(sizeof(struct plist));
	tmplist->pid = get_port();
	tmplist->next =  (struct plist *)malloc(sizeof(struct plist));
	tmplist->next->pid = wave;
	tmplist->next->next = NULL;

        int* propdelay =(int *) get_regvar(get_nid(),tmplist,"PropDelay");
	
	free(tmplist->next);
	free(tmplist);
        
	int	propdelay_in_tick = *propdelay;
	MICRO_TO_TICK(propdelay_in_tick,propdelay_in_tick);
	
	return propdelay_in_tick;	
}

/* This function will release wave length use after delayTick
 * */

void obwa::Send_release_wave_Event(int wave,int  output_port ,u_int64_t delayTick){
		
	Event_		*tep;	
	struct Buffer_event_info        *Binfo;
	tep = createEvent();
		
	/* malloc a space to hold buffer 
	 * information
	 */
	
	Binfo = (struct Buffer_event_info *)malloc(sizeof(struct Buffer_event_info));
	assert(Binfo);	
	Binfo->wave = wave;
	Binfo->output_port = output_port;
	
	tep->DataInfo_ = (void *)Binfo;


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(obwa,release_wave);

	/* set time to release buffer*/
	
	setObjEvent(tep,GetCurrentTime()+delayTick,0,this,type,(void *)tep->DataInfo_);//
}


/* To register sequnem to wave
 * This means if burst seq NO is seqnum then it will be assigned wavelength to wave
 * oldwave is uesd in wavelength conversion  */

void obwa::addto_WA_table(u_long seqnum ,int wave, int oldwave , int output_port){ 
	Wave_assign_table * wat_ptr ;
	wat_ptr = WAT;
	if (wat_ptr == NULL){
		
		WAT = wat_ptr = (struct Wave_assign_table *)malloc(sizeof(struct Wave_assign_table)); 
		wat_ptr->wave = wave;
		wat_ptr->oldwave = oldwave;
		wat_ptr->SeqID = seqnum;
		wat_ptr->output_port = output_port;
		wat_ptr->send = 0;		//has not send
		wat_ptr->next = NULL;
	}
	else{
		wat_ptr ->next  = (struct Wave_assign_table *)malloc(sizeof(struct Wave_assign_table));
		wat_ptr = wat_ptr->next ;
		wat_ptr->wave = wave;
		wat_ptr->oldwave = oldwave;
		wat_ptr->SeqID = seqnum;
		wat_ptr->output_port = output_port;
		wat_ptr->send = 0;		//has not send
		wat_ptr->next = NULL;	
	}
}

//find min available wavelength that can be used in WATable
int  obwa::availwave_in_WA_table(int output_port ){
	Wave_assign_table * wat_ptr ;
	int i;
	int avail_wave = 0;
	
	
	for (i = 1 ; i <= MAX_WAVE ; i++){
		avail_wave = i;
		for (wat_ptr = WAT ; wat_ptr ; wat_ptr = wat_ptr->next ){
			if ((wat_ptr->wave  == avail_wave )&&(wat_ptr->output_port == output_port)){
				avail_wave = 0;	
				break;
			}
		}
		if (avail_wave)
			return (avail_wave);
	}
	return (avail_wave);
	
} 

//check if wavelength is available in WATable
int obwa::Is_avail_wave(int wave , int output_port){
	Wave_assign_table * wat_ptr ;
	for (wat_ptr = WAT;wat_ptr;wat_ptr = wat_ptr->next ){
		if ((wat_ptr->wave  == wave) &&(wat_ptr->output_port == output_port))
				return (0);
	}
	if (wave > MAX_WAVE )
		return 0;
	return 1;
}

void obwa::release_wave(Event_ *ep ){
	struct Buffer_event_info        *bInfo;
	int				wave;
	int				oport;
	
	bInfo = (struct Buffer_event_info *)ep->DataInfo_;
	wave = bInfo->wave;
	oport = bInfo->output_port;
	
	Wave_assign_table * wat_ptr =WAT;
	Wave_assign_table * pre_ptr ;
	Wave_assign_table * free_ptr ;
	
	int flag = 0;
		
	//delete wave assing in WaTable according to wave

	assert(wat_ptr);
	if (wat_ptr->wave  == wave){				//the first one
		flag = 1;
		free_ptr = WAT;
		WAT = wat_ptr->next;
		free(free_ptr);
	}
	else if ((WAT->next  != NULL) &&(flag == 0)){		//more than two
		pre_ptr  = WAT;
		for (wat_ptr = WAT->next ;wat_ptr;wat_ptr = wat_ptr->next,pre_ptr = pre_ptr->next ){
			if ((wat_ptr->wave  == wave)&&(wat_ptr->output_port == oport)){
				pre_ptr->next = wat_ptr->next ;
				free(wat_ptr);
				break;
			}
		}
	}
	freeEvent(ep);
}

//delete wave assing in WaTable according to seqID
void obwa::delwave_byseq(u_long seq){

	/*This is function is used when we want to release buffer,
	 * in this time ,we should release wave bined to seqnum
	 * */
	
	Wave_assign_table * wat_ptr =WAT;
	Wave_assign_table * pre_ptr ;
	Wave_assign_table * free_ptr ;

	int flag =0 ;
	assert(wat_ptr);
	
	if (wat_ptr->SeqID  == seq){				//the first one
		flag = 1;
		free_ptr = WAT;
		WAT = wat_ptr->next;
		free(free_ptr);
		
	}
	else if ((WAT->next  != NULL)&&(flag ==0)){		//more than two

		pre_ptr  = WAT;
		for (wat_ptr = WAT->next ;wat_ptr;wat_ptr = wat_ptr->next,pre_ptr = pre_ptr->next ){
			if (wat_ptr->SeqID   == seq){
				pre_ptr->next = wat_ptr->next ;
				free(wat_ptr);
				break;
			}
		}
	}
}

int obwa::Get_oldwave_by_seq(u_long seq){
	Wave_assign_table * wat_ptr ;
	for (wat_ptr = WAT;wat_ptr;wat_ptr = wat_ptr->next ){
		if (wat_ptr->SeqID   == seq)
				return wat_ptr->oldwave;
	}
	return 0;
}



/*This function is  to check the Assembler Timer of  goal listid */

int  obwa::Is_AssembleTimer_running_goallist (u_long  goal){
	buffer_list *ptr;
	
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal){
			if(ptr->AssembleTimer_active_seq != 0) {	
				return 1;
			}
		}
	}
	return 0;
}



/*This function is set  the Assembler Timer of  goal list running */

void obwa::Set_AssembleTimer_run_goallist (u_long  goal){
	buffer_list *ptr;
	AssembleTimer_seqnum++;
	u_long seq = AssembleTimer_seqnum;

	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal){
			ptr->AssembleTimer_active_seq = seq;
		}
	}

	/* set time schedule to currentime plus Timeout */
	Event_		*ep;	
	ep = createEvent();
		
	/* malloc a space to send buffer information */
	
	Buffer_event_info * Binfo = (struct Buffer_event_info *)malloc(sizeof(struct Buffer_event_info));
	assert(Binfo);	
	Binfo->next = goal;
	Binfo->AssembleTimer_seqnum = seq;
	ep->DataInfo_ = (void *)Binfo;
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(obwa,send_control_packet);
	setObjEvent(ep,GetCurrentTime()+ (u_int64_t)Timeout,0,this,type,(void *)ep->DataInfo_);//
	
}



/*This function is set  the Assembler Timer of  goal list stop */

void  obwa::Set_AssembleTimer_stop_goallist (u_long  goal){
	buffer_list *ptr;
	
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal){
			ptr->AssembleTimer_active_seq = 0;
		}
	}
}

int   obwa::Is_Active_AssembleTimer_event(u_long goal,u_long sequencenum){
	buffer_list *ptr;
	
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if ((ptr->goal == goal) &&(ptr->AssembleTimer_active_seq == sequencenum)){
			return 1;
		}
	}
	return 0 ;
}

/* This function is used to check the goal destination IP is available or not 
 * if the buffer to the destionation IP is not full ,then we will return not 0 */

u_long obwa::Is_available_goallist (u_long  goal){
	buffer_list *ptr;
	
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal){
			if(ptr->full == 0) {	
					return ptr->sequ_num;
			}
		}
	}
	return 0;
}


/* This function is uesd to mark the goal list is full
 * If more packet will put into this buffer will be droped */

void obwa::Set_goallist_full (u_long goal){
	buffer_list *ptr;

	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal ){
			ptr->full = 1;
		}
	}
}


int obwa::Is_goallist_empty (u_long goal){
	buffer_list *ptr;

	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if ((ptr->goal == goal ) &&(ptr->empty == 0)){
			return 1;
		}
	}
	return 0;
}

/* If the goal destination IP buffer is exist then return it ,or return NULL */

buffer_list *  obwa::Is_Exist_goal (u_long goal){
	buffer_list *ptr;
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->goal == goal ){
			return ptr;
		}
	}
	return NULL;
}

/* If the goal destination IP buffer is exist then return it ,or return NULL */

buffer_list *  obwa::Is_Exist_list (u_long sequencenum){
	buffer_list *ptr;
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){	
		if (ptr->sequ_num == sequencenum ){
			return ptr;
		}
	}
	return NULL;
}


/*If the destionation IP buffer is not exist ,then we will create a new list */

buffer_list * obwa::add_new_list(u_long goal){
	buffer_list *new_list = (struct buffer_list *)malloc( sizeof( struct buffer_list));
	
	new_list->entity_num =0;
	new_list->packet_length = 0;
	new_list->AssembleTimer_active_seq = 0;
	new_list->full = 0;
	new_list->empty = 1;
	new_list->goal = goal;
	new_list->adjust_length = 0;
	new_list->next = NULL;
	new_list->first = NULL;
	new_list->last  = NULL;
	
	if (Buffer_head->first  == NULL){
		Buffer_head->first = new_list;
		Buffer_head->last = new_list;
	}
	else{
		Buffer_head->last->next = new_list;
		Buffer_head->last  =new_list;
	}
	Buffer_head->list_num  = Buffer_head->list_num +1 ;
	
	return new_list;
}


void  obwa:: addto_list(ePacket_ *epacket , buffer_list *ptr){
    	Entity *new_entity = (struct Entity *)malloc(sizeof(struct  Entity));

	new_entity->next = NULL;
	new_entity->pkt  = epacket;
	
	
	if (ptr->first  == NULL){
		ptr->first  = new_entity;
		ptr->last = new_entity;
	}
	else{
		
		ptr->last->next  = new_entity; 
		ptr->last = new_entity;
	}
	ptr->entity_num ++;
}


int obwa::putinto_buffer(ePacket_* pkt, u_long next){
	Packet			*p = (Packet *)pkt->DataInfo_;
	buffer_list 		*ptr;
	Buffer_event_info 	*Binfo;
	
	/* add packet into buffer list */
	
	ptr = Is_Exist_goal(next); //adjust 12/31
	if (ptr == NULL){
		ptr = add_new_list(next);
	}
	addto_list(pkt ,ptr);
	
	//add buffer list length       
	
	ptr->packet_length = ptr->packet_length + p->pkt_getlen();
        ptr->empty = 0;
	
	/* when buffer is bigger than some level */
	
	if(ptr->packet_length > (Send_packet_length+ ptr->adjust_length)){

		Set_AssembleTimer_stop_goallist (next);
		AssembleTimer_seqnum++;
		u_long			seq = AssembleTimer_seqnum;
		
		ptr->AssembleTimer_active_seq = seq;
		
		Event_		*ep;	
		ep = createEvent();
		
		/* malloc a space to hold buffer 
	 	 * information */
	
		Binfo = (struct Buffer_event_info *)malloc(sizeof(struct Buffer_event_info));
		Binfo->next	   = next;
		Binfo->AssembleTimer_seqnum =seq;
		ep->DataInfo_ = (void *)Binfo;
		send_control_packet(ep);
	}
	else if (ptr->packet_length > Drop_packet_length){
		Set_goallist_full(next);
		delete_buffer(next);
	}
	
	return 1;
}

int obwa::send_control_packet(Event_ * ep){

	struct Buffer_event_info        *bInfo;
	int    				wave;
	int 				wave_get_from_short;
	int 				output_port_from_short;
	buffer_list	 		*ptr;
	u_long				next;
	u_long 				sequencenum;
	

	bInfo = (struct Buffer_event_info *)ep->DataInfo_;
	sequencenum = bInfo->AssembleTimer_seqnum;
	next = bInfo->next;

	
	if((Is_Active_AssembleTimer_event(next,sequencenum) == 0)){
	
		/*This situation will happen when traffic quickly aggreate to a burst lebgth 
		 * ,therefore ,we will turn off alarm */
			
		freeEvent(ep);
		return -1;
	}
	
	Set_AssembleTimer_stop_goallist (next);
	ptr = Is_Exist_goal(next);
	
	if (ptr == NULL){
		freeEvent(ep);
		return -1;
	}
	
	wave = readpath_->RWA_get_wave(get_nid(),ipv4addr_to_nodeid(next));
	output_port_from_short = readpath_->RWA_get_outputport(get_nid(),ipv4addr_to_nodeid(next));
	
	// if finish get  port
	// wave must get from shortest path
	
	wave_get_from_short = wave;
	
	if (choose_wave > 0)
		wave = choose_wave;	

	if (!Is_avail_wave(wave,output_port_from_short)){
		freeEvent(ep);
		return -1;
	}
		
	if (ptr->packet_length == 0){
		printf("WARN !!!!!!!!!%d burst no exist  \n",get_nid());
		freeEvent(ep);
		return -1;
	}
	
	if (wave <= 0){
		printf("%d wave over in  free event!!!!!!!!!!!!\n",get_nid());
		wave = 1;
		freeEvent(ep);
		delete_buffer(next); //delete buffer
		return -1;
	}
	
	/* Below used to assign wave to sequencenum */
	
	u_long				sendseq = Send_seq++;
	
	addto_WA_table(sendseq,wave,wave_get_from_short,output_port_from_short);
	int PathDistance = readpath_->GetPath_Length(get_nid(), ipv4addr_to_nodeid(next));

	if (PathDistance < 0)
		return -1;

	//send control packet
	
	ePacket_	*Control_EventPacket = createPacket();
	Packet 		*Control_Packet;
	struct lheader	*Control_lh;
	int		OBS = 1;	//used to identity obs control packet
	
	/* if OBS = 1 it identity control packet
	 * in OBS system  */
	
	Control_Packet = (Packet *)Control_EventPacket->DataInfo_;
	Control_Packet->pkt_addinfo("OBS",(char *)&OBS ,sizeof(int));
	Control_Packet->pkt_addinfo("SEQ", (char *)&sendseq, sizeof(u_long));

	// adding wave in shortest path in packet
	int from = get_nid();
	
	Control_Packet->pkt_addinfo("FROM", (char *)&from, sizeof(int));	//  for debug	
	Control_Packet->pkt_addinfo("OLDW",(char *)&wave_get_from_short,sizeof(int));	
	Control_Packet->pkt_addinfo("r_dur",(char *)&ptr->packet_length,sizeof(int));
	Control_Packet->pkt_addinfo("WAVE",(char *)&wave,sizeof(int));
	Control_lh = (struct lheader *)Control_Packet->pkt_malloc(sizeof(struct lheader));
	Control_lh->dstIP = next;
	Control_lh->wave = ControlPacket_wave;
	Control_lh->option = 0;
	Control_lh->srcIP = nodeid_to_ipv4addr(get_nid(), get_port());
       	
	u_int64_t processTick = Calculate_trans_time(Control_Packet->pkt_getlen(), wave);
	int StoreOffset = (PathDistance)*( (u_int64_t)Process_time + processTick); 
	Control_Packet->pkt_addinfo("OFFS",(char *) &StoreOffset , sizeof(u_long));
	
		
	NslObject::send(Control_EventPacket);

	/* avoid add data into this buffer segment */
	
	//set timeout schedule event
	Event_		*tep;	
	struct Buffer_event_info        *Binfo;
	tep = createEvent();
		
	/* malloc a space to hold buffer 
	 * information */
	
	Binfo = (struct Buffer_event_info *)malloc(sizeof(struct Buffer_event_info));
	assert(Binfo);	
	Binfo->sequencenum = sendseq;
	Binfo->next = next; 
	Binfo->sendlength = ptr->packet_length;
	Binfo->wave = wave;
	tep->DataInfo_ = (void *)Binfo;


	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(obwa,release_buffer);

	/* set time to release buffer */
	//Set_goallist_full(next);
	
	u_int64_t	delayTick  = StoreOffset;

	setObjEvent(tep,GetCurrentTime()+delayTick,0,this,type,(void *)tep->DataInfo_);//
	freeEvent(ep);
	return 1;
}

void obwa::Schedule_send(ePacket_ *ep){	
	
	NslObject::send(ep);
}

int obwa::release_buffer(Event_ *ep ){
	struct Buffer_event_info        *bInfo;
	u_long 				sequencenum;
	u_long				goal;	
	int				send_wave;
	double				send_length;
	int				output_port_from_short;
	
	bInfo = (struct Buffer_event_info *)ep->DataInfo_;
	sequencenum = bInfo->sequencenum;	
	send_length = bInfo->sendlength;
	send_wave =bInfo->wave;
	goal = bInfo->next;
	
	
	buffer_list *ptr =NULL;
	buffer_list *pre = NULL;
	
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){
		if (ptr != Buffer_head->first)
		    	pre = pre->next ;
		else
			pre = Buffer_head->first;
		if (ptr->goal == goal){
			break;
		}
	}
	int  send_packet_num =0;

	if (ptr == NULL){
		printf("  WARN!!!! In obwa.cc !! %d no match enent release SEQ %ld \n ",get_nid(),sequencenum);
		delwave_byseq(sequencenum);
		return -1;
	}
	else{
		Entity *temp ;
		Entity *freeone;
		long send_count = 0;
		temp = ptr->first ;
		freeone = ptr->first;
		
		while(temp){	
			if (temp != ptr->first){
				ptr->entity_num--;
				free(freeone);
				if (freeone == temp)
					printf(" no free memory:: it should not happen !!!!\n");
				else{	
					freeone =temp;
				}
			}
				
			Packet	*p = (Packet *)temp->pkt->DataInfo_;
			send_count = send_count + p->pkt_getlen();
			send_packet_num++;
	                
			struct lheader*	lh;
			lh = (struct lheader*)p->pkt_get();
			lh->wave = send_wave;

			int oldwave = Get_oldwave_by_seq(sequencenum);
			p->pkt_addinfo("OLDW",(char *)&oldwave ,sizeof(int));
			int SEQ = sequencenum;
			p->pkt_addinfo("SEQ", (char *)&SEQ, sizeof(u_long));
			int from = get_nid();
			p->pkt_addinfo("FROM", (char *)&from, sizeof(int));	//  for debug	
			
			if (lh->wave <= 0){
				//should not happened
				printf("!!!!!!!!!!!!!!!This should not happened \n");
				printf("%d obwa free pkt wave over\n",get_nid());
				freePacket(temp->pkt);
				return -1;
			}
			// schedule send packet
		
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(obwa, Schedule_send);
			setObjEvent(temp->pkt, GetCurrentTime()+ (send_packet_num-1) +Calculate_trans_time(send_count- p->pkt_getlen(),send_wave),0,this, type,temp->pkt->DataInfo_);
		
			if (send_count >= send_length){
			        temp = temp -> next;
				free(freeone);
				freeone = temp;
				ptr->entity_num--;
				break;
			}

			temp = temp -> next;
			
		}

		/*if none packet is send ,then free struct will not be done in while loop
		 * so we free it at below if */
		
		if (freeone == ptr->first){
			ptr->entity_num--;
			free(freeone);
		}
		
	
		if (temp == NULL){
				ptr->first = NULL;
				ptr->last = NULL;
				ptr->packet_length = 0;
		}
		else{
			ptr->first = temp;
			ptr->packet_length = ptr->packet_length - send_count;
			 
			if (Is_AssembleTimer_running_goallist (goal) == 0){
				printf("obwa::warn !!! this will rarely happen\n");
				Set_AssembleTimer_run_goallist (goal);
			}
		}
	}

	freeEvent(ep);
	
	u_int64_t delayTick  =  Calculate_Propdelay_time((int)send_length,send_wave ) + Calculate_trans_time((int)send_length, send_wave) + send_packet_num;
	
	output_port_from_short = readpath_->RWA_get_outputport(get_nid(),ipv4addr_to_nodeid(goal));
	Send_release_wave_Event( send_wave,output_port_from_short,delayTick);
	return 1;
}


int obwa::delete_buffer(u_long goal){
	
	buffer_list *ptr = NULL;
	buffer_list *pre = NULL;
	
	printf("%50d delete  buffer\n ",get_nid());
	for(ptr = Buffer_head->first ; ptr ; ptr = ptr->next){
		if (ptr->goal  == goal){
			break;
		}
		if (ptr != Buffer_head->first)
		    	pre = pre->next ;
		else
			pre = Buffer_head->first;
	}

	if (ptr == NULL){
		printf("  WARN!!!! In obwa.cc !! %d no match enent release goal %ld \n ",get_nid(),goal);
		return -1;
	}
	
	else{
		Entity *temp ;
		for (temp = ptr->first ;temp ; temp =temp->next ){
			printf("%d obwa delete buffer\n",get_nid());
			freePacket(temp->pkt);
		}
		/* clean data structure in buffer
		 * */
		if (pre == NULL){			//if buffer being to clear in the first position
			if (ptr->next == NULL){ 	//first ,last , only one
				Buffer_head->first = NULL;
				Buffer_head->last = NULL;
				Buffer_head->list_num = 0;
				free(ptr);
			}
			else{				//first but not last ,more than one
				Buffer_head->first = ptr->next ;
				Buffer_head->list_num --;
				free(ptr);
			}
		}
		else if (ptr->next == NULL){ 		//last not first more than one
			Buffer_head->last = pre;
			pre->next = NULL;
			Buffer_head->list_num --;
			free(ptr);
		}
		else{					//not first ,last more than one
			pre->next = ptr->next ;
			Buffer_head->list_num --;
			free(ptr);
		}
	}

	return(1);
}


int obwa::init() {
	
	char * ndt_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodekindfile)+1);
	sprintf(ndt_FILEPATH,"%s%s",GetConfigFileDir(),nodekindfile);
	char * gph_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodeconnectfile)+1);
	sprintf(gph_FILEPATH,"%s%s",GetConfigFileDir(),nodeconnectfile);
	char * osr_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(ringfile)+1);
        sprintf(osr_FILEPATH,"%s%s",GetConfigFileDir(),ringfile);
	char * osp_FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(nodepathfile)+1);
        sprintf(osp_FILEPATH,"%s%s",GetConfigFileDir(),nodepathfile);
	char * Configure_FileDir = (char *) malloc(strlen(GetConfigFileDir())+1);
	sprintf(Configure_FileDir,"%s",GetConfigFileDir());

	readpath_->Start(ndt_FILEPATH,gph_FILEPATH,osr_FILEPATH,osp_FILEPATH,Configure_FileDir);
	free(Configure_FileDir);			
	free(ndt_FILEPATH);
	free(gph_FILEPATH);
	free(osr_FILEPATH);
	free(osp_FILEPATH);

        MICRO_TO_TICK(Timeout,Timeout);
	
	return(1);  
}
