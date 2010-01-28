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

#ifndef	__NCTUNS_node_h__
#define __NCTUNS_node_h__

#include <object.h>
#include <packet.h>
#include <deque>		/* Added By Ssuying For Large Scale Simulation */
#include <event.h>

#include "../its/tactic_api.h"
#include "../its/road.h"

#define MAX_MEMBER 10

/*======================================================================
   Define Data structure
  ======================================================================*/
/*
 * Module-Tree(mTree) & Port-Tree(pTree)
 *
 *	Node-List:
 *	------------------------------------------------------
 *	...  |    | n-1 | n  | n+1 |    |    |    |    | ........
 *	------------------------------------------------------
 *		    |
 *	Port-Tree:  |
 *		------------------------
 *		 ... | p-1 | p  | p+1 | .....
 *		------------------------
 *			     |
 *	Module-Tree:	     |
 *	 	     [Module Interface]
 *		        [Module ARP]
 *		        [Module FIFO]
 *		        [Module MAC]
 *		        [Module PHY]
 *		     [Module Location]
 *
 */

class NslObject;

struct modulelist{
	NslObject		*obj;
	struct Module_Info	*mInfo;
	struct modulelist	*next;
};

enum EventType{
	APPROACHING_INTERSECT,
	IN_INTERSECT,
	LEAVING_INTERSECT
};

enum NodeTypes{
	OTHER_NODE,	// other node type
	AGENT,		// CarAgent or MAgent
	LARGE_SCALE_CAR, // large-scale car
	TRAFFIC_LIGHT_CONTROLLER
};

//If car agent reaches the position (x, y), it changes its direction into "the direction of class Event"
class CarAgentEvent{
	public:
		double x, y;
		double direction; // the directeion of class Event
		EventType type;
};

struct nodeLoc {
	u_int64_t	timeStamp_; 	      /* The time of arriving at this 
						 waypoint */ 
	u_char		pause_; 	      /* The time the node will pause
						 after arriving at this
						 waypoint */
	double 		angle_; 	      /* The moving direction from
						 this waypoint to the next 
						 waypoint viewed in the GUI 
						 working area on the screen. 
						 Note that the (0,0) point 
						 on the GUI screen is at the 
						 top-left corner rather than 
						 at the bottom-right corner. 
						 Therefore, if the angle is
						 A in the normal coordinate
						 system, it is (360-A) in
						 the GUI screen coordinate 
						 system. */
	double		speed_; 	      /* moving speed from this 
						 waypoint to the next waypoint
						 */
	double		LocX_, LocY_, LocZ_;	/* location of this waypoint */
	double		VecX_, VecY_, VecZ_;	/* unit vector to the next 
						   waypoint */
	double		AntX_, AntY_, AntZ_;	/* antenna location relative to
						   node */
	double		acceleration_;		/* speed acceleration starting 
						   from this waypoint. Note that
						   */
	double		MaxVelocity;		// meter / second

	// ----------------------- Added By Large-scale Simulation Group ------------------------
	int		Nid;			// Node ID
	int		Pid;			// Port ID
	int           	CacheOrNot;		// 1: use cache  0: not use cache when search road
	int		NodeType;		// 0: Other  1: Agent Car  2: Large-Scale Car
	int		enableModuleControl;	// module controled traffic light, 1: enable, 0: disable
	int		signalGroupID;		// signal group ID
	int		LaneSwitch;		// 1: Enable 0: Disable
	int		Tid;			// Team ID
	int             TLeader;		// Team Leader
	double		TurningDirection;	// in degree
	int		TurningBlockNumber;
	int		TurningTimes;
	u_int64_t	update_expire;		// update time

	// -------------------------------------------------------------------------------------------
};  

// ----------------------- Added By Large-scale Simulation Group ------------------------
struct drivingInfo {
	double		CurrentDirection;	// node's current moving direction (in degree)
	double		CurrentPos_x, CurrentPos_y, CurrentPos_z; // node's current position (in meter)
	double		CurrentVelocity;	// node's current velocity (in meter / second)
	double		EndPosOfCurrentRoad_x, EndPosOfCurrentRoad_y; // current road's end position (in meter)
	int		isDoingLaneChange;	// check if this node is doing lane change
	double		MaxAcceleration;	// meter / (second^2)
	double		MaxDeceleration;	// meter / (second^2)
	int		oldCollisionCarID;	// prevent continues collision message
	int		SeenTheTrafficLightOrNot; // 1: see   0: no see
	int		SigLight;		// traffic light signal
	int		SigGID;			// traffic light group ID
	int		SigIndex;		// traffic light index in signalList
	double		SigPOS_x, SigPOS_y;	// position of traffic light

