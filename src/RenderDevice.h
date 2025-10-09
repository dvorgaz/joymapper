#pragma once
#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dcomp.h>
#pragma comment(lib, "dcomp")

#include <memory>
#include <SpriteFont.h>
#include <SimpleMath.h>
#include "VertexTypes.h"
#include "CommonStates.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include <wrl/client.h>
#include <functional>

namespace HudColors
{
	XMGLOBALCONST DirectX::XMVECTORF32 HudGreen = { { { 0.2f, 1.f, 0.2f, 1.f } } };
	XMGLOBALCONST DirectX::XMVECTORF32 HudText = { { { 0.0f, 0.f, 0.0f, 1.f } } };
}

//struct SimpleVertex
//{
//	XMFLOAT3 Pos;
//};

class RenderDevice
{
	friend class JoyMapperRenderer;

private:	
	HWND	m_hWnd = nullptr;
	UINT	m_Width;
	UINT	m_Height;

	D3D_DRIVER_TYPE         m_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	ID3D11Device*			m_pd3dDevice = nullptr;
	ID3D11Device1*			m_pd3dDevice1 = nullptr;
	ID3D11DeviceContext*	m_pImmediateContext = nullptr;
	ID3D11DeviceContext1*	m_pImmediateContext1 = nullptr;
	IDXGISwapChain*			m_pSwapChain = nullptr;
	IDXGISwapChain1*		m_pSwapChain1 = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

	//ID3D11VertexShader*		m_pVertexShader = nullptr;
	//ID3D11PixelShader*		m_pPixelShader = nullptr;
	//ID3D11InputLayout*		m_pVertexLayout = nullptr;
	//ID3D11Buffer*				m_pVertexBuffer = nullptr;

	std::unique_ptr<DirectX::SpriteFont>	m_font;
	DirectX::SimpleMath::Vector2			m_fontPos;
	std::unique_ptr<DirectX::SpriteBatch>	m_spriteBatch;

	using VertexType = DirectX::VertexPositionColor;

	std::unique_ptr<DirectX::CommonStates>					m_states;
	std::unique_ptr<DirectX::BasicEffect>					m_effect;
	std::unique_ptr<DirectX::PrimitiveBatch<VertexType>>	m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>				m_inputLayout;

	float m_ClearColor[4] = { 1.f, 0.f, 1.f, 1.f };

	std::function<void()> m_DrawFunc = nullptr;

public:	
	RenderDevice();
	RenderDevice(HWND hWnd);

	HRESULT InitDevice();
	void CleanupDevice();
	void Render();
	void SetClearColor(COLORREF color);
	void SetDrawFunc(std::function<void()> drawFunc);

private:
	HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	void CreateResources();
	void ReleaseResources();

	void DrawQuad(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector2& size, const DirectX::XMVECTORF32& color);
	void DrawFont(const wchar_t* text, const DirectX::SimpleMath::Vector3& pos, const DirectX::XMVECTORF32& color);
};