/*==============================================================================

   ライトの設定[light.h]
														 Author : Youhei Sato
														 Date   : 2025/09/30
--------------------------------------------------------------------------------

==============================================================================*/



#include "light.h"
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include "texture.h"
// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11Buffer* g_pPSConstantBuffer1 = nullptr;//b1
static ID3D11Buffer* g_pPSConstantBuffer2 = nullptr;//b2
													 
static ID3D11Buffer* g_pPSConstantBuffer3 = nullptr;//b2
static ID3D11Buffer* g_pPSConstantBuffer4 = nullptr;//b2

struct DirectionalLight
{
	XMFLOAT4 Directional;
	XMFLOAT4 Color;
	 
};

struct CameraPBRParams
{
	XMFLOAT3 CameraPosition;
	float    Metallic;

	float    Roughness;
	float    AO;
	XMFLOAT2 Dummy;
};

//point light
struct PointLight
{
	XMFLOAT3 Position;
	float Range;
	XMFLOAT4 Color;
	//float SpecularPower; //NOT DONE YET
	//XMFLOAT4 SpecularColor;//NOT DONE YET
};

struct PointLightList
{
	PointLight light[4];
	int count;
	XMFLOAT3 dummy;
};

static PointLightList g_PointLights ={};



void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 顶点buffer 制作
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	 
	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // size of buffer
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1);


	buffer_desc.ByteWidth = sizeof(DirectionalLight);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2);

	buffer_desc.ByteWidth = sizeof(CameraPBRParams);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3);

	buffer_desc.ByteWidth = sizeof(PointLightList);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4);
	/*
	PointLightList list{
		{
			{ {2.0f , 2.0f , 0.0f} , 5.0f , {1.0f , 0.7f ,0.3f ,1.0f} },
			{ {-2.0f , 2.0f , 0.0f} , 5.0f , {0.1f , 0.3f , 0.8f ,1.0f} },
			{ {0.0f , 2.0f , 0.0f} , 5.0f , {1.0f , 1.0f , 1.0f ,1.0f} },
			{ {0.0f , 2.0f , 0.0f} , 5.0f , {1.0f , 1.0f , 1.0f ,1.0f} },
		},
		1
		//dummy

	};
	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &list, 0, 0);

	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);//绘制命令*/


}

void Light_SetAmbient(const DirectX::XMFLOAT3& color)
{
	// 定制buffer设置ambient
	XMFLOAT4 c{ color.x , color.y , color.z , 1.0f };
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &c, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);

}

void Light_Finalize(void)
{
	SAFE_RELEASE(g_pPSConstantBuffer4);
	SAFE_RELEASE(g_pPSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer1);
}

static void DBG(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);

	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
}

static void DumpPSBind0_4()
{
	ID3D11ShaderResourceView* srvs[5]{};
	g_pContext->PSGetShaderResources(0, 5, srvs); // AddRef
	//DBG("PS SRV t0=%p t1=%p t2=%p t3=%p t4=%p",
		//srvs[0], srvs[1], srvs[2], srvs[3], srvs[4]);
	for (auto* s : srvs) if (s) s->Release();
}

void Light_Set_PBRParams(const DirectX::XMFLOAT3& camera_position,float metallic,float roughness,float ao)
{
	CameraPBRParams params{

			camera_position,
			metallic,
			roughness,
			ao,
		{0.0f , 0.0f}

	};

	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &params, 0, 0);//值送给buffer
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3);//绘制命令

	DumpPSBind0_4();
}


void Light_Set_PointLight_Count(int count)
{
	g_PointLights.count = count;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);

	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);//绘制命令
}



void Light_Set_PointLight(int n, const DirectX::XMFLOAT3& position, float range, const DirectX::XMFLOAT3& color)
{
	
	g_PointLights.light[n].Position = position;
	g_PointLights.light[n].Range = range;
	g_PointLights.light[n].Color = { color.x , color.y , color.z ,1.0f };
		

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);

	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);//绘制命令
}



void Light_SetDirectional(const DirectX::XMFLOAT4& world_directional,
	const DirectX::XMFLOAT4& color)
{
	DirectionalLight light{
		world_directional,
		color,
	};
	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &light, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);

}
