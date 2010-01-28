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

#ifndef __NCTUNS_DS_RuleMgr_h
#define __NCTUNS_DS_RuleMgr_h


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class Rule;
class RuleMgr;
class Tokenbucket;

struct RuleLink{
	Rule* 			rule;
	Tokenbucket*	tb;
	RuleLink* 		nextLink;
};

class Rule{
  public:
	Rule(char* sip,char* dip,char* sport,char* dport,char* ptc,char* phb);
	~Rule();
	bool 			match(ePacket_* pkt);
	bool 			match(unsigned sip,unsigned dip, short srcPort, short dstPort, short ptc);
	unsigned char	getCodepoint();
	void			show();
	
  private:
	u_long			transformIP(const char* ipstr,unsigned& mask);	// transform string ip address into u_long
	unsigned char	phbToDSCP(const char* phb);	//transform phb to codepoint
	unsigned char	_codepoint;
	char 			_sip[16];
	char			_dip[16];
	char			_srcPort[8];	// source port
	char 			_dstPort[8];	// destination port
	char			_phb[8];
	char			_ptc[4];

	/* debug & show message */
	int 			debug;
	void 			showMsg(int msgType, const char* funcName, const char* message);
	static const char* className;
};

class RuleMgr{
  private:
	RuleLink*		rules_head;
	RuleLink*		rules_tail;
	int				_count;
	
	/* debug & show message */
	int 			debug;
	void 			showMsg(int msgType,char* funcName,char* message);
	static const char* className;

  public:
	RuleMgr();
	~RuleMgr();
	void 			addRule(char* sip,char* dip,char* sport,char* dport,char* ptc, char* phb,Tokenbucket* tb);
	Rule*			getRule(unsigned sip,unsigned dip, short srcPort, short dstPort, short ptc);
	Rule*			getRule(ePacket_* pkt);
	Tokenbucket*	getTokenbucket(ePacket_* pkt);
	int				getRuleNumber();
	void			giveToken();
	void		 	show();
};


#endif // __NCTUNS_DS_RuleMgr_h

