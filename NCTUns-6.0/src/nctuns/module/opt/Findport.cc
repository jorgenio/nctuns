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

#include "Findport.h"

Port_Construct::Port_Construct(){             // intinal 
   	point_of_list = new Point_of_list;
  	point_of_list->first = NULL;
   	point_of_list->last = NULL;
   	point_of_list->count = 0;
   	//Read_file();
}
void Port_Construct::Read_file(char * filename){
	FILE *fptr;
    	fptr = fopen(filename,"r");
	if (fptr == NULL){
		printf("port mapping error: can\'t open file \n");
	}
	else{
	 	int temp;
	    fscanf(fptr, "%d",&temp); 
	   	while (! feof(fptr)){
	      	Read_data(fptr);
	   	}
	   	fclose(fptr);
    }
}
 
int Port_Construct::Node_list_num(int nodeID){
	List_head *ptr;
	if (point_of_list == NULL)
		return -1;
	for(ptr = point_of_list->first;ptr;ptr= ptr->next ){
		if(ptr->nodeID == nodeID)
			return ptr->count;
	}
	return 0;
}

List_head *Port_Construct::Get_head(int nodeID){
	List_head *ptr;
	if (point_of_list == NULL)
		return NULL;
	for(ptr = point_of_list->first;ptr;ptr= ptr->next ){
		if(ptr->nodeID == nodeID)
			return ptr;
	}
	return (New_list(nodeID));
}
void Port_Construct::Read_data(FILE *fptr){
	int from_node,to_node;
	int from_port,to_port;
	fscanf(fptr,"%d %d %d %d \n",&from_node,&to_node,&from_port,&to_port);
	Add_element_to_ring(from_node,to_node,to_port);
	Add_element_to_ring(to_node,from_node,from_port);
} 

List_head * Port_Construct::New_list(int nodeID){
   	List_head *list_head = new List_head;
	list_head->nodeID = nodeID ;
	list_head->first = NULL;
	list_head->last = NULL;
	list_head->next = NULL;
        list_head->count = 0;

	if (point_of_list->first == NULL)
			point_of_list->first = list_head;
		else
		    	point_of_list->last ->next  = list_head;
	point_of_list->last = list_head;
	point_of_list->count ++;
	return list_head;
} 

void Port_Construct::Add_element_to_ring(int fromnode ,int tonode, int port){
	List_head *temp_head;
   	Node_element * node_element = new Node_element;
   	node_element->next = NULL;
  	node_element->node = tonode;
   	node_element->connect_port = port;
	temp_head = Get_head(fromnode);
   	if (temp_head->first == NULL)
      	temp_head->first = node_element;
   	else
		temp_head->last->next  =  node_element;   
        temp_head->last = node_element;
	temp_head->count ++;
} 

int Port_Construct::Lookup_Uni_port(int from_node,int tonode){
	List_head * head_ptr = NULL;
	Node_element * node_ptr = NULL;
	if (point_of_list== NULL)
		return -1;
	for(head_ptr = point_of_list->first ; head_ptr;head_ptr = head_ptr->next){
        	if (head_ptr->nodeID == from_node){
        		node_ptr = head_ptr->first; 
        		break;
        	}
    }
    if (node_ptr == NULL)
        return -1;
    for(;node_ptr;node_ptr = node_ptr->next){
        if (node_ptr->node == tonode){
        	return	node_ptr->connect_port ;
        }
    }
    return -1;
}

/*
void opmanage::Print(){
	Ring_head * temp1 = point_to_ring->first ;
	Ring_element * temp2;
	while( temp1 != NULL){
	   	printf("New Ring \n");
       		temp2 = temp1->first ;
	   	while (temp2 != NULL){
	      		printf("%d %d %d \n",temp2->node ,temp2->port_from_pre ,temp2->port_to_next );
		  	temp2= temp2->next ;
       		} 
	  	 temp1 = temp1->next ;
	}	
}

void main(){
   	Port_Construct a ;
   	a.Read_file() ;
	for(int i =0 ; i < 5 ; i ++)
		for(int j =0 ; j < 5 ; j ++)
   		printf("%d %d %d \n",i,j,a.Lookup_Uni_port(i,j)  ) ;
}*/
