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

	DirectX::SimpleMath::Vector3 m_Velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);//速度
	DirectX::SimpleMath::Vector3 m_Acceleration = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);//加速度
	DirectX::SimpleMath::Vector3 playerDir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);//向き

	float turnSpeedPerFrame = 0.314f; // ラジアン //reset用

	// 周回パラメータ（Initで設定してUpdateで上書きされないように保持）
	DirectX::SimpleMath::Vector3 m_OrbitCenter = DirectX::SimpleMath::Vector3::Zero; //あとはダムマ各階層かも
	float m_FixedY = 0.0f;			// 高さ固定用
	float m_OrbitRadius = 1.0f;		// 半径
	float m_OrbitTheta = 0.0f;		// ラジアン

	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer;	// 頂点バッファ・インデックスバッファ・インデックス数
	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ

	// 追加：カメラ参照を保持
	Camera* m_Cam = nullptr;

	//state
	//int m_state = 0;
	int m_stopCount = 0;

	// ===== 追加：演出状態（m_stateの置き換え） =====
	enum class HammerActionState : int
	{
		Idle = 0,
		Swinging,   // 位置決定
		Striking,   // 打っている
		Locked,     // 確定後
	};

	HammerActionState m_state = HammerActionState::Idle;


public:

	Hammer();
	~Hammer();

	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();

	//ショット
	//void Shot(DirectX::SimpleMath::Vector3 dir) { m_Velocity = dir; }
	/////////////////////////////////////////
	// 
	//状態の取得・設定
	HammerActionState GetState() const { return m_state; }
	void SetState(HammerActionState state) { m_state = state; }
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

