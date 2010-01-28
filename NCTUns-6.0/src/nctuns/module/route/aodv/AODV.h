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

#ifndef __NCTUNS_AODV_h__      
#define __NCTUNS_AODV_h__    
 
#include <event.h>
#include <object.h>
#include <mylist.h>
#include <timer.h>  
#include <packet.h>
//#include <route/aodv/mstate.h>

#define INFINITY_HOPCNT  0xff
#define INFINITY_LIFETIME  0x7fffffff
#define AODV_MAXQUEUELEN 30

#define TTL_THRESHOLD 7

#define LINK_FAIL_LIFETIME            500   // ms
#define LINK_FAIL_THRESHOLD           4     // times

// millisec
#define SENDHELLO_TIMER               50
#define ROUTE_CHECK                   300
#define PENDING_RREQ_CHECK            50
#define RECENT_RREQ_LIST_CHECK        300
#define ACCUMULATED_RREQ_RERR_TIMER   1000
#define NEI_LIST_CHECK                300
#define LINK_FAIL_LIST_CHECK          1000      


namespace AODVd{   

class AODV;

/*
 *   Neighbor entry and Neighbor List
 */
class Nei_entry {
public:
       u_long		       nei_addr;     // ip address
       u_int64_t               nei_time;     // expire time

       Nei_entry              *next;
public:
       Nei_entry(u_long addr, u_int64_t lifetime);
       ~Nei_entry();
};

class Neighbor {
private:
	Nei_entry            *nei_head;

public:
	Neighbor();
	~Neighbor();

	//int          isExist(u_long addr);
	// If not exist, new entry is created, otherwise update the lifetime.
	int          update(u_long addr, u_int64_t lifetime);
	int          remove(u_long addr);
	Nei_entry*   getEntry(u_long addr); 
	Nei_entry*   getHead();
};

struct Link_fail_entry {
	SLIST_ENTRY(Link_fail_entry) next;
	u_long        dst_ip;
	u_char        acc_cnt;
	u_int64_t     lifetime;
};

/*
 *  Broadcast ID and BroadcastID List
 */
struct BroadcastID {
       SLIST_ENTRY(BroadcastID) nexB;
       u_long			addr;
       u_int32_t		bid;
       u_int8_t                 hopcnt;
       u_int64_t                lifetime;
};



class Rt_entry {
public:
       u_long		       rt_dst;     // destination ip
       u_long                  rt_nexthop; // nexthop
       bool                    rt_valid_dst_seqno;// dst seqno is valid of not
       u_int32_t               rt_seqno;   // destination sequence number
       u_int8_t                rt_hopcount;// hopcount
#define RTF_INVALID          1
#define RTF_VALID            2
#define RTF_REPAIRABLE       3
#define RTF_BEING_REPAIRED   4
       u_int8_t                rt_flags;    // valid/invalid/repairable/repaired
       Neighbor               *rt_preclist; // precurosr list
       u_int64_t               rt_time;     // expire time

       Rt_entry               *next;

       Rt_entry();
       ~Rt_entry();
};



class AODV_RtTable {

/* friend class Neighbor; */

private:
       Rt_entry	          *rt_head; 
       Rt_entry	          *rt_tail; 

public:
       AODV_RtTable(); 
       ~AODV_RtTable(); 

       int                 insert(Rt_entry *r);
       //int                 update();
       int                 remove(u_long addr);
       int                 removeEntry(Rt_entry *target, Rt_entry *pre);
       int                 updatePrecursorList(u_long dst_addr,u_long nei_addr);
#define RT_NOT_EXIST 0
       int                 rt_lookup(u_long ip);
       Rt_entry*           rt_get(u_long ip); 
       Rt_entry*           rt_getHead(); 
       u_int32_t           myOwnSeqno();
       int                 incMyOwnSeqno();

       // It will return count for the active-routes(except the first entry for
       // its own) in the route table. If the count is equal or greater than
       // one, Hello msg should be broadcasted periodically.
       int                 rt_activeCnt();

};
/*
 *  AODV's queue and its entry
 */
struct buf_list {
       ePacket_            *queued_pkt;
       struct buf_list*    next;
};

/*
 *   every destination has a pkt queue list
 */
class Ctrl_entry {
	public:
       u_long             dst_ip;
       u_int              buf_pkt_cnt;   
       struct buf_list*   buffer_list; 

