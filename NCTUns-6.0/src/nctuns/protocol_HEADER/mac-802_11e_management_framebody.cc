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

#include "mac-802_11e_management_framebody.h"
#include <strings.h>


void Initialize80211eFrameBodyWithZero(void *Body, int len) {
	if(Body == NULL) return;
	bzero(Body, len);
	return;
}

void SetCategoryInto80211eActionFrameBody(char *Body, u_char Category) {
	if(Body == NULL) return;
	Body[0] = Category;
	return;
}

void SetActionInto80211eActionFrameBody(char *Body, u_char Action) {
	if(Body == NULL) return;
	Body[1] = Action;
	return;
}

void GetCategoryFrom80211eActionFrameBody(char *Body, u_char *Category) {
	if(Body == NULL) return;
	*Category = (u_char)Body[0];
	return;
}

void GetActionFrom80211eActionFrameBody(char *Body, u_char *Action) {
	if(Body == NULL) return;
	*Action = (u_char)Body[1];
	return;
}


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
bool GetESSFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x80)	return true;
	else				return false; 
}
bool GetIBSSFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x40)	return true;
	else				return false; 
}
bool GetCFPollableFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x20)	return true;
	else				return false; 
}
bool GetCFPollRequestFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x10)	return true;
	else				return false; 
}
bool GetPrivacyFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x08)	return true;
	else				return false; 
}
bool GetShortPreambleFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x04)	return true;
	else				return false; 
}
bool GetPBCCFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x02)	return true;
	else				return false; 
}
bool GetChannelAgilityFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x01)	return true;
	else				return false; 
}
bool GetSpectrumManagementFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x80)	return true;
	else				return false; 
}
bool GetQoSFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x40)	return true;
	else				return false; 
}
bool GetShortSlotTimeFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x20)	return true;
	else				return false; 
}
bool GetAPSDFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x10)	return true;
	else				return false; 
}
bool GetDSSSOFDMFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x04)	return true;
	else				return false; 
}
bool GetDelayedBlockAckFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x02)	return true;
	else				return false; 
}
bool GetImmediateBlockAckFrom80211eCapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[1] & 0x01)	return true;
	else				return false; 
}

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
bool GetDwellTimeFrom80211eFHParameterSet(char *FHParameterSet, unsigned short *DwellTime){
	if(FHParameterSet == NULL || DwellTime == NULL)	return false;
	bcopy((void *)&FHParameterSet[2], (void *)DwellTime, 2);
	return true;
} 
bool GetHopSetFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopSet) {
	if(FHParameterSet == NULL || HopSet == NULL)	return false;
	*HopSet = FHParameterSet[4];
	return true;
}
bool GetHopPatternFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopPattern) {
	if(FHParameterSet == NULL || HopPattern == NULL)	return false;
	*HopPattern = FHParameterSet[5];
	return true;
}
bool GetHopIndexFrom80211eFHParameterSet(char *FHParameterSet, u_char *HopIndex) {
	if(FHParameterSet == NULL || HopIndex == NULL)	return false;
	*HopIndex = FHParameterSet[6];
	return true;
}

/* 
   2007/11/03 By C.L. Chou 
   DS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Current Channel (1 octet)
*/
bool GetCurrentChannelFrom80211eDSParameterSet(char *DSParameterSet, u_char *CurrentChannel) {
	if(DSParameterSet == NULL || CurrentChannel == NULL)	return false;
	*CurrentChannel = DSParameterSet[2];
	return true;
}

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
bool GetCountFrom80211eCFParameterSet(char *CFParameterSet, u_char *Count) {
	if(CFParameterSet == NULL || Count == NULL)	return false;
	*Count = CFParameterSet[2];
	return true;
}
bool GetPeriodFrom80211eCFParameterSet(char *CFParameterSet, u_char *Period) {
	if(CFParameterSet == NULL || Period == NULL)	return false;
	*Period = CFParameterSet[3];
	return true;
}
bool GetMaxDurationFrom80211eCFParameterSet(char *CFParameterSet, unsigned short *MaxDuration) {
	if(CFParameterSet == NULL || MaxDuration == NULL)	return false;
	bcopy((void *)&CFParameterSet[4], (void *)MaxDuration, 2);
	return true;
}
bool GetDurRemainingFrom80211eCFParameterSet(char *CFParameterSet, unsigned short *DurRemaining) {
	if(CFParameterSet == NULL || DurRemaining == NULL)	return false;
	bcopy((void *)&CFParameterSet[6], (void *)DurRemaining, 2);
	return true;
}

/* 
   2007/11/03 By C.L. Chou 
   IBSS Parameter SetField:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- ATIM Window(2 octets)
*/
bool GetATIMWindowFrom80211eIBSSParameterSet(char *IBSSParameterSet, unsigned short *ATIM_Window) {
	if(IBSSParameterSet == NULL || ATIM_Window == NULL)	return false;
	bcopy((void *)&IBSSParameterSet[2], (void *)ATIM_Window, 2);
	return true;
}

/* 
   2007/11/03 By C.L. Chou 
   TIM Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- DTIM Count (1 octet)
    -- DTIM Period (1 octet)
    -- Bitmap Control (1 octet)
    -- Partial Virtual Bitmap (variable)
*/
bool GetCountFrom80211eTIM(char *TIM, u_char *Count) {
	if(TIM == NULL || Count == NULL)	return false;
	*Count = TIM[2];
	return true;
}
bool GetPeriodFrom80211eTIM(char *TIM, u_char *Period) {
	if(TIM == NULL || Period == NULL)	return false;
	*Period = TIM[3];
	return true;
}
bool GetBitmapCtrlFrom80211eTIM(char *TIM, u_char *Bitmap_Ctrl) {
	if(TIM == NULL || Bitmap_Ctrl == NULL)	return false;
	*Bitmap_Ctrl = TIM[4];
	return true;
}
void *GetPartialVirtualBitmapFrom80211eTIM(char *TIM, unsigned int *PartialVirtualBitmap_Len) {
	if(TIM == NULL || PartialVirtualBitmap_Len == NULL)	return NULL;
	*PartialVirtualBitmap_Len = (unsigned int)TIM[1] - 3;
	return &TIM[5];
}

/*
   2007/11/03 By C.L. Chou
   Challenge Text Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Challenge Text (1~253 octets)
*/
void *GetChallengeTextFrom80211eChallengeText(char *ChallengeText, unsigned int *ChallengeText_Len) {
	if(ChallengeText == NULL || ChallengeText_Len == NULL) return NULL;
	*ChallengeText_Len = (unsigned int)ChallengeText[1];
	return (void *)&ChallengeText[2];
}

/*
   2007/11/03 By C.L. Chou
   QBSS Load Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Station Count (2 octet)
    -- Channel Utilization (1 octet)
    -- Available Admission Capacity (2 octets)
*/
bool GetStationCountFromQBSSLoad(char *QBSSLoad, unsigned short *StationCount){
	if(QBSSLoad == NULL || StationCount == NULL)	return false;
	bcopy((void *)&QBSSLoad[2], (void *)StationCount, 2);
	return true;
}
bool GetChannelUtilizationFromQBSSLoad(char *QBSSLoad, u_char *ChannelUtilization){
	if(QBSSLoad == NULL || ChannelUtilization == NULL)	return false;
	*ChannelUtilization = QBSSLoad[4];
	return true;
}
bool GetAvailableAdmissionCapacityFromQBSSLoad(char *QBSSLoad, unsigned short *AvailableAdmissionCapacity){
	if(QBSSLoad == NULL || AvailableAdmissionCapacity)	return false;
	bcopy((void *)&QBSSLoad[5], (void *)AvailableAdmissionCapacity, 2);
	return true;
}

/*
   2007/11/03 By C.L. Chou
   QoS Capability Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- QoS Info (1 octet)
*/
char *GetQoSInfoFromQoSCapability(char *QoSCapability) {
	if(QoSCapability == NULL)	return NULL;
	else				return (char *)&QoSCapability[2];
}

/*
   2007/11/03 By C.L. Chou
   QoS Info Field:
    -- EDCA Parameter Set Update Count (4 bits)
    -- Q-Ack (1 bit)
    -- Queue Request (1 bit)
    -- TXOP Request (1 bit)
    -- Reserved (1 bit)    
*/
bool GetEDCAParameterSetUpdateCountFromQoSInfo(char *QoSInfo, u_char *EDCAParameterSetUpdateCount) {
	if(QoSInfo == NULL || EDCAParameterSetUpdateCount == NULL)	return false;
	*EDCAParameterSetUpdateCount = QoSInfo[0]>>4 & 0x0f;
	return true;
}
bool GetQAckFromQoSInfo(char *QoSInfo) {
	if(QoSInfo[0] & 0x08)	return true;
	else			return false;
}
bool GetQueueRequestFromQoSInfo(char *QoSInfo) {
	if(QoSInfo[0] & 0x04)	return true;
	else			return false;
}
bool GetTXOPRequestFromQoSInfo(char *QoSInfo) {
	if(QoSInfo[0] & 0x02)	return true;
	else			return false;
}

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
char *GetQoSInfoFromEDCAParameterSet(char *EDCAParameterSet) {
	if(EDCAParameterSet == NULL)	return NULL;
	else				return (char *)&EDCAParameterSet[2];
}

char *GetACBEParameterRecordFromEDCAParameterSet(char *EDCAParameterSet) {
	if(EDCAParameterSet == NULL)	return NULL;
	else				return (char *)&EDCAParameterSet[4];
}

char *GetACBKParameterRecordFromEDCAParameterSet(char *EDCAParameterSet) {
	if(EDCAParameterSet == NULL)	return NULL;
	else				return (char *)&EDCAParameterSet[8];
}

char *GetACVIParameterRecordFromEDCAParameterSet(char *EDCAParameterSet) {
	if(EDCAParameterSet == NULL)	return NULL;
	else				return (char *)&EDCAParameterSet[12];
}

