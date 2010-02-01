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

#ifndef __NCTUNS_event_h__
#define __NCTUNS_event_h__

#include <string.h>
#include <config.h>


/*=======================================================================
   Define Macros
  =======================================================================*/
#define CREATE_EVENT(ep)                                                \
{                                                                       \
        ep = (Event_ *)malloc(sizeof(Event_));                          \
        if (!ep)                                                        \
                ErrorMesg("malloc() error in CREATE_EVENT()");          \
                                                                        \
        /* initialize event */                  \
        ep->created_ts_ = 0;                            \
        ep->timeStamp_ = 0;                             \
        ep->perio_ = 0;                                 \
        ep->priority_ = 0x40 & 0x3f;                             \
        ep->calloutObj_ = NULL;                 \
        ep->func_ = NULL;                               \
        ep->DataInfo_ = NULL;                   \
        ep->next_ep = NULL;                             \
        ep->flag = DEFAULT;                             \
        ep->optval = 0;                                 \
}

#define FREE_EVENT(ep)                                                  \
{                                                                       \
        /*rev_deleted_event(ep);*/                                          \
        free(ep);                                                       \
}

#define SET_EVENT_TIMESTAMP(ep, timeStamp, perio)                       \
{                                                                       \
        ep->timeStamp_ = timeStamp;                                     \
        ep->perio_ = perio;                                             \
}

#define SET_EVENT_REUSE(schdlr, ep)                                     \
{                                                                       \
        if (ep->perio_ > 0) {                                           \
           ep->timeStamp_ = currentTime_[0] + ep->perio_;               \
           SCHEDULER_INSERT_EVENT(schdlr, ep);                          \
        }                                                               \
}

#define SET_EVENT_CALLOUTFUNC(ep, fun, data)                            \
{                                                                       \
        ep->func_ = fun;                                                \
        ep->DataInfo_ = data;                                           \
        ep->calloutObj_ = NULL;                                         \
}

#define SET_EVENT_CALLOUTOBJ(ep, obj, memf, data)                       \
{                                                                       \
        ep->calloutObj_ = obj;                                          \
        ep->memfun_ = memf;                                             \
        ep->DataInfo_ = data;                                           \
        ep->func_ = NULL;                                               \
}

#define SET_FUNC_EVENT(schdlr, ep, timeStamp, perio, fun, data)         \
{                                                                       \
        SET_EVENT_TIMESTAMP(ep, timeStamp, perio);                      \
        SET_EVENT_CALLOUTFUNC(ep, fun, data);                           \
        SCHEDULER_INSERT_EVENT(schdlr, ep);                             \
}

#define SET_OBJ_EVENT(schdlr, ep, timeStamp, perio, obj, memf, data)    \
{                                                                       \
        SET_EVENT_TIMESTAMP(ep, timeStamp, perio);                      \
        SET_EVENT_CALLOUTOBJ(ep, obj, memf, data);                      \
        SCHEDULER_INSERT_EVENT(schdlr, ep);                             \
}

#define REG_POLLER(obj, memb, fd, tid)                                  \
{                                                                       \
        extern u_long reg_poller(NslObject *, int (NslObject::*)(Event_ *), int *);\
                                                                        \
        tid = reg_poller(this, (int (NslObject::*)(Event_ *))&obj::memb, &fd);\
}
//extern u_long reg_poller(NslObject *, int (NslObject::*)(Event_ *), int *);
//tid = reg_poller(this, (int (NslObject::*)(Event_ *))&(obj::memb), &fd);




#define Message_                        Event
#define Frame_                          Event
#define ePacket_                        Event
#define Event_                          Event


/*======================================================================
   Event structure definition
  ======================================================================*/

class NslObject;

typedef struct event {
        int flag;
        int optval;
	int pid;
	int socket_fd;


        u_int64_t       timeStamp_;
                        /* In simulation, the event scheduler of NCTUNS
                         * will maintain a virtual clock. All events are
                         * assigned a timestamp based on the virtual cock.
                         * When the timestamp is expired, the event scheduler
                         * will trigger that event to be executed.
                         */

        u_int64_t       perio_; /* for periodically event */

        NslObject       *calloutObj_;
        int             (NslObject::*memfun_)(struct event *);
                        /* When the call out object is a OBJECT, we should
                         * specified which object we want to call and it's
                         * member function. Here memfun_() is a
                         * pointer-to-member pointer.
                         */

        int             (*func_)(struct event *);
                        /* When the call out object is a FUNCTION, we should
                         * specified the address of that function to fun_().
                         * func_() here is a function pointer which points
                         * to the desired function.
                         */

        void            *DataInfo_;
                        /* An argument of func(). When time is expired,
                         * the calloutObj_ and func() will be called. func()
                         * can ether be a method or only a function. The
                         * DataInfo is a variable of func(), which points to
                         * a void type.
                         */

        u_char          priority_;
                        /* The priority of event.
                         * The first two bits in MSB are used to identify
                         * packet priority. The possible values of these two
                         * bits are defined in mbinder.h
                         */


        struct event    *next_ep; /* for call out timer */

        u_int32_t       created_nid_;
        u_int64_t       created_ts_;

} Event;

// define the flag's value
#define DEFAULT		0x01
#define KTIMEOUT	0x02
#define CHKTUN		0x04
#define CMD_SERVER	0x08

/*=======================================================================
   function declaration
  =======================================================================*/
int showCurrentTime_Event               (u_int32_t);
int showCurrentTime                     (Event_ *);
int advanceNodeClock_Event              (u_int32_t);
int advanceNodeClock                    (Event_ *);
int checkCallout_Event                  (u_int32_t);
int checkCallout                        (Event_ *);
int read_trafficGen                     (void);
int SyncWithGUI                         (Event *);
int SyncWithGUI_Event                   (double);
int checkIPC                            (Event *);
int checkIPC_Event                      (double);
int SyncWithRealtime(Event *);
int SyncWithRealtime_Event(double);

struct tgInfo {
        u_char todo;
#define FORK_PROCESS		0x01
#define KILL_PROCESS		0x02
        char cmd[1001];
        int pid;        // pid of killed process
        u_int64_t endtime;      // end time
};

struct tcpdumpInfo {
        int pid;        // pid of tcpdump process
        int nid;        // node id of process executing tcpdump
        int portid;;    // port id of process executing tcpdump
};

int rec_created_event(Event* ep);
int rev_deleted_event(Event* ep);
int trafficGen_Event(Event *ep);

#endif  /* __NCTUNS_event_h__ */
