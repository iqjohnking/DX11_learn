#include "Game.h"
#include "Renderer.h"
#include "Input.h"

// コンストラクタ
Game::Game()
{
}

// デストラクタ
Game::~Game()
{

}

// 初期化
void Game::Init()
{
	// 描画初期化処理
	Renderer::Init();

	Input::Create();

	// カメラ初期化
	m_Camera.Init();

	//オブジェクトを追加
	m_ObjectList.emplace_back(new GolfBall);
	auto* ball = static_cast<GolfBall*>(m_ObjectList.back().get()); //ほかのほうがいいかな？
	ball->SetCamera(&m_Camera);

	m_ObjectList.emplace_back(new Ground);

	//オブジェクト初期化
	for (auto& o: m_ObjectList) {
		o->Init();
	}


}

// 更新
void Game::Update()
{
	Input::Update();

	// カメラ更新
	m_Camera.Update();

	//オブジェクト更新
	for (auto& o : m_ObjectList) {
		o->Update();
	}

}

// 描画
void Game::Draw()
{
	// 描画前処理
	Renderer::DrawStart();

	for (auto& o : m_ObjectList) {
		o->Draw(&m_Camera);
	}

	// 描画後処理
	Renderer::DrawEnd();
}

// 終了処理
void Game::Uninit()
{
	// カメラ終了処理
	m_Camera.Uninit();

	for (auto& o : m_ObjectList) {
		o->Uninit();
	}
	Input::Release();
	// 描画終了処理
	Renderer::Uninit();
}


