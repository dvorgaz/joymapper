#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

MouseThrottleMapper::MouseThrottleMapper()
{
	m_deviceID = GF_DEVICE_ID;

	m_MouseStick = { 0 };

	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;
	m_ZoomAxis = FovToAxis(m_FovDefault);
	m_ThrottleAxis = 0.0;
	m_TaxiThrottleAxis = 0;
	m_axisMode = AM_NONE;
	m_deltaX = 0;
	m_deltaY = 0;	

	m_MenuActivateBtn1 = GF_MENU_ACT_1;
	m_MenuActivateBtn2 = GF_MENU_ACT_2;
	m_MenuAcceptBtn = GF_MENU_ACCEPT;
	m_MenuCancelBtn = GF_MENU_CANCEL;
	m_MenuUpBtn = GF_MENU_UP;
	m_MenuRightBtn = GF_MENU_RIGHT;
	m_MenuDownBtn = GF_MENU_DOWN;
	m_MenuLeftBtn = GF_MENU_LEFT;

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_ZoomAxis;
	m_ButtonAxis.AddValue(0.75).AddValue(0.15);	

	m_WheelBrakeAxis = { 0 };
	m_WheelBrakeAxis.output = &m_AxisRZ;
	m_WheelBrakeAxis.AddValue(0.0).AddValue(1.0);
	m_WheelBrakeAxis.speedModifier = -1.0;

	m_TaxiButtonAxis = { 0 };
	m_TaxiButtonAxis.output = &m_TaxiThrottleAxis;
	m_TaxiButtonAxis.AddValue(0.0).AddValue(0.5);
	m_TaxiButtonAxis.speedModifier = -2.0;

	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	m_ScreenWidth = desktop.right;
	m_ScreenHeight = desktop.bottom;

	m_MouseLocked = 0;
	m_SavedMouseX = 0;
	m_SavedMouseY = 0;
	m_TaxiMode = false;

	m_SpecialButtons[0].SetTempo(GF_EX_1, FLAG(MODE_DEFAULT) | FLAG(MODE_LEFT_MOD));

	m_Filter = new EWMAFilter(0.03, 0.06, 0.97);
}