char *GetACVOParameterRecordFromEDCAParameterSet(char *EDCAParameterSet) {
	if(EDCAParameterSet == NULL)	return NULL;
	else				return (char *)&EDCAParameterSet[16];
}

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
bool GetAIFSNFromACXXParameterRecord(char *ACXXParameterRecord, u_char *AIFSN){
	if(ACXXParameterRecord == NULL || AIFSN == NULL)	return false;
	*AIFSN = ACXXParameterRecord[0]>>4 & 0x0f;
	return true;
}
bool GetACMFromACXXParameterRecord(char *ACXXParameterRecord){
	if(ACXXParameterRecord[0] & 0x08)	return true;
	else					return false;
}
bool GetECWFromACXXParameterRecord(char *ACXXParameterRecord, u_char *ECW){
	if(ACXXParameterRecord == NULL || ECW ==NULL)	return false;
	*ECW = ACXXParameterRecord[1];
	return true;
}
bool GetTXOPLimitFromACXXParameterRecord(char *ACXXParameterRecord, unsigned short *TXOP_Limit){
	if(ACXXParameterRecord == NULL || TXOP_Limit == NULL)	return false;
	bcopy((void *)&ACXXParameterRecord[2], (void *)TXOP_Limit, 2);
	return true;
}

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
char *GetTSInfoFromTSPEC(char *TSPEC) {
	if(TSPEC == NULL) 	return NULL;
	else			return (char *)&TSPEC[2];
}
bool GetNominalMSDUSizeFromTSPEC(char *TSPEC, unsigned short *NominalMSDUSize) {
	u_char	*tmp_ptr;
	u_char  char_buf[2];

	if(TSPEC == NULL || NominalMSDUSize == NULL) return false;

	tmp_ptr = (u_char *)NominalMSDUSize;
	char_buf[1] = (u_char)TSPEC[5];
	char_buf[0] = (u_char)TSPEC[6];

	if(char_buf[1] & 0x01)	char_buf[0] = (char_buf[0]>>1) | 0x80;
	else			char_buf[0] = (char_buf[0]>>1) & 0x7f;

	char_buf[1] = (char_buf[1]>>1) & 0x7f;
	
	tmp_ptr[0] = char_buf[0];
	tmp_ptr[1] = char_buf[1];

	return true;
}
bool GetNominalMSDUSizeFixedFromTSPEC(char *TSPEC) {
	if(TSPEC == NULL) return false;
	if(TSPEC[6] & 0x01)	return true;
	else			return false;
}
bool GetMaximumMSDUSizeFromTSPEC(char *TSPEC, unsigned short *MaximumMSDUSize) {
	if(TSPEC == NULL || MaximumMSDUSize == NULL) return false;
	bcopy((void *)&TSPEC[7], (void *)MaximumMSDUSize, 2);
	return true;
}
bool GetMinimumServiceIntervalFromTSPEC(char *TSPEC, unsigned int *MinimumServiceInterval) {
	if(TSPEC == NULL || MinimumServiceInterval == NULL) return false;
	bcopy((void *)&TSPEC[9], (void *)MinimumServiceInterval, 4);
	return true;
}
bool GetMaximumServiceIntervalFromTSPEC(char *TSPEC, unsigned int *MaximumServiceInterval) {
	if(TSPEC == NULL || MaximumServiceInterval == NULL) return false;
	bcopy((void *)&TSPEC[13], (void *)MaximumServiceInterval, 4);
	return true;
}
bool GetInactivityIntervalFromTSPEC(char *TSPEC, unsigned int *InactivityInterval) {
	if(TSPEC == NULL || InactivityInterval == NULL) return false;
	bcopy((void *)&TSPEC[17], (void *)InactivityInterval, 4);
	return true;
}
bool GetSyspensionIntervalFromTSPEC(char *TSPEC, unsigned int *SyspensionInterval) {
	if(TSPEC == NULL || SyspensionInterval == NULL) return false;
	bcopy((void *)&TSPEC[21], (void *)SyspensionInterval, 4);
	return true;
}
bool GetServiceStartTimeFromTSPEC(char *TSPEC, unsigned int *ServiceStartTime) {
	if(TSPEC == NULL || ServiceStartTime == NULL) return false;
	bcopy((void *)&TSPEC[25], (void *)ServiceStartTime, 4);
	return true;
}
bool GetMinimumDataRateFromTSPEC(char *TSPEC, unsigned int *MinimumDataRate) {
	if(TSPEC == NULL || MinimumDataRate == NULL) return false;
	bcopy((void *)&TSPEC[29], (void *)MinimumDataRate, 4);
	return true;
}
bool GetMeanDataRateFromTSPEC(char *TSPEC, unsigned int *MeanDataRate) {
	if(TSPEC == NULL || MeanDataRate == NULL) return false;
	bcopy((void *)&TSPEC[33], (void *)MeanDataRate, 4);
	return true;
}
bool GetPeakDataRateFromTSPEC(char *TSPEC, unsigned int *PeakDataRate) {
	if(TSPEC == NULL || PeakDataRate == NULL) return false;
	bcopy((void *)&TSPEC[37], (void *)PeakDataRate, 4);
	return true;
}
bool GetBurstSizeFromTSPEC(char *TSPEC, unsigned int *BurstSize) {
	if(TSPEC == NULL || BurstSize == NULL) return false;
	bcopy((void *)&TSPEC[41], (void *)BurstSize, 4);
	return true;
}
bool GetDelayBoundFromTSPEC(char *TSPEC, unsigned int *DelayBound) {
	if(TSPEC == NULL || DelayBound == NULL) return false;
	bcopy((void *)&TSPEC[45], (void *)DelayBound, 4);
	return true;
}
bool GetMinimumPHYRateFromTSPEC(char *TSPEC, unsigned int *MinimumPHYRate) {
	if(TSPEC == NULL || MinimumPHYRate == NULL) return false;
	bcopy((void *)&TSPEC[49], (void *)MinimumPHYRate, 4);
	return true;
}
bool GetSurplusBandwidthAllowanceIntegerPartFromTSPEC(char *TSPEC, unsigned short *SurplusBandwidthAllowance_IntegerPart) {
	u_char	*tmp_ptr;
	u_char  char_buf;

	if(TSPEC == NULL || SurplusBandwidthAllowance_IntegerPart == NULL) return false;

	tmp_ptr = (u_char *)SurplusBandwidthAllowance_IntegerPart;
	char_buf = (u_char)TSPEC[53];
	char_buf = (char_buf>>5) & 0x07;	

	tmp_ptr[1] = 0x00;
	tmp_ptr[0] = char_buf;

	return true;
}
bool GetSurplusBandwidthAllowanceDecimalPartFromTSPEC(char *TSPEC, unsigned short *SurplusBandwidthAllowance_DecimalPart) {
	u_char	*tmp_ptr;
	u_char  char_buf;

	if(TSPEC == NULL || SurplusBandwidthAllowance_DecimalPart == NULL) return false;

	tmp_ptr = (u_char *)SurplusBandwidthAllowance_DecimalPart;
	char_buf = (u_char)TSPEC[53];

	tmp_ptr[1] = (char_buf & 0x1f);
	tmp_ptr[0] = (u_char)TSPEC[54];

	return true;
}
bool GetMediumTimeFromTSPEC(char *TSPEC, unsigned short *MediumTime) {
	if(TSPEC == NULL || MediumTime == NULL) return false;
	bcopy((void *)&TSPEC[55], (void *)MediumTime, 2);
	return true;
}

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
bool GetTrafficTypeFromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[0] & 0x80)	return true;
	else			return false;
}
bool GetTSIDFromTSInfo(char *TSInfo, u_char *TSID) {
	if(TSInfo == NULL || TSID == NULL) return false;
	*TSID = (u_char)((TSInfo[0]>>3) & 0x0f);
	return true;
}
bool GetDirectionBit1FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[0] & 0x04)	return true;
	else			return false;
}
bool GetDirectionBit2FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[0] & 0x02)	return true;
	else			return false;
}
bool GetAccessPolicyBit1FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[0] & 0x01)	return true;
	else			return false;
}
bool GetAccessPolicyBit2FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[1] & 0x80)	return true;
	else			return false;
}
bool GetAggregationFromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[1] & 0x40)	return true;
	else			return false;
}
bool GetAPSDFromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[1] & 0x20)	return true;
	else			return false;
}
bool GetUserPriorityFromTSInfo(char *TSInfo, u_char *UserPriority) {
	if(TSInfo == NULL || UserPriority == NULL) return false;
	*UserPriority = (u_char)((TSInfo[1]>>2) & 0x07);
	return true;
}
bool GetTSInfoAckPolicyBit1FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[1] & 0x02)	return true;
	else			return false;
}
bool GetTSInfoAckPolicyBit2FromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[1] & 0x01)	return true;
	else			return false;
}
bool GetScheduleFromTSInfo(char *TSInfo) {
	if(TSInfo == NULL) return false;
	if(TSInfo[2] & 0x80)	return true;
	else			return false;
}

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
bool GetUserPriorityFromTCLASwithFrameClassifierType0(char *TCLAS, u_char *UserPriority) {
	if(TCLAS == NULL || UserPriority == NULL) return false;
	*UserPriority = (u_char)TCLAS[2];
	return true;
}
bool GetClassifierMaskFromTCLASwithFrameClassifierType0(char *TCLAS, u_char *ClassifierMask) {
	if(TCLAS == NULL || ClassifierMask == NULL) return false;
	*ClassifierMask = (u_char)TCLAS[4];
	return true;
}
bool GetSourceAddressFromTCLASwithFrameClassifierType0(char *TCLAS, void *SourceAddress) {
	if(TCLAS == NULL || SourceAddress == NULL) return false;
	bcopy((void *)&TCLAS[5], (void *)SourceAddress, 6);
	return true;
}
bool GetDestinationAddressFromTCLASwithFrameClassifierType0(char *TCLAS, void *DestinationAddress) {
	if(TCLAS == NULL || DestinationAddress == NULL) return false;
	bcopy((void *)&TCLAS[11], (void *)DestinationAddress, 6);
	return true;
}
bool GetTypeFromTCLASwithFrameClassifierType0(char *TCLAS, unsigned short *Type) {
	if(TCLAS == NULL || Type == NULL) return false;
	bcopy((void *)&TCLAS[17], (void *)Type, 2);
	return true;
}

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
bool GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *UserPriority) {
	if(TCLAS == NULL || UserPriority == NULL) return false;
	*UserPriority = (u_char)TCLAS[2];
	return true;
}
bool GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *ClassifierMask) {
	if(TCLAS == NULL || ClassifierMask == NULL) return false;
	*ClassifierMask = (u_char)TCLAS[4];
	return true;
}
bool GetVersionFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *Version) {
	if(TCLAS == NULL || Version == NULL) return false;
	*Version = (u_char)TCLAS[5];
	return true;
}
bool GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, void *SourceIPAddress) {
	if(TCLAS == NULL || SourceIPAddress == NULL) return false;
	bcopy((void *)&TCLAS[6], (void *)SourceIPAddress, 4);
	return true;
}
bool GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, void *DestinationIPAddress) {
	if(TCLAS == NULL || DestinationIPAddress == NULL) return false;
	bcopy((void *)&TCLAS[10], (void *)DestinationIPAddress, 4);
	return true;
}
bool GetSourcePortFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, unsigned short *SourcePort) {
	if(TCLAS == NULL || SourcePort == NULL) return false;
	bcopy((void *)&TCLAS[14], (void *)SourcePort, 2);
	return true;
}
bool GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, unsigned short *DestinationPort) {
	if(TCLAS == NULL || DestinationPort == NULL) return false;
	bcopy((void *)&TCLAS[16], (void *)DestinationPort, 2);
	return true;
}
bool GetDSCPFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *DSCP) {
	if(TCLAS == NULL || DSCP == NULL) return false;
	*DSCP = (u_char)TCLAS[18];
	return true;
}
bool GetProtocolFromTCLASwithFrameClassifierType1ForIPv4(char *TCLAS, u_char *Protocol) {
	if(TCLAS == NULL || Protocol == NULL) return false;
	*Protocol = (u_char)TCLAS[19];
	return true;
}

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
bool GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *UserPriority) {
	if(TCLAS == NULL || UserPriority == NULL) return false;
	*UserPriority = (u_char)TCLAS[2];
	return true;
}
bool GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *ClassifierMask) {
	if(TCLAS == NULL || ClassifierMask == NULL) return false;
	*ClassifierMask = (u_char)TCLAS[4];
	return true;
}
bool GetVersionFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, u_char *Version) {
	if(TCLAS == NULL || Version == NULL) return false;
	*Version = (u_char)TCLAS[5];
	return true;
}
bool GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *SourceIPAddress) {
	if(TCLAS == NULL || SourceIPAddress == NULL) return false;
	bcopy((void *)&TCLAS[6], (void *)SourceIPAddress, 16);
	return true;
}
bool GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *DestinationIPAddress) {
	if(TCLAS == NULL || DestinationIPAddress == NULL) return false;
	bcopy((void *)&TCLAS[22], (void *)DestinationIPAddress, 16);
	return true;
}
bool GetSourcePortFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, unsigned short *SourcePort) {
	if(TCLAS == NULL || SourcePort == NULL) return false;
	bcopy((void *)&TCLAS[38], (void *)SourcePort, 2);
	return true;
}
bool GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, unsigned short *DestinationPort) {
	if(TCLAS == NULL || DestinationPort == NULL) return false;
	bcopy((void *)&TCLAS[40], (void *)DestinationPort, 2);
	return true;
}
bool GetFlowLabelFromTCLASwithFrameClassifierType1ForIPv6(char *TCLAS, void *FlowLabel) {
	if(TCLAS == NULL || FlowLabel == NULL) return false;
	bcopy((void *)&TCLAS[42], (void *)FlowLabel, 3);
	return true;
}

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
bool GetUserPriorityFromTCLASwithFrameClassifierType2(char *TCLAS, u_char *UserPriority) {
	if(TCLAS == NULL || UserPriority == NULL) return false;
	*UserPriority = (u_char)TCLAS[2];
	return true;
}
bool GetClassifierMaskFromTCLASwithFrameClassifierType2(char *TCLAS, u_char *ClassifierMask) {
	if(TCLAS == NULL || ClassifierMask == NULL) return false;
	*ClassifierMask = (u_char)TCLAS[4];
	return true;
}
bool Get80211QTagTypeFromTCLASwithFrameClassifierType2(char *TCLAS, unsigned short *TagType) {
	if(TCLAS == NULL || TagType == NULL) return false;
	bcopy((void *)&TCLAS[5], (void *)TagType, 2);
	return true;
}

/*
   2007/11/03 By C.L. Chou
   TCLAS Processing Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Processing (1 octet)
*/
bool GetProcessingFromTCLASProcessing(char *TCLASProcessing, u_char *Processing) {
	if(TCLASProcessing == NULL || Processing == NULL) return false;
	*Processing = (u_char)TCLASProcessing[2];
	return true;
}

