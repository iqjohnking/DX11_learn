#pragma once

#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h" //文字列変換用


class Daruma :public Object
{
protected:
	static constexpr int LAYER_COUNT = 8; // 0=head + 1~7=body

	struct Layer
	{
		int index = 0; // 0=head, 1~7=body

		// 判定用（XZだけ使う）
		DirectX::SimpleMath::Vector2 centerXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);

		// 表示用（Yは積み上げ、XZは centerXZ から作る想定）
		DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);

		// 円柱パラメータ（当たり判定に使う予定なら）
		float radius = 0.5f;
		float height = 1.0f;

		// 状態
		enum class State
		{
			Stable,   // 通常
			Falling,  // 倒壊演出中（偽物理）
			Removed   // 消滅（画面外など）
		};

		State state = State::Stable;

		// 倒壊演出用（偽物理）
		DirectX::SimpleMath::Vector2 fallDir = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
		float fallTimer = 0.0f;
	};


	//////////////////////////////////////////////////
	// Daruma 本体データ
	//////////////////////////////////////////////////
	std::vector<Layer> m_Layers;

	// 安定判定（仕様：中心点の平均が原点からズレ過ぎたら倒れる）
	DirectX::SimpleMath::Vector2 m_OriginXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
	float m_StableRadius = 0.35f;

	// state（GolfBall と同じ int 管理）
	// 0=Stable, 1=Warning(未使用でもOK), 2=Collapse
	int m_state = 0;

	// 倒壊決定に使う（最も偏離した層）
	int m_unstableLayer = -1;


	//////////////////////////////////////////////////
	// renderer関連
	//////////////////////////////////////////////////
	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer;

	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures;

	// カメラ参照
	Camera* m_Cam = nullptr;

	//////////////////////////////////////////////////
	// 内部処理用関数
	/////////////////////////////////////////////////

	void BuildLayers();

	// 範囲内階層の平均中心（0 ~ end）を計算する
	DirectX::SimpleMath::Vector2 CalcAverageCenter(int endIndex) const;

	// 安定判定（逐層：0 → 6）
	void EvaluateStability();

	// 崩し開始（0 ~ unstableLayer）
	void StartCollapse(int unstableLayer,
					const DirectX::SimpleMath::Vector2& avgCenter);

	// 更新
	void UpdateStable(Layer& layer);
	void UpdateFalling(Layer& layer);

public:

	//Daruma(Camera* cam);
	Daruma(); //カメラに依存しないように変更
	~Daruma();

	void Init() override;
	void Update() override;
	void Draw(Camera* cam) override;
	void Uninit() override;


	struct HitInfo
	{
		// 
		int targetLayer = 0;

		// 打つ方向（360 度，XZ 平面使用， normalize）
		DirectX::SimpleMath::Vector3 direction =
			DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f);

		// 力
		float power = 0.0f;

		// 命中層
		enum class VerticalZone
		{
			Upper,   // 自分と-1 → targetLayer - 1
			Middle,  // 自分だけ
			Lower    // 自分と+1 → targetLayer + 1
		};

		VerticalZone zone = VerticalZone::Middle;
	};

	void ApplyHit(const HitInfo& hit);


	//状態の取得・設定
	int GetState() const { return m_state; }
	void SetState(int state) { m_state = state; }

	void SetStableRadius(float r) { m_StableRadius = r; }
	float GetStableRadius() const { return m_StableRadius; }

	//void SetCamera(Camera* cam) { m_Cam = cam; }
	//void SetGround(Ground* ground);

};

