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

/* Adaptive Distance Vector daemon */
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
#include <string.h>
#include <string>
#include <nctuns_api.h>
#include <ethernet.h>
#include <ip.h>
#include <packet.h>
#include <route/adv/advd.h>
#include <mbinder.h>

using namespace ADV;

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(ADVd);


RtTable::RtTable(){

	len = 0;
	maxlen = 10;
	rtptr = new rt_entry[maxlen];
}


RtTable::~RtTable(){

	delete [] rtptr;
}


/* When partial update, only the entry which is updated since last update
   will be inserted into update packet.
   The following function will return number of entries whose
   receiver flag is on now.
*/
int RtTable::NumOfJustUpdated(void) {
	// return : how many entry whose JustUpdated and Recv_flag are set

	int	count = 0;

	/* r[0] must be inserted into update, so consider from r[1] */
	for(int i=1;i < len;i++)
		if ((rtptr[i].JustUpdated == YES) && (rtptr[i].Recv_flag == ON))
			count++;

	return count;
}


int RtTable::NumOfRecvFlag(void) {
	// return : how many entry whose Recv_flag are set

	int	count = 0;

	/* r[0] must be inserted into update, so consider from r[1] */
	for(int i=1;i < len;i++)
		if (rtptr[i].Recv_flag == YES) count++;

	return count;
}


inline int RtTable::NumOfEntry(void){
	// return : I think it's trivial

	return len;	
}


inline rt_entry* RtTable::head(void){
	// return : the address of routing table

	return rtptr;
}


int  RtTable::addEntry(u_long in_Dst_IP, u_long in_Nexthop_IP, int in_Metric, int in_seqNum, u_long in_seqSignHost, int inRecv)
{
	// search a invalid entry and replace by incoming information, or
	// insert into next to last entry

	int	i;

	if (ifExist(in_Dst_IP) >= 0)
		return(-1);

	for (i=1;i < len;i++) {
		/* Search a invalid entry and replace by new value */
		if (rtptr[i].Dst_IP == 0)
			goto RtTable_addEntry_Replace;
	}

	/* If routing table is full */
	if( len == maxlen ){
		/* YES !! Double the routing table size. */
		rt_entry *tmp = rtptr;
		maxlen *= 2;
		rtptr = new rt_entry[maxlen];
		memcpy( rtptr, tmp, sizeof(rt_entry)*len );
		delete [] tmp;
	}

	rtptr[len].Dst_IP	= in_Dst_IP;
	rtptr[len].Nexthop_IP	= in_Nexthop_IP;
	rtptr[len].Metric	= in_Metric+1;
	rtptr[len].seqNum	= in_seqNum;
	rtptr[len].seqSignHost	= in_seqSignHost;
	rtptr[len].Recv_flag	= inRecv;
	rtptr[len].Pkts_handled	= 0;
	rtptr[len].JustUpdated	= YES;
	len++;

	return 1;

RtTable_addEntry_Replace:

	rtptr[i].Dst_IP		= in_Dst_IP;
	rtptr[i].Nexthop_IP	= in_Nexthop_IP;
	rtptr[i].Metric		= in_Metric+1;
	rtptr[i].seqNum		= in_seqNum;
	rtptr[i].seqSignHost	= in_seqSignHost;
	rtptr[i].Recv_flag	= inRecv;
	rtptr[i].Pkts_handled	= 0;
	rtptr[i].JustUpdated	= YES;

	return 1;

}


inline int RtTable::ifExist(u_long in_Dst_IP){
	// return : the index of entry about in_Dst_IP, or return -1

	for(int i=0; i<len; i++){
		if( rtptr[i].Dst_IP == in_Dst_IP ){
			return i;    // return index value
		}
	}
	return(-1);	
}


inline rt_entry RtTable::getEntry(int index){

	return rtptr[index];
}


int RtTable::updateEntry(const u_long in_Dst_IP, u_long in_Nexthop_IP, int in_Metric, int in_seqNum, u_long in_seqSignHost, int in_Recv_flag){
	// update route table with DSDV algorithm

	int	rt_index = ifExist(in_Dst_IP);

	if (rt_index < 0)
		return(UPDATE_FAIL);/* Not found route to this IP */

	// update by DSDV algorithm....
	if (in_Dst_IP==in_seqSignHost){
		// the path is ok

		if (rtptr[rt_index].seqNum < in_seqNum){
			// incoming sequence number is fresher

			rtptr[rt_index].Nexthop_IP	= in_Nexthop_IP;
			rtptr[rt_index].Metric		= in_Metric+1;
			rtptr[rt_index].seqNum		= in_seqNum;
		        rtptr[rt_index].seqSignHost     = in_seqSignHost;
		        rtptr[rt_index].Recv_flag       = in_Recv_flag;
		        rtptr[rt_index].JustUpdated     = YES;
			return(UPDATE_SUCCESS);

		}else if (rtptr[rt_index].seqNum == in_seqNum){
			// sequence numbers are the same, check metric

			if (rtptr[rt_index].Metric > (in_Metric+1)){
				// get the shorter path	

				rtptr[rt_index].Nexthop_IP	= in_Nexthop_IP;
				rtptr[rt_index].Metric          = in_Metric+1;
				rtptr[rt_index].seqNum		= in_seqNum;
				rtptr[rt_index].seqSignHost     = in_seqSignHost;
				rtptr[rt_index].Recv_flag       = in_Recv_flag;
				rtptr[rt_index].JustUpdated     = YES;
				return(UPDATE_SUCCESS);

			}/* compare the Metric */

		}else{

			return(UPDATE_FAIL);

		}// compare the sequence number

	}else{
		// this is a broken path, check if we use the path through it

		if (ifMtrInf(rt_index)){
			// If ours already have been invalidated, cry :Q
			return(UPDATE_FAIL);

		}else if (rtptr[rt_index].Nexthop_IP == in_Nexthop_IP){

			rtptr[rt_index].Nexthop_IP	= in_Nexthop_IP;
			rtptr[rt_index].Metric		= METRIC_INFINITE;
			rtptr[rt_index].seqNum		= in_seqNum;
			rtptr[rt_index].seqSignHost	= in_seqSignHost;
			rtptr[rt_index].Recv_flag	= in_Recv_flag;
			rtptr[rt_index].JustUpdated	= YES;
			return(UPDATE_TOINF);

		}// check if we use the route through it

	}// check the path validity


	return(UPDATE_FAIL);
}  


int RtTable::setRecvflag(u_long in_Dst_IP, int onoff) {

	int	rt_index = ifExist(in_Dst_IP);

	if (rt_index < 0)
		return(-1);/* Not found route to this IP */

	rtptr[rt_index].Recv_flag	= onoff;
	rtptr[rt_index].JustUpdated	= YES;
	return(1);
}