	CacheRecord		myCache;	/* cache for saving my road block info */
	CacheRecord		otherCache;	/* cache for checking other car's road block info */

	CarAgentEvent		CurrentEvent;
	deque<CarAgentEvent>	EventQueue;	// event queue of waypoint

};

struct signalInfo {
	struct agentClientGetSignalsInTheSameGroupReply sameG;
	int		changeLightCount;		/* check if it's time to change light */
	int		nextCountToChangeLight;		/* next count to change traffic light */
	int		changeSpecifiedTrafficLight;	/* change traffic light in specified direction */
};
// -------------------------------------------------------------------------------------------

struct updateInfo {
	u_int64_t	pause_;  
	double		LocX_, LocY_, LocZ_;
	double		VecX_, VecY_, VecZ_; 
	double		speed_; 
	double		angle_;
}; 

/*
 * Register-Table:
 *
 *	- The register-table is used to store the variables 
 *	  export to other modules in the same port.
 *
 */

struct varRegtbl {

	struct varRegtbl		*vnext;
	NslObject			*obj_;	  /* pointer to contributor */
	char				*vname_;  /* Variable name */ 
	void				*var_;	  /* pointer to var */
};

/*=======================================================================
  Define Class
  =======================================================================*/
class Node : public NslObject {

private:

	struct varRegtbl  	vartbl_;

	struct modulelist	*mlist;

	nodeLoc			nodeLoc_;   /* Location Information */
	u_int64_t		clock_;	    /* async time */
	u_int32_t		varnum;     /* number of registered variable */

	struct drivingInfo      *drivingInfo_;	/* Driving Information */
	struct signalInfo       *sigInfo_;	/* Signal Controller Information */

	struct WaveShortMessageProtocol mywsmp;
	int                     callTrafficLightInit; /* check if it's first time to initialize */

public:

	Node(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~Node();   

	int			init(); 
	int 			command(int argc, const char *argv[]);

	static int		MobilityEvent(); 
	int			updateLocation(Event_ *ep); 

	int			getNodeAngle(double &); 	
	void 			setNodeAngle(double); 	
	int			getNodeSpeed(double &); 	
	void 			setNodeSpeed(double); 	
	int			getNodeCurrentWaypoint(double &, double &, 
				double &);
	void			setNodeCurrentWaypoint(double, double, double);
	void 			setNodeNextWaypoint(double, double, double);
	int			getNodeCurrentWaypointTimeStamp(u_int64_t &);
	int			getNodeAntenna(double &, double &, double &); 
	void 			setNodeAntenna(double, double, double); 
	int			getNodeVec(double & , double &, double &);
	void			setNodeVec(double, double, double);

	int			getNodePosition(double &, double &, double &); 

	/* The following function are added by automatic vehicle group 2007.2.28*/
	void 			setNodeSpeedAcceleration(double); 
	int			getNodeMaxSpeed(double &);

	void			*getRegVar(struct plist *, const char *);
	void			*getRegVar(u_int32_t, const char *);
	int			regVar(NslObject *, const char *, void *); 
	u_int64_t		getNodeClock();

	// ----------------------- Added By Ssuying For Large Scale Simulation ------------------------
	int			nodeRecv(ePacket_ *pkt);
	int			nodeSend();
	int			SpecifyNodeType();
	void			FillTheQueue(int, double *, double *, double *, double, double, double);
	int			ReachTheNextTriggerPointOrNot();
	void			DetermineVc(double &, double &, double, double, double, double, int);
	double			DetermineAcceleration(double, double, double, double, int);
	void			SetAutoUpdateToEvent(Event_ *);
	int			getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreenInNode(double, double, double, double &, double &, double &, double &, int &);
	int			getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(int, int, double, double, double, double, double, double, int, double &, double &, double &, double &, int &, int);
	int			checkIfSafeToChangeLane(int &, double &);
	void			changeLane();
	void			LS_CarInit();
	void			LS_TrafficLightInit();
	void			UpdateNodeLocation();
	void			UpdateTrafficLight();
	void			getTeamInfo(int &, int &);
	void			getNearestPrecedingTeamMember(int &);
	void			getNearestFollowingTeamMember(int &);
	void			getTeamTurningInfo(double &, int &, int &);
	void			setTeamTurningInfo(double, int, int);
	void			getLaneChangeInfo(int &);
	int			SwitchOrderWithPrecedingCarInTeam();
	int			getNodeType(int &);
	void			resetCarStatus(); // error handler
};
#endif	/* __NCTUNS_node_h__ */
