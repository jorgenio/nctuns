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

#ifndef __MSH_NCFG_NET_ENTRY_OPEN_IE_H
#define __MSH_NCFG_NET_ENTRY_OPEN_IE_H


#include "emb_data_hdr.h"


namespace Msh_ncfg {
    class Net_entry_open_ie;
}


class Alloc_base;

class Msh_ncfg::Net_entry_open_ie: public Embedded_data_hdr {

private:
    struct fields_t {
        u_char msStart;
        u_char msRange;
        u_char msbFnum;
        u_char lsbFnum:4;
        u_char XmtChannel:4;
        u_char msbSchValidity:8;
        u_char lsbSchValidity:4;
        u_char RcvChannel:4;
        u_char EstPropDelay:4;
        u_char reserved:4;
    };

private:
    fields_t* _fields;

public:
    Net_entry_open_ie(Net_entry_open_ie* ie_p);
    Net_entry_open_ie(u_char pStart, u_char pRange, u_int32_t pFn,
            u_char pXmtCh, u_char pRcvCh, u_int32_t pValid, u_char pDelay);
    Net_entry_open_ie(void* pBuffer, int pLength);
    virtual ~Net_entry_open_ie();

    int copyTo(u_char* dstbuf);
    void proc(mac802_16_MeshSS*, uint16_t);
    void dump();

    inline int getChInfo(int& XmtCh, int& RcvCh)
    {
        XmtCh = _fields->XmtChannel;
        RcvCh = _fields->RcvChannel;
        return XmtCh;
    }
    inline int getDelay()
    { return _fields->EstPropDelay; }

private:
    void _get_sch_info(Alloc_base& sch_alloc);
};


#endif /* __MSH_NCFG_NET_ENTRY_OPEN_IE_H */
