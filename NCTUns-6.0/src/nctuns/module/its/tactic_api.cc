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

/*
	The API functions provided in this file can be called by a 
	user-level agent program to support dynamic tactic mobile
	ad hoc network researches.
 
        A programmer can include this file into his (her) agent program 
	so that his (her) agent program can call these API functions 
	to achieve a certin goal. 

	Some functions are IPC commands exchanged between the calling
	agent program and the simulation engine while some others
	are not.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "sock_skel.h"
#include "command_format.h"
#include "obstacle.h"
#include "map.h"
#include "math_fun.h"
#include "auto_vehicle_signal.h"
#include "nctuns_syscall.h"
#include "tactic_api.h"
#include "road.h"


#define MAX_NODES	4096
#define TRUE		1
#define FALSE		0

using namespace std;

/* Important global variables */
struct obstacle         *obs_head = NULL;
struct obstacle         *obs_tail = NULL;
struct obstacle         *obstacleListP = NULL;
u_int32_t               Num_obs = 0;

double 			maxXMeter=1000, maxYMeter=600, maxZMeter=0;
int 			maxXGrid, maxYGrid, maxZGrid;
unsigned char		*gridMapP;
Map 			*m_Map;		    // the grid map of the field

int callagentGetScriptName = 0;
char *scriptName;

/* Helper function */
char* GetWorkingDir() {
char *dir;

        dir = getenv("NCTUNS_WORKDIR");
        if(dir)
                return(dir);
        else {
                printf("getenv(NCTUNS_WORKDIR) failed\n");
                exit(0);
        }
}

/* Helper function */
char* agentGetScriptName() {
	if(!callagentGetScriptName)
	{
		// make sure we call scandir just only time.
		struct dirent **namelist;
		int n, len;
		n = scandir(getenv("NCTUNS_WORKDIR"), &namelist, 0, alphasort);
		if (n < 0) {
			perror("scandir() in working directory failed");
			exit(0);
		}
		else {
			while (n--) {
				if (strstr(namelist[n]->d_name, ".tcl") != NULL) {
					len = strlen(namelist[n]->d_name);
					
					scriptName = (char *) malloc(len);
					strcpy(scriptName, namelist[n]->d_name);
					scriptName[len-4] = '\0';

					callagentGetScriptName = 1;
					/* Truncating the .tcl suffix */
					return scriptName;
				}
			}
		}
		printf("Cannot find the .tcl file in the working dir\n");
		exit(0);
	} else {
		if(scriptName == NULL){
			perror("agentGetScriptName failed");
			exit(0);
		}
		return scriptName;
	}
}

/* Helper function */
void read_obstacles(FILE *obs_fptr) {

	char			line[128];
	double			x1, y1, x2, y2, width;
	int			blockView, blockMovement, blockWirelessSignal;
	double			attenuation;
	double			dx, dy, len;
	struct obstacle		*tmpObs;
	double 			maxX, maxY, maxZ;

	maxX = 0; maxY = 0; maxZ = 0;
	while(!feof(obs_fptr)) {
		line[0] = '\0';
		fgets(line, 127, obs_fptr);
		if ((line[0]=='\0')||(line[0]=='#'))
			continue;

		if (3 == sscanf(line, "(%lf,%lf,%lf)", &maxX, &maxY, &maxZ)) {
			maxXMeter = maxX;
			maxYMeter = maxY;
			maxZMeter = maxZ;

        		maxXGrid = (int) (maxXMeter/gridWidthMeter) + 1;
			if ((fmod(maxXMeter, gridWidthMeter)) != 0) maxXGrid++;
        		maxYGrid = (int) (maxYMeter/gridWidthMeter) + 1;
			if ((fmod(maxYMeter, gridWidthMeter)) != 0) maxYGrid++;
        		maxZGrid = (int) (maxZMeter/gridWidthMeter) + 1;
			if ((fmod(maxZMeter, gridWidthMeter)) != 0) maxZGrid++;
        		gridMapP = (unsigned char *) malloc(sizeof 
				(unsigned char) * maxXGrid * maxYGrid);
		}

		if (9 == sscanf(line, "%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%lf", 
			&x1, &y1, &x2, &y2, &width,
			&blockView, &blockMovement, &blockWirelessSignal,
			&attenuation))
		{

			tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
			tmpObs->blockView = blockView;
			tmpObs->blockMovement = blockMovement;
			tmpObs->blockWirelessSignal = blockWirelessSignal;
			tmpObs->attenuation = attenuation;

			dx = x1 - x2;
			dy = y1 - y2;
			len = sqrt(dx*dx + dy*dy);

			tmpObs->x1 = x1 - (width / 2) * dy / len;
			tmpObs->y1 = y1 + (width / 2) * dx / len;
			tmpObs->x2 = x2 - (width / 2) * dy / len;
			tmpObs->y2 = y2 + (width / 2) * dx / len;
			tmpObs->x3 = x2 + (width / 2) * dy / len;
			tmpObs->y3 = y2 - (width / 2) * dx / len;
			tmpObs->x4 = x1 + (width / 2) * dy / len;
			tmpObs->y4 = y1 - (width / 2) * dx / len;
			tmpObs->next = NULL;

			if(Num_obs == 0) {
				obs_head = tmpObs;
				obs_tail = tmpObs;
			}
			else {
				obs_tail->next = tmpObs;
				obs_tail = tmpObs;
			}

			Num_obs++;
		}
	}

	// Automatically put in top obstacle 
	tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
	tmpObs->x1 = 0;
	tmpObs->y1 = 0;
	tmpObs->x2 = maxXMeter;
	tmpObs->y2 = 0;
	tmpObs->x3 = maxXMeter;
	tmpObs->y3 = gridWidthMeter;
	tmpObs->x4 = 0;
	tmpObs->y4 = gridWidthMeter;
	tmpObs->blockView = 1;
	tmpObs->blockMovement = 1;
	tmpObs->blockWirelessSignal = 1;
	tmpObs->attenuation = 999999999; // infinity: block any wireless signal
	tmpObs->next = NULL;
	if(Num_obs == 0) {
		obs_head = tmpObs;
		obs_tail = tmpObs;
	}
	else {
		obs_tail->next = tmpObs;
		obs_tail = tmpObs;
	}
	Num_obs++;

        // Automatically put in bottom obstacle
        tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
        tmpObs->x1 = 0;
        tmpObs->y1 = maxYMeter;
        tmpObs->x2 = maxXMeter;
        tmpObs->y2 = maxYMeter;
        tmpObs->x3 = maxXMeter;
        tmpObs->y3 = maxYMeter - gridWidthMeter;
        tmpObs->x4 = 0;
        tmpObs->y4 = maxYMeter - gridWidthMeter;
        tmpObs->blockView = 1;
        tmpObs->blockMovement = 1;
        tmpObs->blockWirelessSignal = 1;
	tmpObs->attenuation = 999999999; // infinity: block any wireless signal
        tmpObs->next = NULL;
	if(Num_obs == 0) {
		obs_head = tmpObs;
		obs_tail = tmpObs;
	}
	else {
		obs_tail->next = tmpObs;
		obs_tail = tmpObs;
	}
	Num_obs++;

        // Automatically put in left obstacle
        tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
        tmpObs->x1 = 0;
        tmpObs->y1 = 0;
        tmpObs->x2 = 0;
        tmpObs->y2 = maxYMeter;
        tmpObs->x3 = gridWidthMeter;
        tmpObs->y3 = maxYMeter;
        tmpObs->x4 = gridWidthMeter;
        tmpObs->y4 = 0;
        tmpObs->blockView = 1;
        tmpObs->blockMovement = 1;
        tmpObs->blockWirelessSignal = 1;
	tmpObs->attenuation = 999999999; // infinity: block any wireless signal
        tmpObs->next = NULL;
	if(Num_obs == 0) {
		obs_head = tmpObs;
		obs_tail = tmpObs;
	}
	else {
		obs_tail->next = tmpObs;
		obs_tail = tmpObs;
	}
	Num_obs++;

        // Automatically put in right obstacle
        tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
        tmpObs->x1 = maxXMeter;
        tmpObs->y1 = 0;
        tmpObs->x2 = maxXMeter;
        tmpObs->y2 = maxYMeter;
        tmpObs->x3 = maxXMeter - gridWidthMeter;
        tmpObs->y3 = maxYMeter;
        tmpObs->x4 = maxXMeter - gridWidthMeter;
        tmpObs->y4 = 0;
        tmpObs->blockView = 1;
        tmpObs->blockMovement = 1;
        tmpObs->blockWirelessSignal = 1;
	tmpObs->attenuation = 999999999; // infinity: block any wireless signal
        tmpObs->next = NULL;
	if(Num_obs == 0) {
		obs_head = tmpObs;
		obs_tail = tmpObs;
	}
	else {
		obs_tail->next = tmpObs;
		obs_tail = tmpObs;
	}
	Num_obs++;

	obstacleListP = obs_head;
	return;
}

// add by icchen 09.12.16
int getLaneChangeInfo(int myTCPsockfd, int mynid, int &EnableLaneChange, int moreMsgFollowing)
{
	char    *msg;
	int     n, result;

	if (mynid <= 0) return(-1);
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientGetLaneChangeInfo *) msg)->type =
	  AGENT_CLIENT_GET_LANE_CHANGE_INFO;
        ((struct agentClientGetLaneChangeInfo *) msg)->nid = mynid;
       	((struct agentClientGetLaneChangeInfo *) msg)->moreMsgFollowing = moreMsgFollowing;
       	n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetLaneChangeInfo));
 	n = readn(myTCPsockfd, msg,  sizeof(struct agentClientGetLaneChangeInfoReply));

       	if (((struct agentClientGetLaneChangeInfoReply *) msg)
          ->type != AGENT_CLIENT_GET_LANE_CHANGE_INFO_REPLY) {
          printf("IPC AGENT_CLIENT_GET_LANE_SWITCH_INFO_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetLaneChangeInfoReply *) msg)->result;
        EnableLaneChange = ((struct agentClientGetLaneChangeInfoReply *) msg)->EnableLaneChange;

        free(msg);
        return(result);
}

int getTeamInfo(int myTCPsockfd,int  mynid, int &TeamID,int &TeamLeader, int moreMsgFollowing)
{
	char    *msg;
	int     n, result;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetTeamInfo *) msg)->type =
          AGENT_CLIENT_GET_TEAM_INFO;
        ((struct agentClientGetTeamInfo *) msg)->nid = mynid;
        ((struct agentClientGetTeamInfo *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetTeamInfo));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetTeamInfoReply));

       if (((struct agentClientGetTeamInfoReply *) msg)
          ->type != AGENT_CLIENT_GET_TEAM_INFO_REPLY) {
          printf("IPC AGENT_CLIENT_GET_TEAM_INFO_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetTeamInfoReply *) msg)->result;
        TeamID = ((struct agentClientGetTeamInfoReply *) msg)->TeamID;
        TeamLeader = ((struct agentClientGetTeamInfoReply *) msg)->TeamLeader;

        free(msg);
        return(result);
}

int getTeamTurningInfo(int myTCPsockfd, int mynid, double &TurningDirection, int &TurningBlockNumber, int &TurningTimes, int moreMsgFollowing)
{
        char    *msg;
        int     n, result;
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetTeamTurningInfo *) msg)->type =
          AGENT_CLIENT_GET_TEAM_TURNING_INFO;
        ((struct agentClientGetTeamTurningInfo *) msg)->nid = mynid;
        ((struct agentClientGetTeamTurningInfo *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetTeamTurningInfo));
        n = readn(myTCPsockfd, msg,  sizeof(struct agentClientGetTeamTurningInfoReply));

       if (((struct agentClientGetTeamTurningInfoReply *) msg)
          ->type != AGENT_CLIENT_GET_TEAM_TURNING_INFO_REPLY) {
          printf("IPC AGENT_CLIENT_GET_TEAM_TURNING_INFO_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetTeamTurningInfoReply *) msg)->result;
        TurningDirection = ((struct agentClientGetTeamTurningInfoReply *) msg)->TurningDirection;
        TurningBlockNumber = ((struct agentClientGetTeamTurningInfoReply *) msg)->TurningBlockNumber;
	TurningTimes = ((struct agentClientGetTeamTurningInfoReply *) msg)->TurningTimes;

        free(msg);
        return(result);
}

int setTeamTurningInfo(int myTCPsockfd, int mynid, double TurningDirection, int TurningBlockNumber, int TurningTimes, int moreMsgFollowing)
{
        char    *msg;
        int     n, result;
	
	//printf("%d enter setTeamTurningInfo \n",mynid);
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetTeamTurningInfo *) msg)->type =
          AGENT_CLIENT_SET_TEAM_TURNING_INFO;
        ((struct agentClientSetTeamTurningInfo *) msg)->nid = mynid;
	((struct agentClientSetTeamTurningInfo *) msg)->TurningDirection = TurningDirection;
	((struct agentClientSetTeamTurningInfo *) msg)->TurningBlockNumber = TurningBlockNumber;
	((struct agentClientSetTeamTurningInfo *) msg)->TurningTimes = TurningTimes;
        ((struct agentClientSetTeamTurningInfo *) msg)->moreMsgFollowing =
          moreMsgFollowing;

        n = writen(myTCPsockfd, msg, sizeof(struct agentClientSetTeamTurningInfo));
