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

#include "logpack.h"
#include <stdarg.h>
#include "time.h"

logpack *logpack_;
extern commun_gui *commun_gui_;
struct PtrPacket *ptrpacket_;
int PtrArray_last;
struct LocPacket *locpacket_;
int LocArray_last;
// add by AD
struct Light *light_;
int LightArray_last;


void logRidvan(DebugLevel LEVEL,const char* text, ...)
{

	if(LEVEL<DEBUG_LEVEL)
		return;
	char text2[1000];
	char *token;
	time_t t;
	FILE *fin = fopen("/home/ridvan/nctuns.log","a");
	va_list marker;
	va_start( marker, text);
	strcpy(text2,text);
	time(&t);
	fprintf(fin,"\n\n_______________________________\n%s",ctime(&t));
	token  = strtok(text2,"%");
	while( token != NULL  )
	{
		char sign = token[0];
		char* tok = token+1;

		switch(sign)
		{
		case 'x':
					fprintf(fin,"%x%s",va_arg( marker, int),strlen(tok)>0?tok:"");
					break;
		case 'd':
			fprintf(fin,"%d%s",va_arg( marker, int),strlen(tok)>0?tok:"");
			break;
		case 'f':
			fprintf(fin,"%f%s",va_arg( marker, double),strlen(tok)>0?tok:"");
			break;
		case 's':
			fprintf(fin,"%s%s",va_arg( marker, char*),strlen(tok)>0?tok:"");
			break;
		default:
			fprintf(fin,"%s",token);
		}
		token  = strtok(NULL,"%");
	}
	fprintf(fin,"\n");
	fflush(fin);
	va_end( marker );
	fclose(fin);
}
logpack::logpack()
{

	ptrpacket_ = (struct PtrPacket *)malloc(sizeof (struct PtrPacket));
	PtrArray_last = -1;
	locpacket_ = (struct LocPacket *)malloc(sizeof (struct LocPacket));
	LocArray_last = -1;

	ptrpacket_->num = 0;
	locpacket_->num = 0;

	// add by AD
	light_ = (struct Light *)malloc(sizeof (struct Light));
	LightArray_last = -1;
	light_->num = 0;
	/*
	 * create a file XXX.nll to log node locations during simulation
	 */
	char *FILEPATH = (char *)malloc(strlen(GetScriptName()) + 5);

	sprintf(FILEPATH, "%s%s", GetScriptName(), ".nll");
	if ((logfptr = fopen(FILEPATH, "w+")) == NULL) {
		printf("Error: can't create the file %s\n", FILEPATH);
		exit(-1);
	}
	sprintf(FILEPATH, "%s%s", GetScriptName(), ".sll");
	if ((sigptr = fopen(FILEPATH, "w+")) == NULL) {
		printf("Error: can't create the file %s\n", FILEPATH);
		exit(-1);
	}
	free(FILEPATH);
}

logpack::~logpack()
{
	free(ptrpacket_);
	free(locpacket_);
	free(light_);
}

int logpack::pushtoPtrList()
{

	ptrpacket_->num++;
	if (ptrpacket_->num == PTR_RECORD_NUM_ENOUGH_FOR_SEND) {
		popPtrList(PTR_RECORD_NUM_ENOUGH_FOR_SEND);
	}
	return (0);
}

int logpack::pushtoLocList()
{

	locpacket_->num++;
	if (locpacket_->num == LOC_RECORD_NUM_ENOUGH_FOR_SEND) {
		popLocList(LOC_RECORD_NUM_ENOUGH_FOR_SEND);
	}
	return (0);
}

int logpack::popPtrList(int num)
{

	if (num > ptrpacket_->num)
		num = ptrpacket_->num;
	if (commun_gui_->sendPtrToGUI(num) != num) {
		printf("[logpack::popPtrList] num %d send ptr error \n", num);
		exit(1);
	}
	return (num);
}

int logpack::popLocList(int num)
{

	if (num > locpacket_->num)
		num = locpacket_->num;
	if (commun_gui_->sendLocToGUI(num) != num) {
		printf("[logpack::popLocList] num %d, send loc error \n", num);
		exit(1);
	}
	return (num);
}

// add by AD
int logpack::pushtoLigList()
{
	light_->num++;
	if (light_->num == LIG_ENOUGH_FOR_SEND) {
		popLigList(LIG_ENOUGH_FOR_SEND);
	}
	return (0);

}

int logpack::popLigList(int num)
{
	if (num > light_->num)
		num = light_->num;
	if (commun_gui_->sendLightToGUI(num) != num) {
		printf("[logpack::popLightList] num %d, send light error \n", num);
		exit(1);
	}
	return (num);

}

