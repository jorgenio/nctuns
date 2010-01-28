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

#include <assert.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include <stdlib.h>
#include <stdio.h>
#include <gbind.h>
#include <event.h>
#include <gbind.h>
#include "opmanage.h"
#include <module/opt/readpath.h>
#include <packet.h>
#include <mbinder.h>

//#include <dmalloc.h>
extern RegTable                 RegTable_;
extern typeTable		*typeTable_;
extern scheduler                *scheduler_;

//extern ReadPath                 *readpath_;


MODULE_GENERATOR(opmanage);
opmanage::opmanage(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	vBind("RingPath",&RingPath);
	
        s_flowctl = DISABLED;
        r_flowctl = DISABLED;
        head_of_list = NULL; 
	last_of_list = NULL;
        list_count = 0; 
        nodeID = get_nid();
        point_to_ring = new Point_to_ring;
        point_to_ring->first = NULL;
        point_to_ring->last = NULL;
        point_to_ring->count = 0;
        ring_head = NULL;
	cache_of_LPP2 = NULL;
	cache_of_LNWP = NULL;
	cache_of_INIC = NULL;
	
	//if we want to let cache ENABLE ,set variable CACHE_ENABLE = 1
	CACHE_ENABLE = 0;
}

opmanage::~opmanage(){}


int opmanage:: Lookup_Work_Port(u_int32_t protect){
	Port_mapping_info *ptr;
   	if (head_of_list == NULL)
		return (-1);
   	else{
	   	for (ptr = head_of_list ; ptr ; ptr = ptr->next){
	      		if (ptr -> protect_port == protect)
	          		return ptr->work_port;
	   	}	        
   	}
   	return (-1);
}




void opmanage::Read_file(){
	FILE *fptr;
    	char * FILEPATH = (char *) malloc(strlen(GetConfigFileDir()) + strlen(RingPath)+1);
	sprintf(FILEPATH,"%s%s",GetConfigFileDir(),RingPath);
		
	fptr = fopen(FILEPATH,"r+");
	if (fptr == NULL){
		printf("virtual ring error: can\'t open file %s  \n",RingPath);
	}
	else{
	   	while (! feof(fptr)){
	      		if (Read_data(fptr))
		  		Add_ring();
	   	}
		fclose(fptr);
    	}
	free(FILEPATH);	 
} 

int opmanage::Read_data(FILE *fptr){
	char buffer[BUFFERSIZE];
	char delim[] = " \0\n";
	char *tok;
	char temp[BUFFERSIZE] = "";
	int token[3] = {0};
	int ptr = 0;
	
	if (fgets(buffer,BUFFERSIZE,fptr) > 0){
    		buffer[strlen(buffer)] = '\0';
	}
	else
		return 0;
	tok = strtok(buffer,delim);
	sprintf(temp,"%s",tok);
	New_ring();
	while(tok != NULL){
		token[ptr] = atoi(tok); 
	   	ptr = (ptr + 1) % 3;
	   	if (ptr == 0){
			Add_element_to_ring(token[0],token[1],token[2]);
	   	}   
	   	tok = strtok(NULL,delim); 
	}
	return 1;
} 

void opmanage::New_ring(){
   	ring_head = new Ring_head;
	ring_head->current_node = NULL;
	ring_head->first = NULL;
	ring_head->last = NULL;
	ring_head->next = NULL;
	ring_head->number  = point_to_ring->count ;
} 

void opmanage::Add_element_to_ring(u_int32_t fromport ,u_int32_t node , u_int32_t toport){
   	Ring_element * ring_element = new Ring_element;
   	ring_element->next = NULL;
  	ring_element->node = node;
    	ring_element->port_to_next = toport;
   	ring_element->port_from_pre = fromport;
   	if (node ==  nodeID)
      		ring_head->current_node = ring_element; 
   	if (ring_head->first == NULL)
      		ring_head->first = ring_element;
   	else
		ring_head->last->next  =  ring_element;   
   	ring_head->last = ring_element;
} 


void opmanage::Add_ring(){
	if (ring_head->current_node  != NULL){
		if (point_to_ring->first == NULL)
			point_to_ring->first = ring_head;
		else
		    	point_to_ring->last ->next  = ring_head;
		point_to_ring->last = ring_head;
		point_to_ring->count ++;
	}
} 

