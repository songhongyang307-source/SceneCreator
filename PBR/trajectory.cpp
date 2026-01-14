/*==============================================================================

  軌跡エフェクトの描画[trajectory.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/01
--------------------------------------------------------------------------------

==============================================================================*/



#include "trajectory.h"
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
using namespace DirectX;


struct Trajectory
{
	XMFLOAT2 position;
	XMFLOAT4 color;
	float size;
	double lifetime;
	double spawnTime;//if spawntime == 0 ,
					//its not in use
};

static constexpr unsigned int TRAJECTORYMAX = 4096;
static Trajectory g_Trajectories[TRAJECTORYMAX]={};
static int g_TrajectoryTexId = -1;
static double g_Time = 0.0; //glabol time





void Trajectory_Initialize()
{
	for (Trajectory& t : g_Trajectories)
	{
		t.spawnTime = 0.0;

	}
	g_Time = 0.0;


	g_TrajectoryTexId = Texture_Load(L"resource/texture/effect0.jpg");
}

void Trajectory_Update(double elapsed_time)
{
	for (Trajectory& t : g_Trajectories)
	{
		if (t.spawnTime <= 0.0) continue;
		double time = g_Time - t.spawnTime;
		if (time > t.lifetime)
		{
			t.spawnTime = 0.0;//life runout
		}
	}

	g_Time += elapsed_time; // time update
}

void Trajectory_Finalize()
{
}

void Trajectory_Draw()
{
	Direct3D_SetAlphaBlendAdd();
	for (const Trajectory& t : g_Trajectories)
	{
		if (t.spawnTime <= 0.0) continue;

		double time = g_Time - t.spawnTime; //HOW long since spawned
		float ratio = (float)(time / t.spawnTime);
	
		float size = t.size * (1.0f - ratio);
		float half_size = size * 0.5f;
		XMFLOAT4 color = t.color;
		color.w = t.color.w * (1.0f - ratio);

		Sprite_Draw(g_TrajectoryTexId,
			t.position.x - half_size,
			t.position.y - half_size, 
			size,size,
			t.color
		);
	}
	Direct3D_SetAlphaBlendTransparent();
	//

}

void Trajectory_Create(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT4& color, float size, double lifetime)
{

	for (Trajectory& t : g_Trajectories)
	{
		if (t.spawnTime != 0.0)continue;

		t.spawnTime = g_Time;
		t.lifetime = lifetime;
		t.color = color;
		t.position = position;
		t.size = size;
		break;//不退出的话将生成4096个最大数的拖尾
	}

}

