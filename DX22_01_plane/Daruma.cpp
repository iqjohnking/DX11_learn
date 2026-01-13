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

		// cylinder.obj は底ピボット（y=0）なので、中心に戻す
		static constexpr float kModelCenterY = 0.5f; // (minY+maxY)/2 = 0.5
		Matrix local = Matrix::CreateTranslation(0.0f, -kModelCenterY, 0.0f);

		// 実測：高さ=1、半径=1
		const float sy = layer.height;  // /1.0f
		const float sxz = layer.radius;  // /1.0f

		Matrix s = Matrix::CreateScale(sxz, sy, sxz);
		Matrix r = Matrix::Identity;
		Matrix t = Matrix::CreateTranslation(layer.pos);

		// 「先にモデルを中心へ」→「スケール」→「配置」
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

	Vector2 dir = (avgCenter - m_OriginXZ);
	if (dir.LengthSquared() <= 0.0001f) dir = Vector2(1.0f, 0.0f);
	else dir.Normalize();

	for (int i = 0; i <= unstableLayer && i < (int)m_Layers.size(); i++)
	{
		auto& layer = m_Layers[i];
		if (layer.state != Layer::State::Stable) continue;

		layer.state = Layer::State::Falling;
		layer.fallDir = dir;
		layer.fallTimer = 0.0f;
	}
}

void Daruma::UpdateStable(Layer& layer)
{
	layer.pos.x = layer.centerXZ.x;
	layer.pos.z = layer.centerXZ.y;
}

void Daruma::UpdateFalling(Layer& layer)
{
	layer.fallTimer += kDt * FallSpeed;

	layer.centerXZ += layer.fallDir * (FallTranslatePerSec * kDt);
	layer.pos.x = layer.centerXZ.x;
	layer.pos.z = layer.centerXZ.y;

	layer.pos.y -= (FallDropPerSecBase + (float)layer.index * 0.05f) * kDt;

	if (layer.pos.y < RemovedY)
	{
		layer.state = Layer::State::Removed;
	}
}