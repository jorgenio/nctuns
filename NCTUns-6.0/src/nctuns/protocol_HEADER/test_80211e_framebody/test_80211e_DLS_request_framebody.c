#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			SupportedRates_Len;
	unsigned int			ExtendedSupportedRates_Len;

	unsigned int			framebody_len;
	char				*framebody;

	void				*tmp_void_ptr;

	/* Build frame body */
	SupportedRates_Len = 8;
	ExtendedSupportedRates_Len = 15;
	framebody_len = Calculate80211eDLSRequestFrameBodyLength(SupportedRates_Len, ExtendedSupportedRates_Len);

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x02;
	u_char Action = 0x00;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	u_char DestinationMACAddress[6] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
	SetDestinationMACAddressInto80211eDLSRequestFrameBody(framebody, (void *)DestinationMACAddress);
	printf("\e[0;31;40mSet\e[m DestinationMACAddress = %x:%x:%x:%x:%x:%x\n", DestinationMACAddress[0], DestinationMACAddress[1],DestinationMACAddress[2],DestinationMACAddress[3],DestinationMACAddress[4],DestinationMACAddress[5]);	

	u_char SourceMACAddress[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	SetSourceMACAddressInto80211eDLSRequestFrameBody(framebody, (void *)SourceMACAddress);
	printf("\e[0;31;40mSet\e[m SourceMACAddress = %x:%x:%x:%x:%x:%x\n", SourceMACAddress[0], SourceMACAddress[1],SourceMACAddress[2],SourceMACAddress[3],SourceMACAddress[4],SourceMACAddress[5]);	


	bool ESS = true;
	bool IBSS = false;
	bool CFPollable = true;
	bool CFPollRequest = false;
	bool Privacy = true;
	bool ShortPreamble = true;
	bool PBCC = false;
	bool ChannelAgility = true;
	bool SpectrumManagement = false;
	bool QoS = true;
	bool ShortSlotTime = false;
	bool APSD = true;
	bool DSSS_OFDM = false;
	bool DelayedBloackAck = true;
	bool ImmediateBlockAck = false;
	SetCapabilityInfoInto80211eDLSRequestFrameBody(framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy, ShortPreamble, PBCC, ChannelAgility, SpectrumManagement, QoS, ShortSlotTime, APSD, DSSS_OFDM, DelayedBloackAck, ImmediateBlockAck);
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
	if(ShortPreamble)
	   printf("\e[0;31;40mSet\e[m ShortPreamble = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m ShortPreamble = 0\n");
	if(PBCC)
	   printf("\e[0;31;40mSet\e[m PBCC = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m PBCC = 0\n");
	if(ChannelAgility)
	   printf("\e[0;31;40mSet\e[m ChannelAgility = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m ChannelAgility = 0\n");
	if(SpectrumManagement)
	   printf("\e[0;31;40mSet\e[m SpectrumManagement = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m SpectrumManagement = 0\n");
	if(QoS)
	   printf("\e[0;31;40mSet\e[m QoS = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m QoS = 0\n");
	if(ShortSlotTime)
	   printf("\e[0;31;40mSet\e[m ShortSlotTime = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m ShortSlotTime = 0\n");
	if(APSD)
	   printf("\e[0;31;40mSet\e[m APSD = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m APSD = 0\n");
	if(DSSS_OFDM)
	   printf("\e[0;31;40mSet\e[m DSSS_OFDM = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m DSSS_OFDM = 0\n");
	if(DelayedBloackAck)
	   printf("\e[0;31;40mSet\e[m DelayedBloackAck = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m DelayedBloackAck = 0\n");
	if(ImmediateBlockAck)
	   printf("\e[0;31;40mSet\e[m ImmediateBlockAck = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m ImmediateBlockAck = 0\n");

	unsigned short DLSTimeoutValue = 22;
	SetDLSTimeoutValueInto80211eDLSRequestFrameBody(framebody, &DLSTimeoutValue);
	printf("\e[0;31;40mSet\e[m DLSTimeoutValue = %d\n", DLSTimeoutValue);

	char *SupportedRates = (char *)malloc(SupportedRates_Len);
	strncpy(SupportedRates, "12345678", SupportedRates_Len);
	SetSupportedRatesInto80211eDLSRequestFrameBody(framebody, SupportedRates_Len, (void *)SupportedRates);
	printf("\e[0;31;40mSet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
	if(SupportedRates != NULL && SupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m SupportedRates -> %s\n", SupportedRates);
	free((void *)SupportedRates);

	char *ExtendedSupportedRates = (char *)malloc(ExtendedSupportedRates_Len);
	strncpy(ExtendedSupportedRates, "123456789abcdef", ExtendedSupportedRates_Len);
	SetExtendedSupportedRatesInto80211eDLSRequestFrameBody(framebody, SupportedRates_Len, ExtendedSupportedRates_Len, (void *)ExtendedSupportedRates);
	printf("\e[0;31;40mSet\e[m ExtendedSupportedRates_Len = %d\n", ExtendedSupportedRates_Len);
	if(ExtendedSupportedRates != NULL && ExtendedSupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m ExtendedSupportedRates -> %s\n", ExtendedSupportedRates);
	free((void *)ExtendedSupportedRates);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	GetDestinationMACAddressFrom80211eDLSRequestFrameBody(framebody, (void *)DestinationMACAddress);
	printf("\e[0;32;40mGet\e[m DestinationMACAddress = %x:%x:%x:%x:%x:%x\n", DestinationMACAddress[0], DestinationMACAddress[1],DestinationMACAddress[2],DestinationMACAddress[3],DestinationMACAddress[4],DestinationMACAddress[5]);	

	GetSourceMACAddressFrom80211eDLSRequestFrameBody(framebody, (void *)SourceMACAddress);
	printf("\e[0;32;40mGet\e[m SourceMACAddress = %x:%x:%x:%x:%x:%x\n", SourceMACAddress[0], SourceMACAddress[1],SourceMACAddress[2],SourceMACAddress[3],SourceMACAddress[4],SourceMACAddress[5]);	

	char *CapabilityInfo = GetCapabilityInfoFrom80211eDLSRequestFrameBody(framebody);
	if(GetESSFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ESS = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ESS = 0\n");
	if(GetIBSSFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m IBSS = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m IBSS = 0\n");
	if(GetCFPollableFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m CFPollable = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m CFPollable = 0\n");
	if(GetCFPollRequestFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m CFPollRequest = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m CFPollRequest = 0\n");
	if(GetPrivacyFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m Privacy = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m Privacy = 0\n");
	if(GetShortPreambleFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ShortPreamble = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ShortPreamble = 0\n");
	if(GetPBCCFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m PBCC = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m PBCC = 0\n");
	if(GetChannelAgilityFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ChannelAgility = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ChannelAgility = 0\n");
	if(GetSpectrumManagementFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m SpectrumManagement = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m SpectrumManagement = 0\n");
	if(GetQoSFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m QoS = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m QoS = 0\n");
	if(GetShortSlotTimeFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ShortSlotTime = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ShortSlotTime = 0\n");
	if(GetAPSDFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m APSD = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m APSD = 0\n");
	if(GetDSSSOFDMFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m DSSS_OFDM = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m DSSS_OFDM = 0\n");
	if(GetDelayedBlockAckFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m DelayedBloackAck = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m DelayedBloackAck = 0\n");
	if(GetImmediateBlockAckFrom80211eCapabilityInfo(CapabilityInfo) == true)
	   printf("\e[0;32;40mGet\e[m ImmediateBlockAck = 1\n");
	else
	   printf("\e[0;32;40mGet\e[m ImmediateBlockAck = 0\n");

	GetDLSTimeoutValueFrom80211eDLSRequestFrameBody(framebody, &DLSTimeoutValue);
	printf("\e[0;32;40mGet\e[m DLSTimeoutValue = %d\n", DLSTimeoutValue);

	tmp_void_ptr = GetSupportedRatesFrom80211eDLSRequestFrameBody(framebody, &SupportedRates_Len);
	if(tmp_void_ptr != NULL && SupportedRates_Len != 0) {
		SupportedRates = (char *)malloc(SupportedRates_Len);
		strncpy(SupportedRates, (char *)tmp_void_ptr, SupportedRates_Len);
		SupportedRates[SupportedRates_Len] = '\0';
		printf("\e[0;32;40mGet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
		printf("\e[0;32;40mGet\e[m SupportedRates -> %s\n", SupportedRates);
		free((void *)SupportedRates);
	}

	tmp_void_ptr = GetExtendedSupportedRatesFrom80211eDLSRequestFrameBody(framebody, SupportedRates_Len, &ExtendedSupportedRates_Len);
	if(tmp_void_ptr != NULL && ExtendedSupportedRates_Len != 0) {
		ExtendedSupportedRates = (char *)malloc(ExtendedSupportedRates_Len);
		strncpy(ExtendedSupportedRates, (char *)tmp_void_ptr, ExtendedSupportedRates_Len);
		ExtendedSupportedRates[ExtendedSupportedRates_Len] = '\0';
		printf("\e[0;32;40mGet\e[m ExtendedSupportedRates_Len = %d\n", ExtendedSupportedRates_Len);
		printf("\e[0;32;40mGet\e[m ExtendedSupportedRates -> %s\n", ExtendedSupportedRates);
		free((void *)ExtendedSupportedRates);
	}

	free(framebody);
	return 0;
}
