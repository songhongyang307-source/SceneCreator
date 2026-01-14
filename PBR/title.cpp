/*============================================================================================



//TITLE CONTROLLER [title.cpp]



---------------------------------------------------------------------------------------------


============================================================================================*/


#include "title.h"
#include "texture.h"
#include "sprite.h"
#include "keylogger.h"
#include "fade.h"
#include "scene.h"
static int g_TitleBgTexId = -1; //背景的纹理id

//枚举变量设置Switch（g_state）来实现阶段性的淡入淡出效果



void Title_Initialize()
{
	g_TitleBgTexId = Texture_Load(L"resource/texture/stars.png");
}

void Title_Finalize()
{
	//Texture_AllRelease();
}

void Title_Update(double elapsed_time)
{
	elapsed_time = elapsed_time;
	if(KeyLogger_IsTrigger(KK_ENTER))
	{
		Fade_Start(1.0, true);
	}

	if (Fade_GetState() == FADE_STATE_FINISHED_OUT)
	{
		Scene_Change(SCENE_GAME);
	}
}

void Title_Draw()
{
	Sprite_Draw(g_TitleBgTexId, 0.0f, 0.0f,1600,900);
}
