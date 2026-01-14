/*==============================================================================

   Camera Controller[camera.cpp]
														 Author : Youhei Sato
														 Date   : 2025/09/11
--------------------------------------------------------------------------------

==============================================================================*/



#include "camera.h"
#include "direct3d.h"
#include "Keylogger.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "debug_text.h"
#include <sstream>







//多个相机时使用构造体数组
static XMFLOAT3 g_CameraPosition{ 0.0f , 0.0f , -5.0f };
static XMFLOAT3 g_CameraFrontVec{ 0.0f , 0.0f ,	 1.0f };
static XMFLOAT3 g_CameraUpVec	{ 0.0f , 1.0f ,  0.0f };
static XMFLOAT3 g_CameraRightVec{ 1.0f , 0.0f ,  0.0f };
//

static float g_Fov = XMConvertToRadians(60);
static constexpr float CAMERA_MOVE_SPEED = 5.0f;
static constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(60.0f);

static XMFLOAT4X4 g_CameraMatrix;
static XMFLOAT4X4 g_PerspectiveMatrix;

static hal::DebugText* g_pDT = nullptr;

static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr;//VERTEX BUFFER1 view
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr;//VERTEX BUFFER2 projection(perspective)


void Camera_Initialize(const DirectX::XMFLOAT3 position, const DirectX::XMFLOAT3 front,  const DirectX::XMFLOAT3 right)
{
	Camera_Initialize();


	g_CameraPosition = position;
	XMVECTOR f = XMVector3Normalize(XMLoadFloat3(&front));
	XMVECTOR r = XMVector3Normalize(XMLoadFloat3(&right)* XMVECTOR{ 1.0f, 0.0f, 1.0f });
	//XMStoreFloat3(&g_CameraRightVec, XMVector3Normalize(XMLoadFloat3(&right) * {1.0f , 0.0f , 1.0f}));
	//XMStoreFloat3(&g_CameraUpVec, XMVector3Normalize(XMLoadFloat3(&up)));
	XMVECTOR u = XMVector3Normalize( XMVector3Cross(f, r));
	XMStoreFloat3(&g_CameraFrontVec, f);
	XMStoreFloat3(&g_CameraRightVec, r);
	XMStoreFloat3(&g_CameraUpVec, u);

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // size of buffer
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // �o�C���h�t���O

	
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1);
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2);


}



void Camera_Initialize()
{
	g_CameraPosition={ 0.0f , 0.0f , -5.0f };
	g_CameraFrontVec={ 0.0f , 0.0f ,	 1.0f };
	g_CameraUpVec={ 0.0f , 1.0f ,  0.0f };
	g_CameraRightVec={ 1.0f , 0.0f ,  0.0f };
	g_Fov = XMConvertToRadians(60);

	XMStoreFloat4x4(&g_CameraMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&g_PerspectiveMatrix, XMMatrixIdentity());







#if defined(DEBUG) || defined(_DEBUG)
	g_pDT = new hal::DebugText(Direct3D_GetDevice(), Direct3D_GetContext(),
		L"resource/texture/consolab_ascii_512.png",
		Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
		0.0f, 32.0f,
		0, 0,
		0.0f, 14.0f);

#endif
}

