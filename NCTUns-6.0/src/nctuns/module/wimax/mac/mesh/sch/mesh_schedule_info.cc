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

#include <math.h>
#include <nctuns_api.h>

#include "data/alloc_base.h"
#include "data/scheduler.h"
#include "mesh_schedule_info.h"
#include "../neighbor.h"
#include "../util_map.h"

#define VERBOSE_LEVEL           MSG_INFO
#include "../../verbose.h"


#define PRINF_NCFG_SCHED_INFO   0
#define PRINF_DSCH_SCHED_INFO   0


ScheduleAllocStat::sched_alloc_status_t
ScheduleAllocStat::get_status()
{
    return status;
}

uint64_t
ScheduleAllocStat::get_time_point_for_starting_to_request()
{
    return time_point_for_starting_to_request;

}

uint64_t
ScheduleAllocStat::get_time_point_for_receiving_grant()
{
    return time_point_for_receiving_grant;

}

uint64_t
ScheduleAllocStat::get_time_point_for_starting_to_confirm()
{
    return time_point_for_starting_to_confirm;

}

int
ScheduleAllocStat::set_status(ScheduleAllocStat::sched_alloc_status_t status1)
{
    status = status1;
    return 1;
 
}

int
ScheduleAllocStat::set_time_point_for_starting_to_request(uint64_t tick)
{
    time_point_for_starting_to_request = tick;
    ASSERT((status == init), "ScheduleAllocStat::set_time_point_for_starting_to_request: status!=init.\n");
    status = req_timestamped;
    return 1;

}

int
ScheduleAllocStat::set_time_point_for_receiving_grant(uint64_t tick)
{
    time_point_for_receiving_grant = tick;
    ASSERT((status == req_timestamped), 
        "ScheduleAllocStat::set_time_point_for_starting_to_request: status!=req_timestamped.\n");
    status = grant_timestamped;
    return 1;

}

int
ScheduleAllocStat::set_time_point_for_rejection(uint64_t tick)
{
    time_point_for_receiving_grant = tick;
    ASSERT((status == req_timestamped), 
        "ScheduleAllocStat::set_time_point_for_starting_to_request: status!=req_timestamped.\n");
    status = rejected;
    return 1;

}


int
ScheduleAllocStat::set_time_point_for_starting_to_confirm(uint64_t tick)
{
    time_point_for_starting_to_confirm = tick;
    ASSERT((status == grant_timestamped), 
        "ScheduleAllocStat::set_time_point_for_starting_to_request: status!=grant_timestamped.\n");
    status = confirm_timestamped;
    return 1;

}

int ScheduleAllocStat::set_request_txopp(uint32_t tx_opp) {

    request_txopp = tx_opp;
    return 1;

}

int ScheduleAllocStat::set_confirm_txopp(uint32_t tx_opp, uint32_t ncfg_max_txopp, uint32_t dsch_max_txopp) {

    confirm_txopp = tx_opp;
    
    if ( request_txopp <= confirm_txopp )
        required_txopp = confirm_txopp - request_txopp;
    else
        required_txopp = confirm_txopp + dsch_max_txopp - request_txopp;
        
    return 1;

}

uint32_t ScheduleAllocStat::get_request_txopp() {

    return request_txopp;

}

uint32_t   ScheduleAllocStat::get_confirm_txopp() {

    return confirm_txopp;

}

uint32_t ScheduleAllocStat::get_required_txopp() {

    return required_txopp;

}

ScheduleAllocStat::ScheduleAllocStat()
{
    
    status          = init;
    
    peerNodeID      = 0;
    linkID          = 0;
    startFrameNum   = 0;
    start           = 0;
    range           = 0;
    validity        = 0;
    duration        = 0;

    time_point_for_starting_to_request = 0;
    time_point_for_receiving_grant     = 0;
    time_point_for_starting_to_confirm = 0;

    request_txopp = 0;
    confirm_txopp = 0;
    required_txopp = 0;
}

int ScheduleAllocStat::print_status() {

    const char* str=NULL;
    if (status == req_timestamped)
        str="req_timestamped";
    else if (status == grant_timestamped)
        str="grant_timestamped";
    else if (status == confirm_timestamped)
        str="confirm_timestamped";
    else if (status == rejected)
        str="rejected";
    else {
        FATAL("unknown status.\n");
    }

    printf(" %s",str);
    
    return 1;
    
}

int ScheduleAllocStat::dump() {

    printf("Dump ScheduleAllocStat: \n");
    printf("pNid=%u, LinkID=%u, sFN=%u, sSlot=%u, range=%u, validity=%u, duration=%u, status=",
        peerNodeID, linkID, startFrameNum, start, range, validity, duration);
    print_status();
    printf("\n");
    return 1;
}

