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

#include <stdlib.h>
#include <assert.h>
#include <regcom.h>
#include <object.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/ret/rcs_atm.h>
#include <satellite/dvb_rcs/common/fec/crc.h>
#include <satellite/dvb_rcs/common/sch_info.h>

/*
 * Constructor
 */
Rcs_atm::Rcs_atm(u_int32_t type, u_int32_t id, struct plist *pl,
		 const char *name)
:NslObject(type, id, pl, name)
{
	CIRCLEQ_INIT(&_rx_rbuf_head);
}


/*
 * Destructor
 */
Rcs_atm::~Rcs_atm()
{
	struct rcs_buffer *rbuf, *last_rbuf;

	rbuf = (rcs_buffer *) _rx_rbuf_head.cqh_first;
	while (rbuf != (rcs_buffer *)&_rx_rbuf_head) {

		_free_atm_buf(rbuf->atm_buf_head);
		last_rbuf = rbuf;
		rbuf = rbuf->list.cqe_next;
		free(last_rbuf);
	}
	CIRCLEQ_INIT(&_rx_rbuf_head);
}


/*
 * module initialize
 */
int Rcs_atm::init()
{
	return (NslObject::init());
}


/*
 * segmentation payload depend on AAL5 spec and padding into ATM cell
 * upper layer must send Dvb_pkt packet struct to me, then send those cell
 * to bottom layer.
 */
int Rcs_atm::send(ePacket_ * event)
{
	Dvb_pkt *pkt;
	struct aal5_pdu *aal5_pkt;
	struct atm_cell *atm_pkt;

	/*
	 * if this node is not RCST, then the packet shoule be by pass
	 */

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	/*
	 * user traffic data 
	 */
	if (pkt->pkt_gettype() == PKT_RAWDATA) {
		if ((aal5_pkt = _encapsulation_aal5(pkt)) == NULL)
			return (-1);
		if ((atm_pkt = _segmentation_atm(aal5_pkt)) == NULL)
			return (-1);

		free(aal5_pkt->data);
		delete aal5_pkt;

		/*
		 * replace Dvb_pkt packet data to atm cell linked list 
		 */
		free(pkt->pkt_detach());
		pkt->pkt_attach((void *)atm_pkt, ATM_CELL_LEN);
		pkt->pkt_settype(PKT_ATM);
	}
	/*
	 * others packet type, drop it and show warning message
	 */
	else if (pkt->pkt_gettype() != PKT_RCSSYNC && pkt->pkt_gettype() != PKT_RCSCSC) {
		printf("[DVB_ATM] Warning: packet type is incorrect, drop it.\n");
		dvb_freePacket(event);
		return (1);
	}

	/*
	 * for control data, bypass RCS_ATM module
	 */
	return NslObject::send(event);
}


/*
 * re-assembly atm cell to Dvb_pkt packet and send to upper layer 
 */
