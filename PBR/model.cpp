#include <assert.h>
#include "direct3d.h"
#include "texture.h"
#include "model.h"
#include "DirectXMath.h"
#include "WICTextureLoader11.h"
using namespace DirectX;
#include "shader3d .h"
#include "shader3d_unlit.h"
#include <algorithm>
#include <cctype>

struct Vertex3d
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
};

static int g_TextureWhite = -1;

static void ComputeTangents(
	Vertex3d* vertices,
	unsigned int vertexCount,
	const unsigned int* indices,
	unsigned int indexCount
)
{
	using namespace DirectX;

	for (unsigned int i = 0; i < vertexCount; ++i)
	{
		vertices[i].tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	const unsigned int triangleCount = indexCount / 3;
	for (unsigned int i = 0; i < triangleCount; i++)
	{
		unsigned int i0 = indices[i * 3 + 0];
		unsigned int i1 = indices[i * 3 + 1];
		unsigned int i2 = indices[i * 3 + 2];

		Vertex3d& v0 = vertices[i0];
		Vertex3d& v1 = vertices[i1];
		Vertex3d& v2 = vertices[i2];

		XMVECTOR p0 = XMLoadFloat3(&v0.position);
		XMVECTOR p1 = XMLoadFloat3(&v1.position);
		XMVECTOR p2 = XMLoadFloat3(&v2.position);

		XMFLOAT2 uv0 = v0.texcoord;
		XMFLOAT2 uv1 = v1.texcoord;
		XMFLOAT2 uv2 = v2.texcoord;

		float du1 = uv1.x - uv0.x;
		float dv1 = uv1.y - uv0.y;
		float du2 = uv2.x - uv0.x;
		float dv2 = uv2.y - uv0.y;

		float det = du1 * dv2 - du2 * dv1;

		if (fabsf(det) < 1e-6f)
		{
			continue;
		}

		float invDet = 1.0f / det;

		XMVECTOR edge1 = XMVectorSubtract(p1, p0);
		XMVECTOR edge2 = XMVectorSubtract(p2, p0);

		XMVECTOR tangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge1, dv2),
				XMVectorScale(edge2, dv1)),
			invDet);

		auto Accumulate = [](Vertex3d& v, XMVECTOR t)
			{
				XMVECTOR oldT = XMLoadFloat3(&v.tangent);
				oldT = XMVectorAdd(oldT, t);
				XMStoreFloat3(&v.tangent, oldT);

			};

		Accumulate(v0, tangent);
		Accumulate(v1, tangent);
		Accumulate(v2, tangent);

	}

	for (unsigned int i = 0; i < vertexCount; i++)
	{
		Vertex3d& v = vertices[i];

		XMVECTOR n = XMLoadFloat3(&v.normal);
		XMVECTOR t = XMLoadFloat3(&v.tangent);

		if (XMVector3Equal(n, XMVectorZero())) continue;

		t = XMVector3Normalize(
			XMVectorSubtract(
				t,
				XMVectorMultiply(n, XMVector3Dot(n, t)))
		);

		XMStoreFloat3(&v.tangent, t);

	}



}
AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3 position)
{
	return {
			   {position.x + model ->local_aabb.min.x,
				position.y + model->local_aabb.min.y,
				position.z + model->local_aabb.min.z
				 } ,

			   {position.x + +model->local_aabb.max.x,//模型宽度
				position.y + +model->local_aabb.max.y,//模型高度
				position.z + +model->local_aabb.max.z}//

	};
}

static std::wstring ToWStringFromUtf8(const std::string& s)
{
	if (s.empty()) return std::wstring();

	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	
	if (len <= 0) return std::wstring();

	std::wstring ws(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);

	if (!ws.empty() && ws.back() == L'\0')ws.pop_back();

	return ws;
}

