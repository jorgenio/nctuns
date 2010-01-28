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

#ifndef __NCTUNS_logon_initialize_descriptor_h__
#define __NCTUNS_logon_initialize_descriptor_h__

#include "descriptor.h"

/*
 * when _Connectivity == "0"
 * we will use this object
 */
class Ip_connectivity {

 friend class Atm_trf_traffic;
 friend class Logon_initialize_descriptor;
 
 private:
	uint16_t		_Return_VPI;
	uint16_t		_Return_VCI;
};

/*
 * when _Connectivity == "1"
 * we will use this object
 */
class Atm_connectivity {

 friend class Atm_trf_traffic;
 friend class Logon_initialize_descriptor;

 private:
	uint16_t		_Return_signaling_VPI;
	uint16_t		_Return_signaling_VCI;
	uint16_t		_Forward_signaling_VPI;
	uint16_t		_Forward_signaling_VCI;
};

/*
 * when _Traffic_burst_type == "0"
 * we will use this object
 */

class Atm_traffic {
	
 friend class Logon_initialize_descriptor;
 
 private:
	u_char			_Connectivity;
	void			*_connectivity_pointer;
};

/*
 * when _Traffic_burst_type == "1"
 * we will use this object
 */
class Mpeg2_ts_traffic {

 friend class Logon_initialize_descriptor;
 
 private:
	uint16_t		_Return_TRF_PID;
	uint16_t		_Return_CTRL_MNGM_PID;
};

class Capacity {

 friend class Logon_initialize_descriptor;

 private:
	u_char			_CRA_level[3];
	uint16_t		_VBDC_max;
	u_char			_RBDC_max[3];
	uint16_t		_RBDC_timeout;
};

class Logon_initialize_descriptor : public Descriptor {
	
 /*
  * We use 'friend' mechanism to let other classes can use the private
  * memebers of class Descriptor
  */
 friend class Section;

private:
	/*
	 * all fields are private member
	 * at first two field are 
 	 * descriptor_tag
	 * descriptor_length
	 */
	uint8_t			_Group_ID;
	uint16_t		_Logon_ID;
	
	u_char			_Security_handshake_required		:4;
	u_char			_Prefix_flag				:1;
	u_char			_Data_unit_labelling_flag		:1;
	u_char			_Mini_slot_flag				:1;
	u_char			_Contention_based_mini_slot_flag	:1;

	struct {

		u_char			_Capacity_type_flag		:2;
		u_char			_Traffic_burst_type		:1;
	}_traffic_information;

	void			*_traffic_type_pointer;
	Capacity		*_capacity_pointer;

	/*
	 * private function
	 */
	inline void				set_Capacity_type_flag(u_char set_capacity_type_flag){ 
						_traffic_information._Capacity_type_flag = set_capacity_type_flag;}
	inline void				set_Traffic_burst_type(u_char set_traffic_burst_type){ 
						_traffic_information._Traffic_burst_type = set_traffic_burst_type;}
	void					set_Connectivity(u_char);
 /*
  * public functions
  */
 public:
	Logon_initialize_descriptor(uint8_t , uint16_t , u_char , u_char , u_char , u_char , u_char , u_char , u_char , u_char); 
	Logon_initialize_descriptor(); 
	~Logon_initialize_descriptor(); 

	void					set_Return_VPI(uint16_t);
	uint16_t				get_Return_VPI();
	void					set_Return_VCI(uint16_t);
	uint16_t				get_Return_VCI();
	void					set_CRA_level(uint32_t cra_level_value);
	uint32_t				get_CRA_level();
	void					set_VBDC_max(uint16_t vbdc_max);
	uint16_t				get_VBDC_max();
	void					set_RBDC_max(uint32_t rbdc_max);
	uint32_t				get_RBDC_max();
	void					set_RBDC_timeout(uint16_t rbdc_timeout);
	uint16_t				get_RBDC_timeout();

	void					set_Return_signaling_VPI(uint16_t);
	uint16_t				get_Return_signaling_VPI();
	void					set_Return_signaling_VCI(uint16_t);
	uint16_t				get_Return_signaling_VCI();
	void					set_Forward_signaling_VPI(uint16_t);
	uint16_t				get_Forward_signaling_VPI();
	void					set_Forward_signaling_VCI(uint16_t);
	uint16_t				get_Forward_signaling_VCI();
	void					set_Return_TRF_PID(uint16_t);
	uint16_t				get_Return_TRF_PID();
	void					set_Return_CTRL_MNGM_PID(uint16_t);
	uint16_t				get_Return_CTRL_MNGM_PID();

	void					set_group_id(uint8_t g_id){ _Group_ID = g_id; }
	uint8_t					get_group_id(){ return _Group_ID; }
	uint16_t				get_logon_id(){ return _Logon_ID; }
	void					set_logon_id(uint16_t l_id){ _Logon_ID = l_id; }
	inline u_char				get_Capacity_type_flag(){ return _traffic_information._Capacity_type_flag;}
	inline u_char				get_Traffic_burst_type(){ return _traffic_information._Traffic_burst_type;}
	u_char					get_Connectivity();
	Logon_initialize_descriptor*		descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);
	bool					is_match(uint8_t group_id, uint16_t logon_id);
	void					show_descriptor();
};

#endif /* __NCTUNS_logon_initialize_descriptor_h__ */

