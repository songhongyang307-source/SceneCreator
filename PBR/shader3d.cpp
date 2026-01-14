/*==============================================================================

   3d shader [shader3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

#include "shader3d .h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "sampler.h"
#include "texture.h"
#include "DDSTextureLoader11.h"
#include "wind.h"





static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;//VERTEX BUFFER0 world
static ID3D11Buffer* g_pVSWindCB = nullptr;
//static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr;//VERTEX BUFFER1 view
//static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr;//VERTEX BUFFER2 projection(perspective)
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr;
static ID3D11Buffer* g_pPSConstantBuffer5 = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11Buffer* g_pPSConstantBuffer8 = nullptr; // IBL CB (b8)
// Release不要！！！
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11ShaderResourceView* g_pIrradianceSRV = nullptr; // t30
static ID3D11ShaderResourceView* g_pPrefilterSRV = nullptr; // t31
static ID3D11ShaderResourceView* g_pBRDFLUTSRV = nullptr; // t32
static float g_PrefilterMaxMip = 0.0f;
static ID3D11SamplerState* g_pSamplerIBLLinearClamp = nullptr;

static bool g_IBLBoundThisFrame = false;
static bool g_Shader3dBegunThisFrame = false;

static int g_TexWhite = -1;      // (1,1,1)
static int g_TexBlack = -1;      // (0,0,0)
static int g_TexFlatNormal = -1; // (0.5,0.5,1)

static CB_Wind g_wind{};

struct IBL_CB
{
	float PrefilterMaxMip;
	float EnvIntensity;
	float pad[2];
} cb{};

void DBG(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);

	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
}

void Shader3d_BeginFrame()
{
	// 每帧开始时调用一次，重置标记
	g_IBLBoundThisFrame = false;
	g_Shader3dBegunThisFrame = false;

}
void Shader3d_BeginPassOnce()
{
	if (!g_Shader3dBegunThisFrame)
	{
		g_Shader3dBegunThisFrame = true;

		Shader3d_Begin(); // 你现在这个 Begin：设VS/PS/CB/sampler
		Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// IBL 绑定也只做一次（你已有这个函数）
	Shader3d_BindIBL_Once();
}

// 只绑定一次：t30/t31/t32 + b8
void Shader3d_BindIBL_Once()
{
	if (g_IBLBoundThisFrame) return;
	g_IBLBoundThisFrame = true;

	// IBL sampler (s2)
	Sampler_SetIBLClamp();

	// IBL SRV (t30~t32)
	if (g_pIrradianceSRV) Texture_SetTextureSRV(g_pIrradianceSRV, 30);
	if (g_pPrefilterSRV)  Texture_SetTextureSRV(g_pPrefilterSRV, 31);
	if (g_pBRDFLUTSRV)    Texture_SetTextureSRV(g_pBRDFLUTSRV, 32);

	// IBL 常量（如果你每帧都固定值，也可以只在参数变化时才调用）
	Shader3d_SetIBLParams(g_PrefilterMaxMip, 0.05f);

	DBG("IBL bound this frame");
}



static void DumpSRVTex2D(ID3D11ShaderResourceView* srv, const char* name)
{
	if (!srv) { DBG("%s = null", name); return; }

	ID3D11Resource* res = nullptr;
	srv->GetResource(&res);

	ID3D11Texture2D* tex = nullptr;
	if (res) res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

	if (tex)
	{
		D3D11_TEXTURE2D_DESC d{};
		tex->GetDesc(&d);
		DBG("%s: %ux%u mip=%u fmt=%u", name, d.Width, d.Height, d.MipLevels, (unsigned)d.Format);
		tex->Release();
	}
	if (res) res->Release();
}

static void DumpSRV(ID3D11ShaderResourceView* srv, const char* name)
{
	if (!srv)
	{
		DBG("%s = null", name);
		return;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC d{};
	srv->GetDesc(&d);

	DBG("%s dim=%d format=%d", name, (int)d.ViewDimension, (int)d.Format);
}





static float GetMaxMipFromSRV(ID3D11ShaderResourceView* srv)
{
	if (!srv) return 0.0f;

	ID3D11Resource* res = nullptr;
	srv->GetResource(&res);

	ID3D11Texture2D* tex = nullptr;
	if (res) res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

	float maxMip = 0.0f;
	if (tex)
	{
		D3D11_TEXTURE2D_DESC d{};
		tex->GetDesc(&d);
		maxMip = (d.MipLevels > 0) ? float(d.MipLevels - 1) : 0.0f;
		tex->Release();
	}
	if (res) res->Release();
	return maxMip;
}

static void LoadIBL()
{
	DBG("LoadIBL() enter");

	auto LoadDDS_SRV = [&](const wchar_t* path, ID3D11ShaderResourceView** outSRV, size_t maxSize)
		{
			SAFE_RELEASE(*outSRV);

			HRESULT hr = DirectX::CreateDDSTextureFromFileEx(
				g_pDevice, g_pContext,
				path,
				maxSize, // ★ 关键：限制最大边长（会自动选择合适 mip 来加载）
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
				0, 0,
				DDS_LOADER_IGNORE_SRGB,
				nullptr, outSRV, nullptr
			);
			return hr;
		};

	HRESULT hr1 = LoadDDS_SRV(L"resource/texture/IBL/out/My_EnvDiffuseHDR.dds", &g_pIrradianceSRV, 64);
	HRESULT hr2 = LoadDDS_SRV(L"resource/texture/IBL/out/My_EnvSpecularHDR.dds", &g_pPrefilterSRV, 512);
	HRESULT hr3 = LoadDDS_SRV(L"resource/texture/IBL/out/My_EnvBrdf.dds", &g_pBRDFLUTSRV, 512);
	

	g_PrefilterMaxMip = GetMaxMipFromSRV(g_pPrefilterSRV);
	DumpSRVTex2D(g_pIrradianceSRV, "Irradiance(out)");
	DumpSRVTex2D(g_pPrefilterSRV, "Prefilter(out)");
	DumpSRVTex2D(g_pBRDFLUTSRV, "BRDF(out)");
}


void Shader3d_InitPBRUtilityTextures()
{

}

bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	HRESULT hr; // �߂�l�i�[�p

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̃`�F�b�N
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : �^����ꂽ�f�o�C�X���R���e�L�X�g���s���ł�" << std::endl;
		return false;
	}

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;

	
	g_TexWhite = Texture_Load(L"resource/texture/white.png",false);
	g_TexBlack = Texture_Load(L"resource/texture/Metallic0.png", false);
	g_TexFlatNormal = Texture_Load(L"resource/texture/FlatNormal.jpg", false);
	
	
	Shader3d_InitPBRUtilityTextures();




	// ���O�R���p�C���ςݒ��_�V�F�[�_�[�̓ǂݍ���
	std::ifstream ifs_vs("shader_vertex_3d.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "failed to read \n\nshader_vertex_3d.cso", "error", MB_OK);
		return false;
	}

	// �t�@�C���T�C�Y��擾
	ifs_vs.seekg(0, std::ios::end); // �t�@�C���|�C���^�𖖔��Ɉړ�
	std::streamsize filesize = ifs_vs.tellg(); // �t�@�C���|�C���^�̈ʒu��擾�i�܂�t�@�C���T�C�Y�j
	ifs_vs.seekg(0, std::ios::beg); // �t�@�C���|�C���^��擪�ɖ߂�

	// �o�C�i���f�[�^��i�[���邽�߂̃o�b�t�@��m��
	unsigned char* vsbinary_pointer = new unsigned char[filesize];
	
	ifs_vs.read((char*)vsbinary_pointer, filesize); // �o�C�i���f�[�^��ǂݍ���
	ifs_vs.close(); // �t�@�C�������

	// ���_�V�F�[�_�[�̍쐬
	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ���_�V�F�[�_�[�̍쐬�Ɏ��s���܂���" << std::endl;
		delete[] vsbinary_pointer; // ���������[�N���Ȃ��悤�Ƀo�C�i���f�[�^�̃o�b�t�@����
		return false;
	}


	// vertex layout   
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	};

	UINT num_elements = ARRAYSIZE(layout); // �z��̗v�f����擾

	// ���_���C�A�E�g�̍쐬
	hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // �o�C�i���f�[�^�̃o�b�t�@����

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ���_���C�A�E�g�̍쐬�Ɏ��s���܂���" << std::endl;
		return false;
	}

	


	// 顶点buffer 制作
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // size of buffer
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // �o�C���h�t���O

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);

	// ���O�R���p�C���ς݃s�N�Z���V�F�[�_�[�̓ǂݍ���
	std::ifstream ifs_ps("Shader_Pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "failed to read \n\nshader_pixel_3d.cso", "error", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// �s�N�Z���V�F�[�_�[�̍쐬
	hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // �o�C�i���f�[�^�̃o�b�t�@����

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : �s�N�Z���V�F�[�_�[�̍쐬�Ɏ��s���܂���" << std::endl;
		return false;
	}




	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.ByteWidth = (UINT)((sizeof(CB_Wind) + 15) & ~15);

		hr = g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSWindCB);
		if (FAILED(hr)) return false;

		// 给个默认值，避免未初始化导致 NaN
		g_wind.windDirW = { 1,0,0 };
		g_wind.windStrength = 0.25f;
		g_wind.windSpeed = 1.0f;
		g_wind.windFreq = 0.12f;
		g_wind.gustFreq = 0.6f;
		g_wind.time = 0.0f;
		g_wind.flutter = 0.2f;
		g_pContext->UpdateSubresource(g_pVSWindCB, 0, nullptr, &g_wind, 0, 0);
	}



	//pixel shader  constant value buffer make
	
	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // size of buffer

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.ByteWidth = sizeof(XMFLOAT4);
	bd.CPUAccessFlags = 0;

	hr = pDevice->CreateBuffer(&bd, nullptr, &g_pPSConstantBuffer5);
	if (FAILED(hr)) return false;

	D3D11_BUFFER_DESC bd8{};
	bd8.Usage = D3D11_USAGE_DEFAULT;
	bd8.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd8.ByteWidth = sizeof(IBL_CB); // 16 bytes
	bd8.CPUAccessFlags = 0;

	hr = pDevice->CreateBuffer(&bd8, nullptr, &g_pPSConstantBuffer8);
	if (FAILED(hr)) return false;
	


	{
		D3D11_SAMPLER_DESC sd{};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.MaxAnisotropy = 1;
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sd.MinLOD = 0;
		sd.MaxLOD = D3D11_FLOAT32_MAX;

		hr = g_pDevice->CreateSamplerState(&sd, &g_pSamplerIBLLinearClamp);
		if (FAILED(hr)) return false;
	}

	LoadIBL();
	g_PrefilterMaxMip = GetMaxMipFromSRV(g_pPrefilterSRV);
	Shader3d_SetIBLParams(g_PrefilterMaxMip, 1.0f);
	
	return true;
}

void Shader3d_Finalize()
{
	SAFE_RELEASE(g_pIrradianceSRV);
	SAFE_RELEASE(g_pPrefilterSRV);
	SAFE_RELEASE(g_pBRDFLUTSRV);

	SAFE_RELEASE(g_pPSConstantBuffer8);
	SAFE_RELEASE(g_pPSConstantBuffer5);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pVSWindCB);
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
	if (g_pSamplerIBLLinearClamp) { g_pSamplerIBLLinearClamp->Release(); g_pSamplerIBLLinearClamp = nullptr; }
}

void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix)//shader にセットする
{
	// 定值buffer存储用 数列构造体的定义
	XMFLOAT4X4 transpose;

	// �s���]�u���Ē萔�o�b�t�@�i�[�p�s��ɕϊ�
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定制buffer设置为矩阵
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

//view and perspective is moved to camera.cpp

void Shader3d_SetColor(const DirectX::XMFLOAT4& color)
{
	g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);

}


void Shader3d_Begin()
{
	// VS/PS
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	g_pContext->IASetInputLayout(g_pInputLayout);

	if (g_pVSWindCB)
		g_pContext->VSSetConstantBuffers(7, 1, &g_pVSWindCB);

	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);
	g_pContext->PSSetConstantBuffers(5, 1, &g_pPSConstantBuffer5);
	g_pContext->PSSetConstantBuffers(8, 1, &g_pPSConstantBuffer8);

	// ===== Sampler：明确分工 =====
	Sampler_SetFilterAnisotropic();             // 让它只管 s0（材质用）
	g_pContext->PSSetSamplers(2, 1, &g_pSamplerIBLLinearClamp); // s2（IBL专用，线性clamp）

	

	
}



void Shader3d_SetPBRMaterialParams(const PBRMaterial& mat)
{    // --- 绑定材质贴图（保证每个 slot 都一定有值） ---
	// t0 Albedo
	if (mat.TexAlbedo >= 0) Texture_SetTexture(mat.TexAlbedo, 0);
	else                    Texture_SetTexture(g_TexWhite, 0);

	// t1 Normal
	if (mat.TexNormal >= 0) Texture_SetTexture(mat.TexNormal, 1);
	else                    Texture_SetTexture(g_TexFlatNormal, 1);

	// t2 Metallic
	if (mat.TexMetallic >= 0) Texture_SetTexture(mat.TexMetallic, 2);
	else                      Texture_SetTexture(g_TexBlack, 2);

	// t3 Roughness
	if (mat.TexRoughness >= 0) Texture_SetTexture(mat.TexRoughness, 3);
	else                       Texture_SetTexture(g_TexWhite, 3); // roughness=1

	// t4 AO
	if (mat.TexAo >= 0) Texture_SetTexture(mat.TexAo, 4);
	else                Texture_SetTexture(g_TexWhite, 4); // ao=1

	// --- 更新材质因子常量 b5 ---
	struct MaterialCB
	{
		float NormalFactor;
		float MetallicFactor;
		float RoughnessFactor;
		float AoFactor;
	} cbMat{};

	cbMat.NormalFactor = mat.NormalFactor;
	cbMat.MetallicFactor = mat.MetallicFactor;
	cbMat.RoughnessFactor = mat.RoughnessFactor;
	cbMat.AoFactor = mat.AoFactor;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer5, 0, nullptr, &cbMat, 0, 0);

	// --- 重新绑 IBL（防止被别处覆盖）---
	if (g_pIrradianceSRV) Texture_SetTextureSRV(g_pIrradianceSRV, 30);
	if (g_pPrefilterSRV)  Texture_SetTextureSRV(g_pPrefilterSRV, 31);
	if (g_pBRDFLUTSRV)    Texture_SetTextureSRV(g_pBRDFLUTSRV, 32);

}
void Shader3d_SetIBLParams(float prefilterMaxMip, float envIntensity)
{
	

	cb.PrefilterMaxMip = prefilterMaxMip;
	cb.EnvIntensity = envIntensity;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer8, 0, nullptr, &cb, 0, 0);
}

ID3D11ShaderResourceView* Shader3d_GetIrradianceSRV() { return g_pIrradianceSRV; }
ID3D11ShaderResourceView* Shader3d_GetPrefilterSRV() { return g_pPrefilterSRV; }
ID3D11ShaderResourceView* Shader3d_GetBRDFLUTSRV() { return g_pBRDFLUTSRV; }
float Shader3d_GetPrefilterMaxMip() { return g_PrefilterMaxMip; }
ID3D11Buffer* Shader3d_GetIBLConstantBuffer() { return g_pPSConstantBuffer8; }

void Shader3d_SetWindParams(const DirectX::XMFLOAT3& dirW, float strength, float speed, float freq, float gustFreq, float time, float flutter)
{
		if (!g_pContext || !g_pVSWindCB) return;

		g_wind.windDirW = dirW;
		g_wind.windStrength = strength;
		g_wind.windSpeed = speed;
		g_wind.windFreq = freq;
		g_wind.gustFreq = gustFreq;
		g_wind.time = time;
		g_wind.flutter = flutter;

		g_pContext->UpdateSubresource(g_pVSWindCB, 0, nullptr, &g_wind, 0, 0);
}
