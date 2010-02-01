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

#ifndef	__NCTUNS_mac_802_11e_management_framebody_h__
#define __NCTUNS_mac_802_11e_management_framebody_h__

#include <stdlib.h>
#include <stdbool.h>


/* 
   2007/11/03 By C.L. Chou
   Information contained in 80211e frame body:
 
   <Fixed Length>
    -- Timestamp (8 octets)
    -- Beacon Interval (2 octets)
    -- Capability Information (2 octets)
    -- Reason Code (2 octets)
    -- Listen Interval (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Current AP address (6 octets)
    -- Authentication Algorithm Number (2 octets)
    -- Authentication Transaction Sequence Number (2 octets)
    -- FH Parameter Set (7 octets)
    -- DS Parameter Set (3 octets)
    -- CF Parameter Set (8 octets)
    -- IBSS Parameter Set (4 octets)
    -- Dialog Token (1 octet)
    -- DLS Timeout Value (2 octet)
    -- Block Ack Parameter Set (2 octets)
    -- Block Ack Timeout Value (2 octets)
    -- Block Ack Starting Sequence Control (2 octets)
    -- DELBA Parameter Set (2 octets)
    -- QoS Info (1 octets)
    -- QBSS Load (7 octets)
    -- EDCA Parameter Set (20 octets)
    -- TS Info (3 octets)
    -- Nominal MSDU Size (2 octets)
    -- TSPEC (57 octets) 
    -- Schedule (14 octets)
    -- TS Delay (6 octets)
    -- TCLAS Processing (3 octets)
    -- QoS Capability (3 octets)
    -- Action (1_Category + 1_Action + ... different in each action-subtype management frame body)	
    
   <Variable Length>
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- TIM (1_ElementID + 1_Length + 1_DTIM_Count + 1_DTIM_Period + 1_Bitmap_Control + 1~251_Partial_Virtual_Bitmap)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text)
    -- TCLAS (1_ElementID + 1_Length + 1_User_Priority + 0/3~254_Frame_Classifier) 
    -- Extended Supported Rates ((1_ElementID + 1_Length + 1~255_Supported_Rates)
*/
#define WITH_CHALLENGE_TEXT_FLAG	0x01

#define WITH_FH_FLAG			0x01
#define WITH_DS_FLAG			0x02
#define WITH_CF_FLAG			0x04
#define WITH_IBSS_FLAG			0x08
#define WITH_TIM_FLAG			0x10
#define WITH_QBSS_LOAD_FLAG		0x20
#define	WITH_EDCA_PARAMETER_SET_FLAG	0x40
#define WITH_QOS_CAPABILITY_FLAG	0x80

#define WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG	0x01
#define WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG	0x02
#define WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG	0x04
#define WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG	0x08
#define WITH_TCLAS_PROCESSING_FLAG		0x10

enum TCLAS_FrameClassifierType {NoType, ClassifierType_0, ClassifierType_1_IPv4, ClassifierType_1_IPv6, ClassifierType_2};


#define ELEMENT_ID_SSID				0x00
#define ELEMENT_ID_SUPPORTED_RATES 		0x01
#define ELEMENT_ID_FH_PARAMETER_SET 		0x02
#define ELEMENT_ID_DS_PARAMETER_SET 		0x03
#define ELEMENT_ID_CF_PARAMETER_SET 		0x04
#define ELEMENT_ID_TIM				0x05
#define ELEMENT_ID_IBSS_PARAMETER_SET		0x06
#define ELEMENT_ID_CHALLENGE_TEXT		0x10
#define ELEMENT_ID_QBSS_LOAD			0x0b
#define ELEMENT_ID_EDCA_PARAMETER_SET		0x0c
#define ELEMENT_ID_TSPEC			0x0d
#define ELEMENT_ID_TCLAS			0x0e
#define ELEMENT_ID_SCHEDULE			0x0f
#define ELEMENT_ID_TS_DELAY			0x2b
#define ELEMENT_ID_TCLAS_PROCESSING		0x2c
#define ELEMENT_ID_QOS_CAPABILITY		0x2e
#define ELEMENT_ID_EXTENDED_SUPPORTED_RATES	0x32
/*
   2007/11/03 By C.L. Chou
   Action Field:
    -- Category (1 octet)
    -- Action (1 octet)
    -- ... ... (different in each action-subtype management frame body)
*/
#define ACTION_FRAME_CATEGORY_QOS		0X01
#define ACTION_FRAME_ACTION_ADDTS_REQUEST	0x00
#define ACTION_FRAME_ACTION_ADDTS_RESPONSE	0x01
#define ACTION_FRAME_ACTION_DELTS		0x02
#define ACTION_FRAME_ACTION_SCHEDULE		0x03

#define ACTION_FRAME_CATEGORY_DLS		0X02
#define ACTION_FRAME_ACTION_DLS_REQUEST		0x00
#define ACTION_FRAME_ACTION_DLS_RESPONSE	0x01
#define ACTION_FRAME_ACTION_DLS_TEARDOW		0x02

#define ACTION_FRAME_CATEGORY_BLOCK_ACK		0X03
#define ACTION_FRAME_ACTION_ADDBA_REQUEST	0x00
#define ACTION_FRAME_ACTION_ADDBA_RESPONSE	0x01
#define ACTION_FRAME_ACTION_DELBA		0x02


extern void Initialize80211eFrameBodyWithZero(void *body, int len);

void SetCategoryInto80211eActionFrameBody(char *Body, u_char Category);
void SetActionInto80211eActionFrameBody(char *Body, u_char Action);
void GetCategoryFrom80211eActionFrameBody(char *Body, u_char *Category);
void GetActionFrom80211eActionFrameBody(char *Body, u_char *Action);


