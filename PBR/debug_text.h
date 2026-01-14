/*==============================================================================

   Direct3D11�p �f�o�b�N�e�L�X�g�\�� [debug_text.h]
														 Author : Youhei Sato
														 Date   : 2025/06/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DEBUG_TEXT_H
#define DEBUG_TEXT_H

#include <d3d11.h>
#include <unordered_map>
#include <string>
#include <tuple>
#include <list>
#include <wrl/client.h> // Microsoft::WRL::ComPtr��g�p����ꍇ�͕K�v
#include <DirectXMath.h>


namespace hal
{
	class DebugText
	{
	private:
		// ���ӁI�������ŊO������ݒ肳����́BRelease�s�v�B
		ID3D11Device* m_pDevice = nullptr;
		ID3D11DeviceContext* m_pContext = nullptr;

		float m_OffsetX{ 0.0f }; // �I�t�Z�b�gX���W
		float m_OffsetY{ 0.0f }; // �I�t�Z�b�gY���W
		ULONG m_MaxLine{ 0 }; // �ő�s��
		ULONG m_MaxCharactersPerLine{ 0 }; // 1�s������̍ő啶����
		float m_LineSpacing{ 0.0f }; // �s�Ԋu
		float m_CharacterSpacing{ 0.0f }; // �����Ԋu

		struct Characters { 
			Characters(const DirectX::XMFLOAT4& color) : color(color) {}
			std::string characters; 
			DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct LineStrings {
			std::list<Characters> strings;
			ULONG characterCount{ 0 };
			ULONG spaceCount{ 0 };
		};

		std::list<LineStrings> m_TextLines; // �\������e�L�X�g�s�̃��X�g
		UINT m_CharacterCount{ 0 }; // ���݂̕������i�󔒕����A���s�����A�^�u���������)

		// �t�H���g�e�N�X�`��
		std::wstring m_FileName; // �t�@�C����
		ID3D11Resource* m_pTexture = nullptr;
		ID3D11ShaderResourceView* m_pTextureView = nullptr;
		UINT m_TextureWidth{ 0 }; // �e�N�X�`���̕�
		UINT m_TextureHeight{ 0 }; // �e�N�X�`���̍���

		// �e�N�X�`���Ǘ�MAP
		static std::unordered_map<std::wstring, std::tuple<ID3D11Resource*, ID3D11ShaderResourceView*>> m_TextureMap;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer; // ���_�o�b�t�@
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer; // �C���f�b�N�X�o�b�t�@
		UINT m_BufferSourceCharacterCount{ 0 }; // �o�b�t�@�ɓo�^����Ă��镶����

		static Microsoft::WRL::ComPtr<ID3D11BlendState> m_pBlendState; // �u�����h�X�e�[�g
		static Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDepthStencilState; // �[�x�X�e���V���X�e�[�g
		static Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterizerState; // ���X�^���C�U�[�X�e�[�g

		static Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader; // ���_�V�F�[�_�[
		static Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayout; // ���̓��C�A�E�g
		static Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVSConstantBuffer; // ���_�V�F�[�_�[�萔�o�b�t�@
		static Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPixelShader; // �s�N�Z���V�F�[�_�[
		static Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState; // �T���v���[�X�e�[�g

	public:
		DebugText() = delete;
		DebugText(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const wchar_t* pFontTextureFileName, UINT screenWidth, UINT screenHeight, float offsetX = 0.0f, float offsetY = 0.0f, ULONG maxLine = 0, ULONG maxCharactersPerLine = 0, float lineSpacing = 0.0f, float characterSpacing = 0.0f);
		~DebugText();

		// ���ꖜ�������炢�܂œo�^�\�i�󔒕����A���s�����A�^�u����������j
		void SetText(const char* pText, DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

		void Draw();

		void Clear(); // �o�^����Ă���e�L�X�g��N���A
		void SetOffset(float x, float y) { m_OffsetX = x; m_OffsetY = y; }
	private:
		
		struct Vertex
		{
			DirectX::XMFLOAT3 position; // ���W
			DirectX::XMFLOAT4 color;    // �F
			DirectX::XMFLOAT2 texcoord; // �e�N�X�`�����W
		};

		void createBuffer(ULONG characterCount);
	};
}
#endif // DEBUG_TEXT_H