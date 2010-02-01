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

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <fcntl.h>

#ifndef LINUX
#include <kvm.h>
#include <nlist.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <heap.h>

#include <gbind.h>
#include <tun_mmap.h>
#include <scheduler.h>
#include <mbinder.h>
#include <timer.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>

#include <nctuns_tun.h>
#include <command_server.h>
#include <commun_gui.h>
#include <algorithm>
#if IPC
#include <IPC/ns.h>
#endif

scheduler *scheduler_ = new scheduler;

// Slot 0 of MBH is unused.
MBinder *MBH[MAX_NUM_NODE + 1];
int nid;	// returned by deqheap()/deqtimer()

extern u_int64_t *currentTime_;
extern u_int32_t *tunIFqlen;
extern u_int32_t *t0eqlen;
extern int errno;
extern u_int32_t ifcnt_;
extern struct poll iftun_[]; 
extern commun_gui * commun_gui_;
extern	cmd_server * cmd_server_;

extern list<int> g_tglist;

scheduler::scheduler() : t0efd(-1)
{
	timerHead_ = NULL;  

	// initialize MBH
	memset(MBH, 0, sizeof(struct MBinder *) * (MAX_NUM_NODE + 1));
}

scheduler::~scheduler()
{
}


/*
 * Start the virtual timer in a simulation environment.
 */
void scheduler::run(u_int64_t timeInSec) {
	u_int64_t		  timeInNanoSec;

	/*
	 * Setting how many seconds to simulate. Before
	 * setting this value, we should translate the time
	 * from seconds into clock ticks. 
	 */
	timeInNanoSec = timeInSec * 1000000000;
	simulateTime_ = timeInNanoSec / TICK;  

	t0efd = tun_alloc(EVENT_TUN_NAME);
	
        if (RanSeed == 0) {
           // The GUI user wants to use a random random number seed.
           struct timeval  Time;
           gettimeofday(&Time,NULL);
           srandom(Time.tv_usec);
        }
        else {
           // Otherwise, we should use the seed specified by the GUI user.
           srandom((unsigned long) RanSeed);
        }

        if ( SimSpeed && !strcasecmp(SimSpeed, "AS_FAST_AS_REAL_CLOCK") ) {
           // This is for emulation purpose. The other option 
           // AS_FAST_AS_POSSIBLE is the default option.
           // Right now, the time synchronization granularity is set to 1 ms.
           SyncWithRealtime_Event(1);
        }

	executeEvent(); 

        if (close(t0efd) == -1) {
        	perror("Close /dev/" EVENT_TUN_NAME " failed!");
          	exit(1);
        }
}