int Rcs_atm::recv(ePacket_ * event)
{
	Dvb_pkt *pkt;
	struct atm_cell *atm_pkt;
	struct aal5_pdu *aal5_pkt;
	u_int8_t flag;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	/*
	 * if this node is RCST, then this packet should be by pass
	 */

	/*
	 * user traffic data
	 */
	if (pkt->pkt_gettype() == PKT_RCSMAC) {
		struct slot_info *ts = &pkt->pkt_getretinfo()->timeslot;
		const uint32_t queue_id = pkt->pkt_getretinfo()->queue_id;
		atm_pkt = (struct atm_cell *)pkt->pkt_detach();
		struct rcs_buffer *rbuf;

		/*
		 * must do a multi queue for each (group_id, logon_id)
		 */
		assert((rbuf = find_buffer(queue_id, ts->logon_id)));

		/*
		 * if atm buffer is empty, then mount it to _atm_buf_head 
		 */
		if (!rbuf->atm_buf_head) {
			rbuf->atm_buf_head = rbuf->atm_buf_tail = atm_pkt;
		}
		else {
			rbuf->atm_buf_tail->next = atm_pkt;
			rbuf->atm_buf_tail = atm_pkt;
		}
		rbuf->cnt++;

		ATM_GETPT(flag, atm_pkt);
		/*
		 * if this atm cell is last cell, then re-assembly it and send to
		 * AAL5 layer
		 */
		if (flag & ATM_PT_LAST_CELL) {
			/*
			 * if any atm cell crc check error, then it will return
			 * NULL
			 */
			if (!(aal5_pkt = _re_assembly_atm(rbuf))) {
				_free_atm_buf(rbuf->atm_buf_head);
				rbuf->atm_buf_tail = rbuf->atm_buf_head = NULL;
				rbuf->cnt = 0;

				dvb_freePacket(event);
				return (1);
			}

			/*
			 * de-encapsulation aal5 pdu and attach it to pkt
			 */
			if (_de_encapsulation_aal5(pkt, aal5_pkt) == -1) {
				delete aal5_pkt;

				dvb_freePacket(event);
				return (1);
			}

			/*
			 * free aal5 packet 
			 */
			delete aal5_pkt;

			pkt->pkt_settype(PKT_RAWDATA);

			return (NslObject::recv(event));
		}
		else {
			dvb_freePacket(event);
			return (1);
		}
	}
	/*
	 * for forward data, by pass RCS_ATM module
	 */
	else {
		return (NslObject::recv(event));
	}
}

struct Rcs_atm::rcs_buffer *Rcs_atm::find_buffer(uint8_t vpi, uint16_t vci) {
	struct rcs_buffer *rbuf;

	/*
	 * find original exist queue
	 */
	CIRCLEQ_FOREACH(rbuf, &_rx_rbuf_head, list) {
		if (vpi == rbuf->vpi && vci == rbuf->vci) {
			return rbuf;
		}
	}

	/*
	 * if cannot find the queue for this vpi, vci, then create it
	 */
	rbuf = new struct rcs_buffer();
	rbuf->vpi = vpi;
	rbuf->vci = vci;
	rbuf->atm_buf_head = rbuf->atm_buf_tail = NULL;
	rbuf->cnt = 0;

	CIRCLEQ_INSERT_TAIL(&_rx_rbuf_head, rbuf, list);

	return rbuf;
}

/*
 * encapsulation payload data depend on AAL5 spec and append AAL5 trailer 
 */
struct aal5_pdu *Rcs_atm::_encapsulation_aal5(Dvb_pkt * pkt)
{
	int len, padding_len;
	struct aal5_pdu *aal5_pkt;
	void *payload;

	/*
	 * must have payload data, or it is wrong behavior 
	 */
	assert(pkt && pkt->pkt_getlen() > 0);

	/*
	 * Compute padding len such that total data length are multiple of 48
	 * octets 
	 */
	len = pkt->pkt_getlen();
	padding_len = (len + AAL5_TRAILER_LEN) % ATM_PAYLOAD_LEN;
	if (padding_len > 0)
		padding_len = ATM_PAYLOAD_LEN - padding_len;

	len += padding_len;

	aal5_pkt = (struct aal5_pdu *)malloc(sizeof (struct aal5_pdu));

	/*
	 * memory allocation new space for payload + padding data +
	 * trailer length
	 */
	payload = malloc(len + AAL5_TRAILER_LEN);
	memcpy(payload, pkt->pkt_getdata(), pkt->pkt_getlen());

	/*
	 * set padding be zero 
	 */
	memset((void *)((u_int32_t) payload + pkt->pkt_getlen()), 0x00,
	       len - pkt->pkt_getlen());

