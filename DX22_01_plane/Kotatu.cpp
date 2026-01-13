#include "Kotatu.h"
//#include "Collision.h"
#include "Game.h"
#include "Golfball.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
// 初期化処理
//=======================================
void Kotatu::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;

	// 3Dモデルデータ
	std::u8string modelFile = u8"assets/model/kotatu_HEKIU/kotatu.fbx";

	// テクスチャディレクトリ
	std::string texDirectory = "assets/model/kotatu_HEKIU";

	// Meshを読み込む
	std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
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
		std::unique_ptr<Material> m = std::make_unique<Material>();

		// マテリアル情報をセット
		m->Create(materials[i]);

		// マテリアルオブジェクトを配列に追加
		m_Materials.push_back(std::move(m));
	}

	// モデルによってスケールを調整
	m_Scale.x = 20;
	m_Scale.y = 20;
	m_Scale.z = 20;

	m_Rotation.x = DirectX::XM_PIDIV2;


	// --- ここから：回転/スケール後のAABB計算 ---
	const auto& verts = staticmesh.GetVertices();

	Vector3 minP(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3 maxP(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
	Matrix rs = s * r; // 位置はまだ入れない（原点基準でAABBが欲しい）

	for (const auto& v : verts)
	{
		Vector3 p = Vector3::Transform(v.position, rs);

		minP.x = std::min<float>(minP.x, p.x);
		minP.y = std::min<float>(minP.y, p.y);
		minP.z = std::min<float>(minP.z, p.z);

		maxP.x = std::max<float>(maxP.x, p.x);
		maxP.y = std::max<float>(maxP.y, p.y);
		maxP.z = std::max<float>(maxP.z, p.z);
	}

	Vector3 size = maxP - minP;

	{
		std::ostringstream oss;
		oss << "[Kotatu] AABB(min)=" << minP.x << "," << minP.y << "," << minP.z
			<< " AABB(max)=" << maxP.x << "," << maxP.y << "," << maxP.z
			<< " size=" << size.x << "," << size.y << "," << size.z << "\n";
		OutputDebugStringA(oss.str().c_str());
	}

	m_Position.y = -maxP.y;
}

//=======================================
// 更新処理
//=======================================
void Kotatu::Update()
{

	//// ゴルフボールの位置を取得
	//vector<GolfBall*> ballpt = Game::GetInstance()->GetObjects<GolfBall>();
	//if (ballpt.size() > 0)
	//{
	//	// 矢印の位置を更新
	//	m_Position = ballpt[0]->GetPosition();
	//}


}

//=======================================
// 描画処理
//=======================================
void Kotatu::Draw(Camera* cam)
{
	//if (m_State == 0)return; // 非表示ならreturn

	//カメラを選択する
	cam->SetCamera();

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
			m_subsets[i].IndexNum, // 描画するインデックス数
			m_subsets[i].IndexBase, // 最初のインデックスバッファの位置	
			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	}
}

//=======================================
// 終了処理
//=======================================
void Kotatu::Uninit()
{

}


//状態の設定
void Kotatu::SetState(int s)
{
	//m_State = s;
}

