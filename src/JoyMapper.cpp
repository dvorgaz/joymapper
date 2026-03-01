#include "JoyMapper.h"

#include "stdafx.h"
#include "Math.h"
#include <Windows.h>

void JoyMapper::Stick::SetAxes(long x, long y)
{
	X = (double)x / 32767.0;
	Y = (double)y / 32767.0;

	UpdateAngleMagnitude();
}

void JoyMapper::Stick::UpdateAngleMagnitude()
{
	Angle = atan2(X, Y) / M_PI;
	Magnitude = sqrt(X * X + Y * Y);
}

void JoyMapper::StickSlider::SetRegion(unsigned int regionIdx, double from, double to, double* output, bool relative, bool invert)
{
	if (regionIdx >= MAX_SLIDER_REGIONS)
		return;

	regions[regionIdx].from = from;
	regions[regionIdx].to = to;
	regions[regionIdx].output = output;
	regions[regionIdx].relative = relative;
	regions[regionIdx].invert = invert;
}

void JoyMapper::StickSlider::SetDetent(unsigned int regionIdx, double low, double high, double highDz)
{
	if (regionIdx >= MAX_SLIDER_REGIONS)
		return;

	regions[regionIdx].detentHigh = high;
	regions[regionIdx].detentLow = low;
	regions[regionIdx].detentHigh_Dz = highDz;
}

void JoyMapper::StickSlider::Update(const STime& time)
{
	invalid |= time.time - lastUpdateTime > SLIDER_INVALID_TIME;

	if (stick->Magnitude > STICK_SLIDER_DEADZONE)
	{
		if (!invalid)
		{
			double angle = stick->Angle;

			if (activeRegionIdx < 0)
			{
				StickSliderRegion* region = NULL;
				for (int i = 0; i < MAX_SLIDER_REGIONS; ++i)
				{
					region = &regions[i];
					if (region->output == NULL)
						continue;

					if (region->from < angle && angle < region->to)
					{
						activeRegionIdx = i;
						break;
					}
				}
			}

			if (activeRegionIdx >= 0)
			{
				StickSliderRegion& region = regions[activeRegionIdx];
				bool useDz = region.detentHigh_Dz > 0.01;

				const double prevVal = region.internalValue;// *region.output;
				const double maxVal = (detentUnlocked || useDz) ? 1 : (region.detentHigh < 0.1 ? 1 : region.detentHigh);
				const double minVal = detentUnlocked ? 0 : region.detentLow;

				if (region.relative)
				{
					double deltaAngle = angle - lastAngle;

					// Handle wrap-around
					if (fabs(deltaAngle) > 1.8)
					{
						deltaAngle += deltaAngle < 0 ? 2 : -2;
					}

					if (region.invert)
						deltaAngle = -deltaAngle;

					double newVal = prevVal + deltaAngle * SLIDER_RELATIVE_SPEED;

					if (vibrFunc != NULL && (prevVal < maxVal && newVal > maxVal))
						vibrFunc(65535, 65535, VIBRATION_TIME);

					if (vibrFunc != NULL && (prevVal < 0.5 != newVal < 0.5))
						vibrFunc(0, 2000, VIBRATION_TIME);

					if (vibrFunc != NULL && (prevVal > minVal && newVal < minVal))
						vibrFunc(65535, 65535, VIBRATION_TIME);

					region.internalValue = Clamp(newVal, minVal, maxVal);
					*region.output = ApplyDeadzoneRegion(region.internalValue, region.detentHigh, region.detentHigh_Dz);
				}
				else
				{
					// Clamp angle into range
					if (angle < region.from || region.to < angle)
					{
						angle = (region.invert ? 1.0 - prevVal : prevVal) < 0.5 ? region.from : region.to;
					}

					double outVal = (angle - region.from) / (region.to - region.from);

					if(region.invert)
						outVal = 1.0 - outVal;

					if (vibrFunc != NULL && (prevVal < region.detentHigh != outVal < region.detentHigh))
						vibrFunc(65535, 65535, VIBRATION_TIME);

					outVal = MoveTo(prevVal, outVal, time.deltaTime * SLIDER_FOLLOW_SPEED);
					region.internalValue = Clamp(outVal, minVal, maxVal);
					*region.output = ApplyDeadzoneRegion(region.internalValue, region.detentHigh, region.detentHigh_Dz);
				}				
			}			
		}
	}
	else
	{
		activeRegionIdx = -1;
		detentUnlocked = false;
		invalid = false;
	}

	lastAngle = stick->Angle;
	lastUpdateTime = time.time;
}

