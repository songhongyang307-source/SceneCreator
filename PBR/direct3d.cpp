/*==============================================================================

   Direct3D�̏������֘A [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"

#pragma comment(lib, "d3d11.lib")   //可以在project property 里的linker 入力中添加 d3d11.lib   and DirectXTex_Debug.lib  
// #pragma comment(lib, "dxgi.lib")


/* interface */
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11BlendState* g_BlendStateMultiply = nullptr;
static ID3D11BlendState* g_BlendStateAdd = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthDisable = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthEnable = nullptr;
static ID3D11RasterizerState* g_pRasterizerState = nullptr;

/* backbuffer */
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_Viewport{}; //add after

static bool configureBackBuffer(); // �o�b�N�o�b�t�@�̐ݒ�E����
static void releaseBackBuffer(); // �o�b�N�o�b�t�@�̉��

// --- for water ---
static ID3D11Texture2D* g_pBackBufferTex = nullptr; // 保存 swapchain buffer0
static ID3D11Texture2D* g_pSceneColorCopyTex = nullptr;
static ID3D11ShaderResourceView* g_pSceneColorCopySRV = nullptr;

// depth SRV (non-msaa / msaa)
static ID3D11ShaderResourceView* g_pDepthSRV = nullptr;
static ID3D11ShaderResourceView* g_pMsaaDepthSRV = nullptr;

/*     MSAA         */
static constexpr UINT MSAA_COUNT = 4;
static UINT g_MsaaQuality = 0;

// MSAA render target / depth
static ID3D11Texture2D* g_pMsaaColorBuffer = nullptr;
static ID3D11RenderTargetView* g_pMsaaRenderTargetView = nullptr;
static ID3D11Texture2D* g_pMsaaDepthBuffer = nullptr;
static ID3D11DepthStencilView* g_pMsaaDepthStencilView = nullptr;

// A2C blend state（给树叶用）
static ID3D11BlendState* g_BlendStateA2C = nullptr;

static ID3D11DepthStencilView* g_pDepthStencilViewRO = nullptr;
static ID3D11DepthStencilView* g_pMsaaDepthStencilViewRO = nullptr;

ID3D11DepthStencilView* Direct3D_GetMainDSV_ReadOnly()
{
	return g_pMsaaDepthStencilViewRO ? g_pMsaaDepthStencilViewRO : g_pDepthStencilViewRO;
}


bool Direct3D_Initialize(HWND hWnd)
{

	RECT rc{};
	GetClientRect(hWnd, &rc);
	UINT w = (UINT)(rc.right - rc.left);
	UINT h = (UINT)(rc.bottom - rc.top);
	if (w == 0) w = 1;
	if (h == 0) h = 1;

	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.Windowed = TRUE;
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.BufferDesc.Width = w;   // ★加上
	swap_chain_desc.BufferDesc.Height = h;   // ★加上
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swap_chain_desc.OutputWindow = hWnd;
	/*
	IDXGIFactory1* pFactory;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	IDXGIAdapter1* pAdapter;
	pFactory->EnumAdapters1(1, &pAdapter); // �Z�J���_���A�_�v�^��擾
	pFactory->Release();
	DXGI_ADAPTER_DESC1 desc;
	pAdapter->GetDesc1(&desc); // �A�_�v�^�̏���擾���Ċm�F�������ꍇ
	pAdapter->Release(); // D3D11CreateDeviceAndSwapChain()�̑�P�����ɓn���ė��p���I�������������
	*/

	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
   // device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
 
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &g_pSwapChain,
        &g_pDevice,
        &feature_level,
        &g_pDeviceContext);

    if (FAILED(hr)) {
		MessageBox(hWnd, "Direct3DCreateDeviceAndSwapChain failed", "�G���[", MB_OK);
        return false;
    }

	if (!configureBackBuffer()) {
		MessageBox(hWnd, "failed to create backbuffer", "�G���[", MB_OK);
		return false;
	}

	// ブレンドステート設定
	D3D11_BLEND_DESC bd = {};
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = TRUE; //这里开启混合灰度才能执行下面的混合指令

	//透明blend设置

	//src source(将要绘制的图像)  dest 已经绘制了的图像（背景）

	//RGB
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; //src  source(今から描く絵（色）)　　Destすでに描れた絵
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//SrcRGB * SrcA + DestRGB*(1- SrcA)   当前的画面和下一张画面进行混合

	//A
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;     //OP(演算子)
	//SrcA * 1 + DestA *0 未使用

	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	g_pDevice->CreateBlendState(&bd, &g_BlendStateMultiply);


	/********************************************/

	/*****************加算blend设定************/

	//RGB
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	//SrcRGB * SrcA + DestRGB * 1   当前的画面和下一张画面进行混合

	//A
	//SrcA * 1 + DestA *0 未使用


	g_pDevice->CreateBlendState(&bd, &g_BlendStateAdd);


	Direct3D_SetAlphaBlendTransparent();//default blending

	D3D11_BLEND_DESC a2c{};
	a2c.AlphaToCoverageEnable = TRUE;
	a2c.RenderTarget[0].BlendEnable = FALSE;
	a2c.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	g_pDevice->CreateBlendState(&a2c, &g_BlendStateA2C);

	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = FALSE;
	dsd.DepthEnable = FALSE; // 無効にする
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthDisable);

	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthEnable);

	Direct3D_SetDepthEnable(true);

	// ラスタライザステートの作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	//rd.FillMode = D3D11_FILL_WIREFRAME; // 线框绘制 用于debug
	rd.CullMode = D3D11_CULL_BACK;
	
	//rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = true;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState);

	// デバイスコンテキストにラスタライザーステートを設定
	g_pDeviceContext->RSSetState(g_pRasterizerState);

	  return true;
}