ScheduleAllocStat::ScheduleAllocStat(const Alloc_base* alloc_base_p)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    status          = init;
    peerNodeID      = alloc_p->peer()?
        alloc_p->peer()->node_id():Data_scheduler::NEW_NODE_ID;
    linkID          = alloc_p->link_id();
    startFrameNum   = alloc_p->frame_start();
    start           = alloc_p->slot_start();
    range           = alloc_p->slot_range();
    validity        = alloc_p->frame_range();
    duration        = 0;

    time_point_for_starting_to_request = 0;
    time_point_for_receiving_grant     = 0;
    time_point_for_starting_to_confirm = 0;

}

ScheduleAllocInfoList::StatData::StatData(uint32_t nid1)
{
    memset( this, 0, sizeof(StatData));
    min_finish_time_tick = 0xffffffffffffffffllu;
    min_required_txopp = 0xfffffffflu;
    nid = nid1;

}

int ScheduleAllocInfoList::dump() {
    
    if (!computed_flag) {

        compute_average();
        compute_stddev_for_time();
        compute_stddev_for_txopp();
        computed_flag = 1;

    }
    
    double min_milli = data->min_finish_time;
    double max_milli = data->max_finish_time;
    
    uint32_t min_txopp = data->min_required_txopp;
    uint32_t max_txopp = data->max_required_txopp;


    printf("[%03u] ScheduleStats: Number of schedules: %u (unfinished: %u, rejected: %u), Avg time: %3.5lf (ms), Stddev: %3.5lf\n",
        nid, data->num_of_schedules, data->num_of_unfinished_handshake, data->num_of_rejected_handshake, data->avg_finish_time, data->std_finish_time);

    printf("[%03u] ScheduleStats: Min time: %3.5lf, Max time: %3.5lf\n",
    	   nid, min_milli, max_milli);

    printf("[%03u] ScheduleStats: Number of schedules: %u (unfinished: %u, rejected: %u), Avg txopp: %3.5lf (opp), Stddev: %3.5lf\n",
           nid, data->num_of_schedules, data->num_of_unfinished_handshake,data->num_of_rejected_handshake, data->avg_required_txopp, data->std_required_txopp);

    printf("[%03u] ScheduleStats: Min txopp: %u, Max txopp: %u\n",
    	   nid, min_txopp, max_txopp);

    return 1;
}

ScheduleAllocInfoList::ScheduleAllocInfoList(uint32_t nid1, uint32_t ncfg_max_txopp1, uint32_t dsch_max_txopp1)
{
    computed_flag       = 0;
    nid                 = nid1;
    bucket_num          = MAX_LINK_ID + 1;
    data                = new stat_data_t(nid);
    sched_alloc_list    = new std::vector<sched_alloc_stat_t*>[bucket_num];

    ncfg_max_txopp = ncfg_max_txopp1;
    dsch_max_txopp = dsch_max_txopp1;
}

ScheduleAllocInfoList::~ScheduleAllocInfoList()
{
    for (uint32_t i=0; i<bucket_num  ;++i) {

        std::vector<sched_alloc_stat_t*>::iterator it;
        std::vector<sched_alloc_stat_t*>* list = &sched_alloc_list[i];
    
        if (list->size() == 0 )
            continue;
    
        for (it = list->begin(); it != list->end(); it++) {
            
            delete (*it);
            (*it) = NULL;

        }
    }

    delete data;
    delete[] sched_alloc_list;
    data = NULL;
    sched_alloc_list = NULL;
}

int
ScheduleAllocInfoList::insert(uint32_t bucket_index, sched_alloc_stat_t* stat_p )
{

    ASSERT(stat_p, "ScheduleAllocInfoList::%s: stat_p is null.\n", __FUNCTION__);
    ASSERT((bucket_index<=bucket_num), "ScheduleAllocInfoList::%s: illegal bucket index = %u, bucket_num=%u.\n",
         __FUNCTION__, bucket_index, bucket_num );
    
    sched_alloc_list[bucket_index].push_back(stat_p);

    return 1;
}

int
ScheduleAllocInfoList::remove()
{
    /* there is no need to implement this function yet.*/
    return 1;
}

sched_alloc_stat_t*
ScheduleAllocInfoList::find(const Alloc_base* alloc_base_p,
                            ScheduleAllocStat::sched_alloc_status_t status)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    uint32_t index = alloc_p->link_id();

    std::vector<sched_alloc_stat_t*>::iterator it;
    std::vector<sched_alloc_stat_t*>* list = &sched_alloc_list[index];
    std::vector<sched_alloc_stat_t*>::iterator tmp_it;
     
    tmp_it = list->end();

    DEBUG("search index=%u, bucket_size=%u, status=%u.\n", index, list->size(), status);
    if (list->size() == 0 )
        return NULL;

    for (it = list->begin(); it != list->end(); it++) {

        DEBUG("dump slot_aloc: ");DEBUG_FUNC((*it)->dump());
        if ( (*it)->get_status() == status)
            tmp_it = it;

    }

    DEBUG("dump tmp_slot_aloc: ");DEBUG_FUNC((*tmp_it)->dump());
    if (tmp_it!=list->end()) {
    
        return (*tmp_it);
    
    }
    else {
    
        return NULL;
    
    }

}