int scheduler::executeEvent()
{
	// currentTime_[0] is based on TICK.
	// 1. Dequeue one event.
	// 2. Set time.
	// 3. Polling all tunnel interfaces.
	// 4. Polling MBH.
	// 5. Polling event tunnel.

#if IPC
	static double time, lasttime = 0.0;
#endif
	// Schedule periodic events to check IPC commands sent from user-level agent clients 
	scheduleCheckIPCCommandFds();

	do {
		readt0e();

		nid = 0; // The nid will be updated by the following deqheap() or deqtimer().
		commun_gui_->polling(); // Process IPC commands that may be sent from the GUI

		if ((numEvent() > 0) || (timerHead_)) {	
			// There is at least one event here. We should compare the 
			// timestamp of the first event in the heap and timer lists
			// and choose the one with a smaller timestamp to execute

			if ((numEvent() > 0) && (timerHead_)) {
				if (heap_.eventHeap_[1]->timeStamp_ < timerHead_->timeStamp_) {
					deqheap();
				}
				else if(heap_.eventHeap_[1]->timeStamp_ == timerHead_->timeStamp_) {
					if(heap_.eventHeap_[1]->priority_ >= timerHead_->priority_)
						deqheap();
					else
						deqtimer();
				}
				else {
					deqtimer();
				}
			} else if (numEvent() > 0) {
				deqheap();
			} else if (timerHead_) {
				deqtimer();
			} else {
				assert(0);
			}

			// polling MBH
			if (nid >= 1 && nid <= MAX_NUM_NODE)
			{
				MBinder	*mbp; 
				bool	is_served = false;
				
				mbp = MBH[nid];

				while (mbp != NULL) {
					if (mbp->serve() > 0) {
						is_served = true;
					} else {
					}

					mbp = mbp->next_mb;

					if (mbp == NULL && is_served ) {
						mbp = MBH[nid];
						is_served = false;
					}
				}
			}

#if IPC
			TICK_TO_SEC(time, GetCurrentTime());
			if (time - lasttime >= 0.1) {
				sendTime(GetCurrentTime());
				lasttime = time;
			}
#endif

		} 	// end of checking event
              	else { // No events to process. Now we should set the current time 
                       // to the end of the total simulation time to stop the simulation.
                     currentTime_[0] = simulateTime_ + 1;
              	}
	} while (currentTime_[0] <= simulateTime_);

        /*
         * Noted by Prof. S.Y. Wang on 10/22/2002
         * In the above while condition statment, it is
         * important to use "<=" instead of "<".
         * This is because otherwise if there are multiple events to
         * to be triggered at simulateTime_, only one of them will be
         * triggered. This may cause problems. For example, if 
         * these events are KILLPROCESS events (we want to kill all
         * traffic generator processes at the end of a simulation),
         * only one of them will be killed and all others will be left
         * unattended.
         */

	// Debugging: dump MBH
	/*
	for(int i=1; i<4096; i++) {
		if( MBH[i] != NULL ) {
			MBinder *mbp = MBH[i];

			printf("(%d) ", i);
			while (mbp != NULL) {
				printf("%s ", mbp->bindModule()->get_name());
				mbp = mbp->next_mb;
			}
			printf("\n");
		}
	}
	*/
	return(0); 
}

int scheduler::setEvent(Event_ *ep) {
	return(heap_.insertEvent(ep)); 	
}

int scheduler::numEvent() {

	return(heap_.DumpNumEvent()); 
}

u_int64_t scheduler::numInsertEvent() {

	return(heap_.DumpNumInsertEvent()); 
}

u_int64_t scheduler::numDequeueEvent() {

	return(heap_.DumpNumDequeueEvent()); 
}

int scheduler::schedule_timer(Event_ *T) {

	Event_		*hptr, *tptr;  

 
	if (!T) return(-1);	/* failure */
	if (T->timeStamp_ < currentTime_[0])
		return(-1); /* failure */

	if (!timerHead_) {
		/* queue is empty */
		timerHead_ = T;
	} else {
		/* queue is not empty */
		tptr = NULL; hptr = timerHead_;    
		while(hptr) {
			if (T->timeStamp_ < hptr->timeStamp_) {
				if (!tptr) {
					/* insert to head */
					T->next_ep = timerHead_;
					timerHead_ = T;
					return(1); 
				} else {
					T->next_ep = hptr;
				  	tptr->next_ep = T;
				  	return(1); 
				}
			}
			tptr = hptr;
			hptr = hptr->next_ep; 
			assert(T!=hptr);
		}
		/* insert into last */
		tptr->next_ep = T;  
	}
	return(1); 
}


int scheduler::cancel_timer(Event_ *T) {

	Event_			*dt; 

	if (!T) return(-1);	/* failure */
	if (!timerHead_) return(-1); 

	if (T == timerHead_) {
		/* delete first one */
		timerHead_ = timerHead_->next_ep;
		T->next_ep = NULL;
	} else {
		dt = timerHead_;  
	       	while(dt->next_ep) {
			if (dt->next_ep == T) {
				dt->next_ep = T->next_ep;
		   		T->next_ep = NULL;
  				return(1);
 			}
			dt = dt->next_ep;
  		}
	}
	return(1); 
}


Event_ * scheduler::dequeue_timer() {
	Event_    *dt;

	if (timerHead_ == NULL )
		return(NULL);
	else {
		dt = timerHead_;
		timerHead_ = dt->next_ep;
		dt->next_ep = NULL;
		return(dt);
	}
}