/*
   2007/11/03 By C.L. Chou
   TS Delay Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Delay (1 octet)
*/
bool GetDelayFromTSDelay(char *TSDelay, unsigned int *Delay) {
	if(TSDelay == NULL || Delay == NULL) return false;
	bcopy((void *)&TSDelay[2], (void *)Delay, 4);
	return true;
}

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
bool GetAggregationFromSchedule(char *Schedule) {
	if(Schedule == NULL) return false;
	if(Schedule[2] & 0x80)	return true;
	else			return false;
}
bool GetTSIDFromSchedule(char *Schedule, u_char *TSID) {
	if(Schedule == NULL || TSID == NULL) return false;
	*TSID = (u_char)((Schedule[2]>>3) & 0x0f);
	return true;
}
bool GetDirectionBit1FromSchedule(char *Schedule) {
	if(Schedule == NULL) return false;
	if(Schedule[2] & 0x04)	return true;
	else			return false;
}
bool GetDirectionBit2FromSchedule(char *Schedule) {
	if(Schedule == NULL) return false;
	if(Schedule[2] & 0x02)	return true;
	else			return false;
}
bool GetServiceStartTimeFromSchedule(char *Schedule, unsigned int *ServiceStartTime) {
	if(Schedule == NULL || ServiceStartTime == NULL) return false;
	bcopy((void *)&Schedule[4], (void *)ServiceStartTime, 4);
	return true;
}
bool GetServiceIntervalFromSchedule(char *Schedule, unsigned int *ServiceInterval) {
	if(Schedule == NULL || ServiceInterval == NULL) return false;
	bcopy((void *)&Schedule[8], (void *)ServiceInterval, 4);
	return true;
}
bool GetSpecificationIntervalFromSchedule(char *Schedule, unsigned short *SpecificationInterval) {
	if(Schedule == NULL || SpecificationInterval == NULL) return false;
	bcopy((void *)&Schedule[12], (void *)SpecificationInterval, 2);
	return true;
}

/*
   2007/11/03 By C.L. Chou
   Block Ack Parameter Set Field:
    -- Reserved (1 bit)
    -- Block Ack Policy (1 bit)
    -- TID (4 bits)
    -- Buffer Size (10 bits)
*/
bool GetBlockAckPolicyFromBlockAckParameterSet(char *BlockAckParameterSet) {
	if(BlockAckParameterSet == NULL) return false;
	if(BlockAckParameterSet[0] & 0x40)	return true;
	else					return false;
}
bool GetTIDFromBlockAckParameterSet(char *BlockAckParameterSet, u_char *TID) {
	if(BlockAckParameterSet == NULL || TID == NULL) return false;
	*TID = (u_char)((BlockAckParameterSet[0]>>2) & 0x0f);
	return true;
}
bool GetBufferSizeFromBlockAckParameterSet(char *BlockAckParameterSet, unsigned short *BufferSize) {
	u_char	*tmp_ptr;

	if(BlockAckParameterSet == NULL || BufferSize == NULL) return false;
	tmp_ptr = (u_char *)BufferSize; // little-endian
	tmp_ptr[1] = (u_char)(BlockAckParameterSet[0] & 0x03);
	tmp_ptr[0] = (u_char)BlockAckParameterSet[1];
	return true;
}

/*
   2007/11/03 By C.L. Chou
   Block Ack Starting Sequence Control Field:
    -- Fragment Number (4 bits)
    -- Starting Sequence Number (12 bits)
*/
bool GetFragmentNumberFromBlockAckStartingSequenceControl(char *BlockAckStartingSequenceControl, unsigned short *FragmentNumber) {
	u_char		*tmp_ptr;

	if(BlockAckStartingSequenceControl == NULL || FragmentNumber == NULL) return false;

	tmp_ptr = (u_char *)FragmentNumber; // little-endian
	tmp_ptr[1] = 0x00;
	tmp_ptr[0] =(u_char)((BlockAckStartingSequenceControl[0]>>4) & 0x0f);

	return true;
}
bool GetStartingSequenceNumberFromBlockAckStartingSequenceControl(char *BlockAckStartingSequenceControl, unsigned short *StartingSequenceNumber) {
	u_char		*tmp_ptr;

	if(BlockAckStartingSequenceControl == NULL || StartingSequenceNumber == NULL) return false;

	tmp_ptr = (u_char *)StartingSequenceNumber; // little-endian
	tmp_ptr[1] = (u_char)BlockAckStartingSequenceControl[0] & 0x0f; 
	tmp_ptr[0] = (u_char)BlockAckStartingSequenceControl[1];

	return true;
}

/*
   2007/11/03 By C.L. Chou
   DELBA Parameter Set Field:
    -- Reserved (11 bits)
    -- Initiator (1 bit)
    -- TID (4 bits)
*/
bool GetInitiatorFromDELBAParameterSet(char *DELBAParameterSet) {
	if(DELBAParameterSet == NULL) return false;
	if(DELBAParameterSet[1] & 0x10)	return true;
	else				return false;
}
bool GetTIDFromDELBAParameterSet(char *DELBAParameterSet, u_char *TID) {
	if(DELBAParameterSet == NULL || TID == NULL) return false;
	*TID = (u_char)(DELBAParameterSet[1] & 0x0f);
	return true;
}


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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eBeaconFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithTIM, bool WithQBSS, bool WithEDCA, bool WithQoS) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithFH) flag |= WITH_FH_FLAG;
	if(WithDS) flag |= WITH_DS_FLAG;
	if(WithCF) flag |= WITH_CF_FLAG;
	if(WithIBSS) flag |= WITH_IBSS_FLAG;
	if(WithTIM) flag |= WITH_TIM_FLAG;
	if(WithQBSS) flag |= WITH_QBSS_LOAD_FLAG;
	if(WithEDCA) flag |= WITH_EDCA_PARAMETER_SET_FLAG;
	if(WithQoS) flag |= WITH_QOS_CAPABILITY_FLAG;	

	return flag;
}

unsigned int Calculate80211eBeaconFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len) {
	unsigned int	len;
	
	len = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);

	if(OptionalInfoFlag & WITH_FH_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) len += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) len += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) len += 4;
	if(OptionalInfoFlag & WITH_TIM_FLAG) len += (5+PartialVirtualBitmap_Len);
	if(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_EDCA_PARAMETER_SET_FLAG) len += 20;
	if(OptionalInfoFlag & WITH_QOS_CAPABILITY_FLAG) len += 3;

	return len;
}

void SetTimestampInto80211eBeaconFrameBody(char *Body, const unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)Timestamp, (void *)ptr, 8);
	return;
}

void SetBeaconIntervalInto80211eBeaconFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)Interval, (void *)ptr, 2);
	return;
}

void SetCapabilityInfoInto80211eBeaconFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 8+2;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
} 

void SetSSIDInto80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID == NULL) return;
	offset = 8+2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SSID;
	ptr[1] = (char)SSID_Len;

	bcopy((void *)SSID, (void *)&ptr[2], (unsigned int)ptr[1]);
	return;
}

void SetSupportedRatesInto80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;
	
	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);
	return;
}

void SetFHParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DwellTime == NULL) return;
	if(!(OptionalInfoFlag & WITH_FH_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_FH_PARAMETER_SET;
	ptr[1] = 0x05;
	bcopy((void *)DwellTime, (void *)&ptr[2], 2);
	ptr[4] = HopSet;
	ptr[5] = HopPattern;
	ptr[6] = HopIndex;

	return;
}

void SetDSParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	if(!(OptionalInfoFlag & WITH_DS_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_DS_PARAMETER_SET;
	ptr[1] = 0x01;
	ptr[2] = CurrentChannel;

	return;
}

void SetCFParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration,  const unsigned short *DurRemaining) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || MaxDuration == NULL || DurRemaining == NULL) return;
	if(!(OptionalInfoFlag & WITH_CF_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_CF_PARAMETER_SET;
	ptr[1] = 0x06;
	ptr[2] = Count;
	ptr[3] = Period;
	bcopy((void *)MaxDuration, (void *)&ptr[4], 2);
	bcopy((void *)DurRemaining, (void *)&ptr[6], 2);
	
	return;
}

void SetIBSSParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ATIM_Window == NULL) return;
	if(!(OptionalInfoFlag & WITH_IBSS_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_IBSS_PARAMETER_SET;
	ptr[1] = 0x02;
	bcopy((void *)ATIM_Window, (void *)&ptr[2], 2);

	return;
}


void SetTIMInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, u_char Bitmap_Ctrl, unsigned int PartialVirtualBitmap_Len, const void *PartialVirtualBitmap) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || PartialVirtualBitmap == NULL) return;
	if(!(OptionalInfoFlag & WITH_TIM_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	ptr = Body + offset;


	ptr[0] = ELEMENT_ID_TIM;
	ptr[1] = (char)(3+PartialVirtualBitmap_Len);
	ptr[2] = Count;
	ptr[3] = Period;
	ptr[4] = Bitmap_Ctrl;
	bcopy((void *)PartialVirtualBitmap, (void *)&ptr[5], PartialVirtualBitmap_Len);

	return;
}

void SetQBSSLoadInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, const unsigned short *StationCount, u_char ChannelUtilization, const unsigned short *AvailableAdmissionCapacity) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StationCount == NULL || AvailableAdmissionCapacity == NULL) return;
	if(!(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	if(OptionalInfoFlag & WITH_TIM_FLAG) offset += (5+PartialVirtualBitmap_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_QBSS_LOAD;
	ptr[1] = 0x05;
	bcopy((void *)StationCount, (void *)&ptr[2], 2);
	ptr[4] = ChannelUtilization;
	bcopy((void *)AvailableAdmissionCapacity, (void *)&ptr[5], 2);

	return;
}


void SetEDCAParameterSetInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AC_BE_TXOP_Limit == NULL || AC_BK_TXOP_Limit == NULL || AC_VI_TXOP_Limit == NULL || AC_VO_TXOP_Limit == NULL) return;
	if(!(OptionalInfoFlag & WITH_EDCA_PARAMETER_SET_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	if(OptionalInfoFlag & WITH_TIM_FLAG) offset += (5+PartialVirtualBitmap_Len);
	if(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG) offset += 7;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EDCA_PARAMETER_SET;
	ptr[1] = 0x12;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	ptr[4] = (AC_BE_AIFSN<<4) & 0xf0; 
	if(AC_BE_ACM)	ptr[4] |= 0x08;
	else		ptr[4] &= 0xf7; 
	ptr[4] &= 0xfb; // ACI = "0"0
	ptr[4] &= 0xfd; // ACI = 0"0"
	ptr[5] = AC_BE_ECW;
	bcopy((void *)AC_BE_TXOP_Limit, (void *)&ptr[6], 2);

	ptr[8] = (AC_BK_AIFSN<<4) & 0xf0; 
	if(AC_BK_ACM)	ptr[8] |= 0x08;
	else		ptr[8] &= 0xf7; 
	ptr[8] &= 0xfb; // ACI = "0"1
	ptr[8] |= 0x02; // ACI = 0"1"
	ptr[9] = AC_BK_ECW;
	bcopy((void *)AC_BK_TXOP_Limit, (void *)&ptr[10], 2);

	ptr[12] = (AC_VI_AIFSN<<4) & 0xf0; 
	if(AC_VI_ACM)	ptr[12] |= 0x08;
	else		ptr[12] &= 0xf7; 
	ptr[12] |= 0x04; // ACI = "1"0
	ptr[12] &= 0xfd; // ACI = 1"0"
	ptr[13] = AC_VI_ECW;
	bcopy((void *)AC_VI_TXOP_Limit, (void *)&ptr[14], 2);

	ptr[16] = (AC_VO_AIFSN<<4) & 0xf0; 
	if(AC_VO_ACM)	ptr[16] |= 0x08;
	else		ptr[16] &= 0xf7; 
	ptr[16] |= 0x04; // ACI = "1"1
	ptr[16] |= 0x02; // ACI = 1"1"
	ptr[17] = AC_VO_ECW;
	bcopy((void *)AC_VO_TXOP_Limit, (void *)&ptr[18], 2);

	return;
}


void SetQoSCapabilityInto80211eBeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	if(!(OptionalInfoFlag & WITH_QOS_CAPABILITY_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	if(OptionalInfoFlag & WITH_TIM_FLAG) offset += (5+PartialVirtualBitmap_Len);
	if(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_EDCA_PARAMETER_SET_FLAG) offset += 20;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_QOS_CAPABILITY;
	ptr[1] = 0x01;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	return;
}


void GetTimestampFrom80211eBeaconFrameBody(char *Body, unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Timestamp, 8);

	return;
}

void GetBeaconIntervalFrom80211eBeaconFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);
	
	return;
}

char *GetCapabilityInfoFrom80211eBeaconFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2;
	ptr = Body + offset;
	return ptr;
}

