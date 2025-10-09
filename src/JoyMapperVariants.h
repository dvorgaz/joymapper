#pragma once
#include "JoyMapper.h"

class DefaultMapper : public JoyMapper
{
public:
	DefaultMapper();

private:
	StickSlider m_StickSlider;
	StickView m_StickView;
	ButtonAxis m_ButtonAxis;

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