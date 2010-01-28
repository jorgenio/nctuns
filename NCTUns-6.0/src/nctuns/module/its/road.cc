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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <vector>
#include "road.h"
#include "tactic_api.h"
#include "math_fun.h"
using namespace std;

#define DEBUG_ROAD 0

int CacheHit = 0, CacheMiss = 0;
double RoadWidth = 20;

vector<RoadNode>NodeList;
vector<RoadEdge>EdgeList;

CacheRecord::CacheRecord(){
	CurrentDirty = NextDirty = AfterDirty = 0;
}

void CacheRecord::RefreshCurrent(int NodeIndex, int EdgeIndex, int LaneIndex, int BlockIndex)
{
	CurrentNodeSerial =  NodeIndex;
	CurrentEdgeSerial =  EdgeIndex;
	CurrentLaneSerial =  LaneIndex;
	CurrentBlockSerial =  BlockIndex;
}

void CacheRecord::RefreshNext(int NodeIndex, int EdgeIndex, int LaneIndex, int BlockIndex)
{
	NextNodeSerial =  NodeIndex;
	NextEdgeSerial =  EdgeIndex;
	NextLaneSerial =  LaneIndex;
	NextBlockSerial =  BlockIndex;
}

void CacheRecord::RefreshAfter(int NodeIndex, int EdgeIndex, int LaneIndex, int BlockIndex)
{
	NodeAfterTurn = NodeIndex;
	EdgeAfterTurn = EdgeIndex;
	LaneAfterTurn = LaneIndex;
	BlockAfterTurn = BlockIndex;
}

inline RoadBlock &RoadBlock::operator=(const RoadBlock &temp)
{
	if(this == &temp)
		return *this;
	for(int i = 0; i < 4; ++i)
	{
		x[i] = temp.x[i];
		y[i] = temp.y[i];
	}
	a = temp.a;
	b = temp.b;
	c = temp.c;
	direction = temp.direction;
	end_x[0] = temp.end_x[0];
	end_x[1] = temp.end_x[1];
	end_y[0] = temp.end_y[0];
	end_y[1] = temp.end_y[1];
	type = temp.type;
	RID = temp.RID;
	return *this;

}

// read the .road_structure file
void read_RoadStructure(FILE *fptr)
{
	//Ready to fetch the data from the file.
	vector<RoadBlock>BlockList;
	/*
	 * Note:
	 *    Becareful!! 
	 *    The line size 2048 may be not enough if there are
	 *    too many road blocks in one lane.
	 */
	char line[2048];
	while(!feof(fptr)) {
		line[0] = '\0';
		fgets(line, 2047, fptr);
		if ((line[0]=='\0')||(line[0]=='#'))
			continue;

		char *segment = NULL;
		segment = strtok(line, "\n \t");
		if(segment == NULL)
			continue;
		if(!strcmp(segment, "Node_Block_Start"))
		{
			//Ready to initial a node data.
			RoadNode temp;
			int NumOfEdges = 0, NumOfReadEID = 0;
			while(!feof(fptr))
			{
				line[0] = '\0';
				fgets(line, 2047, fptr);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				segment = strtok(line, " \n");
				if(!strcmp(segment, "NodeID"))
				{
					segment = strtok(NULL, "\t \n");
					int NID = atoi(segment);
					if(NID < 0)
					{
						printf("[%s] error: Wrong NodeID\n", __func__);
						exit(0);
					}
					temp.NID = NID;
					//printf("NodeID %d\n", NID);
					continue;

				}
				else if(!strcmp(segment, "Type"))
				{
					segment = strtok(NULL, "\t \n");
					if(!strcmp(segment, "I"))
						temp.type = NODE_TYPE_I;
					else if(!strcmp(segment, "L"))
						temp.type = NODE_TYPE_L;
					else if(!strcmp(segment, "T"))
						temp.type = NODE_TYPE_T;
					else if(!strcmp(segment, "R"))
						temp.type = NODE_TYPE_R;
					//printf("Type %d\n", temp.type);
					continue;
				}
				else if(!strcmp(segment, "Number_Of_Edges"))
				{
					segment = strtok(NULL, "\t \n");
					NumOfEdges = atoi(segment);
					if(NumOfEdges <= 0)
					{
						printf("[%s] error: Wrong Number_Of_Edges\n", __func__);
						exit(0);
					}
					//printf("Number_Of_Edges %d\n", NumOfEdges);
					temp.NumOfEdges = NumOfEdges;
					temp.EID = new int[NumOfEdges];
					temp.direction = new double[NumOfEdges];
					temp.HeadOrNot = new bool[NumOfEdges];
					continue;
				}
				else if(!strcmp(segment, "EID"))
				{
					segment = strtok(NULL, ":");
					int EID = atoi(segment);
					if(EID < 0)
					{
						printf("[%s] error: Wrong EID, this edge may not be connected to any node\n", __func__);
						exit(0);
					}
					//printf("EID %d\n", EID);
					temp.EID[NumOfReadEID] = EID;
					segment = strtok(NULL, "\t \n");
					if(!strcmp(segment, "Direction"))
					{
						/* Normal coordinate direction not GUI viewd direction */
						segment = strtok(NULL, ":");
						double direction = atof(segment);
						//printf("Direction %lf \n", direction);
						temp.direction[NumOfReadEID] = direction;
					}
					segment = strtok(NULL, "\n \t");
					if(!strcmp(segment, "Head"))
						temp.HeadOrNot[NumOfReadEID] = 1;
					else if(!strcmp(segment, "Tail"))
						temp.HeadOrNot[NumOfReadEID] = 0;
					else 
					{
						printf("[%s] error: Wrong Head/Tail\n", __func__);
						exit(0);
					}
					NumOfReadEID++;
					//printf("Head or Tail %d\n", temp.HeadOrNot);
					//printf("NumOfReadEID %d\n", NumOfReadEID);
					continue;
				}
				else if(!strcmp(segment, "Road_Block"))
				{
					segment = strtok(NULL, "\n \t");
					temp.RoadBlockID = BlockList[atoi(segment)];
					temp.RoadBlockID.type = ROAD_TYPE_NODE;
					//printf("\nNODE: Read the road block\n");
					continue;
				}
				else if(!strcmp(segment, "Node_Block_End"))
				{
					//printf("Push into NodeList\n");
					NodeList.push_back(temp);
					break;
				}
			}
		}
		else if(!strcmp(segment, "Edge_Block_Start"))
		{
			//Ready to initial a edge data.
			RoadEdge temp;
			int NumOfReadNID = 0, NumOfLanes = 0, NumOfNodes = 0;
			while(!feof(fptr))
			{
				line[0] = '\0';
				fgets(line, 2047, fptr);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				segment = strtok(line, "\t \n");
				if(segment==NULL)
					continue;
				if(!strcmp(segment, "EID"))
				{
					segment = strtok(NULL, "\t \n");
					int EID = atoi(segment);
					if(EID < 0)
					{
						printf("[%s] error: Wrong EID\n", __func__);
						exit(0);
					}
					temp.EID = EID;
					//printf("EdgeID %d\n", EID);
					continue;

				}
				else if(!strcmp(segment, "Number_Of_Lanes"))
				{
					segment = strtok(NULL, "\t \n");
					NumOfLanes = atoi(segment);
					if(NumOfLanes <= 0)
					{
						printf("[%s] error: Wrong Number_Of_Lanes\n", __func__);
						exit(0);
					}
					temp.NumOfLane = NumOfLanes;
					temp.lane = new RoadLane[NumOfLanes];
					//printf("NumOfLanes %d\n", temp.NumOfLane);
					continue;
				}
				else if(!strcmp(segment, "Number_Of_Nodes"))
				{
					segment = strtok(NULL, "\t \n");
					NumOfNodes = atoi(segment);
					if(NumOfNodes <= 0)
					{
						printf("[%s] error: Wrong Number_Of_Nodes\n", __func__);
						exit(0);
					}
					temp.NumOfNode = NumOfNodes;
					temp.NID = new int[NumOfNodes];
					//printf("NumOfNodes %d\n", NumOfNodes);
					continue;
				}
				else if(!strcmp(segment, "NID"))
				{
					segment = strtok(NULL, "\t \n");
					int NID = atoi(segment);
					if(NID < 0)
					{
						printf("[%s] error: Wrong NID\n", __func__);
						exit(0);
					}
					temp.NID[NumOfReadNID] = NID;
					NumOfReadNID++;
					//printf("NID %d\n", NumOfReadNID);
					continue;
				}
				else if(!strcmp(segment, "L_Number"))
				{
					segment = strtok(NULL, "\t \n");
					int lNumber = atoi(segment);
					if(lNumber < 0)
					{
						printf("[%s] error: Wrong L_Number\n", __func__);
						exit(0);
					}
					temp.L_Number = lNumber;
					//printf("lNumber %d\n", lNumber);
					continue;
				}
				else if(!strcmp(segment, "R_Number"))
				{
					segment = strtok(NULL, "\t \n");
					int rNumber = atoi(segment);
					if(rNumber < 0)
					{
						printf("[%s] error: Wrong R_Number\n", __func__);
						exit(0);
					}
					temp.R_Number = rNumber;
					//printf("rNumber %d\n", rNumber);
					continue;
				}
				else if(!strcmp(segment, "LENGTH"))
				{
					segment = strtok(NULL, "\t \n");
					double length = atof(segment);
					if(length < 0)
					{
						printf("[%s] error: Wrong LENGTH\n", __func__);
						exit(0);
					}
					temp.length = length;
					//printf("LENGTH %lf\n", length);
					continue;
				}
				else if(!strcmp(segment, "Lane"))
				{
					segment = strtok(NULL, ":");
					int laneNumber = atoi(segment);
					if(laneNumber <= 0)
					{
						printf("[%s] error: Wrong Lane Number\n", __func__);
						exit(0);
					}
					//printf("Read laneNumber %d\n", laneNumber);
					while(segment != NULL)
					{
						segment = strtok(NULL, " ");
						if(segment == NULL)
							break;
						else if(!strcmp(segment, "Road_Block"))
						{
							segment = strtok(NULL, ",\n");
							if(segment == NULL)
								break;
							int roadBlockNumber = atoi(segment);
							if(roadBlockNumber >= ((int) BlockList.size()))
							{
								printf("The Road Block is undefined!\n");
								exit(0);
							}
							RoadBlock TempRoad = BlockList[roadBlockNumber];
							TempRoad.type = ROAD_TYPE_EDGE;
							//printf("Lane number %d\n", laneNumber);
							temp.lane[laneNumber - 1].RoadBlockID.push_back(TempRoad);
							//printf("Fetched the road block number %d\n", roadBlockNumber);
							continue;
						}
						else if(!strcmp(segment, "Exit_Road_Block"))
						{
							segment = strtok(NULL, ",\n");
							if(segment == NULL)
								break;
							int exitRoadBlockIndex = atoi(segment);
							if(exitRoadBlockIndex >= ((int) BlockList.size()))
							{
								printf("The Exit_Road_Block is undefined!\n");
								exit(0);
							}
							temp.lane[laneNumber - 1].ExitRoadBlockIndex = exitRoadBlockIndex;
							//printf("Fetched the Exit_Road_Block index %d\n", exitRoadBlockIndex);
							break;
						}
						else 
						{
							printf("File format error: lane without Rb.\n");
							exit(0);
						}
					}
					continue;

				}
				else if(!strcmp(segment, "Edge_Block_End"))
				{
					//printf("pushed the edge\n");
					EdgeList.push_back(temp);
					break;
				}
			}
		}
		else if(!strcmp(segment, "Road_Block_Start"))
		{
			//Ready to initial a road data.
			RoadBlock temp;
			while(!feof(fptr))
			{
				line[0] = '\0';
				fgets(line, 2047, fptr);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				double x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, a, b, c;
				segment = strtok(line, "\t \n");
				if(sscanf(line, "(%lf,%lf),(%lf,%lf),(%lf,%lf),(%lf,%lf)", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4)==8)
				{
					temp.x[0] = x1;
					temp.y[0] = y1;
					temp.x[1] = x2;
					temp.y[1] = y2;
					temp.x[2] = x3;
					temp.y[2] = y3;
					temp.x[3] = x4;
					temp.y[3] = y4;
					//printf("Fetch 8 vertices successed.\n");
					continue;

				}
				else if(sscanf(line, "(%lf,%lf,%lf)", &a, &b, &c)==3)
				{
					temp.a = a;
					temp.b = b;
					temp.c = c;
					//printf("Fetch the line equality success.\n");
					continue;
				}
				else if(sscanf(line, "(%lf,%lf),(%lf,%lf)", &x5, &y5, &x6, &y6)==4)
				{
					temp.end_x[0] = x5;
					temp.end_y[0] = y5;
					temp.end_x[1] = x6;
					temp.end_y[1] = y6;
					//printf("Fetch two end points success.\n");
					continue;
				}
				else if(!strcmp(segment, "Direction_In_Angle"))
				{
					/* Normal coordinate direction not GUI viewd direction */
					segment = strtok(NULL, "\t \n");
					double angle = atof(segment);
					temp.direction = angle;
					//printf("Angle %lf\n", angle);
					continue;
				}
				else if(!strcmp(segment, "Road_Width"))
				{
					segment = strtok(NULL, "\t \n");
					RoadWidth = atof(segment);
					//printf("RoadWidth%lf\n", RoadWidth);
					continue;
				}
				else if(!strcmp(segment, "Road_Block_End"))
					break;
			}
			//printf("Push road block.\n");
			temp.RID = ((int)BlockList.size());
			BlockList.push_back(temp);
			continue;
		}
	}


	if((NodeList.size() == 0) && (BlockList.size() != 0))
	{
		printf("%s error: Road map needs at least one intersection\n", __func__);
		fflush(stdout);
		BlockList.clear();
		exit(0);
	}

	BlockList.clear();

	printf("Read road structure succeed.\n");

#if DEBUG_ROAD
	dumpRoadStructure();
#endif
}