// 根据材质名（例如 "M_StoneFloor"）按命名规则加载 PBR 贴图
static PBRMaterial LoadPBRMaterialFromName(const std::string& matName)
{
	PBRMaterial mat{};

	// 去前缀 "M_"
	std::string base = matName;
	if (base.rfind("M_", 0) == 0)
	{
		base = base.substr(2);
	}

	std::wstring folder = L"resource/texture/";

	auto loadTexTryExt = [&](const std::string& suffix, bool srgb) -> int
		{
			static const char* exts[] = { ".png", ".jpg", ".jpeg" };

			for (const char* ext : exts)
			{
				std::string full = base + suffix + ext;       // e.g. "StoneFloor_BaseColor.png"
				std::wstring wfull = folder + ToWStringFromUtf8(full);

				int id = Texture_TryLoad(wfull.c_str(), srgb);
				if (id >= 0)
				{
					return id;
				}
			}
			return -1;

		};

	// BaseColor → sRGB
	mat.TexAlbedo = loadTexTryExt("_BaseColor", true);

	// 其他为线性
	mat.TexNormal = loadTexTryExt("_Normal", false);
	mat.TexRoughness = loadTexTryExt("_Roughness", false);
	mat.TexMetallic = loadTexTryExt("_Metallic", false);

	mat.BaseColorTint = { 1,1,1,1 };
	mat.MetallicFactor = 1.0f;
	mat.RoughnessFactor = 1.0f;
	mat.AoFactor = 1.0f;

	return mat;
}

