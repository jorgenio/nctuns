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

#ifndef __NCTUNS_mac802_16_MeshRoute_h__
#define __NCTUNS_mac802_16_MeshRoute_h__


#include <object.h>
#include "../service_flow.h"

class mac802_16_MeshSS;

typedef class TcpCongestionWindowEstimator {

private:

	uint32_t tcp_cur_send_seq;
	uint32_t tcp_cur_recv_seq;

public:

	TcpCongestionWindowEstimator();
	int update_send_seq(u_int32_t new_seqno);
	int update_recv_seq(u_int32_t new_seqno);
	int print_estimated_cwnd_size();

} tcp_cwnd_estimator;

class MeshRoute: public NslObject {

public:
    typedef enum RoutingSchemeType {

        rt_scheme_undefined     = 0,
        rt_scheme_static_route  = 1,
        rt_scheme_dynamic_route = 2
        
    } rt_scheme_t;

private:

    bool                    path_setup_flag;

    uint32_t*               local_ip;
    uint32_t*               netmask;
    
    ClassifierRuleTable     CrTable;
    char*                   meshRtFilename;

    /* Function Declaration */
    int                     cwnd_observe_flag;
    tcp_cwnd_estimator      tcp_cwnd_estimator_ie;

public:
    explicit MeshRoute(uint32_t, uint32_t, plist*, const char*);
    virtual ~MeshRoute();

    int init();
    int recv(Event *);
    int send(Event *);

    int         change_route(in_addr_t dest_ip, u_int32_t nexthop_nid);

    uint32_t    query_route(in_addr_t);
    
    int         showip(ulong ip);

private:
    void readConfig();
};


#endif				/* __NCTUNS_mac802_16_MeshRoute_h__ */
