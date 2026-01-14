#include "Sprite_Anim.h"
#include "sprite.h"
#include "texture.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "billboard.h"



struct AnimPatternData
{
	int m_TextureId = -1; //texture id  
	int m_PatternMax = 0; //pattern count  这里是一行的图片数量
	int m_HPatternMax = 0; //横向的图片个数   即横向有几个图片
	XMUINT2 m_StartPosition = { 0, 0 }; // アニメショーンのスタート座標
	XMUINT2 m_PatternSize{ 0 ,0 }; //１つパタンの幅 と　高さ
	//int m_PatternWidth = 0; // １つパタンの幅
	//int m_PatternHeight = 0; // 1パタンの幅
	double m_seconds_per_pattern = 0.1; 
	bool m_IsLooped = true;  //stop at last farme        &g_AnimPattern[g_AnimPlay[i].m_PatternId]

};
struct AnimPlayData
{
	int m_PatternId = -1; // アニメショーンパタンID
	int m_PatternNum = 0; //current pattern  现在生成中的图片 現在再生中のパタン番号
	double m_accumulated_time = 0.0; //累计时间
	bool m_IsStopped=false;
};

static constexpr int ANIM_PATTERN_MAX = 128;
static AnimPatternData g_AnimPattern[ANIM_PATTERN_MAX];
static constexpr int ANIM_PLAY_MAX = 256;
static AnimPlayData g_AnimPlay[ANIM_PLAY_MAX];




void SpriteAnim_Initialize()
{

	for (AnimPatternData& data : g_AnimPattern)
	{
		data.m_TextureId = -1;
		//data.m_TextureId = false;
	}

	for (AnimPlayData& data : g_AnimPlay)
	{
		data.m_PatternId = -1;
	}
	/*
	//g_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[0].m_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[0].m_PatternMax = 8;
	g_AnimPattern[0].m_PatternSize = {32,32};
	g_AnimPattern[0].m_StartPosition = {32*2,32*5};
	g_AnimPattern[0].m_IsLooped = true;
	g_AnimPlay[0].m_PatternId = 0;
	

	g_AnimPattern[1].m_TextureId = Texture_Load(L"kokosozai.png");
	g_AnimPattern[1].m_PatternMax = 13;
	g_AnimPattern[1].m_PatternSize = { 32,32 };
	g_AnimPattern[1].m_StartPosition = { 0,32 };
	g_AnimPattern[0].m_IsLooped = false;
	g_AnimPlay[1].m_PatternId = 1;
	*/
	
}


void SpriteAnim_Finalize()
{

}
void SpriteAnimUpdate(double elasped_time)
{
	for (int i = 0; i < ANIM_PLAY_MAX; i++)
	{
		if (g_AnimPlay[i].m_PatternId < 0) continue;

		AnimPatternData* pAnimPatternData = &g_AnimPattern[g_AnimPlay[i].m_PatternId];

		if (g_AnimPlay[i].m_accumulated_time >= pAnimPatternData->m_seconds_per_pattern)
		{
			g_AnimPlay[i].m_PatternNum++;
			

			if (g_AnimPlay[i].m_PatternNum >= pAnimPatternData->m_PatternMax)
			{
				if (pAnimPatternData->m_IsLooped) {
					g_AnimPlay[i].m_PatternNum = 0;
				}

				else {
					g_AnimPlay[i].m_PatternNum = pAnimPatternData->m_PatternMax - 1;
					g_AnimPlay[i].m_IsStopped = true;
				}

				
			}
			g_AnimPlay[i].m_accumulated_time -= 0.1;

			
		}
		g_AnimPlay[i].m_accumulated_time += elasped_time;
	}
}

void SpriteAnim_Draw(int playid,float dx, float dy, float dw, float dh) // dw = drawing width  dh = drawing height
{
	int anim_pattern_id = g_AnimPlay[playid].m_PatternId;
	AnimPatternData* pAnimPatternData = &g_AnimPattern[anim_pattern_id];


	
		Sprite_Draw(pAnimPatternData->m_TextureId,
			dx, dy, dw, dh,
			float(pAnimPatternData->m_StartPosition.x
			+ float(pAnimPatternData->m_PatternSize.x)
			* (g_AnimPlay[playid].m_PatternNum % pAnimPatternData->m_HPatternMax)),

			float(pAnimPatternData->m_StartPosition.y
			+ pAnimPatternData->m_PatternSize.y
			* (g_AnimPlay[playid].m_PatternNum / pAnimPatternData->m_HPatternMax)),

			pAnimPatternData->m_PatternSize.x,
			pAnimPatternData->m_PatternSize.y
	
	
		);




}



int SpriteAnim_RegisterPattern(int texid, int pattern_Max,
	int h_pattern_max ,
	double second_per_pattern,
	const DirectX::XMUINT2& pattern_size, 
	const DirectX::XMUINT2& start_position,
	bool is_looped)
{
	for (int i = 0; i < ANIM_PATTERN_MAX; i++)
	{
		//空いてる場所探す
		if (g_AnimPattern[i].m_TextureId >= 0) continue;


		g_AnimPattern[i].m_TextureId = texid;
		g_AnimPattern[i].m_PatternMax = pattern_Max;
		g_AnimPattern[i].m_HPatternMax = h_pattern_max;
		g_AnimPattern[i].m_seconds_per_pattern = second_per_pattern;
		g_AnimPattern[i].m_PatternSize = pattern_size;
		g_AnimPattern[i].m_StartPosition = start_position;
		g_AnimPattern[i].m_PatternSize = pattern_size;
		g_AnimPattern[i].m_IsLooped = is_looped;

		return i; //      anim_pattern_id
	}
	return -1;
}

int SpriteAnim_CreatePlayer(int anim_pattern_id)
{
	for (int i = 0; i < ANIM_PLAY_MAX; i++)
	{
		if (g_AnimPlay[i].m_PatternId >= 0) continue;

		g_AnimPlay[i].m_PatternId = anim_pattern_id;
		g_AnimPlay[i].m_accumulated_time = 0.0;
		g_AnimPlay[i].m_PatternNum = 0;
		g_AnimPlay[i].m_IsStopped = false; 

		return i;
	}

	return -1;
}

bool SpriteAnim_IsStopped(int index)
{
	
	return g_AnimPlay[index].m_IsStopped;

}

void SpriteAnim_DestroyPlayer(int index)
{
	g_AnimPlay[index].m_PatternId = -1;


}



void BillboardAnim_Draw(int playid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT2& pivot)
{
	int anim_pattern_id = g_AnimPlay[playid].m_PatternId;
	AnimPatternData* pAnimPatternData = &g_AnimPattern[anim_pattern_id];

	BillBoard_Draw(pAnimPatternData->m_TextureId,
		position, scale,
		{pAnimPatternData->m_StartPosition.x
			+ pAnimPatternData->m_PatternSize.x
			* (g_AnimPlay[playid].m_PatternNum % pAnimPatternData->m_HPatternMax),

		pAnimPatternData->m_StartPosition.y
			+ pAnimPatternData->m_PatternSize.y
			* (g_AnimPlay[playid].m_PatternNum / pAnimPatternData->m_HPatternMax),

		pAnimPatternData->m_PatternSize.x,
		pAnimPatternData->m_PatternSize.y },
		pivot);


}