void dumpRoadStructure()
{
	/* 
	 * A node represents an intersection.
	 * Node o	:	Nodes are connected by edge.
	 * Edge =====	:	Each edge may contains many lanes.
	 * Lane -----	:	Each lane may contains many road blocks.
	 * Road -	:	Road block is the smallest component of the road map.
	 *
	 *         Edge      Node   Edge    Node   Edge
	 *   ===============  o   =========  o   ========
	 *
	 */

	printf("\n=========== dump road structure start ========\n");

	/* dump node block */
	for(int i=0; i < ((int)NodeList.size()); ++i)
	{
		printf("Node %d, Edge %d, Lane %d, Block %d, NumEdge %d, \n  P1(%lf, %lf), P2(%lf, %lf), P3(%lf, %lf), P4(%lf, %lf)\n",
				i, -1, -1, -1, NodeList[i].NumOfEdges,
				NodeList[i].RoadBlockID.x[0], NodeList[i].RoadBlockID.y[0], 
				NodeList[i].RoadBlockID.x[1], NodeList[i].RoadBlockID.y[1], 
				NodeList[i].RoadBlockID.x[2], NodeList[i].RoadBlockID.y[2], 
				NodeList[i].RoadBlockID.x[3], NodeList[i].RoadBlockID.y[3]);
	}
	
	/* dump edge, lane, and road block */
	for(int i = 0; i< ((int)EdgeList.size()); ++i)
	{
		for(int j = 0; j < EdgeList[i].NumOfLane; ++j)
		{
			for(int k = 0; k < ((int)EdgeList[i].lane[j].RoadBlockID.size()); k++)
			{
				printf("Node %d, Edge %d, Lane %d, Block %d, Direction %lf,\n  P1(%lf, %lf), P2(%lf, %lf), P3(%lf, %lf), P4(%lf, %lf)\n",
				-1, i, j, k, EdgeList[i].lane[j].RoadBlockID[k].direction,
				EdgeList[i].lane[j].RoadBlockID[k].x[0], EdgeList[i].lane[j].RoadBlockID[k].y[0], 
				EdgeList[i].lane[j].RoadBlockID[k].x[1], EdgeList[i].lane[j].RoadBlockID[k].y[1], 
				EdgeList[i].lane[j].RoadBlockID[k].x[2], EdgeList[i].lane[j].RoadBlockID[k].y[2], 
				EdgeList[i].lane[j].RoadBlockID[k].x[3], EdgeList[i].lane[j].RoadBlockID[k].y[3]); 
			}
		}
	}
	printf("=========== dump road structure end ========\n\n");
}

inline int getRoadBlockByIndex (int NodeIndex, int EdgeIndex, int LaneIndex, int BlockIndex, RoadBlock &result)
{
	//printf("edge list size==%d, num of lanes=%d, road block size=%d\n", ((int)EdgeList.size()), EdgeList[EdgeIndex].NumOfLane, ((int)EdgeList[EdgeIndex].lane[LaneIndex].RoadBlockID.size()));
	if((NodeIndex >= 0) && 
		(NodeIndex < ((int)NodeList.size())) && 
		(EdgeIndex == -1) && 
		(LaneIndex == -1) && 
		(BlockIndex == -1))
	{
		result = NodeList[NodeIndex].RoadBlockID;
		return 1;
	}
	else if((NodeIndex == -1) && 
			(EdgeIndex >= 0) && 
			(EdgeIndex < ((int)EdgeList.size())) && 
			(LaneIndex >= 0) && 
			(LaneIndex < EdgeList[EdgeIndex].NumOfLane) && 
			(BlockIndex >= 0) && 
			(BlockIndex < ((int)EdgeList[EdgeIndex].lane[LaneIndex].RoadBlockID.size())))
	{
		result = EdgeList[EdgeIndex].lane[LaneIndex].RoadBlockID[BlockIndex];
		return 1;
	}

	/*
	printf("%s error!! NodeIndex %d, EdgeIndex %d, LaneIndex %d, BlockIndex %d\n",
			__func__, NodeIndex, EdgeIndex, LaneIndex, BlockIndex);
	fflush(stdout);
	*/
	return 0;
}

int getCurrentRoadIndex(double CurrentPOS_x, double CurrentPOS_y, int &NodeSerial,int &EdgeSerial, int &LaneSerial, int &BlockSerial, int CacheOrNot, CacheRecord *tmpCache)
{
	NodeSerial = -1;
	EdgeSerial = -1;	
	LaneSerial = -1;
	BlockSerial = -1;

	if(CacheOrNot == 1)
	{
		if(CheckCurrentCache(CurrentPOS_x, CurrentPOS_y, tmpCache) > 0)
		{
			// cache hit!!
			tmpCache->x = CurrentPOS_x;
			tmpCache->y = CurrentPOS_y;
			NodeSerial = tmpCache->CurrentNodeSerial;
			EdgeSerial = tmpCache->CurrentEdgeSerial;
			LaneSerial = tmpCache->CurrentLaneSerial;
			BlockSerial = tmpCache->CurrentBlockSerial;

			CacheHit++; 
			//printf("CacheHit: NodeSerial = %d , EdgeSerial = %d ,  LaneSerial = %d , BlockSerial = %d\n",NodeSerial, EdgeSerial, LaneSerial, BlockSerial);
			return 1; // find in cache
		}
	}

	CacheMiss++; 
	//printf("CacheHit %d, CacheMiss %d\n", CacheHit, CacheMiss);
	//fflush(stdout);

	double minimumAngle = PI/360;
	//double minimumAngle = 1/PI;
	int numOfEdges = (int)EdgeList.size();
	int numOfNodes = (int)NodeList.size();
	int findFlag = 0;

	//printf("target angle %lf\n", minimumAngle);
	
	/* when the car is on the node */

	for(int i = 0; i < numOfNodes; ++i)
	{
		double angle = CheckNodeInAreaAngle(CurrentPOS_x, CurrentPOS_y, 
				NodeList[i].RoadBlockID.x[0], NodeList[i].RoadBlockID.y[0], 
				NodeList[i].RoadBlockID.x[1], NodeList[i].RoadBlockID.y[1], 
				NodeList[i].RoadBlockID.x[2], NodeList[i].RoadBlockID.y[2], 
				NodeList[i].RoadBlockID.x[3], NodeList[i].RoadBlockID.y[3]);
		if(minimumAngle < angle)
			continue;
		minimumAngle = angle;
		NodeSerial = i;	
		EdgeSerial = -1;	
		LaneSerial = -1;
		BlockSerial = -1;
		findFlag = 1;
		// the angle is small enough 
		if(minimumAngle <= 0.000001)
			goto searchDone;
	}

	//printf("minimumAngle after search node %lf\n", minimumAngle);

	/* when the car is on the edge */
	if(!findFlag)
	{
		// search the edge in cache first
		if(tmpCache->CurrentEdgeSerial >= 0)
		{
			int i = tmpCache->CurrentEdgeSerial;
			int j = tmpCache->CurrentLaneSerial;
			for(int k = 0; k < ((int)EdgeList[i].lane[j].RoadBlockID.size()); ++k)
			{
				double angle = CheckNodeInAreaAngle(CurrentPOS_x, CurrentPOS_y, 
				EdgeList[i].lane[j].RoadBlockID[k].x[0], EdgeList[i].lane[j].RoadBlockID[k].y[0], 
				EdgeList[i].lane[j].RoadBlockID[k].x[1], EdgeList[i].lane[j].RoadBlockID[k].y[1], 
				EdgeList[i].lane[j].RoadBlockID[k].x[2], EdgeList[i].lane[j].RoadBlockID[k].y[2], 
				EdgeList[i].lane[j].RoadBlockID[k].x[3], EdgeList[i].lane[j].RoadBlockID[k].y[3]);
				if(minimumAngle < angle)
					continue;
				minimumAngle = angle;
				NodeSerial = -1;
				EdgeSerial = i;
				LaneSerial = j;
				BlockSerial = k;
				findFlag = 1;
				// the angle is small enough 
				if(minimumAngle <= 0.000001)
					goto searchDone;
			}
			if(findFlag)
				goto searchDone;
		}

		for(int i = 0; i < numOfEdges; ++i)
		{
			// printf("num of lane==%d\n", EdgeList[i].NumOfLane);
			for(int j = 0; j < EdgeList[i].NumOfLane; ++j)
			{

				if(i == tmpCache->CurrentEdgeSerial && j == tmpCache->CurrentLaneSerial)
					continue;	// already searched before

				for(int k = 0; k < ((int)EdgeList[i].lane[j].RoadBlockID.size()); ++k)
				{
					double angle = CheckNodeInAreaAngle(CurrentPOS_x, CurrentPOS_y, 
				EdgeList[i].lane[j].RoadBlockID[k].x[0], EdgeList[i].lane[j].RoadBlockID[k].y[0], 
				EdgeList[i].lane[j].RoadBlockID[k].x[1], EdgeList[i].lane[j].RoadBlockID[k].y[1], 
				EdgeList[i].lane[j].RoadBlockID[k].x[2], EdgeList[i].lane[j].RoadBlockID[k].y[2], 
				EdgeList[i].lane[j].RoadBlockID[k].x[3], EdgeList[i].lane[j].RoadBlockID[k].y[3]);
					if(minimumAngle < angle)
						continue;
					minimumAngle = angle;
					NodeSerial = -1;
					EdgeSerial = i;
					LaneSerial = j;
					BlockSerial = k;
					// the angle is small enough 
					if(minimumAngle <= 0.000001)
						goto searchDone;
				}
			}
		}
	}

searchDone:

	if(CacheOrNot == 1)
	{
		//printf("Cached!! NS %d, ES %d, LS %d, BS %d\n", NodeSerial, EdgeSerial, LaneSerial, BlockSerial);
		tmpCache->RefreshCurrent(NodeSerial, EdgeSerial, LaneSerial, BlockSerial);
		tmpCache->x = CurrentPOS_x;
		tmpCache->y = CurrentPOS_y;
		tmpCache->NextDirty = 0;
		tmpCache->AfterDirty = 0;
		tmpCache->CurrentDirty = 1;
	}

	//printf("minimumAngle after search edge %lf\n", minimumAngle);

	//if(minimumAngle < (1/PI))
	if(minimumAngle < (PI/360))
		return 1; // sucess
	else 
		return 0; // fail
}

