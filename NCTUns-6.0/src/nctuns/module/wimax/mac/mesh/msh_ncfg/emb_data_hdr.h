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

#ifndef __MSH_NCFG_EMBEDDED_H
#define __MSH_NCFG_EMBEDDED_H


#include <inttypes.h>
#include "../msh_ncfg.h"


namespace Msh_ncfg {
    class Embedded_data_hdr;
}

class mac802_16_MeshSS;
class MSH_NCFG;

class Msh_ncfg::Embedded_data_hdr {

public:
    typedef enum {
        TYPE_RESERVED = 0x0,
        TYPE_NETWORK_DESCRIPTOR,
        TYPE_NETWORK_ENTRY_OPEN,
        TYPE_NETWORK_ENTRY_REJECT,
        TYPE_NETWORK_ENTRY_ACK,
        TYPE_NEIGHBOR_LINK_EST,
    } ie_type_t;

private:
    struct hdr_fields_t {

        u_char    Extended :1;
        u_char    reserved :3;
        ie_type_t Type     :4;// here! Maybe use TLV encoding or use inheritance
        u_char    Length   :8;

    }__attribute__((packed));

protected:
    bool            _flag;
    hdr_fields_t*   _fields;
    MSH_NCFG*       _ncfg;

public:
    Embedded_data_hdr(Embedded_data_hdr*);
    Embedded_data_hdr(ie_type_t);
    Embedded_data_hdr(void*);
    virtual ~Embedded_data_hdr();

    virtual int copyTo(u_char* dstbuf) = 0;
    virtual void proc(mac802_16_MeshSS*, uint16_t) = 0;
    virtual void dump() = 0;


    inline size_t hdr_len() { return sizeof(hdr_fields_t); }

    inline void set_extended() { _fields->Extended = 1; }
    inline bool is_extended() { return _fields->Extended; }

    inline ie_type_t type() { return _fields->Type; }

    inline void setLength(u_char l) { _fields->Length = l; }
    inline u_char length() { return _fields->Length; }

    inline void set_ncfg(MSH_NCFG* ncfg) { _ncfg = ncfg; }
    inline MSH_NCFG* ncfg() { return _ncfg; }

public:
    static Embedded_data_hdr* create_emb_ie(
            void*, int, Msh_ncfg::link_est_ie_format_t);
};


namespace Msh_ncfg {
    class Channel_ie;
    class Net_entry_reject_ie;
}

class Msh_ncfg::Channel_ie {
    /*
     * FIXME: implement me.
     */
};


class Msh_ncfg::Net_entry_reject_ie: public Embedded_data_hdr {
    /*
     * FIXME: implement me.
     */
    u_char RejectionCode;
    u_char Reason[20];

    Net_entry_reject_ie()
    : Embedded_data_hdr(TYPE_NETWORK_ENTRY_REJECT)
    {
        _fields->Length = 21;
    }
    Net_entry_reject_ie(u_char* pBuffer, int pLength);

    int copyTo(u_char* dstbuf)
    {
        return 0;
    }
};


#endif
