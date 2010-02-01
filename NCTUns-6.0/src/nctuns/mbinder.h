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

#ifndef __NCTUNS_mbinder_h__
#define __NCTUNS_mbinder_h__

#include <event.h>

class MBinder;
class NslObject;

/*
 * Define Module-Binder Queue 
 */
struct mbqueue {
	ePacket_	*mbq_head;	/* head of mb queue */
	ePacket_	*mbq_tail;	/* tail of mb queue */

	int		mbq_len;	/* current queue length */
	int		mbq_maxlen;	/* max queue length */
	int		mbq_drops;	/* drops count */

        u_short         co_type;        /* call out type, see below */
        union {
	    struct {
		NslObject	*coObject_;
		int		(NslObject::*upcallm_)(MBinder *);
	    } str_1;
	    int			(*upcallf_)(MBinder *);
        } un;
};
#define coObj			un.str_1.coObject_
#define m_upcall		un.str_1.upcallm_
#define f_upcall		un.upcallf_

/* define flags for co_type */
#define NO_UPCALL		0x000
#define MEMB_UPCALL		0x0001
#define FUNC_UPCALL		0x0002

/* Define Macros for Module-Binder Queue */
#define MB_QFULL(mbq)           ((mbq)->mbq_len >= (mbq)->mbq_maxlen)
#define MB_DROP(mbq)            ((mbq)->mbq_drops++)
#define MB_ENQUEUE(mbq, m) { \
        if ((mbq)->mbq_tail == 0) \
                (mbq)->mbq_head = m; \
        else \
                (mbq)->mbq_tail->next_ep = m; \
        (mbq)->mbq_tail = m; \
        (mbq)->mbq_len++; \
}
#define MB_PREPEND(mbq, m) { \
        (m)->next_ep = (mbq)->mbq_head; \
        if ((mbq)->mbq_tail == 0) \
                (mbq)->mbq_tail = (m); \
        (mbq)->mbq_head = (m); \
        (mbq)->mbq_len++; \
}
#define MB_DEQUEUE(mbq, m) { \
        (m) = (mbq)->mbq_head; \
        if (m) { \
                if (((mbq)->mbq_head = (m)->next_ep) == 0) \
                        (mbq)->mbq_tail = 0; \
                (m)->next_ep = 0; \
                (mbq)->mbq_len--; \
        } \
}


/*
 * The priority values are defined for priority_ member 
 * of event struct. We use the first two bits at MSB to
 * identify packet priority_. Hence there are 4 possible
 * priorities, named level-1, level-2, level-3, and level-4.
 * In which of them, level-1 has the most low priority and
 * level-4 has the most high priority.
 */
#define MLEVEL_1				0x00
#define MLEVEL_2				0x40
#define MLEVEL_3				0x80
#define MLEVEL_4				0xc0

#define SET_MLEVEL_1(mepkt) \
	(mepkt)->priority_ = (((mepkt)->priority_ & 0x3f) | MLEVEL_1)
#define SET_MLEVEL_2(mepkt) \
	(mepkt)->priority_ = (((mepkt)->priority_ & 0x3f) | MLEVEL_2)
#define SET_MLEVEL_3(mepkt) \
	(mepkt)->priority_ = (((mepkt)->priority_ & 0x3f) | MLEVEL_3)
#define SET_MLEVEL_4(mepkt) \
	(mepkt)->priority_ = (((mepkt)->priority_ & 0x3f) | MLEVEL_4)
#define GET_MLEVEL(mepkt) \
	((mepkt)->priority_ & 0xc0)


/*===========================================================================
   MBinder Class
  ===========================================================================*/
class MBinder {
 
 private:

	struct mbqueue		mbq;
	NslObject		*frmObj;  /* from which object */
	NslObject		*toObj;   /* to which object */
 public:

	MBinder                 *next_mb; /* to next MBinder */

	MBinder(NslObject *);
	MBinder(NslObject *, int qlen);
	~MBinder();

	inline void 		set_maxqlen(int len) { mbq.mbq_maxlen = len; };
	inline int		get_maxqlen() { return(mbq.mbq_maxlen); };
	inline int		get_curqlen() { return(mbq.mbq_len); };
	inline int		get_drops() { return(mbq.mbq_drops); };
	inline int		qfull() { return((MB_QFULL(&mbq))); };
	inline void		set_upcall(int (*func_)(MBinder *)) {

		mbq.co_type = FUNC_UPCALL;
	        mbq.f_upcall = func_;
	};	
	inline void		set_upcall(NslObject *obj, 
		int (NslObject::*mem_)(MBinder *)) {

		mbq.co_type = MEMB_UPCALL;
	        mbq.coObj = obj;
	        mbq.m_upcall = mem_;
	};	
	inline void		bind_to(NslObject *toO) { toObj = toO; };
	inline NslObject	*myModule() { return(frmObj); };
	inline NslObject	*bindModule() { return(toObj); };

	int			serve();
	ePacket_		*enqueue(ePacket_ *);
	void			try_upcall();
	
};

#endif /* __NCTUNS_mbinder_h__ */

