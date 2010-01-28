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

#include <stdlib.h>
#include <assert.h>
#include <regcom.h>
#include <object.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/ret/rcs_atm.h>
#include <satellite/dvb_rcs/ret/rcs_mac.h>
#include <satellite/dvb_rcs/ret/dvb_rcs.h>
#include <satellite/dvb_rcs/ret/queue/rcst_queue_manager.h>
#include <satellite/dvb_rcs/rcst/rcst_queue_config.h>
#include <mylist.h>

/*
 * Constructor
 */
Rcs_mac::Rcs_mac(u_int32_t type, u_int32_t id, struct plist * pl,
		 const char *name)
: NslObject(type, id, pl, name), _rcst_obj(NULL),
	_rcst_qm(NULL)
{
	_tx_pkt_buf = NULL;
	_ptrlog = 0;

	CIRCLEQ_INIT(&_rx_ptbuf_head);

}


/*
 * Destructor
 */
Rcs_mac::~Rcs_mac()
{
	/*
	 * free all tx atm_burst linked list
	 */
	_free_circleq_buf(&_rx_ptbuf_head);
}


/*
 * module initialize
 */
int Rcs_mac::init()
{
	/*
	 * Initial log flag
	 */
	if ( SatLogFlag && !strcasecmp(SatLogFlag, "on") ) {
		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;
			
			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen
					(GetConfigFileDir())+strlen
					(ptrlogFileName) + 1);
				sprintf(ptrFile,"%s%s", GetConfigFileDir(),ptrlogFileName);
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}
			else {
				ptrFile = (char *)malloc(strlen(GetScriptName())+5);
				sprintf(ptrFile, "%s.ptr", GetScriptName());
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}

			if( fptr == NULL ) {
				printf("Error : Can't create file %s\n",ptrFile);
				exit(-1);
			}
	
			Event_ *heapHandle = createEvent();
			u_int64_t time;
			MILLI_TO_TICK(time, 100);
			u_int64_t chkInt = GetCurrentTime() + time;
			setEventTimeStamp(heapHandle, chkInt, 0);

			int (*__fun)(Event_ *) = 
			(int (*)(Event_ *))&DequeueLogFromHeap;;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}

		_ptrlog = 1;
	}

	return (NslObject::init());
}


/*
 * segmentation payload depend on AAL5 spec and padding into ATM cell upper
 * layer must send Dvb_pkt packet struct to me, then send those cell to bottom
 * layer.
 */
int Rcs_mac::send(ePacket_ * event)
{
	Dvb_pkt *pkt;

	/*
	 * if this node is not RCST, then this packet should be by pass to
	 * forward module
	 */
	assert(pkt = (Dvb_pkt *) event->DataInfo_);
	event->DataInfo_ = NULL;

	/*
	 * this packet is control burst (may be csc/sync burst)
	 */
	if (pkt->pkt_gettype() == PKT_RCSSYNC || pkt->pkt_gettype() == PKT_RCSCSC) {
		/*
		 * when mac received control packet, that's meaning it must
		 * send this control burst right now. So we check the txTimer
		 * whether is busy, if yes, assert it
		 */
		assert(pkt->pkt_getretinfo()->queue_id == 0);
		assert(!_tx_timer.busy_);

		/*
		 * if the wait timer is busy, then cancel it. because of the
		 * control message must be processed first
		 */
		if (_wait_timer.busy_) {
			_wait_timer.cancel();
		}

		_rcst_obj->schedule_timer_for_mac = false;
		_rcst_qm->push(pkt);
	}
	/*
	 * this packet is user traffic burst, we must check total number of atm
	 * cell linked list, if queue has enough space then attach it to buffer
	 * linked list
	 */
	else if (pkt->pkt_gettype() == PKT_ATM) {
		assert(pkt->pkt_getretinfo()->queue_id > 0);
		_rcst_qm->push(pkt);
	}
	else {
		/*
		 * this version just support for RCST node to call this function
		 */
		assert(0);
	}

	/*
	 * check _wait_timer and _tx_timer was not be running or the first burst
	 * buffer whether be changed. If start time of first burst be not equal
	 * to _wait_timer.expire, it means the first entry has changed, then
	 * re-schedule this timer event
	 */
	if (!_wait_timer.busy_ && !_tx_timer.busy_ && !(_rcst_obj->schedule_timer_for_mac))
	{
		schedule_wait_timer();
	}

	dvb_freePacket(event);

	return (1);
}


