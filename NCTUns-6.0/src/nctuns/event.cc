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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <event.h>
#include <scheduler.h>
#include <dispatcher.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>
#include <dispatcher.h>
#include <command_server.h>
#include <gbind.h>
#if IPC
#include <IPC/ns.h>
#endif
#include <list>
#include <algorithm>


#include <errno.h>
#include <sched.h>
#include <agent.h>

using namespace std;

extern int errno;
extern scheduler *scheduler_;
extern Dispatcher *dispatcher_;
extern u_int64_t *currentTime_;
extern Dispatcher *dispatcher_;
extern list<int> g_tglist;
extern struct timeval g_starttv;
extern cmd_server * cmd_server_;
int command_num = 0;

#define MAX_ARGC 10	// max arg count
#define MAX_ARGL 20	// max arg length
#define BUF_SIZE 50

list<tcpdumpInfo> g_tdInfolist;	// tcpdumpInfo list

/*----------------------------------------------------------------------*
 * Periodically print out current virtual time				*
 * 	- showCurrentTime() is the handler				*
 *	- showCurrentTime_Event() will create this type of event into	*
 *	  event scheduler. 						*
 * 	 time: indicate how long this event will be called,		*
 *       the time unit is "ms"						*
 *----------------------------------------------------------------------*/
// obsolete
int showCurrentTime(Event_ *ep)
{
	u_int64_t		time;

	SEC_TO_TICK(time, 1);

	printf("Current Time: %7.2f sec   Event#: <Insert:%llu, Dequeue:%llu, Rest:%d>\n",
			currentTime_[0]/(float)time,
			scheduler_->numInsertEvent(),
			scheduler_->numDequeueEvent(),
			scheduler_->numEvent());

	SET_EVENT_REUSE(scheduler_, ep);
	return(1);
}

// obsolete
int showCurrentTime_Event(u_int32_t time)
{
	Event_			*ep;
	u_int64_t		perodical;

	MILLI_TO_TICK(perodical, time);

	/* create a new event and setting its value */
	CREATE_EVENT(ep);
	SET_FUNC_EVENT(scheduler_,
			ep,
			currentTime_[0],
			perodical,
			showCurrentTime,
			NULL);
	return(1);
}

void inttostr(char *tp, int integer, int *size)
{
	if(integer > 9999){
		tp[0] = integer / 10000 + 48;
		tp[1] = (integer % 10000) / 1000 + 48;
		tp[2] = (integer % 10000 % 1000) / 100 + 48;
		tp[3] = (integer % 10000 % 1000 % 100) /10 + 48;
		tp[4] = integer % 10000 % 1000 % 100 % 10 + 48;
		tp[5] = '\0';
		*size = 5;
	} else if(integer > 999) {
		tp[0] = integer / 1000 + 48;
		tp[1] = (integer % 1000) / 100 + 48;
		tp[2] = (integer % 1000 % 100) / 10 + 48;
		tp[3] = (integer % 1000 % 100 % 10) /1 + 48;
		tp[4] = '\0';
		*size = 4;
	} else if (integer > 99) {
		tp[0] = integer / 100 + 48;
		tp[1] = (integer % 100) / 10 + 48;
		tp[2] = (integer % 100 % 10) / 1 + 48;
		tp[3] = '\0';
		*size = 3;
	} else if (integer > 9){
		tp[0] = integer / 10 + 48;
		tp[1] = (integer % 10) / 1 + 48;
		tp[2] = '\0';
		*size = 2;
	} else{
		tp[0] = integer + 48;
		tp[1] = '\0';
		*size = 1;
	}
}

