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

#ifndef __NCTUNS_obwa_h__
#define __NCTUNS_obwa_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <module/opt/Lheader.h>
#include <module/opt/readpath.h>

//data store in event
struct Buffer_event_info{
	u_long			 AssembleTimer_seqnum;	
	u_long			 sequencenum;	
	u_long 			 next;
	int			 wave;
	double			 sendlength;
	int 			 output_port;  //add 01/16
};


//used for checking the wavelength is assigned or not
struct Wave_assign_table{
	int			  send;
	int			  wave;
	int			  oldwave;
	int 			  output_port; //add 01/16	
    	u_long			  SeqID; //asssisn Id according to different buffer(equal to router destination)
    	struct Wave_assign_table  *next;
};

/*struct ePacket_{
	int num;
} ; */

struct Entity{
	Entity * next;
	ePacket_  *pkt;
};

struct buffer_list{
	int entity_num;
	int full;			//if packet can't put any more packet
	int empty;
	u_long AssembleTimer_active_seq;
	u_long goal;
	Entity* first;
	Entity* last;
	u_long sequ_num;
	int packet_length;
	buffer_list* next;
	long adjust_length;
};

struct buffer_head{
	int list_num;
	buffer_list* first;
    	buffer_list* last;
};

class obwa : public NslObject {

 private:
	char *			nodekindfile;
	char *			nodeconnectfile;
	char *			ringfile;
	char *			nodepathfile;
	int			ControlPacket_wave;
        int			MAX_WAVE ; 
	int 			choose_wave;	// force to shoose wave
	int 			no_use;	// force to shoose wave
  	double 			Timeout; 	//user assigned
  	double 			Offset ; 	//user	assigned
	double			Process_time;
	double			Send_packet_length;
	double			Drop_packet_length;

	/* above variable is modified by user
	 * */

	buffer_head * 		Buffer_head;
	u_long 			AssembleTimer_seqnum;
	u_long 			Send_seq;
	Wave_assign_table *	WAT;		//point to head of Wave_assign_table

 public:
 	obwa(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~obwa();   

	int 			init(); 
	
	int                     get(ePacket_ *pkt, MBinder *frm);
	int                     send(ePacket_ *pkt);
        int 			recv(ePacket_ *pkt);

	u_int64_t		Calculate_trans_time(int length ,int wave);
	int 			Calculate_Propdelay_time(int length, int wave);
	void 			Send_release_wave_Event(int wave,int output_port ,u_int64_t delayTick);
	
	int 			Is_Exist_seq(u_long seq);		//no use
	
	void 			addto_WA_table(u_long seqnum ,int wave , int oldwave, int output_port);
	int 			availwave_in_WA_table(int output_port);
	int 			Is_avail_wave(int wave,int output_port);
	
	int 			Get_wave_by_seq(u_long seq);
	void			release_wave(Event_ *ep);
	void 			delwave_byseq(u_long seq);
	void 			delete_event_byseq(u_long seq);		//added in 12/22
	int			Is_sent_event(u_long seq);		//added in 12/22
	int 			Get_oldwave_by_seq(u_long seq);
	
	int  			Is_AssembleTimer_running_goallist (u_long  goal);
	void  			Set_AssembleTimer_run_goallist (u_long  goal);
	void  			Set_AssembleTimer_stop_goallist (u_long  goal);
	int  			Is_Active_AssembleTimer_event (u_long  goal,u_long seq);
	
	void 			Set_goallist_full(u_long seq);
	int 			Is_goallist_empty(u_long seq);
	int 			Is_first_goallist (u_long  goal);
	u_long 			Is_available_goallist (u_long  goal);
	buffer_list * 		Is_Exist_goal (u_long goal);
	buffer_list * 		Is_Exist_list (u_long seq_num);
	buffer_list * 		add_new_list(u_long goal);
	void 			addto_list(ePacket_* epacket, buffer_list * ptr );

	int 			putinto_buffer(ePacket_* epacket,u_long next);
	int			send_control_packet(Event_ *ep);
	void			Schedule_send(ePacket_ *ep);
	int 			release_buffer(Event_ *ep);
	int 			delete_buffer(u_long sequencenum);
}; 
 

#endif /* __NCTUNS_obwa_h__ */
