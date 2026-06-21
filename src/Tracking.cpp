#include "Tracking.h"

#include "stdafx.h"
#include <shlwapi.h>
#include <cstdio>
#include "Math.h"

#include "Common.h"

static_assert(sizeof(LONG) == sizeof(std::int32_t), "Assert failed");
static_assert(sizeof(LONG) == 4u, "Assert failed");

__declspec(noinline) void store(float volatile& place, const float value)
{
    union
    {
        float f32;
        LONG i32;
    } value_{};

    value_.f32 = value;

    static_assert(sizeof(value_) == sizeof(float), "Assert failed");
    static_assert(offsetof(decltype(value_), f32) == offsetof(decltype(value_), i32), "Assert failed");

    (void)InterlockedExchange((LONG volatile*)&place, value_.i32);
}

template<typename t>
static void store(t volatile& place, t value)
{
    static_assert(sizeof(t) == 4u, "Assert failed");
    (void)InterlockedExchange((LONG volatile*)&place, (LONG)value);
}

static std::int32_t load(std::int32_t volatile& place)
{
    return InterlockedCompareExchange((volatile LONG*)&place, 0, 0);
}

bool CreateRegKey(LPCSTR lpSubKey)
{
    HKEY hKey;
    LSTATUS ret;
    ret = RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (ret != ERROR_SUCCESS)
        return false;

    char selfdir[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, selfdir, MAX_PATH);
    PathRemoveFileSpecA(selfdir);

    BYTE* p = (BYTE*)selfdir;
    ret = RegSetValueExA(hKey, "Path", 0, REG_SZ, p, strlen(selfdir) + 1);
    if (ret != ERROR_SUCCESS)
        return false;

    RegCloseKey(hKey);

    return true;
}

