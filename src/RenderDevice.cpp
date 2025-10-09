#include "stdafx.h"

#include "RenderDevice.h"

#include <DirectXHelpers.h>
#include <exception>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			throw std::exception();
		}
	}
}

RenderDevice::RenderDevice()
{
}

RenderDevice::RenderDevice(HWND hWnd)
{
	m_hWnd = hWnd;
}

HRESULT RenderDevice::InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(m_hWnd, &rc);
	m_Width = rc.right - rc.left;
	m_Height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		m_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
	if (dxgiFactory2)
	{
		// DirectX 11.1 or later
		hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&m_pd3dDevice1));
		if (SUCCEEDED(hr))
		{
			(void)m_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&m_pImmediateContext1));
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = m_Width;
		sd.Height = m_Height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

		hr = dxgiFactory2->CreateSwapChainForHwnd(m_pd3dDevice, m_hWnd, &sd, nullptr, nullptr, &m_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&m_pSwapChain));
		}

		dxgiFactory2->Release();
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1;
		sd.BufferDesc.Width = m_Width;
		sd.BufferDesc.Height = m_Height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = dxgiFactory->CreateSwapChain(m_pd3dDevice, &sd, &m_pSwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	m_pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_pImmediateContext->RSSetViewports(1, &vp);

	CreateResources();

	return S_OK;
}

void RenderDevice::CleanupDevice()
{
	ReleaseResources();

	if (m_pImmediateContext) m_pImmediateContext->ClearState();

	if (m_pRenderTargetView) m_pRenderTargetView->Release();
	if (m_pSwapChain1) m_pSwapChain1->Release();
	if (m_pSwapChain) m_pSwapChain->Release();
	if (m_pImmediateContext1) m_pImmediateContext1->Release();
	if (m_pImmediateContext) m_pImmediateContext->Release();
	if (m_pd3dDevice1) m_pd3dDevice1->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
}

void RenderDevice::Render()
{
	// Just clear the backbuffer
	m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, m_ClearColor);

	// Render a triangle
	//m_pImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	//m_pImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);
	//m_pImmediateContext->Draw(3, 0);

	m_pImmediateContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_pImmediateContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_pImmediateContext->RSSetState(m_states->CullNone());

	m_effect->Apply(m_pImmediateContext);

	m_pImmediateContext->IASetInputLayout(m_inputLayout.Get());

	if (m_DrawFunc)
		m_DrawFunc();

	m_pSwapChain->Present(0, 0);
}

void RenderDevice::SetClearColor(COLORREF color)
{
	for (int i = 0; i < 4; ++i)
	{
		int intVal = (color >> i * 8) & 0xFF;
		m_ClearColor[i] = (float)intVal / 255.0f;
	}
}

void RenderDevice::SetDrawFunc(std::function<void()> drawFunc)
{
	m_DrawFunc = drawFunc;
}

HRESULT RenderDevice::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

void RenderDevice::CreateResources()
{
	/*
	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial02.fxh", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	m_pImmediateContext->IASetInputLayout(m_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial02.fxh", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		XMFLOAT3(0.0f, 0.5f, 0.5f),
		XMFLOAT3(0.5f, -0.5f, 0.5f),
		XMFLOAT3(-0.5f, -0.5f, 0.5f),
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = m_pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	m_pImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Set primitive topology
	m_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
*/

	m_font = std::make_unique<SpriteFont>(m_pd3dDevice, L"CourierNew.spritefont");
	m_spriteBatch = std::make_unique<SpriteBatch>(m_pImmediateContext);

	m_fontPos.x = float(m_Width) / 2.f;
	m_fontPos.y = float(m_Height) / 2.f;

	m_states = std::make_unique<CommonStates>(m_pd3dDevice);

	m_effect = std::make_unique<BasicEffect>(m_pd3dDevice);
	m_effect->SetVertexColorEnabled(true);

	Matrix proj = Matrix::CreateScale(2.f / float(m_Width), -2.f / float(m_Height), 1.f) * Matrix::CreateTranslation(-1.f, 1.f, 0.f);
	m_effect->SetProjection(proj);

	DX::ThrowIfFailed(
		CreateInputLayoutFromEffect<VertexType>(m_pd3dDevice, m_effect.get(),
			m_inputLayout.ReleaseAndGetAddressOf())
	);

	m_batch = std::make_unique<PrimitiveBatch<VertexType>>(m_pImmediateContext);
}

void RenderDevice::ReleaseResources()
{
	m_states.reset();
	m_effect.reset();
	m_batch.reset();
	m_inputLayout.Reset();

	m_spriteBatch.reset();
	m_font.reset();

	//if (m_pVertexBuffer) m_pVertexBuffer->Release();
	//if (m_pVertexLayout) m_pVertexLayout->Release();
	//if (m_pVertexShader) m_pVertexShader->Release();
	//if (m_pPixelShader) m_pPixelShader->Release();
}

void RenderDevice::DrawQuad(const Vector3& pos, const Vector2& size, const XMVECTORF32& color)
{
	VertexPositionColor v1(Vector3(pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, 0.5f), color);
	VertexPositionColor v2(Vector3(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0.5f), color);
	VertexPositionColor v3(Vector3(pos.x + size.x * 0.5f, pos.y - size.y * 0.5f, 0.5f), color);
	VertexPositionColor v4(Vector3(pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, 0.5f), color);

	m_batch->DrawQuad(v1, v2, v3, v4);
}

void RenderDevice::DrawFont(const wchar_t* text, const Vector3& pos, const XMVECTORF32& color)
{
	Vector2 origin = m_font->MeasureString(text) / 2.f;
	m_font->DrawString(m_spriteBatch.get(), text, pos, color, 0.f, origin, 0.75f);
}
