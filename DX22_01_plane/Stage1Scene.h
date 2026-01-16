#pragma once

#include "Scene.h"
#include "Object.h"
#include "Daruma.h"

class Texture2D;

class Stage1Scene : public Scene
{
private:
	std::vector<Object*> m_MySceneObjects;

	int m_State = 0;
	int m_Par = 0;
	int m_StrokeCount = 0;

	// UI refs
	Texture2D* m_UiParNumber = nullptr;
	Texture2D* m_UiStrokeOnes = nullptr;
	Texture2D* m_UiStrokeTens = nullptr;

	// ---- Pending hit (locked at case1, applied in case2) ----
	Daruma::HitInfo m_PendingHit{};
	bool m_HasPendingHit = false;

	// ---- Power meter (case2) ----
	float m_Power = 0.0f;            // 0..10 UI
	float m_PowerVel = 0.0f;         // per-frame increment, accelerates
	float m_PowerAccelStep = 0.02f;   // +0.1 +0.2 +0.3 ...
	float m_PowerMax = 5.0f;

	// ---- NEW: game end ----
	enum class EndState
	{
		None,
		Win,
		Lose
	};

	EndState m_EndState = EndState::None;
	float m_EndTimer = 0.0f;                 // seconds
	static constexpr float kEndWait = 3.0f;  // seconds

	//  ---- Init ----
	void Init();
	void Uninit();

public:
	Stage1Scene();
	~Stage1Scene();

	int GetScore() const;

	void Update();
};
