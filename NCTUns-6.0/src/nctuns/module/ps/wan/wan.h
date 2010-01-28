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

#ifndef __NCTUNS_wan_h__
#define __NCTUNS_wan_h__

#include <object.h>
#include <timer.h>
//#include <event.h>

/*
 * define constant
 */
#define LOSTRATE	0x01
#define DELAYTIME	0x02
#define REORDER		0x04


#define MAX_RANDOM	0x7fffffff

/*
 * Define Interface Queue for every Interface
 */
struct ifqueue {
	ePacket_		*ifq_head; /* head of ifq */
	ePacket_		*ifq_tail; /* tail of ifq */
	int			ifq_len;   /* current queue length */
	int			ifq_maxlen;/* max queue length */
	int			ifq_drops; /* drops count */
};

/*
 * Define Macros for IFq (interface queue)
 */
#define IF_QFULL(ifq)           ((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define IF_DROP(ifq)            ((ifq)->ifq_drops++)
#define IF_ENQUEUE(ifq, m) { \
        if ((ifq)->ifq_tail == 0) \
                (ifq)->ifq_head = m; \
        else \
                (ifq)->ifq_tail->next_ep = m; \
        (ifq)->ifq_tail = m; \
        (ifq)->ifq_len++; \
}
#define IF_PREPEND(ifq, m) { \
        (m)->next_ep = (ifq)->ifq_head; \
        if ((ifq)->ifq_tail == 0) \
                (ifq)->ifq_tail = (m); \
        (ifq)->ifq_head = (m); \
        (ifq)->ifq_len++; \
}
#define IF_DEQUEUE(ifq, m) { \
        (m) = (ifq)->ifq_head; \
        if (m) { \
                if (((ifq)->ifq_head = (m)->next_ep) == 0) \
                        (ifq)->ifq_tail = 0; \
                (m)->next_ep = 0; \
                (ifq)->ifq_len--; \
        } \
}

class WAN : public NslObject {

 private:

	//struct 	ifqueue if_snd; /* output interface queue */

	timerObj		AlarmTimer;
	
	double			*d_normal;
	double			*r_normal;
	int			CASE;
	
	/* on/off */
	char			*lost_opt;
	char			*dtime_opt;
	char			*reorder_opt;

	char			*delay_dis;  /* d_con, d_uni, d_exp, d_nor */
	char			*reorder_dis;/* r_con, r_uni, r_exp, r_nor */
/*	
	int			dcon;
	int			duni;
	int			dexp;
	int			dnor;

	int			rcon;
	int			runi;
	int			rexp;
	int			rnor;
*/	
	double			LostRate;//percent %

	int			dcon_mean;
	int			duni_min;
	int			duni_max;
	int			dexp_min;
	int			dexp_mean;
	int			dexp_max;
	int			dnor_mean;
	int			dnor_var;

	double			R_rate;//percent %
	
	int			rcon_mean;
	int			runi_min;
	int			runi_max;
	int			rexp_min;
	int			rexp_mean;
	int			rexp_max;
	int			rnor_mean;
	int			rnor_var;

	double			var;
	double			mean;

	int			drops;

 public :

	WAN(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~WAN();

	int			init();
	int			send(ePacket_ *);

	int			uniform(double, double);
	int			expo(double, double, double);
	void			normal(double, double, double *);

	int			TimeToSendPkt(ePacket_ *);
};

#endif /* __NCTUNS_wan_h__ */

