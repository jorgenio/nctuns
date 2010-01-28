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

#include "sac.h"

Sac::Sac ()
{
	memset ((void *) this, 0, sizeof (Sac));
	request_flag = m_and_c_flag = group_id_flag = logon_id_flag = 1;
	route_id_flag = acm_flag = 0;
	capacity_request_number = NO_CAPACITY_REQUEST;
}

Sac::Sac (const void *sac_message, bool route_id_flag,
	  bool request_flag, uint8_t capacity_request_number,
	  bool m_and_c_flag,
	  bool group_id_flag,
	  bool logon_id_flag, bool acm_flag)
{
	const void     *look_ptr;

	assert(capacity_request_number < 8);

	memset ((void *) this, 0, sizeof (Sac));
	look_ptr = sac_message;
	Sac::route_id_flag = route_id_flag;
	Sac::request_flag = request_flag;
	Sac::m_and_c_flag = m_and_c_flag;
	Sac::group_id_flag = group_id_flag;
	Sac::logon_id_flag = logon_id_flag;
	Sac::acm_flag = acm_flag;
	Sac::capacity_request_number = capacity_request_number;

	if (route_id_flag == 0)
	{
		uint16_t       *route_id_loc = (uint16_t *) look_ptr;

		route_id = *route_id_loc;
		look_ptr = (char *) route_id_loc + sizeof (uint16_t);
	}

	if (request_flag == 1)
	{
		uint8_t         request_index = 0;

		while (capacity_request_number != NO_CAPACITY_REQUEST)
		{
			Capacity_request	*c_r;
			char			one_byte;


			c_r = &(capacity_requests[request_index++]);

			one_byte = *((char *) look_ptr);

			c_r->scaling_factor = (one_byte & 0x01) ? SCALING_FACTOR_16 : SCALING_FACTOR_1;

			c_r->capacity_request_type =
				(Capacity_request_type_value) ((one_byte >> 1) & 7);

			c_r->channel_id = (one_byte >> 4) & 0x0F;

			one_byte = *((char *) look_ptr + 1);

			c_r->capacity_request_value = one_byte;

			look_ptr = (char *) look_ptr + 2;

			if (capacity_request_number == 0)
				capacity_request_number = NO_CAPACITY_REQUEST;
			else
				capacity_request_number--;
		}
	}

	if (m_and_c_flag == 1)
	{
		uint16_t       *m_and_c_loc = (uint16_t *) look_ptr;

		m_and_c_message = (M_and_c_message_value) (*m_and_c_loc);
		look_ptr = (char *) m_and_c_loc + sizeof (uint16_t);
	}

	if (group_id_flag == 1)
	{
		uint8_t        *group_id_loc = (uint8_t *) look_ptr;

		group_id = *group_id_loc;
		look_ptr = (char *) group_id_loc + sizeof (uint8_t);
	}

	if (logon_id_flag == 1)
	{
		uint16_t       *logon_id_loc = (uint16_t *) look_ptr;

		logon_id = *logon_id_loc;
		look_ptr = (char *) logon_id_loc + sizeof (uint16_t);
	}

	if (acm_flag == 0)
	{
		Acm            *acm_loc = (Acm *) look_ptr;

		acm.cni = acm_loc->cni;
		acm.modcod_rq = acm_loc->modcod_rq;
		look_ptr = (char *) acm_loc + sizeof (Acm);
	}

}

