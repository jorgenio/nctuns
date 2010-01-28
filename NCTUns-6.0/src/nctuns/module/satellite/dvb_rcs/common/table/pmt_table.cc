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

#include "pmt_table.h"
#include "../descriptor/descriptor_q.h"
#include "../descriptor/stream_identifier_descriptor.h"
#include "../descriptor/data_broadcast_id_descriptor.h"
// All copy function, mark descriptor_circleq copy

Pmt_frame_info_entry::Pmt_frame_info_entry()
{
	pmt_frame_info_des_circleq = new Descriptor_circleq();
}

Pmt_frame_info_entry::Pmt_frame_info_entry(Pmt_frame_info info)
{
	frame_info = info;
	pmt_frame_info_des_circleq = new Descriptor_circleq();
}
void 	Pmt_frame_info_circleq::free() {
	Pmt_frame_info_entry*		_entry;

	while((_entry = cqh_first) != ((Pmt_frame_info_entry*) this))
	{	
		CIRCLEQ_REMOVE(this, _entry, entries);
		delete _entry;
	}								
}


void Pmt_frame_info_circleq::copy(Pmt_frame_info_circleq* dst,
		Pmt_frame_info_circleq* src) 
{
	Pmt_frame_info_entry		*_dst_ass_info_entry, *_src_ass_info_entry;


	CIRCLEQ_INIT(dst);						

	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Pmt_frame_info_entry(_src_ass_info_entry->frame_info);
		delete (_dst_ass_info_entry->pmt_frame_info_des_circleq);
		_dst_ass_info_entry->pmt_frame_info_des_circleq = ((_src_ass_info_entry->pmt_frame_info_des_circleq)->copy());
		CIRCLEQ_INSERT_HEAD(dst, _dst_ass_info_entry, entries);	
	}
}


Pmt::Pmt() {
	_table_id = PMT_TABLE_ID;
	CIRCLEQ_INIT(&_pmt_frame_info_circleq);
	_frame_loop_count = 0;
	_pmt_des_circleq = new Descriptor_circleq();
}


Pmt::Pmt(u_int16_t ts_id, u_char version_number, 
		u_char current_next_indicator)
{

	_table_id = PMT_TABLE_ID;
	_program_number = ts_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	CIRCLEQ_INIT(&_pmt_frame_info_circleq);
	_frame_loop_count = 0;
	_pmt_des_circleq = new Descriptor_circleq();
}


Pmt::~Pmt() {
	// Free the dynamically allocated queue.
	delete (_pmt_des_circleq);
	_pmt_frame_info_circleq.free();
}

Pmt* Pmt::copy() {
	Pmt*		clone;
	

	// Copy all fields but the dynamically allocated queue.
	clone = new Pmt(*this);

	Pmt_frame_info_circleq::copy(&(clone-> _pmt_frame_info_circleq), 
				     &(this-> _pmt_frame_info_circleq));

	clone->_pmt_des_circleq = (this->_pmt_des_circleq->copy());
	
	return clone;
}

int	Pmt::add_frame_info(Pmt_frame_info frame_info)
{
	Pmt_frame_info_entry			*_ptr_frame_info_entry, *_new_frame_info_entry;
	bool					_frame_id_exist;
	u_int16_t				_elementary_pid;



	// Check if the frame id exists.
	_frame_id_exist = false;


	// Check if the frame elementary_pid is already used.	
	_elementary_pid = frame_info.elementary_pid;	

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(_elementary_pid == _ptr_frame_info_entry-> get_elementary_pid() )
			return 1;
	}


	// Create a frame info entry.
	frame_info.set_es_info_length(0);
	_new_frame_info_entry = new Pmt_frame_info_entry(frame_info);


	// Attach one node onto the frame info queue within the superframe info.
	CIRCLEQ_INSERT_HEAD(&_pmt_frame_info_circleq,
			_new_frame_info_entry, entries);

	// Increase the frame_loop_count field.
	_frame_loop_count++;


	return 0;
}// End of Pmt::add_frame_info()



