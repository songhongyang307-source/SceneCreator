#include "meshfield.h"
#include "direct3d.h"
#include "shader_field.h"
#include <DirectXMath.h>
#include "texture.h"
#include "camera.h"
#include "mouse.h"
#include "picking.h"
#include "meshfield_brush.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
using namespace DirectX;



/*                     MESH FIELD                                      */
static constexpr float FIELD_MESH_SIZE = 1.0;//一个正方形mesh的长度

static constexpr int FIELD_MESH_H_COUNT = 50; //横向的mesh个数
static constexpr int FIELD_MESH_V_COUNT = 50; //纵向mesh个数

static constexpr int FIELD_MESH_H_VERTEX_COUNT = FIELD_MESH_H_COUNT + 1; //横向顶点数
static constexpr int FIELD_MESH_V_VERTEX_COUNT = FIELD_MESH_V_COUNT + 1;//纵向顶点数

static constexpr int NUM_VERTEX = FIELD_MESH_H_VERTEX_COUNT * FIELD_MESH_V_VERTEX_COUNT;
static constexpr int NUM_INDEX = 6 * FIELD_MESH_H_COUNT * FIELD_MESH_V_COUNT; // 
/*                     MESH FIELD                                      */


static const char* TERRAIN_MAGIC = "MESHFIELD_V1";

#pragma pack(push, 1)
struct MeshFieldSaveHeader
{
	char     magic[16];     // "MESHFIELD_V1"
	uint32_t hVertW;        // FIELD_MESH_H_VERTEX_COUNT
	uint32_t hVertH;        // FIELD_MESH_V_VERTEX_COUNT
	uint32_t controlW;      // CONTROL_W
	uint32_t controlH;      // CONTROL_H
	float    meshSize;      // FIELD_MESH_SIZE
};
#pragma pack(pop)

static_assert(sizeof(ControlPixel) == 4, "ControlPixel size should be 4 bytes (RGBA8).");

static ID3D11Buffer* g_pVertexBuffer = nullptr; //顶点buffer
static ID3D11Buffer* g_pIndexBuffer = nullptr; //Index buffer


static int g_TexId0 = -1;
static int g_NorId0 = -1;
static int g_Roughness0 = -1;
static int g_Metallic0 = -1;

static int g_TexId1 = -1;
static int g_NorId1 = -1;
static int g_Roughness1 = -1;
static int g_Metallic1 = -1;

static int g_TexId2 = -1;
static int g_NorId2 = -1;
static int g_Roughness2 = -1;
static int g_Metallic2 = -1;

static int g_TexId3 = -1;
static int g_NorId3 = -1;
static int g_Roughness3 = -1;
static int g_Metallic3 = -1;

//static int g_Specular0 = -1;
// 注意！初始化的由外部定义的变量，不要release
static ID3D11Device* g_pDevice = nullptr;

static ID3D11DeviceContext* g_pContext = nullptr;


std::vector<ControlPixel> g_ControlCPU;
ID3D11Texture2D* g_pControlTex = nullptr;
ID3D11ShaderResourceView* g_pControlSRV = nullptr;


float g_BrushStrength = 4.0f;
float g_BrushRadius = 8.0f;

struct Vertex3D
{
	//严格按照shader3d设定的顺序进行定义！！！
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT4 color;
	XMFLOAT2 texcoord;
};


static Vertex3D g_MeshFieldVertex[NUM_VERTEX];

unsigned short g_MeshFieldIndex[NUM_INDEX];

static float g_Height[FIELD_MESH_V_VERTEX_COUNT][FIELD_MESH_H_VERTEX_COUNT];

static bool  g_TerrainEditMode = false;


static void ControlMap_ApplyCPUToGPU();



static float GetHeight(float x, float z)
{
	return 0.0f;
}

void ApplyBrushAtWorldPos(const XMFLOAT3& hitPos)
{
	float offset_x = FIELD_MESH_H_COUNT * FIELD_MESH_SIZE * 0.5f;
	float offset_z = FIELD_MESH_V_COUNT * FIELD_MESH_SIZE * 0.5f;

	float lx = hitPos.x + offset_x;
	float lz = hitPos.z + offset_z;

	int centerX = int(lx / FIELD_MESH_SIZE + 0.5f);
	int centerZ = int(lz / FIELD_MESH_SIZE + 0.5f);

	float r = g_BrushRadius;
	float r2 = r * r;

	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; x++)
		{
			float vx = x * FIELD_MESH_SIZE;
			float vz = z * FIELD_MESH_SIZE;

			float wx = vx - offset_x;
			float wz = vz - offset_z;

			float dx = wx - hitPos.x;
			float dz = wz - hitPos.z;
			float dist2 = dx * dx + dz * dz;

			if (dist2 > r2) continue;

			float t = 1.0f - sqrtf(dist2) / r;
			t = t * t;

			g_Height[z][x] += g_BrushStrength * t;
		}
	}
}
void MeshField_SetEditMode(bool enable)
{
	g_TerrainEditMode = enable;
}

