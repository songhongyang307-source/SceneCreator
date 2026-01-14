/*==============================================================================

   map controller[map.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/



#include "map.h"

using namespace DirectX;
#include "cube.h"
#include "texture.h"
#include "meshfield.h"
#include "light.h"
#include "playercamera.h"
#include "model.h"
#include "direct3d.h"
#include "camera.h"


static MapObject g_MapObjects[]
{//0 : ground
 //1:box type1
	//2 : box type2
	//3 : rock01


	{GROUND,     {0.0f,0.0f,0.0f}, {{-25.0f , -1.0f ,-25.0f} , {25.0f , 0.0f ,25.0f}}},
	{BLOCK,      {1.0f, -5.0f, 0.0f}},
	//{BLOCK,      {-1.0f, 0.5f, 0.0f}},
	//{BLOCK,      {0.0f, 0.5f, 1.0f}},
	//{BLOCK,      {1.0f, 0.5f, 1.0f}},
	///{BLOCK,      {-1.0f, 0.5f, 1.0f}},
	//{BLOCK,      {1.0f, 0.5f, 2.0f}},
	//{BLOCK,      {-1.0f, 0.5f, 2.0f}},
	//{BLOCK_STONE,{-1.0f, 1.5f, 2.0f}},
	//{BLOCK_STONE,{0.0f, 1.5f, -1.0f}},
	{ROCK,       {10.0f, 0.0f,10.0f}},
	{TREE,       {10.0f ,0.0f, 0.0f}}
};

static int g_CubeTexHoroId{ -1 };
static int g_CubeTexWoodBoxId{ -1 };
static int g_CubeTexWoodBoxMetallicId{ -1 };
static int g_CubeTexWoodBoxRoughnessId{ -1 };
static int g_CubeTexWoodBoxNormalId{ -1 };

static MODEL* g_pRock01{};
static MODEL* g_pTree{};

void Map_Initialize()
{
	g_CubeTexHoroId = Texture_Load(L"resource/texture/222.png", true);
	g_CubeTexWoodBoxId = Texture_Load(L"resource/texture/woodbox/Planks013_2K_Color.jpg", true);
	g_CubeTexWoodBoxMetallicId = Texture_Load(L"resource/texture/woodbox/Planks013_2K_Metalness.jpg");
	g_CubeTexWoodBoxRoughnessId = Texture_Load(L"resource/texture/woodbox/Planks013_2K_Roughness.jpg");
	g_CubeTexWoodBoxNormalId = Texture_Load(L"resource/texture/woodbox/Planks013_2K_Normal.jpg");


	g_pRock01 = ModelLoad("resource/mesh/cube.fbx", false, 0.2f);
	//g_pTree = ModelLoad("resource/mesh/Tree.fbx", 0.1f);

	for (MapObject& o : g_MapObjects )
	{
		if(o.Kindid ==BLOCK || o.Kindid == BLOCK_STONE)  o.Collision_AABB = Cube_GetAABB(o.Position);
		else if (o.Kindid ==ROCK){

			o.Collision_AABB = Model_GetAABB(g_pRock01, o.Position);
		}
	
	}
}

void Map_Finalize()
{
	//ModelRelease(g_pTree);
	ModelRelease(g_pRock01);
	
}

void Map_Draw()
{
	XMMATRIX  mtxWorld;

	for (const MapObject& o : g_MapObjects)
	{
		
		switch (o.Kindid)
		{
		case GROUND:{

			MeshField_Draw();
			break;
		}
		case BLOCK:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexHoroId,mtxWorld);
			break;

		case BLOCK_STONE:
			//mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			//Cube_Draw(g_CubeTexWoodBoxId,g_CubeTexWoodBoxMetallicId,g_CubeTexWoodBoxRoughnessId,g_CubeTexWoodBoxNormalId, mtxWorld);
			break;

		case ROCK:
			//mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			//ModelDraw(g_pRock01, mtxWorld);
			break;
		case TREE:
			//mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			//ModelDraw(g_pTree, mtxWorld);
			break;
		}
	}
}//打ち合う

const MapObject* Map_GetObjects(int index)
{
	return &g_MapObjects[index];
}

int Map_GetObjectsCount()
{
	return sizeof(g_MapObjects)/sizeof(g_MapObjects[0]);
}
