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

// standard library
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

// nctuns library
#include <config.h>
#include <misc/log/logpack.h>
#include <object.h>
#include <nctuns-dep/node.h>
#include <nctuns-dep/pseudoif.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <random.h>
#include <regcom.h>
#include <tclBinder.h>
#include <tclObject.h>
#include <math_fun.h>
#include <ethernet.h>

#include "LScale_api.h"

#define Max_Team			10
#define Max_Member			10
#define VISIBILITY_SCALE_IN_DEGREE	40.0	//degree
#define VISIBILITY_SCALE_IN_DISTANCE	100.0	//meters
#define TRAFFIC_LIGHTS_PERIOD		60	// second

extern typeTable       		*typeTable_; 
extern RegTable			RegTable_;
Node				*node_lst[MAX_NUM_NODE + 1] = { 0 }; 

int				Team_lst[Max_Team][Max_Member];

/* export to NCTUNS API */
NslObject			**nodelist = (NslObject **)node_lst;  

MODULE_GENERATOR(Node);

//-----Global variables for IPCs and APIs
int MaxNumOfNodeInNdt = 0; // Max valid number of nodes in ndt file
int callSpecifyNodeType = 0;
int callConstructRoadMapofTheWholeField = 0;
double roadWidth_ = 20;

Node::Node(u_int32_t type, u_int32_t id, struct plist* pl, const char *name) 
		: NslObject(type, id, pl, name) 
{

	/* initialize module table */
	mlist = NULL;

	/*  get random offset for each node's virtual clock so that
	 *  GetNodeCurrentTime() returns different virtual clock for
	 *  each node
	 */
	clock_ = Random() % 5000;

	/* initialize node location */
	nodeLoc_.timeStamp_ = 0;
	nodeLoc_.pause_ = 0;
	nodeLoc_.speed_ = 0.0;
	nodeLoc_.angle_ = 0.0;					//add by chenyuan
	nodeLoc_.LocX_ = nodeLoc_.LocY_ = nodeLoc_.LocZ_ = 0.0;
	nodeLoc_.VecX_ = nodeLoc_.VecY_ = nodeLoc_.VecZ_ = 0.0;
	nodeLoc_.AntX_ = nodeLoc_.AntY_ = 0.0;
	nodeLoc_.AntZ_ = 1.5;

	/* The following function are added by automatic vehicle group 2007.2.28*/
	nodeLoc_.acceleration_ = 0.0;
	nodeLoc_.MaxVelocity = 10.0;	// 10 m/s = 36 km/hr

	/* add node location into variable table */
	vartbl_.obj_ = this;
	vartbl_.vname_ = strdup("nodeloc_");
	vartbl_.var_ = (void *)&nodeLoc_;
	vartbl_.vnext = NULL;
	varnum = 1;

	/* add to node list */
	node_lst[get_nid()] = this;

	/* Added By Large Scale Simulation Group */
	nodeLoc_.Nid = get_nid();
	nodeLoc_.Pid = get_port();
	nodeLoc_.CacheOrNot = 1;
	nodeLoc_.NodeType = 0;
	nodeLoc_.enableModuleControl = 0;
	nodeLoc_.signalGroupID = -1;
	nodeLoc_.LaneSwitch = 0;
	nodeLoc_.Tid = 0;
	nodeLoc_.TLeader = 0;
	nodeLoc_.TurningDirection = 0.0;
	nodeLoc_.TurningBlockNumber = 0;
	nodeLoc_.TurningTimes = 0;
	nodeLoc_.update_expire = 0;

	drivingInfo_ = NULL;
	sigInfo_ = NULL;

	callTrafficLightInit = 0;
}

Node::~Node() {

}

int Node::init()
{
	struct modulelist	*ml = mlist;

	/*
	 * Init() method of Node Class is used to
	 * verify node connectivity. If any error
	 * occurs, stop the simulation.
	 */
	while(ml != NULL){
		if (ml->obj->NslObject::init() < 0)
			printf("Node::init(): Warning: The initialization of the module \"%s\" may be defective.\n", ml->obj->get_name());
		if(ml->obj->init() < 0){
			printf("Node::init(): Node[%d] (%s)'s initialization is failed.\n", get_nid(), get_name());
			exit(-1);
		}
		ml = ml->next;
	}

	/* Added By Large Scale Simulation Group on 2008 */
	Event_ *ep;

	ep = createEvent();

	u_int64_t expire;
	BASE_OBJTYPE(type);

	SEC_TO_TICK(expire, 0.01);
	setEventTimeStamp(ep, expire, 0);
	type = POINTER_TO_MEMBER(Node, SetAutoUpdateToEvent);

	/*
	 * Modified By Large-Scale Simulation Group.
	 * Only one node needs to call SpecifyNodeType.
	 * Only one car needs to call BuildTeam, and constructRoadMapofTheWholeField.
	 */
	if(!callSpecifyNodeType)
	{
		/*
		 * Note:
		 *    The location of each node will be initiated in MobilityEvent()
		 */
		if(!SpecifyNodeType())
		{
			printf("Node::init(): Node[%d] (%s)'s initialization is failed!\n", get_nid(), get_name());
			exit(-1);
		}
		callSpecifyNodeType = 1;
	}

	if((!callConstructRoadMapofTheWholeField) && (nodeLoc_.NodeType == LARGE_SCALE_CAR))
	{
		// build the motorcade
		BuildTeam();
		// build the road map
		constructRoadMapofTheWholeField(0);
		callConstructRoadMapofTheWholeField = 1;
	}

	if(nodeLoc_.NodeType == LARGE_SCALE_CAR)
	{
		int tmp_nid = get_nid();

		// initiate large-scale car
		LS_CarInit();
		SEC_TO_TICK(nodeLoc_.update_expire, 0.1); //initial update expire time

		setObjEvent(ep,
				expire,
				0,
				node_lst[tmp_nid],
				type,
				(void *)0
			   );
	}
	else if(nodeLoc_.enableModuleControl)
	{
		int tmp_nid = get_nid();

		// initiate large-scale traffic light controller
		SEC_TO_TICK(nodeLoc_.update_expire, 1); //initial update expire time

		setObjEvent(ep,
				expire,
				0,
				node_lst[tmp_nid],
				type,
				(void *)0
			   );
	}

	//printf("Node::init(): Node[%d] (%s)'s initialization succeeds!\n", get_nid(), get_name());
	fflush(stdout);

	return(1);  
}

void Node::LS_CarInit()
{
	int n, mynid;

	mynid = nodeLoc_.Nid;

	//printf("*************[Node %d] is in Large Scale Simulation mode*****************\n", mynid);
	// default in group 1
	setLargeScaleCarInfo(mynid, 1, -1, -1, -1, -1, -1, -1, -1);

	drivingInfo_ = new (struct drivingInfo);

	drivingInfo_->CurrentDirection = -1;
	drivingInfo_->CurrentPos_x = drivingInfo_->CurrentPos_y = drivingInfo_->CurrentPos_z = 0.0;
	drivingInfo_->CurrentVelocity = 0.0;
	drivingInfo_->EndPosOfCurrentRoad_x = drivingInfo_->EndPosOfCurrentRoad_y = 0.0;
	drivingInfo_->isDoingLaneChange = 0;
	drivingInfo_->MaxAcceleration = 0.0;
	drivingInfo_->MaxDeceleration = 0.0;
	drivingInfo_->oldCollisionCarID = 0;
	drivingInfo_->SeenTheTrafficLightOrNot = 0;
	drivingInfo_->SigLight = 0;
	drivingInfo_->SigGID = -1;
	drivingInfo_->SigIndex = -1;
	drivingInfo_->SigPOS_x = drivingInfo_->SigPOS_y = 0.0;

	/* Cache of my road info */
	(drivingInfo_->myCache).CurrentDirty = 0;
	(drivingInfo_->myCache).NextDirty = 0;
	(drivingInfo_->myCache).AfterDirty = 0;

	(drivingInfo_->myCache).CurrentNodeSerial = -1;
	(drivingInfo_->myCache).CurrentEdgeSerial = -1;
	(drivingInfo_->myCache).CurrentLaneSerial = -1;
	(drivingInfo_->myCache).CurrentBlockSerial = -1;

	/* Cache of preceding car's road info */
	(drivingInfo_->otherCache).CurrentDirty = 0;
	(drivingInfo_->otherCache).NextDirty = 0;
	(drivingInfo_->otherCache).AfterDirty = 0;


	// Get driver behavior from car profile
	n = getProFileData(mynid, nodeLoc_.MaxVelocity, drivingInfo_->MaxAcceleration, drivingInfo_->MaxDeceleration, 0);
	if(n == 0)
	{
		//printf("Car[%d]: No profile is used. Use default value\n", mynid);
		nodeLoc_.MaxVelocity = 10;
		drivingInfo_->MaxAcceleration = 1;
		drivingInfo_->MaxDeceleration = 4;
	}

	// get road width
	getRoadWidth(roadWidth_);

	// Enable Register Information to WSM
	cmd_server_->agentEnableRegInfo(-1, mynid, 0); // fd, nid, pid

	// Fill in the WSMP header
	mywsmp.type			= WAVE_SHORT_MESSAGE_PROTOCOL;
	mywsmp.moreMsgFollowing		= 1;
	mywsmp.nid			= mynid;
	// Set WSM header
	mywsmp.wsm_header.wsm_version	= 0;
	mywsmp.wsm_header.header_len	= 9;
	mywsmp.wsm_header.header_cont	= 0;
	mywsmp.wsm_header.security_type	= 0;
	mywsmp.wsm_header.channel_num	= 174;  // sch 174,175,176,180,182,182 : cch 178
	mywsmp.wsm_header.data_rate	= 13;   // 13:3, 15:4.5, 5:6, 7:9, 9:12, 11:18, 1:24, 3:27 (index:Mbps)
	mywsmp.wsm_header.txpwr_level	= 1;
	mywsmp.wsm_header.psid		= mynid;
	mywsmp.wsm_header.expiry_time	= 0;
	mywsmp.wsm_header.wsm_len	= WsmMaxLength;

}