int opmanage::Lookup_Protect_Port2(int toport,int fromport){

	struct 		 chahe_for_Lookup_Protect_Port2 * temp_cache;
        int		In_cache = 0;
	int 		TOPORT_EXIST =0;
	Ring_head *	temphead;
	Ring_element * 	tempelement;
	int 		tempfromport;
	int 		In_Ring_toport = 0;

	temphead = point_to_ring->first ;
	
	if (CACHE_ENABLE){
		for (temp_cache =cache_of_LPP2;temp_cache ;temp_cache = temp_cache->next){
			if ((temp_cache->toport == toport)&&(temp_cache->fromport == fromport)){
				In_cache = 1;
				return temp_cache->result;
			}
		}
	}
	
    	while(temphead != NULL){
		tempelement = temphead->current_node ;
		if (tempelement->port_to_next == toport){
			TOPORT_EXIST = 1;
			tempfromport = tempelement->port_from_pre;
		    	if (tempelement->port_from_pre  == fromport){
				   if(CACHE_ENABLE){
					   struct chahe_for_Lookup_Protect_Port2 * new_cache =  new chahe_for_Lookup_Protect_Port2;
					   new_cache->toport = toport;
			   		   new_cache->fromport = fromport;
					   new_cache->result = fromport;
					   new_cache->next = NULL;
					   if (temp_cache != NULL)
						temp_cache->next = new_cache;
					   else
					        cache_of_LPP2 = new_cache;				   
				   }
				   return (fromport);
			}
		}
		temphead  = temphead ->next ;
	}
	//to port not in ring
	temphead = point_to_ring->first ;
	while(temphead != NULL) {
		tempelement = temphead->current_node;
		if ((tempelement->port_to_next == toport)||(tempelement->port_from_pre == toport)){
			//TOPORT_EXIST = 1;
			In_Ring_toport = 1;
			//tempfromport = tempelement->port_from_pre;
	   		//	if (tempelement->port_from_pre  == fromport)
			//	   return (fromport);
		}
		temphead  = temphead ->next ;
		
	}
		
	
	if (In_Ring_toport ==0){	
		if (CACHE_ENABLE){
			struct chahe_for_Lookup_Protect_Port2 * new_cache =  new chahe_for_Lookup_Protect_Port2;
			new_cache->toport = toport;
			new_cache->fromport = fromport;
			new_cache->result = toport;
			new_cache->next = NULL;
			if (temp_cache != NULL)
				temp_cache->next = new_cache;
			else
				cache_of_LPP2 = new_cache;	
		}		
		return toport;
	}
	
	if (TOPORT_EXIST ){
		if (CACHE_ENABLE){
			struct chahe_for_Lookup_Protect_Port2 * new_cache =  new chahe_for_Lookup_Protect_Port2;
			new_cache->toport = toport;
			new_cache->fromport = fromport;
			new_cache->result = tempfromport;
			new_cache->next = NULL;
			if (temp_cache != NULL)
				temp_cache->next = new_cache;
			else
				cache_of_LPP2 = new_cache;			
		}	
		return(tempfromport);
	}
	//if (In_Ring_toport ==0)
	//	return toport;
    	else{
		if (CACHE_ENABLE){
			struct chahe_for_Lookup_Protect_Port2 * new_cache =  new chahe_for_Lookup_Protect_Port2;
			new_cache->toport = toport;
			new_cache->fromport = fromport;
			new_cache->result = -1;
			new_cache->next = NULL;
			if (temp_cache != NULL)
				temp_cache->next = new_cache;
			else
				cache_of_LPP2 = new_cache;			   
		}
      		return(-1);
	}
}


int opmanage::Lookup_Protect_Port(int toport,int fromport){
 	int TOPORT_EXIST =0;
	Ring_head * temphead;
	Ring_element * tempelement;
	int tempfromport;
	temphead = point_to_ring->first ;
	while(temphead != NULL){
		tempelement = temphead->current_node ;
		if (tempelement->port_to_next == toport){
			TOPORT_EXIST = 1;
			tempfromport = tempelement->port_from_pre;
		    	if (tempelement->port_from_pre  == fromport)
				   return (fromport);
		}
		temphead  = temphead ->next ;
	}
	if (TOPORT_EXIST )
		return(tempfromport);
    	else
      		return(-1);
}