	/*
	 * fill in aal5 trailer field
	 */
	aal5_pkt->data = payload;
	aal5_pkt->padding_len = padding_len;
	aal5_pkt->trailer =
		(struct aal5_trailer *)((u_int32_t) payload + len);
	/*
	 * cpcs_uu and cpi not be used, fill in 0x00 
	 */
	aal5_pkt->trailer->cpcs_uu = 0x00;
	aal5_pkt->trailer->cpi = 0x00;
	/*
	 * length just only payload length, not include padding 
	 */
	aal5_pkt->trailer->payload_len = pkt->pkt_getlen();

	/*
	 * Calculation CRC32 for CPCU-PDU, include payload, padding, and
	 * the first four octets of trailer (exclude CRC field) 
	 */
	aal5_pkt->trailer->crc32 =
		~CRC32(aal5_pkt->data, len + 4, IEEE_8023_CRC32_POLY, 1);

	return aal5_pkt;
}


/*
 * segmentation payload and encapsulation payload data depend on ATM spec
 * every ATM cell contains 48 octets payload and 5 octets ATM header.
 * In this function, it will linked list all ATM cell to raise efficiency
 */
struct atm_cell *Rcs_atm::_segmentation_atm(struct aal5_pdu *aal5_pkt)
{
	struct atm_cell *atm, *cell, *last_cell;
	int len, cell_cnt;

	len = aal5_pkt->trailer->payload_len + aal5_pkt->padding_len +
		AAL5_TRAILER_LEN;
	/*
	 * must have payload data, or it is wrong behavior 
	 */
	assert(aal5_pkt && aal5_pkt->trailer->payload_len > 0
	       && (len % ATM_PAYLOAD_LEN == 0));

	/*
	 * segmentation AAL5 data to ATM cell 
	 */
	cell_cnt = 0;

	/*
	 * total aal5 packet length is payload length + padding length +
	 * aal5 trailer length 
	 */
	len = aal5_pkt->trailer->payload_len + aal5_pkt->padding_len +
		AAL5_TRAILER_LEN;

	atm = cell = last_cell = NULL;
	do {
		cell = new struct atm_cell;

		cell->next = NULL;

		/*
		 * segmentation payload 
		 */
		if (len > ATM_PAYLOAD_LEN) {
			/*
			 * copy payload data 
			 */
			memcpy(cell->payload,
			       (void *)((u_int32_t) aal5_pkt->data +
					cell_cnt * ATM_PAYLOAD_LEN),
			       ATM_PAYLOAD_LEN);
			ATM_SETPT(ATM_PT_NONE, cell);
		}
		else {
			assert(len == ATM_PAYLOAD_LEN);

			/*
			 * copy payload data, last payload must be
			 * ATM_PAYLOAD_LEN - AAL5_TRAILER_LEN
			 * and copy aal5 trailer data
			 */
			memcpy(cell->payload,
			       (void *)((u_int32_t) aal5_pkt->data +
					cell_cnt * ATM_PAYLOAD_LEN),
			       ATM_PAYLOAD_LEN);

			/*
			 * last segmentation, must set PT:bit4 equal '1' 
			 */
			ATM_SETPT(ATM_PT_LAST_CELL, cell);
		}

		ATM_SETGFC(0x00, cell);
		ATM_SETVPI(_vpi, cell);
		ATM_SETVCI(_vci, cell);
		cell->hec = 0x00;
		cell->hec =
			~CRC8(cell, ATM_CELL_HEADER_LEN,
			      ATM_HEX_CRC8_POLY, 1);
		++cell_cnt;
		len -= ATM_PAYLOAD_LEN;

		/*
		 * linked list ATM cell 
		 */
		if (atm == NULL) {
			atm = last_cell = cell;
		}
		else {
			last_cell->next = cell;
			last_cell = cell;
		}
	} while (len > 0);

	/*
	 * check last data length + ATM_TRAILER_LEN must equal ATM_PAYLOAD_LEN 
	 */
	return atm;
}


/*
 * re-assembly ATM cell into AAL5 PDU, this function must use buffer to save
 * unfinished packet 
 */
