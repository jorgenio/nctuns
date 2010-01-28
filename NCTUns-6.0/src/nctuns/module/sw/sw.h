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

#ifndef __NCTUNS_sw_h__
#define __NCTUNS_sw_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <set>
#include <string>

class Packet;

using namespace std;

#define SW_TBL_MAX_ENTRYS	256


typedef set<int>		nodeSet;
typedef set<int>::iterator      set_iter;

typedef struct mapTbl {
	u_char		mac[6];
	MBinder		*port;
	u_int8_t	portNum;
	u_int64_t	timestamp;
	struct mapTbl	*next;
} mapTbl;

typedef struct swTbl {
	char		*name;
	mapTbl		*head, *tail;
	int		entrys;
} swTbl;

typedef struct tbInfo {
	int		neighbor;
	int		portNum;
	string		mac;
} tbInfo;

typedef struct SpanTreeTbl{
        int RootID;
        int Cost;
        int Bridge;
        int Age;
        u_int8_t RootPort;
}SpanTreeTbl;

struct TopChangeINFO{
              unsigned short Protocol;
              unsigned char  Version;
              unsigned char  Message;
};

typedef	struct SwPort {
        u_int8_t        PortState;
        u_int8_t        SwSignal;
        int 		RootID;
        int 		Cost;
 	int 		Bridge;
	u_int8_t        RootDeath;
        u_int8_t        Age;
 
	u_int8_t	portNum;
	MBinder		*port;
	SwPort		*nextport;
} SwPort;

typedef struct SpanINFO{
                unsigned short    Protocol;
                unsigned char     Version;
                unsigned char     Message;
                unsigned char     TCAReservedTC;
                long long         RootID;
                int               Cost;
                long long         Bridge;
                unsigned short    PortID;
                unsigned short    MessageAge;
                unsigned short    MaxAge;
                unsigned short    HelloTime;
                unsigned short    ForwardDelay;
}SpanINFO;

class sw : public NslObject {

 private: 
	SwPort			*PortList;
	u_int32_t		num_port;
	swTbl			*swTable;
	swTbl			*pmntSwTable; // specially used by AP
	timerObj		flushTimer;
	char			*SWITCH_MODE; //know in advance,user specify,run...
	int			flushInterval; // flush interval(unit:ms)
	u_int64_t		flushInterval_;// flush interval(unit:tick)
        u_int64_t               HelloTimeTick;
        u_int64_t               ForwardDelayTick;    
        u_int64_t               SpanTimeOutTick; 


	char 			*SpanningTreeProtocol;	// on/off
	int			HelloTime ;
	int			MaxAge ;
	int			ForwardDelay ;
	int                     SpanTimeOut;
        char                    STABLE;
        int                     STABLEage; 
        int                     RootDeath; 
	char			*fileName;
   
        /* Spanning Tree implement */
        SpanTreeTbl             SpanTreeTbl0;
        char                    FlushSwTblFlag;
        char                    TCNFlag;
        char                    TCAFlag;
        timerObj                SpanTimer;
        timerObj                TimeOutTimer;
        timerObj                DetectSwTimer;

     public:
 	sw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~sw();   

	int 			init(); 
	int			recv(ePacket_ *pkt);
	int 			command(int argc, const char *argv[]); 
	
	MBinder			*findport(u_char *mac_, swTbl *targetTbl);
	int			findSwTbl(u_char *mac_, MBinder *port_, swTbl *targetTbl);

	int			addSwTbl(u_char *mac_, MBinder *port_,
						swTbl *targetTbl);
	int			delSwTbl(mapTbl *lastEntry, mapTbl *delEntry,
						swTbl *targetTbl);
	int			flushSwTbl();
        int                     getSrcDstMac(u_char *src, u_char *dst,
						ePacket_ *pkt);
	int			dumpSwTable();
	int			parseLine(char *line, char *mac, int *portNum);
        int                     get(ePacket_ *pkt, MBinder *frm);
        int                     send(ePacket_ *pkt, MBinder *frm);

        /* Spanning Tree Implement */
	int                     GetSpanTreeTbl(char *Variable);
	int                     GetSpanTreeChange(Packet *,MBinder *,SwPort *,int);
        int                     GetSpanTreePacket(Packet *_,MBinder *,SwPort *,int);
        int                     SendSpanTreePacket(Event_ *);
        int                     DetectSwitch(Event_ *);
        int                     OpenSpanTreePort(Event_ *);
        int                     CheckTimeOut();
        int                     SpanTreeChange();

}; 
 

#endif /* __NCTUNS_sw_h__ */
