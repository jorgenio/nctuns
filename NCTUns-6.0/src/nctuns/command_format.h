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

#ifndef __NCTUNS_COMMAND_FORMAT_H__
#define __NCTUNS_COMMAND_FORMAT_H__

#include <arpa/inet.h>
#include <map>

/*
 * Define the IPC commands exchanged between agent clients and simulation 
 * engine
 */
#define GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE		0x01
#define AGENT_CLIENT_INFO						0x02
#define AGENT_CLIENT_SET_CURRENT_WAYPOINT				0x03
#define AGENT_CLIENT_GET_CURRENT_WAYPOINT				0x04
#define AGENT_CLIENT_GET_CURRENT_WAYPOINT_REPLY				0x05
#define AGENT_CLIENT_SET_NEXT_WAYPOINT					0x06
#define AGENT_CLIENT_SET_CURRENT_AND_NEXT_WAYPOINT			0x07
#define AGENT_CLIENT_SET_CURRENT_MOVING_SPEED				0x08
#define AGENT_CLIENT_GET_CURRENT_MOVING_SPEED				0x09
#define AGENT_CLIENT_GET_CURRENT_MOVING_SPEED_REPLY			0x0a
#define AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION			0x0b
#define AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION_REPLY			0x0c
#define AGENT_CLIENT_SET_NEXT_WAYPOINT_AND_MOVING_SPEED			0x0d
#define AGENT_CLIENT_GET_CURRENT_POSITION				0x0e
#define AGENT_CLIENT_GET_CURRENT_POSITION_REPLY				0x0f
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE		0x10
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_REPLY	0x11
#define AGENT_CLIENT_REQUEST_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME 0x12
#define AGENT_CLIENT_CANCEL_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME 0x13
#define AGENT_CLIENT_SUSPEND_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME 0x14
#define AGENT_CLIENT_RESUME_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME 0x15
#define SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME 0x16
#define AGENT_CLIENT_DESTROY_MOBILE_NODE				0x17
#define AGENT_CLIENT_STOP_SIMULATION					0x18
#define AGENT_CLIENT_RELEASE_CPU 					0x19		

/* 2007/02/27 added by automatic vehicle group */
#define AGENT_CLIENT_SET_CURRENT_SPEED_ACCELERATION			0x1a
#define AGENT_CLIENT_SET_CURRENT_MOVING_DIRECTION			0x1b

#define AGENT_CLIENT_GET_NODE_MAX_SPEED					0x20
#define AGENT_CLIENT_GET_NODE_MAX_SPEED_REPLY				0x21
#define AGENT_CLIENT_GET_SIGNAL_LIGHT					0x22
#define AGENT_CLIENT_GET_SIGNAL_LIGHT_REPLY				0x23
#define AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP			0x24
#define AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY		0x25
#define AGENT_CLIENT_SET_SIGNAL_LIGHT					0x26
#define AGENT_CLIENT_REPORT_STATUS					0x27 
#define AGENT_CLIENT_GET_SIGNAL_GROUPID					0x28
#define AGENT_CLIENT_GET_SIGNAL_GROUPID_REPLY				0x29
#define AGENT_CLIENT_IS_A_BROKEN_CAR					0x2a
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE		0x2b
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE_REPLY	0x2c

/* 2008/12/8 added by S.W.Chuang for Large-Scale simulation*/
#define SE_SEND_RESULT_TO_NODE						0x2d
#define LargeScaleCar							0x2e
#define AGENT_CLIENT_GET_TEAM_INFO					0x32
#define AGENT_CLIENT_GET_TEAM_INFO_REPLY				0x33
#define AGENT_CLIENT_GET_TEAM_TURNING_INFO				0x34
#define AGENT_CLIENT_GET_TEAM_TURNING_INFO_REPLY			0x35
#define AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER			0x36
#define AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER_REPLY		0x37
#define AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER			0x38
#define AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER_REPLY		0x39
#define AGENT_CLIENT_GET_LANE_CHANGE_INFO				0x3a
#define AGENT_CLIENT_GET_LANE_CHANGE_INFO_REPLY				0x3b
#define AGENT_CLIENT_SET_TEAM_TURNING_INFO				0x3c
#define AGENT_CLIENT_SET_TEAM_TURNING_INFO_REPLY			0x3d
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE		0x3e
#define AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE_REPLY	0x3f
#define AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR	0x40
#define AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR_REPLY	0x41

