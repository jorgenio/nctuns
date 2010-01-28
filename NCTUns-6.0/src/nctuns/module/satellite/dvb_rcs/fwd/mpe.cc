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
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/table/section_draft.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include "mpe.h"

MODULE_GENERATOR(Mpe);

Mpe::Mpe(u_int32_t type, u_int32_t id, struct plist *pl, const char *name)
: NslObject(type, id, pl, name)
{
}

Mpe::~Mpe()
{
}


int
Mpe::recv(ePacket_ *event)
{
	Dvb_pkt*	_pkt;
	void*		_ip_dgm;
	void*		_section;
	

	assert(_pkt = (Dvb_pkt *) event->DataInfo_);


	_node_type = NODE_ID_RCST;

	switch(_node_type)
	{
		case NODE_ID_RCST:
		{
			/*
			 * Transfer datagram section -> rawdata
			 * bypass the others
			 */
			if(_pkt-> pkt_gettype() == PKT_SECTION)	// user traffic data
			{
				// Get the section from DVB packet.
				_section = _pkt-> pkt_detach(); 


				if(table_id(_section)==DGM_TABLE_ID)
				{
					int ip_len;

					// transfer into ip datagram.
					_ip_dgm = section_to_ip_hdl.dgm_section_to_ip_dgm(_section, _mac, &ip_len);

					// Free the datagram section.
					free(_section);
					// Reuse the event and dvb packets.
					_pkt-> pkt_attach(_ip_dgm, ip_len);
					_pkt-> pkt_settype(PKT_RAWDATA);

					return NslObject::recv(event);
				}
				else
				{
					//bypass.
					_pkt-> pkt_attach(_section, section_total_length(_section));
					return NslObject::recv(event);
				}
			}
			else
				return NslObject::recv(event);
		}
		
		case NODE_ID_SP:
		{
			// Bypass every packets.
			return NslObject::recv(event);
		}

		default:
			assert(0);
	}// End of switch.
}// End of Mpe::recv().


int
Mpe::send(ePacket_ *event)
{
	Dvb_pkt*	_pkt;
	void*		_ip_dgm;
	void*		_section;
	int		_section_total_len;
	u_int16_t	ip_payload_len;
	

	assert(_pkt = (Dvb_pkt *) event->DataInfo_);

	_node_type = NODE_ID_SP;

	switch(_node_type)
	{
		case NODE_ID_RCST:
		{
			// Bypass every packets.
			return NslObject::send(event);
		}
		case NODE_ID_SP:
		{
			if(_pkt-> pkt_gettype() == PKT_RAWDATA)	// user traffic data
			{
				/* transfer rawdata -> datagram_section
				 * bypass the others
				 */
				ip_payload_len = _pkt-> pkt_getlen();

				// Get the ip datagram from DVB packet.
				assert( _ip_dgm = _pkt-> pkt_detach() ); 

				// transfer into datagram section.
				_section = ip_to_section_hdl.ip_dgm_to_dgm_section(ip_payload_len, _ip_dgm, _mac);

				_section_total_len = section_total_length(_section);
				
				// Free the ip datagram.
				free(_ip_dgm);

				// Reuse the DVB packet.
				_pkt-> pkt_attach(_section, _section_total_len);
				_pkt-> pkt_settype(PKT_SECTION);


				return NslObject::send(event);
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
