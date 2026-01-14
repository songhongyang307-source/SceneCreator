/*==============================================================================

  Player Camera Controller[playercamera.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/


#include "playercamera.h"
#include <DirectXMath.h>
#include "player.h"
#include "direct3d.h"




using namespace DirectX;

static XMFLOAT3 g_CameraFrontVec{ 0.0f , 0.0f ,	 1.0f };
static XMFLOAT3 g_CameraPosition{ 0.0f , 0.0f ,0.0f };

static XMFLOAT4X4 g_CameraMatrix{};

static XMFLOAT4X4 g_CameraPerspectiveMatrix{};


void PlayerCamera_Initialize()
{


}

void PlayerCamera_Update(double elapsed_time)
{
	//XMVECTOR position = XMLoadFloat3(&Player_GetPosition()) - XMLoadFloat3(&Player_GetFrontVector()) * 5.0f;
	XMVECTOR position = XMLoadFloat3(&Player_GetPosition());
	position *= {1.0f, 0.0f, 1.0f};

	XMVECTOR target = position;

    position += {5.0f,10.0f, -10.0f};

	// target = XMLoadFloat3(&Player_GetPosition());
	XMVECTOR front = XMVector3Normalize(target - position);
	XMStoreFloat3(&g_CameraPosition, position);
	XMStoreFloat3(&g_CameraFrontVec, front);

	//View coordinate 行列式制作
	XMMATRIX mtxView = XMMatrixLookAtLH(
		position,
		//eyePos,
		target, //视点
		{ 0.0f , 1.0f ,0.0f });  //EYEPOSITION  = CAMERA POSTIION   FOCUSPOSITION = REACH POSITION  UPDIRECTION = 摄像机当前朝向 


	//カメラ行列を保存
	XMStoreFloat4x4(&g_CameraMatrix, mtxView);


	//PERSPECTIVE 行列式制作
	//float constexpr fovAngleY = XMConvertToRadians(60.0f);
	float aspectRatio = (float)Direct3D_GetBackBufferWidth() / (float)Direct3D_GetBackBufferHeight();
	float nearz = 0.1f;
	float farz = 100.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(1.0f, aspectRatio, nearz, farz); //fovangle是弧度，这里已嵌入了角度换算 aspectRation的计算式一定要是小数类型变量  nearz 是从相机到屏幕的距离（不能为0，一定要是比0大的数（0.1）），farz是最远看到的距离（本次为100），大陆或者地面绘制时10000


	XMStoreFloat4x4(&g_CameraPerspectiveMatrix, mtxPerspective);
}

void PlayerCamera_Finalize(void)
{
}

const DirectX::XMFLOAT3& PlayerCamera_GetFront()
{
	return g_CameraFrontVec;
	// TODO: return ステートメントをここに挿入します
}

const DirectX::XMFLOAT3& PlayerCamera_GetPosition()
{
	return g_CameraPosition;
	// TODO: return ステートメントをここに挿入します
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix()
{

	return g_CameraMatrix;
	// TODO: return ステートメントをここに挿入します
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix()
{
	return g_CameraPerspectiveMatrix;
}