int getCurrentRoadInformation(double CurrentPOS_x, double CurrentPOS_y, int &NumOfDirections, double *&direction, double &a, double &b, double &c, double &endPoint_x, double &endPoint_y, int &RoadType, int CacheOrNot, CacheRecord *myCache)
{
	/* Note: a, b, c are linear equation coefficients (ax+by+c) of the road block */

	int n;
	int nodeSerial, edgeSerial, laneSerial, blockSerial;
	if((n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, nodeSerial, edgeSerial, laneSerial, blockSerial, CacheOrNot, myCache)) <= 0){
		printf("[%s]: getCurrentRoadIndex error !!!\n", __func__);
		return -1;
	}

	RoadBlock CurrentRoad;
	if((n = getRoadBlockByIndex(nodeSerial, edgeSerial, laneSerial, blockSerial, CurrentRoad)) <= 0){
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return -2;
	}
	RoadType = CurrentRoad.type;
	if(CurrentRoad.type == ROAD_TYPE_EDGE)
	{
		double tempAngle1 = atan2(CurrentRoad.end_y[0] - CurrentRoad.end_y[1], CurrentRoad.end_x[0] - CurrentRoad.end_x[1]) / PI * 180;
		double tempAngle2 = atan2(CurrentRoad.end_y[1] - CurrentRoad.end_y[0], CurrentRoad.end_x[1] - CurrentRoad.end_x[0]) / PI * 180;
		double diff1 = fabs(CurrentRoad.direction - tempAngle1);
		if(diff1 > 180)
			diff1 = 360 - diff1;
		double diff2 = fabs(CurrentRoad.direction - tempAngle2);
		if(diff2 > 180)
			diff2 = 360 - diff2;
		if(diff1 < diff2)
		{
			endPoint_x = CurrentRoad.end_x[0];
			endPoint_y = CurrentRoad.end_y[0];
		}
		else
		{
			endPoint_x = CurrentRoad.end_x[1];
			endPoint_y = CurrentRoad.end_y[1];
		}
		a = CurrentRoad.a;
		b = CurrentRoad.b;
		c = CurrentRoad.c;
		NumOfDirections = 1;
		//Modify for the direction degree from normal coordinate to GUI viewed direction
		direction = new double[NumOfDirections];
		direction[0] = fmod(360 - CurrentRoad.direction, 360);
	}
	else if(CurrentRoad.type == ROAD_TYPE_NODE)
	{
		a = 0;
		b = 0;
		c = 0;
		NumOfDirections = NodeList[nodeSerial].NumOfEdges;
		//Modify for the direction degree from normal coordinate to GUI viewed direction
		direction = new double[NumOfDirections];
		for(int i=0; i< NumOfDirections; ++i)
			direction[i] = fmod(360 - NodeList[nodeSerial].direction[i], 360);
	}
	return 1;
}

/*
 * CurrentDirection is the direction of normal coordinate
 */
int getNextRoadIndex(double CurrentPOS_x, double CurrentPOS_y, double CurrentDirection, int &NextNodeIndex, int &NextEdgeIndex, int &NextLaneIndex, int &NextBlockIndex, int CacheOrNot, CacheRecord *myCache)
{
	int CurrentNodeSerial, CurrentEdgeSerial, CurrentLaneSerial, CurrentBlockSerial;
	int n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, CurrentNodeSerial, CurrentEdgeSerial, CurrentLaneSerial, CurrentBlockSerial, CacheOrNot, myCache);
	if(n <= 0)
	{
		printf("[%s]: getCurrentRoadIndex error !!!\n", __func__);
		return -1;
	}
	RoadBlock CurrentRoad;
	n = getRoadBlockByIndex(CurrentNodeSerial, CurrentEdgeSerial, CurrentLaneSerial, CurrentBlockSerial, CurrentRoad);
	if(n <= 0){
		printf("[%s]: getRoadBlockByIndex error\n", __func__);
		return -2;
	}
	if(CurrentRoad.type == ROAD_TYPE_EDGE){
		if(CacheOrNot)
			if((myCache->NextDirty == 1) && (myCache->CurrentDirty == 1))
			{
				NextNodeIndex = myCache->NextNodeSerial;
				NextEdgeIndex = myCache->NextEdgeSerial;
				NextLaneIndex = myCache->NextLaneSerial;
				NextBlockIndex = myCache->NextBlockSerial;
				//printf("<<- use Next Cache ->> NextNodeIndex %d , NextEdgeIndex %d , NextLaneIndex %d , NextBlockIndex %d\n", NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex);
				return 1;
			}
		if(CurrentBlockSerial < EdgeList[CurrentEdgeSerial].lane[CurrentLaneSerial].ExitRoadBlockIndex)
		{
			// find next road block on the same edge and lane
			NextNodeIndex = -1;
			NextEdgeIndex = CurrentEdgeSerial;
			NextLaneIndex = CurrentLaneSerial;
			NextBlockIndex = CurrentBlockSerial + 1;
		}
		else if(CurrentBlockSerial > EdgeList[CurrentEdgeSerial].lane[CurrentLaneSerial].ExitRoadBlockIndex)
		{
			// find next road block on the same edge and lane
			NextNodeIndex = -1;
			NextEdgeIndex = CurrentEdgeSerial;
			NextLaneIndex = CurrentLaneSerial;
			NextBlockIndex = CurrentBlockSerial - 1;
		}
		else
		{
			// The car agent has reached the end of the edge
			if(CurrentBlockSerial == 0)
				NextNodeIndex = EdgeList[CurrentEdgeSerial].NID[0];
			else 
				NextNodeIndex = EdgeList[CurrentEdgeSerial].NID[1];
			NextEdgeIndex = -1;
			NextLaneIndex = -1;
			NextBlockIndex = -1;
		}
	}
	else if(CurrentRoad.type == ROAD_TYPE_NODE){
		//printf("[%s]: At Intersection, call takingturns to leave here.\n", __func__);
		myCache->NextDirty = myCache->AfterDirty = 0;//Warning clear the cache 
		NextNodeIndex = -1;
		double AngleDiff;
		double MiniAngleDiff = 999;
		int MiniEdgeIndexInNode = -1;

		if(NodeList[CurrentNodeSerial].type == NODE_TYPE_I || NodeList[CurrentNodeSerial].type == NODE_TYPE_T || NodeList[CurrentNodeSerial].type == NODE_TYPE_L)
		{
			for(int i = 0; i < NodeList[CurrentNodeSerial].NumOfEdges; ++i) //no turn
			{
				AngleDiff = fabs(CurrentDirection - NodeList[CurrentNodeSerial].direction[i]);
				if(AngleDiff > 180)
					AngleDiff = 360 - AngleDiff;
				if(AngleDiff < 5)
				{
					MiniEdgeIndexInNode = i;
					break;
				}
			}
			if(MiniEdgeIndexInNode == -1);//turn right or left
			{
				for(int i = 0; i < NodeList[CurrentNodeSerial].NumOfEdges; ++i)
				{
					AngleDiff = fabs(CurrentDirection - NodeList[CurrentNodeSerial].direction[i]);
					if(AngleDiff > 180)
						AngleDiff = 360 - AngleDiff;
					if(AngleDiff < MiniAngleDiff)
					{
						MiniEdgeIndexInNode = i;
						MiniAngleDiff = AngleDiff;
					}
				}
			}
		}
		else
		{
			// fail in reduce_node and circular_node
			return -7;
		}
		int tempEdgeIndex = NodeList[CurrentNodeSerial].EID[MiniEdgeIndexInNode];
		int tempBlockIndex, tempLaneIndex;
		if(NodeList[CurrentNodeSerial].HeadOrNot[MiniEdgeIndexInNode] == true)
			tempBlockIndex = 0;
		else
			tempBlockIndex = EdgeList[tempEdgeIndex].lane[0].RoadBlockID.size() - 1; // Lanes in the same edge has the same size. 		
		double AngleDiff1 = fabs(EdgeList[tempEdgeIndex].lane[EdgeList[tempEdgeIndex].NumOfLane/2 - 1].RoadBlockID[tempBlockIndex].direction - CurrentDirection);
		if(AngleDiff1 > 180)
			AngleDiff1 = 360 - AngleDiff1;
		double AngleDiff2 = fabs(EdgeList[tempEdgeIndex].lane[EdgeList[tempEdgeIndex].NumOfLane/2].RoadBlockID[tempBlockIndex].direction - CurrentDirection);
		if(AngleDiff2 > 180)
			AngleDiff2 = 360 - AngleDiff2;
		if(AngleDiff1 < AngleDiff2)
			tempLaneIndex = 0;
		else
			tempLaneIndex = EdgeList[tempEdgeIndex].NumOfLane - 1;

		NextLaneIndex = tempLaneIndex;
		NextBlockIndex = tempBlockIndex;
		NextEdgeIndex = tempEdgeIndex;
	}

	if(CacheOrNot == 1)
	{
		myCache->RefreshNext(NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex);
		myCache->NextDirty = 1;
		myCache->AfterDirty = 0;
		//printf(">>- Find Next Serial -<< NextNodeIndex %d , NextEdgeIndex %d , NextLaneIndex %d , NextBlockIndex %d\n", NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex);
	}
	return 1;

}

