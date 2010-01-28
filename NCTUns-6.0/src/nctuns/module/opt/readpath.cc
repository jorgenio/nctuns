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
#include "readpath.h"

ReadPath * readpath_  = new ReadPath;

/* This function is called by constructor 
 * its goal is to determine if this class is called first 
 * if this class is called frist ,the value of readfinish  = 0*/

void ReadPath:: Start(char *node_type ,char *node_connect,char *ring_file,char* path_file, char *configure_filedir){
	if (readfinish == 0){
		readfinish = 1;
		ConfigureFileDir =(char *) malloc ((strlen(configure_filedir)+strlen("output.txt"))+1);
		sprintf(ConfigureFileDir,"%s%s",configure_filedir,"output.txt");
		GetRingInformation(ring_file);
		ReadRouter(node_type);
    		Input(node_connect);
		Find_shortestpath(node_connect,path_file);
		ReadFile();
	}
}

/* If we want to find shortest path based on virtual ring , we have to get information of ring , because shortest path is based on
 * virtual ring 
 * The variable of Having_Ring is used to check if topology has ring or not */

void  ReadPath::GetRingInformation(char *ringfile){
	FILE	*inputfile;
	inputfile = fopen(ringfile,"r+");
	int temp_node = 0;
	int start_node = 0;
	int end_node = 0;
	
	if (inputfile == NULL){
		printf("WARN!!!! Read ring error: can\'t open file %s \n",ringfile);
	}
	else{
		int	new_ring_line = 1;
		int	last_ring_element_start=0;
		int	last_ring_element_end=0;

		while(!feof(inputfile)){
			char buffer[BUFFERSIZE];
		        char delim[] = " \0\n";
       			char *tok;
	      	        char temp[BUFFERSIZE] = "";
	        	int token[3] = {0};
      		        int ptr = 0;

     	 	        if (fgets(buffer,BUFFERSIZE,inputfile) > 0){
        	      		  buffer[strlen(buffer)] = '\0';
  	    	        }
	                else
        	                  continue;

 		       	tok = strtok(buffer,delim);
	        	sprintf(temp,"%s",tok);
        		while(tok != NULL){
                		token[ptr] = atoi(tok);
	               		ptr = (ptr + 1) % 3;
        	        	if (ptr == 0){
					Having_Ring = 1;
					temp_node = token[1];

					if (new_ring_line == 1){
						last_ring_element_end = temp_node;
						new_ring_line = 0;
						printf("start ring %d \n",temp_node);
					}
					else{
						printf("end ring %d \n",temp_node);
						last_ring_element_start = temp_node;
					}

					if (start_node == 0)
						start_node = temp_node;
					else if (start_node > 0){
						if (end_node == 0){
							end_node = temp_node;
							Anti_Ring_Info[end_node][start_node] ++;
							Ring_Info[start_node][end_node] ++;
							start_node = end_node;
							end_node = 0;			
						}
						else
							printf("Warning !!! In ReadPath , get ring file error \n");
					}	
                		}
                		tok = strtok(NULL,delim);
        		}
			new_ring_line = 1;
			Anti_Ring_Info[last_ring_element_end][last_ring_element_start] ++;
			Ring_Info[last_ring_element_start][last_ring_element_end] ++;
		}//while
		fclose(inputfile);
	}//else
}

/* This function is used to know which node is router , because shortest path is between each router node */

void ReadPath::ReadRouter(char * FILENAME){
	FILE * file = fopen(FILENAME,"r");
	int	nodeID;
	char    type[40];
	const char *  router = "ROUTER";
	
	if (file == NULL){
		printf("Warning : Can't read file %s \n",FILENAME);
	}
	else{
		int  router_num =0;
		
		fscanf(file,"%d ",&count_node);
		count_node++;
		for (int i=1 ;i < count_node;i++){
			fscanf(file , "%d %s",&nodeID,type);
			if (strcmp(type,router) == 0)
				Router[router_num++] = nodeID;
		}
		fclose(file);
	}
}


