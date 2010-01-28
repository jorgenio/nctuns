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

#ifndef __NCTUNS_rcs_atm_h__
#define __NCTUNS_rcs_atm_h__

#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/atm_cell.h>

/*
 * setting/unsetting flag for variable x
 */
#ifndef SET_FLAGS
#define SET_FLAGS(x, flag)	(x |= flag);
#endif

#ifndef UNSET_FLAGS
#define UNSET_FLAGS(x, flag)	(x &= ~flag);
#endif

/*
 * set value of ATM Header macro
 */
#define ATM_SETGFC(gfc, cell)			\
{						\
	(cell)->header[0] &= 0x0F;		\
	(cell)->header[0] |= (gfc & 0x0F) << 4;	\
}
#define ATM_SETVPI(vpi, cell)			\
{						\
	(cell)->header[0] &= 0xF0;		\
	(cell)->header[1] &= 0x0F;		\
	(cell)->header[0] |= (vpi >> 4) & 0x0F;	\
	(cell)->header[1] |= (vpi << 4) & 0xF0;	\
}
#define ATM_SETVCI(vci, cell)			\
{						\
	(cell)->header[1] &= 0xF0;		\
	(cell)->header[3] &= 0x0F;		\
	(cell)->header[1] |= (vci >> 12) & 0x0F;\
	(cell)->header[2] = vci >> 8;		\
	(cell)->header[3] |= (vci << 4) & 0xF0;	\
}
/* bit1 and bit3 must be zero */
#define ATM_SETPT(flag, cell)		\
{						\
	(cell)->header[3] &= 0xF0;		\
	(cell)->header[3] |= (flag) & 0x0A;	\
}
#define ATM_PT_NONE		0x00
#define ATM_PT_LAST_CELL	0x08
#define ATM_PT_DULM		0x02

/*
 * get value of ATM Header macro
 */
#define ATM_GETGFC(gfc, cell)	gfc = (cell)->header[0]>>4;
#define ATM_GETVPI(vpi, cell)			\
{						\
	vpi = (cell)->header[0]<<4;		\
	vpi |= (cell)->header[1]>>4;		\
}
#define ATM_GETVCI(vci, cell)			\
{						\
	u_int16_t	tmp;			\
						\
	vci = (cell)->header[1];		\
	vci = vci << 12;			\
	tmp = (cell)->header[2];		\
	vci |= tmp << 8;			\
	vci |= (cell)->header[3] >> 4;		\
}
/* bit1 and bit3 must be zero */
#define ATM_GETPT(flag, cell)	flag = (cell)->header[3] & 0x0A;


class NslObject;

class Rcs_atm : public NslObject {

protected:

	struct rcs_buffer {
		CIRCLEQ_ENTRY(rcs_buffer) list;

		uint8_t vpi;
		uint16_t vci;

		/* buffer for atm cell linked list */
		struct atm_cell *atm_buf_head;
		struct atm_cell *atm_buf_tail;
		int cnt;
	};

	CIRCLEQ_HEAD(circleq_head, rcs_buffer) _rx_rbuf_head;

	u_int8_t	_vpi;
	u_int16_t	_vci;

protected:
	struct rcs_buffer *find_buffer(uint8_t vpi, uint16_t vci);

	/* encapsulation packet for sending flow */
	struct aal5_pdu	*_encapsulation_aal5(Dvb_pkt *pkt);
	struct atm_cell	*_segmentation_atm(struct aal5_pdu *aal5_pkt);

	/* re-assembly packet for recving flow */
	struct aal5_pdu	*_re_assembly_atm(struct rcs_buffer *rbuf);
	int		_de_encapsulation_aal5(Dvb_pkt *pkt, struct aal5_pdu *aal5_pkt);
	void		_free_atm_buf(struct atm_cell *atm_head);

public:
	/*
	 * Constructor and Destructor.
	 */
 	Rcs_atm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	~Rcs_atm();   

	/*
	 * Public functions.
	 */
	int	init();
	int	send(ePacket_ *pkt);
	int	recv(ePacket_ *pkt);
}; 
  

#endif	/* __NCTUNS_rcs_atm_h__ */
