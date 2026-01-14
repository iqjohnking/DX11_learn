// Daruma.cpp
#include "Daruma.h"

using namespace DirectX::SimpleMath;

Daruma::Daruma() {}
Daruma::~Daruma() {}

void Daruma::Init()
{
	StaticMesh staticmesh;

	// 1) メッシュ読み込み（必ず Load が先）
	std::u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
	std::string texDirectory = "assets/model/cylinder";

	std::string tmpStr(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr, texDirectory);

	// 2) Renderer 初期化
	m_MeshRenderer.Init(staticmesh);

	// subsets / textures / materials
	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	const std::vector<MATERIAL> materials = staticmesh.GetMaterials();
	m_Materials.clear();
	m_Materials.reserve(materials.size());

	for (int i = 0; i < (int)materials.size(); i++)
	{
		auto m = std::make_unique<Material>();
		m->Create(materials[i]);
		m_Materials.push_back(std::move(m));
	}

	// 3) Daruma 本体初期化
	m_state = 0;
	m_unstableLayer = -1;
	m_OriginXZ = Vector2(0.0f, 0.0f);

	BuildLayers();
}

void Daruma::Update()
{
	for (auto& layer : m_Layers)
	{
		switch (layer.state)
		{
		case Layer::State::Stable:
			UpdateStable(layer);
			break;
		case Layer::State::Falling:
			UpdateFalling(layer);
			break;
		case Layer::State::Removed:
			break;
		}
	}

	if (m_state == 0)
	{
		EvaluateStability();
	}
}
void Daruma::Draw(Camera* cam)
{
	m_Cam = cam;
	if (!m_Cam) return;

	m_Cam->SetCamera();

	m_MeshRenderer.BeforeDraw();

	for (const auto& layer : m_Layers)
	{
		if (layer.state == Layer::State::Removed)
			continue;

		static constexpr float kModelCenterY = 0.5f;
		Matrix local = Matrix::CreateTranslation(0.0f, -kModelCenterY, 0.0f);

		const float sy = layer.height;
		const float sxz = layer.radius;

		Matrix s = Matrix::CreateScale(sxz, sy, sxz);

		Matrix r = Matrix::Identity;
		if (layer.state == Layer::State::Falling)
		{
			r = Matrix::CreateFromAxisAngle(layer.fallAxis, layer.fallAngle);
		}

		Matrix t = Matrix::CreateTranslation(layer.pos);

		Matrix world = local * s * r * t;
		Renderer::SetWorldMatrix(&world);

		for (int i = 0; i < (int)m_subsets.size(); i++)
		{
			const int matIdx = (int)m_subsets[i].MaterialIdx;

			if (0 <= matIdx && matIdx < (int)m_Materials.size())
				m_Materials[matIdx]->SetGPU();

			if (0 <= matIdx && matIdx < (int)m_Textures.size())
			{
				if (m_Materials[matIdx]->isTextureEnable())
					m_Textures[matIdx]->SetGPU();
			}

			m_MeshRenderer.DrawSubset(
				m_subsets[i].IndexNum,
				m_subsets[i].IndexBase,
				m_subsets[i].VertexBase);
		}
	}
}
void Daruma::Uninit() {}

void Daruma::ApplyHit(const HitInfo& hit)
{
	if (m_Layers.empty()) return;

	const int idx = std::clamp(hit.targetLayer, 0, (int)m_Layers.size() - 1);

	Vector2 dirXZ(hit.direction.x, hit.direction.z);
	if (dirXZ.LengthSquared() > 0.0f) dirXZ.Normalize();

	const float push = hit.power;

	auto Push = [&](int i, float scale)
		{
			if (i < 0 || i >= (int)m_Layers.size()) return;
			if (m_Layers[i].state != Layer::State::Stable) return;

			m_Layers[i].centerXZ += dirXZ * (push * scale);
		};

	switch (hit.zone)
	{
	case HitInfo::VerticalZone::Upper:
		Push(idx, 1.0f);
		Push(idx - 1, 0.6f);
		break;
	case HitInfo::VerticalZone::Middle:
		Push(idx, 1.0f);
		break;
	case HitInfo::VerticalZone::Lower:
		Push(idx, 1.0f);
		Push(idx + 1, 0.6f);
		break;
	}

	EvaluateStability();
}

Vector2 Daruma::CalcAverageCenter(int endIndex) const
{
	if (m_Layers.empty()) return Vector2::Zero;

	endIndex = std::clamp(endIndex, 0, (int)m_Layers.size() - 1);

	Vector2 sum(0.0f, 0.0f);
	int count = 0;

	for (int i = 0; i <= endIndex; i++)
	{
		if (m_Layers[i].state == Layer::State::Removed) continue;
		sum += m_Layers[i].centerXZ;
		count++;
	}

	return (count > 0) ? (sum / (float)count) : Vector2::Zero;
}


