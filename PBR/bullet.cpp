/*==============================================================================

   BULLET Controller[bullet.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/



#include "bullet.h"
using namespace DirectX;
#include "model.h"


class Bullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	double m_AccumlatedTime{ 0.0 };
	static constexpr double LIFETIME = 3.0;

public:
	Bullet(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
		:m_position(position), m_velocity(velocity)
	{

	}

	void Update(double elapsed_time) {
		m_AccumlatedTime += elapsed_time;
		XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * float(elapsed_time));
	}
								 //const は自分自身のデータだけ使う?
	const XMFLOAT3& GetPosition() const{
		return m_position;
	}

	XMFLOAT3 GetFront() const{
		XMFLOAT3 front;
		XMStoreFloat3(&front, XMVector3Normalize(XMLoadFloat3(&m_velocity)));
		return front;
	}

	bool IsDestroy()const {

		return m_AccumlatedTime >= LIFETIME;

	}

};

static constexpr int MAX_BULLET = 1024;

static Bullet* g_pBullets[MAX_BULLET]{};
static int g_BulletsCount{ 0 };
static MODEL* g_pBulletModel{ nullptr };

void Bullet_Initialize()
{
	g_pBulletModel = ModelLoad("resource/mesh/test.fbx", false ,0.1f);
	
	for (int i = 0; i < g_BulletsCount; i++)
	{
		delete g_pBullets[i];
	}
	
	g_BulletsCount = 0;
}

void Bullet_Finalize()
{
	ModelRelease(g_pBulletModel);

	for (int i = 0; i < g_BulletsCount; i++)
	{
		delete g_pBullets[i];
	}

	g_BulletsCount = 0;
}

void Bullet_Update(double elapsedtime)
{
	

	for (int i = 0; i < g_BulletsCount; i++) 
	{
		if (g_pBullets[i]->IsDestroy())
		{
			Bullet_Destroy(i);
		}
	}

	for (int i = 0; i < g_BulletsCount; i++)
	{
		g_pBullets[i]->Update(elapsedtime);
	}

}

void Bullet_Draw()
{
	//
	
	
	XMMATRIX mtxWorld;

	for (int i = 0; i < g_BulletsCount; i++)
	{
		XMVECTOR position = XMLoadFloat3(&g_pBullets[i]->GetPosition());

		mtxWorld = XMMatrixTranslationFromVector(position);
		ModelDraw(g_pBulletModel, mtxWorld);
	}
}

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity )
{
	if (g_BulletsCount >= MAX_BULLET)
	{
		return;
	}

	g_pBullets[g_BulletsCount++] = new Bullet(position, velocity);
}

void Bullet_Destroy(int index)
{
	delete g_pBullets[index];
	g_pBullets[index] = g_pBullets[g_BulletsCount - 1];
	g_BulletsCount--;
}

AABB Bullet_GetAABB(int index)
{
	
	return Model_GetAABB(g_pBulletModel,g_pBullets[index]->GetPosition());

}

int Bullet_GetBulletCount()
{
	return g_BulletsCount;
}

const DirectX::XMFLOAT3& Bullet_GetPosition(int index)
{
	return g_pBullets[index]->GetPosition();
}