int getNeighboringLaneInformation(double CurrentPOS_x, double CurrentPOS_y, int RightOrLeft, /*int &NumOfDirections,*/ double &direction, /*double &a, double &b, double &c ,double &endPoint_x, double &endPoint_y,*/ int CacheOrNot, CacheRecord *myCache)
{
	int NodeSerial, EdgeSerial, LaneSerial, BlockSerial,neighboringLaneSerial;
	int n = 0;

	n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, NodeSerial, EdgeSerial, LaneSerial, BlockSerial, CacheOrNot, myCache);
	if(n <= 0)
	{
		printf("[%s]: getCurrentRoadIndex error !!!\n", __func__);
		return -1;
	}
	RoadBlock CurrentRoad;
	n = getRoadBlockByIndex(NodeSerial, EdgeSerial, LaneSerial, BlockSerial, CurrentRoad);
	if(n <= 0)
	{
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return -2;
	}

	int right, left;
	int halfNumOfLane = EdgeList[EdgeSerial].NumOfLane / 2;

	if(LaneSerial < halfNumOfLane)
	{
		/* left-hand side of the lane */
		right = -1;
		left = 1;
	}
	else
	{
		/* right-hand side of the lane */
		right = 1;
		left = -1;
	}
	if(CurrentRoad.type == ROAD_TYPE_EDGE)
	{
		if((BlockSerial == 0) || (BlockSerial == ((int)EdgeList[EdgeSerial].lane[LaneSerial].RoadBlockID.size() - 1)))
		{
			// in the entrance or exit road block of the lane
			return 0;
		}
		if(RightOrLeft == RIGHT)
			neighboringLaneSerial = LaneSerial + right;
		else
			neighboringLaneSerial = LaneSerial + left;

		if((neighboringLaneSerial < 0) || (neighboringLaneSerial >= EdgeList[EdgeSerial].NumOfLane))
		{
			// neighboringLaneSerial is out of range
			return 0;
		}

		if((left == 1) && (neighboringLaneSerial >= halfNumOfLane))
		{
			// The direction of neighboringLaneSerial is different than me
			return 0;
		} 
		else if ((right == 1) && (neighboringLaneSerial < halfNumOfLane))
		{
			// The direction of neighboringLaneSerial is different than me
			return 0;
		}
		else ;
	}
	else
		return 0;

	RoadBlock NeighboringRoad;
	n = getRoadBlockByIndex(NodeSerial, EdgeSerial, neighboringLaneSerial, BlockSerial, NeighboringRoad);
	if(n <= 0)
	{
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return 0;
	}

	//printf(">>- Find Next Serial -<< NextNodeIndex %d , NextEdgeIndex %d , NextLaneIndex %d , NextBlockIndex %d\n", NodeSerial, EdgeSerial, neighboringLaneSerial, BlockSerial); 
	
	//Modify for the direction from mathematical to GUI degree
	direction = fmod(360 - NeighboringRoad.direction, 360);

	/*
	a = NeighboringRoad.a;
	b = NeighboringRoad.b;
	c = NeighboringRoad.c;
	double tempAngle1 = atan2(NeighboringRoad.end_y[0] - NeighboringRoad.end_y[1], NeighboringRoad.end_x[0] - NeighboringRoad.end_x[1]) / PI * 180;
	double tempAngle2 = atan2(NeighboringRoad.end_y[1] - NeighboringRoad.end_y[0], NeighboringRoad.end_x[1] - NeighboringRoad.end_x[0]) / PI * 180;
	double diff1 = fabs(NeighboringRoad.direction - tempAngle1);
	if(diff1 > 180)
		diff1 = 360 - diff1;
	double diff2 = fabs(NeighboringRoad.direction - tempAngle2);
	if(diff2 > 180)
		diff2 = 360 - diff2;
	if(diff1 < diff2)
	{
		endPoint_x = NeighboringRoad.end_x[0];
		endPoint_y = NeighboringRoad.end_y[0];
	}
	else
	{
		endPoint_x = NeighboringRoad.end_x[1];
		endPoint_y = NeighboringRoad.end_y[1];
	}
	*/

	if(CacheOrNot == 1)
	{
		myCache->RefreshNext(NodeSerial, EdgeSerial, neighboringLaneSerial, BlockSerial);
		myCache->NextDirty = 1;
		myCache->AfterDirty = 0;
	}
	return 1;

}

/*
 * This API returns the directions so that car can choose which direction to go.
 *
 * Note:
 *    When there's only one exit in the intersection, this API supports the directions for U turn.
 *    The exit of the intersection may be connected by many edges in the loaded real-world Road Map.
 */

int getNextRoadInformation(double CurrentPOS_x, double CurrentPOS_y, double CurrentDirection, int &NumOfDirections, double *&direction, double &a, double &b, double &c, int CacheOrNot, CacheRecord *myCache)
{
	CurrentDirection = fmod(360 - CurrentDirection, 360);
	int NodeSerial, EdgeSerial, LaneSerial, BlockSerial;
	int n = getNextRoadIndex(CurrentPOS_x, CurrentPOS_y, CurrentDirection, NodeSerial, EdgeSerial, LaneSerial, BlockSerial, CacheOrNot, myCache);
	if(n <= 0)
		return 0;

	RoadBlock NextRoad;
	n = getRoadBlockByIndex(NodeSerial, EdgeSerial, LaneSerial, BlockSerial, NextRoad);
	if(n <= 0)
	{
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return 0;
	}

	if(NextRoad.type == ROAD_TYPE_EDGE)
	{
		a = NextRoad.a;
		b = NextRoad.b;
		c = NextRoad.c;
		NumOfDirections = 1;
		direction = new double[NumOfDirections];
		//Modify for the direction degree from normal coordinate to GUI viewed direction
		direction[0] = fmod(360 - NextRoad.direction, 360);
	}
	else if(NextRoad.type == ROAD_TYPE_NODE)
	{
		a = 0;
		b = 0;
		c = 0;
		NumOfDirections = NodeList[NodeSerial].NumOfEdges;
		
		if(NumOfDirections == 0)
		{
			printf("[%s] error: no directions can choose !!!\n", __func__);
			return 0;
		}

		int j = 1;
		double tmpDirection = NodeList[NodeSerial].direction[0];

		/* Check if there's more than one exit in this intersection. */
		for(int i = 1; i < NumOfDirections; ++i)
		{
			double angleDiff = fabs(tmpDirection - NodeList[NodeSerial].direction[i]);
			if(angleDiff > 180)
				angleDiff = 360 - angleDiff;

			if(angleDiff < 0.01)
			{
				j++;
			}
		}
		
		if(j == NumOfDirections)
		{
			/* This intersection has only one exit (no matter how many edges connect to it). 
			 * In this situation, we support U turn.
			 */
			direction = new double[NumOfDirections];

			for(int i = 0; i < NumOfDirections; ++i)
			{
				// translate into GUI viewed direction
				direction[i] = fmod(360 - NodeList[NodeSerial].direction[i], 360);
			}
		}
		else
		{
			/* There is more than one exits in the intersection */
			int *index = new int[NumOfDirections];
			int numOfIndex = 0;

			// find directions to go through the intersection
			for(int i = 0; i < NumOfDirections; ++i)
			{
				double AngleDiff = fabs(fmod(CurrentDirection + 180, 360) -  NodeList[NodeSerial].direction[i]);
				if(AngleDiff > 180)
					AngleDiff = 360 - AngleDiff;

				if(AngleDiff < 5)
				{
					/* Doesn't support U turn when there's other
					 * exit to go out of this intersection
					 */
					continue;
				}
				index[numOfIndex] = i;
				numOfIndex++;
			}

			NumOfDirections = numOfIndex;
			direction = new double[numOfIndex];

			for(int i = 0; i < NumOfDirections; ++i)
			{
				int tmpI = index[i];

				direction[i] = fmod(360 - NodeList[NodeSerial].direction[tmpI], 360);
			}
			delete index;
		}
	}

	return 1;
}

/* This API returns the next point and direction when car wants to make a turn */
int takeATurn(double CurrentPOS_x, double CurrentPOS_y, double CurrentDirection, double NextDirection, int &NumOfTurningPoints, double *&TurningPointsPOS_x, double *&TurningPointsPOS_y, double &FirstPointAfterTheTurn_x, double &FirstPointAfterTheTurn_y, double *&directionAlongTwoConsecutiveTurningPoints, int CacheOrNot, CacheRecord *myCache)
{
	//Modify for the direction degree GUI coordinate to normal coordinate
	CurrentDirection = fmod(360 - CurrentDirection, 360);
	NextDirection = fmod(360 - NextDirection, 360);

	int CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex, NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex;

	int n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex, CacheOrNot, myCache);
	if(n <= 0) 
	{
		printf("[%s]: getCurrentRoadIndex error !!!\n", __func__);
		return -1;
	}

	n = getNextRoadIndex(CurrentPOS_x, CurrentPOS_y, CurrentDirection, NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex, CacheOrNot, myCache);
	if(n <= 0) return -2;

	RoadBlock CurrentRoad, NextRoad;
	if((n = getRoadBlockByIndex(CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex, CurrentRoad)) <= 0)
	{
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return -3;
	}

	if((n = getRoadBlockByIndex(NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex, NextRoad)) <= 0)
	{
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return -4;
	}

