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

#ifndef NCTUNS_hc_scheduler_h
#define NCTUNS_hc_scheduler_h

#define MAX_NNUM        10

class QosScheduler_HC : public QosScheduler {
public:
	QosScheduler_HC(mac802_11e* mac);
	~QosScheduler_HC(void);

	int 	reg_TSPEC(u_char *mac, char *tspec, enum pro_ proto);	/* add a Traffic Stream */
	void	delete_TS(u_char *mac, dir_ direction, u_char tsid); 	/* delete a TS */
	int 	pkt_enque(ePacket_ *p);			/* enque a packet */
	void 	pkt_deque(HCCA_TxP &scheTx);		/* deque the next packet */
	void 	change_state(HCCA_State e, ePacket_ *pkt);		/* notify a event */
	void 	recover_status(void);	/* restore the state */
	void	next_TSPEC(void);	/* get next TSPEC */
	double 	next_CAP();		/* get the start time of the next TXOP or SI*/		

 protected:
	tspec_desc* 	create_TSPECdesc(u_char *mac, char *tspec, enum pro_ proto);
	bool 	find_TSPECdesc(u_char *mac, dir_ direction, u_char tsid);
	int	getIndex(u_char* src);	

	/* tspec_desc List */
        tspec_desc* 	head_desc;
        tspec_desc* 	curr_desc;
	tspec_desc*	pre_desc;

	/* HCCA MAP */ 
	u_int16_t	Service_Interval;	/* Service Interval in ms */
	u_int16_t	pre_SI;			/* Last Service Interval in ms */
	u_int64_t	SIstart;		/* Service Interval Start Time in ticks*/
	u_int64_t	TXOPstart;		/* TXOP Start Time in ticks */
        u_int64_t 	pre_SIstart;		/* Last Service Interval Start Time in ticks */
   	u_int64_t 	pre_TXOPstart; 		/* TXOP Start Time in ticks */

 private:

	u_int16_t 	sifs;
	u_int16_t	pifs;
	bool		first_call;	/* If build_TSPEC_list is called */
	bool		first_deque;	/* if deque() is first called in current TXOP */
	bool		pre_deque;	/* last value of first_deque */
	u_char		qsta[6];	/* current polled QSTA*/
	double 		pktNum;
	int 		nodeNum;
	
	ParaQueue**	ts_[MAX_NNUM];
	int		max_qsta;

	u_char          srcmac[MAX_NNUM][ETHER_ADDR_LEN];       /* mac address */
};
#endif
