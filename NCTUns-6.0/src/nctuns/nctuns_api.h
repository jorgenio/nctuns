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

#ifndef __NCTUNS_nctuns_api_h__
#define __NCTUNS_nctuns_api_h__

#include <event.h>

#define IPC		0

/* bind global variable */
#define gBind(name, var)		vbind((NslObject *)0, name, var)
#define gBind_bool(name, var)		vbind_bool((NslObject *)0, name ,var)
#define gBind_ip(name, var)		vbind_ip((NslObject *)0, name, var)
#define gBind_mac(name, var)		vbind_mac((NslObject *)0, name, var)

/* bind local variable */
#define vBind(name, var)		vbind(this, name, var)
#define vBind_bool(name, var) 		vbind_bool(this, name, var)
#define vBind_ip(name, var) 		vbind_ip(this, name, var)
#define vBind_mac(name, var) 		vbind_mac(this, name, var)

#define GET_REG_VAR(portid, vname, type)  	\
	(type)get_regvar(get_nid(), portid, vname)

#define GET_REG_VAR1(portls, vname, type)	\
	(type)get_regvar(get_nid(), portls, vname)

#define REG_VAR(vname, var) 		\
	reg_regvar(this , vname, (void *)var)

/* These 3 definitions is for run-time messages */
#ifndef RUNTIMEMSG
	#define RUNTIMEMSG
	#define RTMSG_INFORMATION		0x01
	#define RTMSG_WARNING			0x02
	#define RTMSG_FATAL_ERROR		0x03
#endif


#define EXPORT(name, flags)		nctuns_export(this, name, flags)
#define EXPORT_SET_SUCCESS()		export_set_success()
#define EXPORT_GET_SUCCESS(ExpStr)	export_get_success(ExpStr)

class ExportStr;
class MBinder;
class NslObject;

/*-------------------------------------------------------------------------
 * Function prototype declaration
 *-------------------------------------------------------------------------*/
void str_to_macaddr		(const char *, u_char *);
void macaddr_to_str		(u_char *, char *);
void ipv4addr_to_str		(u_long, char *);
int vbind			(NslObject *, const char *, int *var);
int vbind			(NslObject *, const char *, double *var);
int vbind			(NslObject *, const char *, float *var);
int vbind			(NslObject *, const char *, u_char *var);
int vbind_bool			(NslObject *, const char *, u_char *var);
int vbind_ip			(NslObject *, const char *, u_long *var);
int vbind			(NslObject *, const char *, char **var);
int vbind_mac			(NslObject *, const char *, u_char *var);
int set_tuninfo			(u_int32_t, u_int32_t, u_int32_t,
				     u_long *, u_long *, u_char *);
int RegToMBPoller		(MBinder *);
void display_layer3dev_info	(void);
u_int32_t nodeid_to_ipv4addr	(u_int32_t, u_int32_t);
u_int32_t ipv4addr_to_nodeid	(u_long);
u_long macaddr_to_ipv4addr	(u_char *);
u_char *ipv4addr_to_macaddr	(u_long);
u_char is_ipv4_broadcast	(u_int32_t, u_long);
char *getifnamebytunid		(u_int32_t);
u_int32_t getportbytunid	(u_int32_t);
u_int64_t GetCurrentTime	(void);
u_int64_t GetNodeCurrentTime	(u_int32_t);
u_int64_t GetSimulationTime	(void);

Event_ * createEvent		(void);
int setEventTimeStamp		(Event_ *, u_int64_t, u_int64_t);
int freeEvent			(Event_ *);
int setEventReuse		(Event_ *);
int setEventCallOutFunc		(Event_ *, int (*fun)(Event_ *), void *);
int setEventCallOutObj		(Event_ *, NslObject *,
				     int (NslObject::*memf)(Event_ *),
				     void *);
int setFuncEvent		(Event_ *, u_int64_t, u_int64_t,
				     int (*func)(Event_ *), void *);
int setObjEvent			(Event_ *, u_int64_t, u_int64_t,
				     NslObject *,
				     int (NslObject::*memf)(Event_ *),
				     void *);
int scheduleInsertEvent		(ePacket_ *);
NslObject *InstanceLookup	(u_int32_t, const char *);
NslObject *InstanceLookup       (u_int32_t, u_int32_t, const char *);
NslObject *InstanceLookup       (u_long, const char *);
ePacket_ *createPacket		(void);
int freePacket                  (ePacket_ *);
ePacket_ *pkt_copy		(ePacket_ *);
void *get_regvar		(u_int32_t, u_int32_t, const char *);
void *get_regvar		(u_int32_t, struct plist*, const char*);
int reg_regvar			(NslObject *, const char *, void *);

int GetNodeLoc			(u_int32_t, double &, double &, double &);
int GetNodeAntenna		(u_int32_t, double &, double &, double &);
int GetNodeSpeed                (u_int32_t, double &speed);
int GetNodeAngle                (u_int32_t, double &angle);

char *GetScriptName		(void);
char *GetConfigFileDir		(void);

int nctuns_export		(NslObject *, const char *, u_char);
void export_set_success		(void);
void export_get_success		(ExportStr *);

u_int32_t getNumOfNodes		(void);
const char *getTypeName		(NslObject *);
const char *getNodeName		(u_int32_t);
u_char getNodeLayer		(u_int32_t);

const char *getModuleName	(const NslObject *);
u_int32_t getConnectNode	(u_int32_t, u_int32_t);
u_int32_t macaddr_to_nodeid	(u_char *);

u_long reg_IFpolling		(NslObject *, int (NslObject::*)(Event_ *),
				     int *);
int tun_write			(int, ePacket_ *);
int tun_read			(int, ePacket_ *);

/* hwchu */
double pos2angle		(double, double);
double getAntennaDirection	(double, double);

/* hwchu */
double gaussian			(double, double);
double gaussian_normal		(double, double);
double GetNodeDistance		(u_int32_t, u_int32_t);

void sendRuntimeMsg(u_int32_t, int, const char*, const char*);

#endif /* __NCTUNS_nctuns_api_h__ */
