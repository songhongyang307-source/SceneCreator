#include "scene_model.h"
#include <DirectXMath.h>
using namespace DirectX;
#include <cstdio>
#include <cstring>
#include "shader_shadow.h"
#include "shader3d .h"
#include "direct3d.h"
#include "water.h"



static std::vector<SceneModelInstance> g_SceneModels;
static std::vector<ModelResource> g_ModelResources;
static int g_SelectedIndex = -1;

static const int SEL_WATER = -2;

/*==========================================================*/
/*             Resource Instance List                       */
void SceneModel_Initialize()
{
	g_SceneModels.clear();
}

void SceneModel_Finalize()
{
	g_SceneModels.clear();
}

void SceneModel_Add(MODEL* model, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale)
{
	if (!model) return;

	SceneModelInstance inst{};
	inst.pModel = model;
	inst.position = pos;
	inst.rotation = rot;
	inst.scale = scale;

	inst.pickRadius = 1.0f * scale.x;
	inst.resourceIndex = -1;

	g_SceneModels.push_back(inst);
}

void SceneModel_Add(MODEL* model, const XMFLOAT3& pos)
{
	if (!model) return;

	SceneModelInstance inst{};
	inst.pModel = model;
	inst.position = pos;
	inst.rotation = { 0.0f , 0.0f , 0.0f };
	inst.scale = { 1.0f , 1.0f , 1.0f };

	inst.pickRadius = 1.0f * inst.scale.x;
	inst.resourceIndex = -1;

	g_SceneModels.push_back(inst);  //g_SceneModels ni 書き込む
}

void SceneModel_Clear()
{
	g_SceneModels.clear();
}