int
ScheduleAllocInfoList::record_request_time(
		const Alloc_base* alloc_base_p, uint64_t tick, uint32_t tx_opp)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    uint32_t index = alloc_p->link_id();

    sched_alloc_stat_t* stat_p = new sched_alloc_stat_t(alloc_p);

    ASSERT(stat_p, "ScheduleAllocInfoList::%s: stat_p is null.\n", __FUNCTION__);

    
    stat_p->set_time_point_for_starting_to_request(tick);
    stat_p->set_request_txopp(tx_opp);
    stat_p->set_status(sched_alloc_stat_t::req_timestamped);

    return insert(index, stat_p);

}

int
ScheduleAllocInfoList::record_granted_time(
		const Alloc_base* alloc_base_p, uint64_t tick)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    sched_alloc_stat_t* res_p = find(alloc_p, sched_alloc_stat_t::req_timestamped);

    if (!res_p) {

        printf("ScheduleAllocInfoList::%s: cannot find the corresponding entry. dump the specified entry: \n",
             __FUNCTION__);
        alloc_p->dump();
        assert(0);
    }

    res_p->set_time_point_for_receiving_grant(tick);
    res_p->set_status(sched_alloc_stat_t::grant_timestamped);
    return 1;
}

int
ScheduleAllocInfoList::record_confirm_time(
		const Alloc_base* alloc_base_p, uint64_t tick, uint32_t tx_opp)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    sched_alloc_stat_t* res_p = find(alloc_p, sched_alloc_stat_t::grant_timestamped);

    if (!res_p) {

        printf("ScheduleAllocInfoList::%s: cannot find the corresponding entry. dump the specified entry: \n",
             __FUNCTION__);
        alloc_p->dump();
        assert(0);
    }

    res_p->set_time_point_for_starting_to_confirm(tick);
    res_p->set_confirm_txopp(tx_opp, ncfg_max_txopp, dsch_max_txopp);
    res_p->set_status(sched_alloc_stat_t::confirm_timestamped);
    
    return 1;
}

int
ScheduleAllocInfoList::record_rejected_time(
		const Alloc_base* alloc_base_p, uint64_t tick)
{
    ASSERT(alloc_base_p, "%s: alloc_base_p is null.\n", __FUNCTION__);

    const schedule_alloc_t* alloc_p;
    assert(alloc_p = dynamic_cast<const schedule_alloc_t*>(alloc_base_p));

    sched_alloc_stat_t* res_p = find(alloc_p, sched_alloc_stat_t::req_timestamped);

    if (!res_p) {

        printf("ScheduleAllocInfoList::%s: cannot find the corresponding entry. dump the specified entry: \n",
             __FUNCTION__);
        alloc_p->dump();
        assert(0);
    }

    res_p->set_time_point_for_rejection(tick);
    res_p->set_status(sched_alloc_stat_t::rejected);
    return 1;
}


int
ScheduleAllocInfoList::update_min_max_finish_times(uint64_t fin_time, uint32_t required_txopp)
{
    if ( fin_time < data->min_finish_time_tick)
        data->min_finish_time_tick = fin_time;

    if ( fin_time > data->max_finish_time_tick )
        data->max_finish_time_tick = fin_time;
        
    if ( required_txopp < data->min_required_txopp)
        data->min_required_txopp = required_txopp;
    
    if ( required_txopp > data->max_required_txopp)
        data->max_required_txopp = required_txopp;

    return 1;

}

