#include "fade.h"
#include <algorithm>
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"
using namespace DirectX;

static double g_Fade_Time = 0.0;
static double g_FadeStartTime{ 0.0 };
static double g_AccumulatedTime{ 0.0 };
static XMFLOAT3 g_Color{ 0.0f, 0.0f , 0.0f };
static float g_Alpha = 0.0f;
static FadeState g_State = FADE_STATE_NONE;
static int g_FadeTexId = -1;




void Fade_Initialize()
{
	g_Fade_Time = 0.0;
	g_FadeStartTime=0.0;
	g_AccumulatedTime=0.0;
	g_Color={ 0.0f, 0.0f , 0.0f };
	g_Alpha = 0.0f;
	g_State = FADE_STATE_NONE;

	g_FadeTexId = Texture_Load(L"resource/texture/white.png");
}

void Fade_Finalize()
{
}

void Fade_Update(double elapsed_time)
{
	if (g_State == FADE_STATE_FINISHED_OUT ||
		g_State == FADE_STATE_FINISHED_IN ||
		g_State == FADE_STATE_NONE
		) return;

	g_AccumulatedTime += elapsed_time;

	double ratio = std::min((g_AccumulatedTime - g_FadeStartTime) / g_Fade_Time, 1.0); //向透明变化
	
	if (ratio >= 1.0) {

		g_State = g_State == FADE_STATE_IN ? FADE_STATE_FINISHED_IN : FADE_STATE_FINISHED_OUT;

	}
	
	g_Alpha = (float)(g_State == FADE_STATE_IN ? 1.0 - ratio : ratio);
}

void Fade_Draw()
{
	if (g_State == FADE_STATE_NONE) return;
	if (g_State == FADE_STATE_FINISHED_IN) return;
	
	
	XMFLOAT4 color{ g_Color.x ,g_Color.y, g_Color.z ,  g_Alpha };
	Sprite_Draw(g_FadeTexId, 0.0f, 0.0f, Direct3D_GetBackBufferWidth()*1.0f, Direct3D_GetBackBufferHeight() * 1.0f, color);
}

void Fade_Start(double time, bool IsFadeOut, DirectX::XMFLOAT3 color)
{
	g_FadeStartTime = g_AccumulatedTime;
	g_Fade_Time = time; 
	g_State = IsFadeOut ? FADE_STATE_OUT : FADE_STATE_IN;
	g_Color = color;
	g_Alpha = IsFadeOut ? 0.0f : 1.0f;
}

FadeState Fade_GetState()
{
	return g_State;
}
