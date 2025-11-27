#include "ResultScene.h"
#include "Game.h"
#include "Input.h"

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
