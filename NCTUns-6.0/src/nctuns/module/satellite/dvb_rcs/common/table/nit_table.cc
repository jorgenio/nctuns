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

// nit_table.cc

#include <stdarg.h>
#include <stdint.h>
#include "section_draft.h"
#include "nit_table.h"
#include "../descriptor/descriptor_q.h"


/***************************************************************************
 * struct Nit_transport_stream_info
 ***************************************************************************/
/*
 * Get functions of struct Nit_transport_stream_info
 */
u_int16_t Nit_transport_stream_info::get_transport_stream_id()
{
	return transport_stream_id;
}

u_int16_t Nit_transport_stream_info::get_original_network_id()
{
	return original_network_id;
}

/*
 * Set functions of struct Nit_transport_stream_info
 */
int Nit_transport_stream_info::
set_transport_stream_id(u_int16_t para_transport_stream_id)
{
	transport_stream_id = para_transport_stream_id;
	return (0);
}

int Nit_transport_stream_info::
set_original_network_id(u_int16_t para_original_network_id)
{
	original_network_id = para_original_network_id;
	return (0);
}


/***************************************************************************
 * struct Nit_transport_stream_info_entry
 ***************************************************************************/
/*
 * Constructor/Destructor
 */
Nit_transport_stream_info_entry::Nit_transport_stream_info_entry(Nit_transport_stream_info * info):transport_descriptor_circleq
	(NULL)
{
	if (info)
		transport_stream_info = *info;

	transport_descriptor_circleq = new Descriptor_circleq();
}

Nit_transport_stream_info_entry::~Nit_transport_stream_info_entry()
{
	delete transport_descriptor_circleq;
}

int Nit_transport_stream_info_entry::get_transport_stream_info_len()
{
	return NIT_TRANSPORT_STREAM_INFO_SIZE +
		transport_descriptor_circleq->
		get_all_descriptor_total_length();
}


/*
 * Operator for descriptor circle queue
 */
int Nit_transport_stream_info_entry::add_transport_loop_des(Descriptor *
							    des)
{
	/*
	 * because in descriptor, success will return 0 or else return -1, but
	 * in table success will return 0 or else return 1
	 */
	return transport_descriptor_circleq->add_descriptor(des) ? 1 : 0;
}

Descriptor *Nit_transport_stream_info_entry::
get_transport_loop_des(uint8_t searched_des, int pnum, ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);
	return transport_descriptor_circleq->get_descriptor(pnum,
							    searched_des,
							    p1, p2);
}

Descriptor *Nit_transport_stream_info_entry::
get_transport_loop_des_by_num(int num)
{
	return transport_descriptor_circleq->
		get_descriptor_by_location_num(num);
}

int Nit_transport_stream_info_entry::
remove_transport_loop_des(uint8_t searched_des, int pnum, ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);
	/*
	 * because in descriptor, success will return 0 or else return -1, but
	 * in table success will return 0 or else return 1
	 */
	return transport_descriptor_circleq->remove_descriptor(pnum,
							       searched_des,
							       p1,
							       p2) ? 1 : 0;
}



/***************************************************************************
 * struct Nit_transport_stream_info_circleq
 ***************************************************************************/
void Nit_transport_stream_info_circleq::free()
{
	Nit_transport_stream_info_entry *entry;

	while((entry = cqh_first) != ((Nit_transport_stream_info_entry*) this))
	{	
		CIRCLEQ_REMOVE(this, entry, entries);
		delete entry;
	}
}

void Nit_transport_stream_info_circleq::
copy(Nit_transport_stream_info_circleq * dst,
     Nit_transport_stream_info_circleq * src)
{
	Nit_transport_stream_info_entry *entry, *current;

	CIRCLEQ_INIT(dst);
	CIRCLEQ_FOREACH(current, src, entries) {
		entry = new Nit_transport_stream_info_entry(&current->
							    transport_stream_info);

		delete (entry->transport_descriptor_circleq);

		entry->transport_descriptor_circleq =
			current->transport_descriptor_circleq->copy();
		CIRCLEQ_INSERT_TAIL(dst, entry, entries);
	}
}


/***************************************************************************
 * class Nit
 ***************************************************************************/
/*
 * Constructor/Destructor
 */
Nit::Nit()
{
	_table_id = NIT_ACTIVE_TABLE_ID;
	CIRCLEQ_INIT(&_nit_transport_stream_info_circleq);

	_network_descriptor_circleq = new Descriptor_circleq();
}

Nit::Nit(u_int16_t network_id, u_char version_number,
	 u_char current_next_indicator)
{
	_table_id = NIT_ACTIVE_TABLE_ID;
	_network_id = network_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	CIRCLEQ_INIT(&_nit_transport_stream_info_circleq);

	_network_descriptor_circleq = new Descriptor_circleq();
}

Nit::~Nit()
{
	// Free the dynamically allocated queue.
	_nit_transport_stream_info_circleq.free();
	delete _network_descriptor_circleq;
}

Nit *Nit::copy()
{
	Nit *nit = new Nit(*this);

	/*
	 * copy network descriptor queue
	 */
	nit->_network_descriptor_circleq =
		_network_descriptor_circleq->copy();

	Nit_transport_stream_info_circleq::copy(&nit->
						_nit_transport_stream_info_circleq,
						&_nit_transport_stream_info_circleq);

	return nit;
}

/*
 * calcuate total transport_stream_loop length function of Nit class
 */
