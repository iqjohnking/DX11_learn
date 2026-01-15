#pragma once
#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h" //文字列変換用

class Hammer :public Object
{
private:

	static constexpr float	TWO_PI = 6.283185307f;
	static constexpr float		PI = 3.1415926535;

	//速度
	DirectX::SimpleMath::Vector3 m_Velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//加速度
	DirectX::SimpleMath::Vector3 m_Acceleration = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//向き
	DirectX::SimpleMath::Vector3 playerDir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);

	float AccelPerFrame = 0.35f;
	float decelPower = 0.05f; // 減速度はこれを掛ける、大きいほど減速が早い
	float MaxSpeed = 2.0f;
	float MaxSpeedY = 0.2f;

	float turnSpeedPerFrame = 0.314f; // ラジアン

	float stopEpsilon = 0.3f;


	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数

	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ

	// 追加：カメラ参照を保持
	Camera* m_Cam = nullptr;

	//state
	int m_state = 0;
	int m_stopCount = 0;

public:

	Hammer();
	~Hammer();

	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();

	//状態の取得・設定
	int GetState() { return m_state; }
	void SetState(int state) { m_state = state; }

	//ショット
	//void Shot(DirectX::SimpleMath::Vector3 dir) { m_Velocity = dir; }

	//void SetGround(Ground* ground);


	/////////////////////////////////////////

	void SetCamera(Camera* cam) { m_Cam = cam; }  // ← 追加：カメラを注入

	float GetYaw();//y
	float GetRoll();//z
	float GetPitch();//x

	void SetPosition(float x, float y, float z)
	{
		DirectX::SimpleMath::Vector3 pos;
		pos = { x, y, z };
		m_Position = pos;
	}





};