/* 
   2007/11/03 By C.L. Chou 
   Capability Information Field:
    -- ESS (1 bit)
    -- IBSS (1 bit)
    -- CF Pollable (1 bit)
    -- CF Poll Request (1 bit)
    -- Privacy (1 bit)
    -- Short Preamble (1 bit)
    -- PBCC (1 bit)
    -- Channel Agility (1 bit)
    -- Spectrum Management (1 bit)
    -- QoS (1 bit)
    -- Short Slot Time (1 bit)
    -- APSD (1 bit)
    -- Reserved (1 bit)
    -- DSSS-OFDM (1 bit)
    -- Delayed Block Ack (1 bit)
    -- Immediate Block Ack (1 bit)
*/
bool GetESSFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetIBSSFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetCFPollableFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetCFPollRequestFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetPrivacyFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetShortPreambleFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetPBCCFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetChannelAgilityFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetSpectrumManagementFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetQoSFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetShortSlotTimeFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetAPSDFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetDSSSOFDMFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetDelayedBlockAckFrom80211eCapabilityInfo(char *CapabilityInfo);
bool GetImmediateBlockAckFrom80211eCapabilityInfo(char *CapabilityInfo);

/* 
   2007/11/03 By C.L. Chou 
   FH Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Dwell Time (2 octets)
    -- Hop Set (1 octet)
    -- Hop Pattern (1 octet)
    -- Hop Index (1 octet)
*/
bool GetDwellTimeFrom80211eFHParameterSet(char *FHParameterSet, unsigned short *DwellTime);
bool GetHopSetFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopSet);
bool GetHopPatternFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopPattern);
bool GetHopIndexFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopIndex);


/* 
   2007/11/03 By C.L. Chou 
   DS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Current Channel (1 octet)
*/
bool GetCurrentChannelFrom80211eDSParameterSet(char *DSParameterSet, u_char *CurrentChannel);

/* 
   2007/11/03 By C.L. Chou 
   CF Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- CFP Count (1 octet)
    -- CFP Period (1 octet)
    -- CFP MaxDuration (2 octets)
    -- CFP DurRemaining (2 octets)
*/
bool GetCountFrom80211eCFParameterSet(char *CFParameterSet, u_char *Count);
bool GetPeriodFrom80211eCFParameterSet(char *CFParameterSet, u_char *Period);
bool GetMaxDurationFrom80211eCFParameterSet(char *CFParameterSet, unsigned short *MaxDuration);
bool GetDurRemainingFrom80211eCFParameterSet(char *CFParameterSet, unsigned short *DurRemaining);

/* 
   2007/11/03 By C.L. Chou 
   IBSS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- ATIM Window(2 octets)
*/
bool GetATIMWindowFrom80211eIBSSParameterSet(char *IBSSParameterSet, unsigned short *ATIM_Window);

/* 
   2007/11/03 By C.L. Chou 
   TIM Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- DTIM Count (1 octet)
    -- DTIM Period (1 octet)
    -- Bitmap Control (1 octet)
    -- Partial Virtual Bitmap (1~251 octets)
*/
bool GetCountFrom80211eTIM(char *TIM, u_char *Count);
bool GetPeriodFrom80211eTIM(char *TIM, u_char *Period);
bool GetBitmapCtrlFrom80211eTIM(char *TIM, u_char *Bitmap_Ctrl);
void *GetPartialVirtualBitmapFrom80211eTIM(char *TIM, unsigned int *PartialVirtualBitmap_Len);

/*
   2007/11/03 By C.L. Chou
   Challenge Text Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Challenge Text (1~253 octets)
*/
void *GetChallengeTextFrom80211eChallengeText(char *ChallengeText, unsigned int *ChallengeText_Len);

/*
   2007/11/03 By C.L. Chou
   QBSS Load Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Station Count (2 octet)
    -- Channel Utilization (1 octet)
    -- Available Admission Capacity (2 octets)
*/
bool GetStationCountFromQBSSLoad(char *QBSSLoad, unsigned short *StationCount);
bool GetChannelUtilizationFromQBSSLoad(char *QBSSLoad, u_char *ChannelUtilization);
bool GetAvailableAdmissionCapacityFromQBSSLoad(char *QBSSLoad, unsigned short *AvailableAdmissionCapacity);

/*
   2007/11/03 By C.L. Chou
   QoS Capability Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- QoS Info (1 octet)
*/
char *GetQoSInfoFromQoSCapability(char *QoSCapability);

/*
   2007/11/03 By C.L. Chou
   QoS Info Field:
    -- EDCA Parameter Set Update Count (4 bits)
    -- Q-Ack (1 bit)
    -- Queue Request (1 bit)
    -- TXOP Request (1 bit)
    -- Reserved (1 bit)    
*/
bool GetEDCAParameterSetUpdateCountFromQoSInfo(char *QoSInfo, u_char *EDCAParameterSetUpdateCount);
bool GetQAckFromQoSInfo(char *QoSInfo);
bool GetQueueRequestFromQoSInfo(char *QoSInfo);
bool GetTXOPRequestFromQoSInfo(char *QoSInfo);

/*
   2007/11/03 By C.L. Chou
   EDCA Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- QoS Info (1 octet)
    -- Reserved (1 octet)
    -- AC_BE Parameter Record (4 octets)
    -- AC_BK Parameter Record (4 octets)
    -- AC_VI Parameter Record (4 octets)
    -- AC_VO Parameter Record (4 octets)
*/
char *GetQoSInfoFromEDCAParameterSet(char *EDCAParameterSet);
char *GetACBEParameterRecordFromEDCAParameterSet(char *EDCAParameterSet);
char *GetACBKParameterRecordFromEDCAParameterSet(char *EDCAParameterSet);
char *GetACVIParameterRecordFromEDCAParameterSet(char *EDCAParameterSet);
char *GetACVOParameterRecordFromEDCAParameterSet(char *EDCAParameterSet);

