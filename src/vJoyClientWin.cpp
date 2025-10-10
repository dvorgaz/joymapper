#include "stdafx.h"

#include <Windows.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <hidsdi.h>
#include <stdio.h>

#include "public.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include "vjoyinterface.h"

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <XInput.h>
#pragma comment(lib,"xinput.lib")
#else
#include <XInput.h>
#pragma comment(lib,"xinput9_1_0.lib")
#endif

#include "RenderDevice.h"
#include "JoyMapperVariants.h"
#include "JoyMapperRenderer.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define WC_MAINFRAME	TEXT("MainFrame")
#define MAX_BUTTONS		128
#define CHECK(exp)		{ if(!(exp)) goto Error; }
#define SAFE_FREE(p)	{ if(p) { HeapFree(hHeap, 0, p); (p) = NULL; } }

// XInput
#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers

struct CONTROLLER_STATE
{
	XINPUT_STATE state;
	bool bConnected;
};

CONTROLLER_STATE g_Controllers[MAX_CONTROLLERS];

bool InitJoyDevice(UINT id);
bool UpdateJoyDevice(UINT id, JOYSTICK_POSITION_V2& iReport);
void HandleController(const STime& time);
HRESULT UpdateControllerState();
void SetVibration(DWORD controllerId, WORD left, WORD right);
void MsgBox(HWND hWnd, const char* str, ...);

// Default device ID (Used when ID not specified)
#define DEV_ID_1	1
#define DEV_ID_2	2

#define XBOXCONTROLLER	L"Controller (Xbox One For Windows)"
#define VKBSTICK		L" VKB-Sim Space Gunfighter "
#define VKBKG12			L" VKB-Sim Gunfighter Vintage "
#define TWCSTHROTTLE	L"TWCS Throttle"

JOYSTICK_POSITION_V2 iReport; // The structure that holds the full position data
JOYSTICK_POSITION_V2 iReportEx;

#define MAPPER_TYPE JoystickMapper //AlternateMapper_2// DefaultMapper
//
// Global variables
//
JoyMapper*			g_Mapper = nullptr;
RenderDevice		g_RenderDevice;
JoyMapperRenderer	g_MapperRenderer;

HWND		g_hWnd = nullptr;
HINSTANCE	g_hInst = nullptr;
RECT		g_DesktopRect;
UINT		DevID_1 = DEV_ID_1;
UINT		DevID_2 = DEV_ID_2;
PVOID		pPositionMessage;
BYTE		id;

COLORREF g_TransparentColor = RGB(0, 255, 0);

LARGE_INTEGER StartTime, LastTime, CurrentTime, ElapsedMicroseconds;
LARGE_INTEGER Frequency;

WORD btnCodeConversion[] = {
	JOYPAD_A,
	JOYPAD_B,
	JOYPAD_X,
	JOYPAD_Y,
	JOYPAD_LEFT_SHOULDER,
	JOYPAD_RIGHT_SHOULDER,
	JOYPAD_BACK,
	JOYPAD_START,
	JOYPAD_LEFT_THUMB, 
	JOYPAD_RIGHT_THUMB,
	JOYPAD_GUIDE,
	JOYPAD_SHARE };

struct DeviceCalibrationData
{
	wchar_t productString[126];

	int numHats;

	// Range of axis values
	LONG rangeAxisX;
	LONG rangeAxisY;
	LONG rangeAxisZ;
	LONG rangeAxisRx;
	LONG rangeAxisRy;
	LONG rangeAxisRz;
	LONG rangeSlider;
	LONG rangeDial;

	bool centerAxisX;
	bool centerAxisY;
	bool centerAxisZ;
	bool centerAxisRx;
	bool centerAxisRy;
	bool centerAxisRz;
	bool centerSlider;
	bool centerDial;
};

struct DeviceData
{
	HANDLE hDevice;
	wchar_t productString[126];

	BOOL bButtonStates[MAX_BUTTONS];
	LONG lAxisX;
	LONG lAxisY;
	LONG lAxisZ;
	LONG lAxisRx;
	LONG lAxisRy;
	LONG lAxisRz;
	LONG lSlider;
	LONG lDial;
	LONG lHat;
	INT  NumberOfButtons;

	DeviceCalibrationData* calibration = NULL;


};