/*
 * recv the packet from the Rcst and check whether collision at the same timeslot
 */
int Rcs_mac::recv(ePacket_ * event)
{
	Dvb_pkt *pkt;
	struct pkt_timer_buf *ptimer;
	u_int64_t rxTime;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	/*
	 * if this node is RCST, then this packet should be by pass to control
	 * module
	 */
	rxTime = pkt->pkt_getretinfo()->tx_time;

	/*
	 * search all pkt_timer and check whether be used at this
	 * timeslot
	 */
	if ((ptimer = _check_state(pkt))) {
		/*
		 * collision state
		 */
		ptimer->state = COLL;

		/*
		 * if collision packet txTime over last packet txTime,
		 * then extend rxTimer
		 */
		if (rxTime + GetCurrentTime() > ptimer->timer.expire()) {
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(Rcs_mac, rxHandler);
			ptimer->timer.setCallOutObj(this, type);
			ptimer->timer.cancel();
			ptimer->timer.start(rxTime, 0);
		}
		dvb_freePacket(event);

		return (1);
	}

	/*
	 * cannot find the same pkt_timer, that mean this timeslot is
	 * idle state 
	 */
	ptimer = new struct pkt_timer_buf;

	CIRCLEQ_INSERT_TAIL(&_rx_ptbuf_head, ptimer, list);
	ptimer->state = RECV;
	ptimer->pkt = event;

	/*
	 * schedule rx_timer event
	 */
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(Rcs_mac, rxHandler);
	ptimer->timer.setCallOutObj(this, type);
	ptimer->timer.start(rxTime, 0);

	return (1);
}


/*
 * clear buffer function, this function will be called by rcst control when it has logout
 * it will clean all buffer, and stop timer
 */
int Rcs_mac::clear_buffer()
{
	if (_wait_timer.busy_) {
		_wait_timer.cancel();

		delete _tx_pkt_buf;
	}
	else if (_tx_timer.busy_) {
		_tx_timer.cancel();
	}
	_tx_pkt_buf = NULL;

	return (0);
}


/*
 * _wait_timer handler to send burst data to next layer and schedule _tx_timer event
 * when current time equal to start time of first burst in burst buffer linked list
 */
