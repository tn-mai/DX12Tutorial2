/**
* @file Main.cpp
*/
#include <Windows.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "Scene.h"
#include "Graphics.h"
#include "Texture.h"
#include "PSO.h"
#include "Sprite.h"
#include "Timer.h"
#include "GamePad.h"
#include "Audio.h"

#include "Scene/TitleScene.h"
#include "Scene/MainGameScene.h"
#include "Scene/PauseScene.h"
#include "Scene/GameOverScene.h"
#include "Scene/EndingScene.h"

#include "SampleScene\Title.h"
#include "SampleScene\MainGame.h"
#include "SampleScene\GameClear.h"
#include "SampleScene\GameOver.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

const wchar_t windowClassName[] = L"DX12TutorialApp";
const wchar_t windowTitle[] = L"DX12Tutorial";
const int clientWidth = 800;
const int clientHeight = 600;
HWND hwnd = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

Scene::TransitionController sceneController;
Scene::Context sceneContext;

bool InitializeD3D();
void FinalizeD3D();
bool Render();
void Update(double delta);

/**
* シーンID.
*/
enum SceneId
{
	SceneId_None = -1,
	SceneId_Title,
	SceneId_MainGame,
	SceneId_Ending,
	SceneId_Pause,
	SceneId_GameOver,
};

#define USE_SAMPLE_SCENE
#ifdef USE_SAMPLE_SCENE
/**
* シーン作成情報.
*/
static const Scene::Creator creatorList[] = {
	{ SceneId_Title, SampleScene::TitleScene::Create },
	{ SceneId_MainGame, SampleScene::MainGameScene::Create },
	{ SceneId_Ending, SampleScene::GameClearScene::Create },
	{ SceneId_Pause, PauseScene::Create },
	{ SceneId_GameOver, SampleScene::GameOverScene::Create },
};

/**
* シーン遷移情報.
*/
const Scene::Transition transitionList[] = {
	{ SceneId_Title,{ SampleScene::TitleScene::ExitCode_MainGame, Scene::TransitionType::Jump, SceneId_MainGame } },
	{ SceneId_MainGame,{ SampleScene::MainGameScene::ExitCode_Ending, Scene::TransitionType::Jump, SceneId_Ending } },
	{ SceneId_MainGame,{ SampleScene::MainGameScene::ExitCode_Pause, Scene::TransitionType::Push, SceneId_Pause } },
	{ SceneId_MainGame,{ SampleScene::MainGameScene::ExitCode_GameOver, Scene::TransitionType::Jump, SceneId_GameOver } },
	{ SceneId_Ending,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Jump, SceneId_Title } },
	{ SceneId_Pause,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Pop, 0 } },
	{ SceneId_GameOver,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Jump, SceneId_Title } },
};
#else
/**
* シーン作成情報.
*/
static const Scene::Creator creatorList[] = {
  { SceneId_Title, TitleScene::Create },
  { SceneId_MainGame, MainGameScene::Create },
  { SceneId_Ending, EndingScene::Create },
  { SceneId_Pause, PauseScene::Create },
  { SceneId_GameOver, GameOverScene::Create },
};

/**
* シーン遷移情報.
*/
const Scene::Transition transitionList[] = {
  { SceneId_Title,{ TitleScene::ExitCode_MainGame, Scene::TransitionType::Jump, SceneId_MainGame } },
  { SceneId_MainGame,{ MainGameScene::ExitCode_Ending, Scene::TransitionType::Jump, SceneId_Ending } },
  { SceneId_MainGame,{ MainGameScene::ExitCode_Pause, Scene::TransitionType::Push, SceneId_Pause } },
  { SceneId_MainGame,{ MainGameScene::ExitCode_GameOver, Scene::TransitionType::Jump, SceneId_GameOver } },
  { SceneId_Ending,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Jump, SceneId_Title } },
  { SceneId_Pause,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Pop, 0 } },
  { SceneId_GameOver,{ Scene::Scene::ExitCode_Exit, Scene::TransitionType::Jump, SceneId_Title } },
};
#endif // USE_SAMPLE_SCENE

