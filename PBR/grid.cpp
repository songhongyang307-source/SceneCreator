/*==============================================================================

   XZ平面显示[Grid.cpp]
														 Author : Youhei Sato
														 Date   : 2025/09/09
--------------------------------------------------------------------------------

==============================================================================*/




#include "grid.h"
#include "direct3d.h"
#include <DirectXMath.h>
#include "shader3d .h"
using namespace DirectX;

static constexpr int GRID_H_COUNT = 10;
static constexpr int GRID_V_COUNT = 10;
static constexpr int GRID_H_LINE_COUNT = GRID_H_COUNT + 1;
static constexpr int GRID_V_LINE_COUNT = GRID_H_COUNT + 1;
static constexpr int NUM_VERTEX = GRID_H_LINE_COUNT *2 + GRID_V_LINE_COUNT * 2;

static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer


// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;

static ID3D11DeviceContext* g_pContext = nullptr;

struct Vertex3D
{
	//must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT4 color;

};

static Vertex3D g_GridVertex[NUM_VERTEX]{};



void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;

	//点の数を算出



	// 顶点buffer生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;//DYNAMIC每帧都描画，3d描画时不需要每帧都画
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	//高效化，创建buffer之后不再每帧进行读写
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_GridVertex;

	float x = -5.0f;
	for (int i = 0; i < GRID_H_LINE_COUNT * 2; i += 2)
	{
		g_GridVertex[i] = { {x, 0.0f , 5.0f} , {0.0f , 1.0f ,0.0f , 1.0f} };
		g_GridVertex[i+1] = { {x, 0.0f , -5.0f} , {0.0f , 1.0f ,0.0f , 1.0f} };
		x += 1.0f;
	}

	float z = -5.0f;
	for (int i = GRID_V_LINE_COUNT * 2 ; i < NUM_VERTEX; i += 2)
	{
		g_GridVertex[i] = { {5.0f, 0.0f , z} , {0.0f , 1.0f ,0.0f , 1.0f} };
		g_GridVertex[i + 1] = { {-5.0f, 0.0f , z} , {0.0f , 1.0f ,0.0f , 1.0f} };
		z += 1.0f;
	}


	
	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

void Grid_Draw(void)
{
	//shader绘制管线设置
	Shader3d_Begin();

	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//world matrix
	XMMATRIX mtxWorld = XMMatrixIdentity();//单位行列式制作

	//将世界坐标行列变换到顶点shader
	Shader3d_SetWorldMatrix(mtxWorld);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// drawing polygon
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Grid_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
}