int convert_if(int nid, char **command)
{
	//
	// If it is a tcpdump command, convert the interfaces name from "fxp"
	// to "tun".
	// Return fxp number.
	//

	char *loc;
	int  arg, i, t, tunnumsize, portnumber, *tuns = NULL;
	char fxpnum[6], tunnum[6];
	int retval;


	if (!strstr(command[0], "tcpdump") && !strstr(command[0], "ettercap")) {
		return 0;
	} else {
		// it is a tcpdump command
#ifdef LINUX
		syscall_NCTUNS_misc(syscall_NSC_misc_NIDNUM, nid, 0, (unsigned long)&portnumber);
#else
		syscall(290, syscall_NSC_misc_NIDNUM, nid, 0, &portnumber);
#endif /* LINUX */

		tuns = (int *)malloc(sizeof(int) * portnumber);

#ifdef LINUX
		syscall_NCTUNS_misc(syscall_NSC_misc_NIDTOTID, nid, portnumber, (unsigned long)tuns);
#else
		syscall(290, syscall_NSC_misc_NIDTOTID, nid, portnumber, tuns);
#endif /* LINUX */

		for (arg = 1; command[arg] != NULL; arg++) {
			//if (NULL == (loc = strstr(command[arg],"fxp")))
			if (NULL == (loc = strstr(command[arg],"eth")))
				continue;
			else {
				loc[0] = 't';
				loc[1] = 'u';
				loc[2] = 'n';

				i = 0;
				//while ((loc[i+3] != ' ') && (loc[i+3] != 13) && (loc[i+3] != NULL)) {
				while ((loc[i+3] != ' ') && (loc[i+3] != 13) && (loc[i+3] != 0)) {
					fxpnum[i] = loc[i+3];
					i++;
				}
				fxpnum[i] = '\0';
				t = atoi(fxpnum);
				retval = t;	// save fxp number
				if (t == 0 || t > portnumber) {
					loc[0] = 'e';
					loc[1] = 't';
					loc[2] = 'h';
					break;
				} else {
					inttostr(tunnum, tuns[t-1], &tunnumsize);
					//fprintf(stderr , "tun_num:%d size:%d\n" , atoi(tunnum) , tunnumsize);
					for (t = 0; t < tunnumsize; t++)
						loc[t + 3] = tunnum[t];
					loc[t+3] = '\0';
				}
			}
		}
		
		free(tuns);
	}

	return retval;
}

void sigchld(int sig)
{
	int statloc;
	pid_t pid;
	list<tcpdumpInfo>::iterator itr, itrEnd;
	char dis_buf[BUF_SIZE];

	if (SIGCHLD != sig)
		return ;

	// get the pid of the terminated child
	pid = wait(&statloc);

	// turn off the tcpdump-module's flag if pid found in g_tdInfolist
	itrEnd = g_tdInfolist.end();
	for (itr = g_tdInfolist.begin(); itr != itrEnd; itr++) {
		if (pid == ((tcpdumpInfo)*itr).pid) {
			// turn off the tcpdump-module's flag
			sprintf(dis_buf, "From TCSH : Tcpdump %d %d TCPDUMP DumpFlag off\n",
			  ((tcpdumpInfo)*itr).nid, ((tcpdumpInfo)*itr).portid);
			dispatcher_->dispatching(dis_buf);
			break;
		}
	}

}

