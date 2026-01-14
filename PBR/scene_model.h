/*==============================================================================

			シーンのモデルの制御[scene_model.h]
														 Author : SONG HONGYANG
														 Date   : 2025/12/8
--------------------------------------------------------------------------------

==============================================================================*/

#pragma once

#include <vector>
#include <DirectXMath.h>
#include "model.h"
#include "picking.h"

using namespace DirectX;

/*==========================================================*/
/*             Resource Instance List                       */
struct SceneModelInstance
{
	MODEL* pModel;
	XMFLOAT3 position;
	XMFLOAT3 rotation; //rad
	XMFLOAT3 scale;
	float pickRadius;

	int resourceIndex;
};

void SceneModel_Initialize();
void SceneModel_Finalize();

void SceneModel_Add(MODEL* model, const XMFLOAT3& pos , const XMFLOAT3& rot, const XMFLOAT3& scale);
void SceneModel_Add(MODEL* model, const XMFLOAT3& pos);
 void SceneModel_Clear();

void SceneModel_Draw();

SceneModelInstance* SceneModel_GetInstance(int index);
void SceneModel_UpdatePickRadius(int index);


/*             Resource Instance List                       */
/*==========================================================*/


/*==========================================================*/
/*             Resource List                       */
struct	ModelResource
{
	const char* name;
	const char* path;
	MODEL* pModel;

};

void ModelResource_Initialize();

int ModelResource_GetCount();

ModelResource* ModelResource_Get(int index);

int SceneModel_Pick(const Ray& ray);

void SceneModel_SetSelectedIndex(int index);
int SceneModel_GetSelectedIndex();

void SceneModel_Delete(int index);
void SceneModel_DeleteSelected();

void SceneModel_AddFromResource(int resourceIndex,
	const DirectX::XMFLOAT3& pos,
	const DirectX::XMFLOAT3& rot,
	const DirectX::XMFLOAT3& scale);


void SceneModel_AddFromResource(int resourceIndex,
	const DirectX::XMFLOAT3& pos);

void SceneModel_DrawShadow(const DirectX::XMMATRIX& lightVP);
/*             Resource List                       */
/*==========================================================*/


bool SceneModel_Save(const char* filename);
bool SceneModel_Load(const char* filename);