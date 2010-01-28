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

/* Adaptive Distance Vector daemon header */
#ifndef	__NCTUNS_advd_h__
#define __NCTUNS_advd_h__

#include <event.h>
#include <timer.h>
#include <object.h>
#include <list>
#include <sys/types.h>
#include <netinet/in.h>

// The Time-To-Live unit to decrease whenever receiving a IP packet
#define	ADVIPTTLDEC	1

// return values of RtTable::updateEntry()
#define	UPDATE_FAIL	-1
#define	UPDATE_SUCCESS	1
#define	UPDATE_TOINF	2

/* Parameters used in the ADV */
#define	DEL_INF_ENTRY	3000	// The infinite entry expired time

// In ADV update entry, the value of expected-response
#define	EXP_ZERO	0	/* Expected response, bit sequence 00 */
#define	EXP_LOW		1	/* Expected response, 01 */
#define	EXP_MEDIUM	2	/* 10 */
#define	EXP_HIGH	3	/* 11 */

// In ADV algorithm, the network speed is considered as a factor
//	when sending update entry
// However, it is not implemented yet because of lack support from lower level
#define	HIGH_SPEED	1
#define	LOW_SPEED	2

#define	PERIOD_INTERVAL	-1	/* In ADV, this parameter is always infinite */

// In route entry, the metric is set to this to indicate it's invalid
#define	METRIC_INFINITE	999

/* The constant flag put in packet's pktInfo_ and */
/* are used to distinguish packet's type.	  */
/* If not found, suppose that is a normal data packet */
#define	PARTIAL_UPDATE	"PAR_updatE"
#define	FULL_UPDATE	"fUL_uPdAtE"
#define	INIT_CONNECTION	"InIt_COnn_"
#define	RECV_ALERT	"RecV_aLeRT"

// Some trivial and meaningful constant
#define	YES		1
#define	ON		1
#define	NO		0
#define	OFF		0
#define	REACHABLE	1// for routing()
#define	NOROUTE		2
#define	INFINITEENTRY	3

//Namespace
namespace	ADV{

/* Routing update entry */
typedef	struct update_entry {

	u_long		Dst_IP;
	u_long		Nexthop_IP;
	int		Metric;
	int		seqNum;
	u_long		seqSignHost;
	int		Is_receiver;	/* YES or NO */
	int		Exp_response;	/* Expected response */
	char		Unused[1];	/* Unused now; follow paper */
}update_entry;


typedef struct recv_alert {

	char		pktTYPE[11];
	u_long		replier;
	int		metric;
	int		seqNum;
	u_long		seqSignHost;
	u_long		lasthost;
}recv_alert;


/* Every entry within routing table */
typedef struct rt_entry{

 	u_long  	Dst_IP;
	u_long 		Nexthop_IP;
	int 		Metric;
	int		seqNum;
	u_long		seqSignHost;
	int		Recv_flag;/* ADV's receiver flag; YES or NO */

	int		Pkts_handled;	/* A non-zero value indicates */
					/* I am a forwarding node for */
					/* the entry destination.     */
	int		JustUpdated;/* For partial update; YES or NO  */
}rt_entry;


class RtTable{

 private:
 	rt_entry* 	rtptr;	// pointer to routing table
	int 		maxlen;
	int 		len;	// number of entries

 public:	
 	RtTable();
	~RtTable();

	int			NumOfPartial(void);
	int 			NumOfRecvFlag(void);
	int			NumOfJustUpdated(void);
	inline int		NumOfEntry(void);
	inline rt_entry* 	head(void);
	int 			addEntry(u_long, u_long, int, int, u_long, int);
	inline rt_entry 	getEntry(int);
	inline int 		ifExist(u_long);
	int 			updateEntry(const u_long, u_long, int, int, u_long, int);
	int			setRecvflag(u_long, int);
	int			setJustUpdated(int, int);
	int			hdledCntHalf(int);
	int			hdledCntIncre(int);
	int			pktsHandled(int);
	int			rmEntry(int);
	int			setSeqNow(int);

	bool			ifMtrInf(int);
	bool			ifJustUpdated(int);
	bool			ifRecvflagOn(int);
	int			refresh(void);

