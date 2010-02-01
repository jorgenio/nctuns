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

#ifndef __NCTUNS_WIMAX_MSH_DSCH_H__
#define __NCTUNS_WIMAX_MSH_DSCH_H__


#include <vector>
#include "sch/ctrl/entry_interface.h"
#include "../management_message.h"


struct MSH_DSCH_field {
	u_char CoordFlag:1;
	u_char GrantFlag:1;
	u_char SeqCounter:6;
	u_char NRequest:4;
	u_char NAvail:4;
	u_char NGrant:6;
	u_char reserve:2;
};

struct DSCH_SchedEntry {
	u_char msbNbrNodeID;
	u_char lsbNbrNodeID;
	u_char NextXmtMx:5;
	u_char XmtHoldExp:3;
};

struct DSCH_SchedulingIE_field {
	u_char NextXmtMx:5;
	u_char XmtHoldExp:3;
	u_char NSchedEntry:8;
};

struct DSCH_SchedulingIE {
	struct DSCH_SchedulingIE_field*	f;
	struct DSCH_SchedEntry*		entry;
};

/* Not aligned with the byte boundary */
struct DSCH_RequestIE
{
	u_char LinkID:8;
	u_char DemandLevel:8;
	u_char DemandPersist:3;
	u_char reserve:1;
};

struct DSCH_AvailabilityIE {
	u_char StartNumber:8;
	u_char SlotStart:8;
	u_char SlotRange:7;
	u_char Direction:2;
	u_char Persistence:3;
	u_char Channel:4;
};

struct DSCH_GrantIE {
	u_char LinkID:8;
	u_char StartNumber:8;
	u_char SlotStart:8;
	u_char SlotRange:8;
	u_char Direction:1;
	u_char Persistence:3;
	u_char Channel:4;
};

class Alloc_base;
class Data_scheduler;
class Mesh_connection;
class Neighbor;

class MSH_DSCH: public ManagementMessage, public Ctrl_sch::Entry_interface {

private:
	enum {
		NF_REQUEST_IE_MAX	= 16,
		NF_AVAILABILITY_IE_MAX	= 16,
		NF_GRANT_IE_MAX		= 63,

		DIRECTION_FROM_REQUEST	= 0,
		DIRECTION_TO_REQUEST	= 1,
	};

public:
	enum {
		FRAME_BIT_MASK		= 0x00ff
	};

	enum {
		IS_LAST_GRANT		= 1,
		IS_NOT_LAST_GRANT	= 0,
		IS_LAST_CONFIRM		= 1,
		IS_NOT_LAST_CONFIRM	= 0
	};

private:
	/* Fields of fixed length */
	struct MSH_DSCH_field*     f;
	struct DSCH_SchedulingIE   Scheduling;
	DSCH_AvailabilityIE*       Availability;
	DSCH_RequestIE*            Request;
	DSCH_GrantIE*              Grant;

public:
	MSH_DSCH();
	MSH_DSCH(u_char* pBuffer, int pLength);
	~MSH_DSCH();

	int copyTo(u_char* dstbuf) const;
	int getLen() const;

	void setSeqCounter(u_char pCnt) {
		f->SeqCounter = pCnt;
	}
	void setGrantFlag(u_char pFlag) {
		f->GrantFlag = pFlag;
	}

	int addSchedParam(u_int32_t pCurrXmtTime, u_int32_t pNextXmtTime,
			  u_char pHoldoff, u_int32_t maxXmtOpps);
	int add_nbr_sch_entry(Ctrl_sch::Entry&);

	/* new-version API functions */
	int add_request_info(
			Data_scheduler& scheduler,
			uint16_t cur_frame,
			uint32_t cur_tx_time,
			uint32_t& num_of_requested_data_schedules);

	void add_grant_info(Data_scheduler& scheduler, uint16_t cur_frame, uint32_t txopp);

	int add_confirm_info(Data_scheduler& scheduler,
			uint16_t cur_frame, uint32_t cur_tx_time,
			uint32_t& num_of_granted_data_schedules);

	int set_tx_holdoff_exp(u_int32_t htime_exp_val);

	inline u_char getCoordFlag() { return f->CoordFlag; }
	inline u_char getGrantFlag() { return f->GrantFlag; }
	inline u_char getSeqCounter() { return f->SeqCounter; }
	inline u_char getNumRequest() { return f->NRequest; }
	inline u_char getNumAvail() { return f->NAvail; }
	u_char getNumGrant();
	u_char getNumConfirm();

	void proc_availability(class Data_scheduler&, class Neighbor*, uint16_t);
	void proc_request(class Data_scheduler&, class Neighbor*);
	void proc_grant(class Data_scheduler&, class Neighbor*, uint16_t);
	void proc_confirm(class Data_scheduler&, class Neighbor*, uint16_t);

	bool get_availability_info(Alloc_base&, int, uint16_t);
	bool get_request_info(Alloc_base&, int);
	bool get_grant_info(Alloc_base&, int, uint16_t);
	bool get_confirm_info(Alloc_base&, int, uint16_t);

        inline void get_sch_param(Ctrl_sch::Entry& sch_entry)
        {
            sch_entry.set_next_tx_mx(Scheduling.f->NextXmtMx);
            sch_entry.set_tx_holdoff_exp(Scheduling.f->XmtHoldExp);
        }

        inline uint8_t get_nf_nbr_sch_entry() const { return Scheduling.f->NSchedEntry; }
	void get_nbr_sch_entry(Ctrl_sch::Entry&, int) const;

private:
	bool _add_request_info(Data_scheduler&,
			const Mesh_connection*, uint16_t, uint32_t);

	size_t _add_grant_info(class Data_scheduler*,
			const Alloc_base*, const Alloc_base*,
			uint16_t, size_t needed_area = 0);

	void _adjust_frame_range(Alloc_base&);

	bool _encapsulate_availability(const Alloc_base&);
	bool _encapsulate_request(const Alloc_base&);
	bool _encapsulate_grant(const Alloc_base&);
	bool _encapsulate_confirm(const Alloc_base&);

	uint16_t _frame_sync(uint8_t, uint16_t);

	uint8_t _persistence2frame(uint8_t);
	uint8_t _frame2persistence(uint8_t);

	int _get_numof_grant_matched(class Data_scheduler&, class Neighbor*, uint16_t);
	int _get_numof_confirm_matched(class Data_scheduler&, class Neighbor*, uint16_t);
};


#endif /* __NCTUNS_WIMAX_MSH_DSCH_H__ */
