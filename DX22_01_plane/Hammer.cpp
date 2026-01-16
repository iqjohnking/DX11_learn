#include "Hammer.h"
#include "Input.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

#include <algorithm>
#include <cmath>

using namespace std;
using namespace DirectX::SimpleMath;

Hammer::Hammer()
{
	m_Cam = Game::GetInstance()->GetCamera();
}

Hammer::~Hammer()
{
}

void Hammer::Init()
{
	StaticMesh staticmesh;

	// Model
	u8string modelFile = u8"assets/model/hammer/uploads_files_1971948_Old_Hammer_OBJ.obj";
	string texDirectory = "assets/model/hammer";
	string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmpStr1, texDirectory);

	m_MeshRenderer.Init(staticmesh);
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	vector<MATERIAL> materials = staticmesh.GetMaterials();
	m_Materials.clear();
	for (int i = 0; i < (int)materials.size(); i++)
	{
		unique_ptr<Material> m = make_unique<Material>();
		m->Create(materials[i]);
		m_Materials.push_back(move(m));
	}

	// Scale
	m_Scale = Vector3(0.2f, 0.2f, 0.2f);

	// Initial pos
	SetPosition(0, 25, -50);

	// Orbit init
	m_FixedY = m_Position.y;
	m_OrbitCenter = Vector3::Zero;

	Vector3 rel = m_Position - m_OrbitCenter;
	rel.y = 0.0f;

	m_OrbitRadius = std::sqrt(rel.LengthSquared());
	if (m_OrbitRadius < 0.0001f)
	{
		m_OrbitRadius = 1.0f;
		rel = Vector3(0.0f, 0.0f, m_OrbitRadius);
	}
	m_OrbitTheta = std::atan2(rel.x, rel.z);

	// Aim lock init
	m_LockedY = m_Position.y;
	m_state = 0;
}

void Hammer::Update()
{
	
	if (m_Cam )
	{
		static constexpr float OrbitYawStepRad = 0.0314f;
		static constexpr float OrbitRadiusStep = 0.5f;
		static constexpr float OrbitHightStep = 0.5f;

		if (m_state == 0) {

		if (Input::GetKeyPress(VK_A)) m_OrbitTheta += OrbitYawStepRad;
		if (Input::GetKeyPress(VK_D)) m_OrbitTheta -= OrbitYawStepRad;

		if (Input::GetKeyPress(VK_W)) m_OrbitRadius -= OrbitRadiusStep;
		if (Input::GetKeyPress(VK_S)) m_OrbitRadius += OrbitRadiusStep;
		m_OrbitRadius = std::clamp(m_OrbitRadius, 6.0f, 20.0f);

		if (Input::GetKeyPress(VK_Q)) m_FixedY += OrbitHightStep;
		if (Input::GetKeyPress(VK_E)) m_FixedY -= OrbitHightStep;

		}
		// Orbit position (base)
		m_Position.x = m_OrbitCenter.x + std::sin(m_OrbitTheta) * m_OrbitRadius;
		m_Position.z = m_OrbitCenter.z + std::cos(m_OrbitTheta) * m_OrbitRadius;
		m_Position.y = m_FixedY;

		// Stage1 aim: state-based Y control
		if (m_state == 1) // aiming bob
		{
			m_AimBobPhase += 0.12f; // per-frame
			m_Position.y = m_FixedY + std::sinf(m_AimBobPhase) * m_AimBobAmp;
		}
		else if (m_state == 2) // locked
		{
			m_Position.y = m_LockedY;
		}
		else
		{
			m_Position.y = m_FixedY;
		}
	}

	// Face origin (yaw)
	{
		Vector3 toOrigin = Vector3::Zero - m_Position;
		toOrigin.y = 0.0f;

		if (toOrigin.LengthSquared() > 0.0001f)
		{
			toOrigin.Normalize();

			float targetYaw = atan2(toOrigin.x, toOrigin.z);
			float currentYaw = m_Rotation.y;

			float delta = targetYaw - currentYaw;
			while (delta > PI)  delta -= TWO_PI;
			while (delta < -PI) delta += TWO_PI;

			if (delta > turnSpeedPerFrame)  delta = turnSpeedPerFrame;
			if (delta < -turnSpeedPerFrame) delta = -turnSpeedPerFrame;

			currentYaw += delta;
			if (currentYaw > PI)  currentYaw -= TWO_PI;
			if (currentYaw < -PI) currentYaw += TWO_PI;

			m_Rotation.y = currentYaw;
			m_Rotation.z = PI / 2.0f;
		}
	}

	// Camera follow hammer
	if (m_Cam)
	{
		m_Cam->SetTarget(m_Position);
		m_Cam->SetTargetYaw(GetYaw());
		m_Cam->ResetBehindTarget();
	}
}

void Hammer::Draw(Camera* cam)
{
	cam->SetCamera();

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);

	// visual offset to align head
	float HeadOffset =  (4.87f - 0.45f) * (m_Scale.x * 10.0f);
	//float HeadOffset2 =  (1.f) * (m_Scale.x * 10.0f);
	Vector3 right(std::cos(m_Rotation.y), 0.0f, -std::sin(m_Rotation.y));
	//Vector3 up(std::cos(m_Rotation.z), 0.0f, -std::sin(m_Rotation.z));
	
	Vector3 drawPos = m_Position + (right * HeadOffset) ;

	Matrix t = Matrix::CreateTranslation(drawPos.x, drawPos.y, drawPos.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx);

	m_Shader.SetGPU();

	m_MeshRenderer.BeforeDraw();

	for (int i = 0; i < (int)m_subsets.size(); i++)
	{
		int matIdx = (int)m_subsets[i].MaterialIdx;
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

void Hammer::Uninit()
{
}

float Hammer::GetYaw() { return m_Rotation.y; }
float Hammer::GetRoll() { return m_Rotation.z; }
float Hammer::GetPitch() { return m_Rotation.x; }
