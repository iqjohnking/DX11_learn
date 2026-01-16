#pragma once

#include "Object.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "Texture.h"
#include "Material.h"
#include "utility.h"

class Camera;

class Hammer : public Object
{
private:
    static constexpr float TWO_PI = 6.283185307f;
    static constexpr float PI = 3.1415926535f;

    float turnSpeedPerFrame = 0.314f;

    // Orbit params
    DirectX::SimpleMath::Vector3 m_OrbitCenter = DirectX::SimpleMath::Vector3::Zero;
    float m_FixedY = 0.0f;
    float m_OrbitRadius = 1.0f;
    float m_OrbitTheta = 0.0f;

    // Render
    MeshRenderer m_MeshRenderer;
    std::vector<std::unique_ptr<Material>> m_Materials;
    std::vector<SUBSET> m_subsets;
    std::vector<std::unique_ptr<Texture>> m_Textures;

    Camera* m_Cam = nullptr;

    // Stage flow state
    // 0 = idle/free, 1 = aiming (bob Y), 2 = locked Y
    int   m_state = 0;

    // Aiming bob / lock
    float m_AimBobAmp = 0.5f;   // small vertical motion
    float m_AimBobPhase = 0.0f;
    float m_LockedY = 0.0f;

public:
    Hammer();
    ~Hammer();

    void Init();
    void Update();
    void Draw(Camera* cam);
    void Uninit();

    int  GetState() const { return m_state; }
    void SetState(int state) { m_state = state; }

    void SetCamera(Camera* cam) { m_Cam = cam; }

    // Aim lock helpers
    void LockAimHeight() { m_LockedY = m_Position.y; }
    float GetHitY() const { return (m_state == 2) ? m_LockedY : m_Position.y; }

    float GetYaw();
    float GetRoll();
    float GetPitch();

    void SetPosition(float x, float y, float z)
    {
        m_Position = { x, y, z };
    }
    void SetPosition(DirectX::SimpleMath::Vector3 pos)
    {
        m_Position = pos;
    }
};