int opmanage::Add_Spaninfo_from_file(){	
	return (0);	
}


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

		

int opmanage::find_fromport_workmode(int port){
	Ring_head * temphead ;
	Ring_element * current_element;
		
	temphead = point_to_ring->first ;
	while(temphead != NULL){
		current_element = temphead->current_node;
		if (current_element->port_to_next == port){
			if (current_element->next != NULL){
				return current_element->next->port_from_pre ;
			}
			else{ // current node is the last one
				return temphead->first->port_from_pre  ; 
			}
		}
		temphead = temphead->next ;
	}
	return -1;
}


int opmanage::Lookup_New_Work_Port(int start,int end,int port){
	Ring_head * temphead;
	Ring_element * tempelement;
	Ring_element * current_element;
	int get_start = 0;
	int get_end = 0;
	struct  chahe_for_Lookup_New_Work_Port * temp_cache;
	
	if (CACHE_ENABLE){
		for (temp_cache = cache_of_LNWP;temp_cache;temp_cache =temp_cache ->next){
			if ((temp_cache->start == start)&&(temp_cache->end == end)&&(temp_cache->port == port))
				return temp_cache->result;
		}
	}
	temphead = point_to_ring->first ;
    	while(temphead != NULL){
		current_element = temphead->current_node;
		for(tempelement = temphead -> first;tempelement;tempelement = tempelement->next){
			if (tempelement->node == start)
				get_start = 1;
			else if (tempelement->node == end)
				get_end = 1;
			if (get_start && get_end)
				break;			
		}
		if(get_start && get_end){
	
			if (current_element->port_from_pre == port){
				if (CACHE_ENABLE){
					struct  chahe_for_Lookup_New_Work_Port * new_cache = new  chahe_for_Lookup_New_Work_Port ;
					new_cache->start = start;
					new_cache->end = end;
					new_cache->port = port;
					new_cache->result = current_element->port_to_next ;
					new_cache->next = NULL;
					if (temp_cache != NULL)
						temp_cache->next = new_cache;
					else
			       			cache_of_LNWP = new_cache;
				}	
				return current_element->port_to_next ;
			}
			else if (current_element->port_to_next == port){
				if (CACHE_ENABLE){
					struct  chahe_for_Lookup_New_Work_Port * new_cache = new  chahe_for_Lookup_New_Work_Port ;
					new_cache->start = start;
					new_cache->end = end;
					new_cache->port = port;
					new_cache->result = current_element->port_from_pre ;
					new_cache->next = NULL;
					if (temp_cache != NULL)
						temp_cache->next = new_cache;
					else
				       		cache_of_LNWP = new_cache;
				}
				return current_element->port_from_pre ;
			}
			get_start=0;
			get_end=0;
		}
		temphead  = temphead ->next ;
	}
	return -1;
}

int opmanage::If_NextPort_In_CurrentRing(int start,int end,int NextPort){
	Ring_head * temphead;
	Ring_element * tempelement;
	Ring_element * current_element;
	struct cache_for_If_NextPort_In_CurrentRing * temp_cache;

	if (CACHE_ENABLE){
		for (temp_cache = cache_of_INIC;temp_cache;temp_cache =temp_cache ->next){
			if ((temp_cache->start == start)&&(temp_cache->end == end)&&(temp_cache->NextPort == NextPort))
				return temp_cache->result;
		}
	}
	
	int get_start = 0;
	int get_end = 0;
	int CurrentRingPort=0;	
	
	temphead = point_to_ring->first ;
    	while(temphead != NULL){
		current_element = temphead->current_node;
		for(tempelement = temphead -> first;tempelement;tempelement = tempelement->next){
			if (tempelement->node == start)
				get_start = 1;
			else if (tempelement->node == end)
				get_end = 1;
			if (get_start && get_end)
				break;			
		}
		if(get_start && get_end){
			if (current_element->port_from_pre == NextPort){
				CurrentRingPort=1;
				if (CACHE_ENABLE){
					struct  cache_for_If_NextPort_In_CurrentRing * new_cache = new  cache_for_If_NextPort_In_CurrentRing ;
					new_cache->start = start;
					new_cache->end = end;
					new_cache->NextPort = NextPort;
					new_cache->result = CurrentRingPort ;
					new_cache->next = NULL;
					if (temp_cache != NULL)
						temp_cache->next = new_cache;
					else
			       			cache_of_INIC = new_cache;
				}
				return	CurrentRingPort;
			}
			else if (current_element->port_to_next == NextPort){
				CurrentRingPort=1;
				if (CACHE_ENABLE){
					struct  cache_for_If_NextPort_In_CurrentRing * new_cache = new  cache_for_If_NextPort_In_CurrentRing ;
					new_cache->start = start;
					new_cache->end = end;
					new_cache->NextPort = NextPort;
					new_cache->result = CurrentRingPort ;
					new_cache->next = NULL;
					if (temp_cache != NULL)
						temp_cache->next = new_cache;
					else
			       			cache_of_INIC = new_cache;
				}
				return	CurrentRingPort;
			}
			//reset or next ring will enter this if anyway		
			get_start=0;
			get_end=0;
		}
		temphead  = temphead ->next ;
	}


	return	CurrentRingPort;
	
}

