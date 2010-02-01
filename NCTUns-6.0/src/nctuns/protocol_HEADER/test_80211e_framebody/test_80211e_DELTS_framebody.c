#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eDELTSFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x01;
	u_char Action = 0x02;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	bool TrafficType = true;
	u_char TSID = 0x0f; // 4-bits 
	bool Direction_Bit_1 = true;
	bool Direction_Bit_2 = false;
	bool AccessPolicy_Bit_1 = true;
	bool AccessPolicy_Bit_2 = false;
	bool Aggregation = true;
	bool APSD = false;
	u_char UserPriority = 0x04; // 0~7
	bool TSInfoAckPolicy_Bit_1 = true;
	bool TSInfoAckPolicy_Bit_2 = false;
	bool Schedule = true;
	SetTSInfoInto80211eDELTSFrameBody(framebody, TrafficType, TSID, Direction_Bit_1, Direction_Bit_2, AccessPolicy_Bit_1, AccessPolicy_Bit_2, Aggregation, APSD, UserPriority, TSInfoAckPolicy_Bit_1, TSInfoAckPolicy_Bit_2, Schedule);
	if(TrafficType)	
		printf("\e[0;31;40mSet\e[m TrafficType = 1\n");
	else			
		printf("\e[0;31;40mSet\e[m TrafficType = 0\n");
	printf("\e[0;31;40mSet\e[m TSID = %x\n", TSID);	
	if(Direction_Bit_1)	
		printf("\e[0;31;40mSet\e[m Direction_Bit_1 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m Direction_Bit_1 = 0\n");
	if(Direction_Bit_2)	
		printf("\e[0;31;40mSet\e[m Direction_Bit_2 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m Direction_Bit_2 = 0\n");
	if(AccessPolicy_Bit_1)	
		printf("\e[0;31;40mSet\e[m AccessPolicy_Bit_1 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m AccessPolicy_Bit_1 = 0\n");
	if(AccessPolicy_Bit_2)	
		printf("\e[0;31;40mSet\e[m AccessPolicy_Bit_2 = 1\n");
	else                            
		printf("\e[0;31;40mSet\e[m AccessPolicy_Bit_2 = 0\n");
	if(Aggregation)	
		printf("\e[0;31;40mSet\e[m Aggregation = 1\n");
	else                    
		printf("\e[0;31;40mSet\e[m Aggregation = 0\n");
	if(APSD)	
		printf("\e[0;31;40mSet\e[m APSD = 1\n");
	else            
		printf("\e[0;31;40mSet\e[m APSD = 0\n");
	printf("\e[0;31;40mSet\e[m UserPriority = %x\n", UserPriority);	
	if(TSInfoAckPolicy_Bit_1)
		printf("\e[0;31;40mSet\e[m TSInfoAckPolicy_Bit_1 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m TSInfoAckPolicy_Bit_1 = 0\n");
	if(TSInfoAckPolicy_Bit_2)
		printf("\e[0;31;40mSet\e[m TSInfoAckPolicy_Bit_2 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m TSInfoAckPolicy_Bit_2 = 0\n");
	if(Schedule)
		printf("\e[0;31;40mSet\e[m Schedule = 1\n");
	else
		printf("\e[0;31;40mSet\e[m Schedule = 0\n");

	unsigned short ReasonCode = 22;
	SetReasonCodeInto80211eDELTSFrameBody(framebody, &ReasonCode);
	printf("\e[0;31;40mSet\e[m ReasonCode = %d\n", ReasonCode);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	char *TSInfo = GetTSInfoFrom80211eDELTSFrameBody(framebody);
	if(TSInfo != NULL) {
		if(GetTrafficTypeFromTSInfo(TSInfo))	
			printf("\e[0;32;40mGet\e[m TrafficType = 1\n");
		else			
			printf("\e[0;32;40mGet\e[m TrafficType = 0\n");
		if(GetTSIDFromTSInfo(TSInfo, &TSID))
			printf("\e[0;32;40mGet\e[m TSID = %x\n", TSID);	
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
		if(GetUserPriorityFromTSInfo(TSInfo, &UserPriority))
			printf("\e[0;32;40mGet\e[m UserPriority = %x\n", UserPriority);	
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

	GetReasonCodeFrom80211eDELTSFrameBody(framebody, &ReasonCode);
	printf("\e[0;32;40mGet\e[m ReasonCode = %d\n", ReasonCode);

	free(framebody);
	return 0;
}