bool MeshField_IsEditMode()
{
	return g_TerrainEditMode;
}



void ControlMap_Initialize()
{
	g_ControlCPU.resize(CONTROL_W * CONTROL_H);
	for (auto& p : g_ControlCPU)
	{
		p.r = 255;
		p.g = 0;
		p.b = 0;
		p.a = 0;

	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = CONTROL_W;
	desc.Height = CONTROL_H;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = g_ControlCPU.data();
	initData.SysMemPitch = CONTROL_W * sizeof(ControlPixel);

	g_pDevice->CreateTexture2D(&desc, &initData, &g_pControlTex);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	g_pDevice->CreateShaderResourceView(g_pControlTex, &srvDesc, &g_pControlSRV);



}

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;



	// 顶点buffer生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;//DYNAMIC每帧都描画，3d描画时不需要每帧都画
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX; //SIZE of g_meshfieldVertex 
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;



	//顶点信息数列制作
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{

		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; x++)
		{
			float fx = x * FIELD_MESH_SIZE;
			float fz = z * FIELD_MESH_SIZE;
			g_Height[z][x] = GetHeight(fx, fz);
		}

	}

	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; x++)
		{
			int index = x + FIELD_MESH_H_VERTEX_COUNT * z;

			float fx = x * FIELD_MESH_SIZE;
			float fz = z * FIELD_MESH_SIZE;
			float fy = g_Height[z][x];

			g_MeshFieldVertex[index].position = { fx , fy, fz };
			g_MeshFieldVertex[index].normal = { 0.0f , 1.0f , 0.0f };
			g_MeshFieldVertex[index].tangent = { 1.0f , 0.0f , 0.0f };
			g_MeshFieldVertex[index].color = { 0.0f , 1.0f , 0.0f , 1.0f };
			g_MeshFieldVertex[index].texcoord = { float(x) / float(FIELD_MESH_H_COUNT),
												  float(z) / float(FIELD_MESH_V_COUNT) };
		}

	}

	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{
		int index = 26 + FIELD_MESH_H_VERTEX_COUNT * z;
		g_MeshFieldVertex[index].color = { 1.0f , 0.0f , 0.0f , 1.0f };

	}


	//高效化，创建buffer之后不再每帧进行读写
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_MeshFieldVertex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);


	//create index buffer
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX; // sizeof(g_meshfieldindex)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	//create index  array
	int index = 0;

	for (int z = 0; z < FIELD_MESH_V_COUNT; z++)
	{

		for (int x = 0; x < FIELD_MESH_H_COUNT; x++)
		{
			//横　＋　横最大数　＊　縦
																										
			g_MeshFieldIndex[index + 0] = (z + 0) * FIELD_MESH_H_VERTEX_COUNT + x;		//0 0 1
			g_MeshFieldIndex[index + 1] = (z + 1) * FIELD_MESH_H_VERTEX_COUNT + x +1;	//3 5 6
			g_MeshFieldIndex[index + 2] = (z + 0) * FIELD_MESH_H_VERTEX_COUNT + x +1 ;	//1 1 2

			g_MeshFieldIndex[index + 3 ] = g_MeshFieldIndex[index + 0];		//0 0 1
			g_MeshFieldIndex[index + 4] = g_MeshFieldIndex[index + 1] - 1;	//2 4 5
			g_MeshFieldIndex[index + 5] = g_MeshFieldIndex[index + 1];		//3 5 6   
		
			index += 6;
		}

	}




	//高效化，创建buffer之后不再每帧进行读写
	sd.pSysMem = g_MeshFieldIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);


	g_TexId0 = Texture_Load(L"resource/texture/Grass_And_Rubble_pjwey0_1K_BaseColor.jpg",true);
	g_NorId0 = Texture_Load(L"resource/texture/Grass_And_Rubble_pjwey0_1K_Normal.jpg");
	g_Metallic0 = Texture_Load(L"resource/texture/Metallic0.png");
	g_Roughness0 = Texture_Load(L"resource/texture/Grass_And_Rubble_pjwey0_1K_Roughness.jpg");


	g_TexId1 = Texture_Load(L"resource/texture/Construction_Gravel_ngsqJ0_1K_BaseColor.jpg",true);
	g_NorId1 = Texture_Load(L"resource/texture/Construction_Gravel_ngsqJ0_1K_Normal.jpg");
	g_Metallic1 = Texture_Load(L"resource/texture/Metallic0.png");
	g_Roughness1 = Texture_Load(L"resource/texture/Construction_Gravel_ngsqJ0_1K_Roughness.jpg");

	g_TexId2 = Texture_Load(L"resource/texture/Ground_Rubble_vl3jbbcg_1K_BaseColor.jpg",true);
	g_NorId2 = Texture_Load(L"resource/texture/Ground_Rubble_vl3jbbcg_1K_Normal.jpg");
	g_Metallic2 = Texture_Load(L"resource/texture/Metallic0.png");
	g_Roughness2 = Texture_Load(L"resource/texture/Ground_Rubble_vl3jbbcg_1K_Roughness.jpg");

	g_TexId3 = Texture_Load(L"resource/texture/rubble_ground_Basecolor.png",true);
	g_NorId3 = Texture_Load(L"resource/texture/rubble_ground_Normal.png");
	g_Metallic3 = Texture_Load(L"resource/texture/Metallic0.png");
	g_Roughness3 = Texture_Load(L"resource/texture/rubble_ground_Roughness.png");

	

	
	//g_Specular0 = Texture_Load(L"resource/texture/Roman_Stone_Floor_thsjdfxq_1K_Specular.jpg");
	ShaderField_Initialize(pDevice, pContext);
	ControlMap_Initialize();

}

