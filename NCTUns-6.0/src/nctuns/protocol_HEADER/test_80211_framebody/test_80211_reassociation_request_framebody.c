#include "../mac-802_11_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	
	bool			ESS;
	bool			IBSS;
	bool			CFPollable;
	bool			CFPollRequest;
	bool			Privacy;

	unsigned short		ListenInterval;

	u_char			CurrentAPAddress[6];

	unsigned int		SSID;

	char			*SupportedRates;

	char			*CapabilityInfo;

	void			*tmp_ptr;

	/* Build frame body */
	SSID_Len = 4;
	SupportedRates_Len = 8;
	framebody_len = Calculate80211ReassociationRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	framebody = (char *)malloc(framebody_len);
	InitializeFrameBodyWithZero(framebody, framebody_len);

	ESS = true;
	IBSS = false;
	CFPollable = true;
	CFPollRequest = false;
	Privacy = true;
	SetCapabilityInfoInto80211ReassociationRequestFrameBody(framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy);
	if(ESS)
	   printf("\e[0;31;40mSet\e[m ESS = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m ESS = 0\n");
	if(IBSS)
	   printf("\e[0;31;40mSet\e[m IBSS = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m IBSS = 0\n");
	if(CFPollable)
	   printf("\e[0;31;40mSet\e[m CFPollable = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m CFPollable = 0\n");
	if(CFPollRequest)
	   printf("\e[0;31;40mSet\e[m CFPollRequest = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m CFPollRequest = 0\n");
	if(Privacy)
	   printf("\e[0;31;40mSet\e[m Privacy = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m Privacy = 0\n");

	ListenInterval = 2;
	SetListenIntervalInto80211ReassociationRequestFrameBody(framebody, &ListenInterval);
	printf("\e[0;31;40mSet\e[m ListenInterval = %d\n", ListenInterval);

	CurrentAPAddress[0] = 0xa1;
	CurrentAPAddress[1] = 0xb2;
	CurrentAPAddress[2] = 0xc3;
	CurrentAPAddress[3] = 0xd4;
	CurrentAPAddress[4] = 0xe5;
	CurrentAPAddress[5] = 0xf6;
	SetCurrentAPAddressInto80211ReassociationRequestFrameBody(framebody, (void *)CurrentAPAddress);
	printf("\e[0;31;40mSet\e[m CurrentAPAddress -> %x:%x:%x:%x:%x:%x\n", CurrentAPAddress[0], CurrentAPAddress[1], CurrentAPAddress[2], CurrentAPAddress[3], CurrentAPAddress[4], CurrentAPAddress[5]);

	SSID = 12;
	SetSSIDInto80211ReassociationRequestFrameBody(framebody, SSID_Len, &SSID);
	printf("\e[0;31;40mSet\e[m SSID = %d\n", SSID);

	SupportedRates = (char *)malloc(SupportedRates_Len);
	strncpy(SupportedRates, "12345678", SupportedRates_Len);
	SetSupportedRatesInto80211ReassociationRequestFrameBody(framebody, SSID_Len, SupportedRates_Len, (void *)SupportedRates);
	printf("\e[0;31;40mSet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
	if(SupportedRates != NULL && SupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m SupportedRates -> %s\n", SupportedRates);
	free((void *)SupportedRates);

	/*==Get=========================*/
	CapabilityInfo = GetCapabilityInfoFrom80211ReassociationRequestFrameBody(framebody);
	if(GetESSFrom80211CapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ESS = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ESS = 0\n");
	if(GetIBSSFrom80211CapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m IBSS = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m IBSS = 0\n");
	if(GetCFPollableFrom80211CapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m CFPollable = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m CFPollable = 0\n");
	if(GetCFPollRequestFrom80211CapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m CFPollRequest = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m CFPollRequest = 0\n");
	if(GetPrivacyFrom80211CapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m Privacy = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m Privacy = 0\n");

	GetListenIntervalFrom80211ReassociationRequestFrameBody(framebody, &ListenInterval);
	printf("\e[0;32;40mGet\e[m ListenInterval = %d\n", ListenInterval);

	GetCurrentAPAddressFrom80211ReassociationRequestFrameBody(framebody, (void *)CurrentAPAddress);
	printf("\e[0;32;40mGet\e[m CurrentAPAddress -> %x:%x:%x:%x:%x:%x\n", CurrentAPAddress[0], CurrentAPAddress[1], CurrentAPAddress[2], CurrentAPAddress[3], CurrentAPAddress[4], CurrentAPAddress[5]);

	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211ReassociationRequestFrameBody(framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0) {
		bcopy(tmp_ptr, (void *)&SSID, SSID_Len);
		printf("\e[0;32;40mGet\e[m SSID = %d\n", SSID);
	}

	tmp_ptr = GetSupportedRatesFrom80211ReassociationRequestFrameBody(framebody, SSID_Len, &SupportedRates_Len);
	if(tmp_ptr != NULL && SupportedRates_Len != 0) {
		SupportedRates = (char *)malloc(SupportedRates_Len);
		strncpy(SupportedRates, (char *)tmp_ptr, SupportedRates_Len);
		SupportedRates[SupportedRates_Len] = '\0';
		printf("\e[0;32;40mGet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
		printf("\e[0;32;40mGet\e[m SupportedRates -> %s\n", SupportedRates);
		free((void *)SupportedRates);
	}

	free(framebody);	
	return 0;
}

