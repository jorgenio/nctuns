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

#include "mac-802_11_management_framebody.h"
#include <strings.h>

void InitializeFrameBodyWithZero(void *Body, int len) {
	if(Body == NULL) return;
	bzero(Body, len);
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
    -- Reserved (11 bits)
*/
bool GetESSFrom80211CapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x80)	return true;
	else				return false; 
}
bool GetIBSSFrom80211CapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x40)	return true;
	else				return false; 
}
bool GetCFPollableFrom80211CapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x20)	return true;
	else				return false; 
}
bool GetCFPollRequestFrom80211CapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x10)	return true;
	else				return false; 
}
bool GetPrivacyFrom80211CapabilityInfo(char *CapabilityInfo) {
	if(CapabilityInfo[0] & 0x08)	return true;
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
bool GetDwellTimeFrom80211FHParameterSet(char *FHParameterSet, unsigned short *DwellTime){
	if(FHParameterSet == NULL || DwellTime == NULL)	return false;
	bcopy((void *)&FHParameterSet[2], (void *)DwellTime, 2);
	return true;
} 
bool GetHopSetFrom80211FHParameterSet(char *FHParameterSet, u_char *HopSet) {
	if(FHParameterSet == NULL || HopSet == NULL)	return false;
	*HopSet = FHParameterSet[4];
	return true;
}
bool GetHopPatternFrom80211FHParameterSet(char *FHParameterSet, u_char *HopPattern) {
	if(FHParameterSet == NULL || HopPattern == NULL)	return false;
	*HopPattern = FHParameterSet[5];
	return true;
}
bool GetHopIndexFrom80211FHParameterSet(char *FHParameterSet, u_char *HopIndex) {
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
bool GetCurrentChannelFrom80211DSParameterSet(char *DSParameterSet, u_char *CurrentChannel) {
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
bool GetCountFrom80211CFParameterSet(char *CFParameterSet, u_char *Count) {
	if(CFParameterSet == NULL || Count == NULL)	return false;
	*Count = CFParameterSet[2];
	return true;
}
bool GetPeriodFrom80211CFParameterSet(char *CFParameterSet, u_char *Period) {
	if(CFParameterSet == NULL || Period == NULL)	return false;
	*Period = CFParameterSet[3];
	return true;
}
bool GetMaxDurationFrom80211CFParameterSet(char *CFParameterSet, unsigned short *MaxDuration) {
	if(CFParameterSet == NULL || MaxDuration == NULL)	return false;
	bcopy((void *)&CFParameterSet[4], (void *)MaxDuration, 2);
	return true;
}
bool GetDurRemainingFrom80211CFParameterSet(char *CFParameterSet, unsigned short *DurRemaining) {
	if(CFParameterSet == NULL || DurRemaining == NULL)	return false;
	bcopy((void *)&CFParameterSet[6], (void *)DurRemaining, 2);
	return true;
}

/* 
   2007/11/03 By C.L. Chou 
   IBSS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- ATIM Window(2 octets)
*/
bool GetATIMWindowFrom80211IBSSParameterSet(char *IBSSParameterSet, unsigned short *ATIM_Window) {
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
bool GetCountFrom80211TIM(char *TIM, u_char *Count) {
	if(TIM == NULL || Count == NULL)	return false;
	*Count = TIM[2];
	return true;
}
bool GetPeriodFrom80211TIM(char *TIM, u_char *Period) {
	if(TIM == NULL || Period == NULL)	return false;
	*Period = TIM[3];
	return true;
}
bool GetBitmapCtrlFrom80211TIM(char *TIM, u_char *Bitmap_Ctrl) {
	if(TIM == NULL || Bitmap_Ctrl == NULL)	return false;
	*Bitmap_Ctrl = TIM[4];
	return true;
}
void *GetPartialVirtualBitmapFrom80211TIM(char *TIM, unsigned int *PartialVirtualBitmap_Len) {
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
void *GetChallengeTextFromChallengeText(char *ChallengeText, unsigned int *ChallengeText_Len) {
	if(ChallengeText == NULL || ChallengeText_Len == NULL) return NULL;
	*ChallengeText_Len = (unsigned int)ChallengeText[1];
	return (void *)&ChallengeText[2];
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Beacon frame body:
	
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
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211BeaconFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithTIM) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithFH) flag |= WITH_FH_FLAG;
	if(WithDS) flag |= WITH_DS_FLAG;
	if(WithCF) flag |= WITH_CF_FLAG;
	if(WithIBSS) flag |= WITH_IBSS_FLAG;
	if(WithTIM) flag |= WITH_TIM_FLAG;

	return flag;
}

unsigned int Calculate80211BeaconFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len) {
	unsigned int	len;
	
	len = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);

	if(OptionalInfoFlag & WITH_FH_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) len += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) len += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) len += 4;
	if(OptionalInfoFlag & WITH_TIM_FLAG) len += (5+PartialVirtualBitmap_Len);

	return len;
}