MODEL* ModelLoad(const char* FileName, bool isblender, float scalar , bool isclockwise)
{
	MODEL* model = new MODEL;

	model->AiScene = aiImportFile(
		FileName,
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace); // 有就用，没有我们自己算
	assert(model->AiScene);

	unsigned int meshCount = model->AiScene->mNumMeshes;

	model->VertexBuffer = new ID3D11Buffer * [meshCount];
	model->IndexBuffer = new ID3D11Buffer * [meshCount];
	model->materials = new PBRMaterial[meshCount];

	//=============================
	// 1. 为每个 mesh 构建 VB / IB
	//=============================
	for (unsigned int m = 0; m < meshCount; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		aiMaterial* aimat = model->AiScene->mMaterials[mesh->mMaterialIndex];

		aiString matNameAI;
		aimat->Get(AI_MATKEY_NAME, matNameAI);
		std::string matName = matNameAI.C_Str();

		std::string lower = matName;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });

		bool isLeaves = (lower.find("leaves") != std::string::npos) ||
			(lower.find("foliage") != std::string::npos);
		bool isGrass = (lower.find("grass") != std::string::npos);
		bool isWindMesh = isLeaves || isGrass;

		Vertex3d* vertex = new Vertex3d[mesh->mNumVertices];

		bool hasUV0 = mesh->HasTextureCoords(0);
		bool hasNormal = mesh->HasNormals();

		// 顶点填充
		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			if (isblender)
			{
				vertex[v].position = XMFLOAT3(
					mesh->mVertices[v].x * scalar,
					-mesh->mVertices[v].z * scalar,
					mesh->mVertices[v].y * scalar
				);

				if (hasNormal)
				{
					vertex[v].normal = XMFLOAT3(
						mesh->mNormals[v].x,
						-mesh->mNormals[v].z,
						mesh->mNormals[v].y
					);
				}
				else
				{
					vertex[v].normal = XMFLOAT3(0, 1, 0);
				}
			}
			else
			{
				vertex[v].position = XMFLOAT3(
					mesh->mVertices[v].x * scalar,
					mesh->mVertices[v].y * scalar,
					mesh->mVertices[v].z * scalar
				);

				if (hasNormal)
				{
					vertex[v].normal = XMFLOAT3(
						mesh->mNormals[v].x,
						mesh->mNormals[v].y,
						mesh->mNormals[v].z
					);
				}
				else
				{
					vertex[v].normal = XMFLOAT3(0, 1, 0);
				}
			}

			if (hasUV0 && mesh->mTextureCoords[0])
			{
				int useUV = mesh->HasTextureCoords(1) ? 0 : 0;
				auto* uv = mesh->mTextureCoords[useUV];

				vertex[v].texcoord = XMFLOAT2(uv[v].x, uv[v].y);
			}
			else
			{
				vertex[v].texcoord = XMFLOAT2(0.0f, 0.0f);
			}

			vertex[v].tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);

			if (mesh->HasVertexColors(0) && mesh->mColors[0])
			{
				auto c = mesh->mColors[0][v];
				vertex[v].color = XMFLOAT4(c.r, c.g, c.b, c.a); // R当windWeight
			}
			else
			{
				vertex[v].color = XMFLOAT4(0, 0, 0, 1); // 先占位，后面再自动生成
			}

			// AABB（注意：现在是以最后一个 mesh 的 AABB 为准，如果想要整体 AABB，要在外面初始化一次再取 min/max）
			if (v == 0)
			{
				model->local_aabb.min = vertex[v].position;
				model->local_aabb.max = vertex[v].position;
			}
			else
			{
				model->local_aabb.min.x = std::min(model->local_aabb.min.x, vertex[v].position.x);
				model->local_aabb.min.y = std::min(model->local_aabb.min.y, vertex[v].position.y);
				model->local_aabb.min.z = std::min(model->local_aabb.min.z, vertex[v].position.z);
				model->local_aabb.max.x = std::max(model->local_aabb.max.x, vertex[v].position.x);
				model->local_aabb.max.y = std::max(model->local_aabb.max.y, vertex[v].position.y);
				model->local_aabb.max.z = std::max(model->local_aabb.max.z, vertex[v].position.z);
			}
		}

		// 索引填充
		unsigned int indexCount = mesh->mNumFaces * 3;
		unsigned int* index = new unsigned int[indexCount];

		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			const aiFace* face = &mesh->mFaces[f];
			assert(face->mNumIndices == 3);

			if (isclockwise)
			{
			index[f * 3 + 0] = face->mIndices[0];
			index[f * 3 + 1] = face->mIndices[1];
			index[f * 3 + 2] = face->mIndices[2];
			}
			else
			{
				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[2];
				index[f * 3 + 2] = face->mIndices[1];
			}
		}

		bool hasVColor = mesh->HasVertexColors(0) && mesh->mColors[0];

		// 1) 先把“非风网格”强制停掉（不管 FBX 有无顶点色）
		if (!isWindMesh)
		{
			for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
				vertex[v].color.x = 0.0f;   // 只清 R，其他通道保留
		}
		else
		{
			// 2) 风网格：如果 FBX 没有顶点色，就自动生成 windWeight
			if (!hasVColor)
			{
				// 计算本 mesh 的高度范围（不要用 model->local_aabb）
				float minY = 1e9f;
				float maxY = -1e9f;
				for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
				{
					minY = std::min(minY, vertex[v].position.y);
					maxY = std::max(maxY, vertex[v].position.y);
				}
				float invH = 1.0f / std::max(maxY - minY, 1e-3f);

				if (isLeaves)
				{
					// 叶子：高度渐变（比全 1 更不容易把树干带动）
					for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
					{
						float h = (vertex[v].position.y - minY) * invH; // 0~1
						h = std::max(0.0f, std::min(h, 1.0f));
						float w = powf(h, 0.8f); // 0.6~1.2 调
						vertex[v].color = XMFLOAT4(w, w, w, 1);
					}
				}
				else if (isGrass)
				{
					// 草：根部0，尖端1（按高度渐变）
					for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
					{
						float h = (vertex[v].position.y - minY) * invH; // 0~1
						h = std::max(0.0f, std::min(h, 1.0f));
						float w = powf(h, 1.6f); // 1.2~2.2 调
						vertex[v].color = XMFLOAT4(w, w, w, 1);
					}
				}
			}

			// 3) 如果 hasVColor==true：保持 FBX 的 color.r 作为权重（你已经在上面读进来了）
			//    如果你担心 FBX 顶点色太大，也可以这里 clamp 一下：
			// for (...) vertex[v].color.x = std::max(0.f, std::min(vertex[v].color.x, 1.f));
		}
		


		// 创建 VertexBuffer
		{
			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);
		}

		// 创建 IndexBuffer
		{
			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd{};
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);
		}

		delete[] vertex;
		delete[] index;
	}

	//=============================
	// 2. 材质 & 贴图
	//=============================
	//g_TextureWhite = Texture_Load(L"resource/texture/white.png");

	for (unsigned int m = 0; m < meshCount; ++m)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];
		aiMaterial* aimat = model->AiScene->mMaterials[mesh->mMaterialIndex];

		aiString matNameAI;
		aimat->Get(AI_MATKEY_NAME, matNameAI);
		std::string matName = matNameAI.C_Str();

		PBRMaterial mat = LoadPBRMaterialFromName(matName);

		std::string lower = matName;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });

		if (lower.find("leaves") != std::string::npos ||
			lower.find("foliage") != std::string::npos ||
			lower.find("grass") != std::string::npos)
		{
			mat.UseAlphaToCoverage = true;
			mat.AlphaCutoff = 0.02f; // A2C 下建议小一点
		}

		if (mat.TexAlbedo < 0)
		{
			mat.TexAlbedo = g_TextureWhite;

			aiColor3D diffuse(1, 1, 1);
			aimat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			mat.BaseColorTint = XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
		}

		model->materials[m] = mat;
	}

	return model;
}


