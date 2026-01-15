#include <chrono>
#include <thread>
#include "Application.h"
#include "Game.h"

#define SCREEN_WIDTH (1600)	// ウインドウの幅
#define SCREEN_HEIGHT (900)	// ウインドウの高さ

const auto ClassName = TEXT("2025 framework ひな型");     //ウィンドウクラス名
const auto WindowName = TEXT("2025 framework ひな型");    //ウィンドウ名

HINSTANCE  Application::m_hInst;   // インスタンスハンドル
HWND       Application::m_hWnd;    // ウィンドウハンドル
uint32_t   Application::m_Width;   // ウィンドウの横幅
uint32_t   Application::m_Height;  // ウィンドウの縦幅




static int gCurSx = 0;
static int gCurSy = 0;

static int gFrameX = -1;  // ウィンドウの左右の枠の合計
static int gFrameY = -1;  // ウィンドウの上下の枠の合計
static double gAspect = 0.0; // アスペクト比（16:9）

// 枠の太さを測るための関数
static void UpdateFrameSize(HWND hWnd)
{
	RECT rw, rc;
	GetWindowRect(hWnd, &rw);
	GetClientRect(hWnd, &rc);
	gFrameX = (rw.right - rw.left) - (rc.right - rc.left);
	gFrameY = (rw.bottom - rw.top) - (rc.bottom - rc.top);
}



extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int   AmdPowerXpressRequestHighPerformance = 0x00000001;
}

//-----------------------------------------------------------------------------
// コンストラクタ
//-----------------------------------------------------------------------------
Application::Application(uint32_t width, uint32_t height)
{ 
	m_Height = height;
	m_Width = width;

	// アスペクト比を保存
	gAspect = (double)width / (double)height;

	timeBeginPeriod(1); //タイマー精度を1ミリ秒に設定
}

//-----------------------------------------------------------------------------
// デストラクタ
//-----------------------------------------------------------------------------
Application::~Application()
{ 
	timeEndPeriod(1); // タイマー精度を元に戻す
}

//-----------------------------------------------------------------------------
// 実行
//-----------------------------------------------------------------------------
void Application::Run()
{
	//初期化
	bool okfg = InitApp();
	if (okfg) { MainLoop(); }

	UninitApp(); // 終了処理
}
//-----------------------------------------------------------------------------
// 初期化処理
//-----------------------------------------------------------------------------
bool Application::InitApp()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// インスタンスハンドルを取得
	auto hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// ウィンドウの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// ウィンドウの登録
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// インスタンスハンドル設定
	m_hInst = hInst;

	// ウィンドウのサイズを設定
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// ウィンドウサイズを調整
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// ここでモニターの中央座標を計算して CreateWindowEx に渡す
	LONG winWidth = rc.right - rc.left;
	LONG winHeight = rc.bottom - rc.top;

	// 画面解像度（プライマリモニタ）を取得
	int screenW = GetSystemMetrics(SM_CXSCREEN);
	int screenH = GetSystemMetrics(SM_CYSCREEN);

	// 中央座標を計算
	int posX = (screenW - winWidth) / 2;
	int posY = (screenH - winHeight) / 2;

	// ウィンドウを生成
	m_hWnd = CreateWindowEx(
		0,					// 拡張ウィンドウスタイル
		ClassName,			// ウィンドウクラスの名前
		WindowName,			// ウィンドウの名前
		WS_OVERLAPPEDWINDOW,	// ウィンドウスタイル
		CW_USEDEFAULT,			// ウィンドウの左上Ｘ座標
		CW_USEDEFAULT,			// ウィンドウの左上Ｙ座標 
		SCREEN_WIDTH,			// ウィンドウの幅
		SCREEN_HEIGHT,			// ウィンドウの高さ
		NULL,					// 親ウィンドウのハンドル
		NULL,					// メニューハンドルまたは子ウィンドウID
		m_hInst,				// インスタンスハンドル
		NULL);					// ウィンドウ作成データ



	// ウィンドウのサイズを修正する
	UpdateFrameSize(m_hWnd);
	int sx = SCREEN_WIDTH + gFrameX;
	int sy = SCREEN_HEIGHT + gFrameY;
	gCurSx = sx;
	gCurSy = sy;

	// ===== ここから置中ロジック =====
	// 【新增】対象モニター（ウィンドウに最も近いモニター）を取得
	HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);       // 【新增】
	MONITORINFO mi = {};                                                     // 【新增】
	mi.cbSize = sizeof(MONITORINFO);                                         // 【新增】
	GetMonitorInfo(hMon, &mi);                                               // 【新增】

	// 【新增】作業領域（タスクバー等を除く）で置中
	int workW = mi.rcWork.right - mi.rcWork.left;                            // 【新增】
	int workH = mi.rcWork.bottom - mi.rcWork.top;                            // 【新增】
	posX = mi.rcWork.left + (workW - sx) / 2;                           // 【新增】
	posY = mi.rcWork.top + (workH - sy) / 2;                           // 【新增】

	// 【變更】サイズと位置を「同時に」設定して中央へ配置（前の NOMOVE 呼び出しを置き換え）
	SetWindowPos(m_hWnd, NULL, posX, posY, sx, sy, SWP_NOZORDER | SWP_NOOWNERZORDER);                          // 【變更】
	// ===== 置中ここまで =====


	// ウィンドウを表示
	ShowWindow(m_hWnd, SW_SHOWDEFAULT);		// 指定されたウィンドウの表示状態を設定(ウィンドウを表示)


	// ウィンドウを更新
	UpdateWindow(m_hWnd);

	// ウィンドウにフォーカスを設定
	SetFocus(m_hWnd);

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	Renderer::ResizeWindow(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	SendMessage(m_hWnd, WM_KEYDOWN, VK_F11, 0);

	// 正常終了
	return true;

}

