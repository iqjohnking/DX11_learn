#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Hammer.h"
#include "Skybox.h"
#include "Ground.h"
#include "Kotatu.h"
#include "Daruma.h"
#include "Arrow.h"
#include "Pole.h"
#include "Texture2D.h"

#include <cmath>

using namespace DirectX::SimpleMath;

Stage1Scene::Stage1Scene()
{
    Init();
}

Stage1Scene::~Stage1Scene()
{
    Uninit();
}

int Stage1Scene::GetScore() const
{
    return 0;
}

void Stage1Scene::Init()
{
    m_Par = 6;
    m_StrokeCount = 0;
    m_State = 0;

    m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Hammer>()); // 0
    m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Arrow>());  // 1
    m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Daruma>()); // 2
    m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Kotatu>()); // 3
    m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<SkyBox>()); // 4

    // UI back
    Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>(); // 5
    pt1->SetTexture("assets/texture/ui_back.png");
    pt1->SetPosition(-475.0, -300.0f, 0.0f);
    pt1->SetScale(270.0f, 75.0f, 0.0f);
    m_MySceneObjects.emplace_back(pt1);

    // UI strings
    Texture2D* pt2 = Game::GetInstance()->AddObject<Texture2D>(); // 6
    pt2->SetTexture("assets/texture/ui_string.png");
    pt2->SetPosition(-575.0, -245.0f, 0.0f);
    pt2->SetScale(60.0f, 45.0f, 0.0f);
    pt2->SetUV(1, 1, 2, 1);
    m_MySceneObjects.emplace_back(pt2);

    Texture2D* pt3 = Game::GetInstance()->AddObject<Texture2D>(); // 7
    pt3->SetTexture("assets/texture/ui_string.png");
    pt3->SetPosition(-400.0, -305.0f, 0.0f);
    pt3->SetScale(105.0f, 63.0f, 0.0f);
    pt3->SetUV(2, 1, 2, 1);
    m_MySceneObjects.emplace_back(pt3);

    // Par number
    m_UiParNumber = Game::GetInstance()->AddObject<Texture2D>(); // 8
    m_UiParNumber->SetTexture("assets/texture/number.png");
    m_UiParNumber->SetPosition(-510.0, -245.0f, 0.0f);
    m_UiParNumber->SetScale(65.0f, 45.0f, 0.0f);
    m_UiParNumber->SetUV((float)(m_Par + 1), 1, 10, 1);
    m_MySceneObjects.emplace_back(m_UiParNumber);

    // Stroke ones
    m_UiStrokeOnes = Game::GetInstance()->AddObject<Texture2D>(); // 9
    m_UiStrokeOnes->SetTexture("assets/texture/number.png");
    m_UiStrokeOnes->SetPosition(-485.0, -300.0f, 0.0f);
    m_UiStrokeOnes->SetScale(95.0f, 72.0f, 0.0f);
    m_UiStrokeOnes->SetUV(2, 1, 10, 1);
    m_MySceneObjects.emplace_back(m_UiStrokeOnes);

    // Stroke tens
    m_UiStrokeTens = Game::GetInstance()->AddObject<Texture2D>(); // 10
    m_UiStrokeTens->SetTexture("assets/texture/number.png");
    m_UiStrokeTens->SetPosition(-556.0, -300.0f, 0.0f);
    m_UiStrokeTens->SetScale(95.0f, 72.0f, 0.0f);
    m_UiStrokeTens->SetUV(1, 1, 10, 1);
    m_MySceneObjects.emplace_back(m_UiStrokeTens);

    // Init states
    if (auto* hammer = dynamic_cast<Hammer*>(m_MySceneObjects[0]))
        hammer->SetState(0);

    if (auto* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[1]))
        arrow->SetState(0);

    // power/pending init
    m_HasPendingHit = false;
    m_Power = 0.0f;
    m_PowerVel = 0.0f;
}

void Stage1Scene::Uninit()
{
    for (auto& o : m_MySceneObjects) {
        Game::GetInstance()->DeleteObject(o);
    }
    m_MySceneObjects.clear();
}