//printf("in tacticapi AGENT_CLIENT_SET_TEAM_TURNING_INFO %d(fd=%d) write TurningTimes=%d TurningDirection=%f %d bytes\n",mynid,myTCPsockfd,TurningTimes,TurningDirection,n);

        n = readn(myTCPsockfd, msg,  sizeof(struct agentClientSetTeamTurningInfoReply));

       if (((struct agentClientSetTeamTurningInfoReply *) msg)
          ->type != AGENT_CLIENT_SET_TEAM_TURNING_INFO_REPLY) {
          printf("IPC AGENT_CLIENT_SET_TEAM_TURNING_INFO_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientSetTeamTurningInfoReply *) msg)->result;

        free(msg);
        return(result);
}


int getNearestPrecedingTeamMember(int myTCPsockfd,int  mynid, int &Preceding_Nid,int moreMsgFollowing)
{
	char    *msg;
	int     n, result;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetNearestPrecedingTeamMember *) msg)->type =
          AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER;
        ((struct agentClientGetNearestPrecedingTeamMember *) msg)->nid = mynid;
        ((struct agentClientGetNearestPrecedingTeamMember *) msg)->moreMsgFollowing = 
	  moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetNearestPrecedingTeamMember));
        n = readn(myTCPsockfd, msg,  sizeof(struct agentClientGetNearestPrecedingTeamMemberReply));

        if (((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->type != 
              AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER_REPLY) 
	{
          printf("IPC AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->result;
        Preceding_Nid = ((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->Preceding_Nid;

        free(msg);
        return(result);

}

int getNearestFollowingTeamMember(int myTCPsockfd,int  mynid, int &Following_Nid,int moreMsgFollowing)
{
        char    *msg;
        int     n, result;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetNearestFollowingTeamMember *) msg)->type =
          AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER;
        ((struct agentClientGetNearestFollowingTeamMember *) msg)->nid = mynid;
        ((struct agentClientGetNearestFollowingTeamMember *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetNearestFollowingTeamMember));
        n = readn(myTCPsockfd, msg,  sizeof(struct agentClientGetNearestFollowingTeamMemberReply));

        if (((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->type !=
              AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER_REPLY)
        {
          printf("IPC AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->result;
        Following_Nid = ((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->Following_Nid;

        free(msg);
        return(result);
}

int SwitchOrderWithPrecedingTeamMember(int myTCPsockfd,int mynid,int moreMsgFollowing)
{
	printf("In Tactic_API : Switch start\n");
        char    *msg;
        int     n, result;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSwitchOrderInTeamListWithPrecedingCar *) msg)->type =
          AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR;
        ((struct agentClientSwitchOrderInTeamListWithPrecedingCar *) msg)->nid = mynid;
        ((struct agentClientSwitchOrderInTeamListWithPrecedingCar *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientSwitchOrderInTeamListWithPrecedingCar));
        n = readn(myTCPsockfd, msg,  sizeof(struct agentClientSwitchOrderInTeamListWithPrecedingCarReply));

        if (((struct agentClientSwitchOrderInTeamListWithPrecedingCarReply *) msg)->type !=
              AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR_REPLY)
        {
          printf("IPC AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientSwitchOrderInTeamListWithPrecedingCarReply *) msg)->result;
        free(msg);
        return(result);
}

int getProFileData(int nid, double &MaxSpeed, double &MaxAcceleration, double &MaxDeceleration, int IsAgentClient)
{
	char* filePath = NULL;

	if(IsAgentClient){
		// for agent client
		if((filePath = getProfileName(nid)) == NULL)
			return 0;
	} else {
		// for large-scale node
		char* tmpPath = NULL;
		filePath = (char *) malloc(strlen(GetWorkingDir()) + 15);

		if((tmpPath = getProfileName(nid)) == NULL){
			free(filePath);
			return 0;
		}
		sprintf(filePath, "%s%s", GetWorkingDir(), tmpPath);
	}

	char buf[1024];
	bzero(buf,1024);

        getcwd(buf, 1024);
	FILE* fd = fopen(filePath, "r");
	
	if(!fd)
	    assert(0);
	
	char line[128];

	while(!feof(fd)) {
		line[0] = '\0';
		fgets(line, 127, fd);
		if ((line[0] == '\0')||(line[0] == '#'))
			continue;
		char *head = strtok(line, "\n\r\t= ");
		if(strcmp(head, "MaxSpeed") == 0)
		{
			char *tail = strtok(NULL, "\n\r\t= ");
			MaxSpeed = atof(tail);
			//printf("Max speed = %f\n", MaxSpeed);
		}
		else if(strcmp(head, "MaxAcceleration") == 0)
		{
			char *tail = strtok(NULL, "\n\r\t= ");
			MaxAcceleration = atof(tail);
			//printf("Max Acceleartion = %f\n", MaxAcceleration);
		}
		else if(strcmp(head, "MaxDeceleration") == 0)
		{
			char *tail = strtok(NULL, "\n\r\t= ");
			MaxDeceleration = atof(tail);
			//printf("Max deceleartion = %f\n", MaxDeceleration);
		}
	}

	if(!IsAgentClient)
		free(filePath);

	return 1;
}

char *getProfileName(int nid)
{
	char *FILEPATH;
	FILE *fd;

	/* get the the file path of car_prof_cfg */
	FILEPATH = (char *) malloc(strlen(GetWorkingDir()) +
			strlen(agentGetScriptName()) + 15);

	sprintf(FILEPATH, "%s%s%s", GetWorkingDir(), agentGetScriptName(), ".car_prof_cfg\0");

	if(0) printf("[Agent Debug] Filepath %s, WorkingDir %s\n", FILEPATH, GetWorkingDir());

	fd = fopen(FILEPATH, "r");
	if(!fd) {
		printf("CarAgent: open car profile file failed. FILEPATH %s\n", FILEPATH);
		return NULL;
	}

	free(FILEPATH);
	char *profile_name = NULL;

	while(!feof(fd)) {

		int tmp_nid = 0;
		char line[128];
		line[0] = '\0';

		fgets(line, 127, fd);
		if ((line[0] == '\0') || (line[0] == '#'))
			continue;

		int token_cnt = 0;
		char* str_p = line;
		int match_flag = false;

		while (1) {
			//printf("[Agent Debug] token_cnt = %d\n", token_cnt);
			if (token_cnt) str_p = NULL;
			char* token_p = strtok(str_p, " \t\r\n");

			if (!token_p)
				break;
			if (!token_cnt) {
				tmp_nid = atoi(token_p);

				if (tmp_nid == nid){ 
					match_flag = true;
				}
			}
			else if ((token_cnt > 0) && match_flag) {
				// get profile name successfully
				profile_name = strdup(token_p);
			}
			else ;
			++token_cnt;
		}

		if(profile_name != NULL){
			// get profile name successfully
			break;
		}
	}

	fclose(fd);
	//printf("Car[%d] uses profile: %s\n", nid, profile_name);
	fflush(stdout);
	return profile_name;
}

int createTCPSocketForCommunicationWithSimulationEngine(int mynid, int gid1, 
int gid2, int gid3, int gid4, int gid5, int gid6, int gid7, int gid8,
int humanControlled, int &UDPsocketFd2, int proc_type, int moreMsgFollowing) {
/*
	This API function creates and returns a TCP socket by which the 
	calling agent program can use IPC commands to communicate with 
	the simulation engine. 

        Since IPC commands exchanges between an agent program and the 
	simulation engine should take no time, before creating this socket, 
	this function internally calls a NCTUns system call to tell the kernel 
	that the created socket should be treated differently from 
	other sockets created for communication with other agent programs. 
	This is because packets sent out via the former should not go 
	through the simulated network while the latter should. 
        More specifically, packets sent out via the former should be 
	looped back via the loopback interface in the kernel to the 
	socket used by the simulation engine without going through any 
	network simulation; however, packets sent out via the latter 
	should be sent into the simulated network via a tunnel interface 
	and may go through multi-hop wireless transmission simulation 
	until they reach the socket used by another agent program. 
	What this system call does is to deregister the created socket 
	from the kernel so that it will not be treated as the latter type 
	of socket (which is the default type when a socket is created 
	in an application program running on NCTUns). 

        The argument mynid should be filled in with the node ID of the 
        mobile agent. A mobile agent can simultaneously belong to at most 
        8 different groups. If it belongs to i'th group, the argument gid(i) 
        should be set to 1; otherwise, gid(i) should be set to 0. 
	The movement of a mobile node can be automatically controlled 
	by its agent program or manually controlled by a human. 
        In the former case, the humanControlled flag argument should be 
        set to 0 while in the latter case the humanControlled flag argument 
        should be set to 1. When a mobile node is configured to be 
        controlled by a human, its agent program will obey the moving
        directions issued by a human pressing the right/left/up/down
        arrow keys on the keyboard. 

        This function packs the provided arguments into an IPC command and
        sends it to the simulation engine via the just created socket
        to register this mobile agent. In the future, when the simulation 
        engine has a particular event to notify the mobile agent, it sends 
        a notification message to the mobile agent, which can be received 
        by the mobile agent via this created socket.

        The moreMsgFollowing argument tells the simulation engine whether 
        more IPC commands will follow this IPC command and that they all 
        need to be processed atomically. That is, if moreMsgFollowing is 1,
        after processing this IPC command and sending back an ACK packet,   
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing 
        is 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.
 */

	int s, s1;
        char *msg;
        struct agentClientInfo *p;
        int n;
	struct sockaddr_in   ser_addr;
	struct hostent  *phe;   /* pointer to host information entry    */
	int tryport;

        if((s1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
               perror("server: can't open a UDP socket");
               exit(1);
        }
        if (syscall_NCTUNS_cancel_socknodeID(s1) < 0) {
          printf("Setting the UDP socket to be a loopback socket failed !!!\n");
          exit(1);
        }

	tryport = 7000;

        bzero((char *) &ser_addr, sizeof(ser_addr));
        phe = gethostbyname("127.0.0.1");
        memcpy(&ser_addr.sin_addr, phe->h_addr, phe->h_length);
        ser_addr.sin_family      = AF_INET;
	ser_addr.sin_port        = htons(tryport);

        while(bind(s1, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0){
		tryport++;
        	ser_addr.sin_port        = htons(tryport);
	}
	
	printf("nid %d Bind to UDP port num %d\n", mynid, tryport);
	UDPsocketFd2 = s1;
       
	// create the first TCP conenction
	s = connectTCP("127.0.0.1", SE_IPC_SERVER_PORT_FOR_AGENT, 1);
        p = (struct agentClientInfo *) malloc(sizeof(struct agentClientInfo));
        p->type = AGENT_CLIENT_INFO;
        p->moreMsgFollowing = moreMsgFollowing;
        p->nid = mynid;
        p->pid = getpid();
	p->socket_fd = s;
	p->udpportnum = (u_int16_t) tryport;
        p->humanControlled = humanControlled;
        p->group1 = gid1;
        p->group2 = gid2;
        p->group3 = gid3;
        p->group4 = gid4;
        p->group5 = gid5;
        p->group6 = gid6;
        p->group7 = gid7;
        p->group8 = gid8;
	p->process_type = proc_type;
	n = writen(s, (char *) p, sizeof(struct agentClientInfo));
        if(n < 0) {
          printf("ERROR(%d): Sending agentClientInfo failed\n", errno);
          exit(1);
        }
        free(p);

        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        n = readn(s, (char *) msg, sizeof(struct 
	  GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->type != 
		GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {

          printf("ERROR(%d): Ack type wrong !\n", errno);
          exit(1);
        
	} 
        
	free(msg);

        return(s);
}


unsigned long getMyNodeID()
{
int pid; 
unsigned long nid;
/*
	This API function returns the ID of the node on which the
	calling agent client program is running.
 */

	pid = getpid();
	if (syscall_NCTUNS_misc(syscall_NSC_misc_PIDTONID, pid, 0, (unsigned long)&nid) < 0) {
		printf("System call syscall_NCTUNS_misc failed\n");                 
		exit(1);
	}
	return(nid);
}


void constructGridMapofTheWholeField() {
/*
        This API function is not an IPC command. Instead, it reads the 
        obstacle file exported by the GUI, which is placed in the .sim 
        directory with all other files exported by the GUI. Since all 
        files in the .sim directory will be tarred and copied to a 
        working directory on the simulation engine machine during
        simulation, this function tries to locate this working directory 
        to successfully open the obstacle file.
		 
	The obstacle file contains the field sizes (maxXMeter,
	maxYMeter, maxZMeter), which are returned to the programmer via the
        maxXMeter, maxYMeter, maxZMeter global variables. This file also 
	contains the attributes of every obstacle in the field. Every 
	obstacle is represented as a rectangular and has the following 
	attributes:

           The positions of the four corners of the rectangular 
           obstacle: (X1, Y1),  (X2, Y2),  (X3, Y3), (X4, Y4), These 
	   four positions can be interpreted as either clockwise or 
	   counterclockwise. At present, Z axis is not used. Each obstacle
	   has three three attributes as follows: 

           Block_Node_View ? 
           Block_Node_Movement ?
           Block_Wireless_Signal ?

        The attributes of each obstacle is read out from the obstacle file 
	and stored in an obstacle control block (struct obstacle). 
	These control blocks are linked to form a list and the pointer 
	to this list is returned to the programmer via the obstacleListP
	global variable. The number of obstacles in this list is returned 
	via the Num_obs global variable.
      
        In addition to the information containted in the obstacle 
        file, one input to this function is the width of a grid 
        (used for X, Y, and Z grids) that should be used in the map. 
	Its unit is meter. Based on this information, the sizes of the X, 
        Y, and Z dimensions of the gridMap are returned via the maxXGrid, 
	maxYGrid, and maxZGrid) global variables. Their values are 
	maxXMeter/gridWidthMeter, maxYMeter/gridWidthMeter, and 
	maxZMeter/gridWidthMeter, respectively. With these values, 
        this function dynamically allocates memory space for the gridMap. 

           unsigned char *gridMap = malloc(maxXGrid * maxYGrid);

        and returns the pointer to GridMap to the programmer via the
        gridMapP global variable. (Currently, Z axis is not used.)

        Based on the obstacle information, each element of the gridMap 
	matrix is filled in with one of the values of 0, and 1. A value 
	of 0 represents that the element is not occupied by any obstacle. 
	A value of 1 instead represents that it is in the interior space 
	of an obstacle. 

        This function automatically fills the borders of the gridMap 
	matrix with four obstacles (top/down/left/right) to make the 
	field a closed field. This setting ensures that a mobile 
	node will not move out of the field.

        The returned value indicates whether this function executes 
        successfully (0), or fails (1).
*/

char *FILEPATH;
FILE *fd;

        /* get the obstacle file name and open it */
        FILEPATH = (char *) malloc(strlen(GetWorkingDir()) +
          strlen(agentGetScriptName())+7);
        sprintf(FILEPATH, "%s/%s%s", GetWorkingDir(), agentGetScriptName(), ".obs\0");
        if ((fd=fopen(FILEPATH, "r")) == NULL) {
                printf("Error: can't open file %s\n", FILEPATH);
                exit(-1);
        }
	read_obstacles(fd);

/* The following piece of code should be removed in the final version. */
	maxXGrid = (int) (maxXMeter/gridWidthMeter);
	if ((fmod(maxXMeter, gridWidthMeter)) != 0) maxXGrid++;
	maxYGrid = (int) (maxYMeter/gridWidthMeter) + 1;
	if ((fmod(maxYMeter, gridWidthMeter)) != 0) maxYGrid++;
	gridMapP = (unsigned char *) malloc(sizeof (unsigned char) * maxXGrid 
	  * maxYGrid);
/* The above piece of code should be removed in the final version. */

	m_Map = new Map();
	m_Map->LoadMap();

        free(FILEPATH);
}


int getInitialNodePosition(u_int32_t mynid, double &initX, double &initY, 
double &initZ) { 
/*
  	This API function is not an IPC command. It reads the .sce file 
	exported by the GUI and returns the initial position of the specified 
	mobile node via (initX, initY, initZ). This function returns 0 if
	the initial position of the specified node can be found, otherwise,
	it returns -1 indicating something is wrong.
 */

        FILE                    *fd;
        char                    buf[101];
        u_int32_t               nodeid;
        double                  x, y, z, a_time, p_time, speed;
        int 			n;

        /* get the movement file name and open it */
        char *FILEPATH = (char *)malloc(strlen(GetWorkingDir())+
          strlen(agentGetScriptName())+7);
        sprintf(FILEPATH, "%s/%s%s", GetWorkingDir(), agentGetScriptName(), ".sce\0");
                                                                                
        if ((fd=fopen(FILEPATH, "r")) == NULL) {
                printf("Error: can't open file %s\n", FILEPATH);
                exit(-1);
        }
        free(FILEPATH);

        initX = -1; initX = -1; initX = -1;
        while(!feof(fd)) {
                buf[0] = '\0'; fgets(buf, 100, fd);
                if ((buf[0]=='\0')||(buf[0]=='#'))
                        continue;
                n = sscanf(buf, "$node_(%d) set %lf %lf %lf %lf %lf %lf",
                        &nodeid, &x, &y, &z, &a_time, &p_time, &speed);
                if (n != 7) {
                        printf("Warning: format error -> %s\n", buf);
                        continue;
                }
                if (nodeid == mynid) {
 			initX = x;
 			initY = y;
 			initZ = z;
                        fclose(fd);
                        return(0);
                }
        }
	fclose(fd);
	return(-1);
}

void getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(double 
curX, double curY, double curZ, double 
movingDirectionInDegreeViewedOnGUIScreen, double &collisionX, double 
&collisionY, double &collisionZ, double &distance, double &obstacleAngle) {
/*
        This API function is not an IPC command. Based on the information
        of the obstacles contained in the obstacleListP list, this function
        calculates the nearest collision point (collisionX, collisionY, 
	collisionZ) along the specified moving direction from the specified 
	position (curX, curY, curZ). The distance between the two points 
	is returned via distance. The direction of the encountered obstacle
	is returned via obstacleAngle.

 */
	double a1, b1, c1, dist, dx, dy, minDist;
	double a2, b2, c2, ix, iy, minix, miniy, angle, obsangle;
	double lx, rx, ty, by;
	u_int32_t i;
	int numSolution1, numSolution2, numSolution3, numSolution4;
	struct obstacle *tmpObs;

	minDist = 99999999; // infinity
	minix = 0; miniy = 0;
	PolarEquation(a1, b1, c1, curX, curY, 
	  movingDirectionInDegreeViewedOnGUIScreen);
	
	for(i = 1, tmpObs=obstacleListP; i<=Num_obs && tmpObs != NULL; 
		i++, tmpObs = tmpObs->next) {

                if (!tmpObs->blockMovement)
                        continue;

		/* check intersection */

		TwoPointEquation(a2, b2, c2, tmpObs->x1, tmpObs->y1,
			tmpObs->x2, tmpObs->y2);
		numSolution1 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
		if (numSolution1 == UNIQUE_SOLUTION) {
			if (tmpObs->x1 > tmpObs->x2) {
				rx = tmpObs->x1; lx = tmpObs->x2;
			} else {
				rx = tmpObs->x2; lx = tmpObs->x1;
			}
			if (tmpObs->y1 > tmpObs->y2) {
				by = tmpObs->y1; ty = tmpObs->y2;
			} else {
				by = tmpObs->y2; ty = tmpObs->y1;
			}
			lx = lx - 0.00001;	// calculate secondorder_SimultaneousEqution 
			ty = ty - 0.00001;	// may be cause dobule inaccuracy
			rx = rx + 0.00001;	// use 10^(-5) range to fix it
			by = by + 0.00001;	//
			if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by)) {

				angle = CalcuAngle(curX, curY, ix, iy);
				if (fabs(angle - movingDirectionInDegreeViewedOnGUIScreen) < 10) {
				// Roughly in the same direction
					dx = curX - ix;
					dy = curY - iy;
					dist = sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						obsangle = CalcuAngle(tmpObs->x1, tmpObs->y1, tmpObs->x2, tmpObs->y2);
						if (obsangle > 180) obsangle -= 180;
						minDist = dist;
						minix = ix;
						miniy = iy;
					}
				}

			}

		}

		TwoPointEquation(a2, b2, c2, tmpObs->x2, tmpObs->y2,
			tmpObs->x3, tmpObs->y3);
		numSolution2 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution2 == UNIQUE_SOLUTION) {
			if (tmpObs->x2 > tmpObs->x3) {
				rx = tmpObs->x2; lx = tmpObs->x3;
			} else {
				rx = tmpObs->x3; lx = tmpObs->x2;
			}
			if (tmpObs->y2 > tmpObs->y3) {
				by = tmpObs->y2; ty = tmpObs->y3;
			} else {
				by = tmpObs->y3; ty = tmpObs->y2;
			}
			lx = lx - 0.00001;
                        ty = ty - 0.00001;
                        rx = rx + 0.00001;
                        by = by + 0.00001;
			if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by)) {

				angle = CalcuAngle(curX, curY, ix, iy);
				if (fabs(angle - movingDirectionInDegreeViewedOnGUIScreen) < 10) {
				// Roughly in the same direction
					dx = curX - ix;
					dy = curY - iy;
					dist = sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						obsangle = CalcuAngle(tmpObs->x2, tmpObs->y2, tmpObs->x3, tmpObs->y3);
						if (obsangle > 180) obsangle -= 180;
						minDist = dist;
						minix = ix;
						miniy = iy;
					}
				}
			}

                }

		TwoPointEquation(a2, b2, c2, tmpObs->x3, tmpObs->y3,
			tmpObs->x4, tmpObs->y4);
		numSolution3 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution3 == UNIQUE_SOLUTION) {
			if (tmpObs->x3 > tmpObs->x4) {
				rx = tmpObs->x3; lx = tmpObs->x4;
			} else {
				rx = tmpObs->x4; lx = tmpObs->x3;
			}
			if (tmpObs->y3 > tmpObs->y4) {
				by = tmpObs->y3; ty = tmpObs->y4;
			} else {
				by = tmpObs->y4; ty = tmpObs->y3;
			}
			lx = lx - 0.00001;
                        ty = ty - 0.00001;
                        rx = rx + 0.00001;
                        by = by + 0.00001;
			if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by)) {

				angle = CalcuAngle(curX, curY, ix, iy);
				if (fabs(angle - movingDirectionInDegreeViewedOnGUIScreen) < 10) {
				// Roughly in the same direction
					dx = curX - ix;
					dy = curY - iy;
					dist = sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						obsangle = CalcuAngle(tmpObs->x3, tmpObs->y3, tmpObs->x4, tmpObs->y4);
						if (obsangle > 180) obsangle -= 180;
						minDist = dist;
						minix = ix;
						miniy = iy;
					}
				}
			}

                }


		TwoPointEquation(a2, b2, c2, tmpObs->x4, tmpObs->y4,
			tmpObs->x1, tmpObs->y1);
		numSolution4 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution4 == UNIQUE_SOLUTION) {
			if (tmpObs->x4 > tmpObs->x1) {
				rx = tmpObs->x4; lx = tmpObs->x1;
			} else {
				rx = tmpObs->x1; lx = tmpObs->x4;
			}
			if (tmpObs->y4 > tmpObs->y1) {
				by = tmpObs->y4; ty = tmpObs->y1;
			} else {
				by = tmpObs->y1; ty = tmpObs->y4;
			}
			lx = lx - 0.00001;
                        ty = ty - 0.00001;
                        rx = rx + 0.00001;
                        by = by + 0.00001;
			if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by)) {

				angle = CalcuAngle(curX, curY, ix, iy);
				if (fabs(angle - movingDirectionInDegreeViewedOnGUIScreen) < 10) {
				// Roughly in the same direction
					dx = curX - ix;
					dy = curY - iy;
					dist = sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						obsangle = CalcuAngle(tmpObs->x4, tmpObs->y4, tmpObs->x1, tmpObs->y1);
						if (obsangle > 180) obsangle -= 180;
						minDist = dist;
						minix = ix;
						miniy = iy;
					}
				}
			}

                }
	}
	collisionX = minix;
	collisionY = miniy;
	collisionZ = 0;	// not used
	distance = minDist;
	obstacleAngle = obsangle;
}