#if DEBUG_ROAD
	printf("CurRoad (%d, %d, %d, %d), NextRoad (%d, %d, %d, %d)\n",
			CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex,
			NextNodeIndex, NextEdgeIndex, NextLaneIndex, NextBlockIndex);
#endif
	if(NextRoad.type == ROAD_TYPE_EDGE)
	{
		double angleDiff = fabs(NextRoad.direction - NextDirection);

		if(angleDiff >= 180)
			angleDiff = 360 - angleDiff;
		if(angleDiff >= 10) //The turn which agent wants to take doesn't follow the next road's direction.
			return -5;

		NumOfTurningPoints = 0;
		TurningPointsPOS_x = NULL; 
		TurningPointsPOS_y = NULL;
		directionAlongTwoConsecutiveTurningPoints = NULL; 

		double tempAngle1 = fmod(atan2(CurrentRoad.end_y[0] - CurrentRoad.end_y[1], CurrentRoad.end_x[0] - CurrentRoad.end_x[1]) / PI * 180 + 360, 360);
		double tempAngle2 = fmod(atan2(CurrentRoad.end_y[1] - CurrentRoad.end_y[0], CurrentRoad.end_x[1] - CurrentRoad.end_x[0]) / PI * 180 + 360, 360);

		double diff1 = fabs(CurrentRoad.direction - tempAngle1);
		if(diff1 > 180)
			diff1 = 360 - diff1;
		double diff2 = fabs(CurrentRoad.direction - tempAngle2);
		if(diff2 > 180)
			diff2 = 360 - diff2;

		if(diff1 < diff2)
		{
			FirstPointAfterTheTurn_x = CurrentRoad.end_x[0];
			FirstPointAfterTheTurn_y = CurrentRoad.end_y[0];
		}
		else
		{
			FirstPointAfterTheTurn_x = CurrentRoad.end_x[1];
			FirstPointAfterTheTurn_y = CurrentRoad.end_y[1];
		}

		return 1;
	}
	else if(NextRoad.type == ROAD_TYPE_NODE)
	{
		// In node
		double angleBetweenTwoDirections;

		angleBetweenTwoDirections = fabs(NextDirection - CurrentDirection);
		if(angleBetweenTwoDirections > 180)
			angleBetweenTwoDirections = 360 - angleBetweenTwoDirections;

		if(NodeList[NextNodeIndex].type == NODE_TYPE_R)
		{
			/* This node block is a "Lane-Reduction" intersection */

			double MiniAngleDiff = 999;
			int MiniEdgeIndexInNode;
			for(int i=0 ; i<NodeList[NextNodeIndex].NumOfEdges ; ++i)
			{
				double AngleDiff;
				AngleDiff = fabs(NextDirection - NodeList[NextNodeIndex].direction[i]);
				if(AngleDiff > 180)
					AngleDiff = 360 - AngleDiff;

				if(AngleDiff < MiniAngleDiff)
				{
					MiniAngleDiff = AngleDiff;
					MiniEdgeIndexInNode = i;
				}
			}
			if(MiniAngleDiff > 5)
				return -6; // no way out

			int tempEdgeIndex = NodeList[NextNodeIndex].EID[MiniEdgeIndexInNode];
			int tempBlockIndex;

			if(NodeList[NextNodeIndex].HeadOrNot[MiniEdgeIndexInNode] == true)
				tempBlockIndex = 0;
			else
				tempBlockIndex = EdgeList[tempEdgeIndex].lane[0].RoadBlockID.size() - 1;

			int LaneDiff;

			//find the difference between CurrentLane and MiddleLane
			if(CurrentLaneIndex < (EdgeList[CurrentEdgeIndex].NumOfLane/2))
				LaneDiff = (EdgeList[CurrentEdgeIndex].NumOfLane/2) - 1 - CurrentLaneIndex;
			else
				LaneDiff = CurrentLaneIndex - (EdgeList[CurrentEdgeIndex].NumOfLane/2);

			int tempLaneIndex1, tempLaneIndex2;
			if((EdgeList[CurrentEdgeIndex].NumOfLane > EdgeList[tempEdgeIndex].NumOfLane) && ((CurrentLaneIndex == 0) || (CurrentLaneIndex == (EdgeList[CurrentEdgeIndex].NumOfLane - 1))))//reduce the lane and the car need turn
			{
				tempLaneIndex1 = 0;
				tempLaneIndex2 = EdgeList[tempEdgeIndex].NumOfLane - 1;
				NumOfTurningPoints = 1;
				directionAlongTwoConsecutiveTurningPoints = new double[NumOfTurningPoints];
				TurningPointsPOS_x = new double[NumOfTurningPoints];
				TurningPointsPOS_y = new double[NumOfTurningPoints];
			}
			else//do like no turn
			{
				NumOfTurningPoints = 0;
				directionAlongTwoConsecutiveTurningPoints = NULL;
				TurningPointsPOS_x = NULL;
				TurningPointsPOS_y = NULL;
				tempLaneIndex1 = EdgeList[tempEdgeIndex].NumOfLane/2 -1 - LaneDiff;
				tempLaneIndex2 = EdgeList[tempEdgeIndex].NumOfLane/2 + LaneDiff;
			}
			double AngleDiff1 = fabs (EdgeList[tempEdgeIndex].lane[tempLaneIndex1].RoadBlockID[tempBlockIndex].direction - NextDirection);
			if(AngleDiff1 > 180)
				AngleDiff1 = 360 - AngleDiff1;
			double AngleDiff2 = fabs (EdgeList[tempEdgeIndex].lane[tempLaneIndex2].RoadBlockID[tempBlockIndex].direction - NextDirection);	
			if (AngleDiff2 > 180)
				AngleDiff2 = 360 - AngleDiff2;
			int tempLaneIndex;
			if(AngleDiff1 < AngleDiff2)
				tempLaneIndex = tempLaneIndex1;
			else
				tempLaneIndex = tempLaneIndex2;
			RoadBlock RoadAfterTurn;
			if((n = getRoadBlockByIndex (-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex, RoadAfterTurn)) <= 0)
			{
				printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
				return -7;
			}
			double distance1 = Distance_BetweenTwoNode(CurrentRoad.end_x[0], CurrentRoad.end_y[0], RoadAfterTurn.end_x[0], RoadAfterTurn.end_y[0]);
			double distance2 = Distance_BetweenTwoNode(CurrentRoad.end_x[0], CurrentRoad.end_y[0], RoadAfterTurn.end_x[1], RoadAfterTurn.end_y[1]);
			if(distance1 < distance2)
			{
				FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[0];
				FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[0];
			}	
			else
			{
				FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[1];
				FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[1];
			}
			// reduce the lane and the car need turn
			if((EdgeList[CurrentEdgeIndex].NumOfLane > EdgeList[tempEdgeIndex].NumOfLane) && ((CurrentLaneIndex == 0) || (CurrentLaneIndex == (EdgeList[CurrentEdgeIndex].NumOfLane - 1))))
			{
				distance1 = Distance_BetweenTwoNode(FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, CurrentRoad.end_x[0], CurrentRoad.end_y[0]);
				distance2 = Distance_BetweenTwoNode(FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y, CurrentRoad.end_x[1], CurrentRoad.end_y[1]);
				if(distance1 < distance2)
				{
					TurningPointsPOS_x[0] = CurrentRoad.end_x[0];
					TurningPointsPOS_y[0] = CurrentRoad.end_y[0];
				}
				else
				{
					TurningPointsPOS_x[0] = CurrentRoad.end_x[1];
					TurningPointsPOS_y[0] = CurrentRoad.end_y[1];
				}
				double tempDirections = fmod(atan2(FirstPointAfterTheTurn_y - TurningPointsPOS_y[0], FirstPointAfterTheTurn_x - TurningPointsPOS_x[0]) /PI * 180 + 360, 360);
				directionAlongTwoConsecutiveTurningPoints[0] = fmod(360 - tempDirections, 360);
			}
			myCache->RefreshAfter(-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex);
			myCache->AfterDirty = 1;
			return 1;
		}
		else 
		{
			/* This node block is an intersection */

			if(angleBetweenTwoDirections < 5) // It does not take any turn.
			{
				NumOfTurningPoints = 0;
				directionAlongTwoConsecutiveTurningPoints = NULL;
				TurningPointsPOS_x = NULL; // no TurningPointsPOS
				TurningPointsPOS_y = NULL;

				double MiniAngleDiff = 999;
				int MiniEdgeIndexInNode;
				int NumOfEdges = 0;
				int NumOfTheSameExitEdge = 0;
				int *edgesWithTheSameExit = NULL;

				NumOfEdges = NodeList[NextNodeIndex].NumOfEdges;

				edgesWithTheSameExit = new int[NumOfEdges];

				// search the exit edge of the intersection
				for(int i=0 ; i<NodeList[NextNodeIndex].NumOfEdges ; i++)
				{
					double AngleDiff;
					AngleDiff = fabs(NextDirection - NodeList[NextNodeIndex].direction[i]);
					if(AngleDiff > 180)
						AngleDiff = 360 - AngleDiff;

					if(AngleDiff <= 0.01)
					{
						/* There may be many edges which all connect
						 * to the same exit of the intersection
						 * on the loaded real-world Road Map
						 */
						MiniAngleDiff = AngleDiff;
						edgesWithTheSameExit[NumOfTheSameExitEdge] = i;
						NumOfTheSameExitEdge++;
					}
				}

				if(MiniAngleDiff > 0.01)
					return -8;	//Turn to no way

				// randomly choose one edge
				MiniEdgeIndexInNode = edgesWithTheSameExit[rand() % NumOfTheSameExitEdge];

				delete edgesWithTheSameExit;

				int tempEdgeIndex = NodeList[NextNodeIndex].EID[MiniEdgeIndexInNode];	
				int tempBlockIndex;
				if(NodeList[NextNodeIndex].HeadOrNot[MiniEdgeIndexInNode] == true)
					tempBlockIndex = 0;
				else
					tempBlockIndex = EdgeList[tempEdgeIndex].lane[0].RoadBlockID.size() - 1;

				int tempLaneIndex1 = CurrentLaneIndex;
				int tempLaneIndex2 = EdgeList[tempEdgeIndex].NumOfLane - CurrentLaneIndex - 1;
				double AngleDiff1 = fabs (EdgeList[tempEdgeIndex].lane[tempLaneIndex1].RoadBlockID[tempBlockIndex].direction - NextDirection);
				if(AngleDiff1 > 180)
					AngleDiff1 = 360 - AngleDiff1;
				double AngleDiff2 = fabs (EdgeList[tempEdgeIndex].lane[tempLaneIndex2].RoadBlockID[tempBlockIndex].direction - NextDirection);	
				if (AngleDiff2 > 180)
					AngleDiff2 = 360 - AngleDiff2;
				int tempLaneIndex;
				if(AngleDiff1 < AngleDiff2)
					tempLaneIndex = tempLaneIndex1;
				else
					tempLaneIndex = tempLaneIndex2;
				RoadBlock RoadAfterTurn;
				n = getRoadBlockByIndex (-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex, RoadAfterTurn);
				if(n <= 0)
				{
					printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
					return -9;
				}
				double distance1 = Distance_BetweenTwoNode(CurrentRoad.end_x[0], CurrentRoad.end_y[0], RoadAfterTurn.end_x[0], RoadAfterTurn.end_y[0]);
				double distance2 = Distance_BetweenTwoNode(CurrentRoad.end_x[0], CurrentRoad.end_y[0], RoadAfterTurn.end_x[1], RoadAfterTurn.end_y[1]);
				if(distance1 < distance2)
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[0];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[0];
				}	
				else
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[1];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[1];
				}
				myCache->RefreshAfter(-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex);
				myCache->AfterDirty = 1;
				return 1;
			}
			else if(fabs(angleBetweenTwoDirections - 90) <= 5)//It turns left or right.
			{
				//printf("DEBUG: Turn right or left\n");
				NumOfTurningPoints = 2;
				TurningPointsPOS_x = new double[NumOfTurningPoints];
				TurningPointsPOS_y = new double[NumOfTurningPoints];
				directionAlongTwoConsecutiveTurningPoints = new double[NumOfTurningPoints];

				double MiniAngleDiff = 999;
				int MiniEdgeIndexInNode;
				int NumOfEdges = 0;
				int NumOfTheSameExitEdge= 0;
				int *edgesWithTheSameExit = NULL;

				NumOfEdges = NodeList[NextNodeIndex].NumOfEdges;

				edgesWithTheSameExit = new int[NumOfEdges];

				// search the exit edge of the intersection
				for(int i = 0; i < NumOfEdges; i++)
				{
					double AngleDiff;
					AngleDiff = fabs(NextDirection - NodeList[NextNodeIndex].direction[i]);
					if(AngleDiff > 180)
						AngleDiff = 360 - AngleDiff;

					if(AngleDiff <= 0.01){
						/* There may be many edges which all connect
						 * to the same exit of the intersection
						 * on the loaded real-world Road Map
						 */
						MiniAngleDiff = AngleDiff;
						edgesWithTheSameExit[NumOfTheSameExitEdge] = i;
						NumOfTheSameExitEdge++;
					}
				}
				if(MiniAngleDiff > 0.01)
					return -10; //Turn to no way.

				// randomly choose one edge
				MiniEdgeIndexInNode = edgesWithTheSameExit[rand() % NumOfTheSameExitEdge];
				delete edgesWithTheSameExit;

				int tempEdgeIndex = NodeList[NextNodeIndex].EID[MiniEdgeIndexInNode];
				int tempBlockIndex;
				if(NodeList[NextNodeIndex].HeadOrNot[MiniEdgeIndexInNode] == true)
					tempBlockIndex = 0;
				else 
					tempBlockIndex = EdgeList[tempEdgeIndex].lane[0].RoadBlockID.size() - 1; // Lanes in the same edge has the same size. 
				int base;
				double AngleDiff1 = fabs(EdgeList[tempEdgeIndex].lane[EdgeList[tempEdgeIndex].NumOfLane/2 - 1].RoadBlockID[tempBlockIndex].direction - NextDirection);
				if(AngleDiff1 > 180) 
					AngleDiff1 = 360 - AngleDiff1;

				double AngleDiff2 = fabs(EdgeList[tempEdgeIndex].lane[EdgeList[tempEdgeIndex].NumOfLane/2].RoadBlockID[tempBlockIndex].direction - NextDirection);
				if(AngleDiff2 > 180) 
					AngleDiff2 = 360 - AngleDiff2;

				if(AngleDiff1 < AngleDiff2)
					base = 0;
				else 
					base = EdgeList[tempEdgeIndex].NumOfLane/2;

				/* In the Following, we need to randomly determine
				 * a lane for agent to go through after it taked the turn.
				 */
				int tempLaneIndex = (rand() % (EdgeList[tempEdgeIndex].NumOfLane/2)) + base;
				RoadBlock RoadAfterTurn;
				n = getRoadBlockByIndex(-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex, RoadAfterTurn); 
				if(n <= 0)
				{
					printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
					return -11;
				}

				double CrossPOS_x,CrossPOS_y;

				n = secondorder_SimultaneousEqution(CurrentRoad.a, CurrentRoad.b, CurrentRoad.c, RoadAfterTurn.a, RoadAfterTurn.b, RoadAfterTurn.c, CrossPOS_x, CrossPOS_y);
				if(n == INFINITE_SOLUTION || NONE_SOLUTION)
				{
					printf("[%s]: secondorder_SimultaneousEqution error!!\n", __func__);
#if DEBUG_ROAD
					printf("AfterRoad (%d, %d, %d, %d)\n",
							-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex);
#endif
					return -12;
				}

				// get the nearest end point of next lane
				double distance1 = Distance_BetweenTwoNode(CrossPOS_x, CrossPOS_y, RoadAfterTurn.end_x[0], RoadAfterTurn.end_y[0]);
				double distance2 = Distance_BetweenTwoNode(CrossPOS_x, CrossPOS_y, RoadAfterTurn.end_x[1], RoadAfterTurn.end_y[1]);
				if(distance1 < distance2)
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[0];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[0];
				}
				else 
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[1];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[1];
				}

				double distance3 = Distance_BetweenTwoNode(CrossPOS_x, CrossPOS_y, CurrentRoad.end_x[0], CurrentRoad.end_y[0]);
				double distance4 = Distance_BetweenTwoNode(CrossPOS_x, CrossPOS_y, CurrentRoad.end_x[1], CurrentRoad.end_y[1]);
				// get the end point of current road 
				if(distance3 < distance4)
				{
					TurningPointsPOS_x[0] = CurrentRoad.end_x[0];
					TurningPointsPOS_y[0] = CurrentRoad.end_y[0];
				}
				else
				{
					TurningPointsPOS_x[0] = CurrentRoad.end_x[1];
					TurningPointsPOS_y[0] = CurrentRoad.end_y[1];
				}
				TurningPointsPOS_x[1] = (TurningPointsPOS_x[0] + CrossPOS_x + FirstPointAfterTheTurn_x) / 3;
				TurningPointsPOS_y[1] = (TurningPointsPOS_y[0] + CrossPOS_y + FirstPointAfterTheTurn_y) / 3;
				/*
				 * Calculate the two directions of 
				 * (current road end point) -> (CrossPOS) and 
				 * (CrossPOS) -> (the nearest end point of next lane)
				 */
				double *tempDirections = new double[NumOfTurningPoints];

				tempDirections[0] = fmod(atan2(TurningPointsPOS_y[1] - TurningPointsPOS_y[0], TurningPointsPOS_x[1] - TurningPointsPOS_x[0]) /PI * 180 + 360, 360);
				directionAlongTwoConsecutiveTurningPoints[0] = fmod(360 - tempDirections[0], 360);

				tempDirections[1] = fmod(atan2(FirstPointAfterTheTurn_y - TurningPointsPOS_y[1], FirstPointAfterTheTurn_x - TurningPointsPOS_x[1]) /PI * 180 + 360, 360);
				directionAlongTwoConsecutiveTurningPoints[1] = fmod(360 - tempDirections[1], 360);

				//printf("turnint point (%lf,%lf)end point of the turn(%lf,%lf)\n", TurningPointsPOS_x[0], TurningPointsPOS_y[0], FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y);

				myCache->RefreshAfter(-1, tempEdgeIndex, tempLaneIndex, tempBlockIndex);
				myCache->AfterDirty = 1;

				delete tempDirections;
				return 1;

			} 
			else // Car makes an U turn
			{
				NumOfTurningPoints = 2;
				TurningPointsPOS_x = new double[NumOfTurningPoints];
				TurningPointsPOS_y = new double[NumOfTurningPoints];
				directionAlongTwoConsecutiveTurningPoints = new double[NumOfTurningPoints];

				double MiniAngleDiff = 999;
				int MiniEdgeIndexInNode = 0;
				int NumOfEdges = 0;
				int NumOfTheSameExitEdge = 0;
				int *edgesWithTheSameExit = NULL;

				NumOfEdges = NodeList[NextNodeIndex].NumOfEdges;

				edgesWithTheSameExit = new int[NumOfEdges];

				// search the exit edge of the intersection
				for(int i = 0; i < NumOfEdges; ++i)
				{
					double AngleDiff;
					// check if the two directions are the same
					AngleDiff = fabs(NextDirection - NodeList[NextNodeIndex].direction[i]);
					if(AngleDiff > 180)
						AngleDiff = 360 - AngleDiff;

					if(AngleDiff <= 0.01)
					{
						/* There may be many edges which all connect
						 * to the same exit of the intersection
						 * on the loaded real-world Road Map
						 */
						MiniAngleDiff = AngleDiff;
						edgesWithTheSameExit[NumOfTheSameExitEdge] = i;
						NumOfTheSameExitEdge++;
					}
				}
				if(MiniAngleDiff > 0.01)
					return -13; //Turn to no way.

				// randomly choose one edge
				MiniEdgeIndexInNode = edgesWithTheSameExit[rand() % NumOfTheSameExitEdge];

				delete edgesWithTheSameExit;

				int nextEdgeIndex = NodeList[NextNodeIndex].EID[MiniEdgeIndexInNode];
				int nextBlockIndex;

				if(NodeList[NextNodeIndex].HeadOrNot[MiniEdgeIndexInNode] == true)
					nextBlockIndex = 0;
				else {
					// Lanes in the same edge has the same size. 
					nextBlockIndex = EdgeList[nextEdgeIndex].lane[0].RoadBlockID.size() - 1;
				}

				double AngleDiff1 = fabs(EdgeList[nextEdgeIndex].lane[EdgeList[nextEdgeIndex].NumOfLane/2 - 1].RoadBlockID[nextBlockIndex].direction - NextDirection);
				if(AngleDiff1 > 180) 
					AngleDiff1 = 360 - AngleDiff1;

				double AngleDiff2 = fabs(EdgeList[nextEdgeIndex].lane[EdgeList[nextEdgeIndex].NumOfLane/2].RoadBlockID[nextBlockIndex].direction - NextDirection);
				if(AngleDiff2 > 180) 
					AngleDiff2 = 360 - AngleDiff2;

				int base;

				// check NextDirection is on which half lanes of next edge
				if(AngleDiff1 < AngleDiff2)
					base = 0;
				else 
					base = EdgeList[nextEdgeIndex].NumOfLane/2;

				/* In the Following, we need to randomly determine
				 * a lane for agent to go through after it taked the turn.
				 */
				int nextLaneIndex = (rand() % (EdgeList[nextEdgeIndex].NumOfLane/2)) + base;
				RoadBlock RoadAfterTurn;
				n = getRoadBlockByIndex(-1, nextEdgeIndex, nextLaneIndex, nextBlockIndex, RoadAfterTurn); 
				if(n <= 0)
				{
					printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
					return -14;
				}

				double curRoadEndX = 0, curRoadEndY = 0;

				// get the end point of current road 
				if(isTheSameDirection(CurrentRoad.end_x[0], CurrentRoad.end_y[0], CurrentRoad.end_x[1], CurrentRoad.end_y[1], CurrentDirection))
				{
					curRoadEndX = CurrentRoad.end_x[1];
					curRoadEndY = CurrentRoad.end_y[1];
				}
				else
				{
					curRoadEndX = CurrentRoad.end_x[0];
					curRoadEndY = CurrentRoad.end_y[0];
				}

				// get the nearest end point of next lane 
				double distance1 = Distance_BetweenTwoNode(curRoadEndX, curRoadEndY, RoadAfterTurn.end_x[0], RoadAfterTurn.end_y[0]);
				double distance2 = Distance_BetweenTwoNode(curRoadEndX, curRoadEndY, RoadAfterTurn.end_x[1], RoadAfterTurn.end_y[1]);
				if(distance1 < distance2)
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[0];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[0];
				}
				else 
				{
					FirstPointAfterTheTurn_x = RoadAfterTurn.end_x[1];
					FirstPointAfterTheTurn_y = RoadAfterTurn.end_y[1];
				}

				/* Calculate the middle event point when make U turn
				 * Note: The CurrentDirection is the direction on normal coordinator, 
				 *       not the direction on GUI view.
				 */

				// center point of the node block
				double nodeCenterX = (NodeList[NextNodeIndex].RoadBlockID.x[0] + NodeList[NextNodeIndex].RoadBlockID.x[2]) / 2;
				double nodeCenterY = (NodeList[NextNodeIndex].RoadBlockID.y[0] + NodeList[NextNodeIndex].RoadBlockID.y[2]) / 2;
				double deltaX = (RoadWidth / 2) * cos(NextDirection * PI / 180);
				double deltaY = (RoadWidth / 2) * sin(NextDirection * PI / 180);

				double CrossPOS_x = 0, CrossPOS_y = 0;

				CrossPOS_x = nodeCenterX + deltaX;
				CrossPOS_y = nodeCenterY + deltaY;

				TurningPointsPOS_x[0] = curRoadEndX;
				TurningPointsPOS_y[0] = curRoadEndY;

				TurningPointsPOS_x[1] = CrossPOS_x;
				TurningPointsPOS_y[1] = CrossPOS_y;

				/*
				 * Calculate the two directions of 
				 * (current road end point) -> (CrossPOS) and 
				 * (CrossPOS) -> (the nearest end point of next lane)
				 */
				double *tempDirections = new double[NumOfTurningPoints];

				tempDirections[0] = fmod(atan2(TurningPointsPOS_y[1] - TurningPointsPOS_y[0], TurningPointsPOS_x[1] - TurningPointsPOS_x[0]) /PI * 180 + 360, 360);
				directionAlongTwoConsecutiveTurningPoints[0] = fmod(360 - tempDirections[0], 360);

				tempDirections[1] = fmod(atan2(FirstPointAfterTheTurn_y - TurningPointsPOS_y[1], FirstPointAfterTheTurn_x - TurningPointsPOS_x[1]) /PI * 180 + 360, 360);
				directionAlongTwoConsecutiveTurningPoints[1] = fmod(360 - tempDirections[1], 360);

				/*
				printf("DEBUG: curPos (%lf, %lf), curDir %lf, turnP1 (%lf, %lf), dir1 %lf, turnP2 (%lf, %lf), dir2 %lf, endPos (%lf, %lf), expectDir %lf\n", 
					CurrentPOS_x, CurrentPOS_y, CurrentDirection,
					TurningPointsPOS_x[0], TurningPointsPOS_y[0], 
					directionAlongTwoConsecutiveTurningPoints[0], 
					TurningPointsPOS_x[1], TurningPointsPOS_y[1], 
					directionAlongTwoConsecutiveTurningPoints[1],
					FirstPointAfterTheTurn_x, FirstPointAfterTheTurn_y,
					NextDirection);
				*/
				myCache->RefreshAfter(-1, nextEdgeIndex, nextLaneIndex, nextBlockIndex);
				myCache->AfterDirty = 1;
				delete tempDirections;
				return 1;
			}
		}
	}
	return -15;

}

