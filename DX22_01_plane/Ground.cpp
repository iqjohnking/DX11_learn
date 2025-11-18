#include "Ground.h"
#include "stb_image.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Ground::Init()
{
	// 頂点データ
	m_SizeX = 30;
	m_SizeZ = 30;
	m_Vertices.resize(6 * m_SizeX * m_SizeZ);
	for (int z = 0; z < m_SizeZ; z++) {
		for (int x = 0; x < m_SizeX; x++) {
			int n = z * m_SizeX * 6 + x * 6;
			m_Vertices[n + 0].position = Vector3(-0.5f + x - m_SizeX / 2, 0,  0.5f - z + m_SizeZ / 2);
			m_Vertices[n + 1].position = Vector3( 0.5f + x - m_SizeX / 2, 0,  0.5f - z + m_SizeZ / 2);
			m_Vertices[n + 2].position = Vector3(-0.5f + x - m_SizeX / 2, 0, -0.5f - z + m_SizeZ / 2);
			m_Vertices[n + 3].position = Vector3(-0.5f + x - m_SizeX / 2, 0, -0.5f - z + m_SizeZ / 2);
			m_Vertices[n + 4].position = Vector3( 0.5f + x - m_SizeX / 2, 0,  0.5f - z + m_SizeZ / 2);
			m_Vertices[n + 5].position = Vector3( 0.5f + x - m_SizeX / 2, 0, -0.5f - z + m_SizeZ / 2);

			m_Vertices[n + 0].color = Color(1, 1, 1, 1);
			m_Vertices[n + 1].color = Color(1, 1, 1, 1);
			m_Vertices[n + 2].color = Color(1, 1, 1, 1);
			m_Vertices[n + 3].color = Color(1, 1, 1, 1);
			m_Vertices[n + 4].color = Color(1, 1, 1, 1);
			m_Vertices[n + 5].color = Color(1, 1, 1, 1);

			m_Vertices[n + 0].uv = Vector2(0, 0);
			m_Vertices[n + 1].uv = Vector2(1, 0);
			m_Vertices[n + 2].uv = Vector2(0, 1);
			m_Vertices[n + 3].uv = Vector2(0, 1);
			m_Vertices[n + 4].uv = Vector2(1, 0);
			m_Vertices[n + 5].uv = Vector2(1, 1);

			m_Vertices[n + 0].normal = Vector3(0, 1, 0);
			m_Vertices[n + 1].normal = Vector3(0, 1, 0);
			m_Vertices[n + 2].normal = Vector3(0, 1, 0);
			m_Vertices[n + 3].normal = Vector3(0, 1, 0);
			m_Vertices[n + 4].normal = Vector3(0, 1, 0);
			m_Vertices[n + 5].normal = Vector3(0, 1, 0);
		}
	}


	// インデックデータ
	m_Indices.resize(6 * m_SizeX * m_SizeZ);
	for (int z = 0; z < m_SizeZ; z++) {
		for (int x = 0; x < m_SizeX; x++) {

			int n = z * m_SizeX * 6 + x * 6;

			m_Indices[n + 0] = n + 0;
			m_Indices[n + 1] = n + 1;
			m_Indices[n + 2] = n + 2;
			m_Indices[n + 3] = n + 3;
			m_Indices[n + 4] = n + 4;
			m_Indices[n + 5] = n + 5;
		}
	}


	// 頂点バッファ生成
	m_VertexBuffer.Create(m_Vertices);

	// インデックスバッファ生成
	m_IndexBuffer.Create(m_Indices);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// テクスチャロード
	bool sts = m_Texture.Load("assets/texture/field.jpg");
	assert(sts == true);

	// マテリアル情報取得
	m_Material = std::make_unique<Material>();	// マテリアルオブジェクト生成
	MATERIAL material;
	material.Diffuse = Color(1, 1, 1, 1);
	material.TextureEnable = true;				//テクスチャを使うか否かのフラグ
	m_Material->Create(material);				// マテリアル情報をセット

	m_Position.y = -20;
	m_Scale.x = 3;
	m_Scale.z = 3;
}

//古い
/*
void Ground::Init()
{
	// 頂点データ
	m_Vertices.resize(4);

	m_Vertices[0].position = Vector3(-10, 0, 10);
	m_Vertices[1].position = Vector3(10, 0, 10);
	m_Vertices[2].position = Vector3(-10, 0, -10);
	m_Vertices[3].position = Vector3(10, 0, -10);

	m_Vertices[0].color = Color(1, 1, 1, 1);
	m_Vertices[1].color = Color(1, 1, 1, 1);
	m_Vertices[2].color = Color(1, 1, 1, 1);
	m_Vertices[3].color = Color(1, 1, 1, 1);

	//[0](0, 0)[1](1, 0)
	//[2](0, 1)[3](1, 1)
	m_Vertices[0].uv = Vector2(0, 0);
	m_Vertices[1].uv = Vector2(1, 0);
	m_Vertices[2].uv = Vector2(0, 1);
	m_Vertices[3].uv = Vector2(1, 1);

	// 頂点バッファ生成
	m_VertexBuffer.Create(m_Vertices);

	// インデックデータ
	m_Indices.resize(4);

	m_Indices[0] = 0;
	m_Indices[1] = 1;
	m_Indices[2] = 2;
	m_Indices[3] = 3;

	// インデックスバッファ生成
	m_IndexBuffer.Create(m_Indices);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

	// テクスチャロード
	bool sts = m_Texture.Load("assets/texture/field.jpg");
	assert(sts == true);

	m_Position.y = -40;
	m_Scale.x = 400;
	m_Scale.z = 400;
}
*/

//=======================================
//更新処理
//=======================================
void Ground::Update()
{

}

//=======================================
//描画処理
//=======================================
void Ground::Draw(Camera* cam)
{
	//カメラを選択する
	cam->SetCamera();

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	// 描画の処理
	ID3D11DeviceContext* devicecontext;
	devicecontext = Renderer::GetDeviceContext();

	// トポロジーをセット（プリミティブタイプ）
	//devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Shader.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();
	m_Texture.SetGPU();
	m_Material->SetGPU();

	devicecontext->DrawIndexed(
		m_Indices.size(),	// 描画するインデックス数
		0,					// 最初のインデックスバッファの位置
		0);
}

//=======================================
//終了処理
//=======================================
void Ground::Uninit()
{

}
