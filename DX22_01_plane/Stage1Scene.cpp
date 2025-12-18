#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Ground.h"
#include "Arrow.h"

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
	// オブジェクトを作成
	Camera* cam = Game::GetInstance()->GetCamera();

	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<GolfBall>(cam));
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Ground>());
	m_MySceneObjects.emplace_back(Game::GetInstance()->AddObject<Arrow>());

	GolfBall* ball = dynamic_cast<GolfBall*>(m_MySceneObjects[0]);
	ball->SetState(0);
	Arrow* arrow = dynamic_cast<Arrow*>(m_MySceneObjects[2]);
	arrow->SetState(0);
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
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance()->ChangeScene(RESULT);
	}
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