#define MAX_DEVICES 5

DeviceData g_Devices[MAX_DEVICES] = { 0 };
unsigned int g_NumDevices = 0;

std::vector<DeviceCalibrationData> g_Calibrations;

void SetupCalibrations();
void SetMapperData(JoyMapper* mapper, DeviceData* deviceData);

DeviceData* GetDevice(const wchar_t* deviceName)
{
	for (int i = 0; i < MAX_DEVICES && i < g_NumDevices; ++i)
	{
		if (wcscmp(g_Devices[i].productString, deviceName) == 0)
			return &g_Devices[i];
	}

	return NULL;
}

void ParseRawInput(PRAWINPUT pRawInput)
{
	PHIDP_PREPARSED_DATA pPreparsedData;
	HIDP_CAPS            Caps;
	PHIDP_BUTTON_CAPS    pButtonCaps;
	PHIDP_VALUE_CAPS     pValueCaps;
	USHORT               capsLength;
	UINT                 bufferSize;
	HANDLE               hHeap;
	USAGE                usage[MAX_BUTTONS];
	ULONG                i, usageLength, value;

	char deviceName[256] = { 0 };
	wchar_t productString[126] = { 0 };

	pPreparsedData = NULL;
	pButtonCaps    = NULL;
	pValueCaps     = NULL;
	hHeap          = GetProcessHeap();


	//
	// Identify device
	//

	DeviceData* deviceData = NULL;

	for (int i = 0; i < MAX_DEVICES && i < g_NumDevices; ++i)
	{
		if (g_Devices[i].hDevice == pRawInput->header.hDevice)
		{
			deviceData = &g_Devices[i];
			break;
		}
	}

	if (deviceData == NULL)
	{
		if (g_NumDevices < MAX_DEVICES)
		{
			DeviceData& dd = g_Devices[g_NumDevices++];
			dd.hDevice = pRawInput->header.hDevice;

			CHECK(GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize) == 0);
			CHECK((int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_DEVICENAME, deviceName, &bufferSize) >= 0);

			DWORD err;
			HANDLE file_handle = CreateFile(deviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			err = GetLastError();

			UINT res = HidD_GetProductString(file_handle, dd.productString, sizeof(wchar_t) * 126);
			err = GetLastError();

			CloseHandle(file_handle);
			err = GetLastError();

			for (int i = 0; i < g_Calibrations.size(); ++i)
			{
				if (wcscmp(g_Calibrations[i].productString, dd.productString) == 0)
				{
					dd.calibration = &g_Calibrations[i];
					break;
				}
			}

			deviceData = &dd;
		}
		else
		{
			// Error
			goto Error;
		}
	}

	//
	// Get the preparsed data block
	//

	CHECK( GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0 );
	CHECK( pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize) );
	CHECK( (int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0 );

	//
	// Get the joystick's capabilities
	//

	// Button caps
	CHECK( HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS )
	CHECK( pButtonCaps = (PHIDP_BUTTON_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps) );

	capsLength = Caps.NumberInputButtonCaps;
	CHECK( HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )
	deviceData->NumberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

	// Value caps
	CHECK( pValueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps) );
	capsLength = Caps.NumberInputValueCaps;
	CHECK( HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )

	//
	// Get the pressed buttons
	//

	usageLength = deviceData->NumberOfButtons;
	CHECK(
		HidP_GetUsages(
			HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
			(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
		) == HIDP_STATUS_SUCCESS );

	ZeroMemory(deviceData->bButtonStates, sizeof(deviceData->bButtonStates));
	for(i = 0; i < usageLength; i++)
		deviceData->bButtonStates[usage[i] - pButtonCaps->Range.UsageMin] = TRUE;

	//
	// Get the state of discrete-valued-controls
	//

	for(i = 0; i < Caps.NumberInputValueCaps; i++)
	{
		CHECK(
			HidP_GetUsageValue(
				HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
				(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS );

		switch(pValueCaps[i].Range.UsageMin)
		{
		case 0x30:	// X-axis
			deviceData->lAxisX = (LONG)value;
			break;

		case 0x31:	// Y-axis
			deviceData->lAxisY = (LONG)value;
			break;

		case 0x32: // Z-axis
			deviceData->lAxisZ = (LONG)value;
			break;

		case 0x33: // Rotate-X
			deviceData->lAxisRx = (LONG)value;
			break;

		case 0x34: // Rotate-Y
			deviceData->lAxisRy = (LONG)value;
			break;

		case 0x35: // Rotate-Z
			deviceData->lAxisRz = (LONG)value;
			break;

		case 0x36: // Slider
			deviceData->lSlider = (LONG)value;
			break;

		case 0x37: // Dial
			deviceData->lDial = (LONG)value;
			break;

		case 0x39:	// Hat Switch
			deviceData->lHat = value;
			break;
		}
	}

	//
	// Clean up
	//

Error:
	SAFE_FREE(pPreparsedData);
	SAFE_FREE(pButtonCaps);
	SAFE_FREE(pValueCaps);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CREATE:
		{
			//
			// Register for joystick devices
			//
			const UINT numDevices = 3;
			RAWINPUTDEVICE rid[numDevices];
			rid[0].usUsagePage = 1;
			rid[0].usUsage = 5;	// Gamepad - e.g. XBox 360 or XBox One controllers
			rid[0].dwFlags = RIDEV_INPUTSINK; // Recieve messages when in background.
			rid[0].hwndTarget = hWnd;

			rid[1].usUsagePage = 1;
			rid[1].usUsage = 4;	// Joystick
			rid[1].dwFlags = RIDEV_INPUTSINK; // Recieve messages when in background.
			rid[1].hwndTarget = hWnd;

			rid[2].usUsagePage = 1;
			rid[2].usUsage = 6;	// Keyboard
			rid[2].dwFlags = RIDEV_INPUTSINK; // Recieve messages when in background.
			rid[2].hwndTarget = hWnd;

			if(!RegisterRawInputDevices(rid, numDevices, sizeof(RAWINPUTDEVICE)))
				return -1;
		}
		return 0;

	case WM_INPUT:
		{
			//
			// Get the pointer to the raw device data, process it and update the window
			//

			PRAWINPUT pRawInput;
			UINT      bufferSize;
			HANDLE    hHeap;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));

			hHeap     = GetProcessHeap();
			pRawInput = (PRAWINPUT)HeapAlloc(hHeap, 0, bufferSize);
			if(!pRawInput)
				return 0;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));

			switch (pRawInput->header.dwType)
			{
				case RIM_TYPEHID:
					ParseRawInput(pRawInput);
					break;
				case RIM_TYPEKEYBOARD:
				{
					UINT msg = pRawInput->data.keyboard.Message;
					g_Mapper->SetKey(pRawInput->data.keyboard.VKey, msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
				}
					break;
			}

			HeapFree(hHeap, 0, pRawInput);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC         hDC;
			hDC = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	g_hInst = hInstance;

	MSG msg;
	WNDCLASSEX wcex;

	//
	// Register window class
	//
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hbrBackground = CreateSolidBrush(g_TransparentColor);
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hInstance     = hInstance;
	wcex.lpfnWndProc   = WindowProc;
	wcex.lpszClassName = WC_MAINFRAME;
	wcex.lpszMenuName  = NULL;
	wcex.style         = CS_HREDRAW | CS_VREDRAW;

	if(!RegisterClassEx(&wcex))
		return -1;

	//
	// Create window
	//
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &g_DesktopRect);

	int overlayWidth = g_DesktopRect.right / 4;
	int overlayHeight = g_DesktopRect.bottom / 2;
	int overlayX = 0;
	int overlayY = g_DesktopRect.bottom - overlayHeight;

	g_hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED, WC_MAINFRAME, TEXT("DCS vJoy Mapper"), WS_POPUP, overlayX, overlayY, overlayWidth, overlayHeight, NULL, NULL, hInstance, NULL);
	DWORD er = GetLastError();
	SetLayeredWindowAttributes(g_hWnd, g_TransparentColor, 0, LWA_COLORKEY);

	ShowWindow(g_hWnd, nShowCmd);
	UpdateWindow(g_hWnd);

	// Create DirectX device
	g_RenderDevice = RenderDevice(g_hWnd);

	if (FAILED(g_RenderDevice.InitDevice()))
	{
		g_RenderDevice.CleanupDevice();
		return 0;
	}

	g_RenderDevice.SetClearColor(g_TransparentColor);

	// Setup HID device calibration data
	SetupCalibrations();

	//
	// vJoy init
	//
	int stat = 0;
	id = 1;

	// Get the driver attributes (Vendor ID, Product ID, Version Number)
	if (!vJoyEnabled())
	{
		MsgBox(g_hWnd, "Function vJoyEnabled Failed - make sure that vJoy is installed and enabled\n");
		stat = -2;
		goto Exit;
	}
	else
	{
		wprintf(L"Vendor: %s\nProduct :%s\nVersion Number:%s\n", static_cast<TCHAR*> (GetvJoyManufacturerString()), static_cast<TCHAR*>(GetvJoyProductString()), static_cast<TCHAR*>(GetvJoySerialNumberString()));
	};

	if(!InitJoyDevice(DevID_1))
		goto Exit;

	if (!InitJoyDevice(DevID_2))
		goto Exit;

	g_Mapper = new MAPPER_TYPE();
	g_Mapper->Init();
	g_MapperRenderer = JoyMapperRenderer(&g_RenderDevice, g_Mapper);
	JoyMapperRenderer* ptr = &g_MapperRenderer;
	g_RenderDevice.SetDrawFunc([ptr]() { ptr->Render(); });

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartTime);
	LastTime.QuadPart = StartTime.QuadPart;

	//
	// Message loop
	//	
	bool bGotMsg;
	msg.message = WM_NULL;
	double renderTime = 0.0;

	while (WM_QUIT != msg.message)
	{
		// Use PeekMessage() so we can use idle time to render the scene and call pEngine->DoWork()
		bGotMsg = (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE) != 0);

		if (bGotMsg)
		{
			// Translate and dispatch the message
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			QueryPerformanceCounter(&CurrentTime);
			ElapsedMicroseconds.QuadPart = CurrentTime.QuadPart - LastTime.QuadPart;
			LastTime.QuadPart = CurrentTime.QuadPart;

			ElapsedMicroseconds.QuadPart *= 1000000;
			ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

			double elapsedTime = (double)(((CurrentTime.QuadPart - StartTime.QuadPart) * 1000000) / Frequency.QuadPart) / 1000000.0;
			double deltaTime = (double)ElapsedMicroseconds.QuadPart / 1000000.0;

			STime time = { elapsedTime, deltaTime };

			HandleController(time);
			g_MapperRenderer.Update(time);

			renderTime -= time.time;
			if (renderTime <= 0)
			{
				g_RenderDevice.Render();
				renderTime += 1.0 / 30.0;
			}

			Sleep(8);
		}
	}

	delete g_Mapper;

