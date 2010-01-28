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

#ifndef NCTUNS_qos_scheduler_h
#define NCTUNS_qos_scheduler_h

#include <mac/mac-802_11e.h>
#include <mac/ParaQueue.h>

#define TSID_INDEX	8
#define MAX_TSID	15	/* TSID from 8-15 */
#define MAX_TXOP_LIMIT  8160 	/* 32-8160 TXOPLimit, in microseconds */


/*========================================================================
    Define ECCA Structure
  ========================================================================*/

/* Traffic Stream descriptor */
struct tspec_desc {
   u_int32_t    nid;            /* Node ID */
   dir_         direction;	/* Flow direction */
   u_char       tsid;           /* TSID */
   u_char       qsta[6];        /* Register QSTA mac address */
   u_int32_t    mean_rate;      /* Mean data rate in KB/sec */
   u_int32_t    peak_rate;      /* Peak data rate in KB/sec */
   u_int16_t    mean_MSDU_size; /* Nominal MSDU size in bytes */
   u_int16_t   	max_MSDU_size;  /* Maximum MSDU size in bytes */
   u_int16_t    txop;           /* TXOP duration in microseconds */
   u_int16_t    pre_txop;       /* last TXOP duration in microseconds */
   u_int32_t    max_SI;         /* Maximum service interval in microseconds */
   u_int16_t    overhead;       /* Overheads includes ACKs, CF-Polls, interframe spaces */
   u_int32_t    delay_bound;    /* Delay bound in microseconds */
   u_int32_t	Num;		/* Number of packets during the TXOP */
   pro_		protocol;
   u_char	user_pri;

   tspec_desc*  next;
};

class QosScheduler{

 public:
	QosScheduler(mac802_11e* mac){ mac_ = mac; }

	virtual 	~QosScheduler(void){}

	/* For QSTA Scheduler only */
	virtual int 	get_qsize(int tid){return 0;}			/* get size of tid queue */
	virtual	void	set_poll_tsid(int){}				/* recv poll frame, set poll_tsid */	

	/* For QAP HCCA Scheduler only */
	virtual double  next_CAP(){return 0;}					/* get next CAP start time */
	virtual int    	reg_TSPEC(int nodeID, char *tspec, enum pro_ proto, u_char *mac){return 0;}		/* Register a Traffic Stream */
	virtual void	delete_TS(u_char nid, u_char direction, u_char tsid){}  /* delete a registed TS */
        virtual int     pkt_enque(ePacket_ *p){return 0;}                	/* enque a packet at HCCA scheduler queue */
        virtual void    pkt_deque(HCCA_TxP& schTx){}            		/* deque the next packet to transmit */
	virtual void 	change_state(HCCA_State e, ePacket_ *p){}
	virtual void 	recover_status(void){}
	virtual void 	next_TSPEC(void){return;}

 protected:
	mac802_11e	*mac_;
	int		tsNum;	/* Number of traffic streams */

};

#endif