bool
RtTable::ifJustUpdated(int inRtIndex)
{
	if (inRtIndex >= len)
                return false;

	if ( (rtptr[inRtIndex].JustUpdated == YES) &&\
			(rtptr[inRtIndex].Dst_IP) )
		return true;
	else
		return false;

};

bool
RtTable::ifRecvflagOn(int inRtIndex)
{
	if (inRtIndex >= len)
                return false;

	if ( (rtptr[inRtIndex].Recv_flag == YES) && (rtptr[inRtIndex].Dst_IP) )
		return true;
	else
		return false;
};

int
RtTable::setJustUpdated(int inRtIndex, int yesno)
{
	if (inRtIndex >= len)
                return 0;

	rtptr[inRtIndex].JustUpdated	= yesno;
	return(1);
};

int
RtTable::hdledCntHalf(int inRtIndex)
{
	// The pkts_handled should be halved whenever the entry is broadcased
	// once.

	if (inRtIndex >= len)
		return 0;

	rtptr[inRtIndex].Pkts_handled /= 2;
	return(1);
};

int
RtTable::pktsHandled(int inRtIndex)
{
	if (inRtIndex >= len)
		return (0);

	return( rtptr[inRtIndex].Pkts_handled );
};

int
RtTable::hdledCntIncre(int inRtIndex)
{
	// Every time forwarding a packet by the entry, increment its count

	if (inRtIndex >= len)
		return 0;

	rtptr[inRtIndex].Pkts_handled++;
	return 1;
};

bool
RtTable::ifMtrInf(int inRtIndex)
{
	// If the entry's metric is invalid/infinite
	// 	In DSDV, the sign_host will be different from Dst_IP
	//	when it is invalid.

	if (inRtIndex >= len)
                return 0;
	if (rtptr[inRtIndex].Dst_IP != rtptr[inRtIndex].seqSignHost)
		return true;
	else
		return false;

};

int
RtTable::rmEntry(int inRtIndex)
{
	if (inRtIndex >= len)
		return 0;

	if (inRtIndex == (len-1)){
		return(--len);
	}else{
		len--;
		rtptr[inRtIndex].Dst_IP		= rtptr[len].Dst_IP;
		rtptr[inRtIndex].Nexthop_IP	= rtptr[len].Nexthop_IP;
		rtptr[inRtIndex].Metric		= rtptr[len].Metric;
		rtptr[inRtIndex].seqNum		= rtptr[len].seqNum;
		rtptr[inRtIndex].seqSignHost	= rtptr[len].seqSignHost;
		rtptr[inRtIndex].Recv_flag	= rtptr[len].Recv_flag;
		rtptr[inRtIndex].Pkts_handled	= rtptr[len].Pkts_handled;
		rtptr[inRtIndex].JustUpdated	= rtptr[len].JustUpdated;
	}
	return 1;

};

int
RtTable::setSeqNow(int inSeqNum)
{
	// Set my sequence to new value

	rtptr[0].seqNum = inSeqNum;
	return(1);
};

int
RtTable::NumOfPartial(void)
{
	// How many entry in route table can be included in partial update

	int	count = 0;
	for (int i = 1;i < len;i++)
		if ( (rtptr[i].JustUpdated == YES)||\
			(ifMtrInf(i) &&\
			(rtptr[i].Recv_flag == ON)) )
			count++;
	return (count+1);// 1 for rtptr[0]
};


/********************** imp-of-rt_table *****************************/
/********************************************************************/

// Most function of ctrlTable are similar to RtTable's.

ctrlTable::ctrlTable(){

	len = 0;
	maxlen = 10;
	ctrlptr = new ctrl_entry[maxlen];
}


ctrlTable::~ctrlTable(){

	delete [] ctrlptr;
}


inline ctrl_entry* ctrlTable::head(void) {

	return ctrlptr;
}


int ctrlTable::addEntry(u_long in_Dst_IP, int in_b_id) {

	/* If the control table is full */
	if (len == maxlen) {
		ctrl_entry* temp = ctrlptr;
		maxlen *= 2;
		ctrlptr = new ctrl_entry[maxlen];
		memcpy( ctrlptr, temp, sizeof(ctrl_entry)*len);
		delete [] temp;
	}/* if table is full */

	ctrlptr[len].Dst_IP		= in_Dst_IP;
	ctrlptr[len].b_id		= in_b_id;
	ctrlptr[len].buffer_list	= NULL;
	len++;

	return 1;
}/* ctrlTable::addEntry(u_long, int) */


int ctrlTable::ifExist(u_long in_Dst_IP) {

	for (int i=0;i < len;i++) {
		if (ctrlptr[i].Dst_IP == in_Dst_IP)
			return i;
	}/* for loop */

	/* NOT FOUND */
	return(-1);
}


inline ctrl_entry ctrlTable::getEntry(int index) {

	return ctrlptr[index];
}


int ctrlTable::rmEntry(u_long in_Dst_IP) {

	int	ctrl_index = ifExist(in_Dst_IP);

	if (ctrl_index < 0)
		return(-1);/* Not found control entry for this IP */

	len--;
	ctrlptr[ctrl_index].Dst_IP	= ctrlptr[len].Dst_IP;
	ctrlptr[ctrl_index].b_id	= ctrlptr[len].b_id;
	ctrlptr[ctrl_index].buffer_list	= ctrlptr[len].buffer_list;

	return(1);
}


int ctrlTable::attachPKT(u_long in_Dst_IP, ePacket_ *pkt) {
	// When a packet can't be routed, buffer it first.
	//	The buffer structure are like this:
	//		1. Packet are sorted by their destination IP
	//		2. Packets are linked in a linked list

	int	pktCount = 0;// Use to check if exceed BUF_THRESHOLD
	int	ctrl_index = ifExist(in_Dst_IP);

	if (ctrl_index < 0)
		return(-1);/* Not found control entry for this IP */

	struct buf_list*	plist;
	if ( ctrlptr[ctrl_index].buffer_list == NULL ){

		plist = (struct buf_list*)malloc(sizeof(struct buf_list));
		plist->queued_pkt	= pkt;
		plist->next		= NULL;
		ctrlptr[ctrl_index].buffer_list = plist;
		pktCount++;

	}else{

		pktCount++;
		for (plist = ctrlptr[ctrl_index].buffer_list;\
			plist->next != NULL;plist = plist->next)
			pktCount++;

		plist->next = (struct buf_list*)malloc(sizeof(struct buf_list));
		plist->next->queued_pkt 	= pkt;
		plist->next->next		= NULL;
		pktCount++;

	}

	// It is limited in quantity of packets related to a ctrl_entry
	// When exceeded, ADV mechanism will handled it.
	if (pktCount >= ctrlThreshold)
		return(YES);
	else
		return(NO);
}/* ctrlTable::attachPKT(u_long, ePacket_*) */