Exit:
	RelinquishVJD(DevID_1);
	RelinquishVJD(DevID_2);
	g_RenderDevice.CleanupDevice();
	return (int)msg.wParam;
}

bool InitJoyDevice(UINT id)
{
	// Get the status of the vJoy device before trying to acquire it
	VjdStat status = GetVJDStatus(id);

	switch (status)
	{
	case VJD_STAT_OWN:
		_tprintf("vJoy device %d is already owned by this feeder\n", id);
		break;
	case VJD_STAT_FREE:
		_tprintf("vJoy device %d is free\n", id);
		break;
	case VJD_STAT_BUSY:
		MsgBox(g_hWnd, "vJoy device %d is already owned by another feeder\nCannot continue\n", id);
		return false;
	case VJD_STAT_MISS:
		MsgBox(g_hWnd, "vJoy device %d is not installed or disabled\nCannot continue\n", id);
		return false;
	default:
		MsgBox(g_hWnd, "vJoy device %d general error\nCannot continue\n", id);
		return false;
	};

	// Acquire the vJoy device
	if (!AcquireVJD(id))
	{
		MsgBox(g_hWnd, "Failed to acquire vJoy device number %d.\n", id);
		return false;
	}
	else
		_tprintf("Acquired device number %d - OK\n", id);

	return true;
}

