#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

#define USE_PEDALS 0

AlternateMapper::AlternateMapper()
{
	m_StickSlider = { 0 };
	m_StickSlider.stick = &m_LStick;
	m_StickSlider.SetRegion(0, -0.25, 1.0, &m_Slider, true, true);
	m_StickSlider.SetDetent(0, 0, 0.8, 0.2);
	//m_StickSlider.vibrFunc = [this](unsigned short l, unsigned short r, double dur) { Vibrate(l, r, dur); };
	m_AfterburnerDetent = m_StickSlider.GetDetentPtr(0);

	m_SpecialButtons[0].SetTempo(JOYPAD_SHARE, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_LEFT_THUMB, FLAG(MODE_DEFAULT));
	m_SpecialButtons[3].SetTempo(JOYPAD_LEFT_SHOULDER, 0xFF);
	m_SpecialButtons[4].SetTempo(JOYPAD_RIGHT_SHOULDER, 0xFF);

#if USE_PEDALS
	m_SpecialButtons[7].SetAxis2Btn(&m_LTrigger, 0.9, 0xFF);
	m_SpecialButtons[8].SetAxis2Btn(&m_RTrigger, 0.1, 0xFF);
	m_SpecialButtons[9].SetAxis2Btn(&m_RTrigger, 0.9, 0xFF);
#endif

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.65).AddValue(0.15);

	m_MouseButtons[0] = JOYPAD_DPAD_LEFT;
	m_MouseButtons[1] = JOYPAD_DPAD_RIGHT;
	m_MouseButtons[2] = JOYPAD_DPAD_UP;
	m_MouseButtons[3] = JOYPAD_DPAD_DOWN;

	m_VirtualPOV[0].stick = &m_LStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	m_VirtualPOV[0].showCursor = true;

	m_VirtualPOV[1].stick = &m_LStick;
}

void AlternateMapper::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_RIGHT_THUMB;

	// Modifiers
	if (BTNDOWN(mouseBtn))
	{
		m_Mode = MODE_MOUSE;
	}
	else
	{
		bool lMod = BTNDOWN(JOYPAD_LEFT_SHOULDER);
		bool rMod = BTNDOWN(JOYPAD_RIGHT_SHOULDER);

		m_Mode = lMod && rMod ? MODE_BOTH_MOD : (lMod ? MODE_LEFT_MOD : (rMod ? MODE_RIGHT_MOD : MODE_DEFAULT));
	}

	if (m_Mode != MODE_MOUSE)
	{
		// Roll and pitch
		m_AxisX = m_RStick.X;
		m_AxisY = m_RStick.Y;
	}

	m_ButtonAxis.Update(time);

	if (m_Mode == MODE_DEFAULT)
	{
		// Throttle and zoom
		if (BTNDOWN(JOYPAD_LEFT_THUMB))
			m_StickSlider.detentUnlocked = true;

		m_StickSlider.Update(time);

		if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
			m_ButtonAxis.CycleValue();
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		m_VirtualPOV[0].Update(time);
	}

	if (m_Mode == MODE_RIGHT_MOD)
	{
		m_AxisRX = m_LStick.X;
		m_AxisRY = m_LStick.Y;
	}

	if (m_Mode == MODE_BOTH_MOD)
	{
		m_VirtualPOV[1].Update(time);
	}

	if (m_Mode == MODE_MOUSE && time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
	{
		HandleMouse(m_RStick);
	}

#if !USE_PEDALS
	// Rudder
	m_AxisZ = m_RTrigger - m_LTrigger;
#endif

	// Wheel brake
	double threshold = 0.1;
	m_AxisRZ = (m_RTrigger > threshold && m_LTrigger > threshold) ? (m_RTrigger + m_LTrigger) / 2 : 0;
}

void AlternateMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_A));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_B));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_X));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_Y));

		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, BtnDown(JOYPAD_LEFT_SHOULDER));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_RIGHT_SHOULDER));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_BACK));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_START));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_GUIDE));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_SHARE));
		}

		if (!IsMode(FLAG(MODE_MOUSE)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_LEFT_THUMB));

		//if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_MOUSE)))
		//	SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_RIGHT_THUMB));

		if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_MOUSE)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_LEFT));
		}
	}
}

//-----------------------------------------------------------------------------