u_int16_t Nit::get_transport_stream_loop_length()
{
	Nit_transport_stream_info_entry *entry;
	int len = 0;

	CIRCLEQ_FOREACH(entry, &_nit_transport_stream_info_circleq,
			entries) {
		/*
		 * 2 bytes is length of transport_descriptors_length field
		 */
		len += entry->get_transport_stream_info_len() + 2;
	}

	return len;
}

/*
 * add functions of Nit class
 */
int Nit::add_network_loop_des(Descriptor * des)
{
	/*
	 * because in descriptor, success will return 0 or else return -1, but
	 * in table success will return 0 or else return 1
	 */
	return _network_descriptor_circleq->add_descriptor(des) ? 1 : 0;
}

int Nit::add_transport_info(Nit_transport_stream_info * transport_info)
{
	struct Nit_transport_stream_info_entry *entry;

	/*
	 * check this transport_stream_id whether be used
	 */
	CIRCLEQ_FOREACH(entry, &_nit_transport_stream_info_circleq,
			entries) {
		if (transport_info->transport_stream_id ==
		    entry->get_transport_stream_id())
			return (1);
	}

	/*
	 * create a new entry and mount to circle queue
	 */
	entry = new Nit_transport_stream_info_entry(transport_info);
	CIRCLEQ_INSERT_TAIL(&_nit_transport_stream_info_circleq, entry,
			    entries);

	return (0);
}

int Nit::add_transport_loop_des(u_int16_t transport_stream_id,
				Descriptor * des)
{
	Nit_transport_stream_info_entry *info;

	CIRCLEQ_FOREACH(info, &_nit_transport_stream_info_circleq, entries) {
		if (info->get_transport_stream_id() == transport_stream_id)
			return info->add_transport_loop_des(des);
	}
	return (1);
}

/*
 * get functions of Nit class
 */
Descriptor *Nit::get_network_loop_des(uint8_t searched_des, int pnum, ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);
	/*
	 * because in descriptor, success will return 0 or else return -1, but
	 * in table success will return 0 or else return 1
	 */
	return _network_descriptor_circleq->get_descriptor(pnum,
							   searched_des,
							   p1, p2);
}

Descriptor *Nit::get_network_loop_des_by_num(int num)
{
	return _network_descriptor_circleq->
		get_descriptor_by_location_num(num);
}

int Nit::get_transport_info(u_int16_t transport_stream_id,
			    struct Nit_transport_stream_info *info)
{
	Nit_transport_stream_info_entry *entry;

	CIRCLEQ_FOREACH(entry, &_nit_transport_stream_info_circleq,
			entries) {
		if (entry->get_transport_stream_id() ==
		    transport_stream_id) {
			if (info)
				*info = entry->transport_stream_info;
			return (0);
		}
	}

	return (1);
}

int Nit::get_transport_info_by_num(int num,
				   struct Nit_transport_stream_info *info)
{
	Nit_transport_stream_info_entry *entry;
	int i = 0;

	CIRCLEQ_FOREACH(entry, &_nit_transport_stream_info_circleq,
			entries) {
		if (i++ == num) {
			if (info)
				*info = entry->transport_stream_info;
			return (0);
		}
	}

	return (1);
}

Descriptor *Nit::get_transport_loop_des(u_int16_t transport_stream_id,
					uint8_t searched_des, int pnum,
					...)
{
	struct Nit_transport_stream_info_entry *info;
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);

	CIRCLEQ_FOREACH(info, &_nit_transport_stream_info_circleq, entries) {
		if (info->get_transport_stream_id() == transport_stream_id)
			return info->get_transport_loop_des(searched_des,
							    pnum, p1, p2);
	}
	return NULL;
}

Descriptor *Nit::
get_transport_loop_des_by_num(u_int16_t transport_stream_id, int num)
{
	struct Nit_transport_stream_info_entry *info;

	CIRCLEQ_FOREACH(info, &_nit_transport_stream_info_circleq, entries) {
		if (info->get_transport_stream_id() == transport_stream_id)
			return info->get_transport_loop_des_by_num(num);
	}
	return NULL;
}

/*
 * remove functions of Nit class
 */
int Nit::remove_network_loop_des(uint8_t searched_des, int pnum, ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);
	/*
	 * because in descriptor, success will return 0 or else return -1, but
	 * in table success will return 0 or else return 1
	 */
	return _network_descriptor_circleq->remove_descriptor(pnum,
							      searched_des,
							      p1,
							      p2) ? 1 : 0;
}

int Nit::remove_transport_info(u_int16_t transport_stream_id)
{
	struct Nit_transport_stream_info_entry *info;

	CIRCLEQ_FOREACH(info, &_nit_transport_stream_info_circleq, entries) {
		if (info->get_transport_stream_id() == transport_stream_id) {
			CIRCLEQ_REMOVE(&_nit_transport_stream_info_circleq,
				       info, entries);
			delete info;

			return (0);
		}
	}

	return (1);
}

int Nit::remove_transport_loop_des(u_int16_t transport_stream_id,
				   uint8_t searched_des, int pnum, ...)
{
	Nit_transport_stream_info_entry *info;
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;

	va_start(parg, pnum);
	p1 = va_arg(parg, u_long);
	p2 = va_arg(parg, u_long);
	va_end(parg);

	CIRCLEQ_FOREACH(info, &_nit_transport_stream_info_circleq, entries) {
		if (info->get_transport_stream_id() == transport_stream_id)
			return info->remove_transport_loop_des(pnum,
							       searched_des,
							       p1, p2);
	}
	return (1);
}
