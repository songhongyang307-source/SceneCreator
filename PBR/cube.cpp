/*==============================================================================

   3d CUBE visualize[cube.cpp]
														 Author : Youhei Sato
														 Date   : 2025/09/09
--------------------------------------------------------------------------------

==============================================================================*/


#include "cube.h"
#include "direct3d.h"
#include "shader3d .h"
#include <DirectXMath.h>
using namespace DirectX;
#include "texture.h"


static constexpr int NUM_VERTEX = 24;
static constexpr int NUM_INDEX = 36; // 3* 2 * 6
static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer
static ID3D11Buffer* g_pIndexBuffer = nullptr; //Index buffer
static int g_CubeTexId = -1;
static int g_CubeNomId = -1;
// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;

static ID3D11DeviceContext* g_pContext = nullptr;



// 3D 顶点构造体
struct Vertex3D
{   
	//must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT3 normal; //法线
	XMFLOAT3 tangent;
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
};

static Vertex3D g_CubeVertex[36]
{

	//index
	//   
	//
	//front side // 0 , 1 , 2 , 0 , 3 ,1 ,
	{{-0.5f , 0.5f , -0.5f } ,    {0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } ,  {0.0f , 0.0f}},
	{{ 0.5 ,  -0.5f , -0.5f } ,   {0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } , {0.1f , 0.1f}},
	{{ -0.5f ,-0.5f , -0.5f } ,   {0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } , {0.0f , 0.1f}},
	//{{  -0.5f , 0.5f , -0.5f } ,{0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } ,{0.0f , 0.0f}},
	{{ 0.5f , 0.5f , -0.5f }   ,  {0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } ,  {0.1f , 0.0f}},
	//{{ 0.5f ,  -0.5f , -0.5f } ,{0.0f , 0.0f , -1.0f	},{ 1.0f , 0.0f , 0.0f},{ 1.0f, 1.0f, 1.0f , 1.0f } ,{0.1f , 0.1f}},

	// Back(绿色) //	4 , 5 , 6 , 4 , 6 , 7,
	  {{ -0.5f, 0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.5f , 0.0f}},
	  {{-0.5f, -0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.5f , 0.1f}},
	  {{ 0.5f, -0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.4f , 0.1f}},
	//{{-0.5f,  0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.5f , 0.0f}},
	//{{ 0.5f, -0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.4f , 0.1f}},
	  {{ 0.5f,  0.5f,  0.5f},{0.0f , 0.0f ,   1.0f	},  {-1.0f , 0.0f , 0.0f},   {1,1,1,1} , {0.4f , 0.0f}},

	// Left (蓝色)
	  {{-0.5f,  0.5f,  0.5f}, {-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} , {0.2f , 0.0f}},
	  {{-0.5f, -0.5f, -0.5f}, {-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} , {0.3f , 0.1f}},
	  {{-0.5f, -0.5f,  0.5f}, {-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} , {0.2f , 0.1f}},
	//{{-0.5f,  0.5f,  0.5f}, {-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} ,  {0.2f , 0.0f}},          
	  {{-0.5f,  0.5f, -0.5f}, {-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} , {0.3f , 0.0f}},
	//{{-0.5f,  -0.5f, -0.5f},{-1.0f , 0.0f , 0.0f	}, {0.0f , 0.0f , -1.0f},    {1,1,1,1} , {0.3f , 0.1f}},

	
	// Right (黄色)
	  {{ 0.5f,  0.5f,  -0.5f},{1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.1f , 0.1f}},
	  {{ 0.5f, -0.5f,  0.5f}, {1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.2f , 0.2f}},
	  {{ 0.5f, -0.5f,- 0.5f}, {1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.1f , 0.2f}},
	//{{ 0.5f,  0.5f, - 0.5f},{1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.1f , 0.1f}},
	  {{ 0.5f, 0.5f, 0.5f},   {1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.2f , 0.1f}},
	//{{ 0.5f, - 0.5f, 0.5f}, {1.0f , 0.0f , 0.0f	},  {0.0f , 0.0f , 1.0f},   {1,1,1,1} ,  {0.2f , 0.2f}},

	// Top (紫色) —— 法线 +Y
	  { {-0.5f,  0.5f, -0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.5f , 0.1f}},
	  { {-0.5f,  0.5f,  0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.5f , 0.0f}},
	  { { 0.5f,  0.5f, -0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.6f , 0.1f}},
	//{ {-0.5f,  0.5f,  0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.5f , 0.0f}},
	  { { 0.5f,  0.5f,  0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.6f , 0.0f}},
	//{ { 0.5f,  0.5f, -0.5f},{0.0f , 1.0f , 0.0f	},  {1.0f , 0.0f , 0.0f},   {1,1,1,1} ,  {0.6f , 0.1f}},

	// Bottom (青色) —— 法线 -Y   //	20 , 21 , 22 , 22 , 21 , 23,
	  { {-0.5f, -0.5f, -0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.6f , 0.1f}},
	  { { 0.5f, -0.5f, -0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.7f , 0.1f}},
	  { {-0.5f, -0.5f,  0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.6f , 0.0f}},
	//{ {-0.5f, -0.5f,  0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.6f , 0.0f}},
	//{ { 0.5f, -0.5f, -0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.7f , 0.1f}},
	  { { 0.5f, -0.5f,  0.5f}, {0.0f , -1.0f , 0.0f	},   {1.0f , 0.0f , 0.0f},  {1,1,1,1} ,  {0.7f , 0.0f}},
};

