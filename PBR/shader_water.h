#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

struct WaterParams
{
    XMMATRIX invProj;
    XMFLOAT3 eyePosW;
    float    time = 0.0f;

    XMFLOAT3 absorptionRGB = { 0.15f, 0.06f, 0.03f };
    float    refractStrength = 0.006f;

    XMFLOAT2 texelSize = { 1.0f / 1280.0f, 1.0f / 720.0f };

    float roughness = 0.05f;
    float foamFadeDist = 0.6f;
    float envIntensityWater = 1.2f;
    float foamStrength = 0.15f;

    XMFLOAT2 n0Tiling = { 1.0f, 1.0f };
    XMFLOAT2 n0Speed = { 0.03f, 0.01f };
    XMFLOAT2 n1Tiling = { 2.0f, 2.0f };
    XMFLOAT2 n1Speed = { -0.02f, 0.015f };

    float nearZ = 0.5f;     // ★加默认
    float farZ = 100.0f;   // ★加默认（与你投影一致）
};

bool ShaderWater_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx);
void ShaderWater_Finalize();

void ShaderWater_SetNormalTextures(int texN0, int texN1);
void ShaderWater_SetSceneSRV(ID3D11ShaderResourceView* sceneColorCopy, ID3D11ShaderResourceView* sceneDepth, bool depthIsMSAA);
void ShaderWater_SetIBL(ID3D11ShaderResourceView* prefilter, ID3D11ShaderResourceView* brdfLut, float prefilterMaxMip);

void ShaderWater_SetParams(const WaterParams& p);

// world：每个物体不同；proj：给 invProj 用（和你 camera 的 proj 一致）
void ShaderWater_Begin(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj);
void ShaderWater_End();