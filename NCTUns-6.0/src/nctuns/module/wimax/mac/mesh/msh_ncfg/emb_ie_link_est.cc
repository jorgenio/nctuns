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

#include "emb_ie_link_est.h"
#include "../mac802_16_meshss.h"
#include "../neighbor_mngr.h"
#include "../net_entry_mngr.h"
#include "../node_deg_des.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../../verbose.h"


using namespace Msh_ncfg;


Link_est_ie::Link_est_ie(action_code_t actionCode, u_int32_t authValue,
	u_char linkID, link_est_ie_format_t fmt)
: Embedded_data_hdr(TYPE_NEIGHBOR_LINK_EST)
, format(fmt)
{
                        
    unsigned int len = sizeof(fields_t);
    
    switch (actionCode) {

    case ACTCODE_CHALLENGE:
        len += 4;
        break;

    case ACTCODE_CHALLENGE_RSP:
        len += 5;
        break;

    case ACTCODE_ACCEPT:
        if ( format == original_std )
            len += 1;
        else if ( format == extended )
            len += 5;
        else {
            printf("unsupported format = %d \n", format);
            assert(0);
        }
        break;

    default:
        printf("Link_est_ie: Unsupported Action Code %d\n", actionCode);
        assert(0);
        break;
    }
    setLength(len);

    _fields = new fields_t;
    _fields->ActionCode = actionCode;
    _fields->reserved   = static_cast<action_code_t>(0);
    AuthValue     = authValue;
    LinkID        = linkID;

}

Link_est_ie::Link_est_ie(void* pBuffer, int pLength, link_est_ie_format_t fmt)
: Embedded_data_hdr(pBuffer)
, format(fmt)
{
    char* buf = (char*)pBuffer;

    _fields = new fields_t;
    *_fields = *reinterpret_cast<fields_t*>(buf + hdr_len());
    int pos = hdr_len() + sizeof(*_fields);

    switch(getActionCode()) {
    case ACTCODE_CHALLENGE:
        AuthValue = *((u_int32_t*)(buf + pos));
//        pos += sizeof(u_int32_t);
        break;

    case ACTCODE_CHALLENGE_RSP:
        AuthValue = *((u_int32_t*)(buf + pos));
        pos += sizeof(u_int32_t);
        LinkID = *((u_char*)(buf + pos));
 //       pos += sizeof(u_char);
        break;

    case ACTCODE_ACCEPT:
        if (format == extended) {
            AuthValue = *((u_int32_t*)(buf + pos));
            pos += sizeof(u_int32_t);
        }
        LinkID = *((u_char*)(buf + pos));
        break;

    case ACTCODE_REJECT:
        /*
         * FIXME: implement it.
         */
        assert(0);
        break;

    default:
        assert(0);
    }
}

Link_est_ie::~Link_est_ie()
{
    delete _fields;
}

int
Link_est_ie::copyTo(u_char* dstbuf)
{
    int pos;

    memcpy(dstbuf, Embedded_data_hdr::_fields, hdr_len());

    pos = hdr_len();

    memcpy(dstbuf + pos, _fields, sizeof(fields_t));
    pos += sizeof(fields_t);

    switch(getActionCode()) {
    case ACTCODE_CHALLENGE:
        memcpy(dstbuf + pos, &AuthValue, sizeof(u_int32_t));
        pos += sizeof(u_int32_t);
        break;

    case ACTCODE_CHALLENGE_RSP:
        memcpy(dstbuf + pos, &AuthValue, sizeof(u_int32_t));
        pos += sizeof(u_int32_t);
        memcpy(dstbuf + pos, &LinkID, sizeof(u_char));
        pos += sizeof(u_char);
        break;

    case ACTCODE_ACCEPT:
        /* Added by C.C. Lin:
         * if the format is extended link establishment IE,
         * the access IE should carry authentication value.
         */
        if (format == extended) {
            memcpy(dstbuf + pos, &AuthValue, sizeof(u_int32_t));
            pos += sizeof(u_int32_t);
        }
        memcpy(dstbuf + pos, &LinkID, sizeof(u_char));
        pos += sizeof(u_char);
        break;

    case ACTCODE_REJECT:
        /*
         * FIXME: implement it.
         */
        assert(0);
        break;

    default:
        assert(0);
    }

    return pos;
}

void
Link_est_ie::proc(mac802_16_MeshSS* mac, uint16_t tx_node_id)
{
    VINFO("%s: tx_node_id = %u\n", __func__, tx_node_id);
    VINFO_FUNC(dump());

    mac->net_entry_mngr()->proc_ncfg_neighbor_link_est(
	    mac->nbr_mngr()->neighbor(tx_node_id), this);
}

void
Link_est_ie::dump()
{
    switch(getActionCode()) {

        case ACTCODE_CHALLENGE:
            printf("\tActionCode = Challenge\n");
            printf("\tAuthValue = %u\n", getAuthValue());
            break;

        case ACTCODE_CHALLENGE_RSP:
            printf("\tActionCode = Challenge response\n");
            printf("\tAuthValue = %u\n", getAuthValue());
            break;

        case ACTCODE_ACCEPT:
            if (format == extended)
                printf("\tAuthValue = %u\n", getAuthValue());

            printf("\tActionCode = accept\n");
            printf("\tLinkID = %u\n", getLinkID());
            break;

        case ACTCODE_REJECT:
            /*
             * FIXME: implement it.
             */
            assert(0);
            break;

        default:
            assert(0);
    };
}