/* Read information of topology 
 * */

void ReadPath::Input(char * filename){
	int  start_v = 0;
	int end_v = 0;
	int temp1,temp2;
	FILE *finput;
	finput = fopen(filename,"r+");
	

	if (finput == NULL){
		printf("Readpath get %s error: can\'t open file \n",filename);
	}
	else{
	   	fscanf(finput,"%*d");
	   	while (!feof(finput)){
          		fscanf(finput,"%d %d %d %d \n",&start_v,&end_v,&temp1 ,&temp2); 
			construct_node(end_v,start_v,1);
	      		construct_node(start_v,end_v,1);
	   	}
		fclose(finput);
	}
}



void ReadPath::construct_node(int start,int end,int bandwidth){
	node_struct* ptr;
	node_struct* insert_ptr;
	int vertex_num;

	insert_ptr = NULL; // initial 

	if (graph[start] == NULL){
		graph[start] = new node_point;
		Append_Node(graph[start],end,bandwidth);
	}
	else{
		for(ptr = graph[start]->link ;ptr;ptr = ptr->link){
			vertex_num = ptr->vertex;
		   	if (vertex_num <= end ){
		       		insert_ptr = ptr;
		   	}
		}
	    if (!insert_ptr)
			insert_ptr = graph[start]->link ;
	    if (insert_ptr->vertex != end)//avoide insert the same
	       	Insert_Node(insert_ptr,end,bandwidth);	
	}
}

/* This function is used to connect  all node to one link list ,the head of list is "headnode" 
 * Search according to headnode,we can get any node we want
 * */

void ReadPath::CollectNode(int nodeID, Node *_node){
	HeadNode *headnode_ptr;
	Node *node_ptr ;
	Node *node = (struct Node*)malloc(sizeof(struct Node));
	node->from_port  = _node->from_port ;
	node->NodeID  = _node->NodeID;
	node->to_port  =_node->to_port ;
	node->wave = _node->wave ;
	node->next = NULL;
	int FindHead = 0;
	
	for(headnode_ptr = headnode ; headnode_ptr;headnode_ptr = headnode_ptr->next ){
		if (headnode_ptr->NodeID == nodeID){
			FindHead = 1;
			break;
		}
		if (headnode_ptr->next  == NULL)
			break;
	}
	
	if (!FindHead){
		if (headnode_ptr != NULL){
			headnode_ptr->next  = (struct HeadNode*)malloc(sizeof(struct HeadNode));
			headnode_ptr = headnode_ptr->next;
		}
		else
			headnode_ptr = (struct HeadNode*)malloc(sizeof(struct HeadNode));
		headnode_ptr->NodeID = nodeID;
		headnode_ptr->next = NULL;
		headnode_ptr->nodelist = node;
		if (headnode == NULL)
			headnode = headnode_ptr;
	}
	else{
		for (node_ptr= headnode_ptr->nodelist ;node_ptr ; node_ptr= node_ptr->next ){
			if (node_ptr->next == NULL){
				node_ptr->next = node;
				break;
			}
		}
	}
} 

/* Below three functions will collect all node in the same path
 * HeadList_head is the head of all pathlist
 * if we want get all node in one path ,we can search from "Start_Node"*/

void ReadPath::StoreFirst(int nodeID,int port ,int wave){
	temp_node_ptr = (struct Node *)malloc(sizeof(struct Node));
	temp_node_ptr->NodeID = nodeID;
	temp_node_ptr->from_port = -1;
	temp_node_ptr->to_port = port;
	temp_node_ptr->wave = wave;
	if (temp_pathlist == NULL){
		temp_pathlist = (struct Pathlist *)malloc(sizeof(struct Pathlist));
		HeadList_head = temp_pathlist;
		temp_pathlist->next = NULL;
	}
	else{
		temp_pathlist->next = (struct Pathlist *)malloc(sizeof(struct Pathlist));
		temp_pathlist = temp_pathlist->next ;
	}
	temp_pathlist->Start_Node = temp_node_ptr;
	CollectNode(nodeID, temp_node_ptr);
} 