bool UpdateJoyDevice(UINT id, JOYSTICK_POSITION_V2& iReport)
{
	pPositionMessage = (PVOID)(&iReport);
	if (!UpdateVJD(id, pPositionMessage))
	{
		printf("Feeding vJoy device number %d failed - try to enable device then press enter\n", id);
		AcquireVJD(id);
		return false;
	}

	return true;
}

void HandleController(const STime& time)
{
	UpdateControllerState();
	XINPUT_GAMEPAD& gamepad = g_Controllers[0].state.Gamepad;

	DeviceData* dd = GetDevice(XBOXCONTROLLER);
	if (dd != NULL)
	{
		bool btnGuide = dd->bButtonStates[10];
		bool btnShare = dd->bButtonStates[11];

		if (btnGuide) gamepad.wButtons |= JOYPAD_GUIDE;
		if (btnShare) gamepad.wButtons |= JOYPAD_SHARE;
	}

	SetMapperData(g_Mapper, GetDevice(VKBKG12));
	//g_Mapper->SetButtons(gamepad.wButtons);
	//g_Mapper->SetAxesXInput(gamepad.sThumbLX, gamepad.sThumbLY, gamepad.bLeftTrigger, gamepad.sThumbRX, gamepad.sThumbRY, gamepad.bRightTrigger);
	g_Mapper->Update(time);

	// Set destination vJoy device
	id = (BYTE)DevID_1;
	iReport.bDevice = id;

	// Set position data of axes
	iReport.wAxisX =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_X);
	iReport.wAxisY =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_Y);
	iReport.wAxisZ =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_Z);
	iReport.wAxisXRot =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_RX);
	iReport.wAxisYRot =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_RY);
	iReport.wAxisZRot =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_RZ);
	iReport.wSlider =	g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_SLIDER);
	iReport.wDial =		g_Mapper->GetMappedAxis(0, JoyMapper::AXIS_DIAL);	

	// Set position data of buttons
	iReport.lButtons =		g_Mapper->GetMappedButtons(0);
	iReport.lButtonsEx1 =	g_Mapper->GetMappedButtons(1);
	iReport.lButtonsEx2 =	g_Mapper->GetMappedButtons(2);
	iReport.lButtonsEx3 =	g_Mapper->GetMappedButtons(3);

	// Set position data of hat
	iReport.bHats =		g_Mapper->GetMappedPov(0);
	iReport.bHatsEx1 =	g_Mapper->GetMappedPov(1);
	iReport.bHatsEx2 =	g_Mapper->GetMappedPov(2);
	iReport.bHatsEx3 =	g_Mapper->GetMappedPov(3);

	// Extra vJoy device
	iReportEx.bDevice = (BYTE)DevID_2;

	iReportEx.wAxisX =		g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_X);
	iReportEx.wAxisY =		g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_Y);
	iReportEx.wAxisZ =		g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_Z);
	iReportEx.wAxisXRot =	g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_RX);
	iReportEx.wAxisYRot =	g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_RY);
	iReportEx.wAxisZRot =	g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_RZ);
	iReportEx.wSlider =		g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_SLIDER);
	iReportEx.wDial =		g_Mapper->GetMappedAxis(1, JoyMapper::AXIS_DIAL);	

	// Send position data to vJoy device
	UpdateJoyDevice(DevID_1, iReport);
	UpdateJoyDevice(DevID_2, iReportEx);

	WORD left, right;
	g_Mapper->GetVibration(left, right);
	SetVibration(0, left, right);
}

