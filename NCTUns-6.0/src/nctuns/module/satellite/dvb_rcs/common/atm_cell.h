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

#ifndef __NCTUNS_atm_cell_h__
#define __NCTUNS_atm_cell_h__

#define AAL5_TRAILER_LEN	8		/* AAL5 Trailer length */
#define ATM_CELL_HEADER_LEN	5		/* ATM Cell Header length */
#define ATM_CELL_LEN		53		/* ATM Cell length */
#define ATM_PAYLOAD_LEN		48		/* ATM Payload length */
#define ATM_HEX_CRC8_POLY	0x07		/* ATM HEC CRC-8 Polynomial */
#define IEEE_8023_CRC32_POLY	0x04C11DB7	/* 802.3 CRC-32 Polynomial */

/*       AAL5 CPCS-PDU Format
 * +-------------------------------+
 * |             .                 |
 * |             .                 |
 * |        CPCS-PDU Payload       |
 * |     up to 2^16 - 1 octets)    |
 * |             .                 |
 * |             .                 |
 * +-------------------------------+
 * |      PAD ( 0 - 47 octets)     |
 * +-------------------------------+ -------
 * |       CPCS-UU (1 octet )      |
 * +-------------------------------+
 * |         CPI (1 octet )        |
 * +-------------------------------+CPCS-PDU Trailer
 * |        Length (2 octets)      |
 * +-------------------------------|
 * |         CRC (4 octets)        |
 * +-------------------------------+ -------
 */

/*
 * AAL5 CPCS-PDU trailer struct
 */
struct aal5_trailer {
	u_int8_t	cpcs_uu;	/* not be used by atm */
	u_int8_t	cpi;		/* be 0x00 */
	u_int16_t	payload_len;	/* payload length */
	u_int32_t	crc32;		/* CRC-32 for CPCS-PDU */
};

/*
 * AAL5 CPCS-PDU struct
 */
struct aal5_pdu {
	void			*data;		/* payload max 65535 octets */
	u_int8_t		padding_len;	/* padding length */
	struct aal5_trailer	*trailer;	/* trailer pointer */
};

/*       ATM Cell Format
 * +---------------+---------------+
 * |   GFC(4bits)  |  VPI(1 octest)|
 * +---------------+---------------+
 * |     VPI       |  VCI(2 octets)|
 * +---------------+---------------+
 * |              VCI              |
 * +---------------+----------+----+
 * |     VCI       | PT(3bit) |CLP | CLP(1bit)
 * +---------------+----------+----+
 * |           HEC(1 octets)       |
 * +---------------+---------------+
 * |               .               |
 * |            Payload            |
 * |           48 octets           |
 * |               .               |
 * +-------------------------------+
 */

/*
 * ATM cell struct
 */
struct atm_cell {
	/*
	 * ATM header
	 * gfc	header[0]:0-3
	 * vpi	header[1]:4-header[2]:3
	 * vci	header[2]:4-header[4]:3
	 * pt	header[3]:4-6
	 * clp	header[3]:7
	 */
	u_int8_t	header[4];	/* ATM header exclude CRC */
	u_int8_t	hec;		/* CRC */
	char		payload[ATM_PAYLOAD_LEN];	/* data payload */
	struct atm_cell	*next;		/* linked list next pointer */
};

#endif /* __NCTUNS_atm_cell_h__ */
