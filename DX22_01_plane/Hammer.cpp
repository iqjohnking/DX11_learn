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
	Vector3 dir(0, 0, 0);
	if (m_Cam)
	{
		Vector3 camFwd = GetPosition() - m_Cam->GetPosition();
		camFwd.y = 0.0f;
		if (camFwd.LengthSquared() > 0.0f) camFwd.Normalize();

		Vector3 up(0.0f, 1.0f, 0.0f);
		Vector3 camRight(
			up.y * camFwd.z - up.z * camFwd.y,
			up.z * camFwd.x - up.x * camFwd.z,
			up.x * camFwd.y - up.y * camFwd.x
		);
		if (camRight.LengthSquared() > 0.0f) camRight.Normalize();


		// 中心（いまは原点を周回）
		const Vector3 center = Vector3::Zero;

		// 現在の半径を「不変の軌道」として使う（XZのみ）
		Vector3 rel = m_Position - center;
		rel.y = 0.0f;

		float radius = std::sqrt(rel.LengthSquared());
		if (radius < 0.0001f)
		{
			radius = 1.0f;
			rel = Vector3(0.0f, 0.0f, radius);
		}

		// 現在角度（Z+ を基準に atan2(x, z)）
		float theta = std::atan2(rel.x, rel.z);

		// 角速度（1フレームあたりの回転量）※好みで調整
		static constexpr float OrbitRadPerFrame = 0.05f;

		if (Input::GetKeyPress(VK_A)) theta += OrbitRadPerFrame;
		if (Input::GetKeyPress(VK_D)) theta -= OrbitRadPerFrame;

		// 角度から軌道上の位置を再構築（半径固定）
		m_Position.x = center.x + std::sin(theta) * radius;
		m_Position.z = center.z + std::cos(theta) * radius;

		// 慣性で半径が崩れないように、XZ速度は殺す（軌道運動にするため）
		m_Velocity.x = 0.0f;
		m_Velocity.z = 0.0f;


		if (Input::GetKeyPress(VK_W)) dir += camFwd;
		if (Input::GetKeyPress(VK_S)) dir -= camFwd;
		if (Input::GetKeyPress(VK_Q)) dir += up;
		if (Input::GetKeyPress(VK_E)) dir -= up;
	}

	bool hasInput = (dir.LengthSquared() > 0.0f);

	if (hasInput)
	{
		dir.Normalize();
		m_Velocity += dir * AccelPerFrame;

		// 水平速度の上限（元コードそのまま）
		Vector3 velXZ(m_Velocity.x, 0.0f, m_Velocity.z);
		float spd2 = velXZ.LengthSquared();
		if (spd2 > MaxSpeed * MaxSpeed)
		{
			float spd = sqrt(spd2);
			velXZ /= spd;
			velXZ *= MaxSpeed;
			m_Velocity.x = velXZ.x;
			m_Velocity.z = velXZ.z;
		}
		// 垂直速度(Y)の上限（追加）
		if (m_Velocity.y > MaxSpeedY)
		{
			m_Velocity.y = MaxSpeedY;
		}
		else if (m_Velocity.y < -MaxSpeedY)
		{
			m_Velocity.y = -MaxSpeedY;
		}
	}
	else
	{
		float spd2 = m_Velocity.LengthSquared();
		const float stopEps2 = stopEpsilon * stopEpsilon;

		if (spd2 < stopEps2) // 停止判定
		{
			m_stopCount++;
			m_Velocity = Vector3::Zero;
			m_Acceleration = Vector3::Zero;
		}
		else
		{
			m_stopCount = 0;

			Vector3 deceleration = -m_Velocity;
			deceleration.Normalize();
			m_Acceleration = deceleration * decelPower;
			m_Velocity += m_Acceleration;
		}
	}
	m_Position += m_Velocity;

	// 常に XZ 平面の原点(0,0,0)を向く
	{
		Vector3 toOrigin = Vector3(0.0f, 0.0f, 0.0f) - m_Position;
		toOrigin.y = 0.0f; // XZ 平面のみ

		if (toOrigin.LengthSquared() > 0.0001f)
		{
			toOrigin.Normalize();

			// Z+ を前とする yaw（元の atan2(x,z) と合わせる）
			float targetYaw = atan2(toOrigin.x, toOrigin.z);
			float currentYaw = m_Rotation.y;

			float delta = targetYaw - currentYaw;

			while (delta > PI)   delta -= TWO_PI;
			while (delta < -PI)  delta += TWO_PI;

			if (delta > turnSpeedPerFrame) delta = turnSpeedPerFrame;
			if (delta < -turnSpeedPerFrame) delta = -turnSpeedPerFrame;

			currentYaw += delta;

			if (currentYaw > PI) currentYaw -= TWO_PI;
			if (currentYaw < -PI) currentYaw += TWO_PI;

			m_Rotation.y = currentYaw;
			m_Rotation.z = PI/2;
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
	static constexpr float kHeadOffset = 4.87 - 0.32f; // モデル依存値
	Vector3 right(std::cos(m_Rotation.y), 0.0f, -std::sin(m_Rotation.y));
	Vector3 drawPos = m_Position + right * kHeadOffset;
	
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

