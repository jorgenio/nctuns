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

#ifndef __NS_IPC
#define __NS_IPC

#include <sys/types.h>

#define DomainSockPath	"/tmp/nctuns"

/* These 3 functions are for run-time messages */
#ifndef RUNTIMEMSG
	#define RUNTIMEMSG
	#define RTMSG_INFORMATION             0x01
	#define RTMSG_WARNING                 0x02
	#define RTMSG_FATAL_ERROR             0x03
#endif

struct runtimeMsgInfo {
	u_int32_t type;
	double time;
	int nodeID;
	char module[20];
	char message[128];
};

int InitSock(void);
int StartTrafficGenerator(char *);
int StopTrafficGenerator(char *);
int SimulationDown(void);
int sendtoGUI(const char *, int); //sendtoCO
int recvfromGUI(char *&); //recvfromCO
void sendTime(u_int64_t);//new
void closeSock(void);
int  errexit(const char *, ...);
int sendWarningtoGUI(struct runtimeMsgInfo);

#endif

//extern char *pname(unsigned char);

/*
void   fix_sock(int, char *);
int readn(int, char *, int);
int writen(int, const char *, int);
int readline(int, char *, int);
extern int msock;
*/
/*
#define READ(fd, p, s)                                      \
{                                                           \
    int             recv, cc;                               \
                                                            \
    recv = 0;                                               \
    while(recv < s) {                                       \
        if((cc = read(fd, p+recv, s-recv))>0)               \
            recv+=cc;                                       \
    }                                                       \
}
#define WRITEP(fd, pkt, s)                                  \
{                                                           \
    int             sent, cc;                               \
    char *spkt;                                             \
                                                            \
    sent = 0;                                               \
    spkt = (char *)&pkt;                                    \
    while(sent < s) {                                       \
        if((cc = write(fd, spkt+sent, s - sent))>0)         \
            sent+=cc;                                       \
    }                                                       \
}
*/

