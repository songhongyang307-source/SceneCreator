/*==============================================================================

			Score Controller[score.cpp]
														 Author : satou youhei
														 Date   : 2025/07/09
--------------------------------------------------------------------------------

==============================================================================*/

#include "score.h"
#include "texture.h"
#include  "sprite.h" 
#include <algorithm>

static unsigned int g_Score = 0;
static unsigned int g_ViewScore = 0;
static int g_Digit = 1;
static unsigned int g_CounterStop = 1;
static float g_x = 0.0f, g_y = 0.0f;
static int g_scoreTexId = -1;
static constexpr int ScoreFontWidth = 36;
static constexpr int scoreFontHeight = 36;


static void DrawNum(float x, float y, int num);


void Score_Initialize(float x, float y, int digit)
{
	g_Score = 0;
	g_Digit = digit;
	g_x = x;
	g_y = y;


	//カンストの得点を作る
	for (int i = 0; i < digit; i++)
	{
		g_CounterStop *= 10;

	}
	g_CounterStop--;

	g_scoreTexId = Texture_Load(L"resource/texture/numeric.png"); //313*27
} 

void Score_Finalize()
{
}

void Score_Update()
{

	g_ViewScore = std::min(g_ViewScore + 1, g_Score);

}

void Score_Draw()
{

	unsigned int temp = std::min(g_Score , g_CounterStop);

	for (int i = 0; i < g_Digit; i++)
	{
		int n = temp % 10;

		float x = g_x + (g_Digit - 1 - i) * ScoreFontWidth;
		DrawNum(x, g_y, n);

		temp /= 10;
		
	}
}

unsigned int Score_GetScore()
{
	return g_Score;
}

void Score_Increment(int score)
{
	g_ViewScore = g_Score;
	g_Score += score;
}

void Score_Reset()
{
	g_Score = 0;
}

void Score_DrawHighScore(float x, float y, unsigned int highscore)
{
	unsigned int temp = highscore;

	for (int i = 0; i < g_Digit; i++)
	{
		int n = temp % 10;

		float lx = x + (g_Digit - 1 - i) * ScoreFontWidth;

		Sprite_Draw(g_scoreTexId, lx, y,float(ScoreFontWidth*n), 0 , ScoreFontWidth , scoreFontHeight,{ 1.0f, 1.0f, 0.0f, 1.0f });

		temp /= 10;
	}
}

void DrawNum(float x, float y, int num)
{
	Sprite_Draw(g_scoreTexId, x, y, 16.0f, 16.0f,float(num * ScoreFontWidth) , 0.0f, 31, 37);

}