/*
   2007/11/03 By C.L. Chou
   AC_XX Parameter Record Field:
    -- AIFSN (4 bits)
    -- ACM (1 bit)
    -- ACI (2 bit)
    -- Reserved (1 bit)
    -- ECW (1 octet)
    -- TXOP Limit (2 octets)    
*/
bool GetAIFSNFromACXXParameterRecord(char *ACXXParameterRecord, u_char *AIFSN);
bool GetACMFromACXXParameterRecord(char *ACXXParameterRecord);
bool GetECWFromACXXParameterRecord(char *ACXXParameterRecord, u_char *ECW);
bool GetTXOPLimitFromACXXParameterRecord(char *ACXXParameterRecord, unsigned short *TXOP_Limit);

/*
   2007/11/03 By C.L. Chou
   TSPEC Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- TS Info (3 octets)
    -- Nominal MSDU Size (2 octets, 15_bits_Size + 1_bit_Fixed)
    -- Maximum MSDU Size (2 octets)
    -- Minimum Service Interval (4 octets)
    -- Maximum Service Interval (4 octets)
    -- Inactivity Interval (4 octets)
    -- Suspension Interval (4 octets)
    -- Service Start Time (4 octets)
    -- Minimum Data Rate (4 octets)
    -- Mean Data Rate (4 octets)
    -- Peak Data Rate (4 octets)
    -- Burst Size (4 octets)
    -- Delay Bound (4 octets)
    -- Minimum PHY Rate (4 octets)
    -- Surplus Bandwidth Allowance (2 octets, 3_bits_Integer_Part + 13_bits_Decimal_Part)
    -- Medium Time (2 octets)
*/
char *GetTSInfoFromTSPEC(char *TSPEC);
bool GetNominalMSDUSizeFromTSPEC(char *TSPEC, unsigned short *NominalMSDUSize);
bool GetNominalMSDUSizeFixedFromTSPEC(char *TSPEC);
bool GetMaximumMSDUSizeFromTSPEC(char *TSPEC, unsigned short *MaximumMSDUSize);
bool GetMinimumServiceIntervalFromTSPEC(char *TSPEC, unsigned int *MinimumServiceInterval);
bool GetMaximumServiceIntervalFromTSPEC(char *TSPEC, unsigned int *MaximumServiceInterval);
bool GetInactivityIntervalFromTSPEC(char *TSPEC, unsigned int *InactivityInterval);
bool GetSyspensionIntervalFromTSPEC(char *TSPEC, unsigned int *SyspensionInterval);
bool GetServiceStartTimeFromTSPEC(char *TSPEC, unsigned int *ServiceStartTime);
bool GetMinimumDataRateFromTSPEC(char *TSPEC, unsigned int *MinimumDataRate);
bool GetMeanDataRateFromTSPEC(char *TSPEC, unsigned int *MeanDataRate);
bool GetPeakDataRateFromTSPEC(char *TSPEC, unsigned int *PeakDataRate);
bool GetBurstSizeFromTSPEC(char *TSPEC, unsigned int *BurstSize);
bool GetDelayBoundFromTSPEC(char *TSPEC, unsigned int *DelayBound);
bool GetMinimumPHYRateFromTSPEC(char *TSPEC, unsigned int *MinimumPHYRate);
bool GetSurplusBandwidthAllowanceIntegerPartFromTSPEC(char *TSPEC, unsigned short *SurplusBandwidthAllowance_IntegerPart);
bool GetSurplusBandwidthAllowanceDecimalPartFromTSPEC(char *TSPEC, unsigned short *SurplusBandwidthAllowance_DecimalPart);
bool GetMediumTimeFromTSPEC(char *TSPEC, unsigned short *MediumTime);

/*
   2007/11/03 By C.L. Chou
   TS Info Field:
    -- Traffic Type (1 bit)
    -- TSID (4 bits)
    -- Direction (2 bits)
    -- Access Policy (2 bits)
    -- Aggregation (1 bit)
    -- APSD (1 bit)
    -- User Priority (3 bits)
    -- TSInfo Ack Policy (2 bits)
    -- Schedule (1 bit)
    -- Reserved (7 bits)
*/
bool GetTrafficTypeFromTSInfo(char *TSInfo);
bool GetTSIDFromTSInfo(char *TSInfo, u_char *TSID);
bool GetDirectionBit1FromTSInfo(char *TSInfo);
bool GetDirectionBit2FromTSInfo(char *TSInfo);
bool GetAccessPolicyBit1FromTSInfo(char *TSInfo);
bool GetAccessPolicyBit2FromTSInfo(char *TSInfo);
bool GetAggregationFromTSInfo(char *TSInfo);
bool GetAPSDFromTSInfo(char *TSInfo);
bool GetUserPriorityFromTSInfo(char *TSInfo, u_char *UserPriority);
bool GetTSInfoAckPolicyBit1FromTSInfo(char *TSInfo);
bool GetTSInfoAckPolicyBit2FromTSInfo(char *TSInfo);
bool GetScheduleFromTSInfo(char *TSInfo);

/*
   2007/11/03 By C.L. Chou
   TCLAS with Frame Classifier Type 0 Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- User Priority (1 octet)
    -- Classifier Type (1 octet)
    -- Classifier Mask (1 octet)
    -- Source Address (6 octets)
    -- Destination Address (6 octets)
    -- Type (2 octets)
*/
bool GetUserPriorityFromTCLASwithFrameClassifierType0(char *TCLAS, u_char *UserPriority);
bool GetClassifierMaskFromTCLASwithFrameClassifierType0(char *TCLAS, u_char *ClassifierMask);
bool GetSourceAddressFromTCLASwithFrameClassifierType0(char *TCLAS, void *SourceAddress);
bool GetDestinationAddressFromTCLASwithFrameClassifierType0(char *TCLAS, void *DestinationAddress);
bool GetTypeFromTCLASwithFrameClassifierType0(char *TCLAS, unsigned short *Type);

