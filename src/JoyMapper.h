#pragma once
#include <functional>
#include <string>
#include <vector>

#include "ButtonDefines.h"

//-----------------------------------------------------------------------------
// Windows mouse events
//-----------------------------------------------------------------------------
#define LMB_DOWN				0x0002
#define LMB_UP					0x0004
#define RMB_DOWN				0x0008
#define RMB_UP					0x0010
#define MMB_DOWN				0x0020
#define MMB_UP					0x0040

//-----------------------------------------------------------------------------
// Mapping parameters
//-----------------------------------------------------------------------------
#define STICK_SLIDER_DEADZONE	0.97
#define SLIDER_RELATIVE_SPEED	1.0
#define SLIDER_FOLLOW_SPEED		3.0
#define SLIDER_INVALID_TIME		0.2

#define STICK_VIEW_DEADZONE		0.05
#define STICK_VIEW_SPEED		5.0
#define STICK_VIEW_CURVE		2.0
#define VIEW_CENTERING_TIME		1.0
#define ABSOLUTE_VIEW_SPEED		10.0

#define MOUSE_CURSOR_RADIUS		0.9
#define MOUSE_VIEW_THRESHOLD	0.5
#define MOUSE_VIEW_SPEED		0.4

#define TEMPO_TIME				0.25
#define SCROLL_INTERVAL			0.1

#define VIBRATION_TIME			0.06
#define BUTTON_HOLD_TIME		0.05
#define VIRTUAL_POV_DEADZONE	0.2

//-----------------------------------------------------------------------------
// Helper macros
//-----------------------------------------------------------------------------
#define BTNDOWN(btn) (m_JoyButtons & btn)
#define BTNPRESSED(btn) (!(m_JoyButtonsPrev & btn) && (m_JoyButtons & btn))
#define BTNRELEASED(btn) ((m_JoyButtonsPrev & btn) && !(m_JoyButtons & btn))
#define BTNTIME(btn) m_ButtonPressedTime[GetShiftAmount(btn)]
#define FLAG(a) (1 << a)

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------
double Clamp(double val, double min, double max);
double Magnitude(double x, double y);
double Lerp(double val, double target, double t);
double MoveTo(double val, double target, double maxDelta);
double PowerCurve(double val, double power);
double ApplyDeadzoneRegion(double val, double detent, double dz);
int GetShiftAmount(unsigned long flag);

#define NUM_PHYSICAL_BUTTONS	32
#define MAX_VIRTUAL_POVS		3
#define MAX_LOGICAL_BUTTONS		64
#define MAX_SLIDER_REGIONS		2
#define NUM_BUTTON_EXTENSIONS	4
#define MAX_SPECIAL_BUTTONS		10
#define NUM_MOUSE_BUTTONS		4
#define MAX_BUTTON_ON_AXIS		3
#define MAX_KEYS				245

#define M_PI 3.14159265358979323846

struct STime
{
	double time;
	double deltaTime;
};

class JoyMapper
{
	friend class JoyMapperRenderer;

public:
	enum AxisID
	{
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_RX,
		AXIS_RY,
		AXIS_RZ,
		AXIS_SLIDER,
		AXIS_DIAL,		
	};

protected:
	enum Mode
	{
		MODE_DEFAULT = 0,
		MODE_LEFT_MOD,
		MODE_RIGHT_MOD,
		MODE_BOTH_MOD,
		MODE_MOUSE,
		MODE_NUM
	};

	struct Stick
	{
		double X;
		double Y;
		double Angle;
		double Magnitude;
		
		void SetAxes(long x, long y);
		void UpdateAngleMagnitude();
	};

	struct StickSliderRegion
	{
		double from;
		double to;
		double internalValue;
		double* output;
		bool relative;
		bool invert;
		double detentHigh;
		double detentLow;
		double detentHigh_Dz;
	};

	struct StickSlider
	{
		Stick* stick;
		StickSliderRegion regions[MAX_SLIDER_REGIONS];
		int activeRegionIdx;
		double lastAngle;
		double lastUpdateTime;
		bool detentUnlocked;
		bool invalid;		

		std::function<void(unsigned short, unsigned short, double)> vibrFunc;

		void SetRegion(unsigned int regionIdx, double from, double to, double* output, bool relative, bool invert = false);
		void SetDetent(unsigned int regionIdx, double low, double high, double highDz = 0);
		void Update(const STime& time);

		double* GetDetentPtr(unsigned int regionIdx, bool high = true);
	};

	struct StickView
	{
		Stick* stick;
		double* outputX;
		double* outputY;
		double* fovAxis;
		double fovFrom;
		double fovTo;		
		double centeringTime;
		double lastUpdateTime;
		bool centering;
		bool invalid;
		bool absolute;

		double absOffsetX;
		double absOffsetY;

		void SetFovSettings(double* axis, double from, double to);
		void Update(bool enableInput, const STime& time);
		void Recenter();
		void SetOffset();
	};

	struct RadialButtons
	{
		Stick* stick;
		int numButtons;
		int activeButtonIdx;
		double lastUpdateTime;
		double deadzoneOverride;
		bool showCursor;
		bool overlappedButtons;
		bool invalid;
		
		void Update(const STime& time);
		void AddLogicalButtons(std::function<void(bool)> addFunc);
	};

	enum SpecialButtonType
	{
		SBT_NONE = 0,
		SBT_TEMPO,
		SBT_AXIS2BTN,
	};

	struct SpecialButton
	{
		SpecialButtonType type;
		unsigned long physicalButton;
		double lastPressedTime;
		unsigned char modeMask;
		// Axis2Btn
		double* sourceAxis;
		double sourceThreshold;