void*
Sac::gen_sac_message (uint8_t &length) 
{
	void		*sac_message_buf;
	uint8_t		sac_len;
	uint8_t         num_remainder_requests;


	length = sac_len = sac_length();

	sac_message_buf = (void*) new char[sac_len];

	if (request_flag == 1)
	{
		assert(capacity_request_number!=NO_CAPACITY_REQUEST);

		num_remainder_requests = capacity_request_number + 1;
	}
	else
	{
		num_remainder_requests = 0;
	}

	void           *write_ptr = sac_message_buf;

	memset (sac_message_buf, 0, sac_len);

	if (route_id_flag == 0)
	{
		memcpy (write_ptr,
			(const void *) &route_id,
			sizeof (route_id));

		write_ptr = (char *) write_ptr + 
			     sizeof (route_id);
	}

	while (num_remainder_requests != 0)
	{
		uint8_t         request_index =
				capacity_request_number - 
				num_remainder_requests + 1;

		uint16_t & c_r_ref = *((uint16_t *) write_ptr);

		c_r_ref = 0;

		c_r_ref |=
			(capacity_requests[request_index].scaling_factor==SCALING_FACTOR_1) ? 0 : 1;

		c_r_ref |=
			((capacity_requests[request_index].
			  capacity_request_type & 7) << 1);

		c_r_ref |=
			((capacity_requests[request_index].
			  channel_id & 15) << 4);

		c_r_ref |=
			((uint16_t) capacity_requests[request_index].
			 capacity_request_value << 8);

		write_ptr = (char *) write_ptr + sizeof (uint16_t);

		num_remainder_requests--;
	}

	if (m_and_c_flag == 1)
	{
		memcpy (write_ptr, 
			(const void *) &m_and_c_message,
			sizeof (m_and_c_message));

		write_ptr = (char *) write_ptr + 
			    sizeof (m_and_c_message);
	}

	if (group_id_flag == 1)
	{
		memcpy (write_ptr, 
			(const void *) &group_id,
			sizeof (group_id));

		write_ptr = (char *) write_ptr + 
			    sizeof (group_id);
	}

	if (logon_id_flag == 1)
	{
		memcpy (write_ptr, 
			(const void *) &logon_id,
			sizeof (logon_id));

		write_ptr = (char *) write_ptr + 
			    sizeof (logon_id);
	}

	if (acm_flag == 0)
	{
		memcpy (write_ptr, 
			(const void *) &acm, 
			sizeof (acm));

		write_ptr = (char *) write_ptr + 
			    sizeof (acm);
	}
	return sac_message_buf;
}


uint8_t	
Sac::sac_length(Slot_flags flags)
{
	return (sac_length(flags.route_id_flag,
			   flags.request_flag,
			   flags.capacity_request_number,
			   flags.m_and_c_flag,
			   flags.group_id_flag,
			   flags.logon_id_flag,
			   flags.acm_flag));
}


uint8_t
Sac::sac_length(bool route_id_flag, 
	   bool request_flag, 
	   uint8_t capacity_request_number,
	   bool m_and_c_flag,
	   bool group_id_flag,
	   bool logon_id_flag,
	   bool acm_flag)
{
	uint8_t		len;


	len = (route_id_flag) ? 0 : 2;
	if(request_flag)
	{
		assert(capacity_request_number!=NO_CAPACITY_REQUEST);
		//capacity_request_number==0 means one capacity request exists.
		len += (2 * (capacity_request_number+1));
	}

	len += (m_and_c_flag) ? 2 : 0;
	len += (group_id_flag) ? 1 : 0;
	len += (logon_id_flag) ? 2 : 0;
	len += (acm_flag) ? 0 : 2;

	return (len);
}

uint8_t
Sac::sac_length()
{
	uint8_t		len;


	len = (route_id_flag) ? 0 : 2;
	if(request_flag)
	{
		assert(capacity_request_number!=NO_CAPACITY_REQUEST);
		//capacity_request_number==0 means one capacity request exists.
		len += (2 * (capacity_request_number+1));
	}

	len += (m_and_c_flag) ? 2 : 0;
	len += (group_id_flag) ? 1 : 0;
	len += (logon_id_flag) ? 2 : 0;
	len += (acm_flag) ? 0 : 2;

	return (len);
}

/************************************************************
 * req_full() determine if the capacity_request_number
 * reach maximum.
 ***********************************************************/
bool
Sac::req_full()
{
	assert(capacity_request_number < 8);

	return (capacity_request_number==7);
}



/************************************************************
 * fill_req() tries to break requirement into capacity
 * requests and fills requests into SAC. If capacity
 * requests field is full in SAC, part of requirement is
 * discarded.
 ***********************************************************/
