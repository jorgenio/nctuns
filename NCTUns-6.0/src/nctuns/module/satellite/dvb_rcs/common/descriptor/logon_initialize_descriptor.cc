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

#include <string.h>
#include "logon_initialize_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Logon_initialize_descriptor::Logon_initialize_descriptor ()
{

	_descriptor_tag = LOGON_INITIALIZE_DESCRIPTOR;
	_descriptor_length = 8;
	_traffic_type_pointer = NULL;
	_capacity_pointer = NULL;
}

/*
 * consructor
 */
Logon_initialize_descriptor::
Logon_initialize_descriptor (uint8_t set_group_id, uint16_t set_logon_id,
			     u_char set_security_handshake_required,
			     u_char set_prefix_flag,
			     u_char set_data_unit_labelling_flag,
			     u_char set_mini_slot_flag,
			     u_char set_contention_based_mini_slot_flag,
			     u_char set_capacity_type_flag,
			     u_char set_traffic_burst_type,
			     u_char set_connectivity)
{

	int             length_count = 0;

	_descriptor_tag = LOGON_INITIALIZE_DESCRIPTOR;
	_traffic_type_pointer = NULL;
	_capacity_pointer = NULL;
	_Group_ID = set_group_id;
	_Logon_ID = set_logon_id;
	_Security_handshake_required = set_security_handshake_required;
	_Prefix_flag = set_prefix_flag;
	_Data_unit_labelling_flag = set_data_unit_labelling_flag;
	_Mini_slot_flag = set_mini_slot_flag;
	_Contention_based_mini_slot_flag =
		set_contention_based_mini_slot_flag;
	_traffic_information._Capacity_type_flag = set_capacity_type_flag;
	_traffic_information._Traffic_burst_type = set_traffic_burst_type;

	/* is ATM traffic */
	if (_traffic_information._Traffic_burst_type == 0)
	{

		Atm_traffic    *atm_trf = new Atm_traffic ();

		_traffic_type_pointer = (void *) atm_trf;

		/* is IP connectivity */
		if (set_connectivity == 0)
		{

			Ip_connectivity *ip_connect =
				new Ip_connectivity ();
			set_Connectivity (set_connectivity);
			((Atm_traffic *) (_traffic_type_pointer))->
				_connectivity_pointer =
				(void *) ip_connect;
			length_count = 8;
		}
		/* is optional ATM connectivity */
		else
		{

			Atm_connectivity *atm_connect =
				new Atm_connectivity ();
			set_Connectivity (set_connectivity);
			((Atm_traffic *) (_traffic_type_pointer))->
				_connectivity_pointer =
				(void *) atm_connect;
			length_count = 11;

		}
	}
	/* is optional MPEG2-TS traffic */
	else
	{

		Mpeg2_ts_traffic *mts_tfc = new Mpeg2_ts_traffic ();

		_traffic_type_pointer = (void *) mts_tfc;
		length_count = 8;
	}

	if (_traffic_information._Capacity_type_flag == 0)
	{

		Capacity       *cap = new Capacity ();

		_capacity_pointer = cap;
		length_count += 10;
	}
	_descriptor_length = length_count;
}

/*
 * destructor
 */
