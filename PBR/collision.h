/*==============================================================================

   Collision judgement [bullet.h]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef COLLISION_H
#define COLLISION_H


#include <d3d11.h>
#include <DirectXMath.h>


struct Circle
{
	DirectX::XMFLOAT2 center;//圆的中心坐标
	float radius;
};

struct Box
{
	DirectX::XMFLOAT2 center;
	float half_width;
	float half_height;
};


struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;

	DirectX::XMFLOAT3 GetCenter() const
	{
		DirectX::XMFLOAT3 center;

		center.x = min.x + (max.x - min.x) * 0.5f;
		center.y = min.y + (max.y - min.y) * 0.5f;
		center.z = min.z + (max.z - min.z) * 0.5f;
		
		return center;
	}


	
};

struct Hit
{
	bool isHit = false;
	float depth = 0.0f;
	DirectX::XMFLOAT3 normal{ 0.0f, 0.0f, 0.0f }; // 碰撞到物体的那面的法线
};

bool Collision_OverlapCircleVsCircle(const Circle& a, const Circle& b);

bool Collision_OverlapBoxVsBox(const Box& a, const Box& b);

/* ===============================================================================*/

bool Collisioin_IsOverlappAABB(const AABB& a , const AABB& b);


//進入検測　方法１
Hit Collision_IsHitAABB(const AABB& a, const AABB& b);//b向a移动时的碰撞检测
				//对于物体a，b是从a的那个面移动过来发生碰撞的？

//检测玩家附近的物体，只对这些物体进行检测

//culling处理

//swept AABB方法２


/* ===============================================================================*/
void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_DebugDraw(const Circle& circle , const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 0.0f , 1.0f});
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color = { 1.0f , 1.0f , 0.0f , 1.0f });
void Collision_DebugFinalize();
#endif  // COLLISION_H