/*
   2007/11/03 By C.L. Chou
   TCLAS with Frame Classifier Type 1 for IPv4 Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- User Priority (1 octet)
    -- Classifier Type (1 octet)
    -- Classifier Mask (1 octet)
    -- Version (1 octet)
    -- Source IP Address (4 octets)
    -- Destination IP Address (4 octets)
    -- Source Port (2 octets)
    -- Destination Port (2 octets)
    -- DSCP (1 octet)
    -- Protocol (1 octet)
    -- Reserved (1 octet)
*/
bool GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *UserPriority);
bool GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *ClassifierMask);
bool GetVersionFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *Version);
bool GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, void *SourceIPAddress);
bool GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, void *DestinationIPAddress);
bool GetSourcePortFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, unsigned short *SourcePort);
bool GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, unsigned short *DestinationPort);
bool GetDSCPFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *DSCP);
bool GetProtocolFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *Protocol);

/*
   2007/11/03 By C.L. Chou
   TCLAS with Frame Classifier Type 1 for IPv6 Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- User Priority (1 octet)
    -- Classifier Type (1 octet)
    -- Classifier Mask (1 octet)
    -- Version (1 octet)
    -- Source IP Address (16 octets)
    -- Destination IP Address (16 octets)
    -- Source Port (2 octets)
    -- Destination Port (2 octets)
    -- Flow Label (3 octets)
*/
bool GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *UserPriority);
bool GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *ClassifierMask);
bool GetVersionFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *Version);
bool GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *SourceIPAddress);
bool GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *DestinationIPAddress);
bool GetSourcePortFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, unsigned short *SourcePort);
bool GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, unsigned short *DestinationPort);
bool GetFlowLabelFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *FlowLabel);

/*
   2007/11/03 By C.L. Chou
   TCLAS with Frame Classifier Type 2 Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- User Priority (1 octet)
    -- Classifier Type (1 octet)
    -- Classifier Mask (1 octet)
    -- 802.11Q Tag Type (2 octets)
*/
bool GetUserPriorityFromTCLASwithFrameClassifierType2(char *TCLAS, u_char *UserPriority);
bool GetClassifierMaskFromTCLASwithFrameClassifierType2(char *TCLAS, u_char *ClassifierMask);
bool Get80211QTagTypeFromTCLASwithFrameClassifierType2(char *TCLAS, unsigned short *TagType);

/*
   2007/11/03 By C.L. Chou
   TCLAS Processing Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Processing (1 octet)
*/
bool GetProcessingFromTCLASProcessing(char *TCLASProcessing, u_char *Processing);

/*
   2007/11/03 By C.L. Chou
   TS Delay Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Delay (1 octet)
*/
bool GetDelayFromTSDelay(char *TSDelay, unsigned int *Delay);

/*
   2007/11/03 By C.L. Chou
   Schedule Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Schedule Info: Aggregation (1 bit)
    -- Schedule Info: TSID (4 bits)
    -- Schedule Info: Direction (2 bits)
    -- Schedule Info: Reserved (9 bits)
    -- Service Start Time (4 octets)
    -- Service Interval (4 octets)
    -- Specification Interval (2 octets)
*/
bool GetAggregationFromSchedule(char *Schedule);
bool GetTSIDFromSchedule(char *Schedule, u_char *TSID);
bool GetDirectionBit1FromSchedule(char *Schedule);
bool GetDirectionBit2FromSchedule(char *Schedule);
bool GetServiceStartTimeFromSchedule(char *Schedule, unsigned int *ServiceStartTime);
bool GetServiceIntervalFromSchedule(char *Schedule, unsigned int *ServiceInterval);
bool GetSpecificationIntervalFromSchedule(char *Schedule, unsigned short *SpecificationInterval);

/*
   2007/11/03 By C.L. Chou
   Block Ack Parameter Set Field:
    -- Reserved (1 bit)
    -- Block Ack Policy (1 bit)
    -- TID (4 bits)
    -- Buffer Size (10 bits)
*/
bool GetBlockAckPolicyFromBlockAckParameterSet(char *BlockAckParameterSet);
bool GetTIDFromBlockAckParameterSet(char *BlockAckParameterSet, u_char *TID);
bool GetBufferSizeFromBlockAckParameterSet(char *BlockAckParameterSet, unsigned short *BufferSize);

/*
   2007/11/03 By C.L. Chou
   Block Ack Starting Sequence Control Field:
    -- Fragment Number (4 bits)
    -- Starting Sequence Number (12 bits)
*/
bool GetFragmentNumberFromBlockAckStartingSequenceControl(char *BlockAckStartingSequenceControl, unsigned short *FragmentNumber);
bool GetStartingSequenceNumberFromBlockAckStartingSequenceControl(char *BlockAckStartingSequenceControl, unsigned short *StartingSequenceNumber);

/*
   2007/11/03 By C.L. Chou
   DELBA Parameter Set Field:
    -- Reserved (11 bits)
    -- Initiator (1 bit)
    -- TID (4 bits)
*/
bool GetInitiatorFromDELBAParameterSet(char *DELBAParameterSet);
bool GetTIDFromDELBAParameterSet(char *DELBAParameterSet, u_char *TID);


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Beacon frame body:
	
    -- Timestamp (8 octets)
    -- Beacon Interval (2 octets)
    -- Capability Information (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- FH Parameter Set (7 octets) <OPTIONAL>
    -- DS Parameter Set (3 octets) <OPTIONAL>
    -- CF Parameter Set (8 octets) <OPTIONAL>
    -- IBSS Parameter Set (4 octets) <OPTIONAL>
    -- TIM (1_ElementID + 1_Length + 1_DTIM_Count + 1_DTIM_Period + 1_Bitmap_Control + 1~251_Partial_Virtual_Bitmap) <OPTIONAL>
    -- QBSS Load (7 octets) <OPTIONAL>
    -- EDCA Parameter Set (20 octets) <OPTIONAL>
    -- QoS Capability (3 octets) <OPTIONAL>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eBeaconFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithTIM, bool WithQBSS, bool WithEDCA, bool WithQoS);
