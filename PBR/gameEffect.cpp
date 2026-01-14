#include "gameEffect.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "texture.h"
#include "sprite_anim.h"
#include "Audio.h"


struct Effect
{
	XMFLOAT2 position;
	//XMFLOAT2 velocity;
	int sprite_anim_id;
	bool IsEnabled;
	
	
};

static constexpr int EFFECT_MAX = 256;
static Effect g_Effects[EFFECT_MAX]{};
static int g_EffectTexId = -1;
static int g_AnimPatternId = -1;
static int g_EffectSoundId = -1;

void Effect_Initialize()
{

	for (Effect& e : g_Effects) {

		e.IsEnabled = false;
	}

	g_EffectTexId = Texture_Load(L"resource/texture/Space Ships Explosion.png");  // 58 * 35
	g_EffectSoundId = LoadAudio("resource/Audio/explosion1.wav");
	
	g_AnimPatternId = SpriteAnim_RegisterPattern(g_EffectTexId, 6 , 6 , 0.01 , {58,35},{0 , 0} , false);
	//g_BulletTexId = Texture_Load(L"resource/texture/kokosozai.png");

	

}

void Effect_Update(double elapsed_time)
{
	elapsed_time += 0;
	for (Effect& e : g_Effects) {

		if (!e.IsEnabled) continue;

		if (SpriteAnim_IsStopped(e.sprite_anim_id))
		{
			SpriteAnim_DestroyPlayer(e.sprite_anim_id);
			e.IsEnabled = false;
		}

	}

}

void Effect_Finalize()
{
	UnloadAudio(g_EffectSoundId);
	

}

void Effect_Draw()
{

	for (Effect& e : g_Effects) {

		if (!e.IsEnabled) continue;

		SpriteAnim_Draw(e.sprite_anim_id, e.position.x, e.position.y, 58.0f, 35.0f);//EFFECT SIZE 

	}

}


void Effect_Create(const DirectX::XMFLOAT2& position)
{

	for (Effect& e : g_Effects) {

		if (e.IsEnabled) continue;

		//find empty
		e.IsEnabled = true;
		e.position = position;
		e.sprite_anim_id = SpriteAnim_CreatePlayer(g_AnimPatternId);
		PlayAudio(g_EffectSoundId); // 播放音效
		break;

	}

}
