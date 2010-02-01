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

#include <sys/time.h>
#include <sys/types.h>

#include <tcl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <nodetype.h>
#include <scheduler.h>
#include <maptable.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>
#include <nctuns_divert.h>
#include <sysrt.h>
#include <dispatcher.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <gbind.h>
#include <command_server.h>

#include <sys/resource.h>

#if IPC
#include <IPC/ns.h>
#endif
#include <list>

#include <commun_gui.h>
using namespace std;

list<int> g_tglist;	// list of pids of traffic-generator processes

extern Dispatcher *dispatcher_;
extern scheduler *scheduler_; 
extern typeTable *typeTable_; 
extern sysrt *sysrt_;
extern commun_gui * commun_gui_;
extern logpack  * logpack_;
extern cmd_server *cmd_server_;

u_int64_t simTime_; 		// simulation time 
struct timeval g_starttv;	// the wallclock time when the simulation starts 

/*
 * Reset some kernel variables and clear some data structures left
 * from the previous simulation.
 * 
 */ 
#ifdef LINUX
void resetKernelStates(int nctuns_pid) {
	if (syscall_NCTUNS_clearStateAndReinitialize(nctuns_pid) < 0 ||
		syscall_NCTUNS_misc(syscall_NSC_misc_TICKTONANO, TICK, 0, 0) < 0) {

		ErrorMesg("Can't reset states in kernel\n"); 
	} else 	printf("Reset states in kernel succeeded\n");
}
#else
void resetKernelStates() {
	if (syscall(281) < 0 || syscall(290, 9, TICK, 0, 0) < 0) {
		ErrorMesg("Can't reset states in kernel\n"); 
	} else 	printf("Reset states in kernel succeeded\n"); 
}
#endif

/*
 * Add By Yu-Ming Huang
 * This function is used to shutdown the emulation module.
 */
void Shutdown_Emulation_Module()
{
	const char *kemud_File = "/proc/nctuns_emud";
	char cmd[40];

	// If the module were not set up, we can ignore this step.
	if(access(kemud_File, F_OK) < 0)
		return;

	// Shutdown the emulation module
	sprintf(cmd, "rmmod %s", kemud_File);
	system(cmd);
}

/*
 * Perform cleaning when exit.
 */
void NCTUns_exit()
{
	// kill all traffic-generator processes
	for (list<int>::iterator itr = g_tglist.begin(); itr != g_tglist.end(); itr++) {
		printf("kill the traffic generator process (pid %d)\n", (int)(*itr));
		kill(-(int)(*itr), SIGKILL);
	}

	/* Flush divert rules of emulation */
	syscall_NCTUNS_divert(syscall_NSC_divert_FLUSH, 0, 0, 0, 0);
	syscall_NCTUNS_mapTable(syscall_NSC_mt_FLUSH, 0, 0, 0, 0);

	/* Clear all States in kernel when simulation done */
	resetKernelStates(0);
	Shutdown_Emulation_Module();
	printf("Simulaton Engine exits.\n");
}

void do_stop(int n)
{
	dispatcher_->Pause(NULL);
}

// Without this correct design, the simulation engine will advance its
// virtual clock very fast during the short period between when it is waked up
// and when it wakes up traffic generators. Prof. Wang, 01/11/2005
void do_cont(int n)
{
	dispatcher_->Continue(NULL);
}

/*
 * for user INT signal
 */
void do_int(int n)
{
	exit(1);
}

/*
 * Add by Yu-Ming Huang
 */
void Start_Emulation_Module()
{
	const char* kemud_File="/proc/nctuns_emud";
	char conf_File[200];
	FILE* kemud;
	FILE* conf;
	char in[200];

	sprintf(conf_File, "%s.emu%c", GetScriptName(),0);

	// Check if there has any external node
	if(!(conf = fopen(conf_File, "r")))
	{
		// there has no external nodes
		return;
	}

	if(feof(conf))
		// there has no external nodes
		return;

	// Check if the emulation daemon is start?
	if(!(kemud = fopen(kemud_File, "wba")))
	{
		// invoke
		if(system("insmod /lib/modules/`uname -r`/kernel/kernel/nctuns/nctuns_emud.ko"))
		{
			fprintf(stderr, "Emulation Daemon cannot be invoke...\n");
			return;
		}
		
		if(!(kemud = fopen(kemud_File, "wba")))
		{
			fprintf(stderr, "Emulation Daemon cannot be used...\n");
			return;
		}
	}

	fprintf(kemud,"clear\n");
	fclose(kemud);
	kemud = fopen(kemud_File, "wba");

	while(fgets(in, 200, conf))
	{
		if(in[0] == '#')
			continue;
		fprintf(kemud, "%s\n",in);
		printf("%s",in);
	}

	fclose(conf);
	fclose(kemud);
}