		void SetTempo(unsigned long button, unsigned char mask);
		void SetAxis2Btn(double* axis, double threshold, unsigned char mask);
	};

	struct ButtonAxis
	{
		double* output;
		double values[MAX_BUTTON_ON_AXIS];
		int numValues;
		int valueIdx;
		bool moveToNext;
		double speedModifier;

		ButtonAxis& AddValue(double value);
		void Update(const STime& time);
		void CycleValue(bool reverse = false);
		void MoveTowardNextValue();
	};

	struct ButtonThrottle
	{
		double* output = nullptr;
		double* abDetent = nullptr;
		double speed = 1.0;
		int dir = 0;
		bool pressed = false;

		void Update(const STime& time);
	};

	struct MenuOption
	{
		std::wstring label;
		std::function<void()> func;

		MenuOption();
		MenuOption(std::wstring label, std::function<void()> func);
	};

	// Internal parameters
	Mode m_Mode;
	bool m_IsMenuMode;
	unsigned long m_JoyButtons;
	unsigned long m_JoyButtonsPrev;
	unsigned long m_JoyButtonsProcessed;
	Stick m_LStick;
	Stick m_RStick;
	double m_LTrigger;
	double m_RTrigger;

	unsigned long m_MenuActivateBtn1;
	unsigned long m_MenuActivateBtn2;
	unsigned long m_MenuAcceptBtn;
	unsigned long m_MenuCancelBtn;
	unsigned long m_MenuUpBtn;
	unsigned long m_MenuRightBtn;
	unsigned long m_MenuDownBtn;
	unsigned long m_MenuLeftBtn;
	
	// HID axes
	double m_PhysAxisX;
	double m_PhysAxisY;
	double m_PhysAxisZ;
	double m_PhysAxisRX;
	double m_PhysAxisRY;
	double m_PhysAxisRZ;
	double m_PhysDial;
	double m_PhysSlider;

	RadialButtons m_VirtualPOV[MAX_VIRTUAL_POVS];

	double m_ButtonPressedTime[NUM_PHYSICAL_BUTTONS];
	double m_ButtonHoldTime[NUM_PHYSICAL_BUTTONS];
	unsigned long m_MouseButtons[NUM_MOUSE_BUTTONS];
	SpecialButton m_SpecialButtons[MAX_SPECIAL_BUTTONS];

	double* m_AfterburnerDetent;

	std::vector<MenuOption> m_MenuOptions;
	int m_MenuIdx;

	// Output axes
	double m_AxisX;
	double m_AxisY;
	double m_AxisZ;
	double m_AxisRX;
	double m_AxisRY;
	double m_AxisRZ;
	double m_Dial;
	double m_Slider;

	double m_Ex_AxisX;
	double m_Ex_AxisY;
	double m_Ex_AxisRX;
	double m_Ex_AxisRY;
	double m_Ex_AxisRZ;

	long m_LogicalButtons[NUM_BUTTON_EXTENSIONS];
	int m_ScrollDirection;
	bool m_ScrollDirty;

	unsigned short m_VibrationLeft;
	unsigned short m_VibrationRight;
	double m_VibrationTime;

	bool m_Keys[MAX_KEYS];
	bool m_KeysPrev[MAX_KEYS];

	long m_mouseDeltaX;
	long m_mouseDeltaY;
	bool m_mouseExBtn1;
	bool m_mouseExBtn2;
	bool m_mouseExBtn1_prev;
	bool m_mouseExBtn2_prev;

	void SetLogicalButton(int index, bool on);
	void UpdateLogicalButtons(const STime& time);
	int UpdateSpecialButton(SpecialButton& btnSpec, const STime& time);
	int GetSpecialButton(SpecialButton& btnSpec, const STime& time);

	void UpdateMenu(const STime& time);
	void BuildMenu();

	void HandleMouse(const Stick& stick, StickView* stickView = 0, const STime* time = 0);
	void HandleMouse(const Stick& stick, unsigned long btnLeft, unsigned long btnRight, unsigned long wheelUp, unsigned long wheelDown, StickView* stickView = 0, const STime* time = 0);
	static void SetCursorPosition(double x, double y);
	void MouseButton(unsigned char mouseBtn);
	void ExtraMouseButton(unsigned int btn, bool keyDown);
	void MouseWheel(int dir);	

	void Vibrate(unsigned short left, unsigned short right, double duration);

	virtual void UpdateInternal(const STime& time) = 0;
	virtual void UpdateLogicalButtonsInternal(int& ctr, const STime& time) = 0;

public:
	JoyMapper();

	void Init();

	static long AxisToLong(double value);
	static long HalfAxisToLong(double value);

	void Update(const STime& time);
	void SetButtons(unsigned long buttons);
	void SetButtonsPov(unsigned long buttons, unsigned long hat);
	void SetAxesXInput(long x, long y, long z, long rx, long ry, long rz);
	void SetAxesHID(double x, double y, double z, double rx, double ry, double rz, double sl, double di);
	void SetKey(unsigned long keyCode, bool down);
	void SetMouse(long dX, long dY, bool btn1, bool btn2);

	long GetMappedButtons(unsigned int index);
	unsigned long GetMappedPov(unsigned int index);
	long GetMappedAxis(unsigned int index, AxisID axis);

	void GetVibration(unsigned short& left, unsigned short& right);
	double GetAfterburnerDetent();

	bool GetKey(unsigned long keyCode);
	bool GetKeyDown(unsigned long keyCode);
	bool GetKeyUp(unsigned long keyCode);
};