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

#ifndef __NCTUNS_mpeg2_ts_ncc_h__
#define __NCTUNS_mpeg2_ts_ncc_h__

#include <stdio.h>
#include <stdlib.h>
#include <event.h>
#include <object.h>
#include "../common/dvb_pkt.h"
#include "../common/dvbrcs_api.h"
#include "info.h"

#pragma pack(1)

class Mpeg2_ts_ncc : public NslObject{

private:

	class Ts_header {
		private:
			uint32_t	_sync_byte			: 8;
			uint32_t	_transport_error_indicator	: 1;
			uint32_t	_payload_unit_start_indicator	: 1;
			uint32_t	_transport_priority		: 1;
			uint32_t	_pid				: 13;
			uint32_t	_transport_scrambling_ctl	: 2;
			uint32_t	_adaptation_field_ctl		: 2;
			uint32_t	_continuity_counter		: 4;
			void		*_adaptation_field;
			void		*_data_byte;

		public:
			inline unsigned int header_len()
			{
				return (unsigned int)&_adaptation_field
					- (unsigned int)this;
			}
			friend	class Mpeg2_ts_ncc;
	};

	dvb_node_type	_node_type;
	int		_feeder_nid;
	char		*_feeder_name;
	sec_info	*si_start;
	conti_count	*cc_start;
public:

	Mpeg2_ts_ncc(unsigned int, unsigned int, struct plist*, const char*);
	~Mpeg2_ts_ncc();

	int		init();
	int		recv(ePacket_ *pkt);
	int		send(ePacket_ *pkt);
	sec_info	*search_sec_info_entry(int pid);
	sec_info	*search_prev_sect_info_entry(sec_info *curr_si);
	conti_count	*search_conti_count_entry(int pid);
	int		del_sec_info_entry(int pid);
	void		free_conti_count(conti_count *ptr);
	void		free_sec_info(sec_info *ptr);
};

#pragma pack(0)

#endif /* __NCTUNS_mpeg2_ts_ncc_h__ */
