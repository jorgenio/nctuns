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

/* This header file defines RLC/MAC control messages used
 * in NCTUns GPRS MAC/RLC module. At present, the messages defined here
 * conforms to GPRS standard but not to EGPRS or later extensions.
 *
 * Create Date:         10/07/2003
 * Modified Date:       22/07/2003
 * Author:              Mike Chih-che Lin
 * Email:               jclin@csie.nctu.edu.tw
 */

#ifndef __GPRS_RLCMAC_MESSAGE__H_
#define __GPRS_RLCMAC_MESSAGE__H_
#include <gprs/include/types.h>
#include <gprs/include/gprs_nodetype.h>
#include <gprs/include/bss_message.h>
#include <gprs/include/generic_list.h>
#include <gprs/include/generic_list_impl.h>
#include <string.h>

/* misc. structures */
/* message type definition:
 * types of which the most significant bit is 1 belong to the group of distributed messages
 * types whose the most significant bit is 0 are classfied into non-distributed messages
 */
#define DATA                                -1
#define SI13_INFO                           -2
/* downlink messages */
#define PKT_ACCESS_REJECT                                   0x21
#define PKT_CELL_CHANGE_ORDER                       0x01
#define PKT_DOWNLINK_ASSIGNMENT                     0x02
#define PKT_MEASUREMENT_ORDER                       0x03
#define PKT_PAGING_REQUEST                                  0x22
#define PKT_PDCH_RELEASE                                    0x23
#define PKT_POLLING_REQUEST                                 0x04
#define PKT_POWER_CTL_TIMING_ADVANCE        0x05
#define PKT_PRACH_PARAM                                     0x24
#define PKT_QUEUEING_NOTIFICATION                   0x06
#define PKT_TIMESLOT_RECONFIGURATION        0x07
#define PKT_TBF_RELEASE                                     0x08
#define PKT_UPLINK_ACKNACK                                  0x09
#define PKT_UPLINK_ASSIGNMENT                       0x0a
#define PKT_CELL_CHANGE_CONTINUE                        0x0b
#define PKT_NEIGHBOR_CELL_DATA                          0x0c
#define PKT_SERVING_CELL_DATA                           0x0d
#define PKT_DBPSCH_ASSIGNMENT                           0x0e
#define MULTIPLE_TBF_DOWNLINK_ASSIGNMENT        0x0f
#define MULTIPLE_TBF_UPLINK_ASSIGNMENT          0x10
#define MULTIPLE_TBF_TIMESLOT_RECONFIG          0x11
#define PKT_DOWNLINK_DUMMY_CONTROL_BLK          0x25
#define PSI1                                                0x31
#define PSI2                                0x32
#define PSI3                                                0x33
#define PSI3_BIS                                            0x34
#define PSI4                                                0x35
#define PSI5                                                0x36
#define PSI6                                                0x30
#define PSI7                                                0x38
#define PSI8                                                0x39
#define PSI13                                               0x37
#define PSI14                                               0x3a
#define PSI3_TER                                            0x3c
#define PSI3_QUATER                                         0x3d
#define PSI15                                               0x3e
#define PSI16                                               0x28

/* uplink messages */
#define PKT_CELL_CHANGE_FAILURE                         0x00
#define PKT_CONTROL_ACK                                         0x01
#define PKT_DOWNLINK_ACKNACK                            0x02
#define PKT_UPLINK_DUMMY_CONTROL_BLK            0x03
#define PKT_MEASUREMENT_REPORT                          0x04
#define PKT_ENHANCED_MEASUREMENT_REPORT         0x0a
#define PKT_RESOURCE_REQUEST                            0x05
#define PKT_MOBILE_TBF_STATUS                           0x06
#define PKT_PSI_STATUS                                          0x07
#define EGPRS_PKT_DOWNLINK_ACKNACK                      0x08
#define PKT_PAUSE                                                       0x09
#define ADDITIONAL_MS_RA_CAPABILITIES           0x0b
#define PKT_CELL_CHANGE_NOTIFICATION            0x0c
#define PKT_SI_STATUS                                           0x0d


/* message content definition */
class AckNack {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    bool   final_ack_indication;
    uchar  starting_sequence_number;
    uchar* received_block_bitmap;
    public:

    AckNack();
    ~AckNack() {if (received_block_bitmap) delete received_block_bitmap;}
    int set_fai(bool fai)   {final_ack_indication = fai; return 1;}
    int set_ssn(uchar ssn)  {starting_sequence_number = ssn; return 1;}
    int set_rbb(uchar *rbb) {if (rbb) {received_block_bitmap = rbb;return 1;} else return -1;}
    bool   get_fai() {return final_ack_indication;}
    uchar  get_ssn() {return starting_sequence_number;}
    uchar* get_rbb() {return received_block_bitmap;}
    int pack(uchar* buf);
    int unpack(uchar* buf);
};

class ChannelRequestDescription {
    public:
        uchar  peak_throughput_class;
        uchar  radio_priority;
        uchar  rlc_mode;
        uchar  llc_pdu_type;
        ushort rlc_octet_count;
};


/* GPRS Mobile Allocation */
typedef class GprsMobileAllocation {
    public:
    uchar  hsn;
    ushort rfl_number;
    uchar  rfl_number_list;
    uchar  ma_length;
    uchar  ma_bitmap[65];
    uchar  arfcn_index;
    uchar  arfcn_index_list;
    GprsMobileAllocation() {bzero(this,sizeof(GprsMobileAllocation));}
} GprsMA;


class IndirectEncodingStruct {
    public:
        uchar maio;
        uchar ma_number;
        uchar change_mark1;
        uchar change_mark2;
};

