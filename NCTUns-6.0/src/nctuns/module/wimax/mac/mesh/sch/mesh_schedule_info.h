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

#ifndef __NCTUNS_WIMAX_MESH_SCHEDULE_INFO_H__
#define __NCTUNS_WIMAX_MESH_SCHEDULE_INFO_H__

#include <vector>


class Alloc_base;
class Neighbor;


typedef class ScheduleAllocStat {

public:
    typedef enum ScheduleAllocStatStatusType{

        init                = 0,
        req_timestamped     = 1,
        grant_timestamped   = 2,
        confirm_timestamped = 3,
        rejected            = 4,

    } sched_alloc_status_t;

private:
    
    sched_alloc_status_t status;

    /* statistics */
    uint16_t            peerNodeID;
    uint8_t             linkID;
    uint32_t            startFrameNum;
    uint8_t             start;
    uint8_t             range;
    uint32_t            validity;
    uint32_t            duration;

    uint64_t            time_point_for_starting_to_request;
    uint64_t            time_point_for_receiving_grant;
    uint64_t            time_point_for_starting_to_confirm;
    
    uint32_t            request_txopp;
    uint32_t            confirm_txopp;
    uint32_t            required_txopp;

public:

    ScheduleAllocStat();
    ScheduleAllocStat(const Alloc_base* minislot_alloc_p);

    uint64_t    get_time_point_for_starting_to_request();
    uint64_t    get_time_point_for_receiving_grant();
    uint64_t    get_time_point_for_starting_to_confirm();

    int         set_time_point_for_starting_to_request(uint64_t tick);
    int         set_time_point_for_receiving_grant(uint64_t tick);
    int         set_time_point_for_starting_to_confirm(uint64_t tick);
    int         set_time_point_for_rejection(uint64_t tick);
    int         set_status(sched_alloc_status_t status1);
    
    int         set_request_txopp(uint32_t);
    int         set_confirm_txopp(uint32_t, uint32_t, uint32_t);
    
    uint32_t    get_request_txopp();
    uint32_t    get_confirm_txopp();
    uint32_t    get_required_txopp();
    
    sched_alloc_status_t get_status();
    int         dump();
    int         print_status();
    
} sched_alloc_stat_t;


typedef class ScheduleAllocInfoList {

public:
    typedef class StatData {

    public:
        uint32_t nid;
        uint32_t num_of_schedules;
        uint32_t num_of_unfinished_handshake;
        uint32_t num_of_rejected_handshake;
        uint64_t min_finish_time_tick;
        uint64_t max_finish_time_tick;
        
        double   avg_required_txopp;
        double   std_required_txopp;
        uint32_t min_required_txopp;
        uint32_t max_required_txopp;
        
        double   avg_finish_time;
        double   std_finish_time;
        double   min_finish_time;
        double   max_finish_time;

        StatData(uint32_t nid1);

    } stat_data_t;

private:
    uint32_t                          computed_flag;
    uint32_t                          nid;
    stat_data_t*                      data;
    uint32_t                          ncfg_max_txopp;
    uint32_t                          dsch_max_txopp;
    uint32_t                          bucket_num;
    std::vector<sched_alloc_stat_t*>* sched_alloc_list;


//    int is_in_between(MinislotAlloc* minislot_alloc_p) {return 1;}
    int insert(uint32_t bucket_index, sched_alloc_stat_t* stat_p );
    sched_alloc_stat_t* find(const Alloc_base* minislot_alloc_p, ScheduleAllocStat::sched_alloc_status_t status);
    int remove();
    int update_min_max_finish_times(uint64_t fin_time, uint32_t required_txopp);

    public:
    ScheduleAllocInfoList(uint32_t nid1, uint32_t ncfg_max_txopp1, uint32_t dsch_max_txopp1);
    ~ScheduleAllocInfoList();
    
    int    record_request_time(const Alloc_base*, uint64_t, uint32_t);
    int    record_granted_time(const Alloc_base*, uint64_t);
    int    record_confirm_time(const Alloc_base*, uint64_t, uint32_t);
    int    record_rejected_time(const Alloc_base*, uint64_t);

    double compute_average();
    double compute_stddev_for_time();
    double compute_stddev_for_txopp();
    int    dump();

} sched_alloc_stat_list_t;


#endif /* __NCTUNS_WIMAX_MESH_SCHEDULE_INFO_H__ */