void Stage1Scene::Update()
{
    Camera* cam = Game::GetInstance()->GetCamera();
    auto* hammer = dynamic_cast<Hammer*>(m_MySceneObjects[0]);
    auto* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[1]);
    auto* daruma = dynamic_cast<Daruma*>(m_MySceneObjects[2]);

    if (!cam || !hammer || !daruma) return;

    auto CalcCamDirXZ = [&](Camera* c) -> Vector3
        {
            Vector3 d = Vector3::Zero - c->GetPosition(); // daruma is around origin
            d.y = 0.0f;
            if (d.LengthSquared() < 1e-6f) d = Vector3(1, 0, 0);
            d.Normalize();
            return d;
        };


    // ---------- end judge / timer / scene switch ----------
  // Lose条件：daruma が「崩壊中」(state==2) OR 頭が飛んで消えた
  // Win条件：頭以外が全部 Removed
    if (m_EndState == EndState::None)
    {
        if (daruma->GetState() == 2 || daruma->IsHeadRemoved())
        {
            m_EndState = EndState::Lose;
            m_EndTimer = 0.0f;
        }
        else if (daruma->IsAllBodyRemoved())
        {
            m_EndState = EndState::Win;
            m_EndTimer = 0.0f;

            // 勝利演出：頭以外を全部打ち飛ばす（既に消えているはずだが、確実に演出する）
            daruma->ForceWin_ThrowAllButHead(CalcCamDirXZ(cam), 100.0f);
        }
    }
    else
    {
        // 60FPS固定なので固定dt
        m_EndTimer += (1.0f / 60.0f);

        if (m_EndTimer >= kEndWait)
        {
            // 次のシーンへ（現状は RESULT のみなのでここへ遷移）
            Game::GetInstance()->ChangeScene(RESULT);
            return;
        }
    }

    // end中は操作を止める（カメラ追従と描画は継続）
    if (m_EndState != EndState::None)
        return;
    // -----------------------------------------------------------


    Vector3 dirNow = CalcCamDirXZ(cam); //
    if (arrow && hammer)
    {
        Vector3 p = hammer->GetPosition();
        p.y += 0.2f;
        arrow->SetPosition(p);

        // 
        arrow->SetDirectionXZ(dirNow);
    }

    switch (m_State)
    {
    case 0: // Idle
        if (Input::GetKeyTrigger(VK_SPACE))
        {
            m_StrokeCount++;

            Texture2D* count[2] = { m_UiStrokeOnes, m_UiStrokeTens };
            for (int i = 0; i < 2; i++)
            {
                int cnt = m_StrokeCount % (int)pow(10, i + 1) / (int)pow(10, i);
                if (count[i])
                    count[i]->SetUV((float)(cnt + 1), 1, 10, 1);
            }

            // Enter aiming
            m_State = 1;
            hammer->SetState(1); // Hammer bobbing Y
            m_HasPendingHit = false;
        }
        break;

    case 1: // Decide height (Hammer animates bobbing; SPACE locks)
        if (Input::GetKeyTrigger(VK_SPACE))
        {
            hammer->LockAimHeight();
            hammer->SetState(2);

            const float hitY = hammer->GetHitY();
            const int target = daruma->PickLayerByY(hitY);

            if (target >= 0)
            {
                m_PendingHit = Daruma::HitInfo{};
                m_PendingHit.targetLayer = target;
                m_PendingHit.zone = Daruma::HitInfo::VerticalZone::Middle;
                m_PendingHit.direction = CalcCamDirXZ(cam);

                m_HasPendingHit = true;

                // Enter power select
                m_State = 2;
                m_Power = 0.0f;
                m_PowerVel = 0.0f;
            }
            else
            {
                // fallback
                m_State = 0;
                hammer->SetState(0);
                m_HasPendingHit = false;
            }
        }
        break;

    case 2: // Power select (0..10 accel loop) -> SPACE apply
        // power update
        m_PowerVel += m_PowerAccelStep; // +0.1 +0.2 +0.3...
        m_Power += m_PowerVel;

        if (m_Power >= m_PowerMax)
        {
            m_Power = 0.0f;
            m_PowerVel = 0.0f;
        }

        if (arrow)
        {
            arrow->SetState(2); // 顯示力量?態
            arrow->SetPower01(m_Power / m_PowerMax);
        }

        if (Input::GetKeyTrigger(VK_SPACE))
        {
            m_State = 0;
            hammer->SetState(0);

            if (m_HasPendingHit)
            {
                // UI is 0..10, convert to practical 0..100 for daruma
                m_PendingHit.power = m_Power * 10.0f;

                daruma->ApplyHit(m_PendingHit);
                m_HasPendingHit = false;
            }
        }

        // VK_H: test big power
        if (Input::GetKeyTrigger(VK_H))
        {
            m_State = 0;
            hammer->SetState(0);

            if (m_HasPendingHit)
            {
                auto testHit = m_PendingHit;
                testHit.power = 100.0f;
                daruma->ApplyHit(testHit);
                m_HasPendingHit = false;
            }
        }
        break;
    }
}