/**
* スプライト用セルデータ.
*/
const Sprite::Cell cellList[] = {
	{ XMFLOAT2(0.0f / 32.0f, 0.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(2.0f / 32.0f, 0.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(4.0f / 32.0f, 0.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(6.0f / 32.0f, 0.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },

	{ XMFLOAT2(0.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 1.0f / 32.0f), XMFLOAT2(32, 32) },
	{ XMFLOAT2(1.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 1.0f / 32.0f), XMFLOAT2(32, 32) },
	{ XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 1.0f / 32.0f), XMFLOAT2(32, 32) },

	{ XMFLOAT2(3.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(32, 64) },
	{ XMFLOAT2(4.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(32, 64) },
	{ XMFLOAT2(5.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(32, 64) },

	{ XMFLOAT2(0.0f / 32.0f, 3.0f / 32.0f), XMFLOAT2(1.0f / 32.0f, 1.0f / 32.0f), XMFLOAT2(32, 32) },

	{ XMFLOAT2(0.0f / 32.0f, 4.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(2.0f / 32.0f, 4.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(4.0f / 32.0f, 4.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(6.0f / 32.0f, 4.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(64, 64) },
	{ XMFLOAT2(6.0f / 32.0f, 4.0f / 32.0f), XMFLOAT2(2.0f / 32.0f, 2.0f / 32.0f), XMFLOAT2(0, 0) },
};

/**
* エントリポイント.
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszClassName = windowClassName;
	RegisterClassEx(&wc);

	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	hwnd = CreateWindowEx(
		0,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	if (!InitializeD3D()) {
		return 0;
	}

	Timer timer;
	for (;;) {
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			break;
		}
        Graphics::Graphics::Get().fps = timer.GetFPS();
		Update(timer.GetFrameDelta());
		if (!Audio::Engine::Get().Update()) {
			break;
		}
		if (!Render()) {
			break;
		}
	}
	FinalizeD3D();

	CoUninitialize();

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	GamePad& gamepad = GetGamePad(GamePadId_1P);
	switch (msg) {
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE) {
			DestroyWindow(hwnd);
			return 0;
		}
		switch (wparam) {
		case 'W': gamepad.buttons |= GamePad::DPAD_UP; break;
		case 'A': gamepad.buttons |= GamePad::DPAD_LEFT; break;
		case 'S': gamepad.buttons |= GamePad::DPAD_DOWN; break;
		case 'D': gamepad.buttons |= GamePad::DPAD_RIGHT; break;
		case VK_SPACE: gamepad.buttons |= GamePad::A; break;
		case VK_RETURN: gamepad.buttons |= GamePad::START; break;
		}
		break;
	case WM_KEYUP:
		switch (wparam) {
		case 'W': gamepad.buttons &= ~GamePad::DPAD_UP; break;
		case 'A': gamepad.buttons &= ~GamePad::DPAD_LEFT; break;
		case 'S': gamepad.buttons &= ~GamePad::DPAD_DOWN; break;
		case 'D': gamepad.buttons &= ~GamePad::DPAD_RIGHT; break;
		case VK_SPACE: gamepad.buttons &= ~GamePad::A; break;
		case VK_RETURN: gamepad.buttons &= ~GamePad::START; break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool InitializeD3D()
{
	if (!Graphics::Graphics::Get().Initialize(hwnd, clientWidth, clientHeight)) {
		return false;
	}

	if (!Audio::Engine::Get().Initialize()) {
		return false;
	}

	InitGamePad();

	sceneController.Initialize(transitionList, _countof(transitionList), creatorList, _countof(creatorList));
	sceneController.Start(sceneContext, SceneId_Title);

	return true;
}

void FinalizeD3D()
{
	Graphics::Graphics::Get().WaitForGpu();
	sceneController.Stop(sceneContext);
	Audio::Engine::Get().Destroy();
	Graphics::Graphics::Get().Finalize();
}

bool Render()
{
	Graphics::Graphics& graphics = Graphics::Graphics::Get();
	if (!graphics.BeginRendering()) {
		return false;
	}
	graphics.spriteRenderer.Begin(graphics.currentFrameIndex);

	sceneController.Draw(graphics);

	graphics.spriteRenderer.End();
	if (!graphics.EndRendering()) {
		return false;
	}
	return true;
}

/**
* アプリケーションの状態を更新する.
*/
void Update(double delta)
{
	UpdateGamePad(static_cast<float>(delta));
	Graphics::Graphics& graphics = Graphics::Graphics::Get();
	for (Sprite::Sprite& sprite : graphics.spriteList) {
		sprite.Update(delta);
	}
	sceneController.Update(sceneContext, delta);
}

