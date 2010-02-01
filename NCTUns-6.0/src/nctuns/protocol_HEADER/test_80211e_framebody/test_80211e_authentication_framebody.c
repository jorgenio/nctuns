#include "../mac-802_11e_management_framebody.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	u_char			optional_info_flag;
	bool			WithChallengeText;
	unsigned int		ChallengeText_Len;

	unsigned int		framebody_len;
	char			*framebody;

	unsigned short		AuthAlgoNum;

	unsigned short		AuthTransSeqNum;	

	unsigned short		StatusCode;

	char			*ChallengeText;

	char			*tmp_char_ptr;
	void			*tmp_void_ptr;

	/* Build frame body */
	WithChallengeText = true;
	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eAuthenticationFrameBody(WithChallengeText);
	ChallengeText_Len = 26;
	framebody_len = Calculate80211eAuthenticationFrameBodyLength(optional_info_flag, ChallengeText_Len);

	framebody = (char *)malloc(framebody_len);
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	AuthAlgoNum = 1;
	SetAuthAlgoNumInto80211eAuthenticationFrameBody(framebody, &AuthAlgoNum);
	printf("\e[0;31;40mSet\e[m AuthAlgoNum = %d\n", AuthAlgoNum);

	AuthTransSeqNum = 12345;
	SetAuthTransSeqNumInto80211eAuthenticationFrameBody(framebody, &AuthTransSeqNum);
	printf("\e[0;31;40mSet\e[m AuthTransSeqNum = %d\n", AuthTransSeqNum);

	StatusCode = 9;
	SetStatusCodeInto80211eAuthenticationFrameBody(framebody, &StatusCode);
	printf("\e[0;31;40mSet\e[m StatusCode = %d\n", StatusCode);

	ChallengeText = (char *)malloc(ChallengeText_Len);
	strncpy(ChallengeText, "abcdefghijklmnopqrstuvwxyz", ChallengeText_Len);
	SetChallengeTextInto80211eAuthenticationFrameBody(framebody, optional_info_flag, ChallengeText_Len, (void *)ChallengeText);
	printf("\e[0;31;40mSet\e[m ChallengeText_Len = %d\n", ChallengeText_Len);
	if(ChallengeText != NULL && ChallengeText_Len != 0) 
		printf("\e[0;31;40mSet\e[m ChallengeText -> %s\n", ChallengeText);
	free((void *)ChallengeText);

	/*==Get=========================*/
	GetAuthAlgoNumFrom80211eAuthenticationFrameBody(framebody, &AuthAlgoNum);
	printf("\e[0;32;40mGet\e[m AuthAlgoNum = %d\n", AuthAlgoNum);

	GetAuthTransSeqNumFrom80211eAuthenticationFrameBody(framebody, &AuthTransSeqNum);
	printf("\e[0;32;40mGet\e[m AuthTransSeqNum = %d\n", AuthTransSeqNum);

	GetStatusCodeFrom80211eAuthenticationFrameBody(framebody, &StatusCode);
	printf("\e[0;32;40mGet\e[m StatusCode = %d\n", StatusCode);

	tmp_char_ptr = GetChallengeTextFrom80211eAuthenticationFrameBody(framebody);
	if(tmp_char_ptr != NULL) {
		tmp_void_ptr = GetChallengeTextFrom80211eChallengeText(tmp_char_ptr, &ChallengeText_Len);
		if(tmp_void_ptr != NULL && ChallengeText_Len != 0) {
			ChallengeText = (char *)malloc(ChallengeText_Len);
			strncpy(ChallengeText, (char *)tmp_void_ptr, ChallengeText_Len);
			ChallengeText[ChallengeText_Len] = '\0';
			printf("\e[0;32;40mGet\e[m ChallengeText_Len = %d\n", ChallengeText_Len);
			printf("\e[0;32;40mGet\e[m ChallengeText -> %s\n", ChallengeText);
			free((void *)ChallengeText);
		}
	}
	free(framebody);	
	return 0;
}