unsigned int Calculate80211eBeaconFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len);
/*==Set===============================================*/
void SetTimestampInto80211eBeaconFrameBody(char *Body, const unsigned long long *Timestamp);
void SetBeaconIntervalInto80211eBeaconFrameBody(char *Body, const unsigned short *Interval);
void SetCapabilityInfoInto80211eBeaconFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetSSIDInto80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetFHParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex);
void SetDSParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel);
void SetCFParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration, const unsigned short *DurRemaining);
void SetIBSSParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window);
void SetTIMInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, u_char Bitmap_Ctrl, unsigned int PartialVirtualBitmap_Len, const void *PartialVirtualBitmap);
void SetQBSSLoadInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, const unsigned short *StationCount, u_char ChannelUtilization, const unsigned short *AvailableAdmissionCapacity);
void SetEDCAParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit); 
void SetQoSCapabilityInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest);
/*==Get===============================================*/
void GetTimestampFrom80211eBeaconFrameBody(char *Body, unsigned long long *Timestamp);
void GetBeaconIntervalFrom80211eBeaconFrameBody(char *Body, unsigned short *Interval);
char *GetCapabilityInfoFrom80211eBeaconFrameBody(char *Body);
void *GetSSIDFrom80211eBeaconFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetFHParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetDSParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetCFParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetIBSSParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetTIMFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetQBSSLoadFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len);
char *GetEDCAParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len);
char *GetQoSCapabilityFrom80211eBeaconFrameBody(char *Body,  unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Disassociation frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDisassociationFrameBodyLength();
/*==Set===============================================*/
void SetReasonCodeInto80211eDisassociationFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
void GetReasonCodeFrom80211eDisassociationFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Association Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- QoS Capability (3 octets)
*/
unsigned int Calculate80211eAssociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211eAssociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetListenIntervalInto80211eAssociationRequestFrameBody(char *Body, const unsigned short *Interval);
void SetSSIDInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetQoSCapabilityInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211eAssociationRequestFrameBody(char *Body);
void GetListenIntervalFrom80211eAssociationRequestFrameBody(char *Body, unsigned short *Interval);
void *GetSSIDFrom80211eAssociationRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetQoSCapabilityFrom80211eAssociationRequestFrameBody(char *Body,  unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Association Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- EDCA Parameter Set (20 octets)
*/
unsigned int Calculate80211eAssociationResponseFrameBodyLength(unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211eAssociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetStatusCodeInto80211eAssociationResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetAIDInto80211eAssociationResponseFrameBody(char *Body, const unsigned short *AID);
void SetSupportedRatesInto80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetEDCAParameterSetInto80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit); 
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211eAssociationResponseFrameBody(char *Body);
void GetStatusCodeFrom80211eAssociationResponseFrameBody(char *Body, unsigned short *StatusCode);
void GetAIDFrom80211eAssociationResponseFrameBody(char *Body, unsigned short *AID);
void *GetSupportedRatesFrom80211eAssociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len);
char *GetEDCAParameterSetFrom80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Reassociation Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- Current AP address (6 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- QoS Capability (3 octets)
*/
unsigned int Calculate80211eReassociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211eReassociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetListenIntervalInto80211eReassociationRequestFrameBody(char *Body, const unsigned short *ListenInterval);
void SetCurrentAPAddressInto80211eReassociationRequestFrameBody(char *Body, const void * CurrentAPAddress);
void SetSSIDInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetQoSCapabilityInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211eReassociationRequestFrameBody(char *Body);
void GetListenIntervalFrom80211eReassociationRequestFrameBody(char *Body, unsigned short *ListenInterval);
void GetCurrentAPAddressFrom80211eReassociationRequestFrameBody(char *Body, void * CurrentAPAddress);
void *GetSSIDFrom80211eReassociationRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetQoSCapabilityFrom80211eReassociationRequestFrameBody(char *Body,  unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Reassociation Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- EDCA Parameter Set (20 octets)
*/
unsigned int Calculate80211eReassociationResponseFrameBodyLength(unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211eReassociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetStatusCodeInto80211eReassociationResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetAIDInto80211eReassociationResponseFrameBody(char *Body, const unsigned short *AID);
void SetSupportedRatesInto80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetEDCAParameterSetInto80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit); 
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211eReassociationResponseFrameBody(char *Body);
void GetStatusCodeFrom80211eReassociationResponseFrameBody(char *Body, unsigned short *StatusCode);
void GetAIDFrom80211eReassociationResponseFrameBody(char *Body, unsigned short *AID);
void *GetSupportedRatesFrom80211eReassociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len);
char *GetEDCAParameterSetFrom80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Probe Request frame body:

    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211eProbeRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetSSIDInto80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
void *GetSSIDFrom80211eProbeRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Probe Response frame body:

    -- Timestamp (8 octets)
    -- Beacon Interval (2 octets)
    -- Capability Information (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- FH Parameter Set (7 octets) <OPTIONAL>
    -- DS Parameter Set (3 octets) <OPTIONAL>
    -- CF Parameter Set (8 octets) <OPTIONAL>
    -- IBSS Parameter Set (4 octets) <OPTIONAL>
    -- QBSS Load (7 octets) <OPTIONAL>
    -- EDCA Parameter Set (20 octets) <OPTIONAL>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eProbeResponseFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithQBSS, bool WithEDCA);
unsigned int Calculate80211eProbeResponseFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetTimestampInto80211eProbeResponseFrameBody(char *Body, const unsigned long long *Timestamp);
void SetBeaconIntervalInto80211eProbeResponseFrameBody(char *Body, const unsigned short *Interval);
void SetCapabilityInfoInto80211eProbeResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetSSIDInto80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetFHParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex);
void SetDSParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel);
void SetCFParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration, const unsigned short *DurRemaining);
void SetIBSSParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window);
void SetQBSSLoadInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *StationCount, u_char ChannelUtilization, const unsigned short *AvailableAdmissionCapacity);
void SetEDCAParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit); 
/*==Get===============================================*/
void GetTimestampFrom80211eProbeResponseFrameBody(char *Body, unsigned long long *Timestamp);
void GetBeaconIntervalFrom80211eProbeResponseFrameBody(char *Body, unsigned short *Interval);
char *GetCapabilityInfoFrom80211eProbeResponseFrameBody(char *Body);
void *GetSSIDFrom80211eProbeResponseFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetFHParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetDSParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetCFParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetIBSSParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetQBSSLoadFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetEDCAParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Authentication frame body:

    -- Authentication Algorithm Number (2 octets)
    -- Authentication Transaction Sequence Number (2 octets)
    -- Status Code (2 octets)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text) <OPTIONAL>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eAuthenticationFrameBody(bool WithChallengeText);
