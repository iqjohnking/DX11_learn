#include "Hammer.h"
#include "Input.h"
//#include "Collision.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Hammer::Init()
{

	// メッシュ読み込み
	StaticMesh staticmesh;

	//3Dモデルデータ
	//u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
	u8string modelFile = u8"assets/model/hammer/uploads_files_1971948_Old_Hammer_OBJ.obj";

	//テクスチャディレクトリ
	//string texDirectory = "assets/model/cylinder";
	string texDirectory = "assets/model/hammer";

	//Meshを読み込む
	string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// サブセット情報取得
	m_subsets = staticmesh.GetSubsets();

	// テクスチャ情報取得
	m_Textures = staticmesh.GetTextures();

	// マテリアル情報取得	
	vector<MATERIAL> materials = staticmesh.GetMaterials();

	// マテリアル数分ループ
	for (int i = 0; i < materials.size(); i++)
	{
		// マテリアルオブジェクト生成
		unique_ptr<Material> m = make_unique<Material>();

		// マテリアル情報をセット
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materials.push_back(move(m));
	}

	//モデルによってスケールを調整
	m_Scale.x = 0.1f;
	m_Scale.y = 0.1f;
	m_Scale.z = 0.1f;

	//初速度
	//m_Velocity.x = 0.00f;
	//m_Position.x = -10.00f;
}

//=======================================
//更新処理
//=======================================
void Hammer::Update()
{
	
}


//=======================================
//描画処理
//=======================================
void Hammer::Draw(Camera* cam)
{
	//カメラを選択する
	cam->SetCamera();
	//cam->SetTarget(m_Position);    // 目標の座標をセットする
	//cam->SetTargetYaw(GetYaw());   // 目標の向きをセットする

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット
	m_MeshRenderer.BeforeDraw();

	//マテリアル数分ループ 
	for (int i = 0; i < m_subsets.size(); i++)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
		m_Materials[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materials[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,		// 描画するインデックス数
			m_subsets[i].IndexBase,		// 最初のインデックスバッファの位置	
			m_subsets[i].VertexBase);	// 頂点バッファの最初から使用
	}
}

//=======================================
//終了処理
//=======================================
void Hammer::Uninit()
{

}

// セットの必要はもういない
// GameクラスのインスタンスからGround情報を取得するようにしたため
//void GolfBall::SetGround(Ground* ground)
//{
//	m_Ground = ground;
//}

float Hammer::GetYaw()
{
	return m_Rotation.y;
}

float Hammer::GetRoll()
{
	return m_Rotation.z;
}

float Hammer::GetPitch()
{
	return m_Rotation.x;
}

