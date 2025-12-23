#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Ground.h"
#include "Arrow.h"
#include "Pole.h"
#include "Texture2D.h"

using namespace DirectX::SimpleMath;

// コンストラクタ
Stage1Scene::Stage1Scene()
{
	Init();
}

// デストラクタ
Stage1Scene::~Stage1Scene()
{
	Uninit();
}

// 初期化
void Stage1Scene::Init()
{
	m_Par = 4;
	m_StrokeCount = 0;

	// オブジェクトを作成
	Camera* cam = Game::GetInstance()->GetCamera();

	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<GolfBall>(cam));
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Ground>());
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Arrow>());
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Pole>());

	Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>();
	pt1->SetTexture("assets/texture/ui_back.png");
	pt1->SetPosition(-475.0, -300.0f, 0.0f);
	pt1->SetScale(270.0f, 75.0f, 0.0f);
	m_MySceneObjects.emplace_back(pt1);

	//パー
	Texture2D* pt2 = Game::GetInstance()->AddObject<Texture2D>();
	pt2->SetTexture("assets/texture/ui_string.png");
	pt2->SetPosition(-575.0, -245.0f, 0.0f);
	pt2->SetScale(60.0f, 45.0f, 0.0f);
	pt2->SetUV(1, 1, 2, 1);
	m_MySceneObjects.emplace_back(pt2);

	//打目
	Texture2D* pt3 = Game::GetInstance()->AddObject<Texture2D>();
	pt3->SetTexture("assets/texture/ui_string.png");
	pt3->SetPosition(-400.0, -305.0f, 0.0f);
	pt3->SetScale(105.0f, 63.0f, 0.0f);
	pt3->SetUV(2, 1, 2, 1);
	m_MySceneObjects.emplace_back(pt3);

	//パー数表示
	Texture2D* pt4 = Game::GetInstance()->AddObject<Texture2D>();
	pt4->SetTexture("assets/texture/number.png");
	pt4->SetPosition(-510.0, -245.0f, 0.0f);
	pt4->SetScale(65.0f, 45.0f, 0.0f);
	pt4->SetUV((float)(m_Par+1), 1, 10, 1);
	m_MySceneObjects.emplace_back(pt4);


	//打目数表示 1
	Texture2D* pt5 = Game::GetInstance()->AddObject<Texture2D>();
	pt5->SetTexture("assets/texture/number.png");
	pt5->SetPosition(-485.0, -300.0f, 0.0f);
	pt5->SetScale(95.0f, 72.0f, 0.0f);
	pt5->SetUV(2, 1, 10, 1);
	m_MySceneObjects.emplace_back(pt5);

	//打目数表示 10
	Texture2D* pt6 = Game::GetInstance()->AddObject<Texture2D>();
	pt6->SetTexture("assets/texture/number.png");
	pt6->SetPosition(-556.0, -300.0f, 0.0f);
	pt6->SetScale(95.0f, 72.0f, 0.0f);
	pt6->SetUV(1, 1, 10, 1);
	m_MySceneObjects.emplace_back(pt6);


	GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);
	ball->SetState(0);

	Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);
	arrow->SetState(0);

	Pole* pole = dynamic_cast<Pole*>(m_MySceneObjects[3]);
	pole->SetPosition(0.0f, 0.0f, -50.0f);

}

//更新
void Stage1Scene::Update()
{
	GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);
	Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);

	//
	switch (m_State) {

	case 0: // ボール待機
		if(ball->GetState() == 1){
			m_State = 1;
			arrow->SetState(m_State);

			Texture2D* count[2] = {};
			count[0] = dynamic_cast<Texture2D*>(m_MySceneObjects[8]); //?????
			count[1] = dynamic_cast<Texture2D*>(m_MySceneObjects[9]); //?????
			m_StrokeCount++;

			for (int i = 0; i < 2; i++) {
				int cnt = m_StrokeCount % (int)pow(10,i+1)/(int)pow(10,i);
				count[i]->SetUV((float)(cnt + 1), 1, 10, 1);
			}

		}
		if (ball->GetState() == 2) {
			// hole in one
			Game::GetInstance()->ChangeScene(RESULT);
		}
		break;
	case 1: // 方向選択
		if(Input::GetKeyTrigger(VK_SPACE)){
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
