#include "Daruma.h"

#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

Daruma::Daruma() {}
Daruma::~Daruma() {}

static bool OverlapAfterMove(
    const Vector2& a0, float ra, const Vector2& da,
    const Vector2& b0, float rb, const Vector2& db)
{
    const Vector2 a1 = a0 + da;
    const Vector2 b1 = b0 + db;
    const float limit = ra + rb;
    return (a1 - b1).LengthSquared() <= limit * limit;
}

static bool OverlapNowXZ(const Vector2& a, float ra, const Vector2& b, float rb)
{
    const float limit = ra + rb;
    return (a - b).LengthSquared() <= limit * limit;
}

void Daruma::Init()
{
    StaticMesh staticmesh;

    std::u8string modelFile = u8"assets/model/cylinder/cylinder.obj";
    std::string texDirectory = "assets/model/cylinder";
    std::string bodyTexDirectory = "assets/model/cylinder/body";
    std::string footTexDirectory = "assets/model/cylinder/foot";

    std::string tmpStr(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    staticmesh.Load(tmpStr, texDirectory);

    // body用テクスチャだけ別ディレクトリでロード（同じモデルを読む想定）
    StaticMesh bodymesh;
    bodymesh.Load(tmpStr, bodyTexDirectory);

    // foot用テクスチャだけ別ディレクトリでロード（同じモデルを読む想定）
    StaticMesh footmesh;
    footmesh.Load(tmpStr, footTexDirectory);

    m_MeshRenderer.Init(staticmesh);

    m_subsets = staticmesh.GetSubsets();
    m_Textures = staticmesh.GetTextures();
    m_BodyTextures = bodymesh.GetTextures();
    m_FootTextures = footmesh.GetTextures();

    const std::vector<MATERIAL> materials = staticmesh.GetMaterials();
    m_Materials.clear();
    m_Materials.reserve(materials.size());
    for (int i = 0; i < (int)materials.size(); i++)
    {
        auto m = std::make_unique<Material>();
        m->Create(materials[i]);
        m_Materials.push_back(std::move(m));
    }

    m_state = 0;
    m_unstableLayer = -1;
    m_OriginXZ = Vector2(0.0f, 0.0f);

    m_restackActive = false;
    m_flyingLayerIndex = -1;

    BuildLayers();
    RebuildTargetYs(-1);
}

void Daruma::Update()
{
    for (auto& layer : m_Layers)
    {
        switch (layer.state)
        {
        case Layer::State::Stable:     UpdateStable(layer);     break;
        case Layer::State::Falling:    UpdateFalling(layer);    break;
        case Layer::State::Restacking: UpdateRestacking(layer); break;
        case Layer::State::Removed:    break;
        }
    }

    // When the launched layer is detached (no contact with both neighbors), start restack
    if (!m_restackActive && m_flyingLayerIndex >= 0)
    {
        if (IsSeparatedFromAll(m_flyingLayerIndex))
            StartRestack(m_flyingLayerIndex);
    }

    // Restack finished -> evaluate stability once
    if (m_restackActive && IsAllSettled())
    {
        m_restackActive = false;
        m_flyingLayerIndex = -1;

        m_state = 0;
        EvaluateStability();
        return;
    }

    // Normal stability check
    if (m_state == 0 && !m_restackActive && m_flyingLayerIndex < 0)
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
            r = Matrix::CreateFromAxisAngle(layer.fallAxis, layer.fallAngle);

        Matrix t = Matrix::CreateTranslation(layer.pos);

        Matrix world = local * s * r * t;
        Renderer::SetWorldMatrix(&world);

        const bool isHead = (layer.index == 0);
        const bool isFoot = (layer.index == ((int)m_Layers.size() - 1));

        auto& texturesToUse = isFoot
            ? (m_FootTextures.empty() ? m_Textures : m_FootTextures)
            : ((isHead || m_BodyTextures.empty()) ? m_Textures : m_BodyTextures);

        for (int i = 0; i < (int)m_subsets.size(); i++)
        {
            const int matIdx = (int)m_subsets[i].MaterialIdx;

            if (0 <= matIdx && matIdx < (int)m_Materials.size())
                m_Materials[matIdx]->SetGPU();

            if (0 <= matIdx && matIdx < (int)texturesToUse.size())
            {
                if (m_Materials[matIdx]->isTextureEnable())
                    texturesToUse[matIdx]->SetGPU();
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

    // 只允許打 Stable 的層（不然會亂）
    if (m_Layers[idx].state != Layer::State::Stable) return;

    // dir XZ
    Vector2 dirXZ(hit.direction.x, hit.direction.z);
    if (dirXZ.LengthSquared() > 1e-6f) dirXZ.Normalize();
    else dirXZ = Vector2(1.0f, 0.0f);

    // 決定 kPushPerPower=1：power 直接當位移量
    const float push = hit.power;
    const Vector2 dIdx = dirXZ * push;

    // 上下検察
    const Vector2 dUp = Vector2::Zero;
    const Vector2 dDown = Vector2::Zero;

    bool contactUpper = false, contactLower = false;

    if (idx - 1 >= 0 && m_Layers[idx - 1].state == Layer::State::Stable)
    {
        contactUpper = OverlapAfterMove(
            m_Layers[idx].centerXZ, m_Layers[idx].radius, dIdx,
            m_Layers[idx - 1].centerXZ, m_Layers[idx - 1].radius, dUp);
    }

    // 下が「床」なら判定しない（最下段は下側を無視）
    const bool hasLowerLayer = (idx + 1 < (int)m_Layers.size());
    if (hasLowerLayer && m_Layers[idx + 1].state == Layer::State::Stable)
    {
        contactLower = OverlapAfterMove(
            m_Layers[idx].centerXZ, m_Layers[idx].radius, dIdx,
            m_Layers[idx + 1].centerXZ, m_Layers[idx + 1].radius, dDown);
    }

    // detached 判定は「存在する近傍」のみで評価する
    // 最下段は下側を無視するので、上と接触していなければ detached になる
    const bool detached = (!contactUpper && (!hasLowerLayer || !contactLower));

    static constexpr float kMinLaunchPower = 15.0f;
    if (detached && hit.power >= kMinLaunchPower)
    {
        StartLaunch(idx, dirXZ, hit.power);
        return;
    }

    // 沒分離或力量不足：只推歪
    m_Layers[idx].centerXZ += dIdx;

    m_Layers[idx].pos.x = m_Layers[idx].centerXZ.x;
    m_Layers[idx].pos.z = m_Layers[idx].centerXZ.y;

    if (!m_restackActive && m_flyingLayerIndex < 0)
        EvaluateStability();
}

void Daruma::StartLaunch(int layerIndex, const Vector2& dirXZ, float power)
{
    if (layerIndex < 0 || layerIndex >= (int)m_Layers.size()) return;

    auto& layer = m_Layers[layerIndex];
    if (layer.state != Layer::State::Stable) return;

    static constexpr float kMaxSpeedXZ = 30.0f;      // was 18.0f
    static constexpr float kMinSpeedXZ = 10.0f;      // NEW: ensure it looks launched
    static constexpr float kSpeedPerPower = 0.65f;   // was 0.35f
    static constexpr float kMinLaunchPower = 15.0f;  // 0~100 尺度，低於這個不打飛
    static constexpr float kLaunchYPerPower = 0.05f; // power=100 -> 5.0
    static constexpr float kMaxLaunchY = 6.0f;

    // 保險：力量不足不進 Falling
    if (power < kMinLaunchPower)
        return;

    float spXZ = std::clamp(power * kSpeedPerPower, 0.0f, kMaxSpeedXZ);
    spXZ = max(spXZ, kMinSpeedXZ);

    float launchY = std::clamp(power * kLaunchYPerPower, 0.0f, kMaxLaunchY);

    layer.state = Layer::State::Falling;
    layer.velocity = Vector3(dirXZ.x * spXZ, launchY, dirXZ.y * spXZ);

    layer.fallAxis = Vector3(dirXZ.x, 0.0f, dirXZ.y);
    if (layer.fallAxis.LengthSquared() > 0.0001f) layer.fallAxis.Normalize();
    else layer.fallAxis = Vector3(0.0f, 0.0f, 1.0f);

    layer.fallAngle = 0.0f;
    layer.angularVelocity = 0.0f; // could randomize for more spin

    m_flyingLayerIndex = layerIndex;

    // pause stability while flying/restacking
    m_state = 1;
}


bool Daruma::IsSeparatedFromAll(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= (int)m_Layers.size()) return false;

    const Layer& a = m_Layers[layerIndex];
    if (a.state == Layer::State::Removed) return true;

    auto IsContactPair = [&](int otherIndex) -> bool
        {
            if (otherIndex < 0 || otherIndex >= (int)m_Layers.size()) return false;

            const Layer& b = m_Layers[otherIndex];
            if (b.state == Layer::State::Removed) return false;

            const float limit = a.radius + b.radius;
            return (a.centerXZ - b.centerXZ).LengthSquared() <= limit * limit;
        };

    const bool contactUpper = IsContactPair(layerIndex - 1);

    // 最下段は下側を「床」として無視
    const bool hasLowerLayer = (layerIndex + 1 < (int)m_Layers.size());
    const bool contactLower = hasLowerLayer ? IsContactPair(layerIndex + 1) : false;

    return (!contactUpper && (!hasLowerLayer || !contactLower));
}

void Daruma::StartRestack(int flyingIndex)
{
    m_restackActive = true;

    // IMPORTANT: exclude the flying layer so others actually drop to fill the gap
    RebuildTargetYs(flyingIndex);

    for (int i = 0; i < (int)m_Layers.size(); i++)
    {
        auto& layer = m_Layers[i];
        if (layer.state == Layer::State::Removed) continue;
        if (i == flyingIndex) continue;

        layer.state = Layer::State::Restacking;
        layer.velocity.y = 0.0f;
    }
}

void Daruma::RebuildTargetYs(int excludeIndex)
{
    const float headHeight = 5.0f;
    const float bodyHeight = 4.0f;

    int alive = 0;
    for (int i = 0; i < (int)m_Layers.size(); i++)
    {
        if (m_Layers[i].state == Layer::State::Removed) continue;
        if (i == excludeIndex) continue;
        alive++;
    }
    if (alive <= 0) return;

    const float totalHeight = headHeight + bodyHeight * (alive - 1);
    float yTop = totalHeight;

    for (int i = 0; i < (int)m_Layers.size(); i++)
    {
        auto& layer = m_Layers[i];
        if (layer.state == Layer::State::Removed) continue;
        if (i == excludeIndex) continue;

        const float h = (layer.index == 0) ? headHeight : bodyHeight;
        yTop -= h;
        layer.targetY = yTop + h * 0.5f;
    }
}

void Daruma::UpdateRestacking(Layer& layer)
{
    static constexpr float kGravity = 9.8f * 5.f;

    layer.velocity.y -= kGravity * kDt;
    layer.pos.y += layer.velocity.y * kDt;

    if (layer.pos.y <= layer.targetY)
    {
        layer.pos.y = layer.targetY;
        layer.velocity.y = 0.0f;
        layer.state = Layer::State::Stable;

        layer.pos.x = layer.centerXZ.x;
        layer.pos.z = layer.centerXZ.y;
    }
}

bool Daruma::IsAllSettled() const
{
    for (int i = 0; i < (int)m_Layers.size(); i++)
    {
        if (i == m_flyingLayerIndex) continue; // IMPORTANT: ignore flying layer
        const auto& layer = m_Layers[i];
        if (layer.state == Layer::State::Removed) continue;
        if (layer.state != Layer::State::Stable) return false;
    }
    return true;
}

Vector2 Daruma::CalcAverageCenter(int endIndex) const
{
    if (m_Layers.empty()) return Vector2::Zero;

    endIndex = std::clamp(endIndex, 0, (int)m_Layers.size() - 1);

    Vector2 sum(0.0f, 0.0f);
    int count = 0;

    for (int i = 0; i <= endIndex; i++)
    {
        if (m_Layers[i].state != Layer::State::Stable) continue;

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

    const float topRadius = 5.0f; // you chose ~5 scale
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

        yTop -= layer.height;
        const float halfY = layer.height * 0.5f;
        layer.pos = Vector3(layer.centerXZ.x, yTop + halfY, layer.centerXZ.y);

        layer.targetY = layer.pos.y;
        layer.velocity = Vector3::Zero;
        layer.fallAngle = 0.0f;
        layer.angularVelocity = 0.0f;
    }
}

void Daruma::EvaluateStability()
{
    m_unstableLayer = -1;
    if (m_Layers.size() < 2) return;

    const int lastCheck = (int)m_Layers.size() - 2;

    // If any gap exists, skip stability check (restack design)
    for (int i = 0; i <= lastCheck; i++)
    {
        const Layer& upper = m_Layers[i];
        const Layer& lower = m_Layers[i + 1];

        if (upper.state == Layer::State::Removed) continue;
        if (lower.state == Layer::State::Removed) continue;

        const float d = (upper.centerXZ - lower.centerXZ).Length();
        const float overlapLimit = upper.radius + lower.radius;

        if (d > overlapLimit)
            return;
    }

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

    static constexpr float kLaunchSpeedXZRand = 8.2f;
    static constexpr float kLaunchSpeedXZBase = 10.8f;
    static constexpr float kLaunchSpeedYBase = 2.8f;
    static constexpr float kLaunchSpeedYRand = 2.2f;

    static constexpr float kSpinBase = 6.0f;
    static constexpr float kSpinRand = 8.0f;

    static uint32_t seed = 0x1234567u;
    auto Next01 = [&]()
        {
            seed = seed * 1664525u + 1013904223u;
            return (seed & 0x00FFFFFFu) / 16777215.0f;
        };

    const int last = min(unstableLayer, (int)m_Layers.size() - 1);

    for (int i = 0; i <= last; i++)
    {
        auto& layer = m_Layers[i];
        if (layer.state != Layer::State::Stable) continue;

        layer.state = Layer::State::Falling;
        layer.fallTimer = 0.0f;

        const float a = Next01() * DirectX::XM_2PI;
        Vector3 dir(std::cos(a), 0.0f, std::sin(a));

        const float spXZ = kLaunchSpeedXZBase + kLaunchSpeedXZRand * Next01();
        const float spY = kLaunchSpeedYBase + kLaunchSpeedYRand * Next01();
        layer.velocity = dir * spXZ;
        layer.velocity.y = spY;

        layer.fallAxis = dir;
        if (layer.fallAxis.LengthSquared() > 0.0001f) layer.fallAxis.Normalize();
        else layer.fallAxis = Vector3(0.0f, 0.0f, 1.0f);

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
    static constexpr float kGravity = 9.8f * 5.f;

    // Falling中に上下いずれかと「接触（XZ円重なり）」しているなら、Y方向は落とさない
    // （接触拘束がないことによる穿模対策）
    auto HasAnyNeighborContactXZ = [&]() -> bool
        {
            // 現在posからcenterXZを更新しているが、保険としてpos基準でも見る
            const Vector2 selfXZ(layer.centerXZ.x, layer.centerXZ.y);

            // 上
            if (m_flyingLayerIndex >= 0 && &layer == &m_Layers[m_flyingLayerIndex])
            {
                // no-op: flying layer is a specific index; but this lambda is per-layer
            }

            // 近傍は index で取る（layer.index はセットされている）
            const int i = layer.index;

            // 上
            if (i - 1 >= 0)
            {
                const auto& up = m_Layers[i - 1];
                if (up.state != Layer::State::Removed)
                {
                    if (OverlapNowXZ(selfXZ, layer.radius, up.centerXZ, up.radius))
                        return true;
                }
            }

            // 下
            if (i + 1 < (int)m_Layers.size())
            {
                const auto& down = m_Layers[i + 1];
                if (down.state != Layer::State::Removed)
                {
                    if (OverlapNowXZ(selfXZ, layer.radius, down.centerXZ, down.radius))
                        return true;
                }
            }

            return false;
        };

    layer.fallAngle += layer.angularVelocity * kDt;

    if (HasAnyNeighborContactXZ())
    {
        // 接触中は落下禁止（Y速度を殺す）
        layer.velocity.y = max(layer.velocity.y, 0.0f);
        // 重力を掛けず、Y位置も維持
        // （X/Zは飛行速度で動かしてOK）
        Vector3 v = layer.velocity;
        v.y = 0.0f;
        layer.pos += v * kDt;
    }
    else
    {
        // 分離して初めて自由落下
        layer.velocity.y -= kGravity * kDt;
        layer.pos += layer.velocity * kDt;
    }

    layer.centerXZ.x = layer.pos.x;
    layer.centerXZ.y = layer.pos.z;

    if (layer.pos.y < RemovedY)
        layer.state = Layer::State::Removed;
}

int Daruma::PickLayerByY(float worldY) const
{
    int best = -1;
    float bestDist = 1e30f;

    for (int i = 0; i < (int)m_Layers.size(); i++)
    {
        const auto& layer = m_Layers[i];
        if (layer.state == Layer::State::Removed) continue;

        const float half = layer.height * 0.5f;
        const float y0 = layer.pos.y - half;
        const float y1 = layer.pos.y + half;

        if (worldY >= y0 && worldY <= y1)
            return i;

        const float d = std::fabs(worldY - layer.pos.y);
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    return best;
}


// about game over

void Daruma::ForceWin_ThrowAllButHead(const Vector3& dirXZ, float power)
{
    // 勝利演出：頭(0)以外を全部「打飛」(Falling開始)させる
    // 方向はXZのみ使用（YはStartLaunch内で付与）
    Vector2 d(dirXZ.x, dirXZ.z);
    if (d.LengthSquared() > 1e-6f) d.Normalize();
    else d = Vector2(1.0f, 0.0f);

    for (int i = 1; i < (int)m_Layers.size(); i++)
    {
        if (m_Layers[i].state == Layer::State::Removed) continue;

        // Stable 以外も強制的に飛ばしたいので、最低限 Stable に寄せる
        // （StartLaunchはStableしか受けない実装のため）
        if (m_Layers[i].state != Layer::State::Stable)
        {
            m_Layers[i].state = Layer::State::Stable;
            m_Layers[i].velocity = Vector3::Zero;
            m_Layers[i].fallAngle = 0.0f;
            m_Layers[i].angularVelocity = 0.0f;
        }

        StartLaunch(i, d, max(power, 15.0f));
    }
}

bool Daruma::IsHeadRemoved() const
{
    if (m_Layers.empty()) return true;
    return (m_Layers[0].state == Layer::State::Removed);
}

bool Daruma::IsAllBodyRemoved() const
{
    if (m_Layers.size() <= 1) return true;

    for (int i = 1; i < (int)m_Layers.size(); i++)
    {
        if (m_Layers[i].state != Layer::State::Removed)
            return false;
    }
    return true;
}