//
// Create/kill the traffic generator processes.
//
int trafficGen_Event(Event *ep)
{
	struct tgInfo *ti, *ti2;
	Event *ep2;
	int nid, i, child, portid;
	char *pch, *argv[MAX_ARGC], dis_buf[BUF_SIZE];
	char *toolpath = getenv("NCTUNS_TOOLS");
	struct tcpdumpInfo tdInfo;

#ifdef LINUX
	struct sched_param sched_p;
#endif

	ti = (struct tgInfo *)ep->DataInfo_;
	if (ti->todo & FORK_PROCESS) {
		printf("current ticks= %llu, run \"%s\"\n", GetCurrentTime(), ti->cmd);

		// get node id
		pch = strtok(ti->cmd, " ");
		nid = atoi(pch);

		// get arg
		memset(argv, 0, sizeof(char *) * MAX_ARGC);
		for (i = 0; i < MAX_ARGC; i++) {
			pch = strtok(NULL, " ");
			if (pch == NULL) {
				// no more token
				break;
			}
			if (i == 0) {
				// allocate more memeroy to store toolpath
				argv[i] = (char *)malloc(sizeof(char) * (strlen(pch) + strlen(toolpath) + 4));
			} else {
				argv[i] = (char *)malloc(sizeof(char) * (strlen(pch) + 1));
			}
			if (argv[i] == NULL) {
				perror("malloc");
				break;
			}

			if (i == 0) {
				sprintf(argv[0], "%s/%s", toolpath, pch);
			} else {
				strcpy(argv[i], pch);
			}
		}
		argv[i] = NULL; // don't forget this

		portid = convert_if(nid, argv);	// convert interface names for tcpdump command

#ifdef LINUX
		setpriority(PRIO_PROCESS, 0, -15);
#else
		setpriority(PRIO_PROCESS, 0, -5);
#endif
		//////////////////////////// for gdb ///////////////////////////////////////
#if IPC
		if(strcmp(GdbStart , "on") == 0) {
		    	dispatcher_->GdbMessage(nid , "before");
			kill(getpid() , SIGSTOP);
		}
#else
		if(strcmp(GdbStart , "on") == 0) {
			char answer = '\0';
			printf("\e[1;33mThe simualtion engine is going to fork a traffic generator process on node(%d).\e[m\n" , nid);
			printf("\e[1;32mPlease close the gdb program or detach it from the simulation engine process at this point of time.\e[m\n");
			printf("\e[1;32mPress any key to continue the execution of the simulation ...\e[m\n");
		    	scanf("%c" , &answer);
		}
#endif
		///////////////////////////////////////////////////////////////////////////
		// create the process
		if ((child = vfork())) {
			/* parent process */
#ifndef LINUX
			setpriority(PRIO_PROCESS, 0, 20);
#endif

#ifdef LINUX
			/*
			 * Set traffic generator as Real-time process
			 * with SCHED_RR policy while as simulation engine
			 * is a conventional process.
			 */
			setpriority(PRIO_PROCESS, 0, -5);
			sched_p.sched_priority = 1;
			sched_setscheduler(child, SCHED_FIFO, &sched_p);
#else
			// register this process
			syscall(290, 4, child, nid, 0);
			syscall(286, child);
#endif

			// create an event to kill this process
			ti2 = (struct tgInfo *)malloc(sizeof(tgInfo));
			assert(ti2);
			memset(ti2, 0, sizeof(tgInfo));
			ti2->todo = KILL_PROCESS;
			ti2->pid = child;
			ep2 = createEvent();
			setFuncEvent(ep2, ti->endtime, 0, trafficGen_Event, (void *)ti2);

			// add the pid into g_tglist
			if (find(g_tglist.begin(), g_tglist.end(), child) != g_tglist.end()) {
				// redundant pid
				// This should be a fatal error.
				printf("warnin: there is already a pid %d in the pid-list\n", child);
			}
			else
				g_tglist.insert(g_tglist.end(), child);

			if (strstr(argv[0], "tcpdump") || strstr(argv[0], "ettercap")) {
				// turn on the tcpdump-module's flag
				sprintf(dis_buf, "From TCSH : Tcpdump %d %d TCPDUMP DumpFlag on\n", nid, portid);
				dispatcher_->dispatching(dis_buf);

				// store tcpdump infomation to turn off the tcpdump-module's flag
				tdInfo.pid = child;
				tdInfo.nid = nid;
				tdInfo.portid = portid;
				g_tdInfolist.insert(g_tdInfolist.end(), tdInfo);

				// if this is the first tcpdump executed, register a signal handler for SIGCHLD
				if (1 == g_tdInfolist.size()) {
					signal(SIGCHLD, sigchld);
				}
			}

			//////////////////////////// for gdb ///////////////////////////////////////
#if IPC
			if(strcmp(GdbStart , "on") == 0) {
				dispatcher_->GdbMessage(nid , "after");
				kill(getpid() , SIGSTOP);
			}
#else
			if(strcmp(GdbStart , "on") == 0) {
				char answer = '\0';
				printf("\e[1;33mThe simualtion engine has forked a traffic generator process on node(%d).\e[m\n" , nid);
				printf("\e[1;32mOne can safely attach the gdb program to the simulation engine process at this point of time.\e[m\n");
				printf("\e[1;32mPress any key to continue the execution of the simulation ...\e[m\n");
				scanf("%c" , &answer);
			}
#endif
			///////////////////////////////////////////////////////////////////////////

		} else {
			/* child process */
			/* close all file description */
			fcloseall();
			setpgid(getpid(), getpid());

			// register nodeID of this process
			syscall_NCTUNS_misc(syscall_NSC_misc_REGPID, getpid(), nid, 0);

			// fill the end time of this process into kernel
			syscall_NCTUNS_misc(syscall_NSC_misc_SET_ENDTIME, getpid(), ti->endtime, 0);

			setpriority(PRIO_PROCESS, 0, -10);
			chdir(getenv("NCTUNS_WORKDIR"));
			if (execv(argv[0], argv)) {
				perror("execv");
				_exit(1);
			}
		}

		//
		// Noted by Prof. S.Y. Wang on 1/30/2003
		// The above A and B microseconds must follow the
		// condition that B > A. The following explains how
		// the values of A and B are chosen.
		//
		// The reason why putting a sleep(A) is that we want the
		// parent to be able to call syscall(290) and syscall(286)
		// before the child process calls execv(). This can ensure
		// that before any timer used by the child is set up in
		// the simulated network, the child process has been registered
		// as a traffic generator used in the simulated network and
		// thus its timers will be triggered by the virtual clock.
		//
		// The reason why putting a sleep(B) is that we want the
		// parent process to sleep a while to wait for the child
		// process to become ready to generate its traffic (packets).
		// We must do so otherwise the SE may advance its virtual
		// clock too fast because it does not see any packets
		// to be injected into the simulated network. Actually,
		// this problem may be due to the fact that the child process
		// so far has not got a chance to run.
		//
		// The reason why B must > A is that if A > B  then
		// we surely will encounter the problem discussed above.
		//
		// Right now, A is set to 0.1 second and B is set to
		// 1 second. So far these values work quite well.
		// Experimental results show that if B is less than 1
		// second, simulation results may not be repeatable in
		// some cases where a traffic generator process needs
		// a lot of time before generating its first packet.
		//

		// free memory
		for (i = 0; i < MAX_ARGC; i++) {
			free(argv[i]);
		}
	} else if (ti->todo & KILL_PROCESS) {
		if (g_tglist.end() != find(g_tglist.begin(), g_tglist.end(), ti->pid)) {
			// this child process is not terminated automatically yet
			unsigned long nid;
			printf("current ticks= %llu, kill process of pid %d\n", GetCurrentTime(), ti->pid);
			//solve the car pass through the wall
			syscall_NCTUNS_misc(syscall_NSC_misc_PIDTONID , ti->pid , 0 , (unsigned long)&nid);
			GROUPMEMBER_VECTOR* gv = GROUP_FIND(GROUP_ALL)->GetMember();
			GruopMemItor index = gv->begin();
			for(; index != gv->end() ; index++)
			{
			    if((*index)->GetID() == nid)
			    {	
				gv->erase(index);
				break;
			    }
			}    
			/*
			 * kill -pid is meaning kill this group process
			 */
			kill(-(ti->pid), SIGKILL);
			g_tglist.erase(find(g_tglist.begin(), g_tglist.end(), ti->pid));

			/*
			 * clear the fd_set for specified pid for tactic network
			 */
			cmd_server_->closeSpecifiedClientPID(ti->pid);
		}
	} else {
		printf("invalid ti->todo value\n");
		assert(0);
	}

	free(ep->DataInfo_);
	ep->DataInfo_ = 0;
	freeEvent(ep);

	return 0;
};

