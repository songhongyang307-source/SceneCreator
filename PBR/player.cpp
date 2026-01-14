/*==============================================================================

  player controller [player.cpp]
														 Author : Youhei Sato
														 Date   : 2025/10/31
--------------------------------------------------------------------------------

==============================================================================*/


#include "player.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"
#include "Keylogger.h"
#include "light.h"
#include "camera.h"
#include "playercamera.h"
#include "map.h"
#include "texture.h"
#include "cube.h"
#include "bullet.h"
#include "shader3d .h"





static XMFLOAT3 g_PlayerPosition{};//player world position
static XMFLOAT3 g_PlayerFront { 0.0f , 0.0f , 1.0f };
static XMFLOAT3 g_PlayerVelocity{};//
static MODEL* g_pPlayerModel{nullptr};
static bool g_IsJump = false;

static int g_DiffuseColorTex = -1;
static int g_NormalMapTex = -1;
static int g_MetallicMapTex = -1;
static int g_RoughnessMapTex = -1;

void Player_Initialize(const XMFLOAT3& position , const XMFLOAT3& front)
{
	g_PlayerPosition = position;
	g_PlayerVelocity = { 0.0f , 0.0f , 0.0f};
	XMStoreFloat3(&g_PlayerFront, XMVector3Normalize( XMLoadFloat3(&front)));

	g_pPlayerModel = ModelLoad("resource/mesh/cube.fbx", false, 0.5f);
	g_DiffuseColorTex = Texture_Load(L"resource/texture/Cube_BaseColor.jpg", true);
	g_NormalMapTex = Texture_Load(L"resource/texture/Cube_Normal.jpg");
	g_MetallicMapTex = Texture_Load(L"resource/texture/Cube_Metallic.jpg");
	g_RoughnessMapTex = Texture_Load(L"resource/texture/Cube_Roughness.jpg");
}

void Player_Finalize()
{
	ModelRelease(g_pPlayerModel);
}

void Player_Update(double elapsed_time)
{

	// 先记住上一帧的位置
	XMVECTOR position = XMLoadFloat3(&g_PlayerPosition); // xmvector 只可以进行运算，无法直接取用
	XMVECTOR velocity = XMLoadFloat3(&g_PlayerVelocity);
	//XMVECTOR gvelocity{};


	if (KeyLogger_IsTrigger(KK_J) && !g_IsJump)
	{
		velocity += { 0.0f , 20.0f , 0.0f };

		g_IsJump = true;
	}

	//gravity
	XMVECTOR gdir{ 0.0f, 1.0f, 0.0f};
	velocity  += gdir * -9.8f * 4.0f * (float)elapsed_time;
	position  += velocity * (float)elapsed_time;

	
	for (int i = 0; i < Map_GetObjectsCount(); i++)
	{
		AABB player = Player_ConvertPositionToAABB(position);

		AABB object = Map_GetObjects(i) ->Collision_AABB;
		Hit hit = Collision_IsHitAABB(object, player);

		if (hit.isHit )
		{
			if (hit.normal.y > 0.0f   )// うえにたぶんのっかた
			{
				position = XMVectorSetY(position, object.max.y);
				velocity *= { 1.0f, 0.0f, 1.0f};
				g_IsJump = false;
			}
		}
	}
	

	//重力に引っ張られた関係で物体に当たったか？
	//Y方向的碰撞判定
	


	//进入深度反射法
	// 
	//地面に当たったか?
	/*
	if (XMVectorGetY(position) < 0.0f) 
	{
		position = XMVectorSetY(position,0.0f);//落ちたことをなかったことにする
		velocity *= {1.0f, 0.0f, 1.0f};
		g_IsJump = false;
	}
	
	*/
	XMVECTOR direction{};
	XMVECTOR front = XMLoadFloat3(&PlayerCamera_GetFront()) * XMVECTOR { 1.0, 0.0, 1.0 };


	if (KeyLogger_IsPressed(KK_W))//constrain axis y
	{
		direction += front ;
		 
	}

	if (KeyLogger_IsPressed(KK_S))//constrain axis y
	{
		direction -= front ;
		 
	}

	if (KeyLogger_IsPressed(KK_A))
	{
		direction -= XMVector3Cross({ 0.0f , 1.0f , 0.0f }, front);
		
	}

	if (KeyLogger_IsPressed(KK_D))
	{
		direction += XMVector3Cross({ 0.0f , 1.0f , 0.0f }, front);
		
	}

	
	if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f)
	{
		direction = XMVector3Normalize(direction);
		
		
		//XMStoreFloat3(&g_PlayerFront, direction);
		
		

		//2つのベクトルのナス角度は
		float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_PlayerFront), direction));
		float angle = acosf(dot);

		//回転速度
		const float ROTATION_SPEED = XM_2PI * 2.0f * (float)elapsed_time;

		if (angle < ROTATION_SPEED)
		{
			front = direction;

		}
		else {
			//向きたい方向が右回りか左回りか
			XMMATRIX r = XMMatrixIdentity();

			if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_PlayerFront), direction)) < 0.0f)
			{
				r = XMMatrixRotationY( -ROTATION_SPEED);

			}
			else
			{
				r = XMMatrixRotationY(ROTATION_SPEED);
			}
			front = XMVector3TransformNormal(XMLoadFloat3(&g_PlayerFront), r);
			
			
		}

		velocity += front * (float)(1600.0 / 50.0 * elapsed_time);
		XMStoreFloat3(&g_PlayerFront, front); //前向量处理最后部分
	}

	
	velocity += -velocity*(float)(10.0 * elapsed_time);//抵抗移動作用
	position += velocity * (float)elapsed_time;

