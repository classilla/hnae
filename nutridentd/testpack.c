#include <stdio.h>
#include <assert.h>
#include "ppcpacked.h"

//typedef unsigned int size_t;
#define offsetof(__s_name, __s_member) (size_t)&(((__s_name *)0)->__s_member)

int main(void) {
	struct P_PPCPortRec _PPCPortRec;
	struct P_LocationNameRec _LocationNameRec;
	struct P_PPCOpenPBRec _PPCOpenPBRec;
	struct P_PPCInformPBRec _PPCInformPBRec;
	struct P_PPCEndPBRec _PPCEndPBRec;
	struct P_PPCClosePBRec _PPCClosePBRec;

	assert(sizeof(_PPCPortRec)==72);
	assert(offsetof(struct P_PPCPortRec, u)==38);

	assert(sizeof(_LocationNameRec)==104);
	assert(offsetof(struct P_LocationNameRec, u)==2);

	assert(sizeof(_PPCOpenPBRec)==56);
	assert(offsetof(struct P_PPCOpenPBRec, nbpRegistered)==55);

	assert(sizeof(_PPCInformPBRec)==64);
	assert(offsetof(struct P_PPCInformPBRec, requestType)==62);

	assert(sizeof(_PPCEndPBRec)==44);
	assert(offsetof(struct P_PPCEndPBRec, sessRefNum)==40);

	assert(sizeof(_PPCClosePBRec)==40);
	assert(offsetof(struct P_PPCClosePBRec, portRefNum)==38);
	return 0;
}