int	Pmt::get_frame_info(int elementary_pid, Pmt_frame_info* frame_info_buf)
{
	Pmt_frame_info_entry			*_ptr_frame_info_entry;


	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(elementary_pid == _ptr_frame_info_entry-> get_elementary_pid())// Superframe info entry is found.
		{
			// Copy the frame info.
			*frame_info_buf = _ptr_frame_info_entry-> frame_info;

			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;
}//End of Pmt::get_frame_info


int	Pmt::remove_frame_info(int elementary_pid)
{
	Pmt_frame_info_entry			*_ptr_frame_info_entry;


	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(elementary_pid == _ptr_frame_info_entry-> get_elementary_pid())// Superframe info entry is found.
		{
			// Free the frame info queue within frame info.
			(_ptr_frame_info_entry->pmt_frame_info_des_circleq)->free();

			CIRCLEQ_REMOVE(&_pmt_frame_info_circleq, _ptr_frame_info_entry, entries);
			delete(_ptr_frame_info_entry);

			_frame_loop_count--;

			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;
}// End of Pmt::remove_frame_info


// program_info_descriptor operations

int	Pmt::add_program_info_descriptor(Descriptor *dct)
{
	_pmt_des_circleq->add_descriptor(dct);

	return 0;
}

Descriptor*	Pmt::get_program_info_descriptor(int pnum , uint8_t searched_des , ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	Descriptor		*dct;
	
	va_start(parg , searched_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			dct = _pmt_des_circleq->get_descriptor(pnum, searched_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			dct = _pmt_des_circleq->get_descriptor(pnum, searched_des, p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			dct = _pmt_des_circleq->get_descriptor(pnum, searched_des, p1, p2);
			break;
		}		
		/* wrong input */
		default:{

			va_end(parg);	
			return 0;
		}
	}
	va_end(parg);	
	
	return dct;
}

int	Pmt::remove_program_info_descriptor(int pnum , uint8_t deleted_des , ...)
{
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , deleted_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			_pmt_des_circleq->remove_descriptor(pnum, deleted_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			_pmt_des_circleq->remove_descriptor(pnum, deleted_des, p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			_pmt_des_circleq->remove_descriptor(pnum, deleted_des, p1, p2);
			break;
		}		
		/* wrong input */
		default:{

			va_end(parg);	
			return -1;
		}
	}
	va_end(parg);

	return 1;
}




// es_info_descriptor operations
int	Pmt::add_es_info_descriptor(int elementary_pid, Descriptor *dct)
{
	Pmt_frame_info_entry		*_ptr_frame_info_entry;
	int				size;
	
	size=0;
	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(_ptr_frame_info_entry->get_elementary_pid() == elementary_pid)
		{
			_ptr_frame_info_entry->pmt_frame_info_des_circleq->add_descriptor(dct);
			size = _ptr_frame_info_entry->get_es_info_length();
			size += dct->get_descriptor_total_len();
			_ptr_frame_info_entry->set_es_info_length(size);

			return 1;
		}
	}

	return -1;
}

Descriptor*	Pmt::get_es_info_descriptor(int elementary_pid, int pnum , uint8_t searched_des , ...)
{
	Pmt_frame_info_entry		*_ptr_frame_info_entry;
	Descriptor_circleq		*_ptr_frame_info_des_circleq;
	Descriptor			*dct=0;


	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(_ptr_frame_info_entry->get_elementary_pid() == elementary_pid)
		{
			_ptr_frame_info_des_circleq = (_ptr_frame_info_entry->pmt_frame_info_des_circleq);


			va_list parg;
			u_long p1 = 0;
			u_long p2 = 0;
			
			va_start(parg , searched_des);

			switch (pnum) {

				/* 1 parameter */
				case (1):{

					dct = _ptr_frame_info_des_circleq->get_descriptor(pnum, searched_des);
					break;
				}
				/* 2 parameters */
				case (2):{

					p1 = va_arg(parg , u_long);
					dct = _ptr_frame_info_des_circleq->get_descriptor(pnum, searched_des, p1);
					if(dct==0) printf("@@@@@@@ error get null dct\n");
					break;
				}
				/* 3 parameters */
				case (3):{

					p1 = va_arg(parg , u_long);
					p2 = va_arg(parg , u_long);
					dct = _ptr_frame_info_des_circleq->get_descriptor(pnum, searched_des, p1, p2);
					if(dct==0) printf("@@@@@@@ error get null dct\n");
					break;
				}		
				/* wrong input */
				default:{

					va_end(parg);	
					return 0;
				}
			}
			va_end(parg);	
	
			return dct;
		}
	}
	return 0;

}

int	Pmt::remove_es_info_descriptor(int elementary_pid, int pnum , uint8_t deleted_des , ...)
{

	Pmt_frame_info_entry		*_ptr_frame_info_entry;
	Descriptor_circleq		*_ptr_frame_info_des_circleq;



	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_pmt_frame_info_circleq, entries)
	{
		if(_ptr_frame_info_entry->get_elementary_pid() == elementary_pid)
		{
			_ptr_frame_info_des_circleq = (_ptr_frame_info_entry->pmt_frame_info_des_circleq);

			va_list parg;
			u_long p1 = 0;
			u_long p2 = 0;
			
			va_start(parg , deleted_des);

			switch (pnum) {

				/* 1 parameter */
				case (1):{
					_ptr_frame_info_des_circleq->remove_descriptor(pnum, deleted_des);
					break;
				}
				/* 2 parameters */
				case (2):{
					p1 = va_arg(parg , u_long);
					_ptr_frame_info_des_circleq->remove_descriptor(pnum, deleted_des, p1);
					break;
				}
				/* 3 parameters */
				case (3):{
					p1 = va_arg(parg , u_long);
					p2 = va_arg(parg , u_long);
					_ptr_frame_info_des_circleq->remove_descriptor(pnum, deleted_des, p1, p2);
					break;
				}		
				/* wrong input */
				default:{
					va_end(parg);	
					return -1;
				}
			}
			va_end(parg);	
			
			return 1;
		}
	}
	return -1;
}


int	Pmt::get_es_info_des_pid(int pnum, u_int8_t tag, ...)
{
	Descriptor		*dxt;
	Pmt_frame_info_entry	*pfie;

	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , tag);
	p1 = va_arg(parg , u_long);
	p2 = va_arg(parg , u_long);
	va_end(parg);


CIRCLEQ_FOREACH(pfie,  &_pmt_frame_info_circleq, entries)
{
	_pmt_des_circleq = pfie->pmt_frame_info_des_circleq;
	switch (pnum) {

		/* 1 parameter */
		case (1):{

			dxt = _pmt_des_circleq->get_descriptor(pnum, tag);
			if(dxt != 0)
			{
				delete dxt;
				return pfie->get_elementary_pid();
			}
			delete dxt;
			break;
		}
		/* 2 parameters */
		case (2):{

			dxt = _pmt_des_circleq->get_descriptor(pnum, tag, p1);
			if(dxt != 0)
			{
				delete dxt;
				return pfie->get_elementary_pid();
			}
			delete dxt;
			break;
		}
		/* wrong input */
		default:{

			delete dxt;
			return -1;
		}
	}
}
	return -1;

}



