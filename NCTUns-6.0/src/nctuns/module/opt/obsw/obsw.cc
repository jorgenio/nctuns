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

#include "obsw.h"
#include <packet.h>

//#include <dmalloc.h>
extern RegTable                 RegTable_;
extern typeTable		*typeTable_; 
extern ReadPath			*readpath_;  

   MODULE_GENERATOR(obsw);
   
   obsw::obsw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name){
	
	//initial list of datapkt per node(optical burst switch)
	head_of_datapkt=NULL;
	tail_of_datapkt=NULL;
	temppkt=NULL;
	prevpkt=NULL;
	nextpkt=NULL;
	findpkt=NULL;
	bw=NULL;
	head_of_contrlpkt=NULL;
	tail_of_contrlpkt=NULL;
	current_head_of_contrlpkt=NULL;
	current_tail_of_contrlpkt=NULL;
	next_head_of_contrlpkt=NULL;
	updatectl=NULL;
	Ftempctl=NULL;
	freectl=NULL;
	fprevctl=NULL;
	fnextctl=NULL;
	
	pre_processing_arrive_time=0;
	readpath_->Start();
	clistlen=0;
	dlistlen=0;
	mem=0;
	prev_recv_time=0;	
					 /*
					  * 1. drop all contenting burst 
					  * 2. drop contending burst ,head-dropping
					  * 3. drop oringinal  burst ,tail-dropping	
					  */

	vBind("ContrlPktProcessTime", &ContrlPktProcessTime);
	vBind("TailDrop_control_wavelength", &TailDrop_control_wavelength);
	vBind("select_candidate_method", &select_candidate_method);
	vBind("drop_burst_segmentation_method", &drop_burst_segmentation_method);
	
    }
	
   obsw::~obsw(){
   }