double
ScheduleAllocInfoList::compute_average()
{
    double total_milli = 0.0;
    uint32_t total_txopps = 0;
    
    uint32_t unfinished_handshake_cnt=0;
    uint32_t rejected_handshake_cnt=0;
    
    for (uint32_t i=0; i<bucket_num  ;++i) {

        std::vector<sched_alloc_stat_t*>::iterator it;
        std::vector<sched_alloc_stat_t*>* list = &sched_alloc_list[i];
    
        if (list->size() == 0 )
            continue;
    
        for (it = list->begin(); it != list->end(); it++) {

            if ((*it)->get_status() == ScheduleAllocStat::rejected ) {
            
                VINFO("The status of statistic_entry is rejected.(%u: %llu)\n",
                    (*it)->get_status(), (*it)->get_time_point_for_starting_to_request() );
                
                ++rejected_handshake_cnt;
                continue;
            }
            
            if ((*it)->get_status() != ScheduleAllocStat::confirm_timestamped ) {
            
                VINFO("The status of statistic_entry is not confirm_timestamped.(%u: %llu)\n",
                    (*it)->get_status(), (*it)->get_time_point_for_starting_to_request() );
                
                ++unfinished_handshake_cnt;
                continue;
            }
                
            
            ++(data->num_of_schedules);
            
            uint64_t request_tick = (*it)->get_time_point_for_starting_to_request();

            uint64_t confirm_tick = (*it)->get_time_point_for_starting_to_confirm();
            
            ASSERT((confirm_tick>=request_tick),"confirm_tick < request_tick");

            uint64_t diff_tick = confirm_tick - request_tick;
            
            double diff_milli = 0.0;
            TICK_TO_MILLI(diff_milli, diff_tick);

            total_milli += diff_milli;
            
            uint32_t required_txopp = (*it)->get_required_txopp();
            total_txopps += required_txopp;
            
            update_min_max_finish_times(diff_tick, required_txopp);

        }
    }

    if (data->num_of_schedules) {

        data->avg_finish_time    = total_milli / ((double)(data->num_of_schedules));
        data->avg_required_txopp = (double)total_txopps/ ((double)(data->num_of_schedules));
        
        TICK_TO_MILLI(data->min_finish_time,data->min_finish_time_tick);
        TICK_TO_MILLI(data->max_finish_time,data->max_finish_time_tick);
        data->num_of_unfinished_handshake = unfinished_handshake_cnt;
        data->num_of_rejected_handshake = rejected_handshake_cnt;
    }
    else {
       //WARN(("num_of_schedule is zero."));
    }
    
    return data->avg_finish_time;

}

double
ScheduleAllocInfoList::compute_stddev_for_time()
{
    double total_square = 0.0;
    
    for (uint32_t i=0; i<bucket_num  ;++i) {

        std::vector<sched_alloc_stat_t*>::iterator it;
        std::vector<sched_alloc_stat_t*>* list = &sched_alloc_list[i];
    
        if (list->size() == 0 )
            continue;
        
        for (it = list->begin(); it != list->end(); it++) {
        
            if ((*it)->get_status() != ScheduleAllocStat::confirm_timestamped ) {
            
                VINFO("The statistic_entry is not confirm_timestamped or rejected.(%u)\n",
                    (*it)->get_status());
                
                continue;
            }
            
            uint64_t request_tick = (*it)->get_time_point_for_starting_to_request();

            uint64_t confirm_tick = (*it)->get_time_point_for_starting_to_confirm();
            
            ASSERT((confirm_tick>=request_tick),"confirm_tick < request_tick");

            uint64_t diff_tick = confirm_tick - request_tick;
            double diff_ms_lf;
            TICK_TO_MILLI(diff_ms_lf, diff_tick);

            double var_lf  = diff_ms_lf - data->avg_finish_time;

            double diff_square = pow(var_lf,2);

            total_square += diff_square;
        
        }
    }

    if (data->num_of_schedules) {

       if (data->num_of_schedules<=1) {

           //WARN(("since the number of schedules is less than 2 (cur_value: %u), the stddev is meaningless.\n",
           //data->num_of_schedules));

       }
       else {
       
       double denominator = double(data->num_of_schedules-1);
       double temp = total_square/denominator;
       data->std_finish_time = sqrt(temp);

       }
    }
    else {
       //WARN(("num_of_schedule is zero."));
    }
    
    return data->std_finish_time;

}

double
ScheduleAllocInfoList::compute_stddev_for_txopp()
{
    double total_square = 0.0;
    
    for (uint32_t i=0; i<bucket_num  ;++i) {

        std::vector<sched_alloc_stat_t*>::iterator it;
        std::vector<sched_alloc_stat_t*>* list = &sched_alloc_list[i];
    
        if (list->size() == 0 )
            continue;
        
        for (it = list->begin(); it != list->end(); it++) {
        
            if ((*it)->get_status() != ScheduleAllocStat::confirm_timestamped ) {
            
                VINFO("The status of statistic_entry is not confirm_timestamped.(%u)\n",(*it)->get_status());
                
                continue;
            }
            
            
            uint32_t required_txopp = (*it)->get_required_txopp();
            
            double var_lf  = (double)required_txopp - data->avg_required_txopp;

            double diff_square = pow(var_lf,2);

            total_square += diff_square;
        
        }
    }

    if (data->num_of_schedules) {

       if (data->num_of_schedules<=1) {

           //WARN(("since the number of schedules is less than 2 (cur_value: %u), the stddev is meaningless.\n",
           //data->num_of_schedules));

       }
       else {
       
       double denominator = double(data->num_of_schedules-1);
       double temp = total_square/denominator;
       data->std_required_txopp = sqrt(temp);

       }
    }
    else {
       //WARN(("num_of_schedule is zero."));
    }
    
    return data->std_required_txopp;

}
