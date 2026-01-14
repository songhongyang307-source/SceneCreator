/*==============================================================================

   Solar Display [sky.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/



#include <Windows.h>
#include "sky.h"
#include "direct3d.h"
using namespace DirectX;
#include "model.h"
#include "shader3d_unlit.h"
#include "WICTextureLoader11.h"
#include "texture.h"
#include "d3d11.h"
static MODEL* g_pSkyDome{};
static XMFLOAT3 g_Position{};
static int      g_SkyTex = -1;


struct Vertex3d_Sky
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texcoord;
};


static std::wstring ToWStringFromUtf8(const std::string& s)
{
    if (s.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";

    std::wstring ws;
    ws.resize(len);                 // 分配可写空间（包含 '\0'）
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);

    if (!ws.empty() && ws.back() == L'\0') ws.pop_back(); // 去掉结尾的 '\0'
    return ws;
}

static std::wstring GetFolderFromFileName(const char* file)
{
    std::string s = file ? file : "";
    size_t p = s.find_last_of("/\\");
    std::string folder = (p == std::string::npos) ? "" : s.substr(0, p + 1);
    return ToWStringFromUtf8(folder);
}

static int Sky_TryLoadDiffuseFromFbx(MODEL* model, const char* modelPath)
{
    if (!model || !model->AiScene) return -1;

    std::wstring baseDir = GetFolderFromFileName(modelPath);

    // 取第一个 mesh 的材质（天空一般就一个材质，够用）
    aiMesh* mesh = model->AiScene->mMeshes[0];
    aiMaterial* mat = model->AiScene->mMaterials[mesh->mMaterialIndex];

    aiString texPath;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS) return -1;
    if (texPath.length == 0) return -1;

    std::string p = texPath.C_Str();
    if (!p.empty() && p[0] == '*') return -1; // 内嵌贴图先跳过（你现在这套文件加载读不了）

    std::wstring w = ToWStringFromUtf8(p);

    // 相对路径 → 拼到 fbx 同目录
    bool isAbs = (w.size() >= 2 && w[1] == L':') || (!w.empty() && (w[0] == L'/' || w[0] == L'\\'));
    std::wstring full = isAbs ? w : (baseDir + w);

    // 天空贴图当作 sRGB
    return Texture_TryLoad(full.c_str(), true);
}

void Sky_Initialize()
{
    g_pSkyDome = ModelLoad("resource/mesh/sky.fbx", true, 50.0f);

    // 优先读 FBX 里指定的 diffuse
    g_SkyTex = Sky_TryLoadDiffuseFromFbx(g_pSkyDome, "resource/mesh/sky.fbx");

    // 没有的话就用你自己的默认天空图（你自己准备一张）
    //if (g_SkyTex < 0)
       // g_SkyTex = Texture_TryLoad(L"resource/texture/sky.png", true);

    
}

static void ModelDrawUnlit_ForceTexture(MODEL* model, const XMMATRIX& mtxWorld, int texId)
{
    if (!model || !model->AiScene) return;

    Shader3dUnlit_Begin();
    Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Shader3dUnlit_SetWorldMatrix(mtxWorld);

    // 绑定天空贴图到 slot0
    if (texId >= 0) Texture_SetTexture(texId, 0);
    Shader3dUnlit_SetColor({ 1,1,1,1 });

    for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
    {
        UINT stride = sizeof(Vertex3d_Sky);
        UINT offset = 0;
        Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);
        Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
    }
}

void Sky_Finalize(void)
{
	ModelRelease(g_pSkyDome);
}

void Sky_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}


void Sky_Draw()
{
    if (!g_pSkyDome) return;

    XMFLOAT3 pos = g_Position;
    pos.y = 0.0f; // 你想锁地平线就留着
    ModelDrawUnlit_ForceTexture(g_pSkyDome, XMMatrixTranslationFromVector(XMLoadFloat3(&pos)), g_SkyTex);
}