int Node::nodeRecv(ePacket_ *pkt)
{

	Packet			*p;
	char			*data;
	struct ether_header	*eh;

	assert(pkt && (p=(Packet *)pkt->DataInfo_));
	eh = (struct ether_header *)p->pkt_get();

	/*
	 * FIXME: We should process received packets here,
	 * 	  such as tcp/udp packet.
	 *        But now we just deal with WSM packet.
	 */

	// ether type = WSMP
	if((ntohs(eh->ether_type) == 0x88DC))
	{
		char *wsmp_data;
		//struct WaveShortMessageProtocol *wsmp_header;
		struct WaveShortMessageProtocol *wsmp;

		wsmp = new (struct WaveShortMessageProtocol);
		wsmp_data = new (char[WsmMaxLength]);

		// get WSM datagram
		data = p->pkt_sget();

		/* Fill in the WSMP header.
		 * Starts from WSM_Verion (ignore type, moreMsgFollowing, and nid)
		 */
		//memcpy(&(wsmp_header->WSM_Version), data, sizeof(struct WaveShortMessageProtocol) - 12);
		memcpy(&(wsmp->wsm_header), data, sizeof(struct WSM_Header));
		bzero(wsmp, 12);

		/* remove the WSMP header and copy WSMP data to wsmp_data */
		wsmp->type = WAVE_SHORT_MESSAGE_PROTOCOL;
		memcpy(wsmp_data, (data + sizeof(struct WSM_Header)), wsmp->wsm_header.wsm_len);
		printf("[%llu]: Node[%u] recv WSM data:: %s\n", GetCurrentTime(), nodeLoc_.Nid, (char *)wsmp_data);

		delete wsmp;
		delete wsmp_data;
	}
	else
	{
		; // Only support WSMP now
	}

	return 1;
}

int Node::nodeSend()
{
	char	*msg;
	int	n, mynid;
	char	*AppWsmData;

	mynid = nodeLoc_.Nid;
	AppWsmData = (char *) malloc(sizeof(char) * WsmMaxLength);
	sprintf(AppWsmData, "Node %d says Hello WSM !!\n", mynid);

	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	memcpy(msg, &mywsmp.wsm_header, sizeof(struct WSM_Header));
	memcpy((msg + sizeof(struct WSM_Header)), AppWsmData, mywsmp.wsm_header.wsm_len);

	free(AppWsmData);
	//printf("[%llu]: node %d sends WSM\n", GetCurrentTime(), mynid);
	if((n = cmd_server_->sendWSMtoModule(mynid, msg)) < 0)
	{
		printf("Node::%s() node %d sendWSMtoModule failed\n", __func__, mynid);
	}

	free(msg);
	return 1;
}

// ----------------------- Added By Ssuying For Large Scale Simulation ----------------------------
int BuildTeam()
{
	printf("Start to build team.\n");
	char *FILEPATH;
	char Line[512];
	FILE *fd = NULL;

	int i,j;
	for(i=0; i < Max_Team; i++)
		for(j=0; j < Max_Member; j++)
			Team_lst[i][j] = -1;

	FILEPATH = (char *) malloc(strlen(GetScriptName())+6);

	sprintf(FILEPATH, "%s%s",  GetScriptName(), ".grp") ;

	if ((fd=fopen(FILEPATH, "r")) == NULL)
	{
		printf("Build Team Failed: can't open file %s\n", FILEPATH);
		return 0;
	}

	int Tid;
	char *tmp;
	int NumOfNode = 0;
	while(!feof(fd))
	{
		Line[0] = '\0';
		fgets(Line, 512, fd);
		if ((Line[0]=='\0') || (Line[0]=='#'))
			continue;

		tmp = strtok(Line," \t\r\n");
		Tid = atoi(tmp);
		printf("Tid = %d\n", Tid);
		tmp = strtok(NULL," \t\r\n");
		while(tmp)
		{
			NumOfNode++;
			Team_lst[Tid][NumOfNode] = atoi(tmp);
			tmp = strtok(NULL," \t\r\n");
		}
		Team_lst[Tid][0] = NumOfNode;
		fgets(Line, 512, fd);
	}

	/*      for(j=0;j<Max_Member;j++)
		printf("Team_lst[%d][%d]=%d\n",Tid,j,Team_lst[Tid][j]);
	*/
	free(FILEPATH);
	fclose(fd);
	return 1;
}

/* Added By Large Scale Simulation Group on 2008 */
void Node::SetAutoUpdateToEvent(Event_ *ep)
{
	int tmp_id = get_nid();

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(Node, SetAutoUpdateToEvent);

	if(nodeLoc_.NodeType == LARGE_SCALE_CAR)
	{
		// periodically update node location
		UpdateNodeLocation();
	}
	else if(nodeLoc_.enableModuleControl)
	{
		// periodically update traffic light
		UpdateTrafficLight();
	}
	else
	{
		printf("Node[%d]: %s doesn't support such node type\n", nodeLoc_.Nid, __func__);
		return;
	}
	fflush(stdout);

	setObjEvent(ep,
			(ep->timeStamp_) + nodeLoc_.update_expire,
			0,
			node_lst[tmp_id],
			type,
			(void *)0
		   );

	return;

}

int Node::SpecifyNodeType()
{
	char *FILEPATH;
	char Line[512];
	FILE *fd = NULL;

	/* get the file path */
	FILEPATH = (char *) malloc(strlen(GetScriptName())+6);

	/* Modified By Large Scale Simulation Group on 2008 */
	sprintf(FILEPATH, "%s%s",  GetScriptName(), ".ndt") ;

	if ((fd = fopen(FILEPATH, "r")) == NULL)
	{
		printf("Error: can't open file %s\n", FILEPATH);
		return 0;
	}

	int testI = 0;
	char *tmp;
	fgets(Line, 512, fd);
	tmp = strtok(Line, " \n");
	MaxNumOfNodeInNdt = atoi(tmp); // Max valid number of nodes in ndt file

	// Node ID starts from 1
	// int i = 0;
	//for(i = 1 ; i <= MaxNumOfNodeInNdt ; ++i)
	//{
	while(1)
	{
		bzero(Line, 512);
		fgets(Line, 511, fd);

		if(feof(fd))
			break;

		// get node number
		tmp = strtok(Line, " \n");
		if(tmp == NULL)
			continue;
		testI = atoi(tmp);
		if(testI < 0)
			continue;
		
		if(node_lst[testI] == NULL)
		{
			/* This node belongs to the supernode. */
			continue;
		}

		// get node's type name
		tmp = strtok(NULL, " \n");

		if(!strncmp(tmp, "CAR_LS", 6))
		{
			/* It is a Large-Scale Car
			 * Format in ndt:
			 *      CAR_LS 1 [LaneSwitch] [TeamID] [TeamLeader]
			 */
			//printf("Node[%d] is large-scale car\n", i);
			node_lst[testI]->nodeLoc_.NodeType = LARGE_SCALE_CAR;
			tmp = strtok(NULL," ");
			tmp = strtok(NULL," ");
			node_lst[testI]->nodeLoc_.LaneSwitch = atoi(tmp); // enable lane switch
			tmp = strtok(NULL," ");
			node_lst[testI]->nodeLoc_.Tid = atoi(tmp);  // Team ID
			tmp = strtok(NULL," ");
			node_lst[testI]->nodeLoc_.TLeader = atoi(tmp);  // Team Leader
		}
		else if(strstr(tmp, "CAR") != NULL)
		{
			/* This node may be an CAR_ADHOC, CAR_INFRA, etc */
			//printf("Node[%d] is a car agent\n", i);
			node_lst[testI]->nodeLoc_.NodeType = AGENT;
			tmp = strtok(NULL," ");
			if(tmp != NULL)
			{
				/*
				 * Exampel format in ndt:
				 *      CAR_ADHOC 0 [LaneSwitch] [TeamID] [TeamLeader]
				 */
				tmp = strtok(NULL," ");
				node_lst[testI]->nodeLoc_.LaneSwitch = atoi(tmp); // enable lane switch
				tmp = strtok(NULL," ");
				node_lst[testI]->nodeLoc_.Tid = atoi(tmp);  // Team ID
				tmp = strtok(NULL," ");
				node_lst[testI]->nodeLoc_.TLeader = atoi(tmp);  // Team Leader
			}
		}
		else if(!strcmp(tmp, "TRAFFIC_LIGHT_CONTROLLER"))
		{
			//printf("Node[%d] is traffic light controller, ", i);
			node_lst[testI]->nodeLoc_.NodeType = TRAFFIC_LIGHT_CONTROLLER;
			tmp = strtok(NULL," ");
			node_lst[testI]->nodeLoc_.enableModuleControl = atoi(tmp);
			tmp = strtok(NULL," ");
			node_lst[testI]->nodeLoc_.signalGroupID = abs(atoi(tmp));
			//printf("enableModuleControl %d, signalGroupID %d\n", node_lst[i]->nodeLoc_.enableModuleControl, node_lst[i]->nodeLoc_.signalGroupID);
		}
		else
		{
			//printf("Node[%d] is other type of node\n", i);
			node_lst[testI]->nodeLoc_.NodeType = OTHER_NODE;
		}
	}

	free(FILEPATH);
	fclose(fd);
	return 1;
}

// ------------------------------------------------------------------------------------------------

int Node::command(int argc, const char *argv[]) {

	/*
	 * Construct Command:
	 *	- syntax: Construct Module-Name Module-Instance-Name
	 *	- used to construct a node topology.
	 */
	if ((argc==3) && !strcmp(argv[0], "Construct")) {

		NslObject		*obj;
		struct Module_Info	*minfo;
		struct modulelist	*ml = mlist;

		obj = RegTable_.lookup_Instance(get_nid(), argv[2]); 
		minfo = RegTable_.get_moduleInfo(argv[1]);
		if (obj==NULL || minfo==NULL) 
			return(-1);

		/* Added bj C.C. Lin:
		 * A supernode has to contain modules with all node_types. As such,
		 * it should bypass this matching test.
		 */

		/*
		 * Check node type and node ID. They should
		 * match the current module-instance's node
		 * type and ID.
		 */
		u_int32_t sn_typeid = typeTable_->toType("SUPERNODE");

		if (!sn_typeid) {
			printf("Node::command(): the supernode type is not implemented.\n");
			exit(1);
		}
		if (sn_typeid == get_type())
			assert((obj->get_nid() == get_nid()));
		else
			assert((obj->get_nid() == get_nid()) && (obj->get_type() == get_type()));

		if(mlist == NULL){
			mlist = (struct modulelist *)malloc(sizeof(struct modulelist));
			mlist->next = NULL;
			mlist->mInfo = minfo;
			mlist->obj = obj;
		}
		else{
			while(ml->next != NULL)
				ml = ml->next;
			ml->next = (struct modulelist *)malloc(sizeof(struct modulelist));
			ml = ml->next;
			ml->next = NULL;
			ml->mInfo = minfo;
			ml->obj = obj;						   
		}
	}


	/* Call MobilityEvent() to generate location
	 * update events.
	 */

	if (argc>0 && !strcmp(argv[0], "MobilityEvent")) {
		printf("Node %d executes MobilityEvent()\n",get_nid());
		MobilityEvent();
		return(1);
	}

	return(-1);
}

