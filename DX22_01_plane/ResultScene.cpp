#include "ResultScene.h"
#include "Game.h"
#include "Input.h"
#include "Texture2D.h"

// コンストラクタ
ResultScene::ResultScene()
{
	Init();
}

// デストラクタ
ResultScene::~ResultScene()
{
	Uninit();
}

// 初期化
void ResultScene::Init()
{
	// 背景画像オブジェクト生成
	Texture2D* pt = Game::GetInstance()->AddObject<Texture2D>();
	pt->SetTexture("assets/texture/background2.png");
	pt->SetScale(1920.0f, 1080.0f, 0.0f);
	m_MySceneObjects.emplace_back(pt);


	//Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>();
	//pt1->SetTexture("assets/texture/background3.png");
	//pt1->SetScale(1920.0f, 1080.0f, 0.0f);
	//m_MySceneObjects.emplace_back(pt1);

	//リザルト文字列オブジェクト生成
	Texture2D* pt1 = Game::GetInstance()->AddObject<Texture2D>();
	pt1->SetTexture("assets/texture/resultString.png");
	pt1->SetPosition(0.0f, 0.0f, 0.0f);
	pt1->SetScale(1400.0f, 200.0f, 0.0f);
	pt1->SetUV(1, 1, 1, 13);
	m_MySceneObjects.emplace_back(pt1);





}

// 更新
void ResultScene::Update()
{



	// エンターキーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_RETURN))
	{
		Game::GetInstance()->ChangeScene(TITLE);
	}
}

// 終了処理
void ResultScene::Uninit()
{
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance()->DeleteObject(o);
	}
	m_MySceneObjects.clear();
}

// スコアセット
void ResultScene::SetScore(int score, bool isWin)
{
	Texture2D* bgObj = dynamic_cast<Texture2D*>(m_MySceneObjects[0]);
	Texture2D* stringObj = dynamic_cast<Texture2D*>(m_MySceneObjects[1]);


	if (!isWin) {
		bgObj->SetTexture("assets/texture/background3.png");
		stringObj->SetUV(1, 1, 1, 13);

		Game::GetInstance()->GetSound()->Play(SOUND_LABEL_SE004);

		return;
	}


	Game::GetInstance()->GetSound()->Play(SOUND_LABEL_SE002);

	bgObj->SetTexture("assets/texture/background2.png");
	switch (score) {
	case -4:
		stringObj->SetUV(1, 2, 1, 13);
		break;
	case -3:
		stringObj->SetUV(1, 3, 1, 13);
		break;
	case -2:
		stringObj->SetUV(1, 4, 1, 13);
		break;
	case -1:
		stringObj->SetUV(1, 5, 1, 13);
		break;
	case  0:
		stringObj->SetUV(1, 6, 1, 13);
		break;
	case  1:
		stringObj->SetUV(1, 7, 1, 13);
		break;
	case  2:
		stringObj->SetUV(1, 8, 1, 13);
		break;
	case  3:
		stringObj->SetUV(1, 9, 1, 13);
		break;
	case  4:
		stringObj->SetUV(1, 10, 1, 13);
		break;
	case  5:
		stringObj->SetUV(1, 11, 1, 13);
		break;
	case  6:
		stringObj->SetUV(1, 12, 1, 13);
		break;
	default:
		stringObj->SetUV(1, 13, 1, 13);
		break;
	}
}

void ResultScene::SetIsWin(bool isWin)
{
	Texture2D* bgObj = dynamic_cast<Texture2D*>(m_MySceneObjects[0]);
	if (isWin) {
		bgObj->SetTexture("assets/texture/background2.png");
	}
	else {
		bgObj->SetTexture("assets/texture/background3.png");
	}
}
