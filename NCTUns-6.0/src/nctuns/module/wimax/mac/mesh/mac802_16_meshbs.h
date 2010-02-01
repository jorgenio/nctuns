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

#ifndef __NCTUNS_mac802_16_MeshBS_h__
#define __NCTUNS_mac802_16_MeshBS_h__

#include <list>
#include "mac802_16_meshss.h"


class mac802_16_MeshBS: public mac802_16_MeshSS {

private:
	class MSHNode;

private:
	std::list<MSHNode*>	nodeList;

	/*
	 * Variables for tcl binding.
	 */
	char*			_NodeCfgFile;
	int			_nf_tx_opp;
	int			_nf_dsch_tx_opp;
	int			_csch_data_fraction;
	int			_nf_sch_frame_idx;

public:
	explicit mac802_16_MeshBS(uint32_t, uint32_t, plist*, const char*);
	virtual ~mac802_16_MeshBS();

	int init();
	int recv(Event*);

private:
	void readConfig();
	int procBaseStation(mgmt_msg*, int, int, in_addr_t);
	ifmgmt* procREGREQ(mgmt_msg*, int, int);
};


class mac802_16_MeshBS::MSHNode {

private:
	bool	    _enable;
	u_char	    _MacAddr[6];
	uint16_t    _NodeID;

public:
	explicit MSHNode(u_char pMacAddr[6], u_int16_t pNodeID)
	{
		memcpy(_MacAddr, pMacAddr, 6);
		_NodeID = pNodeID;
		_enable = false;
	}
	virtual ~MSHNode() {};

	inline void setEnable(bool st) { _enable = st; }
	inline bool Equal(u_char pMacAddr[6]) const
	{
		return memcmp(_MacAddr, pMacAddr, 6) ? false : true;
	}
	inline uint16_t getNodeID() { return _NodeID; } const

	inline void dump() const
	{
		printf("MSHNodeEntry: NodeID = %u, MAC = %02x:%02x:%02x:%02x:%02x:%02x, enable_bit = %d \n",
				_NodeID, _MacAddr[0], _MacAddr[1], _MacAddr[2], _MacAddr[3], _MacAddr[4], _MacAddr[5], _enable);
	}
};



#endif				/* __NCTUNS_mac802_16_MeshBS_h__ */
