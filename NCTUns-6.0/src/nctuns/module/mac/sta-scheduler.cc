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

#include <mac/mac-802_11e.h>
#include <mac/qos-scheduler.h>
#include <mac/sta-scheduler.h>

QosScheduler_STA::QosScheduler_STA (mac802_11e* mac) 
	:QosScheduler(mac) 
{
	poll_tsid	= 0;
        tsNum 		= 0;
        txop_limit 	= 0;
   	txop_start 	= 0;
   	null_sent 	= 0;
   	data_sent 	= 0;
   	has_control 	= 0;
        zero_limit_txop = false;

	sifs = mac->phymib->aSIFSTime;

	for(int i = 0; i < TSID_INDEX; i++)
		ts_[i] = new ParaQueue;

}

int QosScheduler_STA::pkt_enque (ePacket_ *pkt) {
	
        Packet                  *p;
        struct hdr_mac802_11e   *hm;
	short			tsid;
	u_int64_t		time = GetCurrentTime();
	
        GET_PKT(p, pkt);
        assert(p);

	hm = (struct hdr_mac802_11e *)p->pkt_get();	
        tsid = hm->dh_qos.tid - TSID_INDEX;
        
	if ( ts_[tsid]->qlen() >= MAX_QUEUE_LEN ) 
                return 0;

        p->pkt_addinfo("TIMESTAMP",(char *)&time,8);

   	// add the packet to the FIFO queue
	ts_[tsid]->_size += p->pkt_getlen();
        ts_[tsid]->enque (pkt);
	return 1;
}

void QosScheduler_STA::pkt_deque(HCCA_TxP& hccaTx_)
{
   	ePacket_* 	head = 0;
   	ePacket_* 	second = 0;
	u_int64_t 	time_left, ticks;
	u_int64_t	now = GetCurrentTime();

	int tsid = poll_tsid - TSID_INDEX;
	
	if( ! has_control ) 
      		return;

        hccaTx_.tsid = tsid;
   	hccaTx_.eosp = 1;
   	hccaTx_.ackP = ACKP_NORMAL_ACK;
	memcpy( hccaTx_.dst, mac_->ap_mac, ETHER_ADDR_LEN);

deque:
        // If the queue is empty a QoS Null frame is sent.
        if (ts_[tsid]->qlen() == 0 || null_sent) {
       		null_sent = 1;
        	hccaTx_.getP = true; 
        	hccaTx_.subtype = MAC_Subtype_QoS_Null; 
        	hccaTx_.qSize = ts_[tsid]->_size; 
        	hccaTx_.dura = mac_->TX_Time(MAC80211E_ACK_LEN) + sifs;
      		return;
        }

   	ts_[tsid]->top(head);
	hccaTx_.getP = true;

   	if( ts_[tsid]->qlen() > 1 )
      		ts_[tsid]->second(second);

	MICRO_TO_TICK(time_left, txop_limit);

	if((time_left + txop_start) < now)
		goto no_send;
	else
   		time_left += txop_start - now;

	MICRO_TO_TICK(ticks,pktTxTime(head,tsid));	

        if ( zero_limit_txop == true || time_left >= ticks ) {

		Packet *p;
		double ms;

		GET_PKT(p, head);
		struct hdr_mac802_11e *hm = (struct hdr_mac802_11e *)p->pkt_get();

		u_int64_t *timestamp = (u_int64_t *)p->pkt_getinfo("TIMESTAMP");
		u_int32_t ttime = now + mac_->TX_Time(p->pkt_getlen()) + 2*sifs 
				  + mac_->TX_Time(MAC80211E_ACK_LEN);
		MICRO_TO_TICK(ticks,ts_[tsid]->_delaybound);

                if((ts_[tsid]->_delaybound > 0) && (ttime - *timestamp > ticks)) {
                        ts_[tsid]->deque();
                        goto deque;
                }
	
      		data_sent = 1;
      		hccaTx_.subtype = hm->dh_fc.fc_subtype;
      		hccaTx_.txp = pkt_copy(head);
		
	      	/* 
		 * if the TXOP limit is zero, or the second packet in the queue does
      		 * not fit in the remaining part of the TXOP, set the duration field
      		 * to the time required to transmit an ACK plus sifs. This indicate
      		 * that the QSTA returns the remaining part of the TXOP to the QAP.
		 */
		TICK_TO_MICRO(ms,time_left);

      		if( zero_limit_txop == true ||  ts_[tsid]->qlen() <= 1
         		|| ms - pktTxTime(head,tsid) < pktTxTime(second,tsid) + sifs)

         		hccaTx_.dura = mac_->TX_Time(MAC80211E_ACK_LEN) + sifs;
      		else {
			GET_PKT(p, head);
         		// set the duration field to cover the remaining part of the TXOP
         		hccaTx_.dura = (u_int16_t)ms - mac_->TX_Time(p->pkt_getlen());
      		}

      		/* if the TXOP limit is zero, the QSTA loose the control of the medium  
		 * after sending the first frame
		 */
      		if(zero_limit_txop) {
         		has_control = 0;
                   	zero_limit_txop = false;
      		}

		GET_PKT(p, head);
               	hccaTx_.qSize= ts_[tsid]->_size - p->pkt_getlen();
        } else {

	no_send:
      		if(!data_sent){
       			null_sent = 1;
        		hccaTx_.getP = true; 
        		hccaTx_.subtype = MAC_Subtype_QoS_Null; 
        		hccaTx_.qSize = ts_[tsid]->_size; 
        		hccaTx_.dura = mac_->TX_Time(MAC80211E_ACK_LEN) + sifs;
      		}else 
         		hccaTx_.getP = false;
   	}
}

