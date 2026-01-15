#pragma once

#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h" //文字列変換用
//#include "Ground.h" 

class GolfBall :public Object
{
private:
	//速度
	DirectX::SimpleMath::Vector3 m_Velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//加速度
	DirectX::SimpleMath::Vector3 m_Acceleration = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	
	static constexpr float	TWO_PI = 6.283185307f;
	static constexpr float		PI = 3.1415926535;

	// 物理パラメータ
	//const float gravity = 1.0f;
	//const float accelPerFrame = 0.35f;  
	//const float maxSpeed = 1.80f;		
	//const float dragFactor = 0.85f;		
	//const float stopEpsilon = 0.001f;	
	
	//キャラクターの向き
	//DirectX::SimpleMath::Vector3 playerDir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);

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

	GolfBall();
	GolfBall(Camera* cam);
	~GolfBall();

	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();

	//状態の取得・設定
	int GetState() { return m_state; }
	void SetState(int state) { m_state = state; }

	//ショット
	void Shot(DirectX::SimpleMath::Vector3 dir) {m_Velocity = dir;}

	//void SetGround(Ground* ground);


	/////////////////////////////////////////

	void SetCamera(Camera* cam) { m_Cam = cam; }  // ← 追加：カメラを注入

	float GetYaw();//y
	float GetRoll();//z
	float GetPitch();//x



};

