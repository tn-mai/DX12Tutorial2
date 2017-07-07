/**
* @file Title.cpp
*/
#include "Title.h"
#include "../Graphics.h"
#include "../PSO.h"
#include "../GamePad.h"

using namespace DirectX;
namespace Texture = Resource;

namespace SampleScene {

/**
* タイトルシーンオブジェクトを作成する.
*
* @return 作成したタイトルシーンオブジェクトへのポインタ.
*/
::Scene::ScenePtr TitleScene::Create()
{
	return ::Scene::ScenePtr(new TitleScene);
}

/**
* コンストラクタ.
*/
TitleScene::TitleScene() : Scene(L"Title")
{
}

/**
* シーンを読み込む.
*/
bool TitleScene::Load(::Scene::Context&)
{
	Graphics::Graphics& graphics = Graphics::Graphics::Get();

	Texture::ResourceLoader loader;
	loader.Begin(graphics.csuDescriptorHeap);
	if (!loader.LoadFromFile(texture[0], 0, L"Res/UnknownPlanet.png")) {
		return false;
	}
	if (!loader.LoadFromFile(texture[1], 1, L"Res/Font.png")) {
		return false;
	}
	ID3D12CommandList* ppCommandLists[] = { loader.End() };
	graphics.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	graphics.WaitForGpu();

	const PSO& pso = GetPSO(PSOType_Sprite);
	ID3D12DescriptorHeap* texDescHeap = graphics.csuDescriptorHeap.Get();
	bundleId[0] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texture[0]);
	bundleId[1] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texture[1]);

	const XMFLOAT2 center(graphics.viewport.Width * 0.5f, graphics.viewport.Height * 0.5f);
	cellList = *Sprite::LoadFromJsonFile(L"Res/Cell/Font.cell")->Get(0);
	spriteList.push_back(Sprite::Sprite(XMFLOAT3(center.x, center.y, 0.5f)));
	spriteList.back().animeController.SetCellIndex(0x81);
	spriteList.push_back(Sprite::Sprite(XMFLOAT3(center.x , center.y * 0.75f , 0.5f)));
	spriteList.back().animeController.SetCellIndex(0x80);
	const char start[] = "START";
	XMFLOAT3 pos(center.x - 40.0f, center.y * 1.5f, 0.5f);
	XMVECTORF32 color = { 1.0f, 1, 1.0f, 1 };
	const XMVECTORF32 colorFactor = { 0.9f, 0.9f, 0.9f, 1 };
	for (const char c : start) {
		spriteList.push_back(Sprite::Sprite(XMFLOAT3(pos.x + 4, pos.y + 4, pos.z)));
		spriteList.back().color = XMFLOAT4(0, 0, 0, 0.75f);
		spriteList.back().animeController.SetCellIndex(c);
		spriteList.push_back(Sprite::Sprite(pos));
		spriteList.back().animeController.SetCellIndex(c);
		color.v *= colorFactor;
		XMStoreFloat4(&spriteList.back().color, color);
		pos.x += 16;
	}

	seStart = Audio::Engine::Get().Prepare(L"Res/SE/Start.wav");

	return true;
}

/**
* シーンを更新する.
*
* @param delta 前回の更新からの経過時間(秒).
*
* @retval ExitCode_Continue シーンを継続する.
* @retval ExitCode_Exit     シーンを終了する.
*/
int TitleScene::Update(::Scene::Context&, double delta)
{
	time += delta;

	for (Sprite::Sprite& sprite : spriteList) {
		sprite.Update(delta);
	}

	if (startTimer > 0) {
		startTimer -= delta;
		if (startTimer <= 0) {
			return ExitCode_MainGame;
		}
		const float blink = (std::fmod(startTimer, 0.25) > 0.125) ? 0.0f : 1.0f;
		for (auto itr = spriteList.begin() + 2; itr != spriteList.end(); ++itr) {
			itr->color.w = blink;
		}
	} else {
		const GamePad gamepad = GetGamePad(GamePadId_1P);
		if (gamepad.buttonDown & (GamePad::A | GamePad::B | GamePad::START)) {
			startTimer = 3;
			seStart->Play();
		}
	}
	return ExitCode_Continue;
}

/**
* シーンを描画する.
*/
void TitleScene::Draw(Graphics::Graphics& graphics) const
{
	Sprite::RenderingInfo spriteRenderingInfo;
	spriteRenderingInfo.rtvHandle = graphics.GetRTVHandle();
	spriteRenderingInfo.dsvHandle = graphics.GetDSVHandle();
	spriteRenderingInfo.viewport = graphics.viewport;
	spriteRenderingInfo.scissorRect = graphics.scissorRect;
	spriteRenderingInfo.texDescHeap = graphics.csuDescriptorHeap.Get();
	spriteRenderingInfo.matViewProjection = graphics.matViewProjection;

	const Sprite::Sprite* p = spriteList.data();
	const size_t end = spriteList.size();
	graphics.spriteRenderer.Draw(p + 0, p + 1, cellList.list.data(), bundleId[0], spriteRenderingInfo);
	graphics.spriteRenderer.Draw(p + 1, p + end, cellList.list.data(), bundleId[1], spriteRenderingInfo);
}

}