int Start_Simulation(int argc, char *argv[])
{
	u_int32_t time;

	assert(scheduler_); 
	assert(typeTable_); 

	/* check environment variables */
	if (getenv("NCTUNS_BIN") == NULL) {
		printf("\nError: can not find environment variable NCTUNS_BIN\n");
		exit(-1);
	}
	if (getenv("NCTUNS_WORKDIR") == NULL) {
		printf("\nError: can not find environment variable NCTUNS_WORKDIR\n");
		exit(-1);
	}
	if (getenv("NCTUNS_TOOLS") == NULL) {
		printf("\nError: can not find environment variable NCTUNS_TOOLS\n");
		exit(-1);
	}          

        // The following statement is to catch the SIGCONT signal, which 
        // is issued when the coordinator resumes the execution of 
        // the paused simulation engine. Prof. Wang, 01/11/2005
	signal(SIGSTOP, do_stop);
	signal(SIGCONT, do_cont);
	signal(SIGINT, do_int);
 
	if ( argc != 2 ) {
		printf("  Usage: run time\n");
		return(-1); 
	}
	time = atoi(argv[1]);  
	SEC_TO_TICK(simTime_, time);

	/* Flush divert rules of emulation and clear all state in kernel */
        // The following statement must be performed before umtbl_configtun().
	resetKernelStates(getpid());

	// configure tunnel interfaces, set inet ip set mac address
	umtbl_configtun();
	umtbl_cpytokern(); 

	//mtbl_display();

	// init dispatcher
	dispatcher_->init();

	// setting kernel routing table
	assert((sysrt_ = new sysrt));

	// Open t0e0: t0e0 device is used to pass timeout events from 
	// the kernel to the user-level simulation engine.
	// (Note that tun0 is reserved and used for tcpdump purpose and
	// t0e0 is therefore created for passing timeout events 
	// from the kernel to the SE.)

#if 0
	fd = tun_alloc("t0e0");
	
	// Close t0e0. We do the open and close operations on the t0e0 
	// device to release all event packets that are left from the 
	// previous simulation and currently residing in the device's
	// output queue. Without doing this, the results of the new simulation
	// may be affected by the previous one.

	if (close(fd) == -1) {
		perror("failed to close t0e0");
		exit(1);
	}
#endif

#ifndef LINUX
	// In the following, we open and immediately close a tunnel
	// device to release all packets that are left from the 
	// previous simulation and currently residing in the device's
	// output queue. Without doing this, the results of the new simulation
	// may be affected by the previous one.

	// Right now we assume that our simulation system has at least
	// 4096 tunnel devices created. As such, we blindly open and
	// close these devices. It does no harm to open a tunnel device
	// that does not exist. The purpose for doing this is the same as
	// above. We need to release all packets that are left from the 
	// previous simulation and currently residing in the device's
	// output queue. Without doing this, the results of the new simulation
	// may be affected by the previous one.

	char path[20];

	for (i=0; i<4096; i++) {
		sprintf(path, "/dev/tun%d", i);
		fd = open(path, O_RDWR);
		if (fd > 0) {
			if (close(fd) < 0) {
				printf("Close tun%d failed\n", i);
			}
		}
	}
#endif /* LINUX */

	showCurrentTime_Event(1000);	/* per 1000 ms */ 

	// create events to generate traffic generator processes at specified times

	Start_Emulation_Module();
	read_trafficGen(); 

	// Now we set the priority of the SE to a value lower than
	// those of forked traffic generator processes. The way we do it is 
	// that we apply nice(20) to the SE while applying nice(-5) to
	// all forked traffic generator processes.
	// This operation is very important. Without doing it, the
	// generated simulation results may not be correct.

        setpriority(PRIO_PROCESS, 0, 20);

#if IPC
	/* open an IPC to communicate with coordinateor */
	if (!InitSock()) {
		printf("nctuns: can not connect to coordinator!\n");
  		exit(-1);
	}

	/* sync with GUI */
	SyncWithGUI_Event(1000);		/* 1000 ms */

  	checkIPC_Event(100); 
#endif

	// register a functon to be called at program exit
	atexit(NCTUns_exit);

#if IPC
	// run-time sending ptr and/or node location information to GUI
	logpack_  = new logpack();
        commun_gui_->initialize();
#endif
	// initialize command server for automatic vehicle
	cmd_server_->ReadSignalFile();

	// get the wallclock time before starting simulation
	gettimeofday(&g_starttv, NULL);

	/////////////////////////// for gdb ///////////////////////////////////////
	if(GdbStart == NULL) {
		GdbStart = new char[4];
		strcpy(GdbStart , "off");
	}
#if IPC
	if(strcmp(GdbStart , "on") == 0) {
		dispatcher_->GdbMessage(0 , "Start_Simulation");
		kill(getpid() , SIGSTOP);
	}
#else
	if(strcmp(GdbStart , "on") == 0) {
	    	char answer;
		printf("\e[1;33mThe simulation is going to start.\e[m\n");
		printf("\e[1;32mOne can attach the gdb program to the simulation engine process at this point of time.\e[m\n");
		printf("\e[1;32mPress any key to continue the execution of the simulation ...\e[m\n");
		scanf("%c" , &answer);
	}
#endif
	//////////////////////////////////////////////////////////////////////////

	/* start event scheduler */
	scheduler_->run(time); 

	/*
	 * When simulator is done, we send a signal
	 * to coordinator and wait for 3 secs for reliability.
	 */
	printf("Simulation is done.\n");
#if IPC
 	SimulationDown();
#endif

	cmd_server_->closeAllClientSocket();

	/*
	 * free all memory
	 */
	delete sysrt_;
	printf("Simulation Engine is going to call exit().\n");	

  	exit(0);
 
	return TCL_OK;
}

