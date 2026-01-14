/*====================================================


ゲーム本体「game.cpp」



=============================================================*/





#include "game.h"
#include "direct3d.h"
//#include "cube.h"
//#include "grid.h"
#include "light.h"
#include "camera.h"
#include <DirectXMath.h>
#include "mouse.h"
using namespace DirectX;
#include "Keylogger.h"
#include "meshfield.h"
#include "meshfield_brush.h"
#include "scene_model.h"
#include "scene_model_ui.h"
#include "picking.h"
#include "player.h"
#include "bullet.h"
#include "sampler.h"
#include "playercamera.h"
#include "model.h"
#include "sprite.h"
#include "map.h"
#include "billboard.h"
#include "texture.h"
#include "Sprite_Anim.h"
#include "bullet_hit_effect.h"
#include "algorithm"
#include "sky.h"
#include "shader_shadow.h"
#include "shadow_map.h"
#include "water.h"
#include "shader3d .h"
#include "shader_volumetric_fog.h"
#include "shader_lightshaft.h"
/*实验代码之后删除*/


static float g_x = 0.0f;
static float g_angle = 0.0f;
static float g_scale = 1.0f;

static double g_AccumulatedTime = 0.0f;

static XMFLOAT3 g_CubePosition{}; 
static XMFLOAT3 g_CubeVelocity{};

static MODEL* g_pModelTest[] = { nullptr };
static MODEL* g_pTestSceneModel;
static int g_TestTexId = -1;


//MODEL* g_pTestSceneModel = nullptr;
/*************实验代码之后删除*******************/

static int g_AnimPatternId = -1;
static int g_AnimPlayId = -1;

static bool g_IsDebug = false;
static bool g_EditTerrain = false;
static bool g_PaintTerrain = false;
static bool g_EditObjects = false;


static void SetAllEditModeOff();
static void ObjectEdit_Update(double dt);
static XMMATRIX BuildLightViewProj(const XMFLOAT4& dir, const XMFLOAT3& camPos);

void Game_Initialize()
{
	
	Player_Initialize({ 0.0f , 0.0f , -5.0f } , {0.0f , 0.0f , 1.0f});
	Camera_Initialize({ 8.0f , 8.0f , -12.0f }, { -0.5f,-0.3f,0.7f }, { 0.8f , 0.0f , 0.55f });
	PlayerCamera_Initialize();
	Map_Initialize();
	ShadowSystem_Initialize(Direct3D_GetDevice());
	SceneModel_Initialize();

	ModelResource_Initialize();
	SceneModelUI_Initialize();
	Sky_Initialize();
	Water_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Bullet_Initialize();
	BillBoard_Initialize();
	BulletHitEffect_Initialize();
	
	g_TestTexId = Texture_Load(L"resource/texture/cat_idle.png");

	g_AnimPatternId = SpriteAnim_RegisterPattern(g_TestTexId, 10, 10,
		0.1,
		{ 32 , 32 },
		{ 0 ,0 }
		);
	g_AnimPlayId = SpriteAnim_CreatePlayer(g_AnimPatternId);

	
	g_pTestSceneModel = ModelLoad("resource/mesh/large_tree_w_roots_a.FBX", false,0.01f);
	SceneModel_Add(g_pTestSceneModel, XMFLOAT3{ 0.0f, 0.0f, 0.0f });


	g_IsDebug = false;


}

void Game_Finalize()
{
	BulletHitEffect_Finalize();
	Bullet_Finalize();
	Water_Finalize();
	BillBoard_Finalize();
	Sky_Finalize();
	//ModelRelease(g_pModelTest[0]);
	SceneModelUI_Finalize();
	SceneModel_Finalize();
	ShadowSystem_Finalize();
	Map_Finalize();
	//ModelRelease(g_pModelTest[1]);
	PlayerCamera_Finalize();
	Camera_Finalize();
}

