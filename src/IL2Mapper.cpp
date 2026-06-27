#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

IL2Mapper::IL2Mapper()
{
	m_deviceID = GF_DEVICE_ID;

	m_MouseStick = { 0 };

	m_ABDetent = 1.0;
	m_FovMin = 30;
	m_FovMax = 105;
	m_AfterburnerDetent = &m_ABDetent;
	m_ZoomAxis = FovToAxis(m_FovDefault);
	m_ThrottleAxis = 0.0;
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

	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	m_ScreenWidth = desktop.right;
	m_ScreenHeight = desktop.bottom;

	m_MouseLocked = 0;
	m_SavedMouseX = 0;
	m_SavedMouseY = 0;

	m_Filter = new EWMAFilter(0.03, 0.06, 0.97);
}

const JoyMapper::Mode g_viewMode = JoyMapper::Mode::MODE_LEFT_MOD;
const JoyMapper::Mode g_throttleMode = JoyMapper::Mode::MODE_RIGHT_MOD;

void IL2Mapper::UpdateInternal(const STime& time)
{
	const bool useMouseLook = true;
	const int throttleBtn = 1;
	const int viewBtn = 0;

	m_Mode = MODE_DEFAULT;
	if (MOUSEDOWN(viewBtn))
	{
		m_Mode = g_viewMode;
	}
	else if (MOUSEDOWN(throttleBtn))
	{
		m_Mode = g_throttleMode;
	}

	static double throttleModTime = time.time;
	static double viewModTime = time.time;
	static double lastLookToggleTime = time.time;

	static POINT point;

	static bool centering = false;
	static double centeringTime = 0;
	auto Recenter = []() { centering = true; centeringTime = 0; };

	static bool mouseLook = false;
	static bool manualZoom = false;

	if (m_ButtonAxis.values[0] != FovToAxis(m_FovDefault))
		m_ButtonAxis.SetValues(2, FovToAxis(m_FovDefault), FovToAxis(45));

	m_ThrottleDisplay = AXD_HIDE;

	if (MOUSEPRESSED(throttleBtn))
	{
		throttleModTime = time.time;
		m_deltaX = 0;
		m_deltaY = 0;

		LockMouse(true);
	}
	if (MOUSERELEASED(throttleBtn))
	{
		m_axisMode = AM_NONE;

		LockMouse(false);

		if (time.time - throttleModTime < TEMPO_TIME)
		{
			// Zoom
			if (manualZoom)
				manualZoom = false;
			else
				m_ButtonAxis.CycleValue();
		}
	}

	if (MOUSEPRESSED(viewBtn))
	{
		viewModTime = time.time;
		m_deltaX = 0;
		m_deltaY = 0;

		LockMouse(true);
	}
	if (MOUSERELEASED(viewBtn))
	{
		LockMouse(false);

		if (time.time - viewModTime < TEMPO_TIME)
		{
			if (useMouseLook)
			{
				Recenter();
			}			
		}
	}

	if (m_MouseLocked)
		SetCursorPositionScreenSpace(1, 0);

	if (useMouseLook)
		mouseLook = true;// MOUSEDOWN(viewBtn);

	bool lateralMove = false;
	if (useMouseLook && (m_Mode == MODE_DEFAULT || m_Mode == g_viewMode))
	{
		if (mouseLook)
		{
			const double lookSensitivity = 0.0015;
			if (!MOUSEDOWN(viewBtn))
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
				m_HeadX = Clamp(m_HeadX + ((double)m_mouseDeltaX * lookSensitivity), -1, 1);
				m_HeadY = Clamp(m_HeadY - ((double)m_mouseDeltaY * lookSensitivity), -1, 1);
			}
		}
	}

	if (!lateralMove)
	{
		// Recenter horizontal
		//if (fabs(m_HeadX) < 0.08)
		m_HeadX = Lerp(m_HeadX, 0, time.deltaTime * 10);
		// Recenter vertical
		//if (fabs(m_HeadY) < 0.08)
		m_HeadY = Lerp(m_HeadY, 0, time.deltaTime * 10);
	}

	if (m_Mode == g_throttleMode)
	{
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
		if (!manualZoom)
			m_ButtonAxis.Update(time);

		m_Dial = m_ZoomAxis;
	}

	if (useMouseLook)
	{
		//if (!centering && !mouseLook)
		//{
		//	// Auto center near the middle
		//	double deadZone = 0.1 * (AxisToFov(m_ZoomAxis) / 90);
		//	double sqMag = m_MouseStick.X * m_MouseStick.X + m_MouseStick.Y * m_MouseStick.Y * 0.25;
		//	if (sqMag < deadZone * deadZone)
		//		Recenter();
		//}

		if (centering)
		{
			centeringTime += time.deltaTime;
			double t = min(centeringTime / VIEW_CENTERING_TIME, 1.0);
			m_MouseStick.X = Lerp(m_MouseStick.X, 0, t);
			m_MouseStick.Y = Lerp(m_MouseStick.Y, 0, t);

			if (fabs(m_MouseStick.X) < 0.001 && fabs(m_MouseStick.Y) < 0.001)
			{
				m_MouseStick.X = 0;
				m_MouseStick.Y = 0;
				centering = false;
			}
		}

		m_HeadRX = m_MouseStick.X;
		m_HeadRY = m_MouseStick.Y;

		//m_HeadZ = MoveTo(m_HeadZ, MOUSEDOWN(viewBtn) ? 1 : 0, time.deltaTime * 5);
	}

	// Throttle
	m_Slider = fmax(m_ThrottleAxis, m_PhysDial);

	// Wheel brake
	m_AxisRZ = m_PhysSlider;

	//m_VirtualPOV[0].SetHatButtons(m_Mode == MODE_DEFAULT || m_Mode == g_throttleMode, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));
	//m_VirtualPOV[1].SetHatButtons(m_Mode == g_viewMode, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));

	//for (int i = 0; i < MAX_VIRTUAL_POVS; ++i)
	//	m_VirtualPOV[i].SetHatButtons(m_Mode == i, BTNDOWN(GF_TRIM_UP), BTNDOWN(GF_TRIM_DOWN), BTNDOWN(GF_TRIM_LEFT), BTNDOWN(GF_TRIM_RIGHT));
}