void getFarthestVisablePointAlongTheMovingDirectionViewedOnGUIScreen(double
curX, double curY, double curZ, double 
movingDirectionInDegreeViewedOnGUIScreen, double &visualX, double &visualY, 
double &visualZ, double &distance) {
/*
        This API function is not an IPC command. Based on the information
        of the obstacles contained in the obstacleListP list, this function
        calculates the farthest point (visualX, visualY, visualZ) that can
	be seen along the specified moving direction 
	(movingDirectionInDegreeViewedOnGUIScreen) from the specified
        position (curX, curY, curZ). The distance between the two points
        is returned in distance.
 */

	double a1, b1, c1, dist, dx, dy, maxDist;
	double a2, b2, c2, ix, iy, maxix, maxiy;
	u_int32_t i;
	int numSolution1, numSolution2, numSolution3, numSolution4;
	struct obstacle *tmpObs;

	maxDist = 0; 
	maxix = 0; maxiy = 0;
	PolarEquation(a1, b1, c1, curX, curY, 
	  movingDirectionInDegreeViewedOnGUIScreen);
	
	for(i=1, tmpObs=obstacleListP; i<=Num_obs && tmpObs != NULL; 
		i++, tmpObs = tmpObs->next) {

                if (!tmpObs->blockView)
                        continue;

		/* check intersection */

		TwoPointEquation(a2, b2, c2, tmpObs->x1, tmpObs->y1,
			tmpObs->x2, tmpObs->y2);
		numSolution1 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
		if (numSolution1 == UNIQUE_SOLUTION) {
			dx = curX - ix;
			dy = curY - iy;
			dist = sqrt(dx*dx + dy*dy);
			if (dist > maxDist) {
				maxDist = dist;
				maxix = ix;
				maxiy = iy;
			}
		}

		TwoPointEquation(a2, b2, c2, tmpObs->x2, tmpObs->y2,
			tmpObs->x3, tmpObs->y3);
		numSolution2 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution2 == UNIQUE_SOLUTION) {
                        dx = curX - ix;
                        dy = curY - iy;
                        dist = sqrt(dx*dx + dy*dy);
                        if (dist > maxDist) {
                                maxDist = dist;
                                maxix = ix;
                                maxiy = iy;
                        }
                }

		TwoPointEquation(a2, b2, c2, tmpObs->x3, tmpObs->y3,
			tmpObs->x4, tmpObs->y4);
		numSolution3 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution3 == UNIQUE_SOLUTION) {
                        dx = curX - ix;
                        dy = curY - iy;
                        dist = sqrt(dx*dx + dy*dy);
                        if (dist > maxDist) {
                                maxDist = dist;
                                maxix = ix;
                                maxiy = iy;
                        }
                }


		TwoPointEquation(a2, b2, c2, tmpObs->x4, tmpObs->y4,
			tmpObs->x1, tmpObs->y1);
		numSolution4 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution4 == UNIQUE_SOLUTION) {
                        dx = curX - ix;
                        dy = curY - iy;
                        dist = sqrt(dx*dx + dy*dy);
                        if (dist > maxDist) {
                                maxDist = dist;
                                maxix = ix;
                                maxiy = iy;
                        }
                }
	}
	visualX = maxix;
	visualY = maxiy;
	visualZ = 0;	// not used
	distance = maxDist;
}

