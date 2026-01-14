//#include <sstream>        // 
//#include "Debug_Ostream.h"



/*
* constexpr char FILE_NAME[] = "tekito.png";
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	std::stringstream ss;

	hal::dout << "debug for there____________________________________________________________________________\n";

	ss << "テクスチャファイル: " << FILE_NAME << "が読み込めませんでした";

	int a = MessageBox(nullptr, ss.str().c_str(), "キャプション", MB_YESNOCANCEL | MB_DEFBUTTON3); //https://learn.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-messagebox?devlangs=cpp&f1url=%3FappId%3DDev17IDEF1%26l%3DJA-JP%26k%3Dk(WINUSER%2FMessageBox)%3Bk(MessageBox)%3Bk(DevLang-C%2B%2B)%3Bk(TargetOS-Windows)%26rd%3Dtrue
																					//" MB_YESNOCANCEL | MB_ICONERROR      二者对应的比特位进行或运算  "
	hal::dout << "debug for there 2____________________________________________________________________________\n";																		//MB_DEFBUTTON3/2/1   确定
	if (a == IDYES)
	{
		MessageBox(nullptr, "Yes", "YES", MB_OK);
	}
	else if(a == IDCANCEL)
	{
		MessageBox(nullptr, "キャンセル", "キャンセル____________________________________________________________________________\n", MB_OK);
	}
	hal::dout << "debug for there 3";
	return 0;

	*/