Logon_initialize_descriptor::~Logon_initialize_descriptor ()
{

	/* is ATM traffic */
	if (_traffic_information._Traffic_burst_type == 0)
	{

		u_char          con_ = 0;

		con_ = get_Connectivity ();

		/* is IP connectivity */
		if (con_ == 0)
		{

			Ip_connectivity *tmp = NULL;

			tmp = (Ip_connectivity
			       *) (((Atm_traffic
				     *) (_traffic_type_pointer))->
				   _connectivity_pointer);
			delete          tmp;

			delete (Atm_traffic *) _traffic_type_pointer;
		}
		/* is optional ATM connectivity */
		else
		{

			Atm_connectivity *tmp = NULL;

			tmp = (Atm_connectivity
			       *) (((Atm_traffic
				     *) (_traffic_type_pointer))->
				   _connectivity_pointer);
			delete          tmp;

			delete (Atm_traffic *) _traffic_type_pointer;
		}
	}
	/* is optional MPEG2-TS traffic */
	else
	{

		delete (Mpeg2_ts_traffic *) _traffic_type_pointer;
	}
	delete (Capacity *) _capacity_pointer;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Logon_initialize_descriptor
 */
Logon_initialize_descriptor *
Logon_initialize_descriptor::descriptor_copy ()
{
	u_char          set_capacity_type_flag = 0;
	u_char          set_traffic_burst_type = 0;
	u_char          set_connectivity = 0;

	set_capacity_type_flag = _traffic_information._Capacity_type_flag;
	set_traffic_burst_type = _traffic_information._Traffic_burst_type;
	if (set_traffic_burst_type == 0)
	{
		set_connectivity = get_Connectivity ();
	}
	Logon_initialize_descriptor *des_copy =
		new Logon_initialize_descriptor (_Group_ID, _Logon_ID,
						 _Security_handshake_required,
						 _Prefix_flag,
						 _Data_unit_labelling_flag,
						 _Mini_slot_flag,
						 _Contention_based_mini_slot_flag,
						 set_capacity_type_flag,
						 set_traffic_burst_type,
						 set_connectivity);

	if (set_traffic_burst_type == 0)
	{
		if (set_connectivity == 0)
		{
			des_copy->set_Return_VPI (get_Return_VPI ());
			des_copy->set_Return_VCI (get_Return_VCI ());
		}
		else
		{
			des_copy->
				set_Return_signaling_VPI
				(get_Return_signaling_VPI ());
			des_copy->
				set_Return_signaling_VCI
				(get_Return_signaling_VCI ());
			des_copy->
				set_Forward_signaling_VPI
				(get_Forward_signaling_VPI ());
			des_copy->
				set_Forward_signaling_VCI
				(get_Forward_signaling_VCI ());
		}

	}
	else
	{
		des_copy->set_Return_TRF_PID (get_Return_TRF_PID ());
		des_copy->
			set_Return_CTRL_MNGM_PID (get_Return_CTRL_MNGM_PID
						  ());
	}
	if (set_capacity_type_flag == 0)
	{
		des_copy->set_CRA_level (get_CRA_level ());
		des_copy->set_VBDC_max (get_VBDC_max ());
		des_copy->set_RBDC_max (get_RBDC_max ());
		des_copy->set_RBDC_timeout (get_RBDC_timeout ());
	}
	return (des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Logon_initialize_descriptor::descriptor_serialized (u_char *
						    serialize_start)
{
#if opened

	int             length = 0;

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	serialize_start[2] = _Group_ID;

	/* Logon_ID is in [3] and [4] */
	memcpy (serialize_start + 3, &_Logon_ID, 2);
	/* _Security_handshake_required is in [5]:0-3 */
	serialize_start[5] &= 0x00;
	serialize_start[5] |= _Security_handshake_required << 4;
	/* _Prefix_flag [5]:4 , _Data_unit_labelling_flag [5]:5 , _Mini_slot_flag [5]:6 , _Contention_based_mini_slot_flag [5]:7 */
	serialize_start[5] |= _Prefix_flag << 3;
	serialize_start[5] |= _Data_unit_labelling_flag << 2;
	serialize_start[5] |= _Mini_slot_flag << 1;
	serialize_start[5] |= _Contention_based_mini_slot_flag;
	/* _Capacity_type_flag [6]:0-1 , _Traffic_burst_type [6]:2 */
	serialize_start[6] &= 0x00;
	serialize_start[6] |=
		_traffic_information._Capacity_type_flag << 6;
	serialize_start[6] |=
		_traffic_information._Traffic_burst_type << 5;
	length = 7;

	if (_traffic_information._Traffic_burst_type == 0)
	{

		u_char          con = 0;

		con = get_Connectivity ();
		/* _Connectivity [6]:3 */
		serialize_start[6] |= (con & 0x01) << 4;

		if (get_Connectivity () == 0)
		{

			uint16_t        vpi = 0;
			uint16_t        vci = 0;

			vpi = get_Return_VPI ();
			vci = get_Return_VCI ();
			/* _Return_VPI reserved bits [6]:4-7 , _Return_VPI info bits [7] */
			serialize_start[6] &= (vpi & 0x0F00) >> 8;;
			serialize_start[7] = vpi;
			/* _Return_VCI [8] and [9] */
			memcpy (serialize_start + 8, &vci, 2);
			length = 10;
		}
		/* optional */
		else
		{

			uint16_t        ret_snl_vpi =
				get_Return_signaling_VPI ();
			uint16_t        ret_snl_vci =
				get_Return_signaling_VCI ();
			uint16_t        fwd_snl_vpi =
				get_Forward_signaling_VPI ();
			uint16_t        fwd_snl_vci =
				get_Forward_signaling_VCI ();

			/* _Return_signaling_VPI reserved bits [6]:4-7 , _Return_signaling_VPI info bits [7] */
			serialize_start[6] |= (ret_snl_vpi & 0x0F00) >> 8;;
			serialize_start[7] = ret_snl_vpi;
			/* _Return_signaling_VCI [8] and [9] */
			memcpy (serialize_start + 8, &ret_snl_vci, 2);
			/* _Forward_signaling_VPI [10] and [11] */
			memcpy (serialize_start + 10, &fwd_snl_vpi, 2);
			/* _Forward_signaling_VCI [12] and [13] */
			memcpy (serialize_start + 12, &fwd_snl_vci, 2);

			length = 14;
		}
	}
	/* optional */
	else
	{

		uint16_t        ret_trf_pid = get_Return_TRF_PID ();
		uint16_t        ret_ctrl_mngm_pid =
			get_Return_CTRL_MNGM_PID ();
		/* _Return_TRF_PID reserved bits [6]:3-7 , _Return_TRF_PID info bits [7] */
		serialize_start[6] |= ((ret_trf_pid >> 8) & 0x1F);
		serialize_start[7] = ((uint8_t) ret_trf_pid);
		/* _Return_CTRL_MNGM_PID [8] and [9] */
		memcpy (serialize_start + 8, &ret_ctrl_mngm_pid, 2);
		length = 10;
	}
	/* _Capacity_type_flag == "0" */
	if (_traffic_information._Capacity_type_flag == 0)
	{

		/* _CRA_level [10] [11] [12] */
		u_char         *y = NULL;
		uint16_t        r = 0;
		u_char         *yy = NULL;
		uint16_t        rr = 0;

		y = get_CRA_level ();
		memcpy (serialize_start + length, y, 3);
		/* _VBDC_max [13] [14] */
		r = get_VBDC_max ();
		memcpy (serialize_start + length + 3, &r, 2);
		/* _RBDC_max [15] [16] [17] */
		yy = get_RBDC_max ();
		memcpy (serialize_start + length + 5, yy, 3);

		/*_RBDC_timeout [18] [19] */
		rr = get_RBDC_timeout ();
		memcpy (serialize_start + length + 8, &rr, 2);
	}
#endif
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Logon_initialize_descriptor::descriptor_deserialized (u_char *
						      serialize_start)
{

#if opened
	int             length = 0;

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	_Group_ID = serialize_start[2];
	memcpy (&_Logon_ID, serialize_start + 3, 2);
	_Security_handshake_required = (serialize_start[5] & 0xF0) >> 4;
	_Prefix_flag = (serialize_start[5] & 0x08) >> 3;
	_Data_unit_labelling_flag = (serialize_start[5] & 0x04) >> 2;
	_Mini_slot_flag = (serialize_start[5] & 0x02) >> 1;
	_Contention_based_mini_slot_flag = serialize_start[5] & 0x01;
	_traffic_information._Capacity_type_flag =
		(serialize_start[6] & 0xC0) >> 6;
	_traffic_information._Traffic_burst_type =
		(serialize_start[6] & 0x20) >> 5;
	length = 7;

	if (_traffic_information._Traffic_burst_type == 0)
	{

		u_char          con = 0;

		Atm_traffic    *atm_trf = new Atm_traffic ();

		_traffic_type_pointer = (void *) atm_trf;

		con = (serialize_start[6] & 0x10) >> 4;
		set_Connectivity (con);

		if (get_Connectivity () == 0)
		{

			uint16_t        vpi = 0;
			uint16_t        vci = 0;

			Ip_connectivity *ip_connect =
				new Ip_connectivity ();
			((Atm_traffic *) (_traffic_type_pointer))->
				_connectivity_pointer =
				(void *) ip_connect;

			vpi &= 0x00;
			vpi |= (serialize_start[6] & 0x0F) << 8;
			vpi |= serialize_start[7];
			memcpy (&vci, serialize_start + 8, 2);
			set_Return_VPI (vpi);
			set_Return_VCI (vci);
			length = 10;
		}
		/* optional */
		else
		{
			uint16_t        rs_vpi = 0;
			uint16_t        rs_vci = 0;
			uint16_t        fs_vpi = 0;
			uint16_t        fs_vci = 0;

			Atm_connectivity *atm_connect =
				new Atm_connectivity ();
			((Atm_traffic *) (_traffic_type_pointer))->
				_connectivity_pointer =
				(void *) atm_connect;

			rs_vpi &= 0x00;
			rs_vpi |= (serialize_start[6] & 0x0F) << 8;
			rs_vpi |= serialize_start[7];
			memcpy (&rs_vci, serialize_start + 8, 2);
			memcpy (&fs_vpi, serialize_start + 10, 2);
			memcpy (&fs_vci, serialize_start + 12, 2);
			set_Return_signaling_VPI (rs_vpi);
			set_Return_signaling_VCI (rs_vci);
			set_Forward_signaling_VPI (fs_vpi);
			set_Forward_signaling_VCI (fs_vci);
			length = 14;
		}
	}
	/* optional */
	else
	{
		uint16_t        ret_trf_pid;
		uint16_t        ret_ctrl_mngm_pid;
		Mpeg2_ts_traffic *mts_tfc = new Mpeg2_ts_traffic ();

		_traffic_type_pointer = (void *) mts_tfc;
		/* _Return_TRF_PID reserved bits [6]:3-7 , _Return_TRF_PID info bits [7] */
		ret_trf_pid &= 0x00;
		ret_trf_pid |= (serialize_start[6] & 0x1F) << 8;
		ret_trf_pid |= serialize_start[7];
		/* _Return_CTRL_MNGM_PID [8] and [9] */
		memcpy (&ret_ctrl_mngm_pid, serialize_start + 8, 2);

		set_Return_TRF_PID (ret_trf_pid);
		set_Return_CTRL_MNGM_PID (ret_ctrl_mngm_pid);

		length = 10;
	}
	if (_traffic_information._Capacity_type_flag == 0)
	{

		u_char          y[3];
		uint16_t        r = 0;
		u_char          yy[3];
		uint16_t        rr = 0;

		Capacity       *cap = new Capacity ();

		_capacity_pointer = cap;

		memcpy (y, serialize_start + length, 3);
		set_CRA_level (y);
		memcpy (&r, serialize_start + length + 3, 2);
		set_VBDC_max (r);
		memcpy (yy, serialize_start + length + 5, 3);
		set_RBDC_max (yy);
		memcpy (&rr, serialize_start + length + 8, 2);
		set_RBDC_timeout (rr);
	}

#endif
}

/*
 * set_Return_VPI function
 */
void
Logon_initialize_descriptor::set_Return_VPI (uint16_t set_return_vpi)
{

	Ip_connectivity *tmp = NULL;

	tmp = (Ip_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	tmp->_Return_VPI = set_return_vpi;
}

/*
 * get_Return_VPI function
 */
uint16_t Logon_initialize_descriptor::get_Return_VPI ()
{

	Ip_connectivity *
		tmp = NULL;

	tmp = (Ip_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	return tmp->_Return_VPI;
}

/*
 * set_Return_VCI function
 */
void
Logon_initialize_descriptor::set_Return_VCI (uint16_t set_return_vci)
{

	Ip_connectivity *tmp = NULL;

	tmp = (Ip_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	tmp->_Return_VCI = set_return_vci;
}

/*
 * get_Return_VCI function
 */
uint16_t Logon_initialize_descriptor::get_Return_VCI ()
{

	Ip_connectivity *
		tmp = NULL;

	tmp = (Ip_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	return tmp->_Return_VCI;
}

void
Logon_initialize_descriptor::
set_Return_signaling_VPI (uint16_t set_Return_signaling_VPI)
{

	Atm_connectivity *tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	tmp->_Return_signaling_VPI = set_Return_signaling_VPI;
}

uint16_t Logon_initialize_descriptor::get_Return_signaling_VPI ()
{

	Atm_connectivity *
		tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	return tmp->_Return_signaling_VPI;
}

void
Logon_initialize_descriptor::
set_Return_signaling_VCI (uint16_t set_Return_signaling_VCI)
{

	Atm_connectivity *tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) (_traffic_type_pointer))->
		   _connectivity_pointer);
	tmp->_Return_signaling_VCI = set_Return_signaling_VCI;
}

uint16_t Logon_initialize_descriptor::get_Return_signaling_VCI ()
{

	Atm_connectivity *
		tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) _traffic_type_pointer)->
		   _connectivity_pointer);
	return tmp->_Return_signaling_VCI;
}

void
Logon_initialize_descriptor::
set_Forward_signaling_VPI (uint16_t set_Forward_signaling_VPI)
{

	Atm_connectivity *tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) _traffic_type_pointer)->
		   _connectivity_pointer);
	tmp->_Forward_signaling_VPI = set_Forward_signaling_VPI;
}

