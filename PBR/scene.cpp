/*==============================================================================

			画面遷移制御[scene.h]
														 Author : SONG HONGYANG
														 Date   : 2025/07/10
--------------------------------------------------------------------------------

==============================================================================*/

#include "scene.h"
#include "game.h"
#include "title.h"


static Scene g_Scene = SCENE_GAME;//Debug时将其设为SCENE_GAME
static Scene g_SceneNext = g_Scene;



void Scene_Initialize()
{
	switch (g_Scene)
	{
	case SCENE_TITLE:
		Title_Initialize();
		break;
	case SCENE_GAME:
		Game_Initialize();
		break;
	case SCENE_RESULT:
		break;
	default:
		break;
	}
}

void Scene_Finalize()
{
	switch (g_Scene)
	{
	case SCENE_TITLE:
		Title_Finalize();
		break;
	case SCENE_GAME:
		Game_Finalize();
		break;
	case SCENE_RESULT:
		break;
	default:
		break;
	}
}

void Scene_Update(double elapsed_time)
{
	switch (g_Scene)
	{
	case SCENE_TITLE:
		Title_Update(elapsed_time);
		break;
	case SCENE_GAME:
		Game_Update(elapsed_time);
		break;
	case SCENE_RESULT:
		break;
	default:
		break;
	}
}

void Scene_Draw()
{
	switch (g_Scene)
	{
	case SCENE_TITLE:
		Title_Draw();
		break;
	case SCENE_GAME:
		Game_Draw();
		break;
	case SCENE_RESULT:
		break;
	default:
		break;
	}
}


void Scene_Change(Scene scene)
{
	g_SceneNext = scene;
}

void Scene_Refresh()
{

	if (g_Scene != g_SceneNext)
	{
		//现在的场景结束
		Scene_Finalize();

		g_Scene = g_SceneNext;

		//下一个场景的初始化开始
		Scene_Initialize();
		
	}
}
