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

#ifndef __COOR_GLOBAL
#define __COOR_GLOBAL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#include "ClientNode.H"
#include "Socket.H"
#include "ServerSocket.H"
#include "DatagramSocket.H"
#include "UnixSocket.H"
#include "SocketManager.H"
#include "Mediator.H"
#include "IOStream.H"

/* Path to domain socket for simulation Engine*/
#define domainSocketPath "/tmp/nctuns"

/* Initially try to bind these two ports for clients*/
#define commandPort	9830
#define dataPort	9840
#define tcshPort	9880

/* Default buffer Size of a message*/
#define BUFFER_SIZE		1024

#endif