/*
 * Define the IPC commands exchanged between simulation engine and the GUI
 */
#define	GUI_REQUEST_LOCATION				0x01
#define SE_SEND_LOC_TO_GUI				0x02
#define GUI_CHANGE_NODE_SPEED				0x03
#define GUI_CHANGE_NODE_MOVING_DIRECTION		0x04
#define SE_SEND_PTR_TO_GUI				0X05
#define SE_SEND_LIGHT_TO_GUI				0x06

#define RSUAGENT_REPORT_WARNING				0x30
#define CARAGENT_REPORT_WARNING				0x31

//
#define WAVE_SHORT_MESSAGE_PROTOCOL			0x2d
#define WAVE_AGENT_ENABLE_REG_INFO			0x2e
#define GET_SE_TIME					0x2f

/* Define the connection types and port numbers used by simulation engine, 
   GUI, and agent clients.
 */
#define SE_IPC_SERVER_PORT_FOR_AGENT	  	        "3353"
#define SE_IPC_SERVER_UDP_PORT_FOR_AGENT_NOTIFICATION	        "3000"
#define GUI_UDP_PORT					"4004"		
#define GUI_ADDRESS					"127.0.0.1"
#define MAX_RECV_BYTES          			8192
#define MESSAGE_SIZE					8192
#define IP_PACKET_MAX_LEN				60000

/*
 *  Define IPC process type
 */
#define PROCESS_TYPE_AGENT				0x01
#define PROCESS_TYPE_WSM				0x02

/*
 * Define WSM Max Data Length
 */
#define WsmMaxLength					1400

struct typeChecker {
	u_int32_t 	type;
};

struct NodeLocationHeader {
	u_int32_t	type;
	int		num;	
};

struct NodeLocation {
	u_int32_t	nid;
	double		timeStamp;
	double		x;
	double		y;
	double		z;
};

struct GUIChangeNodeSpeed {
	u_int32_t	type;
	/* GUI_CHANGE_NODE_SPEED */
	u_int32_t	nid;
	double		speed;
};

struct GUIChangeNodeMovingDirection {
	u_int32_t	type;
	/* GUI_CHANGE_NODE_MOVING_DIRECTION */
	u_int32_t	nid;
	double		angle;	
};

struct agentClientInfo {
	u_int32_t	type;  
	/* AGENT_CLIENT_INFO */
	u_int32_t	moreMsgFollowing;  
	u_int32_t	nid;
	u_int32_t	pid;
	u_int16_t	udpportnum;
	u_int32_t	humanControlled;
	int		socket_fd;
	u_char		group1;
	u_char		group2;
	u_char		group3;
	u_char		group4;
	u_char		group5;
	u_char		group6;
	u_char		group7;
	u_char		group8;
	u_int16_t	process_type;
	/* Simulation engine uses 
	   GeneralACKBetweenAgentClientAndSimulationEngine to ACK this IPC 
	   command. 
         */
};

/* for Large Scale Simulation */
struct LSCarInfo {
	u_int32_t	type;
	u_int32_t	nid;
	u_char		group1;
	u_char		group2;
	u_char		group3;
	u_char		group4;
	u_char		group5;
	u_char		group6;
	u_char		group7;
	u_char		group8;
};