void MouseThrottleMapper::UpdateInternal(const STime& time)
{
	const unsigned long brakeBtn = GF_TRIGGER_1;
	const unsigned long gasBtn = GF_WPN_REL;

	const bool useMouseLook = true;
	const int throttleBtn = 1;
	const int viewBtn = 0;

	m_Mode = MODE_DEFAULT;
	if (MOUSEDOWN(viewBtn))
	{
		m_Mode = MODE_RIGHT_MOD;
	}
	else if (MOUSEDOWN(throttleBtn))
	{
		m_Mode = MODE_LEFT_MOD;
	}

	static double leftModTime = time.time;
	static double rightModTime = time.time;
	static double lastLookToggleTime = time.time;

	static POINT point;

	static bool centering = false;
	static double centeringTime = 0;
	auto Recenter = []() { centering = true; centeringTime = 0; };

	static bool mouseLook = false;
	static bool viewChanged = false;
	static bool viewBtnDirty = false;
	static bool manualZoom = false;
	static bool tdcMode = false;

	if (m_ButtonAxis.values[0] != FovToAxis(m_FovDefault))
		m_ButtonAxis.SetValues(2, FovToAxis(m_FovDefault), FovToAxis(45));

	if (GetKeyDown(VK_OEM_102))
		m_TaxiMode = !m_TaxiMode;

	if(viewChanged)
		m_Mode = MODE_DEFAULT;

	if(useMouseLook)
		mouseLook = MOUSEDOWN(viewBtn);

	m_ThrottleDisplay = AXD_HIDE;

	if (MOUSEPRESSED(throttleBtn))
	{
		if(time.time - leftModTime < 0.3)
			tdcMode = true;

		leftModTime = time.time;
		m_deltaX = 0;
		m_deltaY = 0;

		LockMouse(true);
	}
	if (MOUSERELEASED(throttleBtn))
	{
		m_axisMode = AM_NONE;		
		tdcMode = false;

		LockMouse(false);

		if (time.time - leftModTime < TEMPO_TIME)
		{		
			// Reset zoom
			//m_ZoomAxis = FovToAxis(m_FovDefault);
		}
	}

	if (MOUSEPRESSED(viewBtn))
	{
		rightModTime = time.time;
		m_deltaX = 0;
		m_deltaY = 0;

		LockMouse(true);
	}
	if (MOUSERELEASED(viewBtn))
	{
		viewChanged = false;

		LockMouse(false);

		if (!viewBtnDirty && time.time - rightModTime < TEMPO_TIME)
		{
			//if (useMouseLook)
			//{
			//	Recenter();
			//}

			// Zoom
			if(manualZoom)
				manualZoom = false;
			else
				m_ButtonAxis.CycleValue();
		}

		viewBtnDirty = false;
	}

	if(m_MouseLocked)
		SetCursorPositionScreenSpace(1, 0);

	bool lateralMove = false;
	if (useMouseLook && (m_Mode == MODE_DEFAULT || m_Mode == MODE_RIGHT_MOD))
	{
		if (mouseLook)
		{
			const double lookSensitivity = 0.0015;
			if (!MOUSEDOWN(throttleBtn))
			{
				// Mouse look
				double speed = lookSensitivity * (AxisToFov(m_ZoomAxis) / 80);
				m_MouseStick.X = Clamp(m_MouseStick.X + ((double)m_mouseDeltaX * speed), -1, 1);
				m_MouseStick.Y = Clamp(m_MouseStick.Y - ((double)m_mouseDeltaY * speed * 2), -1, 1);
			}
			else
			{
				// Camera move
				lateralMove = true;
				viewBtnDirty = true;
				m_HeadX = Clamp(m_HeadX + ((double)m_mouseDeltaX * lookSensitivity), -1, 1);
				m_HeadY = Clamp(m_HeadY - ((double)m_mouseDeltaY * lookSensitivity), -1, 1);
			}

			const long axisDeadzone = 15;
			m_deltaX += m_mouseDeltaX;
			m_deltaY += m_mouseDeltaY;

			if (abs(m_deltaY) > axisDeadzone || abs(m_deltaX) > axisDeadzone)
			{
				viewChanged = true;
				viewBtnDirty = true;
				m_Mode = MODE_DEFAULT;
			}
		}
	}

	if (tdcMode && MOUSEDOWN(throttleBtn) && !mouseLook)
	{
#if 0
		const double speed = 0.005;
		m_AxisX = Clamp(m_AxisX + ((double)m_mouseDeltaX * speed), -1, 1);
		m_AxisY = Clamp(m_AxisY - ((double)-m_mouseDeltaY * speed), -1, 1);
#else
		const double speed = 0.001;
		const double smoothing = 20;
		double x = Clamp(((double)m_mouseDeltaX / time.deltaTime) * speed, -1, 1);
		double y = Clamp(((double)-m_mouseDeltaY / time.deltaTime) * speed, -1, 1);
		m_AxisX = Lerp(m_AxisX, x, time.deltaTime * smoothing);
		m_AxisY = Lerp(m_AxisY, y, time.deltaTime * smoothing);
#endif
	}
	else
	{
		m_AxisX = 0;
		m_AxisY = 0;
	}

	if (!lateralMove)
	{
		// Recenter horizontal
		//if (fabs(m_HeadX) < 0.08)
		m_HeadX = Lerp(m_HeadX, 0, time.deltaTime * 10);
		// Recenter vertical
		if(fabs(m_HeadY) < 0.08)
		m_HeadY = Lerp(m_HeadY, 0, time.deltaTime * 10);
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		// Wheel brake
		//if (BTNDOWN(brakeBtn))
		//{
		//	m_WheelBrakeAxis.MoveTowardNextValue();
		//}

		if (m_axisMode == AM_NONE)
		{
			const long axisDeadzone = 15;
			m_deltaX += m_mouseDeltaX;
			m_deltaY += m_mouseDeltaY;

			if (abs(m_deltaY) > axisDeadzone)
			{
				m_axisMode = AM_THROTTLE;
			}
			else if (abs(m_deltaX) > axisDeadzone * 2)
			{
				m_axisMode = AM_ZOOM;
			}
		}

		if (m_axisMode == AM_THROTTLE)
		{
			// Throttle
			m_ThrottleDisplay = AXD_SHOW;
			const double throttleSensitivity = 0.001;
			m_ThrottleAxis = Clamp(m_ThrottleAxis + ((double)m_mouseDeltaY * -throttleSensitivity), 0, 1);			
		}
		else if (m_axisMode == AM_ZOOM)
		{
			// Zoom
			const double zoomSensitivity = 0.001;
			m_ZoomAxis = Clamp(m_ZoomAxis + ((double)m_mouseDeltaX * -zoomSensitivity), 0, 1);
			m_Dial = m_ZoomAxis;
			manualZoom = true;
		}
	}
	else
	{
		if(!manualZoom)
			m_ButtonAxis.Update(time);

		m_Dial = m_ZoomAxis;
	}

	if (m_Mode == MODE_RIGHT_MOD)
	{
		if (!useMouseLook)
		{
			// TDC slew
			const double slewSensitivity = 0.003;
			m_AxisRX = Clamp(m_AxisRX + ((double)m_mouseDeltaX * slewSensitivity), -1, 1);
			m_AxisRY = Clamp(m_AxisRY - ((double)m_mouseDeltaY * slewSensitivity), -1, 1);
		}
	}
	else
	{
		if (!useMouseLook)
		{
			m_AxisRX = 0;
			m_AxisRY = 0;
		}
	}

	if (useMouseLook)
	{
		if (!centering && !mouseLook)
		{
			// Auto center near the middle
			double deadZone = 0.1 * (AxisToFov(m_ZoomAxis) / 90);
			double sqMag = m_MouseStick.X * m_MouseStick.X + m_MouseStick.Y * m_MouseStick.Y * 0.25;
			if (sqMag < deadZone * deadZone)
				Recenter();
		}

		if (centering)
		{
			centeringTime += time.deltaTime;
			double t = min(centeringTime / VIEW_CENTERING_TIME, 1.0);
			m_MouseStick.X = Lerp(m_MouseStick.X, 0, t);
			m_MouseStick.Y = Lerp(m_MouseStick.Y, 0 /*+ m_ViewOffsetY*/, t);

			if (fabs(m_MouseStick.X) < 0.001 && fabs(m_MouseStick.Y /*- m_ViewOffsetY*/) < 0.001)
			{
				m_MouseStick.X = 0;
				m_MouseStick.Y = 0 /*+ m_ViewOffsetY*/;
				centering = false;
			}
		}

		m_HeadRX = m_MouseStick.X;
		m_HeadRY = m_MouseStick.Y;
	}

	if (m_TaxiMode && BTNDOWN(brakeBtn))
	{
		m_WheelBrakeAxis.MoveTowardNextValue();
	}

	if (m_TaxiMode && BTNDOWN(gasBtn))
	{
		m_TaxiButtonAxis.MoveTowardNextValue();
	}

	m_WheelBrakeAxis.Update(time);
	m_TaxiButtonAxis.Update(time);

	// Throttle
	m_Slider = fmax(ApplyDeadzoneRegion(m_ThrottleAxis, m_ABDetent, 0.15), m_TaxiThrottleAxis);

	// Rudder
	m_AxisZ = m_PhysAxisZ;

	m_VirtualPOV[0].SetHatButtons(m_Mode == MODE_DEFAULT || m_Mode == MODE_LEFT_MOD, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));
	m_VirtualPOV[1].SetHatButtons(m_Mode == MODE_RIGHT_MOD, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));

	//for (int i = 0; i < MAX_VIRTUAL_POVS; ++i)
	//	m_VirtualPOV[i].SetHatButtons(m_Mode == i, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));

	if (m_TaxiMode)
	{
		swprintf(m_ServiceText, L"T");
	}
	else if (m_Mode == MODE_RIGHT_MOD && time.time - rightModTime > TEMPO_TIME)
	{
		swprintf(m_ServiceText, L"M");
	}
	else if (tdcMode && MOUSEDOWN(throttleBtn))
	{
		double sqMag = m_AxisX * m_AxisX + m_AxisY * m_AxisY;
		swprintf(m_ServiceText, sqMag >= 1 ? L"|L|" : L"| |");
	}
	else
	{
		swprintf(m_ServiceText, L"");
	}
}

void MouseThrottleMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	AddButton add(this, ctr);
	unsigned char mask = FLAG(MODE_DEFAULT) | FLAG(MODE_RIGHT_MOD);

	for (int i = 0; i < MODE_NUM - 2; ++i)
	{
		add.Layer(i);

		bool taxi = m_TaxiMode && (FLAG(i) & (FLAG(MODE_DEFAULT) | FLAG(MODE_LEFT_MOD)));

		add(0xFF, GF_TRIGGER_1, !taxi);
		add(0xFF, GF_TRIGGER_2, !taxi);
		add(0xFF, GF_WPN_REL, !taxi);
		add(mask, GF_R_SIDE);
		add(mask, GF_PINKIE);
		add(0xFF, GF_TRIM_CENTER);
		add(0xFF, GF_DMS_UP);
		add(0xFF, GF_DMS_RIGHT);
		add(0xFF, GF_DMS_DOWN);
		add(0xFF, GF_DMS_LEFT);
		add(0xFF, GF_DMS_CENTER);
		add(0xFF, GF_TMS_UP);
		add(0xFF, GF_TMS_RIGHT);
		add(0xFF, GF_TMS_DOWN);
		add(0xFF, GF_TMS_LEFT);
		add(0xFF, GF_TMS_CENTER);
		add(mask, GF_EX_1);
		add(0xFF, GF_EX_2);
		add(0xFF, GF_EX_3);
	}

