#include "../mac-802_11_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	unsigned int		framebody_len;
	char			*framebody;
	
	unsigned short		ReasonCode;

	/* Build frame body */
	framebody_len = Calculate80211DisassociationFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	InitializeFrameBodyWithZero(framebody, framebody_len);


	ReasonCode = 8;
	SetReasonCodeInto80211DisassociationFrameBody(framebody, &ReasonCode);
	printf("\e[0;31;40mSet\e[m ReasonCode = %d\n", ReasonCode);

	/*==Get=========================*/
	GetReasonCodeFrom80211DisassociationFrameBody(framebody, &ReasonCode);
	printf("\e[0;32;40mGet\e[m ReasonCode = %d\n", ReasonCode);

	free(framebody);	
	return 0;
}

