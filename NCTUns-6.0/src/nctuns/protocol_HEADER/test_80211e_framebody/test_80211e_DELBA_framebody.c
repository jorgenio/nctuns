#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eDELBAFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x03;
	u_char Action = 0x02;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	bool Initiator = true;
	u_char TID = 15; // 4-bits
	SetDELBAParameterSetInto80211eDELBAFrameBody(framebody, Initiator, TID);
	if(Initiator)	
		printf("\e[0;31;40mSet\e[m Initiator = 1\n");
	else			
		printf("\e[0;31;40mSet\e[m Initiator = 0\n");
	printf("\e[0;31;40mSet\e[m TID = %x\n", TID);

	unsigned short ReasonCode = 22;
	SetReasonCodeInto80211eDELBAFrameBody(framebody, &ReasonCode);
	printf("\e[0;31;40mSet\e[m ReasonCode = %d\n", ReasonCode);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	char *DELBAParameterSet = GetDELBAParameterSetFrom80211eDELBAFrameBody(framebody);
	if(DELBAParameterSet != NULL) {
		if(GetInitiatorFromDELBAParameterSet(DELBAParameterSet))
			printf("\e[0;32;40mGet\e[m Initiator = 1\n");
		else			
			printf("\e[0;32;40mGet\e[m Initiator = 0\n");
		if(GetTIDFromDELBAParameterSet(DELBAParameterSet, &TID))
			printf("\e[0;32;40mGet\e[m TID = %x\n", TID);
	}

	GetReasonCodeFrom80211eDELBAFrameBody(framebody, &ReasonCode);
	printf("\e[0;32;40mGet\e[m ReasonCode = %d\n", ReasonCode);


	free(framebody);
	return 0;
}
