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
#include "math_fun.h"
#include "tactic_api.h"
#include "sock_skel.h"
#include "road.h"
#include <deque>

#define ITS_NET 0
#define ITS_COLLISION_AVOIDANCE 0
#define ITS_SHOW_MSG 0

#define MOTION_LINEAR 0x30
#define MOTION_TAKING_TURNS_STEP_1 0x40
#define MOTION_TAKING_TURNS_STEP_2 0x41
#define MEGA 1000000 //micro secs
#define SLEEPING_PERIOD 100000 //micro secs
#define VISIBILITY_SCALE_IN_DEGREE 30.0 //degree
#define VISIBILITY_SCALE_IN_DISTANCE 100.0 //meters

enum EventType{
	APPROACHING_INTERSECT,
	IN_INTERSECT,
	LEAVING_INTERSECT
};
class msgSequence{
	public:
		u_int32_t nid;
		int seqNum; // sequence number
};

//If car agent reach to the position (x, y), it change its direction into "the direction of class Event"
class Event{
	public:
		double x, y;
		double direction; // the directeion of class Event
		EventType type;
};

//-----Global variables for IPCs and APIs
int mynid, myTCPsockfd, myUDPsockfd, socketfd2;
double *CandidateDirection=NULL, endPOS_x, endPOS_y, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y;
int n;
int lookTo = 1;
int isDoingLaneChange = 0; // Car is doing lane change

//-----Global variables for car's performances.
double MaxAcceleration, MaxDeceleration, MaxVelocity;

//Global variables for car's condition
double CurrentVelocity, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z = 0, CurrentAcceleration, CurrentDirection;
double ExpectedDirection;
double brokenCarDirection; // It will be a problem if there are many broken cars.
Event CurrentEvent;
int WarningDecelerate = 0;
int agentReceivedBrokenCarMsg = 0;
int brokenCarID = 0;

//About the traffic light
int SeenTheTrafficLightOrNot = 0, SigLight;

int CacheOrNot = 0;
deque<Event> EventQueue;

CacheRecord myCache;