void Daruma::BuildLayers()
{
	m_Layers.clear();
	m_Layers.resize(LAYER_COUNT);

	const float headHeight = 5.0f;
	const float bodyHeight = 4.0f;

	const float topRadius = 5.0f;
	const float baseRadius = 5.0f;

	const float totalHeight = headHeight + bodyHeight * (LAYER_COUNT - 1);
	float yTop = totalHeight;

	for (int i = 0; i < LAYER_COUNT; i++)
	{
		auto& layer = m_Layers[i];
		layer.index = i;
		layer.state = Layer::State::Stable;

		layer.centerXZ = m_OriginXZ;

		layer.height = (i == 0) ? headHeight : bodyHeight;

		const float t = (LAYER_COUNT <= 1) ? 0.0f : (float)i / (float)(LAYER_COUNT - 1);
		layer.radius = std::lerp(topRadius, baseRadius, t);

		// 上から下へ積む（head が最上）
		yTop -= layer.height;
		const float halfY = layer.height * 0.5f;
		layer.pos = Vector3(layer.centerXZ.x, yTop + halfY, layer.centerXZ.y);
	}
}

void Daruma::EvaluateStability()
{
	m_unstableLayer = -1;

	if (m_Layers.size() < 2) return;

	const int lastCheck = (int)m_Layers.size() - 2;

	for (int i = 0; i <= lastCheck; i++)
	{
		const Vector2 avg = CalcAverageCenter(i);
		const float dist = (avg - m_OriginXZ).Length();

		if (dist > m_StableRadius)
		{
			m_unstableLayer = i;
			StartCollapse(i, avg);
			return;
		}
	}
}

void Daruma::StartCollapse(int unstableLayer, const Vector2& avgCenter)
{
	m_state = 2;

	// 誇張演出パラメータ
	static constexpr float kLaunchSpeedXZRand = 8.2f;	// もっと遠くに飛ばしたい
	static constexpr float kLaunchSpeedXZBase = 10.8f;	// もっと遠くに飛ばしたい
	static constexpr float kLaunchSpeedYBase = 2.8f;	// もっと高く跳ねさせたい
	static constexpr float kLaunchSpeedYRand = 2.2f;	// もっと高く跳ねさせたい

	static constexpr float kSpinBase = 6.0f; // rad/sec
	static constexpr float kSpinRand = 8.0f; // rad/sec

	// 乱数（簡易：フレーム依存でもテスト用途として十分）
	static uint32_t seed = 0x1234567u;
	auto Next01 = [&]()
		{
			seed = seed * 1664525u + 1013904223u;
			return (seed & 0x00FFFFFFu) / 16777215.0f; // [0,1]
		};

	const int last = min(unstableLayer, (int)m_Layers.size() - 1);

	for (int i = 0; i <= last; i++)
	{
		auto& layer = m_Layers[i];
		if (layer.state != Layer::State::Stable) continue;

		layer.state = Layer::State::Falling;
		layer.fallTimer = 0.0f;

		// XZ 平面のランダム方向
		const float a = Next01() * DirectX::XM_2PI;
		Vector3 dir(std::cos(a), 0.0f, std::sin(a));

		// ?物線初速
		const float spXZ = kLaunchSpeedXZBase + kLaunchSpeedXZRand * Next01();
		const float spY = kLaunchSpeedYBase + kLaunchSpeedYRand * Next01();
		layer.velocity = dir * spXZ;
		layer.velocity.y = spY;

		// 「飛び出し方向」を回転軸にする（要望）
		layer.fallAxis = dir;
		if (layer.fallAxis.LengthSquared() > 0.0001f) layer.fallAxis.Normalize();
		else layer.fallAxis = Vector3(0.0f, 0.0f, 1.0f);

		// 回転（ランダムに回す）
		layer.fallAngle = 0.0f;
		layer.angularVelocity = kSpinBase + kSpinRand * Next01();
	}
}

void Daruma::UpdateStable(Layer& layer)
{
	layer.pos.x = layer.centerXZ.x;
	layer.pos.z = layer.centerXZ.y;
}

void Daruma::UpdateFalling(Layer& layer)
{
	// 誇張演出用の重力（units/sec^2）
	static constexpr float kGravity = 9.8f * 0.6f;

	// 角度更新（回転）
	layer.fallAngle += layer.angularVelocity * kDt;

	// ?物線（速度→位置）
	layer.velocity.y -= kGravity * kDt;
	layer.pos += layer.velocity * kDt;

	// 画面外に落ちたら消す
	if (layer.pos.y < RemovedY)
	{
		layer.state = Layer::State::Removed;
	}
}