/*------------------------------------------------------------------------*
 * Read Traffic file to generate traffic event.							  *
 *------------------------------------------------------------------------*/
int read_trafficGen()
{
	FILE *fd;
	int	i;
	char buf[1201], tmp[1001];
 	char *cmd, *ptr;
	u_int32_t nid;
	double st, et;
	u_int64_t exptime;
 	Event *ep;
	struct tgInfo *ti;
	char *FILEPATH;

	// get traffic file and open it
	FILEPATH = (char *)malloc(strlen(GetScriptName()) + 5);
	sprintf(FILEPATH, "%s.tfc", GetScriptName());

	if ((fd = fopen(FILEPATH, "r")) == NULL) {
		printf("Warning: can't open file %s\n", FILEPATH);
  		return(-1);
	}
	free(FILEPATH);

	for (fgets(buf, 1000, fd); !feof(fd); fgets(buf, 1000, fd)) {
		// remove "\n"
		if ((ptr = strchr(buf, '\n')))
			*ptr = '\0';
		
		if (buf[0] == '\0' || buf[0] == '#')
  			continue;

 		i = sscanf(buf, "$node_(%d) %lf %lf %1000s\n", &nid, &st, &et, tmp);
		if (i != 4) {
			printf("fail to read one traffic-generator record\n");
			printf("\tskip...\n");
			continue;
		}

		/* get command */
		cmd = strstr(buf, tmp);

		/*
		 * Create an event to notify coordinator
		 * to fork a traffic generator.
		 */
		ti = (struct tgInfo *)malloc(sizeof(struct tgInfo));
		assert(ti);
		memset(ti, 0, sizeof(tgInfo));
		ti->todo = FORK_PROCESS;
		sprintf(ti->cmd, "%d %s", nid, cmd);

		ep = createEvent();
		SEC_TO_TICK(exptime, st);
		SEC_TO_TICK(ti->endtime, et);

  		setFuncEvent(ep,
			     exptime,
			     0,
			     trafficGen_Event,
			     (void *)ti
		);
	}
	fclose(fd);

	return 0;
}