void SetTimestampInto80211BeaconFrameBody(char *Body, const unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)Timestamp, (void *)ptr, 8);

	return;
}

void SetBeaconIntervalInto80211BeaconFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)Interval, (void *)ptr, 2);

	return;
}

void SetCapabilityInfoInto80211BeaconFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
} 

void SetSSIDInto80211BeaconFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
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

void SetSupportedRatesInto80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

void SetFHParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex) {
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

void SetDSParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel) {
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

void SetCFParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration,  const unsigned short *DurRemaining) {
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

void SetIBSSParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window) {
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


void SetTIMInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, u_char Bitmap_Ctrl, unsigned int PartialVirtualBitmap_Len, const void *PartialVirtualBitmap) {
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

void GetTimestampFrom80211BeaconFrameBody(char *Body, unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Timestamp, 8);

	return;
}

void GetBeaconIntervalFrom80211BeaconFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);

	return;
}

char *GetCapabilityInfoFrom80211BeaconFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2;
	ptr = Body + offset;
	return ptr;
}

void *GetSSIDFrom80211BeaconFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 8+2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetFHParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetDSParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;

	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetCFParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
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

char *GetIBSSParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
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

char *GetTIMFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
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

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Disassociation frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211DisassociationFrameBodyLength() {
	unsigned int	len;

	len = 2;
	return len;
}

void SetReasonCodeInto80211DisassociationFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;

	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

void GetReasonCodeFrom80211DisassociationFrameBody(char *Body, unsigned short *ReasonCode) {
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
   Information contained in 80211 Association Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211AssociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	return len;
}

void SetCapabilityInfoInto80211AssociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
}

void SetListenIntervalInto80211AssociationRequestFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 2;
	ptr = Body + offset;

	bcopy((void *)Interval, (void *)ptr, 2);

	return;
}

void SetSSIDInto80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
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

void SetSupportedRatesInto80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

char *GetCapabilityInfoFrom80211AssociationRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetListenIntervalFrom80211AssociationRequestFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);

	return;
}

void *GetSSIDFrom80211AssociationRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Association Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211AssociationResponseFrameBodyLength(unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+2+(2+SupportedRates_Len);
	return len;
}

void SetCapabilityInfoInto80211AssociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
}

void SetStatusCodeInto80211AssociationResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetAIDInto80211AssociationResponseFrameBody(char *Body, const unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)AID, (void *)ptr, 2);

	return;
}

void SetSupportedRatesInto80211AssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

char *GetCapabilityInfoFrom80211AssociationResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetStatusCodeFrom80211AssociationResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

void GetAIDFrom80211AssociationResponseFrameBody(char *Body, unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AID, 2);

	return;
}

void *GetSupportedRatesFrom80211AssociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+2;
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Reassociation Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- Current AP address (6 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ReassociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+6+(2+SSID_Len)+(2+SupportedRates_Len);
	return len;
}

void SetCapabilityInfoInto80211ReassociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
}

void SetListenIntervalInto80211ReassociationRequestFrameBody(char *Body, const unsigned short *ListenInterval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ListenInterval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ListenInterval, (void *)ptr, 2);

	return;
}

