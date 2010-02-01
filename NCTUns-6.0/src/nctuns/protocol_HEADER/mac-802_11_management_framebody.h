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

#ifndef	__NCTUNS_mac_802_11_management_framebody_h__
#define __NCTUNS_mac_802_11_management_framebody_h__

#include <stdlib.h>
#include <stdbool.h>

/* 
   2007/11/03 By C.L. Chou
   Information contained in 80211 frame body:
 
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

   <Variable Length>
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
    -- TIM (1_ElementID + 1_Length + 1_DTIM_Count + 1_DTIM_Period + 1_Bitmap_Control + 1~251_Partial_Virtual_Bitmap)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text)
*/
#define WITH_CHALLENGE_TEXT_FLAG	0x01

#define WITH_FH_FLAG			0x01
#define WITH_DS_FLAG			0x02
#define WITH_CF_FLAG			0x04
#define WITH_IBSS_FLAG			0x08
#define WITH_TIM_FLAG			0x10

#define ELEMENT_ID_SSID			0x00
#define ELEMENT_ID_SUPPORTED_RATES 	0x01
#define ELEMENT_ID_FH_PARAMETER_SET 	0x02
#define ELEMENT_ID_DS_PARAMETER_SET 	0x03
#define ELEMENT_ID_CF_PARAMETER_SET 	0x04
#define ELEMENT_ID_TIM			0x05
#define ELEMENT_ID_IBSS_PARAMETER_SET	0x06
#define ELEMENT_ID_CHALLENGE_TEXT	0x10


void InitializeFrameBodyWithZero(void *body, int len);

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
bool GetESSFrom80211CapabilityInfo(char *CapabilityInfo);
bool GetIBSSFrom80211CapabilityInfo(char *CapabilityInfo);
bool GetCFPollableFrom80211CapabilityInfo(char *CapabilityInfo);
bool GetCFPollRequestFrom80211CapabilityInfo(char *CapabilityInfo);
bool GetPrivacyFrom80211CapabilityInfo(char *CapabilityInfo);

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
bool GetDwellTimeFrom80211FHParameterSet(char *FHParameterSet, unsigned short *DwellTime);
bool GetHopSetFrom80211FHParameterSet(char *FHParameterSet, u_char *HopSet);
bool GetHopPatternFrom80211FHParameterSet(char *FHParameterSet, u_char *HopPattern);
bool GetHopIndexFrom80211FHParameterSet(char *FHParameterSet, u_char *HopIndex);

/* 
   2007/11/03 By C.L. Chou 
   DS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Current Channel (1 octet)
*/
bool GetCurrentChannelFrom80211DSParameterSet(char *DSParameterSet, u_char *CurrentChannel);

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
bool GetCountFrom80211CFParameterSet(char *CFParameterSet, u_char *Count);
bool GetPeriodFrom80211CFParameterSet(char *CFParameterSet, u_char *Period);
bool GetMaxDurationFrom80211CFParameterSet(char *CFParameterSet, unsigned short *MaxDuration);
bool GetDurRemainingFrom80211CFParameterSet(char *CFParameterSet, unsigned short *DurRemaining);

/* 
   2007/11/03 By C.L. Chou 
   IBSS Parameter Set Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- ATIM Window(2 octets)
*/
bool GetATIMWindowFrom80211IBSSParameterSet(char *IBSSParameterSet, unsigned short *ATIM_Window);

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
bool GetCountFrom80211TIM(char *TIM, u_char *Count);
bool GetPeriodFrom80211TIM(char *TIM, u_char *Period);
bool GetBitmapCtrlFrom80211TIM(char *TIM, u_char *Bitmap_Ctrl);
void *GetPartialVirtualBitmapFrom80211TIM(char *TIM, unsigned int *PartialVirtualBitmap_Len);