class DirectEncoding1Struct {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    DirectEncoding1Struct();
    ~DirectEncoding1Struct();
    uchar maio;
    GprsMobileAllocation* gprs_ma;


};

class MaFreqList {};
class DirectEncoding2Struct {
    public:
    uchar maio;
    uchar hsn;
    uchar len_of_ma_freq_list;
    MaFreqList* ma_freq_list;
    DirectEncoding2Struct();
    ~DirectEncoding2Struct();
};

class FrequencyParameter {
    public:
    uchar  tsc;
    uchar  format; /* 00 indicates arfcn, 01 indicates indirect encoding,
                    * 10 indicates direct encoding 1, 11 indicates direct encoding 2
                    */
    ushort arfcn;
    IndirectEncodingStruct* indirect_encoding;
    DirectEncoding1Struct*  direct_encoding1;
    DirectEncoding2Struct*  direct_encoding2;

    FrequencyParameter() {bzero(this,sizeof(FrequencyParameter));}
    FrequencyParameter(uchar format1);
    ~FrequencyParameter();
};

/* Global Power Control Parameters */
typedef class GlobalPowerControlParameter {
    public:
    uchar alpha;
    uchar t_avg_w;
    uchar t_avg_t;
    uchar pb;
    uchar pc_meas_chan;
    uchar int_meas_channel_list_avail;
    uchar n_avg_l;
}GlobalPowerCtlParam;

/* Global TFI is made up by either an uplink tfi or a downlink tfi. Thus,
 * it seems not be necessary to build a corrsponding structure. So far, it
 * is represented as a 5-bit integer.
 */
#define D_BIT_UPLINK   0
#define D_BIT_DOWNLINK 1
class GlobalTfi {
    public:
    bool  direction; /* 0 indicates uplink TFI, otherwise downlink TFI. */
    uchar tfi;
};


class PacketRequestReference {
    public:
    ushort ra_info;
    ushort fn;

};

class PacketTimingAdvance {
    public:
    uchar timing_advance_value;
    uchar timing_adance_index;
    uchar timing_advance_timeslot_number;
};

class GlobalPacketTimingAdvance {
    uchar timing_advance_value;
    /* uplink */
    uchar uplink_timing_adance_index;
    uchar uplink_timing_advance_timeslot_number;
    /* downlink */
    uchar downlink_timing_adance_index;
    uchar downlink_timing_advance_timeslot_number;

};

class PowerControlParameter {
    public:
    uchar alpha;
    uchar gamma_tn0;
    uchar gamma_tn1;
    uchar gamma_tn2;
    uchar gamma_tn3;
    uchar gamma_tn4;
    uchar gamma_tn5;
    uchar gamma_tn6;
    uchar gamma_tn7;
};

class PrachControlParameter {
    public:
    ushort access_control_class;
    uchar  max_retrans[4];
    uchar  s;
    uchar  tx_int;
    uchar  persistence_level[4];

};

typedef class CellIdentification {
    public:
    uchar  lai; /* location area identifier */
    uchar  rac; /* routing area code */
    ushort cell_identity; /* cell identity */

} CellId;

class GprsCellOptions {
    public:
    uchar nmo;
    uchar timer3168;
    uchar timer3192;
    uchar drx_timer_max;
    uchar access_burst_type;
    uchar control_ack_type;
    uchar bs_cv_max;
    uchar pan_dec;
    uchar pan_inc;
    uchar pan_max;
    /*uchar egprs_packet_channel_request;
    uchar bep_period;
    uchar pfc_feature_mode;
    uchar dtm_support;
    uchar ccn_active;
    uchar nw_ext_utbf;
    uchar bss_paging_coordination;*/
};

typedef class PccchOrganizationParameters {
    public:
    uchar bs_pcc_rel;
    uchar bs_pbcch_blks;
    uchar bs_pag_blks_res;
    uchar bs_prach_blks;

} PccchOrgParam;

class PrachControlParameters {
    public:
    ushort  acc_ctl_class;
    uchar   max_retrans[4];
    uchar   s;
    uchar   tx_int;
    uchar   persistence_level[4];
};

/**** Uplink TBF establishment messages ****/

/* Packet Access Reject */
typedef class PacketAccessReject {
      friend class NonDistributedMsg;
      friend class DistributedMsg;
      public:
      uchar wait_indication;
      uchar wait_indication_size;
      uchar rb_id;


} PAR;

/* Packet Channel Request */
/* According to ACCESS_BURST_TYPE indicated in System Information,
 * Packet Channel Request message has two kinds of format with 11 bits or 8 bits.
 */
#define EIGHT_BITS      0
#define ELEVEN_BITS     1

#define ONEPHASE_ACCESS 1
#define TWOPHASE_ACCESS 2
#define FOR_PAGING_RESPONSE 3

typedef class PacketChannelRequest {

    public:
    bool  length_type;
    uchar format_type;
    uchar multislot_class;
    uchar priority;
    uchar number_of_blocks;
    uchar random_bits;
    PacketChannelRequest() {bzero(this,sizeof(PacketChannelRequest));}

    ushort  pack();
    int     unpack(ushort rainfo);

} PktChReq;


/* Packet Queuing Notification */
class PacketQueuingNotification {
    public:
    ushort tqi;
    friend class NonDistributedMsg;
    friend class DistributedMsg;
};



/* MS Radio Access Capabilities (see TS 24.008 10.5.1x )*/
class MsRadioAccessCapabilities {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    uchar ms_ra_capabilities_value_part;
    uchar access_technology_type;
    //uchar gmsk_power_class;
    //uchar _8psk_power_class;

