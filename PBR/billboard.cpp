/*==============================================================================

   Billboard Drawing[billboard.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/


#include "billboard.h"
#include <DirectXMath.h>
#include "direct3d.h"
#include "shader_billboard.h"
using namespace DirectX;
#include "texture.h"
#include "playercamera.h"


static constexpr int NUM_VERTEX = 4;

static ID3D11Buffer* g_pVertexBuffer = nullptr;

//static XMFLOAT4X4 g_InverseViewMatrix{}; // inverse view matrix 转置矩阵
static XMFLOAT4X4 g_mtxView{};

// 3D 顶点构造体
struct Vertex3d
{
	//must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
};


void BillBoard_Initialize()
{
	ShaderBillboard_Initialize();

	static Vertex3d vertex[]
	{

		//front side // 0 , 1 , 2 , 0 , 3 ,1 ,
		{{-0.5f , 0.5f , 0.0f } , { 1.0f, 1.0f, 1.0f , 1.0f } ,  {0.0f , 0.0f}},
		{{ 0.5 ,  0.5f ,0.0f } , { 1.0f, 1.0f, 1.0f , 1.0f } ,   {1.0f , 0.0f}},
		{{ -0.5f ,-0.5f ,0.0f } , { 1.0f, 1.0f, 1.0f , 1.0f } ,  {0.0f , 1.0f}},
		{{ 0.5f , -0.5f , 0.0f },  { 1.0f, 1.0f, 1.0f , 1.0f } , {1.0f , 1.0f}},
	};



	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;//DYNAMIC每帧都描画，3d描画时不需要每帧都画
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;// access to cpu prohibit

	//高效化，创建buffer之后不再每帧进行读写
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertex;



	Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);


}




void BillBoard_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);

	ShaderBillboard_Finalize();
}


void BillBoard_Draw(int Texid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2 scale, const DirectX::XMFLOAT2& pivot)
{
	ShaderBillboard_SetUVParameter({ {1.0f , 1.0f}, {0.0f , 0.0f} });


	ShaderBillboard_Begin();

	//pixel shader color setup

	ShaderBillboard_SetColor({ 1.0f , 1.0f ,1.0f ,1.0f });


	//texture set up
	Texture_SetTexture(Texid);


	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	//頂点シェーダー
	// 回転軸のオフセット行列
	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);


	//カメラ行列の回転だけ逆行列を作る
	//XMFLOAT4X4 mtxCamera = PlayerCamera_GetViewMatrix();
	//mtxCamera._41 = mtxCamera._42 = mtxCamera._43 = 0.0f; //移転部分なしにする
	//XMMATRIX iv = XMMatrixInverse(nullptr , XMLoadFloat4x4(&mtxCamera));//非常耗费资源

	//直交行列の逆行列は転置行列と等しい
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView));


	XMMATRIX s = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX t = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);
	ShaderBillboard_SetWorldMatrix(pivot_offset * s* iv *t);
	
	
	// drawing polygon
	Direct3D_GetContext()->Draw(NUM_VERTEX,0);
	

	
}

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view)
{


	//カメラ行列の平行移動成分カット
	g_mtxView = view;
	g_mtxView._41 = g_mtxView._42 = g_mtxView._43 = 0.0f;

}


void BillBoard_Draw(int Texid, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2 scale, const DirectX::XMUINT4& tex_cut, const DirectX::XMFLOAT2& pivot)
{
	ShaderBillboard_Begin();


	float uv_x = (float)tex_cut.x / Texture_Width(Texid);
	float uv_y = (float)tex_cut.y / Texture_Height(Texid);
	float uv_w = (float)tex_cut.z / Texture_Width(Texid);
	float uv_h = (float)tex_cut.w / Texture_Height(Texid);

	ShaderBillboard_SetUVParameter({ {uv_w , uv_h}, { uv_x , uv_y} });
	

	//pixel shader color setup

	ShaderBillboard_SetColor({ 1.0f , 1.0f ,1.0f ,1.0f });


	//texture set up
	Texture_SetTexture(Texid);


	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);




	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	//頂点シェーダー
	

	//カメラ行列の回転だけ逆行列を作る
	//XMFLOAT4X4 mtxCamera = PlayerCamera_GetViewMatrix();
	//mtxCamera._41 = mtxCamera._42 = mtxCamera._43 = 0.0f; //移転部分なしにする
	//XMMATRIX iv = XMMatrixInverse(nullptr , XMLoadFloat4x4(&mtxCamera));//非常耗费资源

	//直交行列の逆行列は転置行列と等しい
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView));

	// 回転軸のオフセット行列
	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);


	XMMATRIX s = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX t = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);
	ShaderBillboard_SetWorldMatrix(pivot_offset * s * iv * t);


	// drawing polygon
	Direct3D_GetContext()->Draw(NUM_VERTEX, 0);

}