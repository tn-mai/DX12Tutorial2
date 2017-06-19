/**
* @file TitleScene.cpp
*/
#include "TitleScene.h"
#include "../Graphics.h"
#include "../PSO.h"
#include "../GamePad.h"
#include <DirectXMath.h>
#include "../Collision.h"
#include <random>
#include <time.h>

using namespace DirectX;

/**
* スプライト用セルデータ.
*/
const Sprite::Cell cellList[] = {
	{ XMFLOAT2(0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT2(800, 600) },

	{ XMFLOAT2(16.0f / 1024.0f, 48.0f / 512.0f), XMFLOAT2(480.0f / 1024.0f, 256.0f / 512.0f), XMFLOAT2(480, 256) },
};

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
* 円同士の衝突ハンドラ.
*
* @param a  衝突したエンティティの片方.
* @param b  衝突したエンティティのもう片方.
*/
void CollisionHandler(SpatialGrid::Entity& a, SpatialGrid::Entity& b)
{
	const XMVECTOR ab = XMLoadFloat3(&b.pos) - XMLoadFloat3(&a.pos);
	const XMVECTOR va = XMLoadFloat2(&a.actController.GetMove());
	const XMVECTOR vb = XMLoadFloat2(&b.actController.GetMove());

	// 互いに遠ざかるように動いていたらベクトルはそのまま.
	if (XMVectorGetX(XMVector2Dot(ab, va - vb)) <= 0) {
		return;
	}

	// 角度に応じて運動エネルギーを交換する.
	const XMVECTOR normal = XMVector2Normalize(ab);
	const XMVECTOR halfNormal = ab * XMVectorReplicate(0.5f);
	const XMVECTOR a1 = XMVectorSplatX(XMVector2Dot(normal, va));
	const XMVECTOR a2 = XMVectorSplatX(XMVector2Dot(normal, vb));
	const XMVECTOR newVa = va + normal * (a2 - a1);
	const XMVECTOR newVb = vb + normal * (a1 - a2);
	XMFLOAT2A ma, mb;
	XMStoreFloat2A(&ma, newVa);
	XMStoreFloat2A(&mb, newVb);
	a.actController.SetManualMove(ma);
	b.actController.SetManualMove(mb);
#if 1
	a.SetSeqIndex(4);
	a.SetGroupId(2);
	b.SetSeqIndex(4);
	b.SetGroupId(2);
#endif
}

void BombHandler(SpatialGrid::Entity& a, SpatialGrid::Entity& b)
{
	if (a.GroupId() == 2) {
		b.SetSeqIndex(4);
		b.SetGroupId(2);
	} else {
		a.SetSeqIndex(4);
		a.SetGroupId(2);
	}
}

/**
* 円と線分の衝突ハンドラ.
*
* @param a  衝突したエンティティの片方.
* @param b  衝突したエンティティのもう片方.
*/
void WallCollisionHandler(SpatialGrid::Entity& a, SpatialGrid::Entity& b)
{
	SpatialGrid::Entity* pA = &a;
	SpatialGrid::Entity* pB = &b;
	if (a.GroupId() == 1) {
		std::swap(pA, pB);
	}
	const Collision::Shape::Line line = pB->Shape().AsLine();
	const XMVECTOR normal = XMVectorSwizzle<1, 0, 2, 3>(XMVector2Normalize(XMLoadFloat2(&line.start) - XMLoadFloat2(&line.end)));
	XMFLOAT2 move = pA->actController.GetMove();
	XMStoreFloat2(&move, XMVector2Reflect(XMLoadFloat2(&move), normal));
	a.actController.SetManualMove(move);
}

/**
* コンストラクタ.
*/
TitleScene::TitleScene() : Scene(L"Title")
{
	world.SetWorldSize({ 800, 600 }, { 16, 12 }, 1024);
	world.RegisterHandler(0, 0, CollisionHandler);
	world.RegisterHandler(0, 1, WallCollisionHandler);
	world.RegisterHandler(0, 2, BombHandler);
}

/**
*
*/
bool TitleScene::Load(::Scene::Context&)
{
	Graphics::Graphics& graphics = Graphics::Graphics::Get();

	graphics.texMap.Begin();
	if (!graphics.texMap.LoadFromFile(texBackground, L"Res/UnknownPlanet.png")) {
		return false;
	}
	if (!graphics.texMap.LoadFromFile(texLogo, L"Res/Title.png")) {
		return false;
	}
	if (!graphics.texMap.LoadFromFile(texFont, L"Res/TextFont.png")) {
		return false;
	}
	if (!graphics.texMap.LoadFromFile(texObjects, L"Res/Objects.png")) {
		return false;
 	}
	ID3D12CommandList* ppCommandLists[] = { graphics.texMap.End() };
	graphics.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	cellFile = Sprite::LoadFromJsonFile(L"Res/Cell/CellFont.json");
	cellObjects = Sprite::LoadFromJsonFile(L"Res/Cell/CellEnemy.json");

	animationFile = LoadAnimationFromJsonFile(L"Res/Anm/AnmTitle.json");
	anmObjects = LoadAnimationFromJsonFile(L"Res/Anm/Animation.json");

	graphics.WaitForGpu();
	graphics.texMap.ResetLoader();

	sprBackground.push_back(Sprite::Sprite(animationFile[0], XMFLOAT3(400, 300, 1.0f)));
	sprBackground[0].SetSeqIndex(0);

	sprLogo.push_back(Sprite::Sprite(animationFile[0], XMFLOAT3(400, 200, 0.9f)));
	sprLogo[0].SetSeqIndex(1);

	static const char text[] = "START";
	XMFLOAT3 textPos(400 - (_countof(text) - 2) * 16, 400, 0.8f);
	for (const char c : text) {
		if (c >= ' ' && c < '`') {
			sprFont.push_back(Sprite::Sprite(animationFile[1], textPos, 0, XMFLOAT2(1, 1), XMFLOAT4(0.5f, 1.0f, 0.5f, 1.0f)));
			sprFont.back().SetSeqIndex(c - ' ');
			textPos.x += 32.0f;
		}
	}

	seStart = Audio::Engine::Get().Prepare(L"Res/SE/Start.wav");

	time = 0.0f;
	started = 0.0f;

	const PSO& pso = GetPSO(PSOType_Sprite);
	ID3D12DescriptorHeap* texDescHeap = graphics.csuDescriptorHeap.Get();
	bundleId[0] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texBackground);
	bundleId[1] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texLogo);
	bundleId[2] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texFont);
	bundleId[3] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texObjects);

	{
		using namespace Action::B;
		ptnList.resize(2);
		ptnList[0].name = "MoveTest";
		ptnList[0].data.push_back(Code(Type::Move));
		ptnList[0].data.push_back(Code(-100));
		ptnList[0].data.push_back(Code(100));
		ptnList[0].data.push_back(Code(100));
		ptnList[0].data.push_back(Code(Type::Stop));
		ptnList[0].data.push_back(Code(1));
		ptnList[0].data.push_back(Code(Type::Move));
		ptnList[0].data.push_back(Code(100));
		ptnList[0].data.push_back(Code(100));
		ptnList[0].data.push_back(Code(200));
		ptnList[1].name = "BezierTest";
		ptnList[1].data.push_back(Code(Type::Bezier));
		ptnList[1].data.push_back(Code(-400));
		ptnList[1].data.push_back(Code(600));
		ptnList[1].data.push_back(Code(400));
		ptnList[1].data.push_back(Code(600));
		ptnList[1].data.push_back(Code(0));
		ptnList[1].data.push_back(Code(0));
		ptnList[1].data.push_back(Code(4));

		actController[0].SetSeparationCount(10);
		actController[0].SetSectionCount(16);
		actController[0].SetPattern(&ptnList[1]);

		actController[1].SetSeparationCount(10);
		actController[1].SetSectionCount(16);
		actController[1].UseSimpsonsRule(true);
		actController[1].SetPattern(&ptnList[1]);

		actController[2].SetSeparationCount(100);
		actController[2].SetSectionCount(100);
		actController[2].UseSimpsonsRule(true);
		actController[2].SetPattern(&ptnList[1]);

		sprBezier.push_back(Sprite::Sprite(animationFile[1], {430, 0, 0.7f}, 0, { 1, 1 }, {0, 0, 1, 1}));
		sprBezier.back().SetSeqIndex('0' - ' ');
		sprBezier.push_back(Sprite::Sprite(animationFile[1], {370, 0, 0.7f}, 0, { 1, 1 }, {1, 0, 0, 1}));
		sprBezier.back().SetSeqIndex('0' - ' ');
		sprBezier.push_back(Sprite::Sprite(animationFile[1], {400, 0, 0.7f}, 0, { 1, 1 }, {1, 1, 1, 1}));
		sprBezier.back().SetSeqIndex('0' - ' ');

		sprBezier.push_back(Sprite::Sprite(animationFile[1], {500, 300, 0.7f}, 0, { 1, 1 }, {1, 1, 1, 1}));
		sprBezier.back().SetSeqIndex('0' - ' ');

		time_t tm;
		std::mt19937 random(::time(&tm));
		std::uniform_real<float> gen;
		const XMVECTOR baseMove = XMVectorSet(200, 0, 0, 0);
		for (int i = 0; i < 101; ++i) {
			const float x = gen(random) * (800 - 24 * 2) + 24;
			const float y = gen(random) * (600 - 24 * 2) + 24;
			SpatialGrid::Entity* p = world.AddEntity(0, anmObjects[0], { x, y, 0.5f }, Collision::Shape::MakeCircle(16));
			p->SetSeqIndex(3);
			const float r = gen(random) * 3.141592653f * 2.0f;
			const XMMATRIX m = XMMatrixRotationZ(r);
			XMFLOAT2A move;
			XMStoreFloat2A(&move, XMVector2Transform(baseMove, m));
			p->actController.SetManualMove(move);
		}
		world.AddEntity(1, anmObjects[0], { 0, 0, 0 }, Collision::Shape::MakeLine(XMFLOAT2(0, 0), XMFLOAT2(800, 0)));
		world.AddEntity(1, anmObjects[0], { 0, 0, 0 }, Collision::Shape::MakeLine(XMFLOAT2(0, 600), XMFLOAT2(800, 600)));
		world.AddEntity(1, anmObjects[0], { 0, 0, 0 }, Collision::Shape::MakeLine(XMFLOAT2(0, 0), XMFLOAT2(0, 600)));
		world.AddEntity(1, anmObjects[0], { 0, 0, 0 }, Collision::Shape::MakeLine(XMFLOAT2(800, 0), XMFLOAT2(800, 600)));
	}

	return true;
}

