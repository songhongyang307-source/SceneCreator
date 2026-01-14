/*==============================================================================

   �|���S���`�� [polygon.cpp]
														 Author : Youhei Sato
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
/*

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;
#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"







//static constexpr int NUM_VERTEX = 9; //顶点数


static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer
static ID3D11ShaderResourceView* g_pTexture = nullptr;//texture


// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_NumVertex = 0;
static float g_Radius = 300.0f;
static float g_Cx = 512.0f;
static float g_Cy = 512.0f;


// 顶点构造体
struct Vertex
{                                      //must in order by pos col tex cause the layout defination function is in order by float3 float 4 and float2
	XMFLOAT3 position; // vertex position
	XMFLOAT4 color;
	XMFLOAT2 texcood
		;// texture coordinate
};


void Polygon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̃`�F�b�N
	if (!pDevice || !pContext) {
		hal::dout << "Sprite_Initialize() : �^����ꂽ�f�o�C�X���R���e�L�X�g���s���ł�" << std::endl;
		return;
	}

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;

	//点の数を算出
	g_NumVertex = int(g_Radius * 2.0f * XM_PI);


	// 顶点buffer生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * g_NumVertex;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	//读取texture
	TexMetadata metadata;
	ScratchImage image;

	LoadFromWICFile(L"resource/texture/white.png", WIC_FLAGS_NONE, &metadata, image);
	HRESULT hr = CreateShaderResourceView(g_pDevice,
		image.GetImages(), image.GetImageCount(), metadata, &g_pTexture);

	if (FAILED(hr)) {

		MessageBox(nullptr, "failed to read texture in polygon.cpp", "ERROR", MB_OK | MB_ICONERROR);
	}
}

void Polygon_Finalize(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexBuffer);
}

void Polygon_Draw(void)
{
	// �V�F�[�_�[��`��p�C�v���C���ɐݒ�
	Shader_Begin();

	// ���_�o�b�t�@����b�N����
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// ���_�o�b�t�@�ւ̉��z�|�C���^��擾
	Vertex* v = (Vertex*)msr.pData;



	// ���_�����������
	const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();




	

	// ��ʂ̍��ォ��E���Ɍ�����������`�悷��
	
	const float rad = XM_2PI / g_NumVertex;
	
	for (int i = 0; i < g_NumVertex; i++)
	{

		v[i].position.x = cosf(rad * i) * g_Radius + g_Cx;
		v[i].position.y = sinf(rad * i) * g_Radius + g_Cy;
		v[i].position.z = 0.0f;
		v[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		v[i].texcood = { 0.0f, 0.0f };
	}

	/*
	v1[0].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.6f , 0.0f };
	v1[1].position = { SCREEN_WIDTH *0.45f, SCREEN_HEIGHT * 0.6f , 0.0f };
	v1[2].position = { SCREEN_WIDTH*0.1f , SCREEN_HEIGHT*0.95f , 0.0f };
	v1[3].position = { SCREEN_WIDTH * 0.45f ,SCREEN_HEIGHT * 0.95f , 0.0f };
	


	// ���_�o�b�t�@�̃��b�N����
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// ���_�o�b�t�@��`��p�C�v���C���ɐݒ�
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// ���_�V�F�[�_�[�ɕϊ��s���ݒ�
	Shader_SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	// primitiveTopology set up
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	//texture set up
	//g_pContext->PSSetShaderResources( 0, 1, &g_pTexture);

	// drawing polygon
	g_pContext->Draw(g_NumVertex, 0);
}
*/