int Rcs_mac::send_burst()
{
	int count;
	struct atm_burst *burst;
	struct slot_info *timeslot;
	ePacket_ *event;
	int pkt_bytelen;
	double pkt_bitlen;

	assert(!_tx_pkt_buf);
	_tx_pkt_buf = _rcst_qm->shift();

	/*
	 * data dvb_pkt, must merge atm cell into one burst
	 */
	if (_tx_pkt_buf->pkt_gettype() == PKT_ATM) {
		/*
		 * encapsulation atm burst depend on timeslot_payload_type
		 */
		count = _tx_pkt_buf->pkt_getlen();

		/*
		 * at this, payload of Dvb_pkt should be ATM cell linked list 
		 */
		burst = _encapsulation_atm_burst((struct atm_cell *)_tx_pkt_buf->pkt_detach(), count);

		_tx_pkt_buf->pkt_attach(burst, ATM_CELL_LEN * burst->cell_cnt);
	}

	_tx_pkt_buf->pkt_settype(PKT_RCSMAC);

	/*
	 * get first burst in burst buffer list, and set up state to SEND
	 */
	pkt_bytelen = _tx_pkt_buf->pkt_getlen();

	/*
	 * create new Dvb_pkt and new ePacket_ event
	 */
	event = createEvent();
	event->DataInfo_ = _tx_pkt_buf;

	timeslot = &_tx_pkt_buf->pkt_getretinfo()->timeslot;

	/*
	 * CSC burst must do Randomization, crc16, RS, CC
	 * RS encoding will increase 16bytes parity bytes
	 *
	 * CSC burst must do Randomization, crc16, CC
	 *
	 * user traffic burst must do Randomization, RS/CC or TC
	 */

	/*
	 * outer_coding == X1b => enable RS
	 * outer_coding == 1Xb => enable CRC
	 */
	if (timeslot->outer_coding & 0x01) {
		pkt_bytelen += RS_PARITY_LENGTH;
	}
	if (timeslot->outer_coding & 0x02) {
		assert(0);
	}

	/*
	 * inner_code_type == 0 => CC
	 * inner_code_type == 1 => TC
	 */
	if (timeslot->inner_code_type == 0) {
		pkt_bitlen = pkt_bytelen * 8 + timeslot->preamble_length;

		switch (timeslot->inner_code_puncturing) {
			case PUNCTURING_1_2:
				pkt_bitlen = pkt_bitlen * 2;
				break;
			case PUNCTURING_2_3:
				pkt_bitlen = (pkt_bitlen * 4.0) / 2.0;
				break;
			case PUNCTURING_3_4:
				pkt_bitlen = (pkt_bitlen * 4.0) / 3.0;
				break;
			case PUNCTURING_5_6:
				pkt_bitlen = (pkt_bitlen * 6.0) / 5.0;
				break;
			case PUNCTURING_7_8:
				pkt_bitlen = (pkt_bitlen * 8.0) / 7.0;
				break;
			default:
				assert(0);
		}
	}
	else {
		pkt_bitlen = pkt_bytelen * 8;
		pkt_bitlen = pkt_bytelen * 8 + timeslot->preamble_length;

		switch (timeslot->inner_code_puncturing) {
			case PUNCTURING_1_2:
				pkt_bitlen = pkt_bitlen * 2;
				break;
			case PUNCTURING_2_3:
				pkt_bitlen = (pkt_bitlen * 4.0) / 2.0;
				break;
			case PUNCTURING_3_4:
				pkt_bitlen = (pkt_bitlen * 4.0) / 3.0;
				break;
			case PUNCTURING_1_3:
				pkt_bitlen = pkt_bitlen * 3;
				break;
			case PUNCTURING_2_5:
				pkt_bitlen = (pkt_bitlen * 5.0) / 2.0;
				break;
			case PUNCTURING_4_5:
				pkt_bitlen = (pkt_bitlen * 5.0) / 4.0;
				break;
			case PUNCTURING_6_7:
				pkt_bitlen = (pkt_bitlen * 7.0) / 6.0;
				break;
			default:
				assert(0);
		}
		assert(0);
	}

	/*
	 * compute transmission time and set up timer
	 */
	const uint32_t symbol_len = (uint32_t) (pkt_bitlen / QPSK_BIT_PER_SYMBOL + 0.5);

	const uint64_t txTime = calculate_tx_time(symbol_len, timeslot->symbol_rate / 1000.0);

	_tx_pkt_buf->pkt_getretinfo()->tx_time = txTime;

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(Rcs_mac, txHandler);
	_tx_timer.setCallOutObj(this, type);
	_tx_timer.start(txTime, 0);

	return NslObject::send(event);
}


/*
 * _tx_timer handler to clean first burst data in burst buffer linked list and log it
 * when burst has sent duration a transmission time. If burst buffer linked
 * list is not empty, then continue to start next circle (send burst)
 */
int Rcs_mac::txHandler()
{
	/*
	 * remove burst buffer
	 */
	_tx_pkt_buf = NULL;

	/*
	 * burst buffer linked list not empty, then schedule wait_time for next sending
	 */
	schedule_wait_timer();

	return (0);
}


/*
 * rx_timer handler to clean first burst data in burst buffer linked list and log it
 * when burst has sent duration a transmission time. If burst buffer linked
 * list is not empty, then continue to start next circle (send burst)
 */
