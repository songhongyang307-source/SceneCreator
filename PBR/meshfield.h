/*==============================================================================

   3d CUBE visualize[cube.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once


#ifndef MESHFIELD_H
#define MESHFIELD_H


#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>


struct ControlPixel
{
	uint8_t r, g, b, a;

};


#define CONTROL_W 512
#define CONTROL_H 512



// CPU / GPU 端 control map 的“声明”（注意 extern）
// 真正的定义放在 meshfield.cpp 里
extern std::vector<ControlPixel> g_ControlCPU;
extern ID3D11Texture2D* g_pControlTex;
extern ID3D11ShaderResourceView* g_pControlSRV;

void ControlMap_Initialize();

extern float g_BrushStrength ; // 每次修改的高度
extern float g_BrushRadius ; // 单位：世界空间

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void MeshField_Draw();
void MeshField_Finalize(void);
void MeshField_RebuildVertices();
void MeshField_SetEditMode(bool enable);
bool MeshField_IsEditMode();
//void TerrainEdit_Update(double dt);
void ApplyBrushAtWorldPos(const DirectX::XMFLOAT3& hitPos);

bool GroundPosToControlUV(const DirectX::XMFLOAT3& hitPos, float& u, float& v);
bool ControlUVToPixel(float u, float v, int& px, int& py);

void MeshField_UpdateTerrainPaint();

bool MeshField_Save(const char* filename);
bool MeshField_Load(const char* filename);
bool MeshField_DeleteSave(const char* filename);
#endif  // MESHFIELD_H