	int			dumprt(void);
};/* class RtTable */

struct buf_list {
	ePacket_		*queued_pkt;// The buffered ePacket
	struct buf_list*	next;
};

typedef struct ctrl_entry {

	u_long			Dst_IP;/* where control pkt will go */
	int			b_id;/* Broadcast ID */
	struct buf_list*	buffer_list;
}ctrl_entry;

class ctrlTable{

		/* ctrlTable: Control Table
		 *
		 * The table is used to save the init-connection records.
		 *
		 */
	private:
		ctrl_entry*	ctrlptr;
		int		maxlen;
		int		len;
		int		ctrlThreshold;

	public:
		ctrlTable();
		~ctrlTable();

		inline void		setThreshold(int t) {\
						ctrlThreshold = t;};
		inline ctrl_entry*	head(void);
		int			addEntry(u_long, int);
		int			attachPKT(u_long, ePacket_*);
		int			rmEntry(u_long);
		int			updateEntry(u_long, int);
		int			ifExist(u_long);
		int			ifPktBuffered(u_long dst_IP);
		inline ctrl_entry	getEntry(int index);
		int			getBid(u_long);
		void			bidIncre(u_long);


		int			dumpctrl(void);
};/* class ctrlTable */


typedef struct init_conn {

	char		pktTYPE[11];
	ctrl_entry	wanted;
	u_long		sIP;// source's IP
	int		sMetric;// source's Metric
	int		sSeqNum;// source's seqNum
	u_long		sSeqSignHost;// source's seqSignHost
	u_long		lasthost;
}init_conn;


class ADVd : public NslObject {

 private:
 	/* member data */
 	timerObj		IF_timer, IF_timerRefresh;
 	RtTable			rt_table;
	ctrlTable		ctrl_table;
 	char			*rtfd; 
 	u_long			*myip; 
	u_int64_t		begin_tick;
	u_int64_t		thre_tick;
 	int 			sequence_now;
	int			buf_pkt_cnt;
 	
	int			TRGMETER_FULL;// The following 7 parameters
	int			TRGMETER_HIGH;// 	are set from user
	int			TRGMETER_MED;//		in the begging.
	int			TRGMETER_LOW;
	int			MIN_INTERVAL;
	int			BUF_THRESHOLD;
	int			MAXBUFFEREDPKT;
	int			trigger_thresh;	/* trigger threshold */
	int			trigger_meter;	/* Trigger meter */
	int			network_speed;	/* HIGH_SPEED or LOW_SPEED */
	int			num_partial;	/* for compute_thresh() */
	int			sum_TRGMETER;	/* for compute_thresh() */

		/* For rd_queue control */
	int			qmax_;
	int			qcur_;
	ePacket_		*rd_head;
	ePacket_		*rd_tail;

 	/* member functions */
	int			partialUpdate(void);
	int			fullUpdate(void);

	int			processInit(ePacket_*);
	int			processAlert(ePacket_*);
	int 			processUpdate(ePacket_ *p); 

	int			checkTrgmeter(void);
	int			updateTrgmeter(int);
	int			processBuffered(u_long);
	int			floodingInit(u_long, u_long, int, int, u_long);
	int			replyRecvalert(u_long, int, int, u_long);
	int			expected(int EXPvalue);
	int			convert(update_entry* , int);
				/*
				 In convert(), the information indexed by
				 integer will be copied into update_entry.
				 And the expected response will be set at the
				 same time, because it will do the considera-
				 tion about the response.
				*/

	int			compute_thresh(void);
	int			buf_pkt_incre(void);
	int			buf_pkt_decre(void);

	int			refreshRtTable(void) { return(rt_table.refresh()); };
		/* For rd_queue contrl */
	int			push(void);
	int			sendToQueue(ePacket_*);

 public:
 	ADVd(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	~ADVd(); 

 	 
 	int			init();
 	int			recv(ePacket_ *pkt);
 	int			send(ePacket_ *pkt);
 	int			Debugger(char*);   

	int			routing(u_long dst, Packet *pkt);
	int			adv_hdler(ePacket_ *);

};/* class ADVd */
 
}; // Namespace ADV
 
#endif	/* __NCTUNS_advd_h__ */