double* JoyMapper::StickSlider::GetDetentPtr(unsigned int regionIdx, bool high)
{
	if (regionIdx >= MAX_SLIDER_REGIONS)
		return NULL;

	StickSliderRegion& region = regions[regionIdx];

	return high ? &region.detentHigh : &region.detentLow;
}

void JoyMapper::StickView::SetFovSettings(double* axis, double from, double to)
{
	fovAxis = axis;
	fovFrom = from;
	fovTo = to;
}

void JoyMapper::StickView::Update(bool enableInput, const STime& time)
{
	invalid |= time.time - lastUpdateTime > SLIDER_INVALID_TIME;	

	if (!invalid && enableInput && absolute)
	{
		const double dz = STICK_VIEW_DEADZONE * 2;
		double mul = ((stick->Magnitude - dz) / (1.0 - dz)) / stick->Magnitude;
		double x = stick->Magnitude < dz ? 0 : stick->X * mul;
		double y = stick->Magnitude < dz ? 0 : stick->Y * mul;

		x = Clamp(x + absOffsetX, -1, 1);
		y = Clamp(y + absOffsetY, -1, 1);

		if (stick->Magnitude > 0.99)
		{
			double max = max(fabs(stick->X), fabs(stick->Y));
			x /= max;
			y /= max;
		}

		*outputX = Clamp(Lerp(*outputX, x, time.deltaTime * ABSOLUTE_VIEW_SPEED), -1, 1);
		*outputY = Clamp(Lerp(*outputY, y, time.deltaTime * ABSOLUTE_VIEW_SPEED), -1, 1);
	}
	else if (centering)
	{
		centeringTime += time.deltaTime;
		double t = min(centeringTime / VIEW_CENTERING_TIME, 1.0);
		*outputX = Lerp(*outputX, 0, t);
		*outputY = Lerp(*outputY, 0, t);

		if (fabs(*outputX) < 0.001 && fabs(*outputY) < 0.001)
		{
			*outputX = 0;
			*outputY = 0;
			centering = false;
		}
	}
	else if (!invalid && enableInput && stick->Magnitude > STICK_VIEW_DEADZONE)
	{		
		double speed = (stick->Magnitude - STICK_VIEW_DEADZONE) / (1.0 - STICK_VIEW_DEADZONE);
		speed = PowerCurve(speed, STICK_VIEW_CURVE) / stick->Magnitude;

		if (fovAxis)
		{
			speed *= (*fovAxis - fovFrom)/(fovTo - fovFrom);
		}

		*outputX = Clamp(*outputX + stick->X * speed * time.deltaTime * STICK_VIEW_SPEED, -1, 1);
		*outputY = Clamp(*outputY + stick->Y * speed * time.deltaTime * STICK_VIEW_SPEED, -1, 1);
	}

	if (enableInput)
	{
		if (stick->Magnitude < 0.2)
			invalid = false;

		lastUpdateTime = time.time;
	}
}

void JoyMapper::StickView::Recenter()
{
	centering = true;
	centeringTime = 0;
}

void JoyMapper::StickView::SetOffset()
{
	if (absolute)
	{
		if(stick->Magnitude < 0.1)
		{
			absOffsetX = 0;
			absOffsetY = 0;
		}
		else
		{
			absOffsetX = *outputX;
			absOffsetY = *outputY;
		}

		invalid = true;
	}
}

