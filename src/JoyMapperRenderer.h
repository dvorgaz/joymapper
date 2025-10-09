#pragma once
#include "RenderDevice.h"
#include "JoyMapper.h"

class JoyMapperRenderer
{
private:
	RenderDevice*	m_Device = nullptr;
	JoyMapper*		m_Mapper = nullptr;

	double m_LastThrottle = 0.0;
	double m_LastThrottleTime = 0.0;
	bool m_ShowThrottle = false;

public:
	JoyMapperRenderer();
	JoyMapperRenderer(RenderDevice* device, JoyMapper* mapper);

	void Update(const STime& time);
	void Render();

private:
	void DrawShapes();
	void DrawTexts();

	void DrawAxis(float value, float detent, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size);
};