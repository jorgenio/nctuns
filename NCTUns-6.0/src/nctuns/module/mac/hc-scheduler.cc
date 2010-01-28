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
#include <packet.h>

#include <mac/qos-scheduler.h>
#include <mac/hc-scheduler.h>
#include <mac/mac-802_11e.h>


QosScheduler_HC::QosScheduler_HC (mac802_11e* mac)
		:QosScheduler(mac) 
{
	first_deque 	= 0;
	pre_deque 	= 0;
	first_call	= 1;

   	for(int i = 0 ; i < MAX_NNUM ; i++) 
        	ts_[i]	= 0;

        max_qsta 	= -1;

	Service_Interval= 0;
	SIstart		= 0;
	TXOPstart	= 0;
	pre_SIstart 	= 0;
	pre_TXOPstart 	= 0;
	curr_desc	= 0;	
	pre_desc 	= 0;
        tsNum           = 0;
	head_desc	= 0;
	nodeNum 	= 1;
	pktNum		= 0;

	sifs = mac->phymib->aSIFSTime;
	pifs = mac->phymib->aSlotTime + sifs;

	bzero(qsta,ETHER_ADDR_LEN);
	memset(srcmac , 0 , sizeof(u_char) * 6 * MAX_NNUM);

        if (mac_)
                memcpy(srcmac[0], mac_->mac_, 6);
        
}

QosScheduler_HC::~QosScheduler_HC() 
{
	// deallocate the downlink transmission queues
        for ( int i = 0 ; i < MAX_NNUM ; i++ ) {
                if ( ts_[i] != 0 ) {
                        for ( int j = 0 ; j <= TSID_INDEX ; j++ ) {
                                if ( ts_[i][j] != 0 ) 
					delete ts_[i][j];
                        }
                        delete[] ts_[i];
                }
                ts_[i] = 0;
        }

	/* delete TSPEC List */
        tspec_desc* p = head_desc;
        while ( p != 0 ) {
                tspec_desc* q = p;
                p = p->next;
                delete q;
        }

        head_desc 	= 0;
        curr_desc	= 0;
        tsNum 		= 0;
   	first_call 	= 1;
        max_qsta 	= -1;
}

double QosScheduler_HC::next_CAP() {

	u_int64_t time = GetCurrentTime();

	if(tsNum > 0) {

		if(TXOPstart > time){
			time = TXOPstart - time;
			return time;
		}else 
			return 0;
	} 
	return (-1);	
}

int QosScheduler_HC::pkt_enque (ePacket_ *pkt) {
	
        Packet                  *p;
        struct hdr_mac802_11e   *hm;
	short			tid;
	int			index;
	u_int64_t		time = GetCurrentTime();	

        GET_PKT(p, pkt);
        assert(p);

	hm = (struct hdr_mac802_11e *)p->pkt_get();	
        tid = hm->dh_qos.tid;
	index = getIndex(hm->dh_addr1);

        // check for packet queue overflow
        if ( ts_[index][tid-TSID_INDEX]->qlen() >= MAX_QUEUE_LEN ) 
                return 0;

	p->pkt_addinfo("TIMESTAMP",(char *)&time,8);

        // add the packet to the corresponding transmission queue
        ts_[index][tid-TSID_INDEX]->enque(pkt);
	return 1;
}

int QosScheduler_HC::getIndex(u_char* src)
{
        for (int i = 0; i < nodeNum ; ++i) {
                if (!bcmp(srcmac[i],src,ETHER_ADDR_LEN))
                        return i;
        }

        /* discover a new node */
        memcpy(srcmac[nodeNum++],src,ETHER_ADDR_LEN);
        assert(nodeNum <= MAX_NNUM);

        return (nodeNum-1);
}

/* restore the state previous to the deque of the last packet */
void QosScheduler_HC::recover_status(void)
{
   	first_deque 	= pre_deque;
   	TXOPstart 	= pre_TXOPstart;
   	curr_desc	= pre_desc;
   	SIstart   	= pre_SIstart;
	first_call = 1;
}