double twoPointsDistance(double x1, double y1, double x2, double y2) {
/*
        This API function returns the distance between (x1, y1) and 
	(x2, y2).
*/
	
	double dx, dy, dist;

	dx = x1 - x2;
	dy = y1 - y2;
	dist = sqrt(dx*dx + dy*dy);
	return(dist);
}

/* Helper function */
unsigned char getGridElement(int xGrid, int yGrid) {
/*
  	This function gets the grid element located at (xGrid, yGrid).
 */

	return(*(gridMapP + (yGrid * maxXGrid + xGrid)));
	
}

int inObstacleGridLocation(double x, double y, double z) {
/*
       	This API function is not an IPC command. It checks whether the point 
	(x, y, z) falls into a grid location that is occupied by an obstacle. 
	Currently, z is ignored.
 */
        int xGrid, yGrid;  
		
        xGrid = (int) x/gridWidthMeter;
        yGrid = (int) y/gridWidthMeter;

        if (getGridElement(xGrid, yGrid) == 0) return(FALSE);
        else return(TRUE);

}

int setCurrentWaypoint(int myTCPsockfd, u_int32_t mynid, double x, double y, 
double z, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to
        the simulation engine. Upon receiving this command,
        the simulation engine sets the current waypoint of the specified
        mobile node to the position specified by (curX, curY, curZ).
        The position of the next waypoint of this mobile node remains 
        unchanged. The moving direction (unit vector) of this mobile node 
        will need to be adjusted according to the new current waypoint. 
        The moving speed of this mobile node remaints unchanged.
	
        Note that a mobile node moves from its current waypoint to its
        next waypoint linearly at a given constant speed. 
        The current position of the mobile node returned by 
        getCurrentPosition() is calculated based on currentWaypoint + 
        movingDirectionUnitVector * MovingSpeed * (CurTime - 
        theTimeWhenTheCurrentWaypointIsReached). Therefore, after reaching 
 	its next waypoint, the mobile node will keep moving in the current 
        direction unless its next waypoint (which affects its 
        movingDirectionUnitVector) is updated to another location.

        Note that when setCurrentWaypoint() is called, if the agent program 
        immediately calls getCurrentPosition(), the reported current position 
        of the mobile node will be changed to the specified current waypoint 
        (curX, curY, curZ). Therefore, if the specified position is not the 
        same as the current position reported by getCurrentPosition(), the 
        mobile node may unrealisticaly jump in the field.

        To avoid this unrealistic phenomenon, the (curX, curY, curZ) 
        arguments provided to setCurrentWaypoint() normally should be the 
        current position of the mobile node, which can be provided by 
        getCurrentPosition().
          
        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

char	*msg;
int 	n, result;

	if (mynid <= 0) return(-1);
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientSetCurrentWayPoint *) msg)->type = 
	  AGENT_CLIENT_SET_CURRENT_WAYPOINT;
	((struct agentClientSetCurrentWayPoint *) msg)->nid = mynid;
	((struct agentClientSetCurrentWayPoint *) msg)->x = x;
	((struct agentClientSetCurrentWayPoint *) msg)->y = y;
	((struct agentClientSetCurrentWayPoint *) msg)->z = z;
	((struct agentClientSetCurrentWayPoint *) msg)->moreMsgFollowing = 
	  moreMsgFollowing;
	n = writen(myTCPsockfd, msg, sizeof(struct 
	  agentClientSetCurrentWayPoint));
	n = readn(myTCPsockfd, msg,  sizeof(struct
	  GeneralACKBetweenAgentClientAndSimulationEngine));
	if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
	  ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
	  printf("[%s]: IPC ACK failed.\n", __func__);
	  exit(0);
	}
	result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result; 

	free(msg);
	return(result); 

}