static void MeshField_RebuildNormals()
{
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; ++z)
	{
		for ( int x =0; x < FIELD_MESH_H_VERTEX_COUNT; ++x)
		{
			int idx = x + FIELD_MESH_H_VERTEX_COUNT * z;
			g_MeshFieldVertex[idx].normal = { 0 , 0, 0 };

		}
	}

	for (int i = 0 ; i < NUM_INDEX; i += 3)
	{
		int i0 = g_MeshFieldIndex[i + 0];
		int i1 = g_MeshFieldIndex[i + 1];
		int i2 = g_MeshFieldIndex[i + 2];

		XMFLOAT3 p0 = g_MeshFieldVertex[i0].position;
		XMFLOAT3 p1 = g_MeshFieldVertex[i1].position;
		XMFLOAT3 p2 = g_MeshFieldVertex[i2].position;

		XMVECTOR v0 = XMLoadFloat3(&p0);
		XMVECTOR v1 = XMLoadFloat3(&p1);
		XMVECTOR v2 = XMLoadFloat3(&p2);

		XMVECTOR e1 = v1 - v0;
		XMVECTOR e2 = v2 - v0;
		XMVECTOR n = XMVector3Cross(e1, e2);

		XMVECTOR n0 = XMLoadFloat3(&g_MeshFieldVertex[i0].normal);
		XMVECTOR n1 = XMLoadFloat3(&g_MeshFieldVertex[i1].normal);
		XMVECTOR n2 = XMLoadFloat3(&g_MeshFieldVertex[i2].normal);

		n0 += n;
		n1 += n;
		n2 += n;

		XMStoreFloat3(&g_MeshFieldVertex[i0].normal, n0);
		XMStoreFloat3(&g_MeshFieldVertex[i1].normal, n1);
		XMStoreFloat3(&g_MeshFieldVertex[i2].normal, n2);
	}

	// 归一化
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; ++z)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; ++x)
		{
			int idx = x + FIELD_MESH_H_VERTEX_COUNT * z;
			XMVECTOR n = XMLoadFloat3(&g_MeshFieldVertex[idx].normal);
			n = XMVector3Normalize(n);
			XMStoreFloat3(&g_MeshFieldVertex[idx].normal, n);
		}
	}
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; ++z)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; ++x)
		{
			int idx = x + FIELD_MESH_H_VERTEX_COUNT * z;
			g_MeshFieldVertex[idx].tangent = { 1,0,0 };
		}
	}
}


