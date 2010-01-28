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

/* This file implements classes used by RLC module
 *
 * CreateDate: 	09/29/2003
 * Author:	Chih-che Lin
 * Email:	jclin@csie.nctu.edu.tw
 */

#include <stdio.h>
#include <string.h>
#include <gprs/include/types.h>
#include <gprs/include/channel_coding.h>
#include <gprs/rlc/rlc_shared_def.h>

using namespace std;
#ifndef __RLC_IMPL_H
#define __RLC_IMPL_H

//#define __PACK_UNPACK_FUNCTION_DEBUG
    uchar* channel_encoding(int scheme_num, uchar* data) {

        if (!data) {
            printf("Assertion failed: The input of channel_coding(): pointer 'data' is NULL \n");
            return NULL;
        }


        int (*fp) (uchar* , uchar* ) = NULL ;

        uchar *symbol = new uchar[57];
        IS_NULL_STR(symbol,"channel_encoding(): symbol is null \n",NULL);
        bzero(symbol,57);

        switch (scheme_num) {
        case 1: fp = CS1_encode;
        	break;
        case 2: fp = CS2_encode;
        	break;
        case 3: fp = CS3_encode;
        	break;
        case 4: fp = CS4_encode;
        	break;
        default: fp = CS2_encode;
        }

        if (!fp) {
            printf("In channel_coding(): Assertion failed: No corresponding function pointer\n");
            return NULL;
        }


        if ( ( fp(data,symbol) ) < 0 ) {
            printf("Debug: Rlc channel_encoding(): Block encoding failed. \n");
            return NULL;
        }

        return symbol;
    }

    uchar* channel_decoding(int scheme_num, uchar* symbol) {

        if (!symbol) {
            printf("Assertion failed: The input of channel_coding(): pointer 'symbol' is NULL \n");
            return NULL;
        }

        int (*fp) (uchar* , uchar* ) = NULL;
        uchar* data = NULL;

        try {
            switch (scheme_num) {
            case 1: fp 	= CS1_decode;
                    data 	= new uchar[CS1_BLKSIZE+1];
                    bzero(data, CS1_BLKSIZE+1);
                    break;
            case 2: fp 	= CS2_decode;
        	    data	= new uchar[CS2_BLKSIZE+1];
        	    bzero(data, CS2_BLKSIZE+1);
        	    break;
            case 3: fp 	= CS3_decode;
        	    data	= new uchar[CS3_BLKSIZE+1];
        	    bzero(data, CS3_BLKSIZE+1);
        	    break;
            case 4: fp 	= CS4_decode;
        	    data	= new uchar[CS4_BLKSIZE+1];
        	    bzero(data, CS4_BLKSIZE+1);
        	    break;
            default:
        	    printf ("RLC DECODE function: unknown coding scheme number %d \n", scheme_num);
        	    return NULL;
            }
        }
        catch (std::exception& e) {
            cout << "channel_decoding():" << e.what() << endl;
            return NULL;
        }

        if ( ( fp(symbol,data) ) < 0 ) {
            printf("Debug: Rlc channel_decoding(): Block decoding failed. It may be caused by BIT error\n");
            return NULL;
        }
        return data;
    }

#endif