int getCurrentWaypoint(int myTCPsockfd, int mynid, double &x, double &y, 
double &z, int moreMsgFollowing) {
/*
	This API function forms and IPC command and sends it to the
	simulation engine asking it to return the current waypoint of the 
	specified mobile node via (x, y, z).

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientGetCurrentWayPoint *) msg)->type = 
	  AGENT_CLIENT_GET_CURRENT_WAYPOINT;
        ((struct agentClientGetCurrentWayPoint *) msg)->nid = mynid;
        ((struct agentClientGetCurrentWayPoint *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetCurrentWayPoint));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetCurrentWayPointReply));
        if (((struct agentClientGetCurrentWayPointReply *) msg)
          ->type != AGENT_CLIENT_GET_CURRENT_WAYPOINT_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_WAYPOINT_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetCurrentWayPointReply *) msg)->result; 
	x = ((struct agentClientGetCurrentWayPointReply *) msg)->x;
	y = ((struct agentClientGetCurrentWayPointReply *) msg)->y;
	z = ((struct agentClientGetCurrentWayPointReply *) msg)->z;

	free(msg);
        return(result); 
}

int setNextWaypoint(int myTCPsockfd, int mynid, double x, double y, 
double z, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the 
	simulation engine. Upon receiving this command, the simulation 
	engine sets the next waypoint of the specified mobile node to 
	the position specified by (x, y, z). The current waypoint 
	of this mobile node remains unchanged. However, the moving direction 
	unit vector of this mobile node is adjusted corresponding to the new 
        next waypoint. The moving speed of this mobile node remaints unchanged.

        Note that calling setNextWaypoint() without calling 
	setCurrentWaypoint() at the same time may cause the mobile node 
        to unrealisticaly jump in the field. This is because 
	getCurrentPosition() uses the following formula:

	currentPosition = 
          currentWaypoint + movingDirectionUnitVector * movingSpeed * 
	  (curTime - theTimeWhenTheCurrentWaypointIsReached) 

        to report the current position of the mobile node. To avoid this 
        unrealistic phenomenon, the simulation engine internally calls 
	setNodeCurrentWaypoint(getNodePosition()) before calling 
	setNodeNextWaypoint(nextX, nextY, nextZ). That is, the current 
	waypoint is first set to the current position before the simulation
	engine sets the next waypoint of the mobile node. 
              
        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetNextWayPoint *) msg)->type =
          AGENT_CLIENT_SET_NEXT_WAYPOINT;
        ((struct agentClientSetNextWayPoint *) msg)->nid = mynid;
        ((struct agentClientSetNextWayPoint *) msg)->x = x;
        ((struct agentClientSetNextWayPoint *) msg)->y = y;
        ((struct agentClientSetNextWayPoint *) msg)->z = z;
        ((struct agentClientSetNextWayPoint *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetNextWayPoint));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);
        return(result);

}

int getCurrentMovingDirectionViewedOnGUIScreen(int myTCPsockfd, int mynid,
double &angle, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation
        engine. Upon receiving this command, the simulation engine returns
        the current moving direction of the specified mobile node via angle.
        Note that the moving direction is viewed on the GUI screen rather than
        in the normal coordinate system.
                                                                                
        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.
                                                                                
        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
*/

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetCurrentMovingDirection *) msg)->type =
          AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION;
        ((struct agentClientGetCurrentMovingDirection *) msg)->nid = mynid;
        ((struct agentClientGetCurrentMovingDirection *) msg)->
	  moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetCurrentMovingDirection));
        n = readn(myTCPsockfd, msg,  sizeof(struct
	  agentClientGetCurrentMovingDirectionReply));
        if (((struct agentClientGetCurrentMovingDirectionReply *) msg)           ->type != AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetCurrentMovingDirectionReply *) msg)->result;
        angle = ((struct agentClientGetCurrentMovingDirectionReply *) msg)
	  ->angle;
                                                                                
        free(msg);
        return(result);
                                                                                
}

int setCurrentAndNextWaypoint(int myTCPsockfd, int mynid, double curX, 
double curY, double curZ, double nextX, double nextY, double nextZ, 
int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine. Upon receiving this command, the simulation engine sets 
	the specified mobile node's current waypoint to (curX, curY, curZ) 
	and its next waypoints to (nextX, nextY, nextZ);
	   
        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->type =
          AGENT_CLIENT_SET_CURRENT_AND_NEXT_WAYPOINT;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nid = mynid;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->curx = curX;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->cury = curY;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->curz = curZ;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nextx = nextX;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nexty = nextY;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nextz = nextZ;
        ((struct agentClientSetCurrentAndNextWayPoint *) msg)->moreMsgFollowing
           = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetCurrentAndNextWayPoint));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
	result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);         
	return(result);

}

