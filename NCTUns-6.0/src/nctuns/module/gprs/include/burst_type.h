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

#ifndef __BURST_TYPE_H__
#define __BURST_TYPE_H__

#include <gprs/include/types.h>

#define NB_DATA_LENGTH              114
#define TRAINING_SEQ_LENGTH         26
#define BURST_LENGTH                148
#define ACCESS_BURST_LENGTH         88

/* burst period */

#ifdef __TICK_IS_ONE_NANO_SEC

#define BIT_PERIOD      3692    /* nano sec.  = 36.92 */
#define BURST_PERIOD    577000  /* micro sec. */

#else

#define BIT_PERIOD      3700      /* nano sec.  : the accurate value = 36.92 tick where tick = 100 ns */
#define BURST_PERIOD    5772      /* micro sec. : the accurate value = 5759.52 tick where tick = 100 ns*/

#endif

/* burst type definition */
#define NORMAL_BURST    1
#define ACCESS_BURST    2
#define FREQ_COR_BURST  3
#define SYNC_BURST      4
#define DUMMY_BURST     5
#define SI13_BURST      6
#define CTL_BURST       7

class SharedMsgElem;

typedef class Burst {
    protected:
       uchar                    burst_type_;
       ulong                    ref_cnt;
       SharedMsgElem*           rec_entry_ptr;

    public:
        Burst(uchar burst_type)             {burst_type_ = burst_type;ref_cnt=0;rec_entry_ptr=0;}
        ~Burst();
        uchar           get_burst_type()    {return burst_type_;}
        int             inc_refcnt()        {++ref_cnt;return 1;}
        int             dec_refcnt()        { if (ref_cnt>0) --ref_cnt;return 1;}
        ulong           get_refcnt()        {return ref_cnt;}
        SharedMsgElem*  get_rec_entry()     {return rec_entry_ptr;}

} Burst ;

typedef class NormalBurst : public Burst {
    private:
        ulong  ts_;
        uchar  stealing_bit_;
        uchar* data_ptr1;
        uchar* data_ptr2;
    public:
        NormalBurst (uchar* data_area,ulong ts, uchar stealing_bit,uchar burst_type=NORMAL_BURST);
        ulong   get_ts()           {return ts_;}
        bool    get_stealing_bit() {return stealing_bit_;}
        uchar*  get_data_ptr1()    {return data_ptr1;}
        uchar*  get_data_ptr2()    {return data_ptr2;}
        int     copy_nb(NormalBurst* nb1);
} NB;

typedef class AccessBurst : public Burst{
    private:
        ulong  ts1_;
        ulong  ts2_;
        ushort ra_info_;
    public:
        AccessBurst(ushort ra_info, ulong ts1, ulong ts2,uchar burst_type=ACCESS_BURST);
        ulong   get_ts1()       {return ts1_;}
        ulong   get_ts2()       {return ts2_;}
        ushort  get_ra_info()   {return ra_info_;}
} AB;

typedef class FreqCorrectionBurst : public Burst{
    private:
        uchar* padding_bit;
    public:
        FreqCorrectionBurst(uchar burst_type=FREQ_COR_BURST): Burst(burst_type) {padding_bit=NULL;}
} FCB;

typedef class SynchronizationBurst : public Burst {
    private:
       ulong  ts1_;
       ulong  ts2_;
       uchar* data_ptr1_;
       uchar* data_ptr2_;
    public:
       SynchronizationBurst(ulong ts1, ulong ts2, uchar* data_area, uchar burst_type=SYNC_BURST);
       ulong    get_ts1()           {return ts1_;}
       ulong    get_ts2()           {return ts2_;}
       uchar*   get_data_ptr1()     {return data_ptr1_;}
       uchar*   get_data_ptr2()     {return data_ptr2_;}
} SB;

typedef class DummyBurst : public Burst {
    private:
        ushort ts_;
    public:
        DummyBurst(ulong ts,uchar burst_type=DUMMY_BURST);
        ulong get_ts()   {return ts_;}
} DB;

#endif
