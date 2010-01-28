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

// tim_table.cc

#include "tim_table.h"

Tim::Tim() {
	_table_id = TIM_TABLE_ID;
	_descriptor_loop_count = 0;
	_tim_des_circleq = new Descriptor_circleq();
}

Tim::Tim(u_char current_next_indicator)
{
	_table_id = TIM_TABLE_ID;
	_current_next_indicator = current_next_indicator;
	_descriptor_loop_count = 0;
	_tim_des_circleq = new Descriptor_circleq();
}


Tim::~Tim() {
	// Free the dynamically allocated queue.
	delete (_tim_des_circleq);
}


Tim* Tim::copy() {
	Tim*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Tim(*this);

	clone->_tim_des_circleq = this->_tim_des_circleq->copy();
	return clone;
}

int
Tim::add_descriptor(Descriptor *dct){
	_tim_des_circleq->add_descriptor(dct);
	_descriptor_loop_count += 1;
	return 0;
}

Descriptor*             
Tim::get_descriptor(int pnum , uint8_t searched_des , ...){
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	Descriptor		*dct;
	
	va_start(parg , searched_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			dct = _tim_des_circleq->get_descriptor(pnum, searched_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			dct = _tim_des_circleq->get_descriptor(pnum, searched_des, p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			dct = _tim_des_circleq->get_descriptor(pnum, searched_des, p1, p2);
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

int
Tim::remove_descriptor(int pnum , uint8_t deleted_des , ...){
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , deleted_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			_tim_des_circleq->remove_descriptor(pnum, deleted_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			_tim_des_circleq->remove_descriptor(pnum, deleted_des, p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			_tim_des_circleq->remove_descriptor(pnum, deleted_des, p1, p2);
			break;
		}		
		/* wrong input */
		default:{

			va_end(parg);	
			return -1;
		}
	}
	va_end(parg);

	_descriptor_loop_count -= 1;

	return 1;
}


