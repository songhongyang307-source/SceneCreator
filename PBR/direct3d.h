/*==============================================================================

   Direct3D�̏������֘A [direct3d.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H


#include <Windows.h>
#include <d3d11.h>

// �Z�[�t�����[�X�}�N��
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }

ID3D11DepthStencilView* Direct3D_GetMainDSV_ReadOnly();

bool Direct3D_Initialize(HWND hWnd); // Direct3D�̏�����
void Direct3D_Finalize(); // Direct3D�̏I������

void Direct3D_Clear(); // �o�b�N�o�b�t�@�̃N���A
void Direct3D_Present(); // �o�b�N�o�b�t�@�̕\��

void Direct3D_SetAlphaToCoverage();
//得到缓冲区buffer的大小
unsigned int Direct3D_GetBackBufferWidth();
unsigned int Direct3D_GetBackBufferHeight();

// Get  Direct3D device
ID3D11Device* Direct3D_GetDevice();

// Get  Direct3D device context  
ID3D11DeviceContext* Direct3D_GetContext();

void Direct3D_SetAlphaBlendTransparent(); //透明处理
void Direct3D_SetAlphaBlendAdd();   //加算合成

//depth buffer setting
void Direct3D_SetDepthEnable(bool enable);



ID3D11ShaderResourceView* Direct3D_GetSceneDepthSRV();
ID3D11ShaderResourceView* Direct3D_GetSceneColorCopySRV();
void Direct3D_PrepareWaterInputs();

ID3D11RenderTargetView* Direct3D_GetMainRTV();      // MSAA 就返回 msaaRTV，否则 backbufferRTV
ID3D11DepthStencilView* Direct3D_GetMainDSV();      // MSAA 就返回 msaaDSV，否则 dsv
ID3D11ShaderResourceView* Direct3D_GetSceneColorCopySRV(); // t40
ID3D11ShaderResourceView* Direct3D_GetDepthSRV();          // t41 (MSAA depth SRV)
void Direct3D_PrepareWaterInputs();                         // Resolve/Copy scene color 到 copyTex
bool Direct3D_UseMSAA();
ID3D11ShaderResourceView* Direct3D_GetMsaaDepthSRV();

#endif // DIRECT3D_H