void Direct3D_Finalize()
{
	SAFE_RELEASE(g_pDepthStencilStateDepthDisable);
	SAFE_RELEASE(g_pDepthStencilStateDepthEnable);
	SAFE_RELEASE(g_BlendStateMultiply);
	SAFE_RELEASE(g_BlendStateA2C);
	SAFE_RELEASE(g_pRasterizerState);
	SAFE_RELEASE(g_BlendStateAdd);
	releaseBackBuffer();
	
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pDeviceContext);
	SAFE_RELEASE(g_pDevice);
	SAFE_RELEASE(g_pDepthSRV);
	SAFE_RELEASE(g_pMsaaDepthSRV);
	/*
	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	if (g_pDeviceContext) {
		g_pDeviceContext->Release();
		g_pDeviceContext = nullptr;
	}
	
    if (g_pDevice) {
		g_pDevice->Release();
		g_pDevice = nullptr;
	}
	*/
}

void Direct3D_Clear()
{
	float clear_color[4] = { 0.8f, 0.4f, 0.2f, 1.0f };

	if (g_pMsaaRenderTargetView)
	{
		g_pDeviceContext->ClearRenderTargetView(g_pMsaaRenderTargetView, clear_color);
		g_pDeviceContext->ClearDepthStencilView(g_pMsaaDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		g_pDeviceContext->OMSetRenderTargets(1, &g_pMsaaRenderTargetView, g_pMsaaDepthStencilView);
	}
	else
	{
		g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
		g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	}

}//由cpu向GPU发送图形处理请求，GPU接收到后自动处理 pDeviceContext
//GPU 的if文处理很慢，但是重复操作非常的快
void Direct3D_Present()  // 
{
	// �X���b�v�`�F�[���̕\��
	if (g_pMsaaColorBuffer)
	{
		ID3D11Texture2D* back_buffer_pointer = nullptr;
		HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);
		if (SUCCEEDED(hr) && back_buffer_pointer)
		{
			g_pDeviceContext->ResolveSubresource(
				back_buffer_pointer, 0,
				g_pMsaaColorBuffer, 0,
				g_BackBufferDesc.Format
			);
			back_buffer_pointer->Release();
		}
	}

	g_pSwapChain->Present(1, 0);// 1 变为0时，解除fps限制
								//ベンチマークを取得時に
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}

unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetContext()
{
	return g_pDeviceContext;
}

void Direct3D_SetAlphaBlendTransparent()
{
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_BlendStateMultiply, blend_factor, 0xffffffff);

}

void Direct3D_SetAlphaBlendAdd()
{
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_BlendStateAdd, blend_factor, 0xffffffff);

}

void Direct3D_SetDepthEnable(bool enable)
{
	if (enable)
	{
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthEnable, NULL);
	}
	else
	{
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthDisable, NULL);

	}
}


bool Direct3D_UseMSAA() { return g_pMsaaColorBuffer != nullptr; }

ID3D11ShaderResourceView* Direct3D_GetMsaaDepthSRV()
{
	return g_pMsaaDepthSRV;
}

ID3D11RenderTargetView* Direct3D_GetMainRTV()
{
	return g_pMsaaRenderTargetView ? g_pMsaaRenderTargetView : g_pRenderTargetView;
}
ID3D11DepthStencilView* Direct3D_GetMainDSV()
{
	return g_pMsaaDepthStencilView ? g_pMsaaDepthStencilView : g_pDepthStencilView;
}

ID3D11ShaderResourceView* Direct3D_GetSceneColorCopySRV() { return g_pSceneColorCopySRV; }

