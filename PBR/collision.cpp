/*==============================================================================

   Collision judgement [collision.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/


#include "collision.h"
#include <d3d11.h>
#include "direct3d.h"
#include "texture.h"
#include "shader.h"
using namespace DirectX;
#include <algorithm>

static constexpr int NUM_VERTEX = 5000; //顶点数


static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer


// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_WhiteTexId = -1;


// ���_�\����
struct Vertex
{                                      //must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT4 color;
	XMFLOAT2 texcood
		;// texture coordinate
};

bool Collision_OverlapCircleVsCircle(const Circle& a, const Circle& b)
{
	/*
	float x1 = b.center.x - a.center.x;
	float y1 = b.center.y - a.center.y;

	return (x1 * x1 + y1 * y1) < (a.radius + b.radius) * (a.radius + b.radius);
	*/

	XMVECTOR ac = XMLoadFloat2(&a.center);
	XMVECTOR bc = XMLoadFloat2(&b.center);
	XMVECTOR lsq = XMVector2LengthSq(bc - ac);

	return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
	}

bool Collision_OverlapBoxVsBox(const Box& a, const Box& b)
{
	float at = a.center.y - a.half_height;
	float ab = a.center.y + a.half_height;
	float al = a.center.x - a.half_width;
	float ar = a.center.x + a.half_width;
	float bt = b.center.y - b.half_height;
	float bb = b.center.y + b.half_height;
	float bl = b.center.x - b.half_width;
	float br = b.center.x + b.half_width;

		
	

    return al < br && ar> bl &&
			at < bb && ab>bt;
}

bool Collisioin_IsOverlappAABB(const AABB& a, const AABB& b)
{
	return a.min.x < b.max.x
		&& a.max.x > b.min.x
		&& a.min.y < b.max.y
		&& a.max.y > b.min.y
		&& a.min.z < b.max.z
		&& a.max.z > b.min.z;
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit;

	//重叠了吗
	hit.isHit = Collisioin_IsOverlappAABB(a, b);

	if (!hit.isHit)
	{
		//no overlap
		return hit;
	}

	//every depth of axis
	float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);



	//一番深度が浅い軸


	float minDepth = xdepth;
	int axis = 0; // 0=x, 1=y, 2=z

	if (ydepth < minDepth) {
		minDepth = ydepth;
		axis = 1;
	}
	if (zdepth < minDepth) {
		minDepth = zdepth;
		axis = 2;
	}



	//+or - ?确定了碰撞的面后，进一步确定是该方向面的正方向还是负方向
	
	XMFLOAT3 center_a = a.GetCenter();
	XMFLOAT3 center_b = b.GetCenter();
	XMFLOAT3 normal = { 0.0f, 0.0f, 0.0f };
	//XMVECTOR normal = XMLoadFloat3(&center_b) - XMLoadFloat3(&center_a);
	
	switch (axis)
	{
	case 0: // x
		// 如果B在A的右边，分离方向应该向右(+x)，否则向左(-x)
		normal.x = (center_b.x >= center_a.x) ? 1.0f : -1.0f;
		break;
	case 1: // y
		normal.y = (center_b.y >= center_a.y) ? 1.0f : -1.0f;
		break;
	case 2: // z
		normal.z = (center_b.z >= center_a.z) ? 1.0f : -1.0f;
		break;
	}

	hit.normal = normal;

	//XMStoreFloat3(&hit.normal, normal);
	hit.depth = minDepth;

	return hit;
}

void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	// デバイスとデバイスコンテキスト　の保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	


	// 顶点buffer生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexId = Texture_Load(L"resource/texture/white.png");
}


void Collision_DebugDraw(const Circle& circle,const DirectX::XMFLOAT4& color)
{
	//点の数を算出
	int numVertex = (int)(circle.radius * 2.0f * XM_PI + 1);


	// 
	Shader_Begin();

	// 
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	
	Vertex* v = (Vertex*)msr.pData;

	

	const float rad = XM_2PI / numVertex;

	for (int i = 0; i < numVertex; i++)
	{

		v[i].position.x = cosf(rad * i) * circle.radius + circle.center.x;
		v[i].position.y = sinf(rad * i) * circle.radius + circle.center.y;
		v[i].position.z = 0.0f;
		v[i].color = color;
		v[i].texcood = { 0.0f, 0.0f };
	}
	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// ���_�o�b�t�@��`��p�C�v���C���ɐݒ�
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	Texture_SetTexture(g_WhiteTexId);

	// drawing polygon
	g_pContext->Draw(numVertex, 0);

}

void Collision_DebugDraw(const Box& box , const DirectX::XMFLOAT4& color)
{
	Shader_Begin();

	
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);


	Vertex* v = (Vertex*)msr.pData;


	v[0].position = {box.center.x - box.half_width, box.center.y - box.half_height , 0.0f};
	v[1].position = { box.center.x + box.half_width, box.center.y - box.half_height , 0.0f };
	v[2].position = { box.center.x + box.half_width, box.center.y + box.half_height , 0.0f };
	v[3].position = { box.center.x - box.half_width, box.center.y + box.half_height , 0.0f };
	v[4].position = { box.center.x - box.half_width, box.center.y - box.half_height , 0.0f };

	
	
	for (int i = 0; i < 5; i++)
	{
		v[i].color = color;
		v[i].texcood = { 0.0f, 0.0f };
	}
	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// ���_�o�b�t�@��`��p�C�v���C���ɐݒ�
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// primitiveTopology set up  
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	
	Texture_SetTexture(g_WhiteTexId);

	// drawing polygon
	g_pContext->Draw(5, 0);
}

void Collision_DebugFinalize()
{
	SAFE_RELEASE(g_pVertexBuffer);
}
