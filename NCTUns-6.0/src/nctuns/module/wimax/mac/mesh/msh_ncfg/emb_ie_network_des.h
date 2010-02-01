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

#ifndef __MSH_NCFG_NETWORK_DES_IE_H
#define __MSH_NCFG_NETWORK_DES_IE_H


#include "emb_data_hdr.h"


namespace Msh_ncfg {
    class Network_Des_ie;
}


class Frame_mngr;

class Msh_ncfg::Network_Des_ie: public Embedded_data_hdr {

private:
    struct fields_t {

        u_char FrameLengthCode:4;
        u_char MSH_CTRL_LEN:4;
        u_char MSH_DSCH_NUM:4;
        u_char MSH_CSCH_DATA:4;
        u_char SchedulingFrame:4;
        u_char Num_Burst_Profile:4;
        u_char msbOperatorID;
        u_char lsbOperatorID;
        u_char XmtEnergyUnitExp:4;
        u_char Channels:4;
        u_char MinCSForwardingDelay:7;
        u_char ExtendNbrType:1;
    };

    struct c_burst_profile_t {
        u_char FecCodeType;
        u_char ExitThreshold;
        u_char EntryThreshold;
    };

private:
    fields_t*           _fields;
    Channel_ie*         _channel_ie;
    c_burst_profile_t** _burst_profile;

public:

    Network_Des_ie(const Frame_mngr*);
    Network_Des_ie(void* pBuffer, int pLength);
    virtual ~Network_Des_ie();

    int copyTo(u_char* dstbuf);
    void proc(mac802_16_MeshSS*, uint16_t);
    void dump();

    inline size_t fields_len() { return sizeof(fields_t); }
};

#endif /* __MSH_NCFG_NETWORK_DES_IE_H */
