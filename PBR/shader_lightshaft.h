#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

struct LightShaftParams
{
    // 方向光方向（和你 Light_SetDirectional 传进去的 dir 一样：光线传播方向）
    DirectX::XMFLOAT3 lightDirW{ 0,-1,0 };

    float intensity = 1.0f;     // 总强度（最后合成前乘）
    float skyDepthEps = 0.9999f;  // depth>=eps 认为是天空（没有几何）

    // 太阳盘大小（越接近1越小）
    float sunThreshold = 0.995f;  // 0.99~0.999
    float sunPower = 128.0f;  // 32~256

    // 径向采样参数（经典 godrays）
    float density = 0.9f;   // 0.6~1.2
    float decay = 0.96f;  // 0.90~0.99
    float weight = 0.35f;  // 0.1~0.6
    float exposure = 0.8f;   // 0.2~2.0

    // 低分辨率RT：2=1/2分辨率，4=1/4分辨率（推荐 2 或 4）
    int downsample = 2;
};

namespace ShaderLightShaft
{
    bool Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    void Finalize();

    // t41 depth + 是否MSAA（用来选 mask PS / mask_msaa PS）
    void SetSceneDepthSRV(ID3D11ShaderResourceView* depthSRV, bool depthIsMSAA);

    // 每帧调用（建议在不透明物体都画完、透明之前）
    void Draw(const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        const LightShaftParams& p);
}
