/* gcc 2.x doesn't understand mac68k packing */

#define P_32(w) *(unsigned int *)&(w)
#define P_16(w) *(unsigned short *)&(w)
#define P_8(w)  *(unsigned char *)&(w)

struct P_PPCPortRec { /* 72 */
    char nameScript[2]; /* 0 */
    char name[33]; /* 2 */
    char _pad68k[1];
    char portKindSelector[2]; /* 36 */
    union {
        char portTypeStr[33]; /* 38 */
        union {
            char creator[4]; /* 38 */
            char type[4]; /* 42 */
        } port;
    } u;
    char _pad68k2[1];
};

struct P_LocationNameRec { /* 104 */
    char locationKindSelector[2]; /* 0 */
    union {
        char nbpEntity[102]; /* 2 */
        char nbpType[33]; /* 2 */
    } u;
};

struct P_PPCOpenPBRec { /* 56 */
    char qLink[4]; /* 0 */
    char csCode[2]; /* 4 */
    char intUse[2]; /* 6 */
    char intUsePtr[4]; /* 8 */
    char ioCompletion[4]; /* 12 */
    char ioResult[2]; /* 16 */
    char Reserved[20]; /* 18 */
    char portRefNum[2]; /* 38 */
    char filler1[4]; /* 40 */
    char serviceType[1]; /* 44 */
    char resFlag[1]; /* 45 */
    char portName[4]; /* 46 */
    char locationName[4]; /* 50 */
    char networkVisible[1]; /* 54 */
    char nbpRegistered[1]; /* 55 */
};

struct P_PPCInformPBRec { /* 64 */
    char qLink[4]; /* 0 */
    char csCode[2]; /* 4 */
    char intUse[2]; /* 6 */
    char intUsePtr[4]; /* 8 */
    char ioCompletion[4]; /* 12 */
    char ioResult[2]; /* 16 */
    char Reserved[20]; /* 18 */
    char portRefNum[2]; /* 38 */
    char sessRefNum[4]; /* 40 */
    char serviceType[1]; /* 44 */
    char autoAccept[1]; /* 45 */
    char portName[4]; /* 46 */
    char locationName[4]; /* 50 */
    char userName[4]; /* 54 */
    char userData[4]; /* 58 */
    char requestType[1]; /* 62 */
    char _pad68k[1];
};

struct P_PPCEndPBRec { /* 44 */
    char qLink[4]; /* 0 */
    char csCode[2]; /* 4 */
    char intUse[2]; /* 6 */
    char intUsePtr[4]; /* 8 */
    char ioCompletion[4]; /* 12 */
    char ioResult[2]; /* 16 */
    char Reserved[20]; /* 18 */
    char filler1[2]; /* 38 */
    char sessRefNum[4]; /* 40 */
};

struct P_PPCClosePBRec { /* 40 */
    char qLink[4]; /* 0 */
    char csCode[2]; /* 4 */
    char intUse[2]; /* 6 */
    char intUsePtr[4]; /* 8 */
    char ioCompletion[4]; /* 12 */
    char ioResult[2]; /* 16 */
    char Reserved[20]; /* 18 */
    char portRefNum[2]; /* 38 */
};