HRESULT UpdateControllerState()
{
	DWORD dwResult;
	for (DWORD i = 0; i < MAX_CONTROLLERS; i++)
	{
		// Simply get the state of the controller from XInput.
		dwResult = XInputGetState(i, &g_Controllers[i].state);

		if (dwResult == ERROR_SUCCESS)
			g_Controllers[i].bConnected = true;
		else
			g_Controllers[i].bConnected = false;
	}

	return S_OK;
}

void SetVibration(DWORD controllerId, WORD left, WORD right)
{
	XINPUT_VIBRATION v;

	ZeroMemory(&v, sizeof(XINPUT_VIBRATION));

	v.wLeftMotorSpeed = left;
	v.wRightMotorSpeed = right;

	XInputSetState(controllerId, &v);
}

void MsgBox(HWND hWnd, const char* str, ...)
{
	va_list vl;
	va_start(vl, str);
	char buff[1024];  // May need to be bigger
	vsprintf(buff, str, vl);
	MessageBox(hWnd, buff, "MsgTitle", MB_OK | MB_ICONERROR);
}

void SetupCalibrations()
{
	DeviceCalibrationData vkbCalib = { 0 };
	wcscpy(vkbCalib.productString , VKBSTICK);
	vkbCalib.numHats = 0;

	vkbCalib.rangeAxisX = 4096;
	vkbCalib.rangeAxisY = -4096;
	vkbCalib.rangeAxisZ = 2048;
	vkbCalib.rangeAxisRz = 2048;

	vkbCalib.centerAxisX = true;
	vkbCalib.centerAxisY = true;
	vkbCalib.centerAxisZ = true;
	vkbCalib.centerAxisRz = true;

	g_Calibrations.push_back(vkbCalib);

	DeviceCalibrationData vkbKg12Calib = { 0 };
	wcscpy(vkbKg12Calib.productString, VKBKG12);
	vkbKg12Calib.numHats = 0;

	vkbKg12Calib.rangeAxisX = 4096;
	vkbKg12Calib.rangeAxisY = -4096;
	vkbKg12Calib.rangeAxisZ = 2048;

	vkbKg12Calib.centerAxisX = true;
	vkbKg12Calib.centerAxisY = true;
	vkbKg12Calib.centerAxisZ = true;

	g_Calibrations.push_back(vkbKg12Calib);

	DeviceCalibrationData twcsCalib = { 0 };
	wcscpy(twcsCalib.productString, TWCSTHROTTLE);
	twcsCalib.numHats = 1;

	twcsCalib.rangeAxisX = 1024;
	twcsCalib.rangeAxisY = -1024;
	twcsCalib.rangeAxisZ = -65536;
	twcsCalib.rangeAxisRz = 1024;
	twcsCalib.rangeDial = -1024;

	twcsCalib.centerAxisX = true;
	twcsCalib.centerAxisY = true;
	twcsCalib.centerAxisRz = true;

	g_Calibrations.push_back(twcsCalib);
}

