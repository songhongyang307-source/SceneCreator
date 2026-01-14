/*==============================================================================

   スプライト [sprite.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"
#include "sprite.h"
#include "texture.h"
#include "Sprite_Anim.h"
using namespace DirectX;






static constexpr int NUM_VERTEX = 4; //顶点数



static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer
static ID3D11ShaderResourceView* g_pTexture = nullptr;//texture


// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int Tex_White = -1;
// ���_�\����
struct Vertex
{                                      //must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT4 color;
	XMFLOAT2 texcood;// texture coordinate
};


void Sprite_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̃`�F�b�N
	if (!pDevice || !pContext) {
		hal::dout << "Sprite_Initialize() : �^����ꂽ�f�o�C�X���R���e�L�X�g���s���ł�" << std::endl;
		return;
	}

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 顶点buffer生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);


	Tex_White =Texture_Load(L"resource/texture/white.png");

}
	

void Sprite_Finalize(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexBuffer);
}



void Sprite_Begin()
{
	// 行列变换到顶点shader
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();
	Shader_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

}     //





void Sprite_Draw(int texid, float dx, float dy, const XMFLOAT4& color)
{
	// �V�F�[�_�[��`��p�C�v���C���ɐݒ�
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;

	float dw = Texture_Width(texid);
	float dh = Texture_Height(texid);
	// ���_�����������
	

	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	v[0].position = { dx,      dy,       0.0f };
	v[1].position = { dx + dw ,   dy ,      0.0f };
	v[2].position = { dx  ,   dy + dh ,     0.0f };
	v[3].position = { dx + dw  , dy + dh  ,    0.0f };

	v[0].color = color ;
	v[1].color = color ;
	v[2].color = color ;
	v[3].color = color ;


	
	v[0].texcood = { 0.0f, 0.0f };
	v[1].texcood = { 1.0f, 0.0f };
	v[2].texcood = { 0.0f, 1.0f };
	v[3].texcood = { 1.0f, 1.0f };


	/*
	v1[0].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.6f , 0.0f };
	v1[1].position = { SCREEN_WIDTH *0.45f, SCREEN_HEIGHT * 0.6f , 0.0f };
	v1[2].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.95f , 0.0f };
	v1[3].position = { SCREEN_WIDTH * 0.45f ,SCREEN_HEIGHT * 0.95f , 0.0f };
	*/


	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 顶点shader管线设置
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	

	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, const XMFLOAT4& color)
{
	// �V�F�[�_�[��`��p�C�v���C���ɐݒ�
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;


	// ���_�����������

/*
	constexpr float x = 32.0f;
	constexpr float y = 32.0f;
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;
	*/
	





	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	v[0].position = { dx,      dy,       0.0f };
	v[1].position = { dx + dw ,   dy ,      0.0f };
	v[2].position = { dx  ,   dy + dh ,     0.0f };
	v[3].position = { dx + dw  , dy + dh  ,    0.0f };

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	

	v[0].texcood = { 0.0f, 0.0f };
	v[1].texcood = { 1.0f, 0.0f };
	v[2].texcood = { 0.0f, 1.0f };
	v[3].texcood = { 1.0f, 1.0f };


	/*
	v1[0].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.6f , 0.0f };
	v1[1].position = { SCREEN_WIDTH *0.45f, SCREEN_HEIGHT * 0.6f , 0.0f };
	v1[2].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.95f , 0.0f };
	v1[3].position = { SCREEN_WIDTH * 0.45f ,SCREEN_HEIGHT * 0.95f , 0.0f };
	*/


	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 顶点shader管线设置
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);



	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}


void Sprite_Draw_R(int texid, float dx, float dy, float dw, float dh,float angle,float pivotx,float pivoty, const XMFLOAT4& color)
{
	// �V�F�[�_�[��`��p�C�v���C���ɐݒ�
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;


	// ���_�����������

/*
	constexpr float x = 32.0f;
	constexpr float y = 32.0f;
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;
	*/






	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	v[0].position = { -0.5f + pivotx,      -0.5f + pivoty,     0.0f };
	v[1].position = { 0.5f + pivotx ,      -0.5f + pivoty ,    0.0f };
	v[2].position = { -0.5f + pivotx  ,    0.5f + pivoty ,     0.0f };
	v[3].position = { 0.5f + pivotx , 0.5f + pivoty ,          0.0f  };

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;


	v[0].texcood = { 0.0f, 0.0f };
	v[1].texcood = { 1.0f, 0.0f };
	v[2].texcood = { 0.0f, 1.0f };
	v[3].texcood = { 1.0f, 1.0f };



	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	//回転行列设置
	XMMATRIX scale = XMMatrixScaling(dw, dh, 1.0f);
	XMMATRIX translation = XMMatrixTranslation(dx + dw * 0.5f, dy + dh * 0.5f, 0.0f);

	XMMATRIX rotation = XMMatrixRotationZ(angle);//XMMATRIX 计算用矩阵

	Shader_SetWorldMatrix(scale * rotation * translation);

	// angle 是弧度

	// 顶点shader管线设置
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);



	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}