void JoyMapper::RadialButtons::Update(const STime& time)
{
	invalid |= time.time - lastUpdateTime > SLIDER_INVALID_TIME;

	int numRegions = numButtons * (overlappedButtons ? 2 : 1);
	if (stick->Magnitude > (deadzoneOverride > 0.01 ? deadzoneOverride : STICK_SLIDER_DEADZONE))
	{
		if (!invalid)
		{
			double angle = stick->Angle + (2.0 / numRegions) / 2.0;

			if (angle > 1.0)
				angle -= 2.0;

			activeButtonIdx = floor((angle / 2.0 + 0.5) * numRegions);
		}
	}
	else
	{	
		activeButtonIdx = -1;
		invalid = false;
	}

	if (!invalid && showCursor && stick->Magnitude > STICK_VIEW_DEADZONE)
		JoyMapper::SetCursorPosition(stick->X, stick->Y);

	lastUpdateTime = time.time;
}

void JoyMapper::RadialButtons::AddLogicalButtons(std::function<void(bool)> addFunc)
{
	for (int i = 0; i < numButtons; ++i)
	{
		if (overlappedButtons)
		{
			bool res = (i * 2) == activeButtonIdx;
			res |= (i * 2) == activeButtonIdx - 1;
			res |= (i * 2) == (activeButtonIdx + 1) % (numButtons * 2);
			
			addFunc(res && activeButtonIdx != -1);
		}
		else
		{
			addFunc(activeButtonIdx == i);
		}
	}

	activeButtonIdx = -1;
}

void JoyMapper::SpecialButton::SetTempo(unsigned long button, unsigned char mask)
{
	type = SBT_TEMPO;
	physicalButton = button;
	modeMask = mask;
}

void JoyMapper::SpecialButton::SetAxis2Btn(double* axis, double threshold, unsigned char mask)
{
	type = SBT_AXIS2BTN;
	sourceAxis = axis;
	sourceThreshold = threshold;
	modeMask = mask;
}

JoyMapper::ButtonAxis& JoyMapper::ButtonAxis::AddValue(double value)
{
	if (numValues < MAX_BUTTON_ON_AXIS)
	{
		values[numValues++] = value;
	}

	return *this;
}

void JoyMapper::ButtonAxis::Update(const STime& time)
{
	int targetIdx = valueIdx;

	if (moveToNext)
	{
		targetIdx = ++targetIdx % numValues;
		moveToNext = false;
	}

	*output = MoveTo(*output, values[targetIdx], time.deltaTime * (SLIDER_FOLLOW_SPEED + speedModifier));
}

void JoyMapper::ButtonAxis::CycleValue(bool reverse)
{
	valueIdx = (valueIdx + numValues + (reverse ? -1 : 1)) % numValues;
}

void JoyMapper::ButtonAxis::MoveTowardNextValue()
{
	moveToNext = true;
}

void JoyMapper::ButtonThrottle::Update(const STime& time)
{
	double thrDelta = 0;
	if (dir > 0)
		thrDelta += speed;

	if (dir < 0)
		thrDelta -= speed;

	static bool disableDetent = false;

	if (dir > 0 && pressed && fabs(*output - *abDetent) < 0.01)
		disableDetent = true;

	if (*output < *abDetent)
		disableDetent = false;

	double maxThr = disableDetent ? 1 : *abDetent;
	*output = Clamp(*output + thrDelta * time.deltaTime, 0, maxThr);
}

