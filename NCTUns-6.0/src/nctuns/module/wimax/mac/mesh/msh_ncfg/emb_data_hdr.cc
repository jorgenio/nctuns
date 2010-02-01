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

#include "emb_data_hdr.h"

#include <cassert>
#include <string.h>
#include "emb_ie_network_des.h"
#include "emb_ie_net_entry_open.h"
#include "emb_ie_net_entry_ack.h"
#include "emb_ie_link_est.h"


using namespace Msh_ncfg;


Embedded_data_hdr::Embedded_data_hdr(Embedded_data_hdr* hdr)
: _flag(1)
, _ncfg(NULL)
{
    _fields = new hdr_fields_t;
    *_fields = *hdr->_fields;
}

Embedded_data_hdr::Embedded_data_hdr(ie_type_t pType)
: _flag(1)
, _ncfg(NULL)
{
    _fields = new hdr_fields_t;
    memset(_fields, 0, sizeof(hdr_fields_t));
    _fields->Type = pType;
}

Embedded_data_hdr::Embedded_data_hdr(void* pBuffer)
: _flag(0)
, _ncfg(NULL)
{
    _fields = (hdr_fields_t*)pBuffer;
}

Embedded_data_hdr::~Embedded_data_hdr()
{
    if (_flag)
        delete _fields;
}

Embedded_data_hdr*
Embedded_data_hdr::create_emb_ie(void* buf, int len, link_est_ie_format_t fmt)
{
    hdr_fields_t* hdr = (hdr_fields_t*)buf;

    switch (hdr->Type) {

        case TYPE_NETWORK_DESCRIPTOR:
            return new Network_Des_ie(buf, len);

        case TYPE_NETWORK_ENTRY_OPEN:
            return new Net_entry_open_ie(buf, len);

        case TYPE_NETWORK_ENTRY_REJECT:
            /*
             * FIXME: implement me.
             */
            return NULL;

        case TYPE_NETWORK_ENTRY_ACK:
            return new Net_entry_ack_ie(buf, len);

        case TYPE_NEIGHBOR_LINK_EST:
            return new Link_est_ie(buf, len, fmt);

        default:
            return NULL;
    }
}