void ReadPath::StoreNode(int fromP,int nodeID ,int toP,int wave){
	temp_node_ptr->next = (struct Node *)malloc(sizeof(struct Node));
	temp_node_ptr = temp_node_ptr->next ;
	temp_node_ptr->from_port = fromP;
	temp_node_ptr->NodeID = nodeID;
	temp_node_ptr->to_port = toP;
  	temp_node_ptr->wave = wave;
	CollectNode(nodeID, temp_node_ptr);
}

void ReadPath::StoreEnd(int port, int nodeID,int wave ,int distance){
	temp_node_ptr->next = (struct Node *)malloc(sizeof(struct Node));
	temp_node_ptr = temp_node_ptr->next ;
	temp_node_ptr->from_port  = port;
	temp_node_ptr->NodeID = nodeID;
	temp_node_ptr->to_port = -1;
	temp_node_ptr->next = NULL;
	temp_node_ptr->wave = wave;
	temp_pathlist->End_Node = temp_node_ptr;
	temp_pathlist->path_length = distance;
	CollectNode(nodeID, temp_node_ptr);
}


void ReadPath::StoreWave(int wave){
	temp_pathlist->wave = wave;
}
  

void ReadPath::ReadFile(){
	FILE *finput;
	char temp[BUFFSIZE];
	int start_v;
	int end_v;
	int start_p;
	int nextend_p;
	int end_p;
	int distance;
	int wave;
	int i ;
	
	
	finput = fopen(ConfigureFileDir,"r");

	if (finput == NULL){
		printf("read path error: can\'t open file %s\n",ConfigureFileDir);
	}
	else{
	   	while (!feof(finput)){
			temp[0] = '\0';
			fgets(temp,BUFFSIZE,finput);	
			if (strstr(temp,"Unreach") == NULL){
          			sscanf(temp,"%d -> %d distance %d :: ",&start_v,&end_v,&distance); 
				fscanf(finput,"%*s %d ",&wave);
				for(i = 0 ; i < distance;i++){
					fscanf(finput,"%d (port : %d , port : %d) -> ",&start_v,&start_p,&nextend_p);
					if (i == 0)	{
						StoreFirst(start_v,start_p,wave);
						end_p = nextend_p;
					}
					else{
						StoreNode(end_p,start_v,start_p,wave);
						end_p = nextend_p;
					}
				}
				fscanf(finput,"%d%*c",&end_v); 
				StoreEnd(end_p,end_v,wave,distance);
				StoreWave(wave);
			}				
	   	}
		fclose(finput);
	}
}

/*This function is provided for switch to get mapping*/

int ReadPath::SW_get_nextport(int nodeID,int fromport ,int wave){
	HeadNode	    	*head;
	Node			*tempnode;	
	
	struct cache_for_SW_get_nextport * cache = head_of_cacahe_for_SW_get_nextport;
	int			in_cache = 0;
	//check cache
	
	if (CACHE_ENABLE){
		for (cache = head_of_cacahe_for_SW_get_nextport; cache ; cache = cache ->link){
			if ((cache->nodeID == nodeID) &&(cache->fromport == fromport)&&(cache->wave == wave)){
				in_cache = 1;
				break;
			}	
		}
	}
	
	for(head = headnode;head; head = head->next ){
		if (head->NodeID == nodeID){
			tempnode =head->nodelist; 
			while (tempnode){
				if ((tempnode->from_port  == fromport)&& (tempnode->wave  == wave)){
					if ((CACHE_ENABLE)&&(in_cache == 0)){
						struct cache_for_SW_get_nextport * new_cache = new cache_for_SW_get_nextport;
						new_cache->nodeID = nodeID;
						new_cache->fromport = fromport;
						new_cache->wave = wave;
						new_cache->link = NULL;
						new_cache->result = tempnode->to_port;
						if (cache)
							cache->link = new_cache;
						else
							head_of_cacahe_for_SW_get_nextport = new_cache;
						  	
					}
					return tempnode->to_port;
				}
				tempnode =tempnode->next;
			}
		}
	}	
	return -1;
} 