void SceneModel_Draw()
{
	Shader3d_BeginPassOnce();

	// IBL：每帧一次（绑定 t30/t31/t32 + 更新 b8）
	Shader3d_BindIBL_Once(); // 你现在用的强度
	for (auto& inst : g_SceneModels)
	{
		if (!inst.pModel) continue;

		XMMATRIX S = XMMatrixScaling(inst.scale.x, inst.scale.y, inst.scale.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(
			inst.rotation.x,
			inst.rotation.y,
			inst.rotation.z
		);
		XMMATRIX T = XMMatrixTranslation(
			inst.position.x,
			inst.position.y,
			inst.position.z
		);

		XMMATRIX mtxworld = S * R * T;

		ModelDraw(inst.pModel, mtxworld);

	}


}
SceneModelInstance* SceneModel_GetInstance(int index)
{
	if (index < 0 || index >= (int)g_SceneModels.size()) return nullptr;
	return &g_SceneModels[index];

}
void SceneModel_UpdatePickRadius(int index)
{
	SceneModelInstance* inst = SceneModel_GetInstance(index);
	float r = std::max({ inst->scale.x, inst->scale.y, inst->scale.z });

	r = std::max(r, 1.0f);

	inst->pickRadius = r;
}
/*             Resource Instance List                       */
/*==========================================================*/





/*==========================================================*/
/*             Resource List                                */


void ModelResource_Initialize()
{
	g_ModelResources.clear();

	{
		ModelResource crate{};
		crate.name = "Rock";
		crate.path = "resource/mesh/rock_boulder_a.fbx";
		crate.pModel = ModelLoad(crate.path, false, 0.01f);

		g_ModelResources.push_back(crate);
	}

	{
		ModelResource crate{};
		crate.name = "Grass";
		crate.path = "resource/mesh/grass_small_02.fbx";
		crate.pModel = ModelLoad(crate.path, false, 0.01f);

		g_ModelResources.push_back(crate);
	}

	{
		ModelResource crate{};
		crate.name = "leaves_fern_01";
		crate.path = "resource/mesh/leaves_fern_01.fbx";
		crate.pModel = ModelLoad(crate.path, true, 1.0f);

		g_ModelResources.push_back(crate);
	}

	{
		ModelResource crate{};
		crate.name = "Tree";
		crate.path = "resource/mesh/large_tree_w_roots_a.fbx";
		crate.pModel = ModelLoad(crate.path, false, 0.01f);

		g_ModelResources.push_back(crate);
	}
}

int ModelResource_GetCount()
{
	return (int)g_ModelResources.size();
}

ModelResource* ModelResource_Get(int index)
{
	if (index < 0 || index >= (int)g_ModelResources.size()) return nullptr;
	return &g_ModelResources[index];
}
int SceneModel_Pick(const Ray& ray)
{
	XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
	XMVECTOR rayDir = XMLoadFloat3(&ray.dir);

	int hitIndex = -1;
	float closestT = 1e9f;

	for (int i = 0; i < (int)g_SceneModels.size(); i++)
	{
		const SceneModelInstance& inst = g_SceneModels[i];
		XMVECTOR center = XMLoadFloat3(&inst.position);
		float radius = inst.pickRadius;

		XMVECTOR oc = center - rayOrigin;

		float t = XMVectorGetX(XMVector3Dot(oc, rayDir));
		if (t < 0.0f) continue;

		XMVECTOR closestPoint = rayOrigin + rayDir * t;

		XMVECTOR diff = closestPoint - center;
		float dist2 = XMVectorGetX(XMVector3LengthSq(diff));
		float r2 = radius * radius;

		if (dist2 <= r2)
		{
			if (t < closestT)
			{
				closestT = t;
				hitIndex = i;
			}
		}

	}
	return hitIndex;
}
void SceneModel_SetSelectedIndex(int index)
{
	if (index == SEL_WATER)
	{
		g_SelectedIndex = SEL_WATER;
		return;
	}
	if (index < 0 || index >= (int)g_SceneModels.size())
		g_SelectedIndex = -1;
	else
		g_SelectedIndex = index;
}
int SceneModel_GetSelectedIndex()
{
	return g_SelectedIndex;
}
void SceneModel_Delete(int index)
{
	if (index < 0 || index >= (int)g_SceneModels.size()) return;

	g_SceneModels.erase(g_SceneModels.begin() + index);

	if (g_SelectedIndex == index)
	{
		g_SelectedIndex = -1;
	}
	else if (g_SelectedIndex > index) {
		g_SelectedIndex--;
	}
	
}
void SceneModel_DeleteSelected()
{
	if (g_SelectedIndex < 0) return;
	SceneModel_Delete(g_SelectedIndex);
}
void SceneModel_AddFromResource(int resourceIndex, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale)
{
	ModelResource* res = ModelResource_Get(resourceIndex);
	if (!res || !res->pModel) return;

	SceneModelInstance inst{};
	inst.pModel = res->pModel;
	inst.rotation = rot;
	inst.position = pos;
	inst.scale = scale;

	inst.pickRadius = 1.0f * inst.scale.x;
	inst.resourceIndex = resourceIndex;

	g_SceneModels.push_back(inst);
	SceneModel_UpdatePickRadius((int)g_SceneModels.size() - 1);
}
void SceneModel_AddFromResource(int resourceIndex, const DirectX::XMFLOAT3& pos)
{
	SceneModel_AddFromResource(resourceIndex,
		pos,
		XMFLOAT3{ 0,0,0 },
		XMFLOAT3{ 1,1,1 });


}
void SceneModel_DrawShadow(const DirectX::XMMATRIX& lightVP)
{
	for (auto& inst : g_SceneModels)
	{
		if (!inst.pModel) continue;

		XMMATRIX S = XMMatrixScaling(inst.scale.x, inst.scale.y, inst.scale.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(
			inst.rotation.x,
			inst.rotation.y,
			inst.rotation.z
		);
		XMMATRIX T = XMMatrixTranslation(
			inst.position.x,
			inst.position.y,
			inst.position.z
		);

		XMMATRIX world = S * R * T;

		ShadowShader_SetWVPMatrix(world * lightVP);
		ModelDrawShadowRaw(inst.pModel);   // 你的 shadow 用的绘制函数
	}
}
bool SceneModel_Save(const char* filename)
{
	if (!filename) return false;

	FILE* fp = nullptr;
	if (fopen_s(&fp, filename, "w") != 0 || !fp)
		return false;

	int saveCount = 0;
	for (auto& inst : g_SceneModels)
	{
		if (inst.resourceIndex >= 0)
			saveCount++;
	}
	fprintf(fp, "SCENEMODEL_V1 %d\n", saveCount);

	for (auto& inst : g_SceneModels)
	{
		if (inst.resourceIndex < 0)
			continue;

		fprintf(fp, "%d %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n",
			inst.resourceIndex,
			inst.position.x, inst.position.y, inst.position.z,
			inst.rotation.x, inst.rotation.y, inst.rotation.z,
			inst.scale.x, inst.scale.y, inst.scale.z);
	}
	{
		WaterTransform* t = Water_GetTransformPtr();
		if (t)
		{
			fprintf(fp, "WATER %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n",
				t->position.x, t->position.y, t->position.z,
				t->rotation.x, t->rotation.y, t->rotation.z,
				t->scale.x, t->scale.y, t->scale.z);
		}
		else
		{
			// 如果你希望即使没有 transform 也写默认值，也可以写一行默认 WATER
			// fprintf(fp, "WATER 0 0 0 0 0 0 1 1 1\n");
		}
	}
	fclose(fp);
	return true;
}
bool SceneModel_Load(const char* filename)
{
	if (!filename) return false;

	FILE* fp = nullptr;
	if (fopen_s(&fp, filename, "r") != 0 || !fp)
		return false;

	g_SceneModels.clear();
	g_SelectedIndex = -1;

	char header[32]{};
	int count = 0;

	if (fscanf_s(fp, "%31s %d", header, (unsigned)_countof(header), &count) != 2)
	{
		fclose(fp);
		return false;
	}

	if (strcmp(header, "SCENEMODEL_V1") != 0)
	{
		fclose(fp);
		return false;
	}
	for (int i = 0; i < count; ++i) {
		int resourceIndex = -1;
		XMFLOAT3 pos{}, rot{}, scale{};

		int n = fscanf_s(fp, "%d %f %f %f %f %f %f %f %f %f",
			&resourceIndex,
			&pos.x, &pos.y, &pos.z,
			&rot.x, &rot.y, &rot.z,
			&scale.x, &scale.y, &scale.z);

		if (n != 10)
			break;

		ModelResource* res = ModelResource_Get(resourceIndex);
		if (!res || !res->pModel)
		{
			continue;
		}

		SceneModelInstance inst{};
		inst.pModel		   = res->pModel;
		inst.position	   = pos;
		inst.rotation	   = rot;
		inst.scale		   = scale;
		inst.pickRadius    = 1.0f * inst.scale.x;
		inst.resourceIndex = resourceIndex;

		g_SceneModels.push_back(inst);


	}
	{
		char tag[16]{};
		int nTag = fscanf_s(fp, "%15s", tag, (unsigned)_countof(tag));
		if (nTag == 1)
		{
			if (strcmp(tag, "WATER") == 0)
			{
				WaterTransform* t = Water_GetTransformPtr();
				if (t)
				{
					int n = fscanf_s(fp, "%f %f %f %f %f %f %f %f %f",
						&t->position.x, &t->position.y, &t->position.z,
						&t->rotation.x, &t->rotation.y, &t->rotation.z,
						&t->scale.x, &t->scale.y, &t->scale.z);

					if (n == 9)
					{
						Water_ApplyTransformNow(); // ★ 关键：更新水的 world
					}
					else
					{
						// 读坏了就恢复默认（可选）
						// t->position = {0,0,0}; t->rotation={0,0,0}; t->scale={1,1,1};
						// Water_ApplyTransformNow();
					}
				}
			}
			else
			{
				// 不是 WATER：说明你未来可能加别的块，这里就忽略
			}
		}
	}
	fclose(fp);
	return true;
	
}
/*             Resource List                                */
/*==========================================================*/
