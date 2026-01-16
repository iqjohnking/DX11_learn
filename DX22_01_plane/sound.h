#pragma once

#include <xaudio2.h>

// サウンドファイル
typedef enum
{
	SOUND_LABEL_BGM000 = 0,	// サンプルBGM		music.wav", true},	
	SOUND_LABEL_SE000,		// サンプルSE	   HAJIME.wav", false},	
	SOUND_LABEL_SE001,		// サンプルSE	   don.wav", false},	
	SOUND_LABEL_SE002,		// サンプルSE	   KATU.wav", false},	
	SOUND_LABEL_SE003,		// サンプルSE	   BAKUHATU.wav", false},
	SOUND_LABEL_SE004,		// サンプルSE	   YOOO.wav", false},	

	SOUND_LABEL_MAX,
} SOUND_LABEL;

class Sound {
private:
	// パラメータ構造体
	typedef struct
	{
		LPCSTR filename;	// 音声ファイルまでのパスを設定
		bool bLoop;			// trueでループ。通常BGMはture、SEはfalse。
	} PARAM;

	PARAM m_param[SOUND_LABEL_MAX] =
	{
		{"assets/music/music.wav", true},
		{"assets/music/HAJIME.wav", false},
		{"assets/music/don.wav", false},
		{"assets/music/KATU.wav", false},
		{"assets/music/BAKUHATU.wav", false},
		{"assets/music/YOOO.wav", false},
	};

	IXAudio2* m_pXAudio2 = NULL;
	IXAudio2MasteringVoice* m_pMasteringVoice = NULL;
	IXAudio2SourceVoice* m_pSourceVoice[SOUND_LABEL_MAX];
	WAVEFORMATEXTENSIBLE m_wfx[SOUND_LABEL_MAX]; // WAVフォーマット
	XAUDIO2_BUFFER m_buffer[SOUND_LABEL_MAX];
	BYTE* m_DataBuffer[SOUND_LABEL_MAX];

	HRESULT FindChunk(HANDLE, DWORD, DWORD&, DWORD&);
	HRESULT ReadChunkData(HANDLE, void*, DWORD, DWORD);

public:
	// ゲームループ開始前に呼び出すサウンドの初期化処理
	HRESULT Init(void);

	// ゲームループ終了後に呼び出すサウンドの解放処理
	void Uninit(void);

	// 引数で指定したサウンドを再生する
	void Play(SOUND_LABEL label);

	// 引数で指定したサウンドを停止する
	void Stop(SOUND_LABEL label);

	// 引数で指定したサウンドの再生を再開する
	void Resume(SOUND_LABEL label);

};