void SetCurrentAPAddressInto80211ReassociationRequestFrameBody(char *Body, const void * CurrentAPAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || CurrentAPAddress == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)CurrentAPAddress, (void *)ptr, 6);

	return;
}

void SetSSIDInto80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
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

void SetSupportedRatesInto80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

char *GetCapabilityInfoFrom80211ReassociationRequestFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetListenIntervalFrom80211ReassociationRequestFrameBody(char *Body, unsigned short *ListenInterval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ListenInterval == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ListenInterval, 2);

	return;
}

void GetCurrentAPAddressFrom80211ReassociationRequestFrameBody(char *Body, void * CurrentAPAddress) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || CurrentAPAddress == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)CurrentAPAddress, 6);

	return;
}

void *GetSSIDFrom80211ReassociationRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 2+2+6;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+6+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Reassociation Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ReassociationResponseFrameBodyLength(unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = 2+2+2+(2+SupportedRates_Len);
	return len;
}

void SetCapabilityInfoInto80211ReassociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
}

void SetStatusCodeInto80211ReassociationResponseFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetAIDInto80211ReassociationResponseFrameBody(char *Body, const unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)AID, (void *)ptr, 2);

	return;
}

void SetSupportedRatesInto80211ReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

char *GetCapabilityInfoFrom80211ReassociationResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;
	return ptr;
}

void GetStatusCodeFrom80211ReassociationResponseFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

void GetAIDFrom80211ReassociationResponseFrameBody(char *Body, unsigned short *AID) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AID == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AID, 2);

	return;
}

void *GetSupportedRatesFrom80211ReassociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 2+2+2;
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Probe Request frame body:

    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ProbeRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;

	len = (2+SSID_Len)+(2+SupportedRates_Len);
	return len;
}

void SetSSIDInto80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
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

void SetSupportedRatesInto80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

void *GetSSIDFrom80211ProbeRequestFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 0;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
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
   Information contained in 80211 Probe Response frame body:

    -- Timestamp (8 octets)
    -- Beacon Interval (2 octets)
    -- Capability Information (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- FH Parameter Set (7 octets) <optional>
    -- DS Parameter Set (3 octets) <optional>
    -- CF Parameter Set (8 octets) <optional>
    -- IBSS Parameter Set (4 octets) <optional>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211ProbeResponseFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithFH) flag |= WITH_FH_FLAG;
	if(WithDS) flag |= WITH_DS_FLAG;
	if(WithCF) flag |= WITH_CF_FLAG;
	if(WithIBSS) flag |= WITH_IBSS_FLAG;

	return flag;
}

unsigned int Calculate80211ProbeResponseFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	unsigned int	len;
	
	len = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);

	if(OptionalInfoFlag & WITH_FH_FLAG) len += 7;
	if(OptionalInfoFlag & WITH_DS_FLAG) len += 3;
	if(OptionalInfoFlag & WITH_CF_FLAG) len += 8;
	if(OptionalInfoFlag & WITH_IBSS_FLAG) len += 4;

	return len;
}

void SetTimestampInto80211ProbeResponseFrameBody(char *Body, const unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)Timestamp, (void *)ptr, 8);

	return;
}

void SetBeaconIntervalInto80211ProbeResponseFrameBody(char *Body, const unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)Interval, (void *)ptr, 2);

	return;
}

void SetCapabilityInfoInto80211ProbeResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy) {
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

	return;
} 

void SetSSIDInto80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, const void *SSID) {
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

void SetSupportedRatesInto80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates) {
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

void SetFHParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex) {
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

void SetDSParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel) {
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

void SetCFParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration,  const unsigned short *DurRemaining) {
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

void SetIBSSParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window) {
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

void GetTimestampFrom80211ProbeResponseFrameBody(char *Body, unsigned long long *Timestamp) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Timestamp == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Timestamp, 8);

	return;
}

void GetBeaconIntervalFrom80211ProbeResponseFrameBody(char *Body, unsigned short *Interval) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || Interval == NULL) return;
	offset = 8;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)Interval, 2);

	return;
}

