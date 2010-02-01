#include "../mac-802_11_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	
	bool			ESS;
	bool			IBSS;
	bool			CFPollable;
	bool			CFPollRequest;
	bool			Privacy;

	unsigned short		StatusCode;

	unsigned short		AID;

	char			*SupportedRates;

	char			*CapabilityInfo;

	void			*tmp_ptr;

	/* Build frame body */
	SupportedRates_Len = 8;
	framebody_len = Calculate80211ReassociationResponseFrameBodyLength(SupportedRates_Len);

	framebody = (char *)malloc(framebody_len);
	InitializeFrameBodyWithZero(framebody, framebody_len);

	ESS = true;
	IBSS = false;
	CFPollable = true;
	CFPollRequest = false;
	Privacy = true;
	SetCapabilityInfoInto80211ReassociationResponseFrameBody(framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy);
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

	StatusCode = 2;
	SetStatusCodeInto80211ReassociationResponseFrameBody(framebody, &StatusCode);
	printf("\e[0;31;40mSet\e[m StatusCode = %d\n", StatusCode);

	AID = 12;
	SetAIDInto80211ReassociationResponseFrameBody(framebody, &AID);
	printf("\e[0;31;40mSet\e[m AID = %d\n", AID);

	SupportedRates = (char *)malloc(SupportedRates_Len);
	strncpy(SupportedRates, "12345678", SupportedRates_Len);
	SetSupportedRatesInto80211ReassociationResponseFrameBody(framebody, SupportedRates_Len, (void *)SupportedRates);
	printf("\e[0;31;40mSet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
	if(SupportedRates != NULL && SupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m SupportedRates -> %s\n", SupportedRates);
	free((void *)SupportedRates);

	/*==Get=========================*/
	CapabilityInfo = GetCapabilityInfoFrom80211ReassociationResponseFrameBody(framebody);
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

	GetStatusCodeFrom80211ReassociationResponseFrameBody(framebody, &StatusCode);
	printf("\e[0;32;40mGet\e[m StatusCode = %d\n", StatusCode);

	GetAIDFrom80211ReassociationResponseFrameBody(framebody, &AID);
	printf("\e[0;32;40mGet\e[m AID = %d\n", AID);

	tmp_ptr = GetSupportedRatesFrom80211ReassociationResponseFrameBody(framebody, &SupportedRates_Len);
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

