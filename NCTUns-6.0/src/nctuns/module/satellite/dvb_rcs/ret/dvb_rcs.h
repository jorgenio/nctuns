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

#ifndef __NCTUNS_dvb_rcs_h__
#define __NCTUNS_dvb_rcs_h__

#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/errormodel/saterrormodel.h>
#include <misc/log/logmacro.h>
#include <misc/log/logHeap.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/rcst/rcst_ctl.h>

#define IBM_CRC16_POLY		0x8005	/* IBM CRC-16 Polynomial */
#define RANDOMIZER_SEED		0x9500	/* Randomizer seed */
#define RS_PARITY_LENGTH	16	/* RS parity check length */
#define CC_X_GEN_POLY		0171	/* CC X Generator Polynomial */
#define CC_Y_GEN_POLY		0133	/* CC Y Generator Polynomial */
#define CC_GEN_LEN		7	/* CC Generator Polynomial Length */

#define SPEED_OF_LIGHT		300000000.0 // for propagation delay

class ConvoCode;
class ReedSolomonCode;

class Dvb_rcs : public NslObject {

protected:
	
	/*
	 * Convolutional encode/decode table array
	 */
	static struct cc_puncture {
		double				rate;
		int				PX;
		int				PY;
		int				PLen;
		union {
			ConvoCode			*CC;
		} coding;
	} _cc_punctured_table[];

	/*
	 * _node_type will get from control module
	 */
	dvb_node_type	_node_type;

	/*
	 * location
	 */
	double		_loc_x;
	double		_loc_y;
	double		_loc_z;

	/*
	 * upper layer module name and pointer
	 */
	int		_sp_nid;
	int		_ncc_nid;
	char		*_sp_name;
	char		*_ncc_name;

	/*
	 * ReedSolomonCode object
	 */
	ReedSolomonCode	*_rs;

	/*
	 * bir error rate
	 */
	double		_bit_err_rate;
	struct link_budget	_budget;
	struct link_info	_info;

	/*
	 * for log
	 */
	u_char		_ptrlog;

	/*
	 * sat link object
	 */
	NslObject	*_link_obj;

	/*
	 * for turn on/off link fail flag
	 */
	uint32_t	LinkFailFlag;
	char            *_linkfail;
	char            *linkfailFileName;
	FILE            *linkfailFile;
	int		*tunfd_;
        FILE            *freqFile;

protected:
	ePacket_	*recv_handler(ePacket_ *pkt);
	void		_encode_atm_burst(Dvb_pkt *pkt);
	void		_decode_atm_burst(Dvb_pkt *pkt);
	int		_randomization(char *input, char *output, int inputLen);
	void		_create_cc_table(inner_coding_puncture type);
	u_int64_t	_prop_delay_time(NslObject *obj);
	u_int64_t       _tx_delay(int len, int bw);
	int             _parse_nodeid_cfg(char *path, list<dvbrcs_node_id> &node_id);

public:
	/*
	 * Constructor and Destructor.
	 */
 	Dvb_rcs(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	virtual ~Dvb_rcs();   

	/*
	 * Public functions.
	 */
	virtual int	init();
	virtual int	send(ePacket_ *pkt);
	virtual int	recv(ePacket_ *pkt);
}; 
  

#endif	/* __NCTUNS_dvb_rcs_h__ */
