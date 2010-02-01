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

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include "tactic_api.h"
#include "sock_skel.h"

#define WsmMaxLength		1400 
#define	AppSendMesgInterval	1000000	//MircoSec 
//#define CMD_SERVER_TRIGER	100000	

int             mynid, myTCPsockfd, socketfd2;
int		packet_send_period;
pid_t           pid;
char 		*AppWsmData;
int		WSM_ahead_recv;


struct WaveShortMessageProtocol my_wsmp, recv_wsmp;
struct WaveAgentEnableRegInfo wave_enable_reg_info;
struct GetSETime get_se_time;
sigset_t	usr1_mask;

void sig_usr1(int signo);
int sendWSM(int myTCPsockfd, struct WaveShortMessageProtocol send_wsmp, char *wsm_data);
char* recvWSM(int myTCPsockfd, struct WaveShortMessageProtocol recvWSMPheader);
int enableReginfo(int myTCPsockfd, struct WaveAgentEnableRegInfo wave_enable_reg_info);
u_int64_t getCurrentTime(int myTCPsockfd, int moreMsgFollowing);
void init();

