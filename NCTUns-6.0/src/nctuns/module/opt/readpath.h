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
#include <stdlib.h>
#include <string.h>
#include "Findport.h"

#define BUFFSIZE 1024

#define Max_Int 10000
#define MaxSum(a,b)  (((a) != Max_Int && (b) != Max_Int) ? ((a)+(b)) :Max_Int)
#define SWAP(a,b) {temp = a; a = b; b = temp;}

const int Max_NODE = 100;         //adjust accroding to Max need
const int Max_Wavelength = 100;	  //adjust accroding to Max need


//define struct of stack
struct stack{
	int data1;
	int data2;  
       	struct stack *link;
};


struct cache_for_SW_get_nextport{
	int nodeID;
	int fromport;
	int wave;
	int result;
	struct cache_for_SW_get_nextport * link;	
};


//define structure of node
struct node_struct{
   	int vertex;                   			//the number of node
	int distance;                			//only two value , 1 , Max_Int
	int bandwidth[Max_Wavelength];                	//Max wavelength    
	struct node_struct *link;
};

struct node_point{
	struct node_struct* link;
};



/* This struct records information about node
*/
struct Node{
	int from_port;
	int NodeID;
	int to_port;
	int wave;
	Node *next;
};

/* This struct link all node in one path
 */
struct Pathlist{
	Node *Start_Node;
	Node *End_Node;
	int path_length;
	int wave;
	Pathlist *next;
};

//define strcut of subnet
//This struct is not used now
struct subnet_node{
	int vertex;
	subnet_node *link;
};



/* This struct connect all node together
 * But this is sort by nodeID
 * one nodeID has one nodelist
 */
struct HeadNode{  
	int NodeID;			//all node equal NodeID will be attached in nodelist
	Node *nodelist;
	HeadNode *next;		
};


class ReadPath {
private:
	int		CACHE_ENABLE;
	// below two for ring 
	char 		*ConfigureFileDir;
	Pathlist	*temp_pathlist;	//this is only a temp pointer used to help reading data to construct HeadList
	Node		*temp_node_ptr;	// this is  a temp pointer to help building pathlist
	Pathlist	*HeadList_head;
	
	Port_Construct	findport;
	HeadNode    	*headnode;
	node_struct 	node;
	node_point 	*graph[Max_NODE];
	stack* 		top; 		// point to the top of stack
	
	int		Having_Ring;
	int 		count_node;
	int		readfinish;	
	int		Anti_Ring_Info[Max_NODE][Max_NODE];
	int		Ring_Info[Max_NODE][Max_NODE];
	
	int dfn[Max_NODE];
	int low[Max_NODE];
	int SetPath[Max_NODE][Max_NODE];
	int Dist[Max_NODE][Max_NODE];				//distance array
	int Path[Max_NODE][Max_NODE];				//distance array
	int Router[Max_NODE] ;

	//below for cache	
	struct cache_for_SW_get_nextport * head_of_cacahe_for_SW_get_nextport;	
	
public:
	ReadPath(){
		temp_pathlist = NULL;
		HeadList_head = NULL;
		headnode = NULL;
		temp_node_ptr = NULL;
		for (int i =0 ; i <Max_NODE ; i ++){
			graph[i] = NULL;
			for (int j =0 ; j <Max_NODE ; j++){
				Anti_Ring_Info[i][j] = 0;
				Ring_Info[i][j] = 0;
				SetPath[i][j] = 0;
			}
		}
		count_node =0;
		readfinish = 0;
		head_of_cacahe_for_SW_get_nextport = NULL;

		//if we want to make cache enable, we set variable CACHE_ENABLE =1
		//otherwise,we set this variable 0
		CACHE_ENABLE = 0; 
	}
	
	void ReadFile();
	void StoreFirst(int nodeID , int port,int wave);
   	void StoreNode(int fromP ,int nodeID, int toP,int wave);
	void StoreEnd(int port, int nodeID,int wave,int distance);
	void StoreWave(int wave); 
	void CollectNode(int nodeID,Node *node);
	int  SW_get_nextport(int nodeID,int fromport ,int wave);
	int  RWA_get_wave(int nodeID, int destID);
	int  RWA_get_outputport(int nodeID, int destID);
	int  GetPath_Length(int nodeID,int destID);
	int  min(int first ,int second);
	void push_Stack(stack* *atop ,int data1 , int data2 );
	void pop_Stack(stack* *atop, int * data1, int* data2);
	void reverse(int* x,int n);
	void Append_Node(node_point *insert_ptr,int vertex,int band);
	void Insert_Node(node_struct *insert_ptr,int vertex,int band);
	void floyd();
	void mark_through(int from ,int end,int wavelength);
	int  display_path(int i,int j,int wavelength,FILE *foutput);
	void ReadinDistArray(int wavelength);
	void construct_node(int start,int end,int bandwidth);
	void Input(char * filename);
	void ReadRouter(char * filename);
	int  InRouter(int node);
	void init();
	int  Find_shortestpath(char *nodeconnectfile,char * path_file);
	void ReadSetShortestPath(char * filename,FILE* ptr);
	void Start(char * node_kind , char * node_connect,char * ring_file ,char * path_file,char *configure_file_dir);
	void Start(){};
	void GetRingInformation(char *ringfile );
	
};