#define SET_UPDATE_INFO(uinfo, x, y, z, s, pt)				\
{									\
	SEC_TO_TICK(uinfo->pause_, pt);					\
	uinfo->LocX_ = x;						\
	uinfo->LocY_ = y;						\
	uinfo->LocZ_ = z;						\
	uinfo->speed_ = s;						\
	uinfo->VecX_ = 0.0;						\
	uinfo->VecY_ = 0.0;						\
	uinfo->VecZ_ = 0.0;						\
	uinfo->angle_= 0.0;						\
}; 

#define PI                      3.14159267
inline double NodeCalcuAngle(double x1, double y1, double x2, double y2) {

	// Be careful! The GUI coordinate system differs from the normal one.
	// On the GUI coordinate system, if the angle is A, then the
	// corresponding angle on the normal system is (360 - A).
	// The angle returned here is the moving angle viewed in the GUI
	// coordinate system rather than in the normal coordinate system.

	double radians = atan2(y1-y2, x2-x1);
	double angle = radians * 180 / PI ;

	angle = fmod((angle+360),360);

	angle = angle * 1000;
	angle = roundl(angle);
	angle = angle / 1000;
	angle = fmod(angle,360);
	return angle;
}

int Node::MobilityEvent() {

	FILE			*fd;
	int			i; 
	char			buf[101];
	u_int32_t		nodeid;
	u_int64_t		expire; 
	double			x, y, z, a_time, p_time, speed;
	Event_			*ep[MAX_NUM_NODE + 1] = { 0 };
	int			hasOnlyOneRecord[MAX_NUM_NODE + 1];
	struct updateInfo	*uInfo; 
	double			dx, dy, dz, dis; 
	BASE_OBJTYPE(type); 


	/* get movement file name and open it */
	char *FILEPATH = (char *)malloc(strlen(GetScriptName()) + 6);
	sprintf(FILEPATH, "%s%s", GetScriptName(), ".sce");

	if ((fd = fopen(FILEPATH, "r")) == NULL) {
		printf("Error: can't open file %s\n", FILEPATH);
		exit(-1);
	}

	for(int i = 0; i < MAX_NUM_NODE + 1; ++i)
	{
		hasOnlyOneRecord[i] = 1;
	}

	type = POINTER_TO_MEMBER(Node, updateLocation);
	while(!feof(fd)) {
		buf[0] = '\0';
		fgets(buf, 100, fd);
		if ((buf[0]=='\0')||(buf[0]=='#')) 
			continue;

		i = sscanf(buf, "$node_(%d) set %lf %lf %lf %lf %lf %lf",
				&nodeid, &x, &y, &z, &a_time, &p_time, &speed);
		if (i != 7) {
			printf("Warning: format error -> %s\n", buf);
			continue; 
		}

		/* Create an event with updateInfo structure 
		 * to update node's location.
		 */
		if (ep[nodeid]) {
			/* This node has at least two records in this file. */
			hasOnlyOneRecord[nodeid] = 0;
			/* calculate direction vector */
			uInfo = (struct updateInfo *)ep[nodeid]->DataInfo_;
			dx = x - uInfo->LocX_;
			dy = y - uInfo->LocY_;
			dz = z - uInfo->LocZ_;
			dis = sqrt(dx*dx + dy*dy + dz*dz);
			// printf("node %d Mobilevent :: dx %lf %lf %lf dis %lf \n",nodeid,dx,dy,dz,dis);
			uInfo->VecX_ = dx / dis;
			uInfo->VecY_ = dy / dis;
			uInfo->VecZ_ = dz / dis;

			/* The following angle is the moving direction viewed
			   in the GUI coordinate system rather than in the
			   normal coordinate system.
			   */
			uInfo->angle_ = NodeCalcuAngle(uInfo->LocX_, uInfo->LocY_, x, y); 

			/* setting event */
			setObjEvent(ep[nodeid],
					ep[nodeid]->timeStamp_,
					0, 
					node_lst[nodeid],
					type,
					ep[nodeid]->DataInfo_
				   ); 
		}

		assert(node_lst[nodeid]); 
		ep[nodeid] = createEvent();

		/* malloc a space to hold update 
		 * information
		 */
		uInfo = (struct updateInfo *)malloc(sizeof(struct updateInfo));
		assert(uInfo);
		SET_UPDATE_INFO(uInfo, x, y, z, speed, p_time);
		ep[nodeid]->DataInfo_ = (void *)uInfo;

		/* set event timestamp */
		SEC_TO_TICK(expire, a_time); 
		setEventTimeStamp(ep[nodeid], expire, 0); 

		node_lst[nodeid]->nodeLoc_.LocX_ = x;
		node_lst[nodeid]->nodeLoc_.LocY_ = y;
		node_lst[nodeid]->nodeLoc_.LocZ_ = z;

		/* Add by Large-Scale Simulation Group */
		if(node_lst[nodeid]->nodeLoc_.NodeType == LARGE_SCALE_CAR)
		{
			int n = 0;
			double CorrectedPOS_x, CorrectedPOS_y;
			/* Correct this car's position to the middle of the road */
			n = selfCorrectness(x, y, CorrectedPOS_x, CorrectedPOS_y, node_lst[nodeid]->nodeLoc_.CacheOrNot, &(node_lst[nodeid]->drivingInfo_->myCache));
			if(n == 1)
			{
				node_lst[nodeid]->nodeLoc_.LocX_ = CorrectedPOS_x;
				node_lst[nodeid]->nodeLoc_.LocY_ = CorrectedPOS_y;
			}
		}
	}


	/* add remainder event to heap */
	for(i = 1; i <= MAX_NUM_NODE; i++) {
		if (ep[i] != 0) {
			// printf("Node[%d] sets timer Update location\n",i);
			if (hasOnlyOneRecord[i]) {
				/* Initialize this node's location */
				ep[i]->perio_ = 0;
				ep[i]->calloutObj_ = node_lst[i];
				ep[i]->memfun_ = type;
				ep[i]->DataInfo_ = ep[i]->DataInfo_;

				// directly call updateLocation without pushing event into heap
				(ep[i]->calloutObj_->*(ep[i]->memfun_))(ep[i]);
			}
			else
			{
				// push this event into event heap
				setObjEvent(ep[i],
						ep[i]->timeStamp_,
						0,
						node_lst[i],
						type,
						ep[i]->DataInfo_
					   );
			}
		}
	}

	fclose(fd);
	free(FILEPATH);
	return(1);  
}

int Node::updateLocation(Event_ *ep) {

	struct updateInfo	*uInfo;
	BASE_OBJTYPE(type); 


	/* If DataInfo_ == NULL, it means that we  
	 * should clear pause_ flag to indicate
	 * that the pause action is complete.
	 */
	if (ep->DataInfo_ == 0) {
		nodeLoc_.pause_ = 0;
		nodeLoc_.timeStamp_ = ep->timeStamp_; 
		freeEvent(ep);
		return(1); 
	}   

	uInfo = (struct updateInfo *)ep->DataInfo_;

	/* Update node's location, speed, and timeStamp
	 * and direction Vector.
	 */
	nodeLoc_.timeStamp_ = ep->timeStamp_;  

	nodeLoc_.LocX_ = uInfo->LocX_;
	nodeLoc_.LocY_ = uInfo->LocY_;
	nodeLoc_.LocZ_ = uInfo->LocZ_;
	nodeLoc_.VecX_ = uInfo->VecX_;
	nodeLoc_.VecY_ = uInfo->VecY_;
	nodeLoc_.VecZ_ = uInfo->VecZ_;  
	nodeLoc_.speed_ = uInfo->speed_;
	nodeLoc_.angle_ = uInfo->angle_;

	/* check to see if the pause function is enabled */
	if (uInfo->pause_) {
		nodeLoc_.pause_ = 1;
		type = POINTER_TO_MEMBER(Node, updateLocation);
		setObjEvent(ep,
				ep->timeStamp_ + uInfo->pause_, 
				0,
				node_lst[get_nid()],
				type,
				(void *)0
			   ); 
		free(uInfo);
		return(1);  
	}

	// free this event
	freeEvent(ep); 
	return(1);
}

int Node::getNodeAntenna(double &x, double &y, double &z) {

	x = nodeLoc_.AntX_;
	y = nodeLoc_.AntY_;
	z = nodeLoc_.AntZ_;
	return(1); 
}

void Node::setNodeAntenna(double x, double y, double z) {

	nodeLoc_.AntX_ = x;
	nodeLoc_.AntY_ = y;
	nodeLoc_.AntZ_ = z;
}

/* The following function are re-wrote by automatic vehicle group 2007.2.28 */
int Node::getNodeSpeed(double &speed) {
	u_int64_t	elapsedTimeInTicks;
	double		elapsedTimeInSecond;
	double		curx, cury, curz;
	u_int64_t	ctime;

	ctime = GetCurrentTime();
	if (nodeLoc_.acceleration_ == 0)
	{
		/* original function */
		speed = nodeLoc_.speed_; 
	}
	else {
		elapsedTimeInTicks = ctime -  nodeLoc_.timeStamp_;
		TICK_TO_SEC(elapsedTimeInSecond, elapsedTimeInTicks);

		speed = nodeLoc_.speed_ + nodeLoc_.acceleration_ * elapsedTimeInSecond; 

		/* nodeLoc_.acceleration_ < 0 */
		if (speed <= 0) {
			speed = 0;
			getNodePosition(curx, cury, curz);
			setNodeCurrentWaypoint(curx, cury, curz);

			nodeLoc_.acceleration_ = 0;
			nodeLoc_.speed_ = 0;
		}
	}
	return(1); 
}

void Node::setNodeSpeed(double speed) {
	double curx, cury, curz;

	getNodePosition(curx, cury, curz);
	setNodeCurrentWaypoint(curx, cury, curz);
	nodeLoc_.speed_ = speed;
}

/* The following function are added by automatic vehicle group 2007.2.28*/
void Node::setNodeSpeedAcceleration(double acceleration) {
	double		speed;

	/* Here there is no need to execute the following two statements:

	   getNodePosition(curx, cury, curz);
	   setNodeCurrentWaypoint(curx, cury, curz);

	   because they are already executed in Node::setNodeSpeed().
	   These two statements must be executed in Node::setNodeSpeed() rather than
	   in this function. This is because a mobile node may directly call 
	   Node::setNodeSpeed() without calling setNodeSpeedAcceleration().

	   These two statements should not be executed twice, one in each function.

	   Prof. S.Y. Wang 04/30/2006
	   */
	getNodeSpeed(speed);
	setNodeSpeed(speed);
	nodeLoc_.acceleration_ = acceleration;
	nodeLoc_.timeStamp_ = GetCurrentTime();
	/* Here we create a new waypath due to a different acceleration */
}

