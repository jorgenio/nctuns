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

#ifndef __NCTUNS_WIMAX_NODE_DEG_DESCRIPTOR_H__
#define __NCTUNS_WIMAX_NODE_DEG_DESCRIPTOR_H__


#include <cstdio>
#include <inttypes.h>


class Node_deg_descriptor {

private:
    typedef enum neighbor_des_file_format {

        BLOCK_FORMAT = 1,
        LINE_FORMAT = 2,

    } neighbor_des_file_format_t;

    typedef enum parse_neighbor_des_file_state {

        NONE        = 0,
        NID_LINE    = 1,

        LOC_X       = 11,
        LOC_Y       = 12,
        LOC_Z       = 13,

        NUM_ONE_HOP_NEIGHBOR = 20,
        NUM_TWO_HOP_NEIGHBOR = 21


    } parse_ndf_state_t;

private:
    char*       _file_name;
    uint32_t    _nf_1_hop_neighbors;
    uint32_t    _nf_2_hop_neighbors;

public:
	explicit Node_deg_descriptor();
	virtual ~Node_deg_descriptor();

    void init(uint32_t, char*);
    void dump() const;

    inline char** bind_ptr() { return &_file_name; }
    inline uint32_t nf_1_hop_neighbors()
    { return _nf_1_hop_neighbors; }
    inline uint32_t nf_2_hop_neighbors()
    { return _nf_2_hop_neighbors; }

private:
    void _parse_node_degree(FILE*, uint16_t);
};


#endif /* __NCTUNS_WIMAX_NODE_DEG_DESCRIPTOR_H__ */