void QosScheduler_HC::pkt_deque(HCCA_TxP& hccaTx_)
{
	int		index;
	u_int64_t 	BI_tick, ticks;		//in ticks
        u_int64_t 	poll_time = mac_->TX_Time(sizeof(struct hdr_mac802_11e));
	u_int64_t	now = GetCurrentTime();

	if(curr_desc == NULL)
		return;

	MILLI_TO_TICK(BI_tick, *mac_->Beacon_Timeval);
	MICRO_TO_TICK(ticks, curr_desc->txop);

	if((mac_->last_beacon + BI_tick) < (ticks + now))
		mac_->near_beacon = 1;

	/* Save the current state. */
	pre_TXOPstart 	= TXOPstart;
	pre_desc 	= curr_desc;
	pre_SIstart   	= SIstart;
   	pre_deque	= first_deque;

   	while(1) {

      		if( tsNum == 0 )
         		return;

      		/* If the start time of the current TXOP is greater than the current
      	   	   time, return FALSE */
      		if( TXOPstart > now)
         		return;

      		/* First call in the current service interval */
   		if(first_call) {
      			first_call = 0;
      			SIstart = now;
      			TXOPstart = SIstart;
   		}

      		hccaTx_.tsid = curr_desc->tsid;
      		memcpy(hccaTx_.dst, curr_desc->qsta, ETHER_ADDR_LEN);

      		if(first_deque) 
         		first_deque = 0;


      		// downlink TXOP
      		if(curr_desc->direction == DOWNLINK) {

			Packet		*p;
         		ePacket_	*pkt;
		deque:
         		/* Get a pkt of the curr_desc. If the queue is empty, *
			 * move to the next TSPEC. 			      */
			index = getIndex(hccaTx_.dst);
              		ts_[index][hccaTx_.tsid-TSID_INDEX]->top(pkt);

         		if(!pkt) {
            			first_deque = 1;
            			next_TSPEC();
            			continue;
         		}
		
			GET_PKT(p,pkt);
			assert(p);

         		/* calculate the remaining time in the TXOP */
			u_int64_t time_left;
			MICRO_TO_TICK(time_left,curr_desc->txop);

			if( (TXOPstart + time_left) < now ){
				first_deque = 1;
				next_TSPEC();
				continue;
			}

         		time_left += TXOPstart - now;

			/* transmit time */
         		u_int32_t ttime = mac_->TX_Time(p->pkt_getlen()) + 2*sifs 
					  + mac_->TX_Time(MAC80211E_ACK_LEN);

         		/* if delay bound > 0 and the access delay of the current packet
         	   	   is greater than its delay bound, discard it. */
			u_int64_t *timestamp = (u_int64_t *)p->pkt_getinfo("TIMESTAMP");

			MICRO_TO_TICK(ticks, curr_desc->delay_bound);
		
         		if((curr_desc->delay_bound > 0) && 
			   		(now + ttime - *timestamp > ticks)) {

				index = getIndex(hccaTx_.dst);
            			ts_[index][hccaTx_.tsid-TSID_INDEX]->deque();
            			goto deque;
         		}

         		/* if the remaining time in the TXOP is not sufficient to *
			 * accomodate the transmission of the next frame, return  *
			 * NO_PKT and move to the next TSPEC. 		    	  */
         		if( ttime + pifs > time_left ) {
            			first_deque = 1;
            			next_TSPEC();
            			continue;
         		}

			pktNum++;

         		// initialize the QoS Data frame fields
			struct hdr_mac802_11e	*hm = (struct hdr_mac802_11e *)p->pkt_get();
         		hccaTx_.txp 		= pkt_copy(pkt);
         		hccaTx_.subtype 	= hm->dh_fc.fc_subtype;
         		hccaTx_.dura 		= mac_->TX_Time(MAC80211E_ACK_LEN) + sifs;
         		hccaTx_.ackP 		= ACKP_NORMAL_ACK;
			hccaTx_.getP 		= true;
         		return;

      		} else { // uplink TXOP
		
			if(mac_->near_beacon){
				double tick;

				TICK_TO_MICRO(tick, (mac_->last_beacon + BI_tick - now));
				ticks = (u_int64_t)tick;
				hccaTx_.txop= (u_int64_t)(ticks - poll_time);	
				hccaTx_.dura=(int)(ticks + sifs);
			}else{
         			hccaTx_.txop 	= curr_desc->txop - poll_time;
         			hccaTx_.dura 	= curr_desc->txop - poll_time + sifs;
			}

         		// initialize the QoS CF-Poll frame fields and move to the next TXOP
         		hccaTx_.subtype 	= MAC_Subtype_QoS_Poll;
         		hccaTx_.ackP 		= ACKP_NOEXP_ACK;
    			hccaTx_.getP		= true;
	 
	    		first_deque = 1;
         		next_TSPEC();
         		return;
      		}
   	}
}

