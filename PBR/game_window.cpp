#include "game_window.h"
#include <algorithm>
#include "keyboard.h"
#include "mouse.h"
//WINDOW PROCEDURE PROTOTYPE CLARE
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


//WINDOW Information
static constexpr char WINDOW_CLASS[] = "GameWindow";
static constexpr char TITLE[] = "SpaceSoldier";
constexpr int SCREEN_WIDTH = 1600;
constexpr int SCREEN_HEIGHT = 900;


HWND GameWindowCreate(_In_ HINSTANCE hInstance)
{
	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc; //必须的windowprocedure
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&wcex);


	RECT window_rect{ 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };

	const int WINDOW_WIDTH = window_rect.right - window_rect.left;
	const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top;

	int desktop_width = GetSystemMetrics(SM_CXSCREEN);
	int desktop_height = GetSystemMetrics(SM_CYSCREEN);


	DWORD style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
	AdjustWindowRect(&window_rect, style, FALSE);

	const int window_x = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
	const int window_y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

	HWND hWnd = CreateWindow(WINDOW_CLASS,
		TITLE,
		style,//窗口类型（当前为普通类型） 
		window_x,//窗口左上角在屏幕中的位置
		window_y,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		nullptr, //this two nullptr is used for parent window and menu, indicating no parent and no menu
		nullptr,
		hInstance,
		nullptr);

	return hWnd;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATEAPP:
		Keyboard_ProcessMessage(message, wParam, lParam);
		Mouse_ProcessMessage(message, wParam, lParam);
		break;
	
		     case WM_INPUT:
		     case WM_MOUSEMOVE:
		     case WM_LBUTTONDOWN:
		     case WM_LBUTTONUP:
		     case WM_RBUTTONDOWN:
		     case WM_RBUTTONUP:
		     case WM_MBUTTONDOWN:
		     case WM_MBUTTONUP:
		     case WM_MOUSEWHEEL:
		     case WM_XBUTTONDOWN:
		     case WM_XBUTTONUP:
		     case WM_MOUSEHOVER:
		         Mouse_ProcessMessage(message, wParam, lParam);
		         break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);

		}
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		         Keyboard_ProcessMessage(message, wParam, lParam);
		         break;

	case WM_CLOSE:
		if (MessageBox(hWnd, "終了してよろしですか?", "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
		{
			DestroyWindow(hWnd);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;


}