int Node::getNodeAngle(double &angle) {
	/* The returned angle is the moving direction viewed in the 
	   GUI coordinate system rather than in the normal coordinate system.
	   */
	angle = nodeLoc_.angle_;
	return(1); 
}

void Node::setNodeAngle(double angle) {
	/* The input angle should be the moving direction viewed in the 
	   GUI coordinate system rather than in the normal coordinate system.
	   Note that on the GUI coordinate system, if the viewed moving 
	   angle is A, then the corresponding angle on the normal system 
	   is (360 - A). This means that if an agent program uses the
	   normal way to calculate the moving angle from (x1, y1) to
	   (x2, y2), which is atan2(y2-y1, x2-x1) and is represented A here, 
	   before calling Node::setNodeAngle(), the agent program should
	   convert A to (360-A) and then give (360-A) as the input argument
	   to this function.
	   */
	double curx, cury, curz;

	getNodePosition(curx, cury, curz);
	setNodeCurrentWaypoint(curx, cury, curz);
	nodeLoc_.angle_ = angle;
	/* The following function are added by automatic vehicle group 2007.2.28*/
	nodeLoc_.VecX_ = cos(((double) (360 - angle) / (double) 180) * PI);
	nodeLoc_.VecY_ = sin(((double) (360 - angle) / (double) 180) * PI); 
	nodeLoc_.VecZ_ = 0;
}

int Node::getNodeCurrentWaypointTimeStamp(u_int64_t &timestamp) { 	
	timestamp = nodeLoc_.timeStamp_;
	return(1);
}

int Node::getNodeCurrentWaypoint(double &x ,double &y, double &z) {
	x = nodeLoc_.LocX_;
	y = nodeLoc_.LocY_;
	z = nodeLoc_.LocZ_;
	return(1);
}

void Node::setNodeCurrentWaypoint(double x ,double y, double z) {
	nodeLoc_.LocX_ = x;
	nodeLoc_.LocY_ = y;
	nodeLoc_.LocZ_ = z;
	nodeLoc_.timeStamp_ = GetCurrentTime();
}

void Node::setNodeNextWaypoint(double x ,double y, double z) {
	double curx, cury, curz, dx, dy, dz, dis;

	getNodePosition(curx, cury, curz);
	setNodeCurrentWaypoint(curx, cury, curz);

	dx = x - nodeLoc_.LocX_;
	dy = y - nodeLoc_.LocY_;
	dz = z - nodeLoc_.LocZ_;
	dis = sqrt(dx*dx + dy*dy + dz*dz);
	nodeLoc_.VecX_ = dx / dis;
	nodeLoc_.VecY_ = dy / dis;
	nodeLoc_.VecZ_ = dz / dis;

	/* The following angle is the moving direction viewed
	 * in the GUI coordinate system rather than in the
	 * normal coordinate system.
	 */
	nodeLoc_.angle_ = NodeCalcuAngle(nodeLoc_.LocX_, nodeLoc_.LocY_, x, y);
}

int Node::getNodeVec(double &vec_x ,double &vec_y ,double &vec_z) {
	vec_x = nodeLoc_.VecX_;
	vec_y = nodeLoc_.VecY_;
	vec_z = nodeLoc_.VecZ_;
	return(1);
}

void Node::setNodeVec(double vec_x ,double vec_y ,double vec_z) {
	nodeLoc_.VecX_ = vec_x;
	nodeLoc_.VecY_ = vec_y;
	nodeLoc_.VecZ_ = vec_z;
}

int Node::getNodePosition(double &x, double &y, double &z) {
	u_int64_t		ctime, elapsedTimeInTicks;
	double 			elapsedTimeInSecond;
	double 			totalMovementInMeter;

	if (nodeLoc_.pause_) {
		x = nodeLoc_.LocX_;
		y = nodeLoc_.LocY_;
		z = nodeLoc_.LocZ_;
		return(1);
	} 

	ctime = GetCurrentTime();

	/* constant speed */
	if (nodeLoc_.acceleration_ == 0) {
		double second;
		TICK_TO_SEC(second, (ctime - nodeLoc_.timeStamp_));
		x = nodeLoc_.LocX_ + 
			nodeLoc_.VecX_*nodeLoc_.speed_ * second;
		y = nodeLoc_.LocY_ +
			nodeLoc_.VecY_*nodeLoc_.speed_ * second;
		z = nodeLoc_.LocZ_ +
			nodeLoc_.VecZ_*nodeLoc_.speed_ * second;
	}
	/* variable speed */
	else {
		elapsedTimeInTicks = ctime - nodeLoc_.timeStamp_;
		TICK_TO_SEC(elapsedTimeInSecond, elapsedTimeInTicks);
		if(nodeLoc_.speed_ + nodeLoc_.acceleration_ * elapsedTimeInSecond > 0){
			totalMovementInMeter = nodeLoc_.speed_*elapsedTimeInSecond + (nodeLoc_.acceleration_*elapsedTimeInSecond*elapsedTimeInSecond)/((double)2);
			x = nodeLoc_.LocX_ + nodeLoc_.VecX_ * totalMovementInMeter;
			y = nodeLoc_.LocY_ + nodeLoc_.VecY_ * totalMovementInMeter;
			z = nodeLoc_.LocZ_ + nodeLoc_.VecZ_ * totalMovementInMeter;
		}
		else{
			//The agent should stop at the position that its velocity reaches zero.
			elapsedTimeInSecond = fabs(nodeLoc_.speed_ / nodeLoc_.acceleration_);
			totalMovementInMeter = nodeLoc_.speed_*elapsedTimeInSecond + (nodeLoc_.acceleration_*elapsedTimeInSecond*elapsedTimeInSecond)/((double)2);
			x = nodeLoc_.LocX_ + nodeLoc_.VecX_ * totalMovementInMeter;
			y = nodeLoc_.LocY_ + nodeLoc_.VecY_ * totalMovementInMeter;
			z = nodeLoc_.LocZ_ + nodeLoc_.VecZ_ * totalMovementInMeter;

		}
	}
	return(1); 
}

void *Node::getRegVar(u_int32_t portid, const char *vname) {
	struct varRegtbl        *vtbl;

	vtbl = &vartbl_;
	while(vtbl) {
		if ((portid == vtbl->obj_->get_port()) && 
				(!strcmp(vname, vtbl->vname_)))   {
			/* match */
			return(vtbl->var_);
		}
		vtbl = vtbl->vnext;
	}
	return(NULL);
}

void *Node::getRegVar(struct plist *pl, const char *vname) {

	struct varRegtbl        *vtbl;
	struct plist            *p, *p_;
	struct plist            *curP;
	int                     i,mark = 1;

	vtbl = &vartbl_;
	for (i=0; i<(int)varnum; i++) {
		curP = vtbl->obj_->get_portls();	
		p_ = curP;
		p = pl;
		while ((p) && (p_)) {
			if (p->pid == p_->pid)
				mark = 1;
			else {
				mark = 0;
				break;
			}
			p_ = p_->next;
			p = p->next;
		}
		if ((p != NULL) || (p_ != NULL))
			mark = 0;
		if (mark && (!strcmp(vtbl->vname_, vname))) {
			/* match */
			return (vtbl->var_);
		}
		vtbl = vtbl->vnext;

	}
	return(NULL);

}

int Node::regVar(NslObject *obj, const char *vname, void *ptr) {

	struct varRegtbl	*vtbl, *v;

	/* check to see if there is a duplicate one. */
	vtbl = &vartbl_;
	while (vtbl) {
		if (!strcmp(vtbl->vname_, vname) && 
			!strcmp(vtbl->obj_->get_name(), obj->get_name())){
			printf("Already registered : %s %s!!\n", vname, 
				obj->get_name());
			return 0;
		}
		if (vtbl->vnext)
			vtbl = vtbl->vnext;
		else
			break;
	}

	v = (struct varRegtbl *) malloc(sizeof(struct varRegtbl));
	v->obj_   = obj;
	v->vname_ = strdup(vname);
	v->var_   = ptr;
	v->vnext= NULL;
	vtbl->vnext = v;
	varnum++;
	return(1);
}

u_int64_t Node::getNodeClock()
{
	return clock_ + GetCurrentTime();
}

/* The following functions are added by automatic vehicle group 2007.2.28*/
int Node::getNodeMaxSpeed(double &maxspeed){
	maxspeed = nodeLoc_.MaxVelocity;

	return 1;
}

// ----------------------- Added By Ssuying For Large Scale Simulation ----------------------------

int Node::getNodeType(int &nodeType){
	nodeType = nodeLoc_.NodeType;
	return 1;
}

void Node::FillTheQueue(int NumOfTurns, double *turningPOS_x, double *turningPOS_y, double *directions, double FirstPointAfterTheTurn_x, double FirstPointAfterTheTurn_y, double ExpectDirection){
	/*
	 * This function fulls the queue when agent just has reached a new road block.
	 * After agent calls the takeATurn function,
	 * it uses "pass by reference parameters" to full the queue by this function.
	 */
	CarAgentEvent temp;
	for (int i = 0; i<NumOfTurns; ++i){
		//put every turning points into the queue.
		temp.x = turningPOS_x[i];
		temp.y = turningPOS_y[i];
		temp.direction = directions[i];
		drivingInfo_->EventQueue.push_back(temp);
		//printf("Car %d : turningPOS (%lf, %lf), Direction %lf\n", nodeLoc_.Nid, temp.x, temp.y, temp.direction);

	}
	//Finally, put the end point of the turning into the queue.
	temp.x = FirstPointAfterTheTurn_x;
	temp.y = FirstPointAfterTheTurn_y;
	temp.direction = ExpectDirection;
	//printf("Car %d : Final point (%lf, %lf), Direction %lf\n", nodeLoc_.Nid, temp.x, temp.y, temp.direction);
	drivingInfo_->EventQueue.push_back(temp);

}

inline double SelectMinimum(double a, double b){
	if(a <= b)
		return a;
	else
		return b;
}

inline double SelectMaximum(double a, double b){
	if(a >= b)
		return a;
	else
		return b;
}

double RandomDirection(int NumOfDirections, double *directions){
	int RandInt = rand() % NumOfDirections;
	return directions[RandInt];
}

int Node::ReachTheNextTriggerPointOrNot()
{
	if(((int) drivingInfo_->EventQueue.size()) == 0)
		return -1;// Error!!
	double tempAngle = fmod(atan2(drivingInfo_->CurrentEvent.y - drivingInfo_->CurrentPos_y, drivingInfo_->CurrentEvent.x - drivingInfo_->CurrentPos_x)/PI*180 + 360, 360);
	tempAngle = fmod(360 - tempAngle, 360);

	double angleDiff = fabs(tempAngle - drivingInfo_->CurrentDirection);

	if(angleDiff > 180)
		angleDiff = 360 - angleDiff;
	if(angleDiff >= 90)
	{
		if(drivingInfo_->EventQueue.empty() != true)
			drivingInfo_->EventQueue.pop_front();
		return 1; // reach Event point
	}
	else
		return 0; // unreach
}

