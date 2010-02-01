#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	unsigned int		SSID_Len;
	unsigned int		SupportedRates_Len;

	unsigned int		framebody_len;
	char			*framebody;
	
	unsigned int		SSID;

	char			*SupportedRates;

	void			*tmp_ptr;

	/* Build frame body */
	SSID_Len = 4;
	SupportedRates_Len = 8;
	framebody_len = Calculate80211eProbeRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	SSID = 12;
	SetSSIDInto80211eProbeRequestFrameBody(framebody, SSID_Len, &SSID);
	printf("\e[0;31;40mSet\e[m SSID = %d\n", SSID);

	SupportedRates = (char *)malloc(SupportedRates_Len);
	strncpy(SupportedRates, "12345678", SupportedRates_Len);
	SetSupportedRatesInto80211eProbeRequestFrameBody(framebody, SSID_Len, SupportedRates_Len, (void *)SupportedRates);
	printf("\e[0;31;40mSet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
	if(SupportedRates != NULL && SupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m SupportedRates -> %s\n", SupportedRates);
	free((void *)SupportedRates);

	/*==Get=========================*/
	SSID_Len = 0;
	tmp_ptr = GetSSIDFrom80211eProbeRequestFrameBody(framebody, &SSID_Len);
	if(tmp_ptr != NULL && SSID_Len != 0) {
		bcopy(tmp_ptr, (void *)&SSID, SSID_Len);
		printf("\e[0;32;40mGet\e[m SSID = %d\n", SSID);
	}

	tmp_ptr = GetSupportedRatesFrom80211eProbeRequestFrameBody(framebody, SSID_Len, &SupportedRates_Len);
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