ID3D11ShaderResourceView* Direct3D_GetDepthSRV()
{
	return g_pMsaaDepthSRV ? g_pMsaaDepthSRV : g_pDepthSRV;
}



void Direct3D_PrepareWaterInputs()
{
	if (!g_pSceneColorCopyTex) return;

	g_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

	if (g_pMsaaColorBuffer)
	{
		g_pDeviceContext->ResolveSubresource(
			g_pSceneColorCopyTex, 0,
			g_pMsaaColorBuffer, 0,
			g_BackBufferDesc.Format
		);
	}
	else
	{
		// backbuffer 的资源你已经保存了 g_pBackBufferTex
		g_pDeviceContext->CopyResource(g_pSceneColorCopyTex, g_pBackBufferTex);
	}
	ID3D11RenderTargetView* rtv = Direct3D_GetMainRTV();
	ID3D11DepthStencilView* dsv = Direct3D_GetMainDSV();
	g_pDeviceContext->OMSetRenderTargets(1, &rtv, dsv);
}


ID3D11ShaderResourceView* Direct3D_GetSceneDepthSRV()
{
	return g_pMsaaColorBuffer ? g_pMsaaDepthSRV : g_pDepthSRV;
}


bool configureBackBuffer()
{
	HRESULT hr;
	ID3D11Texture2D* back = nullptr;

	// 1) 取 backbuffer
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
	if (FAILED(hr) || !back) return false;

	// 2) 拿 desc（Width/Height/Format）
	back->GetDesc(&g_BackBufferDesc);

	// 3) RTV
	SAFE_RELEASE(g_pRenderTargetView);
	hr = g_pDevice->CreateRenderTargetView(back, nullptr, &g_pRenderTargetView);
	if (FAILED(hr)) { back->Release(); return false; }

	// 4) ★保存 backbuffer：拿走 back 的引用（不要 Release back）
	SAFE_RELEASE(g_pBackBufferTex);
	g_pBackBufferTex = back;     // g_pBackBufferTex 现在持有 1 个引用
	back = nullptr;              // 防止下面误 Release
	SAFE_RELEASE(g_pSceneColorCopySRV);
	SAFE_RELEASE(g_pSceneColorCopyTex);

	D3D11_TEXTURE2D_DESC sc{};
	sc.Width = g_BackBufferDesc.Width;
	sc.Height = g_BackBufferDesc.Height;
	sc.MipLevels = 1;
	sc.ArraySize = 1;
	sc.Format = g_BackBufferDesc.Format;   // R8G8B8A8_UNORM
	sc.SampleDesc.Count = 1;
	sc.SampleDesc.Quality = 0;
	sc.Usage = D3D11_USAGE_DEFAULT;
	sc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	hr = g_pDevice->CreateTexture2D(&sc, nullptr, &g_pSceneColorCopyTex);
	if (FAILED(hr)) { releaseBackBuffer(); return false; }

	hr = g_pDevice->CreateShaderResourceView(g_pSceneColorCopyTex, nullptr, &g_pSceneColorCopySRV);
	if (FAILED(hr)) return false;




	// 2) 现在才有 Format/Width/Height，开始检查 MSAA
	g_MsaaQuality = 0;
	hr = g_pDevice->CheckMultisampleQualityLevels(g_BackBufferDesc.Format, MSAA_COUNT, &g_MsaaQuality);
	bool useMsaa = SUCCEEDED(hr) && (g_MsaaQuality > 0);

	// 3) 如果支持 MSAA：创建 MSAA Color RT
	if (useMsaa)
	{
		D3D11_TEXTURE2D_DESC msaaRT{};
		msaaRT.Width = g_BackBufferDesc.Width;
		msaaRT.Height = g_BackBufferDesc.Height;
		msaaRT.MipLevels = 1;
		msaaRT.ArraySize = 1;
		msaaRT.Format = g_BackBufferDesc.Format;
		msaaRT.SampleDesc.Count = MSAA_COUNT;
		msaaRT.SampleDesc.Quality = g_MsaaQuality - 1;
		msaaRT.Usage = D3D11_USAGE_DEFAULT;
		msaaRT.BindFlags = D3D11_BIND_RENDER_TARGET;

		hr = g_pDevice->CreateTexture2D(&msaaRT, nullptr, &g_pMsaaColorBuffer);
		if (FAILED(hr)) return false;

		hr = g_pDevice->CreateRenderTargetView(g_pMsaaColorBuffer, nullptr, &g_pMsaaRenderTargetView);
		if (FAILED(hr)) return false;
	}

	
	// 4) 深度：可采样 depth（no stencil）
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthStencilView);
	SAFE_RELEASE(g_pDepthSRV);

	SAFE_RELEASE(g_pMsaaDepthBuffer);
	SAFE_RELEASE(g_pMsaaDepthStencilView);
	SAFE_RELEASE(g_pMsaaDepthSRV);

	D3D11_TEXTURE2D_DESC ds{};
	ds.Width = g_BackBufferDesc.Width;
	ds.Height = g_BackBufferDesc.Height;
	ds.MipLevels = 1;
	ds.ArraySize = 1;
	ds.Format = DXGI_FORMAT_R24G8_TYPELESS; // ★ typeless
	ds.Usage = D3D11_USAGE_DEFAULT;
	ds.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	ds.CPUAccessFlags = 0;
	ds.MiscFlags = 0;

	if (useMsaa)
	{
		ds.SampleDesc.Count = MSAA_COUNT;
		ds.SampleDesc.Quality = g_MsaaQuality - 1;

		hr = g_pDevice->CreateTexture2D(&ds, nullptr, &g_pMsaaDepthBuffer);
		if (FAILED(hr)) return false;

		// DSV
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
		dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		hr = g_pDevice->CreateDepthStencilView(g_pMsaaDepthBuffer, &dsvd, &g_pMsaaDepthStencilView);
		if (FAILED(hr)) return false;

		//ReadOnly DSV
		SAFE_RELEASE(g_pMsaaDepthStencilViewRO);
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvRO{};
		dsvRO.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvRO.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		dsvRO.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

		hr = g_pDevice->CreateDepthStencilView(g_pMsaaDepthBuffer, &dsvRO, &g_pMsaaDepthStencilViewRO);
		if (FAILED(hr)) return false;

		// SRV (Texture2DMS)
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
		srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
		hr = g_pDevice->CreateShaderResourceView(g_pMsaaDepthBuffer, &srvd, &g_pMsaaDepthSRV);
		if (FAILED(hr)) return false;
	}
	else
	{
		ds.SampleDesc.Count = 1;
		ds.SampleDesc.Quality = 0;

		hr = g_pDevice->CreateTexture2D(&ds, nullptr, &g_pDepthStencilBuffer);
		if (FAILED(hr)) return false;

		// DSV
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
		dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvd.Texture2D.MipSlice = 0;
		hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvd, &g_pDepthStencilView);
		if (FAILED(hr)) return false;

		//在这里加 ReadOnly DSV
		SAFE_RELEASE(g_pDepthStencilViewRO);
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvRO{};
		dsvRO.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvRO.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvRO.Texture2D.MipSlice = 0;
		dsvRO.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

		hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvRO, &g_pDepthStencilViewRO);
		if (FAILED(hr)) return false;

		// SRV (Texture2D)
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
		srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MostDetailedMip = 0;
		srvd.Texture2D.MipLevels = 1;
		hr = g_pDevice->CreateShaderResourceView(g_pDepthStencilBuffer, &srvd, &g_pDepthSRV);
		if (FAILED(hr)) return false;
	}

	// 5) viewport
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = (FLOAT)g_BackBufferDesc.Width;
	g_Viewport.Height = (FLOAT)g_BackBufferDesc.Height;
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;
	g_pDeviceContext->RSSetViewports(1, &g_Viewport);

	return true;
}