void Game_Update(double elapsed_time)
{
	elapsed_time = elapsed_time;
	g_AccumulatedTime += elapsed_time;

	Mouse_State ms{};
	Mouse_GetState(&ms);

	//g_x = sin(g_AccumulatedTime )* 4.5f ;
	//g_angle = float(g_AccumulatedTime * 3.0);
	//g_scale = float(sin(g_AccumulatedTime) * 2.0);
	Player_Update(elapsed_time);
	
	
	
	if (g_IsDebug)
	{
		Camera_Update(elapsed_time);
		
		// ===== 0) 这一帧先缓存所有 Trigger（只读一次）=====
		const bool tF1 = KeyLogger_IsTrigger(KK_F1);
		const bool tF2 = KeyLogger_IsTrigger(KK_F2);
		const bool tF3 = KeyLogger_IsTrigger(KK_F3);
		const bool tF5 = KeyLogger_IsTrigger(KK_F5);
		const bool tF6 = KeyLogger_IsTrigger(KK_F6);

		// ===== 1) 先做模式切换（F1/F2/F3）=====
		if (tF1)
		{
			bool was = g_EditTerrain;
			SetAllEditModeOff();
			g_EditTerrain = !was;
			MeshField_SetEditMode(g_EditTerrain || g_PaintTerrain);
		}

		if (tF2)
		{
			bool was = g_PaintTerrain;
			SetAllEditModeOff();
			g_PaintTerrain = !was;
			MeshField_SetEditMode(g_EditTerrain || g_PaintTerrain);
		}

		if (tF3)
		{
			bool was = g_EditObjects;
			SetAllEditModeOff();
			g_EditObjects = !was;
			SceneModelUI_SetActive(g_EditObjects);
			MeshField_SetEditMode(false);
		}

		// ===== 2) 再根据“最终模式”更新 =====
		if (g_EditTerrain)
		{
			TerrainEdit_Update(elapsed_time);
		}
		else if (g_PaintTerrain)
		{
			UpdateBrushInput();
			MeshField_UpdateTerrainPaint();
		}
		else if (g_EditObjects)
		{
			SceneModelUI_Update(elapsed_time);
			ObjectEdit_Update(elapsed_time);
		}

		// ===== 3) 最后根据“最终模式”只处理一次 F5/F6 =====
		if (tF5)
		{
			if (g_EditObjects)
				SceneModel_Save("scene_save.txt");
			else if (g_EditTerrain || g_PaintTerrain)
				MeshField_Save("terrain.bin");
		}
		
		if (tF6)
		{
			if (g_EditObjects)
				SceneModel_Load("scene_save.txt");
			else if (g_EditTerrain || g_PaintTerrain)
				MeshField_Load("terrain.bin");
		}
	}
	else
	{
		PlayerCamera_Update(elapsed_time);
		Sky_SetPosition(Player_GetPosition());
	}
	Water_Update((float)elapsed_time);
	Bullet_Update(elapsed_time);
	/*
	for (int j = 0; j < Map_GetObjectsCount(); j++)
	{
		//粉砕
	
		for (int i = 0; i < Bullet_GetBulletCount(); i++)
		{
			AABB bullet = Bullet_GetAABB(i);
			AABB object = Map_GetObjects(j)->Collision_AABB;
			if (Collisioin_IsOverlappAABB(bullet, object))
			{
				BulletHitEffect_Create(Bullet_GetPosition(i));
				Bullet_Destroy(i);
				
			}

		}
	}
	*/


	SpriteAnimUpdate(elapsed_time);

	BulletHitEffect_Update(elapsed_time);
	if (KeyLogger_IsTrigger(KK_L))
	{
		g_IsDebug = !g_IsDebug;
	}
}