void *GetSSIDFrom80211eBeaconFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 8+2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetFHParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetDSParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;

	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetCFParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;

	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetIBSSParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;

	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetTIMFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;

	if(ptr[0] == ELEMENT_ID_TIM)			return (char *)ptr;
	else						return NULL;
}


char *GetQBSSLoadFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;
	if(ptr[0] == ELEMENT_ID_TIM)			ptr += (5+PartialVirtualBitmap_Len);

	if(ptr[0] == ELEMENT_ID_QBSS_LOAD)		return (char *)ptr;
	else						return NULL;
}

char *GetEDCAParameterSetFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;
	if(ptr[0] == ELEMENT_ID_TIM)			ptr += (5+PartialVirtualBitmap_Len);
	if(ptr[0] == ELEMENT_ID_QBSS_LOAD)		ptr += 7;

	if(ptr[0] == ELEMENT_ID_EDCA_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetQoSCapabilityFrom80211eBeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;
	if(ptr[0] == ELEMENT_ID_TIM)			ptr += (5+PartialVirtualBitmap_Len);
	if(ptr[0] == ELEMENT_ID_QBSS_LOAD)		ptr += 7;
	if(ptr[0] == ELEMENT_ID_EDCA_PARAMETER_SET)	ptr += 20;

	if(ptr[0] == ELEMENT_ID_QOS_CAPABILITY)		return (char *)ptr;
	else						return NULL;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Disassociation frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDisassociationFrameBodyLength() {
	unsigned int	len;

	len = 2;
	return len;
}

void SetReasonCodeInto80211eDisassociationFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);
	
	return;
}

void GetReasonCodeFrom80211eDisassociationFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Association Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- QoS Capability (3 octets)
*/
unsigned int Calculate80211eAssociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+(2+SSID_Len)+(2+SupportedRates_Len)+3;
	return len;
}

void SetCapabilityInfoInto80211eAssociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 0;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetListenIntervalInto80211eAssociationRequestFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)Interval, (void *)ptr, 2);

	return;
}

void SetSSIDInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SSID;
	ptr[1] = (char)SSID_Len;

	bcopy((void *)SSID, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetSupportedRatesInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 2+2+(2+SSID_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;

	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetQoSCapabilityInto80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_QOS_CAPABILITY;
	ptr[1] = 0x01;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	return;
}


char *GetCapabilityInfoFrom80211eAssociationRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetListenIntervalFrom80211eAssociationRequestFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);

	return;
}

void *GetSSIDFrom80211eAssociationRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211eAssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetQoSCapabilityFrom80211eAssociationRequestFrameBody(char *Body,  unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	return (char *)ptr;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Association Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- EDCA Parameter Set (20 octets)
*/
unsigned int Calculate80211eAssociationResponseFrameBodyLength(unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+2+(2+SupportedRates_Len)+20;
	return len;
}

void SetCapabilityInfoInto80211eAssociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 0;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetStatusCodeInto80211eAssociationResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetAIDInto80211eAssociationResponseFrameBody(char *Body, const unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)AID, (void *)ptr, 2);

	return;
}

void SetSupportedRatesInto80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 2+2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;

	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetEDCAParameterSetInto80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AC_BE_TXOP_Limit == NULL || AC_BK_TXOP_Limit == NULL || AC_VI_TXOP_Limit == NULL || AC_VO_TXOP_Limit == NULL) return;
	offset = 2+2+2+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EDCA_PARAMETER_SET;
	ptr[1] = 0x12;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	ptr[4] = (AC_BE_AIFSN<<4) & 0xf0; 
	if(AC_BE_ACM)	ptr[4] |= 0x08;
	else		ptr[4] &= 0xf7; 
	ptr[4] &= 0xfb; // ACI = "0"0
	ptr[4] &= 0xfd; // ACI = 0"0"
	ptr[5] = AC_BE_ECW;
	bcopy((void *)AC_BE_TXOP_Limit, (void *)&ptr[6], 2);

	ptr[8] = (AC_BK_AIFSN<<4) & 0xf0; 
	if(AC_BK_ACM)	ptr[8] |= 0x08;
	else		ptr[8] &= 0xf7; 
	ptr[8] &= 0xfb; // ACI = "0"1
	ptr[8] |= 0x02; // ACI = 0"1"
	ptr[9] = AC_BK_ECW;
	bcopy((void *)AC_BK_TXOP_Limit, (void *)&ptr[10], 2);

	ptr[12] = (AC_VI_AIFSN<<4) & 0xf0; 
	if(AC_VI_ACM)	ptr[12] |= 0x08;
	else		ptr[12] &= 0xf7; 
	ptr[12] |= 0x04; // ACI = "1"0
	ptr[12] &= 0xfd; // ACI = 1"0"
	ptr[13] = AC_VI_ECW;
	bcopy((void *)AC_VI_TXOP_Limit, (void *)&ptr[14], 2);

	ptr[16] = (AC_VO_AIFSN<<4) & 0xf0; 
	if(AC_VO_ACM)	ptr[16] |= 0x08;
	else		ptr[16] &= 0xf7; 
	ptr[16] |= 0x04; // ACI = "1"1
	ptr[16] |= 0x02; // ACI = 1"1"
	ptr[17] = AC_VO_ECW;
	bcopy((void *)AC_VO_TXOP_Limit, (void *)&ptr[18], 2);

	return;
} 

char *GetCapabilityInfoFrom80211eAssociationResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetStatusCodeFrom80211eAssociationResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

void GetAIDFrom80211eAssociationResponseFrameBody(char *Body, unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AID, 2);

	return;
}

void *GetSupportedRatesFrom80211eAssociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+2;
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetEDCAParameterSetFrom80211eAssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 2+2+2+(2+SupportedRates_Len);
	ptr = Body + offset;

	return (char *)ptr;
}


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
unsigned int Calculate80211eReassociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+6+(2+SSID_Len)+(2+SupportedRates_Len)+3;
	return len;
}

void SetCapabilityInfoInto80211eReassociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 0;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetListenIntervalInto80211eReassociationRequestFrameBody(char *Body, const unsigned short *ListenInterval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ListenInterval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ListenInterval, (void *)ptr, 2);

	return;
}

void SetCurrentAPAddressInto80211eReassociationRequestFrameBody(char *Body, const void * CurrentAPAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || CurrentAPAddress == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)CurrentAPAddress, (void *)ptr, 6);

	return;
}

void SetSSIDInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID == NULL) return;
	offset = 2+2+6;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SSID;
	ptr[1] = (char)SSID_Len;

	bcopy((void *)SSID, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetSupportedRatesInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 2+2+6+(2+SSID_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;

	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetQoSCapabilityInto80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 2+2+6+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_QOS_CAPABILITY;
	ptr[1] = 0x01;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	return;
}

char *GetCapabilityInfoFrom80211eReassociationRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetListenIntervalFrom80211eReassociationRequestFrameBody(char *Body, unsigned short *ListenInterval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ListenInterval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ListenInterval, 2);

	return;
}

void GetCurrentAPAddressFrom80211eReassociationRequestFrameBody(char *Body, void * CurrentAPAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || CurrentAPAddress == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)CurrentAPAddress, 6);

	return;
}

void *GetSSIDFrom80211eReassociationRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 2+2+6;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211eReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+6+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetQoSCapabilityFrom80211eReassociationRequestFrameBody(char *Body,  unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 2+2+6+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	return (char *)ptr;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Reassociation Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- EDCA Parameter Set (20 octets)
*/
unsigned int Calculate80211eReassociationResponseFrameBodyLength(unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+2+(2+SupportedRates_Len)+20;
	return len;
}

void SetCapabilityInfoInto80211eReassociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 0;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetStatusCodeInto80211eReassociationResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetAIDInto80211eReassociationResponseFrameBody(char *Body, const unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)AID, (void *)ptr, 2);

	return;
}

void SetSupportedRatesInto80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 2+2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;

	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetEDCAParameterSetInto80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AC_BE_TXOP_Limit == NULL || AC_BK_TXOP_Limit == NULL || AC_VI_TXOP_Limit == NULL || AC_VO_TXOP_Limit == NULL) return;
	offset = 2+2+2+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EDCA_PARAMETER_SET;
	ptr[1] = 0x12;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	ptr[4] = (AC_BE_AIFSN<<4) & 0xf0; 
	if(AC_BE_ACM)	ptr[4] |= 0x08;
	else		ptr[4] &= 0xf7; 
	ptr[4] &= 0xfb; // ACI = "0"0
	ptr[4] &= 0xfd; // ACI = 0"0"
	ptr[5] = AC_BE_ECW;
	bcopy((void *)AC_BE_TXOP_Limit, (void *)&ptr[6], 2);

	ptr[8] = (AC_BK_AIFSN<<4) & 0xf0; 
	if(AC_BK_ACM)	ptr[8] |= 0x08;
	else		ptr[8] &= 0xf7; 
	ptr[8] &= 0xfb; // ACI = "0"1
	ptr[8] |= 0x02; // ACI = 0"1"
	ptr[9] = AC_BK_ECW;
	bcopy((void *)AC_BK_TXOP_Limit, (void *)&ptr[10], 2);

	ptr[12] = (AC_VI_AIFSN<<4) & 0xf0; 
	if(AC_VI_ACM)	ptr[12] |= 0x08;
	else		ptr[12] &= 0xf7; 
	ptr[12] |= 0x04; // ACI = "1"0
	ptr[12] &= 0xfd; // ACI = 1"0"
	ptr[13] = AC_VI_ECW;
	bcopy((void *)AC_VI_TXOP_Limit, (void *)&ptr[14], 2);

	ptr[16] = (AC_VO_AIFSN<<4) & 0xf0; 
	if(AC_VO_ACM)	ptr[16] |= 0x08;
	else		ptr[16] &= 0xf7; 
	ptr[16] |= 0x04; // ACI = "1"1
	ptr[16] |= 0x02; // ACI = 1"1"
	ptr[17] = AC_VO_ECW;
	bcopy((void *)AC_VO_TXOP_Limit, (void *)&ptr[18], 2);

	return;
} 

char *GetCapabilityInfoFrom80211eReassociationResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetStatusCodeFrom80211eReassociationResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

void GetAIDFrom80211eReassociationResponseFrameBody(char *Body, unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AID, 2);

	return;
}

void *GetSupportedRatesFrom80211eReassociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+2;
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetEDCAParameterSetFrom80211eReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 2+2+2+(2+SupportedRates_Len);
	ptr = Body + offset;

	return (char *)ptr;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Probe Request frame body:

    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211eProbeRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = (2+SSID_Len)+(2+SupportedRates_Len);
	return len;
}

void SetSSIDInto80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID == NULL) return;
	offset = 0;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SSID;
	ptr[1] = (char)SSID_Len;

	bcopy((void *)SSID, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetSupportedRatesInto80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = (2+SSID_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;

	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void *GetSSIDFrom80211eProbeRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211eProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = (2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}


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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eProbeResponseFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithQBSS, bool WithEDCA) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithFH) flag |= WITH_FH_FLAG;
	if(WithDS) flag |= WITH_DS_FLAG;
	if(WithCF) flag |= WITH_CF_FLAG;
	if(WithIBSS) flag |= WITH_IBSS_FLAG;
	if(WithQBSS) flag |= WITH_QBSS_LOAD_FLAG;
	if(WithEDCA) flag |= WITH_EDCA_PARAMETER_SET_FLAG;

	return flag;
}

unsigned int Calculate80211eProbeResponseFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;
	
	len = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);

	if(OptionalInfoFlag & WITH_FH_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) len += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) len += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) len += 4;
	if(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_EDCA_PARAMETER_SET_FLAG) len += 20;

	return len;
}

void SetTimestampInto80211eProbeResponseFrameBody(char *Body, const unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)Timestamp, (void *)ptr, 8);

	return;
}

void SetBeaconIntervalInto80211eProbeResponseFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)Interval, (void *)ptr, 2);

	return;
}

void SetCapabilityInfoInto80211eProbeResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 8+2;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
} 