int ctrlTable::updateEntry(u_long in_Dst_IP, int in_b_id){
	// refresh the broadcast ID to prevent from broadcast storm

	int	ctrl_index = ifExist(in_Dst_IP);

	if (ctrl_index < 0)
		return(-1);/* Not found control entry for this IP */

	ctrlptr[ctrl_index].b_id	= in_b_id;

	return(1);
}


int ctrlTable::getBid(u_long in_Dst_IP) {

	int	ctrl_index = ifExist(in_Dst_IP);

	if (ctrl_index < 0)
		return(0);/* Not found control entry for this IP */

	return(ctrlptr[ctrl_index].b_id);
}


int ctrlTable::ifPktBuffered(u_long in_Dst_IP) {

	int	ctrl_index = ifExist(in_Dst_IP);

	if (ctrl_index < 0)
		return(-1);/* Not found control entry for this IP */

	if (ctrlptr[ctrl_index].buffer_list != NULL)
		return(YES);
	else
		return(NO);
}

void
ctrlTable::bidIncre(u_long in_Dst_IP)
{
	// This can only be called by the broadcast source host.

	int	ctrl_index = ifExist(in_Dst_IP);
	if (ctrl_index < 0)
		return;

	ctrlptr[ctrl_index].b_id += 2;
	return;
}



/********************** im-of-ctrl ************************************/
/**********************************************************************/

ADVd::ADVd(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	s_flowctl = DISABLED;
	r_flowctl = DISABLED; 

	/* Initialize */
	sequence_now = 0;
	vBind("TRGMETER_HIGH", &TRGMETER_HIGH);
	vBind("TRGMETER_MED", &TRGMETER_MED);
	vBind("TRGMETER_LOW", &TRGMETER_LOW);
	vBind("TRGMETER_FULL", &TRGMETER_FULL);
	vBind("MIN_INTERVAL", &MIN_INTERVAL);
	vBind("BUF_THRESHOLD", &BUF_THRESHOLD);
	vBind("MAX_BUFFERED_PKT", &MAXBUFFEREDPKT);

	ctrl_table.setThreshold(BUF_THRESHOLD);
}


ADVd::~ADVd() {

};


int ADVd::init() {


	trigger_meter = 0;
	trigger_thresh = TRGMETER_HIGH;	/* Default value */
	network_speed = HIGH_SPEED;
	num_partial = 0;
	sum_TRGMETER = 0;
	buf_pkt_cnt = 0;
	begin_tick = GetNodeCurrentTime(get_nid());
	thre_tick = GetNodeCurrentTime(get_nid());

	qcur_ = 0;
	rd_head = rd_tail = 0;
	qmax_ = 30;

	/* get my ip address */
	myip = GET_REG_VAR(get_port(), "IP", u_long*);

	/* fill local information in routing table */
	// So the first entry of route table is myself.
	rt_table.addEntry(*myip, *myip, -1, sequence_now, *myip, OFF);
     	sequence_now += 2;
     	
	return(1); 
}/* init(void) */


int ADVd::partialUpdate(void) {
	
	u_long		toip = inet_addr("1.0.1.255");
	ePacket_	*epkt;
	epkt = createEvent();
	Packet		*pkt = new Packet;
	char		*pPktType;
	update_entry	*pUpdate;

	// alloc enough space to keep routing msg
	int len_ = rt_table.NumOfPartial();
	pPktType = (char*)pkt->pkt_malloc(11+sizeof(int));
	pUpdate = (update_entry*)pkt->pkt_sattach(sizeof(update_entry)*len_);
	assert(pUpdate);
	if (pkt->pkt_sprepend((char*)pUpdate, sizeof(update_entry)*len_) < 0)
		return(-1);
	
	/* Insert routing entries into update */
	// first, add seq_num, then fill it in the adv_msg
	rt_table.setSeqNow(sequence_now);
	sequence_now +=2;
	convert(pUpdate , 0);
	memcpy(pPktType, (char*)PARTIAL_UPDATE, sizeof(char[11]));
	// Indicate how many entry are there.....
	memcpy(pPktType+11, (char*)&len_, sizeof(int));


	// fill in other routing information
	for( int i=1;i < rt_table.NumOfEntry(); ++i ){
		/* This is just PARTIAL UPDATE */
		if ( rt_table.ifJustUpdated(i) || \
			(rt_table.ifRecvflagOn(i) && rt_table.ifMtrInf(i)) ){

			pUpdate++;
			convert((update_entry*)pUpdate, i);

			/* some routines */
			rt_table.hdledCntHalf(i);
			rt_table.setJustUpdated(i, OFF);
		}
	}	


	// attach packet(p) to event(epkt) 
	pkt->rt_setgw(toip);
	pkt->pkt_setflow(PF_SEND);
	ATTACH_PKT(pkt, epkt);
	//SET_EVENT_CALLOUTOBJ(epkt, NULL, NULL, (void*)pkt);

	/* Reset variable for computation of trigger threshold */
	num_partial++;
	sum_TRGMETER += trigger_meter;
	trigger_meter = 0;

	return(sendToQueue(epkt));
}/* partialUpdate(void) */


int ADVd::fullUpdate(void) {


	u_long		toip = inet_addr("1.0.1.255");
	ePacket_        *epkt;
	epkt = createEvent();
	Packet		*pkt = new Packet;
	update_entry	*pUpdate;
	char		*pPktType;

	// alloc enough space to keep routing msg
	int len_ = rt_table.NumOfRecvFlag() + 1;	/* 1 for r[0] */
	pPktType = (char*)pkt->pkt_malloc(11+sizeof(int));
	pUpdate = (update_entry*)pkt->pkt_sattach(sizeof(update_entry)*len_);
	assert(pUpdate);
	if (pkt->pkt_sprepend((char*)pUpdate, sizeof(update_entry)*len_) < 0)
		return(-1);

	// first, add seq_num, then fill it in the update message
	rt_table.setSeqNow(sequence_now);
	sequence_now +=2;
	convert((update_entry*)pUpdate, 0);
	memcpy(pPktType, (char*)FULL_UPDATE, sizeof(char[11]));
	memcpy(pPktType+11, (char*)&len_, sizeof(int));

	// fill in other routing information
	for( int i=1;i < rt_table.NumOfEntry(); i++ ){
		/* This is Full Update */
		if ( rt_table.ifRecvflagOn(i) ) {

			pUpdate++;
			convert((update_entry*)pUpdate, i);

			/* some routines */
			rt_table.hdledCntHalf(i);
			rt_table.setJustUpdated(i, OFF);
		}
	}

	// attach packet(p) to event(epkt)
	pkt->rt_setgw(toip);
	pkt->pkt_setflow(PF_SEND);
	ATTACH_PKT(pkt, epkt);

	/* Reset variable and compute trigger threshold */
	trigger_meter = 0;
	compute_thresh();

	return(sendToQueue(epkt));
}/* fullUpdate(void) */


