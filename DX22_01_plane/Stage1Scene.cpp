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

using namespace DirectX::SimpleMath;

Stage1Scene::Stage1Scene()
{
	Init();
}

Stage1Scene::~Stage1Scene()
{
	Uninit();
}

void Stage1Scene::Init()
{
	// パーと打目数初期化
	m_Par = 6;
	m_StrokeCount = 0;

	Camera* cam = Game::GetInstance()->GetCamera();

	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<GolfBall>(cam));	//0
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Kotatu>());		//1
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Arrow>());			//2
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Pole>());			//3
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Hammer>());		//4
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<SkyBox>());		//5

	// Daruma（追加）
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Daruma>());		//6


	// ui_back
	Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>();
	pt1->SetTexture("assets/texture/ui_back.png");
	pt1->SetPosition(-475.0, -300.0f, 0.0f);
	pt1->SetScale(270.0f, 75.0f, 0.0f);
	m_MySceneObjects.emplace_back(pt1);

	// パー、文字列表示
	Texture2D* pt2 = Game::GetInstance()->AddObject<Texture2D>();
	pt2->SetTexture("assets/texture/ui_string.png");
	pt2->SetPosition(-575.0, -245.0f, 0.0f);
	pt2->SetScale(60.0f, 45.0f, 0.0f);
	pt2->SetUV(1, 1, 2, 1);
	m_MySceneObjects.emplace_back(pt2);

	// 打目文字列表示
	Texture2D* pt3 = Game::GetInstance()->AddObject<Texture2D>();
	pt3->SetTexture("assets/texture/ui_string.png");
	pt3->SetPosition(-400.0, -305.0f, 0.0f);
	pt3->SetScale(105.0f, 63.0f, 0.0f);
	pt3->SetUV(2, 1, 2, 1);
	m_MySceneObjects.emplace_back(pt3);

	// パー数表示（参照を保存）
	m_UiParNumber = Game::GetInstance()->AddObject<Texture2D>();
	m_UiParNumber->SetTexture("assets/texture/number.png");
	m_UiParNumber->SetPosition(-510.0, -245.0f, 0.0f);
	m_UiParNumber->SetScale(65.0f, 45.0f, 0.0f);
	m_UiParNumber->SetUV((float)(m_Par + 1), 1, 10, 1);
	m_MySceneObjects.emplace_back(m_UiParNumber);

	// 打目数表示 1の位（参照を保存）
	m_UiStrokeOnes = Game::GetInstance()->AddObject<Texture2D>();
	m_UiStrokeOnes->SetTexture("assets/texture/number.png");
	m_UiStrokeOnes->SetPosition(-485.0, -300.0f, 0.0f);
	m_UiStrokeOnes->SetScale(95.0f, 72.0f, 0.0f);
	m_UiStrokeOnes->SetUV(2, 1, 10, 1);
	m_MySceneObjects.emplace_back(m_UiStrokeOnes);

	// 打目数表示 10の位（参照を保存）
	m_UiStrokeTens = Game::GetInstance()->AddObject<Texture2D>();
	m_UiStrokeTens->SetTexture("assets/texture/number.png");
	m_UiStrokeTens->SetPosition(-556.0, -300.0f, 0.0f);
	m_UiStrokeTens->SetScale(95.0f, 72.0f, 0.0f);
	m_UiStrokeTens->SetUV(1, 1, 10, 1);
	m_MySceneObjects.emplace_back(m_UiStrokeTens);

	GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);
	ball->SetState(0);

	Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);
	arrow->SetState(0);

	Pole* pole = dynamic_cast<Pole*>(m_MySceneObjects[3]);
	pole->SetPosition(0.0f, 0.0f, -50.0f);

	Hammer* hammer = dynamic_cast<Hammer*>(m_MySceneObjects[4]);
	hammer->SetState(0);
	hammer->SetPosition(0.0f, 0.0f, 0.0f);

	//SkyBox* skybox = dynamic_cast<SkyBox*>(m_MySceneObjects[5]);
}

// 更新
void Stage1Scene::Update()
{
	GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);
	Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);
	Hammer* hammer = dynamic_cast<Hammer*>(m_MySceneObjects[4]);
	switch (m_State) {

	case 0: // ボール待機
		if(ball->GetState() == 1){
			m_State = 1;
			arrow->SetState(m_State);

			m_StrokeCount++;

			// インデックス依存を廃止（Daruma追加でズレない）
			Texture2D* count[2] = { m_UiStrokeOnes, m_UiStrokeTens };

			for (int i = 0; i < 2; i++) {
				int cnt = m_StrokeCount % (int)pow(10, i + 1) / (int)pow(10, i);
				if (count[i]) {
					count[i]->SetUV((float)(cnt + 1), 1, 10, 1);
				}
			}
		}
		if (ball->GetState() == 2) {
			Game::GetInstance()->ChangeScene(RESULT);
		}
		break;

	case 1: // 方向選択
		if (Input::GetKeyTrigger(VK_SPACE)) {
			m_State = 2;
			arrow->SetState(m_State);
		}
		break;
	case 2: // パワー選択
		// ショット
		if (Input::GetKeyTrigger(VK_SPACE)) {
			m_State = 0;
			ball->SetState(m_State);
			arrow->SetState(m_State);
			Vector3 v = arrow->GetVector();
			ball->Shot(v);
			
		}
		break;
	}
	// エンターキーを押してリザルトへ
	
	/*if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance()->ChangeScene(RESULT);
	}*/

}

// 終了処理
void Stage1Scene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance()->DeleteObject(o);
	}
	m_MySceneObjects.clear();
}

int Stage1Scene::GetScore() const
{
	return (m_StrokeCount - m_Par);
}
