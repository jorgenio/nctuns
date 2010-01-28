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

#include "nit_section.h"
#include "../descriptor/linkage_descriptor.h"

#define	mem_location (void*)((char*)_section + _offset)


void Nit_table_to_section_handler::
_calculate_last_section_number_and_section_length()
{
	u_int32_t offset, sec_cnt = 0;
	bool end_of_table = false;
	bool end_of_1st_loop = false;
	bool cut = false;
	Descriptor *des;

	int des1_start_num, des1_cnt, des1_len;
	int des2_start_num, des2_cnt, des2_len;
	Nit_transport_stream_info_entry *te, *ts;
	int i;

	des2_cnt = des1_cnt = 0;
	te = CIRCLEQ_FIRST(&_nit_table->
			   _nit_transport_stream_info_circleq);
	while (!end_of_table) {
		// Initialization.
		cut = false;
		offset = NIT_SECTION_DRAFT_SIZE +
			DESCRIPTORS_LENGTH_FIELD_LEN;

		/*
		 * compute network descriptor loop
		 */
		if (!end_of_1st_loop) {
			des1_start_num = des1_cnt;
			i = 0;
			for (;
			     des1_cnt <
			     _nit_table->_network_descriptor_circleq->
			     descriptor_count; des1_cnt++) {
				des = _nit_table->
					get_network_loop_des_by_num
					(des1_cnt);
				des1_len = des->get_descriptor_total_len();

				/*
				 * remember to delete descriptor because of get
				 * function will copy a descriptor and return it
				 */
				delete (Linkage_descriptor*)des;

				/*
				 * check descriptor length whether more then
				 * max section size, if over it, cut here
				 */
				if (offset + des1_len >
				    MAX_NIT_SECTION_SIZE) {
					INSERT_SEC_BUF(&_sec_buf_head,
						       offset + CRC32_SIZE, sec_cnt,
						       des1_start_num,
						       des1_cnt, i,
						       NULL, NULL, 0, 0,
						       0);
					++sec_cnt;
					cut = true;
					break;
				}
				offset += des1_len;
				i += des1_len;
			}
			/*
			 * has be cut, then do next loop
			 */
			if (cut)
				continue;

			des1_len = i;
			end_of_1st_loop = true;
		}
		offset += DESCRIPTORS_LENGTH_FIELD_LEN;
		ts = te;
		if (des1_cnt == 0)
			++des1_cnt;
		des2_start_num = des2_len = 0;
		i = 0;

		/*
		 * compute transport_stream loop
		 */
		des2_start_num = des2_cnt;
		while ((Nit_transport_stream_info_circleq *) te !=
		       &_nit_table->_nit_transport_stream_info_circleq) {
			offset +=
				NIT_TRANSPORT_STREAM_INFO_SIZE +
				DESCRIPTORS_LENGTH_FIELD_LEN;

			for (;
			     des2_cnt <
			     te->transport_descriptor_circleq->
			     descriptor_count; des2_cnt++) {
				des = _nit_table->
					get_network_loop_des_by_num
					(des2_cnt);
				des2_len = des->get_descriptor_total_len();

				/*
				 * remember to delete descriptor because of get
				 * function will copy a descriptor and return it
				 */
				delete des;

				/*
				 * check descriptor length whether more then
				 * max section size, if over it, cut here
				 */
				if (offset + des2_len >
				    MAX_NIT_SECTION_SIZE) {
					if (ts == te
					    &&
					    (Nit_transport_stream_info_circleq
					     *) te ==
					    &_nit_table->
					    _nit_transport_stream_info_circleq)
						offset -=
							NIT_TRANSPORT_STREAM_INFO_SIZE;

					INSERT_SEC_BUF(&_sec_buf_head,
						       offset + CRC32_SIZE, sec_cnt,
						       des1_start_num,
						       des1_cnt, des1_len,
						       ts, te,
						       des2_start_num,
						       des2_cnt, i);
					++sec_cnt;
					cut = true;
					des1_start_num = des1_cnt =
						des1_len = 0;
					break;
				}
				offset += des2_len;
				i += des2_len;
			}
			/*
			 * has be cut, then do next loop
			 */

			if (cut)
				break;

			te = CIRCLEQ_NEXT(te, entries);
			if ((Nit_transport_stream_info_circleq *) te !=
			    &_nit_table->
			    _nit_transport_stream_info_circleq) {
				des2_cnt = 0;
			}
		}
		if ((Nit_transport_stream_info_circleq *) te ==
		    &_nit_table->_nit_transport_stream_info_circleq)
			end_of_table = true;
	}
	if (des2_cnt == 0)
		++des2_cnt;
	INSERT_SEC_BUF(&_sec_buf_head, offset + CRC32_SIZE, sec_cnt, des1_start_num,
		       des1_cnt, des1_len, ts,
		       _nit_table->_nit_transport_stream_info_circleq.
		       cqh_last, des2_start_num, des2_cnt, i);

	// Set the variable '_last_section_number'.
	_last_section_number = sec_cnt;

}				// End of Nit_table_to_section_handler::_calculate_last_section_number_and_section_length()



