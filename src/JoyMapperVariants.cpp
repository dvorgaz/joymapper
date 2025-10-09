#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

#define USE_PEDALS 0

DefaultMapper::DefaultMapper()
{
	m_StickSlider = { 0 };
	m_StickSlider.stick = &m_RStick;
	m_StickSlider.SetRegion(0, -1.0, 0.25, &m_Slider, true, false);
	m_StickSlider.SetDetent(0, 0, 0.8, 0.2);	
	//m_StickSlider.vibrFunc = [this](unsigned short l, unsigned short r, double dur) { Vibrate(l, r, dur); };
	m_AfterburnerDetent = m_StickSlider.GetDetentPtr(0);

	m_StickView = { 0 };
	m_StickView.stick = &m_RStick;
	m_StickView.outputX = &m_Ex_AxisRX;
	m_StickView.outputY = &m_Ex_AxisRY;
	m_StickView.SetFovSettings(&m_Dial, -0.2, 0.65);

	m_SpecialButtons[0].SetTempo(JOYPAD_START, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_DPAD_LEFT, FLAG(MODE_DEFAULT));
	m_SpecialButtons[2].SetTempo(JOYPAD_DPAD_RIGHT, FLAG(MODE_DEFAULT));
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

	m_VirtualPOV[0].stick = &m_RStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	m_VirtualPOV[0].showCursor = true;

	m_VirtualPOV[1].stick = &m_RStick;
}

void DefaultMapper::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_RIGHT_THUMB;
	const unsigned long throttleBtn = JOYPAD_LEFT_THUMB;
	const unsigned long viewCenterBtn = JOYPAD_LEFT_THUMB;

	// Modifiers
	if (BTNDOWN(mouseBtn) && time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
	{
		m_Mode = MODE_MOUSE;
	}
	else
	{
		bool lMod = BTNDOWN(JOYPAD_LEFT_SHOULDER);
		bool rMod = BTNDOWN(JOYPAD_RIGHT_SHOULDER);

		m_Mode = lMod && rMod ? MODE_BOTH_MOD : (lMod ? MODE_LEFT_MOD : (rMod ? MODE_RIGHT_MOD : MODE_DEFAULT));
	}

	// Roll and pitch
	m_AxisX = m_LStick.X;
	m_AxisY = m_LStick.Y;		

	if (BTNRELEASED(viewCenterBtn) && time.time - BTNTIME(viewCenterBtn) < TEMPO_TIME)
		m_StickView.Recenter();

	m_ButtonAxis.Update(time);
	m_StickView.Update(!BTNDOWN(throttleBtn) && (m_Mode == MODE_DEFAULT), time);

	if (m_Mode == MODE_DEFAULT)
	{
		if (BTNDOWN(throttleBtn))
		{
			// Throttle and zoom
			if (BTNDOWN(JOYPAD_RIGHT_THUMB))
				m_StickSlider.detentUnlocked = true;

			m_StickSlider.Update(time);
		}
		else
		{
			if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
				m_ButtonAxis.CycleValue();

			m_StickView.lastUpdateTime = time.time;
		}
	}

	switch (m_Mode)
	{
	case MODE_RIGHT_MOD:
		m_VirtualPOV[0].Update(time);
		break;
	case MODE_LEFT_MOD:
		m_AxisRX = m_RStick.X;
		m_AxisRY = m_RStick.Y;
		break;
	case MODE_BOTH_MOD:
		m_VirtualPOV[1].Update(time);
		break;
	}

	if (m_Mode == MODE_MOUSE)
	{
		HandleMouse(m_RStick, &m_StickView, &time);
	}

#if !USE_PEDALS
	// Rudder
	m_AxisZ = m_RTrigger - m_LTrigger;
#endif

	// Wheel brake
	double threshold = 0.1;
	m_AxisRZ = (m_RTrigger > threshold && m_LTrigger > threshold) ? (m_RTrigger + m_LTrigger) / 2 : 0;
}

void DefaultMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
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

		if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_MOUSE)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_RIGHT_THUMB));

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