JoyMapper::JoyMapper()
{
	m_Mode = MODE_DEFAULT;
	m_IsMenuMode = false;
	m_JoyButtons = 0;
	m_JoyButtonsPrev = 0;
	m_JoyButtonsProcessed = 0;
	m_LStick = { 0 };
	m_RStick = { 0 };
	m_LTrigger = 0;
	m_RTrigger = 0;

	m_MenuActivateBtn1 = JOYPAD_BACK;
	m_MenuActivateBtn2 = JOYPAD_START;
	m_MenuAcceptBtn = JOYPAD_A;
	m_MenuCancelBtn = JOYPAD_B;
	m_MenuUpBtn = JOYPAD_DPAD_UP;
	m_MenuRightBtn = JOYPAD_DPAD_RIGHT;
	m_MenuDownBtn = JOYPAD_DPAD_DOWN;
	m_MenuLeftBtn = JOYPAD_DPAD_LEFT;

	m_PhysAxisX = 0;
	m_PhysAxisY = 0;
	m_PhysAxisZ = 0;
	m_PhysAxisRX = 0;
	m_PhysAxisRY = 0;
	m_PhysAxisRZ = 0;
	m_PhysDial = 0;
	m_PhysSlider = 0;

	memset(m_VirtualPOV, 0, sizeof(RadialButtons) * MAX_VIRTUAL_POVS);
	for (int i = 0; i < MAX_VIRTUAL_POVS; ++i)
	{
		m_VirtualPOV[i].numButtons = 8;
		m_VirtualPOV[i].activeButtonIdx = -1;
		m_VirtualPOV[i].deadzoneOverride = VIRTUAL_POV_DEADZONE;
	}

	memset(m_ButtonPressedTime, 0, sizeof(double) * NUM_PHYSICAL_BUTTONS);
	memset(m_ButtonHoldTime, 0, sizeof(double) * NUM_PHYSICAL_BUTTONS);
	memset(m_MouseButtons, 0, sizeof(unsigned short) * NUM_MOUSE_BUTTONS);
	memset(m_SpecialButtons, 0, sizeof(SpecialButton) * MAX_SPECIAL_BUTTONS);

	m_AfterburnerDetent = NULL;
	m_MenuIdx = 0;

	m_AxisX = 0;
	m_AxisY = 0;
	m_AxisZ = 0;
	m_AxisRX = 0;
	m_AxisRY = 0;
	m_AxisRZ = 0;
	m_Dial = 0;
	m_Slider = 0;

	m_Ex_AxisX = 0;
	m_Ex_AxisY = 0;
	m_Ex_AxisRX = 0;
	m_Ex_AxisRY = 0;
	m_Ex_AxisRZ = 0;

	memset(m_LogicalButtons, 0, sizeof(long) * NUM_BUTTON_EXTENSIONS);

	m_ScrollDirection = 0;
	m_ScrollDirty = false;

	m_VibrationLeft = 0;
	m_VibrationRight = 0;
	m_VibrationTime = 0;	

	memset(m_Keys, 0, sizeof(bool) * MAX_KEYS);
	memset(m_KeysPrev, 0, sizeof(bool) * MAX_KEYS);

	m_mouseDeltaX = 0;
	m_mouseDeltaY = 0;
	m_mouseExBtn1 = false;
	m_mouseExBtn2 = false;
	m_mouseExBtn1_prev = false;
	m_mouseExBtn2_prev = false;
}

void JoyMapper::Init()
{
	BuildMenu();
}

long JoyMapper::AxisToLong(double value)
{
	return (long)(((value + 1.0) / 2) * 32767);
}

long JoyMapper::HalfAxisToLong(double value)
{
	return (long)(value * 32767);
}

void JoyMapper::Update(const STime& time)
{
	static double lastMouseTime = time.time;
	if (m_ScrollDirty || time.time - lastMouseTime > SCROLL_INTERVAL)
	{
		if (m_ScrollDirection != 0)
		{
			INPUT Input = { 0 };
			Input.type = INPUT_MOUSE;
			Input.mi.mouseData = 120 * m_ScrollDirection;
			Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
			SendInput(1, &Input, sizeof(INPUT));
		}

		m_ScrollDirty = false;
		lastMouseTime = time.time;
	}

	for (int i = 0; i < NUM_PHYSICAL_BUTTONS; ++i)
	{
		if (BTNPRESSED(1 << i))
			m_ButtonPressedTime[i] = time.time;
	}

	if (m_VibrationTime > 0)
	{
		m_VibrationTime -= time.deltaTime;
	}
	else
	{
		m_VibrationLeft = 0;
		m_VibrationRight = 0;
	}

	if (!m_IsMenuMode && BTNDOWN(m_MenuActivateBtn1) && BTNDOWN(m_MenuActivateBtn2))
	{
		const bool holdTime = 1.0;
		bool backHeld = time.time - BTNTIME(m_MenuActivateBtn1) > holdTime;
		bool startHeld = time.time - BTNTIME(m_MenuActivateBtn2) > holdTime;

		if (backHeld && startHeld)
			m_IsMenuMode = true;
	}

	if (m_IsMenuMode)
	{		
		UpdateMenu(time);
	}
	else
	{
		UpdateInternal(time);
		UpdateLogicalButtons(time);
	}

	memcpy(m_KeysPrev, m_Keys, sizeof(bool) * MAX_KEYS);
}

