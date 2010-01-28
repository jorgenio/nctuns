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

#ifndef		__NCTUNS_SECTION__
#define		__NCTUNS_SECTION__


#include <list>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/table/dgm_section.h>
#include <satellite/dvb_rcs/common/table/section_draft.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/table/pat_table.h>
#include <satellite/dvb_rcs/common/table/pmt_table.h>
#include <satellite/dvb_rcs/common/table/nit_table.h>
#include <satellite/dvb_rcs/common/table/int_table.h>
#include <satellite/dvb_rcs/common/table/sct_table.h>
#include <satellite/dvb_rcs/common/table/fct_table.h>
#include <satellite/dvb_rcs/common/table/tct_table.h>
#include <satellite/dvb_rcs/common/table/tbtp_table.h>
#include <satellite/dvb_rcs/common/table/tim_table.h>
#include <satellite/dvb_rcs/common/table/pat_table_q.h>
#include <satellite/dvb_rcs/common/table/pmt_table_q.h>
#include <satellite/dvb_rcs/common/table/nit_table_q.h>
#include <satellite/dvb_rcs/common/table/int_table_q.h>
#include <satellite/dvb_rcs/common/table/sct_table_q.h>
#include <satellite/dvb_rcs/common/table/fct_table_q.h>
#include <satellite/dvb_rcs/common/table/tct_table_q.h>
#include <satellite/dvb_rcs/common/table/tbtp_table_q.h>

#include <satellite/dvb_rcs/common/table/pat_section.h>
#include <satellite/dvb_rcs/common/table/pmt_section.h>
#include <satellite/dvb_rcs/common/table/nit_section.h>
#include <satellite/dvb_rcs/common/table/int_section.h>
#include <satellite/dvb_rcs/common/table/sct_section.h>
#include <satellite/dvb_rcs/common/table/fct_section.h>
#include <satellite/dvb_rcs/common/table/tct_section.h>
#include <satellite/dvb_rcs/common/table/tbtp_section.h>
#include <satellite/dvb_rcs/common/table/tim_section.h>

using std::list;

class NslObject;

class Section : public NslObject {
  private:
	dvb_node_type				_node_type;

	list<Sct_section_to_table_handler>	_sct_s2t_hdl_list;
	Fct_section_to_table_handler*		_fct_s2t_hdl;
	Tct_section_to_table_handler*		_tct_s2t_hdl;
	Pat_section_to_table_handler*		_pat_s2t_hdl;
	Pmt_section_to_table_handler*		_pmt_s2t_hdl;
	Nit_section_to_table_handler*		_nit_s2t_hdl;
	Int_section_to_table_handler*		_int_s2t_hdl;
	list<Tbtp_section_to_table_handler>	_tbtp_s2t_hdl_list;
	list<Tim_section_to_table_handler>	_tim_s2t_hdl_list;

	Sct_table_to_section_handler		_sct_t2s_hdl;
	Fct_table_to_section_handler		_fct_t2s_hdl;
	Tct_table_to_section_handler		_tct_t2s_hdl;
	Pat_table_to_section_handler		_pat_t2s_hdl;
	Pmt_table_to_section_handler		_pmt_t2s_hdl;
	Nit_table_to_section_handler		_nit_t2s_hdl;
	Int_table_to_section_handler		_int_t2s_hdl;
	Tbtp_table_to_section_handler		_tbtp_t2s_hdl;
	Tim_table_to_section_handler		_tim_t2s_hdl;

  public:
	Section(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~Section();

	int			init();
	int			send(ePacket_ *Epkt);
	int			recv(ePacket_ *Epkt);
};

#endif		/*__NCTUNS_SECTION__*/
