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

#ifndef __NCTUNS_nit_table_h__
#define __NCTUNS_nit_table_h__

#include <stdint.h>
#include <mylist.h>
#include "table.h"
#include "../descriptor/descriptor_q.h"

#define	NIT_TRANSPORT_STREAM_INFO_SIZE	sizeof(Nit_transport_stream_info)

#pragma pack(1)
/* Declare the entry of NIT transport stream information queue. */
struct Nit_transport_stream_info {

	u_int16_t	transport_stream_id;
	u_int16_t	original_network_id;

	/* Get functions */
	u_int16_t	get_transport_stream_id();

	u_int16_t	get_original_network_id();

	/* Set functions */
	int		set_transport_stream_id(u_int16_t para_transport_stream_id);

	int		set_original_network_id(u_int16_t para_original_network_id);
};


/* Declare the entry of NIT transport stream information queue. */
struct Nit_transport_stream_info_entry {
	struct Nit_transport_stream_info		transport_stream_info;
	Descriptor_circleq				*transport_descriptor_circleq;
	CIRCLEQ_ENTRY(Nit_transport_stream_info_entry)	entries;


	/* Constructor/Destructor */
	Nit_transport_stream_info_entry(Nit_transport_stream_info *transport_stream_info);
	~Nit_transport_stream_info_entry();

	/* return the length of this transport_stream_info in bytes */
	int		get_transport_stream_info_len();

	/* Get functions */
	u_int16_t 	get_transport_stream_id() { return 	transport_stream_info.get_transport_stream_id();}

	u_int16_t 	get_original_network_id() { return 	transport_stream_info.get_original_network_id();}

	u_int16_t	get_transport_descriptors_length() { return transport_descriptor_circleq->get_all_descriptor_total_length();}

	/* Set functions */
	int set_transport_stream_id(u_int16_t id) { return transport_stream_info.set_transport_stream_id(id);}

	int set_original_network_id(u_int16_t id) { return transport_stream_info.set_original_network_id(id);}

	/* Operator for descriptor circle queue */
	int		add_transport_loop_des (Descriptor *des);

	Descriptor	*get_transport_loop_des (uint8_t searched_des, int pnum, ...);

	Descriptor	*get_transport_loop_des_by_num (int num);

	int		remove_transport_loop_des (uint8_t searched_des, int pnum, ...);
};

/* Declare the NIT transport stream information queue. */
struct Nit_transport_stream_info_circleq {
  public:
	struct Nit_transport_stream_info_entry *cqh_first;		/* first element */	
	struct Nit_transport_stream_info_entry *cqh_last;		/* last element */

	void free();
	static void copy(Nit_transport_stream_info_circleq* dst, Nit_transport_stream_info_circleq* src);
};


class Nit : public Table {
  friend class Nit_table_to_section_handler;
  friend class Nit_section_to_table_handler;

  private:
	u_int16_t						_network_id;
	u_char							_version_number;
	u_char							_current_next_indicator;
	/* Note: network_descriptors_length==0 means no loop here.
	 * 	 while in the NIT section, network_descriptors_length==0 
	 *	 means one loop.
	 */

	Descriptor_circleq					*_network_descriptor_circleq;

	/* Note: transport_stream_loop_length==0 means no loop here.
	 * 	 while in the NIT section, transport_stream_loop_length==0 
	 *	 means one loop.
	 */

	u_int16_t						_transport_stream_loop_length;
	Nit_transport_stream_info_circleq			_nit_transport_stream_info_circleq;

  public:
			Nit();

			Nit(u_int16_t _network_id, u_char _version_number, 
				u_char _current_next_indicator);

			~Nit();

	Nit*		copy();

	/* Get functions */

	u_int16_t	get_network_id() {return _network_id;}

	u_char		get_version_number() {return _version_number;}

	u_char		get_current_next_indicator() {return _current_next_indicator;}

	u_int16_t	get_network_descriptors_length() {return _network_descriptor_circleq->get_all_descriptor_total_length();}

	u_int16_t	get_transport_stream_loop_length();


	/* Set functions */

	inline int	set_network_id(u_int16_t network_id) {
		_network_id = network_id;
		return (0);
	}

	inline int	set_version_number(u_char version_number) {
		_version_number = version_number;
		return (0);
	}

	inline int	set_current_next_indicator(u_char current_next_indicator) {
		_current_next_indicator = current_next_indicator;
		return (0);
	}

	/* Table operation functions */
	
	/*
	 * add function will add one descriptor to network loop or to transport loop.
	 * if add one descriptor to transport loop, then caller must assign transport_stream_id
	 */
	int		add_network_loop_des (Descriptor *des);

	int		add_transport_info (Nit_transport_stream_info *transport_info);

	int		add_transport_loop_des (u_int16_t transport_stream_id, Descriptor *des);

	/*
	 * get function will get the descriptor from network loop or from transport loop.
	 * if get the descriptor to transport loop, then caller must assign transport_stream_id
	 */
	Descriptor	*get_network_loop_des (uint8_t searched_des, int pnum, ...);

	Descriptor	*get_network_loop_des_by_num (int num);

	int		get_transport_info (u_int16_t transport_stream_id, Nit_transport_stream_info *info);

	int		get_transport_info_by_num (int num, Nit_transport_stream_info *info);

	Descriptor	*get_transport_loop_des (u_int16_t transport_stream_id, uint8_t searched_des, int pnum, ...);

	Descriptor	*get_transport_loop_des_by_num (u_int16_t transport_stream_id, int num);

	/*
	 * remove function will remove the descriptor from network loop or from transport loop.
	 * if remove the descriptor from transport loop, then caller must assign transport_stream_id
	 */
	int		remove_network_loop_des (uint8_t searched_des, int pnum, ...);

	int		remove_transport_info (u_int16_t transport_stream_id);

	int		remove_transport_loop_des (u_int16_t transport_stream_id, uint8_t searched_des, int pnum, ...);
}; // End of class Nit.
#pragma pack()
#endif /* __NCTUNS_nit_table_h__ */
