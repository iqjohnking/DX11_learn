#include "GolfBall.h"
#include "Input.h"
#include "Collision.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

using namespace std;
using namespace DirectX::SimpleMath;

GolfBall::GolfBall(Camera* cam)
{
	SetCamera(cam);
}

GolfBall::~GolfBall()
{
}

//=======================================
//初期化処理
//=======================================
void GolfBall::Init()
{

	// メッシュ読み込み
	StaticMesh staticmesh;

	//3Dモデルデータ
	//u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
	u8string modelFile = u8"assets/model/golfball/golf_ball.obj";

	//テクスチャディレクトリ
	//string texDirectory = "assets/model/cylinder";
	string texDirectory = "assets/model/golfball";

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
	m_Scale.x = 1;
	m_Scale.y = 1;
	m_Scale.z = 1;

	//初速度
	m_Velocity.x = 0.00f;
	m_Position.x = 0.00f;
}

//=======================================
//更新処理
//=======================================
void GolfBall::Update()
{
	Vector3 oldPos = m_Position;

	// ===== 物理パラメータ =====
	const float dt = 1.0f / 60.0f;      // Δt（1フレームあたりの時間）
	const float accelPerFrame = 0.35f;  // 毎frameの加速度
	const float maxSpeed = 1.80f;       // 最大速度
	const float decelPower = 0.05f;     // 減速度
	const float stopEpsilon = 0.03f;    // 
	const float gravityAccel = 9.8f;    // 重力加速度
	const float gravityPerFrame = gravityAccel * dt;

	// =========================================================
	// 1) 入力方向（カメラ基準）
	// =========================================================
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

		if (Input::GetKeyPress(VK_A)) dir -= camRight;
		if (Input::GetKeyPress(VK_D)) dir += camRight;
		if (Input::GetKeyPress(VK_W)) dir += camFwd;
		if (Input::GetKeyPress(VK_S)) dir -= camFwd;
	}

	const bool hasInput = (dir.LengthSquared() > 0.0f);

	// =========================================================
	// 2) 停止状態の扱い（ここが重要）
	//    - 停止中に入力があれば解除（wake）
	//    - 停止中で入力なしなら、重力/座標更新/当たり判定をしない（sleep）
	// =========================================================
	if (m_state == 1)
	{
		if (hasInput)
		{
			m_state = 0;
			m_stopCount = 0;
		}
		else
		{
			// 完全停止：以降の物理を回さない
			m_Velocity = Vector3::Zero;

			// camera追従（元の処理は残す）
			if (m_Cam) {
				m_Cam->SetTarget(m_Position);
				m_Cam->SetTargetYaw(GetYaw());
			}

			// リスポーン（元の処理は残す）
			if (m_Position.y < -100.0f) {
				m_Position = Vector3(0.0f, 50.0f, 0.0f);
				m_Velocity = Vector3::Zero;
			}
			return;
		}
	}

	// =========================================================
	// 3) 速度更新（入力加速 or 減速）
	// =========================================================
	if (hasInput)
	{
		dir.Normalize();
		m_Velocity += dir * accelPerFrame;

		// 水平速度の上限（元コードそのまま）
		Vector3 velXZ(m_Velocity.x, 0.0f, m_Velocity.z);
		float spd2 = velXZ.LengthSquared();
		if (spd2 > maxSpeed * maxSpeed)
		{
			float spd = sqrt(spd2);
			velXZ /= spd;
			velXZ *= maxSpeed;
			m_Velocity.x = velXZ.x;
			m_Velocity.z = velXZ.z;
		}
	}
	else
	{
		float spd2 = m_Velocity.LengthSquared();

		if (spd2 < stopEpsilon)
		{
			m_stopCount++;
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

	// =========================================================
	// 4) 停止判定（停止に入ったら、以降を回さない）
	// =========================================================
	if (m_stopCount >= 10)
	{
		m_Velocity = Vector3::Zero;
		m_state = 1;

		// 「停止したフレーム」にも重力・移動をさせないため return
		// （ここが“斜面でまだ動く”の直接原因を潰す）
		if (m_Cam) {
			m_Cam->SetTarget(m_Position);
			m_Cam->SetTargetYaw(GetYaw());
		}
		if (m_Position.y < -100.0f) {
			m_Position = Vector3(0.0f, 0.0f, 0.0f);
			m_Velocity = Vector3::Zero;
		}
		return;
	}

	// =========================================================
	// 5) 重力 → 座標更新
	// =========================================================
	//m_Velocity.y -= gravityPerFrame;
	m_Position += m_Velocity;

	// =========================================================
	// 6) 当たり判定（元コードの構造を保ったまま整理）
	// =========================================================
	float radius = m_Scale.x;

	std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
	std::vector<VERTEX_3D> vertices;
	for (auto g : grounds)
	{
		std::vector<VERTEX_3D> vecs = g->GetVertices();
		for (auto v : vecs) vertices.emplace_back(v);
	}

	float moveDistance = 9999.0f;
	Vector3 contactPoint;
	Vector3 normal;

	for (int i = 0; i < (int)vertices.size(); i += 3)
	{
		Collision::Polygon poly = {
			vertices[i + 0].position,
			vertices[i + 1].position,
			vertices[i + 2].position
		};

		Vector3 cp;
		Collision::Segment seg = { oldPos, m_Position };
		Collision::Sphere  sph = { m_Position, radius };

		bool isSegmentHit = Collision::CheckHit(seg, poly, cp);
		bool isHit = Collision::CheckHit(sph, poly, cp);

		if (isSegmentHit)
		{
			float md = 0;
			Vector3 np = Collision::moveSphere(seg, radius, poly, cp, md);
			if (moveDistance > md)
			{
				moveDistance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(poly);
			}
		}
		else if (isHit)
		{
			Vector3 np = Collision::moveSphere(sph, poly, cp);
			float md = (np - oldPos).Length();
			if (moveDistance > md)
			{
				moveDistance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(poly);
			}
		}
	}

	// 衝突後の速度処理（元コードそのまま）
	if (moveDistance != 9999.0f)
	{
		float velocityNormal = Collision::Dot(m_Velocity, normal);
		Vector3 v1 = velocityNormal * normal;
		Vector3 v2 = m_Velocity - v1;

		const float restitution = 0.8f;
		const float friction = 0.9f;

		Vector3 reflectedVelocity = v2 * friction - v1 * restitution;
		m_Velocity = reflectedVelocity;
	}

	// =========================================================
	// 7) face to move direction（元コードそのまま）
	// =========================================================
	const float turnSpeedPerFrame = 0.314f;
	const float moveEpsilon2 = 1e-6f;
	const float steepDotThreshold = 0.95f;

	if (m_Velocity.LengthSquared() > moveEpsilon2)
	{
		bool shouldTurn = false;

		if (hasInput)
		{
			shouldTurn = true;
		}
		else
		{
			if (moveDistance != 9999.0f)
			{
				Vector3 up(0.0f, 1.0f, 0.0f);
				Vector3 n = normal;
				n.Normalize();

				float dotNU = n.x * up.x + n.y * up.y + n.z * up.z;
				if (dotNU < steepDotThreshold)
				{
					shouldTurn = true;
				}
			}
		}

		if (shouldTurn)
		{
			float targetYaw = atan2(m_Velocity.x, m_Velocity.z);
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
		}
	}

	// =========================================================
	// 8) camera追従 / リスポーン
	// =========================================================
	if (m_Cam) {
		m_Cam->SetTarget(m_Position);
		m_Cam->SetTargetYaw(GetYaw());
	}

	if (m_Position.y < -100.0f) {
		m_Position = Vector3(0.0f, 50.0f, 0.0f);
		m_Velocity = Vector3::Zero;
	}
	// =========================================================
	// 9)poleとの当たり判定
	// =========================================================
	vector<Pole*> pole = Game::GetInstance()->GetObjects<Pole>();
	if (pole.size() > 0)
	{
		Vector3 polePos = pole[0]->GetPosition();
		Collision::Sphere golfSphere = { m_Position, radius };
		Collision::Sphere poleSphere = { polePos, 0.5f };

		if(Collision::CheckHit(golfSphere, poleSphere))
		{
			m_state = 2; // hole in one
		}
	}
}


//=======================================
//描画処理
//=======================================
void GolfBall::Draw(Camera* cam)
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
void GolfBall::Uninit()
{

}

// セットの必要はもういない
// GameクラスのインスタンスからGround情報を取得するようにしたため
//void GolfBall::SetGround(Ground* ground)
//{
//	m_Ground = ground;
//}

float GolfBall::GetYaw()
{
	return m_Rotation.y;
}

float GolfBall::GetRoll()
{
	return m_Rotation.z;
}

float GolfBall::GetPitch()
{
	return m_Rotation.x;
}