    /* access capabilities struct */
    uchar   length;
    uchar   rf_power_capability;
    bool    a5;             /* encryption capability indicator */
    bool    es_ind;         /* Controlled early Classmark Sending */
    bool    ps;             /* pseudo syncronization */
    bool    vgcs;           /* voice group call service */
    bool    vbs;                /* voice broadcast service */

    /* multislot capability struct */
    uchar   hscsd_multislot_class;
    uchar   gprs_multislot_class;
    bool    gprs_extended_dynamic_allocation_capability;
    uchar   sms_value;          /* Switch-measure-switch : It indicates the time a MS needs to switch from a radio channel to another,
                                 * performing a neighbor cell power management and switching from that channel to another.
                                 */

    uchar   sm_value;           /* Switch-measure : It indicates the time a MS needs to swich from a radio channel to another and then
                                 * perform a neighbor cell power management.
                                 */

    uchar   dtm_gprs_multislot_class;
    uchar   mac_mode_support;
    uchar   revision_level_indicator;   /* Is a ME is release 98 or older if 0 or release 99 or later if 1. */
    uchar   geran_feature_package;          /* 0        GERAN feature package 1 not supported.
                                         * 1    GERAN feature package 1 supported.
                                         */

    uchar   extended_dtm_gprs_multislot_class;
    uchar   high_multislot_capability;

};

/* Packet Resource Request */
class MsRadioAccessCapability2 {
    public:
    MsRadioAccessCapabilities ms_rac;
};

class PacketResourceRequest {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    friend class GprsMsMac;
    friend class GrsBtsMac;

    uchar access_type;
    MsRadioAccessCapability2    ms_ra_ca2_ie;
    ChannelRequestDescription   ch_req_des;
    uchar change_mark;
    uchar c_value;
    uchar sign_var;
    uchar i_level_tn[8];
    uchar pfi;                  /* packet flow identifier */
    bool  additonal_ms_rac_information_available;
    bool  retransmission_of_prr; /* this bit indicates that whether this packet resource request is a retransmission. */

    /* indicators for packing message */
    bool  access_type_ind;
    bool  ms_ra_ca2_ie_ind;
    bool  change_mark_ind;
    bool  sign_var_ind;
    bool  i_level_tn_ind[8];

};

/* Packet Uplink Assignment */
typedef class SingleBlockAllocation {
    public:
    uchar   tn;
    uchar   alpha;
    uchar   gamma_tn;
    uchar   p0;
    bool    pr_mode;
    ushort  tbf_starting_time;

    SingleBlockAllocation() {bzero(this,sizeof(SingleBlockAllocation));}
}SBA;

class DynamicAllocation;
class SingleBlockAllocation;
typedef class PacketUplinkAssignment {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    friend class GprsMsMac;
    friend class GprsBtsMac;
    private:

    uchar                   persistence_level[4];
    uchar                   channel_coding_command;
    bool                    tlli_block_channel_coding;
    PacketTimingAdvance     packet_timing_advance;
    uchar                   sba_or_da;                 /* 0 stands for sba , 1 stands for da */
    FrequencyParameter*     frequency_params;
    DynamicAllocation*      dynamic_allocation;
    SingleBlockAllocation*  single_block_allocation;
    uchar                   packet_extended_timing_advance;
    /* EGPRS related fields are left for future expansion. */
    public:
    PacketUplinkAssignment() {bzero(this,sizeof(PacketUplinkAssignment));}
    ~PacketUplinkAssignment();
} PUA;

typedef class SingleBlkAllocFreqParam {
    public:
    SingleBlockAllocation*   sba;
    FrequencyParameter*      fp;
    SingleBlkAllocFreqParam() {bzero(this,sizeof(SingleBlkAllocFreqParam));}
} SbaFreq;

/**** Downlink TBF establishment messages ****/

/* Packet Downlink Assignment */
class PacketTimingAdvance;
typedef class PacketDownlinkAssignment {

    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:

    uchar                   persistence_level[4];
    uchar                   mac_mode;
    uchar                   rlc_mode;
    bool                    ctl_ack;
    uchar                   ts_alloc;
    PacketTimingAdvance*    pkt_timing_advance;
    uchar                   p0;
    bool                    pr_mode;
    PowerControlParameter*  power_ctl_param;
    FrequencyParameter*     freq_param;
    uchar                   downlink_tfi_assignment;
    ushort                  tbf_starting_time;
    PacketDownlinkAssignment();
    ~PacketDownlinkAssignment();

} PDA;

/**** Dummy messages ****/
/* Packet Downlink Dummy Control Block */
class PacketDownlinkDummyControlBlock {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    uchar persistence_level[4];
};

/* Packet Uplink Dummy Control Block */
class PacketUplinkDummyControlBlock {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    ulong tlli;
};

/**** TBF release messages ****/
/* Packet TBF Release */
class PacketTbfRelease {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    bool        uplink_release;
    bool        downlink_release;
    uchar       tbf_release_cause;
};

/**** Paging messages ****/
/* Packet Paging Request */

class PacketPagingRequest {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    PacketPagingRequest() {bzero(this,sizeof(PacketPagingRequest));}
    uchar persistence_level[4];
    uchar nln;
    /* repeated page info struct */
    uchar  ptmsi_or_imsi; /* 0 stands for ptmsi, 1 stands for imsi */
    ulong ptmsi;
    uchar imsi_len;
    ulong imsi;

};