int scheduler::triggerKernelCallouts()
{
	int retval;

	// Trigger possible callout requests in the kernel
	retval = syscall_NCTUNS_callout_chk();
        if (retval < 0) {
                perror("syscall_NCTUNS_callout_chk");
                exit(1);
        }
	readt0e();
	return 1;
}

int scheduler::readt0e()
{
	// Read t0e and insert the readout event(s) into the event heap

	int cnt;
	struct tun_event te;
	Event_ *ep;
	u_int64_t ticks;

	if (*t0eqlen <= 0)
		return 0;

	while (*t0eqlen > 0) {
		memset(&te, 0, sizeof(struct tun_event));
		cnt = read(t0efd, &te, sizeof(struct tun_event));
		if (cnt != -1) {
			CREATE_EVENT(ep);
			if (te.flag == T0E_TIMEOUT || te.flag == T0E_CHK_CMDIPC || te.flag == T0E_NOTICE_CHILD_PGID) {
				if ((currentTime_[0]/(1000/TICK)) > te.value) {
					printf("T0E_TIMEOUT: currentTime_[0] > (te.value*%d)\n", (1000/TICK));
					printf("CurTime %llu ", currentTime_[0]);
					printf("readt0e(): T0E_TIMEOUT, te.value = %llu\n", te.value);
					exit(0);
				}
				
				MICRO_TO_TICK(ticks, te.value);
				/*
				 * NOTE:XXX
				 * We allow te.value*(1000/TICK) < currentTime_[0].
				 * (When currentTime_[0]%(1000/TICK) != 0, 
				 * this situation may happen.)
				 * This is because the kernel callout timestamp
				 * (te.value) is in the microsecond granularity.
				 */
				if (ticks < currentTime_[0])
					ticks = currentTime_[0];

				/*
				 * ystseng: this special event should be sent
				 * when other task (tacticMANET's agent) to
				 * call system call
				 * syscall_NCTUNS_usleep_After_Msg_Send. then
				 * this system call will create this event into
				 * tun0 and notice SE to read IRC message for
				 * this agent
				 */
				if (te.flag == T0E_CHK_CMDIPC) {
					ep->optval = te.nid;
					ep->flag = CMD_SERVER;
					ep->socket_fd = te.socket_fd;
					ep->pid = te.pid;
					++ticks;
				}
				else if (te.flag == T0E_TIMEOUT) {
					ep->optval = te.value;
					ep->flag = KTIMEOUT;
					ep->priority_ = 0x00 & 0x3f; // event priority level 01, default priority level 02
				}
				else {
					struct tgInfo *ti = (struct tgInfo *)malloc(sizeof(struct tgInfo));

					ticks = te.value;

					memset(ti, 0, sizeof(tgInfo));
					ti->todo = KILL_PROCESS;
					ti->pid = te.pid;

					ep->func_ = trafficGen_Event;
					ep->DataInfo_ = (void *)ti;

					// add the pgid into g_tglist
					if (find(g_tglist.begin(), g_tglist.end(), te.pid) != g_tglist.end()) {
						// redundant pgid
						// This should be a fatal error.
						printf("warnin: there is already a pgid %d in the pgid-list\n", te.pid);
					}
					else
						g_tglist.insert(g_tglist.end(), te.pid);
				}
			}
			else if (te.flag == T0E_CHKTUN) {
				ticks = currentTime_[0];
				ep->optval = (int)te.value;
				ep->flag = CHKTUN;
			}
			else {
				printf("t0eqlen = %d, cnt = %d\n", *t0eqlen, cnt);
				//return 1;
				assert(0);
			}

			SET_EVENT_TIMESTAMP(ep, ticks, 0);

			if (setEvent(ep) < 0) {
				// fail to insert the event
				printf("fail to insert the event ...\n");
				FREE_EVENT(ep);
				if (te.flag == T0E_TIMEOUT) {
					printf("te.flag == T0E_TIMEOUT\n");
					exit(0);
				}
			}

		}	// if (cnt != -1)
	}
	return 1;
}