/*
 * S: distance in meter
 * v0: current velocity in meter/s (> 0)
 * vt: target velocity in meter/s (> 0, because movng in the same direction with v0)
 * a: acceleration in meter/(s^2)
 * t: time in second
 *
 * E1: vt = v0 + a*t
 * E2: S = v0*t + (1/2)*a*(t^2)
 *
 * From E1, we get a = (vt - v0)/t
 * Put a into E2, we get S = 0.5*(v0 + vt)*t
 * Therefore, we get
 * E3: t = 2 * S / (v0 + vt)
 */
void Node::DetermineVc(double &BufferTime, double &vt, double NearestPrecedingCarPOS_x,
		double NearestPrecedingCarPOS_y, double NearestPrecedingCarPOS_z,
		double NearestPrecedingCarDirection, int NearestPrecedingCarID)
{
	double NearestPrecedingCarSpeed = 999999;
	double DesiredMaxSpeed, DistanceToNextEventPoint; // in meter/secs.
	double SAFETY_INTERVEHICLE_DISTANCE = 10;
	double BufferTime1, BufferTime2, BufferTime3 = 999999;

	//Desired max speed should be lower at the corner.
	DistanceToNextEventPoint = Distance_BetweenTwoNode(drivingInfo_->CurrentEvent.x, drivingInfo_->CurrentEvent.y, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y);

	/* Cosider what to do when close to the end of a road block. */
	if((DistanceToNextEventPoint != 0) && (drivingInfo_->CurrentVelocity != 0))
	{
		BufferTime3 = roundf(DistanceToNextEventPoint / drivingInfo_->CurrentVelocity);
	}

	if(DistanceToNextEventPoint >= 30)
	{
		DesiredMaxSpeed = nodeLoc_.MaxVelocity; // meter/second
	}
	else
	{
		// If close to the next event point, don't drive too fast.
		DesiredMaxSpeed = rand() % 6 + 5; // 10 m/s => 36 km/hr at maxmimum
	}

	double vt1, vt2;

	vt1 = DesiredMaxSpeed;
	vt2 = DesiredMaxSpeed;

	// set default time, make car run as fast as possible
	/* Put a = MaxAcceleration into E1 and get BufferTime1 */
	BufferTime1 = fabs(vt1 - drivingInfo_->CurrentVelocity) / drivingInfo_->MaxAcceleration;
	BufferTime2 = BufferTime1;

	if(NearestPrecedingCarID > 0)
	{
		/* There's a car in front of me. */

		int n = checkIfOnTheSameLane(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache), &(drivingInfo_->otherCache));
		if(n != 0)
		{
			/* The preceding car is on the same lane with me. */

			double distance = 999999;

			// Get the distance between me and the preceding car.
			distance = Distance_BetweenTwoNode(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);

			// Get preceding car's velocity.
			node_lst[NearestPrecedingCarID]->getNodeSpeed(NearestPrecedingCarSpeed);

			/* Too close to the preceding car. */
			if(distance <= 3)
			{
				BufferTime = 0;
				vt = 0;
				return;
			}

			/* Consider the effects of the preceding car's velocity and position. */
			if(NearestPrecedingCarSpeed == 0)
			{
				vt1 = NearestPrecedingCarSpeed;

				if(drivingInfo_->CurrentVelocity == 0)
				{
					vt = 0;
					BufferTime = 0;
					return;
				}
				/* Set vt to 0 and put vt into E3.
				 * We get t = 2 * S / v0
				 */
				BufferTime1 = 2 * (distance - 3) / drivingInfo_->CurrentVelocity;
			}
			else
			{
				if(distance < 2 * SAFETY_INTERVEHICLE_DISTANCE)
				{
					/* Set the target velocity to NearestPrecedingCarSpeed. */
					double tmpV;

					tmpV = drivingInfo_->CurrentVelocity + NearestPrecedingCarSpeed;
					// Using E3 to calculate t.
					BufferTime1 = 2*(distance - SAFETY_INTERVEHICLE_DISTANCE)/tmpV;
					vt1 = NearestPrecedingCarSpeed;
				}
				else
				{
					/* Preceding car is far from me.
					 * Using default vt1 and BufferTime1.
					 */
				}
			}
		}
	}

	/* Cosider what to do when close to the traffic light */
	if((drivingInfo_->SeenTheTrafficLightOrNot == 1) && (drivingInfo_->SigLight == RED || drivingInfo_->SigLight == YELLOW))
	{
		double DistanceToTrafficLight = 9999999;

		DistanceToTrafficLight = Distance_BetweenTwoNode(drivingInfo_->SigPOS_x, drivingInfo_->SigPOS_y, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y);

		if(DistanceToTrafficLight <= 2*SAFETY_INTERVEHICLE_DISTANCE){
			// Getting close to the traffic light, need to slow down.
			vt2 = SelectMinimum(0 - (DistanceToTrafficLight - SAFETY_INTERVEHICLE_DISTANCE) / 10 , DesiredMaxSpeed);
		}

		if((DistanceToTrafficLight != 0) || (drivingInfo_->CurrentVelocity != 0))
		{
			if((BufferTime2 = DistanceToTrafficLight / drivingInfo_->CurrentVelocity) < 0)
			{
				BufferTime = 0;
				return;
			}

			/*
			 * From E1, put vt = 0 and we get 0 = v0 + at
			 * Ignore (1/2)*a*(t^2) part in E2 because t is very small.
			 * We get
			 * E1': 0 = v0 + a*t
			 * E2': S = v0*t
			 *
			 * From E1' and E2' we get v0 = sqrt(a * S)
			 */
			double scv1 = 0; // safe current velocity

			scv1 = sqrt(drivingInfo_->MaxDeceleration * DistanceToTrafficLight);

			/* safe check */
			if(drivingInfo_->CurrentVelocity >= scv1){
				/* Dangerous Velocity!! Need to slow down fast. */
				BufferTime = 0;
				return;
			}
		}
	}

	BufferTime = SelectMinimum(BufferTime1, BufferTime2);
	BufferTime = SelectMinimum(BufferTime, BufferTime3);

	vt = SelectMinimum(vt1, vt2);
	/*printf("DEBUG: vt %lf, vt1 %lf, vt2 %lf, buffTime %lf, bf1 %lf, bf2 %lf, bf3 %lf\n",
			vt, vt1, vt2, BufferTime, BufferTime1, BufferTime2, BufferTime3);*/
}

double Node::DetermineAcceleration(double NearestPrecedingCarPOS_x, double NearestPrecedingCarPOS_y,
		double NearestPrecedingCarPOS_z, double NearestPrecedingCarDirection, int NearestPrecedingCarID)
{
	/*
	 * The following implementation is based on
	 * VATSIM: A Simulator for Vehicles and Traffic ,
	 * Jia Lei Keith Redmill Umit Ozguncr ,
	 * Department of Electrical Engineering, The Ohio State University,
	 * 2001 IEEE Intelligent Transportation Systems Conference Proceedings
	 *
	 * vt:	the target velocity
	 * bfTime: the buffer time to achieve vt.
	 */
	double vt, bfTime;

	DetermineVc(bfTime, vt, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID);

	double acc;

	if(bfTime > 0)
	{
		/* Equation: vt = CurrentVelocity + acc * bfTime */
		acc = roundf((vt - drivingInfo_->CurrentVelocity) / bfTime * 10);
		acc /= 10;
	} else {
		/* Emergency Stop */
		acc = -(drivingInfo_->MaxDeceleration);
	}

	if(acc > drivingInfo_->MaxAcceleration)
		return drivingInfo_->MaxAcceleration;
	else if(acc < -(drivingInfo_->MaxDeceleration))
		return -(drivingInfo_->MaxDeceleration);
	else
		return  acc;
}

/*
 * You can use this API to get the car position who is in the scope of range and
 * inside the distance of tDistance.
 */
int Node::getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreenInNode(double direction, double range, double tDistance, double &x, double &y, double &z, double &tDirection, int &id)
{
	int i, tempN, result = -1;
	double tempAngle, distance;
	double tempDistance = tDistance;

	if(direction < 0)
		return result;

	//printf("Car %d : Range = %lf\n",nodeLoc_.Nid,range);
	for(i = 1; i <= MaxNumOfNodeInNdt; ++i)
	{
		if(node_lst[i] == NULL)
		{
			/* This node belongs to the supernode. */
			continue;
		}

		if(node_lst[i]->nodeLoc_.Nid == nodeLoc_.Nid)
		{
			continue;
		}

		if((node_lst[i]->nodeLoc_.NodeType == AGENT) ||
				(node_lst[i]->nodeLoc_.NodeType == LARGE_SCALE_CAR))
		{
			// No need to care the car who is doing lane changing
			if(node_lst[i]->drivingInfo_->isDoingLaneChange == 1)
				continue;

			tempAngle = atan2(node_lst[i]->nodeLoc_.LocY_ - nodeLoc_.LocY_, node_lst[i]->nodeLoc_.LocX_ - nodeLoc_.LocX_);
			tempAngle = 360 - fmod(360 + (tempAngle*180/PI), 360); /* GUI direction */
			tempAngle = fmod(fabs(tempAngle - direction), 360);

			if(tempAngle > 180)
				tempAngle = 360 - tempAngle;

			if(tempAngle < range)
			{
				distance = twoPointsDistance(nodeLoc_.LocX_, nodeLoc_.LocY_, node_lst[i]->nodeLoc_.LocX_, node_lst[i]->nodeLoc_.LocY_);
				if(distance < tempDistance)
				{
					tempDistance = distance;
					x = node_lst[i]->nodeLoc_.LocX_;
					y = node_lst[i]->nodeLoc_.LocY_;
					z = node_lst[i]->nodeLoc_.LocX_;
					id = node_lst[i]->nodeLoc_.Nid;
					tempN = node_lst[id]->getNodeAngle(tDirection);
					result = tempDistance;
				}
			}
		}
	}
	//printf("Node %d Choose node %d  distance = %lf\n",nodeLoc_.Nid,id,tempDistance);
	/* If find, return the two nodes distance,
	 * If not find, return -1
	 */
	return (result);
}