#if 0
	const unsigned long timeBtn = GF_EX_1;
	const double pulseDelay = 1.0 / 5;
	const int maxPulse = 6;

	static double lastPulseTime = time.time;
	static int pulseCtr = 0;

	if (BTNRELEASED(timeBtn) && time.time - BTNTIME(timeBtn) < TEMPO_TIME)
	{
		m_ButtonHoldTime[GetShiftAmount(timeBtn)] = BUTTON_HOLD_TIME;
	}
	else if (BTNDOWN(timeBtn) && time.time - BTNTIME(timeBtn) >= TEMPO_TIME)
	{
		if (time.time - lastPulseTime >= pulseDelay && pulseCtr < maxPulse)
		{
			lastPulseTime = time.time;
			pulseCtr++;
		}
	}
	else
	{
		pulseCtr = 0;
	}

	const int btOffset = MAX_LOGICAL_BUTTONS - MAX_SPECIAL_BUTTONS - 2;
	if (ctr < btOffset)
		ctr = btOffset;

	SetLogicalButton(ctr++, m_ButtonHoldTime[GetShiftAmount(timeBtn)] > 0);
	SetLogicalButton(ctr++, time.time - lastPulseTime < BUTTON_HOLD_TIME);
#endif
}

void MouseThrottleMapper::LockMouse(bool locked)
{
	if (locked && !m_MouseLocked)
	{
		POINT point;
		GetCursorPos(&point);
		m_SavedMouseX = point.x;
		m_SavedMouseY = point.y;
	}
	else
	{
		SetCursorPos(m_SavedMouseX, m_SavedMouseY);
	}

	m_MouseLocked += locked ? 1 : -1;
	m_MouseLocked = max(0, m_MouseLocked);
}