void ModelRelease(MODEL* model)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		model->VertexBuffer[m]->Release();
		model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;
	delete[] model->materials;

	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		pair.second->Release();
	}


	aiReleaseImport(model->AiScene);


	delete model;
}
static void DumpPSSRV(ID3D11DeviceContext* ctx, UINT slot, const char* name)
{
	ID3D11ShaderResourceView* srv = nullptr;
	ctx->PSGetShaderResources(slot, 1, &srv); // AddRef

	if (!srv) { OutputDebugStringA((std::string(name) + " = null\n").c_str()); return; }

	ID3D11Resource* res = nullptr;
	srv->GetResource(&res);

	ID3D11Texture2D* tex = nullptr;
	if (res) res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

	if (tex)
	{
		D3D11_TEXTURE2D_DESC td{};
		tex->GetDesc(&td);

		char buf[256];
		sprintf_s(buf, "%s t%u: %ux%u mips=%u fmt=%d\n",
			name, slot, td.Width, td.Height, td.MipLevels, (int)td.Format);
		OutputDebugStringA(buf);

		tex->Release();
	}

	if (res) res->Release();
	srv->Release();
}
void ModelDraw(MODEL* model , const DirectX::XMMATRIX& mtxWorld)
{
	if (g_TextureWhite < 0)
		g_TextureWhite = Texture_Load(L"resource/texture/white.png", true); // sRGB
	//shader绘制管线设置
	//Shader3d_Begin();

	//Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//
	Shader3d_SetWorldMatrix(mtxWorld);

	unsigned int meshCount = model->AiScene->mNumMeshes;


	//texture set up

	for (unsigned int m = 0; m < meshCount; m++)
	{
		const PBRMaterial& mat = model->materials[m];

		if (mat.UseAlphaToCoverage)
			Direct3D_SetAlphaToCoverage();
		else
			Direct3D_SetAlphaBlendTransparent();

		Shader3d_SetPBRMaterialParams(mat);
		
		//
		//顶点buffer绘画管线设定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);


		//index buffer set up to darwing buffer
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);//IF NOT enough ,change R16 to R32

		
		// drawing polygon
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}
	Direct3D_SetAlphaBlendTransparent();


}


void ModelDrawUnlit(MODEL* model, const DirectX::XMMATRIX& mtxWorld)
{

	//shader绘制管线设置
	Shader3dUnlit_Begin();

	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//
	Shader3dUnlit_SetWorldMatrix(mtxWorld);


	//texture set up

	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiString texture;
		aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture.length != 0) {
			Direct3D_GetContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]);
			Shader3dUnlit_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

		}
		else
		{
			Texture_SetTexture(g_TextureWhite);

			aiColor3D diffuse;
			aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			Shader3dUnlit_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });

		}


		//
		//顶点buffer绘画管线设定
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);


		//index buffer set up to darwing buffer
		Direct3D_GetContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);//IF NOT enough ,change R16 to R32


		// drawing polygon
		Direct3D_GetContext()->DrawIndexed(model->AiScene->mMeshes[m]->mNumFaces * 3, 0, 0);
	}



}


// 只用于阴影 / 深度等 pass：不调用 Shader3d_Begin、不绑定贴图
void ModelDrawShadowRaw(MODEL * model)
{
	auto* ctx = Direct3D_GetContext();
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int meshCount = model->AiScene->mNumMeshes;

	for (unsigned int m = 0; m < meshCount; ++m)
	{
		// VB
		UINT stride = sizeof(Vertex3d);
		UINT offset = 0;
		ctx->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// IB
		ctx->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// Draw
		const aiMesh* mesh = model->AiScene->mMeshes[m];
		ctx->DrawIndexed(mesh->mNumFaces * 3, 0, 0);
	}
}
