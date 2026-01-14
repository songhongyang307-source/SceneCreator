/*------------------------------------------------------------------------


  keyboard input

------------------------------------------------------------------------*/

#include "debug_ostream.h"
#include "Keylogger.h"
#include "debug_text.h"
static Keyboard_State g_PrevState{}; // 前一帧的鼠标输入情况信息
static Keyboard_State g_TriggerState{};
static Keyboard_State g_ReleaseState{};

void KeyLogger_Initialize()
{
	Keyboard_Initialize();
}

void KeyLogger_Update()
{
	const Keyboard_State* pState = Keyboard_GetState();
	//(LPBYTE) unisigned char point type

	LPBYTE pn = (LPBYTE)pState; //现在的键盘状态
	LPBYTE pp = (LPBYTE)&g_PrevState;//上一针的键盘状态
	LPBYTE pt = (LPBYTE)&g_TriggerState;
	LPBYTE pr = (LPBYTE)&g_ReleaseState;

	//0 1 -> 1      0 1 -> 0
	//1 0 -> 0		1 0 -> 1
	//1 1 -> 0		1 1 -> 0	
	//0 0 -> 0		0 0 -> 0	
	for (int i = 0; i < sizeof(Keyboard_State); i++)
	{
		pt[i] = (pp[i] ^ pn[i]) & pn[i]; //
		pr[i] = (pp[i] ^ pn[i]) & pp[i];
	}


	g_PrevState = *pState;

}
bool KeyLogger_IsPressed(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key);
}

bool KeyLogger_IsTrigger(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key, &g_TriggerState);
}

bool KeyLogger_IsReleased(Keyboard_Keys key)
{
	return Keyboard_IsKeyDown(key, &g_ReleaseState);
}