u_int64_t obsw::calculate_transmit_time(char from_port,int from_wave,int burst_bytes){
   	//we get burst_bytes ,we should use bw(bandwidth) to calculate the transmit time
		
	
	struct		plist *tmplist;
	int		fromport;
	u_int64_t	onesec;
	u_int64_t	transmit_time;
	double		transmit_time_double;
	fromport=from_port;
	tmplist=(struct plist *)malloc(sizeof(struct plist));
	tmplist->pid=fromport;//from port
	tmplist->next=(struct plist *)malloc(sizeof(struct plist));
	tmplist->next->pid=from_wave;//from wave
	tmplist->next->next=NULL;//we don't want to move the pointer of tmplist 
	
	bw=(double *)get_regvar(get_nid(),tmplist,"BW");
	mem++;
	mem++;
	tmplist->pid=0;
	tmplist->next->pid=0;
	free(tmplist->next);
	free(tmplist);
	mem--;
	mem--;
	
	SEC_TO_TICK(onesec,1);
	transmit_time=burst_bytes * 8 * onesec / (u_int64_t)(*bw);
	transmit_time_double=burst_bytes * 8 * onesec / (*bw);
	if((transmit_time_double-transmit_time)*10>=5){
		transmit_time++;
	}	
	return transmit_time;
   	


	   
   }	

   int obsw::calculate_transmit_bytes(char from_port,int from_wave,int burst_duration){
   	//we get burst_bytes ,we should use bw(bandwidth) to calculate the transmit time
		
	
	struct		plist *tmplist;
	int		fromport;
	u_int64_t	onesec;
	u_int64_t	transmit_bytes;
	fromport=from_port;
	tmplist=(struct plist *)malloc(sizeof(struct plist));
	tmplist->pid=fromport;//from port
	tmplist->next=(struct plist *)malloc(sizeof(struct plist));
	tmplist->next->pid=from_wave;//from wave
	tmplist->next->next=NULL;//we don't want to move the pointer of tmplist 
	
	bw=(double *)get_regvar(get_nid(),tmplist,"BW");
	mem++;
	mem++;
	tmplist->pid=0;
	tmplist->next->pid=0;
	free(tmplist->next);
	free(tmplist);
	SEC_TO_TICK(onesec,1);
	mem--;
	mem--;
	transmit_bytes=burst_duration  * (u_int64_t)(*bw) / (8 * onesec);
	
	
	return transmit_bytes;
   	


	   
   }

    int	obsw::update_prevOL_datapkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t new_duration){//wave is datapkt's wave


		if(head_of_datapkt!=NULL){
		for(temppkt=head_of_datapkt;temppkt!=NULL;temppkt=temppkt->next){
			if(burst_id==temppkt->burstid){
           	    		if((wave==temppkt->wavelength)&&(NextPort==temppkt->nextport)){
					if((source==temppkt->src)&&(destination==temppkt->dst)){
				
						temppkt->end_transmit_time = temppkt->start_transmit_time + new_duration;
						return 1;
					}//if
		    		}//if
			}//if
		}//for
		}//if
	
		return 0;
	}
    int	obsw::update_prevOL_contrlpkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t new_duration){//wave is control pkt's wave 
	
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){
				if(tempctl->burstid==burst_id){
					if((source==tempctl->src)&&(destination==tempctl->dst)){
						if(NextPort==tempctl->nextport){
							if(wave==tempctl->wavelength){
							
								tempctl->reserve_dur=new_duration;
								return 1;
								
												
							}//if	
						}//if
					}//if	
				}//if
			
				
			}//for
		}//if !=NULL	
		
	    return 0;
	}
    int obsw::Send_TailDrop_control_packet(Event  *ep){
	
	    
		u_long          burstid;
		u_int32_t       src;
		u_int32_t       dst;
		u_int8_t        wavelength;
		int             nextport;
		u_int64_t       new_reserve_dur;
		int             oldwave;
		int		DataWave;
		char		pkt_protection=1;
		
		struct	controlpkt	*TailDropControl;	
		assert(ep);
		TailDropControl= (struct controlpkt *)ep->DataInfo_;
	    
		burstid=TailDropControl->burstid;
		wavelength=TailDropControl->wavelength;
		nextport=TailDropControl->nextport;
		src=TailDropControl->src;
		dst=TailDropControl->dst;
		new_reserve_dur=TailDropControl->reserve_dur;
		//we let update represent OLDW
		oldwave=TailDropControl->update;
	    	//have_found represent DataWave
		DataWave=TailDropControl->have_found;
		
		freeEvent(ep);
		mem--;	
		
		ePacket_        *TD_Control_EventPacket = createPacket();
	        Packet          *TD_Control_Packet;
		struct lheader  *TD_Control_lh;
		TD_Control_Packet = (Packet *)TD_Control_EventPacket->DataInfo_;
		int		OBS=3;

		TD_Control_Packet->pkt_addinfo("OBS",(char *)&OBS ,sizeof(int));
		TD_Control_Packet->pkt_addinfo("SEQ", (char *)&burstid, sizeof(u_long));
		// adding wave in shortest path in packet

		int from = get_nid();
		TD_Control_Packet->pkt_addinfo("FROM", (char *)&from, sizeof(int));	//  for debug	
	
		TD_Control_Packet->pkt_addinfo("OLDW",(char *)&oldwave,sizeof(int));	
		TD_Control_Packet->pkt_addinfo("WAVE",(char *)&DataWave,sizeof(int));	
		
		TD_Control_Packet->pkt_addinfo("Cwave",(char *)&wavelength,sizeof(int));	
		
		TD_Control_Packet->pkt_addinfo("r_dur",(char *)&new_reserve_dur,sizeof(int));//?? be length?
		TD_Control_Packet->pkt_addinfo("Next",(char *)&nextport,sizeof(int));
		
		TD_Control_lh = (struct lheader *)TD_Control_Packet->pkt_malloc(sizeof(struct lheader));
		TD_Control_lh->dstIP =dst;
		TD_Control_lh->wave =3;//TailDrop_control_wavelength;//wavelength ;
		TD_Control_lh->option = 0;
		TD_Control_lh->srcIP =src;
		
		TD_Control_Packet->pkt_addinfo("pm",&pkt_protection,1);	
		
		return NslObject::send(TD_Control_EventPacket);
	
    
    }

 u_int64_t obsw::check_if_reserve_time_overlap(u_int64_t  start_time,int NextPort,int WaveLength,int burst_id,u_int8_t contrlWave,int  oldwave){ 	
	//check if new reserve time has overlap
	int	overlap=0;
	u_int64_t	NodeCurrentTime;
	int		do_check_overlap;
	int		process_time_;
	process_time_=ContrlPktProcessTime;
	   
	NodeCurrentTime=GetCurrentTime();
	if(NodeCurrentTime!=prev_recv_time){
		do_check_overlap=1;
		prev_recv_time=NodeCurrentTime;//control packet arrive at different time
	}
	else if(NodeCurrentTime==prev_recv_time){
		do_check_overlap=0;
	
	}
	
	if(do_check_overlap==1){
	
	for(findpkt=head_of_datapkt;findpkt!=NULL;findpkt=findpkt->next){
		
		//we should not only judge if reserve time overlap but also next output port 
		if(NextPort==findpkt->nextport){
			if(WaveLength==findpkt->wavelength){
				if(start_time < findpkt->end_transmit_time){
					overlap=1;

					if(drop_burst_segmentation_method==1){//drop all
					
						return 0;//impossible startx_time==0 
					}
					
					
					if(drop_burst_segmentation_method==2){//segment head dropping
						
						//change the control pkt offset and duration
						return  findpkt->end_transmit_time;
					}
					
					if(drop_burst_segmentation_method==3){//segment tail dropping
						
						
						//change the prev overlap datapkt data and send an message out
						u_int64_t	new_tail_duration;
						u_int64_t	old_duration;
						new_tail_duration = start_time - findpkt->start_transmit_time;
						old_duration=findpkt->end_transmit_time -findpkt->start_transmit_time;

						update_prevOL_datapkt(findpkt->burstid,findpkt->src,findpkt->dst,findpkt->nextport,findpkt->wavelength,new_tail_duration);//wave is datapkt wave
						
						update_prevOL_contrlpkt(findpkt->burstid,findpkt->src,findpkt->dst,findpkt->nextport,contrlWave,new_tail_duration);//wave is controlpkt wave
					
						/*
						 * create an event ,bring the message to Send control packet()
						 * 
						 */
						
						Event          *ep;
						struct 		controlpkt	*TailDropContrl;
						ep = createEvent();
						TailDropContrl= (struct controlpkt *)malloc(sizeof(struct controlpkt));
						assert(TailDropContrl);
						mem++;				
						
						//New_DUR_LEN=calculate_transmit_bytes(from[0],from_wave,New_duration);
						TailDropContrl->burstid=findpkt->burstid;
						TailDropContrl->wavelength=contrlWave;
						TailDropContrl->nextport=findpkt->nextport;
						TailDropContrl->src=findpkt->src;
						TailDropContrl->dst=findpkt->dst;
						TailDropContrl->reserve_dur=new_tail_duration;
						
						//we let update represent OLDW
						TailDropContrl->update=oldwave;
						//have_found represent DataWave
						TailDropContrl->have_found=findpkt->wavelength;
				
						
						TailDropContrl->offset=0;
						TailDropContrl->used_dur=0;
						TailDropContrl->arrive_time=0;
						TailDropContrl->been_divided=0;
						TailDropContrl->list_number=0;
						TailDropContrl->next=NULL;
						TailDropContrl->prev=NULL;
						ep->DataInfo_ = (void *)TailDropContrl;

						BASE_OBJTYPE(type);
						type = POINTER_TO_MEMBER(obsw,Send_TailDrop_control_packet);
						//ep=createEvent();
						setObjEvent(ep,GetCurrentTime()+process_time_,0,this,type,(void *)ep->DataInfo_);
						
						return start_time;
					}
				}
			}
		}//if
	}//for
	
	}//if




	
	if(overlap==0){
		return	start_time;	
   	}//if
   
	
	overlap=0;
	do_check_overlap=1;
	return 1;
   }

	//record the reserved time to data packet structure
    u_int64_t obsw::reserve_duration(int offset,int process_time,u_int64_t  duration_time ,int burst_id, u_int32_t  src, u_int32_t dst, int wave,int  NextPort,u_int8_t contrlWave,int	oldwave){
	//create a new datapkt struct  , when a control packet coming in
	struct		datapacket	*datapkt;
	datapkt= (struct datapacket *) malloc(sizeof(struct datapacket));
	u_int64_t	node_current_time;
	//u_int64_t	prev_recv_time;
	u_int64_t	startx_time;
	u_int64_t	startx_time_;
	u_int64_t	endx_time;
	u_int64_t	new_offset=0;
	mem++;
	//create a new datapkt and record data in it
	datapkt->burstid=burst_id;
	datapkt->wavelength=wave;
	datapkt->nextport=NextPort;
	datapkt->src=src;
	datapkt->dst=dst;
	datapkt->size=0;
	dlistlen++;
	
	node_current_time=GetCurrentTime();
	datapkt->node_current_time=node_current_time;

	//we will judge drop or get by get totally packet
	//try we add transmit time
	startx_time=node_current_time+offset;//first data packet will arrive in startx_time

	/*!!! we should check overlap start_time not only start_time but output Next port !!!  */
	startx_time_=check_if_reserve_time_overlap(startx_time,NextPort,wave, burst_id,contrlWave,oldwave);
	
	endx_time=startx_time+duration_time;//+4160000;//startx_time is initial start time
	if((drop_burst_segmentation_method==1)&&(startx_time_==0)){
		startx_time_=endx_time+1;
		new_offset=offset + (startx_time_ - startx_time);
	}
	if(drop_burst_segmentation_method==2){
		if(startx_time_!=startx_time){
			
			new_offset=offset + (startx_time_ - startx_time);
		}
	}
	datapkt->start_transmit_time=startx_time_;
	datapkt->end_transmit_time=endx_time;
	datapkt->next=NULL;
	datapkt->prev=NULL; 	
		
	//add to the datapkt list
	    

	//first node of datapkt's list
	if(head_of_datapkt==NULL){
		head_of_datapkt=datapkt;//let first node be head and tail
		tail_of_datapkt=datapkt;
		//head_of_datapkt->size=9;
		
		head_of_datapkt->size=head_of_datapkt->size+1;
	}//if first node
	else {//head_of_datapkt!=NULL
	
		//add new datapkt to tail of list

		tail_of_datapkt->next=datapkt;//add to list of tail
		datapkt->prev=tail_of_datapkt;
		tail_of_datapkt=tail_of_datapkt->next;//point to the new tail of list
		
		head_of_datapkt->size=head_of_datapkt->size+1;
	}//else if
	

	return	new_offset;

	
    }//reserve_duration 







	//see if we have a reservation for current packet or not 
        //reserved return 1
        //not reserved return 0
        //reserve_for_current_packet(node_current_time,burst_id_,source,destination,wavelength)
    int obsw::reserve_for_current_packet(u_int64_t node_current_time,int inpkt_burstid,u_int32_t src,u_int32_t dst,int NextPort,u_int8_t wave){
	//packet create in here?????????????????  or in class like head and tail    
	//
	//we put below in class


	if(head_of_datapkt!=NULL){
	for(temppkt=head_of_datapkt;temppkt!=NULL;temppkt=temppkt->next){//traverse every node in list
		if(inpkt_burstid==temppkt->burstid){//find the related datapkt record which recorded by control packet
           	    if((wave==temppkt->wavelength)&&(NextPort==temppkt->nextport)){//if wavelength match
			if((src==temppkt->src)&&(dst==temppkt->dst)){//if src && dst match
			    		//exactly the datapkt record compared with incoming packet					
				if(temppkt->start_transmit_time <= node_current_time){//begin transmit
						
					if(temppkt->end_transmit_time > node_current_time){//during reserved time		
						return (1);
					}
					else if(temppkt->end_transmit_time==node_current_time){//last packet 
											       //send and clear datapkt record 
						//clear_reservation_record(temppkt);
					        //remove temppkt node from datapkt list
					
						return (1);
					}
					else if(temppkt->end_transmit_time < node_current_time){//expired 
												// drop pkt and clear datapkt record
						//clear_reservation_record(temppkt);
						//remove temppkt node from datapkt list

						return (0);
					}
				}//if
				else if(temppkt->start_transmit_time > node_current_time){//not yet transmit
											  //drop pkt	
						
						
										
						return (0);
                                }//else if
				
				break;//?????ok? found the record so end the for loop    



				
				
			    }//if
			    else if((src!=temppkt->src)||(dst!=temppkt->dst)){//if src && dst not match
			    
			         	if(temppkt->next==NULL){
						return (0);
					}
			    }
		    }//if
		    else if(wave!=temppkt->wavelength){//if wavelength not match
		    
			 	if(temppkt->next==NULL){
					return (0);
				}
		    }
		}//if
		else if(inpkt_burstid !=temppkt->burstid){//if burst id not match
		     	
			if(temppkt->next==NULL){
				return (0);
			}
		}



	}//for
	}//if
	if(head_of_datapkt==NULL){
	
		return (0);
	}    
	if(temppkt==NULL){
		return (0);
	}
	return 1;
    }//reserve_for_current_packet
    
	//send the packet   ref WAN.cc


      //RECORD
     void obsw::record_control_pkt_with_recv_time(int burst_id,u_int32_t source,u_int32_t destination,int offset,u_int64_t duration,u_int64_t control_pkt_arrive_time,int NextPort,u_int8_t wave){

	struct		controlpkt	*contrlpkt;
	contrlpkt=(struct controlpkt *)malloc(sizeof(struct  controlpkt));

	//u_int64_t	node_current_time;
	//u_int64_t	prev_recv_time;
	mem++;
	clistlen++;
	//create a new datapkt and record data in it
	contrlpkt->burstid=burst_id;
	contrlpkt->wavelength=wave;
	contrlpkt->nextport=NextPort;
	contrlpkt->src=source;
	contrlpkt->dst=destination;
	contrlpkt->arrive_time=control_pkt_arrive_time;
	contrlpkt->reserve_dur=duration;
	contrlpkt->size=0;
	contrlpkt->used_dur=0;
	contrlpkt->offset=offset;
	contrlpkt->have_found=0;
	contrlpkt->update=0;

	contrlpkt->next=NULL;
	contrlpkt->prev=NULL; 	
		//first node of contrlpkt's list
		if(head_of_contrlpkt==NULL){
			head_of_contrlpkt=contrlpkt;//let first node be head and tail
			tail_of_contrlpkt=contrlpkt;
			//head_of_contrlpkt->size=0;
			head_of_contrlpkt->size=head_of_contrlpkt->size+1;
		}//if first node
		else {//head_of_contrlpkt!=NULL
	
			//add new contrlpkt to tail of list

			tail_of_contrlpkt->next=contrlpkt;//add to list of tail
			contrlpkt->prev=tail_of_contrlpkt;
			tail_of_contrlpkt=tail_of_contrlpkt->next;//point to the new tail of list
			head_of_contrlpkt->size=head_of_contrlpkt->size+1;
		}//else if

	     
	}

	
   void obsw::Free_contrlpkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t  wave,u_int64_t contrlpkt_arrive_time){

	   
		
		int		found_freectl=0;
		
		/*first judge if there is the other packets with same arriving time*/
		if(head_of_contrlpkt!=NULL){
			for(Ftempctl=head_of_contrlpkt;Ftempctl!=NULL;Ftempctl=Ftempctl->next){
				if(Ftempctl->burstid==burst_id){
					if((source==Ftempctl->src)&&(destination==Ftempctl->dst)){
						if((NextPort==Ftempctl->nextport)&&(wave==Ftempctl->wavelength)){
							if(contrlpkt_arrive_time==Ftempctl->arrive_time){
													
								//found the contrlpkt that should be free
								
								freectl=Ftempctl;
								found_freectl=1;
								break;
												
							}//if	
						}//if
					}//if	
				}//if
				
			}//for
		}//if !=NULL	
		if(found_freectl==1){


			/*freectl is head_of_contrlpkt */
			if(freectl==head_of_contrlpkt){	

				if(head_of_contrlpkt->next!=NULL){
					
					head_of_contrlpkt->next->size=head_of_contrlpkt->size;
					
					head_of_contrlpkt=head_of_contrlpkt->next;
					if(head_of_contrlpkt->prev!=NULL)
						head_of_contrlpkt->prev=NULL;	
					if(freectl->prev!=NULL)
						freectl->prev=NULL;
					if(freectl->next!=NULL)
						freectl->next=NULL;
					clistlen--;
					head_of_contrlpkt->size=head_of_contrlpkt->size-1;
					free(freectl);
					freectl=NULL;
					mem--;

				}
				else{
				
				}
			}	
			/*freectl is tail_of_contrlpkt*/
			else if(freectl==tail_of_contrlpkt){
			

				if(tail_of_contrlpkt->prev!=NULL){
					tail_of_contrlpkt=tail_of_contrlpkt->prev;
					if(tail_of_contrlpkt->next!=NULL)
						tail_of_contrlpkt->next=NULL;	
					if(freectl->prev!=NULL)
						freectl->prev=NULL;
					if(freectl->next!=NULL)
						freectl->next=NULL;
  					clistlen--;
					head_of_contrlpkt->size=head_of_contrlpkt->size-1;
					free(freectl);
					mem--;
					freectl=NULL;
				}
				else{
					
				}
			}	
			/*freectl is in the middle of contrlpkt list */
			else if((freectl->next!=NULL)&&(freectl->prev!=NULL)){	
			
				
				fprevctl=freectl->prev;
				fnextctl=freectl->next;
				
				fprevctl->next=fnextctl;
				fnextctl->prev=fprevctl;
				
				if(freectl->prev!=NULL)
					freectl->prev=NULL;
				if(freectl->next!=NULL)
					freectl->next=NULL;
				clistlen--;
				head_of_contrlpkt->size=head_of_contrlpkt->size-1;
				free(freectl);
				freectl=NULL;
				mem--;
				
				
			}//if
			else{

			}
		}
		else if(found_freectl==0){
		
			
		}

		found_freectl=0;

   }






   u_int64_t obsw::Get_useful_time(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave){


		int		found_freepkt=0;
		u_int64_t	UsefulTime;
		
		if(head_of_datapkt!=NULL){
			for(temppkt=head_of_datapkt;temppkt!=NULL;temppkt=temppkt->next){
				if(temppkt->burstid==burst_id){
					if((source==temppkt->src)&&(destination==temppkt->dst)){
						if((NextPort==temppkt->nextport)&&(wave==temppkt->wavelength)){
													
							//found the datapkt that should be free
								
							UsefulTime=temppkt->end_transmit_time - temppkt->node_current_time;		
							
							found_freepkt=1;
							return UsefulTime;
												
						}//if
					}//if	
				}//if
				
			}//for
		}//if !=NULL	
	  
		if(found_freepkt==0){
			return 0;
	   	}
	return 1;
   }

   int obsw::Free_datapacket2(Event_ *ep2){


		struct	datapacket	*freedata;	
		int		found_freepkt=0;
		int		burst_id;
		u_int32_t  	source;
		u_int32_t  	destination;
		int 		NextPort;
		u_int8_t 	wave;
		assert(ep2);
		freedata = (struct datapacket *)ep2->DataInfo_;
		
		burst_id=freedata->burstid;
		wave=freedata->wavelength;
		NextPort=freedata->nextport;
		source=freedata->src;
		destination=freedata->dst;

		
		freeEvent(ep2);
		mem--;
		
		if(head_of_datapkt!=NULL){
			for(Ftemppkt=head_of_datapkt;Ftemppkt!=NULL;Ftemppkt=Ftemppkt->next){
				if(Ftemppkt->burstid==burst_id){
					if((source==Ftemppkt->src)&&(destination==Ftemppkt->dst)){
						if((NextPort==Ftemppkt->nextport)&&(wave==Ftemppkt->wavelength)){
													
							//found the datapkt that should be free
								
							freepkt=Ftemppkt;
							found_freepkt=1;
							break;
												
						}//if
					}//if	
				}//if
				
			}//for
		}//if !=NULL	
	   

		if(found_freepkt==1){


			/*freepkt is head_of_datapkt */
			if(freepkt==head_of_datapkt){	

				if(head_of_datapkt->next!=NULL){
					//printf("%d cut head of datapkt\n",get_nid());
					head_of_datapkt->next->size=head_of_datapkt->size;
					head_of_datapkt=head_of_datapkt->next;
					if(head_of_datapkt->prev!=NULL)
						head_of_datapkt->prev=NULL;	
					if(freepkt->prev!=NULL)
						freepkt->prev=NULL;
					if(freepkt->next!=NULL)
						freepkt->next=NULL;
					dlistlen--;
					head_of_datapkt->size=head_of_datapkt->size-1;
					free(freepkt);
					freepkt=NULL;	
					mem--;

				}
				else {
				}
			}	
			/*freepkt is tail_of_datapkt*/
			else if(freepkt==tail_of_datapkt){
			

				if(tail_of_datapkt->prev!=NULL){
					tail_of_datapkt=tail_of_datapkt->prev;
					if(tail_of_datapkt->next!=NULL)
						tail_of_datapkt->next=NULL;	
					if(freepkt->prev!=NULL)
						freepkt->prev=NULL;
					if(freepkt->next!=NULL)
						freepkt->next=NULL;
					dlistlen--;
					head_of_datapkt->size=head_of_datapkt->size-1;
					free(freepkt);
					mem--;
					freepkt=NULL;	
				}
				else{
				}
			}	
			/*freepkt is in the middle of datapkt list */
			else if((freepkt->next!=NULL)&&(freepkt->prev!=NULL)){	
			
				
				fprevpkt=freepkt->prev;
				fnextpkt=freepkt->next;
				
				fprevpkt->next=fnextpkt;
				fnextpkt->prev=fprevpkt;
				
				if(freepkt->prev!=NULL)
					freepkt->prev=NULL;
				if(freepkt->next!=NULL)
					freepkt->next=NULL;
				dlistlen--;
				head_of_datapkt->size=head_of_datapkt->size-1;
				free(freepkt);
				freepkt=NULL;	
				mem--;
				
				
			}//if
			else {
			}

		}
		else if(found_freepkt==0){
		
			
		}

		found_freepkt=0;
		
				
		
		return	(1);


		

   }
    void obsw::update_datapkt(struct controlpkt * update){
	    														      int             burstid;
 		u_int32_t       src;
		u_int32_t       dst;
		u_long          offset;
		u_int8_t        wavelength;
		int             nextport;
		u_int64_t       reserve_dur;
		u_int64_t       arrive_time;
										
		burstid=update->burstid;
		src=update->src;
		dst=update->dst;
		offset=update->offset;
		wavelength=update->wavelength;
		nextport=update->nextport;
		reserve_dur=update->reserve_dur;
		arrive_time=update->arrive_time;
		if(head_of_datapkt!=NULL){
			for(temppkt=head_of_datapkt;temppkt!=NULL;temppkt=temppkt->next){//traverse every node in list
				if(burstid==temppkt->burstid){//burst_id match
           	    			if(wavelength==temppkt->wavelength){//if wavelength match
						if((src==temppkt->src)&&(dst==temppkt->dst)){//
							if((nextport=temppkt->nextport)){
								temppkt->start_transmit_time=temppkt->node_current_time+offset;
								temppkt->end_transmit_time=temppkt->node_current_time+offset+reserve_dur;
							
							}
						}						
		  			}	
				}	
	    		}//for
		}//if	

	}//update datapkt

	
    void obsw::update_record_of_control_pkt(struct controlpkt *current_head_of_contrlpkt ,struct controlpkt *next_head_of_contrlpkt,int number_of_contrl){
		
	int	min_offs=50000000;
	int	max_offs=0;
	int	count;
	int	random_number=-1;
	int	random_history[100];
	int	i;
	int	random_begin=1;
	int	randomtimes=0;

	for(i=0;i<100;i++){		
		random_history[i]=-1;
	}

	for(count=0;count<number_of_contrl;count++){//do number of control pkt times
	    		
		if(select_candidate_method==1){//randomly choose
		
		
			while(random_begin==1){
			
				//if we record random number in history "number_of_contrl" times
				//we should not random anymore
				if(randomtimes==number_of_contrl)
					break;
				
				random_number=random() % number_of_contrl;
				for(i=0;i<100;i++){
					if(random_number==random_history[i]){
						random_begin=1;
						break;//jump out and random again
					}//if
					else if(random_number!=random_history[i]){
						random_begin=0;//keep finding no jump
					}	
				}//for
				
	
			}//while
			random_begin=1;//so next for loop we will enter while loop
			
			for(i=0;i<100;i++){
				if(random_history[i]==-1){
					random_history[i]=random_number;
					
					randomtimes++;//increase randomtimes , randomtimes should not
						      //more than number_of_contrl			
					break;
				}//if	
				
			}//if
			
			updatectl=current_head_of_contrlpkt;
			for(i=0;i < random_number;i++){
				if(updatectl->next!=NULL){
					
					updatectl=updatectl->next;
					if(updatectl==next_head_of_contrlpkt){
						updatectl=current_head_of_contrlpkt;
					}//if
				}//if !=NULL
				else{

				}//if
			}//for
			
	
		
		
		}//randomly choose
		if(select_candidate_method==2){//minimus offset first		
			for(Tempctl=current_head_of_contrlpkt;Tempctl!=next_head_of_contrlpkt;Tempctl=Tempctl->next){
	    			if(Tempctl->update==0){
			
					if(Tempctl->offset < (unsigned int)min_offs){
						min_offs=Tempctl->offset;
						updatectl=Tempctl;
					}

			    
				
				}//if Tempctl->update==0 
			
			}//for
		
		}//minimus offset first
		
		if(select_candidate_method==3){//maximum offset first		
			for(Tempctl=current_head_of_contrlpkt;Tempctl!=next_head_of_contrlpkt;Tempctl=Tempctl->next){
	    			if(Tempctl->update==0){
			
					if(Tempctl->offset > (unsigned int)max_offs){
						max_offs=Tempctl->offset;
						updatectl=Tempctl;
					}

			    
				
				}//if Tempctl->update==0 
			
			}//for
		
		}//maximum offset first

		
		for(Tempctl=current_head_of_contrlpkt;Tempctl!=next_head_of_contrlpkt;Tempctl=Tempctl->next){
	    		if(Tempctl->update== 1 ){
			 if(Tempctl->burstid!=updatectl->burstid){	
				if(Tempctl->nextport==updatectl->nextport){
						
					if(Tempctl->wavelength==updatectl->wavelength){
						if((Tempctl->offset+Tempctl->reserve_dur) > updatectl->offset){
							updatectl->offset=(Tempctl->offset+Tempctl->reserve_dur);
							updatectl->reserve_dur=updatectl->reserve_dur - (Tempctl->offset+Tempctl->reserve_dur - updatectl->offset);
				        //update datapkt
							update_datapkt(updatectl);
						}//if
					}//if
						
				}//if
			  }// != burstid	
		  	}//if Tempctl->update==1
			
		}//for

		/* no need to compare with itself*/
		updatectl->update=1;

		
		
	}//for
		
			
	}//update
	


	//0129
	u_int64_t obsw::add_datapkt_transmit_time_to_contrlpkt(int burst_id,u_int32_t source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t  data_pkt_transmit_time){

		//find the related contrlpkt
		
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){//traverse every node in list
				if(tempctl->burstid==burst_id){//
					if((source==tempctl->src)&&(destination==tempctl->dst)){//
						if(NextPort==tempctl->nextport){
							if(wave==tempctl->wavelength){
							
								
								//found 
								tempctl->used_dur=tempctl->used_dur + data_pkt_transmit_time;//add new datapkt transmit time
								return	tempctl->used_dur;
												
							}//if	
						}//if
					}//if	
				}//if
			
				
			}//for
		}//if !=NULL	



		return ~0;
		
	}
	u_long obsw::get_new_OFFS(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave){
		

		/*first judge if there is the other packets with same arriving time*/
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){//traverse every node in list
				if(tempctl->burstid==burst_id){//
					if((source==tempctl->src)&&(destination==tempctl->dst)){//
						if(NextPort==tempctl->nextport){
							if(wave==tempctl->wavelength){
							
								
								return tempctl->offset;
								
												
							}//if	
						}//if
					}//if	
				}//if
			
				
			}//for
		}//if !=NULL	
		
		return ~0;//if we didn't find related offset
	}
	u_int64_t obsw::get_new_rdur(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave){
		
		
		/*first judge if there is the other packets with same arriving time*/
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){//traverse every node in list
				if(tempctl->burstid==burst_id){//
					if((source==tempctl->src)&&(destination==tempctl->dst)){//
						if(NextPort==tempctl->nextport){
							if(wave==tempctl->wavelength){
							
											
								return tempctl->reserve_dur;
								
												
							}//if	
						}//if
					}//if	
				}//if
			
				
			}//for
		}//if !=NULL	

		return ~0;//if we didn't find related reserve time
		
	}

	int obsw::deal_control_pkt_with_same_arrive_time(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave){
			
		u_int64_t	found_arrive_time=0;
		//int		there_is_contrlpkt_arrive_at_same_time=0;	
		int		number_of_same_arrive_time=0;
		
		/*first judge if there is the other packets with same arriving time*/
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){//traverse every node in list
			if(tempctl->have_found==0){//first found
				if(tempctl->burstid==burst_id){//find the related datapkt record which recorded by control packet
					if((source==tempctl->src)&&(destination==tempctl->dst)){//if src && dst match
						if(NextPort==tempctl->nextport){
							if(wave==tempctl->wavelength){
							//get the tempctl control packet arrive time
								
								found_arrive_time=tempctl->arrive_time;
								break;
												
							}//if	
						}//if
					}//if	
				}//if
			}//if
				
			}//for
		}//if !=NULL	
		
		if(found_arrive_time==0){//represent  we found this control pkt before
			return -1; // not first control pkt of several control pkt with same arrive time
		}

		//find the same arrive time
		//we will get the current head of controlpkt and next head of control pkt
		//and get the number of control pkt with same arrive time
		if(head_of_contrlpkt!=NULL){
			for(tempctl=head_of_contrlpkt;tempctl!=NULL;tempctl=tempctl->next){//traverse every node in list
				if(tempctl->have_found==0){//first found
							//get the tempctl control packet arrive time
					if(tempctl->arrive_time==found_arrive_time){				
						if(number_of_same_arrive_time > 0){
						}
						//if it is first node with same arrive time
						//let it be current head of control packet
						if(number_of_same_arrive_time==0){
							current_head_of_contrlpkt=tempctl;
							
							Tctl=tempctl;
							
						}//if
						
						number_of_same_arrive_time++;
						//alter have found
						tempctl->have_found=1;
						//we pretend next node will be different arrive time
						current_tail_of_contrlpkt=tempctl;
						next_head_of_contrlpkt=tempctl->next;	
					}//if							
				}//if
			
			}//for
		}//if !=NULL	

		
		if(number_of_same_arrive_time==1){
			return 0;// only one control pkt with the arrive time	
		}
		if(number_of_same_arrive_time > 1){
			
			/*  !!!!!! select  */
			update_record_of_control_pkt(current_head_of_contrlpkt,next_head_of_contrlpkt,number_of_same_arrive_time);

			return 1;
			//first control pkt of several control pkt with same arrive time
		}
		return 1;
	}
	

	//??????????????????????????????????????????????????
    int obsw::time_to_send_pkt(ePacket_	*pkt){	

	    

	Packet	*p=(Packet *)pkt->DataInfo_;

	struct lheader          *lh;
	char		*from;
	int		next_port;
	int		next_wave;
	int		from_wave;
	char		*temp;
	char		*nextport;
	int		*oldwave;
	u_long    	*burst_id_;
	u_int32_t	source;
	u_int32_t	destination;
	int		several_contrl_same_time;
	u_long		New_OFFS;
	u_int64_t	New_duration;
	int		New_DUR_LEN;
	int		process_time_;
	
	process_time_=ContrlPktProcessTime;
	
	// down  need oswa!!!!!!!!
	lh=(struct lheader *)p->pkt_get();

	destination=lh->dstIP;	//get every incoming packet's destination
	source=lh->srcIP;	//get every incoming packet's source
	
	from = p->pkt_getinfo("From");
	oldwave=(int *)p->pkt_getinfo("OLDW");
	
	u_int64_t                       current_time;
	u_int64_t                       contrlpkt_arrive_time;

	burst_id_=(u_long*)p->pkt_getinfo("SEQ");

	current_time=GetCurrentTime();
			
	//pass from port and from wave to get the to port , to wave
	next_port=readpath_->SW_get_nextport(get_nid(),from[0],*oldwave);	
	
	//!!!@@@  IF WAVE doesn't change use ordinary wave
	next_wave=lh->wave;
	from_wave=lh->wave;
	temp=(char *)&next_port;
	nextport=strdup(temp);
	//add to the packet , 
	//but why one should pass with address and one can't not
	p->pkt_addinfo("Next", nextport, 1);
	lh->wave=next_wave;

	free(nextport);


	/*0115 deal with control packet arrive at same time*/
	several_contrl_same_time=deal_control_pkt_with_same_arrive_time(*burst_id_,source,destination,next_port,next_wave);


	if(several_contrl_same_time!=0){//several control pkt in same time
		
		
		//we save offset before packet processing , so we should minus processing time
		//when we get saved new offset after packet processing time
		New_OFFS=get_new_OFFS(*burst_id_,source,destination,next_port,next_wave);
		New_OFFS=New_OFFS - process_time_;//we should minus process time due to process time pass by
		New_duration=get_new_rdur(*burst_id_,source,destination,next_port,next_wave);
		New_DUR_LEN=calculate_transmit_bytes(from[0],from_wave,New_duration);
		p->pkt_addinfo("OFFS",(char *) &New_OFFS , sizeof(u_long));
		p->pkt_addinfo("r_dur",(char *)&New_DUR_LEN,sizeof(int));
	}




	/*free controlpkt before send it out*/	
	contrlpkt_arrive_time=GetCurrentTime() - process_time_;

	Free_contrlpkt(*burst_id_,source,destination,next_port,next_wave,contrlpkt_arrive_time);

	return NslObject::send(pkt);

	
    }

    int obsw::SendTailDrop_ControlPacket(ePacket_ *pkt){


	    

	Packet	*p=(Packet *)pkt->DataInfo_;
	assert(pkt);

	struct lheader          *lh;
	char		*from;
	int		next_port;
	char		*temp;
	char		*nextport;
	int		*oldwave;
	u_long    	*burst_id_;
	u_int32_t	source;
	u_int32_t	destination;
	int		process_time_;
	
	process_time_=ContrlPktProcessTime;
	
	// down  need oswa!!!!!!!!
	lh=(struct lheader *)p->pkt_get();
	destination=lh->dstIP;	//get every incoming packet's destination
	source=lh->srcIP;	//get every incoming packet's source
	burst_id_=(u_long*)p->pkt_getinfo("SEQ");
	
	from = p->pkt_getinfo("From");
	oldwave=(int *)p->pkt_getinfo("OLDW");
	

	next_port=readpath_->SW_get_nextport(get_nid(),from[0],*oldwave);	
	temp=(char *)&next_port;
	nextport=strdup(temp);
	p->pkt_addinfo("Next", nextport, 1);
	free(nextport);	
	return NslObject::send(pkt);

    }

    int obsw::init(){
	    
	    	return(1);  
    	
    }
    
    int obsw::recv(ePacket_ *pkt){
    	
	return NslObject::recv(pkt);
    }



	//get the control packet
	//send by port & wave according to shortest path 
	//
	//
    int obsw::send(ePacket_ *pkt){
    
	Event		*ep2;    
	Packet		*p=(Packet *)pkt->DataInfo_;
	char		*from;
	char		*FromPort;
	char		*burst_type_;
	int		burst_type;
	int     	process_time_;
	u_int64_t	useful_time;
	u_int64_t	UseTime;
	char		*nextport;
	int		NextPort;
	u_int32_t	source;
	u_int32_t	destination;
	u_int8_t	wavelength;
	u_long		*burst_id_;
	struct	lheader		*lh;
	struct	datapacket	*freedatapkt;	
	
	
	burst_type_=p->pkt_getinfo("OBS");//get the burst type to see if this packet is control or data
	                                         //control packet->"1" data packet->"2" so value is just one char
	
	burst_type=burst_type_[0];
	
	burst_id_=(u_long*)p->pkt_getinfo("SEQ");
	lh = (struct lheader *)p->pkt_get();
	
	destination=lh->dstIP;	//get every incoming packet's destination
	source=lh->srcIP;	//get every incoming packet's source
	wavelength=lh->wave;	//get every incoming packet's mapped wavelength	
	
	FromPort = p->pkt_getinfo("From");
	
	if(burst_type_==NULL){
	
		return NslObject::send(pkt);
	}
	if(burst_type_!=NULL){

		//get the process_time in the control packet
		
		//we default set it to 1
		process_time_=ContrlPktProcessTime;
		
		
		if(burst_type==1){//control packet
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			//send the control packet after a duration (control packet processing time)
			
			
			lh = (struct lheader *)p->pkt_get();
			from = p->pkt_getinfo("From");
			int	node_current_time;
			
			
			node_current_time=GetCurrentTime();
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(obsw,time_to_send_pkt);
			setObjEvent(pkt,GetCurrentTime()+process_time_,0,this,type,(void *)pkt->DataInfo_);
		}//control
		else if(burst_type==2){//data packet
			//pass from port and from wave to get the to port , to wave
			

			nextport=p->pkt_getinfo("Next");
			if(nextport!=NULL){
				NextPort=nextport[0];
			}
		
			
			/*SECOND method*/
			/*
			 * set an event , so we get data pkt then we free datapacket recorded node
			 * after 2*(duration+offset ) 
			 * because we won't used this datapacket after 2*(duration+offset)
			 */

			
			freedatapkt= (struct datapacket *) malloc(sizeof(struct datapacket));
			assert(freedatapkt);
			
			freedatapkt->burstid=*burst_id_;
			freedatapkt->wavelength=wavelength;
			freedatapkt->nextport=NextPort;
			freedatapkt->src=source;
			freedatapkt->dst=destination;
			
			// time data is useless
			freedatapkt->node_current_time=GetCurrentTime();		
			freedatapkt->start_transmit_time=GetCurrentTime();		
			freedatapkt->end_transmit_time=GetCurrentTime();
			freedatapkt->next=NULL;
			freedatapkt->prev=NULL;

			UseTime=Get_useful_time(*burst_id_,source,destination,NextPort,wavelength);
			if(UseTime>0){
				useful_time=UseTime * 3;
			
		
				
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(obsw,Free_datapacket2);
				ep2=createEvent();
				mem++;
				setObjEvent(ep2,GetCurrentTime()+useful_time,0,this,type,(void *)freedatapkt);
			}
			return NslObject::send(pkt);
		}//data
		else if(burst_type==3){//tail drop control packet
			/*
			 *send out tail drop control packet after process time
			 *change Next info according by control packet nextport 
			 */
			
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(obsw,SendTailDrop_ControlPacket);
			setObjEvent(pkt,GetCurrentTime()+process_time_,0,this,type,(void *)pkt->DataInfo_);
			
		
		}
	}
	return 1;
    } 
     
    int obsw::get(ePacket_ *pkt, MBinder *frm){

	char 		*burst_type_;
	char		*FromPort;
	int		burst_type;
	u_long     	*offset_;
	u_long     	process_time_;
	u_long		*burst_id_;
	u_int64_t	duration_;
	u_int64_t	contrl_pkt_transmit_time;
	u_int32_t	source;
	u_int32_t	destination;
	u_int8_t	wavelength;
	u_int64_t	node_current_time;
	u_int64_t	control_pkt_recv_time;
	u_long		old_offset;
	u_long		current_offset;
	u_long		new_offset;
	int		default_offset=300;
	int		*OldWave;
	int		*DataWave;
	int		NextPort;
	int		New_DUR_LEN;
	int		*DURATION_length;
	//?? can we use to get the src and dst from control packet and data packet, where we put packet data 
	Packet	*p=(Packet *)pkt->DataInfo_;
	
	struct	lheader	*lh;
	lh=(struct lheader *)p->pkt_get();
	destination=lh->dstIP;	//get every incoming packet's destination
	source=lh->srcIP;	//get every incoming packet's source
	wavelength=lh->wave;	//get every incoming packet's mapped wavelength	
	
	
	FromPort = p->pkt_getinfo("From");
	burst_type_=p->pkt_getinfo("OBS");//get the burst type to see if this packet is control or data
	                                          //control packet->"1" data packet->"2" so value is just one char
	burst_type=burst_type_[0];
	//printf("%d obsw\n",get_nid());	
	
	if(burst_type_==NULL){
	
		return NslObject::get(pkt,frm);	
	}//normal packet
	if(burst_type_!=NULL){
	
	
	
	 if(burst_type==1){	//control packet
				//***send by shortest path , send out in obsw module****
		
		
		//get the system time , record the reserved time
		offset_=(u_long*)p->pkt_getinfo("OFFS");//get the offset time of data packet in control packet
		//get the control packet processing time!!
	
		if(offset_==NULL){
			//printf("1231 offset <=0\n");
			offset_=(u_long*)default_offset;
		}
		
		OldWave=(int *)p->pkt_getinfo("OLDW");
		DataWave=(int *)p->pkt_getinfo("WAVE");
		burst_id_=(u_long*)p->pkt_getinfo("SEQ");
		
		DURATION_length=(int* )p->pkt_getinfo("r_dur");//get the reserve duration from control packet??????
		

		control_pkt_recv_time=GetCurrentTime();//get the current time

		
		process_time_=ContrlPktProcessTime;//??????????????????????default set it to 50
		
		//<CALL> calculate transmission time here with the value DURATION length	
		duration_=calculate_transmit_time(FromPort[0],wavelength,*DURATION_length);		

		
		/* we should reserve time by burst_id , wavelength ,and to_port */

		NextPort=readpath_->SW_get_nextport(get_nid(),FromPort[0],*OldWave);	
		
		/* we record control packet with arrive time */
		/*0116 we minus oldOFFS with contrl pkt transmit time */
		/* because we recv contrl pkt after tranmit time*/
		contrl_pkt_transmit_time=calculate_transmit_time(FromPort[0],wavelength,p->pkt_getlen());
		
		
		
		current_offset=*offset_ - contrl_pkt_transmit_time;
		u_int64_t	overlap_new_offset;
		overlap_new_offset=reserve_duration(current_offset,process_time_,duration_,*burst_id_,source,destination,*DataWave,NextPort,wavelength,*OldWave);//add control pkt wave & its oldwave
		
		if(overlap_new_offset!=0){	
			if(drop_burst_segmentation_method==1){
			
				current_offset=overlap_new_offset;
				New_DUR_LEN=0;
				p->pkt_addinfo("r_dur",(char *)&New_DUR_LEN,sizeof(int));

			}
			if(drop_burst_segmentation_method==2){
		
				duration_=duration_ -(overlap_new_offset - current_offset); 
				current_offset=overlap_new_offset;
				if(duration_ >=0 )
					New_DUR_LEN=calculate_transmit_bytes(FromPort[0],wavelength,duration_);
				if(duration_<0)
					New_DUR_LEN=0;
			
			
				p->pkt_addinfo("r_dur",(char *)&New_DUR_LEN,sizeof(int));
			}
		}	
	
		record_control_pkt_with_recv_time(*burst_id_,source,destination,current_offset,duration_,control_pkt_recv_time,NextPort,wavelength);
		
		
		
		//1223 if we get the offset value and deal with control packet within processing time
		//we should minus offset  by packet processing time
		
		//0116 because control packet is store and forward so we should minus contrl pkt transmit time
		
		
		
		if(overlap_new_offset==0){
			old_offset=*offset_;
			new_offset=old_offset-process_time_-contrl_pkt_transmit_time;
		}
		if(overlap_new_offset!=0){
			new_offset=current_offset - process_time_;
			
		}
		p->pkt_addinfo("OFFS",(char *)&new_offset,sizeof(u_long));
		



		p->pkt_setflow(PF_SEND);
		return  send(pkt);
	 }//if type==1
	 else if(burst_type==2){	//data packet
		 			//***send up , call recv and let osw deal with it 
		
		node_current_time=GetCurrentTime();//get the current time
		
		burst_id_=(u_long* )p->pkt_getinfo("SEQ");
               	int	*oldwave;
		int	next_port;
		
		oldwave=(int *)p->pkt_getinfo("OLDW");
		

		next_port=readpath_->SW_get_nextport(get_nid(),FromPort[0],*oldwave);
						
		
		
		if(reserve_for_current_packet(node_current_time,*burst_id_,source,destination,next_port,wavelength)){
                	//reserve_for_current_packet() see if we have reservation for current packet
			//if true return 1 , if not return 0
			
			
			return NslObject::get(pkt,frm);//send out the packet 
                }
                else {
			
			printf("%d OBSW GET <FREE1>we drop data packet call freepacket burst_id=%lu current time=%llu \n",get_nid(),*burst_id_,GetCurrentTime());
		
			
			/*
			 * set an event , so we get data pkt then we free datapacket recorded node
			 * after 2*(duration+offset ) 
			 * because we won't used this datapacket after 2*(duration+offset)
			 */
				
			Event			*ep2;    
			struct	datapacket	*freedatapkt;	
			u_int64_t	useful_time;
			u_int64_t	UseTime;
			
			freedatapkt= (struct datapacket *) malloc(sizeof(struct datapacket));
			assert(freedatapkt);
			
			freedatapkt->burstid=*burst_id_;
			freedatapkt->wavelength=wavelength;
			freedatapkt->nextport=next_port;
			freedatapkt->src=source;
			freedatapkt->dst=destination;
			
			// time data is useless
			freedatapkt->node_current_time=GetCurrentTime();		
			freedatapkt->start_transmit_time=GetCurrentTime();		
			freedatapkt->end_transmit_time=GetCurrentTime();
			freedatapkt->next=NULL;
			freedatapkt->prev=NULL;

			UseTime=Get_useful_time(*burst_id_,source,destination,next_port,wavelength);
				if(UseTime>0){
				useful_time=UseTime * 3;
				if(useful_time<=0){
					printf("%d useful time=%llu\n",get_nid(),useful_time);		
				}
		
				
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(obsw,Free_datapacket2);
				ep2=createEvent();
				mem++;
				setObjEvent(ep2,GetCurrentTime()+useful_time,0,this,type,(void *)freedatapkt);
			}
			freePacket(pkt);
		
		}
		
		return (1);
	 }//if type==2
	 else if(burst_type==3){	//tail drop control packet
		/*
		 * if we receive tail drop packet
		 * we should change reserve duration in datapackets and controlpkts	
		 * and send it out after packet process time
		 */
		int	*TD_new_duration;
		int	*ControlWave;
		
		OldWave=(int *)p->pkt_getinfo("OLDW");
		burst_id_=(u_long*)p->pkt_getinfo("SEQ");
		DataWave=(int *)p->pkt_getinfo("WAVE");
		ControlWave=(int *)p->pkt_getinfo("Cwave");
		
		NextPort=readpath_->SW_get_nextport(get_nid(),FromPort[0],*OldWave);	
		TD_new_duration=(int* )p->pkt_getinfo("r_dur");
		update_prevOL_datapkt(*burst_id_,source,destination,NextPort,*DataWave,*TD_new_duration);
		update_prevOL_contrlpkt(*burst_id_,source,destination,NextPort,*ControlWave,*TD_new_duration);
		p->pkt_setflow(PF_SEND);
		return  send(pkt);
	 }
	}//obs packet
	

    	
	return 1;
     }