double AxisRemap(long input, long range, bool centering)
{
	if (range < 0)
		input = -range - input - 1;

	range = fabs(range);

	return ((double)(input - (centering ? range / 2 : 0)) / (double)(range - 1)) * (centering ? 2 : 1);
}

void SetMapperData(JoyMapper* mapper, DeviceData* deviceData)
{
	if (deviceData == NULL)
		return;	

	unsigned long buttons = 0;
	int numHats = 0;

	if (deviceData->calibration != NULL)
	{
		DeviceCalibrationData* cd = deviceData->calibration;
		numHats = cd->numHats;

		mapper->SetAxesHID(
			AxisRemap(deviceData->lAxisX, cd->rangeAxisX, cd->centerAxisX),
			AxisRemap(deviceData->lAxisY, cd->rangeAxisY, cd->centerAxisY),
			AxisRemap(deviceData->lAxisZ, cd->rangeAxisZ, cd->centerAxisZ),
			AxisRemap(deviceData->lAxisRx, cd->rangeAxisRx, cd->centerAxisRx),
			AxisRemap(deviceData->lAxisRy, cd->rangeAxisRy, cd->centerAxisRy),
			AxisRemap(deviceData->lAxisRz, cd->rangeAxisRz, cd->centerAxisRz),
			AxisRemap(deviceData->lSlider, cd->rangeSlider, cd->centerSlider),
			AxisRemap(deviceData->lDial, cd->rangeDial, cd->centerDial)
		);
	}
	else
	{
		mapper->SetAxesHID(
			AxisRemap(deviceData->lAxisX, 65535, true),
			AxisRemap(deviceData->lAxisY, 65535, true),
			AxisRemap(deviceData->lAxisZ, 256, true),
			AxisRemap(deviceData->lAxisRx, 65535, true),
			AxisRemap(deviceData->lAxisRy, 65535, true),
			AxisRemap(deviceData->lAxisRz, 256, true),
			AxisRemap(deviceData->lSlider, 65535, false),
			AxisRemap(deviceData->lDial, 65535, false)
		);
	}

	for (int i = 0; i < sizeof(long) * 8 && i < deviceData->NumberOfButtons; ++i)
	{
		if (deviceData->bButtonStates[i])
			buttons |= 1 << i;
	}

	mapper->SetButtonsPov(buttons, numHats > 0 ? deviceData->lHat : 8);
}