void SetSSIDInto80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID == NULL) return;
	offset = 8+2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SSID;
	ptr[1] = (char)SSID_Len;

	bcopy((void *)SSID, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetSupportedRatesInto80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;
	
	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetFHParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DwellTime == NULL) return;
	if(!(OptionalInfoFlag & WITH_FH_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_FH_PARAMETER_SET;
	ptr[1] = 0x05;
	bcopy((void *)DwellTime, (void *)&ptr[2], 2);
	ptr[4] = HopSet;
	ptr[5] = HopPattern;
	ptr[6] = HopIndex;

	return;
}

void SetDSParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	if(!(OptionalInfoFlag & WITH_DS_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_DS_PARAMETER_SET;
	ptr[1] = 0x01;
	ptr[2] = CurrentChannel;

	return;
}

void SetCFParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration,  const unsigned short *DurRemaining) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || MaxDuration == NULL || DurRemaining == NULL) return;
	if(!(OptionalInfoFlag & WITH_CF_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_CF_PARAMETER_SET;
	ptr[1] = 0x06;
	ptr[2] = Count;
	ptr[3] = Period;
	bcopy((void *)MaxDuration, (void *)&ptr[4], 2);
	bcopy((void *)DurRemaining, (void *)&ptr[6], 2);

	return;
}

void SetIBSSParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ATIM_Window == NULL) return;
	if(!(OptionalInfoFlag & WITH_IBSS_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_IBSS_PARAMETER_SET;
	ptr[1] = 0x02;
	bcopy((void *)ATIM_Window, (void *)&ptr[2], 2);

	return;
}

void SetQBSSLoadInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *StationCount, u_char ChannelUtilization, const unsigned short *AvailableAdmissionCapacity) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StationCount == NULL || AvailableAdmissionCapacity == NULL) return;
	if(!(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_QBSS_LOAD;
	ptr[1] = 0x05;
	bcopy((void *)StationCount, (void *)&ptr[2], 2);
	ptr[4] = ChannelUtilization;
	bcopy((void *)AvailableAdmissionCapacity, (void *)&ptr[5], 2);

	return;
}

void SetEDCAParameterSetInto80211eProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char QoSInfo_EDCAParameterSetUpdateCount, bool QoSInfo_QAck, bool QoSInfo_QueueRequest, bool QoSInfo_TXOPRequest, u_char AC_BE_AIFSN, bool AC_BE_ACM, u_char AC_BE_ECW, const unsigned short *AC_BE_TXOP_Limit, u_char AC_BK_AIFSN, bool AC_BK_ACM, u_char AC_BK_ECW, const unsigned short *AC_BK_TXOP_Limit, u_char AC_VI_AIFSN, bool AC_VI_ACM, u_char AC_VI_ECW, const unsigned short *AC_VI_TXOP_Limit, u_char AC_VO_AIFSN, bool AC_VO_ACM, u_char AC_VO_ECW, const unsigned short *AC_VO_TXOP_Limit) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AC_BE_TXOP_Limit == NULL || AC_BK_TXOP_Limit == NULL || AC_VI_TXOP_Limit == NULL || AC_VO_TXOP_Limit == NULL) return;
	if(!(OptionalInfoFlag & WITH_EDCA_PARAMETER_SET_FLAG)) return;

	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	if(OptionalInfoFlag & WITH_FH_FLAG) offset += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) offset += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) offset += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) offset += 4;
	if(OptionalInfoFlag & WITH_QBSS_LOAD_FLAG) offset += 7;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EDCA_PARAMETER_SET;
	ptr[1] = 0x12;

	ptr[2] = (QoSInfo_EDCAParameterSetUpdateCount<<4) & 0xf0;
	if(QoSInfo_QAck)		ptr[2] |= 0x08;
	else				ptr[2] &= 0xf7;
	if(QoSInfo_QueueRequest)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(QoSInfo_TXOPRequest)		ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	ptr[4] = (AC_BE_AIFSN<<4) & 0xf0; 
	if(AC_BE_ACM)	ptr[4] |= 0x08;
	else		ptr[4] &= 0xf7; 
	ptr[4] &= 0xfb; // ACI = "0"0
	ptr[4] &= 0xfd; // ACI = 0"0"
	ptr[5] = AC_BE_ECW;
	bcopy((void *)AC_BE_TXOP_Limit, (void *)&ptr[6], 2);

	ptr[8] = (AC_BK_AIFSN<<4) & 0xf0; 
	if(AC_BK_ACM)	ptr[8] |= 0x08;
	else		ptr[8] &= 0xf7; 
	ptr[8] &= 0xfb; // ACI = "0"1
	ptr[8] |= 0x02; // ACI = 0"1"
	ptr[9] = AC_BK_ECW;
	bcopy((void *)AC_BK_TXOP_Limit, (void *)&ptr[10], 2);

	ptr[12] = (AC_VI_AIFSN<<4) & 0xf0; 
	if(AC_VI_ACM)	ptr[12] |= 0x08;
	else		ptr[12] &= 0xf7; 
	ptr[12] |= 0x04; // ACI = "1"0
	ptr[12] &= 0xfd; // ACI = 1"0"
	ptr[13] = AC_VI_ECW;
	bcopy((void *)AC_VI_TXOP_Limit, (void *)&ptr[14], 2);

	ptr[16] = (AC_VO_AIFSN<<4) & 0xf0; 
	if(AC_VO_ACM)	ptr[16] |= 0x08;
	else		ptr[16] &= 0xf7; 
	ptr[16] |= 0x04; // ACI = "1"1
	ptr[16] |= 0x02; // ACI = 1"1"
	ptr[17] = AC_VO_ECW;
	bcopy((void *)AC_VO_TXOP_Limit, (void *)&ptr[18], 2);

	return;
}


void GetTimestampFrom80211eProbeResponseFrameBody(char *Body, unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Timestamp, 8);

	return;
}

void GetBeaconIntervalFrom80211eProbeResponseFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);

	return;
}

char *GetCapabilityInfoFrom80211eProbeResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2;
	ptr = Body + offset;
	return ptr;
}

void *GetSSIDFrom80211eProbeResponseFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 8+2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetFHParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetDSParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;

	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetCFParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;

	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetIBSSParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;

	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetQBSSLoadFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;

	if(ptr[0] == ELEMENT_ID_QBSS_LOAD)		return (char *)ptr;
	else						return NULL;
}

char *GetEDCAParameterSetFrom80211eProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;
	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	ptr += 3;
	if(ptr[0] == ELEMENT_ID_CF_PARAMETER_SET)	ptr += 8;
	if(ptr[0] == ELEMENT_ID_IBSS_PARAMETER_SET)	ptr += 4;
	if(ptr[0] == ELEMENT_ID_QBSS_LOAD)		ptr += 7;

	if(ptr[0] == ELEMENT_ID_EDCA_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Authentication frame body:

    -- Authentication Algorithm Number (2 octets)
    -- Authentication Transaction Sequence Number (2 octets)
    -- Status Code (2 octets)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text) <optional>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eAuthenticationFrameBody(bool WithChallengeText) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithChallengeText) flag |= WITH_CHALLENGE_TEXT_FLAG;

	return flag;
}

unsigned int Calculate80211eAuthenticationFrameBodyLength(u_char OptionalInfoFlag, unsigned int ChallengeText_Len) {
	unsigned int	len;
	
	len = 2+2+2;

	if(OptionalInfoFlag & WITH_CHALLENGE_TEXT_FLAG) len += ChallengeText_Len;

	return len;
}

void SetAuthAlgoNumInto80211eAuthenticationFrameBody(char *Body, const unsigned short *AuthAlgoNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthAlgoNum == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)AuthAlgoNum, (void *)ptr, 2);

	return;
}

void SetAuthTransSeqNumInto80211eAuthenticationFrameBody(char *Body, const unsigned short *AuthTransSeqNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthTransSeqNum == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)AuthTransSeqNum, (void *)ptr, 2);

	return;
}

void SetStatusCodeInto80211eAuthenticationFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetChallengeTextInto80211eAuthenticationFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int ChallengeText_Len, const void *ChallengeText) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ChallengeText == NULL) return;
	if(!(OptionalInfoFlag & WITH_CHALLENGE_TEXT_FLAG)) return;

	offset = 2+2+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_CHALLENGE_TEXT;
	ptr[1] = (char)ChallengeText_Len;

	bcopy((void *)ChallengeText, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void GetAuthAlgoNumFrom80211eAuthenticationFrameBody(char *Body, unsigned short *AuthAlgoNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthAlgoNum == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AuthAlgoNum, 2);

	return;
}

void GetAuthTransSeqNumFrom80211eAuthenticationFrameBody(char *Body, unsigned short *AuthTransSeqNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthTransSeqNum == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AuthTransSeqNum, 2);

	return;
}

void GetStatusCodeFrom80211eAuthenticationFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

char *GetChallengeTextFrom80211eAuthenticationFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 2+2+2;
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_CHALLENGE_TEXT) return (char *)ptr;
	else					return NULL;
}



/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Deauthentication frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211eDeauthenticationFrameBodyLength() {
	unsigned int	len;

	len = 2;
	return len;
}

void SetReasonCodeInto80211eDeauthenticationFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

void GetReasonCodeFrom80211eDeauthenticationFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}



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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSRequestFrameBody(bool WithTCLAS, enum TCLAS_FrameClassifierType TCLASFrameClassifierType, bool WithTCLASProcessing) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithTCLAS){
		if(TCLASFrameClassifierType == ClassifierType_0) flag |= WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_1_IPv4) flag |= WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_1_IPv6) flag |= WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_2) flag |= WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG;
	}

	if(WithTCLASProcessing) flag |= WITH_TCLAS_PROCESSING_FLAG;

	return flag;
}

unsigned int Calculate80211eADDTSRequestFrameBodyLength(u_char OptionalInfoFlag) {
	unsigned int	len;
	
	len = 1+1+1+57;

	if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG) len += (2+1+16); 
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG) len += (2+1+18);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG) len += (2+1+42);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG) len += (2+1+4);

	if(OptionalInfoFlag & WITH_TCLAS_PROCESSING_FLAG) len += 3;

	return len;
}

void SetDialogTokenInto80211eADDTSRequestFrameBody(char *Body, u_char DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = (char)DialogToken;
	return; 
}