int Node::checkIfSafeToChangeLane(int &lookTo, double &neighborDirection)
{
	/* lookTo = -1  -> look to left-hand side on GUI screen
	   lookTo = 1 -> look to right-hand side on GUI screen
	   */
	int nRight = 0, nLeft = 0;
	double lookDirection[4] = {-1, -1, -1, -1};
	double DistanceToRoadEnd;
	double tmpNeighborLaneDirection[2] = {0.0, 0.0};

	DistanceToRoadEnd = Distance_BetweenTwoNode(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->EndPosOfCurrentRoad_x, drivingInfo_->EndPosOfCurrentRoad_y);
	if(DistanceToRoadEnd <= 20)
	{
		// Not safe to change lane when close to the end of the current road
		lookTo = 0;
		return 0;
	}

	//printf("Look to right-hand side!!\n");
	nRight = getNeighboringLaneInformation(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, RIGHT, tmpNeighborLaneDirection[0], nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));
	if(nRight < 0)
	{
		printf("Warning_1_4: getNeighboringLaneInformation error!! Node %d position(%lf, %lf)\n", nodeLoc_.Nid, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y);
		fflush(stdout);
		//stopLargeScaleSimulation(nodeLoc_.Nid);
	}
	else if(nRight == 1)
	{
		// northeast
		lookDirection[0] = fmod(drivingInfo_->CurrentDirection - 45 + 360, 360);
		// southeast
		lookDirection[1] = fmod(drivingInfo_->CurrentDirection - 135 + 360, 360);
	}

	//printf("Look to left-hand side!!\n");
	nLeft = getNeighboringLaneInformation(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, LEFT, tmpNeighborLaneDirection[1], nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));
	if(nLeft < 0)
	{
		printf("Warning_1_5: getNeighboringLaneInformation error!! Node %d position(%lf, %lf)\n", nodeLoc_.Nid, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y);
		fflush(stdout);
		//stopLargeScaleSimulation(nodeLoc_.Nid);
	}
	else if(nLeft == 1)
	{
		// northwest
		lookDirection[2] = fmod(drivingInfo_->CurrentDirection + 45 + 360, 360);
		// southwest
		lookDirection[3] = fmod(drivingInfo_->CurrentDirection + 135 + 360, 360);
	}

	if((nRight == 1) || (nLeft == 1))
	{
		int notSafe[4] = {0, 0, 0, 0};
		double searchScope = 0; // meters

		searchScope = roadWidth_ + roadWidth_/4;

		int i;
		// i is equal to node ID
		for(i = 1; i <= MaxNumOfNodeInNdt; ++i)
		{
			if(notSafe[0] & notSafe[1] & notSafe[2] & notSafe[3])
			{
				// all directions are not safe
				return 0;
			}

			// skip other node type
			if((node_lst[i] == NULL) || (node_lst[i]->nodeLoc_.Nid == nodeLoc_.Nid) ||
				(node_lst[i]->nodeLoc_.NodeType == TRAFFIC_LIGHT_CONTROLLER) ||
				(node_lst[i]->nodeLoc_.NodeType == OTHER_NODE))
				continue;

			if(node_lst[i]->nodeLoc_.NodeType) // It's a car
			{
				double tempAngle, tempDirection;
				double angleDiff = 0;
				int j = 0;

				node_lst[i]->getNodeAngle(tempDirection);
				angleDiff = fabs(drivingInfo_->CurrentDirection - tempDirection);
				if(angleDiff > 0.1) // This node is not going to the same direction with me
					continue;

				tempAngle = atan2(node_lst[i]->nodeLoc_.LocY_ - nodeLoc_.LocY_, node_lst[i]->nodeLoc_.LocX_ - nodeLoc_.LocX_);
				tempAngle = 360 - fmod(360 + (tempAngle*180/PI), 360); /* GUI direction */

				// check northeast, northwest, southwest, and southeast directions
				for(j = 0; j < 4; ++j)
				{
					if(lookDirection[j] == -1)
						continue;
					tempAngle = fmod(fabs(tempAngle - lookDirection[j]), 360);

					if(tempAngle > 180)
						tempAngle = 360 - tempAngle;

					if(tempAngle < VISIBILITY_SCALE_IN_DEGREE)
					{
						double speed, distance;

						distance = Distance_BetweenTwoNode(nodeLoc_.LocX_, nodeLoc_.LocY_, node_lst[i]->nodeLoc_.LocX_, node_lst[i]->nodeLoc_.LocY_);
						if(distance < searchScope){
							node_lst[i]->getNodeSpeed(speed);
							if((j == 0) || (j == 2)){
								if(speed < drivingInfo_->CurrentVelocity - 0.1) {
									// Front car's speed is slower than me
									notSafe[j] = 1;
									if(nLeft <= 0)
									{
										/* right lane is not safe and
										 * there's no left lane to check
										 */
										return 0;
									}
									else
									{
										// keep checking other node
										break;
									}
								}
							} else {
								if(speed > drivingInfo_->CurrentVelocity + 0.1) {
									// Back car's speed is faster than me
									notSafe[j] = 1;
									if(nRight <= 0)
									{
										/* left lane is not safe and
										 * there's no right lane to check
										 */
										return 0;
									}
									else
									{
										// keep checking other node
										break;
									}
								}
							}
						}
						// one node can only mach one direction
						break;
					}
				}
			}
		}

		if((nRight == 1) && (notSafe[0] == 0) && (notSafe[1] == 0))
		{
			// It's safe to change to right lane.
			neighborDirection = tmpNeighborLaneDirection[0];
			lookTo = 1;
			return 1;
		}
		else if ((nLeft == 1) && (notSafe[2] == 0) && (notSafe[3] == 0))
		{
			// It's safe to change to left lane.
			neighborDirection = tmpNeighborLaneDirection[1];
			lookTo = -1;
			return 1;
		}
		else ;
	}


	lookTo = 0;

	// It's not safe to change lane
	return 0;
}

void Node::changeLane()
{
	/* lookTo = -1  -> look to left-hand side on GUI screen
	   lookTo = 1 -> look to right-hand side on GUI screen
	   */
	//printf("Car %d : changeLane\n", nodeLoc_.Nid);

	int n = 0;
	int lookTo = 0;
	double neighborDirection = 0;

	n = checkIfSafeToChangeLane(lookTo, neighborDirection);

	if(n == 1)
	{
		/*
		 * There's a lane next to me and no approaching car in that lane
		 */
		CarAgentEvent tmpEvent;
		double midDirection = fmod(drivingInfo_->CurrentDirection - lookTo*45 + 360, 360);
		double midDirectionArc;
		double delta_x, delta_y;
		double radius = roadWidth_/2; /* radius of a circle */

		midDirectionArc = (fmod(360 - midDirection, 360))*PI/180;

		// clear the event queue
		while(drivingInfo_->EventQueue.empty() != true)
			drivingInfo_->EventQueue.pop_front();

		delta_x = radius*sqrt(2)*cos(midDirectionArc);
		delta_y = radius*sqrt(2)*sin(midDirectionArc);

		// First event point
		tmpEvent.x = drivingInfo_->CurrentPos_x + delta_x;
		tmpEvent.y = drivingInfo_->CurrentPos_y + delta_y;
		tmpEvent.direction = midDirection;
		drivingInfo_->EventQueue.push_back(tmpEvent);

		// Second event point
		tmpEvent.x = drivingInfo_->CurrentPos_x + 2*delta_x;
		tmpEvent.y = drivingInfo_->CurrentPos_y + 2*delta_y;
		tmpEvent.direction = neighborDirection;
		drivingInfo_->EventQueue.push_back(tmpEvent);

		/*
		printf("DEBUG: changeLane lookTo %d, neighborDir %lf, CurPos (%lf, %lf), MidPos1 (%lf, %lf), MidDir %lf, MidPos2 (%lf, %lf)\n",
			lookTo, neighborDirection, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y,
			drivingInfo_->CurrentPos_x + delta_x, drivingInfo_->CurrentPos_y + delta_y, midDirection,
			drivingInfo_->CurrentPos_x + 2*delta_x, drivingInfo_->CurrentPos_y + 2*delta_y);
		*/
		node_lst[nodeLoc_.Nid]->setNodeAngle(midDirection);
		drivingInfo_->isDoingLaneChange = 1;
	}
}

void Node::getTeamInfo(int &TeamID,int &TeamLeader)
{
	TeamID = nodeLoc_.Tid;
	TeamLeader = nodeLoc_.TLeader;
	return;
}

void Node::getNearestPrecedingTeamMember(int &Preceding_Nid)
{
	int myTid, count;
	myTid = nodeLoc_.Tid;
	int team_num = Team_lst[myTid][0];

	for(count = 1; count <= team_num; count++)
	{
		if(Team_lst[myTid][count] == nodeLoc_.Nid)
			break;
	}
	Preceding_Nid = Team_lst[myTid][count - 1];

	/*
	printf("Node::getNearestPrecedingTeamMember : Tid = %d, Nid = %d, Preceding_Nid = %d\n",
			myTid,nodeLoc_.Nid,Preceding_Nid);
	printf("nid: %d  x:%lf y:%lf\n",nodeLoc_.Nid,nodeLoc_.LocX_,nodeLoc_.LocY_);
	printf("Teamlist :");

	for(count = 1; count <= team_num; count++)
	{
		printf("  %d",Team_lst[myTid][count]);
	}
	printf("\n");
	*/

	return;
}

void Node::getNearestFollowingTeamMember(int &Following_Nid)
{
	int myTid, count;
	myTid = nodeLoc_.Tid;
	for(count = 1; count <= Max_Member; count++)
	{
		if(Team_lst[myTid][count] == nodeLoc_.Nid)
			break;
	}
	Following_Nid = Team_lst[myTid][count+1];
	return;
}

void Node::getTeamTurningInfo(double &Turning_direction, int &Turning_block, int &Turning_times)
{
	Turning_direction = node_lst[nodeLoc_.Nid]->nodeLoc_.TurningDirection;
	Turning_block = node_lst[nodeLoc_.Nid]->nodeLoc_.TurningBlockNumber;
	Turning_times = node_lst[nodeLoc_.Nid]->nodeLoc_.TurningTimes;

	/*
	printf("get Turning_times in node nid = %d Turning_times = %d Turning_direction = %lf\n",
			nodeLoc_.Nid,Turning_times,Turning_direction);
	*/
	return;
}

void Node::setTeamTurningInfo(double Turning_direction, int Turning_block, int Turning_times)
{
	node_lst[nodeLoc_.Nid]->nodeLoc_.TurningDirection = Turning_direction;
	node_lst[nodeLoc_.Nid]->nodeLoc_.TurningBlockNumber = Turning_block;
	node_lst[nodeLoc_.Nid]->nodeLoc_.TurningTimes = Turning_times;

	/*
	printf("set Turning_times in node nid = %d Turning_times = %d Turning_direction = %lf\n",
			nodeLoc_.Nid,node_lst[nodeLoc_.Nid]->nodeLoc_.TurningTimes,Turning_direction);
	*/
	return;
}