       u_int64_t          rreq_lifetime;    
       u_int              rreq_retries;

       Ctrl_entry        *next;

	public:
	Ctrl_entry();
	~Ctrl_entry();
};

class CtrlTable {

     private:
        Ctrl_entry*           ctrl_head;

     public:
        CtrlTable();
	~CtrlTable();

	int                   insert(u_long dst, u_int64_t rreq_lt);
	int                   remove(u_long dst);
	int                   attachPkt(u_long dst, ePacket_*);
        bool		      ifExist(u_long);
	Ctrl_entry*           getHead(); 
	Ctrl_entry*           getEntry(u_long);

};

class LocalRepair_entry {
public:
       u_long             dst_ip;
       u_int              buf_pkt_cnt;   
       struct buf_list*   buffer_list; 

       u_int64_t          local_repair_lifetime;    
       u_int              local_repair_retries;
       // brokenlink_node is used for local-repair(linkLayer upcall), 
       // It stores the pkt_gateway(i.e. the disconnected next-hop).
#define NO_USE      0
       u_long             brokenlink_node;

       LocalRepair_entry        *next;

public:
	LocalRepair_entry();
	~LocalRepair_entry();
};

class LocalRepairTable {

private:
        LocalRepair_entry*           local_repair_head;

	/* Ctrl_entry*           getHead(void); */

public:
        LocalRepairTable();
	~LocalRepairTable();

	int                   insert(u_long dst, u_int64_t rreq_lt, u_long disc_node);
	int                   remove(u_long dst);
	int                   attachPkt(u_long dst, ePacket_*);
        bool		      ifExist(u_long);
	LocalRepair_entry*    getHead(); 
	LocalRepair_entry*    getEntry(u_long);

};


struct RREQ_msg {
	char   type;

	u_short  J:1;
	u_short  R:1;
	u_short  G:1;
	u_short  D:1;
	u_short  U:1;
	u_short  Reserved:11;

	u_int8_t          rreq_hopcount;
	u_int32_t         rreq_id;
	u_long            rreq_dst_addr;
	u_int32_t         rreq_dst_seqno;
	u_long            rreq_src_addr;
	u_int32_t         rreq_src_seqno;

};


struct RREP_msg {

	char   type;

	u_short  R:1;
	u_short  A:1;
	u_short  Reserved:9;
	u_short  prefix_size:5;

	u_int8_t          rrep_hopcount;
	u_long            rrep_dst_addr;
	u_int32_t         rrep_dst_seqno;
	u_long            rrep_ori_addr;
	u_int64_t         rrep_lifetime;

};


struct unreach_tuple {
	u_long        unreach_dip;
	u_int32_t     unreach_dseq;
};

struct RERR_msg {
	char  type;

	u_short N:1;
	u_short Reserved:15;

	u_char        destCount;

	struct unreach_tuple  unreach_e;
};

struct unreach_entry {
	u_long        unreach_dip;
	u_int32_t     unreach_dseq;

	struct unreach_entry *next;
};

class Unreach_list {
	u_char                  unr_count;

	struct unreach_entry   *unr_head;

	public:
	Unreach_list();
	~Unreach_list();

	int                     insert(u_long, u_int32_t);
	struct unreach_entry*   getHead();
	u_char                  unreach_count();
};

/*
 *   AODV's pkt format: {
 *     protocol type => { AODV_RREQ or AODV_RREP or AODV_RERR or AODV_HELLO};
 *     destination ip addr;
 *     source ip addr;
 *     who send this pkt to me; => last ipaddr;
 *     who send this pkt to me(it's seqno): => last sequence number;
 *     pointer point to RREQ or RREP or RERR or HELLO;
 *   }
 */


struct AODV_packet {
     char               pro_type[11];
     u_long		src_ip;
     u_long		dst_ip;
     u_char             ttl;
     char *		point_msg;
};



class AODV : public NslObject {
                                    // default
        int  HELLO_INTERVAL;        //1000
        int  ALLOWED_HELLO_LOSS ;   //2 
        int  ACTIVE_ROUTE_TIMEOUT;  //3000
        int  DELETE_PERIOD;         //3000
	int  NET_DIAMETER;          //15
	int  NODE_TRAVERSAL_TIME;   //40 ms
	int  RREQ_RETRIES;          //5
	int  RREQ_RATELIMIT;        //10/per sec
	int  RERR_RATELIMIT;        //10/per sec

