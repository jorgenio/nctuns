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

#include <object.h>
#include "section.h"

MODULE_GENERATOR(Section);

Section::Section(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
:NslObject(type, id, pl, name), _node_type(NODE_ID_NONE)
{
	_fct_s2t_hdl = 0;
	_tct_s2t_hdl = 0;
	_pat_s2t_hdl = 0;
	_pmt_s2t_hdl = 0;
	_nit_s2t_hdl = 0;
	_int_s2t_hdl = 0;
}


Section::~Section()
{
}

int
Section::init()
{
	int *type_id;

	/*
	 * get _node_type from control module
	 */
	type_id = GET_REG_VAR1(get_portls(), "NODE_TYPE", int *); 
	if (type_id)
		_node_type = (dvb_node_type) *type_id;
	return NslObject::init();
}


int
Section::recv(ePacket_ *event)
{
	Dvb_pkt*	_pkt;
	void*		_section;
	Table*		_table;
	
	assert(_pkt = (Dvb_pkt *) event->DataInfo_);

	switch(_node_type)
	{
		case NODE_ID_RCST:
		{
			/* transfer contorl section -> table
			 * bypass the others
			 */

			if(_pkt-> pkt_gettype() == PKT_SECTION)	
			{
				
				// Get the section from DVB packet.
				assert( _section = _pkt-> pkt_detach() ); 

				// Verify the CRC32 value.
				if(!crc_okay(_section))
				{
					free(_section);
					dvb_freePacket(event);
					return 1;
				}

				switch(table_id(_section))
				{
					case DGM_TABLE_ID:
					{
						/*
						 * The DGM_TABLE will is by-pass the section layer,
						 * so set the data type to PKT_SECTION that the MPE
						 * layer can receive this packet.
						 */
						_pkt-> pkt_attach(_section, section_total_length(_section));
						_pkt->pkt_settype(PKT_SECTION);
						return NslObject::recv(event);
					}
					case PAT_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_pat_s2t_hdl==0)
						{
							_pat_s2t_hdl = new Pat_section_to_table_handler;
							_pat_s2t_hdl-> init(_section);
						}
						else
						{
							if(_pat_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _pat_s2t_hdl;
								_pat_s2t_hdl = new Pat_section_to_table_handler;
								_pat_s2t_hdl-> init(_section);
							}
						}
							
						_table = _pat_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _pat_s2t_hdl;
							_pat_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case PMT_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_pmt_s2t_hdl==0)
						{
							_pmt_s2t_hdl = new Pmt_section_to_table_handler;
							_pmt_s2t_hdl-> init(_section);
						}
						else
						{
							if(_pmt_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _pmt_s2t_hdl;
								_pmt_s2t_hdl = new Pmt_section_to_table_handler;
								_pmt_s2t_hdl-> init(_section);
							}
						}
							
						_table = _pmt_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _pmt_s2t_hdl;
							_pmt_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case NIT_ACTIVE_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_nit_s2t_hdl==0)
						{
							_nit_s2t_hdl = new Nit_section_to_table_handler;
							_nit_s2t_hdl-> init(_section);
						}
						else
						{
							if(_nit_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _nit_s2t_hdl;
								_nit_s2t_hdl = new Nit_section_to_table_handler;
								_nit_s2t_hdl-> init(_section);
							}
						}
							
							
						_table = _nit_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _nit_s2t_hdl;
							_nit_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case INT_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_int_s2t_hdl==0)
						{
							_int_s2t_hdl = new Int_section_to_table_handler;
							_int_s2t_hdl-> init(_section);
						}
						else
						{
							if(_int_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _int_s2t_hdl;
								_int_s2t_hdl = new Int_section_to_table_handler;
								_int_s2t_hdl-> init(_section);
							}
						}
							
							
						_table = _int_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _int_s2t_hdl;
							_int_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case SCT_TABLE_ID:
					{
						uint8_t	version_of_incoming_section	= version_number(_section);
						list<Sct_section_to_table_handler>::iterator	it;
						list<Sct_section_to_table_handler>::iterator	it_point_to_hdl_which_will_be_used	= _sct_s2t_hdl_list.end();


						for(it = _sct_s2t_hdl_list.begin(); it!=_sct_s2t_hdl_list.end(); it++)
						{
							uint8_t version_of_hdl = it ->get_version_number();

							CUR_NEXT_INDICATOR idctor_of_hdl = it ->get_current_next_indicator();

							if(version_of_incoming_section==version_of_hdl)
							// There exists a handler of same version as the imcoming section.
							{
								it_point_to_hdl_which_will_be_used = it;
							}
							else
							{
								if(idctor_of_hdl==CUR_INDICATOR && idctor_of_hdl==CUR_INDICATOR)
								{
									// Free the handler.
									_sct_s2t_hdl_list.erase(it);
								}
							}
						} //End of "for each handler...".


						if(it_point_to_hdl_which_will_be_used==_sct_s2t_hdl_list.end())
						// Which means there exists no handler of same version as incoming section.
						{
							//We initialize a new handler, and then put it into the handler list.
							Sct_section_to_table_handler	hdl;

							hdl.init(_section);
							_sct_s2t_hdl_list.push_back(hdl);
							// it_point_to_hdl_which_will_be_used --> hdl
							it_point_to_hdl_which_will_be_used = _sct_s2t_hdl_list.end();
							it_point_to_hdl_which_will_be_used--;
						}

						/* Finally hand out the incoming section to corresponding handler. */

						_table = it_point_to_hdl_which_will_be_used ->to_table(_section);
						free(_section);

						// If handler have completed collection of a table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							_sct_s2t_hdl_list.erase(it_point_to_hdl_which_will_be_used);

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}


						break;
					}
					case FCT_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_fct_s2t_hdl==0)
						{
							_fct_s2t_hdl = new Fct_section_to_table_handler;
							_fct_s2t_hdl-> init(_section);
						}
						else
						{
							if(_fct_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _fct_s2t_hdl;
								_fct_s2t_hdl = new Fct_section_to_table_handler;
								_fct_s2t_hdl-> init(_section);
							}
						}
							
							
						_table = _fct_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _fct_s2t_hdl;
							_fct_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case TCT_TABLE_ID:
					{
						// Create one handler instance if meet one unfamiliar section.
						if(_tct_s2t_hdl==0)
						{
							_tct_s2t_hdl = new Tct_section_to_table_handler;
							_tct_s2t_hdl-> init(_section);
						}
						else
						{
							if(_tct_s2t_hdl ->get_version_number() != version_number(_section))
							{
								delete _tct_s2t_hdl;
								_tct_s2t_hdl = new Tct_section_to_table_handler;
								_tct_s2t_hdl-> init(_section);
							}
						}
							
							
						_table = _tct_s2t_hdl-> to_table(_section);

						free(_section);

						// If we collect a complete table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							delete _tct_s2t_hdl;
							_tct_s2t_hdl = 0;

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}
						break;
					}
					case TBTP_TABLE_ID:
					{
						uint8_t	version_of_incoming_section = version_number(_section);
						list<Tbtp_section_to_table_handler>::iterator	it;
						list<Tbtp_section_to_table_handler>::iterator	it_point_to_hdl_which_will_be_used	= _tbtp_s2t_hdl_list.end();


						for(it = _tbtp_s2t_hdl_list.begin(); it!=_tbtp_s2t_hdl_list.end(); it++)
						{
							uint8_t version_of_hdl = it ->get_version_number();

							CUR_NEXT_INDICATOR idctor_of_hdl = it ->get_current_next_indicator();

							if(version_of_incoming_section==version_of_hdl)
							// There exists a handler of same version as the imcoming section.
							{
								it_point_to_hdl_which_will_be_used = it;
							}
							else
							{
								if(idctor_of_hdl==CUR_INDICATOR && idctor_of_hdl==CUR_INDICATOR)
								{
									// Free the handler.
									_tbtp_s2t_hdl_list.erase(it);
								}
							}
						} //End of "for each handler...".


						if(it_point_to_hdl_which_will_be_used==_tbtp_s2t_hdl_list.end())
						// Which means there exists no handler of same version as incoming section.
						{
							//We initialize a new handler, and then put it into the handler list.
							Tbtp_section_to_table_handler	hdl;

							hdl.init(_section);
							_tbtp_s2t_hdl_list.push_back(hdl);
							// it_point_to_hdl_which_will_be_used --> hdl
							it_point_to_hdl_which_will_be_used = _tbtp_s2t_hdl_list.end();
							it_point_to_hdl_which_will_be_used--;
						}

						/* Finally hand out the incoming section to corresponding handler. */

						_table = it_point_to_hdl_which_will_be_used ->to_table(_section);
						free(_section);

						// If handler have completed collection of a table, pass it to the upper module.
						if(_table)
						{
							// Free the handler.
							_tbtp_s2t_hdl_list.erase(it_point_to_hdl_which_will_be_used);

							// Pass table to the upper module.
							_pkt-> pkt_attach(_table, UNKNOWN_LEN);
							_pkt->pkt_settype(PKT_TABLE);
							return NslObject::recv(event);
						}

						break;
					}
					default:
						assert(0);
				}// End of switch.

				dvb_freePacket(event);
				return 1;
			}
			else
				return NslObject::recv(event);
			assert(0);
		}
		
		case NODE_ID_SP:
		case NODE_ID_NCC:
		{
			// Bypass every packets.
			return NslObject::recv(event);
		}

		default:
			assert(0);
	}// End of switch.

}// End of Section::recv().