void Node::getLaneChangeInfo(int &EnableLaneChange)
{
	EnableLaneChange = node_lst[nodeLoc_.Nid]->nodeLoc_.LaneSwitch;
}

int Node::SwitchOrderWithPrecedingCarInTeam()
{
	int i,tmp = 0;

	//printf("In Node::SwitchOrderWithPrecedingCarInTeam \n");

	for(i = 1 ; i <= Max_Member ; i++)
	{
		if(Team_lst[nodeLoc_.Tid][i] == nodeLoc_.Nid)
			break;
	}
	//printf("Nid:%d => I:%d PrecedingCar: %d\n",nodeLoc_.Nid,Team_lst[nodeLoc_.Tid][i],Team_lst[nodeLoc_.Tid][i-1]);


	tmp = Team_lst[nodeLoc_.Tid][i];
	Team_lst[nodeLoc_.Tid][i] = Team_lst[nodeLoc_.Tid][i-1];
	Team_lst[nodeLoc_.Tid][i-1] = tmp;

	i -= 1;

	return 1;
}

/* Error handler function */
void Node::resetCarStatus()
{
	/* Set car's location and moving direction to previous event status or default location */
	if(((drivingInfo_->CurrentEvent.x <= 0.000001) && (drivingInfo_->CurrentEvent.y <= 0.000001)))
	{
		// not on the road in the beginning
		stopLargeScaleSimulation(nodeLoc_.Nid);
	}
	else
	{
		drivingInfo_->CurrentDirection = drivingInfo_->CurrentEvent.direction;
		drivingInfo_->CurrentPos_x = drivingInfo_->CurrentEvent.x;
		drivingInfo_->CurrentPos_y = drivingInfo_->CurrentEvent.y;
	}
	node_lst[nodeLoc_.Nid]->setNodeCurrentWaypoint(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentPos_z);
	// set current moving direction viewed on GUI screen
	node_lst[nodeLoc_.Nid]->setNodeAngle(drivingInfo_->CurrentDirection);
	nodeLoc_.speed_ = 0.5;
	nodeLoc_.acceleration_ = 0.0;
}

void Node::UpdateNodeLocation()
{
	/*
	 * Node's dynamic moving behavior starts!!
	 */

	assert(drivingInfo_);

	node_lst[nodeLoc_.Nid]->getNodePosition(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentPos_z);

	// decide my search range of traffic light based on my speed
	double d;
	if(nodeLoc_.MaxVelocity > 20)
		d = 120.0;
	else    d = 80.0;

	int n = 0;

	//Search the Traffic light signal.
	n = getTheNearestPrecedingTrafficLight_LS(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentDirection, d, drivingInfo_->SigGID, drivingInfo_->SigLight, drivingInfo_->SigPOS_x, drivingInfo_->SigPOS_y, drivingInfo_->SigIndex);

	if(n == 1)
		drivingInfo_->SeenTheTrafficLightOrNot = 1; // see
	else
		drivingInfo_->SeenTheTrafficLightOrNot = 0; // no see

	if(drivingInfo_->EventQueue.size() > 0)
	{
		drivingInfo_->CurrentEvent = drivingInfo_->EventQueue.front();
	}
	node_lst[nodeLoc_.Nid]->getNodeSpeed(drivingInfo_->CurrentVelocity);

	int NearestPrecedingCarID = -1;
	double NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z;
	double NearestPrecedingCarDirection;
	double CurrentAcceleration = 0.5;
	double DistanceToNearestNode = -1;

	// get the preceding car information
	DistanceToNearestNode = getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreenInNode(drivingInfo_->CurrentDirection, VISIBILITY_SCALE_IN_DEGREE, VISIBILITY_SCALE_IN_DISTANCE, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID);

	if((nodeLoc_.LaneSwitch == 1) && (drivingInfo_->SeenTheTrafficLightOrNot == 0))
	{
		if(drivingInfo_->CurrentVelocity >= nodeLoc_.MaxVelocity)
			CurrentAcceleration = -0.1;
		else
			CurrentAcceleration = CurrentAcceleration; // simulate a crazy car
	}
	else
	{
		CurrentAcceleration = DetermineAcceleration(NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID);
	}

	node_lst[nodeLoc_.Nid]->setNodeSpeedAcceleration(CurrentAcceleration);

	if(DistanceToNearestNode >= 0)
	{
		/*
		if((DistanceToNearestNode <= 0.3) && (drivingInfo_->oldCollisionCarID != NearestPrecedingCarID))
		{
			printf("[%llu]: Collision of Car %d (%lf, %lf) and Car %d (%lf, %lf).\n",
			GetCurrentTime(), nodeLoc_.Nid,
			drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y,
			NearestPrecedingCarID, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);
			drivingInfo_->oldCollisionCarID = NearestPrecedingCarID;
		}
		*/
		if(nodeLoc_.LaneSwitch)  // CarAgent Editoin
		{
			if((DistanceToNearestNode <= 15) && (drivingInfo_->isDoingLaneChange == 0))
			{
				//printf("Node[%d] too close: wants to change lane\n", nodeLoc_.Nid);
				changeLane();
			}
		}
	}

	if(drivingInfo_->EventQueue.empty() == false)
	{
		//SEC_TO_TICK(nodeLoc_.update_expire,0.1);
		n = ReachTheNextTriggerPointOrNot();
		// n = 1; Reach the event point.
		// n = 0; Unreach the event point.
	}
	else
	{
		n = 1;
		drivingInfo_->isDoingLaneChange = 0;
	}

	if(n == 0)
	{
		//Not reach the event point, do nothing, keep going.
		SEC_TO_TICK(nodeLoc_.update_expire, 0.1);
		return;
	}
	else if(n == 1)
	{
		SEC_TO_TICK(nodeLoc_.update_expire, 0.01);

		// create new event queue.
		if(((int) drivingInfo_->EventQueue.size()) == 0) //End of taking turns, so the queue became empty.
		{
			double CorrectedPOS_x = 0;
			double CorrectedPOS_y = 0;
			double a, b, c; // linear equtation of ax + by + c = 0
			int numOfDirections;
			int RoadType;
			drivingInfo_->isDoingLaneChange = 0;

			n = selfCorrectness(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, CorrectedPOS_x, CorrectedPOS_y, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));

			if(n == 1)
			{
				double tmpDist = Distance_BetweenTwoNode(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, CorrectedPOS_x, CorrectedPOS_y);
				if(tmpDist < roadWidth_*1.5)
				{
					drivingInfo_->CurrentPos_x = CorrectedPOS_x;
					drivingInfo_->CurrentPos_y = CorrectedPOS_y;
				}
			}
			fflush(stdout);

			node_lst[nodeLoc_.Nid]->setNodeCurrentWaypoint(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentPos_z);

			/* get front node block ID which connected by current edge.
			 * Note:
			 *    node block ID is the same as signal group ID
			 */
			getFrontNID(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->SigGID,nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));

			double *CandidateDirection = NULL;

			n = getCurrentRoadInformation(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, numOfDirections, CandidateDirection, a, b, c, drivingInfo_->EndPosOfCurrentRoad_x, drivingInfo_->EndPosOfCurrentRoad_y, RoadType, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));
			if(n <= 0)
			{
				printf("error : n = %d, Warning_1_2: Node is not on the lane!! Node %d position(%lf, %lf), current direction %lf, currentSpeed %lf, eventPos (%lf, %lf)\n",
						n, nodeLoc_.Nid, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentDirection, drivingInfo_->CurrentVelocity, drivingInfo_->CurrentEvent.x, drivingInfo_->CurrentEvent.y);
				fflush(stdout);
				//stopLargeScaleSimulation(nodeLoc_.Nid);

				resetCarStatus();
				return;
			}

			// Handle the situation when the car is wake up in the intersection
			if(RoadType == ROAD_TYPE_NODE)
			{
				double distanceToNextEventPoint = Distance_BetweenTwoNode(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentEvent.x, drivingInfo_->CurrentEvent.y);
				double angleDiff = fabs(drivingInfo_->CurrentDirection - drivingInfo_->CurrentEvent.direction);
				if(angleDiff > 180)
					angleDiff = 360 - angleDiff;

				if(((drivingInfo_->CurrentEvent.x <= 0.000001) && (drivingInfo_->CurrentEvent.y <= 0.000001)) || ((distanceToNextEventPoint >= roadWidth_) && (angleDiff < 90)))
				{
					/*
					 * Node is in the node block at beginning ||
					 * Node skips one lane and directly enters into the node block
					 * (It happens when the road length is very small)
					 */
					double exitPOS_x, exitPOS_y, exitDirection;
					double nextDirection;

					n = getCurrentNodeExit(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, exitPOS_x, exitPOS_y, exitDirection, nextDirection, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));
					if(n < 0)
					{
						printf("Node[%d] getCurrentNodeExit error\n", nodeLoc_.Nid);
					}

					drivingInfo_->CurrentDirection = exitDirection;
					// set current moving direction viewed on GUI screen
					node_lst[nodeLoc_.Nid]->setNodeAngle(drivingInfo_->CurrentDirection);

					FillTheQueue(0, NULL, NULL, NULL, exitPOS_x, exitPOS_y, nextDirection);
				}
				else
				{
					/* After the turn, if car is still on the previous road,
					 * make this car go to the final point of the previous road one more time.
					 */
					FillTheQueue(0, NULL, NULL, NULL, drivingInfo_->CurrentEvent.x, drivingInfo_->CurrentEvent.y, drivingInfo_->CurrentEvent.direction);
				}
				free(CandidateDirection);
				CandidateDirection = NULL;
				return;
			}

			drivingInfo_->CurrentDirection = RandomDirection(numOfDirections, CandidateDirection);
			free(CandidateDirection);
			CandidateDirection = NULL;

			// set current moving direction viewed on GUI screen
			node_lst[nodeLoc_.Nid]->setNodeAngle(drivingInfo_->CurrentDirection);

			n = getNextRoadInformation(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentDirection, numOfDirections, CandidateDirection, a, b, c, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));

			if(n <= 0){
				printf("Warning_1_3: Cannot find the next road!! Node %d position(%lf, %lf)\n", nodeLoc_.Nid, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y);
				fflush(stdout);
				stopLargeScaleSimulation(nodeLoc_.Nid);
			}

			if(numOfDirections == 0){
				printf("Error: numOfDirections can't be zero\n");
				fflush(stdout);
				stopLargeScaleSimulation(nodeLoc_.Nid);
			}

			double ExpectedDirection = 0.0;

			ExpectedDirection = RandomDirection(numOfDirections, CandidateDirection);

			free(CandidateDirection);
			CandidateDirection = NULL;

			int numOfTurns = 0;
			double *turningPOS_x = NULL, *turningPOS_y = NULL, *DirectionQueue = NULL;
			double FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y;

			/* Get next point and direction */
			n = takeATurn(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentDirection, ExpectedDirection, numOfTurns, turningPOS_x, turningPOS_y, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, DirectionQueue, nodeLoc_.CacheOrNot, &(drivingInfo_->myCache));
			if(n <= 0)
			{
				printf("Error: cannot get the taking turns info, error no. %d, pos (%lf, %lf), currentDir %lf,  expectDir %lf\n", n, drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentDirection, ExpectedDirection);
				fflush(stdout);
				stopLargeScaleSimulation(nodeLoc_.Nid);
			}

			/* Fill the next point and direction into the event queue */
			FillTheQueue(numOfTurns, turningPOS_x, turningPOS_y, DirectionQueue, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, ExpectedDirection);

			delete turningPOS_x;
			delete turningPOS_y;
			delete DirectionQueue;
		}
		else if(((int)drivingInfo_->EventQueue.size()) > 0)
		{
			//Pop an item from the queue
			drivingInfo_->CurrentDirection = drivingInfo_->CurrentEvent.direction;
			drivingInfo_->CurrentPos_x = drivingInfo_->CurrentEvent.x;
			drivingInfo_->CurrentPos_y = drivingInfo_->CurrentEvent.y;
			node_lst[nodeLoc_.Nid]->setNodeCurrentWaypoint(drivingInfo_->CurrentPos_x, drivingInfo_->CurrentPos_y, drivingInfo_->CurrentPos_z);
			// set current moving direction viewed on GUI screen
			node_lst[nodeLoc_.Nid]->setNodeAngle(drivingInfo_->CurrentDirection);
		}
	}
	else
	{
		printf("CarAgent: Error for reached or not, n %d\n", n);
		fflush(stdout);
		stopLargeScaleSimulation(nodeLoc_.Nid);
	}
}