void Tracking::StartDummy()
{
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;

    char selfdir[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, selfdir, MAX_PATH);
    PathRemoveFileSpecA(selfdir);
    strcat(selfdir, "\\TrackIR.exe");

    HRESULT hr = CreateProcess(NULL, selfdir, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
    if (hr != 0)
    {
        hProcess = ProcessInfo.hProcess;
        hThread = ProcessInfo.hThread;
    }
    else
    {
        _tprintf("Can't start dummy process!\n");
    }
}

bool Tracking::GetGameData(int gameId, char* outData)
{
    switch (gameId)
    {
    case 1003: strcpy(outData, "02A9B6DCD15F5A572F6500"); break; // DCS
    case 1008: strcpy(outData, "02CA4D5368D1FD2DF2AE00"); break; // IL-2
    default:
        return false;
    }

    return true;
}

Tracking::Tracking()
{
}

Tracking::~Tracking()
{
    TerminateProcess(hProcess, 0);

    if (hProcess)
        CloseHandle(hProcess);

    if (hThread)
        CloseHandle(hThread);
}

bool Tracking::Initialize()
{
    if (!shm.success())
    {
        //return error(tr("Can't load freetrack memory mapping"));
        _tprintf("Can't load freetrack memory mapping\n");
        return false;
    }

    if (!SetProtocols())
    {
        return false;
    }

    pMemData->data.DataID = 1;
    pMemData->data.CamWidth = 100;
    pMemData->data.CamHeight = 250;

    store(pMemData->GameID2, 0);

    for (unsigned k = 0; k < 2; k++)
        store(pMemData->table_ints[k], 0);

    StartDummy();

    return true;
}

bool Tracking::SetProtocols()
{
    if (!CreateRegKey("SOFTWARE\\Freetrack\\FreetrackClient"))
        return false;

    if (!CreateRegKey("SOFTWARE\\NaturalPoint\\NATURALPOINT\\NPClient Location"))
        return false;

    return true;
}

void Tracking::ClearProtocols()
{
}

auto do_scanf(const char* s, unsigned(&tmp)[8])
{
    unsigned fuzz[3];
    return std::sscanf(s,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        fuzz + 2,
        fuzz + 0,
        tmp + 3, tmp + 2, tmp + 1, tmp + 0,
        tmp + 7, tmp + 6, tmp + 5, tmp + 4,
        fuzz + 1);
};

void Tracking::Pose(const double* headpose, const double* raw)
{
    constexpr double d2r = M_PI / 180;

    const float yaw = float(-headpose[AXIS_RX] * d2r);
    const float roll = float(headpose[AXIS_RZ] * d2r);
    const float tx = float(headpose[AXIS_X] * 10);
    const float ty = float(headpose[AXIS_Y] * 10);
    const float tz = float(headpose[AXIS_Z] * 10);

    // HACK: Falcon BMS makes a "bump" if pitch is over the value -sh 20170615
    const bool is_crossing_90 = fabs(headpose[AXIS_RY] - 90) < .15;
    const float pitch = float(-d2r * (is_crossing_90 ? 89.86 : headpose[AXIS_RY]));

    FTHeap* const ft = pMemData;
    FTData* const data = &ft->data;

    store(data->X, tx);
    store(data->Y, ty);
    store(data->Z, tz);

    store(data->Yaw, yaw);
    store(data->Pitch, pitch);
    store(data->Roll, roll);

    store(data->RawYaw, float(-raw[AXIS_RX] * d2r));
    store(data->RawPitch, float(raw[AXIS_RY] * d2r));
    store(data->RawRoll, float(raw[AXIS_RZ] * d2r));
    store(data->RawX, float(raw[AXIS_X] * 10));
    store(data->RawY, float(raw[AXIS_Y] * 10));
    store(data->RawZ, float(raw[AXIS_Z] * 10));

    const std::int32_t id = load(ft->GameID);

    if (intGameID != id)
    {
        union {
            unsigned char table[8];
            std::int32_t ints[2];
        } t{};
        t.ints[0] = 0; t.ints[1] = 0;

        char gameData[30];
        if (GetGameData(id, gameData))
        {
            unsigned tmp[8]{};
            int res = do_scanf(gameData, tmp);
            if (res == 11)
            {
                using uchar = unsigned char;
                for (int i = 0; i < 8; i++)
                    t.table[i] = uchar(tmp[i]);
            }
        }

        {
            // FTHeap pMemData happens to be aligned on a page boundary by virtue of
            // memory mapping usage (MS Windows equivalent of mmap(2)).
            static_assert((offsetof(FTHeap, table) & (sizeof(LONG) - 1)) == 0, "Assert failed");

            for (unsigned k = 0; k < 2; k++)
                store(pMemData->table_ints[k], t.ints[k]);
        }

        store(ft->GameID2, id);
        store(data->DataID, 0u);

        intGameID = id;
    }
    else
        (void)InterlockedAdd((LONG volatile*)&data->DataID, 1);
}

shm_wrapper::shm_wrapper(const char* shm_name, const char* mutex_name, int map_size)
{
    if (mutex_name == nullptr)
        mutex = nullptr;
    else
    {
        mutex = CreateMutexA(nullptr, false, mutex_name);

        if (!mutex)
        {
            //warn("CreateMutexA", (int)GetLastError());
            _tprintf("CreateMutexA %d\n", (int)GetLastError());
            return;
        }
    }

    mapped_file = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        map_size,
        shm_name);

    if (!mapped_file)
    {
        //warn("CreateFileMappingA", (int)GetLastError());
        _tprintf("CreateFileMappingA %d\n", (int)GetLastError());
        return;
    }

    mem = MapViewOfFile(mapped_file,
        FILE_MAP_WRITE,
        0,
        0,
        map_size);

    if (!mem)
        //warn("MapViewOfFile:", (int)GetLastError());
        _tprintf("MapViewOfFile: %d\n", (int)GetLastError());
}

shm_wrapper::~shm_wrapper()
{
    if (mem && !UnmapViewOfFile(mem))
        goto fail;

    if (mapped_file && !CloseHandle(mapped_file))
        goto fail;

    if (mutex && !CloseHandle(mutex))
        goto fail;

    return;

fail:
    //warn("failed to close mapping", (int)GetLastError());
    _tprintf("failed to close mapping %d\n", (int)GetLastError());
}

bool shm_wrapper::lock()
{
    if (mutex)
        return WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0;
    else
        return false;
}

bool shm_wrapper::unlock()
{
    if (mutex)
        return ReleaseMutex(mutex);
    else
        return false;
}

bool shm_wrapper::success()
{
    return mem != nullptr;
}
