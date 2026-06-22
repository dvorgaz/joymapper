#pragma once
#include "JoyMapper.h"

class DefaultMapper : public JoyMapper
{
public:
	DefaultMapper();

private:
	StickSlider m_StickSlider;
	StickView m_StickView;
	StickView m_StickMove;
	ButtonAxis m_ButtonAxis;

	double m_TempX;
	double m_TempY;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class DefaultMapper_2: public JoyMapper
{
public:
	DefaultMapper_2();

private:
	StickSlider m_StickSlider;
	StickView m_StickView;
	ButtonAxis m_ButtonAxis;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class DefaultMapper_3 : public JoyMapper
{
public:
	DefaultMapper_3();

private:
	//StickSlider m_StickSlider;
	ButtonAxis m_ButtonAxis;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class AlternateMapper : public JoyMapper
{
public:
	AlternateMapper();

private:
	StickSlider m_StickSlider;
	ButtonAxis m_ButtonAxis;	

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class AlternateMapper_2 : public JoyMapper
{
public:
	AlternateMapper_2();

private:
	RadialButtons m_RadialButtons;
	ButtonAxis m_ButtonAxis;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class HalfpadMapper : public JoyMapper
{
public:
	HalfpadMapper();

private:
	StickSlider m_StickSlider;
	ButtonAxis m_ButtonAxis;
	bool m_DoubleTap;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class HotasMapper : public JoyMapper
{
public:
	HotasMapper();

private:
	ButtonAxis m_ButtonAxis;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class JoystickMapper : public JoyMapper
{
public:
	JoystickMapper();

private:
	ButtonThrottle m_buttonThrottle;
	ButtonAxis m_ButtonAxis;
	ButtonAxis m_WheelBrakeAxis;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class BMSMapper : public JoyMapper
{
public:
	BMSMapper();

private:
	ButtonThrottle m_buttonThrottle;
	ButtonAxis m_ButtonZoom;
	ButtonAxis m_WheelBrakeAxis;
	double m_ABDetent;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;
};

//-----------------------------------------------------------------------------

class MouseThrottleMapper : public JoyMapper
{
	enum AxisMode
	{
		AM_NONE,
		AM_THROTTLE,
		AM_ZOOM
	};

public:
	MouseThrottleMapper();

private:
	Stick m_MouseStick;
	ButtonAxis m_ButtonAxis;
	ButtonAxis m_WheelBrakeAxis;
	double m_ABDetent;
	double m_ZoomAxis;
	double m_ThrottleAxis;
	AxisMode m_axisMode;
	long m_deltaX;
	long m_deltaY;
	long m_ScreenWidth;
	long m_ScreenHeight;
	int m_MouseLocked;
	long m_SavedMouseX;
	long m_SavedMouseY;

	void UpdateInternal(const STime& time) override;
	void UpdateLogicalButtonsInternal(int& ctr, const STime& time) override;

	void LockMouse(bool locked);
};