        int  MY_ROUTE_TIMEOUT;
	int  NET_TRAVERSAL_TIME;   
	int  PATH_DISCOVERY_TIME;   

 private:
   	timerObj		SendHello_timer;
	timerObj		DelHello_timer;
	timerObj                RT_timer;
	timerObj                SendRREQ_timer;
	timerObj                RecentRREQ_timer;
	timerObj                AccRREQ_RERR_timer;
	timerObj                Nei_List_timer;
	timerObj                PrintLoc_timer;
   	 
	// interval tmp variable
   	u_int64_t               hello_interval_; 
   	u_int64_t               delete_period_; 
	 
	u_int64_t               active_route_timeout_;
	u_int64_t               my_route_timeout_;
	u_int64_t               node_traversal_time_;
	u_int64_t               net_traversal_time_;
	u_int64_t               path_discovery_time_;
	
	u_int64_t               route_check_timer_;
	u_int64_t               rreq_check_timer_;
	u_int64_t               recent_rreq_list_timer_;
	u_int64_t               accumulated_rreq_rerr_timer_; 
	u_int64_t               nei_list_check_timer_;
	u_int64_t               sendhello_timer_;
	u_int64_t               link_fail_list_check_timer_;
	u_int64_t               printLoc_timer_;

   	u_long			*mip;               // my IP address

   	int                     rreq_id;            // my rreq ID
	
   	AODV_RtTable            rtable;             // my Routing Table
   	CtrlTable               ctrl_table;         // my AODVqueue table
	LocalRepairTable        local_repair_table;
   	SLIST_HEAD( ,Link_fail_entry)  link_fail_list;
   	SLIST_HEAD( ,BroadcastID)  bcache;          // Broadcase ID cache
	Neighbor                nei_list;           // neighbor list
	
	// accumulated count for RREQ/RERR, will be clean to 0 every second
	int                     acc_rreq;           
	int                     acc_rerr;           
	
   	int                     qmax_;              // AODVqueue's max
	int                     qcur_;              // AODVqueue current count
	ePacket_                *rd_head;           // AODVqueue's head
	ePacket_                *rd_tail;
	
	
	public:

   	AODV(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
    	~AODV();
	
   	int                      init();
   	int                      recv(ePacket_ *pkt);
   	int                      send(ePacket_ *pkt);
   	int			 sendHello();
	int			 miew();
	
	int			 HelloTimer();
	int                      RTTimer();
	int                      RREQ_retry();
	int                      CheckRecentRREQ();
	int                      ClearAccRREQ_RERR();
	int                      CheckNeiList();
	int                      CheckLinkFailList();

	 
   	int			 UpdateHello(struct HELLO_msg *);
   	int			 routing(u_long dst, Packet *p);
	int                      updateSimpleRRoute(u_long prevhop_ip);
	int		         updateRT(u_long dst, u_long nexthop, u_int32_t seqno, u_int16_t hopcount, u_int64_t lifetime);
	
   	int			 sendRREQ(u_long dst, u_char ttl);
   	int			 forwardRREQ(struct RREQ_msg *my_rreq, u_char cur_ttl);		
   	int			 sendRREP(u_long dst, u_long src, u_long toward, u_int8_t hopcount, u_int32_t seqno, u_int64_t lifetime);
   	int 			 forwardRREP(struct RREP_msg *my_rrep, u_char cur_ttl);
   	int			 sendRERR(u_long delip, Unreach_list *); 
   	int			 bcastRERR(Unreach_list *); 
	
   	int			 push(void);
	int			 sendToQueue(ePacket_*);
	int			 processBuffered(u_long);
	int			 LinkLayerCall(ePacket_ *);
	int			 PrintIP(u_long);
};

}; //namespace AODVd

#endif  /* __NCTUNS_AODV_h__ */
