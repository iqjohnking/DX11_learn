#include "Hammer.h"
#include "Input.h"
//#include "Collision.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

using namespace std;
using namespace DirectX::SimpleMath;

Hammer::Hammer()
{
	m_Cam = Game::GetInstance()->GetCamera();
}

Hammer::~Hammer()
{
}

//=======================================
//初期化処理
//=======================================
void Hammer::Init()
{
	// メッシュ読み込み
	StaticMesh staticmesh;
	//3Dモデルデータ
	u8string modelFile = u8"assets/model/hammer/uploads_files_1971948_Old_Hammer_OBJ.obj";
	//テクスチャディレクトリ
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
		unique_ptr<Material> m = make_unique<Material>();// マテリアルオブジェクト生成
		m->Create(materials[i]);						// マテリアル情報をセット
		m_Materials.push_back(move(m));					// マテリアルオブジェクトを配列に追加
	}

	//モデルによってスケールを調整
	m_Scale.y = 0.2f;
	m_Scale.z = 0.2f;
	m_Scale.x = 0.2f;

	// ここが「初期地点」
	SetPosition(0, 25, -50);

	m_FixedY = m_Position.y;					// 固定高度
	m_OrbitCenter = Vector3::Zero;				// 円心（必要ならここで変える）
	Vector3 rel = m_Position - m_OrbitCenter;	// 初期地点から半径・角度を決める（XZのみ）
	rel.y = 0.0f;								// Y成分無視

	m_OrbitRadius = std::sqrt(rel.LengthSquared());// 半径
	if (m_OrbitRadius < 0.0001f)				// 半径ゼロ回避
	{
		m_OrbitRadius = 1.0f;
		rel = Vector3(0.0f, 0.0f, m_OrbitRadius);
	}


	m_OrbitTheta = std::atan2(rel.x, rel.z);	// Z+基準の atan2(x, z)
}

//=======================================
//更新処理
//=======================================
void Hammer::Update()
{
	Vector3 dir(0, 0, 0);
	if (m_Cam)
	{
		Vector3 camFwd = GetPosition() - m_Cam->GetPosition();
		camFwd.y = 0.0f;
		if (camFwd.LengthSquared() > 0.0f) camFwd.Normalize();

		Vector3 up(0.0f, 1.0f, 0.0f);
		//Vector3 camRight(
		//	up.y * camFwd.z - up.z * camFwd.y,
		//	up.z * camFwd.x - up.x * camFwd.z,
		//	up.x * camFwd.y - up.y * camFwd.x
		//);
		//if (camRight.LengthSquared() > 0.0f) camRight.Normalize();

		static constexpr float OrbitYawStepRad = 0.0314f;
		static constexpr float OrbitRadiusStep = 0.5f;
		static constexpr float OrbitHightStep = 0.5f;

		if (Input::GetKeyPress(VK_A)) m_OrbitTheta += OrbitYawStepRad;
		if (Input::GetKeyPress(VK_D)) m_OrbitTheta -= OrbitYawStepRad;

		// 半径変更（円心との距離）
		if (Input::GetKeyPress(VK_W)) m_OrbitRadius -= OrbitRadiusStep; // W=内側へ
		if (Input::GetKeyPress(VK_S)) m_OrbitRadius += OrbitRadiusStep; // S=外側へ.
		m_OrbitRadius = std::clamp(m_OrbitRadius, 6.f, 20.0f);

		if (Input::GetKeyPress(VK_Q)) m_FixedY += OrbitHightStep;
		if (Input::GetKeyPress(VK_E)) m_FixedY -= OrbitHightStep;

		// 角度から軌道上の位置を再構築（半径固定）
		m_Position.x = m_OrbitCenter.x + std::sin(m_OrbitTheta) * m_OrbitRadius;
		m_Position.z = m_OrbitCenter.z + std::cos(m_OrbitTheta) * m_OrbitRadius;
		m_Position.y = m_FixedY;

		// 半径や高さが変わらないように


	}


	// 向き調整
	{
		Vector3 toOrigin = Vector3(0.0f, 0.0f, 0.0f) - m_Position;
		toOrigin.y = 0.0f;

		if (toOrigin.LengthSquared() > 0.0001f)
		{
			toOrigin.Normalize();

			float targetYaw = atan2(toOrigin.x, toOrigin.z);
			float currentYaw = m_Rotation.y;

			float delta = targetYaw - currentYaw;
			while (delta > PI) delta -= TWO_PI;
			while (delta < -PI) delta += TWO_PI;

			if (delta > turnSpeedPerFrame) delta = turnSpeedPerFrame;
			if (delta < -turnSpeedPerFrame) delta = -turnSpeedPerFrame;

			currentYaw += delta;
			if (currentYaw > PI) currentYaw -= TWO_PI;
			if (currentYaw < -PI) currentYaw += TWO_PI;

			m_Rotation.y = currentYaw;
			m_Rotation.z = PI / 2;
		}
	}



	if (m_Cam) {
		m_Cam->SetTarget(m_Position);
		m_Cam->SetTargetYaw(GetYaw());
		m_Cam->ResetBehindTarget();
	}

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

	// 見た目だけ右にずらす（頭を中央に寄せる）
	//static constexpr float kHeadOffset = 4.87 - 0.32f; // モデル依存値
	float HeadOffset = (4.87f - 0.32f) * (m_Scale.x * 10); // モデル依存値
	Vector3 right(std::cos(m_Rotation.y), 0.0f, -std::sin(m_Rotation.y));
	Vector3 drawPos = m_Position + right * HeadOffset;

	//Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix t = Matrix::CreateTranslation(drawPos.x, drawPos.y, drawPos.z);

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