int ReadPath::RWA_get_wave(int nodeID, int destID){
	Pathlist	*templist;
	Node		*firstnode;

	for(templist = HeadList_head ;templist ;templist =templist->next ){
		firstnode =  templist->Start_Node ;
		if (templist->Start_Node->NodeID   == nodeID && templist->End_Node->NodeID  == destID){
				return firstnode->wave  ; 
		}
	}	
	return -1;
}

int ReadPath::RWA_get_outputport(int nodeID, int destID){
	Pathlist	*templist;
	Node		*firstnode;

	for(templist = HeadList_head ;templist ;templist =templist->next ){
		firstnode =  templist->Start_Node ;
		if (templist->Start_Node->NodeID   == nodeID && templist->End_Node->NodeID  == destID){
				return firstnode->to_port  ; 
		}
	}	
	return -1;
}

int ReadPath::GetPath_Length(int nodeID, int destID){
	Pathlist	*templist;
	Node		*firstnode;

	for(templist = HeadList_head ;templist ;templist =templist->next ){
		firstnode =  templist->Start_Node ;
		if (templist->Start_Node->NodeID   == nodeID && templist->End_Node->NodeID  == destID){
				return templist->path_length ; 
		}
	}	
	return -1;
}

int ReadPath::min(int first ,int second){
   	return  first < second ? first : second;
}

void ReadPath::push_Stack(stack* *atop ,int data1 , int data2 ){
	stack* temp = new stack;
	temp->data1  = data1;
	temp->data2 = data2;
	temp->link = *atop;
	*atop = temp;
}

void ReadPath::pop_Stack(stack* *atop, int * data1, int* data2){
        stack* temp = *atop;
	*data1 = temp->data1;
	*data2 = temp->data2;
	*atop = (*atop)->link ;
	delete temp;
}

void ReadPath::reverse(int* x,int n){
   	int i,j,temp;

   	for (i=0,j= n-1;i < j;i++,j--)
		SWAP(x[i],x[j]);
}

void ReadPath::Append_Node(node_point *insert_ptr,int vertex,int band){
	node_struct *new_node = new node_struct;	
	new_node->vertex = vertex;
	for (int i = 0 ; i < Max_Wavelength ; i ++)
		new_node->bandwidth[i] = 0;
	new_node->distance  = 1;					// for default
	new_node->link = NULL;
	insert_ptr ->link  = new_node;
}

void ReadPath::Insert_Node(node_struct *insert_ptr,int vertex,int band){
	if (insert_ptr->link == NULL){
		node_struct * new_node = new node_struct;
		new_node->vertex = vertex;
		for (int i = 0 ; i < Max_Wavelength ; i ++)
			new_node->bandwidth[i] = 0;
		new_node->distance  = 1 ;              //for default
		new_node->link = NULL;
		insert_ptr->link = new_node;
	}
	else{
		node_struct * temp_ptr;
		temp_ptr = insert_ptr->link;
		node_struct *new_node = new node_struct;
		new_node->vertex = vertex;
		for (int i = 0 ; i < Max_Wavelength ; i ++)
			new_node->bandwidth[i] = 0;
		new_node->distance = 1;                // for default
		new_node->link = temp_ptr;
		insert_ptr->link = new_node;
	}
}

void ReadPath::floyd(){
	int i,j,k;
   	for (i= 0 ; i < count_node ;i++)
		for (j = 0 ; j < count_node;j++)
           		Path[i][j] = i;
   	for (k = 0; k < count_node ; k++)
		for (i= 0 ; i < count_node ;i++)
	        	for (j = 0 ; j < count_node;j++)
				if (Dist[i][j] > MaxSum(Dist[i][k],Dist[k][j])){
			       		Path[i][j] = Path[k][j];
				   	Dist[i][j] = MaxSum(Dist[i][k],Dist[k][j]);
			   	}
}

