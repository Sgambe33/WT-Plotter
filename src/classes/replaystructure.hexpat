bitfield difficulty
{
    unsigned  unk_nib : 4;
    unsigned  difficulty : 4;
};

struct ReplayHeader {
    u32 magic;
    u32 version;
    char level[128];
    char levelSettings[260];
    char battleType[128];
    char environment[128];
    char visibility[32];
    u32 rezOffset;
    difficulty diff;
    padding[35];
    u32 sessionType;
    padding[4];
    u64 sessionIdHex;
    padding[4];
    u32 mSetSize;
    padding[32];
    char locName[128];
    u32 startTime;
    u32 timeLimit;
    u32 scoreLimit;
    padding[48];
    char battleClass[128];
    char battleKillStreak[128];
};

ReplayHeader repHeader @ 0x0 [[name("Header")]];