int ADVd::compute_thresh(void) {

	int		count = num_partial;
	int		sum = sum_TRGMETER;
	int		len = rt_table.NumOfEntry();
	int		max_count;

	/* Reset as soon as possible */
	num_partial = 0;
	sum_TRGMETER = 0;


	/* For non-active nodes, i.e. neither active receiver nor forward-
	   ing node, we will discourage them from transmitting updates
	*/
	if (rt_table.ifRecvflagOn(0))
		goto I_AM_ACTIVE_NODE;

	for (int i=1;i < len;i++)
		if ((rt_table.pktsHandled(i) > 0) && rt_table.ifRecvflagOn(i))
			goto I_AM_ACTIVE_NODE;

	trigger_thresh = TRGMETER_HIGH;	/* Paper: ... is set to a high */
					/* constant value.	       */
	thre_tick = GetNodeCurrentTime(get_nid());
	return 1;


I_AM_ACTIVE_NODE:

	if ( count == 0 ) {/* No triggered update since last full update */
		trigger_thresh = TRGMETER_HIGH;	/* Following paper */
		thre_tick = GetNodeCurrentTime(get_nid());
		return 1;
	}

	thre_tick = GetNodeCurrentTime(get_nid()) - thre_tick;

	/* The following sentence is from config.h/MILLI_TO_TICK macro */
	max_count = (int)(thre_tick / (u_int64_t)(MIN_INTERVAL*(1000000.0/TICK)));
	if ( count < (max_count / 4) ) {
		trigger_thresh /= 2;
		thre_tick = GetNodeCurrentTime(get_nid());
		return 1;
	}

	trigger_thresh = ( trigger_thresh + (sum/count) ) / 2;
	thre_tick = GetNodeCurrentTime(get_nid());
	return 1;
}/* compute_thresh(void) */


int ADVd::processUpdate(ePacket_ *epkt) {
	/* When receiving ADV update pkt(either partial or full one, 
	   process one by one.
	*/
	Packet	*pkt;
	GET_PKT(pkt, epkt);
	assert(pkt);


	int		len_, updateResult;
	memcpy(&len_, pkt->pkt_get()+11, sizeof(int));

	update_entry	*pUpdate;
	u_long		updateSrcIP;
	pUpdate = (update_entry*)pkt->pkt_sget();
	// The 1st entry must be the last_host_IP
	memcpy(&updateSrcIP, &pUpdate->Dst_IP, sizeof(u_long));
	// If 1st is not a active receiver, ignore it
	if (pUpdate->Is_receiver == NO){
		pUpdate++;
		len_--;
	}

	/* here start to process each entry in ADV algorithm */
	for(int i=0;i < len_;++i, ++pUpdate){


		if ( rt_table.ifExist(pUpdate->Dst_IP) < 0 ) {
			/* No route entry to this Dst */

			rt_table.addEntry(pUpdate->Dst_IP, updateSrcIP,\
				pUpdate->Metric, pUpdate->seqNum,\
				pUpdate->seqSignHost, pUpdate->Is_receiver);

		}else{
			// Yes, then update our own entry
			if ((updateResult = rt_table.updateEntry(\
				pUpdate->Dst_IP, updateSrcIP,\
				pUpdate->Metric, pUpdate->seqNum,\
				pUpdate->seqSignHost,\
				pUpdate->Is_receiver))==UPDATE_SUCCESS){

				// Since we have fresher route,
				//	 check buffered list
				int	ctrlIndex = ctrl_table.ifExist(pUpdate->Dst_IP);
				if (ctrlIndex >= 0)
				{
					processBuffered(pUpdate->Dst_IP);
					ctrl_table.rmEntry(pUpdate->Dst_IP);
				}

			}else if(updateResult == UPDATE_TOINF){
				// We know the route have been invalidated
				// After 5 sec., delete it to learn a new one
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(ADVd, refreshRtTable);
				IF_timerRefresh.setCallOutObj(this, type);
				IF_timerRefresh.start((u_int64_t)(DEL_INF_ENTRY*1000000.0/TICK), 0);
			}

		}// If I have the rt_entry

		if (expected(pUpdate->Exp_response)==2)
		{
			int	RtIndex = rt_table.ifExist(pUpdate->Dst_IP);
			rt_table.setJustUpdated(RtIndex, YES);

		}// Someone need this route

	}/* End of For-loop for processing every entry within update */

	freePacket(epkt);// Needn't send forward

	return 1;
}/* processUpdate(ePacket_ *p) */


int ADVd::expected(int EXPvalue) {

	switch(EXPvalue) {

		case EXP_HIGH:
			updateTrgmeter(TRGMETER_HIGH);
			return 2;
		case EXP_MEDIUM:
			updateTrgmeter(TRGMETER_MED);
			return 1;
		case EXP_LOW:
			updateTrgmeter(TRGMETER_LOW);
			return 1;
		case EXP_ZERO:
			return 1;
		default:
			return(-1);
	}

}/* expected(int value) */


int ADVd::convert(update_entry* u, int inRtIndex) {

	rt_entry	myRtEntry = rt_table.getEntry(inRtIndex);

	u->Dst_IP	= myRtEntry.Dst_IP;
	u->Nexthop_IP	= myRtEntry.Nexthop_IP;
	u->Metric	= myRtEntry.Metric;
	u->seqNum	= myRtEntry.seqNum;
	u->seqSignHost	= myRtEntry.seqSignHost;
	u->Is_receiver	= myRtEntry.Recv_flag;
	
	if (ctrl_table.ifPktBuffered(myRtEntry.Dst_IP) == YES)
		u->Exp_response = EXP_HIGH;
	else if ( (myRtEntry.Pkts_handled > 0) &&\
			(network_speed == HIGH_SPEED) )
		u->Exp_response = EXP_MEDIUM;
	else if ( (myRtEntry.Pkts_handled > 0) &&\
			(network_speed == LOW_SPEED) )
		u->Exp_response = EXP_LOW;
	else
		u->Exp_response = EXP_ZERO;

	return 1;

}/* convert(const update_entry* u, int inRtIndex) */