void QosScheduler_HC::change_state(HCCA_State e, ePacket_ *pktRx_)
{
	Packet		*p;
   	hdr_mac802_11e	*hm;
	int		index;

  	if( tsNum == 0 )
      		return;

   	switch(e) {
      		case HCCA_SUCCESS:
         		/* If the previous packet transmission is successful,	*
          	   	 * drop that packet from the queue 			*/
         		assert(pktRx_);
			GET_PKT(p, pktRx_);
			assert(p);
			
			hm = (struct hdr_mac802_11e*)p->pkt_get();
			index = getIndex(hm->dh_addr2); 
         		ts_[index][hm->dh_qos.tid-TSID_INDEX]->deque();
         		break;
     		default:
         		printf("QosScheduler_HC:: Unknown HCCA State\n");
         		abort();
   	}
}

	
void QosScheduler_HC::next_TSPEC(void)
{
	u_int64_t tick;
	
	pktNum = 0;

	if(curr_desc==NULL)
		return;
	
	if (curr_desc->next) {

		MICRO_TO_TICK(tick,(curr_desc->txop-sifs)); //mwhsu
     		TXOPstart += tick;
      		curr_desc = curr_desc->next;

   	} else {

		MILLI_TO_TICK(tick,Service_Interval);
      		SIstart += tick;
		MICRO_TO_TICK(tick,sifs);
      		TXOPstart = SIstart - tick;
      		curr_desc = head_desc;
   	}
}