DefaultMapper_2::DefaultMapper_2()
{
	m_StickSlider = { 0 };
	m_StickSlider.stick = &m_RStick;
	m_StickSlider.SetRegion(0, -1.0, 0.25, &m_Slider, true, false);
	m_StickSlider.SetDetent(0, 0, 0.8, 0.2);
	//m_StickSlider.vibrFunc = [this](unsigned short l, unsigned short r, double dur) { Vibrate(l, r, dur); };
	m_AfterburnerDetent = m_StickSlider.GetDetentPtr(0);

	m_StickView = { 0 };
	m_StickView.stick = &m_RStick;
	m_StickView.outputX = &m_Ex_AxisRX;
	m_StickView.outputY = &m_Ex_AxisRY;
	m_StickView.SetFovSettings(&m_Dial, -0.2, 0.65);
	m_StickView.absolute = true;

	m_SpecialButtons[0].SetTempo(JOYPAD_START, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_DPAD_LEFT, FLAG(MODE_DEFAULT));
	m_SpecialButtons[2].SetTempo(JOYPAD_DPAD_RIGHT, FLAG(MODE_DEFAULT));
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
	//m_VirtualPOV[0].showCursor = true;

	m_VirtualPOV[1].stick = &m_RStick;
}

void DefaultMapper_2::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_RIGHT_SHOULDER;
	const unsigned long throttleBtn = JOYPAD_LEFT_SHOULDER;
	const unsigned long viewCenterBtn = JOYPAD_LEFT_SHOULDER;

	// Modifiers
	bool lMod = BTNDOWN(JOYPAD_LEFT_SHOULDER);
	bool rMod = BTNDOWN(JOYPAD_RIGHT_SHOULDER);

	m_Mode = lMod && rMod ? MODE_BOTH_MOD : (lMod ? MODE_LEFT_MOD : (rMod ? MODE_RIGHT_MOD : MODE_DEFAULT));

	if (m_Mode == MODE_DEFAULT || m_Mode == MODE_LEFT_MOD)
	{
		// Roll and pitch
		m_AxisX = m_LStick.X;
		m_AxisY = m_LStick.Y;
	}

	if (BTNRELEASED(viewCenterBtn) && time.time - BTNTIME(viewCenterBtn) < TEMPO_TIME)
	{
		//m_StickView.Recenter();
		m_StickView.SetOffset();
	}

	m_ButtonAxis.Update(time);
	m_StickView.Update(!BTNDOWN(throttleBtn) && (m_Mode == MODE_DEFAULT), time);

	if (m_Mode == MODE_DEFAULT)
	{
		if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
			m_ButtonAxis.CycleValue();

		m_StickView.lastUpdateTime = time.time;
	}

	if (m_Mode == MODE_LEFT_MOD)
	{
		// Throttle and zoom
		if (BTNDOWN(JOYPAD_RIGHT_THUMB))
			m_StickSlider.detentUnlocked = true;

		m_StickSlider.Update(time);
	}

	if (m_Mode == MODE_RIGHT_MOD)
	{
		if (time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
			HandleMouse(m_RStick, &m_StickView, &time);

		m_VirtualPOV[0].Update(time);
	}

	if (m_Mode == MODE_BOTH_MOD)
	{
		m_AxisRX = m_LStick.X;
		m_AxisRY = m_LStick.Y;

		m_VirtualPOV[1].Update(time);
	}

#if !USE_PEDALS
	// Rudder
	m_AxisZ = m_RTrigger - m_LTrigger;
#else
	double thrSpeed = 3.0;
	m_Slider = Clamp(m_Slider + PowerCurve(m_RTrigger - m_LTrigger, STICK_VIEW_CURVE) * time.deltaTime * thrSpeed, 0, 1);
#endif

	// Wheel brake
	double threshold = 0.1;
	m_AxisRZ = (m_RTrigger > threshold && m_LTrigger > threshold) ? (m_RTrigger + m_LTrigger) / 2 : 0;
}

void DefaultMapper_2::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
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

		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_LEFT_THUMB));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_RIGHT_THUMB));

		if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_RIGHT_MOD)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_LEFT));
		}
	}
}

//-----------------------------------------------------------------------------

DefaultMapper_3::DefaultMapper_3()
{
	//m_StickSlider = { 0 };
	//m_StickSlider.stick = &m_RStick;
	//m_StickSlider.SetRegion(0, -1.0, 0.25, &m_Slider, true, false);
	//m_StickSlider.SetDetent(0, 0, 0.8, 0.2);

	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;

	m_SpecialButtons[0].SetTempo(JOYPAD_START, 0xFF);
	m_SpecialButtons[1].SetTempo(JOYPAD_DPAD_LEFT, FLAG(MODE_DEFAULT));
	m_SpecialButtons[2].SetTempo(JOYPAD_DPAD_RIGHT, FLAG(MODE_DEFAULT));
	m_SpecialButtons[3].SetTempo(JOYPAD_LEFT_SHOULDER, 0xFF);
	//m_SpecialButtons[4].SetTempo(JOYPAD_RIGHT_SHOULDER, 0xFF);

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

	m_VirtualPOV[0].stick = &m_RStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	//m_VirtualPOV[0].showCursor = true;

	m_VirtualPOV[1].stick = &m_RStick;
}

