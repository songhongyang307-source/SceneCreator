/*==============================================================================

   Effect of bullt on collision[bullet_hit_effect.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/



#include "bullet_hit_effect.h"
#include "texture.h"
using namespace DirectX;
#include "Sprite_Anim.h"
#include "billboard.h"




static int g_TexId = -1;
static int g_AnimPatternId = -1;


class BulletHitEffect 
{

	private:

		XMFLOAT3 m_position{};
		int m_anim_play_id{ -1 };
		bool m_is_destroy{ false };
		

	public:
		BulletHitEffect(const XMFLOAT3& position)
			: m_position(position) , m_anim_play_id(SpriteAnim_CreatePlayer(g_AnimPatternId)){
		}

		void Update();

		void Draw() const;

		bool IsDestroy() const {
			return m_is_destroy;
		}

		~BulletHitEffect() {
			SpriteAnim_DestroyPlayer(m_anim_play_id);
		}

};

static constexpr int EFFECT_MAX = 256;
static BulletHitEffect* g_pEffects[EFFECT_MAX]{};
static int g_EffectCount = 0;


void BulletHitEffect_Initialize()
{
	g_TexId = Texture_Load(L"resource/texture/explode01.jpg"); // 1280 * 748   3 * 5 / 256 * 246

	g_AnimPatternId = SpriteAnim_RegisterPattern(
		g_TexId, 15, 5, 0.1, { 256 , 246 }, { 0 , 0 },false
	);


}

void BulletHitEffect_Finalize()
{
	for (int i = 0; i < g_EffectCount; i++)
	{
		delete g_pEffects[i];
	
	}
}

void BulletHitEffect_Update(double elapsedtime)
{
	for (int i = 0; i < g_EffectCount; i++)
	{
	g_pEffects[i] -> Update();
	}

	for (int i = g_EffectCount-1; i >= 0; i--)
	{
		if (g_pEffects[i]->IsDestroy())
		{
			delete g_pEffects[i];
			g_pEffects[i] = g_pEffects[g_EffectCount - 1];
			g_EffectCount--;
		}
	}
}

void BulletHitEffect_Draw()
{
	for (int i = 0; i < g_EffectCount; i++)
	{
		g_pEffects[i]->Draw();
	}
}

void BulletHitEffect_Create(const DirectX::XMFLOAT3& position)
{
	g_pEffects[g_EffectCount++] = new BulletHitEffect(position);
}

void BulletHitEffect::Update()
{
	if (SpriteAnim_IsStopped(m_anim_play_id))
	{
		m_is_destroy = true;
	}
}

void BulletHitEffect::Draw() const
{
	BillboardAnim_Draw(m_anim_play_id, m_position, { 1.0f , 1.0f });//半透明最后描画
}
