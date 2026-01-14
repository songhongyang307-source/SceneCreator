
/*============================================================================================
//テクスチャ管理[texture.cpp]



---------------------------------------------------------------------------------------------


============================================================================================ */
#include "texture.h"
#include <string>
#include "direct3d.h"
#include "WICTextureLoader11.h"
#include "DDSTextureLoader11.h"
using namespace DirectX;


//管理最大数
static constexpr int TEXTURE_MAX = 2048;




struct Texture
{
	std::wstring filename;
	
	ID3D11ShaderResourceView* pTextureView = nullptr;
	unsigned int width = 0;
	unsigned int height = 0;
    unsigned int mipLevels = 1;

	ID3D11Resource* pTexture = nullptr;
    
    bool isSRGB = false;
};



static Texture g_Textures[TEXTURE_MAX]{};
static unsigned int g_SetTextureIndex = unsigned int (-1) ; //减轻运行压力


static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static bool EndsWithNoCase(const std::wstring& s, const std::wstring& suffix)
{
    if (s.size() < suffix.size()) return false;
    size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        wchar_t a = towlower(s[off + i]);
        wchar_t b = towlower(suffix[i]);
        if (a != b) return false;
    }
    return true;
}


static int Texture_LoadImpl(const wchar_t* pFilename, bool forceSRGB, bool showError)
{
    for (int i = 0; i < TEXTURE_MAX; i++)
    {
        if (g_Textures[i].filename == pFilename &&
            g_Textures[i].isSRGB == forceSRGB)
        {
            return i;
        }
    }

    for (int i = 0; i < TEXTURE_MAX; i++)
    {
        if (g_Textures[i].pTexture) continue;

        HRESULT hr = E_FAIL;

        const std::wstring file = pFilename;
        const bool isDDS = EndsWithNoCase(file, L".dds");

        if (isDDS)
        {
            DDS_LOADER_FLAGS flags =
                forceSRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_IGNORE_SRGB;

            hr = CreateDDSTextureFromFileEx(
                g_pDevice, g_pContext, pFilename,
                0,
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE,
                0, 0,
                flags,
                &g_Textures[i].pTexture,
                &g_Textures[i].pTextureView
            );
        }
        else
        {
            WIC_LOADER_FLAGS flags =
                forceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_IGNORE_SRGB;

            hr = CreateWICTextureFromFileEx(
                g_pDevice, g_pContext, pFilename,
                0,
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE,
                0, 0,
                flags,
                &g_Textures[i].pTexture,
                &g_Textures[i].pTextureView
            );
        }

        // ✅ 关键：失败就别写缓存信息，直接找下一个槽位
        if (FAILED(hr) || !g_Textures[i].pTextureView)
        {
            // 保险：避免残留（一般这里应该还是 nullptr，但写了更稳）
            SAFE_RELEASE(g_Textures[i].pTexture);
            SAFE_RELEASE(g_Textures[i].pTextureView);

            if (showError)
            {
                // 你项目里想怎么报错都行：MessageBox / debug_ostream
                // MessageBoxW(nullptr, pFilename, L"Texture load failed", MB_OK);
            }
            continue;
        }

        // ✅ 成功才写 filename/isSRGB
        g_Textures[i].filename = pFilename;
        g_Textures[i].isSRGB = forceSRGB;

        // 读取宽高 / mipLevels
        g_Textures[i].width = 0;
        g_Textures[i].height = 0;
        g_Textures[i].mipLevels = 1;

        ID3D11Texture2D* tex2D = nullptr;
        if (SUCCEEDED(g_Textures[i].pTexture->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex2D)))
        {
            D3D11_TEXTURE2D_DESC desc{};
            tex2D->GetDesc(&desc);
            g_Textures[i].width = desc.Width;
            g_Textures[i].height = desc.Height;
            g_Textures[i].mipLevels = desc.MipLevels;
            tex2D->Release();
        }

        return i; // ✅ 成功立即返回
    }

    // 没有空槽位 / 全部加载失败
    if (showError)
    {
        // MessageBoxW(nullptr, pFilename, L"Texture slots full or load failed", MB_OK);
    }
    return -1;
}
void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	for (Texture& t : g_Textures) {

        t.filename.clear();
        t.pTexture = nullptr;
        t.pTextureView = nullptr;
        t.width = 0;
        t.height = 0;
        t.mipLevels = 1;
        t.isSRGB = false;

	}

	g_SetTextureIndex = unsigned int (-1);

	//save device and devicecontext 

	g_pDevice = pDevice;

	g_pContext = pContext;

}

void Texture_Finalize(void)
{
	
	Texture_AllRelease();

}

unsigned int Texture_MipLevels(int texid)
{
    if (texid < 0) return 0;
    return g_Textures[texid].mipLevels;
}

int Texture_Load(const wchar_t* pFilename , bool forceSRGB)
{
    return Texture_LoadImpl(pFilename, forceSRGB, true);
}

int Texture_TryLoad(const wchar_t* pFilename, bool forceSRGB)
{
    return Texture_LoadImpl(pFilename, forceSRGB, false);
}

void Texture_AllRelease() {


		for (Texture& t : g_Textures) 
		{

			t.filename.clear();
            t.mipLevels = 1;
			SAFE_RELEASE(t.pTexture);
			SAFE_RELEASE(t.pTextureView);

		}


}



void Texture_SetTexture(int texid , int slot)
{


	if (texid < 0) return ; //如果访问的数组位置超出了头指针，直接返回

	//if (g_SetTextureIndex == texid) return;


	g_SetTextureIndex = texid;



	//texture setting up
	g_pContext->PSSetShaderResources(slot, 1, &g_Textures[texid].pTextureView);



}

void Texture_SetTextureSRV(ID3D11ShaderResourceView* pSRV, UINT slot)
{
    ID3D11ShaderResourceView* srvs[1] = { pSRV };
    g_pContext->PSSetShaderResources(slot, 1, srvs);
}

float Texture_Width(int texid)
{

	if (texid < 0) return 0;


	return float(g_Textures[texid].width);


}

float Texture_Height(int texid)
{
	if (texid < 0) return 0;


	return float(g_Textures[texid].height);


}