unsigned int Calculate80211eAuthenticationFrameBodyLength(u_char OptionalInfoFlag, unsigned int ChallengeText_Len);
/*==Set===============================================*/
void SetAuthAlgoNumInto80211eAuthenticationFrameBody(char *Body, const unsigned short *AuthAlgoNum);
void SetAuthTransSeqNumInto80211eAuthenticationFrameBody(char *Body, const unsigned short *AuthTransSeqNum);
void SetStatusCodeInto80211eAuthenticationFrameBody(char *Body, const unsigned short *StatusCode);
void SetChallengeTextInto80211eAuthenticationFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int ChallengeText_Len, const void *ChallengeText);
/*==Get===============================================*/
void GetAuthAlgoNumFrom80211eAuthenticationFrameBody(char *Body, unsigned short *AuthAlgoNum);
void GetAuthTransSeqNumFrom80211eAuthenticationFrameBody(char *Body, unsigned short *AuthTransSeqNum);
void GetStatusCodeFrom80211eAuthenticationFrameBody(char *Body, unsigned short *StatusCode);
char *GetChallengeTextFrom80211eAuthenticationFrameBody(char *Body);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Deauthentication frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDeauthenticationFrameBodyLength();
/*==Set===============================================*/
void SetReasonCodeInto80211eDeauthenticationFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
void GetReasonCodeFrom80211eDeauthenticationFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e ADDTS Request frame body:
   Category: 1
   Action: 0

    -- Category (1 octet)
    -- Action (1 octet)
    -- Dialog Token (1 octet)
    -- TSPEC (57 octets) 
    -- TCLAS (1_ElementID + 1_Length + 1_User_Priority + 0/3~254_Frame_Classifier) <OPTIONAL> 
    -- TCLAS Processing (3 octets) <OPTIONAL>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSRequestFrameBody(bool WithTCLAS, enum TCLAS_FrameClassifierType TCLASFrameClassifierType, bool WithTCLASProcessing);
unsigned int Calculate80211eADDTSRequestFrameBodyLength(u_char OptionalInfoFlag);
/*==Set===============================================*/
void SetDialogTokenInto80211eADDTSRequestFrameBody(char *Body, u_char DialogToken);
void SetTSPECInto80211eADDTSRequestFrameBody(char *Body, bool TSInfo_TrafficType, u_char TSInfo_TSID, bool TSInfo_Direction_Bit_1, bool TSInfo_Direction_Bit_2, bool TSInfo_AccessPolicy_Bit_1, bool TSInfo_AccessPolicy_Bit_2, bool TSInfo_Aggregation, bool TSInfo_APSD, u_char TSInfo_UserPriority, bool TSInfo_TSInfoAckPolicy_Bit_1, bool TSInfo_TSInfoAckPolicy_Bit_2, bool TSInfo_Schedule, const unsigned short *NominalMSDUSize, bool NominalMSDUSize_Fixed, const unsigned short *MaximumMSDUSize, const unsigned int *MinimumServiceInterval, const unsigned int *MaximumServiceInterval, const unsigned int *InactivityInterval, const unsigned int *SyspensionInterval, const unsigned int *ServiceStartTime, const unsigned int *MinimumDataRate, const unsigned int *MeanDataRate, const unsigned int *PeakDataRate, const unsigned int *BurstSize, const unsigned int *DelayBound, const unsigned int *MinimumPHYRate, const unsigned short *SurplusBandwidthAllowance_IntegerPart, const unsigned short *SurplusBandwidthAllowance_DecimalPart, const unsigned short *MediumTime);
void SetTCLASwithFrameClassifierType0Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const void *SourceAddress, const void *DestinationAddress, const unsigned short *Type);
void SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, u_char DSCP, u_char Protocol);
void SetTCLASwithFrameClassifierType1ForIPv6Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, const void *FlowLabel);
void SetTCLASwithFrameClassifierType2Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const unsigned short *TagType);
void SetTCLASProcessingInto80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char Processing);
/*==Get===============================================*/
void GetDialogTokenFrom80211eADDTSRequestFrameBody(char *Body, u_char *DialogToken);
char *GetTSPECFrom80211eADDTSRequestFrameBody(char *Body);
char *GetTCLASFrom80211eADDTSRequestFrameBody(char *Body, u_char *FrameClassifierType);
char *GetTCLASProcessingFrom80211eADDTSRequestFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e ADDTS Response frame body:
   Category: 1
   Action: 1

    -- Category (1 octet)
    -- Action (1 octet)
    -- Dialog Token (1 octet)
    -- Status Code (2 octets)
    -- TS Delay (6 octets)
    -- TSPEC (57 octets) 
    -- TCLAS (1_ElementID + 1_Length + 1_User_Priority + 0/3~254_Frame_Classifier) <OPTIONAL> 
    -- TCLAS Processing (3 octets) <OPTIONAL>
    -- Schedule (14 octets)
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSResponseFrameBody(bool WithTCLAS, enum TCLAS_FrameClassifierType TCLASFrameClassifierType, bool WithTCLASProcessing);
unsigned int Calculate80211eADDTSResponseFrameBodyLength(u_char OptionalInfoFlag);
/*==Set===============================================*/
void SetDialogTokenInto80211eADDTSResponseFrameBody(char *Body, u_char DialogToken);
void SetStatusCodeInto80211eADDTSResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetTSDelayInto80211eADDTSResponseFrameBody(char *Body, const unsigned int *Delay);
void SetTSPECInto80211eADDTSResponseFrameBody(char *Body, bool TSInfo_TrafficType, u_char TSInfo_TSID, bool TSInfo_Direction_Bit_1, bool TSInfo_Direction_Bit_2, bool TSInfo_AccessPolicy_Bit_1, bool TSInfo_AccessPolicy_Bit_2, bool TSInfo_Aggregation, bool TSInfo_APSD, u_char TSInfo_UserPriority, bool TSInfo_TSInfoAckPolicy_Bit_1, bool TSInfo_TSInfoAckPolicy_Bit_2, bool TSInfo_Schedule, const unsigned short *NominalMSDUSize, bool NominalMSDUSize_Fixed, const unsigned short *MaximumMSDUSize, const unsigned int *MinimumServiceInterval, const unsigned int *MaximumServiceInterval, const unsigned int *InactivityInterval, const unsigned int *SyspensionInterval, const unsigned int *ServiceStartTime, const unsigned int *MinimumDataRate, const unsigned int *MeanDataRate, const unsigned int *PeakDataRate, const unsigned int *BurstSize, const unsigned int *DelayBound, const unsigned int *MinimumPHYRate, const unsigned short *SurplusBandwidthAllowance_IntegerPart, const unsigned short *SurplusBandwidthAllowance_DecimalPart, const unsigned short *MediumTime);
void SetTCLASwithFrameClassifierType0Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const void *SourceAddress, const void *DestinationAddress, const unsigned short *Type);
void SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, u_char DSCP, u_char Protocol);
void SetTCLASwithFrameClassifierType1ForIPv6Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, const void *FlowLabel);
void SetTCLASwithFrameClassifierType2Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const unsigned short *TagType);
void SetTCLASProcessingInto80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char Processing);
void SetScheduleInto80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, bool Aggregation, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, const unsigned int *ServiceStartTime, const unsigned int *ServiceInterval, const unsigned short *SpecificationInterval); 
/*==Get===============================================*/
void GetDialogTokenFrom80211eADDTSResponseFrameBody(char *Body, u_char *DialogToken);
void GetStatusCodeFrom80211eADDTSResponseFrameBody(char *Body, unsigned short *StatusCode);
char *GetTSDelayFrom80211eADDTSResponseFrameBody(char *Body);
char *GetTSPECFrom80211eADDTSResponseFrameBody(char *Body);
char *GetTCLASFrom80211eADDTSResponseFrameBody(char *Body, u_char *FrameClassifierType);
char *GetTCLASProcessingFrom80211eADDTSResponseFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType);
char *GetScheduleFrom80211eADDTSResponseFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e DELTS frame body:
   Category: 1
   Action: 2

    -- Category (1 octet)
    -- Action (1 octet)
    -- TS Info (3 octets)
    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDELTSFrameBodyLength();