void logpack::logLight(int nodeid, int groupid, int light, double x, double y, double a_time)
{
	double ta_time;

	TICK_TO_SEC(ta_time, a_time);

#if IPC
	struct LightInfo *tmp;
	tmp = &light_->LightArray[++LightArray_last];
	tmp->nid = nodeid;
	tmp->gid = groupid;
	tmp->light = light;
	tmp->x = x;
	tmp->y = y;
	tmp->timeStamp = ta_time;

	pushtoLigList();
#endif

	fprintf(sigptr, "$trafficlight_(%d) set %d %lf %lf %lf %d\n",
			nodeid, groupid, x, y, ta_time, light);
	fflush(sigptr);

}

void logpack::logNodeLocation(int nid, double x, double y, double z,
			      double a_time, double p_time, double speed)
{

	double ta_time;

	TICK_TO_SEC(ta_time, a_time);

#if IPC
	struct NodeLocation *packet;
	if (dynamicLocLogFlag && !strcasecmp(dynamicLocLogFlag, "on")) {
		packet = &locpacket_->LocArray[++LocArray_last];
		packet->nid = nid;
		packet->timeStamp = ta_time;
		packet->x = x;
		packet->y = y;
		packet->z = z;
		pushtoLocList();
	}
#endif
	fprintf(logfptr, "$node_(%d) set %lf %lf %lf %lf %lf %lf\n",
		nid, x, y, z, ta_time, p_time, speed);
	fflush(logfptr);
}

int logpack::addMac8023Log(u_char proto, u_char event, u_int32_t diff,
			   struct mac802_3_log *mac8023, struct mac802_3_log *_mac8023, int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac8023->Time;
	packet->diff = diff;
	packet->FrameType = _mac8023->FrameType;
	packet->IP_Src = _mac8023->IP_Src;
	packet->IP_Dst = _mac8023->IP_Dst;
	packet->PHY_Src = _mac8023->PHY_Src;
	packet->PHY_Dst = _mac8023->PHY_Dst;
	packet->FrameID = _mac8023->FrameID;
	packet->FrameLen = _mac8023->FrameLen;
	packet->RetryCount = _mac8023->RetryCount;
	packet->DropReason = _mac8023->DropReason;
	packet->pad = pad;

	pushtoPtrList();
	return (1);
}

int logpack::addMac80211Log(u_char proto, u_char event, u_int32_t diff,
			    struct mac802_11_log *mac80211, struct mac802_11_log *_mac80211,
			    int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac80211->Time;
	packet->diff = diff;
	packet->FrameType = mac80211->FrameType;
	packet->IP_Src = mac80211->IP_Src;
	packet->IP_Dst = mac80211->IP_Dst;
	packet->PHY_Src = mac80211->PHY_Src;
	packet->PHY_Dst = mac80211->PHY_Dst;
	packet->MAC_Dst = mac80211->MAC_Dst;
	packet->FrameID = mac80211->FrameID;
	packet->FrameLen = mac80211->FrameLen;
	packet->RetryCount = mac80211->RetryCount;
	packet->DropReason = _mac80211->DropReason;
	packet->Channel = mac80211->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);
}

#ifdef CONFIG_OPTICAL
int logpack::addOphyLog(u_char proto, u_char event, u_int32_t diff,
			struct ophy_log *ophy, struct ophy_log *_ophy, int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = ophy->Time;
	packet->diff = diff;
	packet->FrameType = _ophy->FrameType;
	packet->IP_Src = _ophy->IP_Src;
	packet->IP_Dst = _ophy->IP_Dst;
	packet->PHY_Src = _ophy->PHY_Src;
	packet->PHY_Dst = _ophy->PHY_Dst;
	packet->FrameID = _ophy->FrameID;
	packet->FrameLen = _ophy->FrameLen;
	packet->RetryCount = _ophy->RetryCount;
	packet->DropReason = _ophy->DropReason;
	packet->Channel = _ophy->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);
}
#endif	/* CONFIG_OPTICAL */

