
#pragma once

#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h"

class Daruma : public Object
{
protected:
	static constexpr float kDt = 1.0f / 60.0f;

	static constexpr float RemovedY = -50.0f;
	static constexpr int   LAYER_COUNT = 8; // 0=head + 1~7=body

	struct Layer
	{
		int index = 0;

		DirectX::SimpleMath::Vector2 centerXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
		DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);

		float radius = 0.5f;
		float height = 1.0f;

		enum class State
		{
			Stable,
			Falling,
			Restacking,
			Removed
		};

		State state = State::Stable;

		// Falling
		float fallTimer = 0.0f;

		DirectX::SimpleMath::Vector2 fallDir = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
		float fallAngle = 0.0f;
		DirectX::SimpleMath::Vector3 fallAxis = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);

		DirectX::SimpleMath::Vector3 velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
		float angularVelocity = 0.0f;

		float targetY = 0.0f;
	};

	std::vector<Layer> m_Layers;

	DirectX::SimpleMath::Vector2 m_OriginXZ = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
	float m_StableRadius = 0.8f; // slightly larger than 1.0f

	int m_state = 0;
	int m_unstableLayer = -1;

	bool m_restackActive = false;
	int  m_flyingLayerIndex = -1;

	MeshRenderer m_MeshRenderer;

	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures;
	std::vector<std::unique_ptr<Texture>> m_BodyTextures; // NEW: body textures
	std::vector<std::unique_ptr<Texture>> m_FootTextures; // NEW: body textures


	Camera* m_Cam = nullptr;

protected:
	void BuildLayers();
	DirectX::SimpleMath::Vector2 CalcAverageCenter(int endIndex) const;
	void EvaluateStability();
	void StartCollapse(int unstableLayer, const DirectX::SimpleMath::Vector2& avgCenter);

	void UpdateStable(Layer& layer);
	void UpdateFalling(Layer& layer);

	void StartLaunch(int layerIndex, const DirectX::SimpleMath::Vector2& dirXZ, float power);
	bool IsSeparatedFromAll(int layerIndex) const;

	void StartRestack(int flyingIndex);
	void UpdateRestacking(Layer& layer);
	bool IsAllSettled() const;

	void RebuildTargetYs(int excludeIndex = -1);

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
		DirectX::SimpleMath::Vector3 direction = DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f);
		float power = 0.0f;

		enum class VerticalZone
		{
			Upper,
			Middle,
			Lower
		};

		VerticalZone zone = VerticalZone::Middle;
	};

	void ApplyHit(const HitInfo& hit);

	// pick which layer is hit by world Y
	int PickLayerByY(float worldY) const;

	int GetState() const { return m_state; }
	void SetState(int state) { m_state = state; }

	void SetStableRadius(float r) { m_StableRadius = r; }
	float GetStableRadius() const { return m_StableRadius; }


	// game-end and helpers
	void ForceWin_ThrowAllButHead(const DirectX::SimpleMath::Vector3& dirXZ, float power = 100.0f);
	bool IsHeadRemoved() const;
	bool IsAllBodyRemoved() const;
};