int Rcs_mac::rxHandler()
{
	struct pkt_timer_buf *ptimer;
	int pktlen;
	int physrc;
	int pkttype;
	int recvstate;

	CIRCLEQ_FOREACH(ptimer, &_rx_ptbuf_head, list) {
		/*
		 * find the timer which has expired
		 */
		if (ptimer->timer.expire() == GetCurrentTime()) {
			switch (ptimer->state) {
					/*
					 * if state equal to RECV, it means
					 * this packet received success and
					 * send it to upper layer then free
					 * this memory
					 */
				case RECV:
				{
					/* Get info for log */
					pktlen = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_getlen();
					physrc = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_getretinfo()->loginfo.phy_src;
					pkttype = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_gettype();
					recvstate = ptimer->state;

					_recv_burst(ptimer->pkt);
					CIRCLEQ_REMOVE(&_rx_ptbuf_head,
						       ptimer, list);
					break;
				}
					/*
					 * if state equal to COLL, it means
					 * this packet received fault then free
					 * this memory
					 */
				case COLL:
				{
					assert(0);
					/* Get info for log */
					pktlen = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_getlen();
					physrc = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_getretinfo()->loginfo.phy_src;
					pkttype = ((Dvb_pkt*)ptimer->pkt->DataInfo_)->pkt_gettype();
					recvstate = ptimer->state;

					CIRCLEQ_REMOVE(&_rx_ptbuf_head,
						       ptimer, list);
					dvb_freePacket(ptimer->pkt);
					break;
				}
				default:
					assert(0);
			};
			/*
			 * Log it
			 */

			/* log "StartRX" event and "SuccessRX" event*/
			if(_ptrlog == 1){
				struct logEvent*        logep;
				uint64_t		rx_time;
				dvbrcs_log* rsdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
				dvbrcs_log* redvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
				rx_time = _tx_delay(pktlen, 55);

				LOG_SAT_DVBRCS(rsdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
						get_type(), get_nid(), StartRX,physrc,
						get_nid(), 0, pkttype, pktlen, 1, DROP_NONE);
				INSERT_TO_HEAP(logep, rsdvbrcs_log->PROTO, rsdvbrcs_log->Time+START, rsdvbrcs_log);
				
				if(recvstate == COLL){
					LOG_SAT_DVBRCS(redvbrcs_log, GetCurrentTime()+rx_time , GetCurrentTime(),
							get_type(), get_nid(), DropRX,physrc,
							get_nid(), 0, pkttype, pktlen, 1, DROP_COLL);
				}
				else{
					LOG_SAT_DVBRCS(redvbrcs_log, GetCurrentTime()+rx_time , GetCurrentTime(),
							get_type(), get_nid(), SuccessRX,physrc,
							get_nid(), 0, pkttype, pktlen, 1, DROP_NONE);
				}
				INSERT_TO_HEAP(logep, redvbrcs_log->PROTO, redvbrcs_log->Time+ENDING, redvbrcs_log);
			}

			free(ptimer);
			return (0);
			break;
		}
	}
	assert(0);
	return (-1);
}


/*
 * recv burst and de-encapsulation atm burst, then send it to upper layer
 */
int Rcs_mac::_recv_burst(ePacket_ * event)
{
	Dvb_pkt *pkt;
	struct atm_burst *burst;
	int i;

	assert((pkt = (Dvb_pkt *) event->DataInfo_));

	switch (pkt->pkt_getretinfo()->burst_type) {
			/*
			 * for SYNC/CSC control burst
			 */
		case PKT_RCSSYNC:
		case PKT_RCSCSC:
		{
			pkt->pkt_settype(pkt->pkt_getretinfo()->burst_type);
			assert(NslObject::recv(event));
			break;
		}

			/*
			 * for user traffic burst
			 */
		case PKT_RCSMAC:
		{
			ePacket_ *cpy_event;
			struct atm_cell *atm_pkt;

			/*
			 * split user traffic burst to each atm cell
			 */
			assert(burst = (struct atm_burst *)pkt->pkt_getdata());


			pkt->pkt_settype(PKT_RCSMAC);

			for (i = 0; i < burst->cell_cnt; i++) {
				atm_pkt = new struct atm_cell;
				atm_pkt->next  = NULL;

				/*
				 * copy a new event for new atm_cell
				 */
				cpy_event = dvb_pkt_copy(event);
				pkt = (Dvb_pkt *) cpy_event->DataInfo_;

				memcpy(atm_pkt, 
				       (void *)((u_int32_t) burst-> payload + i * ATM_CELL_LEN), 
				       ATM_CELL_LEN);

				pkt->pkt_attach(atm_pkt, ATM_CELL_LEN);

				assert(NslObject::recv(cpy_event));
			}
			dvb_freePacket(event);
			break;
		}
		default:
			assert (0);
	};

	return (0);
}


/*
 * encapsulation a burst and set up wait timer to wait for start time of this
 * timeslot belong to the burst, if next TRF burst timeslot slot have not
 * exist, then it will get this information from RCST control
 */
int Rcs_mac::schedule_wait_timer()
{
	struct slot_info *timeslot;

	/*
	 * check _wait_timer, at current stage, it shouldn't be started
	 */
	assert(!_wait_timer.busy_ && !_tx_pkt_buf);

	/*
	 * timeslot information must get it from queue_manager, and queue
	 * manager will get it from rcst control
	 */
	if (!(timeslot = _rcst_qm->get_timeslot())) {
		/*
		 * if I can't get timeslot information, I must schedule a
		 * timer at the start of next frame to get again. I just
		 * set up the flag.And rcst_ctl module will invoke this
		 * function after granting demand.
		 */
		_rcst_obj->schedule_timer_for_mac = true;
		return (0);
	}

	_rcst_obj->schedule_timer_for_mac = false;

	const uint64_t burst_start_time = timeslot->start_time + timeslot->burst_start_offset;

	assert(burst_start_time >= GetCurrentTime());

	/*
	 * if burst_start_time equal to current time, then call the handle function
	 */
	if (burst_start_time == GetCurrentTime()) {
		return send_burst();
	}
	/*
	 * otherwise schedule a timer event to wait for send this burst
	 */
	else {
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(Rcs_mac, send_burst);
		_wait_timer.setCallOutObj(this, type);
		_wait_timer.start(burst_start_time - GetCurrentTime(), 0);
	}

	return (0);
}