//移动后玩家 和物体的碰撞判定

	for (int i = 0; i < Map_GetObjectsCount(); i++)
	{
		AABB player = Player_ConvertPositionToAABB(position);

		AABB object = Map_GetObjects(i)->Collision_AABB;
	
		Hit hit = Collision_IsHitAABB(object, player);
		if (hit.isHit)
		{
			if (hit.normal.x > 0.0f)// うえにたぶんのっかた
			{
				position = XMVectorSetX(position, object.max.x + 1.0f );
				velocity *= { 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.x < 0.0f)
			{
				position = XMVectorSetX(position, object.min.x - 0.5f);
				velocity *= { 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.y < 0.0f)
			{
				position = XMVectorSetY(position, object.min.y - 2.0f);
				velocity *= { 1.0f,0.0f, 1.0f };
			}
			else if (hit.normal.z > 0.0f)
			{
				position = XMVectorSetZ(position, object.max.z + 1.0f);
				velocity *= { 1.0f, 1.0f, 0.0f };
			}
			else if (hit.normal.z < 0.0f)
			{
				position = XMVectorSetZ(position, object.min.z - 1.0f);
				velocity *= { 1.0f, 1.0f, 0.0f };
			}
		
		}
	}

	XMStoreFloat3(&g_PlayerPosition, position );
	XMStoreFloat3(&g_PlayerVelocity, velocity);

	if (KeyLogger_IsTrigger(KK_SPACE))
	{
		XMFLOAT3 velocity1;
		XMFLOAT3 shot_position = g_PlayerPosition;
		shot_position.y += 1.0f;
		shot_position.z -= 1.0f;
		XMStoreFloat3(&velocity1, XMLoadFloat3(&g_PlayerFront) * 10.0f);
		Bullet_Create(shot_position, velocity1);

		
	}
	
}

void Player_Draw()
{
	Shader3d_BeginPassOnce();
	Texture_SetTexture(g_DiffuseColorTex, 0);
	Texture_SetTexture(g_NormalMapTex, 1);
	Texture_SetTexture(g_MetallicMapTex, 2);
	Texture_SetTexture(g_RoughnessMapTex, 3);

	 float metallic  = 0.0f;
            float roughness = 0.8f;   // 越大越糙
            float ao        = 1.0f;

            //Light_Set_PBRParams(Camera_GetPosition(), metallic, roughness, ao);

	//float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_PlayerFront),XMVECTOR{1.0 , 0.0 , 0.0}));
	                              //  the result of XMVector3Dot is  { result , result , result} vector
	//float angle = acosh(dot) + XMConvertToRadians(270);
	float angle = -atan2(g_PlayerFront.z, g_PlayerFront.x) + XMConvertToRadians(270);

	XMMATRIX r = XMMatrixRotationY(angle);
	XMMATRIX t = XMMatrixTranslation(g_PlayerPosition.x, g_PlayerPosition.y + 2.5f, g_PlayerPosition.z);
	XMMATRIX world = r * t;

	//ModelDraw(g_pPlayerModel, world);
}

const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_PlayerPosition;
	// TODO: return ステートメントをここに挿入します
}

const DirectX::XMFLOAT3& Player_GetFrontVector()
{
	return g_PlayerFront;
	// TODO: return ステートメントをここに挿入します
}

AABB Player_GetAABB()
{
	return {
			   {g_PlayerPosition.x -1.0f,
				g_PlayerPosition.y ,
				g_PlayerPosition.z - 1.0f
				 } ,	 

			   {g_PlayerPosition.x + 1.0f,//模型宽度
				g_PlayerPosition.y + 2.0f,//模型高度
				g_PlayerPosition.z + 1.0f}//

	};
	
}

AABB Player_ConvertPositionToAABB(const DirectX::XMVECTOR& position)
{
	AABB aabb;
	XMStoreFloat3(&aabb.min, position - XMVectorSet(1.0f, 0.1f, 1.0f, 0.0f));;
	XMStoreFloat3(&aabb.max, position + XMVectorSet( 1.0f,   2.0f , 1.0f , 0.0f));


	return aabb;
}