void Camera_Update(double elapsed_time)
{
	XMVECTOR front = XMLoadFloat3(&g_CameraFrontVec);
	XMVECTOR right = XMLoadFloat3(&g_CameraRightVec);
	XMVECTOR up = XMLoadFloat3(&g_CameraUpVec);
	XMVECTOR position = XMLoadFloat3(&g_CameraPosition);

	//向下朝向(不是vector变量无法进行旋转变换)
	if (KeyLogger_IsPressed(KK_DOWN))
	{
		XMMATRIX rotation = XMMatrixRotationAxis(right, float(CAMERA_ROTATION_SPEED * elapsed_time));
		front = XMVector3TransformNormal(front, rotation);

		//防止误差出现，重新进行正规化
		front = XMVector3Normalize(front);

		up = XMVector3Cross(front, right);
	}


	//向上朝向(不是vector变量无法进行旋转变换)
	if (KeyLogger_IsPressed(KK_UP))
	{
		XMMATRIX rotation = XMMatrixRotationAxis(right, float(-CAMERA_ROTATION_SPEED * elapsed_time));
		front = XMVector3TransformNormal(front, rotation);

		//防止误差出现，重新进行正规化
		front = XMVector3Normalize(front);

		up = XMVector3Cross(front, right);
	}


	//向右朝向(不是vector变量无法进行旋转变换)
	if (KeyLogger_IsPressed(KK_RIGHT))
	{
	//	XMMATRIX rotation = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsed_time);
		
		XMMATRIX rotation = XMMatrixRotationY(float(CAMERA_ROTATION_SPEED * elapsed_time));
		//使用Y旋转左右视角时必须把所有的轴都同时旋转
		
		up = XMVector3Normalize(XMVector3TransformNormal(up, rotation));
		
		front = XMVector3TransformNormal(front, rotation);

		//防止误差出现，重新进行正规化


		front = XMVector3Normalize(front);

		XMVector3Normalize(right = (XMVector3Cross(up, front))* XMVECTOR {1.0F , 0.0f , 1.0f });
		//XMVector3Normalize(right = XMVector3Cross(up, front));
	}

	//向左朝向(不是vector变量无法进行旋转变换)
	if (KeyLogger_IsPressed(KK_LEFT))
	{
		//XMMATRIX rotation = XMMatrixRotationAxis(up, -CAMERA_ROTATION_SPEED * elapsed_time);
		XMMATRIX rotation = XMMatrixRotationY(float(-CAMERA_ROTATION_SPEED * elapsed_time));

		up = XMVector3TransformNormal(up, rotation);

		front = XMVector3TransformNormal(front, rotation);

		//防止误差出现，重新进行正规化
		front = XMVector3Normalize(front);

		right = XMVector3Normalize(XMVector3Cross(up, front));
	}

	//move forward
	if (KeyLogger_IsPressed(KK_W))
	{
		//position += front * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVector3Normalize(front * XMVECTOR{ 0.0f , 0.0f ,1.0f }) * float(CAMERA_MOVE_SPEED * elapsed_time);
	}

	//move LEFT
	if (KeyLogger_IsPressed(KK_A))
	{
		position += -right * float(CAMERA_MOVE_SPEED * elapsed_time);

	}

	//move BACK
	if (KeyLogger_IsPressed(KK_S))
	{
		//position += -front * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVector3Normalize(-front * XMVECTOR{ 0.0f , 0.0f ,1.0f }) * float(CAMERA_MOVE_SPEED * elapsed_time);

	}

	//move RIGHT
	if (KeyLogger_IsPressed(KK_D))
	{
		position += right * float(CAMERA_MOVE_SPEED * elapsed_time);

	}

	//上升
	if (KeyLogger_IsPressed(KK_Q))
	{
		//position += up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{0.0f, 1.0f, 0.0f} *float(CAMERA_MOVE_SPEED * elapsed_time);

	}
	//下降
	if (KeyLogger_IsPressed(KK_E))
	{
		//position += -up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{ 0.0f, -1.0f, 0.0f } *float(CAMERA_MOVE_SPEED * elapsed_time);

	}




	if (KeyLogger_IsPressed(KK_Z))
	{
		g_Fov += XMConvertToRadians(1) * elapsed_time;
	}


	if (KeyLogger_IsPressed(KK_C))
	{
		g_Fov -= XMConvertToRadians(1) * elapsed_time;

	}
	//保存更新后的各种结果

	XMStoreFloat3(&g_CameraPosition, position);
	XMStoreFloat3(&g_CameraFrontVec, front);
	XMStoreFloat3(&g_CameraRightVec, right);
	XMStoreFloat3(&g_CameraUpVec, up);

	//XMVECTOR eyePos = XMLoadFloat3(&g_CameraPosition);

	//View coordinate 行列式制作
	XMMATRIX mtxView = XMMatrixLookAtLH(
		position,
		//eyePos,
		position + front, //视点
		up);  //EYEPOSITION  = CAMERA POSTIION   FOCUSPOSITION = REACH POSITION  UPDIRECTION = 摄像机当前朝向 

	XMStoreFloat4x4(&g_CameraMatrix, mtxView);
	

	//PERSPECTIVE 行列式制作
	//float constexpr fovAngleY = XMConvertToRadians(60.0f);

	float w = (float)Direct3D_GetBackBufferWidth();
	float h = (float)Direct3D_GetBackBufferHeight();
	if (w < 1.0f || h < 1.0f) return;
	float aspectRatio = w / h;
	float nearz = 0.5f;
	float farz = 100.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_Fov, aspectRatio, nearz, farz); //fovangle是弧度，这里已嵌入了角度换算 aspectRation的计算式一定要是小数类型变量  nearz 是从相机到屏幕的距离（不能为0，一定要是比0大的数（0.1）），farz是最远看到的距离（本次为100），大陆或者地面绘制时10000

	XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);
	
}

void Camera_Finalize(void)
{
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pVSConstantBuffer1);
	delete g_pDT;
}

const DirectX::XMFLOAT4X4& Camera_GetMatrix()
{

	return g_CameraMatrix;

}

const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix()
{
	return g_PerspectiveMatrix;

}

const DirectX::XMFLOAT3& Camera_GetFront()
{

	return g_CameraFrontVec;

}
const DirectX::XMFLOAT3& Camera_GetPosition()
{
	return g_CameraPosition;

}

float Camera_GetFov()
{
	return g_Fov;
}

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection)
{
	XMFLOAT4X4 v, p;
	XMStoreFloat4x4(&v, XMMatrixTranspose(view));
	XMStoreFloat4x4(&p, XMMatrixTranspose(projection));

	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &v, 0, 0);
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &p, 0, 0);

	Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
	Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
}

void Camera_DebugDraw()
{
#if defined(DEBUG) || defined(_DEBUG)
	std::stringstream ss;

	ss << " Camera Position : x = " << g_CameraPosition.x;
	ss << " y = " << g_CameraPosition.y;
	ss << " z = " << g_CameraPosition.z << std::endl;


	ss << " Camera Front    : x = " << g_CameraFrontVec.x;
	ss << " y = " << g_CameraFrontVec.y;
	ss << " z = " << g_CameraFrontVec.z <<std::endl;

	ss << " Camera Right    : x = " << g_CameraRightVec.x;
	ss << " y = " << g_CameraRightVec.y;
	ss << " z = " << g_CameraRightVec.z << std::endl;

	ss << " Camera Up       : x = " << g_CameraUpVec.x;
	ss << " y = " << g_CameraUpVec.y;
	ss << " z = " << g_CameraUpVec.z << std::endl;

	ss << " Camera Fov       :    " << g_Fov;
	
	g_pDT->SetText(ss.str().c_str(),{0.0f , 1.0f , 0.0f , 1.0f});
	g_pDT->Draw();
	g_pDT->Clear();
#endif
}