struct aal5_pdu	*Rcs_atm::_re_assembly_atm(struct rcs_buffer *rbuf)
{
	struct aal5_pdu *aal5_pkt;
	void *payload;
	u_int32_t len, i;
	u_int8_t crc8, hec;

	/*
	 * calculate total AAL5 data length (include payload, padding) 
	 */
	assert((len = rbuf->cnt * ATM_PAYLOAD_LEN) > 0);

	payload = malloc(len);

	/*
	 * re-assembly atm cell to payload data
	 */
	for (i = 0; i * ATM_PAYLOAD_LEN < len && i < (u_int32_t)rbuf->cnt; ++i) {
		/*
		 * check CRC
		 */
		hec = rbuf->atm_buf_head->hec;
		rbuf->atm_buf_head->hec = 0;
		crc8 = CRC8(rbuf->atm_buf_head, ATM_CELL_HEADER_LEN,
			      ATM_HEX_CRC8_POLY, 1);

		/*
		 * The CRC field shall contain the ones complemen of the sum, so we can
		 * do crc again and plus two value, if it equals to 0xFFFFFFFF, then
		 * this packet is correctly, else drop it
		 */
		if ((hec | crc8) != 0xFF)
		{
			return NULL;
		}



		memcpy((void *)((u_int32_t) payload +
				(i * ATM_PAYLOAD_LEN)),
		       rbuf->atm_buf_head->payload, ATM_PAYLOAD_LEN);

		/*
		 * free atm cell buffer 
		 */
		rbuf->atm_buf_tail = rbuf->atm_buf_head;
		rbuf->atm_buf_head = rbuf->atm_buf_head->next;
		free(rbuf->atm_buf_tail);
	}
	/*
	 * initialize atm cell buffer to NULL 
	 */
	assert(i == (u_int32_t)rbuf->cnt);
	rbuf->atm_buf_tail = rbuf->atm_buf_head = NULL;
	rbuf->cnt = 0;
	len -= AAL5_TRAILER_LEN;

	aal5_pkt = new struct aal5_pdu;

	aal5_pkt->data = payload;
	aal5_pkt->trailer =
		(struct aal5_trailer *)((u_int32_t) payload + len);

	aal5_pkt->padding_len = len - aal5_pkt->trailer->payload_len;

	return aal5_pkt;
}


/*
 * de-encapsulation AAL5 PDU into payload data, it will drop AAL5 trailer and
 * padding, and check CRC
 */
int Rcs_atm::_de_encapsulation_aal5(Dvb_pkt * pkt,
				    struct aal5_pdu *aal5_pkt)
{
	u_int32_t crc32;
	int len;

	assert(aal5_pkt->data && aal5_pkt->trailer->payload_len);

	/*
	 * check CRC
	 */
	len = aal5_pkt->trailer->payload_len + aal5_pkt->padding_len;
	crc32 = CRC32(aal5_pkt->data, len + 4, IEEE_8023_CRC32_POLY, 1);

	/*
	 * The CRC field shall contain the ones complemen of the sum, so we can
	 * do crc again and plus two value, if it equals to 0xFFFFFFFF, then
	 * this packet is correctly, else drop it
	 */
	if ((aal5_pkt->trailer->crc32 | crc32) != 0xFFFFFFFF) {
		free(aal5_pkt->data);
		return (-1);
	}

	pkt->pkt_setflag(FLOW_RECV);
	pkt->pkt_attach(aal5_pkt->data, aal5_pkt->trailer->payload_len);
	pkt->pkt_settype(PKT_RAWDATA);

	return (0);
}

/*
 * free all atm cell buffer
 */
void Rcs_atm::_free_atm_buf(struct atm_cell *atm_head)
{
	struct atm_cell *atm_pkt;

	while (atm_head) {
		atm_pkt = atm_head;
		atm_head = atm_head->next;
		free(atm_pkt);
	}
}
