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

#include "network_des.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <string.h>

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


/*
 * Member function definitions of class `Network_descriptor'.
 */

/*
 * Constructor
 */
Network_descriptor::Network_descriptor()
: _file_name(NULL)
, _id(NULL)
{
    _bs_ip_list.clear();
}


/*
 * Destructor
 */
Network_descriptor::~Network_descriptor()
{
    if (_id)
        delete _id;
}

const char*
Network_descriptor::id() const
{
    return _id->c_str();
}

bool
Network_descriptor::init(char* config_file_dir)
{
    assert(_file_name);

    std::string long_file_name(config_file_dir);
    long_file_name += _file_name;

    FILE* file;
    if (!(file = fopen(long_file_name.c_str(), "r"))) {

        FATAL("%s: Could not open Network Description File (%s)\n",
                __func__, long_file_name.c_str());
    }
    _parse_network_des_block(file);

    fclose(file);

    return _id;
}

void
Network_descriptor::_parse_network_des_block(FILE* file)
{
    ASSERT(file, "file is null.\n");

    const int max_line_len = 80;
    char readbuf[max_line_len];
    memset(readbuf, 0, max_line_len);

    int max_token_num = 100;
    char* token_p_pool[max_token_num];

    char* read_ptr = NULL;
    char* token_p = NULL;


    int found_netdes_blk_flag = false;
    while (1) {

        if (feof(file))
            break;

        read_ptr = fgets(readbuf, max_line_len, file);

        if (!read_ptr)
            break;

        int token_cnt=0;
        while (1) {

            token_p = strsep(&read_ptr, " \t\n=");

            if (!token_p)
                break;

            if (*token_p == '\0')
                continue;

            token_p_pool[token_cnt] = token_p;
            DEBUG("%s: token_p_pool[%d]=%s\n", __func__, token_cnt, token_p);

            ++token_cnt;
        }

        /* parse the keyword */

        if (!strcmp("NetworkDesBlock", token_p_pool[0]) ) {

            ASSERT(!_id, "%s: Multiple networks structure is not supported yet.\n",
                        __func__);

            found_netdes_blk_flag = true;
            continue;
        }

        if (!strcmp("EndNetworkDesBlock", token_p_pool[0]) ) {

            found_netdes_blk_flag = false;
            continue;

        }

        if (found_netdes_blk_flag) {

            if (!strcmp("Network_ID", token_p_pool[0]) ) {

                assert(!_id);

                _id = new std::string(token_p_pool[1]);

            }
            else if (!strncmp("BS",token_p_pool[0],2)) {

                size_t str_len = strlen(token_p_pool[0]);

                ASSERT(str_len >= 5, "keyword is too short.\n");

                //u_int32_t index = static_cast<u_int32_t> (token_p_pool[0][2]);

                /* parse sub-command */
                if (!strncmp("IP", &(token_p_pool[0][4]),2)) {

                    in_addr addr;
                    assert(inet_aton(token_p_pool[1], &addr));
                    _bs_ip_list.push_back(addr);
                    DEBUG("%s: addr = %s\n", __func__, inet_ntoa(addr));
                }
                else {

                    ASSERT(0, "unknown sub-command (%s).\n", token_p_pool[0]);
                }


            }
            else if (!strncmp("SSList",token_p_pool[0],6)) {

                ;

            }
            else if (!strncmp("Authentication",token_p_pool[0],14)) {

                ;

            }
            else {
                //ASSERT(0, "unknown command (%s).\n", token_p_pool[0]);
                continue;
            }
        }
        else {

            ASSERT(0, "command is out of a control block(%s).\n", token_p_pool[0]);

        }
    }

    if (!_id)
        FATAL("%s: fail to create network descriptor block.\n", __func__);
}

const in_addr*
Network_descriptor::get_bs_ip() const
{
    if (_bs_ip_list.empty())
        return NULL;
    /*
     * so far, we just take the first one BS IP.
     */
    return &_bs_ip_list.front();
}

void
Network_descriptor::dump() const
{
    STAT("Network_descriptor::%s:\n", __func__);

    STAT("\tNetwork ID: %s\n", _id?_id->c_str():"NULL");

    for (std::list<in_addr>::const_iterator it = _bs_ip_list.begin();
            it != _bs_ip_list.end(); it++)

        STAT("\t\tBS IP: %s\n", inet_ntoa(*it));


    /* order test */
#if 0
    in_addr_t net_order_addr = inet_addr("1.2.3.4");
    in_addr_t host_order_addr = inet_network("1.2.3.4");

    char* ptr1=(char*)&net_order_addr;
    char* ptr2=(char*)&host_order_addr;
    printf("network order: ");
    for (int i=0; i<4 ;++i) {

        printf("%d ", (int)(*ptr1));
        ptr1++;

    }

    printf("\nhost order: ");
    for (int i=0; i<4 ;++i) {

        printf("%d ", (int)(*ptr2));
        ptr2++;

    }
    printf("\n");
#endif
}
