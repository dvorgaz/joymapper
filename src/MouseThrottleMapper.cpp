#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

MouseThrottleMapper::MouseThrottleMapper()
{
	m_MouseStick = { 0 };

	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;
	m_ZoomAxis = FovToAxis(m_FovDefault);
	m_ThrottleAxis = 0.0;
	m_axisMode = AM_NONE;
	m_deltaX = 0;
	m_deltaY = 0;

#if USE_KG12
	m_deviceID = VKBKG12;

	m_MenuActivateBtn1 = GF_KG12_HAT_UP;
	m_MenuActivateBtn2 = GF_KG12_TOP;
	m_MenuAcceptBtn = GF_KG12_TOP;
	m_MenuCancelBtn = GF_KG12_PINKIE;
	m_MenuUpBtn = GF_KG12_HAT_UP;
	m_MenuRightBtn = GF_KG12_HAT_RIGHT;
	m_MenuDownBtn = GF_KG12_HAT_DOWN;
	m_MenuLeftBtn = GF_KG12_HAT_LEFT;

#else
	m_deviceID = VKBSTICK;

	m_MenuActivateBtn1 = GF_KOS_R_SIDE;
	m_MenuActivateBtn2 = GF_KOS_WPN_REL;
	m_MenuAcceptBtn = GF_KOS_WPN_REL;
	m_MenuCancelBtn = GF_KOS_R_SIDE;
	m_MenuUpBtn = GF_KOS_TRIM_UP;
	m_MenuRightBtn = GF_KOS_TRIM_RIGHT;
	m_MenuDownBtn = GF_KOS_TRIM_DOWN;
	m_MenuLeftBtn = GF_KOS_TRIM_LEFT;

#endif

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.75).AddValue(0.15);

	m_WheelBrakeAxis = { 0 };
	m_WheelBrakeAxis.output = &m_AxisRZ;
	m_WheelBrakeAxis.AddValue(0.0).AddValue(1.0);
	m_WheelBrakeAxis.speedModifier = -1.0;

	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	m_ScreenWidth = desktop.right;
	m_ScreenHeight = desktop.bottom;

	m_MouseLocked = 0;
	m_SavedMouseX = 0;
	m_SavedMouseY = 0;
}

