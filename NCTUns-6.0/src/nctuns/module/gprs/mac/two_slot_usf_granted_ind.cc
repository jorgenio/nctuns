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

#include <stdio.h>
#include <stdlib.h>
#include "two_slot_usf_granted_ind.h"

using namespace std;

TwoSlotUsfGrantedInd::TwoSlotUsfGrantedInd(uchar clear_value) {

    clear_value_    = clear_value;
    ind = new uchar*[8];

    for ( ulong i=0 ; i<8 ; ++i ) {

        ind[i] = new uchar[12];

        for ( ulong j=0 ; j<12 ;++j)
            ind[i][j] = clear_value_;
    }
}

TwoSlotUsfGrantedInd::~TwoSlotUsfGrantedInd() {
    for ( ulong i=0 ; i<8 ; ++i)
        delete ind[i];

    delete ind;
}

int TwoSlotUsfGrantedInd::clear() {

    for ( ulong i=0 ; i<8 ; ++i ) {

        for ( ulong j=0 ; j<12 ;++j)
            ind[i][j] = clear_value_;
    }

    return 1;
}

int TwoSlotUsfGrantedInd::unset(ulong cur_tn , long ind_blkn) {

    if ( ind_blkn < 0 ) {
        printf("TSUGI::unset(): illegal block number.\n");
        exit(1);
    }

    ind[cur_tn][ind_blkn] = 0;

    return 1;
}

int TwoSlotUsfGrantedInd::set(ulong cur_tn , long ind_blkn) {

    if ( ind_blkn < 0 ) {
        printf("TSUGI::set(): illegal block number.\n");
        exit(1);
    }

    ind[cur_tn][ind_blkn] = 1;

    return 1;
}

int TwoSlotUsfGrantedInd::hit_search(ulong cur_tn,long cur_blkn) {

    if ( cur_blkn < 0 )
        return false;

    if ( ind[cur_tn][cur_blkn] )
        return true;
    else
        return false;
}