int opmanage::Find_the_default_dst(int port){
	Ring_head * temphead;
	temphead = point_to_ring->first ;
	Ring_element * tempelement;
	Ring_element * pre_element;
	while(temphead != NULL){
		pre_element = 0;
		for(tempelement = temphead->first ;tempelement ; tempelement = tempelement->next){
			if (tempelement != temphead->first ){
				if (pre_element == NULL){
					pre_element = temphead->first ;
				}
				else
				    pre_element = pre_element->next;
			}
				
			if (tempelement == temphead->current_node ){
				break;
			}
		}
		if (tempelement->port_to_next  == port){
			if (tempelement->next == NULL)
				return temphead->first->node;
			else
				return tempelement->next->node ; 
		}
		else if(tempelement->port_from_pre == port){
			if (pre_element == NULL){
				while(tempelement-> next != NULL)
					tempelement = tempelement->next ;
				return tempelement->node ;
			}
			else
				return pre_element->node ;
		}
		temphead = temphead->next ;
	}
	return -1;
}



int opmanage::send(ePacket_ *pkt) {

	
	//ePacket_ 		*pkt_;
        Packet 			*p = (Packet *)pkt->DataInfo_;   
        char   			*packetmode;
	//char			*span_mode;	
	u_int32_t		spanmode;//span mode
	char			*current_to_port;
	int			Current_To_Port;
	int test1=1;
	char test2;
	test2=(char)test1;
	
	packetmode=p->pkt_getinfo("pm");
	
        current_to_port=p->pkt_getinfo("Next");
	if(current_to_port!=NULL){
		Current_To_Port=current_to_port[0];//in order to get the link_mode[] array
	}
	if(current_to_port==NULL){
		Current_To_Port=0;	
	}	
	 
        spanmode=1;
	if(link_mode[Current_To_Port]==1){//if link is normal , not broken 
         if(packetmode[0]==1)//node is normal packet is working mode
	{
        	struct		MBlist   	*tmp = BinderList;
            	char		*to_port;
	    	char		*from_port;
            	//u_int32_t   	ToPort;
	    	ePacket_	*ep;
	    	char			*packet_default_src;
		char			*packet_default_dst;
		int			CurrentRingPort=1;
		//osw add info below  
                //so we get next port and next wavelength to send
                to_port=p->pkt_getinfo("Next");
           	// ToPort=atoi(to_port);
                 
		//BROADCAST  -->next=0
		if(!to_port){
			from_port=p->pkt_getinfo("From");	
			while(tmp){ 
				if(from_port[0]!=tmp->portnum){
					// change in 12/23 we don't need to broadcast in protection path
					//use Lookop_Protect_Port find the protect port 
					//if return -1 , representing it's a protect path not send
					//if return not -1 , >0  work port , send
		
						if(Lookup_Protect_Port2(tmp->portnum,from_port[0])!=-1){	
							ep=pkt_copy(pkt);
							put(ep,tmp->sendt);
						
						}
				}
				tmp=tmp->next;
			}//
			return 1;
		   }//if



			/*
			 * if next port is not in current ring
			 * set Psrc & Pdst = NULL
			 */
       		packet_default_dst=p->pkt_getinfo("Pdst");
		packet_default_src=p->pkt_getinfo("Psrc");
		if((packet_default_dst!=NULL)&&(packet_default_dst[0]!='0')){


		CurrentRingPort=If_NextPort_In_CurrentRing(packet_default_src[0],packet_default_dst[0],to_port[0]);
		if(CurrentRingPort==0){/* Not Port In Current Ring*/
			char		DefaultSrc;
			char		DefaultDst;
			int ResetPsrcPdst=0;
				
			DefaultSrc=(char)ResetPsrcPdst;
			p->pkt_addinfo("Psrc",&DefaultSrc,1);//Default src
		
			DefaultDst=(char)ResetPsrcPdst;
			p->pkt_addinfo("Pdst",&DefaultDst,1);//Default dst
			

       			packet_default_dst=p->pkt_getinfo("Pdst");
			packet_default_src=p->pkt_getinfo("Psrc");
			}//if==0	

				
		}//if!=NULL


		
		while(tmp){
                      	if(tmp->portnum ==to_port[0]){
				return put(pkt , tmp->sendt);//put packet to related port
			
			}else 
                      		tmp = tmp->next;      	
                   	 
              	}//while
              	//should not happen     
              	if(!tmp)
                     	freePacket(pkt);     
         
              	return 1;
           	}//if  ptmode=1
      				//2.if packet is protection mode packet 
	if(packetmode[0]==2)	//node is normal packet is protecting mode
        {                   	// send out by our function
      
      
            	Packet 			*p = (Packet *)pkt->DataInfo_;
	    	struct MBlist   	*tmp = BinderList;
	    	char	 		*changed_port;
	    	char        		*new_port;
	    	char             	*to_port;
	    	//char			span_port;
	    	char			*packet_default_src;
		char			*packet_default_dst;
		//u_int32_t              	ToPort; 
	    	char             	*from_port; 
	    	//Pu_int32_t              	FromPort;
	    	//char             	*ProtectPort;
	    	//char			*ChangedPort;
	 	//int			protectport;
		int			newworkport;//find by opmanage not by osw
		char			*p1;
		int			CurrentRingPort=1;
		to_port=p->pkt_getinfo("Next");
             	from_port=p->pkt_getinfo("From");
       		packet_default_dst=p->pkt_getinfo("Pdst");
		packet_default_src=p->pkt_getinfo("Psrc");
	     
	     	//find the new output port
			newworkport=Lookup_New_Work_Port(packet_default_src[0],packet_default_dst[0],from_port[0]);//get the new work port
			p1 = (char *)&newworkport;
			new_port=strdup(p1);

			changed_port=strdup(new_port);//???
			/*
			 * if changed port is not is current ring 
			 * set Psrc & Pdst = NULL
			 * and packet mode  pm =1
			 */
			CurrentRingPort=If_NextPort_In_CurrentRing(packet_default_src[0],packet_default_dst[0],changed_port[0]);
			
			if(CurrentRingPort==0){/* Not Port In Current Ring*/
			
				char		DefaultSrc;
				char		DefaultDst;
				int PacketProtection=1;//change to work mode
				int ResetPsrcPdst=0;
				char pkt_protection;	
			
				DefaultSrc=(char)ResetPsrcPdst;
				p->pkt_addinfo("Psrc",&DefaultSrc,1);//Default src
		
				DefaultDst=(char)ResetPsrcPdst;
				p->pkt_addinfo("Pdst",&DefaultDst,1);//Default dst
			

					
				pkt_protection=(char )PacketProtection;
				p->pkt_addinfo("pm",&pkt_protection,1);// "1" ->working path   "2"->protection path
				
			}	
			
			//printf("%dSEND pmode=2 12/18packet is protect  changed_port=%d\n",get_nid(),changed_port[0]);
			
	      	while(tmp){
		   	if(tmp->portnum == changed_port[0]){//different node will affect other ??????
              			return put(pkt , tmp->sendt);//put packet to related port
			}
			else
			  	tmp = tmp->next;
	      	}//while
		//before send it out, change  the "Next" value to new port 
	      
	      	p->pkt_addinfo("Next",changed_port,1);
	   
		free(new_port);
		free(changed_port);
	      	//should not happen     
              	if(!tmp)
                     	freePacket(pkt);     
         
              	return 1;
     
	     
	}//if ptmode=2
     }//if linkmode=1 work
	
	
     if(link_mode[Current_To_Port]==0){  //protection , represent current link is broken
                    
          if(packetmode[0]==1){ //link is broken  packet is working
                         //addinfo to packet "packet_mode"->2 
                         //so we  know that packet should go to protect path 
           		 //!! broken src	
      
           
		Packet 			*p = (Packet *)pkt->DataInfo_;
	
	    	struct MBlist   	*tmp = BinderList;
          
	    	char		 	*changed_port;
	    	char	        	*protect_port;
	    	char             	*to_port;
	    	char             	*from_port; 
		int			PacketProtection;
		char			pkt_protection;
	     	int 			test;
	    	char			*packet_default_src;
		char			*packet_default_dst;

		int			CurrentRingPort=1;
		char			*p1;
		
		to_port=p->pkt_getinfo("Next");
             	
             	from_port=p->pkt_getinfo("From");
        
             	if(spanmode==1){//no span
			PacketProtection=2;

			
			pkt_protection=(char )PacketProtection;
			p->pkt_addinfo("pm",&pkt_protection,1);// "1" ->working path   "2"->protection path
			
			test=Lookup_Protect_Port(to_port[0],from_port[0]);//get the protection port
			p1 = (char *)&test;
			protect_port=strdup(p1);
			changed_port=strdup(protect_port);
	     		
			
			/*
			 * if changed_port is not in current ring
			 * set Psrc & Pdst = NULL
			 */
       			packet_default_dst=p->pkt_getinfo("Pdst");
			packet_default_src=p->pkt_getinfo("Psrc");
			if((packet_default_dst!=NULL)&&(packet_default_dst[0]!='0')){


				CurrentRingPort=If_NextPort_In_CurrentRing(packet_default_src[0],packet_default_dst[0],changed_port[0]);
				if(CurrentRingPort==0){/* Not Port In Current Ring*/
			
					char		DefaultSrc;
					char		DefaultDst;
					int ResetPsrcPdst=0;
				
					DefaultSrc=(char)ResetPsrcPdst;
					p->pkt_addinfo("Psrc",&DefaultSrc,1);//Default src
		
					DefaultDst=(char)ResetPsrcPdst;
					p->pkt_addinfo("Pdst",&DefaultDst,1);//Default dst
			

       					packet_default_dst=p->pkt_getinfo("Pdst");
					packet_default_src=p->pkt_getinfo("Psrc");
					
					
				
				}//if==0	

				
			}//if!=NULL
		}
	      	while(tmp){
			if(tmp->portnum == changed_port[0]){//different node will affect other ??????
				return put(pkt , tmp->sendt);//put packet to related port
			}
			else	
			  	tmp = tmp->next;
	        }//while
		//before send it out, change  the "Next" value to new port 
		p->pkt_addinfo("Next",changed_port,1);
	     
	  	free(protect_port);
		free(changed_port);
	      	//should not happen     
              	if(!tmp)
                     	freePacket(pkt);     
         
              	return 1;

        }//if ptf(spanmode==2){
	}//if linkmode==0 fail
	return 1;		
}//send

