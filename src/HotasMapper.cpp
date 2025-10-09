#include "JoyMapperVariants.h"

#include "stdafx.h"
#include "Math.h"

HotasMapper::HotasMapper()
{
	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;

	m_MenuActivateBtn1 = TWCS_BOATSW_UP;
	m_MenuActivateBtn2 = TWCS_CASTLESW_UP;
	m_MenuAcceptBtn = TWCS_FRONT_2;
	m_MenuCancelBtn = TWCS_FRONT_1;

	m_SpecialButtons[0].SetTempo(TWCS_CASTLESW_UP, 0xFF);
	m_SpecialButtons[1].SetTempo(TWCS_CASTLESW_FORWARD, 0xFF);
	m_SpecialButtons[2].SetTempo(TWCS_CASTLESW_DOWN, 0xFF);
	m_SpecialButtons[3].SetTempo(TWCS_CASTLESW_AFT, 0xFF);

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.65).AddValue(0.15).AddValue(0.0);

	m_VirtualPOV[0].stick = &m_LStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	m_VirtualPOV[0].showCursor = true;
}

void HotasMapper::UpdateInternal(const STime& time)
{
	// Remap stick
	double min = min(fabs(m_LStick.X), fabs(m_LStick.Y));
	double max = max(fabs(m_LStick.X), fabs(m_LStick.Y));

	double diag = min / max;
	const double sqrt2 = sqrt(2);
	double mul = Lerp(1.1, 1.0 / sqrt2, diag);

	m_LStick.X = m_LStick.X * mul;
	m_LStick.Y = m_LStick.Y * mul;
	m_LStick.UpdateAngleMagnitude();

	// Throttle
	m_Slider = ApplyDeadzoneRegion(m_PhysAxisZ, m_ABDetent, 0.15);

	static bool modPrev = false;
	bool mod = m_PhysAxisRZ < -0.5;

	if (mod)
	{
		m_VirtualPOV[0].Update(time);
	}
	else
	{
		// Stick
		m_AxisRX = m_LStick.X;
		m_AxisRY = m_LStick.Y;
	}

	// Zoom
	static int prevZoomIdx = 0;
	if (BTNRELEASED(TWCS_BOATSW_UP) && time.time - BTNTIME(TWCS_BOATSW_UP) < TEMPO_TIME)
	{
		switch (m_ButtonAxis.valueIdx)
		{
		case 2:
			m_ButtonAxis.valueIdx = prevZoomIdx;
			break;
		case 1:		
			m_ButtonAxis.valueIdx = 0;
			break;
		case 0:
			m_ButtonAxis.valueIdx = 1;
			break;
		}

		prevZoomIdx = m_ButtonAxis.valueIdx;
	}
	else if (BTNDOWN(TWCS_BOATSW_UP) && time.time - BTNTIME(TWCS_BOATSW_UP) >= TEMPO_TIME)
	{		
		m_ButtonAxis.valueIdx = 2;
	}
	
	m_ButtonAxis.Update(time);
	//m_Dial = m_PhysDial;

	m_Ex_AxisY = m_PhysDial * 2 - 1;

	// Wheel brake
	m_AxisRZ = Clamp(m_PhysAxisRZ, 0, 1);

	modPrev = mod;
}

void HotasMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_THUMB));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_FRONT_1));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_FRONT_2));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_ROCKER_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_ROCKER_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_STICK_DEPRESS));
			//SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_FORWARD));
			//SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_AFT));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_FORWARD));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_AFT));
		}
	}

	const unsigned long timeBtn = TWCS_BOATSW_DOWN;
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

//---