/* This API is used to make car correct his position
 * so that the car would not easily go out of the road 
 */
int selfCorrectness(double CurrentPOS_x, double CurrentPOS_y, double &CorrectedPOS_x, double &CorrectedPOS_y, int CacheOrNot, CacheRecord *myCache)
{
	int CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex;
	RoadBlock CurrentRoad;
	int n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex, CacheOrNot, myCache);
	if(n <= 0)
	{
		// It may be traffic light controller, therefore, don't return the error code (< 0)
		return 0;
	}
	n = getRoadBlockByIndex(CurrentNodeIndex, CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex, CurrentRoad);
	if((n <= 0) || (CurrentRoad.type == ROAD_TYPE_NODE))
	{
		// It may be in the node block, therefore, don't return the error code (< 0)
		return 0;
	}

	double CurrentRoadDirection;
	double PolarDirection = fmod(CurrentRoad.direction + 90, 360);

	// Translate the direction from normal coordinate to GUI viewed direction
	PolarDirection = fmod(360 - PolarDirection, 360);
	CurrentRoadDirection = fmod(360 - CurrentRoad.direction, 360);

	double pa, pb, pc; // polar line
	double ca, cb, cc; // current road line

	PolarEquation(pa, pb, pc, CurrentPOS_x, CurrentPOS_y, PolarDirection);
	PolarEquation(ca, cb, cc, CurrentRoad.end_x[0], CurrentRoad.end_y[0], CurrentRoadDirection);

	//Check whether the two lines intercept. If yes, return the intercept point
	n = secondorder_SimultaneousEqution(pa, pb, pc, ca, cb, cc, CorrectedPOS_x, CorrectedPOS_y);
	if(n != UNIQUE_SOLUTION)
		return 0;

	int CorrectedNodeIndex, CorrectedEdgeIndex, CorrectedLaneIndex, CorrectedBlockIndex;

	getCurrentRoadIndex(CorrectedPOS_x, CorrectedPOS_y, CorrectedNodeIndex, CorrectedEdgeIndex, CorrectedLaneIndex, CorrectedBlockIndex, CacheOrNot, myCache);
	if((CorrectedNodeIndex != CurrentNodeIndex) || 
			(CorrectedEdgeIndex != CurrentEdgeIndex) || 
			(CorrectedLaneIndex != CurrentLaneIndex) || 
			(CorrectedBlockIndex != CurrentBlockIndex))
		return 0;
	return 1;
}

