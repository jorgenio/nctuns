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

#ifndef __COOR_SOCKETMANAGER
#define	__COOR_SOCKETMANAGER

#include "ClientList.H"

#define max_connection  144

#define conn_disp_idx 	0
#define serv_se_idx		1
#define serv_cmd_idx	2
#define serv_data_idx	3
#define serv_tcsh_idx	4
#define conn_se_idx		5
#define conn_tcsh_idx	6
#define cli_cmd_offset 	7
#define cli_data_offset 70

class Mediator;

class SocketManager {

	public:

		SocketManager(){};
		SocketManager(int dispatcher_fd);

		void 	buildSelectList();
		void 	Select();
		void 	closeByIndex(int index);
		void 	closeByFD(int fd);

		int  	getConnectedFD(int index);
		int	 	getToDispatcherFD();
		int  	find_data_fd(char* ip, int data_port);

		int  	getUnixSocketFD();
		void 	regist(Mediator* mediator);
		string 	getCoorInfo();

		char* 	getLocalIP(int fd);
		char* 	getPeerIP(int fd);
		int   	getLocalPort(int fd);
		int	  	getPeerPort(int fd);
		string 	toString();

	protected:

		int  	setupNetwork();
		void 	readSocks();
		void 	handleNewCommand(int fd);
		void 	handleNewData(int fd);

	private:

		Mediator*	mediator;

		int 	command_port;
		int 	data_port;
		int 	tcsh_port;

		int 	connect_list[max_connection];
		fd_set 	socks;
		int 	highsock;
};

#endif
