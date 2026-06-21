#pragma once
#include <windows.h>
#include <inttypes.h>
#include <cinttypes>

#define FREETRACK_HEAP "FT_SharedMem"
#define FREETRACK_MUTEX "FT_Mutext"

/* only 6 headpose floats and the data id are filled -sh */
typedef struct FTData__ {
    uint32_t DataID;
    int32_t CamWidth;
    int32_t CamHeight;
    /* virtual pose */
    float  Yaw;   /* positive yaw to the left */
    float  Pitch; /* positive pitch up */
    float  Roll;  /* positive roll to the left */
    float  X;
    float  Y;
    float  Z;
    /* raw pose with no smoothing, sensitivity, response curve etc. */
    float  RawYaw;
    float  RawPitch;
    float  RawRoll;
    float  RawX;
    float  RawY;
    float  RawZ;
    /* raw points, sorted by Y, origin top left corner */
    float  X1;
    float  Y1;
    float  X2;
    float  Y2;
    float  X3;
    float  Y3;
    float  X4;
    float  Y4;
} volatile FTData;

typedef struct FTHeap__ {
    FTData data;
    int32_t GameID;
    union
    {
        unsigned char table[8];
        int32_t table_ints[2];
    };
    int32_t GameID2;
} volatile FTHeap;

class __declspec(dllexport) shm_wrapper final
{
    void* mem;
    HANDLE mutex, mapped_file;

public:
    shm_wrapper(const char* shm_name, const char* mutex_name, int map_size);
    ~shm_wrapper();
    bool lock();
    bool unlock();
    bool success();
    inline void* ptr() { return mem; }

    shm_wrapper(const shm_wrapper&) = delete;
    shm_wrapper(shm_wrapper&&) = delete;
    shm_wrapper& operator=(const shm_wrapper&) = delete;
    shm_wrapper& operator=(shm_wrapper&&) = delete;
};

class Tracking
{
private:
    shm_wrapper shm{ FREETRACK_HEAP, FREETRACK_MUTEX, sizeof(FTHeap) };
    FTHeap* pMemData{ (FTHeap*)shm.ptr() };
    HANDLE hProcess = 0;
    HANDLE hThread = 0;
    int intGameID = -1;

    void StartDummy();
    bool GetGameData(int gameId, char* outData);

public:
	Tracking();
	~Tracking();
	bool Initialize();
    bool SetProtocols();
    void ClearProtocols();
    void Pose(const double* headpose, const double* raw);
};