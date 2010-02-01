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
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "math_fun.h"
#include "tactic_api.h"
#include "sock_skel.h"
#include "road.h"
#include <deque>

#define ITS_NET 1
#define ITS_COLLISION_AVOIDANCE 1
#define ITS_SHOW_MSG 0

#define MOTION_LINEAR 0x30
#define MOTION_TAKING_TURNS_STEP_1 0x40
#define MOTION_TAKING_TURNS_STEP_2 0x41
#define MEGA 1000000 //micro secs
#define SLEEPING_PERIOD 100000 //micro secs
#define VISIBILITY_SCALE_IN_DEGREE 40.0 //degree
#define VISIBILITY_SCALE_IN_DISTANCE 100.0 //meters

enum EventType
{
	APPROACHING_INTERSECT,
	IN_INTERSECT,
	LEAVING_INTERSECT
};

class msgSequence
{
	public:
		u_int32_t nid;
		int seqNum; // sequence number
};

//If car agent reach to the position (x, y), it change its direction into "the direction of class Event"
class Event
{
	public:
		double x, y;
		double direction; // the directeion of class Event
		EventType type;
};

timeval now;
//-----Global variables for IPCs and APIs
int mynid, myTCPsockfd, myUDPsockfd, socketfd2;
int agentUDPportNum = 4000;
int oldCollisionCarID = 0;

//-----Global variables for car's performances.
double MaxAcceleration, MaxDeceleration, MaxVelocity;

//Global variables for car's condition
//1 m/s = 3.6 km/hr
double CurrentVelocity = 0, CurrentAcceleration, CurrentDirection;
double CurrentPOS_x, CurrentPOS_y, CurrentPOS_z = 0;
double ExpectedDirection;
double brokenCarDirection; // It will be a problem if there are many broken cars.
double roadWidth;
Event CurrentEvent;
int WarningDecelerate = 0;
int WakedTimes = -1;
int agentReceivedBrokenCarMsg = 0;
int brokenCarID = 0;

//About the traffic light
int SeenTheTrafficLightOrNot = 0, SigLight, SigGID = -1, SignalIndex = -1;
double SigPOS_x, SigPOS_y;
vector<msgSequence> msgSeq;

int CacheOrNot = 1;
deque<Event> EventQueue;

CacheRecord myCache;
CacheRecord otherCache;

void FillTheQueue(int NumOfTurns, double *turningPOS_x, double *turningPOS_y, double *directions, double FirstPointAfterTheTurn_x, double FirstPointAfterTheTurn_y, double ExpectDirection)
{
	//This function fulls the queue when agent just has reached a new road block.
	//After agent calls the takeATurn function, it uses "pass by reference parameters" to full the queue by this function. 
	Event temp;
	for (int i = 0; i<NumOfTurns; i++){
		//put every turning points into the queue.
		temp.x = turningPOS_x[i];
		temp.y = turningPOS_y[i];
		temp.direction = directions[i];
		EventQueue.push_back(temp);

	}
	//Finally, put the end point of the turning into the queue.
	temp.x = FirstPointAfterTheTurn_x;
	temp.y = FirstPointAfterTheTurn_y;
	temp.direction = ExpectDirection;
	EventQueue.push_back(temp);

}

