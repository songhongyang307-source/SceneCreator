#pragma once
#include "DirectXMath.h"
#include "direct3d.h"

struct CB_Wind
{
    DirectX::XMFLOAT3 windDirW;   float windStrength;
    float windSpeed;             float windFreq;
    float gustFreq;              float time;
    float flutter;               float pad0[3];   // 补齐到16字节倍数
};
static ID3D11Buffer* g_cbWind = nullptr;