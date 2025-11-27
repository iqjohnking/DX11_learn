#include "GolfBall.h"
#include "Input.h"
#include "Collision.h"

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
	//std::u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
	std::u8string modelFile = u8"assets/model/golfball/golf_ball.obj";

	//テクスチャディレクトリ
	//std::string texDirectory = "assets/model/cylinder";
	std::string texDirectory = "assets/model/golfball";

	//Meshを読み込む
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
	std::vector<MATERIAL> materials = staticmesh.GetMaterials();

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

	//モデルによってスケールを調整
	m_Scale.x = 5;
	m_Scale.y = 5;
	m_Scale.z = 5;

	//初速度
	m_Velocity.x = 0.00f;
	m_Position.x = -10.00f;
}

//=======================================
//更新処理
//=======================================
void GolfBall::Update()
{
	Vector3 oldPos = m_Position;
	// 
	//const float accelPerFrame = 0.35f;  // 毎frameの加速度
	//const float maxSpeed = 1.80f;		　// 最大速度
	const float dragFactor = 0.02f;		  // 阻力係數
	const float stopEpsilon = 0.001f;	  // 


	//ボールを操作する処理
	/*
	Vector3 camFwd = GetPosition() - m_Cam->GetPosition();
	camFwd.y = 0.0f; //y軸成分を無視して水平ベクトルにする
	if (camFwd.LengthSquared() > 0) camFwd.Normalize();


	Vector3 up(0.0f, 1.0f, 0.0f);
	//「上向き × 前向き = 右向き」的外積（クロス積）
	Vector3 camRight(
		up.y * camFwd.z - up.z * camFwd.y,
		up.z * camFwd.x - up.x * camFwd.z,
		up.x * camFwd.y - up.y * camFwd.x
	);
	if (camRight.LengthSquared() > 0.0f) { camRight.Normalize(); }

	// 1) 
	Vector3 dir(0, 0, 0);
	if (Input::GetKeyPress(VK_A)) dir -= camRight; // 左 = 反向右
	if (Input::GetKeyPress(VK_D)) dir += camRight; // 右
	if (Input::GetKeyPress(VK_W)) dir += camFwd;   // 前
	if (Input::GetKeyPress(VK_S)) dir -= camFwd;   // 後
	*/

	// 2) velocity更新/正規化/速度制限/減速
	/*
	if (dir.LengthSquared() > 0.0f)
	{
		dir.Normalize();                       // 
		m_Velocity += dir * accelPerFrame;     // 

		// 最高速度限制
		float spd2 = m_Velocity.LengthSquared();
		if (spd2 > (maxSpeed * maxSpeed)) {
			m_Velocity.Normalize();
			m_Velocity *= maxSpeed;
		}
	}
	else
	{
		// 減速
		m_Velocity *= dragFactor;
		// 0
		if (m_Velocity.LengthSquared() < stopEpsilon) {
			m_Velocity = Vector3::Zero;
		}
	}
	*/
	if (m_Velocity.LengthSquared() < stopEpsilon)
	{
		m_Velocity = Vector3::Zero;
	}
	else
	{

		// 減速
		Vector3 deceleration = -m_Velocity;
		deceleration.Normalize();
		m_Acceleration = deceleration * dragFactor;

		// 加速度を速度に加算
		m_Velocity += m_Acceleration;
	}

	// 重力
	m_Velocity.y -= gravity;

	// 3) 速度を座標に加算
	m_Position += m_Velocity;

	//float radius = 2.0f * m_Scale.x; // 球の半径
	float radius =  m_Scale.x; // 球の半径

	// Groundのverticesデータを取得
	std::vector<VERTEX_3D> vertices = m_Ground->GetVertices();

	float moveDistance = 9999.0f;
	Vector3 contactPoint;
	Vector3 normal;
	// 地面との当たり判定
	for (int i = 0; i < vertices.size(); i += 3)
	{
		Collision::Polygon collisionPolygon = {
			vertices[i + 0].position,
			vertices[i + 1].position,
			vertices[i + 2].position
		};
		Vector3 cp;
		Collision::Segment collisionSegment = { oldPos, m_Position };
		Collision::Sphere collisionSphere = { m_Position,radius };

		bool isSegmentHit = Collision::CheckHit(collisionSegment, collisionPolygon, cp);
		bool isHit = Collision::CheckHit(collisionSphere, collisionPolygon, cp);

		if (isSegmentHit) {
			float md = 0;
			Vector3 np = Collision::moveSphere(collisionSegment, radius, collisionPolygon, cp, md);
			if (moveDistance > md)
			{
				moveDistance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(collisionPolygon);
			}
		}
		else if (isHit)
		{
			Vector3 np = Collision::moveSphere(collisionSphere, collisionPolygon, cp);
			float md = (np - oldPos).Length();
			if (moveDistance > md)
			{
				moveDistance = md;
				m_Position = np;
				contactPoint = cp;
				normal = Collision::GetNormal(collisionPolygon);
			}
		}
		if (moveDistance != 9999.0f)
		{
			//MessageBoxA(NULL, "当たった", "当たり判定確認", MB_OK);
			// ballの速度ベクトル法線方向成分と接線方向成分に分解
			float velocityNormal = Collision::Dot(m_Velocity, normal);
			Vector3 v1 =  velocityNormal * normal; // 法線方向成分
			Vector3 v2 = m_Velocity - v1;          // 接線方向成分
			// 反射ベクトルを計算
			const float restitution = 1.0f; // 反発係数
			const float friction = 0.9f;    // 摩擦係数
			Vector3 reflectedVelocity = v2 * friction - v1 * restitution;
			m_Velocity = reflectedVelocity;
		}
	}

	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////
	// ---- face to move direction (XZ) ----
	const float turnSpeedPerFrame = 0.314f;        // 
	const float moveEpsilon2 = 1e-6f;        // 

	if (m_Velocity.LengthSquared() > moveEpsilon2)
	{
		// 
		float targetYaw = std::atan2(m_Velocity.x, m_Velocity.z);
		float currentYaw = m_Rotation.y;

		// 
		float delta = targetYaw - currentYaw;
		//const float	TWO_PI = 6.283185307f;
		//const float		PI = 3.1415926535;

		while (delta > PI) delta -= TWO_PI;
		while (delta < -PI) delta += TWO_PI;

		// 
		if (delta > turnSpeedPerFrame) delta = turnSpeedPerFrame;
		if (delta < -turnSpeedPerFrame) delta = -turnSpeedPerFrame;

		currentYaw += delta;

		// 
		if (currentYaw > PI) currentYaw -= TWO_PI;
		if (currentYaw < -PI) currentYaw += TWO_PI;

		m_Rotation.y = currentYaw;
	}

	//camera追従
	if (m_Cam) {
		m_Cam->SetTarget(m_Position);    // 目標の座標をセットする
		m_Cam->SetTargetYaw(GetYaw());   // 目標の向きをセットする
	}

	if (m_Cam) {
		Vector3 camPos = m_Cam->GetPosition();
		Vector3 camFwd = GetPosition() - camPos;
		float len2 = camFwd.LengthSquared();	//ベクトルの大きさを判定用
		if (len2 > 0.0f) {
			camFwd /= std::sqrt(len2);			//ベクトルを正規化
			float horiz = std::sqrt(camFwd.x * camFwd.x + camFwd.z * camFwd.z); //ｘｚ平面上の長さ
			//float targetPitch = (std::atan2(camFwd.y, horiz))*0.5f; // 
			//float targetPitch = -0.2f - (std::atan2(camFwd.y, horiz)); // 
			float targetPitch = -(std::atan2(camFwd.y, horiz)) * 0.5f; // 

			m_Rotation.x = targetPitch;
		}
	}

	/*
   //速度が0に近づいたら止まる
   if (m_Velocity.LengthSquared() < 0.1f)
   {
	   m_Velocity = Vector3(0.0f, 0.0f, 0.0f);
   }
   else {
	   //何割に減速度（１フレーム
	   float decelerationPower = 0.1f;

	   Vector3 deceleration = -m_Velocity;					//速度の逆ベクトルを計算
	   deceleration.Normalize();							//ベクトルを正規化
	   m_Acceleration = deceleration * decelerationPower;	//加速度＝逆速度*何割に減速度
	   //加速度を速度に加算
	   m_Velocity += m_Acceleration;
   }
   //速度を座標に加算
   m_Position += m_Velocity;
	*/
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

void GolfBall::SetGround(Ground* ground)
{
	m_Ground = ground;
}

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