void JoyMapper::SetButtons(unsigned long buttons)
{
	m_JoyButtonsPrev = m_JoyButtons;
	m_JoyButtons = buttons;
}

void JoyMapper::SetButtonsPov(unsigned long buttons, unsigned long hat)
{
	buttons = buttons << 4;

	switch (hat)
	{
		// Up
	case 7:
	case 0:
	case 1:
		buttons |= 1 << 0;
		break;
		// Down
	case 3:
	case 4:
	case 5:
		buttons |= 1 << 1;
		break;
	}

	switch (hat)
	{
		// Left
	case 5:
	case 6:
	case 7:
		buttons |= 1 << 2;
		break;
		// Right
	case 1:
	case 2:
	case 3:
		buttons |= 1 << 3;
		break;
	}

	SetButtons(buttons);
}

void JoyMapper::SetAxesXInput(long x, long y, long z, long rx, long ry, long rz)
{
	m_LStick.SetAxes(x, y);
	m_RStick.SetAxes(rx, ry);

	m_LTrigger = (double)z / 255.0;
	m_RTrigger = (double)rz / 255.0;
}

void JoyMapper::SetAxesHID(double x, double y, double z, double rx, double ry, double rz, double sl, double di)
{
	m_LStick.X = x;
	m_LStick.Y = y;
	m_LStick.UpdateAngleMagnitude();

	m_PhysAxisX = x;
	m_PhysAxisY = y;
	m_PhysAxisZ = z;
	m_PhysAxisRX = rx;
	m_PhysAxisRY = ry;
	m_PhysAxisRZ = rz;
	m_PhysSlider = sl;
	m_PhysDial = di;	
}

void JoyMapper::SetKey(unsigned long keyCode, bool down)
{
	if (keyCode < MAX_KEYS)
		m_Keys[keyCode] = down;
}

void JoyMapper::SetMouse(long dX, long dY, bool btn1, bool btn2)
{
	m_mouseDeltaX = dX;
	m_mouseDeltaY = dY;

	m_mouseExBtn1_prev = m_mouseExBtn1;
	m_mouseExBtn1 = btn1;

	m_mouseExBtn2_prev = m_mouseExBtn2;
	m_mouseExBtn2 = btn2;
}

long JoyMapper::GetMappedButtons(unsigned int index)
{
	if (index >= NUM_BUTTON_EXTENSIONS)
		return 0;

	return m_LogicalButtons[index];
}

unsigned long JoyMapper::GetMappedPov(unsigned int index)
{
	if (index == 0)
	{
		if (m_Mode != MODE_DEFAULT)
			return -1;

		static unsigned long mapping[3][3] = { {31500,0,4500}, {27000,-1,9000}, {22500,18000,13500} };

		int x = 1;
		int y = 1;

		if (m_JoyButtonsProcessed & JOYPAD_DPAD_UP)		y -= 1;
		if (m_JoyButtonsProcessed & JOYPAD_DPAD_RIGHT)	x += 1;
		if (m_JoyButtonsProcessed & JOYPAD_DPAD_DOWN)	y += 1;
		if (m_JoyButtonsProcessed & JOYPAD_DPAD_LEFT)	x -= 1;

		return mapping[y][x];
	}
	else if (0 < index && index <= MAX_VIRTUAL_POVS)
	{
		static unsigned long mapping[8] = { 18000, 22500, 27000, 31500, 0, 4500, 9000, 13500 };
		int idx = m_VirtualPOV[index - 1].activeButtonIdx;

		if (idx >= 0)
			return mapping[idx];
	}

	return -1;
}

