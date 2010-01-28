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

#ifndef __NCTUNS_opmanage_h__
#define __NCTUNS_opmanage_h__

#include <object.h>
#include <module/opt/Lheader.h>
#define BUFFERSIZE 256


struct chahe_for_Lookup_Protect_Port2{
	int	toport;
	int	fromport;
	int 	result;
	struct  chahe_for_Lookup_Protect_Port2 * next;
};

struct chahe_for_Lookup_New_Work_Port{
	int	start;
	int	end;
	int	port;
	int 	result;
	struct  chahe_for_Lookup_New_Work_Port* next;
};

struct cache_for_If_NextPort_In_CurrentRing{
	int	start;
	int 	end;
	int 	NextPort;
	int 	result;
	struct  cache_for_If_NextPort_In_CurrentRing * next;
};
	

struct Port_mapping_info{
   	u_int32_t work_port;
   	u_int32_t protect_port;
   	Port_mapping_info * next;
};


struct Point_to_ring{
	struct Ring_head * first;
   	struct Ring_head * last;
   	int count;
};

struct Ring_head{
   	int number;
   	struct Ring_element * current_node;
   	struct Ring_head* next;
   	struct Ring_element * first;
   	struct Ring_element * last;
};

struct Ring_element{
 	int	node;
   	struct 	Ring_element * next;
   	int 	port_from_pre;
   	int 	port_to_next;
};






/*class opmanage must include NSLobject
 * so if virtual function doesn't include anything
 * it will use the function of object.cc*/

class opmanage : public NslObject {

   private:

	int	   CACHE_ENABLE ;				   
	struct 	   chahe_for_Lookup_Protect_Port2 * cache_of_LPP2;
	struct 	   chahe_for_Lookup_New_Work_Port * cache_of_LNWP;
	struct 	   cache_for_If_NextPort_In_CurrentRing * cache_of_INIC;
	char       *RingPath;
	u_int32_t  pmode_currentnode;
 	u_int32_t  to_node; 
	u_int32_t  to_port;	
  	u_int32_t  to_wavelength;
  	u_int32_t  nodeID;
    
      	Port_mapping_info * head_of_list;
      	Port_mapping_info * last_of_list;
      	u_int32_t     list_count ;
          
          
       	Ring_head * ring_head;
	Point_to_ring * point_to_ring;
    	int		add_Psrc_Pdst; 
	int		link_mode[1024];
	int		channel_mode[1024][1024];
	int		workport[1024];
	int		spanport[1024];
	
  
   public:
    	opmanage(u_int32_t type ,u_int32_t id ,struct plist* pl , const char *name);/*constructor*/
    	~opmanage();/*disconstructor*/

   	void Read_file();
	int Read_data(FILE * fptr);
	void New_ring();
	void Add_ring();
	void Add_element_to_ring(u_int32_t from,u_int32_t node,u_int32_t to);
    
	void Print();//for test
	int Lookup_Protect_Port(int toport,int fromport);	
	int Lookup_Work_Port(u_int32_t protect);
	int Lookup_Protect_Port2(int toport,int fromport);
	int If_NextPort_In_CurrentRing(int start,int end,int NextPort);
	
	
	int Add_Spaninfo_from_file();
	int Lookup_Span_Port(char  toport,char fromport);	  
  	//  void update_ring_condition();
 	//   int  decide_WorkOrProtection();
	int find_fromport_workmode(int port);
	int Find_the_default_dst(int port);
	int Lookup_New_Work_Port(int start,int end,int port);
	
	int init();
 	int get(ePacket_ *pkt, MBinder *frm);
  
	int recv(ePacket_ *pkt);
    	int send(ePacket_ *pkt);
};

#endif