void releaseBackBuffer()
{
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthStencilView);

	SAFE_RELEASE(g_pMsaaRenderTargetView);
	SAFE_RELEASE(g_pMsaaColorBuffer);
	SAFE_RELEASE(g_pMsaaDepthStencilView);
	SAFE_RELEASE(g_pMsaaDepthBuffer);
	SAFE_RELEASE(g_pSceneColorCopySRV);
	SAFE_RELEASE(g_pSceneColorCopyTex);
	SAFE_RELEASE(g_pBackBufferTex);

	SAFE_RELEASE(g_pDepthSRV);
	SAFE_RELEASE(g_pMsaaDepthSRV);
	SAFE_RELEASE(g_pDepthStencilViewRO);
	SAFE_RELEASE(g_pMsaaDepthStencilViewRO);
	/*
	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
	}

	if (g_pDepthStencilBuffer) {
		g_pDepthStencilBuffer->Release();
		g_pDepthStencilBuffer = nullptr;
	}

	if (g_pDepthStencilView) {
		g_pDepthStencilView->Release();
		g_pDepthStencilView = nullptr;
	}
	*/
}

void Direct3D_SetAlphaToCoverage()
{
	float bf[4] = { 0,0,0,0 };
	g_pDeviceContext->OMSetBlendState(g_BlendStateA2C, bf, 0xFFFFFFFF);
}