int ADVd::updateTrgmeter(int flag){


	trigger_meter += flag;
	checkTrgmeter();

	return(1);
}// updateTrgmeter(int)


int
ADVd::checkTrgmeter(void)
{
	if ( trigger_meter > TRGMETER_FULL ) {

		// If the interval is over MIN_INTERVAL ms since last update ?

		if ( (GetNodeCurrentTime(get_nid()) - begin_tick) < \
			(u_int64_t)(MIN_INTERVAL*(1000000.0/TICK)) ) {
			/* Not trigger time */
			/* Set timer to trigger */

			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(ADVd, fullUpdate);
			IF_timer.setCallOutObj(this, type);
			IF_timer.start((u_int64_t)(MIN_INTERVAL*(1000000.0/TICK)) - (GetNodeCurrentTime(get_nid()) - begin_tick), 0);

			begin_tick = GetNodeCurrentTime(get_nid());

		}else{
			/* Trigger now */

			fullUpdate();
			begin_tick = GetNodeCurrentTime(get_nid());

		}/* if interval > 500ms */

	}else if ( trigger_meter > trigger_thresh ) {
		/* If the interval is over 500ms since last update ? */

		if ( (GetNodeCurrentTime(get_nid()) - begin_tick) < \
			(u_int64_t)(MIN_INTERVAL*(1000000.0/TICK)) ) {
			/* Not trigger time */
			/* Set timer to trigger */

			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(ADVd, partialUpdate);
			IF_timer.setCallOutObj(this, type);
			IF_timer.start((u_int64_t)(MIN_INTERVAL*(1000000.0/TICK)) - (GetNodeCurrentTime(get_nid()) - begin_tick), 0);

			begin_tick = GetNodeCurrentTime(get_nid());

		}else{

			partialUpdate();
			begin_tick = GetNodeCurrentTime(get_nid());

		}/* if interval > 500ms */

	}/* if it is full or partial update */


	return(1);

}/* checkTrgmeter(void) */


int ADVd::send(ePacket_ *epkt) {

 	Packet			*pkt; 
	u_long			in_Dst_IP;


	GET_PKT(pkt, epkt);
	rt_table.setRecvflag(*myip, ON);// This func. is called by upper
					// level and so I must be a active
					// receiver.

	/* get destination ip address. */
	IP_DST(in_Dst_IP, pkt->pkt_sget());
	if (is_ipv4_broadcast(get_nid(), in_Dst_IP))
		return(sendToQueue(epkt));

	//debug start
	/*{
	  u_long src_ip;
	  struct in_addr dst, src;
	  IP_SRC(src_ip, pkt->pkt_sget());
	  dst.s_addr = in_Dst_IP;
	  src.s_addr = src_ip;
	  printf("in pkt send nid ==%d , dst_ip==%s, src_ip==%s\n", get_nid(), inet_ntoa(dst), inet_ntoa(src));
	}

*/
	//debug end
	/* do routing and ADV's algorithm about new route */ 
	int routingResult;
	routingResult = routing(in_Dst_IP, pkt);
	//fprintf(stderr , "[%s]nid:%d routingResult:%d\n" , __func__ , get_nid() , routingResult);
	if ( routingResult == NOROUTE) {

		/* The first packet wanted this route */
		/* Start flooding init-connection packet */
		// Insert into route table and mark as INFINITEENTRY
		rt_table.addEntry(in_Dst_IP, 0, METRIC_INFINITE,\
			-1, 0, ON);

		if (ctrl_table.ifExist(in_Dst_IP) < 0)
		{
			/* Add a new entry into ctrl_table, */
			/* and enque the packet to wait for route */
			ctrl_table.addEntry(in_Dst_IP, 0);
			if (buf_pkt_incre()==YES){
				if (ctrl_table.attachPKT(in_Dst_IP, epkt)==YES)
					updateTrgmeter(TRGMETER_MED);
			}else{
				freePacket(epkt);
				updateTrgmeter(TRGMETER_MED);
			}

			/* copy my information */
			rt_entry	temp = rt_table.getEntry(0);
			floodingInit(in_Dst_IP, temp.Dst_IP, temp.Metric,\
				temp.seqNum, temp.seqSignHost);
		}else{
			// It may already exist from perious learning
			if (buf_pkt_incre()==YES){
				if (ctrl_table.attachPKT(in_Dst_IP, epkt)==YES)
					updateTrgmeter(TRGMETER_MED);
			}else{
				freePacket(epkt);
				updateTrgmeter(TRGMETER_MED);
			}
		}

	}else if( routingResult == INFINITEENTRY ){

		if (ctrl_table.ifExist(in_Dst_IP) < 0)
			ctrl_table.addEntry(in_Dst_IP, 0);

		if (buf_pkt_incre()==YES){
			if (ctrl_table.attachPKT(in_Dst_IP, epkt)==YES)
				updateTrgmeter(TRGMETER_MED);
		}else{
			freePacket(epkt);
			updateTrgmeter(TRGMETER_MED);
		}
			
	}else{
		/* Route successfully */
		// Whenever rd_queue is empty and s_queue is empty,
		// send out ASAP
		sendToQueue(epkt);
	}// if routing() ....

	return(1);

}/* send(ePacket* pkt) */


int ADVd::recv(ePacket_ *epkt) {

	Packet			*pkt;
	u_long			in_Dst_IP;
	struct ip		*pIP;

	GET_PKT(pkt, epkt);
	assert(pkt);
	pIP = (struct ip*)pkt->pkt_sget();


	/* copy incoming packet type */
	char	pktTYPE[11];
	strncpy(pktTYPE, (char*)pkt->pkt_get(), 10);
	//pktTYPE[10] = NULL;
	pktTYPE[10] = '\0';

	/* What is the incoming packet type ? */
	if ((!strcmp(pktTYPE, PARTIAL_UPDATE)) ||\
				(!strcmp(pktTYPE, FULL_UPDATE)))
		return(processUpdate(epkt));
	else if (!strcmp(pktTYPE, INIT_CONNECTION))
	{	
		//fprintf(stderr , "[%s]NID:%d floodying\n" , __func__ , get_nid());
		return(processInit(epkt));
	}	
	else if (!strcmp(pktTYPE, RECV_ALERT))
		return(processAlert(epkt));
	/* else ...... */

	/* Get the packet's destination IP */
	IP_DST(in_Dst_IP, pkt->pkt_sget());


/* hwchu: moved to below
	// It is decreased first, then checked. The equality operator proves.
	// Check its Time To Live
	if (pIP->ip_ttl <= ADVIPTTLDEC){
		return(put(epkt, recvtarget_));
	}
	// Or decrease it
	pIP->ip_ttl -= ADVIPTTLDEC;
*/

	/* Check if it's mine */
	if(*myip == in_Dst_IP || is_ipv4_broadcast(get_nid(), in_Dst_IP)) {
		/* Yes */
		//printf("recevie broad cast msg\n");
		return(put(epkt, recvtarget_));   
	}else{
		/* No */

		/* hwchu:
		 *   There is no chance for this packet to enter the kernel, 
		 *   so we decrement its TTL here.
		 */
		u_char ttl;

		GET_IP_TTL(ttl, pkt->pkt_sget());
		if (ttl <= 1) {
			return put(epkt, recvtarget_);
		}
		IP_DEC_TTL(pkt->pkt_sget());

		pkt->pkt_setflow(PF_SEND);
		int	routingResult;
  		if ((routingResult = routing(in_Dst_IP, pkt))==NOROUTE){
			/* It's none of my businuse */

			freePacket(epkt);
			return(1); 
		}else if(routingResult == INFINITEENTRY){
			// Route is still invalid .....

			if (ctrl_table.ifExist(in_Dst_IP) < 0)
				ctrl_table.addEntry(in_Dst_IP, 0);

			if (buf_pkt_incre()==YES){
				if (ctrl_table.attachPKT(in_Dst_IP, epkt)==YES)
					updateTrgmeter(TRGMETER_MED);
			}else{
				// if we can't buffer any more packet...
				freePacket(epkt);
				updateTrgmeter(TRGMETER_MED);
			}
			return(1);
		}else{

		/* Routing success */
		return(sendToQueue(epkt));  
		}	

	}

}/* recv(ePacket_* pkt); */