void Nit_table_to_section_handler::nit_table_to_section_init(Nit *
							     nit_table)
{
	CIRCLEQ_INIT(&_sec_buf_head);

	// Fetch information from the input table.
	_nit_table = nit_table;
	_table_id = _nit_table->get_table_id();
	_network_id = _nit_table->get_network_id();
	_version_number = _nit_table->get_version_number();
	_current_next_indicator = _nit_table->get_current_next_indicator();

	_calculate_last_section_number_and_section_length();

	// Initialize data which will be used.
	_current_section_number = 0;
}				// End of Nit_section::nit_table_to_section_init()

void *Nit_table_to_section_handler::nit_table_to_section()
{
	u_int32_t offset;
	struct nit_sec_buf *sbuf;
	void *section;
	Nit_section_draft *ptr_draft;
	Nit_transport_stream_info_entry *info;
	Descriptor *des;
	u_int16_t des_len;
	u_int16_t *len_field, *count;
	int cnt;

	// No more section is generated.
	if (_current_section_number > _last_section_number)
		return NULL;

	/*
	 * fetch the first cut pointer buffer in buf linked list
	 */
	sbuf = CIRCLEQ_FIRST(&_sec_buf_head);
	assert(sbuf->section_number == _current_section_number);


	// Memory allocation for one section.
	section = malloc(sbuf->len);

	/*
	 * The draft is in the top of section.
	 * And we copy the draft to the right assition.
	 */
	ptr_draft = (Nit_section_draft *) section;

	ptr_draft->set_section_draft(_table_id,
				     0x1, 0x0,
				     sbuf->len - SECTION_DRAFT_SIZE +
				     CRC32_SIZE);

	ptr_draft->set_si_type_a_section_draft(_network_id,
					       _version_number,
					       _current_next_indicator,
					       _current_section_number,
					       _last_section_number);

	++_current_section_number;
	offset = NIT_SECTION_DRAFT_SIZE;

	/*
	 * write network descriptor length
	 */
	memcpy((void *)((u_int32_t) section + offset), &sbuf->des_len_1st,
	       DESCRIPTORS_LENGTH_FIELD_LEN);
	offset += DESCRIPTORS_LENGTH_FIELD_LEN;

	/*
	 * copy network descriptor loop
	 */
	if (sbuf->des_len_1st > 0) {
		/*
		 * according to cut pointer buffer, copy index of network
		 * descriptor between des_start_num_1st and des_start_num_1st +
		 * des_cnt_1st
		 */
		for (int i = sbuf->des_start_num_1st;
		     i < sbuf->des_cnt_1st; i++) {
			assert(des =
			       _nit_table->get_network_loop_des_by_num(i));

			assert(des_len =
			       des->
			       descriptor_serialize((u_char *) ((u_int32_t)
								section +
								offset)) ==
			       0);
			offset += des->get_descriptor_total_len();
			delete (Linkage_descriptor*)des;
		}
	}

	/*
	 * checkpointer, if section is full, then return and free section buffer
	 */
	if (offset == (u_int32_t)sbuf->len) {
		CIRCLEQ_REMOVE(&_sec_buf_head, sbuf, list);
		free(sbuf);

		fill_crc(section);
		return section;
	}

	/*
	 * get transport_stream_loop_length pointer
	 */
	*(len_field = (u_int16_t *) ((u_int32_t) section + offset)) = 0;
	offset += DESCRIPTORS_LENGTH_FIELD_LEN;
	info = sbuf->ts_start;
	cnt = sbuf->des_start_num_2nd;
	do {
		/*
		 * if ts and equal to head, then it means that don't exist any
		 * transport stream in this table, and return this section
		 */
		if (sbuf->ts_start == sbuf->ts_end &&
		    (Nit_transport_stream_info_circleq *) sbuf->ts_start ==
		    &_nit_table->_nit_transport_stream_info_circleq) {
			assert(offset == (u_int32_t)sbuf->len - CRC32_SIZE);
			CIRCLEQ_REMOVE(&_sec_buf_head, sbuf, list);
			free(sbuf);

			fill_crc(section);
			return section;
		}
		/*
		 * memory copy for transport stream info
		 */
		memcpy((void *)((u_int32_t) section + offset), info, NIT_TRANSPORT_STREAM_INFO_SIZE);
		offset += NIT_TRANSPORT_STREAM_INFO_SIZE;

		*len_field +=
			NIT_TRANSPORT_STREAM_INFO_SIZE +
			DESCRIPTORS_LENGTH_FIELD_LEN;

		/*
		 * write transport descriptor length
		 */
		*(count = (u_int16_t *) ((u_int32_t) section + offset)) =
			0;
		offset += DESCRIPTORS_LENGTH_FIELD_LEN;

		/*
		 * copy transport descriptor to section from descriptor between
		 * index sbuf->des_start_num_2nd of sbuf->ts_start and index
		 * sbuf->des_cnt_2nd of sbuf->tx_end, (not include
		 * sbuf->des_cnt_2nd)
		 */
		while (info != sbuf->ts_end || cnt != sbuf->des_cnt_2nd) {
			if (sbuf->des_len_2nd == 0
			    || (sbuf->des_cnt_2nd == 1
				&& offset == (u_int32_t)sbuf->len))
				break;

			assert(des =
			       info->get_transport_loop_des_by_num(cnt));

			assert(des->
			       descriptor_serialize((u_char
						     *) ((u_int32_t)
							 section +
							 offset)) == 0);

			des_len = des->get_descriptor_total_len();

			offset += des_len;
			*count += des_len;

			delete des;

			if (++cnt ==
			    info->transport_descriptor_circleq->
			    descriptor_count) {
				cnt = 0;
				break;
			}
		}

		info = CIRCLEQ_NEXT(info, entries);
	} while (info != CIRCLEQ_NEXT(sbuf->ts_end, entries));
	*len_field += sbuf->des_len_2nd;
	*len_field &= DESCRIPTORS_LENGTH_FIELD_MASK;

	assert(offset == (u_int32_t)sbuf->len - CRC32_SIZE);
	CIRCLEQ_REMOVE(&_sec_buf_head, sbuf, list);
	delete (sbuf);

	// Compute and fill the CRC32 value.
	fill_crc(section);

	return section;
}				// End of nit_section::nit_table_to_section().


