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

#include "dgm_section.h"


/* This function is to encapsulate the ip datagram
 * into datagram section(s). The MTU of ip datagram
 * is limited to 1500 bytes now.
 * The section_syntax_indicator and private_indicator checking
 * is not done. Neither do the CRC32 and checksum.
 */
void* Ip_datagram_to_dgm_section_handler::ip_dgm_to_dgm_section(u_int16_t ip_payload_len , const void* ip_datagram, const char* dhost) {
	void*				_ptr_section;
	void*				_ptr_ip_data_src;
	void*				_ptr_ip_data_dst;
	Dgm_section_draft*		_ptr_draft;
	u_int16_t			_total_section_length, _section_length;

	assert(ip_datagram && dhost);

	_total_section_length = sizeof(Dgm_section_draft) + ip_payload_len + CRC32_SIZE;
	_section_length = _total_section_length - SECTION_DRAFT_SIZE; 
	
	// Memory allocation for one section.
	_ptr_section = malloc(_total_section_length);
	
	// Fill the section with the fields in datagram section draft.
	_ptr_draft = (Dgm_section_draft*) _ptr_section;
	_ptr_draft-> _table_id = DGM_TABLE_ID;
	_ptr_draft-> _section_syntax_indicator = 1; // The CRC32 is not really applied here.
	_ptr_draft-> _private_indicator = 0;
	_ptr_draft-> _section_length = _section_length;
	_ptr_draft-> _mac_address_1 = 0; 
	_ptr_draft-> _mac_address_2 = 0;
	_ptr_draft-> _mac_address_3 = 0;
	_ptr_draft-> _mac_address_4 = 0;
	_ptr_draft-> _mac_address_5 = 0;
	_ptr_draft-> _mac_address_6 = 0;

	_ptr_draft-> _payload_scrambling_control = 0; // Scrambling is not supported now.
	_ptr_draft-> _address_scrambling_control = 0; // Scrambling is not supported now.
	_ptr_draft-> _llc_snap_flag = 0; // LLC/SNAP is not supported now.
	_ptr_draft-> _section_number = 0; // The MTU is limited to 1500, thus we need only one section to carry the whole ip datagram.
	_ptr_draft-> _last_section_number = 0;

	// Copy the ip data into the section.
	_ptr_ip_data_src = (void*)ip_datagram; 
	_ptr_ip_data_dst = (void*) ((char*) _ptr_section + sizeof(Dgm_section_draft));
	memcpy(_ptr_ip_data_dst, _ptr_ip_data_src, ip_payload_len);

	// Compute and fill the CRC32 value.
	fill_crc(_ptr_section);


	return _ptr_section;
}

/* This function is to unpack datagram section(s) back to
 * ip datagram. The MTU of ip datagram is limited to 1500 bytes now.
 * TODO:Generalization for larger MTU.
 */
void* Dgm_section_to_ip_datagram_handler::dgm_section_to_ip_dgm(const void* dgm_section, char* dhost, int *ip_len) {
	void*				_ptr_ip_data_src;
	void*				_ptr_ip_data_dst;
	Dgm_section_draft*	_ptr_draft;
	u_int16_t			_total_section_length;

	assert(dgm_section && dhost);

	_ptr_draft = (Dgm_section_draft*) dgm_section;

	// Get the mac address of the destination host.
	dhost[0] = _ptr_draft-> _mac_address_6;
	dhost[1] = _ptr_draft-> _mac_address_5;
	dhost[2] = _ptr_draft-> _mac_address_4;
	dhost[3] = _ptr_draft-> _mac_address_3;
	dhost[4] = _ptr_draft-> _mac_address_2;
	dhost[5] = _ptr_draft-> _mac_address_1;

	
	// Compute the length of ip datagram.
	_total_section_length = section_total_length((void*) dgm_section);
	*ip_len = _total_section_length - sizeof(Dgm_section_draft) - CRC32_SIZE;
	
	// Memory allocation for one ip datagram.
	_ptr_ip_data_dst = malloc(*ip_len);
	assert(_ptr_ip_data_dst);

	// Copy the ip data from the section into the ip datagram.
	_ptr_ip_data_src = (void*) ((char*) dgm_section + sizeof(Dgm_section_draft));
	memcpy(_ptr_ip_data_dst, _ptr_ip_data_src, *ip_len);

	return _ptr_ip_data_dst;
}