uint16_t Logon_initialize_descriptor::get_Forward_signaling_VPI ()
{

	Atm_connectivity *
		tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) _traffic_type_pointer)->
		   _connectivity_pointer);
	return tmp->_Forward_signaling_VPI;
}

void
Logon_initialize_descriptor::
set_Forward_signaling_VCI (uint16_t set_Forward_signaling_VCI)
{

	Atm_connectivity *tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) _traffic_type_pointer)->
		   _connectivity_pointer);
	tmp->_Forward_signaling_VCI = set_Forward_signaling_VCI;
}

uint16_t Logon_initialize_descriptor::get_Forward_signaling_VCI ()
{

	Atm_connectivity *
		tmp = NULL;

	tmp = (Atm_connectivity
	       *) (((Atm_traffic *) _traffic_type_pointer)->
		   _connectivity_pointer);
	return tmp->_Forward_signaling_VCI;
}

/*
 * set_Connectivity function
 */
void
Logon_initialize_descriptor::set_Connectivity (u_char set_connectivity)
{

	((Atm_traffic *) (_traffic_type_pointer))->_Connectivity =
		set_connectivity;
}

/*
 * get_Connectivity function
 */
u_char Logon_initialize_descriptor::get_Connectivity ()
{

	return ((Atm_traffic *) (_traffic_type_pointer))->_Connectivity;
}