int CheckCurrentCache(double CurrentPOS_x, double CurrentPOS_y, CacheRecord *tmpCache)
{
	if(tmpCache->CurrentDirty == 0) // no current road block cache
	{
		tmpCache->NextDirty = 0;
		tmpCache->AfterDirty = 0;
		return 0;	   //cache miss
	}
	else if(((CurrentPOS_x == tmpCache->x) && (CurrentPOS_x == tmpCache->y)) || (CheckPOSInBlock(CurrentPOS_x, CurrentPOS_y, tmpCache->CurrentNodeSerial, tmpCache->CurrentEdgeSerial, tmpCache->CurrentLaneSerial, tmpCache->CurrentBlockSerial) > 0))
	{
		return 1;//Current cache success 
	}

	if(tmpCache->NextDirty == 0) // no next road block cache
	{
		tmpCache->AfterDirty = 0;
		return 0;
	}
	else if(CheckPOSInBlock(CurrentPOS_x, CurrentPOS_y, tmpCache->NextNodeSerial, tmpCache->NextEdgeSerial, tmpCache->NextLaneSerial, tmpCache->NextBlockSerial) > 0)
	{
		tmpCache->RefreshCurrent(tmpCache->NextNodeSerial, tmpCache->NextEdgeSerial, tmpCache->NextLaneSerial, tmpCache->NextBlockSerial);
		tmpCache->NextDirty = 0;
		tmpCache->AfterDirty = 0;
		/*
		if(tmpCache->AfterDirty != 0)
		{
			tmpCache->RefreshNext(tmpCache->NodeAfterTurn, tmpCache->EdgeAfterTurn, tmpCache->LaneAfterTurn, tmpCache->BlockAfterTurn);
			tmpCache->AfterDirty = 0;
		}
		*/
		return 1;//Next cache success
	}

	if(tmpCache->AfterDirty == 0)
		return 0;
	else if(CheckPOSInBlock(CurrentPOS_x, CurrentPOS_y, tmpCache->NodeAfterTurn, tmpCache->EdgeAfterTurn, tmpCache->LaneAfterTurn, tmpCache->BlockAfterTurn) > 0)
	{
		//printf("Use After Cache\n");
		tmpCache->RefreshCurrent(tmpCache->NodeAfterTurn, tmpCache->EdgeAfterTurn, tmpCache->LaneAfterTurn, tmpCache->BlockAfterTurn);
		tmpCache->NextDirty = 0;
		tmpCache->AfterDirty = 0;
		return 1;//After cache success
	}

	// cache miss
	tmpCache->CurrentDirty = 0;
	tmpCache->NextDirty = 0;
	tmpCache->AfterDirty = 0;
	return 0;
}

int CheckPOSInBlock(double CurrentPOS_x, double CurrentPOS_y, int NodeSerial, int EdgeSerial, int LaneSerial, int BlockSerial)
{
	RoadBlock TestRoad;
	int n = getRoadBlockByIndex(NodeSerial, EdgeSerial, LaneSerial, BlockSerial, TestRoad);
	if(n <= 0)
	{
		//printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return 0;
	}
	double angle = CheckNodeInAreaAngle(CurrentPOS_x, CurrentPOS_y, TestRoad.x[0], TestRoad.y[0], TestRoad.x[1], TestRoad.y[1], TestRoad.x[2], TestRoad.y[2], TestRoad.x[3], TestRoad.y[3]);
	//if(angle < (1/PI))
	if(angle < (PI/360))
		return 1;
	else
		return 0;
}

