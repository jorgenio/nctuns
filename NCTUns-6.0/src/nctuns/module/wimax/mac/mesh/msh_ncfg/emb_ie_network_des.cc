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

#include "emb_ie_network_des.h"

#include "../frame_mngr.h"
#include "../mac802_16_meshss.h"
#include "../../../phy/ofdm_mesh.h"


using namespace Msh_ncfg;


Network_Des_ie::Network_Des_ie(const Frame_mngr* fr_mngr)
: Embedded_data_hdr(TYPE_NETWORK_DESCRIPTOR)
{
    _fields = new fields_t;

    _fields->FrameLengthCode        = 4;	// 10 ms
    _fields->MSH_CTRL_LEN           = fr_mngr->nf_tx_opp();
    _fields->MSH_DSCH_NUM           = fr_mngr->nf_dsch_tx_opp();
    _fields->MSH_CSCH_DATA          = fr_mngr->csch_data_fraction();
    _fields->SchedulingFrame        = fr_mngr->nf_sch_frame_idx();
    _fields->Num_Burst_Profile      = 6;
    _fields->msbOperatorID          = 0;
    _fields->lsbOperatorID          = 0;
    _fields->XmtEnergyUnitExp       = 0;
    _fields->Channels               = 0;
    _fields->MinCSForwardingDelay   = 0;
    _fields->ExtendNbrType          = 0;	// 2-hop neighborhood

    _channel_ie                     = NULL;
    _burst_profile                  = new c_burst_profile_t*[6];

    for (int i = 0; i < _fields->Num_Burst_Profile; i++) {

        _burst_profile[i] = new c_burst_profile_t;
        _burst_profile[i]->FecCodeType = i + 1;

    }

    setLength(fields_len() + 6 * 3);
}

Network_Des_ie::Network_Des_ie(void* pBuffer, int pLength)
: Embedded_data_hdr(pBuffer)
{
    _fields = (fields_t*)((char*)pBuffer + hdr_len());
}

Network_Des_ie::~Network_Des_ie()
{
    if (_flag) {
        int i;

        if (_channel_ie) {
            delete _channel_ie;
        }

        if (_burst_profile) {
            for (i = 0; i < _fields->Num_Burst_Profile; i++) {
                delete _burst_profile[i];
            }

            delete[] _burst_profile;
        }

        delete _fields;
    }
}

int
Network_Des_ie::copyTo(u_char* dstbuf)
{
    int pos;
    int i;

    memcpy(dstbuf, Embedded_data_hdr::_fields, hdr_len());
    pos = hdr_len();
    memcpy(dstbuf + pos, _fields, sizeof(fields_t));
    pos += sizeof(fields_t);
    for (i = 0; i < _fields->Channels; i++) {
    }
    for (i = 0; i < _fields->Num_Burst_Profile; i++) {
        memcpy(dstbuf + pos, _burst_profile[i],
            sizeof(c_burst_profile_t));
        pos += sizeof(c_burst_profile_t);
    }

    return pos;
}

void
Network_Des_ie::proc(mac802_16_MeshSS* mac, uint16_t)
{
    if (mac->is_initialized())
        return;

    mac->set_frame_duration(frameDurationCodes[_fields->FrameLengthCode]);
    mac->fr_mngr()->set_nf_tx_opp(_fields->MSH_CTRL_LEN);
    mac->fr_mngr()->set_nf_dsch_tx_opp(_fields->MSH_DSCH_NUM);
    mac->fr_mngr()->set_csch_data_fraction(_fields->MSH_CSCH_DATA);
    mac->fr_mngr()->set_nf_sch_frame_idx(_fields->SchedulingFrame);
}

void
Network_Des_ie::dump()
{
    printf("\tFLC = %d\n", _fields->FrameLengthCode);
    printf("\tCTRL_LEN = %d\n", _fields->MSH_CTRL_LEN);
    printf("\tDSCH_LEN = %d\n", _fields->MSH_DSCH_NUM);
    printf("\tOpID = %d\n", _fields->msbOperatorID << 8 || _fields->lsbOperatorID);
    printf("\tXmtExp = %d\n", _fields->XmtEnergyUnitExp);
}