void MouseThrottleMapper::UpdateInternal(const STime& time)
{
#if USE_KG12
	const unsigned long brakeBtn = GF_KG12_TRIGGER;
#else
	const unsigned long brakeBtn = GF_KOS_TRIGGER_1;
#endif

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

	static bool mouseLook = false;
	static bool viewChanged = false;

	if(viewChanged)
		m_Mode = MODE_DEFAULT;

	if(useMouseLook)
		mouseLook = MOUSEDOWN(viewBtn);

	if (MOUSEPRESSED(throttleBtn))
	{
		leftModTime = time.time;
		m_deltaX = 0;
		m_deltaY = 0;

		LockMouse(true);
	}
	if (MOUSERELEASED(throttleBtn))
	{
		m_axisMode = AM_NONE;		

		LockMouse(false);

		if (time.time - leftModTime < TEMPO_TIME)
		{
			// Zoom
			//m_ButtonAxis.CycleValue();

			// Reset zoom
			m_ZoomAxis = FovToAxis(m_FovDefault);
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

		if (time.time - rightModTime < TEMPO_TIME)
		{
			if (useMouseLook)
			{
				centering = true;
				centeringTime = 0;
			}
		}
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
				m_MouseStick.UpdateAngleMagnitude();
			}
			else
			{
				// Camera move
				lateralMove = true;
				m_AxisX = Clamp(m_AxisX - ((double)m_mouseDeltaX * lookSensitivity), -1, 1);
				m_AxisY = Clamp(m_AxisY + ((double)m_mouseDeltaY * lookSensitivity), -1, 1);
			}

			const long axisDeadzone = 15;
			m_deltaX += m_mouseDeltaX;
			m_deltaY += m_mouseDeltaY;

			if (abs(m_deltaY) > axisDeadzone || abs(m_deltaX) > axisDeadzone)
			{
				viewChanged = true;
				m_Mode = MODE_DEFAULT;
			}
		}
	}

	if (!lateralMove)
	{
		m_AxisX = MoveTo(m_AxisX, 0, time.deltaTime * (SLIDER_FOLLOW_SPEED));
		m_AxisY = MoveTo(m_AxisY, 0, time.deltaTime * (SLIDER_FOLLOW_SPEED));
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		// Wheel brake
		if (BTNDOWN(brakeBtn))
		{
			m_WheelBrakeAxis.MoveTowardNextValue();
		}

		if (m_axisMode == AM_NONE)
		{
			const long axisDeadzone = 15;

			m_deltaX += m_mouseDeltaX;
			m_deltaY += m_mouseDeltaY;

			if (abs(m_deltaY) > axisDeadzone)
			{
				m_axisMode = AM_THROTTLE;
			}
			else if (abs(m_deltaX) > axisDeadzone)
			{
				m_axisMode = AM_ZOOM;
			}
		}

		if (m_axisMode == AM_THROTTLE)
		{
			// Throttle
			const double throttleSensitivity = 0.001;
			m_ThrottleAxis = Clamp(m_ThrottleAxis + ((double)m_mouseDeltaY * -throttleSensitivity), 0, 1);
			m_Slider = ApplyDeadzoneRegion(m_ThrottleAxis, m_ABDetent, 0.15);
		}
		else if (m_axisMode == AM_ZOOM)
		{
			// Zoom
			const double zoomSensitivity = 0.001;
			m_ZoomAxis = Clamp(m_ZoomAxis + ((double)m_mouseDeltaX * -zoomSensitivity), 0, 1);
			m_Dial = m_ZoomAxis;
		}
	}
	else
	{
		//m_ZoomAxis = MoveTo(m_ZoomAxis, 0.5, time.deltaTime * (SLIDER_FOLLOW_SPEED) * 0.5);
		//m_Dial = m_ZoomAxis;

		m_Dial = MoveTo(m_Dial, m_ZoomAxis, time.deltaTime * (SLIDER_FOLLOW_SPEED));
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

			m_MouseStick.UpdateAngleMagnitude();
		}

		m_AxisRX = m_MouseStick.X;//Lerp(m_AxisRX, m_MouseStick.X, 0.5);
		m_AxisRY = m_MouseStick.Y;//Lerp(m_AxisRY, m_MouseStick.Y, 0.5);
	}

	//m_ButtonAxis.Update(time);
	m_WheelBrakeAxis.Update(time);

	// Stick
	//m_AxisX = m_LStick.X;
	//m_AxisY = m_LStick.Y;

	// Rudder
	m_AxisZ = m_PhysAxisZ;
}

void MouseThrottleMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		if (IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_LEFT_MOD) | FLAG(MODE_RIGHT_MOD)))
		{
#if USE_KG12
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_TRIGGER));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_TOP));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_PINKIE));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_HAT_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_HAT_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_HAT_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KG12_HAT_LEFT));
#else
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIGGER_1));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIGGER_2));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_WPN_REL));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_R_SIDE));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_PINKIE));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TMS_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TMS_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TMS_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TMS_LEFT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TMS_CENTER));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIM_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIM_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIM_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIM_LEFT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_TRIM_CENTER));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_DMS_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_DMS_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_DMS_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_DMS_LEFT));
			SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_DMS_CENTER));
			//SetLogicalButton(ctr++, enabled && BtnDown(GF_KOS_THUMB));
#endif
		}
	}

	const unsigned long timeBtn = GF_KOS_THUMB;
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

	SetLogicalButton(ctr++, m_ButtonHoldTime[GetShiftAmount(timeBtn)] > 0);
	SetLogicalButton(ctr++, time.time - lastPulseTime < BUTTON_HOLD_TIME);
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