//-----------------------------------------------------------------------------
// 終了処理
//-----------------------------------------------------------------------------
void Application::UninitApp()
{
	// ウィンドウの登録を解除
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

//-----------------------------------------------------------------------------
// メインループ
//-----------------------------------------------------------------------------
void Application::MainLoop()
{
	MSG msg = {};

	//// ゲームオブジェクト
	//Game game;
	// ゲーム初期化処理
	//game.Init();

	Game::Init(); // // namespace Game 中の Init()を呼ぶ


	// FPS計測用変数
   int fpsCounter = 0;
   long long oldTick = GetTickCount64(); // 前回計測時の時間
   long long nowTick = oldTick; // 今回計測時の時間

   // FPS固定用変数
   LARGE_INTEGER liWork; // workがつく変数は作業用変数
   long long frequency;// どれくらい細かく時間をカウントできるか
   QueryPerformanceFrequency(&liWork);
   frequency = liWork.QuadPart;
   // 時間（単位：カウント）取得
   QueryPerformanceCounter(&liWork);
   long long oldCount = liWork.QuadPart;// 前回計測時の時間
   long long nowCount = oldCount;// 今回計測時の時間


   // ゲームループ
   while (1)
   {
	   // 新たにメッセージがあれば
	   if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	   {
		   // ウィンドウプロシージャにメッセージを送る
		   TranslateMessage(&msg);
		   DispatchMessage(&msg);

		   // 「WM_QUIT」メッセージを受け取ったらループを抜ける
		   if (msg.message == WM_QUIT) {
			   break;
		   }
	   }
		else
	   {
		   QueryPerformanceCounter(&liWork);// 現在時間を取得
		   nowCount = liWork.QuadPart;
		   // 1/60秒が経過したか？
		   if (nowCount >= oldCount + frequency / 60) {

			   // ゲーム更新
			   Game::Update();

			   // ゲーム描画
			   Game::Draw();

			   fpsCounter++; // ゲーム処理を実行したら＋１する
			   oldCount = nowCount;
		   }
		}
	}

   // ゲーム終了処理
   Game::Uninit();
}

//-----------------------------------------------------------------------------
// ウィンドウプロシージャ
//-----------------------------------------------------------------------------
LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZING:
	{
		RECT* r = (RECT*)lParam;
		if (gFrameX < 0 || gFrameY < 0) {
			UpdateFrameSize(hWnd);
		}

		int outerW = r->right - r->left;
		int outerH = r->bottom - r->top;
		int clientW = max(1, outerW - gFrameX);
		int clientH = max(1, outerH - gFrameY);

		switch (wParam)
		{
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		{
			int targetCH = (int)(clientW / gAspect + 0.5);
			int targetOH = targetCH + gFrameY;
			r->bottom = r->top + targetOH;

			gCurSx = r->right - r->left;
			gCurSy = r->bottom - r->top;

			return TRUE;
		}
		case WMSZ_TOP:
		case WMSZ_BOTTOM:
		{
			int targetCW = (int)(clientH * gAspect + 0.5);
			int targetOW = targetCW + gFrameX;
			r->right = r->left + targetOW;

			gCurSx = r->right - r->left;
			gCurSy = r->bottom - r->top;

			return TRUE;
		}
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
		{
			int targetCH = (int)(clientW / gAspect + 0.5);
			int targetOH = targetCH + gFrameY;

			if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT) {
				r->top = r->bottom - targetOH;
			}
			else {
				r->bottom = r->top + targetOH;
			}

			gCurSx = r->right - r->left;
			gCurSy = r->bottom - r->top;

			return TRUE;
		}
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
	{
		int res = MessageBoxA(hWnd, "終了しますか？", "確認", MB_OKCANCEL);
		if (res == IDOK) {
			DestroyWindow(hWnd);
		}
	}
	break;

	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_ESCAPE)
		{
			PostMessage(hWnd, WM_CLOSE, wParam, lParam);
		}
		else if (LOWORD(wParam) == VK_F11)
		{
			// F11 = Alt + Enter と同じ挙動にする
			Renderer::ToggleFullscreen();

		}
		break;
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) break;

		RECT rc;
		GetClientRect(hWnd, &rc);
		uint32_t w = (uint32_t)(rc.right - rc.left);
		uint32_t h = (uint32_t)(rc.bottom - rc.top);

		if (w > 0 && h > 0)
		{
			Application::SetClientSize(w, h);   // 讓 Camera 的 aspect
			Renderer::ResizeWindow((int)w, (int)h);
		}
		break;
	}
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}