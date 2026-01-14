#include "sampler.h"
#include "direct3d.h"



static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


static ID3D11SamplerState* g_pSamplerFilterPoint = nullptr;
static ID3D11SamplerState* g_pSamplerFilterLinear = nullptr;
static ID3D11SamplerState* g_pSamplerFilterAnisotropic = nullptr;

static ID3D11SamplerState* g_pSamplerIBLLinearClamp = nullptr; 

void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	//sampler_state setting
	D3D11_SAMPLER_DESC sampler_desc{};

	//filtering setting up										
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; //质量最高的渲染方式

	//UV参照外的取り扱い(UV addressing mode)
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	


	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterPoint);

	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterLinear);

	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterAnisotropic);


	// IBL / BRDF LUT 用：Linear + Clamp
	D3D11_SAMPLER_DESC iblDesc{};
	iblDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	iblDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	iblDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	iblDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	iblDesc.MipLODBias = 0;
	iblDesc.MaxAnisotropy = 1;
	iblDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	iblDesc.MinLOD = 0;
	iblDesc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pDevice->CreateSamplerState(&iblDesc, &g_pSamplerIBLLinearClamp);


}


void Sampler_SetIBLClamp()
{
	g_pContext->PSSetSamplers(2, 1, &g_pSamplerIBLLinearClamp); // slot = 2 => s2
}


void Sampler_Finalize(void)
{
	SAFE_RELEASE(g_pSamplerIBLLinearClamp);
	SAFE_RELEASE(g_pSamplerFilterPoint);
	SAFE_RELEASE(g_pSamplerFilterLinear);
	SAFE_RELEASE(g_pSamplerFilterAnisotropic);
}

void Sampler_SetFilterPoint()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterPoint);
}

void Sampler_SetFilterLinear()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterLinear);
}

void Sampler_SetFilterAnisotropic()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterAnisotropic);
}