struct GeneralACKBetweenAgentClientAndSimulationEngine {
	u_int32_t	type; 
	/* GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE */
	int		result;	// -1: error
				//  0: success
	u_int32_t	moreMsgFollowing;  
	/* When an agent program receives a notification message sent by
	   the simulation engine and uses this struct to send back an ACK,
	   it can set moreMsgFollowing to a value > 0 to indicate that 
	   it will immediately issue an IPC command to the simulation engine
	   after receiving this notification.
         */
};

struct SESendResultToNode {
	u_int32_t	type;
	int		result;
	// -1: error
	//  0: success
};

struct agentClientSetCurrentWayPoint {
	u_int32_t	type;  
	/* AGENT_CLIENT_SET_CURRENT_WAYPOINT */
	u_int32_t	moreMsgFollowing;  
	u_int32_t	nid;
	double		x;
	double		y;
	double		z;
};

struct agentClientGetCurrentWayPoint {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_WAYPOINT */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
};

struct agentClientGetCurrentWayPointReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_WAYPOINT_REPLY */
        int             result; // -1: error
                                //  0: success
        double          x;
        double          y;
        double          z;
};

struct agentClientSetNextWayPoint {
        u_int32_t       type;  
	/* AGENT_CLIENT_SET_NEXT_WAYPOINT */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          x;
        double          y;
        double          z;
};

struct agentClientGetCurrentMovingDirection {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
};
                                                                                                                            
struct agentClientGetCurrentMovingDirectionReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION_REPLY */
	int             result; // -1: error
			//  0: success
        double          angle; /* in degrees */
};

struct agentClientSetCurrentAndNextWayPoint {
        u_int32_t       type;  
	/* AGENT_CLIENT_SET_CURRENT_AND_NEXT_WAYPOINT */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          curx;
        double          cury;
        double          curz;
        double          nextx;
        double          nexty;
        double          nextz;
};

struct agentClientSetCurrentMovingSpeed {
        u_int32_t       type;  
	/* AGENT_CLIENT_SET_CURRENT_MOVING_SPEED */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          speed; /* meter/sec */
};

struct agentClientGetCurrentMovingSpeed {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_MOVING_SPEED */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
};

struct agentClientGetCurrentMovingSpeedReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_MOVING_SPEED_REPLY */
        int             result; // -1: error
                                //  0: success
	double          speed; /* meter/sec */
};

struct agentClientSetNextWayPointAndMovingSpeed {
	u_int32_t       type;  
	/* AGENT_CLIENT_SET_NEXT_WAYPOINT_AND_MOVING_SPEED */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          nextx;
        double          nexty;
        double          nextz;
	double 		speed;
};

struct agentClientGetCurrentPosition {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
};

struct agentClientGetCurrentPositionReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION_REPLY */
	int             result; // -1: error
			//  0: success
	double		x;
	double		y;
	double		z;
};

struct agentClientGetCurrentPositionOfAGroupOfNode {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE */
        u_int32_t       moreMsgFollowing;
	u_int32_t       gid;
};
struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE */
        u_int32_t       moreMsgFollowing;
	u_int32_t       gid;
	double		distance;
	double 		CurrentPOS_x, CurrentPOS_y;
};

struct nodePosition {
        u_int32_t	nid;
        double          x;
        double          y;
        double          z;
};

struct agentClientGetCurrentPositionOfAGroupOfNodeReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_REPLY */
        int             result; // -1: error
                                //  0: success
        int		numNode;
	/*  There are numNode records of struct nodePosition starting here. */
};
struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_REPLY */
        int             result; // -1: error
                                //  0: success
        int		numNode;
	/*  There are numNode records of struct nodePosition starting here. */
};

#define TRIGGER_TYPE_NODE_APPROACHING 0x01

