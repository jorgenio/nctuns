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

#ifndef __NCTUNS_obsw_h__
#define __NCTUNS_obsw_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <module/opt/Lheader.h>
#include <module/opt/readpath.h>
#include <assert.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <maptable.h>
#include <nodetype.h>
#include <stdlib.h>
#include <gbind.h>


struct datapacket{
	int burstid;
	u_int64_t	node_current_time;		
	u_int64_t	start_transmit_time;		
	u_int64_t	end_transmit_time;
	u_int8_t	wavelength;
	int		nextport;
	int		size;
	u_int32_t	src;
	u_int32_t	dst;
	struct	datapacket	*next;
	struct	datapacket	*prev;

	
};



struct controlpkt{
	int 		burstid;
	u_int32_t	src;
	u_int32_t	dst;
	u_long		offset;
	u_int8_t	wavelength;
	int		nextport;
	u_int64_t	reserve_dur;	
	u_int64_t	used_dur;	
	u_int64_t	arrive_time;
	int		size;
	int		have_found;
	int		update;
	int		been_divided;//been divided into an same arrive time same port same wave list
	int		list_number;//give those same arrive time controlpkts different number by port and wav
	struct	controlpkt	*next;
	struct	controlpkt	*prev;
};

class obsw : public NslObject {

 private: 
	
	datapacket	*head_of_datapkt;
	datapacket	*tail_of_datapkt;

	datapacket	*Ftemppkt;
	datapacket	*freepkt;
	datapacket	*fprevpkt;
	datapacket	*fnextpkt;
	
	//we sould put below datapkt variables here 
	//because we only need one temppkt, prevpkt, nextpkt
	datapacket	*temppkt;
	datapacket	*prevpkt;
	datapacket	*nextpkt;
	datapacket	*findpkt;
	double		*bw;

	u_int64_t       pre_processing_arrive_time;
	int		select_candidate_method;//1:random choice  2:minimus offset first 	
	int		drop_burst_segmentation_method;
	int		ContrlPktProcessTime;
	int		TailDrop_control_wavelength;
	int		clistlen;
	int		dlistlen;
	int		mem;
	controlpkt	*head_of_contrlpkt;
	controlpkt	*tail_of_contrlpkt;
	controlpkt	*current_head_of_contrlpkt;
	controlpkt	*current_tail_of_contrlpkt;
	controlpkt	*next_head_of_contrlpkt;
	
	controlpkt	*Ftempctl;
	controlpkt	*freectl;
	controlpkt	*fprevctl;
	controlpkt	*fnextctl;
	controlpkt	*tempctl;
	controlpkt	*Tempctl;
	controlpkt	*prevctl;
	controlpkt	*nextctl;
	controlpkt	*findctl;
	controlpkt	*updatectl;
	controlpkt      *Tctl;
	controlpkt      *tctl;
	controlpkt      *tctl_;
	controlpkt      *list_of_updatectl_head[100];
	controlpkt      *list_of_updatectl_end[100];
	u_int64_t	prev_recv_time; 

 protected:

 public:
 	obsw(u_int32_t type, u_int32_t id, struct plist* pl , const char *name); 
 	~obsw();   

	u_int64_t 		check_if_reserve_time_overlap(u_int64_t  start_time,int NextPort,int WaveLength,int burst_id,u_int8_t contrlWave,int  oldwave);
	
	
	u_int64_t		calculate_transmit_time(char from_port,int from_wave,int burst_bytes);

	int 			update_prevOL_datapkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t new_duration);

	int 			update_prevOL_contrlpkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t new_duration);		

	int 			Send_TailDrop_control_packet(Event  *ep);
	
	u_int64_t 		reserve_duration(int offset,int process_time,u_int64_t  duration_time ,int burst_id, u_int32_t  src, u_int32_t dst, int wave,int  NextPort,u_int8_t contrlWave,int  oldwave);

	
	int                     calculate_transmit_bytes(char from_port,int from_wave,int burst_duration);
	
	void 			record_control_pkt_with_recv_time(int burst_id,u_int32_t source,u_int32_t destination,int offset,u_int64_t duration,u_int64_t control_pkt_arrive_time,int NextPort,u_int8_t wave);
	
	void			Free_contrlpkt(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t  wave,u_int64_t contrlpkt_arrive_time);


 	u_int64_t 		Get_useful_time(int burst_id,u_int32_t  source,u_int32_t  destination,int NextPort,u_int8_t wave);
	 
	
	int 			Free_datapacket2(Event_ *ep2);

	
	void 			update_datapkt(struct controlpkt * update);
	
	void 			update_record_of_control_pkt(struct controlpkt *current_head_of_contrlpkt ,struct controlpkt *next_head_of_contrlpkt,int number_of_contrl);
	
	
	 u_int64_t 		add_datapkt_transmit_time_to_contrlpkt(int burst_id,u_int32_t source,u_int32_t  destination,int NextPort,u_int8_t wave,u_int64_t  data_pkt_transmit_time);
	
	u_long 			get_new_OFFS(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave);
	
	u_int64_t 		get_new_rdur(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave);
	
	int 			deal_control_pkt_with_same_arrive_time(int burst_id,u_int32_t source,u_int32_t destination,int NextPort,u_int8_t wave);
	
	
	
        int			time_to_send_pkt(ePacket_  *pkt);
       
	int 			SendTailDrop_ControlPacket(ePacket_ *pkt);
	
	int			reserve_for_current_packet(u_int64_t node_current_time,int  burst_id, u_int32_t  src, u_int32_t dst,int NextPort,u_int8_t wave);
	
	int 			init(); 
	int			recv(ePacket_ *pkt);
	
        int                     get(ePacket_ *pkt, MBinder *frm);
        int                     send(ePacket_ *pkt);
}; 
 

#endif /* __NCTUNS_obsw_h__ */
