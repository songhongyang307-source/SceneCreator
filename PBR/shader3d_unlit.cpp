
/*==============================================================================

   3D Shader (Unlighting)[shader3d_unlit.cpp]
														 Author : Youhei Sato
														 Date   : 2025/07/03
--------------------------------------------------------------------------------

==============================================================================*/




#include "shader3d_unlit.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "sampler.h"


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;//VERTEX BUFFER0 world
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;



bool Shader3dUnlit_Initialize()
{
	HRESULT hr; // 戻り値格納用

	// ���O�R���p�C���ςݒ��_�V�F�[�_�[�̓ǂݍ���
	std::ifstream ifs_vs("shader_vertex_3d_unlit.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "failed to read \n\nshader_vertex_3d_unlit.cso", "error", MB_OK);
		return false;
	}

	// �t�@�C���T�C�Y��擾
	ifs_vs.seekg(0, std::ios::end); // �t�@�C���|�C���^�𖖔��Ɉړ�
	std::streamsize filesize = ifs_vs.tellg(); // �t�@�C���|�C���^�̈ʒu��擾�i�܂�t�@�C���T�C�Y�j
	ifs_vs.seekg(0, std::ios::beg); // �t�@�C���|�C���^��擪�ɖ߂�

	// �o�C�i���f�[�^��i�[���邽�߂̃o�b�t�@��m��
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // �o�C�i���f�[�^��ǂݍ���
	ifs_vs.close(); // �t�@�C�������

	// ���_�V�F�[�_�[�̍쐬
	hr = Direct3D_GetDevice()->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader3dUnlit_Initialize : 頂点シェーダー作成失敗" << std::endl;
		delete[] vsbinary_pointer; // ���������[�N���Ȃ��悤�Ƀo�C�i���f�[�^�̃o�b�t�@����
		return false;
	}


	// vertex layout   
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		 { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		 { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		 { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		 { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	};

	UINT num_elements = ARRAYSIZE(layout); // �z��̗v�f����擾

	// ���_���C�A�E�g�̍쐬
	hr = Direct3D_GetDevice()->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // �o�C�i���f�[�^�̃o�b�t�@����

	if (FAILED(hr)) {
		hal::dout << "Shader3dUnlit_Initialize : 頂点シェーダー作成失敗" << std::endl;
		return false;
	}




	// 顶点buffer 制作
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // size of buffer
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // �o�C���h�t���O

	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);
	

	// ���O�R���p�C���ς݃s�N�Z���V�F�[�_�[�̓ǂݍ���
	std::ifstream ifs_ps("shader_pixel_3d_unlit.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "failed to read \n\nshader_pixel_3d_unlit.cso", "error", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// �s�N�Z���V�F�[�_�[�̍쐬
	hr = Direct3D_GetDevice()->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // �o�C�i���f�[�^�̃o�b�t�@����

	if (FAILED(hr)) {
		hal::dout << "Shader3dUnlit_Initialize : Pixelシェーダー作成失敗" << std::endl;
		return false;
	}

	//pixel shader  constant value buffer make

	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // size of buffer

	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);


	return true;
}

void Shader3dUnlit_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pPSConstantBuffer0);

	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);

}

void Shader3dUnlit_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定值buffer存储用 数列构造体的定义
	XMFLOAT4X4 transpose;

	// �s���]�u���Ē萔�o�b�t�@�i�[�p�s��ɕϊ�
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定制buffer设置为矩阵
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);

}

void Shader3dUnlit_SetColor(const DirectX::XMFLOAT4& color)
{
	Direct3D_GetContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);

}

void Shader3dUnlit_Begin()
{
	// vertex shader and pixel shader
	Direct3D_GetContext()->VSSetShader(g_pVertexShader, nullptr, 0);
	Direct3D_GetContext()->PSSetShader(g_pPixelShader, nullptr, 0);

	// ���_���C�A�E�g��`��p�C�v���C���ɐݒ�
	Direct3D_GetContext()->IASetInputLayout(g_pInputLayout);

	// 设置为定值buffer描画管线
	Direct3D_GetContext()->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	Direct3D_GetContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

}
