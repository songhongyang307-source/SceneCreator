#pragma once

#include <unordered_map>
#include <DirectXMath.h>
#include <d3d11.h>
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h"
using namespace DirectX;

struct PBRMaterial
{
    int TexAlbedo = -1;
    int TexNormal = -1;
    int TexMetallic = -1;
    int TexRoughness = -1;
    int TexAo = -1;
    XMFLOAT4 BaseColorTint{1.0f , 1.0f , 1.0f , 1.0f};

    float NormalFactor = 1.0f;
    float MetallicFactor = 1.0f;
    float RoughnessFactor = 1.0f;
    float AoFactor = 1.0f;

    bool  UseAlphaToCoverage = false;
    float AlphaCutoff = 0.1f;
};

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer = nullptr;
	ID3D11Buffer** IndexBuffer = nullptr;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	AABB local_aabb;

    PBRMaterial* materials = nullptr;
};

template<class T>
T Clamp(T v, T lo, T hi)
{
    return std::max(lo, std::min(v, hi));
}

    
AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3 position);

MODEL* ModelLoad(const char* FileName, bool isblender = false, float scalar = 1.0 , bool isclockwise = true);
void ModelRelease(MODEL* model);

void ModelDraw(MODEL* model , const DirectX::XMMATRIX& mtxWorld);

void ModelDrawUnlit(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

void ModelDrawShadowRaw(MODEL* model);