char *GetCapabilityInfoFrom80211ProbeResponseFrameBody(char *Body) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2;
	ptr = Body + offset;
	return ptr;
}

void *GetSSIDFrom80211ProbeResponseFrameBody(char *Body, unsigned int *SSID_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SSID_Len == NULL) return NULL;
	offset = 8+2+2;
	ptr = Body + offset;

	*SSID_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

void *GetSupportedRatesFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || SupportedRates_Len == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len);
	ptr = Body + offset;

	*SupportedRates_Len = (unsigned int)ptr[1];
	return (void *)&ptr[2];
}

char *GetFHParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetDSParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL) return NULL;
	offset = 8+2+2+(2+SSID_Len)+(2+SupportedRates_Len);
	ptr = Body + offset;

	if(ptr[0] == ELEMENT_ID_FH_PARAMETER_SET)	ptr += 7;

	if(ptr[0] == ELEMENT_ID_DS_PARAMETER_SET)	return (char *)ptr;
	else						return NULL;
}

char *GetCFParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
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

char *GetIBSSParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len) {
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

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Authentication frame body:

    -- Authentication Algorithm Number (2 octets)
    -- Authentication Transaction Sequence Number (2 octets)
    -- Status Code (2 octets)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text) <optional>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211AuthenticationFrameBody(bool WithChallengeText) {
	u_char	flag;
	
	flag &= 0x00;

	if(WithChallengeText) flag |= WITH_CHALLENGE_TEXT_FLAG;

	return flag;
}

unsigned int Calculate80211AuthenticationFrameBodyLength(u_char OptionalInfoFlag, unsigned int ChallengeText_Len) {
	unsigned int	len;
	
	len = 2+2+2;

	if(OptionalInfoFlag & WITH_CHALLENGE_TEXT_FLAG) len += ChallengeText_Len;

	return len;
}

void SetAuthAlgoNumInto80211AuthenticationFrameBody(char *Body, const unsigned short *AuthAlgoNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthAlgoNum == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)AuthAlgoNum, (void *)ptr, 2);

	return;
}

void SetAuthTransSeqNumInto80211AuthenticationFrameBody(char *Body, const unsigned short *AuthTransSeqNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthTransSeqNum == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)AuthTransSeqNum, (void *)ptr, 2);

	return;
}

void SetStatusCodeInto80211AuthenticationFrameBody(char *Body, const unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)StatusCode, (void *)ptr, 2);

	return;
}

void SetChallengeTextInto80211AuthenticationFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int ChallengeText_Len, const void *ChallengeText) {
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

void GetAuthAlgoNumFrom80211AuthenticationFrameBody(char *Body, unsigned short *AuthAlgoNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthAlgoNum == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AuthAlgoNum, 2);

	return;
}

void GetAuthTransSeqNumFrom80211AuthenticationFrameBody(char *Body, unsigned short *AuthTransSeqNum) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || AuthTransSeqNum == NULL) return;
	offset = 2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)AuthTransSeqNum, 2);

	return;
}

void GetStatusCodeFrom80211AuthenticationFrameBody(char *Body, unsigned short *StatusCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || StatusCode == NULL) return;
	offset = 2+2;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)StatusCode, 2);

	return;
}

char *GetChallengeTextFrom80211AuthenticationFrameBody(char *Body) {
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
   Information contained in 80211 Deauthentication frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211DeauthenticationFrameBodyLength() {
	unsigned int	len;

	len = 2;
	return len;
}

void SetReasonCodeInto80211DeauthenticationFrameBody(char *Body, const unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ReasonCode, (void *)ptr, 2);

	return;
}

void GetReasonCodeFrom80211DeauthenticationFrameBody(char *Body, unsigned short *ReasonCode) {
	char		*ptr;
	unsigned int	offset;

	if(Body == NULL || ReasonCode == NULL) return;
	offset = 0;
	ptr = Body + offset;
	bcopy((void *)ptr, (void *)ReasonCode, 2);

	return;
}