void Game_Draw()
{
	Shader3d_BeginFrame();

	XMFLOAT4X4 mtxView = g_IsDebug ? Camera_GetMatrix() : PlayerCamera_GetViewMatrix();

	XMMATRIX view = XMLoadFloat4x4(&mtxView);

	XMMATRIX proj = g_IsDebug ?
		XMLoadFloat4x4(&Camera_GetPerspectiveMatrix())
		: XMLoadFloat4x4(&PlayerCamera_GetPerspectiveMatrix());

	XMFLOAT3 camera_position = g_IsDebug ?
		Camera_GetPosition()
		: PlayerCamera_GetPosition();


	//カメラに関する行列をシェーダに設定
	Camera_SetMatrix(view,proj);
	Billboard_SetViewMatrix(mtxView);
	Sampler_SetFilterAnisotropic();

	Light_SetAmbient({ 0.03f, 0.03f, 0.03f });
	XMVECTOR v = XMVector3Normalize(XMVECTOR{ -0.35f, -1.0f, -0.2f });
	XMFLOAT4 dir;
	XMStoreFloat4(&dir, v);

	//Light_SetDirectional(dir, { 1.0f , 0.9f , 0.7f , 1.0f });
	Light_SetDirectional(dir, { 3.0f, 2.8f, 2.6f, 1.0f });

	Light_Set_PBRParams(camera_position, 0.0f, 1.0f, 1.0f);
	
	XMMATRIX lightVP = BuildLightViewProj(dir, camera_position);
	
	ShadowSystem_RenderShadowMap(Direct3D_GetContext(), lightVP);

	Direct3D_SetDepthEnable(false);
	Sky_Draw();
	Direct3D_SetDepthEnable(true);

	// ✅ 2) 主渲染前绑定 t10/s10 + b6（你的 PBR PS 里会采样）
	ShadowSystem_BindForMainPass(Direct3D_GetContext(), lightVP);


	Shader3d_BeginPassOnce();

	Shader3d_SetWindParams(
		XMFLOAT3(dir.x, dir.y, dir.z), // 风方向（你方向光dir也能先复用）
		0.25f,   // strength：0.05~0.6
		1.2f,    // speed：0.5~3
		0.12f,   // freq：0.05~0.3
		0.6f,    // gustFreq：0.2~1.5
		(float)g_AccumulatedTime,
		0.35f    // flutter：0~1（叶子抖动）
	);



	Light_Set_PointLight_Count(0);
	//XMVECTOR position = { 0.0 , 2.0 , -3.0 };
	//XMMATRIX rot = XMMatrixRotationY(g_angle);
	//position = XMVector3Transform(position, rot);
	//XMFLOAT3 pp;
	//XMStoreFloat3(&pp, position);

	//Light_Set_PointLight(0,  pp,				    5.0f, {1.0f , 0.0f , 0.0f});
	//Light_Set_PointLight(1, { 2.0f, 0.3f , -3.0f }, 3.0f, { 0.0f , 1.0f ,0.0f });
	//Light_Set_PointLight(2, { -2.0f, 0.3f , -3.0f }, 3.0f, { 0.0f , 0.0f ,1.0f });
	Sampler_SetFilterAnisotropic();
	//Sampler_SetIBLClamp();
	ID3D11DepthStencilState* st = nullptr;
	UINT ref = 0;
	Direct3D_GetContext()->OMGetDepthStencilState(&st, &ref);

	
	Player_Draw();
	
	Map_Draw();
	
	SceneModel_Draw();

	
	
	Water_Draw(view, proj, camera_position);


	// ===== 体积雾：准备输入(t40/t41) =====
	Direct3D_PrepareWaterInputs();

	ShaderFog::SetSceneSRV(
		Direct3D_GetSceneColorCopySRV(),  // t40
		Direct3D_GetDepthSRV(),           // t41
		Direct3D_UseMSAA()                // msaa?
	);

	// 参数（先用能看到效果的）
	FogParams fp{};
	fp.camPosW = camera_position;
	fp.time = (float)g_AccumulatedTime;
	fp.fogColor = { 0.55f, 0.65f, 0.75f };
	fp.fogDensityNear = 0.004f;  // 近处几乎无雾
	fp.fogDensityFar = 0.04f;  // 远处浓度
	fp.densityRampPow = 2.5f;    // 越大越“前清后浓”

	fp.fogStart = 3.0f;
	fp.fogEnd = 60.0f;
	fp.anisotropyG = 0.6f;
	fp.lightDirW = { dir.x, dir.y, dir.z };
	fp.lightColor = { 3.0f, 2.8f, 2.6f };
	fp.lightIntensity = 1.0f;

	ShaderFog::SetParams(fp);
	ShaderFog::Draw(proj);



	ShaderLightShaft::SetSceneDepthSRV(Direct3D_GetDepthSRV(), Direct3D_UseMSAA());

	LightShaftParams sp{};
	sp.lightDirW = { dir.x, dir.y, dir.z }; // 你 Light_SetDirectional 用的那个 dir
	sp.intensity = 1.2f;
	sp.sunThreshold = 0.995f;
	sp.sunPower = 128.0f;
	sp.density = 0.9f;
	sp.decay = 0.96f;
	sp.weight = 0.35f;
	sp.exposure = 0.8f;
	sp.downsample = 2; // 2 或 4

	ShaderLightShaft::Draw(view, proj, sp);


	

	Direct3D_SetDepthEnable(false);

	Sprite_Begin();
	if (SceneModelUI_IsActive())
	{
		SceneModelUI_Draw();  // 这里会画半透明面板 + 高亮条目
	}

	//BillboardAnim_Draw(g_AnimPlayId, { -10.0f , 5.0f , -10.0f }, { 1.0f , 1.0f }, { 0.0f,-2.5f });//半透明最后描画
	Direct3D_SetDepthEnable(true);

	Bullet_Draw();

	BulletHitEffect_Draw();
	//BillBoard_Draw(g_TestTexId, { -10.0f , 0.5f , -10.0f },{10.0f , 1.0f}, {0.0f , 0.0f,32.0f,32.0f}, {0.0f , -2.0f});


	
	

	if (g_IsDebug)
	{
		//Camera_DebugDraw();
	}
	
}

