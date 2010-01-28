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

#ifndef	__NCTUNS_dsdvd_h__
#define __NCTUNS_dsdvd_h__

#include <event.h>
#include <timer.h>
#include <object.h>
#include <list>
#include <sys/types.h>
#include <netinet/in.h>

#define 	BIG 				250
#define 	limit				43  

/*
 *  when define DUMPROUTINGTABLE , dsdvd will dump routing information to log file.
 *  and each node has its own file.
 */
//#define 	DUMPROUTINGTABLE

namespace DSDV{

class sequence_num{
 public:
    int num;
    u_long sign_host; // the host which set the destination seq_num
    sequence_num(){
    	num = 0;	
        sign_host = 0;
    }
};	

class dsdv_hdr{
 public:
     char name[5]; // fill in dsdv protocol name 
     int len; 	// rt_unit element number. 			 	
     dsdv_hdr(){
         strncpy(name, "dsdv", 5);	
     }
};

class rt_unit{
 public:
 	u_long  		dst;
	u_long 			nextHop;
	int 			metric;
	sequence_num 		seq_num;
	u_int64_t	 	install_time; // rt_unit modify time.
	// if nextHop change. set need_adv_now =1
	u_int8_t 		need_adv_now;
	u_int8_t		expire_flag; 
	u_int8_t 		already_bcast;
	
	rt_unit(){
	    dst = 0;
	    nextHop = 0;
	    metric = 0;
	    install_time = 0;	
	    need_adv_now = 0;
	    //need_adv_regular = 0;
	    expire_flag = 0;
	    already_bcast = 0;
	}
};

class RtTable{
 private:
 	rt_unit 		*rtptr;
	int 			maxlen;
	int 			len;
	int 			now_counter; // count need_adv_now. 	
 public:	
 	RtTable();
	~RtTable(){delete [] rtptr;}
	inline int 		getLen(){ return len;}
	inline int 		getMaxLen(){ return maxlen; }
	inline rt_unit* 	getHead(){ return rtptr; }
	inline int 		flushNowCounter(){
					now_counter = 0;
					return 1;
 	                        }
	inline int 		getNowCounter(){ return now_counter; }
	inline int 		addNowCounter(){
	                        	now_counter ++;
					return 1;
				}
	inline rt_unit 		getUnit(int index){ return rtptr[index]; }
	int 			addUnit(const rt_unit *unit);
	int 			getIndex(u_long dst);
	int 			updateUnit(int index, rt_unit *r);
#ifdef DUMPROUTINGTABLE
	void 			dump(FILE *);
#endif
	int 			rebuild();  // return routing table length
};

class DSDVd : public NslObject {
 private:
 	// member data
 	timerObj		IF_timer;
 	RtTable			rt_table;	
#ifdef DUMPROUTINGTABLE
	FILE			*fp;
#endif
 	u_long			*mip; 
 	int 			seq;
 	 	
 	int			adv_interval ;	   // (ms)
	u_int64_t		adv_interval_ ;	   // (tick)		
 	int 			expire_time_out ;   // (ms)
 	u_int64_t 		expire_time_out_ ; // (tick)
 	bool	 		adv_flag;  
 	
 	// member func that use by myself
	int 	 		map[11][11];  // 10 node at most 
	int 			sendAdvPktRegular();
	int 			sendAdvPktNow();
	int 			recvAdvPkt(Packet *p); 
#ifdef DUMPROUTINGTABLE
	int 			dumpAdvPkt(Packet *p);
#endif
	int  			update(u_long from, const rt_unit *unit);

	// the method confirm_src is used for temporary
	int 			selfCheck();
 public:
 	DSDVd(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	~DSDVd(); 
 	 
 	int			init();
 	int			recv(ePacket_ *pkt);
 	int			send(ePacket_ *pkt);
 	int			Debugger();   

	int			routing(u_long dst, Packet *pkt);

};
 
}; // end of namespace DSDVd 
 
#endif	/* __NCTUNS_dsdvd_h__ */
