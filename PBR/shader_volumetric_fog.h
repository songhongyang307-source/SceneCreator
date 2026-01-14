#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

struct FogParams
{
    DirectX::XMFLOAT3 camPosW{ 0,0,0 };
    float time = 0.0f;

    DirectX::XMFLOAT3 fogColor{ 0.6f,0.7f,0.8f };
    float fogDensityNear = 0.0f;

    float fogDensityFar = 0.02f;
    float densityRampPow = 2.0f;
    float fogStart = 0.0f;
    float fogEnd = 120.0f;

    float anisotropyG = 0.6f;
    float heightFalloff = 0.0f;

    DirectX::XMFLOAT3 lightDirW{ 0,-1,0 };
    float pad0 = 0.0f;
    DirectX::XMFLOAT3 lightColor{ 1,1,1 };
    float lightIntensity = 1.0f;
};

namespace ShaderFog
{
    bool Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    void Finalize();

    void SetSceneSRV(ID3D11ShaderResourceView* sceneColorCopySRV,
        ID3D11ShaderResourceView* sceneDepthSRV,
        bool depthIsMSAA);

    void SetParams(const FogParams& p);

    // 只需要 proj（用来算 invProj）
    void Draw(const DirectX::XMMATRIX& proj);
}
