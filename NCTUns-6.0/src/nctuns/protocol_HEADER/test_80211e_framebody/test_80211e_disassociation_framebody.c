#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	unsigned int		framebody_len;
	char			*framebody;
	
	unsigned short		ReasonCode;

	/* Build frame body */
	framebody_len = Calculate80211eDisassociationFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);


	ReasonCode = 8;
	SetReasonCodeInto80211eDisassociationFrameBody(framebody, &ReasonCode);
	printf("\e[0;31;40mSet\e[m ReasonCode = %d\n", ReasonCode);

	/*==Get=========================*/
	GetReasonCodeFrom80211eDisassociationFrameBody(framebody, &ReasonCode);
	printf("\e[0;32;40mGet\e[m ReasonCode = %d\n", ReasonCode);

	free(framebody);	
	return 0;
}