int setCurrentMovingSpeed(int myTCPsockfd, int mynid, double curSpeed, 
int moreMsgFollowing) {
/*
	
        This API function forms an IPC command and sends it to the simulation 
	engine. Upon receiving this command, the simulation engine sets the 
	moving speed of the specified mobile node to curSpeed. Because the 
	current position of the mobile node returned by getCurrentPosition() 
	is based on the following formula: 

        currentPosition =
          currentWaypoint + movingDirectionUnitVector * movingSpeed *
          (curTime - theTimeWhenTheCurrentWaypointIsReached)

        the mobile node may unrealisticaly jump in the field when its 
	moving speed is changed.

        To avoid this unrealistic phenomenon, the simulation engine 
        internally calls setNodeCurrentWaypoint(getNodePosition()) before 
        calling setNodeSpeed();

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetCurrentMovingSpeed *) msg)->type =
          AGENT_CLIENT_SET_CURRENT_MOVING_SPEED;
        ((struct agentClientSetCurrentMovingSpeed *) msg)->nid = mynid;
        ((struct agentClientSetCurrentMovingSpeed *) msg)->speed = curSpeed;
        ((struct agentClientSetCurrentMovingSpeed *) msg)->moreMsgFollowing
	   = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetCurrentMovingSpeed));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
	result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);         
	return(result);

}

int getCurrentMovingSpeed(int myTCPsockfd, int mynid, double &curSpeed,
int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation
        engine. Upon receiving this command, the simulation engine returns 
	the current moving speed of the specified mobile node via curSpeed. 

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.
                                                                                
        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientGetCurrentMovingSpeed *) msg)->type =
          AGENT_CLIENT_GET_CURRENT_MOVING_SPEED;
        ((struct agentClientGetCurrentMovingSpeed *) msg)->nid = mynid;
        ((struct agentClientGetCurrentMovingSpeed *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetCurrentMovingSpeed));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetCurrentMovingSpeedReply));
        if (((struct agentClientGetCurrentMovingSpeedReply *) msg)
          ->type != AGENT_CLIENT_GET_CURRENT_MOVING_SPEED_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_MOVING_SPEED_REPLY failed. %d\n", ((struct agentClientGetCurrentMovingSpeedReply *) msg)->type);
          exit(0);
        }
        result = ((struct agentClientGetCurrentMovingSpeedReply *) msg)->result;
        curSpeed = ((struct agentClientGetCurrentMovingSpeedReply *) msg)->speed;
                                                                                
        free(msg);
        return(result);

}

int setNextWaypointAndMovingSpeed(int myTCPsockfd, int mynid, double nextX, 
double nextY, double nextZ, double curSpeed, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation
        engine. Upon receiving this command, the simulation engine sets
        the specified mobile node's next waypoints to (nextX, nextY, nextZ), 
	and its current speed to curSpeed. Because the current position of 
	the mobile node returned by getCurrentPosition() is based on the 
	following formula:
                                                                                
        currentPosition =
          currentWaypoint + movingDirectionUnitVector * movingSpeed *
          (curTime - theTimeWhenTheCurrentWaypointIsReached)
                                                                                
        the mobile node may unrealisticaly jump in the field when its
        next waypoint and(or) moving speed is changed.
                                                                                
        To avoid this unrealistic phenomenon, the simulation engine
        internally calls setNodeCurrentWaypoint(getNodePosition()) before
        calling setNodeSpeed() and setNodeNextWaypoint();

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->type =
          AGENT_CLIENT_SET_NEXT_WAYPOINT_AND_MOVING_SPEED;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nid = mynid;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nextx = 
	  nextX;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nexty = 
	  nextY;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nextz = 
	  nextZ;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->speed = 
	  curSpeed;
        ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->
	  moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetNextWayPointAndMovingSpeed));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);
        return(result);

}

int getCurrentPosition(int myTCPsockfd, int mynid, double &curX, double &curY, 
double &curZ, int moreMsgFollowing) {
/*

        This API function forms an IPC command and sends it to the simulation 
	engine. Upon receiving this command, the simulation engine calculates 
	the current positon of the mobile node specified by mynid and returns 
	its current position via (curX, curY, curY).

	The current position of a mobile node returned by getCurrentPosition() 
	is based on the following formula:
                                                                                
        currentPosition =
          currentWaypoint + movingDirectionUnitVector * movingSpeed *
          (curTime - theTimeWhenTheCurrentWaypointIsReached)

        Therefore, after reaching its next waypoint, a mobile node will
        keep moving in the current direction unless its next waypoint 
        (which affects its movingDirectionUnitVector) is set to another 
	location. It will not stay at its next waypoint.

        The current waypoint, next waypoint, moving direction, and moving 
	speed of this mobile node all remain unchanged when this function 
	is called.

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg , *longMsg;
int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        longMsg = (char *)malloc(sizeof(struct agentClientGetCurrentPositionReply));
	((struct agentClientGetCurrentPosition *) msg)->type = AGENT_CLIENT_GET_CURRENT_POSITION;
        ((struct agentClientGetCurrentPosition *) msg)->nid = mynid;
        ((struct agentClientGetCurrentPosition *) msg)->moreMsgFollowing = moreMsgFollowing;

	n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetCurrentPosition));
	n = readn(myTCPsockfd, longMsg, sizeof(struct agentClientGetCurrentPositionReply));
	if (((struct agentClientGetCurrentPositionReply *)longMsg)->type != AGENT_CLIENT_GET_CURRENT_POSITION_REPLY)
	{
		printf("Nid %d IPC AGENT_CLIENT_GET_CURRENT_POSITION_REPLY failed.\n", mynid);
		exit(0);
	}
	result = ((struct agentClientGetCurrentPositionReply *) longMsg)->result;
	curX = ((struct agentClientGetCurrentPositionReply *) longMsg)->x;
	curY = ((struct agentClientGetCurrentPositionReply *) longMsg)->y;
        curZ = ((struct agentClientGetCurrentPositionReply *) longMsg)->z;

        free(msg);
	free(longMsg);
        return(result);

}

int getCurrentPositionOfAGroupOfNode(int myTCPsockfd, int mynid, int gid, 
struct nodePosition **groupNodePositionArray, int &numNodeInThisArray, 
int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine. Upon receiving this command, the simulation engine 
	calculates the current positon of every mobile node that belongs to 
	the group specified by gid and collects these information into 
	an array of struct nodePosition and returns the pointer to this array
	via groupNodePositionArray. The number of struct nodePosition in
	the returned array is returned via numNodeInThisArray. After using
	this array, the agent program is responsible for releasing the 
	memory space occupied by this array by calling 
	free(groupNodePositionArray).

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg, *longMsg;
int     result,n, numNode;

        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        longMsg = (char *) malloc(sizeof(char) * IP_PACKET_MAX_LEN);

        ((struct agentClientGetCurrentPositionOfAGroupOfNode *) msg)->type =
          AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE;
        ((struct agentClientGetCurrentPositionOfAGroupOfNode *) msg)->gid = gid;
        ((struct agentClientGetCurrentPositionOfAGroupOfNode *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetCurrentPositionOfAGroupOfNode));
        n = readn(myTCPsockfd, longMsg,  sizeof(struct
          agentClientGetCurrentPositionOfAGroupOfNodeReply));
        if (((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)
          ->type != AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GOURP_OF_NODE_REPLY failed.\n");
          exit(0);
        }  
        result = ((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) 
	  longMsg)->result;
        numNode = ((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->numNode;
	numNodeInThisArray = numNode;
        n = readn(myTCPsockfd, longMsg, sizeof(struct nodePosition) * numNode);
	if (numNode > 0) {
	  *groupNodePositionArray = (struct nodePosition *) longMsg;
	}
                                                                                
        free(msg);
	//free(longMsg);
        return(result);

}

int getCurrentPositionOfATeamOfNode(int myTCPsockfd, int mynid, int tid,
struct nodePosition **groupNodePositionArray, int &numNodeInThisArray,
int moreMsgFollowing) {
char    *msg, *longMsg;
int     result,n, numNode;

        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        longMsg = (char *) malloc(sizeof(char) * IP_PACKET_MAX_LEN);

        ((struct agentClientGetCurrentPositionOfATeamOfNode *) msg)->type =
          AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE;
        ((struct agentClientGetCurrentPositionOfATeamOfNode *) msg)->tid = tid;
        ((struct agentClientGetCurrentPositionOfATeamOfNode *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientGetCurrentPositionOfATeamOfNode));
        n = readn(myTCPsockfd, longMsg,  sizeof(struct agentClientGetCurrentPositionOfATeamOfNodeReply));
        if (((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)
          ->type != AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE_REPLY failed. %d\n",(((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)->type));
          exit(0);
        }
        result = ((struct agentClientGetCurrentPositionOfATeamOfNodeReply *)longMsg)->result;
        numNode = ((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)->numNode;
        numNodeInThisArray = numNode;
        n = readn(myTCPsockfd, longMsg, sizeof(struct nodePosition) * numNode);
        if (numNode > 0) {
          *groupNodePositionArray = (struct nodePosition *) longMsg;
        }
        free(msg);
        //free(longMsg);
        return(result);
 }

//New add IPC for faster communication
int getCurrentPositionOfAGroupOfNodeInADistance(int myTCPsockfd, int mynid, int gid, 
		struct nodePosition **groupNodePositionArray, int &numNodeInThisArray, 
		double CurrentPOS_x, double CurrentPOS_y, double distance, int moreMsgFollowing) {

char    *msg, *longMsg;
int     result,n, numNode;

        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        longMsg = (char *) malloc(sizeof(char) * IP_PACKET_MAX_LEN);

        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->type =
          AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE;
        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->gid = gid;
        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->moreMsgFollowing = moreMsgFollowing;
        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->CurrentPOS_x = CurrentPOS_x;
        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->CurrentPOS_y = CurrentPOS_y;
        ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->distance = distance;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetCurrentPositionOfAGroupOfNodeInADistance));
        n = readn(myTCPsockfd, longMsg,  sizeof(struct
          agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply));
        if (((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)
          ->type != AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE_REPLY) {
          printf("IPC AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GOURP_OF_NODE_REPLY IN A DISTANCE failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) 
	  longMsg)->result;
        numNode = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->numNode;
	numNodeInThisArray = numNode;
        n = readn(myTCPsockfd, longMsg, sizeof(struct nodePosition) * numNode);
	if (numNode > 0) {
	  *groupNodePositionArray = (struct nodePosition *) longMsg;
	}
                                                                                
        free(msg);
	//free(longMsg);
        return(result);

}

void getVisableMobileNodesFromThePosition(double x, double y, double z, 
struct nodePosition *groupNodePositionArray, int numNodeInTheArray, struct 
nodePosition **visableNodePositionArray, int &numNodeVisable) {
/*
        This API function is not an IPC command. Based on the provided 
	obstacle list (which is pointed to by obstacleListP and its obstacle 
	number is given by Num_obs), for each mobile node in the 
	nodePosition array pointed to by groupNodePositionArray, this funtion 
	checks whether it can be "seen" from the position specified by 
	(x, y, z). The numNodeInTheArray argument indicates the number of 
	entries in this array. The positions of the mobile nodes that can 
	be seen are grouped to form an array of struct nodePosition pointed 
	to by visableNodePositionArray. The number of entries in this array 
	is returned via numNodeVisable. The agent program is responsible for 
	freeing the memory space occupied by the returned 
	visableNodePositionArray after its use.

        Here "seen" means that there is no obstacle sitting on the visual
        line connecting the specified position (x, y, z) with the postion 
	of the checked mobile node. An agent program can use this function 
	to implement its "eyes" by itself without burdening the simulation 
	engine.
 */

u_int32_t i;
int j, viewblocked, numVisable = 0;
int numSolution1, numSolution2, numSolution3, numSolution4;
struct nodePosition *arrayp, *np, tmpArray[MAX_NODES];
double a1, b1, c1, a2, b2, c2, ix, iy;
struct obstacle *tmpObs;

	np = groupNodePositionArray;
	for (j = 0; j < numNodeInTheArray; j++) {
	  TwoPointEquation(a1, b1, c1, x, y, np->x, np->y);
	  viewblocked = FALSE;
	  for(i = 1, tmpObs = obstacleListP; i <= Num_obs && 
	    tmpObs != NULL; i++, tmpObs = tmpObs->next) {

            if (!tmpObs->blockView)
              continue;

	    /* check intersection */

	    TwoPointEquation(a2, b2, c2, tmpObs->x1, tmpObs->y1,
	      tmpObs->x2, tmpObs->y2);
	    numSolution1 = secondorder_SimultaneousEqution(a1, b1, c1, 
	      a2, b2, c2, ix, iy);
	    TwoPointEquation(a2, b2, c2, tmpObs->x2, tmpObs->y2,
	      tmpObs->x3, tmpObs->y3);
	    numSolution2 = secondorder_SimultaneousEqution(a1, b1, c1, 
	      a2, b2, c2, ix, iy);
	    TwoPointEquation(a2, b2, c2, tmpObs->x3, tmpObs->y3,
	      tmpObs->x4, tmpObs->y4);
	    numSolution3 = secondorder_SimultaneousEqution(a1, b1, c1, 
              a2, b2, c2, ix, iy);
	    TwoPointEquation(a2, b2, c2, tmpObs->x4, tmpObs->y4,
	      tmpObs->x1, tmpObs->y1);
	    numSolution4 = secondorder_SimultaneousEqution(a1, b1, c1, 
	      a2, b2, c2, ix, iy);
	    if ((numSolution1 + numSolution2 + numSolution3 + numSolution4) 
	      > 0) {
	      // The checked mobile node cannot be seen from the (x, y, z) 
	      // point because some obstacles sit between them. 
		viewblocked = TRUE;
		break;
	    } 
	  } // for i
	  if (viewblocked == FALSE) {
	    tmpArray[numVisable].nid = np->nid;
	    tmpArray[numVisable].x = np->x;
	    tmpArray[numVisable].y = np->y;
	    tmpArray[numVisable].z = np->z;
	    numVisable++;
	  }
	  np++;
	} // for j

        if (numVisable > 0) {
   	  arrayp = (struct nodePosition *) malloc(sizeof(struct nodePosition) 
	    * numVisable);
	  np = arrayp;
	  for (i = 0; i < (u_int32_t)numVisable; ++i) {
	    np->nid = tmpArray[i].nid;
	    np->x = tmpArray[i].x;
	    np->y = tmpArray[i].y;
	    np->z = tmpArray[i].z;
	    np++;
	  }
  	} else {
	  arrayp = NULL;
	}
	numNodeVisable = numVisable;
	*visableNodePositionArray = arrayp;
}

void getSurroundingViews(double x, double y, double z, double 
**viewDistance360Degree) {
/*
        This API function is not an IPC command. Based on the given
	obstacle list pointed to by obstacleListP (whose number is given
	by Num_obs), for each degree from 0 to 359, this function
	calculates how far the mobile node whose position is specified by
	(x, y, z) can "see" before its view is blocked by some obstacle. 
	The calculated distance information is returned via an array with 
	consisting of 360 distances of "double" type, which is pointed to 
	by viewDistance360Degree. The agent program is responsible for 
	freeing the memory space occupied by the returned array pointed to 
	by viewDistance360Degree.

        Note that the degree is viewed on the GUI screen.  Therefore, 
	degree 0 points to the right, degree 90 points to the top, 
	degree 180 points to the left, and degree 270 points to the down.
      
        An agent program can use this function to implement its "eyes"
        by itself without burdening the simulation engine. 
 */

int i;
double distance, cx, cy, cz;
double *viewDistanceArray, obsAngle;

	viewDistanceArray = (double *) malloc(sizeof(double) * 360);
	for (i = 0; i < 360; ++i) {
	  getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(x, 
	    y, z, i, cx, cy, cz, distance, obsAngle);
	  viewDistanceArray[i] = distance;
	}
	*viewDistance360Degree = viewDistanceArray;
}


int requestNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd, 
int mynid, int registrationID, int timeIntervalInMilliseconds, 
int anotherNodeID, int gid, double withinRangeinMeter, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine. Upon receiving this command, the simulation engine creates 
	and registers a struct registeredApproachingCheckingRecord entry 
	in its triggering system.

        The triggering system is invoked periodically by the simulation engine 
	at a fine grain (once every 0.1 second) to check whether the registered 
	circumstance has happened or not. If timeIntervalInMilliseconds is 
	set to 0, the triggering system will use its default frequency to 
	check and report the checking results. If timeIntervalInMilliseconds 
	is set to a value > 0, the triggering system will check the registered 
	circumstance once every timeIntervalInMilliseconds milliseconds.

        If anotherNodeID is greater than 0, this function asks the triggering 
	system to monitor whether the mobile node specified by anotherNodeID 
	has approached the mobile node specified by mynid within a distance 
	specified by withinRangeInMeter. If anotherNodeID is less than 0, 
	it means that a group of nodes rather than a single node should be 
	monitored and in this case the value of gid should be used.

        If gid is greater than 0, the triggering system will monitor any 
	mobile node belonging to the group specified by gid. If gid is less 
	than 0, the simulation engine will ignore it. When the registered 
	circumstance occurs, the simulation engine will send a 
	struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe 
	notification IPC message to the agent program running on the node 
	specified by mynid. 

        When the registered circumstance persists, the triggering system will 
	continuously send a 
	SimulationEngineNotificationWhenAnotherNodeHasApproachedMe  
	notification IPC message to the agent program running on the node 
	specified by mynid. As long as the registered circumstance persists, 
	the triggering system will send such an IPC message to the agent 
	program every time when it is invoked. This continuous message 
	sending will stop when the registered circumstance no longer exists.

        The argument withinRangeInMeter can be set to 0. In such a setting, 
	a mobile nodes may have collided with another mobile node when such 
	a situation is detected. If withinRangeInMeter is set to a larger 
	value, a mobile node will have more space to make turns to avoid 
	collisions.  

  	The registrationID must be provided by the agent program and be unique 
	across all registered notification requests issued within the agent 
	program. Later on, the agent program may cancel, suspend, or resume 
	a registered notification request by providing its corresponding 
	registrationID.

        After receiving a notification IPC message, the agent program needs to
        send back an ACK packet (struct 
	GeneralACKBetweenAgentClientAndSimulationEngine) to the simulation 
	engine. The ACK packet carries a moreMsgFollowing field to indicate 
	whether the agent program wants to issue more IPC commands to the 
	simulation engine after this ACK packet. In case the carried 
        moreMsgFollowing is 0, the simulation engine can process its other 
        events and advance its simulation clock. Otherwise, it must wait 
        until all following IPC commands have been processed.

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if ((mynid <= 0) || (registrationID < 0)) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->type = AGENT_CLIENT_REQUEST_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid = mynid;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID = registrationID;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->timeIntervalInMilliseconds = timeIntervalInMilliseconds;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->gid = gid;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->anid = anotherNodeID;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->withinRangeinMeter = withinRangeinMeter;
        ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientRequestNotificationWhenAnotherNodeHasApproachedMe));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);
        return(result);

}

int cancelNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
int mynid, int registrationID, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine to cancel a previously registered notification request. Upon 
	receiving this command, the simulation engine uses the provided 
	node ID (mynid) and registration ID (registrationID) to search and 
	remove the specified request record. 

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
*/

char    *msg;
int     n, result;
                                                                                                                            
        if ((mynid <= 0) || (registrationID < 0)) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->type = AGENT_CLIENT_CANCEL_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
        ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid = mynid;
        ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID = registrationID;
        ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientCancelNotificationWhenAnotherNodeHasApproachedMe));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;

        free(msg);
        return(result);

}

int suspendNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
int mynid, int registrationID, int timeIntervalInMilliseconds,
int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine to suspend a previously registered notification request. Upon 
	receiving this command, the simulation engine uses the provided 
	node ID (mynid) and registration ID (registrationID) to locate and
	suspend the specified request record.

	if timeIntervalInMilliseconds is set to 0, the request will be 
	suspended forever. If timeIntervalInMilliseconds is set to a value > 0,
	the request will be suspended from now to 
	now + timeIntervalInMilliseconds only.
                                                                                
        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
*/

char    *msg;
int     n, result;
                                                                                                                            
        if ((mynid <= 0) || (registrationID < 0)) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->type = AGENT_CLIENT_SUSPEND_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
        ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid = mynid;
        ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID = registrationID;
        ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->timeInterval = timeIntervalInMilliseconds;
        ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                                                            
        free(msg);
        return(result);

}

int resumeNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
int mynid, int registrationID, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine to resume a previously registered notification request that 
	is suspended. Upon receiving this command, the simulation engine uses 
	the provided node ID (mynid) and registration ID (registrationID) to 
	locate and resume the specified request record.

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
*/

char    *msg;
int     n, result;
                                                                                                                            
        if ((mynid <= 0) || (registrationID < 0)) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->type = AGENT_CLIENT_RESUME_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
        ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid = mynid;
        ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID = registrationID;
        ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientResumeNotificationWhenAnotherNodeHasApproachedMe));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                                                            
        free(msg);
        return(result);

}

int destroyMobileNode(int myTCPsockfd, int targetNid, int moreMsgFollowing) {
/*
        This API function forms an IPC command and sends it to the simulation 
	engine to tell it that the mobile node specified by targetNid
        no longer exists in the simulated network (because it may have 
        been captured or destroyed by the enemy force). Upon receiving this 
        command, the simulation engine sets the moving speed of the 
        specified mobile node to zero and turns off the "UP" flag in the 
        mobile node's control block to indicate this fact. When the 
        triggering system is invoked to detect whether any registered 
        circumstance has occured, a "DOWN" mobile node is not taken into 
        account in the checking.

        The moreMsgFollowing argument tells the simulation engine whether
        more IPC commands will follow this IPC command and that they all
        need to be processed atomically. That is, if moreMsgFollowing == 1,
        after processing this IPC command and sending back an ACK packet,
        the simulation engine should freeze its simulation clock and
        wait for more IPC commands to come. In contrast, if moreMsgFollowing
        == 0, the simulation engine can advance its simulation clock after
        processing this IPC command and process other events.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
 */

char    *msg;
int     n, result;
                                                                                
        if (targetNid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientDestroyMobileNode *) msg)->type =
          AGENT_CLIENT_DESTROY_MOBILE_NODE;
        ((struct agentClientDestroyMobileNode *) msg)->nid = targetNid;
        ((struct agentClientDestroyMobileNode *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct 
	  agentClientDestroyMobileNode));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
                                                                                
        free(msg);
        return(result);

}

int stopSimulation(int myTCPsockfd, int mynid) {
/*
        This API function forms an IPC command and sends it to
        the simulation engine to ask it to stop the whole simulation. 
        Upon receiving this command, the simulation engine stops the
        simulation in the same way as it receives a "STOP" command
        from the GUI. The simulation results are then transferred back
        to the GUI.
 */

char    *msg;
int     n;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientStopSimulation *) msg)->type =
          AGENT_CLIENT_STOP_SIMULATION;
        ((struct agentClientStopSimulation *) msg)->nid = mynid;
        n = writen(myTCPsockfd, msg, sizeof(struct agentClientStopSimulation));
                                                                                
        free(msg);
        return(0);

}

void usleepAndReleaseCPU(int myTCPsockfd, int mynid, int usecond, int pre_schedule_select) {
char    *msg;

/*
        This API function forms an IPC command and sends it to the simulation 
	engine to release the use of CPU. At the same time, it also puts
	the agent program to sleep for the amount of microseconds specified
	by usecond. These two operations are done atomically to avoid
	the simulation engine from advancing its simulation clock.
 */
                                                                                
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientReleaseCPU *) msg)->type =
          AGENT_CLIENT_RELEASE_CPU;
        ((struct agentClientReleaseCPU *) msg)->nid = mynid;
	if (syscall_NCTUNS_usleep_After_Msg_Send(usecond, myTCPsockfd, msg, sizeof(struct agentClientReleaseCPU),
		pre_schedule_select) < 0) {
		printf("System call syscall_NCTUNS_usleep_After_Msg_Send failed\n");                 
		exit(1);
        }
        free(msg);
}

int selectAndReleaseCPU(int myTCPsockfd, int mynid, int nfds, fd_set *inp, fd_set *outp, fd_set *exp, timeval *tvp) {

char    *msg;
int     n;

/*
        This API function forms an IPC command and sends it to the simulation
        engine to release the use of CPU. At the same time, it also puts
        the agent program to select on a few file descriptors for the amount 
	of time specified by timeout. These two operations are done atomically 
	to avoid the simulation engine from advancing its simulation clock.
 */

        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientReleaseCPU *) msg)->type =
          AGENT_CLIENT_RELEASE_CPU;
        ((struct agentClientReleaseCPU *) msg)->nid = mynid;
        if ((n = syscall(319, nfds, myTCPsockfd, sizeof(struct agentClientReleaseCPU), inp, tvp, msg, outp, exp)) < 0) {
          printf("syscall 319 failed !!!\n");
          exit(1);
        }
        free(msg);
	return(n);
}



int getApproachingNotificationInformation(int myTCPsockfd, int &anid, 
double &ax, double &ay, double &az, double &aspeed, double &aangle, 
double &distance) {

/*
        This API function reads an IPC command --
	SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME
	sent to the agent program by the simulation engine and returns an ACK
	to the simulation engine. The information of the node that has 
	approached this agent program is returned. anid is the ID of such a 
	node. (ax, ay, az) is the position of such a node. aspeed is the
	speed of such a node. aangle is the moving angle of such a node.
	distance is the distance between the agent program node and such a
	node.

        This function returns 0 if it executes successfully; otherwise,
        it returns -1, which indicates something is wrong.
	
*/

struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *msg;
int     n;

        msg = (struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) malloc(sizeof(char) * MAX_RECV_BYTES);
        n = readn(myTCPsockfd, (char *) msg,  sizeof(struct
          SimulationEngineNotificationWhenAnotherNodeHasApproachedMe));
        if (msg->type != SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME) {
/*
	  ack.type = GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE;
	  ack.result = -1;
	  ack.moreMsgFollowing = 0;
          n = writen(myTCPsockfd, (char *) &ack, sizeof(struct
            GeneralACKBetweenAgentClientAndSimulationEngine));
*/
	  return(-1);
	} 
	anid = msg->anid;
	ax = msg->x;
	ay = msg->y;
	az = msg->z;
	distance = msg->distance;
	aspeed = msg->speed;
	aangle = msg->angle;
/*
	ack.type = GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE;
	ack.result = 0;
	ack.moreMsgFollowing = 0;
        n = writen(myTCPsockfd, (char *) &ack, sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
*/
	return(0);
}

void constructRoadMapofTheWholeField(int IsAgentClient) {

	char *FILEPATH;
	FILE *fd = NULL;
	int filePathLength = 0;

        /* get the road structure file name and open it */
	filePathLength = strlen(GetWorkingDir()) + strlen(agentGetScriptName());

	FILEPATH = (char *) malloc(filePathLength + 17);

	sprintf(FILEPATH, "%s/%s%s", GetWorkingDir(), agentGetScriptName(), ".road_structure");

        if ((fd = fopen(FILEPATH, "r")) == NULL) {
                printf("Error: can't open road structure file %s\n", FILEPATH);
                exit(-1);
        }

	printf("Road structure path: %s\n", FILEPATH);

	read_RoadStructure(fd); // read road structure

	fclose(fd);
        free(FILEPATH);
}

int setCurrentSpeedAcceleration(int myTCPsockfd, int mynid, double 
speedAcceleration, int moreMsgFollowing) {

char    *msg;
int     n, result;
    
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetCurrentSpeedAcceleration *) msg)->type =
          AGENT_CLIENT_SET_CURRENT_SPEED_ACCELERATION;
        ((struct agentClientSetCurrentSpeedAcceleration *) msg)->nid = mynid;
        ((struct agentClientSetCurrentSpeedAcceleration *) msg)->acceleration = 
          speedAcceleration;
        ((struct agentClientSetCurrentSpeedAcceleration *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetCurrentSpeedAcceleration));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
    
        free(msg);
        return(result);
}


int setCurrentMovingDirectionViewedOnGUIScreen(int myTCPsockfd, int mynid,
double angle, int moreMsgFollowing) {

char    *msg;
int     n, result;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
        ((struct agentClientSetCurrentMovingDirection *) msg)->type =  
          AGENT_CLIENT_SET_CURRENT_MOVING_DIRECTION;
        ((struct agentClientSetCurrentMovingDirection *) msg)->nid = mynid;
        ((struct agentClientSetCurrentMovingDirection *) msg)->angle = angle; 
        ((struct agentClientSetCurrentMovingDirection *) msg)->moreMsgFollowing = moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientSetCurrentMovingDirection));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          GeneralACKBetweenAgentClientAndSimulationEngine));
        if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
          ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE) {
          printf("[%s]: IPC ACK failed.\n", __func__);
          exit(0);
        }
        result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;
        free(msg);
        return(result);
}

/* The following APIs are added by mshsu */

