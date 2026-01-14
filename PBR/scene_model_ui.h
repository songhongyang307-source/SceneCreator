#pragma once

#include <string>
#include "picking.h"
struct ModelResource;  // 前置声明
struct TEXTURE;

struct MODEL_RESOURCE_ITEM
{
    std::string name;
    TEXTURE* pIconTexture;
    int         resourceIndex;   // 对应 g_ModelResources 的索引
};


void SceneModelUI_Initialize();
void SceneModelUI_Finalize();

void SceneModelUI_SetActive(bool active);
bool SceneModelUI_IsActive();

void SceneModelUI_Update(double elapsed_time);
void SceneModelUI_Draw();

// 选中了哪个 ModelResource（对应 g_ModelResources 的 index）
int SceneModelUI_GetSelectedResourceIndex();
const MODEL_RESOURCE_ITEM* SceneModelUI_GetSelectedItem();

bool SceneModelUI_IsPointInPanel(float x, float y);
ModelResource* SceneModelUI_GetSelectedResource();

bool ClosestPoint_RayLine_S(
    const Ray& ray,
    const DirectX::XMFLOAT3& linePoint,
    const DirectX::XMFLOAT3& lineDir,
    float& outS
);