void ReadPath::mark_through(int from ,int end,int wavelength){
        node_struct * temp_ptr;
	if (graph[from] != NULL)
		for(temp_ptr = graph[from]->link  ; temp_ptr ; temp_ptr = temp_ptr->link ){
			if (temp_ptr->vertex  == end)
				temp_ptr->bandwidth[wavelength] = 1;
		}
}

int ReadPath::display_path(int i,int j,int wavelength,FILE *foutput){
	int k,count ;
   	int *chain;//[Max_NODE];

   	chain = (int*) malloc(sizeof(int)*count_node);
	if (i != j){//diagonal
		if (Dist[i][j] == Max_Int){
			return (0);
		}
		else{
			fprintf(foutput,"distance %d :: \n",Dist[i][j]);
			fprintf(foutput,"wavelength %d\n",wavelength);
			count = 0;
			k = j;
			do {
				k = chain[count++] = Path[i][k];
			}while(i != k);
			reverse(chain,count);
			fprintf(foutput,"%d ",chain[0] );
				
			for ( k = 1 ; k < count ; k++){
				
				if (k != 1){ 
					int m  = chain[k-1];
					int n = chain[k];
					mark_through(m,n,wavelength);
					fprintf(foutput,"(port : %d" ,findport.Lookup_Uni_port(n,m)); 
					fprintf(foutput,",port : %d)" ,findport.Lookup_Uni_port(m,n)); 
				}
				else{
					mark_through(chain[0],chain[1],wavelength);
					fprintf(foutput,"(port : %d" ,findport.Lookup_Uni_port(chain[1],chain[0])); 
					fprintf(foutput,",port : %d)" ,findport.Lookup_Uni_port(chain[0],chain[1])); 
				}
				fprintf(foutput,"-> %d ",chain[k] ); 
			}
			fprintf(foutput,"(port : %d " ,findport.Lookup_Uni_port(j,chain[count-1])); 
			fprintf(foutput,",port : %d) " ,findport.Lookup_Uni_port(chain[count-1],j));
			fprintf(foutput,"-> %d\n",j );
			mark_through(chain[count-1],j,wavelength);
			return(1);
		}
	}
	return 0;
}

void  ReadPath::ReadinDistArray(int wavelength){ 
	int i,j ; 
   	for (i=1 ; i < count_node;i++){  // initial
	        for (j =0 ; j < count_node;j++)
			Dist[i][j] = Max_Int;
		Dist[i][i]= 0;
   	}
	
   	node_struct* nodeptr;
   	for (int m = 1; m <  count_node ; m ++){
		for (int n = 1 ; n <  count_node; n ++){
			if (graph[m] != NULL){
		        	for(nodeptr = graph[m]->link ;nodeptr;nodeptr = nodeptr->link ){
					if (n == nodeptr->vertex ){
                			        if (nodeptr->bandwidth[wavelength] ==1)
							Dist[m][n] = Max_Int;
						else
				    			Dist[m][n] = nodeptr->distance ;
						
						/*Below code adjusts probiem in the topology having Ring */
						
						if (Having_Ring == 1){
		                			if ((Anti_Ring_Info[m][n] == 1)&&(Ring_Info[m][n] <= 0)){
								Dist[m][n] = Max_Int;
							}
						}
                                        } 
				}
                        }
		}
   	}//for
}


int ReadPath::InRouter(int node){
	for(int i = 0 ; i < Max_NODE ; i++){
		if(Router[i] == node)
			return 1;
	}
	return 0;	
}

void ReadPath::init(){
	int i ;
	top = new stack; //assign memory
	top = NULL;
	for (i = 0; i < 100 ; i++){
		dfn[i] = low[i] = -1;
	}
}