/* 
   2007/11/03 By C.L. Chou 
   Challenge Text Field:
    -- Element ID (1 octet)
    -- Length (1 octet)
    -- Challenge Text (1~253 octets)
*/
void *GetChallengeTextFromChallengeText(char *ChallengeText, unsigned int *ChallengeText_Len);

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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211BeaconFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS, bool WithTIM);
unsigned int Calculate80211BeaconFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, unsigned int PartialVirtualBitmap_Len);
/*==Set===============================================*/
void SetTimestampInto80211BeaconFrameBody(char *Body, const unsigned long long *Timestamp);
void SetBeaconIntervalInto80211BeaconFrameBody(char *Body, const unsigned short *Interval);
void SetCapabilityInfoInto80211BeaconFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetSSIDInto80211BeaconFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetFHParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex);
void SetDSParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel);
void SetCFParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration, const unsigned short *DurRemaining);
void SetIBSSParameterSetInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window);
void SetTIMInto80211BeaconFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, u_char Bitmap_Ctrl, unsigned int PartialVirtualBitmap_Len, const void *PartialVirtualBitmap);
/*==Get===============================================*/
void GetTimestampFrom80211BeaconFrameBody(char *Body, unsigned long long *Timestamp);
void GetBeaconIntervalFrom80211BeaconFrameBody(char *Body, unsigned short *Interval);
char *GetCapabilityInfoFrom80211BeaconFrameBody(char *Body);
void *GetSSIDFrom80211BeaconFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetFHParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetDSParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetCFParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetIBSSParameterSetFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetTIMFrom80211BeaconFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Disassociation frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211DisassociationFrameBodyLength();
/*==Set===============================================*/
void SetReasonCodeInto80211DisassociationFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
void GetReasonCodeFrom80211DisassociationFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Association Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211AssociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211AssociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetListenIntervalInto80211AssociationRequestFrameBody(char *Body, const unsigned short *Interval);
void SetSSIDInto80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211AssociationRequestFrameBody(char *Body);
void GetListenIntervalFrom80211AssociationRequestFrameBody(char *Body, unsigned short *Interval);
void *GetSSIDFrom80211AssociationRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211AssociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Association Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211AssociationResponseFrameBodyLength(unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211AssociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetStatusCodeInto80211AssociationResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetAIDInto80211AssociationResponseFrameBody(char *Body, const unsigned short *AID);
void SetSupportedRatesInto80211AssociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211AssociationResponseFrameBody(char *Body);
void GetStatusCodeFrom80211AssociationResponseFrameBody(char *Body, unsigned short *StatusCode);
void GetAIDFrom80211AssociationResponseFrameBody(char *Body, unsigned short *AID);
void *GetSupportedRatesFrom80211AssociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len);
/*=================================================*/

/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Reassociation Request frame body:

    -- Capability Information (2 octets)
    -- Listen Interval (2 octets)
    -- Current AP address (6 octets)
    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ReassociationRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211ReassociationRequestFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetListenIntervalInto80211ReassociationRequestFrameBody(char *Body, const unsigned short *ListenInterval);