void TerrainEdit_Update(double dt)
{
	

	XMFLOAT4X4 view = g_IsDebug ? Camera_GetMatrix()
		: PlayerCamera_GetViewMatrix();

	XMFLOAT4X4 proj = g_IsDebug ? Camera_GetPerspectiveMatrix()
		: PlayerCamera_GetPerspectiveMatrix();

	float screenW = Direct3D_GetBackBufferWidth();
	float screenH = Direct3D_GetBackBufferHeight();

	Mouse_State ms{};
	Mouse_GetState(&ms);
	float mx = float(ms.x);
	float my = float(ms.y);

	Ray ray = ScreenPointToRay(mx, my, screenW, screenH, view, proj);

	XMFLOAT3 hitPos;
	if (!RayHitPlaneY0(ray, hitPos)) return;

	if (ms.leftButton)
	{
		g_BrushStrength = 1.0f * float(dt);
		ApplyBrushAtWorldPos(hitPos);
		MeshField_RebuildVertices();

	}

	else if (ms.rightButton)
	{
		g_BrushStrength = -1.0f * float(dt);
		ApplyBrushAtWorldPos(hitPos);
		MeshField_RebuildVertices();
	}
	if (KeyLogger_IsPressed(KK_Q)) g_BrushRadius = std::max(0.5f, g_BrushRadius - float(dt) * 2.0f);
	if (KeyLogger_IsPressed(KK_E)) g_BrushRadius = g_BrushRadius + float(dt) * 2.0f;
}

void SetAllEditModeOff()
{
	g_EditTerrain = false;
	g_PaintTerrain = false;
	g_EditObjects = false;
	SceneModelUI_SetActive(false);
	// 地形那边只关掉 editMode 标志
	MeshField_SetEditMode(false);
}

void ObjectEdit_Update(double dt)
{
	return;

	Mouse_State ms{};
	Mouse_GetState(&ms);

	float mx = (float)ms.x;
	float my = (float)ms.y;

	// 鼠标“触发”判定（只在按下那一帧生效）
	static bool s_LeftPrev = false;
	bool leftNow = ms.leftButton;
	bool leftTrigger = (leftNow && !s_LeftPrev);
	s_LeftPrev = leftNow;

	if (!leftTrigger)
		return;   // 不是刚按下这一帧就不处理

	// 2) 如果点在 UI 面板上，就只交给 UI，不在场景里放东西
	if (SceneModelUI_IsPointInPanel(mx, my))
		return;

	// 3) 看看 UI 里有没有选中某个资源
	const MODEL_RESOURCE_ITEM* uiItem = SceneModelUI_GetSelectedItem();
	if (!uiItem || uiItem->resourceIndex < 0)
		return;

	int resIndex = SceneModelUI_GetSelectedResourceIndex();
	if (resIndex < 0) return;

	// 4) 生成从屏幕点射出的拾取射线（和 TerrainEdit 一样套路）
	XMFLOAT4X4 view = g_IsDebug ? Camera_GetMatrix()
		: PlayerCamera_GetViewMatrix();

	XMFLOAT4X4 proj = g_IsDebug ? Camera_GetPerspectiveMatrix()
		: PlayerCamera_GetPerspectiveMatrix();

	float screenW = (float)Direct3D_GetBackBufferWidth();
	float screenH = (float)Direct3D_GetBackBufferHeight();

	Ray ray = ScreenPointToRay(mx, my, screenW, screenH, view, proj);

	// 5) 和 y=0 平面求交点（暂时先放在地面高度 0 的位置）
	XMFLOAT3 hitPos{};
	if (!RayHitPlaneY0(ray, hitPos))
		return;

	// 6) 在命中的位置创建一个实例
	//SceneModel_AddFromResource(resIndex, hitPos);
}
static XMMATRIX BuildLightViewProj(const XMFLOAT4& dir, const XMFLOAT3& camPos)
{
	// 方向光方向（指向地面）
	XMVECTOR L = XMVector3Normalize(XMVectorSet(dir.x, dir.y, dir.z, 0));

	// 以相机附近为中心做一个阴影盒（先简单：半径 40m，距离 60m）
	XMVECTOR center = XMVectorSet(camPos.x, camPos.y, camPos.z, 1.0f);
	float radius = 40.0f;
	float dist = 60.0f;

	XMVECTOR lightPos = center - L * dist;

	// up 向量避免与光方向平行
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	if (fabsf(dir.y) > 0.99f) up = XMVectorSet(0, 0, 1, 0);

	XMMATRIX V = XMMatrixLookAtLH(lightPos, center, up);

	// 正交投影（方向光用 ortho）
	float zn = 1.0f;
	float zf = dist + radius * 2.0f;
	XMMATRIX P = XMMatrixOrthographicLH(radius * 2.0f, radius * 2.0f, zn, zf);

	return V * P; // lightVP
}