void FillTheQueue(double end_x, double end_y, double CurrentDirection, int NumOfTurns, double *turningPOS_x, double *turningPOS_y, double *directions, double FirstPointAfterTheTurn_x, double FirstPointAfterTheTurn_y, double ExpectDirection){
	/*
	 * This function fulls the queue when agent just has reached a new road block.
	 * After agent calls the takeATurn function, 
	 * it uses "pass by reference parameters" to full the queue by this function. 
	 */
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

double RandomDirection(int NumOfDirections, double *directions){
	int RandInt = rand() % NumOfDirections;
	return directions[RandInt];
}

void init()
{
	double CorrectedPOS_x, CorrectedPOS_y;
	int agentUDPportNum = 4000;
	char portNumStr[32];

	mynid = getMyNodeID();	  
	srand(mynid);
	// CarAgent is in group 1
	myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
	sprintf(portNumStr, "%d", agentUDPportNum);
	myUDPsockfd = passiveUDP(portNumStr);
	printf("Agent(%d) created myUDPsockfd %d\n", mynid, myUDPsockfd);
	if (myUDPsockfd < 0) {
		printf("Agent(%d): Creating myUDPsockfd failed\n", mynid);
		exit(0);
	}
	n = fcntl(myUDPsockfd, F_SETFL, O_NONBLOCK);

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
	n = selfCorrectness(CurrentPOS_x, CurrentPOS_y, CorrectedPOS_x, CorrectedPOS_y, CacheOrNot, &myCache);
	if(n == 1)
	{
		CurrentPOS_x = CorrectedPOS_x;
		CurrentPOS_y = CorrectedPOS_y;
	}
	n = setCurrentWaypoint(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
	/* default parameter */
	CurrentVelocity = 7; /* 7 m/s = 25.2 km/hr */
	CurrentAcceleration = 0; /* acceleration = 1 m/sec^2 */

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

void DetermineVc(double &BufferTime, double &vc)
{
	double DesiredMaxSpeed, DistanceToNextEventPoint; // in meter/secs.
	double SAFETY_INTERVEHICLE_DISTANCE = 10;
	double NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarSpeed, distance;
	double BufferTime1, BufferTime2, BufferTime3;
	int NearestPrecedingCarID, n;
	//Desired max speed should be lower at the corner.
	DistanceToNextEventPoint = Distance_BetweenTwoNode(CurrentEvent.x, CurrentEvent.y, CurrentPOS_x, CurrentPOS_y);
	if((DistanceToNextEventPoint == 0) || (CurrentVelocity == 0)){
		BufferTime3 = 99999;
	}
	else{
		BufferTime3 = roundf(DistanceToNextEventPoint / CurrentVelocity);
	}
	if(DistanceToNextEventPoint >= 20)
		DesiredMaxSpeed = 20; //About 72 km/hr
	else {
		DesiredMaxSpeed = 10; // about 35 km/hr
		//printf("At the Corner\n");
	}

	n = getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, 0, CurrentDirection, VISIBILITY_SCALE_IN_DEGREE, VISIBILITY_SCALE_IN_DISTANCE, 1, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID, 1);

	double a, b, c, vc1, vc2;
	if(n == -1){
		NearestPrecedingCarSpeed = 999999;
		distance = 9999999;
		BufferTime1 = 99999;
	}
	else {
		PolarEquation(a, b, c, CurrentPOS_x, CurrentPOS_y, CurrentDirection);
		double distance1 = Distance_NodeToLine(a, b, c, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);
		if(distance1 >= 3){
			//The car near to me is not in the same lane.
			NearestPrecedingCarSpeed = 999999;
			distance = 9999999;
			BufferTime1 = 99999;
		}
		else
		{
			//The car near to me is in the same lane.
			distance = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);
			if(distance <= 0.3){
				printf("Data: Collision of Car %d and Car %d at the intersection.\n", mynid, NearestPrecedingCarID);
			}
			getCurrentMovingSpeed(myTCPsockfd, NearestPrecedingCarID, NearestPrecedingCarSpeed, 1);
			if((distance == 0) || (CurrentVelocity == 0)){
				BufferTime1 = 99999;
			}
			else{
				BufferTime1 = roundf(distance / CurrentVelocity);
			}
			if(agentReceivedBrokenCarMsg == 1){
				if((brokenCarDirection != ExpectedDirection) && (brokenCarID == NearestPrecedingCarID)){
					//broken car is in the same lane but I will turn to other lane.
					//So, the font car shouldn't affect me.
					NearestPrecedingCarSpeed = 999999;
					distance = 9999999;
					BufferTime1 = 99999;
				}
			}
		}
	}
	//Now, only consider the effects of the Preceding car's velocity and position. 
	//If the preceding car is further than the Event point, car agent would ignore the preceding car since car agent will take a turn at the event point without colliding to the preceding car.
	if(NearestPrecedingCarSpeed <= CurrentVelocity && distance < DistanceToNextEventPoint)
		vc1 = SelectMinimum(NearestPrecedingCarSpeed - 0.1 * (distance - SAFETY_INTERVEHICLE_DISTANCE), DesiredMaxSpeed);
	else if(NearestPrecedingCarSpeed > CurrentVelocity)
		vc1 = SelectMinimum(NearestPrecedingCarSpeed - CurrentVelocity, DesiredMaxSpeed);

	if(SeenTheTrafficLightOrNot == 1 && (SigLight == RED || SigLight == YELLOW)){
		//Only deal with the Buffer time at this time, the vc is left.
		vc2 = SelectMinimum(0 - (DistanceToNextEventPoint - SAFETY_INTERVEHICLE_DISTANCE) / 10 , DesiredMaxSpeed);
		if((DistanceToNextEventPoint == 0) || (CurrentVelocity == 0)){
			BufferTime2 = 99999;
		}
		else{
			BufferTime2 = roundf(DistanceToNextEventPoint/CurrentVelocity);
		}
	}
	else {
		BufferTime2 = 999999;
		vc2 = vc1;
	}
	BufferTime = SelectMinimum(BufferTime1, BufferTime2);
	BufferTime = SelectMinimum(BufferTime, BufferTime3);

	vc = SelectMinimum(vc1, vc2);
}

double DetermineAcceleration()
{
	/*
	 * The following implementation is based on 
	 * VATSIM: A Simulator for Vehicles and Traffic ,
	 * Jia Lei Keith Redmill Umit Ozguncr ,
	 * Department of Electrical Engineering, The Ohio State University, 
	 * 2001 IEEE Intelligent Transportation Systems Conference Proceedings
	 */
	double vc, bfTime;

	DetermineVc(bfTime, vc);
	if(bfTime > 10)
		bfTime = 10;

	double acc;

	if(bfTime == 0){
		acc = 0;
	}
	else{
		acc = roundf((vc- CurrentVelocity) / bfTime * 10);
	}
	acc /= 10;
	//Our behavior was changed due to warning from RSU
	if(WarningDecelerate < 0)
		return -4.5;
	if(acc > 0.7)
		return 0.7;
	else if(acc < 0 && acc >-0.5) //Prevent the deceleration lower than that value in order to remain the safety.
		return -0.5;
	else if(acc < -4.5)
		return -4.5;
	else{
		return  acc;
	}
}

void changeLaneOrNot(){
	/* lookTo = -1  -> look to left-hand side on GUI screen
	   lookTo = 1 -> look to right-hand side on GUI screen
	   */
	int loop2;
	double neighborRoadDirection = 0;

	for(loop2=0; loop2<2; loop2++)
	{
		if(lookTo == 1){
			n = getNeighboringLaneInformation(CurrentPOS_x, CurrentPOS_y, RIGHT, neighborRoadDirection, CacheOrNot, &myCache);
		}
		else{
			n = getNeighboringLaneInformation(CurrentPOS_x, CurrentPOS_y, LEFT, neighborRoadDirection, CacheOrNot, &myCache);
		}
		if(n == 1)
		{
			double testAngle = 0;

			testAngle = fabs(CurrentDirection - neighborRoadDirection);
			if(fabs(testAngle - 180) <= 0.5){
				/* The neighboring lane has different direction */
				if(lookTo == 1)
					lookTo = -1;
				else
					lookTo = 1;
			}
			else{
				Event tmpEvent;
				double midDirection = fmod(CurrentDirection - lookTo*45 + 360, 360);
				double midDirectionArc;
				double delta_x, delta_y;
				double radius = 10.0; /* radius of a circle */

				midDirectionArc = (fmod(360 - midDirection, 360))*PI/180;
				while(EventQueue.empty() != true)
					EventQueue.pop_front();

				delta_x = radius*sqrt(2)*cos(midDirectionArc);
				delta_y = radius*sqrt(2)*sin(midDirectionArc);

				//First event point
				tmpEvent.x = CurrentPOS_x + delta_x; 
				tmpEvent.y = CurrentPOS_y + delta_y; 
				tmpEvent.direction = midDirection;
				EventQueue.push_back(tmpEvent);

				//Second event point
				tmpEvent.x = CurrentPOS_x + 2*delta_x;
				tmpEvent.y = CurrentPOS_y + 2*delta_y;
				tmpEvent.direction = neighborRoadDirection;
				EventQueue.push_back(tmpEvent);

				lookTo = 1;
				isDoingLaneChange = 1;
				break;
			}
		}
		else {
			if(lookTo == 1)
				lookTo = -1;
			else
				lookTo = 1;
		}
	}
}

int main(int argc, char* argv[])
{
	// second parameter of command line that user decides if enable LaneChange or not
	int EnableLaneChange = 0; 
	
	init();
	if(argc == 3)
	{
		EnableLaneChange = atoi(argv[1]);
		CurrentVelocity = atof(argv[2]);
	}
	setCurrentMovingSpeed(myTCPsockfd, mynid, CurrentVelocity, 1);

	while(1)
	{
		fflush(stdout);
		usleepAndReleaseCPU(myTCPsockfd, mynid, SLEEPING_PERIOD/2, 1);
		WarningDecelerate++;
		n = getCurrentPosition(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
		CurrentEvent = EventQueue.front();
		getCurrentMovingSpeed(myTCPsockfd, mynid, CurrentVelocity, 1);
		setCurrentSpeedAcceleration(myTCPsockfd, mynid, CurrentAcceleration, 1);

		double NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection;
		int NearestPrecedingCarID;
		n = getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, 0, CurrentDirection, 90, VISIBILITY_SCALE_IN_DISTANCE, 1, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y, NearestPrecedingCarPOS_z, NearestPrecedingCarDirection, NearestPrecedingCarID, 1);
		if(n>-1){
			double twoCarDistance=Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, NearestPrecedingCarPOS_x, NearestPrecedingCarPOS_y);
			if(twoCarDistance <= 0.3){
				printf("Data: Collision of Car %d and Car %d at the intersection.\n", mynid, NearestPrecedingCarID);
			}
			if((EnableLaneChange == 1) && (twoCarDistance <= 15)){
				if(isDoingLaneChange == 0){
					changeLaneOrNot();
				}
			}
		}
		if(EventQueue.empty() == false){
			n = ReachTheNextTriggerPointOrNot();
			// n = 1; Reach the event point.
			// n = 0; Unreach the event point.
		}
		else
		{
			n = 1;
			isDoingLaneChange = 0;
		}
		if(n == 0)
		{
			continue;
			//Doing nothing, keep going.
		}
		else if(n == 1)
		{
			isDoingLaneChange = 0;
			// create new event queue.
			if(((int)EventQueue.size()) == 0)//End of taking turns, so the queue became empty.
			{
				int RoadType;
				int numOfDirections;
				double CorrectedPOS_x, CorrectedPOS_y;
				double a, b, c; // linear equation of ax + by + c = 0

				n = selfCorrectness(CurrentPOS_x, CurrentPOS_y, CorrectedPOS_x, CorrectedPOS_y, CacheOrNot, &myCache);
				if(n == 1)
				{
					CurrentPOS_x = CorrectedPOS_x;
					CurrentPOS_y = CorrectedPOS_y;
				}
				n = setCurrentWaypoint(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
				n = getCurrentRoadInformation(CurrentPOS_x, CurrentPOS_y, numOfDirections, CandidateDirection, a, b, c, endPOS_x, endPOS_y, RoadType, CacheOrNot, &myCache);
				if(n <= 0)
				{
					printf("Warning_1_2: Node is not on the lane!! Node %d position(%lf, %lf), current direction %lf\n", mynid, CurrentPOS_x, CurrentPOS_y, CurrentDirection);
					fflush(stdout);
					stopSimulation(myTCPsockfd, mynid);
				}

				if(RoadType == ROAD_TYPE_NODE)
				{
					/*
					 * After the turn, if car is still on the previous road, 
					 * make this car go to the final point of the previous road one more time.
					 */
					FillTheQueue(0, 0, 0, 0, NULL, NULL, NULL, CurrentEvent.x, CurrentEvent.y, CurrentEvent.direction);
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

				int numOfTurns;
				double *turningPOS_x = NULL, *turningPOS_y = NULL, *DirectionQueue = NULL; 

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

				n = takeATurn(CurrentPOS_x, CurrentPOS_y, CurrentDirection, ExpectedDirection, numOfTurns, turningPOS_x, turningPOS_y, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, DirectionQueue, CacheOrNot, &myCache);
				if(n <= 0){
					printf("Error: cannot get the taking turns info, error no. %d, curPos (%lf, %lf), curDirection %lf,  expectedDirection %lf\n", n, CurrentPOS_x, CurrentPOS_y, CurrentDirection, ExpectedDirection);
					fflush(stdout);
					stopSimulation(myTCPsockfd, mynid);
				}
				FillTheQueue(endPOS_x, endPOS_y, CurrentDirection, numOfTurns, turningPOS_x, turningPOS_y, DirectionQueue, FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, ExpectedDirection);

				delete turningPOS_x;
				delete turningPOS_y;
				delete DirectionQueue;

			}
			else if(((int)EventQueue.size()) > 0){
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
			printf("CarAgent :Error for reached or not, n %d\n", n); 
			fflush(stdout);
			stopSimulation(myTCPsockfd, mynid);
		}
	}
}
