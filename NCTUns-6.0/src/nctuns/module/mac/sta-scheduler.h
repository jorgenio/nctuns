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

#ifndef NCTUNS_sta_scheduler_h
#define NCTUNS_sta_scheduler_h

class QosScheduler_STA : public QosScheduler {

public:
	QosScheduler_STA(mac802_11e* mac);
	~QosScheduler_STA(){ }

	int 		reg_TSPEC(u_char *mac, char *tspec, enum pro_ proto);	/* add a Traffic Stream */

	void		set_poll_tsid(int tsid) { poll_tsid = tsid; }
	int 		get_qsize(int tid)	{ return ts_[tid]->_size; }

	int 		pkt_enque(ePacket_ *p);		/* enque a packet */
	void 		pkt_deque(HCCA_TxP &scheTx);	/* deque the next packet */
	void 		change_state(HCCA_State e, ePacket_ *pkt);  	
	void 		recover_status(void){}		/* restore the state */
	
	u_int64_t 	pktTxTime(ePacket_ *pkt, int tsid);

 private:
	int		poll_tsid;
   	bool 		null_sent;		/* True if STA has been sent a Null */
   	bool 		data_sent;		/* True if STA has sent Data packets */
   	bool 		has_control;		/* True if STA has control now */
        bool 		zero_limit_txop;	/* True if received a Poll with TXOPlimit 0. */

        u_int16_t 	sifs;	
        u_int16_t 	txop_limit;		/* TXOP limit in microseconds */
   	u_int64_t 	txop_start;		/* TXOP start time */
	ParaQueue 	*ts_[TSID_INDEX];
};
#endif