/*-----------------------------------------------------------------------*
 * Communicate with Coordinator                                          *
 *-----------------------------------------------------------------------*/
int SyncWithGUI(Event *ep)
{
	/*
	 * When this function is called, we should
	 * send the current time to GUI to advance
	 * GUI's counter.
 	 */
#if IPC
	sendTime(GetCurrentTime());
#endif
	setEventReuse(ep);

	return 0;
}

int SyncWithGUI_Event(double time)
{
	u_int64_t timeInTick;
	Event *ep;

	MILLI_TO_TICK(timeInTick, time);
	ep = createEvent();
	setFuncEvent(ep, timeInTick, timeInTick, SyncWithGUI, 0);
	return(1);
}

/*-------------------------------------------------------------------------*
 * Receive Data from GUI                                                   *
 *-------------------------------------------------------------------------*/
int checkIPC(Event *ep)
{
	/*
	 * When this function is called, we
	 * try to read command from GUI.
	 */
#if IPC
	char *buf;

	if (recvfromGUI(buf) > 0) {
		dispatcher_->dispatching(buf);
		free(buf);
	}
#endif
	setEventReuse(ep);

	return 0;
}

int checkIPC_Event(double time)
{
	u_int64_t timeInTick;
	Event *ep;

	MILLI_TO_TICK(timeInTick, time);
	ep = createEvent();
	setFuncEvent(ep, timeInTick, timeInTick, checkIPC, 0);
	return(1);
}

/*-----------------------------------------------------------------------*
 * Emulation purpose. We purposely slow down the virtual clock speed so  *
 * that its speed is the same as that of the real-world clock.           *
 *-----------------------------------------------------------------------*/
int SyncWithRealtime(Event *ep)
{
	struct timeval tv;
	u_int64_t ran;
	double sec;

	// get the real time
	gettimeofday(&tv, NULL);

	// get the real elapsed time
	timersub(&tv, &g_starttv, &tv);
	TV_TO_TICK(ran, tv);

	if (GetCurrentTime() > ran) {
		TICK_TO_SEC(sec, (GetCurrentTime() - ran));
		usleep((unsigned int)(sec * 1000000));
	}

	setEventReuse(ep);

	return 0;
}

int SyncWithRealtime_Event(double time)
{
	u_int64_t timeInTick;
	Event *ep;

	MILLI_TO_TICK(timeInTick, time);
	ep = createEvent();
	setFuncEvent(ep, timeInTick, timeInTick, SyncWithRealtime, 0);

	return(1);
}