/**** RLC messages ****/
/* Packet Downlink Ack/Nack */
class ChannelQualityReport {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    private:
        uchar   c_value;
        uchar   rxqual;
        uchar   sign_var;
        uchar   i_level_tn[8];
    public:
        ChannelQualityReport() {c_value=0;rxqual=0;sign_var=0;bzero(i_level_tn,8);}
};
typedef class PacketDownlinkAckNack {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    private:
    AckNack*                    ack_nack_description;
    ChannelRequestDescription*  channel_request_description;
    ChannelQualityReport*       cqr;
    uchar                       pfi;
    uchar                       rb_id;
    public:
    PacketDownlinkAckNack();
    ~PacketDownlinkAckNack();
    int set_ssn(uchar ssn)  {return ack_nack_description->set_ssn(ssn);}
    int set_fai(bool fai)   {return ack_nack_description->set_fai(fai);}
    int set_rbb(uchar* rbb) {return ack_nack_description->set_rbb(rbb);}
    AckNack* get_acknack()  {return ack_nack_description;}
    int pack(uchar* buf);
    int unpack(uchar* buf);
} PDACK;


/* Packet Uplink Ack/Nack */
class PowerControlParameters {};
typedef class PacketUplinkAckNack {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:

    uchar                   channel_coding_command;
    AckNack*                ack_nack_description;
    ulong                   contention_resolution_tlli;
    PacketTimingAdvance*    packet_timing_advance;
    PowerControlParameters* power_control_params;

    AckNack* get_acknack()  {return ack_nack_description;}
    int set_ssn(uchar ssn)  {return ack_nack_description->set_ssn(ssn);}
    int set_fai(bool fai)   {return ack_nack_description->set_fai(fai);}
    int set_rbb(uchar* rbb) {return ack_nack_description->set_rbb(rbb);}
    PacketUplinkAckNack();
    ~PacketUplinkAckNack();

}PUACK;

/**** System information messages ****/

/* PSI 1 */
class Psi1 {

    friend class NonDistributedMsg;
    friend class DistributedMsg;

    public:

    uchar                           pbcch_change_mark;
    uchar                           psi_change_field;
    uchar                           psi_repeat_period;
    uchar                           psi_count_lr;
    uchar                           psi_count_hr;
    uchar                           measurement_order;
    GprsCellOptions                 gprs_cell_options;
    PrachControlParameters          prach_control_parameters;
    PccchOrganizationParameters     pccch_organization_parameters;
    GlobalPowerCtlParam             global_power_ctl_param;
    uchar                           psi_status_ind;
    uchar                           msc_r;
    uchar                           sgsn_r;
    uchar                           band_indicator;


    Psi1() {bzero(this,sizeof(Psi1));}

};

/* PSI 2 */

class Psi2 {

    friend class NonDistributedMsg;
    friend class DistributedMsg;

    public:

    uchar                   psi2_change_mark;
    uchar                   psi2_index;
    uchar                   psi2_count;
    CellIdentification      cell_identification;
    /* Reference Freq. List sturct */
    uchar                   rfl_number;
    uchar                   length_of_rfl_contents;
    void*                   rfl_contents;

    /* Gprs mobile allocation struct */
    uchar                   ma_number;
    GprsMobileAllocation    gprs_ma_struct;

    /* Pccch description lists struct */
    uchar                   tsc;
    /* Non_hopping_pccch_carriers_struct */
    ushort                  arfcn;
    uchar                   time_slot_allocation;

    /* Additional PSI message struct */
    uchar                   psi8_broadcast;
    uchar                   psi3ter_broadcast;
    uchar                   psi3quater_broadcast;

    /**** indicators for packing function ****/

};

/**** PSI 3 ****/
/* hcs struct */
class Hcs {
    public:
    uchar   priority_class;
    uchar   hcs_thr;
};

/* SI13 PBCCH location struct */
class Si13PbcchLocation {
    public:
    uchar   si13_location;
    uchar   pbcch_location;
    uchar   psi1_repeat_period;
};

/* Serving cell parameter struct */
class ServingCellParameter {

    public:
        uchar   cell_bar_access;
        uchar   exc_acc;
        uchar   gprs_rxlevel_access_min;
        uchar   gprs_ms_txpower_max_cch;
        /* hcs struct */
        Hcs     hcs_struct1;
        uchar   multiband_reporting;
};

/* General cell select parameters */
class GeneralCellSelectionParameter {

    public:
        uchar   gprs_cell_reselect_hysteresis;
        uchar   c31_hyst;
        uchar   c32_qual;
        uchar   random_access_retry;
        uchar   t_resel;
        uchar   ra_reselect_hysteresis;
};

/* cell selection struct */
class CellSelectionStruct {

    public:
    uchar bsic;
    uchar   cell_bar_access;
    uchar   exc_acc;
    uchar   same_ra_as_serving_cell;
    uchar   gprs_rxlev_access_min;
    uchar   gprs_ms_txpwr_max_cch;
    uchar   gprs_temporary_offset;
    uchar   gprs_penalty_time;
    uchar   gprs_reselect_offset;
    /* hcs struct */
    Hcs   hcs_struct2;
    Si13PbcchLocation si13_pbcch_location;
};

/* Neighbor cell parameters struct */
class NeighborCellParameters {
    public:
    ushort                  start_frequency;
    CellSelectionStruct     cell_selection_struct;
    uchar                   nr_of_remaining_cells;
    uchar                   freq_diff_length;
    CellSelectionStruct*    freq_diff;

};

typedef class DistributedBssDescriptor {

    public:
    u_int32_t   nid;
    uchar       rai;
    uchar       bsic;
    uchar       tsc;
    uchar       bcch_no;
    long        start_ch;
    long        end_ch;

} DBssDes;


class Psi3 {

    friend class NonDistributedMsg;
    friend class DistributedMsg;

