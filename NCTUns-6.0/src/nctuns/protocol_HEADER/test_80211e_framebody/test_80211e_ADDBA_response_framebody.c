#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eADDBAResponseFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x03;
	u_char Action = 0x01;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	u_char DialogToken = 0x11;
	SetDialogTokenInto80211eADDBAResponseFrameBody(framebody, DialogToken);
	printf("\e[0;31;40mSet\e[m DialogToken = %x\n", DialogToken);

	unsigned short StatusCode = 22;
	SetStatusCodeInto80211eADDBAResponseFrameBody(framebody, &StatusCode);
	printf("\e[0;31;40mSet\e[m StatusCode = %d\n", StatusCode);

	bool BlockAckPolicy = true;
	u_char TID = 0x0f; // 4-bits
	unsigned short BufferSize = 1023; // 10-bits
	SetBlockAckParameterSetInto80211eADDBAResponseFrameBody(framebody, BlockAckPolicy, TID, &BufferSize);
	if(BlockAckPolicy)	
		printf("\e[0;31;40mSet\e[m BlockAckPolicy = 1\n");
	else			
		printf("\e[0;31;40mSet\e[m BlockAckPolicy = 0\n");
	printf("\e[0;31;40mSet\e[m TID = %x\n", TID);
	printf("\e[0;31;40mSet\e[m BufferSize = %d\n", BufferSize);

	unsigned short BlockAckTimeoutValue = 3456;
	SetBlockAckTimeoutValueInto80211eADDBAResponseFrameBody(framebody, &BlockAckTimeoutValue);
	printf("\e[0;31;40mSet\e[m BlockAckTimeoutValue = %d\n", BlockAckTimeoutValue);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	GetDialogTokenFrom80211eADDBAResponseFrameBody(framebody, &DialogToken);
	printf("\e[0;32;40mGet\e[m DialogToken = %x\n", DialogToken);

	GetStatusCodeFrom80211eADDBAResponseFrameBody(framebody, &StatusCode);
	printf("\e[0;32;40mGet\e[m StatusCode = %d\n", StatusCode);

	char *BlockAckParameterSet = GetBlockAckParameterSetFrom80211eADDBAResponseFrameBody(framebody);
	if(BlockAckParameterSet != NULL) {
		if(GetBlockAckPolicyFromBlockAckParameterSet(BlockAckParameterSet))	
			printf("\e[0;32;40mGet\e[m BlockAckPolicy = 1\n");
		else			
			printf("\e[0;32;40mGet\e[m BlockAckPolicy = 0\n");
		if(GetTIDFromBlockAckParameterSet(BlockAckParameterSet, &TID))
			printf("\e[0;32;40mGet\e[m TID = %x\n", TID);
		if(GetBufferSizeFromBlockAckParameterSet(BlockAckParameterSet, &BufferSize))
			printf("\e[0;32;40mGet\e[m BufferSize = %d\n", BufferSize);
	}

	GetBlockAckTimeoutValueFrom80211eADDBAResponseFrameBody(framebody, &BlockAckTimeoutValue);
	printf("\e[0;32;40mGet\e[m BlockAckTimeoutValue = %d\n", BlockAckTimeoutValue);


	free(framebody);
	return 0;
}
