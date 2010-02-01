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

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <cassert>
#include <string>
#include "node_deg_des.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


/*
 * Member function definitions of class `Node_deg_descriptor'.
 */

/*
 * Constructor
 */
Node_deg_descriptor::Node_deg_descriptor()
: _file_name(NULL)
, _nf_1_hop_neighbors(0)
, _nf_2_hop_neighbors(0)
{
}


/*
 * Destructor
 */
Node_deg_descriptor::~Node_deg_descriptor()
{
}

void
Node_deg_descriptor::init(uint32_t tcl_node_id, char* config_file_dir)
{
    assert(_file_name);

    std::string long_file_name(config_file_dir);
    long_file_name += _file_name;

    FILE* file;
    if (!(file = fopen(long_file_name.c_str(), "r"))) {

        FATAL("%s: Could not open Node Degree Description File (%s)\n",
                __func__, long_file_name.c_str());
    }

    _parse_node_degree(file, tcl_node_id);

    fclose(file);

    ASSERT(_nf_2_hop_neighbors,
            "%s: No neighboring nodes! The network may be paritioned.\n",
            __func__);
}

void
Node_deg_descriptor::_parse_node_degree(FILE* file, uint16_t tcl_node_id)
{
    /*
     * So far, there are two formats that may be fed into this file.
     * The first one is the block format, described as follows.
     *
     *  nid = 1
     *       loc_x = 252
     *       loc_y = 1575
     *      Number of neighbors = 8
     *              nid = 17
     *              nid = 19
     *              nid = 20
     *               nid = 21
     *               nid = 22
     *               nid = 28
     *               nid = 40
     *               nid = 44
     *       Number of ext_neighbors = 8
     *               nid = 10
     *               nid = 13
     *               nid = 14
     *               nid = 4
     *               nid = 6
     *               nid = 36
     *               nid = 32
     *               nid = 45
     */

    /*
     * The second one is the line format, described as follows:
     *     NID one_hop_degree two_hop_degree start_functioning_time
     */

    neighbor_des_file_format_t  used_format  = BLOCK_FORMAT;
    parse_ndf_state_t           parse_state  = NONE;

    if (!file) {

        FATAL("could not open Neigbor Description File.\n");

    }

    int max_token_num = 100;
    char* token_p_pool[max_token_num];
    const int max_line_len = 80;
    char readbuf[max_line_len];
    memset(readbuf, 0, max_line_len);

    char*       read_ptr    = NULL;
    int         found_flag  = 0;
    u_int32_t   block_nid   = 0;

    int exp_skip_lines = 0;
    int skip_lines = 0;

    while (1) {

        if (feof(file))
            break;

        read_ptr = fgets(readbuf, max_line_len, file);


        if (!read_ptr)
            break;


        char *token_p = NULL;
        int token_cnt = 0;


        if ( used_format == BLOCK_FORMAT ) {


            for (int i=0; i<max_token_num ;++i) {

                token_p_pool[i] = NULL;

            }


            /* parse all the token */
            int skipped_header_cnt = 0;

            while (1) {

                token_p = strsep(&read_ptr, " \t");

                if (!token_p)
                    break;

                if ((*token_p) == '\0') {

                    skipped_header_cnt++;

                    if (skipped_header_cnt>max_token_num)
                        break;
                    else
                        continue;

                }


                token_p_pool[token_cnt] = token_p;
                ++token_cnt;


            }

            if (token_cnt<3) {

                fprintf(stderr, "too few tokens (cnt=%d) in a line. Dump the line as follows: \n", token_cnt);

                for (int i=0 ; i<token_cnt ;++i) {

                    fprintf(stderr, " %s ", token_p_pool[i] );

                }

                FATAL("\n");
            }

            /* parse the first level token */
            if ( !strcmp(token_p_pool[0],"nid")) {

                if ( parse_state == NONE) {

                    /* read NID */
                    block_nid = strtol(token_p_pool[2], 0, 10);

                    if (tcl_node_id == block_nid) {

                        ++found_flag;

                        if (found_flag > 1) {

                            FATAL("Found more than one block with nid = %u\n", block_nid);
                        }

                    }
                }
                else if ( parse_state == NUM_ONE_HOP_NEIGHBOR ) {

                    ++skip_lines;/* ignore this line */
                    if (exp_skip_lines == skip_lines) {

                        parse_state     = NONE;
                        skip_lines      = 0;
                        exp_skip_lines  = 0;
                    }

                }
                else if ( parse_state == NUM_TWO_HOP_NEIGHBOR ) {

                    ++skip_lines;/* ignore this line */
                    if (exp_skip_lines == skip_lines) {

                        parse_state     = NONE;
                        skip_lines      = 0;
                        exp_skip_lines  = 0;
                    }
                }
                else {

                    FATAL("Unrecognized structure. (parse state=%d)\n",
                            parse_state);
                }

            }
            else if ( !strcmp(token_p_pool[0],"loc_x")) {

                ;/* ignore this line */

            }
            else if ( !strcmp(token_p_pool[0],"loc_y")) {

                ;/* ignore this line */

            }
            else if ( !strcmp(token_p_pool[0],"loc_z")) {

                ;/* ignore this line */

            }
            else if ( !strcmp(token_p_pool[0],"Number")) {

                ASSERT(!skip_lines, "skipped line number does not equal to zero.\n" );
                ASSERT(!strcmp(token_p_pool[1],"of") , "miss the \"of\" keyword.\n" );

                if (!strcmp(token_p_pool[2],"neighbors")) {

                    ASSERT(!strcmp(token_p_pool[3],"=") , "miss the \"=\" keyword.\n" );

                    int value = strtol(token_p_pool[4], 0, 10);
                    if ( block_nid == tcl_node_id ) {

                        _nf_1_hop_neighbors = value;
                        DEBUG("The number of one-hop neighbors = %u.\n", _nf_1_hop_neighbors);

                    }

                    parse_state = NUM_ONE_HOP_NEIGHBOR;

                    exp_skip_lines = value;

                    if (_nf_1_hop_neighbors >= 32) {

                        FATAL("The number of one-hop neighbors exceeds 31, which is \
                        the maximum value allowed in the IEEE 802.16-2004 standard. \n");
                    }

                    //NSLOBJ_ASSERT((!exp_skip_lines),
                    //    "A node cannot be isolated. one-hop neighbor being zero is detected.\n");

                    if (exp_skip_lines == 0) {

                        parse_state     = NONE;
                        skip_lines      = 0;
                        exp_skip_lines  = 0;
                    }



                }
                else if (!strcmp(token_p_pool[2],"ext_neighbors")) {

                    ASSERT(!strcmp(token_p_pool[3],"=") , "miss the \"=\" keyword.\n" );

                    int value = strtol(token_p_pool[4], 0, 10);
                    if ( block_nid == tcl_node_id ) {

                        _nf_2_hop_neighbors = value ;

                        /* C.C. Lin:
                         *
                         * file format bug!! temporarily solve this problem by the following statement.
                         */

                        _nf_2_hop_neighbors += _nf_1_hop_neighbors;
                        DEBUG("The number of two-hop neighbors = %u.\n",
                                _nf_2_hop_neighbors);

                    }

                    parse_state = NUM_TWO_HOP_NEIGHBOR;

                    exp_skip_lines = value;

                    //NSLOBJ_ASSERT((!exp_skip_lines),
                    //    "A node cannot be isolated. two-hop neighbor being zero is detected.\n");

                    if (exp_skip_lines == 0) {

                        parse_state     = NONE;
                        skip_lines      = 0;
                        exp_skip_lines  = 0;
                    }


                }
                else {

                    FATAL("detect unrecognized token = %s.\n", token_p);
                }



            }
            else {

                FATAL("detect unrecognized token = %s.\n", token_p);
            }

        }
        else if ( used_format == LINE_FORMAT ) {

            while (1) {

                token_p = strsep(&read_ptr, " \t");

                if (!token_p)
                    break; /* empty line */


                if (!token_cnt) {

                    /* read NID */
                    u_int32_t line_nid = strtol(token_p, 0, 10);

                    if (tcl_node_id != line_nid)
                        break;	/* This is not the line I should read. */

                    if (found_flag > 0) {

                        FATAL("Found more than one line with nid = %u\n", line_nid);
                    }


                    found_flag++;
                }
                else if (token_cnt == 1) {

                    /* The number of one-hop neighbors */
                    _nf_1_hop_neighbors = strtol(token_p, 0, 10);

                    if (_nf_1_hop_neighbors >= 32) {

                        FATAL("The number of one-hop neighbors exceeds 31, which is \
                        the maximum value allowed in the IEEE 802.16-2004 standard. \n");
                    }

                    DEBUG("The number of one-hop neighbors = %u.\n",
                            _nf_1_hop_neighbors);

                }
                else if (token_cnt == 2) {

                    /* The number of two-hop neighbors */
                    _nf_2_hop_neighbors = strtol(token_p, 0, 10);

                    DEBUG("The number of two-hop neighbors = %u.\n",
                            _nf_2_hop_neighbors);

                }
                else {

                    FATAL("detect excessive token = %s.\n", token_p);
                }


                ++token_cnt;
                if (!read_ptr)
                    break;  /* reach the end of line */
            }

        }
        else {

            FATAL("detect undefined mode = %d.\n", used_format);
        }
    }
}

void
Node_deg_descriptor::dump() const
{
    INFO("Node_deg_descriptor::%s: nf_1_hop_neighbors = %u, nf_2_hop_neighbors = %u\n",
            __func__, _nf_1_hop_neighbors, _nf_2_hop_neighbors);
}