void SetTSPECInto80211eADDTSRequestFrameBody(char *Body, bool TSInfo_TrafficType, u_char TSInfo_TSID, bool TSInfo_Direction_Bit_1, bool TSInfo_Direction_Bit_2, bool TSInfo_AccessPolicy_Bit_1, bool TSInfo_AccessPolicy_Bit_2, bool TSInfo_Aggregation, bool TSInfo_APSD, u_char TSInfo_UserPriority, bool TSInfo_TSInfoAckPolicy_Bit_1, bool TSInfo_TSInfoAckPolicy_Bit_2, bool TSInfo_Schedule, const unsigned short *NominalMSDUSize, bool NominalMSDUSize_Fixed, const unsigned short *MaximumMSDUSize, const unsigned int *MinimumServiceInterval, const unsigned int *MaximumServiceInterval, const unsigned int *InactivityInterval, const unsigned int *SyspensionInterval, const unsigned int *ServiceStartTime, const unsigned int *MinimumDataRate, const unsigned int *MeanDataRate, const unsigned int *PeakDataRate, const unsigned int *BurstSize, const unsigned int *DelayBound, const unsigned int *MinimumPHYRate, const unsigned short *SurplusBandwidthAllowance_IntegerPart, const unsigned short *SurplusBandwidthAllowance_DecimalPart, const unsigned short *MediumTime) {
	char		*ptr;
	unsigned int	offset;
	u_char		*tmp_ptr;
	u_char		char_buf[2];

	if(Body == NULL || MaximumMSDUSize == NULL || MinimumServiceInterval == NULL || MaximumServiceInterval == NULL || InactivityInterval == NULL || SyspensionInterval == NULL || ServiceStartTime == NULL || MinimumDataRate == NULL || MeanDataRate == NULL || PeakDataRate == NULL || BurstSize == NULL || DelayBound == NULL || MinimumPHYRate == NULL || MediumTime == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TSPEC;
	ptr[1] = 0x37; //55

	ptr[2] = (TSInfo_TSID<<3) & 0x78;

	if(TSInfo_TrafficType)	ptr[2] |= 0x80;
	else			ptr[2] &= 0x7f;

	if(TSInfo_Direction_Bit_1)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(TSInfo_Direction_Bit_2)	ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	if(TSInfo_AccessPolicy_Bit_1)	ptr[2] |= 0x01;
	else				ptr[2] &= 0xfe;

	ptr[3] = (TSInfo_UserPriority<<2) & 0x1c;

	if(TSInfo_AccessPolicy_Bit_2)	ptr[3] |= 0x80;
	else				ptr[3] &= 0x7f;

	if(TSInfo_Aggregation)		ptr[3] |= 0x40;
	else				ptr[3] &= 0xbf;

	if(TSInfo_APSD)			ptr[3] |= 0x20;
	else				ptr[3] &= 0xdf;

	if(TSInfo_TSInfoAckPolicy_Bit_1)	ptr[3] |= 0x02;
	else					ptr[3] &= 0xfd;

	if(TSInfo_TSInfoAckPolicy_Bit_2)	ptr[3] |= 0x01;
	else					ptr[3] &= 0xfe;

	if(TSInfo_Schedule)		ptr[4] |= 0x80;
	else				ptr[4] &= 0x7f;

	tmp_ptr = (u_char *)NominalMSDUSize;	// little-endian
	char_buf[0] = tmp_ptr[0];
	char_buf[1] = tmp_ptr[1];
	if(tmp_ptr[0] & 0x80)	char_buf[1] = (char_buf[1]<<1) | 0x01;
	else			char_buf[1] = (char_buf[1]<<1) & 0xfe;
	if(NominalMSDUSize_Fixed)	char_buf[0] = (char_buf[0]<<1) | 0x01;
	else				char_buf[0] = (char_buf[0]<<1) & 0xfe;
	ptr[5] = (char)char_buf[1];
	ptr[6] = (char)char_buf[0];

	bcopy((void *)MaximumMSDUSize, (void *)&ptr[7], 2);
	bcopy((void *)MinimumServiceInterval, (void *)&ptr[9], 4);
	bcopy((void *)MaximumServiceInterval, (void *)&ptr[13], 4);
	bcopy((void *)InactivityInterval, (void *)&ptr[17], 4);
	bcopy((void *)SyspensionInterval, (void *)&ptr[21], 4);
	bcopy((void *)ServiceStartTime, (void *)&ptr[25], 4);
	bcopy((void *)MinimumDataRate, (void *)&ptr[29], 4);
	bcopy((void *)MeanDataRate, (void *)&ptr[33], 4);
	bcopy((void *)PeakDataRate, (void *)&ptr[37], 4);
	bcopy((void *)BurstSize, (void *)&ptr[41], 4);
	bcopy((void *)DelayBound, (void *)&ptr[45], 4);
	bcopy((void *)MinimumPHYRate, (void *)&ptr[49], 4);

	tmp_ptr = (u_char *)SurplusBandwidthAllowance_IntegerPart; // little-endian
	char_buf[0] = (tmp_ptr[0]<<5) & 0xe0;
	tmp_ptr = (u_char *)SurplusBandwidthAllowance_DecimalPart; // little-endian
	char_buf[0] |= (tmp_ptr[1] & 0x1f);
	char_buf[1] = tmp_ptr[0];
	ptr[53] = (char)char_buf[0];
	ptr[54] = (char)char_buf[1];

	bcopy((void *)MediumTime, (void *)&ptr[55], 2);

	return;
}

void SetTCLASwithFrameClassifierType0Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const void *SourceAddress, const void *DestinationAddress, const unsigned short *Type) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceAddress == NULL || DestinationAddress == NULL || Type == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG)) return;
	offset = 1+1+1+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x11; //1+16
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x00;
	ptr[4] = (char)ClassifierMask;

	bcopy((void *)SourceAddress, (void *)&ptr[5], 6);
	bcopy((void *)DestinationAddress, (void *)&ptr[11], 6);
	bcopy((void *)Type, (void *)&ptr[17], 2);

	return;
}

void SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, u_char DSCP, u_char Protocol) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceIPAddress == NULL || DestinationIPAddress == NULL || SourcePort == NULL || DestinationPort == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG)) return;
	offset = 1+1+1+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x13; //1+18
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x01;
	ptr[4] = (char)ClassifierMask;
	ptr[5] = (char)Version;

	bcopy((void *)SourceIPAddress, (void *)&ptr[6], 4);
	bcopy((void *)DestinationIPAddress, (void *)&ptr[10], 4);
	bcopy((void *)SourcePort, (void *)&ptr[14], 2);
	bcopy((void *)DestinationPort, (void *)&ptr[16], 2);

	ptr[18] = (char)(DSCP & 0x3f);
	ptr[19] = (char)Protocol;

	return;
}

void SetTCLASwithFrameClassifierType1ForIPv6Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, const void *FlowLabel) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceIPAddress == NULL || DestinationIPAddress == NULL || SourcePort == NULL || DestinationPort == NULL || FlowLabel == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG)) return;
	offset = 1+1+1+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x2b; //1+42
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x01;
	ptr[4] = (char)ClassifierMask;
	ptr[5] = (char)Version;

	bcopy((void *)SourceIPAddress, (void *)&ptr[6], 16);
	bcopy((void *)DestinationIPAddress, (void *)&ptr[22], 16);
	bcopy((void *)SourcePort, (void *)&ptr[38], 2);
	bcopy((void *)DestinationPort, (void *)&ptr[40], 2);
	bcopy((void *)FlowLabel, (void *)&ptr[42], 3);

	return;
}

void SetTCLASwithFrameClassifierType2Into80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const unsigned short *TagType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || TagType == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG)) return;
	offset = 1+1+1+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x05; //1+4
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x02;
	ptr[4] = (char)ClassifierMask;

	bcopy((void *)TagType, (void *)&ptr[5], 2);

	return;
}

void SetTCLASProcessingInto80211eADDTSRequestFrameBody(char *Body, u_char OptionalInfoFlag, u_char Processing) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_PROCESSING_FLAG)) return;
	offset = 1+1+1+57;
	if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG) offset += (2+1+16); 
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG) offset += (2+1+18);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG) offset += (2+1+42);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG) offset += (2+1+4);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS_PROCESSING;
	ptr[1] = 0x01;
	ptr[2] = (char)Processing;

	return;
}

void GetDialogTokenFrom80211eADDTSRequestFrameBody(char *Body, u_char *DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DialogToken == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	*DialogToken = (u_char)ptr[0];
	return; 
}

char *GetTSPECFrom80211eADDTSRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1;
	ptr = Body + offset;

	return (char *)ptr;
}

char *GetTCLASFrom80211eADDTSRequestFrameBody(char *Body, u_char *FrameClassifierType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || FrameClassifierType == NULL) return NULL;
	offset = 1+1+1+57;
	ptr = Body + offset;

	if(ptr[0] != ELEMENT_ID_TCLAS) return NULL;

	*FrameClassifierType = (u_char)ptr[3];
	return (char *)ptr;
}

char *GetTCLASProcessingFrom80211eADDTSRequestFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+57;
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_TCLAS) {
		if(FrameClassifierType == ClassifierType_0)		ptr += (2+1+16); 
		else if(FrameClassifierType == ClassifierType_1_IPv4)	ptr += (2+1+18);
		else if(FrameClassifierType == ClassifierType_1_IPv6)	ptr += (2+1+42); 
		else if(FrameClassifierType == ClassifierType_2)	ptr += (2+1+4);
	}

	if(ptr[0] == ELEMENT_ID_TCLAS_PROCESSING) 	return (char *)ptr;
	else						return NULL;
}



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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSResponseFrameBody(bool WithTCLAS, enum TCLAS_FrameClassifierType TCLASFrameClassifierType, bool WithTCLASProcessing) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithTCLAS) {
		if(TCLASFrameClassifierType == ClassifierType_0) flag |= WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_1_IPv4) flag |= WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_1_IPv6) flag |= WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG;
		else if(TCLASFrameClassifierType == ClassifierType_2) flag |= WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG;
	}

	if(WithTCLASProcessing) flag |= WITH_TCLAS_PROCESSING_FLAG;

	return flag;
}

unsigned int Calculate80211eADDTSResponseFrameBodyLength(u_char OptionalInfoFlag) {
	unsigned int	len;
	
	len = 1+1+1+2+6+57;

	if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG) len += (2+1+16); 
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG) len += (2+1+18);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG) len += (2+1+42);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG) len += (2+1+4);

	if(OptionalInfoFlag & WITH_TCLAS_PROCESSING_FLAG) len += 3;

	len += 14;

	return len;
}

void SetDialogTokenInto80211eADDTSResponseFrameBody(char *Body, u_char DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = (char)DialogToken;
	return; 
}

void SetStatusCodeInto80211eADDTSResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetTSDelayInto80211eADDTSResponseFrameBody(char *Body, const unsigned int *Delay) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Delay == NULL) return;
	offset = 1+1+1+2;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TS_DELAY;
	ptr[1] = 0x04;
	bcopy((void *)Delay, (void *)&ptr[2], 4);

	return;
}

void SetTSPECInto80211eADDTSResponseFrameBody(char *Body, bool TSInfo_TrafficType, u_char TSInfo_TSID, bool TSInfo_Direction_Bit_1, bool TSInfo_Direction_Bit_2, bool TSInfo_AccessPolicy_Bit_1, bool TSInfo_AccessPolicy_Bit_2, bool TSInfo_Aggregation, bool TSInfo_APSD, u_char TSInfo_UserPriority, bool TSInfo_TSInfoAckPolicy_Bit_1, bool TSInfo_TSInfoAckPolicy_Bit_2, bool TSInfo_Schedule, const unsigned short *NominalMSDUSize, bool NominalMSDUSize_Fixed, const unsigned short *MaximumMSDUSize, const unsigned int *MinimumServiceInterval, const unsigned int *MaximumServiceInterval, const unsigned int *InactivityInterval, const unsigned int *SyspensionInterval, const unsigned int *ServiceStartTime, const unsigned int *MinimumDataRate, const unsigned int *MeanDataRate, const unsigned int *PeakDataRate, const unsigned int *BurstSize, const unsigned int *DelayBound, const unsigned int *MinimumPHYRate, const unsigned short *SurplusBandwidthAllowance_IntegerPart, const unsigned short *SurplusBandwidthAllowance_DecimalPart, const unsigned short *MediumTime) {
	char		*ptr;
	unsigned int	offset;
	u_char		*tmp_ptr;
	u_char		char_buf[2];

	if(Body == NULL || MaximumMSDUSize == NULL || MinimumServiceInterval == NULL || MaximumServiceInterval == NULL || InactivityInterval == NULL || SyspensionInterval == NULL || ServiceStartTime == NULL || MinimumDataRate == NULL || MeanDataRate == NULL || PeakDataRate == NULL || BurstSize == NULL || DelayBound == NULL || MinimumPHYRate == NULL || MediumTime == NULL) return;
	offset = 1+1+1+2+6;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TSPEC;
	ptr[1] = 0x37; //55

	ptr[2] = (TSInfo_TSID<<3) & 0x78;

	if(TSInfo_TrafficType)	ptr[2] |= 0x80;
	else			ptr[2] &= 0x7f;

	if(TSInfo_Direction_Bit_1)	ptr[2] |= 0x04;
	else				ptr[2] &= 0xfb;
	if(TSInfo_Direction_Bit_2)	ptr[2] |= 0x02;
	else				ptr[2] &= 0xfd;

	if(TSInfo_AccessPolicy_Bit_1)	ptr[2] |= 0x01;
	else				ptr[2] &= 0xfe;

	ptr[3] = (TSInfo_UserPriority<<2) & 0x1c;

	if(TSInfo_AccessPolicy_Bit_2)	ptr[3] |= 0x80;
	else				ptr[3] &= 0x7f;

	if(TSInfo_Aggregation)		ptr[3] |= 0x40;
	else				ptr[3] &= 0xbf;

	if(TSInfo_APSD)			ptr[3] |= 0x20;
	else				ptr[3] &= 0xdf;

	if(TSInfo_TSInfoAckPolicy_Bit_1)	ptr[3] |= 0x02;
	else					ptr[3] &= 0xfd;

	if(TSInfo_TSInfoAckPolicy_Bit_2)	ptr[3] |= 0x01;
	else					ptr[3] &= 0xfe;

	if(TSInfo_Schedule)		ptr[4] |= 0x80;
	else				ptr[4] &= 0x7f;

	tmp_ptr = (u_char *)NominalMSDUSize;	// little-endian
	char_buf[0] = tmp_ptr[0];
	char_buf[1] = tmp_ptr[1];
	if(tmp_ptr[0] & 0x80)	char_buf[1] = (char_buf[1]<<1) | 0x01;
	else			char_buf[1] = (char_buf[1]<<1) & 0xfe;
	if(NominalMSDUSize_Fixed)	char_buf[0] = (char_buf[0]<<1) | 0x01;
	else				char_buf[0] = (char_buf[0]<<1) & 0xfe;
	ptr[5] = (char)char_buf[1];
	ptr[6] = (char)char_buf[0];

	bcopy((void *)MaximumMSDUSize, (void *)&ptr[7], 2);
	bcopy((void *)MinimumServiceInterval, (void *)&ptr[9], 4);
	bcopy((void *)MaximumServiceInterval, (void *)&ptr[13], 4);
	bcopy((void *)InactivityInterval, (void *)&ptr[17], 4);
	bcopy((void *)SyspensionInterval, (void *)&ptr[21], 4);
	bcopy((void *)ServiceStartTime, (void *)&ptr[25], 4);
	bcopy((void *)MinimumDataRate, (void *)&ptr[29], 4);
	bcopy((void *)MeanDataRate, (void *)&ptr[33], 4);
	bcopy((void *)PeakDataRate, (void *)&ptr[37], 4);
	bcopy((void *)BurstSize, (void *)&ptr[41], 4);
	bcopy((void *)DelayBound, (void *)&ptr[45], 4);
	bcopy((void *)MinimumPHYRate, (void *)&ptr[49], 4);

	tmp_ptr = (u_char *)SurplusBandwidthAllowance_IntegerPart; // little-endian
	char_buf[0] = (tmp_ptr[0]<<5) & 0xe0;
	tmp_ptr = (u_char *)SurplusBandwidthAllowance_DecimalPart; // little-endian
	char_buf[0] |= (tmp_ptr[1] & 0x1f);
	char_buf[1] = tmp_ptr[0];
	ptr[53] = (char)char_buf[0];
	ptr[54] = (char)char_buf[1];

	bcopy((void *)MediumTime, (void *)&ptr[55], 2);

	return;
}