long JoyMapper::GetMappedAxis(unsigned int index, AxisID axis)
{
	if (index == 0)
	{
		switch (axis)
		{
		case JoyMapper::AXIS_X:			return AxisToLong(m_AxisX);
		case JoyMapper::AXIS_Y:			return AxisToLong(-m_AxisY);
		case JoyMapper::AXIS_Z:			return AxisToLong(m_AxisZ);
		case JoyMapper::AXIS_RX:		return AxisToLong(m_AxisRX);
		case JoyMapper::AXIS_RY:		return AxisToLong(-m_AxisRY);
		case JoyMapper::AXIS_RZ:		return HalfAxisToLong(m_AxisRZ);
		case JoyMapper::AXIS_SLIDER:	return HalfAxisToLong(m_Slider);
		case JoyMapper::AXIS_DIAL:		return HalfAxisToLong(m_Dial);		
		}
	}
	else if (index == 1)
	{
		switch (axis)
		{
		case JoyMapper::AXIS_X:			return AxisToLong(m_Ex_AxisX);
		case JoyMapper::AXIS_Y:			return AxisToLong(-m_Ex_AxisY);
		//case JoyMapper::AXIS_Z:			return AxisToLong(m_AxisZ);
		case JoyMapper::AXIS_RX:		return AxisToLong(m_Ex_AxisRX);
		case JoyMapper::AXIS_RY:		return AxisToLong(-m_Ex_AxisRY);
		case JoyMapper::AXIS_RZ:		return HalfAxisToLong(m_Ex_AxisRZ);
		//case JoyMapper::AXIS_SLIDER:	return HalfAxisToLong(m_Slider);
		//case JoyMapper::AXIS_DIAL:		return HalfAxisToLong(m_Dial);		
		}
	}

	return 0;
}

void JoyMapper::GetVibration(unsigned short& left, unsigned short& right)
{
	left = m_VibrationLeft;
	right = m_VibrationRight;	
}

double JoyMapper::GetAfterburnerDetent()
{
	if (m_AfterburnerDetent != NULL)
		return *m_AfterburnerDetent;

	return 1.0;
}

bool JoyMapper::GetKey(unsigned long keyCode)
{
	if(keyCode < MAX_KEYS)
		return m_Keys[keyCode];

	return false;
}

bool JoyMapper::GetKeyDown(unsigned long keyCode)
{
	if (keyCode < MAX_KEYS)
		return !m_KeysPrev[keyCode] && m_Keys[keyCode];

	return false;
}

bool JoyMapper::GetKeyUp(unsigned long keyCode)
{
	if (keyCode < MAX_KEYS)
		return m_KeysPrev[keyCode] && !m_Keys[keyCode];

	return false;
}

void JoyMapper::SetLogicalButton(int index, bool on)
{
	if (index >= NUM_BUTTON_EXTENSIONS * 32)
		return;

	int block = index / 32;
	int offset = index % 32;

	if(on)
		m_LogicalButtons[block] |= 1 << offset;
	else
		m_LogicalButtons[block] &= ~(1 << offset);
}

void JoyMapper::UpdateLogicalButtons(const STime& time)
{
	memset(m_LogicalButtons, 0, sizeof(long) * NUM_BUTTON_EXTENSIONS);	
	m_JoyButtonsProcessed = m_JoyButtons;	

	for (int i = 0; i < MAX_SPECIAL_BUTTONS; ++i)
	{
		int result = UpdateSpecialButton(m_SpecialButtons[i], time);

		if (result == -1)
		{
			m_JoyButtonsProcessed &= ~m_SpecialButtons[i].physicalButton;
		}
		else if (result == 1)
		{
			m_JoyButtonsProcessed |= m_SpecialButtons[i].physicalButton;
		}
	}

	for (int i = 0; i < NUM_PHYSICAL_BUTTONS; ++i)
	{
		double& t = m_ButtonHoldTime[i];
		if (t > 0)
		{
			t -= time.deltaTime;
			m_JoyButtonsProcessed |= 1 << i;
		}
		else
		{
			t = 0;
		}
	}

	int ctr = 0;
	UpdateLogicalButtonsInternal(ctr, time);

	const int btOffset = MAX_LOGICAL_BUTTONS - MAX_SPECIAL_BUTTONS;
	if (ctr < btOffset)
		ctr = btOffset;

	for (int i = 0; i < MAX_SPECIAL_BUTTONS; ++i)
	{
		int result = GetSpecialButton(m_SpecialButtons[i], time);
		SetLogicalButton(ctr++, result);
	}
}