static unsigned short g_CubeIndex[]{ //if not enough change short to int type
			0 , 1 , 2 , 0 , 3 ,1 ,
			4 , 5 , 6 , 4 , 6 , 7,
			8 , 9 , 10 , 8 , 11 , 9 ,
			12 , 13 , 14 , 12 , 15 , 13 ,
			16 , 17 , 18 , 17 , 19 , 18,
			20 , 21 , 22 , 22, 21 , 23,
};

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
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
	sd.pSysMem = g_CubeVertex;



	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);


	//create index buffer
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX; // sizeof(g_CubeVertex)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	//高效化，创建buffer之后不再每帧进行读写
	sd.pSysMem = g_CubeIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);


	g_CubeTexId = Texture_Load(L"resource/texture/222.png");
	g_CubeNomId = Texture_Load(L"resource/texture/222.png");
}

void Cube_Draw(int Texid , const DirectX::XMMATRIX& mtxworld)
{


	//shader绘制管线设置
	Shader3d_Begin();


	//pixel shader color setup

	Shader3d_SetColor({ 1.0f , 1.0f ,1.0f ,1.0f });


	//texture set up
	Texture_SetTexture(Texid);





	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	
	//world matrix
	//XMMATRIX mtxTrans = XMMatrixTranslation(0.5f, 0.5f, 0.0f);//单位行列式制作

	//XMMATRIX mtxRot = XMMatrixRotationY(g_angle);
	//XMMATRIX mtxScale =	XMMatrixScaling(3.0f, 1.0f, 1.0f);

	//XMMATRIX mtxWorld =  mtxTrans * mtxScale * mtxRot;   //XMMatrixScaling(1.0f,3.0f,1.0f);
	
	
	////XMMATRIX mtxWorld = XMMatrixScaling(1.0f, g_scale, 1.0f);//XMMatrixRotationY(g_angle);
	//将世界坐标行列变换到顶点shader

	/*
	for (int i = 0; i < 10; i++)
	{
		
			
				XMMATRIX mtxTrans = XMMatrixTranslation( 0.0f, 10.0f -  float(i), 0.0f);
				XMMATRIX mtxScale = XMMatrixScaling(1.0f * float(i), 1.0f , 1.0f * float(i));
				XMMATRIX mtxRot = XMMatrixRotationY(g_angle);

				XMMATRIX mtxWorld = mtxTrans * mtxScale * mtxRot;
				Shader3d_SetWorldMatrix(mtxWorld);
				g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				// drawing polygon
				g_pContext->Draw(NUM_VERTEX, 0);



		

		
	}
	*/

	
	
	Shader3d_SetWorldMatrix(mtxworld);

	//index buffer set up to darwing buffer
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);//IF NOT enough ,change R16 to R32

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	

	// drawing polygon
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
	// primitiveTopology set up
	//g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	//WORLD
	//VIEW
	
	
	




}

void Cube_Draw(int diffuse_tex_id, int metallic_tex_id, int roughness_tex_id, int normal_tex_id, const DirectX::XMMATRIX& mtxworld)
{
	//shader绘制管线设置
	Shader3d_Begin();


	//pixel shader color setup

	Shader3d_SetColor({ 1.0f , 1.0f ,1.0f ,1.0f });


	//texture set up
	Texture_SetTexture(diffuse_tex_id);
	Texture_SetTexture(metallic_tex_id,1);
	Texture_SetTexture(roughness_tex_id,2);
	Texture_SetTexture(normal_tex_id,3);






	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


	Shader3d_SetWorldMatrix(mtxworld);

	//index buffer set up to darwing buffer
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);//IF NOT enough ,change R16 to R32

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// drawing polygon
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

void Cube_Finalize(void)
{
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

AABB Cube_GetAABB(const DirectX::XMFLOAT3 position)
{
	return { 
			{position.x - 0.5f,
			 position.y - 0.5f,
			 position.z - 0.5f
			 } ,		

			{position.x + 0.5f,
			 position.y + 0.5f,
			 position.z + 0.5f} 

			};				  
}