/*==Set===============================================*/
void SetTSInfoInto80211eDELTSFrameBody(char *Body, bool TrafficType, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, bool AccessPolicy_Bit_1, bool AccessPolicy_Bit_2, bool Aggregation, bool APSD, u_char UserPriority, bool TSInfoAckPolicy_Bit_1, bool TSInfoAckPolicy_Bit_2, bool Schedule);
void SetReasonCodeInto80211eDELTSFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
char *GetTSInfoFrom80211eDELTSFrameBody(char *Body);
void GetReasonCodeFrom80211eDELTSFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Schedule frame body:
   Category: 1
   Action: 3

    -- Category (1 octet)
    -- Action (1 octet)
    -- Schedule (14 octets)
*/
unsigned int Calculate80211eScheduleFrameBodyLength();
/*==Set===============================================*/
void SetScheduleInto80211eScheduleFrameBody(char *Body, bool Aggregation, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, const unsigned int *ServiceStartTime, const unsigned int *ServiceInterval, const unsigned short *SpecificationInterval); 
/*==Get===============================================*/
char *GetScheduleFrom80211eScheduleFrameBody(char *Body);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e DLS Request frame body:
   Category: 2
   Action: 0

    -- Category (1 octet)
    -- Action (1 octet)
    -- Destination MAC Address (6 octets)
    -- Source MAC Address (6 octets)
    -- Capability Information (2 octets)
    -- DLS Timeout Value (2 octet)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- Extended Supported Rates ((1_ElementID + 1_Length + 1~255_Supported_Rates) 
*/
unsigned int Calculate80211eDLSRequestFrameBodyLength(unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len);
/*==Set===============================================*/
void SetDestinationMACAddressInto80211eDLSRequestFrameBody(char *Body, const void *DestinationMACAddress);
void SetSourceMACAddressInto80211eDLSRequestFrameBody(char *Body, const void *SourceMACAddress);
void SetCapabilityInfoInto80211eDLSRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetDLSTimeoutValueInto80211eDLSRequestFrameBody(char *Body, const unsigned short *DLSTimeoutValue);
void SetSupportedRatesInto80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetExtendedSupportedRatesInto80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len, const void *ExtendedSupportedRates);
/*==Get===============================================*/
void GetDestinationMACAddressFrom80211eDLSRequestFrameBody(char *Body, void *DestinationMACAddress);
void GetSourceMACAddressFrom80211eDLSRequestFrameBody(char *Body, void *SourceMACAddress);
char *GetCapabilityInfoFrom80211eDLSRequestFrameBody(char *Body);
void GetDLSTimeoutValueFrom80211eDLSRequestFrameBody(char *Body, unsigned short *DLSTimeoutValue);
void *GetSupportedRatesFrom80211eDLSRequestFrameBody(char *Body, unsigned int *SupportedRates_Len);
void *GetExtendedSupportedRatesFrom80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int *ExtendedSupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e DLS Response frame body:
   Category: 2
   Action: 1

    -- Category (1 octet)
    -- Action (1 octet)
    -- Status Code (2 octets)
    -- Destination MAC Address (6 octets)
    -- Source MAC Address (6 octets)
    -- Capability Information (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- Extended Supported Rates ((1_ElementID + 1_Length + 1~255_Supported_Rates) 
*/
unsigned int Calculate80211eDLSResponseFrameBodyLength(unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len);
/*==Set===============================================*/
void SetStatusCodeInto80211eDLSResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetDestinationMACAddressInto80211eDLSResponseFrameBody(char *Body, const void *DestinationMACAddress);
void SetSourceMACAddressInto80211eDLSResponseFrameBody(char *Body, const void *SourceMACAddress);
void SetCapabilityInfoInto80211eDLSResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck);
void SetSupportedRatesInto80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetExtendedSupportedRatesInto80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len, const void *ExtendedSupportedRates);
/*==Get===============================================*/
void GetStatusCodeFrom80211eDLSResponseFrameBody(char *Body, unsigned short *StatusCode);
void GetDestinationMACAddressFrom80211eDLSResponseFrameBody(char *Body, void *DestinationMACAddress);
void GetSourceMACAddressFrom80211eDLSResponseFrameBody(char *Body, void *SourceMACAddress);
char *GetCapabilityInfoFrom80211eDLSResponseFrameBody(char *Body);
void *GetSupportedRatesFrom80211eDLSResponseFrameBody(char *Body, unsigned int *SupportedRates_Len);
void *GetExtendedSupportedRatesFrom80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int *ExtendedSupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e DLS Teardown frame body:
   Category: 2
   Action: 2

    -- Category (1 octet)
    -- Action (1 octet)
    -- Destination MAC Address (6 octets)
    -- Source MAC Address (6 octets)
    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDLSTeardownFrameBodyLength();
/*==Set===============================================*/
void SetDestinationMACAddressInto80211eDLSTeardownFrameBody(char *Body, const void *DestinationMACAddress);
void SetSourceMACAddressInto80211eDLSTeardownFrameBody(char *Body, const void *SourceMACAddress);
void SetReasonCodeInto80211eDLSTeardownFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
void GetDestinationMACAddressFrom80211eDLSTeardownFrameBody(char *Body, void *DestinationMACAddress);
void GetSourceMACAddressFrom80211eDLSTeardownFrameBody(char *Body, void *SourceMACAddress);
void GetReasonCodeFrom80211eDLSTeardownFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e ADDBA Request frame body:
   Category: 3
   Action: 0

    -- Category (1 octet)
    -- Action (1 octet)
    -- Dialog Token (1 octet)
    -- Block Ack Parameter Set (2 octets)
    -- Block Ack Timeout Value (2 octets)
    -- Block Ack Starting Sequence Control (2 octets)
*/
unsigned int Calculate80211eADDBARequestFrameBodyLength();
/*==Set===============================================*/
void SetDialogTokenInto80211eADDBARequestFrameBody(char *Body, u_char DialogToken);
void SetBlockAckParameterSetInto80211eADDBARequestFrameBody(char *Body, bool BlockAckPolicy, u_char TID, const unsigned short *BufferSize);
void SetBlockAckTimeoutValueInto80211eADDBARequestFrameBody(char *Body, const unsigned short *BlockAckTimeoutValue);
void SetBlockAckStartingSequenceControlInto80211eADDBARequestFrameBody(char *Body, const unsigned short *FragmentNumber, const unsigned short *StartingSequenceNumber);
/*==Get===============================================*/
void GetDialogTokenFrom80211eADDBARequestFrameBody(char *Body, u_char *DialogToken);
char *GetBlockAckParameterSetFrom80211eADDBARequestFrameBody(char *Body);
void GetBlockAckTimeoutValueFrom80211eADDBARequestFrameBody(char *Body, unsigned short *BlockAckTimeoutValue);
char *GetBlockAckStartingSequenceControlFrom80211eADDBARequestFrameBody(char *Body);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e ADDBA Response frame body:
   Category: 3
   Action: 1

    -- Category (1 octet)
    -- Action (1 octet)
    -- Dialog Token (1 octet)
    -- Status Code (2 octets)
    -- Block Ack Parameter Set (2 octets)
    -- Block Ack Timeout Value (2 octets)
*/
unsigned int Calculate80211eADDBAResponseFrameBodyLength();
/*==Set===============================================*/
void SetDialogTokenInto80211eADDBAResponseFrameBody(char *Body, u_char DialogToken);
void SetStatusCodeInto80211eADDBAResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetBlockAckParameterSetInto80211eADDBAResponseFrameBody(char *Body, bool BlockAckPolicy, u_char TID, const unsigned short *BufferSize);
void SetBlockAckTimeoutValueInto80211eADDBAResponseFrameBody(char *Body, const unsigned short *BlockAckTimeoutValue);
/*==Get===============================================*/
void GetDialogTokenFrom80211eADDBAResponseFrameBody(char *Body, u_char *DialogToken);
void GetStatusCodeFrom80211eADDBAResponseFrameBody(char *Body, unsigned short *StatusCode);
char *GetBlockAckParameterSetFrom80211eADDBAResponseFrameBody(char *Body);
void GetBlockAckTimeoutValueFrom80211eADDBAResponseFrameBody(char *Body, unsigned short *BlockAckTimeoutValue);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e DELBA frame body:
   Category: 3
   Action: 2

    -- Category (1 octet)
    -- Action (1 octet)
    -- DELBA Parameter Set (2 octets)
    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDELBAFrameBodyLength();
/*==Set===============================================*/
void SetDELBAParameterSetInto80211eDELBAFrameBody(char *Body, bool Initiator, u_char TID);
void SetReasonCodeInto80211eDELBAFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
char *GetDELBAParameterSetFrom80211eDELBAFrameBody(char *Body);
void GetReasonCodeFrom80211eDELBAFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/


#endif	/* __NCTUNS_mac_802_11e_management_framebody_h__ */