void ReadPath::ReadSetShortestPath(char *path_file,FILE *foutput){

   	FILE * fileptr;
	int start_node;
	int end_node;	
	int shortestpath[BUFFSIZE];
   	fileptr = fopen(path_file,"r+");
	if (fileptr == NULL){
		printf("Wrong !! In ReadPath open file to read error!! can't open %s!!\n ",path_file);
	}
	else{	
	   	while (!feof(fileptr)){
			int	total_input = 0;
			char buffer[BUFFERSIZE];
        		char delim[] = " \0\n";
	        	char *tok;
		        char temp[BUFFERSIZE] = "";
			
			if (fgets(buffer,BUFFERSIZE,fileptr) > 0){
       				buffer[strlen(buffer)] = '\0';
			}
			else 
				break;
		        tok = strtok(buffer,delim);
			sprintf(temp,"%s",tok);
			while(tok != NULL){
		        	shortestpath[total_input++] = atoi(tok);
                		tok = strtok(NULL,delim);
        		}
			
			int find = 0;
			int illegal =0 ;
			int length =0;
			for (int k = 1 ;k < Max_Wavelength; k++){
				int	traverse_index = 0;
				if (find == 0){
					while (traverse_index < (total_input-1)){
						length ++;
						start_node = shortestpath[traverse_index];
						traverse_index = traverse_index +3;
						end_node = shortestpath[traverse_index];
					
						if (graph[start_node] != NULL){
   							node_struct* nodeptr;
		        				for(nodeptr = graph[start_node]->link ;nodeptr;nodeptr = nodeptr->link ){
								if ((end_node == nodeptr->vertex )&& (nodeptr->bandwidth[k] ==1)){
									     illegal = 1;
									     break;
								}
							}
						}//if 
					}//while
					if (illegal == 0){
						find = 1;	
						SetPath[shortestpath[0]][shortestpath[total_input-1]] = 1;
						fprintf(foutput,"%d -> %d " , shortestpath[0] ,shortestpath[total_input-1] );
					 	fprintf(foutput,"distance %d :: \n",length);
				  		fprintf(foutput,"wavelength %d\n",k);
						int m =0 ;
						while( m < (total_input-1)){
							if (m ==0)
								fprintf(foutput,"%d " ,shortestpath[m]);
							else	
								fprintf(foutput,"-> %d " ,shortestpath[m]); 
							fprintf(foutput,"(port : %d" ,shortestpath[m+1]); 
							fprintf(foutput,",port : %d)" ,shortestpath[m+2]); 
							mark_through(m,m+2,k);
							m = m + 3;
						
						}
						fprintf(foutput,"-> %d\n",shortestpath[total_input-1] );
						break;
					}//if
				}//if				
			}//for
		}//while
		fclose(fileptr);	
	}//else 

}
int ReadPath::Find_shortestpath(char *nodeconnectfile,char * path_file){
	int i,j,k = 0;
	int find ;
   
	init();
   	FILE * ffoutput ;
   	ffoutput = fopen(ConfigureFileDir,"w+");
	if (ffoutput == NULL){
		printf("Wrong !! In ReadPath open file to write error!! can't open %s!!\n ",ConfigureFileDir);
	}
	else{
		ReadSetShortestPath(path_file,ffoutput);
		findport.Read_file(nodeconnectfile);
		for ( i = 1 ; i < count_node; i++ ){
			if (InRouter(i)){
				for ( j = 1 ; j < count_node; j++ ){
					if (InRouter(j) && (SetPath[i][j] != 1)){
						find = 0;
						fprintf(ffoutput,"%d -> %d " , i , j);
						for (k = 1 ;k < Max_Wavelength; k++){
							ReadinDistArray(k);
							floyd();
							if (display_path(i,j,k,ffoutput)){
								find = 1;
								break;
							}
						}
						if (find == 0)
							fprintf(ffoutput,"Unreach \n");
					}
				}
			}
    		}
   		for (i=1 ; i < count_node;i++){ // return to 0
       			for (j =1 ; j < count_node;j++)
				Dist[i][j] = Max_Int;
	   		Dist[i][i]= 0;
   		}
		fclose(ffoutput);
	}
    	return 0 ;
}

