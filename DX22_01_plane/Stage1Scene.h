#pragma once
#include "Scene.h"
#include "Object.h"

#include "GolfBall.h"

class Texture2D;

class Stage1Scene : public Scene
{
private:
	std::vector<Object*> m_MySceneObjects; // このシーンのオブジェクト

	int m_State = 0;
	int m_Par = 0;
	int m_StrokeCount = 0;

	// UI参照（index依存を排除）
	Texture2D* m_UiParNumber = nullptr;
	Texture2D* m_UiStrokeOnes = nullptr;
	Texture2D* m_UiStrokeTens = nullptr;

	GolfBall* m_Ball = nullptr;

	void Init(); // 初期化
	void Uninit(); // 終了処理

public:
	Stage1Scene(); // コンストラクタ
	~Stage1Scene(); // デストラクタ

	int GetScore() const;

	void Update(); // 更新
};

