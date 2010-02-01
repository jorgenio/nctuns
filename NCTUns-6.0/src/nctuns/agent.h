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

#ifndef __NCTUNS_AGENT_H__
#define __NCTUNS_AGENT_H__

#include <sys/types.h>
#include <vector>

#define DATA(x) x,
enum EnumGroupType {
        #include "group_type_define.h"
};

class CAgent {
public:
	CAgent();
	CAgent(unsigned long nid)	{
		m_id = nid;
		m_control = 0;		// by default, it is not controlled by a human
        }
	const unsigned long		GetID() const 		{ return m_id; }
	const int			GetControl()		{ return m_control; }
	void				SetID(unsigned long id) 	{ m_id = id; }
	void				SetControl(u_char _control)  	{ m_control = _control;}
protected:
	unsigned long			m_id;
	u_char				m_control;	//controlled by a program or a human
};

typedef std::vector<CAgent *> GROUPMEMBER_VECTOR;
typedef GROUPMEMBER_VECTOR::iterator  GruopMemItor;

class GroupAgent {
private :
	EnumGroupType			m_eGroupType;	
	GROUPMEMBER_VECTOR		m_vGroupMember;
public:
	GroupAgent();
	~GroupAgent();
	GroupAgent(EnumGroupType type)  {m_eGroupType = type;}
	void				SetGroupType(EnumGroupType type)  {m_eGroupType = type;}
	const int			size() const { return m_vGroupMember.size(); }
	const EnumGroupType		GetGroupType() const { return m_eGroupType; }
	GROUPMEMBER_VECTOR*		GetMember()  { return &m_vGroupMember; }
	void				AddAgent(CAgent* _agent) { m_vGroupMember.push_back(_agent); }
};

typedef std::vector<GroupAgent *> GROUPAGENT_VECTOR;

extern  CAgent *GetAgent(int nid);
extern const u_char getAgentHumanOrProgramControlled(int nid);
extern int setAgentHumanOrProgramControlled(int nid, u_char control);
extern void GROUP_INIT();
extern GroupAgent *GROUP_FIND(EnumGroupType type);

#endif