int ADVd::processInit(ePacket_ *pkt){

	Packet		*p;
	init_conn	*pin_init;
	u_long		from_IP;

	GET_PKT(p, pkt);
	pin_init = (init_conn*)p->pkt_get();
	memcpy(&from_IP,&(pin_init->lasthost), sizeof(u_long));


	/* Is it for me ?? */
	if (pin_init->wanted.Dst_IP == *myip){


		rt_entry	my_rt_entry = rt_table.getEntry(0);

		// According paper, it should be learned from update
		if (my_rt_entry.Recv_flag == NO){

			replyRecvalert(*myip, 0, sequence_now, *myip);
			rt_table.setRecvflag(*myip, YES);

		}/* I have not been a active receiver yet */

		int	index_rt = rt_table.ifExist(pin_init->sIP);
		if ( index_rt < 0 ){
			/* I have no idea for the sender */
			/* Add into rt_table */
			rt_table.addEntry(pin_init->sIP, from_IP,\
				pin_init->sMetric, pin_init->sSeqNum,\
				pin_init->sSeqSignHost, ON);
		}else{
			/* Update the route entry */
			rt_table.updateEntry(pin_init->sIP, from_IP,\
				pin_init->sMetric, pin_init->sSeqNum,\
				pin_init->sSeqSignHost, ON);
		}/* If I know the original sender */

		goto PROCESS_INIT_SIDE_EFFECT;

	}/* If the init-connection is for me */


	// Check route table if it exist
	int		rt_index, ctrl_index;
	if ( (rt_index = rt_table.ifExist(pin_init->wanted.Dst_IP)) > 0)
	{
		// Yes, I have. But I'm not sure its validity
		if (rt_table.ifMtrInf(rt_index))
		{
			// Invalid. So I need learning.
			// If I have entry in ctrl_table
			rt_table.rmEntry(rt_index);
			rt_table.addEntry(pin_init->wanted.Dst_IP,\
				0, METRIC_INFINITE, -1, 0, ON);
			goto LEARNING_IN_INIT;

		}// If the entry is still valid?

		// It's valid. But avoid infinite-loop
		rt_table.setRecvflag(pin_init->wanted.Dst_IP, ON);
		rt_entry	wantedEntry = rt_table.getEntry(rt_index);
		if (wantedEntry.Nexthop_IP == from_IP)
		{
			// So now I need learning too.
			rt_table.rmEntry(rt_index);
			rt_table.addEntry(pin_init->wanted.Dst_IP,\
				0, METRIC_INFINITE, -1, 0, ON);
			goto LEARNING_IN_INIT;
		}
		// I will reply what I know to save bandwidth.
		replyRecvalert(pin_init->wanted.Dst_IP,\
			wantedEntry.Metric, wantedEntry.seqNum,\
			wantedEntry.seqSignHost);
		goto PROCESS_INIT_SIDE_EFFECT;

	}else{
		// No, I need learn too.
		rt_table.addEntry(pin_init->wanted.Dst_IP,\
				0, METRIC_INFINITE, -1, 0, ON);
		goto LEARNING_IN_INIT;

	}// if I have route in my route table

LEARNING_IN_INIT:

	if ((ctrl_index = ctrl_table.ifExist(pin_init->wanted.Dst_IP)) < 0)
	{
		ctrl_table.addEntry(pin_init->wanted.Dst_IP,\
					pin_init->wanted.b_id);
		floodingInit(pin_init->wanted.Dst_IP,\
			pin_init->sIP, pin_init->sMetric+1,\
			pin_init->sSeqNum, pin_init->sSeqSignHost);

	}else if(ctrl_table.getBid(pin_init->wanted.Dst_IP) >=\
		pin_init->wanted.b_id)
	{
		// My broadcast ID is fresher, stop to avoid
		// broadcast storm
		
	}else{
		// Update broadcast ID and flood forward
		ctrl_table.updateEntry(pin_init->wanted.Dst_IP,\
					pin_init->wanted.b_id);
		floodingInit(pin_init->wanted.Dst_IP,\
			pin_init->sIP, pin_init->sMetric+1,\
			pin_init->sSeqNum, pin_init->sSeqSignHost);
	}// Check ctrl_table

PROCESS_INIT_SIDE_EFFECT:
		/* We may learn a route from this init-connection */

		if (rt_table.ifExist(pin_init->sIP) < 0){

			rt_table.addEntry(pin_init->sIP,\
					from_IP, pin_init->sMetric,\
					pin_init->sSeqNum,\
					pin_init->sSeqSignHost, ON);

		}else{
			rt_table.updateEntry(pin_init->sIP, from_IP,\
					pin_init->sMetric, pin_init->sSeqNum,\
					pin_init->sSeqSignHost, ON);

		}/* Learn how to get the Source */

		if ((rt_index = rt_table.ifExist(from_IP)) < 0){

			/* We can learn it but not use it now */
			rt_table.addEntry(from_IP, from_IP, 0, 0, from_IP, OFF);

		}// else{

			// Check validity of the entry in table
			// ccli: But I think it's safe to take it
			//rt_table.rmEntry(rt_index);
			//rt_table.addEntry(from_IP, from_IP, 0, 0, from_IP, OFF);

		//}

		freePacket(pkt);
		return(1);
}/* ADVd::processInit(Packet *) */


