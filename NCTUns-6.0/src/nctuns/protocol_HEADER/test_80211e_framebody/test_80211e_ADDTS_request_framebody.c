#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	u_char				optional_info_flag;
	bool				WithTCLAS;
	enum TCLAS_FrameClassifierType	TCLASFrameClassifierType;
	bool				WithTCLASProcessing;
	
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	WithTCLAS = true;
	TCLASFrameClassifierType = ClassifierType_0;
	//TCLASFrameClassifierType = ClassifierType_1_IPv4;
	//TCLASFrameClassifierType = ClassifierType_1_IPv6;
	//TCLASFrameClassifierType = ClassifierType_2;
	WithTCLASProcessing = true;	

	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSRequestFrameBody(WithTCLAS, TCLASFrameClassifierType, WithTCLASProcessing);

	framebody_len = Calculate80211eADDTSRequestFrameBodyLength(optional_info_flag);

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x01;
	u_char Action = 0x00;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	u_char DialogToken = 0x11;
	SetDialogTokenInto80211eADDTSRequestFrameBody(framebody, DialogToken);
	printf("\e[0;31;40mSet\e[m DialogToken = %x\n", DialogToken);

	bool TSInfo_TrafficType = true;
	u_char TSInfo_TSID = 0x0f; // 4-bits 
	bool TSInfo_Direction_Bit_1 = true;
	bool TSInfo_Direction_Bit_2 = false;
	bool TSInfo_AccessPolicy_Bit_1 = true;
	bool TSInfo_AccessPolicy_Bit_2 = false;
	bool TSInfo_Aggregation = true;
	bool TSInfo_APSD = false;
	u_char TSInfo_UserPriority = 0x04; // 0~7
	bool TSInfo_TSInfoAckPolicy_Bit_1 = true;
	bool TSInfo_TSInfoAckPolicy_Bit_2 = false;
	bool TSInfo_Schedule = true;
	unsigned short NominalMSDUSize = 555;
	bool NominalMSDUSize_Fixed = true;
	unsigned short MaximumMSDUSize = 999;
	unsigned int MinimumServiceInterval = 1313;
	unsigned int MaximumServiceInterval = 1414;
	unsigned int InactivityInterval = 1515;
	unsigned int SyspensionInterval = 1616;
	unsigned int ServiceStartTime = 1717;
	unsigned int MinimumDataRate = 1818;
	unsigned int MeanDataRate = 1919;
	unsigned int PeakDataRate = 2020;
	unsigned int BurstSize = 1500;
	unsigned int DelayBound = 2121;
	unsigned int MinimumPHYRate = 2323;
	unsigned short SurplusBandwidthAllowance_IntegerPart = 5; // 3-bits
	unsigned short SurplusBandwidthAllowance_DecimalPart = 4567; // 13-bits
	unsigned short MediumTime = 777;

	SetTSPECInto80211eADDTSRequestFrameBody(framebody, TSInfo_TrafficType, TSInfo_TSID, TSInfo_Direction_Bit_1, TSInfo_Direction_Bit_2, TSInfo_AccessPolicy_Bit_1, TSInfo_AccessPolicy_Bit_2, TSInfo_Aggregation, TSInfo_APSD, TSInfo_UserPriority, TSInfo_TSInfoAckPolicy_Bit_1, TSInfo_TSInfoAckPolicy_Bit_2, TSInfo_Schedule, &NominalMSDUSize, NominalMSDUSize_Fixed, &MaximumMSDUSize, &MinimumServiceInterval, &MaximumServiceInterval, &InactivityInterval, &SyspensionInterval, &ServiceStartTime, &MinimumDataRate, &MeanDataRate, &PeakDataRate, &BurstSize, &DelayBound, &MinimumPHYRate, &SurplusBandwidthAllowance_IntegerPart, &SurplusBandwidthAllowance_DecimalPart, &MediumTime);
	if(TSInfo_TrafficType)	
		printf("\e[0;31;40mSet\e[m TSInfo_TrafficType = 1\n");
	else			
		printf("\e[0;31;40mSet\e[m TSInfo_TrafficType = 0\n");
	printf("\e[0;31;40mSet\e[m TSInfo_TSID = %x\n", TSInfo_TSID);	
	if(TSInfo_Direction_Bit_1)	
		printf("\e[0;31;40mSet\e[m TSInfo_Direction_Bit_1 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m TSInfo_Direction_Bit_1 = 0\n");
	if(TSInfo_Direction_Bit_2)	
		printf("\e[0;31;40mSet\e[m TSInfo_Direction_Bit_2 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m TSInfo_Direction_Bit_2 = 0\n");
	if(TSInfo_AccessPolicy_Bit_1)	
		printf("\e[0;31;40mSet\e[m TSInfo_AccessPolicy_Bit_1 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m TSInfo_AccessPolicy_Bit_1 = 0\n");
	if(TSInfo_AccessPolicy_Bit_2)	
		printf("\e[0;31;40mSet\e[m TSInfo_AccessPolicy_Bit_2 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m TSInfo_AccessPolicy_Bit_2 = 0\n");
	if(TSInfo_Aggregation)	
		printf("\e[0;31;40mSet\e[m TSInfo_Aggregation = 1\n");
	else                    
		printf("\e[0;31;40mSet\e[m TSInfo_Aggregation = 0\n");
	if(TSInfo_APSD)	
		printf("\e[0;31;40mSet\e[m TSInfo_APSD = 1\n");
	else            
		printf("\e[0;31;40mSet\e[m TSInfo_APSD = 0\n");
	printf("\e[0;31;40mSet\e[m TSInfo_UserPriority = %x\n", TSInfo_UserPriority);	
	if(TSInfo_TSInfoAckPolicy_Bit_1)
		printf("\e[0;31;40mSet\e[m TSInfo_TSInfoAckPolicy_Bit_1 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m TSInfo_TSInfoAckPolicy_Bit_1 = 0\n");
	if(TSInfo_TSInfoAckPolicy_Bit_2)
		printf("\e[0;31;40mSet\e[m TSInfo_TSInfoAckPolicy_Bit_2 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m TSInfo_TSInfoAckPolicy_Bit_2 = 0\n");
	if(TSInfo_Schedule)
		printf("\e[0;31;40mSet\e[m TSInfo_Schedule = 1\n");
	else
		printf("\e[0;31;40mSet\e[m TSInfo_Schedule = 0\n");
	printf("\e[0;31;40mSet\e[m NominalMSDUSize = %d\n", NominalMSDUSize);	
	if(NominalMSDUSize_Fixed)
		printf("\e[0;31;40mSet\e[m NominalMSDUSize_Fixed = 1\n");
	else
		printf("\e[0;31;40mSet\e[m NominalMSDUSize_Fixed = 0\n");
	printf("\e[0;31;40mSet\e[m MaximumMSDUSize = %d\n", MaximumMSDUSize);	
	printf("\e[0;31;40mSet\e[m MinimumServiceInterval = %d\n", MinimumServiceInterval);	
	printf("\e[0;31;40mSet\e[m MaximumServiceInterval = %d\n", MaximumServiceInterval);	
	printf("\e[0;31;40mSet\e[m InactivityInterval = %d\n", InactivityInterval);	
	printf("\e[0;31;40mSet\e[m SyspensionInterval = %d\n", SyspensionInterval);	
	printf("\e[0;31;40mSet\e[m ServiceStartTime = %d\n", ServiceStartTime);	
	printf("\e[0;31;40mSet\e[m MinimumDataRate = %d\n", MinimumDataRate);	
	printf("\e[0;31;40mSet\e[m MeanDataRate = %d\n", MeanDataRate);	
	printf("\e[0;31;40mSet\e[m PeakDataRate = %d\n", PeakDataRate);	
	printf("\e[0;31;40mSet\e[m BurstSize = %d\n", BurstSize);	
	printf("\e[0;31;40mSet\e[m DelayBound = %d\n", DelayBound);	
	printf("\e[0;31;40mSet\e[m MinimumPHYRate = %d\n", MinimumPHYRate);	
	printf("\e[0;31;40mSet\e[m SurplusBandwidthAllowance_IntegerPart = %d\n", SurplusBandwidthAllowance_IntegerPart);	
	printf("\e[0;31;40mSet\e[m SurplusBandwidthAllowance_DecimalPart = %d\n", SurplusBandwidthAllowance_DecimalPart);	
	printf("\e[0;31;40mSet\e[m MediumTime = %d\n", MediumTime);	


	u_char UserPriority = 0x04; // 0~7
	u_char ClassifierMask = 0xff; 
	u_char Version = 0x05;
	unsigned short SourcePort = 1111;
	unsigned short DestinationPort = 2222;
	/* ClassifierType_0 */
	u_char SourceAddress[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	u_char DestinationAddress[6] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
	unsigned short Type = 3333;
	/* ClassifierType_1_V4 */
	u_char SourceIPAddressV4[4] = {0x11, 0x22, 0x33, 0x44};
	u_char DestinationIPAddressV4[4] = {0x44, 0x33, 0x22, 0x11};
	u_char DSCP = 0x3f; // 6-bits
	u_char Protocol = 0xaa;
	/* ClassifierType_1_V6 */
	u_char SourceIPAddressV6[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	u_char DestinationIPAddressV6[16] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
	u_char FlowLabel[3] = {'A', 'B', 'C'};
	/* ClassifierType_2 */
	unsigned short TagType = 33;

	if(TCLASFrameClassifierType == ClassifierType_0) {
		SetTCLASwithFrameClassifierType0Into80211eADDTSRequestFrameBody(framebody, optional_info_flag, UserPriority, ClassifierMask, (void *)SourceAddress, (void *)DestinationAddress, &Type);
		if(WithTCLAS) {
			printf("\e[0;31;40mSet\e[m UserPriority = %x\n", UserPriority);	
			printf("\e[0;31;40mSet\e[m ClassifierMask = %x\n", ClassifierMask);	
			printf("\e[0;31;40mSet\e[m SourceAddress = %x:%x:%x:%x:%x:%x\n", SourceAddress[0], SourceAddress[1],SourceAddress[2],SourceAddress[3],SourceAddress[4],SourceAddress[5]);	
			printf("\e[0;31;40mSet\e[m DestinationAddress = %x:%x:%x:%x:%x:%x\n", DestinationAddress[0], DestinationAddress[1],DestinationAddress[2],DestinationAddress[3],DestinationAddress[4],DestinationAddress[5]);	
			printf("\e[0;31;40mSet\e[m Type = %d\n", Type);	
		}
	}
	else if(TCLASFrameClassifierType == ClassifierType_1_IPv4) {
		SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSRequestFrameBody(framebody, optional_info_flag, UserPriority, ClassifierMask, Version, (void *)SourceIPAddressV4, (void *)DestinationIPAddressV4, &SourcePort, &DestinationPort, DSCP, Protocol);
		if(WithTCLAS) {
			printf("\e[0;31;40mSet\e[m UserPriority = %x\n", UserPriority);	
			printf("\e[0;31;40mSet\e[m ClassifierMask = %x\n", ClassifierMask);	
			printf("\e[0;31;40mSet\e[m Version = %x\n", Version);	
			printf("\e[0;31;40mSet\e[m SourceIPAddressV4 = %d.%d.%d.%d\n", (unsigned int)SourceIPAddressV4[0], (unsigned int)SourceIPAddressV4[1],(unsigned int)SourceIPAddressV4[2],(unsigned int)SourceIPAddressV4[3]);	
			printf("\e[0;31;40mSet\e[m DestinationIPAddressV4 = %d.%d.%d.%d\n", (unsigned int)DestinationIPAddressV4[0], (unsigned int)DestinationIPAddressV4[1],(unsigned int)DestinationIPAddressV4[2],(unsigned int)DestinationIPAddressV4[3]);	
			printf("\e[0;31;40mSet\e[m SourcePort = %d\n", SourcePort);	
			printf("\e[0;31;40mSet\e[m DestinationPort = %d\n", DestinationPort);	
			printf("\e[0;31;40mSet\e[m DSCP = %x\n", DSCP);	
			printf("\e[0;31;40mSet\e[m Protocol = %x\n", Protocol);	
		}
	}
	else if(TCLASFrameClassifierType == ClassifierType_1_IPv6) {
		SetTCLASwithFrameClassifierType1ForIPv6Into80211eADDTSRequestFrameBody(framebody, optional_info_flag, UserPriority, ClassifierMask, Version, (void *)SourceIPAddressV6, (void *)DestinationIPAddressV6, &SourcePort, &DestinationPort, (void *)FlowLabel);
		if(WithTCLAS) {
			printf("\e[0;31;40mSet\e[m UserPriority = %x\n", UserPriority);	
			printf("\e[0;31;40mSet\e[m ClassifierMask = %x\n", ClassifierMask);	
			printf("\e[0;31;40mSet\e[m Version = %x\n", Version);	
			printf("\e[0;31;40mSet\e[m SourceIPAddressV6 = %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", (unsigned int)SourceIPAddressV6[0], (unsigned int)SourceIPAddressV6[1],(unsigned int)SourceIPAddressV6[2],(unsigned int)SourceIPAddressV6[3], (unsigned int)SourceIPAddressV6[4], (unsigned int)SourceIPAddressV6[5],(unsigned int)SourceIPAddressV6[6],(unsigned int)SourceIPAddressV6[7], (unsigned int)SourceIPAddressV6[8], (unsigned int)SourceIPAddressV6[9],(unsigned int)SourceIPAddressV6[10],(unsigned int)SourceIPAddressV6[11], (unsigned int)SourceIPAddressV6[12], (unsigned int)SourceIPAddressV6[13],(unsigned int)SourceIPAddressV6[14],(unsigned int)SourceIPAddressV6[15]);	
			printf("\e[0;31;40mSet\e[m DestinationIPAddressV6 = %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", (unsigned int)DestinationIPAddressV6[0], (unsigned int)DestinationIPAddressV6[1],(unsigned int)DestinationIPAddressV6[2],(unsigned int)DestinationIPAddressV6[3], (unsigned int)DestinationIPAddressV6[4], (unsigned int)DestinationIPAddressV6[5],(unsigned int)DestinationIPAddressV6[6],(unsigned int)DestinationIPAddressV6[7], (unsigned int)DestinationIPAddressV6[8], (unsigned int)DestinationIPAddressV6[9],(unsigned int)DestinationIPAddressV6[10],(unsigned int)DestinationIPAddressV6[11], (unsigned int)DestinationIPAddressV6[12], (unsigned int)DestinationIPAddressV6[13],(unsigned int)DestinationIPAddressV6[14],(unsigned int)DestinationIPAddressV6[15]);	
			printf("\e[0;31;40mSet\e[m SourcePort = %d\n", SourcePort);	
			printf("\e[0;31;40mSet\e[m DestinationPort = %d\n", DestinationPort);	
			printf("\e[0;31;40mSet\e[m FlowLabel = %c%c%c\n", FlowLabel[0], FlowLabel[1], FlowLabel[2]);	
		}
	}
	else if(TCLASFrameClassifierType == ClassifierType_2) {
		SetTCLASwithFrameClassifierType2Into80211eADDTSRequestFrameBody(framebody, optional_info_flag, UserPriority, ClassifierMask, &TagType);
		if(WithTCLAS) {
			printf("\e[0;31;40mSet\e[m UserPriority = %x\n", UserPriority);	
			printf("\e[0;31;40mSet\e[m ClassifierMask = %x\n", ClassifierMask);	
			printf("\e[0;31;40mSet\e[m TagType = %d\n", TagType);	
		}
	}

	u_char	Processing = 0x01; // 0,1,or 2
	SetTCLASProcessingInto80211eADDTSRequestFrameBody(framebody, optional_info_flag, Processing);
	if(WithTCLASProcessing)
		printf("\e[0;31;40mSet\e[m Processing = %x\n", Processing);	

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	GetDialogTokenFrom80211eADDTSRequestFrameBody(framebody, &DialogToken);
	printf("\e[0;32;40mGet\e[m DialogToken = %x\n", DialogToken);

	char *TSPEC = GetTSPECFrom80211eADDTSRequestFrameBody(framebody);	
	if(TSPEC != NULL) {
		char *TSInfo = GetTSInfoFromTSPEC(TSPEC);
		if(TSInfo != NULL) {
			if(GetTrafficTypeFromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m TrafficType = 1\n");
			else			
				printf("\e[0;32;40mGet\e[m TrafficType = 0\n");
			if(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID))
				printf("\e[0;32;40mGet\e[m TSID = %x\n", TSInfo_TSID);	
			if(GetDirectionBit1FromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m Direction_Bit_1 = 1\n");
			else                            
				printf("\e[0;32;40mGet\e[m Direction_Bit_1 = 0\n");
			if(GetDirectionBit2FromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m Direction_Bit_2 = 1\n");
			else                            
				printf("\e[0;32;40mGet\e[m Direction_Bit_2 = 0\n");
			if(GetAccessPolicyBit1FromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m AccessPolicy_Bit_1 = 1\n");
			else                            
				printf("\e[0;32;40mGet\e[m AccessPolicy_Bit_1 = 0\n");
			if(GetAccessPolicyBit2FromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m AccessPolicy_Bit_2 = 1\n");
			else                            
				printf("\e[0;32;40mGet\e[m AccessPolicy_Bit_2 = 0\n");
			if(GetAggregationFromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m Aggregation = 1\n");
			else                    
				printf("\e[0;32;40mGet\e[m Aggregation = 0\n");
			if(GetAPSDFromTSInfo(TSInfo))	
				printf("\e[0;32;40mGet\e[m APSD = 1\n");
			else            
				printf("\e[0;32;40mGet\e[m APSD = 0\n");
			if(GetUserPriorityFromTSInfo(TSInfo, &TSInfo_UserPriority))
				printf("\e[0;32;40mGet\e[m UserPriority = %x\n", TSInfo_UserPriority);	
			if(GetTSInfoAckPolicyBit1FromTSInfo(TSInfo))
				printf("\e[0;32;40mGet\e[m TSInfoAckPolicy_Bit_1 = 1\n");
			else
				printf("\e[0;32;40mGet\e[m TSInfoAckPolicy_Bit_1 = 0\n");
			if(GetTSInfoAckPolicyBit2FromTSInfo(TSInfo))
				printf("\e[0;32;40mGet\e[m TSInfoAckPolicy_Bit_2 = 1\n");
			else
				printf("\e[0;32;40mGet\e[m TSInfoAckPolicy_Bit_2 = 0\n");
			if(GetScheduleFromTSInfo(TSInfo))
				printf("\e[0;32;40mGet\e[m Schedule = 1\n");
			else
				printf("\e[0;32;40mGet\e[m Schedule = 0\n");
		}

		if(GetNominalMSDUSizeFromTSPEC(TSPEC, &NominalMSDUSize))
			printf("\e[0;32;40mGet\e[m NominalMSDUSize = %d\n", NominalMSDUSize);	
		if(GetNominalMSDUSizeFixedFromTSPEC(TSPEC))
			printf("\e[0;32;40mGet\e[m NominalMSDUSize_Fixed = 1\n");
		else
			printf("\e[0;32;40mGet\e[m NominalMSDUSize_Fixed = 0\n");
		if(GetMaximumMSDUSizeFromTSPEC(TSPEC, &MaximumMSDUSize))
			printf("\e[0;32;40mGet\e[m MaximumMSDUSize = %d\n", MaximumMSDUSize);	
		if(GetMinimumServiceIntervalFromTSPEC(TSPEC, &MinimumServiceInterval))
			printf("\e[0;32;40mGet\e[m MinimumServiceInterval = %d\n", MinimumServiceInterval);	
		if(GetMaximumServiceIntervalFromTSPEC(TSPEC, &MaximumServiceInterval))
			printf("\e[0;32;40mGet\e[m MaximumServiceInterval = %d\n", MaximumServiceInterval);	
		if(GetInactivityIntervalFromTSPEC(TSPEC, &InactivityInterval))
			printf("\e[0;32;40mGet\e[m InactivityInterval = %d\n", InactivityInterval);	
		if(GetSyspensionIntervalFromTSPEC(TSPEC, &SyspensionInterval))
			printf("\e[0;32;40mGet\e[m SyspensionInterval = %d\n", SyspensionInterval);	
		if(GetServiceStartTimeFromTSPEC(TSPEC, &ServiceStartTime))
			printf("\e[0;32;40mGet\e[m ServiceStartTime = %d\n", ServiceStartTime);	
		if(GetMinimumDataRateFromTSPEC(TSPEC, &MinimumDataRate))
			printf("\e[0;32;40mGet\e[m MinimumDataRate = %d\n", MinimumDataRate);	
		if(GetMeanDataRateFromTSPEC(TSPEC, &MeanDataRate))
			printf("\e[0;32;40mGet\e[m MeanDataRate = %d\n", MeanDataRate);	
		if(GetPeakDataRateFromTSPEC(TSPEC, &PeakDataRate))
			printf("\e[0;32;40mGet\e[m PeakDataRate = %d\n", PeakDataRate);	
		if(GetBurstSizeFromTSPEC(TSPEC, &BurstSize))
			printf("\e[0;32;40mGet\e[m BurstSize = %d\n", BurstSize);	
		if(GetDelayBoundFromTSPEC(TSPEC, &DelayBound))
			printf("\e[0;32;40mGet\e[m DelayBound = %d\n", DelayBound);	
		if(GetMinimumPHYRateFromTSPEC(TSPEC, &MinimumPHYRate))
			printf("\e[0;32;40mGet\e[m MinimumPHYRate = %d\n", MinimumPHYRate);	
		if(GetSurplusBandwidthAllowanceIntegerPartFromTSPEC(TSPEC, &SurplusBandwidthAllowance_IntegerPart))
			printf("\e[0;32;40mGet\e[m SurplusBandwidthAllowance_IntegerPart = %d\n", SurplusBandwidthAllowance_IntegerPart);	
		if(GetSurplusBandwidthAllowanceDecimalPartFromTSPEC(TSPEC, &SurplusBandwidthAllowance_DecimalPart))
			printf("\e[0;32;40mGet\e[m SurplusBandwidthAllowance_DecimalPart = %d\n", SurplusBandwidthAllowance_DecimalPart);	
		if(GetMediumTimeFromTSPEC(TSPEC, &MediumTime))
			printf("\e[0;32;40mGet\e[m MediumTime = %d\n", MediumTime);	
	}

	u_char FrameClassifierType = 0xff; // not equal to 0x00, 0x01 or 0x02
	char *TCLAS = GetTCLASFrom80211eADDTSRequestFrameBody(framebody, &FrameClassifierType);
	if(TCLAS != NULL) {
		if(FrameClassifierType == 0x00) {
			if(GetUserPriorityFromTCLASwithFrameClassifierType0(TCLAS, &UserPriority))
				printf("\e[0;32;40mGet\e[m UserPriority = %x\n", UserPriority);	
			if(GetClassifierMaskFromTCLASwithFrameClassifierType0(TCLAS, &ClassifierMask))
				printf("\e[0;32;40mGet\e[m ClassifierMask = %x\n", ClassifierMask);	
			if(GetSourceAddressFromTCLASwithFrameClassifierType0(TCLAS, (void *)SourceAddress))
				printf("\e[0;32;40mGet\e[m SourceAddress = %x:%x:%x:%x:%x:%x\n", SourceAddress[0], SourceAddress[1],SourceAddress[2],SourceAddress[3],SourceAddress[4],SourceAddress[5]);	
			if(GetDestinationAddressFromTCLASwithFrameClassifierType0(TCLAS, (void *)DestinationAddress))
				printf("\e[0;32;40mGet\e[m DestinationAddress = %x:%x:%x:%x:%x:%x\n", DestinationAddress[0], DestinationAddress[1],DestinationAddress[2],DestinationAddress[3],DestinationAddress[4],DestinationAddress[5]);	
			if(GetTypeFromTCLASwithFrameClassifierType0(TCLAS, &Type))
				printf("\e[0;32;40mGet\e[m Type = %d\n", Type);	
		}
		else if(FrameClassifierType == 0x01 && TCLASFrameClassifierType == ClassifierType_1_IPv4) {
			if(GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &UserPriority))
				printf("\e[0;32;40mGet\e[m UserPriority = %x\n", UserPriority);	
			if(GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &ClassifierMask))
				printf("\e[0;32;40mGet\e[m ClassifierMask = %x\n", ClassifierMask);	
			if(GetVersionFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &Version))
				printf("\e[0;32;40mGet\e[m Version = %x\n", Version);	
			if(GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, (void *)SourceIPAddressV4))
				printf("\e[0;32;40mGet\e[m SourceIPAddressV4 = %d.%d.%d.%d\n", (unsigned int)SourceIPAddressV4[0], (unsigned int)SourceIPAddressV4[1],(unsigned int)SourceIPAddressV4[2],(unsigned int)SourceIPAddressV4[3]);	
			if(GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, (void *)DestinationIPAddressV4))
				printf("\e[0;32;40mGet\e[m DestinationIPAddressV4 = %d.%d.%d.%d\n", (unsigned int)DestinationIPAddressV4[0], (unsigned int)DestinationIPAddressV4[1],(unsigned int)DestinationIPAddressV4[2],(unsigned int)DestinationIPAddressV4[3]);	
			if(GetSourcePortFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &SourcePort))
				printf("\e[0;32;40mGet\e[m SourcePort = %d\n", SourcePort);	
			if(GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &DestinationPort))
				printf("\e[0;32;40mGet\e[m DestinationPort = %d\n", DestinationPort);	
			if(GetDSCPFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &DSCP))
				printf("\e[0;32;40mGet\e[m DSCP = %x\n", DSCP);	
			if(GetProtocolFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &Protocol))
				printf("\e[0;32;40mGet\e[m Protocol = %x\n", Protocol);	
		}
		else if(FrameClassifierType == 0x01 && TCLASFrameClassifierType == ClassifierType_1_IPv6) {
			if(GetUserPriorityFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, &UserPriority))
				printf("\e[0;32;40mGet\e[m UserPriority = %x\n", UserPriority);	
			if(GetClassifierMaskFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, &ClassifierMask))
				printf("\e[0;32;40mGet\e[m ClassifierMask = %x\n", ClassifierMask);	
			if(GetVersionFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, &Version))
				printf("\e[0;32;40mGet\e[m Version = %x\n", Version);	
			if(GetSourceIPAddressFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, (void *)SourceIPAddressV6))
				printf("\e[0;32;40mGet\e[m SourceIPAddressV6 = %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", (unsigned int)SourceIPAddressV6[0], (unsigned int)SourceIPAddressV6[1],(unsigned int)SourceIPAddressV6[2],(unsigned int)SourceIPAddressV6[3], (unsigned int)SourceIPAddressV6[4], (unsigned int)SourceIPAddressV6[5],(unsigned int)SourceIPAddressV6[6],(unsigned int)SourceIPAddressV6[7], (unsigned int)SourceIPAddressV6[8], (unsigned int)SourceIPAddressV6[9],(unsigned int)SourceIPAddressV6[10],(unsigned int)SourceIPAddressV6[11], (unsigned int)SourceIPAddressV6[12], (unsigned int)SourceIPAddressV6[13],(unsigned int)SourceIPAddressV6[14],(unsigned int)SourceIPAddressV6[15]);	
			if(GetDestinationIPAddressFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, (void *)DestinationIPAddressV6))
				printf("\e[0;32;40mGet\e[m DestinationIPAddressV6 = %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n", (unsigned int)DestinationIPAddressV6[0], (unsigned int)DestinationIPAddressV6[1],(unsigned int)DestinationIPAddressV6[2],(unsigned int)DestinationIPAddressV6[3], (unsigned int)DestinationIPAddressV6[4], (unsigned int)DestinationIPAddressV6[5],(unsigned int)DestinationIPAddressV6[6],(unsigned int)DestinationIPAddressV6[7], (unsigned int)DestinationIPAddressV6[8], (unsigned int)DestinationIPAddressV6[9],(unsigned int)DestinationIPAddressV6[10],(unsigned int)DestinationIPAddressV6[11], (unsigned int)DestinationIPAddressV6[12], (unsigned int)DestinationIPAddressV6[13],(unsigned int)DestinationIPAddressV6[14],(unsigned int)DestinationIPAddressV6[15]);	
			if(GetSourcePortFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, &SourcePort))
				printf("\e[0;32;40mGet\e[m SourcePort = %d\n", SourcePort);	
			if(GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, &DestinationPort))
				printf("\e[0;32;40mGet\e[m DestinationPort = %d\n", DestinationPort);	
			if(GetFlowLabelFromTCLASwithFrameClassifierType1ForIPv6(TCLAS, (void *)FlowLabel))
				printf("\e[0;32;40mGet\e[m FlowLabel = %c%c%c\n", FlowLabel[0], FlowLabel[1], FlowLabel[2]);	
		}
		else if(FrameClassifierType == 0x02) {
			if(GetUserPriorityFromTCLASwithFrameClassifierType2(TCLAS, &UserPriority))
				printf("\e[0;32;40mGet\e[m UserPriority = %x\n", UserPriority);	
			if(GetClassifierMaskFromTCLASwithFrameClassifierType2(TCLAS, &ClassifierMask))
				printf("\e[0;32;40mGet\e[m ClassifierMask = %x\n", ClassifierMask);	
			if(Get80211QTagTypeFromTCLASwithFrameClassifierType2(TCLAS, &TagType))
				printf("\e[0;32;40mGet\e[m TagType = %d\n", TagType);	
		}
	}

	char *TCLASProcessing = NULL;
	enum TCLAS_FrameClassifierType ClassifierType = NoType;
	if(FrameClassifierType == 0x00) {
		ClassifierType = ClassifierType_0;
		TCLASProcessing = GetTCLASProcessingFrom80211eADDTSRequestFrameBody(framebody, ClassifierType);
	}
	else if(FrameClassifierType == 0x01 && TCLASFrameClassifierType == ClassifierType_1_IPv4) {
		ClassifierType = ClassifierType_1_IPv4;
		TCLASProcessing = GetTCLASProcessingFrom80211eADDTSRequestFrameBody(framebody, ClassifierType);
	}
	else if(FrameClassifierType == 0x01 && TCLASFrameClassifierType == ClassifierType_1_IPv6) {
		ClassifierType = ClassifierType_1_IPv6;
		TCLASProcessing = GetTCLASProcessingFrom80211eADDTSRequestFrameBody(framebody, ClassifierType);
	}
	else if(FrameClassifierType == 0x02) {
		ClassifierType = ClassifierType_2;
		TCLASProcessing = GetTCLASProcessingFrom80211eADDTSRequestFrameBody(framebody, ClassifierType);
	}
	else
		TCLASProcessing = GetTCLASProcessingFrom80211eADDTSRequestFrameBody(framebody, ClassifierType);

	if(TCLASProcessing != NULL) {
		if(GetProcessingFromTCLASProcessing(TCLASProcessing, &Processing))
			printf("\e[0;32;40mGet\e[m Processing = %x\n", Processing);	
	}


	free(framebody);
	return 0;
}
