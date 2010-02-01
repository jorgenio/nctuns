#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eScheduleFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x01;
	u_char Action = 0x03;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	bool Aggregation = true;
	u_char TSID = 0x0e; // 4-bits
	bool Direction_Bit_1 = false;
	bool Direction_Bit_2 = true;
	unsigned int ServiceStartTime = 1234;
	unsigned int ServiceInterval = 4321;
	unsigned short SpecificationInterval = 666;
	SetScheduleInto80211eScheduleFrameBody(framebody, Aggregation, TSID, Direction_Bit_1, Direction_Bit_2, &ServiceStartTime, &ServiceInterval, &SpecificationInterval);
	if(Aggregation)
		printf("\e[0;31;40mSet\e[m Aggregation = 1\n");
	else
		printf("\e[0;31;40mSet\e[m Aggregation = 0\n");
	printf("\e[0;31;40mSet\e[m TSID = %x\n", TSID);	
	if(Direction_Bit_1)
		printf("\e[0;31;40mSet\e[m Direction_Bit_1 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m Direction_Bit_1 = 0\n");
	if(Direction_Bit_2)
		printf("\e[0;31;40mSet\e[m Direction_Bit_2 = 1\n");
	else
		printf("\e[0;31;40mSet\e[m Direction_Bit_2 = 0\n");
	printf("\e[0;31;40mSet\e[m ServiceStartTime = %d\n", ServiceStartTime);	
	printf("\e[0;31;40mSet\e[m ServiceInterval = %d\n", ServiceInterval);	
	printf("\e[0;31;40mSet\e[m SpecificationInterval = %d\n", SpecificationInterval);	
	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	char *Schedule = GetScheduleFrom80211eScheduleFrameBody(framebody);
	if(Schedule != NULL) {
		if(GetAggregationFromSchedule(Schedule))
			printf("\e[0;32;40mGet\e[m Aggregation = 1\n");
		else
			printf("\e[0;32;40mGet\e[m Aggregation = 0\n");
		if(GetTSIDFromSchedule(Schedule, &TSID))
			printf("\e[0;32;40mGet\e[m TSID = %x\n", TSID);	
		if(GetDirectionBit1FromSchedule(Schedule))
			printf("\e[0;32;40mGet\e[m Direction_Bit_1 = 1\n");
		else
			printf("\e[0;32;40mGet\e[m Direction_Bit_1 = 0\n");
		if(GetDirectionBit2FromSchedule(Schedule))
			printf("\e[0;32;40mGet\e[m Direction_Bit_2 = 1\n");
		else
			printf("\e[0;32;40mGet\e[m Direction_Bit_2 = 0\n");
		if(GetServiceStartTimeFromSchedule(Schedule, &ServiceStartTime))
			printf("\e[0;32;40mGet\e[m ServiceStartTime = %d\n", ServiceStartTime);	
		if(GetServiceIntervalFromSchedule(Schedule, &ServiceInterval))
			printf("\e[0;32;40mGet\e[m ServiceInterval = %d\n", ServiceInterval);	
		if(GetSpecificationIntervalFromSchedule(Schedule, &SpecificationInterval))
			printf("\e[0;32;40mGet\e[m SpecificationInterval = %d\n", SpecificationInterval);	
	}

	free(framebody);
	return 0;
}
