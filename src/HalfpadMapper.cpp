#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

HalfpadMapper::HalfpadMapper()
{
	m_StickSlider = { 0 };
	m_StickSlider.stick = &m_RStick;
	m_StickSlider.SetRegion(0, 0.0, 1.0, &m_Ex_AxisRZ, false, true);
	m_StickSlider.SetDetent(0, 0, 0.5, 0.2);
	//m_StickSlider.vibrFunc = [this](unsigned short l, unsigned short r, double dur) { Vibrate(l, r, dur); };
	//m_AfterburnerDetent = m_StickSlider.GetDetentPtr(0);

	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;

	m_SpecialButtons[0].SetTempo(JOYPAD_SHARE, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_DPAD_LEFT, FLAG(MODE_DEFAULT));
	m_SpecialButtons[2].SetTempo(JOYPAD_DPAD_RIGHT, FLAG(MODE_DEFAULT));

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.65).AddValue(0.15);

	m_DoubleTap = false;

	m_VirtualPOV[0].stick = &m_LStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	m_VirtualPOV[0].showCursor = true;
}

void HalfpadMapper::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_LEFT_THUMB;
	const unsigned long mouseKey = 1;

	// Modifiers
	if (BTNDOWN(mouseBtn))
	{
		m_Mode = MODE_MOUSE;
	}
	else
	{
		static double doubleTapTime = time.time;

		if (BTNPRESSED(JOYPAD_LEFT_SHOULDER))
		{
			m_DoubleTap = time.time - doubleTapTime < 0.3;
			doubleTapTime = time.time;
		}

		if (BTNRELEASED(JOYPAD_LEFT_SHOULDER))
		{
			m_DoubleTap = false;
		}

		m_Mode = BTNDOWN(JOYPAD_LEFT_SHOULDER) ? (m_DoubleTap ? MODE_RIGHT_MOD : MODE_LEFT_MOD) : MODE_DEFAULT;
	}

	m_ButtonAxis.Update(time);

	if (BTNPRESSED(mouseBtn))
		ExtraMouseButton(mouseKey, true);

	if (BTNRELEASED(mouseBtn))
		ExtraMouseButton(mouseKey, false);

	if (m_Mode == MODE_DEFAULT)
	{
		// Throttle and zoom
		//if (BTNDOWN(JOYPAD_LEFT_THUMB))
		//	m_StickSlider.detentUnlocked = true;

		m_StickSlider.Update(time);

		if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
			m_ButtonAxis.CycleValue();

		const double lookSpeed = 10.0;

		double x = m_LStick.Magnitude < STICK_VIEW_DEADZONE ? 0 : m_LStick.X;
		double y = m_LStick.Magnitude < STICK_VIEW_DEADZONE ? 0 : m_LStick.Y;
		m_Ex_AxisRX = Clamp(Lerp(m_Ex_AxisRX, x, time.deltaTime * lookSpeed), -1, 1);
		m_Ex_AxisRY = Clamp(Lerp(m_Ex_AxisRY, y, time.deltaTime * lookSpeed), -1, 1);

		const double throttleSpeed = 1.0;
		double thrDelta = 0;
		if (BTNDOWN(JOYPAD_DPAD_UP))
			thrDelta += throttleSpeed;

		if (BTNDOWN(JOYPAD_DPAD_DOWN))
			thrDelta -= throttleSpeed;

		static bool disableDetent = false;

		if (BTNPRESSED(JOYPAD_DPAD_UP) && fabs(m_Slider - m_ABDetent) < 0.01)
			disableDetent = true;

		if (m_Slider < m_ABDetent)
			disableDetent = false;

		double maxThr = disableDetent ? 1 : m_ABDetent;
		m_Slider = Clamp(m_Slider + thrDelta * time.deltaTime, 0, maxThr);
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		m_AxisRX = m_LStick.X;
		m_AxisRY = m_LStick.Y;
	}

	if (m_Mode == MODE_RIGHT_MOD)
	{
		m_VirtualPOV[0].Update(time);
	}

	if (m_Mode == MODE_MOUSE && time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
	{
		HandleMouse(m_LStick);
	}

	// Wheel brake
	m_AxisRZ = m_LTrigger;
}

void HalfpadMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, !m_DoubleTap && BtnDown(JOYPAD_LEFT_SHOULDER));
			SetLogicalButton(ctr++, m_DoubleTap && BtnDown(JOYPAD_LEFT_SHOULDER));

			SetLogicalButton(ctr++, BtnDown(JOYPAD_BACK));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_GUIDE));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_SHARE));
		}

		if (!IsMode(FLAG(MODE_DEFAULT)))
		{
			//SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_RIGHT));
			//SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_LEFT));
		}
	}
}