int ADVd::floodingInit(u_long in_Dst_IP, u_long in_sIP, int in_sMetric, int in_sSeqNum, u_long in_sSeqSignHost) {

	int	ctrl_index = ctrl_table.ifExist(in_Dst_IP);
	if (ctrl_index < 0){
		/* Add a new entry into ctrl_table */

		ctrl_table.addEntry(in_Dst_IP, 0);
		ctrl_index = ctrl_table.ifExist(in_Dst_IP);
	}

	// Only the Source can increment the broadcast ID
	if (in_sIP == *myip)
		ctrl_table.bidIncre(in_Dst_IP);/* Increment by 2 */

	ctrl_entry	dst_entry = ctrl_table.getEntry(ctrl_index);
	init_conn	*pinit_conn;
	u_long		toip = inet_addr("1.0.1.255");
	ePacket_	*pkt;
	Packet		*p = new Packet;
	pkt = createEvent();

	pinit_conn = (init_conn*)p->pkt_malloc(sizeof(init_conn));
	memcpy(&pinit_conn->lasthost, myip, sizeof(u_long));
	memcpy(pinit_conn->pktTYPE, (char*)INIT_CONNECTION, sizeof(char[11]));
	memcpy(&pinit_conn->wanted, &dst_entry, sizeof(ctrl_entry));
	pinit_conn->sIP		= in_sIP;
	pinit_conn->sMetric	= in_sMetric;
	pinit_conn->sSeqNum	= in_sSeqNum;
	pinit_conn->sSeqSignHost= in_sSeqSignHost;
	pinit_conn->wanted.buffer_list = NULL;


	/* attach packet(p) to event(pkt) */
	p->rt_setgw(toip);
	p->pkt_setflow(PF_SEND);
	ATTACH_PKT(p, pkt);


	return(sendToQueue(pkt));

}/* ADVd::floodingInit(u_long, rt_entry) */


int ADVd::replyRecvalert(u_long in_Src_IP, int in_metric, int in_seqNum, u_long in_seqSignHost){

	u_long		toip = inet_addr("1.0.1.255");
	ePacket_	*epkt = createEvent();
	Packet		*p = new Packet;
	recv_alert	*precv = (recv_alert*)p->pkt_malloc(sizeof(recv_alert)+sizeof(u_long));


	memcpy(precv->pktTYPE, (char*)RECV_ALERT, sizeof(char[11]));
	memcpy(&precv->replier, &in_Src_IP, sizeof(u_long));
	precv->metric = in_metric;// The metric is mine, and you must
	precv->seqNum = in_seqNum;	// increment for yourself
	precv->seqSignHost = in_seqSignHost;
	memcpy(&precv->lasthost, myip, sizeof(u_long));

	p->rt_setgw(toip);
	p->pkt_setflow(PF_SEND);
	ATTACH_PKT(p, epkt);

	return(sendToQueue(epkt));

}/* ADVd::replyRecvalert(u_long, int, u_long) */


int ADVd::processAlert(ePacket_ *epkt) {

	Packet		*p;
	u_long		lasthost_IP;
	recv_alert	*precv;

	GET_PKT(p, epkt);
	precv = (recv_alert*)p->pkt_get();
	memcpy(&lasthost_IP, &precv->lasthost, sizeof(u_long));


	/* Save the original host IP, which flooding this recv-alert */
	u_long in_Src_IP = precv->replier;

	/* Add/update a new route to rt_table */
	int	rt_index = rt_table.ifExist(in_Src_IP);
	if (rt_index > 0){
		rt_table.updateEntry(in_Src_IP, lasthost_IP, precv->metric,\
			precv->seqNum, precv->seqSignHost, ON);
	}else{
		rt_table.addEntry(in_Src_IP, lasthost_IP, precv->metric,\
			precv->seqNum, precv->seqSignHost, ON);
	}

	/* Process the buffered packet */
	int	ctrl_index = ctrl_table.ifExist(in_Src_IP);
	if (ctrl_index >= 0){

		processBuffered(in_Src_IP);

		/* This Alert-packet should be sent forward */
		replyRecvalert(in_Src_IP, precv->metric+1,\
				precv->seqNum, precv->seqSignHost);

		/* Remove the entry in ctrl_table */
		ctrl_table.rmEntry(in_Src_IP);


	}/* If I have ctrl_entry for this in_Src_IP */

	// Learn the neighbor
	if (rt_table.ifExist(lasthost_IP) < 0)
		rt_table.addEntry(lasthost_IP, lasthost_IP, 0, 0,\
					lasthost_IP, OFF);
	else
		rt_table.updateEntry(lasthost_IP, lasthost_IP, 0, 0,\
			lasthost_IP, OFF);

	freePacket(epkt);
	return(1);

}/* ADVd::processAlert(Packet* p) */



int ADVd::processBuffered(u_long wanted_IP) {

	int		ctrl_index = ctrl_table.ifExist(wanted_IP);
	ctrl_entry	theEntry = ctrl_table.getEntry(ctrl_index);
	struct buf_list	*plist = theEntry.buffer_list;

	for (;plist != NULL;plist = plist->next) {
		Packet	*ppp;
		GET_PKT(ppp, plist->queued_pkt);

		ppp->pkt_setflow(PF_SEND);
		if (routing((u_long)theEntry.Dst_IP, ppp) < 0){
			freePacket(plist->queued_pkt);
		}else{


			sendToQueue(plist->queued_pkt);
		}/* if routing() */
		buf_pkt_decre();
		
	}/* For-loop for traversing the buffered ePacket-list */

	//Since sent out all ePkt, free the buf_list
	struct buf_list	*tmp;
	for (plist = theEntry.buffer_list;plist != NULL;){

		tmp = plist->next;
		free(plist);
		plist = tmp;

	}

	return(1);

}/* ADVd::processBuffered(u_long) */


int ADVd::routing(u_long in_Dst_IP, Packet *p) {
	/*//Dirty sol by welljeng 0921,2007
	
	if(is_ipv4_broadcast(get_nid(), in_Dst_IP)){
		u_long toip = inet_addr("1.0.1.255");
		//rt_table.hdledCntIncre(rt_index);

		p->rt_setgw(toip);
		//p->pkt_setflow(PF_SEND);
		return(REACHABLE);
	}
	//End of dirty
	*/
	int	rt_index = rt_table.ifExist(in_Dst_IP);

	if (rt_index < 0){
		/* No route */
		return(NOROUTE);
	}

	rt_entry	theEntry = rt_table.getEntry(rt_index);

	if (theEntry.Dst_IP != theEntry.seqSignHost){
		// Not a valid route entry !
		return(INFINITEENTRY);
	}
	rt_table.hdledCntIncre(rt_index);

	p->rt_setgw(theEntry.Nexthop_IP);
	p->pkt_setflow(PF_SEND);
	return(REACHABLE);

}/* ADVd::routing(u_long, Packet*) */