PedalMapper::PedalMapper()
{
	m_ABDetent = 0.8;
	m_AfterburnerDetent = &m_ABDetent;

	m_MenuActivateBtn1 = TWCS_BOATSW_UP;
	m_MenuActivateBtn2 = TWCS_CASTLESW_UP;
	m_MenuAcceptBtn = TWCS_FRONT_2;
	m_MenuCancelBtn = TWCS_FRONT_1;

	m_SpecialButtons[0].SetTempo(TWCS_CASTLESW_UP, 0xFF);
	m_SpecialButtons[1].SetTempo(TWCS_CASTLESW_FORWARD, 0xFF);
	m_SpecialButtons[2].SetTempo(TWCS_CASTLESW_DOWN, 0xFF);
	m_SpecialButtons[3].SetTempo(TWCS_CASTLESW_AFT, 0xFF);

	m_ButtonAxis = { 0 };
	m_ButtonAxis.output = &m_Dial;
	m_ButtonAxis.AddValue(0.65).AddValue(0.15).AddValue(0.0);

	m_VirtualPOV[0].stick = &m_LStick;
	m_VirtualPOV[0].deadzoneOverride = 0;
	m_VirtualPOV[0].showCursor = true;
}

void PedalMapper::UpdateInternal(const STime& time)
{
	/*
	// Remap stick
	double min = min(fabs(m_LStick.X), fabs(m_LStick.Y));
	double max = max(fabs(m_LStick.X), fabs(m_LStick.Y));

	double diag = min / max;
	const double sqrt2 = sqrt(2);
	double mul = Lerp(1.1, 1.0 / sqrt2, diag);

	m_LStick.X = m_LStick.X * mul;
	m_LStick.Y = m_LStick.Y * mul;
	m_LStick.UpdateAngleMagnitude();

	// Throttle
	m_Slider = ApplyDeadzoneRegion(m_PhysAxisZ, m_ABDetent, 0.15);

	static bool modPrev = false;
	bool mod = m_PhysAxisRZ < -0.5;

	if (mod)
	{
		m_VirtualPOV[0].Update(time);
	}
	else
	{
		// Stick
		m_AxisRX = m_LStick.X;
		m_AxisRY = m_LStick.Y;
	}

	// Zoom
	static int prevZoomIdx = 0;
	if (BTNRELEASED(TWCS_BOATSW_UP) && time.time - BTNTIME(TWCS_BOATSW_UP) < TEMPO_TIME)
	{
		switch (m_ButtonAxis.valueIdx)
		{
		case 2:
			m_ButtonAxis.valueIdx = prevZoomIdx;
			break;
		case 1:
			m_ButtonAxis.valueIdx = 0;
			break;
		case 0:
			m_ButtonAxis.valueIdx = 1;
			break;
		}

		prevZoomIdx = m_ButtonAxis.valueIdx;
	}
	else if (BTNDOWN(TWCS_BOATSW_UP) && time.time - BTNTIME(TWCS_BOATSW_UP) >= TEMPO_TIME)
	{
		m_ButtonAxis.valueIdx = 2;
	}

	m_ButtonAxis.Update(time);
	//m_Dial = m_PhysDial;

	m_Ex_AxisY = m_PhysDial * 2 - 1;

	// Wheel brake
	m_AxisRZ = Clamp(m_PhysAxisRZ, 0, 1);

	modPrev = mod;
	*/
	m_Dial = 1 - m_LStick.Y;
	m_Slider = 1 - m_PhysAxisRZ;
}

void PedalMapper::UpdateLogicalButtonsInternal(int& ctr, const STime& time)
{
	auto BtnDown = [this](unsigned long btn) { return this->m_JoyButtonsProcessed & btn; };

	for (int i = 0; i < MODE_NUM; ++i)
	{
		bool enabled = m_Mode == (Mode)i;
		auto IsMode = [i](unsigned char flags) { return FLAG(i) & flags; };

		if (IsMode(FLAG(MODE_DEFAULT)))
		{
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_THUMB));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_FRONT_1));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_FRONT_2));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_ROCKER_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_ROCKER_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_STICK_DEPRESS));
			//SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_FORWARD));
			//SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_BOATSW_AFT));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_UP));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_FORWARD));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_DOWN));
			SetLogicalButton(ctr++, enabled && BtnDown(TWCS_CASTLESW_AFT));
		}
	}

	const unsigned long timeBtn = TWCS_BOATSW_DOWN;
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