struct registeredApproachingCheckingRecord {
	int 		triggerType;
	/* Currently, only TRIGGER_TYPE_NODE_APPROACHING */
        int		nid;
        int 		registrationID;
	int		timeIntervalInMilliseconds; // periodic notification time interval 
						    // 0: as soon as possible
	int		suspendedflag;	// 1: suspended, 0: normal 
	u_int64_t	resumeTime; 	// the time when the suspension should be ended, 
					// timestamp 0 means forever when suspendedflag is 1
        int 		anid;
        int 		gid;
	double		withinRangeinMeter;
};

typedef std::multimap<u_int64_t, registeredApproachingCheckingRecord *> TRIGGER_MAP;
typedef TRIGGER_MAP::iterator  TrigMapItor;

struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe {
        u_int32_t       type;
        /* AGENT_CLIENT_REQUEST_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        u_int32_t       registrationID;
	int		timeIntervalInMilliseconds; // periodic notification time interval 
        int		anid;		// the ID of another node, -1 means 
					// gid should be used instead 
        int 		gid;		// the ID of a group, -1 means gid 
					// should be ignored
	double		withinRangeinMeter;
	/* Simulation engine uses 
	   GeneralACKBetweenAgentClientAndSimulationEngine to ACK this IPC 
	   command. 
         */
};

struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe {
        u_int32_t       type;
        /* AGENT_CLIENT_CANCEL_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        u_int32_t       registrationID;
};

struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe {
        u_int32_t       type;
        /* AGENT_CLIENT_SUSPEND_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        u_int32_t       registrationID;
	int		timeInterval;	// > 0: automatically resumed after the specified time interval (milliseconds) 
					// <= 0: should be suspended forever unless receiving the resume command
};

struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe {
        u_int32_t       type;
        /* AGENT_CLIENT_RESUME_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        u_int32_t       registrationID;
};

struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe {
        u_int32_t       type;
        /* SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME */
        u_int32_t       anid;
        double          x;
        double          y;
        double          z;
	double		speed;
	double		angle;
	double		distance;
        /* The agent program uses 
	   GeneralACKBetweenAgentClientAndSimulationEngine to ACK this IPC 
           command. */
};

struct agentClientDestroyMobileNode {
        u_int32_t       type;
        /* AGENT_CLIENT_DESTROY_MOBILE_NODE */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
	/* Simulation engine uses 
	   GeneralACKBetweenAgentClientAndSimulationEngine to ACK this IPC 
	   command. 
         */
};

struct agentClientStopSimulation {
        u_int32_t       type;
        /* AGENT_CLIENT_STOP_SIMULATION	*/
        u_int32_t       nid;
};

struct agentClientReleaseCPU {
        u_int32_t       type;
        /* AGENT_CLIENT_RELEASE_CPU */
        u_int32_t       nid;
};

/* 2007/2/27 added by automatic vehicle group */
struct agentClientSetCurrentSpeedAcceleration {
        u_int32_t       type;
        /* AGENT_CLIENT_SET_CURRENT_SPEED_ACCELERATION */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          acceleration; /* meter/sec^2 */
};

struct agentClientSetCurrentMovingDirection {
        u_int32_t       type;
        /* AGENT_CLIENT_SET_CURRENT_MOVING_DIRECTION */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
        double          angle; /* in degrees */
};

struct agentClientSetCurrentClockWiseAndCenterOfCircle{
	u_int32_t	type;
	/* AGENT_CLIENT_SET_CURRENT_CENTER_OF_CIRCLE */
	u_int32_t	moreMsgFollowing;
	u_int32_t	nid;
	int		clockWise;
	double		x;
	double		y;
	double		z;
};

struct agentClientGetNodeMaxSpeed {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_NODE_MAX_SPEED */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
};

struct agentClientGetNodeMaxSpeedReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_NODE_MAX_SPEED_REPLY */
        int             result; // -1: error
                        //  0: success
        double		maxSpeed;
};

struct agentClientReportStatus{
	u_int32_t	type;
	u_int32_t	moreMsgFollowing;
	u_int32_t	nid;
	double		x;
	double 		y;
	double 		acceleration;
	double 		speed;
	double 		direction;
	double 		timeStamp;
	int		TTL;
	int		seqNum;