int opmanage::recv(ePacket_ *pkt){
          
	Packet 		*p = (Packet *)pkt->DataInfo_;   
	char   		*packetmode;
	//int		PacketProtection;
	char		pkt_protection;
	packetmode=p->pkt_getinfo("pm");
	//add info into packet
	
	if(packetmode==NULL) //initial set path is working
        {
		
		pkt_protection=1;
		p->pkt_addinfo("pm",&pkt_protection,1);// "1" ->working path   "2"->protection path
	 }

	return NslObject::recv(pkt);


	
  }

int opmanage::get(ePacket_ *pkt, MBinder *frm) {

	
        Packet 			*p = (Packet *)pkt->DataInfo_;   
        char    		*linkmode;
	struct MBlist   	*tmp = BinderList;
	char			*packet_default_dst;
	//char			*packet_default_src;// add by chenyuan
	int			pkt_dstID;
	int			LS_broken_port;
	int			found_LS_port=0;//we should reset when new packet come in
	linkmode=p->pkt_getinfo("LS");
	//get the current link condition LS[0]=0->fail LS[0]=1=>work
		
	if(linkmode!=NULL){		
	      	while(tmp){
			if(tmp->sendt->bindModule() == frm->myModule()){//we should find which port the LS packet come from
				LS_broken_port=tmp->portnum;//found broken port 
				found_LS_port=1;
				break;
			}
			else
				tmp=tmp->next;
		}//while
		if(found_LS_port==0){
			LS_broken_port=-1;
		
		}	
	}


       //-----------------------------------------------------------------------------------
	packet_default_dst=p->pkt_getinfo("Pdst");
	if((packet_default_dst!=NULL)&&(packet_default_dst[0]!=0)){
		pkt_dstID=packet_default_dst[0];
	}

	if(p->pkt_getflags()&PF_SEND){// add to packet when send out and decide add_Psrc_Pdst in recv
		if(add_Psrc_Pdst==1){//broken src and dst will come in
			if((packet_default_dst==NULL)||(packet_default_dst[0]==0)){//broken src 	  
		  	//we sould add Pdst Psrc at broken src not broken dst
		
				char		*dst_port;	
       	       		  	int		default_to_port;
				//char		DefaultToPort;
				int		current_nodeID;
				int		default_dst;
				char		DefaultSrc;
				char		DefaultDst;
				int		fromport_in_workmode;
				char		DefaultFromPort;
				//get the Next port value in  osw , so we should do those in send	
				dst_port=p->pkt_getinfo("Next");//to port
		
				default_to_port=dst_port[0];

				fromport_in_workmode=find_fromport_workmode(default_to_port);
		
				DefaultFromPort=(char )fromport_in_workmode;

		
				p->pkt_addinfo("Fport",&DefaultFromPort,1);//default from  port
		
				current_nodeID=get_nid();
		
				DefaultSrc=(char )current_nodeID;
				//printf("%d 0420default src=%d\n",get_nid(),DefaultSrc);
				p->pkt_addinfo("Psrc",&DefaultSrc,1);//Default src
		
				default_dst=Find_the_default_dst(dst_port[0]);//new add function!!

				DefaultDst=(char )default_dst;
				p->pkt_addinfo("Pdst",&DefaultDst,1);//Default dst

	   			}// if ==NULL
		  }//if LS tell us should add Psrc Pdst	
	 }//if send
	//-------------------------------------------------------------------------------------------
	if(linkmode==NULL){
		if (p->pkt_getflags() & PF_SEND)
			return send(pkt);
		
		else if (p->pkt_getflags() & PF_RECV) {
			char	*packetmode;
			packetmode=p->pkt_getinfo("pm");

			if (packetmode != NULL){
		
				if(packetmode[0]==2){	//packet is broken
						     	//data packet will go to the path that does't 
							//include in LPT
		
					if((packet_default_dst!=NULL)&&(packet_default_dst[0]!=0)){//represent node is not broken src
					
						//if current nodeID !=sourceID ->send down to the op
						
						//if current nodeID ==sourceID ->send up   to the op
						if((int)get_nid()==pkt_dstID){
				 			//change packet mode	
							int PacketProtection=1;//change to work mode
							char pkt_protection;	
					
							pkt_protection=(char )PacketProtection;
							p->pkt_addinfo("pm",&pkt_protection,1);// "1" ->working path   "2"->protection path
							
							//we set from port =1 ---12/19
							int  default_Pdst_fromport;//Pdst's fromport
							char *DefaultPdstFromport;
							char *p1;
							char *new_port;
							char *changed_port;	
							
							DefaultPdstFromport=p->pkt_getinfo("Fport");
							default_Pdst_fromport=DefaultPdstFromport[0];
							p1 = (char *)&default_Pdst_fromport;;
							new_port=strdup(p1);

							changed_port=strdup(new_port);//???
							p->pkt_addinfo("From",changed_port,1);
				
							free(new_port);
							free(changed_port);
							
							/*modify by chenyuan*/

						        char zero=(char )0;
							p->pkt_addinfo("Psrc",&zero,1);//Default src
		
							p->pkt_addinfo("Pdst",&zero,1);//Default dst

							return  recv(pkt);
					
						}
						
						if((int)get_nid()!=pkt_dstID){//not the dst node
							
	    						char			*packet_default_src;
							char			*packet_default_dst;
	    						char             	*from_port; 
             						int			newworkport;
							from_port=p->pkt_getinfo("From");  //add by chenyuan
							//int temp_from = from_port[0];

       							packet_default_dst=p->pkt_getinfo("Pdst");
							packet_default_src=p->pkt_getinfo("Psrc");
	     
	     						//find the new output port
						
							newworkport=Lookup_New_Work_Port(packet_default_src[0],packet_default_dst[0],from_port[0]);//get the new work port

							if(link_mode[newworkport]==0){
				
	
							//deal with node broken here
							//packetmode==2
							//packet_default_dst!=NULL
							//1. link_mode is protect
							//2. packetmode is protect
						
							//FIRST change packet mode

							int PacketProtection=1;//change to work mode
							char pkt_protection;	
					
							pkt_protection=(char )PacketProtection;
							p->pkt_addinfo("pm",&pkt_protection,1);// "1" ->working path   "2"->protection path

							//SECOND change from port
							

							//we set from port =1 ---12/19
							char *p1;
							char *new_port;
							char *changed_port;	
							
							p1 = (char *)&newworkport;
							new_port=strdup(p1);

							changed_port=strdup(new_port);//???
							//printf("%d 0420 broken node not dst changed_port=%d\n",get_nid(),newworkport);
							p->pkt_addinfo("From",changed_port,1);
							free(new_port);
							free(changed_port);
							//dmalloc_log_unfreed();	
							
						        char zero=(char )0;
							p->pkt_addinfo("Psrc",&zero,1);//Default src
							p->pkt_addinfo("Pdst",&zero,1);//Default dst
							return  recv(pkt);

								

							}//if	
							
							else if(link_mode[newworkport]==1){ 
							//normal packet broken node
							//1.link_mode is work
							//2.packetmode is protect
							//printf("%dGET I  am middle node with bad packet\n",get_nid());

							
							/*we should change Next port 
							 * so when send get it
							 * linkmode[to_port] will be correct linkmode
							 * when we want to send newworkport
							 */
							char *p1;
							char *new_port;
							char *changed_port;	
							
							p1 = (char *)&newworkport;
							new_port=strdup(p1);
							
							//printf("%d 0420 ok node not dst changed_port=%d\n",get_nid(),newworkport);
							changed_port=strdup(new_port);//???
							p->pkt_addinfo("Next",changed_port,1);
							free(new_port);
							free(changed_port);
							p->pkt_setflow(PF_SEND);				
							//dmalloc_log_unfreed();	
							return send(pkt);
							}//if
						}

					}//packet_default_dst!=NULL
					
				}//packetmode==2
				//dmalloc_log_unfreed();	
	
				return  recv(pkt);//packetmode=1 packet is work ,send up by "recv"
				
			}//packetmode!=NULL
		
			else if(packetmode==NULL)
				//dmalloc_log_unfreed();	
				return recv(pkt);
		
		}//PF_RECV
	}
	if(linkmode!=NULL){
		if(linkmode[0]==1){//work
	
			add_Psrc_Pdst=0;// node is work so we should not add Psrc Pdst
			link_mode[LS_broken_port]=1;	
			printf("%d <<WORK>>GET change link_mode work link mode=%d\n",get_nid(),link_mode[LS_broken_port]);
			freePacket(pkt);
		
	  	}
		else if(linkmode[0]==0){//fail 
				//if packet through a node which have broken port
	
			add_Psrc_Pdst=1;//so we should add in "send" ,but we sould only add in broken src		
			link_mode[LS_broken_port]=0;
			printf("%d <<FAIL>>GET change link_mode fail link mode=%d\n",get_nid(),link_mode[LS_broken_port]);
			//dmalloc_log_unfreed();	
			freePacket(pkt);
	
		  }
	}
	return (0);
}
// after tcl etc. it will init every associated module
// begin the first step
int opmanage::init()
{
	struct MBlist	*tmp = BinderList;
	int		j;
	for(j=0;j<1024;j++){
		link_mode[j]=1;//initial good
	}
	add_Psrc_Pdst=0;//initial not to add
	while(tmp != NULL){
		tmp = tmp->next;
	}

	Read_file();	
	return(1);  
}
