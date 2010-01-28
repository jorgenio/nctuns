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

#ifndef __NCTUNS_SAC_H__
#define __NCTUNS_SAC_H__

#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <math.h>
#include <satellite/dvb_rcs/common/sch_info.h>

class           Sac
{
	friend class    Ncc_ctl;
      public:
	static const uint8_t NO_CAPACITY_REQUEST = 255;

	enum Scaling_factor_value
	{
		SCALING_FACTOR_1	= 1,
		SCALING_FACTOR_16	= 16
	};

	enum Capacity_request_type_value
	{
		CAPACITY_REQUEST_TYPE_VALUE_VBDC	= 0,
		CAPACITY_REQUEST_TYPE_VALUE_RBDC	= 1,
		CAPACITY_REQUEST_TYPE_VALUE_AVBDC	= 2
	};

	enum M_and_c_message_value
	{
		M_AND_C_MESSAGE_VALUE_NO_MESSAGE			= 0x0000,
		M_AND_C_MESSAGE_VALUE_FINE_SYNCHRONIZATION_ACHIEVED	= 0x0001,
		M_AND_C_MESSAGE_VALUE_LOG_OFF_REQUEST			= 0x0002
	};

	struct Capacity_request
	{
		Scaling_factor_value		scaling_factor;
		Capacity_request_type_value	capacity_request_type;
		uint8_t        			channel_id;
		uint8_t         		capacity_request_value;
	} capacity_requests[8];

      private:
	bool 		route_id_flag;	//Indicate whether the route_id field is defined.
	bool            request_flag;	//Indicate whether the capacity_request field(s) is/are defined.
	bool            m_and_c_flag;	//Indicate whether the m_and_c_message field is defined.
	bool            group_id_flag;	//Indicate whether the group_id field is defined.
	bool            logon_id_flag;	//Indicate whether the logon_id field is defined.
	bool            acm_flag;	//Indicate whether the acm field is defined.
	/*
	   If request_flag==1, capacity_request_number Specifies 
	   one less than the number of capacity request(s) as it do in Spec.
	 */
	uint8_t         capacity_request_number;
	uint16_t        route_id;
	uint8_t         group_id;
	uint16_t        logon_id;
	M_and_c_message_value	m_and_c_message;

	struct Acm
	{
		uint8_t         cni;
		uint8_t         modcod_rq;
	} acm;

      public:
	                Sac ();

			/* The Sac() constructor below is used to convert a SAC message defined
			   in Spec to the Sac struct. The flags and the capacity_request_number
			   should be given for interpreting the SAC message.
			 */
			Sac (const void *sac_message,
			     bool route_id_flag,
			     bool request_flag,
			     uint8_t capacity_request_number,
			     bool m_and_c_flag,
			     bool group_id_flag,
			     bool logon_id_flag,
			     bool acm_flag);

	/************************************************************
	 * gen_sac_message() is used to construct a SAC message 
	 * defined in Spec from the Sac struct. The length of
	 * the SAC message is returned in the parameter.
	 ***********************************************************/
	void*		gen_sac_message (uint8_t& sac_length);
	
	static uint8_t	sac_length(Slot_flags flags);

	static uint8_t	sac_length(bool route_id_flag, 
				   bool request_flag, 
				   uint8_t capacity_request_number,
				   bool m_and_c_flag,
				   bool group_id_flag,
				   bool logon_id_flag,
				   bool acm_flag);
	
	uint8_t		sac_length();

	bool		req_full();

	int		fill_req(Capacity_request_type_value type, 
				 uint32_t value, 
				 uint8_t channel_id,
				 uint8_t max_capacity_request_number);

	void		pad_dummy_req(uint8_t max_capacity_request_number);

/****** Functions below are defined for setting subfield *******/
	inline void	set_route_id_flag(bool flag)
	{
		route_id_flag = flag;
	}

	inline void	set_request_flag(bool flag)
	{
		request_flag = flag;
	}

	/*
	inline void	set_capacity_request_number(uint8_t num)
	{
		capacity_request_number = num;
	}
	*/

	inline void	set_m_and_c_flag(bool flag)
	{
		m_and_c_flag = flag;
	}

	inline void	set_group_id_flag(bool flag)
	{
		group_id_flag = flag;
	}

	inline void	set_logon_id_flag(bool flag)
	{
		logon_id_flag = flag;
	}

	inline void	set_acm_flag(bool flag)
	{
		acm_flag = flag;
	}

	inline int      set_route_id (uint16_t r_id)
	{
		route_id_flag = 0;
		route_id = r_id;
		return 0;
	}

	int             add_capacity_request (Scaling_factor_value
					      sc_factor,
					      Capacity_request_type_value
					      c_r_type, uint8_t ch_id,
					      uint8_t c_r_value);

	inline int      set_m_and_c_message (M_and_c_message_value m_value)
	{
		m_and_c_flag = 1;
		m_and_c_message = m_value;
		return 0;
	}

	inline int      set_group_id (uint8_t g_id)
	{
		group_id_flag = 1;
		group_id = g_id;
		return 0;
	}

	inline int      set_logon_id (uint16_t l_id)
	{
		logon_id_flag = 1;
		logon_id = l_id;
		return 0;
	}

	inline int      set_acm (uint8_t cni, uint8_t modcod_rq)
	{
		acm_flag = 0;
		acm.cni = cni;
		acm.modcod_rq = modcod_rq;
		return 0;
	}

/**************************************************************/

/****** Functions below are defined for drawing subfield from SAC *******/

/* Note: These functions return 0 if the subfield specified is defined.Else,
			-1 will be returned.
*/
	inline int      get_route_id (uint16_t & rid) const
	{
		if (route_id_flag == 0)
		{
			rid = route_id;
			return 0;
		}
		else
		{
			return -1;
		}
	}

	inline int      get_capacity_request_number() const
	{
		return capacity_request_number;
	}

	int             pop_capacity_request (Scaling_factor_value &
					      sc_factor,
					      Capacity_request_type_value &
					      c_r_type, uint8_t & ch_id,
					      uint8_t & c_r_value);

	int		pop_capacity_request (Capacity_request& c_r);

	inline int      get_m_and_c_message (M_and_c_message_value &
					     m_and_c) const
	{
		if (m_and_c_flag == 1)
		{
			m_and_c = m_and_c_message;
			return 0;
		}
		else
		{
			return -1;
		}
	}

	inline int      get_group_id (uint8_t & g_id) const
	{
		if (group_id_flag == 1)
		{
			g_id = group_id;
			return 0;
		}
		else
		{
			return -1;
		}
	}

	inline int      get_logon_id (uint16_t & l_id) const
	{
		if (logon_id_flag == 1)
		{
			l_id = logon_id;
			return 0;
		}
		else
		{
			return -1;
		}
	}

	inline int      get_acm (uint8_t & cni, uint8_t & modcod_rq) const
	{
		if (acm_flag == 0)
		{
			cni = acm.cni;
			modcod_rq = acm.modcod_rq;
			return 0;
		}
		else
		{
			return -1;
		}
	}

	void show_sac();

/************************************************************************/

};

#endif	/*__NCTUNS_SAC_H__*/