bool Nit_section_to_table_handler::_is_complete()
{
	for (int i = 0; i <= _last_section_number; i++) {
		if (!(_received[i]))
			return false;
	}
	return true;
}				// End of nit_section_to_table_handler::_is_complete(). 



/*
 * Function     init should be called each time
 * * when we need to receive sections for a new table.
 */


int Nit_section_to_table_handler::init(void *section)
{
	Nit_section_draft *ptr_draft;

	// Fetch basis information within table.
	ptr_draft = (Nit_section_draft *) section;

	_network_id = ptr_draft->get_network_id();
	_version_number = ptr_draft->get_version_number();
	_current_next_indicator = ptr_draft->get_current_next_indicator();
	_last_section_number = ptr_draft->get_last_section_number();

	/*
	 * memory allocate for received map
	 */
	_received = new bool[_last_section_number + 1];
	memset(_received, 0, sizeof (bool) * (_last_section_number + 1));

	/*
	 * Create nit table instance
	 */
	_nit_table = new Nit(_network_id,
			     _version_number, _current_next_indicator);

	return (0);
}				// End of nit_section_to_table_handler::init()


/*
 * Function nit_section_to_table_handler do the following:
 * 1.Fetch informatin from the input section.
 * 2.Add the information into the imcomplete table.
 * 3.Determine if the table is completed.
 * 4.If the table is completed, return the pointer to this table.
 *     Else return null.
 */