void reportMyStatusToAGroupOfNode()
{
	static int seqNum=0;
	double lastTime = (double) now.tv_sec * 1000000 + (double) now.tv_usec;
	int value;
	socklen_t len;

	agentClientReportStatus *msg;
	msg = new agentClientReportStatus;

	msg->x = CurrentPOS_x;
	msg->y = CurrentPOS_y;
	msg->type = AGENT_CLIENT_REPORT_STATUS;
	msg->nid = mynid;
	msg->moreMsgFollowing = 1;
	msg->acceleration = CurrentAcceleration;
	msg->speed = CurrentVelocity;
	msg->direction = CurrentDirection;
	msg->seqNum = seqNum;
	msg->timeStamp=lastTime;
	msg->TTL=3;
	seqNum++;
	sockaddr_in cli_addr;

	//Broadcast start

	value = 1;
	setsockopt(myUDPsockfd , SOL_SOCKET , SO_BROADCAST , &value , sizeof(value));
	len =  sizeof(cli_addr);
	memset(&cli_addr, 0, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port   = htons(agentUDPportNum);
	cli_addr.sin_addr.s_addr = inet_addr("1.0.1.255");
	int n = sendto(myUDPsockfd, msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, len);
	if (n < 0) {
		printf("Agent (%d) sendto failed\n", mynid);
	}
	else{
		//printf("Car Agent (%d) broadcast pkt, Size %d, seqNum %d, packet type==%d\n", mynid, sizeof(struct agentClientReportStatus), msg->seqNum, msg->type);
	}
	//Broadcast end
	free(msg);
}

void receiveMsg()
{
	socklen_t len;
	int n = 1;
	sockaddr_in cli_addr;
	while(n > 0){
		typeChecker p;
		len = sizeof(cli_addr);
		n = recvfrom(myUDPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK, (struct sockaddr *) &cli_addr, &len); 
		if(n == 0){
			//printf("Car Agent: %d UDP socket error\n", mynid);
			return;
		}
		if(p.type == RSUAGENT_REPORT_WARNING){
			RSUAgentReportWarning msg;
			n = recvfrom(myUDPsockfd, (char *)&msg, sizeof(struct RSUAgentReportWarning), 0, (struct sockaddr *) &cli_addr, &len); 
			printf("Agent %d : Receive Warning from RSU %d\n", mynid, msg.RSUnid);
			if(msg.AccelerationOrDeceleration == -1)
				WarningDecelerate = -10;
			continue;
		}
		else if(p.type == AGENT_CLIENT_REPORT_STATUS || p.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
			agentClientReportStatus msg;
			n = recvfrom(myUDPsockfd, (char *)&msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, &len); 
			if(n > 0){
				if(msg.seqNum > 0){
					int nidExists = 0;
					for (int i=0 ; i<(int)(msgSeq.size()); i++)
					{
						if(msg.nid == msgSeq[i].nid)
						{
							nidExists=1;
							if(msg.seqNum > msgSeq[i].seqNum)
							{
								//double twoDistance = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, msg.x, msg.y);
								//printf("Agent %d receive msg from agent %d, distance %lf\n", mynid, msg.nid, twoDistance);
								msgSeq[i].seqNum = msg.seqNum;
								if(msg.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
									agentReceivedBrokenCarMsg = 1;
									if(ITS_SHOW_MSG)
										printf("CarAgent(%d) received a broken car report from Agent (%d): %d\n", mynid, msg.nid, msg.seqNum);

									if(ITS_COLLISION_AVOIDANCE){
										brokenCarDirection = msg.direction;
										brokenCarID = msg.nid;
									}
									break;
								}
								if(ITS_SHOW_MSG)
									printf("CarAgent(%d) received a status report from Agent (%d): %d\n", mynid, msg.nid, msg.seqNum);

							}
							else{ 
								//	printf("CarAgent(%d): Message is already received from agent (%d)_%d\n", mynid, msg.nid, msg.seqNum);
							}
							break;

						}

					}
					if(nidExists == 0)
					{
						msgSequence temp;
						temp.nid = msg.nid;
						temp.seqNum = msg.seqNum;
						msgSeq.push_back(temp);
						//double twoDistance = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, msg.x, msg.y);
						//printf("Agent %d receive msg from agent %d, distance %lf\n", mynid, msg.nid, twoDistance);
						if(msg.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
							agentReceivedBrokenCarMsg = 1;
							if(ITS_SHOW_MSG)
								printf("CarAgent(%d) received a broken car report from Agent(%d): %d\n", mynid, msg.nid, msg.seqNum);

							if(ITS_COLLISION_AVOIDANCE)
								brokenCarDirection = msg.direction;
						}
						else {
							if(ITS_SHOW_MSG)
								printf("CarAgent(%d) received a status report from Agent(%d): %d\n", mynid, msg.nid, msg.seqNum);
						}
					}
				}
			}
		}
	}
}

double RandomDirection(int NumOfDirections, double *directions)
{
	int RandInt = rand() % NumOfDirections;
	return directions[RandInt];
}

void init()
{
	int n;
	char portNumStr[32];
	double CorrectedPOS_x, CorrectedPOS_y;

	// get car node id
	mynid = getMyNodeID();	  
	srand(mynid);

	// create IPC connection with SE and set agent in group 1
	myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);

	// create UDP socket for listening broadcasted message
	sprintf(portNumStr, "%d", agentUDPportNum);
	myUDPsockfd = passiveUDP(portNumStr);
	printf("Agent(%d) created myUDPsockfd %d\n", mynid, myUDPsockfd);
	if (myUDPsockfd < 0) {
		printf("Agent(%d): Creating myUDPsockfd failed\n", mynid);
		exit(0);
	}
	n = fcntl(myUDPsockfd, F_SETFL, O_NONBLOCK);

	// Get driving behavior from car profile
	n = getProFileData(mynid, MaxVelocity, MaxAcceleration, MaxDeceleration, 1);
	if(n == 0){
		printf("Car %d open car profile fail. Use default value\n", mynid);
		MaxVelocity = 18;
		MaxAcceleration = 1;
		MaxDeceleration = 4;
	}

	// Construct the whole road map by using this function
	constructRoadMapofTheWholeField(1);

	n = getInitialNodePosition(mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z);
	CurrentPOS_z = 0;

	/* correct my position into the middle of the road */
	n = selfCorrectness(CurrentPOS_x, CurrentPOS_y, CorrectedPOS_x, CorrectedPOS_y, CacheOrNot, &myCache);
	if(n == 1)
	{
		CurrentPOS_x = CorrectedPOS_x;
		CurrentPOS_y = CorrectedPOS_y;
	}
	n = setCurrentWaypoint(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
	/* default parameter */
	//CurrentVelocity = 0;
	CurrentAcceleration = 0; /* acceleration = 1 m/sec^2 */
	//setCurrentMovingSpeed(myTCPsockfd, mynid, CurrentVelocity, 1);

	getRoadWidth(roadWidth);
}

inline int ReachTheNextTriggerPointOrNot()
{
	if(((int) EventQueue.size()) == 0)
		return -1;// Error!!

	double tempAngle = fmod(atan2(CurrentEvent.y- CurrentPOS_y, CurrentEvent.x - CurrentPOS_x)/ PI* 180+ 360, 360);
	tempAngle = fmod(360- tempAngle, 360);

	double angleDiff = fabs(tempAngle- CurrentDirection);
	if(angleDiff > 180)
		angleDiff = 360- angleDiff;

	if(angleDiff >= 90){
		// move over the event point, which means reach the event point
		if(EventQueue.empty() != true)
			EventQueue.pop_front();
		return 1; // reach Event point
	}
	else 
		return 0; // unreach
}

inline double SelectMinimum(double a, double b)
{
	if(a <= b)
		return a;
	else 
		return b;
}

inline double SelectMaximum(double a, double b)
{
	if(a >= b)
		return a;
	else 
		return b;
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
void DetermineVc(double &BufferTime, double &vt)
{
	double NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection;
	double NearestPrecedingCarSpeed = 999999;
	double DesiredMaxSpeed, DistanceToNextEventPoint; // in meter/secs.
	double SAFETY_INTERVEHICLE_DISTANCE = 10;
	double BufferTime1, BufferTime2, BufferTime3 = 999999;
	int NearestPrecedingCarID, n = 1;

	//Desired max speed should be lower at the corner.
	DistanceToNextEventPoint = Distance_BetweenTwoNode(CurrentEvent.x, CurrentEvent.y, CurrentPOS_x, CurrentPOS_y);
	
	/* Cosider what to do when close to the end of a road block */
	if((DistanceToNextEventPoint != 0) && (CurrentVelocity != 0))
	{
		BufferTime3 = roundf(DistanceToNextEventPoint / CurrentVelocity);
	}

	if(DistanceToNextEventPoint >= 30)
	{
		DesiredMaxSpeed = MaxVelocity; // meter/second
	}
	else 
	{
		// If close to the next event point, don't drive too fast.
		DesiredMaxSpeed = rand() % 6 + 5; // 10 m/s => 36 km/hr at maxmimum
	}

	n = getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, 0, CurrentDirection, VISIBILITY_SCALE_IN_DEGREE, VISIBILITY_SCALE_IN_DISTANCE, 1, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID, 1);

	double vt1, vt2;

	vt1 = DesiredMaxSpeed;
	vt2 = DesiredMaxSpeed;

	// set default time, make car run as fast as possible
	/* Put a = MaxAcceleration into E1 and get BufferTime1 */
	BufferTime1 = fabs(vt1 - CurrentVelocity) / MaxAcceleration; 
	BufferTime2 = BufferTime1;

	if(n == 0)
	{
		/* There's a car in front of me. */

		n = checkIfOnTheSameLane(CurrentPOS_x, CurrentPOS_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, CacheOrNot, &myCache, &otherCache);
		if(n != 0)
		{
			/* The preceding car is on the same lane with me. */

			double distance = 999999;

			// Get the distance between me and the preceding car.
			distance = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);
			if((distance <= 0.3) && (oldCollisionCarID != NearestPrecedingCarID)){
				printf("Collision of Car %d (%lf, %lf) and Car %d (%lf, %lf).\n", 
						mynid, CurrentPOS_x, CurrentPOS_y, 
						NearestPrecedingCarID, NearestPrecedingCarPOS_x, 
						NearestPrecedingCarPOS_y);
				oldCollisionCarID = NearestPrecedingCarID;
			}

			// Get preceding car's velocity.
			getCurrentMovingSpeed(myTCPsockfd, NearestPrecedingCarID, NearestPrecedingCarSpeed, 1);

			if(agentReceivedBrokenCarMsg == 1){
				if((brokenCarDirection != ExpectedDirection) && (brokenCarID == NearestPrecedingCarID)){
					/* Broken car is on the same lane but I will switch to other lane.
					 * So, the preceding car shouldn't affect me.
					 */
					NearestPrecedingCarSpeed = 999999;
					distance = 9999999;
					BufferTime1 = 999999;
				}
			}

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

				if(CurrentVelocity == 0)
				{
					vt = 0;
					BufferTime = 0;
					return;
				}
				/* Set vt to 0 and put vt into E3.
				 * We get t = 2 * S / v0
				 */
				BufferTime1 = 2 * (distance - 3) / CurrentVelocity;
			}
			else
			{
				if(distance < 2 * SAFETY_INTERVEHICLE_DISTANCE)
				{
					/* Set the target velocity to NearestPrecedingCarSpeed. */
					double tmpV;

					tmpV = CurrentVelocity + NearestPrecedingCarSpeed;
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
	if((SeenTheTrafficLightOrNot == 1) && (SigLight == RED || SigLight == YELLOW))
	{
		double DistanceToTrafficLight = 9999999;

		DistanceToTrafficLight = Distance_BetweenTwoNode(SigPOS_x, SigPOS_y, CurrentPOS_x, CurrentPOS_y);
		if(DistanceToTrafficLight <= 2*SAFETY_INTERVEHICLE_DISTANCE){
			// Getting close to the traffic light, need to slow down a little
			vt2 = SelectMinimum(0 - (DistanceToTrafficLight - SAFETY_INTERVEHICLE_DISTANCE) / 10 , DesiredMaxSpeed);
		}
		if((DistanceToTrafficLight != 0) || (CurrentVelocity != 0))
		{
			if((BufferTime2 = DistanceToTrafficLight / CurrentVelocity) < 0)
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

			scv1 = sqrt(MaxDeceleration * DistanceToTrafficLight); 

			/* safe check */
			if(CurrentVelocity >= scv1){
				/* Dangerous Velocity!! Need to slow down fast. */
				BufferTime = 0;
				return;
			}
		}
	}

	BufferTime = SelectMinimum(BufferTime1, BufferTime2);
	BufferTime = SelectMinimum(BufferTime, BufferTime3);

	vt = SelectMinimum(vt1, vt2);
}

double DetermineAcceleration()
{
	/*	
	 * The following implementation is based on 
	 * VATSIM: A Simulator for Vehicles and Traffic ,
	 * Jia Lei Keith Redmill Umit Ozguncr ,
	 * Department of Electrical Engineering, The Ohio State University, 
	 * 2001 IEEE Intelligent Transportation Systems Conference Proceedings
	 *
	 *
	 * vt:	   the target velocity
	 * bfTime: the buffer time to achieve vt.
	 */
	double vt, bfTime;

	DetermineVc(bfTime, vt);

	double acc;

	if(bfTime > 0)
	{
		/* Equation: vt = CurrentVelocity + acc * bfTime */
		acc = roundf((vt- CurrentVelocity) / bfTime * 10);
		acc /= 10;
	} else {
		/* Emergent Stop */
		acc = -MaxDeceleration;
	}

	//Our behavior was changed due to warning from RSU
	if(WarningDecelerate < 0)
		return -MaxDeceleration;
	if(acc > MaxAcceleration)
		return MaxAcceleration;
	if(acc < -MaxDeceleration)
		return -MaxDeceleration;
	else
		return  acc;
}

int main()
{
	int sleepingPeriod = 100000;
	init();
	while(1)
	{
		if(ITS_NET)
			reportMyStatusToAGroupOfNode();
		fflush(stdout);

		/* FIXME: If necessary, make every node wake up in different time. 
		 * (Avoid collision of broadcast pkt on mac)
		 */
		//usleepAndReleaseCPU(myTCPsockfd, mynid, SLEEPING_PERIOD, 1);
		usleepAndReleaseCPU(myTCPsockfd, mynid, sleepingPeriod, 1);
		WarningDecelerate++;
		WakedTimes++;
		gettimeofday(&now, 0);

		if(ITS_NET){
			// receive msg from other nodes
			receiveMsg();
		}

		int n = 0;
		double *CandidateDirection = NULL;

		n = getCurrentPosition(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);

		// See the Traffic light signal.
		n = getTheNearestTrafficLightInfrontOfMe(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentDirection, 100.0, SigGID, SigLight, SigPOS_x, SigPOS_y, SignalIndex, 1);
		if(n == 1)
		{
			SeenTheTrafficLightOrNot = 1;
		}
		else 
			SeenTheTrafficLightOrNot = 0;

		// get next moving event (point) from event queue
		CurrentEvent = EventQueue.front();

		getCurrentMovingSpeed(myTCPsockfd, mynid, CurrentVelocity, 1);
		CurrentAcceleration = DetermineAcceleration();

		setCurrentSpeedAcceleration(myTCPsockfd, mynid, CurrentAcceleration, 1);

		if(EventQueue.empty() == false)
		{
			n = ReachTheNextTriggerPointOrNot();
			// n = 1; Reach the event point.
			// n = 0; Unreach the event point.
		}
		else
		{
			n = 1;
		}

		if(n == 0)
		{
			//Not reach the event point, do nothing, keep going.
			sleepingPeriod = 100000;
			continue;
		}
		else if(n == 1)
		{
			sleepingPeriod = 10000;
			/* create new event queue */
			if(((int)EventQueue.size()) == 0)
			{
				/* End of taking turns, so the queue became empty */
				int numOfDirections;
				double CorrectedPOS_x, CorrectedPOS_y;
				double a, b, c;

				// correct my position into the middle of the road
				n = selfCorrectness(CurrentPOS_x, CurrentPOS_y, CorrectedPOS_x, CorrectedPOS_y, CacheOrNot, &myCache);
				if(n == 1)
				{
					CurrentPOS_x = CorrectedPOS_x;
					CurrentPOS_y = CorrectedPOS_y;
				}
				n = setCurrentWaypoint(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);

				/* get front node block ID which connected by current edge.
				 * Note:
				 *    node block ID is the same as signal group ID
				 */
				getFrontNID(CurrentPOS_x, CurrentPOS_y, SigGID, CacheOrNot, &myCache);

				int RoadType;
				double endPOS_x, endPOS_y;

				n = getCurrentRoadInformation(CurrentPOS_x, CurrentPOS_y, numOfDirections, CandidateDirection, a, b, c, endPOS_x, endPOS_y, RoadType, CacheOrNot, &myCache);
				if(n <= 0){
					printf("Warning_1_2: Node is not on the lane!! Node %d position(%lf, %lf), current direction %lf, eventPos (%lf, %lf), eventDirection %lf\n", 
							mynid, CurrentPOS_x, CurrentPOS_y, CurrentDirection, CurrentEvent.x, CurrentEvent.y, CurrentEvent.direction);
					fflush(stdout);
					stopSimulation(myTCPsockfd, mynid);
				}

				if(RoadType == ROAD_TYPE_NODE)
				{
					double distanceToNextEventPoint = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, CurrentEvent.x, CurrentEvent.y);
					double angleDiff = fabs(CurrentDirection - CurrentEvent.direction);
					if(angleDiff > 180)
						angleDiff = 360 - angleDiff;

					if(((CurrentEvent.x <= 0.000001) && (CurrentEvent.y <= 0.000001)) ||
						((distanceToNextEventPoint >= roadWidth) && (angleDiff < 90)))
					{
						/*
						 * Node is in the node block at beginning ||
						 * Node skips one lane and directly enters into the node block
						 * (It happens when the road length is very small)
						 */
						double exitPOS_x, exitPOS_y, exitDirection;
						double nextDirection;

						n = getCurrentNodeExit(CurrentPOS_x, CurrentPOS_y, exitPOS_x, exitPOS_y, exitDirection, nextDirection, CacheOrNot, &myCache);
						if(n < 0)
						{
							printf("Node[%d] getCurrentNodeExit error\n", mynid);
						}

						CurrentDirection = exitDirection;
						// set current moving direction viewed on GUI screen
						n = setCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, mynid, CurrentDirection, 1);

						FillTheQueue(0, NULL, NULL, NULL, exitPOS_x, exitPOS_y, nextDirection);
					}
					else
					{
						/* After the turn, if car is still on the previous road, 
						 * make this car go to the final point of the previous road 
						 * one more time.
						 */
						FillTheQueue(0, NULL, NULL, NULL, CurrentEvent.x, CurrentEvent.y, CurrentEvent.direction);
					}
					free(CandidateDirection);
					CandidateDirection = NULL;
					continue;
				}

				CurrentDirection = RandomDirection(numOfDirections, CandidateDirection);
				free(CandidateDirection);
				CandidateDirection = NULL;

				n = setCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, mynid, CurrentDirection, 1);

				n = getNextRoadInformation(CurrentPOS_x, CurrentPOS_y, CurrentDirection, numOfDirections, CandidateDirection, a, b, c, CacheOrNot, &myCache);
				if(n <= 0){
					printf("Warning_1_3: Cannot find the next road!! Node %d position(%lf, %lf)\n", mynid, CurrentPOS_x, CurrentPOS_y);
					fflush(stdout);
					stopSimulation(myTCPsockfd, mynid);
				}

				if(numOfDirections == 0){
					printf("Error: numOfDirections can't be zero\n");
					fflush(stdout); 
					stopSimulation(myTCPsockfd, mynid);
				}

				ExpectedDirection = RandomDirection(numOfDirections, CandidateDirection);
				if(agentReceivedBrokenCarMsg == 1){
					if(numOfDirections >= 2){
						while(ExpectedDirection == brokenCarDirection){
							ExpectedDirection = RandomDirection(numOfDirections, CandidateDirection);
						}
					}
				}
				free(CandidateDirection);
				CandidateDirection = NULL;

				int numOfTurns;
				double *turningPOS_x = NULL, *turningPOS_y = NULL, *DirectionQueue = NULL;
				double FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y;

				n = takeATurn(CurrentPOS_x, CurrentPOS_y, CurrentDirection, ExpectedDirection, numOfTurns, turningPOS_x, turningPOS_y, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, DirectionQueue, CacheOrNot, &myCache);
				if(n <= 0)
				{
					printf("Error: cannot get the taking turns info, error no. %d, curPos (%lf, %lf), curDirection %lf, expectedDirection %lf\n", n, CurrentPOS_x, CurrentPOS_y, CurrentDirection, ExpectedDirection);
					fflush(stdout);
					stopSimulation(myTCPsockfd, mynid);
				}
				FillTheQueue(numOfTurns, turningPOS_x, turningPOS_y, DirectionQueue, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, ExpectedDirection);

				delete turningPOS_x;
				delete turningPOS_y;
				delete DirectionQueue;

			}
			else if(((int)EventQueue.size()) > 0)
			{
				//Pop an item from the queue
				CurrentDirection = CurrentEvent.direction;
				CurrentPOS_x = CurrentEvent.x;
				CurrentPOS_y = CurrentEvent.y;
				n = setCurrentWaypoint(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
				n = setCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, mynid, CurrentDirection, 1);
			}
		}
		else
		{
			printf("Node[%d]: Error for reached or not, n %d\n", mynid, n); 
			fflush(stdout);
			stopSimulation(myTCPsockfd, mynid);
		}
	}
}