    public:

    uchar       psi3_change_mark;
    uchar       psi_bis_count;

    #ifdef __USE_SPEC_STRUCTURE__

    /* Serving cell parameter struct */
    ServingCellParameter            serving_cell_params;

    /* General cell select parameters */
    GeneralCellSelectionParameter   gen_cell_sel;

    /* Neighbor cell parameters struct */
    NeighborCellParameters          neighbor_cell_params;

    #else

    DBssDes  neighbor_cell[6];


    #endif
    DBssDes get_neighbor_cell( ulong index ) { return (index<6)?neighbor_cell[index]:neighbor_cell[5];}
    Psi3() {bzero(this, sizeof(Psi3) ); }
};

/**** PSI 3bis ****/
class Ncp2Property {
    public:
    bool  same_ra_as_serving_cell;
    bool  cell_bar_access_2;
    uchar bcc;
};

class Ncp2Repeat {
    public:
    ushort start_frequency;
    Ncp2Property  ncp2_property;
    uchar  nr_of_remaining_cells; /* 0000 is reserved and disallowed to be used */
    uchar  freq_diff_length;
    Ncp2Property *freq_diff;
};

class NeighborParameterSet {
    public:
    uchar               ncc;
    bool                exc_acc;
    uchar               gprs_rxlev_access_min;
    uchar               gprs_ms_txpwr_max_cch;
    uchar               priority_class;
    uchar               hcs_thr;
    Si13PbcchLocation   si13_pbcch_location;
    uchar               gprs_temporary_offset;
    uchar               gprs_penalty_time;
    uchar               gprs_reselect_offset;
};

class NeighborCellParameters2 {
    public:
    Ncp2Repeat                  ncp2_repeat;
    uchar                       cell_params_pointers;
    NeighborParameterSet        neighbor_parameter_set;
};

class Psi3bis {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    uchar psi3_change_mark;
    uchar psi3_bis_index;
    uchar psi3_bis_count;

    NeighborCellParameters  neighbor_cell_params;
    /* Neighbor Cell Parameter2 is used when the number of neighbor cells is high
     * and many cells share the same parameter values. The structure contains pointers
     * to the list of sets of actual parameters.
     */
};
/* PSI 3ter */
class RealtimeDifferenceDescription {
    public:
    uchar cell_index_start_rtd;
    //RTD6  rtd;
    //RTD12 rtd1;
};

class GprsRepPriorityDescription {
    public:
    uchar number_cells;
    void* rep_priority;
};

class Psi3ter {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;*/
    uchar psi3_change_mark;
    uchar psi3_ter_index;
    uchar psi3_ter_count;
    RealtimeDifferenceDescription       realtime_difference_description;
    GprsRepPriorityDescription          gprs_rep_priority_description;
};

/* PSI 3quater: is a type of message that gives information on 3G neighbor cells in addition to
 * measurement and reporting parameters.
 */

/* PSI 4 */
class ChannelGroupStruct {
    public:
    ushort  arfcn;
    uchar   ma_number;
    uchar   maio;
    uchar   timeslot_allocation;
};

class ChannelList {
    public:
    ChannelGroupStruct *channel_group;
};
class Psi4 {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;
    uchar page_mode;*/
    uchar psi4_change_mark;
    uchar psi4_index;
    uchar psi4_count;
    ChannelList channel_list;

};
/**** PSI 5 ****/
class AddFrequencyList {
    public:
    ushort  start_frequency;
    uchar   bsic;
    CellSelectionStruct cell_selection_params;
    uchar   nr_of_frequencies;
    uchar   freq_diff_length;
    void    *frequency_diff;


};

class NcFrequencyList {
    public:
    uchar nr_of_removed_freq;
    void  *removed_freq_index;
    AddFrequencyList *list_of_added_frequency;

};

class NcMeasurementParameters {
    public:
    uchar               network_control_order;
    uchar               nc_non_drx_period;
    uchar               nc_reporting_period_h;
    uchar               nc_reporting_period_t;
    NcFrequencyList     nc_frequency_list;

};


class Psi5 {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;*/
    uchar psi5_change_mark;
    uchar psi5_index;
    uchar psi5_count;

    //NcmeasurementParameters   nc_measurement_params;

};
/* PSI 6 message is used by network to broadcast information required by
 * non-GPRS network.
 */

/* PSI 7 message is used by network to broadcast information required by
 * non-GPRS network.
 */

/**** PSI 8 ****/
class CbchChannelDescription {
   public:
   uchar channel_type_and_TDMA_offset;
   uchar tn;
   FrequencyParameter* freq_param;
};

class DynamicArfcnMapping {
   public:
   uchar  gsm_band;
   ushort arfcn_first;
   ushort band_offset;
   uchar  arfcn_range;
};
class DynamicArfcnMappingDescription {
    public:
    uchar dm_change_mark;
    DynamicArfcnMapping dynamic_arfcn_mapping;
};

class Psi8 {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;*/
    uchar                           page_mode;
    uchar                           psi8_change_mark;
    uchar                           psi8_index;
    uchar                           psi8_count;
    CbchChannelDescription          cbch_channel_description;
    DynamicArfcnMappingDescription  dynamic_arfcn_mapping_description;
};
/* PSI 13 */
class PbcchDescription {
    public:
    uchar   pb;
    uchar   tsc;
    uchar   tn;
    ushort  arfcn;
    uchar   maio;
};

