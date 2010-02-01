#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	unsigned int			framebody_len;
	char				*framebody;

	/* Build frame body */
	framebody_len = Calculate80211eDLSTeardownFrameBodyLength();

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	u_char Category = 0x02;
	u_char Action = 0x02;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);
	printf("\e[0;31;40mSet\e[m Category = %x\n", Category);
	printf("\e[0;31;40mSet\e[m Action = %x\n", Action);

	u_char DestinationMACAddress[6] = {0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
	SetDestinationMACAddressInto80211eDLSTeardownFrameBody(framebody, (void *)DestinationMACAddress);
	printf("\e[0;31;40mSet\e[m DestinationMACAddress = %x:%x:%x:%x:%x:%x\n", DestinationMACAddress[0], DestinationMACAddress[1],DestinationMACAddress[2],DestinationMACAddress[3],DestinationMACAddress[4],DestinationMACAddress[5]);	

	u_char SourceMACAddress[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	SetSourceMACAddressInto80211eDLSTeardownFrameBody(framebody, (void *)SourceMACAddress);
	printf("\e[0;31;40mSet\e[m SourceMACAddress = %x:%x:%x:%x:%x:%x\n", SourceMACAddress[0], SourceMACAddress[1],SourceMACAddress[2],SourceMACAddress[3],SourceMACAddress[4],SourceMACAddress[5]);	

	unsigned short ReasonCode = 22;
	SetReasonCodeInto80211eDLSTeardownFrameBody(framebody, &ReasonCode);
	printf("\e[0;31;40mSet\e[m ReasonCode = %d\n", ReasonCode);

	/*==Get=========================*/
	GetCategoryFrom80211eActionFrameBody(framebody, &Category);
	GetActionFrom80211eActionFrameBody(framebody, &Action);
	printf("\e[0;32;40mGet\e[m Category = %x\n", Category);
	printf("\e[0;32;40mGet\e[m Action = %x\n", Action);

	GetDestinationMACAddressFrom80211eDLSTeardownFrameBody(framebody, (void *)DestinationMACAddress);
	printf("\e[0;32;40mGet\e[m DestinationMACAddress = %x:%x:%x:%x:%x:%x\n", DestinationMACAddress[0], DestinationMACAddress[1],DestinationMACAddress[2],DestinationMACAddress[3],DestinationMACAddress[4],DestinationMACAddress[5]);	

	GetSourceMACAddressFrom80211eDLSTeardownFrameBody(framebody, (void *)SourceMACAddress);
	printf("\e[0;32;40mGet\e[m SourceMACAddress = %x:%x:%x:%x:%x:%x\n", SourceMACAddress[0], SourceMACAddress[1],SourceMACAddress[2],SourceMACAddress[3],SourceMACAddress[4],SourceMACAddress[5]);	

	GetReasonCodeFrom80211eDLSTeardownFrameBody(framebody, &ReasonCode);
	printf("\e[0;32;40mGet\e[m ReasonCode = %d\n", ReasonCode);

	free(framebody);
	return 0;
}