void SetTCLASwithFrameClassifierType0Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const void *SourceAddress, const void *DestinationAddress, const unsigned short *Type) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceAddress == NULL || DestinationAddress == NULL || Type == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG)) return;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x11; //1+16
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x00;
	ptr[4] = (char)ClassifierMask;

	bcopy((void *)SourceAddress, (void *)&ptr[5], 6);
	bcopy((void *)DestinationAddress, (void *)&ptr[11], 6);
	bcopy((void *)Type, (void *)&ptr[17], 2);

	return;
}

void SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, u_char DSCP, u_char Protocol) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceIPAddress == NULL || DestinationIPAddress == NULL || SourcePort == NULL || DestinationPort == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG)) return;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x13; //1+18
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x01;
	ptr[4] = (char)ClassifierMask;
	ptr[5] = (char)Version;

	bcopy((void *)SourceIPAddress, (void *)&ptr[6], 4);
	bcopy((void *)DestinationIPAddress, (void *)&ptr[10], 4);
	bcopy((void *)SourcePort, (void *)&ptr[14], 2);
	bcopy((void *)DestinationPort, (void *)&ptr[16], 2);

	ptr[18] = (char)(DSCP & 0x3f);
	ptr[19] = (char)Protocol;

	return;
}

void SetTCLASwithFrameClassifierType1ForIPv6Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, u_char Version, const void *SourceIPAddress, const void *DestinationIPAddress, const unsigned short *SourcePort, const unsigned short *DestinationPort, const void *FlowLabel) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceIPAddress == NULL || DestinationIPAddress == NULL || SourcePort == NULL || DestinationPort == NULL || FlowLabel == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG)) return;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x2b; //1+42
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x01;
	ptr[4] = (char)ClassifierMask;
	ptr[5] = (char)Version;

	bcopy((void *)SourceIPAddress, (void *)&ptr[6], 16);
	bcopy((void *)DestinationIPAddress, (void *)&ptr[22], 16);
	bcopy((void *)SourcePort, (void *)&ptr[38], 2);
	bcopy((void *)DestinationPort, (void *)&ptr[40], 2);
	bcopy((void *)FlowLabel, (void *)&ptr[42], 3);

	return;
}

void SetTCLASwithFrameClassifierType2Into80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char UserPriority, u_char ClassifierMask, const unsigned short *TagType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || TagType == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG)) return;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS;
	ptr[1] = 0x05; //1+4
	ptr[2] = (char)UserPriority;
	ptr[3] = 0x02;
	ptr[4] = (char)ClassifierMask;

	bcopy((void *)TagType, (void *)&ptr[5], 2);

	return;
}

void SetTCLASProcessingInto80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, u_char Processing) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	if(!(OptionalInfoFlag & WITH_TCLAS_PROCESSING_FLAG)) return;
	offset = 1+1+1+2+6+57;
	if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG) offset += (2+1+16); 
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG) offset += (2+1+18);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG) offset += (2+1+42);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG) offset += (2+1+4);
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_TCLAS_PROCESSING;
	ptr[1] = 0x01;
	ptr[2] = (char)Processing;

	return;
}

void SetScheduleInto80211eADDTSResponseFrameBody(char *Body, u_char OptionalInfoFlag, bool Aggregation, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, const unsigned int *ServiceStartTime, const unsigned int *ServiceInterval, const unsigned short *SpecificationInterval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ServiceStartTime == NULL || ServiceInterval == NULL || SpecificationInterval == NULL) return;
	offset = 1+1+1+2+6+57;
	if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_0_FLAG) offset += (2+1+16); 
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV4_FLAG) offset += (2+1+18);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_1_IPV6_FLAG) offset += (2+1+42);
	else if(OptionalInfoFlag & WITH_TCLAS_CLASSIFIER_TYPE_2_FLAG) offset += (2+1+4);

	if(OptionalInfoFlag & WITH_TCLAS_PROCESSING_FLAG) offset += 3;

	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SCHEDULE;
	ptr[1] = 0x0c;

	ptr[2] = (TSID<<3) & 0x78;

	if(Aggregation)		ptr[2] |= 0x80;
	else			ptr[2] &= 0x7f;

	if(Direction_Bit_1)	ptr[2] |= 0x04;
	else			ptr[2] &= 0xfb;

	if(Direction_Bit_2)	ptr[2] |= 0x02;
	else			ptr[2] &= 0xfd;

	bcopy((void *)ServiceStartTime, (void *)&ptr[4], 4);
	bcopy((void *)ServiceInterval, (void *)&ptr[8], 4);
	bcopy((void *)SpecificationInterval, (void *)&ptr[12], 2);

	return;
}

void GetDialogTokenFrom80211eADDTSResponseFrameBody(char *Body, u_char *DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DialogToken == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	*DialogToken = (u_char)ptr[0];

	return; 
}

void GetStatusCodeFrom80211eADDTSResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

char *GetTSDelayFrom80211eADDTSResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2;
	ptr = Body + offset;

	return (char *)ptr;
}

char *GetTSPECFrom80211eADDTSResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2+6;
	ptr = Body + offset;

	return (char *)ptr;
}

char *GetTCLASFrom80211eADDTSResponseFrameBody(char *Body, u_char *FrameClassifierType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || FrameClassifierType == NULL) return NULL;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	if(ptr[0] != ELEMENT_ID_TCLAS) return NULL;

	*FrameClassifierType = (u_char)ptr[3];
	return (char *)ptr;
}

char *GetTCLASProcessingFrom80211eADDTSResponseFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_TCLAS) {
		if(FrameClassifierType == ClassifierType_0)		ptr += (2+1+16); 
		else if(FrameClassifierType == ClassifierType_1_IPv4)	ptr += (2+1+18);
		else if(FrameClassifierType == ClassifierType_1_IPv6)	ptr += (2+1+42); 
		else if(FrameClassifierType == ClassifierType_2)	ptr += (2+1+4);
	}

	if(ptr[0] == ELEMENT_ID_TCLAS_PROCESSING) 	return (char *)ptr;
	else						return NULL;
}

char *GetScheduleFrom80211eADDTSResponseFrameBody(char *Body, enum TCLAS_FrameClassifierType FrameClassifierType) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2+6+57;
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_TCLAS) {
		if(FrameClassifierType == ClassifierType_0)		ptr += (2+1+16); 
		else if(FrameClassifierType == ClassifierType_1_IPv4)	ptr += (2+1+18);
		else if(FrameClassifierType == ClassifierType_1_IPv6)	ptr += (2+1+42); 
		else if(FrameClassifierType == ClassifierType_2)	ptr += (2+1+4);
	}

	if(ptr[0] == ELEMENT_ID_TCLAS_PROCESSING) ptr += 3;	

	if(ptr[0] == ELEMENT_ID_SCHEDULE)	return (char *)ptr;
	else					return NULL;
}

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
unsigned int Calculate80211eDELTSFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+3+2;

	return len;
}

void SetTSInfoInto80211eDELTSFrameBody(char *Body, bool TrafficType, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, bool AccessPolicy_Bit_1, bool AccessPolicy_Bit_2, bool Aggregation, bool APSD, u_char UserPriority, bool TSInfoAckPolicy_Bit_1, bool TSInfoAckPolicy_Bit_2, bool Schedule) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = (TSID<<3) & 0x78;

	if(TrafficType)	ptr[0] |= 0x80;
	else		ptr[0] &= 0x7f;

	if(Direction_Bit_1)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;
	if(Direction_Bit_2)	ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(AccessPolicy_Bit_1)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	ptr[1] = (UserPriority<<2) & 0x1c;

	if(AccessPolicy_Bit_2)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;

	if(Aggregation)		ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;

	if(APSD)		ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;

	if(TSInfoAckPolicy_Bit_1)	ptr[1] |= 0x02;
	else				ptr[1] &= 0xfd;

	if(TSInfoAckPolicy_Bit_2)	ptr[1] |= 0x01;
	else				ptr[1] &= 0xfe;

	if(Schedule)		ptr[2] |= 0x80;
	else			ptr[2] &= 0x7f;

	return;
}

void SetReasonCodeInto80211eDELTSFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+3;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

char *GetTSInfoFrom80211eDELTSFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1;
	ptr = Body + offset;

	return (char *)ptr;
}

void GetReasonCodeFrom80211eDELTSFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+3;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211e Schedule frame body:
   Category: 1
   Action: 3

    -- Category (1 octet)
    -- Action (1 octet)
    -- Schedule (14 octets)
*/
unsigned int Calculate80211eScheduleFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+14;

	return len;
}

void SetScheduleInto80211eScheduleFrameBody(char *Body, bool Aggregation, u_char TSID, bool Direction_Bit_1, bool Direction_Bit_2, const unsigned int *ServiceStartTime, const unsigned int *ServiceInterval, const unsigned short *SpecificationInterval)  {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ServiceStartTime == NULL || ServiceInterval == NULL || SpecificationInterval == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SCHEDULE;
	ptr[1] = 0x0c;

	ptr[2] = (TSID<<3) & 0x78;

	if(Aggregation)		ptr[2] |= 0x80;
	else			ptr[2] &= 0x7f;

	if(Direction_Bit_1)	ptr[2] |= 0x04;
	else			ptr[2] &= 0xfb;

	if(Direction_Bit_2)	ptr[2] |= 0x02;
	else			ptr[2] &= 0xfd;

	bcopy((void *)ServiceStartTime, (void *)&ptr[4], 4);
	bcopy((void *)ServiceInterval, (void *)&ptr[8], 4);
	bcopy((void *)SpecificationInterval, (void *)&ptr[12], 2);

	return;
}

char *GetScheduleFrom80211eScheduleFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1;
	ptr = Body + offset;

	return (char *)ptr;
}

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
unsigned int Calculate80211eDLSRequestFrameBodyLength(unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len) {
	unsigned int	len;
	
	len = 1+1+6+6+2+2+(2+SupportedRates_Len)+(2+ExtendedSupportedRates_Len);

	return len;
}

void SetDestinationMACAddressInto80211eDLSRequestFrameBody(char *Body, const void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)DestinationMACAddress, (void *)ptr, 6);

	return;
}

void SetSourceMACAddressInto80211eDLSRequestFrameBody(char *Body, const void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+6;
	ptr = Body + offset;
	bcopy((void *)SourceMACAddress, (void *)ptr, 6);

	return;
}

void SetCapabilityInfoInto80211eDLSRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1+6+6;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetDLSTimeoutValueInto80211eDLSRequestFrameBody(char *Body, const unsigned short *DLSTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DLSTimeoutValue == NULL) return;
	offset = 1+1+6+6+2;
	ptr = Body + offset;
	bcopy((void *)DLSTimeoutValue, (void *)ptr, 2);

	return;
}
void SetSupportedRatesInto80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 1+1+6+6+2+2; 
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;
	
	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetExtendedSupportedRatesInto80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len, const void *ExtendedSupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ExtendedSupportedRates == NULL) return;
	offset = 1+1+6+6+2+2+(2+SupportedRates_Len); 
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EXTENDED_SUPPORTED_RATES;
	ptr[1] = (char)ExtendedSupportedRates_Len;
	
	bcopy((void *)ExtendedSupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void GetDestinationMACAddressFrom80211eDLSRequestFrameBody(char *Body, void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)DestinationMACAddress, 6);

	return;
}