void MeshField_RebuildVertices()
{
	// 1) 从高度表写回顶点 y
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; ++z)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; ++x)
		{
			int idx = x + FIELD_MESH_H_VERTEX_COUNT * z;
			g_MeshFieldVertex[idx].position.y = g_Height[z][x];
		}
	}

	// 2) 重算法线
	MeshField_RebuildNormals();

	// 3) 写回 GPU buffer
	g_pContext->UpdateSubresource(g_pVertexBuffer, 0, nullptr,
		g_MeshFieldVertex, 0, 0);
}



void MeshField_Draw()
{
	ShaderField_SetTiling(8.0f,8.0f);
	//shader绘制管线设置
	ShaderField_Begin();
	
	//texture set up
	Texture_SetTextureSRV(g_pControlSRV, 0);

	Texture_SetTexture(g_TexId0, 1);  // Albedo0
	Texture_SetTexture(g_NorId0, 2);  // Normal0
	Texture_SetTexture(g_Metallic0, 3);  // Metallic0
	Texture_SetTexture(g_Roughness0, 4);  // Roughness0

	// layer1
	Texture_SetTexture(g_TexId1, 5);
	Texture_SetTexture(g_NorId1, 6);
	Texture_SetTexture(g_Metallic1, 7);
	Texture_SetTexture(g_Roughness1, 8);

	// layer2
	Texture_SetTexture(g_TexId2, 9);
	Texture_SetTexture(g_NorId2, 10);
	Texture_SetTexture(g_Metallic2, 11);
	Texture_SetTexture(g_Roughness2, 12);

	// layer3
	Texture_SetTexture(g_TexId3, 13);
	Texture_SetTexture(g_NorId3, 14);
	Texture_SetTexture(g_Metallic3, 15);
	Texture_SetTexture(g_Roughness3, 16);


	

	ShaderField_SetColor({ 1.0f,1.0f, 1.0f, 1.0f });

	ShaderField_SetMaterialParam(1.0f, 1.0f, 1.0f, 1.0f);
	//Texture_SetTexture(g_Specular0, 4);
	//顶点buffer绘画管线设定
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


	//index buffer set up to darwing buffer
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);//IF NOT enough ,change R16 to R32

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	float offset_x = FIELD_MESH_H_COUNT * FIELD_MESH_SIZE * 0.5f;
	float offset_z = FIELD_MESH_V_COUNT * FIELD_MESH_SIZE * 0.5f;

	ShaderField_SetWorldMatrix(XMMatrixTranslation(-offset_x ,0.0f, -offset_z));

	// drawing polygon
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);

}