class Psi13 {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;
    uchar page_mode;*/
    uchar bcch_change_mark;
    uchar si_changed_field;
    uchar si_changed_mark;
    GprsMobileAllocation            gprs_mobile_allocation;
    uchar rac;
    uchar spgc_ccch_sup;
    uchar priority_access_thr;
    uchar network_control_order;
    GprsCellOptions                 gprs_cell_options;
    //GprsPowerControlParameter     gprs_power_control_param;
    uchar psi1_repeat_period;
    PbcchDescription                pbcch_description;
    bool  sgsnr;
    bool  si_status_indicator;

};

/* PSI 14 */

class CcchAccessInformation {
    public:
    uchar bcch_change_mark;
    uchar si13_change_mark;
    GprsMobileAllocation gprs_mobile_allocation;
    bool  spgc_ccch_sup;
    uchar priority_access_thr;
    uchar network_control_order;
    GprsCellOptions      gprs_cell_options;
    //GprsPowerControlParameter gprs_power_control_param;
    bool  sgsnr;
};
class Psi14 {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;
    uchar page_mode;*/
    CcchAccessInformation       ccch_access_information;
    PbcchDescription            pbcch_description;
};

/* PSI 15 provides the information for UTRAN(3G) system. It contains frequencies used in
 * UTRAN network. A MS which doens't have UTRAN capability shall ignore this message.
 */

/* PSI 16 provides information for Iu mode. Again, in our GPRS system, this message
 * can be ignored.
 */


/**** Misaneous messages ****/
/* Packet Control Acknowledgement *
 * Packet Control Ack. has three types of formats. The first is
 * RLC/MAC control block format, the second format is 11-bit access burst
 * type, and the third is 8-bit access burst. The last two types have
 * additional fields indicating message type
 */
typedef class PacketControlAcknowledgement {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    PacketControlAcknowledgement() {bzero(this,sizeof(PacketControlAcknowledgement));}
    ulong tlli;
    uchar ctlack;
} PktCtlAck;

/* Packet Cell Change Continue */

class PacketCellChangeContinue {
    /*uchar     message_type;
    uchar       page_mode;
    GlobalTfi  global_tfi;*/
    ushort      arfcn;
    uchar       bsic;
    uchar       container_id;

};

/* Packet Cell Change Failure */

class PacketCellChangeFailure {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar  message_type;*/
    ushort arfcn;
    uchar  bsic;
    uchar  cause;
};

/* Packet Cell Change Notification */
class PacketCellChangeNotification {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar     message_type;
    GlobalTfi   global_tfi;*/
    ushort      arfcn;
    uchar       bsic;
    uchar       ba_used;
    uchar       psi3_change_mark;
    uchar       pmo_used;
    uchar       ccn_measurement_report_struct;
    uchar       pccn_sending;
};

/* Packet Cell Change Order */
class GsmTargetCellStruct {
    public:
    ushort arfcn;
    uchar  bsic;

};

class CcnSupportDescription {
    public:
    uchar number_cells;
    void* ccn_supported;
};

class LsaIdInfo;
class LsaParameters;
class PacketCellChangeOrder {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    uchar                       immediate_rel;
    GsmTargetCellStruct         gsm_target_cell;
    LsaParameters*              lsa_params;
    bool                        ccn_active;
    uchar                       container_id;
    CcnSupportDescription       ccn_support_description;


};

/* Packet Measurement Report */
class PacketMeasurementReport {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar message_type;
    ulong tlli;*/
    uchar psi5_change_mark;
    uchar ba_used;
    uchar psi3_change_mark;
    uchar pmo_used;
    uchar nc_mode;
    uchar rx_level_serving_cell;
    uchar interference_level_serving_cell;
    uchar i_level_tn[8];
    uchar frequency_n;
    //Gsm_neighbor_cell_list nc_measurement;
    uchar bsic_n;
    uchar rxlev_n;


};

/* Packet Measurement Order */
class PacketMeasurementOrder {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar                     message_type;
    uchar                       page_mode;
    GlobalTfi                   global_tfi;
    ulong                       tlli;*/
    uchar                       pmo_index;
    uchar                       pmo_count;
    NcMeasurementParameters     nc_measurement_params;
    /* ExtMeasurementParameters are omitted */
    LsaParameters*              lsa_params;
    bool                        ccn_active;
    CcnSupportDescription       ccn_support_description;
};

/* Packet Mobile TBF Status */
class PacketMobileTbfStatus {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar     message_type;
    GlobalTfi   global_tfi;*/
    uchar       tbf_cause;
    uchar       status_message_type;

};

/* Packet Neighbor Cell Data (optional)*/

/* Packet PDCH Release */
class PacketPdchRelease {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    private:
        uchar timeslots_available;
    public:
        PacketPdchRelease() {bzero(this,sizeof(PacketPdchRelease));}
};

/* Packet Polling Request */

class PacketPollingRequest {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar     message_type;
    uchar       page_mode;
    ushort      tqi;
    ulong       tlli;
    GlobalTfi   global_tfi;*/
    uchar       type_of_ack;
    uchar       grnti_extension;

};

/* Packet Power Control/Timing Advance */
typedef class PacketPowerControlTimingAdvance {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar                     message_type;
    GlobalTfi                   global_tfi;*/
    GlobalPowerControlParameter global_powercontrol_param;
    GlobalPacketTimingAdvance   global_pkt_timing_advance;
    //PacketControlParameter    packet_control_parameter;
    uchar                       packet_extended_timing_advance;
} PPCTA ;

/* Packet PRACH Parameters */
class PacketPrachParameter {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    /*uchar                     message_type;
    uchar                       page_mode;*/
    PrachControlParameter       prach_control_param;

};

/* Packet PSI Status */