// Return value:
// 0: do nothing
// 1: add physical button
// -1:  remove physical button
int JoyMapper::UpdateSpecialButton(SpecialButton& btnSpec, const STime& time)
{
	if (btnSpec.type == SBT_NONE || !(FLAG(m_Mode) & btnSpec.modeMask))
		return 0;

	if (btnSpec.type == SBT_TEMPO)
	{
		if (BTNPRESSED(btnSpec.physicalButton))
			btnSpec.lastPressedTime = time.time;

		if (BTNDOWN(btnSpec.physicalButton))
			return -1;

		if (BTNRELEASED(btnSpec.physicalButton) && (time.time - btnSpec.lastPressedTime < TEMPO_TIME))
		{
			m_ButtonHoldTime[GetShiftAmount(btnSpec.physicalButton)] = BUTTON_HOLD_TIME;
			return 1;
		}
	}

	return 0;
}

int JoyMapper::GetSpecialButton(SpecialButton& btnSpec, const STime& time)
{
	if (btnSpec.type == SBT_NONE || !(FLAG(m_Mode) & btnSpec.modeMask))
		return 0;

	if (btnSpec.type == SBT_TEMPO)
	{
		if (BTNDOWN(btnSpec.physicalButton) && (time.time - btnSpec.lastPressedTime >= TEMPO_TIME))
			return 1;
	}

	if (btnSpec.type == SBT_AXIS2BTN)
	{
		if (*btnSpec.sourceAxis > btnSpec.sourceThreshold)
			return 1;
	}

	return 0;
}

void JoyMapper::UpdateMenu(const STime& time)
{
	if (BTNPRESSED(m_MenuCancelBtn))
		m_IsMenuMode = false;

	if (BTNPRESSED(m_MenuAcceptBtn))
	{
		if (m_MenuIdx < m_MenuOptions.size() && m_MenuOptions[m_MenuIdx].func != NULL)
		{
			m_MenuOptions[m_MenuIdx].func();
			m_IsMenuMode = false;
		}
	}

	if (BTNPRESSED(m_MenuRightBtn) | BTNPRESSED(m_MenuDownBtn))
		m_MenuIdx = (m_MenuIdx + 1) % m_MenuOptions.size();

	if (BTNPRESSED(m_MenuLeftBtn) | BTNPRESSED(m_MenuUpBtn))
		m_MenuIdx = (m_MenuIdx - 1 + m_MenuOptions.size()) % m_MenuOptions.size();
}

void JoyMapper::BuildMenu()
{
	double* abDetent = m_AfterburnerDetent;
	m_MenuOptions.push_back(MenuOption(L"F-16", [abDetent]() { *abDetent = 0.75; }));
	m_MenuOptions.push_back(MenuOption(L"F-14/F-15C", [abDetent]() { *abDetent = 0.8; }));
	m_MenuOptions.push_back(MenuOption(L"F-18", [abDetent]() { *abDetent = 0.74; }));
	m_MenuOptions.push_back(MenuOption(L"F-5", [abDetent]() { *abDetent = 0.81; }));
	m_MenuOptions.push_back(MenuOption(L"Mig-21", [abDetent]() { *abDetent = 0.91; }));
	m_MenuOptions.push_back(MenuOption(L"Mig-29", [abDetent]() { *abDetent = 0.61; }));
	m_MenuOptions.push_back(MenuOption(L"Su-27", [abDetent]() { *abDetent = 0.75; }));
	m_MenuOptions.push_back(MenuOption(L"Mirage F1", [abDetent]() { *abDetent = 0.59; }));
	m_MenuOptions.push_back(MenuOption(L"Mirage 2000", [abDetent]() { *abDetent = 0.89; }));
	m_MenuOptions.push_back(MenuOption(L"AJS37", [abDetent]() { *abDetent = 0.79; }));
	m_MenuOptions.push_back(MenuOption(L"No detent", [abDetent]() { *abDetent = 1.0; }));
}