void GetSourceMACAddressFrom80211eDLSRequestFrameBody(char *Body, void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+6;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)SourceMACAddress, 6);

	return;
}

char *GetCapabilityInfoFrom80211eDLSRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+6+6;
	ptr = Body + offset;

	return (char *)ptr;
}

void GetDLSTimeoutValueFrom80211eDLSRequestFrameBody(char *Body, unsigned short *DLSTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DLSTimeoutValue == NULL) return;
	offset = 1+1+6+6+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)DLSTimeoutValue, 2);

	return;
}

void *GetSupportedRatesFrom80211eDLSRequestFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 1+1+6+6+2+2; 
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetExtendedSupportedRatesFrom80211eDLSRequestFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int *ExtendedSupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ExtendedSupportedRates_Len == NULL) return NULL;
	offset = 1+1+6+6+2+2+(2+SupportedRates_Len); 
	ptr = Body + offset;

	*ExtendedSupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

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
unsigned int Calculate80211eDLSResponseFrameBodyLength(unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len) {
	unsigned int	len;
	
	len = 1+1+2+6+6+2+(2+SupportedRates_Len)+(2+ExtendedSupportedRates_Len);

	return len;
}

void SetStatusCodeInto80211eDLSResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetDestinationMACAddressInto80211eDLSResponseFrameBody(char *Body, const void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1+2;
	ptr = Body + offset;
	bcopy((void *)DestinationMACAddress, (void *)ptr, 6);

	return;
}

void SetSourceMACAddressInto80211eDLSResponseFrameBody(char *Body, const void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+2+6;
	ptr = Body + offset;
	bcopy((void *)SourceMACAddress, (void *)ptr, 6);

	return;
}

void SetCapabilityInfoInto80211eDLSResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy, bool ShortPreamble, bool PBCC, bool ChannelAgility, bool SpectrumManagement, bool QoS, bool ShortSlotTime, bool APSD, bool DSSS_OFDM, bool DelayedBloackAck, bool ImmediateBlockAck) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1+2+6+6;
	ptr = Body + offset;

	if(ESS)			ptr[0] |= 0x80;
	else			ptr[0] &= 0x7f;
	
	if(IBSS)		ptr[0] |= 0x40;
	else			ptr[0] &= 0xbf;
	
	if(CF_Pollable)		ptr[0] |= 0x20;
	else			ptr[0] &= 0xdf;
	
	if(CF_Poll_Request)	ptr[0] |= 0x10;
	else			ptr[0] &= 0xef;

	if(Privacy)		ptr[0] |= 0x08;
	else			ptr[0] &= 0xf7;

	if(ShortPreamble)	ptr[0] |= 0x04;
	else			ptr[0] &= 0xfb;

	if(PBCC)		ptr[0] |= 0x02;
	else			ptr[0] &= 0xfd;

	if(ChannelAgility)	ptr[0] |= 0x01;
	else			ptr[0] &= 0xfe;

	if(SpectrumManagement)	ptr[1] |= 0x80;
	else			ptr[1] &= 0x7f;
                                              
	if(QoS)			ptr[1] |= 0x40;
	else			ptr[1] &= 0xbf;
                                              
	if(ShortSlotTime)	ptr[1] |= 0x20;
	else			ptr[1] &= 0xdf;
                                              
	if(APSD)		ptr[1] |= 0x10;
	else			ptr[1] &= 0xef;

	if(DSSS_OFDM)		ptr[1] |= 0x04;
	else			ptr[1] &= 0xfb;
                                              
	if(DelayedBloackAck)	ptr[1] |= 0x02;
	else			ptr[1] &= 0xfd;
                                              
	if(ImmediateBlockAck)	ptr[1] |= 0x01;
	else			ptr[1] &= 0xfe;

	return;
}

void SetSupportedRatesInto80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates == NULL) return;
	offset = 1+1+2+6+6+2; 
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_SUPPORTED_RATES;
	ptr[1] = (char)SupportedRates_Len;
	
	bcopy((void *)SupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void SetExtendedSupportedRatesInto80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int ExtendedSupportedRates_Len, const void *ExtendedSupportedRates) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ExtendedSupportedRates == NULL) return;
	offset = 1+1+2+6+6+2+(2+SupportedRates_Len); 
	ptr = Body + offset;

	ptr[0] = ELEMENT_ID_EXTENDED_SUPPORTED_RATES;
	ptr[1] = (char)ExtendedSupportedRates_Len;
	
	bcopy((void *)ExtendedSupportedRates, (void *)&ptr[2], (unsigned int)ptr[1]);

	return;
}

void GetStatusCodeFrom80211eDLSResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

void GetDestinationMACAddressFrom80211eDLSResponseFrameBody(char *Body, void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)DestinationMACAddress, 6);

	return;
}

void GetSourceMACAddressFrom80211eDLSResponseFrameBody(char *Body, void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+2+6;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)SourceMACAddress, 6);

	return;
}

char *GetCapabilityInfoFrom80211eDLSResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+2+6+6;
	ptr = Body + offset;

	return (char *)ptr;
}

void *GetSupportedRatesFrom80211eDLSResponseFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 1+1+2+6+6+2; 
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetExtendedSupportedRatesFrom80211eDLSResponseFrameBody(char *Body, unsigned int SupportedRates_Len, unsigned int *ExtendedSupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ExtendedSupportedRates_Len == NULL) return NULL;
	offset = 1+1+2+6+6+2+(2+SupportedRates_Len); 
	ptr = Body + offset;

	*ExtendedSupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}


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
unsigned int Calculate80211eDLSTeardownFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+6+6+2;

	return len;
}

void SetDestinationMACAddressInto80211eDLSTeardownFrameBody(char *Body, const void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)DestinationMACAddress, (void *)ptr, 6);

	return;
}

void SetSourceMACAddressInto80211eDLSTeardownFrameBody(char *Body, const void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+6;
	ptr = Body + offset;
	bcopy((void *)SourceMACAddress, (void *)ptr, 6);

	return;
}

void SetReasonCodeInto80211eDLSTeardownFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+6+6;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

void GetDestinationMACAddressFrom80211eDLSTeardownFrameBody(char *Body, void *DestinationMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DestinationMACAddress == NULL) return;
	offset = 1+1;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)DestinationMACAddress, 6);

	return;
}

void GetSourceMACAddressFrom80211eDLSTeardownFrameBody(char *Body, void *SourceMACAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SourceMACAddress == NULL) return;
	offset = 1+1+6;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)SourceMACAddress, 6);

	return;
}

void GetReasonCodeFrom80211eDLSTeardownFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+6+6;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}

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
unsigned int Calculate80211eADDBARequestFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+1+2+2+2;

	return len;
}

void SetDialogTokenInto80211eADDBARequestFrameBody(char *Body, u_char DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = (char)DialogToken;
	return; 
}

void SetBlockAckParameterSetInto80211eADDBARequestFrameBody(char *Body, bool BlockAckPolicy, u_char TID, const unsigned short *BufferSize) {
	char		*ptr;
	unsigned int	offset;
	u_char		*tmp_ptr;
	u_char		char_buf[2];


	if(Body == NULL || BufferSize == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;

	char_buf[0] = (TID<<2) & 0x3c;

	if(BlockAckPolicy)	char_buf[0] |= 0x40;
	else			char_buf[0] &= 0xbf;

	tmp_ptr = (u_char *)BufferSize;	// little-endian
	char_buf[0] |= (tmp_ptr[1] & 0x03);
	char_buf[1] = tmp_ptr[0];

	ptr[0] = (char)char_buf[0];
	ptr[1] = (char)char_buf[1];

	return;
}

void SetBlockAckTimeoutValueInto80211eADDBARequestFrameBody(char *Body, const unsigned short *BlockAckTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || BlockAckTimeoutValue == NULL) return;
	offset = 1+1+1+2;
	ptr = Body + offset;
	bcopy((void *)BlockAckTimeoutValue, (void *)ptr, 2);

	return;
}

void SetBlockAckStartingSequenceControlInto80211eADDBARequestFrameBody(char *Body, const unsigned short *FragmentNumber, const unsigned short *StartingSequenceNumber) {
	char		*ptr;
	unsigned int	offset;
	u_char		*tmp_ptr;
	u_char		char_buf[2];

	if(Body == NULL || FragmentNumber == NULL || StartingSequenceNumber == NULL) return;
	offset = 1+1+1+2+2;
	ptr = Body + offset;

	tmp_ptr = (u_char *)FragmentNumber; // little-endian
	char_buf[0] = (tmp_ptr[0]<<4) & 0xf0;
	
	tmp_ptr = (u_char *)StartingSequenceNumber; // little-endian
	char_buf[0] |= (tmp_ptr[1] & 0x0f);
	char_buf[1] = tmp_ptr[0];

	ptr[0] = (char)char_buf[0];
	ptr[1] = (char)char_buf[1];

	return;
}

void GetDialogTokenFrom80211eADDBARequestFrameBody(char *Body, u_char *DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DialogToken == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	*DialogToken = (u_char)ptr[0];

	return; 
}

char *GetBlockAckParameterSetFrom80211eADDBARequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1;
	ptr = Body + offset;

	return (char *)ptr;
}

void GetBlockAckTimeoutValueFrom80211eADDBARequestFrameBody(char *Body, unsigned short *BlockAckTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || BlockAckTimeoutValue == NULL) return;
	offset = 1+1+1+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)BlockAckTimeoutValue, 2);

	return;
}

char *GetBlockAckStartingSequenceControlFrom80211eADDBARequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2+2;
	ptr = Body + offset;

	return (char *)ptr;
}


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
unsigned int Calculate80211eADDBAResponseFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+1+2+2+2;

	return len;
}

void SetDialogTokenInto80211eADDBAResponseFrameBody(char *Body, u_char DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = (char)DialogToken;
	return; 
}

void SetStatusCodeInto80211eADDBAResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetBlockAckParameterSetInto80211eADDBAResponseFrameBody(char *Body, bool BlockAckPolicy, u_char TID, const unsigned short *BufferSize) {
	char		*ptr;
	unsigned int	offset;
	u_char		*tmp_ptr;
	u_char		char_buf[2];


	if(Body == NULL || BufferSize == NULL) return;
	offset = 1+1+1+2;
	ptr = Body + offset;

	char_buf[0] = (TID<<2) & 0x3c;

	if(BlockAckPolicy)	char_buf[0] |= 0x40;
	else			char_buf[0] &= 0xbf;

	tmp_ptr = (u_char *)BufferSize; // litten-endian
	char_buf[0] |= (tmp_ptr[1] & 0x03);
	char_buf[1] = tmp_ptr[0];

	ptr[0] = (char)char_buf[0];
	ptr[1] = (char)char_buf[1];

	return;
}

void SetBlockAckTimeoutValueInto80211eADDBAResponseFrameBody(char *Body, const unsigned short *BlockAckTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || BlockAckTimeoutValue == NULL) return;
	offset = 1+1+1+2+2;
	ptr = Body + offset;
	bcopy((void *)BlockAckTimeoutValue, (void *)ptr, 2);

	return;
}

void GetDialogTokenFrom80211eADDBAResponseFrameBody(char *Body, u_char *DialogToken) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || DialogToken == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	*DialogToken = (u_char)ptr[0];

	return; 
}

void GetStatusCodeFrom80211eADDBAResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 1+1+1;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

char *GetBlockAckParameterSetFrom80211eADDBAResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1+1+2;
	ptr = Body + offset;

	return (char *)ptr;
}

void GetBlockAckTimeoutValueFrom80211eADDBAResponseFrameBody(char *Body, unsigned short *BlockAckTimeoutValue) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || BlockAckTimeoutValue == NULL) return;
	offset = 1+1+1+2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)BlockAckTimeoutValue, 2);

	return;
}


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
unsigned int Calculate80211eDELBAFrameBodyLength() {
	unsigned int	len;
	
	len = 1+1+2+2;

	return len;
}

void SetDELBAParameterSetInto80211eDELBAFrameBody(char *Body, bool Initiator, u_char TID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return;
	offset = 1+1;
	ptr = Body + offset;

	ptr[0] = 0x00;
	ptr[1] = TID & 0x0f;
	if(Initiator)	ptr[1] |= 0x10;
	else		ptr[1] &= 0xef;

	return;
}

void SetReasonCodeInto80211eDELBAFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+2;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

char *GetDELBAParameterSetFrom80211eDELBAFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 1+1;
	ptr = Body + offset;

	return (char *)ptr;
}

void GetReasonCodeFrom80211eDELBAFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 1+1+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}