Nit *Nit_section_to_table_handler::to_table(void *nit_section)
{
	Nit_section_draft *ptr_draft;
	u_char section_number;
	u_int32_t offset;
	int total_len, i;
	u_int16_t len, ts_len;
	Descriptor *des;

	/*
	 * if nit table insance do not exist, then create it
	 */
	if (_nit_table == NULL)
		init(nit_section);


	//Step1.Fetch informatin from the input section.
	ptr_draft = (Nit_section_draft *) nit_section;

	_current_next_indicator = ptr_draft->get_current_next_indicator();
	section_number = ptr_draft->get_section_number();
	total_len =
		ptr_draft->get_section_length() - CRC32_SIZE +
		SECTION_DRAFT_SIZE;

	//Step2.Add the information into the imcomplete table.
	/*
	 * If the current_next_indicator in the section is '1',
	 * which means that the table is currently appliable,
	 * we turn on the table's flag.
	 */

	if (_current_next_indicator == 1)
		_nit_table->set_current_next_indicator(1);

	/*
	 * get network_descriptors_length
	 */
	offset = NIT_SECTION_DRAFT_SIZE;
	memcpy(&len, (void *)((u_int32_t) nit_section + offset),
	       DESCRIPTORS_LENGTH_FIELD_LEN);
	offset += DESCRIPTORS_LENGTH_FIELD_LEN;

	/*
	 * create network descriptor
	 */
	while (len > 0) {
		/*
		 * deserialize descriptor and add into table
		 */
		assert(des = Descriptor::
		       descriptor_deserialize((u_char *) ((u_int32_t)
							  nit_section +
							  offset), &i));
		_nit_table->add_network_loop_des(des);
		offset += i;
		len -= i;
		delete (Linkage_descriptor*)des;
	}
	assert(len == 0);

	/*
	 * checkpointer, if cut pointer in network descriptor loop or after
	 * network descriptor and before transport stream info, then must check
	 * section whether goto end
	 */
	if (offset == (u_int32_t)total_len) {
		_received[section_number] = true;
		if (_is_complete())
			return _nit_table;
		else
			return NULL;
	}

	/*
	 * get transport_stream_loop_length
	 */
	memcpy(&ts_len, (void *)((u_int32_t) nit_section + offset),
	       DESCRIPTORS_LENGTH_FIELD_LEN);
	offset += DESCRIPTORS_LENGTH_FIELD_LEN;

	/*
	 * create transport stream info
	 */
	while (ts_len > 0) {
		/*
		 * create transport stream info
		 */
		Nit_transport_stream_info *info =
			(Nit_transport_stream_info *) ((u_int32_t)
						       nit_section +
						       offset);

		/*
		 * add transport stream info into table, if this info exist in
		 * table then it will abort anything
		 */
		_nit_table->add_transport_info(info);
		ts_len -= NIT_TRANSPORT_STREAM_INFO_SIZE;
		offset += NIT_TRANSPORT_STREAM_INFO_SIZE;

		/*
		 * get transport_descriptors_length
		 */
		memcpy(&len, (void *)((u_int32_t) nit_section + offset),
		       DESCRIPTORS_LENGTH_FIELD_LEN);
		ts_len -= DESCRIPTORS_LENGTH_FIELD_LEN;
		offset += DESCRIPTORS_LENGTH_FIELD_LEN;

		/*
		 * get transport stream descriptor
		 */
		while (len > 0) {
			assert(des = Descriptor::
			       descriptor_deserialize((u_char
						       *) ((u_int32_t)
							   nit_section +
							   offset), &i));

			/*
			 * add transport stream descriptor into table, if this
			 * info which the descriptor belong to don't exist in
			 * table then it will abort anything
			 */
			assert(_nit_table->add_transport_loop_des(info->
								  get_transport_stream_id
								  (),
								  des) ==
			       0);

			offset += i;
			len -= i;
			ts_len -= i;
			delete des;
		}
	}
	assert(ts_len == 0 && offset == (u_int32_t)total_len - CRC32_SIZE);
	//Step3.Determine if the table is completed.
	_received[section_number] = true;

	//Step4.If the table is completed, return the pointer to this table.
	//Else return null.
	if (_is_complete())
		return _nit_table;
	else
		return NULL;
}