int		
Sac::fill_req(Capacity_request_type_value type, 
	      uint32_t value, 
	      uint8_t channel_id,
	      uint8_t max_capacity_request_number)
{
	const uint8_t		MAX_REQ_VALUE = 255;
	const uint8_t		SCALING_RATIO = 16;

	assert(max_capacity_request_number < 8);

	// Special case.
	if (value==0 && type==CAPACITY_REQUEST_TYPE_VALUE_RBDC)
	{
		add_capacity_request(Sac::SCALING_FACTOR_1, type, 
				     channel_id, 0);

		return (0);
	}

	while( value > 0 )
	{
		if(value > MAX_REQ_VALUE) // Scaling is needed.
		{
			if( capacity_request_number >= max_capacity_request_number
				&& capacity_request_number != NO_CAPACITY_REQUEST )
			{
				printf("\e[36m[Warning] Capacity request field in SAC is full, remainder requirements(%u) are discarded.\e[m\n", value);
				break;
			}

			const uint32_t req = value / SCALING_RATIO;

			const uint8_t req_value = (req > MAX_REQ_VALUE) ? MAX_REQ_VALUE : req;

			const uint32_t scaled_req_value = req_value * SCALING_RATIO;

			value = (value > scaled_req_value) ? value - scaled_req_value : 0;

			add_capacity_request(Sac::SCALING_FACTOR_16, type, 
					     channel_id, req_value);
		}
		else // Scaling is not needed.
		{
			if( capacity_request_number < max_capacity_request_number 
				|| capacity_request_number == NO_CAPACITY_REQUEST )
			{
				add_capacity_request(Sac::SCALING_FACTOR_1, 
							  type,
							  channel_id, 
							  value);
				value = 0;
			}
			else
			{
				printf("\e[36m[Warning] Capacity request field in SAC is full, remainder requirements(%u) are discarded.\e[m\n", value);
				break;
			}
		}
	}

	return (0);
}

void		
Sac::pad_dummy_req(uint8_t max_capacity_request_number)
{
	assert(max_capacity_request_number < 8);

	if (capacity_request_number==NO_CAPACITY_REQUEST)
	{
		add_capacity_request(SCALING_FACTOR_1,
				     CAPACITY_REQUEST_TYPE_VALUE_VBDC,
				     0, 0);
	}

	while (capacity_request_number < max_capacity_request_number)
	{
		add_capacity_request(SCALING_FACTOR_1,
				     CAPACITY_REQUEST_TYPE_VALUE_VBDC,
				     0, 0);
	}
}

int
Sac::add_capacity_request (Scaling_factor_value sc_factor,
			   Capacity_request_type_value c_r_type,
			   uint8_t c_id, uint8_t c_r_value)
{
	if ((c_id <= 15)
	    && ((capacity_request_number == NO_CAPACITY_REQUEST)
		|| (capacity_request_number < 7)))
	{
		request_flag = 1;

		if (capacity_request_number == NO_CAPACITY_REQUEST)
			capacity_request_number = 0;
		else
			capacity_request_number++;

		capacity_requests[capacity_request_number].scaling_factor =
			sc_factor;

		capacity_requests[capacity_request_number].
			capacity_request_type = c_r_type;

		capacity_requests[capacity_request_number].channel_id =
			c_id;

		capacity_requests[capacity_request_number].
			capacity_request_value = c_r_value;

		return (0);
	}
	else
	{
		return (-1);
	}
}

int
Sac::pop_capacity_request (Scaling_factor_value & sc_factor,
			   Capacity_request_type_value & c_r_type,
			   uint8_t & ch_id, uint8_t & c_r_value)
{
	if (capacity_request_number != NO_CAPACITY_REQUEST)
	{
		//Pop out the last capacity_request.
		const           Capacity_request & c_r =
			capacity_requests[capacity_request_number];

		sc_factor = c_r.scaling_factor;
		c_r_type = c_r.capacity_request_type;
		ch_id = c_r.channel_id;
		c_r_value = c_r.capacity_request_value;

		if (capacity_request_number == 0)
			capacity_request_number = NO_CAPACITY_REQUEST;
		else
			capacity_request_number--;

		return (0);
	}
	else
	{
		return (-1);
	}
}

int
Sac::pop_capacity_request (Capacity_request & cap_req)
{
	if (capacity_request_number != NO_CAPACITY_REQUEST)
	{
		//Pop out the last capacity_request.
		cap_req = capacity_requests[capacity_request_number];

		if (capacity_request_number == 0)
			capacity_request_number = NO_CAPACITY_REQUEST;
		else
			capacity_request_number--;

		return 0;
	}
	else
	{
		return -1;
	}
}

