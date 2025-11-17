#pragma once
#include <iostream>

#include "GolfBall.h"
#include "Ground.h"

class Game
{
private:

	// カメラ
	Camera  m_Camera;

	// テストオブジェクト
	GolfBall m_Ball;

	//Objects
	std::vector<std::unique_ptr<Object>> m_ObjectList;

public:
	Game(); // コンストラクタ
	~Game(); // デストラクタ

	void Init(); // 初期化
	void Update(); // 更新
	void Draw(); // 描画
	void Uninit(); // 終了処理
};