void Node::LS_TrafficLightInit()
{
	int groupID, n;
	int mynid = 0;

	if(sigInfo_ != NULL)
	{
		printf("%s warning: double init of TrafficLightController %d\n", __func__, get_nid());
		delete sigInfo_;
	}

	sigInfo_ = new (struct signalInfo);

	mynid = nodeLoc_.Nid;
	groupID = nodeLoc_.signalGroupID;

	if(groupID < 0)
	{
		// If no signal group ID in .ndt file, get an unused signal group ID.
		n = getSignalGroupID_LS(groupID);
		if(n <= 0){
			printf("Error in %s:: getSignalGroupID_LS fail!! \n", __func__);
			stopLargeScaleSimulation(mynid);
		}
	}

	printf("TrafficLightController[%u]: groupID is %d\n", mynid, groupID);

	n = getSignalsInTheSameGroup_LS(&(sigInfo_->sameG), groupID);
	if(n <= 0){
		printf("Error in %s:: getSignalsInTheSameGroup_LS fail!! \n", __func__);
		stopLargeScaleSimulation(mynid);
	}

	/* Initialize the status of traffic lights */
	for(int i=0; i<((sigInfo_->sameG).numOfSigs); ++i){
		if(((sigInfo_->sameG).facingDirection[i] == (sigInfo_->sameG).facingDirection[0]) ||
				((sigInfo_->sameG).facingDirection[i] == fmod((sigInfo_->sameG).facingDirection[0]+180,360))){
			setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, GREEN);
			(sigInfo_->sameG).light[i] = GREEN;
		}
		else{
			setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, RED);
			(sigInfo_->sameG).light[i] = RED;
		}
	}

	/*
	for(int j=0; j<(sigInfo_->sameG).numOfSigs; ++j)
	{
		printf("[%d] light %d, direction %f\n",
			groupID, (sigInfo_->sameG).light[j], (sigInfo_->sameG).facingDirection[j]);
	}
	*/

	sigInfo_->changeLightCount = 1;
	sigInfo_->nextCountToChangeLight = TRAFFIC_LIGHTS_PERIOD; // initial time period of changing light
	sigInfo_->changeSpecifiedTrafficLight = 1;

	// Enable Register Information to WSM
	cmd_server_->agentEnableRegInfo(-1, mynid, 0); // fd, nid, pid

	// Fill in the WSMP header
	mywsmp.type			= WAVE_SHORT_MESSAGE_PROTOCOL;
	mywsmp.moreMsgFollowing	= 1;
	mywsmp.nid			= mynid;
	// Set WSM header
	mywsmp.wsm_header.wsm_version	= 0;
	mywsmp.wsm_header.header_len	= 9;
	mywsmp.wsm_header.header_cont	= 0;
	mywsmp.wsm_header.security_type	= 0;
	mywsmp.wsm_header.channel_num	= 174;  // sch 174,175,176,180,182,182 : cch 178
	mywsmp.wsm_header.data_rate	= 13;   // 13:3, 15:4.5, 5:6, 7:9, 9:12, 11:18, 1:24, 3:27 (index:Mbps)
	mywsmp.wsm_header.txpwr_level	= 1;
	mywsmp.wsm_header.psid		= mynid;
	mywsmp.wsm_header.expiry_time	= 0;
	mywsmp.wsm_header.wsm_len	= WsmMaxLength;
}

void Node::UpdateTrafficLight()
{
	int mynid = nodeLoc_.Nid;

	if(!callTrafficLightInit)
	{
		// initialize the traffic light
		LS_TrafficLightInit();
		callTrafficLightInit = 1;
	}

	assert(sigInfo_);

	if((sigInfo_->changeSpecifiedTrafficLight == 0) && ((sigInfo_->changeLightCount % sigInfo_->nextCountToChangeLight) == 0))
	{
		for(int i = 0; i < (sigInfo_->sameG).numOfSigs; ++i){
			if(((sigInfo_->sameG).facingDirection[i] == (sigInfo_->sameG).facingDirection[0]) ||
					((sigInfo_->sameG).facingDirection[i] == fmod((sigInfo_->sameG).facingDirection[0]+180,360)))
			{
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, GREEN);
				(sigInfo_->sameG).light[i] = GREEN;
			}
			else
			{
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, RED);
				(sigInfo_->sameG).light[i] = RED;
			}
		}

		sigInfo_->changeLightCount = 0;
		sigInfo_->nextCountToChangeLight = TRAFFIC_LIGHTS_PERIOD; // time period of changing light
		sigInfo_->changeSpecifiedTrafficLight = 1;
#define SHOWLIGHT 0
#if SHOWLIGHT
		printf("Signal [%d]: light1 %d, light2 %d, light3 %d, light4 %d\n", mynid,
				(sigInfo_->sameG).light[0], (sigInfo_->sameG).light[1], (sigInfo_->sameG).light[2], (sigInfo_->sameG).light[3]);
#endif

	}
	else if((sigInfo_->changeSpecifiedTrafficLight == 1) && ((sigInfo_->changeLightCount % sigInfo_->nextCountToChangeLight) == 0))
	{
		for(int i=0; i<(sigInfo_->sameG).numOfSigs; ++i){
			if(((sigInfo_->sameG).facingDirection[i] == (sigInfo_->sameG).facingDirection[0]) ||
					((sigInfo_->sameG).facingDirection[i] == fmod((sigInfo_->sameG).facingDirection[0]+180,360))){
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, YELLOW);
				(sigInfo_->sameG).light[i] = YELLOW;
			}
			else{
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, RED);
				(sigInfo_->sameG).light[i] = RED;
			}
		}
		sigInfo_->changeLightCount = 0;
		sigInfo_->nextCountToChangeLight = 5; // time period of changin light
		sigInfo_->changeSpecifiedTrafficLight = 2;
#if SHOWLIGHT
		printf("Signal [%d]: light1 %d, light2 %d, light3 %d, light4 %d\n", mynid,
				(sigInfo_->sameG).light[0], (sigInfo_->sameG).light[1], (sigInfo_->sameG).light[2], (sigInfo_->sameG).light[3]);
#endif
	}
	else if((sigInfo_->changeSpecifiedTrafficLight == 2) && ((sigInfo_->changeLightCount % sigInfo_->nextCountToChangeLight) == 0))
	{
		for(int i=0; i<(sigInfo_->sameG).numOfSigs; ++i){
			if(((sigInfo_->sameG).facingDirection[i] == (sigInfo_->sameG).facingDirection[0]) ||
					((sigInfo_->sameG).facingDirection[i] == fmod((sigInfo_->sameG).facingDirection[0]+180,360))){
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, RED);
				(sigInfo_->sameG).light[i] = RED;
			}
			else{
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, GREEN);
				(sigInfo_->sameG).light[i] = GREEN;
			}
		}
		sigInfo_->changeLightCount = 0;
		sigInfo_->nextCountToChangeLight = TRAFFIC_LIGHTS_PERIOD; // time period of changing light
		sigInfo_->changeSpecifiedTrafficLight = 3;
#if SHOWLIGHT
		printf("Signal [%d]: light1 %d, light2 %d, light3 %d, light4 %d\n", mynid,
				(sigInfo_->sameG).light[0], (sigInfo_->sameG).light[1], (sigInfo_->sameG).light[2], (sigInfo_->sameG).light[3]);
#endif
	}
	else if((sigInfo_->changeSpecifiedTrafficLight == 3) && ((sigInfo_->changeLightCount % sigInfo_->nextCountToChangeLight) == 0))
	{
		for(int i=0; i<(sigInfo_->sameG).numOfSigs; ++i){
			if(((sigInfo_->sameG).facingDirection[i] == (sigInfo_->sameG).facingDirection[0]) ||
					((sigInfo_->sameG).facingDirection[i] == fmod((sigInfo_->sameG).facingDirection[0]+180,360))){
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, RED);
				(sigInfo_->sameG).light[i] = RED;
			}
			else{
				setSignalLight_LS(mynid, (sigInfo_->sameG).sigGID, i, YELLOW);
				(sigInfo_->sameG).light[i] = YELLOW;
			}
		}
		sigInfo_->changeLightCount = 0;
		sigInfo_->nextCountToChangeLight = 5; // time period of changing light
		sigInfo_->changeSpecifiedTrafficLight = 0;
#if SHOWLIGHT
		printf("Signal [%d]: light1 %d, light2 %d, light3 %d, light4 %d\n", mynid,
				(sigInfo_->sameG).light[0], (sigInfo_->sameG).light[1], (sigInfo_->sameG).light[2], (sigInfo_->sameG).light[3]);
#endif
	}

	sigInfo_->changeLightCount++;
}

