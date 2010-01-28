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

#ifndef __NCTUNS_ofifo_h__
#define __NCTUNS_ofifo_h__

#include <object.h>
#include <event.h>
#include <timer.h>

/* Define Interface Queue for every Interface */
struct ifqueue {
	ePacket_		*ifq_head; /* head of ifq */
	ePacket_		*ifq_tail; /* tail of ifq */
	int			ifq_len;   /* current queue length */
	int			ifq_maxlen;/* max queue length */
	int			ifq_drops; /* drops count */
};


/* Define Macros for IFq */
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


class ofifo : public NslObject {

 private:

	struct ifqueue		if_snd; /* output interface queue */

	char			*log_qlen_flag; /* on/off */
	char			*logQlenFileName;
	FILE			*logQlenFile;
	
	char			*log_option; /* sample_rate / full_log */
	int			log_SampleRate; /* samples/sec */
	char			full_log_flag;

	timerObj		logTimer;

 protected:
 	int 			intrq(MBinder *);

 public:

	ofifo(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~ofifo();

	int			init();
	int			recv(ePacket_ *);
	int			send(ePacket_ *);
	int			command(int, const char **);
	int			log();
};


#endif /* __NCTUNS_ofifo_h__ */

