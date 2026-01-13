#pragma once

#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h" //文字列変換用


class Daruma : public Object
{
protected:
	// ===== cylinder.obj の実測サイズ（ユーザー確認：Y=1 / X=Z=2）=====
	// つまり：高さ=1、半径=1（直径=2）
	static constexpr float kModelHeight = 1.0f; // Y
	static constexpr float kModelRadius = 1.0f; // XZ の半径

	// ===== 調整用（Daruma専用）=====
	static constexpr float kDt = 1.0f / 60.0f;

	// 倒れ演出（擬似物理）
	static constexpr float FallSpeed = 1.2f;
	static constexpr float FallTranslatePerSec = 0.35f;
	static constexpr float FallDropPerSecBase = 0.6f;
	static constexpr float RemovedY = -50.0f;

	static constexpr int LAYER_COUNT = 8; // 0=head + 1~7=body

	struct Layer
	{
		int index = 0; // 0=head, 1~7=body

		// 判定用（XZだけ使う）
		DirectX::SimpleMath::Vector2 centerXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);

		// 表示用（Yは積み上げ、XZは centerXZ から作る想定）
		DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);

		// 円柱パラメータ
		float radius = 0.5f;
		float height = 1.0f;

		enum class State
		{
			Stable,
			Falling,
			Removed
		};

		State state = State::Stable;

		// Falling 用
		DirectX::SimpleMath::Vector2 fallDir = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
		float fallTimer = 0.0f;
	};

	//////////////////////////////////////////////////
	// Daruma 本体データ
	//////////////////////////////////////////////////
	std::vector<Layer> m_Layers;

	// 安定判定（平均中心が原点からズレたら倒れる）
	DirectX::SimpleMath::Vector2 m_OriginXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
	float m_StableRadius = 0.35f;

	// 0=Stable, 2=Collapse
	int m_state = 0;
	int m_unstableLayer = -1;

	//////////////////////////////////////////////////
	// renderer 関連
	//////////////////////////////////////////////////
	MeshRenderer m_MeshRenderer;

	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures;

	Camera* m_Cam = nullptr;

	//////////////////////////////////////////////////
	// 内部処理
	//////////////////////////////////////////////////
	void BuildLayers();
	DirectX::SimpleMath::Vector2 CalcAverageCenter(int endIndex) const;
	void EvaluateStability();
	void StartCollapse(int unstableLayer, const DirectX::SimpleMath::Vector2& avgCenter);

	void UpdateStable(Layer& layer);
	void UpdateFalling(Layer& layer);

public:
	Daruma();
	~Daruma();

	void Init() override;
	void Update() override;
	void Draw(Camera* cam) override;
	void Uninit() override;

	struct HitInfo
	{
		int targetLayer = 0;

		// 方向（XZ 使用、normalize）
		DirectX::SimpleMath::Vector3 direction =
			DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f);

		float power = 0.0f;

		enum class VerticalZone
		{
			Upper,   // targetLayer と targetLayer-1
			Middle,  // targetLayer のみ
			Lower    // targetLayer と targetLayer+1
		};

		VerticalZone zone = VerticalZone::Middle;
	};

	void ApplyHit(const HitInfo& hit);

	int GetState() const { return m_state; }
	void SetState(int state) { m_state = state; }

	void SetStableRadius(float r) { m_StableRadius = r; }
	float GetStableRadius() const { return m_StableRadius; }
};