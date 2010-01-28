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

#ifndef __NCTUNS_dvb_pkt_h__
#define __NCTUNS_dvb_pkt_h__

#include <sys/types.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/common/dvb_s2_pls.h>
#include <satellite/dvb_rcs/common/errormodel/attenuation.h>
#include <satellite/dvb_rcs/common/errormodel/saterrormodel.h>

/*
 * packet traffic flow direction enumeration
 */
enum flow_direct {
	FLOW_SEND	= 0x0001,	/* send to up layer */
	FLOW_RECV	= 0x0002	/* recv from bottom layer */
};

/*
 * packet padload data type enumeration
 */
enum packet_type {
	PKT_TABLE	= 0x0029,	/* ncc control table */
	PKT_SECTION	= 0x002a,	/* section frame */
	PKT_MPEG2_TS	= 0x002b,	/* mpeg2-ts frame */
	PKT_DVB_S2	= 0x002c,	/* dvb-s2 frame */
	PKT_RCSSYNC	= 0x002d,	/* dvs-rcs sync control burst */
	PKT_RCSCSC	= 0x002e,	/* dvs-rcs csc control burst */
	PKT_ATM		= 0x002f,	/* atm frame */
	PKT_RCSMAC	= 0x0030,	/* dvs-rcs mac burst */
	PKT_DVBRCS	= 0x0031,	/* dvb-rcs frame */
	PKT_RAWDATA	= 0x0032,	/* raw data */
	PKT_TIM		= 0x0034,	/* dvb_rcs tim control packet for correction time */
	PKT_NONE	= 0x0035,	/* none */
};

/*
 * log infomation
 */
struct log_info {
	uint16_t		phy_src;
	int			tx_time;
};

/*
 * forward link information, those information will be filled sp_ctl, ncc_ctl
 */
struct forward_link_info {
	u_int16_t		pid;
	u_int8_t		interface;
	struct Dvb_s2_pls::info	dvb_s2_pls;
	double			symbol_rate;
	struct link_info	linfo;
	struct link_budget	lbudget;
	struct log_info		loginfo;
};

/*
 * return link information, those information will be filled at RCS_MAC
 */
struct return_link_info {
	uint32_t		queue_id;
	struct slot_info	timeslot;
	u_int64_t		tx_time;
	packet_type		burst_type;
	int			encode_data_len;
	struct link_info	linfo;
	struct link_budget	lbudget;
	struct log_info		loginfo;

};

class Packet;

class Dvb_pkt {

private:
	struct pbuf {
		u_int32_t	refcnt;		/* total number of reference to me */
		packet_type	type;		/* padload type */
		int		data_len;	/* padload data length */
		void		*data;		/* padload data pointer */
	};

	/*
	 * fake header offset, in order to let _flag offset are the same with
	 * Packet's _flags position, this reserved space have 32 bytes. If have
	 * other request can apply this space, but must quarantee _flag's
	 * offset is 32 bytes
	 */
public:
	char reserved[32];
	short	_flag;				/* flow direct flag */

	struct pbuf*	_pbuf;			/* pointer to struct pbuf */

	union {
		/* return link information */
		struct return_link_info		ret_info;
		/* return link information */
		struct forward_link_info	fwd_info;
	} _info;

private:
	void _free_pbuf();

public:
 	Dvb_pkt();
 	Dvb_pkt(Packet *pkt);
 	~Dvb_pkt();

	void *get_pbuf() {return _pbuf;};
	/*
	 * Set/Get flow direction flag.
	 */
	void pkt_setflag(enum flow_direct flag);
	enum flow_direct pkt_getflag();

	/*
	 * Set/Get data payload type.
	 */
	bool pkt_settype(enum packet_type);
	enum packet_type pkt_gettype();

	/*
	 * Get data & data length
	 */
	void* pkt_getdata();
	int pkt_getlen();

	/*
	 * Get information structure.
	 */
	struct return_link_info* pkt_getretinfo();
	struct forward_link_info* pkt_getfwdinfo();

	Dvb_pkt			*pkt_copy();
	int			pkt_attach(void *padload, const int len);
	void			*pkt_sattach(const int len);
	void			*pkt_detach();

	/* convert to NCTUns packet class */
	int			convert_from_nctuns_pkt(Packet *pkt);
	Packet			*convert_to_nctuns_pkt();
};


#endif	/* __NCTUNS_dvb_pkt_h__ */