void JoyMapper::HandleMouse(const Stick& stick, StickView* stickView, const STime* time)
{
	HandleMouse(stick, m_MouseButtons[0], m_MouseButtons[1], m_MouseButtons[2], m_MouseButtons[3], stickView, time);
}

void JoyMapper::HandleMouse(const Stick& stick, unsigned long btnLeft, unsigned long btnRight, unsigned long wheelUp, unsigned long wheelDown, StickView* stickView, const STime* time)
{
	SetCursorPosition(stick.X, stick.Y);

	if (BTNPRESSED(btnLeft))	MouseButton(LMB_DOWN); 
	if (BTNRELEASED(btnLeft))	MouseButton(LMB_UP);
	if (BTNPRESSED(btnRight))	MouseButton(RMB_DOWN);
	if (BTNRELEASED(btnRight))	MouseButton(RMB_UP);

	MouseWheel((BTNDOWN(wheelUp) ? 1 : 0) + (BTNDOWN(wheelDown) ? -1 : 0));

	if (stickView && time && stick.Magnitude > MOUSE_VIEW_THRESHOLD)
	{
		const double scaling = (stick.Magnitude - MOUSE_VIEW_THRESHOLD) / (1.0 - MOUSE_VIEW_THRESHOLD);
		const double speed = PowerCurve(scaling, STICK_VIEW_CURVE) * MOUSE_VIEW_SPEED / stick.Magnitude;
		*stickView->outputX = Clamp(*stickView->outputX + stick.X * time->deltaTime * speed, -1, 1);
		*stickView->outputY = Clamp(*stickView->outputY + stick.Y * time->deltaTime * speed, -1, 1);
	}
}

void JoyMapper::SetCursorPosition(double x, double y)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);

	const int halfWidth = desktop.right / 2;
	const int halfHeight = desktop.bottom / 2;

	SetCursorPos(halfWidth + x * MOUSE_CURSOR_RADIUS * halfHeight, halfHeight - y * MOUSE_CURSOR_RADIUS * halfHeight);
}

void JoyMapper::MouseButton(unsigned char mouseBtn)
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = mouseBtn;
	SendInput(1, &Input, sizeof(INPUT));
}

void JoyMapper::ExtraMouseButton(unsigned int btn, bool keyDown)
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = keyDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
	Input.mi.mouseData = btn;
	SendInput(1, &Input, sizeof(INPUT));
}

void JoyMapper::MouseWheel(int dir)
{
	m_ScrollDirty = m_ScrollDirection != dir;
	m_ScrollDirection = dir;
}

void JoyMapper::Vibrate(unsigned short left, unsigned short right, double duration)
{
	m_VibrationLeft = left;
	m_VibrationRight = right;
	m_VibrationTime = duration;
}

double Clamp(double val, double min, double max)
{
	return min(max(val, min), max);
}

double Magnitude(double x, double y)
{
	return sqrt(x * x + y * y);
}

double Lerp(double val, double target, double t)
{
	return val * (1 - t) + target * t;
}

double MoveTo(double val, double target, double maxDelta)
{
	double delta = target - val;

	if (fabs(delta) < maxDelta)
		return target;

	return val + maxDelta * (delta < 0 ? -1 : 1);
}

double PowerCurve(double val, double power)
{
	return pow(val, power) * (val < 0 ? -1 : 1);
}

double ApplyDeadzoneRegion(double val, double detent, double dz)
{
	if (dz < 0.01 || detent > 0.99)
		return val;

	dz *= 0.5;

	if (detent - dz <= val && val <= detent + dz)
		return detent;

	if (val < detent - dz)
	{
		return detent * val / (detent - dz);
	}
	else
	{
		return (1 - detent) * (val - detent - dz) / (1 - detent - dz) + detent;
	}
}

int GetShiftAmount(unsigned long flag)
{
	if (flag > 0)
	{
		for (int i = 0;; ++i)
		{
			if (flag >> (i + 1) == 0)
				return i;
		}
	}

	return -1;
}

JoyMapper::MenuOption::MenuOption()
{
	label = L"undefined";
	func = NULL;
}

JoyMapper::MenuOption::MenuOption(std::wstring label, std::function<void()> func)
{
	this->label = label;
	this->func = func;
}
