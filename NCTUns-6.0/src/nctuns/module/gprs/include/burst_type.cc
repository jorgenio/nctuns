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

#include <iostream>
#include <nctuns_api.h>
#include <gprs/include/GPRS_rlcmac_message.h>

#include "burst_type.h"

using namespace std;

 NormalBurst::NormalBurst (uchar* data_area,ulong ts, uchar stealing_bit,uchar burst_type): Burst(burst_type) {
    ts_            = ts;
    stealing_bit_  = stealing_bit;

    if (!data_area) {
        cout << "NB constructor() : data_area is null" << endl;
        return;
    }

    data_ptr1  = data_area;
    data_ptr2  = data_area+7;

    ref_cnt         = 0;
    rec_entry_ptr   = new SharedMsgElem( (GetCurrentTime()) , DUMMYBURST , NORMAL_BURST , this );
    shared_obj_insert(rec_entry_ptr);

 }

 int NormalBurst::copy_nb(NormalBurst* nb1) {

    if (!nb1) {
        cout << "NB::copy_nb(): given NB is null" << endl;
        return 0;
    }

    if (!nb1->data_ptr1) {
        cout << "NB::copy_nb(): nb1->data_area is null" << endl;
        return 0;
    }

    ts_             = nb1->ts_;
    stealing_bit_   = nb1->stealing_bit_;
    data_ptr1       = nb1->data_ptr1;
    data_ptr2       = nb1->data_ptr2;

    return 1;
 }

 AccessBurst::AccessBurst(ushort ra_info, ulong ts1, ulong ts2,uchar burst_type) : Burst(burst_type) {

    ts1_       = ts1;
    ts2_       = ts2;
    ra_info_   = ra_info;

    ref_cnt         = 0;
    rec_entry_ptr   = new SharedMsgElem( (GetCurrentTime()) , DUMMYBURST , ACCESS_BURST , this );
    shared_obj_insert(rec_entry_ptr);

 }

 SynchronizationBurst::SynchronizationBurst(ulong ts1,ulong ts2,uchar* data_area, uchar burst_type): Burst(SYNC_BURST) {

    ts1_   = ts1;
    ts2_   = ts2;

    if (!data_area) {
        cout << "NB constructor() : data_area is null" << endl;
        return;
    }

    data_ptr1_  = data_area;
    data_ptr2_  = data_area+4;

    ref_cnt         = 0;
    rec_entry_ptr   = new SharedMsgElem( (GetCurrentTime()) , DUMMYBURST , SYNC_BURST , this );
    shared_obj_insert(rec_entry_ptr);

}

DummyBurst::DummyBurst(ulong ts, uchar burst_type): Burst(DUMMY_BURST) {

    ts_=ts;
    ref_cnt         = 0;
    rec_entry_ptr   = new SharedMsgElem( (GetCurrentTime()) , DUMMYBURST , DUMMY_BURST , this );
    shared_obj_insert(rec_entry_ptr);

}

Burst::~Burst() {

    if ( rec_entry_ptr ) {
        shared_obj_remove_entry( rec_entry_ptr );
        delete rec_entry_ptr;
    }
    rec_entry_ptr   = NULL;
    burst_type_     = 0;
    ref_cnt         = 0;
    //printf("~Burst(): delete burst type %ld.\n", burst_type_);
}