int scheduler::deqheap()
{
	Event_ *ep;	
	u_int64_t deq_time;

	deq_time = heap_.eventHeap_[1]->timeStamp_;
	ep = heap_.dequeueEvent(currentTime_[0]);
		
	if (ep->timeStamp_ < GetCurrentTime()) {
		printf("We have missed this event; i.e., it is too late.\n");
		printf("deqheap(): ep->timeStamp = %llu, GetCurrentTime = %llu, ep->optval = %u\n", 
		  ep->timeStamp_, GetCurrentTime(), ep->optval);
		exit(0);
	}
	currentTime_[0] = ep->timeStamp_;

	if (ep->calloutObj_ != NULL) {
		nid = ep->calloutObj_->get_nid();
		// Important! nid will be needed and used in executeEvent()
	}

	if (ep->flag == KTIMEOUT) {
		triggerKernelCallouts();
		FREE_EVENT(ep);
		return 0;
	} else if (ep->flag == CHKTUN) {
		chktunnel(ep->optval);
		FREE_EVENT(ep);
		return 0;
	} else if (ep->flag == CMD_SERVER) {
		cmd_server_->checkIPCmessagesNodeID(ep->optval, ep->socket_fd, ep->pid);
		FREE_EVENT(ep);
		return 0;
	}
	if ( ep->calloutObj_ )
		(ep->calloutObj_->*(ep->memfun_))(ep);  
	else if ( ep->func_ )
		ep->func_(ep);
	else
		FREE_EVENT(ep); 
	return 0;
}

int scheduler::deqtimer()
{
	Event_ *ep;

	ep = dequeue_timer();
	if (ep->timeStamp_ < GetCurrentTime()) {
		printf("We have missed this timer event; i.e., it is too late.\n");
		printf("deqtimer(): ep->timeStamp = %llu, GetCurrentTime = %llu\n", ep->timeStamp_, GetCurrentTime());
		exit(0);
	}
	currentTime_[0] = ep->timeStamp_;

	if (ep->calloutObj_ != NULL) {
		nid = ep->calloutObj_->get_nid();
		// Important! nid will be needed and used in executeEvent()
	}

	if (ep->flag == KTIMEOUT) {
 		triggerKernelCallouts(); 
		FREE_EVENT(ep);
		return 0;
	} else if (ep->flag == CHKTUN) {
		chktunnel(ep->optval);
		FREE_EVENT(ep);
		return 0;
	} else if (ep->flag == CMD_SERVER) {
		cmd_server_->checkIPCmessagesNodeID(ep->optval, ep->socket_fd, ep->pid);
		FREE_EVENT(ep);
		return 0;
	}
 	if( ep->perio_ > 0 ) {
 		ep->timeStamp_ += ep->perio_;
 		scheduler_->schedule_timer(ep);
 	}
 	else {
 		((timerObj *)ep->DataInfo_)->init();
 	}
	if ( ep->calloutObj_ )
 		(ep->calloutObj_->*(ep->memfun_))(0);
 	else if ( ep->func_ )
 		ep->func_(0);
 	else {
 		// Here we cannot call FREE_EVENT(), because the 
		// timer must be freed by timerObj Object.
 	}
	return 0;
}

int scheduler::chktunnel(const int tunid)
{
	// Check whether the tunnel has packets to send.

	Event_ *ep; 

	if ((tunid <= 0) || (tunid > (int)ifcnt_)) {
		printf("chktunnel: tunid %d doesn't exist\n", tunid);
		for (unsigned int i = 0; i < ifcnt_; i++) {
			if (tunIFqlen[i] > 0) {
				printf("chktunnel: but tunnel %d has pkts\n", i);
			}
		}
		return 0;
	}

	if (tunIFqlen[tunid]) {
		// If the corresponding tunnel is not specified, we just ignore
	 	// it, no matter whether it has packet to sent.
		if (!iftun_[tunid].obj || !iftun_[tunid].meth_)
			return 0;

		CREATE_EVENT(ep);
		SET_OBJ_EVENT(this, ep, currentTime_[0], 0, iftun_[tunid].obj, iftun_[tunid].meth_, NULL); 
		return 1;
	}
	return 0;
}

u_int64_t scheduler::maxsimtime()
{
    return simulateTime_;
}

void scheduler::chkt0e()
{
        readt0e();  
}

