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
#include <string.h>
#include <stdlib.h>

#ifndef __NCTUNS_findport_h__
#define __NCTUNS_findport_h__
#define BUFFERSIZE 256
struct Node_element;

struct Point_of_list{
	struct List_head * first;
   	struct List_head * last;
   	int count;
};

struct List_head{
   	int nodeID;
   	struct List_head* next;
   	struct Node_element * first;
   	struct Node_element * last;
	int count;
};

struct Node_element{
   	int node;
   	struct Node_element * next;
   	int connect_port;
};

class Port_Construct{ 
   	private:
	  	Point_of_list * point_of_list;
   	public:
      		Port_Construct();
	  	void Read_file(char * filename);
		void Read_data(FILE * fptr);
		int Node_list_num(int nodeID);
	  	List_head * New_list(int nodeID);
		List_head *Get_head(int nodeID);
	  	void Add_element_to_ring(int node,int from,int to);
	  	void Print();//for test
		int Lookup_Uni_port(int from_node,int tonode);
};
#endif