	//Just for paper use.
	inline agentClientReportStatus &operator=( const agentClientReportStatus temp);
};

struct RSUAgentReportWarning{
	u_int32_t       type;
	u_int32_t       moreMsgFollowing;
	//u_int32_t       nid;
	u_int32_t       RSUnid;
	int AccelerationOrDeceleration;
	
};
struct CarAgentReportWarning{
	u_int32_t       type;
	u_int32_t       moreMsgFollowing;
	//u_int32_t       nid;
	u_int32_t       CarID;
	double 		deceleration;
	
};

struct agentClientGetSignalGroupID{
	u_int32_t	type;
	/* AGENT_CLIENT_GET_SIGNAL_GROUPID */
	u_int32_t	moreMsgFollowing;
	u_int32_t	nid;
};

struct agentClientGetSignalGroupIDReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_SIGNAL_GROUPID_REPLY */
        int             result; // -1: error
                        //  0: success
	int 		numOfSigs;
        int		sigGID;	//signal group ID
};

struct agentClientGetSignalsInTheSameGroup{
	u_int32_t	type;
	/* AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP */
	u_int32_t	moreMsgFollowing;
	u_int32_t	nid;
	int		sigGID;	// signal group ID
};

struct agentClientGetSignalsInTheSameGroupReply{
	u_int32_t	type;
	/* AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY */
	int		result;
	int		sigGID;	// signal group ID
	int		signalType;
	int		numOfSigs;
	int		light[4];
	double		x[4], y[4], facingDirection[4];
};

struct agentClientGetSignalLight {
	u_int32_t	type;
	/* AGENT_CLIENT_GET_SIGNAL LIGHT */
	u_int32_t	moreMsgFollowing;
	u_int32_t	nid;
	double		x;
	double		y;		
	double		myDirection;
	double		distance;
	int		sigGID;
};

struct agentClientGetSignalLightReply {
        u_int32_t       type;
        /* AGENT_CLIENT_GET_SIGNAL_LIGHT_REPLY */
        int             result; // 0: fail
                        //  1: success
        int		light;
	double		sig_x;
	double		sig_y;
	int		signalIndex;
};

struct agentClientSetSignalLight {
        u_int32_t       type;  
	/* AGENT_CLIENT_SET_SIGNAL_LIGHT */
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
	int		sigGID;		// signal group ID
	int		signalIndex;	// signal index in the specified signal
        int	        light;		// traffic light to be set
};
struct WSM_Header {
	uint8_t		wsm_version;
	uint8_t		header_len;
	uint8_t		header_cont;
	uint8_t		security_type;
	uint8_t		channel_num;
	uint8_t		data_rate;
	uint8_t		txpwr_level;
	uint32_t	psid;			// spec use 1 byte may be error
	uint64_t	expiry_time;
	uint16_t	wsm_len;

};
struct WaveShortMessageProtocol {
	u_int32_t       type;		//WAVE_SHORT_MESSAGE_PROTOCOL
	u_int32_t       moreMsgFollowing;
	u_int32_t       nid;
	struct WSM_Header	wsm_header;
	/*
	u_char		WSM_Version;
	u_char		SecurityType;
	u_char		ChannelNumber;
	u_char		DateRate;
	u_char		TxPwrLevel;
	u_int32_t	PSID;
	u_int16_t	WSM_Length;
	*/
};

struct WaveAgentEnableRegInfo {
	u_int32_t       type;		//WAVE_AGENT_ENABLE_REG_INFO
        u_int32_t       moreMsgFollowing;
        u_int32_t       nid;
	pid_t		pid;
};

struct GetSETime {
	u_int32_t       type;
	u_int32_t       moreMsgFollowing;
	u_int32_t       nid;
	u_int64_t	time;
};