/*
 * set_CRA_level function
 */
void
Logon_initialize_descriptor::set_CRA_level (uint32_t cra_level_value)
{
	_capacity_pointer->_CRA_level[0] = cra_level_value & 0xFF;
	_capacity_pointer->_CRA_level[1] = (cra_level_value & 0xFF00) >> 8;
	_capacity_pointer->_CRA_level[2] =
		(cra_level_value & 0xFF0000) >> 16;
}

/*
 * get_CRA_level function
 */
uint32_t Logon_initialize_descriptor::get_CRA_level ()
{
	return _capacity_pointer->_CRA_level[0] +
		(_capacity_pointer->_CRA_level[1] * 256) +
		(_capacity_pointer->_CRA_level[2] * 65536);
}

/*
 * set_VBDC_max function
 */
void
Logon_initialize_descriptor::set_VBDC_max (uint16_t set_vbdc_max)
{

	_capacity_pointer->_VBDC_max = set_vbdc_max;
}

/*
 * get_VBDC_max function
 */
uint16_t Logon_initialize_descriptor::get_VBDC_max ()
{

	return _capacity_pointer->_VBDC_max;
}

/*
 * set_RBDC_max function
 */
void
Logon_initialize_descriptor::set_RBDC_max (uint32_t rbdc_max)
{
	_capacity_pointer->_RBDC_max[0] = rbdc_max & 0xFF;
	_capacity_pointer->_RBDC_max[1] = (rbdc_max & 0xFF00) >> 8;
	_capacity_pointer->_RBDC_max[2] = (rbdc_max & 0xFF0000) >> 16;
}