class PsiMessageListStruct {
    public:
    /*uchar message_type;*/
    uchar psix_change_mark;
    uchar psix_count;
    uchar *instance_bitmap;
    uchar additional_msg_type;
};
class PacketPsiStatus {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    GlobalTfi                   global_tfi;
    uchar                       pbcch_change_mark;
    /* psi message list */
    PsiMessageListStruct        psi_message_list;

};

/* Packet Serving Cell Data */
class PacketServingCellData {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar message_type;
    uchar page_mode;
    uchar tfi;*/
    uchar pd;
    uchar cd_length;
    uchar *container_data;
};

/* Packet SI Status */
class PacketSiStatus {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    GlobalTfi   global_tfi;
    uchar       bcch_change_mark;
    /* leave for fulfiling */

};

/* Packet Pause */
class PacketPause {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar  message_type;
    ushort tlli;*/
    char   rai[6];

};

/* Packet Timeslot Reconfigure */
typedef class DynamicAllocation {
    friend class    NonDistributedMsg;
    friend class    DistributedMsg;
    friend class    GprsMsMac;
    friend class    GprsBtsMac;
    private:
    bool    extended_dynamic_allocation_indicator;
    uchar   p0;
    bool    pr_mode;
    bool    usf_granularity; /* if 0 , the MS shall transmit one RLC/MAC block, otherwise,
                             * the MS should transmit four consecutive RLC/MAC blocks.
                             */
    uchar   uplink_tfi_assignment;
    uchar   rlc_data_blocks_granted;
    ushort  tbf_starting_time;
    uchar   usf_tn[8];       /* USF for each time slot assigned to a MS is indicated here */
    uchar   alpha;
    uchar   gamma_tn[8];

    /**********************************/
    bool    power_ctl_param_presence;
    bool    uplink_tfi_assignment_presence;
    bool    rlc_data_blks_granted_presence;
    bool    tbf_starting_time_presence;
    bool    ta_or_ta_pow;
    bool    tn_presence[8];

    public:
    DynamicAllocation() {bzero(this,sizeof(DynamicAllocation));for (int i=0;i<8;++i) usf_tn[i]=gprs::usf_unused;}

} DynAlloc;

class PacketTimeslotReconfigure {
    friend class NonDistributedMsg;
    friend class DistributedMsg;
    public:
    /*uchar                     message_type;
    GlobalTfi                   global_tfi;*/
    /* normal gprs fields */
    uchar                       channel_coding_command;
    GlobalPacketTimingAdvance*  global_pkt_timing_advance;
    bool                        downlink_rlc_mode;
    bool                        control_ack;
    uchar                       downlink_tfi_assignment;
    uchar                       uplink_tfi_assignment;

    uchar                       downlink_timeslot_allocation;
    FrequencyParameter          freq_param;
    DynamicAllocation           dynamic_allocation;
    uchar                       packet_extended_timing_advance;
    uchar                       rb_ld_of_downlink_tbf;
    uchar                       rb_ld_of_uplink_tbf;
    uchar                       uplink_control_timeslot;
};

/**** 12.28 LSA parameters ****/
class LsaIdInfo {
    public:
    ulong lsa_id;
    ushort short_lsa_id;
};

class LsaParameters {
    public:
    uchar       nr_of_freq_or_cells;
    LsaIdInfo   *lsa_id_info;
};


class AdditionalMsRadioAccessCapabilities {
    public:
    /*uchar message_type;
    ulong tlli;*/
    MsRadioAccessCapability2 ms_rac2;
};

/*********** End of control message definition ***********/


/**********************************************************/
/* The shared message class: Adopt reference count mechanism
 * to avoid copies of complex data structures in control messages.
 */



 typedef class SharedMsgElem   {

    protected:
        u_int64_t   ts;
        ObjType     obj_type;
        ulong       obj_subtype;
        void*       obj_ptr;


    public:
        SharedMsgElem( u_int64_t ts1, ObjType obj_type1, ulong obj_subtype1, void* obj_ptr1 );

        u_int64_t   get_ts()            {return ts;}
        ObjType     get_obj_type()      {return obj_type;}
        ulong       get_obj_subtype()   {return obj_subtype;}
        void*       get_obj_ptr()       {return obj_ptr;}
        int         update_subtype( ulong subtype );
        int         update_obj_ptr( void* obj_ptr1 );
        int         release();

 } SMsgElem;

/**********************************************************/

class timerObj;
typedef class SharedMsgList : public SList<SharedMsgElem> {
    private:
        ulong       release_period_;
        ulong       expiry_time_;
        timerObj*   release_timer;

        ulong       dmsg_cnt;
        ulong       ndmsg_cnt;
        ulong       d_burst_cnt;
        ulong       n_burst_cnt;

    public:
        SharedMsgList( ulong release_period , ulong expiry_time );
        int    activate_timer();
        uchar  is_rtimer_activated() { return (release_timer)?true:false; }
        int    release();
        int    inc_dmsg_cnt();
        int    inc_ndmsg_cnt();
        int    inc_d_burst_cnt();
        int    inc_n_burst_cnt();
        int    dec_dmsg_cnt();
        int    dec_ndmsg_cnt();
        int    dec_d_burst_cnt();
        int    dec_n_burst_cnt();

        /* get_nid() function is mandortory for an object that does not belong to
         * any node. Returning zero is used to notify S.E. that this object is a global
         * object.
         */

        int get_nid() { return 0; }


} SMsgList;

int shared_obj_insert       (SharedMsgElem* elem);
int shared_obj_remove_entry (SharedMsgElem* elem);
int shared_obj_inc_refcnt   ( ObjType msg_type, void* msg);
int shared_obj_dec_refcnt   ( ObjType msg_type, void* msg);
int shared_obj_release      ( ObjType msg_type, void* msg);