int QosScheduler_HC::reg_TSPEC(u_char *mac, char *tspec, enum pro_ proto) {

        tspec_desc 		*p, *q;
	u_int32_t  		N;
	u_int32_t		mean_pkt_time, max_pkt_time;
	u_int32_t		nodeID;

	char 			*TSInfo;
	u_char 			TSInfo_TSID;
	unsigned int 		DelayBound;
	unsigned short 		NominalMSDUSize;
	unsigned short 		MaximumMSDUSize;
	unsigned int 		MeanDataRate;
	unsigned int 		MaximumServiceInterval;
	bool			TSInfo_Direction_Bit_1;
	bool			TSInfo_Direction_Bit_2;
	dir_			Direction;

	TSInfo = GetTSInfoFromTSPEC(tspec);
	assert(TSInfo);
	assert(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID));
	assert(GetDelayBoundFromTSPEC(tspec, &DelayBound));
	assert(GetNominalMSDUSizeFromTSPEC(tspec, &NominalMSDUSize));
	assert(GetMaximumMSDUSizeFromTSPEC(tspec, &MaximumMSDUSize));
	assert(GetMeanDataRateFromTSPEC(tspec, &MeanDataRate));
	assert(GetMaximumServiceIntervalFromTSPEC(tspec, &MaximumServiceInterval));


	nodeID = ipv4addr_to_nodeid(macaddr_to_ipv4addr(mac));

	printf("addTSPEC:from node%d tsid=%d %u %u %u %u\n", nodeID, (int)TSInfo_TSID,
		MeanDataRate, NominalMSDUSize, MaximumMSDUSize, DelayBound);

	if(GetDirectionBit1FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_1 = true;
	else
		TSInfo_Direction_Bit_1 = false;

	if(GetDirectionBit2FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_2 = true;
	else
		TSInfo_Direction_Bit_2 = false;

	if(TSInfo_Direction_Bit_1 == false && TSInfo_Direction_Bit_2 == false)
		Direction = UPLINK;
	else if(TSInfo_Direction_Bit_1 == true && TSInfo_Direction_Bit_2 == false)
		Direction = DOWNLINK;

	if(find_TSPECdesc(mac, Direction, TSInfo_TSID))
		return (-1);

        /* check if a minimum set of TSPEC information is present */
        if ( NominalMSDUSize <= 0 || MaximumMSDUSize <= 0 ||
               MeanDataRate <= 0 || MaximumServiceInterval <= 0 ) {
               printf ("invalid TSPEC parameter!\n");
                return (-1);
	}
 
	if ( (int)TSInfo_TSID > MAX_TSID) {
   		printf ("TID %d is higher than the maximum TID allowed %d\n", 
				 (int)TSInfo_TSID, MAX_TSID);
		return (-1);
        }

	if ( Direction == DOWNLINK) {

		int index = getIndex(mac);
		int tsid = (int)TSInfo_TSID - TSID_INDEX;

        	if ( ts_[index] != 0 ) {

        		if ( index <= max_qsta && ts_[index][tsid] != 0 ) {
               		 	printf ("there is already an established flow with TID %d "
					"from QSTA %d\n", (int)TSInfo_TSID, index);
                        	return (-1);
                        }

                } else {
                        ts_[index] = new ParaQueue*[TSID_INDEX];
                        max_qsta = index;

                        for ( int i = 0 ; i < TSID_INDEX ; i++ )
                                ts_[index][i] = 0;
                }

                ts_[index][tsid] = new ParaQueue;

	} else if (Direction != UPLINK) {

		printf("unknown direction '%d' for flow with TID %d from ""QSTA %d\n", 
			Direction, (int)TSInfo_TSID, nodeID);
		return (-1);
	}
	
        tspec_desc* newdesc = create_TSPECdesc(mac, tspec, proto);
	memcpy(newdesc->qsta, mac, ETHER_ADDR_LEN);

        if ( head_desc != 0 ) {

                p = head_desc;
                while ( p->next != 0 ) 
			p = p->next;
                p->next = newdesc;

        } else {
                head_desc = newdesc;
        }

        ++tsNum;

        /* the first step, calculate of the min SI among the max SIs of all admitted TS */
	pre_SI = Service_Interval;
        Service_Interval = 0;

        p  = head_desc;
        while ( p != 0 ) {
                if ( Service_Interval == 0 || Service_Interval > p->max_SI )
                        Service_Interval = p->max_SI;
                p = p->next;
        }

        /* the second step, calculate the TXOP duration for a given SI for the stream */
        p = head_desc;
        while ( p != 0 ) {
		/*
		 * the scheduler calculates the number of MSDUs that arrived at the Mean * 
		 * Data Rate During SI mean_rate in KB/s=B/ms, mean_MSDU_size in bytes.  *
		 */
               	N = (u_int32_t) ceil((double)Service_Interval*p->mean_rate/p->mean_MSDU_size);
		p->Num = N;

		/*
                 * calculate the TXOP duration as the maximum of:
                 *  (1) the time to transmit N frames at Ri
                 *  (2) the time to transmit one maximum size MSDU at Ri 
		 * (plus overhead_descs) in microseconds
                 */	
        	mean_pkt_time 	= mac_->TX_Time(p->mean_MSDU_size + MAC80211E_HDR_LEN) 
                		  + 2*sifs + mac_->TX_Time(MAC80211E_ACK_LEN);
        	max_pkt_time 	= mac_->TX_Time(p->max_MSDU_size + MAC80211E_HDR_LEN)
                	 	  + 2*sifs + mac_->TX_Time(MAC80211E_ACK_LEN);
		p->pre_txop = p->txop;
		p->txop = (u_int16_t)max( N * mean_pkt_time + p->overhead, 
				max_pkt_time + p->overhead) + pifs;

       		if ( p->txop > MAX_TXOP_LIMIT )
                	p->txop = MAX_TXOP_LIMIT;
                p = p->next;
        }

        // check if the schedule is feasible
        u_int16_t txop_sum = 0;

        p = head_desc;
        while ( p != 0 ) {
                txop_sum += p->txop;
                txop_sum += pifs;
                p = p->next;
        }

        if ( txop_sum > (u_int32_t) Service_Interval * 1000 ) {  //milli to micro

                printf ("failure negotiation of TSPEC with TSID %d from QSTA %d\n"
                         "\ttxop_sum = %d microsec > Service Interval = %u millisec\n",
                         newdesc->tsid, nodeID, txop_sum, Service_Interval);

		p = head_desc;

		if(p->next == NULL){
			head_desc = NULL;
			delete p;
		}else{
			while(p->next != NULL){ 
				q = p;
				p = p->next;
			}
			q->next = NULL;
			delete p;
		}

		if ( Direction == DOWNLINK) {

			int index = getIndex(mac);
			int tsid = (int)TSInfo_TSID - TSID_INDEX;

        		delete ts_[index][tsid];
        		ts_[index][tsid] = 0;
		}

		p = head_desc;
                while ( p != 0 ){
                        p->txop = p->pre_txop;
                        p = p->next;
                }

                Service_Interval = pre_SI;
		tsNum--;
                return 1;
        }

        printf ("\tThe negotiation of TSPEC with TSID %d from QSTA %x \n"
                "\ttxop_sum: %d microsec(max 8160 microsec/each)\n""\tSI: %u millisec\n",
                         newdesc->tsid , nodeID, txop_sum, Service_Interval);

        // set the round pointer to the first list element
        curr_desc = head_desc;
   	first_call = 1;
        return 0;
}

tspec_desc* QosScheduler_HC::create_TSPECdesc(u_char *mac, char *tspec, enum pro_ proto)
{
	char 			*TSInfo;
	u_char 			TSInfo_TSID;
	u_char			TSInfo_UserPriority;
	unsigned int 		DelayBound;
	unsigned short 		NominalMSDUSize;
	unsigned short 		MaximumMSDUSize;
	unsigned int 		MeanDataRate;
	unsigned int 		MaximumServiceInterval;
	bool			TSInfo_Direction_Bit_1;
	bool			TSInfo_Direction_Bit_2;
	dir_			Direction;

	TSInfo = GetTSInfoFromTSPEC(tspec);
	assert(TSInfo);
	assert(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID));
	assert(GetUserPriorityFromTSInfo(TSInfo, &TSInfo_UserPriority));
	assert(GetDelayBoundFromTSPEC(tspec, &DelayBound));
	assert(GetNominalMSDUSizeFromTSPEC(tspec, &NominalMSDUSize));
	assert(GetMaximumMSDUSizeFromTSPEC(tspec, &MaximumMSDUSize));
	assert(GetMeanDataRateFromTSPEC(tspec, &MeanDataRate));
	assert(GetMaximumServiceIntervalFromTSPEC(tspec, &MaximumServiceInterval));

	if(GetDirectionBit1FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_1 = true;
	else
		TSInfo_Direction_Bit_1 = false;

	if(GetDirectionBit2FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_2 = true;
	else
		TSInfo_Direction_Bit_2 = false;

	if(TSInfo_Direction_Bit_1 == false && TSInfo_Direction_Bit_2 == false)
		Direction = UPLINK;
	else if(TSInfo_Direction_Bit_1 == true && TSInfo_Direction_Bit_2 == false)
		Direction = DOWNLINK;

        // create a new parameter element
	tspec_desc* newdesc = new tspec_desc;

        newdesc->next          	= 0;
        newdesc->tsid           = TSInfo_TSID; 	// from 0 to 7
        newdesc->direction     	= Direction;
        newdesc->mean_rate     	= MeanDataRate;	// bit/sec
        newdesc->max_SI        	= MaximumServiceInterval;
        newdesc->mean_MSDU_size = NominalMSDUSize;
        newdesc->max_MSDU_size  = MaximumMSDUSize;
	newdesc->delay_bound	= DelayBound;
	newdesc->user_pri	= TSInfo_UserPriority;
	newdesc->protocol 	= proto; 
	memcpy(newdesc->qsta, &mac, 6);

        // poll overhead  
        if ( Direction == DOWNLINK )
                newdesc->overhead = 0;
        else
                newdesc->overhead = mac_->TX_Time(MAC80211E_HDR_LEN) + sifs 
				    + mac_->TX_Time(MAC80211E_ACK_LEN);
   	return newdesc;
}

bool QosScheduler_HC::find_TSPECdesc(u_char *mac, dir_ direction, u_char tsid)
{
	tspec_desc *tmp = head_desc;
	while(tmp!=NULL){
		if(!(bcmp((void *)mac, (void *)tmp->qsta, 6)) && (tmp->direction == direction) && (tmp->tsid == tsid)) 
			return true;
		tmp = tmp->next;
	}
	return false;
}

void QosScheduler_HC::delete_TS(u_char *mac, dir_ direction, u_char tsid){

        tspec_desc *tmp = head_desc, *pre_tmp = NULL;

        while(tmp!=NULL){
                if(!(bcmp((void *)mac, (void *)tmp->qsta, 6)) && (tmp->direction == direction) && (tmp->tsid == tsid))
                        break;
                pre_tmp = tmp;
                tmp = tmp->next;
        }

        if(!pre_tmp)
                head_desc = tmp->next;
        else
                pre_tmp->next = tmp->next;

        if(curr_desc == tmp){
                recover_status();
        }

        delete tmp;
        tsNum--;
}