int getSignalGroupID(int myTCPsockfd, int mynid, int &sigGID, int moreMsgFollowing) {
	/*
  	 * This API will get signal's group ID;
	 * This function returns 0 if it executes successfully; otherwise,
	 * it returns -1, which indicates something is wrong.
 	 */

	char    *msg;
	int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientGetSignalGroupID *) msg)->type = 
	  AGENT_CLIENT_GET_SIGNAL_GROUPID;
        ((struct agentClientGetSignalGroupID *) msg)->nid = mynid;
        ((struct agentClientGetSignalGroupID *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetSignalGroupID));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetSignalGroupIDReply));
        if (((struct agentClientGetSignalGroupIDReply *) msg)
          ->type != AGENT_CLIENT_GET_SIGNAL_GROUPID_REPLY) {
          printf("IPC AGENT_CLIENT_GET_SIGNAL_GROUPID_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetSignalGroupIDReply *) msg)->result; 
	sigGID = ((struct agentClientGetSignalGroupIDReply *) msg)->sigGID;

	free(msg);
        return(result); 
}

/*
 * Parameters:
 *
 *    sigGID:       signal group ID
 *    signalIndex:  signal index in the specified signal group ID
 *    light:        traffic light to be set
 */
int setSignalLight(int myTCPsockfd, int mynid, int sigGID, int signalIndex, int light, int moreMsgFollowing){
	/*
	   This API can set signal's light.
	 */
	char *msg;
	int n, result;
	
	if(mynid <= 0) return (-1);
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientSetSignalLight *) msg)->type =
	  AGENT_CLIENT_SET_SIGNAL_LIGHT;
	((struct agentClientSetSignalLight *) msg)->nid = mynid;
	((struct agentClientSetSignalLight *) msg)->sigGID = sigGID;
	((struct agentClientSetSignalLight *) msg)->signalIndex = signalIndex;
	((struct agentClientSetSignalLight *) msg)->light = light;
	((struct agentClientSetSignalLight *) msg)->moreMsgFollowing = moreMsgFollowing;
	n = writen(myTCPsockfd, msg, sizeof(struct
	  agentClientSetSignalLight));
	n = readn(myTCPsockfd, msg,  sizeof(struct
	  GeneralACKBetweenAgentClientAndSimulationEngine));
	if (((struct GeneralACKBetweenAgentClientAndSimulationEngine*) msg)
	  ->type != GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE){
	  printf("[%s]: IPC ACK failed.\n", __func__);
	  exit(0);
	}
	result = ((struct GeneralACKBetweenAgentClientAndSimulationEngine *) msg)->result;

	free(msg);
	return(result);
	
}

int getSignalsInTheSameGroup(int myTCPsockfd, int mynid, agentClientGetSignalsInTheSameGroupReply *tempG, int sigGID, int moreMsgFollowing) {
/*
   This API can get signals which are in the same group.
 */
	char    *msg;
	int     n;

        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientGetSignalsInTheSameGroup*) msg)->type = 
	  AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP;
        ((struct agentClientGetSignalsInTheSameGroup*) msg)->nid = mynid;
        ((struct agentClientGetSignalsInTheSameGroup*) msg)->sigGID = sigGID;
        ((struct agentClientGetSignalsInTheSameGroup*) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetSignalsInTheSameGroup));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetSignalsInTheSameGroupReply));
        if (((struct agentClientGetSignalsInTheSameGroupReply*) msg)
          ->type != AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY) {
          printf("IPC  AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY failed.\n");
          exit(0);
        }
        tempG->type = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->type; 
        tempG->result = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->result;
	if(tempG->result == 0){
		printf("IPC  AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY failed.\n");
		return 1; // fail
	}
        tempG->numOfSigs = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->numOfSigs;
	tempG->sigGID = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->sigGID; 

	for(int i = 0; i < tempG->numOfSigs; ++i){ 
	        tempG->light[i] = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->light[i]; 
		tempG->x[i] = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->x[i];
		tempG->y[i] = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->y[i];
		tempG->facingDirection[i] = ((struct agentClientGetSignalsInTheSameGroupReply*) msg)->facingDirection[i];
	}
	free(msg);
        return 0; 
}

int getNearestTeamMemberPositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(int myTCPsockfd, int mynid,
double curx, double cury, double curz, double direction, double range, double tDistance, int tid, double &x,
double &y, double &z, double &tDirection, int &id, int moreMsgFollowing){

	struct nodePosition   *tempNPArray, *team_member;
	int    indx, tempN, result = -1, member_num;
        double tempAngle, distance;
        double tempDistance = tDistance;

	getCurrentPositionOfATeamOfNode(myTCPsockfd, mynid, tid, &team_member, member_num,1);
	tempNPArray = team_member;
        for(indx = 0; indx<member_num; indx++){
		printf("tempNPArray : (%f,%f)\n",tempNPArray->x,tempNPArray->y);
                if(tempNPArray->nid == (u_int32_t)mynid){
                        tempNPArray++;
                        continue;
                }
                tempAngle = atan2(tempNPArray->y - cury, tempNPArray->x - curx);
                tempAngle = 360-fmod(360+(tempAngle*180/PI), 360); /* GUI direction*/
                tempAngle = fmod(fabs(tempAngle - direction), 360);
                if(tempAngle > 180)
                        tempAngle = 360 - tempAngle;
                if(tempAngle < range){
                        distance = twoPointsDistance(curx, cury, tempNPArray->x, tempNPArray->y);
                        if(distance< tempDistance){
                                tempDistance = distance;
                                x = tempNPArray->x;
                                y = tempNPArray->y;
                                z = tempNPArray->z;
                                id = tempNPArray->nid;
                                tempN = getCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, id,tDirection, 1);
                                result = 0;
                        }
                }
                tempNPArray++;
        }
       free((char *)team_member);
	return (result);
}


int getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(int myTCPsockfd, int mynid, 
double curx, double cury, double curz, double direction, double range, double tDistance, int gid, double &x, 
double &y, double &z, double &tDirection, int &id, int moreMsgFollowing){
/*
	you can use this API to get the car position who is in the scope of range and 
	inside the distance of tDistance.
*/	
	struct nodePosition *tempNPArray, *tNPArray;
	int tempNumNodeInThisGroup, indx, tempN, result = -1;
	double tempAngle, distance;
	double tempDistance = tDistance;
	getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, gid, &tNPArray, tempNumNodeInThisGroup, 1);
	tempNPArray = tNPArray;
	for(indx = 0; indx<tempNumNodeInThisGroup; indx++){
		if(tempNPArray->nid == (u_int32_t)mynid){
			tempNPArray++;
			continue;
		}	
		tempAngle = atan2(tempNPArray->y - cury, tempNPArray->x - curx);
		tempAngle = 360-fmod(360+(tempAngle*180/PI), 360); /* GUI direction*/
		tempAngle = fmod(fabs(tempAngle - direction), 360);
		if(tempAngle > 180)
			tempAngle = 360 - tempAngle;
		if(tempAngle < range){
			distance = twoPointsDistance(curx, cury, tempNPArray->x, tempNPArray->y);
			if(distance < tempDistance){
				tempDistance = distance;
				x = tempNPArray->x;
				y = tempNPArray->y;
				z = tempNPArray->z;
				id = tempNPArray->nid;
				tempN = getCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, id, tDirection, 1);
				result = 0;
			}
		}
		tempNPArray++;
	}
	free((char *)tNPArray);
	/* 0 means find , -1 not find*/
	return (result);
}

int getNodeMaxSpeed(int myTCPsockfd, int mynid, double &maxSpeed, int moreMsgFollowing) {
/*
	This function returns 0 if it executes successfully; otherwise,
	it returns -1, which indicates something is wrong.
 */

	char    *msg;
	int     n, result;
                                                                                
        if (mynid <= 0) return(-1);
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientGetNodeMaxSpeed *) msg)->type = 
	  AGENT_CLIENT_GET_NODE_MAX_SPEED;
        ((struct agentClientGetNodeMaxSpeed *) msg)->nid = mynid;
        ((struct agentClientGetNodeMaxSpeed *) msg)->moreMsgFollowing =
          moreMsgFollowing;
        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetNodeMaxSpeed));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetNodeMaxSpeedReply));
        if (((struct agentClientGetNodeMaxSpeedReply *) msg)
          ->type != AGENT_CLIENT_GET_NODE_MAX_SPEED_REPLY) {
          printf("IPC AGENT_CLIENT_GET_NODE_MAX_SPEED_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetNodeMaxSpeedReply *) msg)->result; 
	maxSpeed = ((struct agentClientGetNodeMaxSpeedReply *) msg)->maxSpeed;

	free(msg);
        return(result); 
}

/*
 * Note:
 *    signal group ID is related to Node block ID.
 *    Therefore, we can get current node block ID in front of me by calling getFrontNID (in road.cc)
 *    and then use the node block ID to search the traffic light.
 *
 * Parameters:
 *    my_x, my_y:       My current position
 *    myDirection:      My current moving direction viewed on GUI screen
 *    distance:         My search range in meters
 *    sigGID:           signal group ID which need to search (one signal group controls one intersection)
 *    light:            return signal light which is facing me (the opposite direction of myDirection)
 *    TrafficLightPOS_x, TrafficLightPOS_y: Traffic light position
 *    signalIndex:      return the searched traffic light index
 *                      (There are 4 traffic light in one intersection (group) by default)
 */
int getTheNearestTrafficLightInfrontOfMe(int myTCPsockfd, int mynid, double my_x, double my_y, double myDirection, double distance, int sigGID, int &light, double &TrafficLightPOS_x, double &TrafficLightPOS_y, int &signalIndex,  int moreMsgFollowing)
{
	char    *msg;
	int     n, result;
                                                                                
        if ((mynid <= 0) || (sigGID < 0))
		return(-1);
	
        msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);

	((struct agentClientGetSignalLight *) msg)->type = AGENT_CLIENT_GET_SIGNAL_LIGHT;
        ((struct agentClientGetSignalLight *) msg)->nid = mynid;
        ((struct agentClientGetSignalLight *) msg)->moreMsgFollowing = moreMsgFollowing;
        ((struct agentClientGetSignalLight *) msg)->x = my_x;
        ((struct agentClientGetSignalLight *) msg)->y = my_y;
        ((struct agentClientGetSignalLight *) msg)->myDirection = myDirection;
        ((struct agentClientGetSignalLight *) msg)->distance = distance;
        ((struct agentClientGetSignalLight *) msg)->sigGID = sigGID;

        n = writen(myTCPsockfd, msg, sizeof(struct
          agentClientGetSignalLight));
        n = readn(myTCPsockfd, msg,  sizeof(struct
          agentClientGetSignalLightReply));
        if (((struct agentClientGetSignalLightReply *) msg)
          ->type != AGENT_CLIENT_GET_SIGNAL_LIGHT_REPLY) {
          printf("IPC AGENT_CLIENT_GET_SIGNAL_LIGHT_REPLY failed.\n");
          exit(0);
        }
        result = ((struct agentClientGetSignalLightReply *) msg)->result; 
	light = ((struct agentClientGetSignalLightReply *) msg)->light;
	TrafficLightPOS_x = ((struct agentClientGetSignalLightReply *) msg)->sig_x;
	TrafficLightPOS_y = ((struct agentClientGetSignalLightReply *) msg)->sig_y;
	signalIndex = ((struct agentClientGetSignalLightReply *) msg)->signalIndex;

	free(msg);
        return(result); 
}

