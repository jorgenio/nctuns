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

#ifndef __NCTUNS_DS_DSQueue_h
#define __NCTUNS_DS_DSQueue_h


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <packet.h>
#include <nctuns_api.h>

class DSQueue{
  public:
	
	DSQueue(const char* name,int maxLen,int rate,int nid,int port);
	virtual ~DSQueue();
	
	int 				enqueue(ePacket_* pkt);
	ePacket_*			dequeue();
	ePacket_*			getNext();
	void				addAccount();
	virtual void		show();
	virtual void		showName();
	inline bool			isFull()		{	return (_length == _maxLength);	}
	inline bool			isEmpty()		{	return (_length == 0);	}
	inline int			getLength()		{	return _length;	}
	inline int			getRate()		{	return _rate;	}
	//inline char*		getName()		{	return _name;	}	// should not exist!! , used for debug
	
	virtual void		enableLogLength(char* const filePath);
	virtual void		logLength();
	virtual void		enableLogDrop(char* const filePath);
	virtual void		logDrop();

	unsigned int		_length;

  protected:
	unsigned int		_maxLength;
	unsigned int		_rate;
	char*			_name;
	unsigned int		_account;
	static const unsigned int	_accountUnit = 1500;
	
	ePacket_*			_head;
	ePacket_*			_tail;

	// for log files
	bool				_logLength;
	bool				_logDrop;
	unsigned int		_drops;
	unsigned int		_pass;
	FILE*				logLenFile;
	FILE*				logDropFile;


	/* debug */
	bool				DDDshowIt;	
	int 				_DDDnid;			// DEBUG
	int 				_DDDport;			// DEBUG
	int 				_DDDdebug;

	friend class ds_i;
};


class AFQueue:public DSQueue{
  public:
	AFQueue(char* name,int maxLen,int rate, int ts1, int ts2,int maxDR,int nid,int port);	
	int				enqueue(ePacket_* pkt,int type);
	int				enqueue(ePacket_* pkt);
	void 			show();
	void 			showName();
	
	virtual void	enableLogDrop(char* const d0_filePath,char* const d1_filePath);
	virtual void	logDrop();
	
  private:
	unsigned int	_ts1;
	unsigned int	_ts2;
	unsigned int	_maxDR;		// (%) max drop rate of low drop precedence phb 
	unsigned int	_drop1;
	unsigned int	_pass1;
	unsigned int 	_drop2;
	unsigned int	_pass2;

	FILE*			logDrop1File;
	FILE*			logDrop2File;
};

#endif

