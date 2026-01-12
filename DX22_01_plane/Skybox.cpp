#include "SkyBox.h"
#include "Renderer.h"
#include <assert.h>

using namespace DirectX::SimpleMath;

//--------------------------------------------------
// Utility
//--------------------------------------------------
static VERTEX_3D MakeVtx(const Vector3& pos, const Vector3& n, const Vector2& uv)
{
	VERTEX_3D v{};
	v.position = pos;
	v.normal = n;                    // 幾何正確法線
	v.color = Color(1, 1, 1, 1);
	v.uv = uv;
	return v;
}

//--------------------------------------------------
// Init
//--------------------------------------------------
void SkyBox::Init()
{
	// テクスチャ（6枚）ロード（+X,-X,+Y,-Y,+Z,-Z）
	bool ok = true;
	//ok &= m_Textures[0].Load("assets/texture/galaxy/galaxy+X.tga");
	//ok &= m_Textures[1].Load("assets/texture/galaxy/galaxy-X.tga");
	//ok &= m_Textures[2].Load("assets/texture/galaxy/galaxy+Y.tga");
	//ok &= m_Textures[3].Load("assets/texture/galaxy/galaxy-Y.tga");
	//ok &= m_Textures[4].Load("assets/texture/galaxy/galaxy+Z.tga");
	//ok &= m_Textures[5].Load("assets/texture/galaxy/galaxy-Z.tga");
	ok &= m_Textures[0].Load("assets/texture/skyboxes/heaven/heaven_rt.jpg");
	ok &= m_Textures[1].Load("assets/texture/skyboxes/heaven/heaven_lf.jpg");
	ok &= m_Textures[2].Load("assets/texture/skyboxes/heaven/heaven_up.jpg");
	ok &= m_Textures[3].Load("assets/texture/skyboxes/heaven/heaven_dn.jpg");
	ok &= m_Textures[4].Load("assets/texture/skyboxes/heaven/heaven_ft.jpg");
	ok &= m_Textures[5].Load("assets/texture/skyboxes/heaven/heaven_bk.jpg");
	assert(ok && "SkyBox texture load failed");

	// シェーダ
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

	// 頂点（24）/ インデックス（36）
	m_Vertices.resize(24);
	m_Indices.resize(36);

	const float s = m_Size * 0.5f;

	//==================================================
	// 正しい法線（幾何の外向き）
	// ※ index で内側可視にしているので反転しない
	//==================================================
	const Vector3 nPosX(+1, 0, 0);
	const Vector3 nNegX(-1, 0, 0);
	const Vector3 nPosY(0, +1, 0);
	const Vector3 nNegY(0, -1, 0);
	const Vector3 nPosZ(0, 0, +1);
	const Vector3 nNegZ(0, 0, -1);

	//==================================================
	// 頂点定義（各面4頂点）
	//==================================================

	constexpr float EPS = 0.001f;

	// +X
	m_Vertices[0] = MakeVtx(Vector3(+s, -s, -s), nPosX, Vector2(0 + EPS, 1 - EPS));
	m_Vertices[1] = MakeVtx(Vector3(+s, +s, -s), nPosX, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[2] = MakeVtx(Vector3(+s, +s, +s), nPosX, Vector2(1 - EPS, 0 + EPS));
	m_Vertices[3] = MakeVtx(Vector3(+s, -s, +s), nPosX, Vector2(1 - EPS, 1 - EPS));

	// -X
	m_Vertices[4] = MakeVtx(Vector3(-s, -s, +s), nNegX, Vector2(0 + EPS, 1 - EPS));
	m_Vertices[5] = MakeVtx(Vector3(-s, +s, +s), nNegX, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[6] = MakeVtx(Vector3(-s, +s, -s), nNegX, Vector2(1 - EPS, 0 + EPS));
	m_Vertices[7] = MakeVtx(Vector3(-s, -s, -s), nNegX, Vector2(1 - EPS, 1 - EPS));

	// +Y（天井）
	m_Vertices[8] = MakeVtx(Vector3(-s, +s, -s), nPosY, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[9] = MakeVtx(Vector3(-s, +s, +s), nPosY, Vector2(1 - EPS, 0 + EPS));
	m_Vertices[10] = MakeVtx(Vector3(+s, +s, +s), nPosY, Vector2(1 - EPS, 1 - EPS));
	m_Vertices[11] = MakeVtx(Vector3(+s, +s, -s), nPosY, Vector2(0 + EPS, 1 - EPS));

	// -Y（床）
	m_Vertices[12] = MakeVtx(Vector3(-s, -s, -s), nNegY, Vector2(0 + EPS, 1 - EPS));
	m_Vertices[13] = MakeVtx(Vector3(+s, -s, -s), nNegY, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[14] = MakeVtx(Vector3(+s, -s, +s), nNegY, Vector2(1 - EPS, 0 + EPS));
	m_Vertices[15] = MakeVtx(Vector3(-s, -s, +s), nNegY, Vector2(1 - EPS, 1 - EPS));

	// +Z
	m_Vertices[16] = MakeVtx(Vector3(-s, -s, +s), nPosZ, Vector2(1 - EPS, 1 - EPS));
	m_Vertices[17] = MakeVtx(Vector3(+s, -s, +s), nPosZ, Vector2(0 + EPS, 1 - EPS));
	m_Vertices[18] = MakeVtx(Vector3(+s, +s, +s), nPosZ, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[19] = MakeVtx(Vector3(-s, +s, +s), nPosZ, Vector2(1 - EPS, 0 + EPS));

	// -Z
	m_Vertices[20] = MakeVtx(Vector3(+s, -s, -s), nNegZ, Vector2(1 - EPS, 1 - EPS));
	m_Vertices[21] = MakeVtx(Vector3(-s, -s, -s), nNegZ, Vector2(0 + EPS, 1 - EPS));
	m_Vertices[22] = MakeVtx(Vector3(-s, +s, -s), nNegZ, Vector2(0 + EPS, 0 + EPS));
	m_Vertices[23] = MakeVtx(Vector3(+s, +s, -s), nNegZ, Vector2(1 - EPS, 0 + EPS));

	//==================================================
	// インデックス（内側から見える巻き）
	//==================================================
	m_Indices = {
		0, 2, 1,  0, 3, 2,      // +X
		4, 6, 5,  4, 7, 6,      // -X
		8,10, 9,  8,11,10,     // +Y
		12,14,13, 12,15,14,    // -Y
		16,18,17, 16,19,18,    // +Z
		20,22,21, 20,23,22     // -Z
	};

	// バッファ作成
	m_VertexBuffer.Create(m_Vertices);
	m_IndexBuffer.Create(m_Indices);

	// Transform
	m_Scale = Vector3(1, 1, 1);
}

//--------------------------------------------------
// Update
//--------------------------------------------------
void SkyBox::Update()
{
}

//--------------------------------------------------
// Draw
//--------------------------------------------------
void SkyBox::Draw(Camera* cam)
{
	// カメラ
	cam->SetCamera();

	// 常にカメラ位置へ追従
	const Vector3 p = cam->GetPosition();
	Matrix world = Matrix::CreateTranslation(p);
	Renderer::SetWorldMatrix(&world);

	ID3D11DeviceContext* dc = Renderer::GetDeviceContext();
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Shader.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();

	// 面ごと描画
	for (int face = 0; face < 6; ++face)
	{
		m_Textures[face].SetGPU();
		dc->DrawIndexed(6, face * 6, 0);
	}
}

//--------------------------------------------------
// Uninit
//--------------------------------------------------
void SkyBox::Uninit()
{
}