void IL2Mapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	AddButton add(this, ctr);
	unsigned char modeMask = FLAG(MODE_DEFAULT) | FLAG(g_viewMode);

	for (int i = 0; i < MODE_NUM - 2; ++i)
	{
		add.Layer(i);
		//add(modeMask, GF_TRIGGER_1);
		//add(modeMask, GF_TRIGGER_2);
		//add(modeMask, GF_WPN_REL);
		//add(modeMask, GF_R_SIDE);
		//add(modeMask, GF_PINKIE);
		//add(modeMask, GF_TRIM_CENTER);
		//add(modeMask, GF_DMS_UP);
		//add(modeMask, GF_DMS_RIGHT);
		//add(modeMask, GF_DMS_DOWN);
		//add(modeMask, GF_DMS_LEFT);
		//add(modeMask, GF_DMS_CENTER);
		//add(modeMask, GF_TMS_UP);
		//add(modeMask, GF_TMS_RIGHT);
		//add(modeMask, GF_TMS_DOWN);
		//add(modeMask, GF_TMS_LEFT);
		//add(modeMask, GF_TMS_CENTER);
		//add(0xff, GF_EX_1);
		//add(modeMask, GF_EX_2);
		//add(modeMask, GF_EX_3);
	}
}

void IL2Mapper::UpdateHeadAxes(double* outAxes)
{
	double yaw = m_HeadRX * M_PI;
	double sinYaw = sin(yaw);
	double cosYaw = cos(yaw);

	double threshold = 0.75;
	double offsetX = fabs(m_HeadRX) > threshold ? fabs(m_HeadRX) - threshold : 0;
	offsetX /= 1 - threshold;
	offsetX = m_HeadRX > 0 ? -offsetX : offsetX;
	double headX = m_HeadX + offsetX;

	headX = headX * cosYaw + m_HeadZ * sinYaw;
	double headY = m_HeadY;
	double headZ = m_HeadZ * cosYaw - m_HeadX * sinYaw;

	outAxes[AXIS_X] = -headX * 22;
	outAxes[AXIS_Y] = headY * 20;
	outAxes[AXIS_Z] = -headZ * 15;
	outAxes[AXIS_RX] = m_HeadRX * 180;
	outAxes[AXIS_RY] = m_HeadRY * 135;
	outAxes[AXIS_RZ] = m_HeadRZ * 180;
}

void IL2Mapper::LockMouse(bool locked)
{
	return;

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