#ifdef CONFIG_GPRS
int logpack::addGprsLog(u_char proto, u_char event, u_int32_t diff, struct gprs_log *gprs, int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->sTime = gprs->sTime;
	packet->diff = diff;
	packet->BurstType = gprs->BurstType;
	packet->IP_Src = gprs->IP_Src;
	packet->IP_Dst = gprs->IP_Dst;
	packet->PHY_Src = gprs->PHY_Src;
	packet->PHY_Dst = gprs->PHY_Dst;
	packet->BurstID = gprs->BurstID;
	packet->BurstLen = gprs->BurstLen;
	packet->RetryCount = gprs->RetryCount;
	packet->DropReason = gprs->DropReason;
	packet->Channel = gprs->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);
}
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_WIMAX
int logpack::addMac80216Log(u_char proto, u_char event, u_int32_t diff,
			    struct mac802_16_log *mac80216, struct mac802_16_log *_mac80216,
			    int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac80216->Time;
	packet->diff = diff;
	packet->BurstType = mac80216->BurstType;
	packet->IP_Src = mac80216->IP_Src;
	packet->IP_Dst = mac80216->IP_Dst;
	packet->PHY_Src = mac80216->PHY_Src;
	packet->PHY_Dst = mac80216->PHY_Dst;
	packet->ConnID = mac80216->ConnID;
	packet->BurstLen = mac80216->BurstLen;
	packet->RetryCount = mac80216->RetryCount;
	packet->DropReason = _mac80216->DropReason;
	packet->Channel = mac80216->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);

}
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
int logpack::addMac80216eLog(u_char proto, u_char event, u_int32_t diff,
			    struct mac802_16e_log *mac80216e, struct mac802_16e_log *_mac80216e,
			    int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac80216e->Time;
	packet->diff = diff;
	packet->BurstType = mac80216e->BurstType;
	packet->IP_Src = mac80216e->IP_Src;
	packet->IP_Dst = mac80216e->IP_Dst;
	packet->PHY_Src = mac80216e->PHY_Src;
	packet->PHY_Dst = mac80216e->PHY_Dst;
	packet->BurstLen = mac80216e->BurstLen;
	packet->RetryCount = mac80216e->RetryCount;
	packet->DropReason = _mac80216e->DropReason;
	packet->Channel = mac80216e->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);

}
#endif  /* CONFIG_MobileWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
int logpack::addMac80216jLog(u_char proto, u_char event, u_int32_t diff,
			    struct mac802_16j_log *mac80216j, struct mac802_16j_log *_mac80216j,
			    int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac80216j->Time;
	packet->diff = diff;
	packet->BurstType = mac80216j->BurstType;
	packet->IP_Src = mac80216j->IP_Src;
	packet->IP_Dst = mac80216j->IP_Dst;
	packet->PHY_Src = mac80216j->PHY_Src;
	packet->PHY_Dst = mac80216j->PHY_Dst;
	packet->BurstLen = mac80216j->BurstLen;
	packet->RetryCount = mac80216j->RetryCount;
	packet->DropReason = _mac80216j->DropReason;
	packet->Channel = mac80216j->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);

}
#endif  /* CONFIG_MobileRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
int logpack::addMac80216j_NT_Log(u_char proto, u_char event, u_int32_t diff,
                            struct mac802_16j_NT_log *mac80216j_NT, struct mac802_16j_NT_log *_mac80216j_NT,
                            int pad)
{

	struct LogObject *packet;

	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = mac80216j_NT->Time;
	packet->diff = diff;
	packet->BurstType = mac80216j_NT->BurstType;
	packet->IP_Src = mac80216j_NT->IP_Src;
	packet->IP_Dst = mac80216j_NT->IP_Dst;
	packet->PHY_Src = mac80216j_NT->PHY_Src;
	packet->PHY_Dst = mac80216j_NT->PHY_Dst;
	packet->BurstLen = mac80216j_NT->BurstLen;
	packet->RetryCount = mac80216j_NT->RetryCount;
	packet->DropReason = _mac80216j_NT->DropReason;
	packet->Channel = mac80216j_NT->Channel;
	packet->pad = pad;

	pushtoPtrList();
	return (1);

}
#endif	/* CONFIG_MR_WIMAX_NT */

#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
int logpack::addDvbrcsLog(u_char proto, u_char  event, u_int32_t diff,
	struct  dvbrcs_log *dvbrcs, struct dvbrcs_log *_dvbrcs, int pad) {

	struct LogObject * packet;
	packet = &ptrpacket_->LogObjectArray[++PtrArray_last];

	packet->PROTO = proto;
	packet->Event = event;
	packet->Time = dvbrcs->Time;
	packet->diff = diff;
	packet->FrameType = _dvbrcs->BurstType;
	packet->IP_Src  = _dvbrcs->IP_Src;
	packet->IP_Dst  = _dvbrcs->IP_Dst;
	packet->PHY_Src = _dvbrcs->PHY_Src;
	packet->PHY_Dst = _dvbrcs->PHY_Dst;
	packet->FrameID = _dvbrcs->BurstID;
	packet->FrameLen = _dvbrcs->BurstLen ;
	packet->RetryCount = _dvbrcs->RetryCount;
	packet->DropReason = _dvbrcs->DropReason;
	packet->pad = pad;
	packet->Channel = _dvbrcs->Channel;

	pushtoPtrList();
	return(1);
}
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */
