#include <Windows.h>
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include "game_window.h"
#include "direct3d.h"
#include "sprite.h"
#include "shader.h"
#include "texture.h"
#include "Sprite_Anim.h"
#include "fade.h"
#include "collision.h"
#include "debug_text.h"
#include <sstream>
#include "system_timer.h" //direct 3d toolkit
#include <DirectXmath.h>
using namespace DirectX;
#include "keylogger.h"
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#include "mouse.h"
#include "scene.h"
#include "Audio.h"
#include "light.h"
#include "grid.h"
#include "cube.h"
#include "shader3d .h"
#include "shader3d_unlit.h"
#include "shader_shadow.h"
#include "shader_water.h"
#include "sampler.h"
#include "meshfield.h"
#include "shader_volumetric_fog.h"
#include "shader_lightshaft.h"




//void ProcessInput(UINT message, WPARAM wParam, float& acc, bool& isjumped);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{ 
	(void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	hPrevInstance = hPrevInstance;
	lpCmdLine = lpCmdLine;

	//win 11 屏幕调整 SetProcessDpiAwareness
	//SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
	// WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME   （禁止放大缩小窗口）
	// WS_OVERLAPPEDWINDOW &  ~WS_THICKFRAME
	//显示制作的窗口

	HWND hWnd = GameWindowCreate(hInstance);

	SystemTimer_Initialize();
	KeyLogger_Initialize();
	Mouse_Initialize(hWnd);
	Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
	InitAudio();

	//Mouse_SetVisible(false);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	Direct3D_Initialize(hWnd);
	Light_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Shader3d_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	

	ShaderWater_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	ShaderFog::Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	ShaderLightShaft::Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Shader3dUnlit_Initialize();
	ShadowShader_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	
	//注意 任何初始化都要在D3D和shader初始化之后进行
	//Polygon_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

	

	Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());

	//不能是纯黑  不能有静止的画面 

	SpriteAnim_Initialize();

	Fade_Initialize();
	 
	Scene_Initialize();

	Grid_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	

#if defined(DEBUG) || defined(_DEBUG)
	UINT w = Direct3D_GetBackBufferWidth();
	UINT h = Direct3D_GetBackBufferHeight();
	if (w == 0) w = 1;
	if (h == 0) h = 1;

	hal::DebugText dt(Direct3D_GetDevice(), Direct3D_GetContext(),
		L"resource/texture/consolab_ascii_512.png",
		w, h,
		0.0f, 0.0f,
		0, 0,
		0.0f, 12.0f);


	Collision_DebugInitialize(Direct3D_GetDevice(), Direct3D_GetContext());

#endif
	

	

	


	//fps  フreーム計測用计算帧率

	double exec_last_time = SystemTimer_GetTime();
	double fps_last_time = exec_last_time;
	double current_time = 0.0;
	ULONG frame_count = 0;
	double fps = 0.0;

	
	
	//GAME LOOP  & MESSAGE LOOP
	MSG msg;
	//unreal中不能定义循环（被封存），只能制作event
	 //但是unity中可以修改循环
	do
	{ 
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))  //peekmessage使用时
		{ 
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			

			//fps 計測用
			current_time = SystemTimer_GetTime(); //最新时间
			double elapsed_time = current_time - fps_last_time; //fps計測用の経つ時間を計算    
			/*，在每个循环迭代中，current_time 会随着时间的推移不断更新，而 fps_last_time 只有在进行FPS计算后才会更新。这种设计允许代码在每经过一秒时计算一次FPS，而不是在每个帧都计算。

			这样做的目的是为了计算每秒的帧数（FPS），而不是每帧的帧数。通过这种方式，程序可以在每秒结束时计算出当前的平均帧率，并在下一个秒周期开始时重置帧计数。*/

			if (elapsed_time >= 1.0)
			{
				fps = frame_count / elapsed_time;
				fps_last_time = current_time; //fpsを測定した時間を保存
				frame_count = 0;   //カウントをクリア
			}




			//1/60sごとに実行
			elapsed_time = current_time - exec_last_time;
			if (elapsed_time >= (1.0 / 60.0)) 
			{
			
				//if (true){
				exec_last_time = current_time;

				//ゲーム更新

				KeyLogger_Update();
				
				Scene_Update(elapsed_time);

				//SpriteAnimUpdate(elapsed_time);

				Fade_Update(elapsed_time);
				//ゲームの描画
				Direct3D_Clear();

				//Sprite_Begin();

				Scene_Draw();
				Fade_Draw();
				
				//dt.Draw();
				//dt.Clear();

#if defined(DEBUG) || defined(_DEBUG)
				std::stringstream ss;
				ss << " fps : " << fps << std::endl;
				dt.SetText(ss.str().c_str());
				#endif

				Direct3D_Present();

				Scene_Refresh();

				frame_count++;
				
			}

		}


	} while (msg.message != WM_QUIT);

#if defined(DEBUG) || defined(_DEBUG)
	Collision_DebugFinalize();
#endif

	Cube_Finalize();
	Grid_Finalize();
	Light_Finalize();
	MeshField_Finalize();
	Scene_Finalize();

	Fade_Finalize();
	Sprite_Finalize();
	SpriteAnim_Finalize();
	Texture_Finalize();
	Sampler_Finalize();
	
	Shader3d_Finalize();
	Shader_Finalize();
	ShadowShader_Finalize();
	ShaderFog::Finalize();
	ShaderWater_Finalize();
	Shader3dUnlit_Finalize();
	Direct3D_Finalize();
	UninitAudio();
	Mouse_Finalize();
	return (int)msg.wParam;

}

