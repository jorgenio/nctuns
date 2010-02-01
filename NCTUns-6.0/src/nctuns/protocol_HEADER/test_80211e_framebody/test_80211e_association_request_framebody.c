#include "../mac-802_11e_management_framebody.h"
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
	bool			ShortPreamble;
	bool			PBCC;
	bool			ChannelAgility;
	bool			SpectrumManagement;
	bool			QoS;
	bool			ShortSlotTime;
	bool			APSD;
	bool			DSSS_OFDM;
	bool			DelayedBloackAck;
	bool			ImmediateBlockAck;

	unsigned short		ListenInterval;

	unsigned int		SSID;

	char			*SupportedRates;

	u_char 			QoSInfo_EDCAParameterSetUpdateCount;
	bool 			QoSInfo_QAck;
	bool 			QoSInfo_QueueRequest;
	bool 			QoSInfo_TXOPRequest;

	char			*CapabilityInfo;
	char			*QoSInfo;

	void			*tmp_void_ptr;
	char			*tmp_char_ptr;

	/* Build frame body */
	SSID_Len = 4;
	SupportedRates_Len = 8;
	framebody_len = Calculate80211eAssociationRequestFrameBodyLength(SSID_Len, SupportedRates_Len);

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	ESS = true;
	IBSS = false;
	CFPollable = true;
	CFPollRequest = false;
	Privacy = true;
	ShortPreamble = true;
	PBCC = false;
	ChannelAgility = true;
	SpectrumManagement = false;
	QoS = true;
	ShortSlotTime = false;
	APSD = true;
	DSSS_OFDM = false;
	DelayedBloackAck = true;
	ImmediateBlockAck = false;
	SetCapabilityInfoInto80211eAssociationRequestFrameBody(framebody, ESS, IBSS, CFPollable, CFPollRequest, Privacy, ShortPreamble, PBCC, ChannelAgility, SpectrumManagement, QoS, ShortSlotTime, APSD, DSSS_OFDM, DelayedBloackAck, ImmediateBlockAck);
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

	ListenInterval = 2;
	SetListenIntervalInto80211eAssociationRequestFrameBody(framebody, &ListenInterval);
	printf("\e[0;31;40mSet\e[m ListenInterval = %d\n", ListenInterval);

	SSID = 12;
	SetSSIDInto80211eAssociationRequestFrameBody(framebody, SSID_Len, &SSID);
	printf("\e[0;31;40mSet\e[m SSID = %d\n", SSID);

	SupportedRates = (char *)malloc(SupportedRates_Len);
	strncpy(SupportedRates, "12345678", SupportedRates_Len);
	SetSupportedRatesInto80211eAssociationRequestFrameBody(framebody, SSID_Len, SupportedRates_Len, (void *)SupportedRates);
	printf("\e[0;31;40mSet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
	if(SupportedRates != NULL && SupportedRates_Len != 0) 
		printf("\e[0;31;40mSet\e[m SupportedRates -> %s\n", SupportedRates);
	free((void *)SupportedRates);

	QoSInfo_EDCAParameterSetUpdateCount = 0x0d;
	QoSInfo_QAck = false;
	QoSInfo_QueueRequest = true;
	QoSInfo_TXOPRequest = false;
	SetQoSCapabilityInto80211eAssociationRequestFrameBody(framebody, SSID_Len, SupportedRates_Len, QoSInfo_EDCAParameterSetUpdateCount, QoSInfo_QAck, QoSInfo_QueueRequest, QoSInfo_TXOPRequest);	
	printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_EDCAParameterSetUpdateCount = %x\n", QoSInfo_EDCAParameterSetUpdateCount);
	if(QoSInfo_QAck)
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_QAck = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_QAck = 0\n");
	if(QoSInfo_QueueRequest)
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_QueueRequest = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_QueueRequest = 0\n");
	if(QoSInfo_TXOPRequest)
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_TXOPRequest = 1\n");
	else
	   printf("\e[0;31;40mSet\e[m QoS Capability QoSInfo_TXOPRequest = 0\n");

	/*==Get=========================*/
	CapabilityInfo = GetCapabilityInfoFrom80211eAssociationRequestFrameBody(framebody);
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


	GetListenIntervalFrom80211eAssociationRequestFrameBody(framebody, &ListenInterval);
	printf("\e[0;32;40mGet\e[m ListenInterval = %d\n", ListenInterval);

	SSID_Len = 0;
	tmp_void_ptr = GetSSIDFrom80211eAssociationRequestFrameBody(framebody, &SSID_Len);
	if(tmp_void_ptr != NULL && SSID_Len != 0) {
		bcopy(tmp_void_ptr, (void *)&SSID, SSID_Len);
		printf("\e[0;32;40mGet\e[m SSID = %d\n", SSID);
	}

	tmp_void_ptr = GetSupportedRatesFrom80211eAssociationRequestFrameBody(framebody, SSID_Len, &SupportedRates_Len);
	if(tmp_void_ptr != NULL && SupportedRates_Len != 0) {
		SupportedRates = (char *)malloc(SupportedRates_Len);
		strncpy(SupportedRates, (char *)tmp_void_ptr, SupportedRates_Len);
		SupportedRates[SupportedRates_Len] = '\0';
		printf("\e[0;32;40mGet\e[m SupportedRates_Len = %d\n", SupportedRates_Len);
		printf("\e[0;32;40mGet\e[m SupportedRates -> %s\n", SupportedRates);
		free((void *)SupportedRates);
	}

	tmp_char_ptr = GetQoSCapabilityFrom80211eAssociationRequestFrameBody(framebody, SSID_Len, SupportedRates_Len);
	if(tmp_char_ptr != NULL) {
		QoSInfo = GetQoSInfoFromQoSCapability(tmp_char_ptr);
		if(QoSInfo != NULL) {
			if(GetEDCAParameterSetUpdateCountFromQoSInfo(QoSInfo, &QoSInfo_EDCAParameterSetUpdateCount) == true)
				printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_EDCAParameterSetUpdateCount = %x\n", QoSInfo_EDCAParameterSetUpdateCount);
			if(GetQAckFromQoSInfo(QoSInfo) == true) 
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_QAck = 1\n");
			else
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_QAck = 0\n");
			if(GetQueueRequestFromQoSInfo(QoSInfo) == true) 
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_QueueRequest = 1\n");
			else
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_QueueRequest = 0\n");
			if(GetTXOPRequestFromQoSInfo(QoSInfo) == true) 
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_TXOPRequest = 1\n");
			else
			   printf("\e[0;32;40mGet\e[m QoS Capability QoSInfo_TXOPRequest = 0\n");
		}
	}
	
	free(framebody);	
	return 0;
}

