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

#ifndef	__NCTUNS_nodetype_h__
#define	__NCTUNS_nodetype_h__

#include <sys/types.h>

#define MAX_NODE_TYPE		100
#define MAX_TYPE_NAME		100

typedef struct tTable {
	char			name_[MAX_TYPE_NAME]; 
	u_char			layer_; 
} tTable; 

 
class typeTable {

 private:
 	static tTable		ttable_[MAX_NODE_TYPE]; 
	static u_char		flag_; 
 	static u_int32_t	idx_;
 	 
 public:
 
 	typeTable();
 	~typeTable();
 	
 	int			Reg_NodeType(const char *name, u_char layer);
 	u_int32_t		toType(const char *name);
 	const char 		*toName(u_int32_t type);
	u_char			NameToLayer(const char *name);
	u_char			TypeToLayer(u_int32_t type);  
	void			display();
};
 

#endif	/* __NCTUNS_nodetype_h__ */
