#include "stdafx.h"

#include "JoyMapperRenderer.h"
#include <cmath>

using namespace DirectX;
using namespace DirectX::SimpleMath;

#define BATCH		m_Device->m_batch
#define SPRITEBATCH	m_Device->m_spriteBatch

#define WIDTH	m_Device->m_Width
#define HEIGHT	m_Device->m_Height

JoyMapperRenderer::JoyMapperRenderer()
{
}

JoyMapperRenderer::JoyMapperRenderer(RenderDevice* device, JoyMapper* mapper)
{
	m_Device = device;
	m_Mapper = mapper;
}

void JoyMapperRenderer::Update(const STime& time)
{
	if (fabs(m_LastThrottle - m_Mapper->m_Slider) > 0.02)
	{
		m_LastThrottleTime = time.time;
		m_LastThrottle = m_Mapper->m_Slider;
	}

	m_ShowThrottle = time.time - m_LastThrottleTime < 2.0;
}

void JoyMapperRenderer::Render()
{
	// Draw shapes
	BATCH->Begin();
	DrawShapes();
	BATCH->End(); // Draw shapes

	// Draw text
	SPRITEBATCH->Begin();
	DrawTexts();
	SPRITEBATCH->End(); // Draw text
}

void JoyMapperRenderer::DrawShapes()
{
	// Throttle axis
	if(m_ShowThrottle)
		DrawAxis(m_Mapper->m_Slider, m_Mapper->GetAfterburnerDetent(), Vector3(400, 105, 0.5), Vector3(20, 200, 0));
}

void JoyMapperRenderer::DrawTexts()
{
	wchar_t buf[100];
	if (m_Mapper->m_IsMenuMode)
	{
		int len = swprintf(buf, 100, L"DETENT SETTINGS");
		m_Device->DrawFont(buf, Vector3(WIDTH / 2, 50, 0.5), HudColors::HudGreen);

		swprintf(buf, 100, m_Mapper->m_MenuOptions[m_Mapper->m_MenuIdx].label.c_str());
		m_Device->DrawFont(buf, Vector3(WIDTH / 2, 150, 0.5), HudColors::HudGreen);
	}
}

void JoyMapperRenderer::DrawAxis(float value, float detent, const Vector3& pos, const Vector3& size)
{
	Vector3 right(size.x * 0.5f, 0, 0);
	Vector3 up(0, size.y * 0.5f, 0);

	XMVECTORF32 color = { { { 0.2f, 1.f, 0.2f, 1.f } } };

	VertexPositionColor v1(pos - right - up, color);
	VertexPositionColor v2(pos + right - up, color);
	VertexPositionColor v3(pos - right + up, color);
	VertexPositionColor v4(pos + right + up, color);

	VertexPositionColor detent1(pos - right + up - (2 * up * detent), color);
	VertexPositionColor detent2(pos + right + up - (2 * up * detent), color);

	VertexPositionColor value1(pos - right + up - (2 * up * value), color);
	VertexPositionColor value2(pos + right * 0.5f + up - (2 * up * value), color);
	VertexPositionColor value3(pos + right * 0.5f + up, color);

	BATCH->DrawLine(v1, v3);
	BATCH->DrawLine(v1, v2);
	BATCH->DrawLine(v3, v4);

	BATCH->DrawLine(detent1, detent2);

	//BATCH->DrawLine(value1, value2);
	//BATCH->DrawLine(value2, value3);

	BATCH->DrawQuad(value1, value2, value3, v3);
}