/* Copy myCache to otherCache */
void cacheCopy(CacheRecord *myCache, CacheRecord *otherCache){

	/* 
	 * Most of the time, we will search the preceding car
	 * who may be in the same or next road block 
	 */
	otherCache->x = myCache->x;
	otherCache->y = myCache->y;
	otherCache->CurrentDirty = myCache->CurrentDirty;
	otherCache->NextDirty = myCache->NextDirty;
	otherCache->AfterDirty = myCache->AfterDirty;
	otherCache->CurrentNodeSerial = myCache->CurrentNodeSerial;
	otherCache->CurrentEdgeSerial = myCache->CurrentEdgeSerial;
	otherCache->CurrentLaneSerial = myCache->CurrentLaneSerial;
	otherCache->CurrentBlockSerial = myCache->CurrentBlockSerial;
	if(otherCache->NextDirty == 1){
		otherCache->NextNodeSerial = myCache->NextNodeSerial;
		otherCache->NextEdgeSerial = myCache->NextEdgeSerial;
		otherCache->NextLaneSerial = myCache->NextLaneSerial;
		otherCache->NextBlockSerial = myCache->NextBlockSerial;
	}
	if(otherCache->AfterDirty == 1){
		otherCache->NodeAfterTurn = myCache->NodeAfterTurn;
		otherCache->EdgeAfterTurn = myCache->EdgeAfterTurn;
		otherCache->LaneAfterTurn = myCache->LaneAfterTurn;
		otherCache->BlockAfterTurn = myCache->BlockAfterTurn;
	}
}

int checkIfOnTheSameLane(double pos_x1, double pos_y1, double pos_x2, double pos_y2, int CacheOrNot, CacheRecord *myCache, CacheRecord *otherCache)
{

	int NodeSerial1, EdgeSerial1, LaneSerial1, BlockSerial1; // For position 1
	int NodeSerial2, EdgeSerial2, LaneSerial2, BlockSerial2; // For position 2
	int n = 0;

	/* search my road index */
	n = getCurrentRoadIndex(pos_x1, pos_y1, NodeSerial1, EdgeSerial1, LaneSerial1, BlockSerial1, CacheOrNot, myCache);
	if(n <= 0)
	{
		printf("[%s] getCurrentRoadIndex error\n", __func__);
		return -1;
	}
	//printf("After call: myCache (%lf, %lf)\n", myCache->x, myCache->y);

	if(CacheOrNot == 1)
	{
		/* 
		 * Copy myCache to otherCache for each search because
		 * most of the time the preceding car is in the same
		 * road block with me.
		 *
		 * Note: otherCache saves the road info of preceding car
		 */
		cacheCopy(myCache, otherCache);
	}

	/* 
	 * Search the preceding car's road index.
	 * Here we use cache_other to avoid affecting the original myCache
	 */
	n = getCurrentRoadIndex(pos_x2, pos_y2, NodeSerial2, EdgeSerial2, LaneSerial2, BlockSerial2, CacheOrNot, otherCache);
	if(n <= 0)
	{
		//printf("[%s] getOtherRoadIndex error\n", __func__);
		//printf("DEBUG: pos2 (%lf, %lf)\n", pos_x2, pos_y2);
		return -2;
	}
	//printf("After call: otherCache (%lf, %lf)\n", otherCache->x, otherCache->y);

	/*if(((NodeSerial1 != -1) && (NodeSerial1 == NodeSerial2)) || 
		((LaneSerial1 != -1) && (LaneSerial1 == LaneSerial2))){*/
	if((LaneSerial1 != -1) && (LaneSerial1 == LaneSerial2))
	{
		// on the same lane
		return 1;
	} else {
		// not on the same lane
		return 0;
	}
} 

// get the exit of the intersection
int getCurrentNodeExit(double CurrentPOS_x, double CurrentPOS_y, double &ExitPOS_x, double &ExitPOS_y, double &ExitDirection, double &NextDirection, int CacheOrNot, CacheRecord *myCache)
{
	int n;
	int nodeSerial, edgeSerial, laneSerial, blockSerial;

	if((n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, nodeSerial, edgeSerial, laneSerial, blockSerial, CacheOrNot, myCache)) <= 0){
		printf("[%s]: getCurrentRoadIndex error !!!\n", __func__);
		return -1;
	}

	RoadBlock CurrentRoad;
	if((n = getRoadBlockByIndex(nodeSerial, edgeSerial, laneSerial, blockSerial, CurrentRoad)) <= 0){
		printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
		return -2;
	}

	if(CurrentRoad.type == ROAD_TYPE_NODE)
	{
		if(NodeList[nodeSerial].type == NODE_TYPE_I || NodeList[nodeSerial].type == NODE_TYPE_T || NodeList[nodeSerial].type == NODE_TYPE_L)
		{
			int chooseEdge;
			int tmpEdgeIndex, tmpLaneIndex;
			int exitRoadBlockIndex;
			
			// random choose one edge for exit
			chooseEdge = rand() % NodeList[nodeSerial].NumOfEdges;
			tmpEdgeIndex = NodeList[nodeSerial].EID[chooseEdge];

			// random choose the lane number which is bigger than half 
			tmpLaneIndex = ((rand() % EdgeList[tmpEdgeIndex].NumOfLane) + (EdgeList[tmpEdgeIndex].NumOfLane / 2)) % (EdgeList[tmpEdgeIndex].NumOfLane);
			exitRoadBlockIndex = EdgeList[tmpEdgeIndex].lane[tmpLaneIndex].ExitRoadBlockIndex;

			double angleDiff = fabs(EdgeList[tmpEdgeIndex].lane[tmpLaneIndex].RoadBlockID[exitRoadBlockIndex].direction - NodeList[nodeSerial].direction[tmpEdgeIndex]);

			if(angleDiff > 360)
				angleDiff = 360 - angleDiff;

			// test the direction of lane to see if this lane is the exit of the node
			if(angleDiff >= 0.1)
			{
				// random choose the lane number which is smaller than half 
				tmpLaneIndex = rand() % (EdgeList[tmpEdgeIndex].NumOfLane / 2);
				exitRoadBlockIndex = EdgeList[tmpEdgeIndex].lane[tmpLaneIndex].ExitRoadBlockIndex;
			} 
	
			/*
			printf("DEBUG: node %d, edge %d, lane %d, road %d, direction %lf\n",
					nodeSerial, tmpEdgeIndex, tmpLaneIndex, exitRoadBlockIndex, NodeList[nodeSerial].direction[tmpEdgeIndex]);
			*/
			
			RoadBlock ExitRoad1;
			RoadBlock ExitRoad2;
			int numOfRoad;

			numOfRoad = (int)((EdgeList[tmpEdgeIndex].lane[tmpLaneIndex].RoadBlockID).size());

			// get the first road block of this lane
			n = getRoadBlockByIndex(-1, tmpEdgeIndex, tmpLaneIndex, 0, ExitRoad1); 
			if(n <= 0)
			{
				printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
				return -3;
			}

			// get the last road block of this lane
			n = getRoadBlockByIndex(-1, tmpEdgeIndex, tmpLaneIndex, (numOfRoad - 1), ExitRoad2); 
			if(n <= 0)
			{
				printf("[%s]: getRoadBlockByIndex error !!!\n", __func__);
				return -3;
			}

			// get the nearest exit position of this node 
			double tmpX[4], tmpY[4];
			tmpX[0] = ExitRoad1.end_x[0]; tmpY[0] = ExitRoad1.end_y[0];
			tmpX[1] = ExitRoad1.end_x[1]; tmpY[1] = ExitRoad1.end_y[1];
			tmpX[2] = ExitRoad2.end_x[0]; tmpY[2] = ExitRoad2.end_y[0];
			tmpX[3] = ExitRoad2.end_x[1]; tmpY[3] = ExitRoad2.end_y[1];

			int miniI;
			double miniDistance = 999999;
			for(int i = 0; i < 4; ++i)
			{
				double tmpDist = Distance_BetweenTwoNode(CurrentPOS_x, CurrentPOS_y, tmpX[i], tmpY[i]);
				if(tmpDist < miniDistance)
				{
					miniDistance = tmpDist;
					miniI = i;
				}
			}

			ExitPOS_x = tmpX[miniI];
			ExitPOS_y = tmpY[miniI];

			double tmpDirection;
			tmpDirection = fmod(atan2(ExitPOS_y - CurrentPOS_y, ExitPOS_x - CurrentPOS_x) /PI * 180 + 360, 360);
			ExitDirection = fmod(360 - tmpDirection, 360);
			if(miniI < 2)
				NextDirection = fmod(360 - ExitRoad1.direction, 360);
			else
				NextDirection = fmod(360 - ExitRoad2.direction, 360);
			return 1;
		}
	}

	return -4;
}

void getRoadWidth(double &roadWidth)
{
	roadWidth = RoadWidth;
}

int isTheSameDirection(double pos1_x, double pos1_y, double pos2_x, double pos2_y, double direction)
{
	/*
	 * Note:
	 *    direction is the mathematical direction, not the direction viewed on GUI
	 */
	double tmpDirection;
	double angleDiff;

	// tmpDirection is the direction from pos1 to pos2
	tmpDirection = fmod(atan2(pos2_y - pos1_y, pos2_x - pos1_x) / PI * 180 + 360, 360);

	angleDiff = fabs(tmpDirection - direction);
	if(angleDiff > 180)
		angleDiff = 360 - angleDiff;

	if(angleDiff <= 0.01)
		return 1; // the same direction
	else
		return 0; // not the same direction
}

// get front node block ID which connected by current edge
int getFrontNID(double CurrentPOS_x, double CurrentPOS_y, int &groupID, int CacheOrNot, CacheRecord *myCache)
{
	int n;
	int nodeSerial, edgeSerial, laneSerial, blockSerial;

	groupID = -1;

	if((n = getCurrentRoadIndex(CurrentPOS_x, CurrentPOS_y, nodeSerial, edgeSerial, laneSerial, blockSerial, CacheOrNot, myCache)) <= 0)
	{
		printf("[%s]: getCurrentRoadIndex error!! Pos(%lf, %lf)\n", __func__, CurrentPOS_x, CurrentPOS_y);
		return -1;
	}

	if(nodeSerial == -1)
	{
		/* We can get front NID by the relationship between ExitRoadBlockIndex and NID */
		if(EdgeList[edgeSerial].lane[laneSerial].ExitRoadBlockIndex == 0)
		{
			groupID = EdgeList[edgeSerial].NID[0];
		}
		else
		{
			groupID = EdgeList[edgeSerial].NID[1];
		}

		groupID = groupID;
	}
	return 1;
}

