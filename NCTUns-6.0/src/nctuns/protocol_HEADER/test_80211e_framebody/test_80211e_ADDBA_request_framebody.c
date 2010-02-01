#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eADDBARequestFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x03;
	u_char Action = 0x00;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	u_char DialogToken = 0x11;
	SetDialogTokenInto80211eADDBARequestFrameBody(framebody, DialogToken);
	printf("\e[0;31;40mSet\e[m DialogToken = %x\n", DialogToken);

	bool BlockAckPolicy = true;
	u_char TID = 0x0f; // 4-bits
	unsigned short BufferSize = 1023; // 10-bits
	SetBlockAckParameterSetInto80211eADDBARequestFrameBody(framebody, BlockAckPolicy, TID, &BufferSize);
	if(BlockAckPolicy)	
		printf("\e[0;31;40mSet\e[m BlockAckPolicy = 1\n");
	else			
		printf("\e[0;31;40mSet\e[m BlockAckPolicy = 0\n");
	printf("\e[0;31;40mSet\e[m TID = %x\n", TID);
	printf("\e[0;31;40mSet\e[m BufferSize = %d\n", BufferSize);

	unsigned short BlockAckTimeoutValue = 3456;
	SetBlockAckTimeoutValueInto80211eADDBARequestFrameBody(framebody, &BlockAckTimeoutValue);
	printf("\e[0;31;40mSet\e[m BlockAckTimeoutValue = %d\n", BlockAckTimeoutValue);

	unsigned short FragmentNumber = 15; // 4-bits
	unsigned short StartingSequenceNumber = 2048; // 12-bits
	SetBlockAckStartingSequenceControlInto80211eADDBARequestFrameBody(framebody, &FragmentNumber, &StartingSequenceNumber);
	printf("\e[0;31;40mSet\e[m FragmentNumber = %d\n", FragmentNumber);
	printf("\e[0;31;40mSet\e[m StartingSequenceNumber = %d\n", StartingSequenceNumber);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	GetDialogTokenFrom80211eADDBARequestFrameBody(framebody, &DialogToken);
	printf("\e[0;32;40mGet\e[m DialogToken = %x\n", DialogToken);

	char *BlockAckParameterSet = GetBlockAckParameterSetFrom80211eADDBARequestFrameBody(framebody);
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

	GetBlockAckTimeoutValueFrom80211eADDBARequestFrameBody(framebody, &BlockAckTimeoutValue);
	printf("\e[0;32;40mGet\e[m BlockAckTimeoutValue = %d\n", BlockAckTimeoutValue);

	char *BlockAckStartingSequenceControl = GetBlockAckStartingSequenceControlFrom80211eADDBARequestFrameBody(framebody);
	if(BlockAckStartingSequenceControl != NULL) {
		if(GetFragmentNumberFromBlockAckStartingSequenceControl(BlockAckStartingSequenceControl, &FragmentNumber))
			printf("\e[0;32;40mGet\e[m FragmentNumber = %d\n", FragmentNumber);
		if(GetStartingSequenceNumberFromBlockAckStartingSequenceControl(BlockAckStartingSequenceControl, &StartingSequenceNumber))
			printf("\e[0;32;40mGet\e[m StartingSequenceNumber = %d\n", StartingSequenceNumber);
	}

	free(framebody);
	return 0;
}