void SetCurrentAPAddressInto80211ReassociationRequestFrameBody(char *Body, const void * CurrentAPAddress);
void SetSSIDInto80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211ReassociationRequestFrameBody(char *Body);
void GetListenIntervalFrom80211ReassociationRequestFrameBody(char *Body, unsigned short *ListenInterval);
void GetCurrentAPAddressFrom80211ReassociationRequestFrameBody(char *Body, void * CurrentAPAddress);
void *GetSSIDFrom80211ReassociationRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211ReassociationRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Reassociation Response frame body:

    -- Capability Information (2 octets)
    -- Status Code (2 octets)
    -- Association ID (AID) (2 octets)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ReassociationResponseFrameBodyLength(unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetCapabilityInfoInto80211ReassociationResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetStatusCodeInto80211ReassociationResponseFrameBody(char *Body, const unsigned short *StatusCode);
void SetAIDInto80211ReassociationResponseFrameBody(char *Body, const unsigned short *AID);
void SetSupportedRatesInto80211ReassociationResponseFrameBody(char *Body, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
char *GetCapabilityInfoFrom80211ReassociationResponseFrameBody(char *Body);
void GetStatusCodeFrom80211ReassociationResponseFrameBody(char *Body, unsigned short *StatusCode);
void GetAIDFrom80211ReassociationResponseFrameBody(char *Body, unsigned short *AID);
void *GetSupportedRatesFrom80211ReassociationResponseFrameBody(char *Body, unsigned int *SupportedRates_Len);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Probe Request frame body:

    -- SSID (1_ElementID + 1_Length + 0~32_SSID)
    -- Supported Rates (1_ElementID + 1_Length + 1~8_Supported_Rates)
*/
unsigned int Calculate80211ProbeRequestFrameBodyLength(unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetSSIDInto80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
/*==Get===============================================*/
void *GetSSIDFrom80211ProbeRequestFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211ProbeRequestFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
/*=================================================*/


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
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211ProbeResponseFrameBody(bool WithFH, bool WithDS, bool WithCF, bool WithIBSS);
unsigned int Calculate80211ProbeResponseFrameBodyLength(u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*==Set===============================================*/
void SetTimestampInto80211ProbeResponseFrameBody(char *Body, const unsigned long long *Timestamp);
void SetBeaconIntervalInto80211ProbeResponseFrameBody(char *Body, const unsigned short *Interval);
void SetCapabilityInfoInto80211ProbeResponseFrameBody(char *Body, bool ESS, bool IBSS, bool CF_Pollable, bool CF_Poll_Request, bool Privacy);
void SetSSIDInto80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, const void *SSID);
void SetSupportedRatesInto80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len, const void *SupportedRates);
void SetFHParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *DwellTime, u_char HopSet, u_char HopPattern, u_char HopIndex);
void SetDSParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char CurrentChannel);
void SetCFParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, u_char Count, u_char Period, const unsigned short *MaxDuration, const unsigned short *DurRemaining);
void SetIBSSParameterSetInto80211ProbeResponseFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int SSID_Len, unsigned int SupportedRates_Len, const unsigned short *ATIM_Window);
/*==Get===============================================*/
void GetTimestampFrom80211ProbeResponseFrameBody(char *Body, unsigned long long *Timestamp);
void GetBeaconIntervalFrom80211ProbeResponseFrameBody(char *Body, unsigned short *Interval);
char *GetCapabilityInfoFrom80211ProbeResponseFrameBody(char *Body);
void *GetSSIDFrom80211ProbeResponseFrameBody(char *Body, unsigned int *SSID_Len);
void *GetSupportedRatesFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int *SupportedRates_Len);
char *GetFHParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetDSParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetCFParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
char *GetIBSSParameterSetFrom80211ProbeResponseFrameBody(char *Body, unsigned int SSID_Len, unsigned int SupportedRates_Len);
/*=================================================*/



/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Authentication frame body:

    -- Authentication Algorithm Number (2 octets)
    -- Authentication Transaction Sequence Number (2 octets)
    -- Status Code (2 octets)
    -- Challenge Text (1_ElementID + 1_Length + 1~253_Challenge_Text) <optional>
*/
u_char SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211AuthenticationFrameBody(bool WithChallengeText);
unsigned int Calculate80211AuthenticationFrameBodyLength(u_char OptionalInfoFlag, unsigned int ChallengeText_Len);
/*==Set===============================================*/
void SetAuthAlgoNumInto80211AuthenticationFrameBody(char *Body, const unsigned short *AuthAlgoNum);
void SetAuthTransSeqNumInto80211AuthenticationFrameBody(char *Body, const unsigned short *AuthTransSeqNum);
void SetStatusCodeInto80211AuthenticationFrameBody(char *Body, const unsigned short *StatusCode);
void SetChallengeTextInto80211AuthenticationFrameBody(char *Body, u_char OptionalInfoFlag, unsigned int ChallengeText_Len, const void *ChallengeText);
/*==Get===============================================*/
void GetAuthAlgoNumFrom80211AuthenticationFrameBody(char *Body, unsigned short *AuthAlgoNum);
void GetAuthTransSeqNumFrom80211AuthenticationFrameBody(char *Body, unsigned short *AuthTransSeqNum);
void GetStatusCodeFrom80211AuthenticationFrameBody(char *Body, unsigned short *StatusCode);
char *GetChallengeTextFrom80211AuthenticationFrameBody(char *Body);
/*=================================================*/


/*
   2007/11/03 By C.L. Chou
   Information contained in 80211 Deauthentication frame body:

    -- Reason Code (2 octets)
*/
unsigned int Calculate80211DeauthenticationFrameBodyLength();
/*==Set===============================================*/
void SetReasonCodeInto80211DeauthenticationFrameBody(char *Body, const unsigned short *ReasonCode);
/*==Get===============================================*/
void GetReasonCodeFrom80211DeauthenticationFrameBody(char *Body, unsigned short *ReasonCode);
/*=================================================*/


#endif	/* __NCTUNS_mac_802_11_management_framebody_h__ */