void MeshField_Finalize(void)
{
	ShaderField_Finalize();

	SAFE_RELEASE(g_pControlSRV);
	SAFE_RELEASE(g_pControlTex);

	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

bool GroundPosToControlUV(const XMFLOAT3& hitPos, float& u, float& v)
{
	// 地面范围 [-offset_x, offset_x], [-offset_z, offset_z]
	float offset_x = FIELD_MESH_H_COUNT * FIELD_MESH_SIZE * 0.5f;
	float offset_z = FIELD_MESH_V_COUNT * FIELD_MESH_SIZE * 0.5f;

	// 模型世界矩阵把地面整体往 -offset 移了一下，这里要反一下
	float localX = hitPos.x + offset_x;
	float localZ = hitPos.z + offset_z;

	if (localX < 0 || localX > offset_x * 2 ||
		localZ < 0 || localZ > offset_z * 2)
		return false; // 点击到地面外面去了

	u = localX / (offset_x * 2.0f); // 0~1
	v = localZ / (offset_z * 2.0f); // 0~1
	return true;
}

bool ControlUVToPixel(float u, float v, int& px, int& py)
{
	px = (int)(u * CONTROL_W);
	py = (int)(v * CONTROL_H);
	if (px < 0 || px >= CONTROL_W || py < 0 || py >= CONTROL_H) return false;
	return true;
}

void MeshField_UpdateTerrainPaint()
{

	if (!MeshField_IsEditMode()) return;  // 如果你有编辑模式开关的话

	Mouse_State ms{};
	Mouse_GetState(&ms);
	if (!ms.leftButton) return;           // 没按左键就不画

	float mouseX = float(ms.x);
	float mouseY = float(ms.y);

	// 1. 屏幕坐标 -> 射线
	Ray r = ScreenPointToRay(
		mouseX, mouseY,
		(float)Direct3D_GetBackBufferWidth(),
		(float)Direct3D_GetBackBufferHeight(),
		Camera_GetMatrix(),              // view
		Camera_GetPerspectiveMatrix()    // proj
	);

	// 2. 射线 → 地面 y=0 求交
	XMFLOAT3 hitPos;
	if (!RayHitPlaneY0(r, hitPos))
		return;

	// 3. 把这个交点交给“笔刷模块”去画
	PaintControlMapAt(hitPos);
}

bool MeshField_Save(const char* filename)
{
	if (!filename) return false;

	FILE* fp = nullptr;
	if (fopen_s(&fp, filename, "wb") != 0 || !fp) return false;

	MeshFieldSaveHeader hdr{};
	memset(&hdr, 0, sizeof(hdr));
	strncpy_s(hdr.magic, TERRAIN_MAGIC, _TRUNCATE);
	hdr.hVertW = FIELD_MESH_H_VERTEX_COUNT;
	hdr.hVertH = FIELD_MESH_V_VERTEX_COUNT;
	hdr.controlW = CONTROL_W;
	hdr.controlH = CONTROL_H;
	hdr.meshSize = FIELD_MESH_SIZE;

	// 1) 写 header
	if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return false; }

	// 2) 写高度（二维数组是连续内存，可直接写）
	const size_t heightCount = size_t(FIELD_MESH_H_VERTEX_COUNT) * size_t(FIELD_MESH_V_VERTEX_COUNT);
	if (fwrite(&g_Height[0][0], sizeof(float), heightCount, fp) != heightCount)
	{
		fclose(fp);
		return false;
	}

	// 3) 写控制图（笔刷绘制结果）
	const size_t controlCount = size_t(CONTROL_W) * size_t(CONTROL_H);
	if (g_ControlCPU.size() != controlCount)
	{
		fclose(fp);
		return false;
	}
	if (fwrite(g_ControlCPU.data(), sizeof(ControlPixel), controlCount, fp) != controlCount)
	{
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}

bool MeshField_Load(const char* filename)
{
	if (!filename) return false;

	FILE* fp = nullptr;
	if (fopen_s(&fp, filename, "rb") != 0 || !fp) return false;

	MeshFieldSaveHeader hdr{};
	if (fread(&hdr, sizeof(hdr), 1, fp) != 1) { fclose(fp); return false; }

	// 1) 校验 magic + 尺寸（你现在地形尺寸是编译期常量，建议严格匹配）
	if (strncmp(hdr.magic, TERRAIN_MAGIC, strlen(TERRAIN_MAGIC)) != 0)
	{
		fclose(fp);
		return false;
	}
	if (hdr.hVertW != FIELD_MESH_H_VERTEX_COUNT ||
		hdr.hVertH != FIELD_MESH_V_VERTEX_COUNT ||
		hdr.controlW != CONTROL_W ||
		hdr.controlH != CONTROL_H)
	{
		// 尺寸不一致：你可以选择“失败”或“做兼容读取”
		fclose(fp);
		return false;
	}

	// 2) 读高度
	const size_t heightCount = size_t(FIELD_MESH_H_VERTEX_COUNT) * size_t(FIELD_MESH_V_VERTEX_COUNT);
	if (fread(&g_Height[0][0], sizeof(float), heightCount, fp) != heightCount)
	{
		fclose(fp);
		return false;
	}

	// 3) 读控制图
	const size_t controlCount = size_t(CONTROL_W) * size_t(CONTROL_H);
	g_ControlCPU.resize(controlCount);
	if (fread(g_ControlCPU.data(), sizeof(ControlPixel), controlCount, fp) != controlCount)
	{
		fclose(fp);
		return false;
	}

	fclose(fp);

	// 4) 应用到渲染
	MeshField_RebuildVertices();     // 高度 -> 顶点y + 法线 + UpdateSubresource
	ControlMap_ApplyCPUToGPU();      // CPU 控制图 -> GPU 控制贴图

	return true;
}

static void ControlMap_ApplyCPUToGPU()
{
	if (!g_pControlTex || !g_pContext) return;

	// UpdateSubresource 对 D3D11_USAGE_DEFAULT 的纹理是 OK 的
	g_pContext->UpdateSubresource(
		g_pControlTex,
		0,
		nullptr,
		g_ControlCPU.data(),
		CONTROL_W * sizeof(ControlPixel), // row pitch
		0
	);
}

bool MeshField_DeleteSave(const char* filename)
{
	if (!filename) return false;
	return (remove(filename) == 0);
}