/* Added By Icchen, Ssuying for Group Mode */
struct agentClientGetLaneChangeInfo {
	u_int32_t	type;
	int		nid;
	int		moreMsgFollowing;
};

struct agentClientGetLaneChangeInfoReply {
	u_int32_t	type;
	int		result;
	int		EnableLaneChange;
};

struct agentClientGetTeamInfo {
	u_int32_t	type;
	int		nid;
	int	moreMsgFollowing;
};

struct agentClientGetTeamInfoReply {
	u_int32_t	type;
	int		result;
	int		TeamID;
	int		TeamLeader;
};

struct agentClientGetTeamTurningInfo {
	u_int32_t	type;
	int		nid;
	int		moreMsgFollowing;
};

struct agentClientGetTeamTurningInfoReply {
	u_int32_t	type;
	int		result;
	double		TurningDirection;
	int		TurningBlockNumber;
	int		TurningTimes;
};

struct agentClientSetTeamTurningInfo {
	u_int32_t	type;
	int		nid;
	double		TurningDirection;
	int		TurningBlockNumber;
	int		TurningTimes;
	int		moreMsgFollowing;
};

struct agentClientSetTeamTurningInfoReply {
	u_int32_t	type;
	int		result;
};

struct agentClientGetNearestPrecedingTeamMember {
	u_int32_t	type;
	int		nid;
	int		moreMsgFollowing;
};

struct agentClientGetNearestPrecedingTeamMemberReply {
	u_int32_t	type;
	int		result;
	int		Preceding_Nid;
};

struct agentClientGetNearestFollowingTeamMember {
	u_int32_t	type;
	int		nid;
	int		moreMsgFollowing;
};

struct agentClientGetNearestFollowingTeamMemberReply {
	u_int32_t	type;
	int		result;
	int		Following_Nid;
};

struct agentClientGetCurrentPositionOfATeamOfNode {
	u_int32_t	type;
	int		tid;
	int		moreMsgFollowing;

};

struct agentClientGetCurrentPositionOfATeamOfNodeReply{
	u_int32_t	type;
	int		result;
	// -1: error
	//  0: success
	int		numNode;

};

struct agentClientSwitchOrderInTeamListWithPrecedingCar {
	u_int32_t	type;
	int		nid;
	int		moreMsgFollowing;
};

struct agentClientSwitchOrderInTeamListWithPrecedingCarReply {
	u_int32_t	type;
	int		result;
};

/*
 * This struct can be changed to whatever content you want to send.
 *
 * Note:
 * 	The size of WsmDataContent can't be more than WsmMaxLength (1400) bytes.
 */
struct WsmDataContent {
	u_int32_t	type;		// message data type
	int		edgeSerial;
	int		laneSerial;
	int		SigGID;		// signal's group ID
	int		SigIndex;	//index of which traffic light in the intersection.
};

#ifndef LOGOBJECT
#define LOGOBJECT
struct LogObject {
        u_char          PROTO;
        u_char          Event;
        union {
                u_int64_t       Time;
                u_int64_t       sTime;
        };
        u_int32_t       diff;
        union {
                 u_char          FrameType;
                 u_char          BurstType;
        };
        u_int32_t       IP_Src;
        u_int32_t       IP_Dst;
	u_int32_t       PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       MAC_Dst;		
        union {
                u_int64_t       FrameID;
                u_int64_t       BurstID;
                u_int64_t       ConnID;
        };
        union {
                u_int32_t       FrameLen;
                u_int32_t       BurstLen;
        };
       	u_int32_t       RetryCount;
        u_char          DropReason;
        u_char          Channel;
        u_int32_t       pad;
};

struct PTR_Header {
        u_char          type;
        int             num;
};

#define MAX_LOG_NUM_PER_PACKET 2048
struct PtrPacket {
        char type;
        int num;
        struct LogObject LogObjectArray[MAX_LOG_NUM_PER_PACKET];
};

#endif

#endif