int ADVd::push(void) {

	ePacket_		*epkt;
	int			(NslObject::*upcall)(MBinder *);


	if (qcur_ > 0){
		epkt = rd_head;
		rd_head = rd_head->next_ep;
		qcur_--;

		upcall = (int (NslObject::*)(MBinder *))&ADVd::push;
		sendtarget_->set_upcall(this, upcall);
		assert(put(epkt, sendtarget_) > 0);
	}/* if (qcur_ > 0) */

	return(1);

}/* ADVd::push(void) */


int ADVd::sendToQueue(ePacket_ *epkt) {

	int			(NslObject::*upcall)(MBinder *);
				/* Do flow control for myself with s_queue */

	if ( sendtarget_->get_curqlen() > 0 ){
		/* Insert to rd_queue */
		if (qcur_ < qmax_){
			if (qcur_ == 0){
				/* The first ePacket_ I want to send */
				rd_head = epkt;
				rd_tail = epkt;
				epkt->next_ep = 0;
			}else{
				rd_tail->next_ep = epkt;
				rd_tail = epkt;
			}/* If (qcur_ == 0) */
			qcur_++;
		}else{
			/* queue is full, drop the ePacket_ */
			freePacket(epkt);
		}/* If (qcur_ < qmax_) */
		return (1);
	}else{
		/* call put() method */
		upcall = (int (NslObject::*)(MBinder *))&ADVd::push;
		Packet	*pkt;
		GET_PKT(pkt, epkt);
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(ADVd, adv_hdler);
		pkt->pkt_setHandler(this, type);
		sendtarget_->set_upcall(this, upcall);
		return(put(epkt, sendtarget_));
	}/* If (squeue->qcur_ > 0) */

}/* ADVd::sendToQueue(ePacket_*) */


int ADVd::adv_hdler(ePacket_ *epkt){

	Packet		*pkt;
	u_long		in_Dst_IP;


	GET_PKT(pkt, epkt);
	IP_DST(in_Dst_IP, pkt->pkt_sget());

	int		rtIndex = rt_table.ifExist(in_Dst_IP);
	rt_entry	myEntry = rt_table.getEntry(rtIndex);

	// If it is not an invalid entry, invalidate it :)
	if (!rt_table.ifMtrInf(rtIndex))
		rt_table.updateEntry(in_Dst_IP, myEntry.Nexthop_IP,\
			METRIC_INFINITE, myEntry.seqNum+1,\
			*myip, myEntry.Recv_flag);

// After 5 second, the routing table will be refresh
// in order to accpet new route entry
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(ADVd, refreshRtTable);
	IF_timerRefresh.setCallOutObj(this, type);
	IF_timerRefresh.start((u_int64_t)(DEL_INF_ENTRY*1000000.0/TICK), 0);

	int		ctrlIndex = ctrl_table.ifExist(in_Dst_IP);
	if (ctrlIndex < 0)
		ctrl_table.addEntry(in_Dst_IP, 0);

	if (buf_pkt_incre()==YES){
		if (ctrl_table.attachPKT(in_Dst_IP, epkt)==YES)
			updateTrgmeter(TRGMETER_MED);
	}else{
		freePacket(epkt);
		updateTrgmeter(TRGMETER_MED);
	}

	return(1);
}/* ADVd::adv_hdler(ePacket_ *) */


int ADVd::buf_pkt_incre(void) {

	if (buf_pkt_cnt < MAXBUFFEREDPKT){
		// Normal condition

		buf_pkt_cnt++;
		return(YES);
	}
	// Exceeded the maximum buffered packets per node
	return(NO);


	return 1;
}/* ADVd::buf_pkt_incre() */


int ADVd::buf_pkt_decre(void) {

	if (buf_pkt_cnt > 0)
		buf_pkt_cnt--;
	return 1;
}/* ADVd::buf_pkt_decre() */


int ADVd::Debugger(char *mystr) {
	char			ip[20];

	ipv4addr_to_str(*myip, ip); 
	printf("	ip %s", ip);
	printf("	MSG: %s\n", mystr);
	return(1);  
 
}


int ctrlTable::dumpctrl(void) {

char ip[20];
printf("--------Control Table--------\n");
printf("Dst_IP		b_id\n");
for (int i=0;i<len;i++){
	ipv4addr_to_str(ctrlptr[i].Dst_IP, ip);
	printf("%s\t\t%d\n", ip, ctrlptr[i].b_id);
}
printf("--------Control Table-----------------------\n");
return (1);
}


int RtTable::dumprt(void) {

char	dst[20], next[20], sign[20];
printf("========RtTable========\n");
printf("Dst_IP\tNextIP\tMetric\tRecv\tseqNum\tsignHost\tPktHdled\tJus\n");
for (int i=0;i<len;i++){
	ipv4addr_to_str(rtptr[i].Dst_IP, dst);
	ipv4addr_to_str(rtptr[i].Nexthop_IP, next);
	ipv4addr_to_str(rtptr[i].seqSignHost, sign);
	printf("%s\t%s\t%d\t%d\t%d\t%s\t\t%d\t\t%d\n",dst,next,rtptr[i].Metric,\
		rtptr[i].Recv_flag, rtptr[i].seqNum, sign,\
		rtptr[i].Pkts_handled, rtptr[i].JustUpdated);
}
printf("========RtTable========================\n");
return 1;
}

int
RtTable::refresh(void)
{
	for (int i = 1; i < len; ++i)
	{
		// Clear the invalid the entries
		if (rtptr[i].Dst_IP != rtptr[i].seqSignHost)
		{
			if ( i == len-1 )
				len--;
			else{
				rtptr[i].Dst_IP		= rtptr[len-1].Dst_IP;
				rtptr[i].Nexthop_IP	= rtptr[len-1].Nexthop_IP;
				rtptr[i].Metric		= rtptr[len-1].Metric;
				rtptr[i].seqNum		= rtptr[len-1].seqNum;
				rtptr[i].seqSignHost	= rtptr[len-1].seqSignHost;
				rtptr[i].Recv_flag	= rtptr[len-1].Recv_flag;
				rtptr[i].Pkts_handled	= rtptr[len-1].Pkts_handled;
				rtptr[i].JustUpdated	= rtptr[len-1].JustUpdated;
				len--;
			}
		}
	}

	return 1;
};