AlternateMapper_2::AlternateMapper_2()
{
	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;

	m_RadialButtons = { 0 };
	m_RadialButtons.stick = &m_LStick;
	m_RadialButtons.numButtons = 8;
	m_RadialButtons.showCursor = true;

	m_SpecialButtons[0].SetTempo(JOYPAD_START, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_LEFT_THUMB, FLAG(MODE_DEFAULT));
	m_SpecialButtons[3].SetTempo(JOYPAD_LEFT_SHOULDER, 0xFF);
	//m_SpecialButtons[4].SetTempo(JOYPAD_RIGHT_SHOULDER, 0xFF);

	//m_SpecialButtons[7].SetAxis2Btn(&m_LTrigger, 0.9, 0xFF);
	m_SpecialButtons[8].SetAxis2Btn(&m_RTrigger, 0.1, 0xFF);
	m_SpecialButtons[9].SetAxis2Btn(&m_RTrigger, 0.9, 0xFF);

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.65).AddValue(0.15);

	m_MouseButtons[0] = JOYPAD_DPAD_LEFT;
	m_MouseButtons[1] = JOYPAD_DPAD_RIGHT;
	m_MouseButtons[2] = JOYPAD_DPAD_UP;
	m_MouseButtons[3] = JOYPAD_DPAD_DOWN;
}

void AlternateMapper_2::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_RIGHT_THUMB;

	// Modifiers
	if (BTNDOWN(mouseBtn))
	{
		m_Mode = MODE_MOUSE;
	}
	else
	{
		bool lMod = BTNDOWN(JOYPAD_LEFT_SHOULDER);
		bool rMod = BTNDOWN(JOYPAD_RIGHT_SHOULDER);

		m_Mode = lMod && rMod ? MODE_BOTH_MOD : (lMod ? MODE_LEFT_MOD : (rMod ? MODE_RIGHT_MOD : MODE_DEFAULT));
	}

	if (m_Mode != MODE_MOUSE)
	{
		// Roll and pitch
		m_AxisX = m_RStick.X;
		m_AxisY = m_RStick.Y;
	}

	m_ButtonAxis.Update(time);

	static bool disableDetent = false;
	static bool invalid = false;
	if (m_Mode == MODE_DEFAULT)
	{
		if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
			m_ButtonAxis.CycleValue();

		if (BTNPRESSED(JOYPAD_LEFT_THUMB))
		{
			disableDetent = true;
		}
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		m_RadialButtons.Update(time);
	}

	if (m_Mode == MODE_DEFAULT)
	{
		const double lookSpeed = 10.0;

		bool lookBack = BTNDOWN(JOYPAD_LEFT_THUMB);
		double offsetX = lookBack ? (m_LStick.X < 0 ? -0.5 : 0.5) : 0.0;

		double x = (m_LStick.Magnitude < STICK_VIEW_DEADZONE ? 0 : m_LStick.X) * 0.5 + offsetX;
		double y = m_LStick.Magnitude < STICK_VIEW_DEADZONE ? 0 : m_LStick.Y;
		m_AxisRX = Clamp(Lerp(m_AxisRX, x, time.deltaTime * lookSpeed), -1, 1);
		m_AxisRY = Clamp(Lerp(m_AxisRY, y, time.deltaTime * lookSpeed), -1, 1);

		const double throttleSpeed = 1.0;
		double thrDelta = 0;
		if (BTNDOWN(JOYPAD_DPAD_UP))
			thrDelta += throttleSpeed;

		if (BTNDOWN(JOYPAD_DPAD_DOWN))
			thrDelta -= throttleSpeed;

		if (BTNPRESSED(JOYPAD_DPAD_UP) && fabs(m_Slider - m_ABDetent) < 0.01)
			disableDetent = true;

		if (m_Slider < m_ABDetent)
			disableDetent = false;

		double maxThr = disableDetent ? 1 : m_ABDetent;
		m_Slider = Clamp(m_Slider + thrDelta * time.deltaTime, 0, maxThr);
	}

	if (m_Mode == MODE_MOUSE && time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
	{
		HandleMouse(m_RStick);
	}

	// Rudder
	m_AxisZ = m_RTrigger - m_LTrigger;

	// Wheel brake
	double threshold = 0.1;
	m_AxisRZ = (m_RTrigger > threshold && m_LTrigger > threshold) ? (m_RTrigger + m_LTrigger) / 2 : 0;
}

void AlternateMapper_2::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_A));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_B));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_X));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_Y));

		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, BtnDown(JOYPAD_LEFT_SHOULDER));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_RIGHT_SHOULDER));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_BACK));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_START));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_GUIDE));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_SHARE));
		}

		if (!IsMode(FLAG(MODE_MOUSE)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_LEFT_THUMB));

		//if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_MOUSE)))
		//	SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_RIGHT_THUMB));

		if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_MOUSE)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_LEFT));
		}
	}

	const int btOffset = MAX_LOGICAL_BUTTONS - MAX_SPECIAL_BUTTONS - m_RadialButtons.numButtons;
	if (ctr < btOffset)
		ctr = btOffset;

	m_RadialButtons.AddLogicalButtons([this, &ctr](bool b) { SetLogicalButton(ctr++, b); });
}