/*
 * encapsulate atm cell to atm burst and calculate total number of atm burst
 * then return this number
 */
struct atm_burst *
Rcs_mac::_encapsulation_atm_burst(struct atm_cell *atm_pkt, int count)
{
	int i;
	struct atm_cell *last_atm_pkt;
	struct atm_burst *burst;

	burst = new struct atm_burst;
	memset(burst, 0x00, sizeof (struct atm_burst));

	assert(atm_pkt);
	for (i = 0; i < count; i++) {
		memcpy((void *)((u_int32_t) burst->payload +
				burst->cell_cnt * ATM_CELL_LEN),
		       atm_pkt, ATM_CELL_LEN);
		burst->cell_cnt++;

		/*
		 * free memory atm_pkt 
		 */
		last_atm_pkt = atm_pkt;
		atm_pkt = atm_pkt->next;
		free(last_atm_pkt);
	}
	return burst;
}


/*
 * check foreach pkt_timer whether collision with this pkt
 */
struct Rcs_mac::pkt_timer_buf * Rcs_mac::_check_state(Dvb_pkt * pkt)
{
	struct pkt_timer_buf *ptimer;
	struct slot_info *ts_org, *ts_curr;

	ts_curr = &pkt->pkt_getretinfo()->timeslot;

	CIRCLEQ_FOREACH(ptimer, &_rx_ptbuf_head, list) {
		Dvb_pkt *pkt = (Dvb_pkt *) ptimer->pkt->DataInfo_;

		ts_org = &pkt->pkt_getretinfo()->timeslot;

		/*
		 * if groupd_id is not the same, then skip it
		 */
		if (ts_org->group_id != ts_curr->group_id) {
			continue;
		}

		/*
		 * if frequency bandwidth is not cover each other, the skip it
		 */
		if (ts_org->min_frequency > ts_curr->max_frequency || ts_org->max_frequency < ts_curr->min_frequency) {
			continue;
		}
		return ptimer;
	}

	return NULL;
}


/*
 * compute number of atm cell linked list
 */
int Rcs_mac::_compute_cell_cnt(struct atm_cell *atm_head)
{
	struct atm_cell *atm_pkt;
	int i = 0;

	atm_pkt = atm_head;
	while (atm_pkt) {
		atm_pkt = atm_pkt->next;
		++i;
	}
	return (i);
}


/*
 * compute transmission time depend on packet size, transmission formula is
 * time = packet_size / bandwidth
 * symbol_rate's unit is Kbuad/sec
 */
uint64_t Rcs_mac::calculate_tx_time(uint32_t symbol_len, double symbol_rate)
{
	/*
	 * symbol_rate unit is Kbps
	 */
	return (u_int64_t) ((symbol_len / symbol_rate) * (1000000.0 / TICK) +
			    0.5);
}


/*
 * free all entries in circleq list, and initialize circleq_head
 */
void Rcs_mac::_free_circleq_buf(struct circleq_head *head)
{
	struct pkt_timer_buf *ptimer, *last_ptimer;

	ptimer = (pkt_timer_buf *) head->cqh_first;
	while (ptimer != (pkt_timer_buf *) head) {

		if (ptimer->pkt)
			dvb_freePacket(ptimer->pkt);

		last_ptimer = ptimer;
		ptimer = ptimer->list.cqe_next;
		free(last_ptimer);
	}
	CIRCLEQ_INIT(head);
}

/*
 * calculate tx_delay time
 */
u_int64_t Rcs_mac::_tx_delay(int len, int bw)
{
        u_int64_t       tx_delay_in_tick;
        u_int64_t       tx_delay_in_ns;
        tx_delay_in_ns = (len*8/bw)*1000;
        NANO_TO_TICK(tx_delay_in_tick, tx_delay_in_ns);
        return tx_delay_in_tick;
}

