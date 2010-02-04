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

#ifndef __NCTUNS_LOGPACK_H__
#define __NCTUNS_LOGPACK_H__

#include <sys/types.h>
#include <stdio.h>
#include <misc/log/logHeap.h>
#include <mylist.h>
#include <commun_gui.h>
#include <nctuns_api.h>
#include <stdarg.h>

#define MAX_PTR_RECORD_NUM_PER_PACKET  20
#define PTR_RECORD_NUM_ENOUGH_FOR_SEND 20 /* This number should be less than MAX_PTR_RECORD_NUM_PER_PACKET */
#define MAX_LOC_RECORD_NUM_PER_PACKET  200
#define LOC_RECORD_NUM_ENOUGH_FOR_SEND 10  /* This number should be less than MAX_LOC_RECORD_NUM_PER_PACKET */
// add by AD
#define MAX_LIG_RECORD_NUM		100
#define LIG_ENOUGH_FOR_SEND		4

#ifndef LOGOBJECT
#define LOGOBJECT

typedef struct LogObject{
        u_char          PROTO;
        u_char          Event;
	union{
		u_int64_t       Time;
		u_int64_t	sTime;
	};
	u_int32_t	diff;
        union{
		 u_char          FrameType;
       		 u_char          BurstType;
	};
        u_int32_t       IP_Src;
        u_int32_t       IP_Dst;
        u_int32_t       PHY_Src;
        u_int32_t       PHY_Dst;

	u_int32_t       MAC_Dst;
     	union{
		u_int64_t       BurstID;
		u_int64_t       ConnID;
        	u_int64_t       FrameID;
	};
	union{
   	  	u_int32_t       FrameLen;
     	        u_int32_t       BurstLen;
	};
	u_int32_t       RetryCount;
	u_char          DropReason;
	u_char		Channel;
	u_int32_t 		pad;
};

struct PtrPacket {
	char type;
	int num;
	LogObject LogObjectArray[MAX_PTR_RECORD_NUM_PER_PACKET];
};

#endif
struct LocPacket {
	char type;
	int num;
	struct NodeLocation LocArray[MAX_LOC_RECORD_NUM_PER_PACKET];
};

// add by AD
struct LightInfoHeader{
	u_int32_t type;
	int num;
};
struct LightInfo{
	u_int32_t nid;
	int gid;
	int light;
	double x;
	double y;
	double  timeStamp;
};
struct Light {
	char type;
	int num;
	struct LightInfo LightArray[MAX_LIG_RECORD_NUM];
};


enum DebugLevel
{
	TRACE=1,
	WARN=2,
	ERROR=3
};

#define DEBUG_LEVEL TRACE
#define RDV
void logRidvan(DebugLevel,const char* , ...);

class logpack {
private:
	FILE *logfptr;
	FILE *sigptr;
public:
	logpack();
	~logpack();

	int pushtoPtrList();
	int pushtoLocList();
	int popPtrList(int num);
	int popLocList(int num);
	void logNodeLocation(int nodeid, double x, double y, double z, double a_time, double p_time, double speed);
	// add by AD
	int pushtoLigList();
	int popLigList(int num);
	void logLight(int nodeid, int groupid, int light, double x, double y, double a_time);

	int addMac8023Log(u_char, u_char, u_int32_t, struct mac802_3_log *, struct mac802_3_log*, int);
	int addMac80211Log(u_char, u_char, u_int32_t, struct mac802_11_log *, struct mac802_11_log*, int);

#ifdef CONFIG_OPTICAL
	int addOphyLog(u_char, u_char, u_int32_t, struct ophy_log *, struct ophy_log *, int);
#endif	/* CONFIG_OPTICAL */

#ifdef CONFIG_GPRS
	int addGprsLog(u_char, u_char, u_int32_t, struct gprs_log*, int);
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_WIMAX
	int addMac80216Log(u_char proto, u_char event, u_int32_t diff,
		struct  mac802_16_log *mac80216, struct mac802_16_log* _mac80216, int pad);
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
	int addMac80216eLog(u_char proto, u_char event, u_int32_t diff,
		struct  mac802_16e_log *mac80216e, struct mac802_16e_log* _mac80216e, int pad);
#endif	/* CONFIG_MoblieWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
	int addMac80216jLog(u_char proto, u_char event, u_int32_t diff,
		struct  mac802_16j_log *mac80216j, struct mac802_16j_log* _mac80216j, int pad);
#endif	/* CONFIG_MoblieRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
	int addMac80216j_NT_Log(u_char proto, u_char event, u_int32_t diff,
		struct  mac802_16j_NT_log *mac80216j_NT, struct mac802_16j_NT_log* _mac80216j_NT, int pad);
#endif	/* CONFIG_MR_WIMAX_NT */

#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
	int addDvbrcsLog(u_char proto, u_char  event, u_int32_t diff,
		struct  dvbrcs_log *dvbrcs, struct dvbrcs_log *_dvbrcs, int pad);
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */
};

#endif	/* __NCTUNS_LOGPACK_H__ */
