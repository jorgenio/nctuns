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

#ifndef __NCTUNS_DS_Tokenbucket_h
#define __NCTUNS_DS_Tokenbucket_h

#include <packet.h>

//#define REMARK

class TrafCond;

class Tokenbucket{
  public:
	Tokenbucket(float rate, float size, int maxQL,char* phb,TrafCond* owner);
	~Tokenbucket();
	
	bool			input(ePacket_* pkt);
	void			getToken();
	void			show();
	void			show(char* message);
	
  private:		
	TrafCond		*_owner;
	unsigned int	_size;			//	(byte)
	unsigned int	_tokenUnit;		//	(byte)
	unsigned int	_tokens;		//	(byte)
	unsigned int	_in;			//	(packet) , number of in-profile packets in the queue
	unsigned int	_maxIN;			//	(packet)
	unsigned int	_ql;			//	(packet) , all the packets in the queue
	unsigned int	_maxQL;			//	(packet)
	ePacket_		*_head;
	ePacket_		*_tail;

	/* remarking facility */
	bool			_remark;
	char			_ofp_tag[10];	//	out-of-profile tag
	unsigned char	_2ndCodePoint;
	unsigned char	phbTo2ndDSCP(const char* phb);
	void			remark(ePacket_* pkt);

	void			tryOutput();
	int				enqueue(ePacket_* pkt);
	ePacket_*		dequeue();

	
	int _debug;
	
};

#endif	// __NCTUNS_DS_Tokenbucket_h