int Section::send(ePacket_ *event) {

	Dvb_pkt*	_pkt;
	void*		_section;
	Table*		_table;
	int		_section_total_len;

	assert(_pkt = (Dvb_pkt *) event->DataInfo_);

	switch(_node_type)
	{
		case NODE_ID_RCST:
		{
			// Bypass every packets.
			return NslObject::send(event);
		}
		
		case NODE_ID_SP:
		case NODE_ID_NCC:
		{
			/* transfer contorl table -> section
			 * bypass the others(e.g., datagram section from MPE)
			 */

			if(_pkt-> pkt_gettype() == PKT_TABLE)	// control table.
			{
				// Get the control table from DVB packet.
				uint16_t pid;

				pid = _pkt->pkt_getfwdinfo()->pid;
				assert( _table = (Table*)_pkt-> pkt_detach() ); 
				dvb_freePacket(event);
				switch(_table-> get_table_id())
				{
					case PAT_TABLE_ID:
					{
						_pat_t2s_hdl.pat_table_to_section_init((Pat*) _table);

						// transfer into sections and send out.
						while(( _section = _pat_t2s_hdl.pat_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;
							

							ePacket_* new_event = new ePacket_(*event);
							new_event-> DataInfo_ = new_pkt;

							
							NslObject::send(new_event);
						}
						delete ((Pat*) _table);
						break;
					}

					case SCT_TABLE_ID:
					{
						_sct_t2s_hdl.sct_table_to_section_init((Sct*) _table);

						// transfer into sections and send out.
						while((_section = _sct_t2s_hdl.sct_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							//new_pkt-> pkt_setmodule_type(_module_type);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Sct*) _table);
						break;
					}

					case FCT_TABLE_ID:
					{
						_fct_t2s_hdl.fct_table_to_section_init((Fct*) _table);

						// transfer into sections and send out.
						while((_section = _fct_t2s_hdl.fct_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Fct*) _table);
						break;
					}

					case TCT_TABLE_ID:
					{
						_tct_t2s_hdl.tct_table_to_section_init((Tct*) _table);

						// transfer into sections and send out.
						while((_section = _tct_t2s_hdl.tct_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Tct*) _table);
						break;
					}

					case PMT_TABLE_ID:
					{
						_pmt_t2s_hdl.pmt_table_to_section_init((Pmt*) _table);

						// transfer into sections and send out.
						while((_section = _pmt_t2s_hdl.pmt_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Pmt*) _table);
						break;
					}

					case NIT_ACTIVE_TABLE_ID:
					{
						_nit_t2s_hdl.nit_table_to_section_init((Nit*) _table);

						// transfer into sections and send out.
						while((_section = _nit_t2s_hdl.nit_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Nit*) _table);
						break;
					}

					case INT_TABLE_ID:
					{
						_int_t2s_hdl.int_table_to_section_init((Int*) _table);

						// transfer into sections and send out.
						while((_section = _int_t2s_hdl.int_table_to_section() ))
						{
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Int*) _table);
						break;
					}

					case TBTP_TABLE_ID:
					{
						_tbtp_t2s_hdl.tbtp_table_to_section_init((Tbtp*) _table);

						// transfer into sections and send out.
						while((_section = _tbtp_t2s_hdl.tbtp_table_to_section() ))
						{
							
							// create Dvb_pkt.
							Dvb_pkt * new_pkt = new Dvb_pkt();
							new_pkt-> pkt_setflag(FLOW_SEND);
							_section_total_len = section_total_length(_section);
							new_pkt-> pkt_attach(_section, _section_total_len);
							new_pkt-> pkt_settype(PKT_SECTION);
							new_pkt-> pkt_getfwdinfo()->pid = pid;

							ePacket_* new_event = new ePacket_;
							new_event-> DataInfo_ = new_pkt;

							NslObject::send(new_event);
						}
						delete ((Tbtp*) _table);
						break;
					}
					default:
						assert(0);
				}// End of switch.
				// Free the control table.
				return 1;
			}
			else
			{
				return NslObject::send(event);
			}
		}

		default:
			assert(0);
	}// End of switch.
}