void Sprite_Draw(int texid, float dx, float dy, float px, float py, int pw, int ph, const XMFLOAT4& color)
{
	// �V�F�[�_�[��`��p�C�v���C���ɐݒ�
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;


	// ���_�����������

/*
	constexpr float x = 32.0f;
	constexpr float y = 32.0f;
	constexpr float w = 512.0f;
	constexpr float h = 512.0f;
	*/






	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	v[0].position = { dx,      dy,       0.0f };
	v[1].position = { dx + pw ,   dy ,      0.0f };
	v[2].position = { dx  ,   dy + ph ,     0.0f };
	v[3].position = { dx + pw  , dy + ph  ,    0.0f };

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);
	
	float u0 =  px        / tw;
	float v0 =  py  / th;
	float u1 = (px + pw)  / tw;
	float v1 = (py + ph)  / th;

	v[0].texcood = { u0, v0 };
	v[1].texcood = { u1, v0 };
	v[2].texcood = { u0, v1 };
	v[3].texcood = { u1, v1 };


	/*
	v1[0].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.6f , 0.0f };
	v1[1].position = { SCREEN_WIDTH *0.45f, SCREEN_HEIGHT * 0.6f , 0.0f };
	v1[2].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.95f , 0.0f };
	v1[3].position = { SCREEN_WIDTH * 0.45f ,SCREEN_HEIGHT * 0.95f , 0.0f };
	*/


	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 顶点shader管线设置
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);



	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, float px, float py, int pw, int ph, const XMFLOAT4& color)
{
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;


	// ���_�����������
	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	v[0].position = { dx,      dy,       0.0f };
	v[1].position = { dx + dw ,   dy ,      0.0f };
	v[2].position = { dx  ,   dy + dh ,     0.0f };
	v[3].position = { dx + dw  , dy + dh  ,    0.0f };

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);

	float u0 = px / tw;
	float v0 = py / th;
	float u1 = (px + pw) / tw;
	float v1 = (py + ph) / th;

	v[0].texcood = { u0, v0 };
	v[1].texcood = { u1, v0 };
	v[2].texcood = { u0, v1 };
	v[3].texcood = { u1, v1 };

	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	//world transfer matrix set up
	Shader_SetWorldMatrix(XMMatrixIdentity());


	// 顶点shader管线设置
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);



	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texid, float dx, float dy, float dw, float dh, float px, float py,int pw, int ph, float angle, const XMFLOAT4& color)
{
	Shader_Begin();

	//set shader to drawing pipeline
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 顶点的缓冲区的虚拟顶点获得
	Vertex* v = (Vertex*)msr.pData;


	// ���_�����������
	//a += 0.04;
	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��  // 首先将图片变为单位长度为1，中心坐标为（0.0）（移动到原点）

												 //之后与图片的尺寸相乘（矩阵乘法）获得扩大后的图片
												//之后进行旋转运算
												//所有的操作结束之后，移动到原来的位置
	v[0].position = { -0.5f,   -0.5f,       0.0f };
	v[1].position = { +0.5f ,   -0.5f ,      0.0f };
	v[2].position = { -0.5f ,   +0.5f ,     0.0f };
	v[3].position = { +0.5f , +0.5f  ,    0.0f };

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	float tw = (float)Texture_Width(texid);
	float th = (float)Texture_Height(texid);

	float u0 = px / tw;
	float v0 = py / th;
	float u1 = (px + pw) / tw;
	float v1 = (py + ph) / th;

	v[0].texcood = { u0, v0 };
	v[1].texcood = { u1, v0 };
	v[2].texcood = { u0, v1 };
	v[3].texcood = { u1, v1 };

	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);


	//回転行列设置
	XMMATRIX scale = XMMatrixScaling(dw, dh, 1.0f);
	XMMATRIX translation = XMMatrixTranslation(dx+dw*0.5f,dy+dh*0.5f,0.0f);

	XMMATRIX rotation = XMMatrixRotationZ(angle);//XMMATRIX 计算用矩阵

	Shader_SetWorldMatrix(scale * rotation * translation);
	
	// angle 是弧度
	// 顶点shader管线设置



	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);



	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//texture set up
	Texture_SetTexture(texid);

	// drawing 
	g_pContext->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(float dx, float dy, float dw, float dh, const DirectX::XMFLOAT4& color)
{
	// 1) 使用 sprite 用的着色器
	Shader_Begin();

	// 2) 映射顶点缓冲
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	Vertex* v = (Vertex*)msr.pData;

	// ★★ 关键：这里直接用屏幕像素坐标，不要再转 [-1,1]
	v[0].position = { dx,        dy,        0.0f }; // 左上
	v[1].position = { dx + dw,   dy,        0.0f }; // 右上
	v[2].position = { dx,        dy + dh,   0.0f }; // 左下
	v[3].position = { dx + dw,   dy + dh,   0.0f }; // 右下

	v[0].color = color;
	v[1].color = color;
	v[2].color = color;
	v[3].color = color;

	// UV 可以随便先设成 0~1，将来如果 PS 不用纹理无所谓
	v[0].texcood = { 0.0f, 0.0f };
	v[1].texcood = { 1.0f, 0.0f };
	v[2].texcood = { 0.0f, 1.0f };
	v[3].texcood = { 1.0f, 1.0f };

	g_pContext->Unmap(g_pVertexBuffer, 0);

	Shader_SetWorldMatrix(XMMatrixIdentity());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ★ 如果你现在用的 Pixel Shader 一定要采样纹理，那这里最好绑一张 1×1 白色纹理：
	 Texture_SetTexture(Tex_White);

	g_pContext->Draw(NUM_VERTEX, 0);
}



float GetScreenHeight(int texid)
{
	texid = texid;
	return (float)Direct3D_GetBackBufferHeight();
	
}

float GetScreenWidth(int texid)
{
	texid = texid;
	return (float)Direct3D_GetBackBufferWidth();
}