void QosScheduler_STA::change_state(HCCA_State e, ePacket_* pktRx_) 
{
   	Packet *p;
	struct hdr_mac802_11e *hm;

   	switch(e) {
      		case HCCA_LOST_CONTROL:
         		has_control = 0;
         		break;
      		case HCCA_HAS_CONTROL:
         		/*
		 	 * when the QSTA gains the control of the medium, it is stored the
         	 	 * TXOP limit in 'txop_limit'. If the TXOP limit is equal to zero, the
         	 	 * variable 'zero_txop_limit' is set to 1; in this case the QSTA
         	 	 * can transmit only one QoS Data frame.
		 	 */
			assert(pktRx_);
         		null_sent = 0;
         		data_sent = 0;
         		has_control = 1;

			GET_PKT(p, pktRx_);
			hm = (struct hdr_mac802_11e*)p->pkt_get();
			txop_limit = (hm->dh_qos.txop_queue << 5);	// 32-8160 microseconds
              		zero_limit_txop = ( txop_limit == 0 ) ? true : false;
         		txop_start = GetCurrentTime();
         		break;

      		case HCCA_SUCCESS:
		{
			if(null_sent) {
				null_sent = 0;
				return;
			}
			ePacket_	*pkt;

			GET_PKT(p,pktRx_);
			hm = (struct hdr_mac802_11e*)p->pkt_get();
			int tsid = hm->dh_qos.tid - TSID_INDEX;

			ts_[tsid]->top(pkt);
			GET_PKT(p,pkt);
         		assert(p);
         		// Update the queue length at the QSTA
			ts_[tsid]->_size -= p->pkt_getlen();	
         		if ( ts_[tsid]->_size < 0 ) {
            			printf("Negative payload\n");
            			abort ();
         		}
			// drop the packet from the queue
			ts_[tsid]->deque();
         		break;
		}
      		default:
         		printf("ERROR:QosScheduler_STA :: HCCA State unknown");
         		break;
   	}
}

u_int64_t QosScheduler_STA::pktTxTime (ePacket_ *pkt, int tsid)
{
        u_int64_t 	txop_needed = 0;
	int		size;
	Packet		*p;

   	if (!pkt) 
		return 0;

        if ( ts_[tsid]->qlen() > 0 ) {
		GET_PKT(p,pkt);
        	size = p->pkt_getlen();
                txop_needed = mac_->TX_Time(size) + 2*sifs 
			      + mac_->TX_Time(MAC80211E_ACK_LEN) - 100;
        }
        return txop_needed;
}

int QosScheduler_STA::reg_TSPEC(u_char *mac, char *tspec, enum pro_ proto) 
{
	char 		*TSInfo;
	u_char 		TSInfo_TSID;
	unsigned int 	DelayBound;

	TSInfo = GetTSInfoFromTSPEC(tspec);
	assert(TSInfo);
	assert(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID));
	assert(GetDelayBoundFromTSPEC(tspec, &DelayBound));

	ts_[(unsigned int)TSInfo_TSID-TSID_INDEX]->_delaybound = DelayBound;
        ++tsNum;
        return 0;
}