/*
 * get_RBDC_max function
 */
uint32_t Logon_initialize_descriptor::get_RBDC_max ()
{
	return _capacity_pointer->_RBDC_max[0] +
		(_capacity_pointer->_RBDC_max[1] * 256) +
		(_capacity_pointer->_RBDC_max[2] * 65536);
}

/*
 * set_RBDC_timeout function
 */
void
Logon_initialize_descriptor::set_RBDC_timeout (uint16_t set_rbdc_timeout)
{

	_capacity_pointer->_RBDC_timeout = set_rbdc_timeout;
}

/*
 * get_RBDC_timeout function
 */
uint16_t Logon_initialize_descriptor::get_RBDC_timeout ()
{

	return _capacity_pointer->_RBDC_timeout;
}

void
Logon_initialize_descriptor::
set_Return_TRF_PID (uint16_t set_Return_TRF_PID)
{

	((Mpeg2_ts_traffic *) _traffic_type_pointer)->_Return_TRF_PID =
		set_Return_TRF_PID;
}

uint16_t Logon_initialize_descriptor::get_Return_TRF_PID ()
{

	return ((Mpeg2_ts_traffic *) _traffic_type_pointer)->
		_Return_TRF_PID;
}

void
Logon_initialize_descriptor::
set_Return_CTRL_MNGM_PID (uint16_t set_Return_CTRL_MNGM_PID)
{

	((Mpeg2_ts_traffic *) _traffic_type_pointer)->
		_Return_CTRL_MNGM_PID = set_Return_CTRL_MNGM_PID;
}

uint16_t Logon_initialize_descriptor::get_Return_CTRL_MNGM_PID ()
{

	return ((Mpeg2_ts_traffic *) _traffic_type_pointer)->
		_Return_CTRL_MNGM_PID;
}

bool Logon_initialize_descriptor::is_match (uint8_t group_id,
					    uint16_t logon_id)
{

	return (group_id == _Group_ID && logon_id == _Logon_ID);
}