void DefaultMapper_3::UpdateInternal(const STime& time)
{
	const unsigned long mouseBtn = JOYPAD_RIGHT_SHOULDER;
	const unsigned long throttleBtn = JOYPAD_LEFT_SHOULDER;

	// Modifiers
	bool lMod = BTNDOWN(JOYPAD_LEFT_SHOULDER);
	bool rMod = BTNDOWN(JOYPAD_RIGHT_SHOULDER);

	m_Mode = lMod && rMod ? MODE_BOTH_MOD : (lMod ? MODE_LEFT_MOD : (rMod ? MODE_RIGHT_MOD : MODE_DEFAULT));

	// Roll and pitch
	m_AxisX = m_LStick.X;
	m_AxisY = m_LStick.Y;

	m_ButtonAxis.Update(time);

	if (BTNRELEASED(mouseBtn) && time.time - BTNTIME(mouseBtn) < TEMPO_TIME)
		m_ButtonAxis.CycleValue();

	if (m_Mode == MODE_DEFAULT)
	{
		m_AxisRX = m_RStick.X;
		m_AxisRY = m_RStick.Y;

		const double throttleSpeed = 1.0;
		double thrDelta = 0;
		if (BTNDOWN(JOYPAD_Y))
			thrDelta += throttleSpeed;

		if (BTNDOWN(JOYPAD_B))
			thrDelta -= throttleSpeed;

		static bool disableDetent = false;

		if (BTNPRESSED(JOYPAD_Y) && fabs(m_Slider - m_ABDetent) < 0.01)
			disableDetent = true;

		if (m_Slider < m_ABDetent)
			disableDetent = false;

		double maxThr = disableDetent ? 1 : m_ABDetent;
		m_Slider = Clamp(m_Slider + thrDelta * time.deltaTime, 0, maxThr);
	}

	if (m_Mode == MODE_LEFT_MOD)
	{	
		m_VirtualPOV[0].Update(time);
	}

	if (m_Mode == MODE_RIGHT_MOD)
	{
		if (time.time - BTNTIME(mouseBtn) >= TEMPO_TIME)
			HandleMouse(m_RStick);
	}

	if (m_Mode == MODE_BOTH_MOD)
	{	
		m_VirtualPOV[1].Update(time);
	}

#if !USE_PEDALS
	// Rudder
	m_AxisZ = m_RTrigger - m_LTrigger;
#else
	double thrSpeed = 3.0;
	m_Slider = Clamp(m_Slider + PowerCurve(m_RTrigger - m_LTrigger, STICK_VIEW_CURVE) * time.deltaTime * thrSpeed, 0, 1);
#endif

	// Wheel brake
	double threshold = 0.1;
	m_AxisRZ = (m_RTrigger > threshold && m_LTrigger > threshold) ? (m_RTrigger + m_LTrigger) / 2 : 0;
}

void DefaultMapper_3::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_A));

		if (!IsMode(FLAG(MODE_DEFAULT)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_B));

		if(IsMode(FLAG(MODE_DEFAULT)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_X) && !BtnDown(JOYPAD_A));
		else
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_X));

		if (!IsMode(FLAG(MODE_DEFAULT)))
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_Y));
		
		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, BtnDown(JOYPAD_A) && BtnDown(JOYPAD_X));

			SetLogicalButton(ctr++, BtnDown(JOYPAD_LEFT_SHOULDER));
			//SetLogicalButton(ctr++, BtnDown(JOYPAD_RIGHT_SHOULDER));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_BACK));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_START));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_GUIDE));
			SetLogicalButton(ctr++, BtnDown(JOYPAD_SHARE));
		}

		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_LEFT_THUMB));
		SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_RIGHT_THUMB));

		if (!IsMode(FLAG(MODE_DEFAULT) | FLAG(MODE_RIGHT_MOD)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_RIGHT));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(JOYPAD_DPAD_LEFT));
		}
	}
}