/**********************************************************/
class AddressInfo;
class DistributedContent;


enum Direction {

    downlink = 1,
    uplink

};


/* distributed message */

class DistributedContent {

    friend class NonDistributedMsg;
    friend class DistributedMsg;

    protected:
        uchar page_mode;
        void* msg;
    public:

         DistributedContent(uchar pg_mode) {page_mode = pg_mode;msg=NULL;}
         ~DistributedContent();
        int     free_msg(uchar message_type, Direction direction );
        int     set_page_mode(uchar pg_mode)  {page_mode = pg_mode; return 1;}
        int     set_msg(void* msg1) {msg=msg1;return 1;}
};



typedef class DistributedMsg {
    protected:
        ulong                   ref_cnt;
        SharedMsgElem*          rec_entry_ptr;
        uchar                   message_type;
        Direction               direction;
        DistributedContent*     distr_content;
    public:
        DistributedMsg();
        virtual ~DistributedMsg();
        int     set_pgmode(uchar pg_mode);
        //int     free_dmsg();
        uchar   get_pgmode() {return (distr_content)?distr_content->page_mode:0;}
        uchar   get_msgtype() {return message_type;}
        uchar*  pack();
        int     unpack(uchar* ptr);
        int     set_distr_msg(uchar msg_type, Direction dir, void* msg);
        void*   get_distr_msg();

        int             inc_refcnt()        {++ref_cnt;return 1;}
        int             dec_refcnt()        { if (ref_cnt>0) --ref_cnt;return 1;}
        ulong           get_refcnt()        {return ref_cnt;}
        SharedMsgElem*  get_rec_entry()     {return rec_entry_ptr;}

} DistrMsg;


/* non-distributed message */
#define GLOBAL_TFI_INDICATOR    1
#define TLLI_INDICATOR          2
#define TQI_INDICATOR           3
#define PKT_REQ_REF_INDICATOR   4
#define UNUSED                  100

class AddressInfo {
        friend class NonDistributedMsg;
    private:
        uchar           field_used;
        GlobalTfi       global_tfi_ie;
        ulong           tlli;
        ushort          tqi;
        PacketRequestReference prr_ie;
    public:
        AddressInfo(uchar field,GlobalTfi tfi_ie) {
            if (field==GLOBAL_TFI_INDICATOR) {
                global_tfi_ie.direction = tfi_ie.direction;
                global_tfi_ie.tfi           = tfi_ie.tfi;
                field_used              = field;
            }
            else
                field_used = UNUSED;
        }
        AddressInfo(uchar field,ushort value) {
            if (field == TQI_INDICATOR) {
                tlli                    = value;
                field_used              = field;
            }
            else
                field_used = UNUSED;
        }
        AddressInfo(uchar field,ulong value) {
            if (field == TLLI_INDICATOR) {
                tlli                    = value;
                field_used              = field;
            }
            else
                field_used = UNUSED;
        }
        AddressInfo(uchar field, PacketRequestReference req) {
            if (field==PKT_REQ_REF_INDICATOR) {
                prr_ie.ra_info          = req.ra_info;
                prr_ie.fn                   = req.fn;
                field_used              = field;
            }
            else
                field_used = UNUSED;
        }

};



typedef class NonDistributedMsg {
    protected:
        ulong                   ref_cnt;
        SharedMsgElem*          rec_entry_ptr;
        uchar                   message_type;
        Direction               direction;
        DistributedContent*     distr_content;
        AddressInfo*            addr_info;
        void*                   nondistr_msg;
    public:
        NonDistributedMsg();
        virtual ~NonDistributedMsg();
        int     free_ndmsg();
        int     set_pgmode(uchar pg_mode);
        uchar   get_pgmode()  {return (distr_content)?distr_content->page_mode:0;}
        uchar   get_msgtype() {return message_type;}
        int     set_distr_msg(void* msg);
        uchar   get_addr_info_type() {return addr_info->field_used;}
        int     set_addr_info(uchar field, GlobalTfi tfi_ie);
        int     set_addr_info(uchar field, ushort value);
        int     set_addr_info(uchar field, ulong value);
        int     set_addr_info(uchar field, PacketRequestReference req);
        int     get_addr_info(GlobalTfi*);
        int     get_addr_info(ulong*);
        int     get_addr_info(ushort*);
        int     get_addr_info(PacketRequestReference*);
        int     pack(uchar* ptr);
        int     unpack(uchar* ptr);
        int     set_nondistr_msg(uchar msg_type, Direction dir ,void* msg1);
        void*   get_nondistr_msg();

        int             inc_refcnt()        { ++ref_cnt;return 1;}
        int             dec_refcnt()        { if (ref_cnt>0) --ref_cnt;return 1;}
        ulong           get_refcnt()        {return ref_cnt;}
        SharedMsgElem*  get_rec_entry()     {return rec_entry_ptr;}

} NDistrMsg;

/******************************************/


#define INC_REFCNT(ptr) do {    \
   ptr->inc_refcnt();           \
} while(0)

#define DEC_REFCNT(ptr) do {        \
   ptr->dec_refcnt();               \
   if ( ptr->get_refcnt() == 0 ) {  \
        SharedMsgElem* entry = ptr->get_rec_entry();    \
        ASSERTION(entry, "Assertion failed: SharedMsgElem is not found.\n");    \
        shared_msg_list->remove_entry(entry);   \
        delete entry;                           \
        delete ptr;                             \
   }                                            \
   ptr = NULL;                                  \
} while(0)


#endif