/**
*
*/
bool TitleScene::Unload(::Scene::Context&)
{
	return true;
}

/**
* 更新関数.
*
* @param delta 前回の呼び出しからの経過時間.
*
* @return 終了コード.
*/
int TitleScene::Update(::Scene::Context&, double delta)
{
	time += delta;
	const float brightness = static_cast<float>(std::fabs(std::fmod(time, 2.0) - 1.0));
	for (auto& e : sprFont) {
		e.color.w = brightness;
	}

	for (Sprite::Sprite& sprite : sprBackground) {
		sprite.Update(delta);
	}
	for (Sprite::Sprite& sprite : sprLogo) {
		sprite.Update(delta);
	}
	for (Sprite::Sprite& sprite : sprFont) {
		sprite.Update(delta);
	}

	for (int i = 0; i < 3; ++i) {
		actController[i].Update(sprBezier[i], delta * 0.5);
		if (actController[i].IsFinished()) {
			actController[i].Restart();
		}
	}

	// 衝突判定サンプルコード.
	{
		using namespace Collision;
		static XMFLOAT2A speed(3, 4);
		const Shape a = Shape::MakeRectangle(XMFLOAT2(-16, -16), XMFLOAT2(16, 16));
		const Shape wallTop = Shape::MakeLine(XMFLOAT2(0, 0), XMFLOAT2(800, 0));
		const Shape wallBottom = Shape::MakeLine(XMFLOAT2(0, 600), XMFLOAT2(800, 600));
		const Shape wallLeft = Shape::MakeLine(XMFLOAT2(0, 0), XMFLOAT2(0, 600));
		const Shape wallRight = Shape::MakeLine(XMFLOAT2(800, 0), XMFLOAT2(800, 600));
		const Shape planet = Shape::MakeCircle(200);
		const XMFLOAT2A planetPos(270, 320);

		XMFLOAT3& pos = sprBezier.back().pos;
		XMVECTOR v = XMLoadFloat2A(&speed) * XMVectorReplicate(static_cast<float>(delta));
		XMFLOAT2A newPos;
		XMStoreFloat2A(&newPos, XMLoadFloat3(&pos) + v);
		if (IsCollision(a, newPos, planet, planetPos)) {
			const XMVECTOR normal = XMVector2Normalize(XMLoadFloat2A(&newPos) - XMLoadFloat2A(&planetPos));
			XMStoreFloat2A(&speed, XMVector2Reflect(XMLoadFloat2A(&speed), normal));
		} else {
			if (IsCollision(a, newPos, wallTop, XMFLOAT2(0, 0))) {
				speed.y *= -1;
			} else if (IsCollision(a, newPos, wallBottom, XMFLOAT2(0, 0))) {
				speed.y *= -1;
			}
			if (IsCollision(a, newPos, wallLeft, XMFLOAT2(0, 0))) {
				speed.x *= -1;
			} else if (IsCollision(a, newPos, wallRight, XMFLOAT2(0, 0))) {
				speed.x *= -1;
			}
		}
		pos.x += speed.x;
		pos.y += speed.y;
	}

	{
		world.Update(1.0f / 60.0f);
		auto end = world.End();
		for (auto itr = world.Begin(); itr != end; ++itr) {
			if (itr->animeController.IsFinished()) {
				itr->RequestRemove();
			}
		}
		world.RemoveEntity();
	}

	if (started) {
		if (seStart->GetState() & Audio::State_Stopped) {
			return ExitCode_MainGame;
		}
	} else {
		const GamePad gamepad = GetGamePad(GamePadId_1P);
		if (gamepad.buttonDown & (GamePad::A | GamePad::B | GamePad::START)) {
			seStart->Play();
			started = true;
		}
		static uint32_t vibSeqNo = 0;
		if (gamepad.buttonDown & GamePad::X) {
			VibrateGamePad(GamePadId_1P, vibSeqNo);
		}
		if (gamepad.buttonDown & GamePad::L) {
			++vibSeqNo;
			if (vibSeqNo >= GetVibrationListSize()) {
				vibSeqNo = 0;
			}
		}
		if (gamepad.buttonDown & GamePad::R) {
			if (vibSeqNo <= 0) {
				vibSeqNo = GetVibrationListSize();
			}
			--vibSeqNo;
		}
	}
	return ExitCode_Continue;
}

/**
* シーンの描画.
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

	graphics.spriteRenderer.Draw(sprBackground, cellList, bundleId[0], spriteRenderingInfo);
	graphics.spriteRenderer.Draw(sprLogo, cellList, bundleId[1], spriteRenderingInfo);
	graphics.spriteRenderer.Draw(sprFont, cellFile->Get(0)->list.data(), bundleId[2], spriteRenderingInfo);
	graphics.spriteRenderer.Draw(sprBezier, cellFile->Get(0)->list.data(), bundleId[2], spriteRenderingInfo);
	graphics.spriteRenderer.Draw(world.Begin(), world.End(), cellObjects->Get(0)->list.data(), bundleId[3], spriteRenderingInfo);
}
