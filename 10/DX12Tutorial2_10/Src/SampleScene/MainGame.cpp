/**
* @file MainGame.cpp
*/
#include "MainGame.h"
#include "../GamePad.h"
#include "../Graphics.h"
#include "../PSO.h"
#include <algorithm>

using namespace DirectX;
namespace Texture = Resource;

namespace SampleScene {

enum GroupId {
  GroupId_Player,
  GroupId_PlayerShot,
  GroupId_Enemy,
  GroupId_EnemyShot,
  GroupId_Others,
};

enum AnmSeqId {
  AnmSeqId_Player = 0,
  AnmSeqId_PlayerShot = 2,
  AnmSeqId_Enemy = 0,
  AnmSeqId_EnemyShot = 1,
  AnmSeqId_Blast = 4,
};

/**
* アクション名からIDを取得する.
*/
struct ActionIdFromName
{
  explicit ActionIdFromName(const Action::B::PatternList& ptnList) : begin(ptnList.begin()), end(ptnList.end()) {}
  uint32_t operator()(const char* name) const
  {
    const auto itr = std::find_if(begin, end, [name](const Action::B::Pattern& p) { return p.name == name; });
    if (itr == end) {
      return EventProducer::TimeBasedProducer::unknownName;
    }
    return static_cast<uint32_t>(itr - begin);
  }
  Action::B::PatternList::const_iterator begin;
  Action::B::PatternList::const_iterator end;
};

/**
* 敵の種類名からIDを取得する.
*/
struct EnemyIdFromName
{
  uint32_t operator()(const char* name) const
  {
    static const char* enemyNameList[] = { "enemy0", "enemy1" };
    static const char** const end = enemyNameList + _countof(enemyNameList);
    const auto itr = std::find_if(enemyNameList, end, [name](const char* p) { return strcmp(p, name) == 0; });
    if (itr == end) {
      return EventProducer::TimeBasedProducer::unknownName;
    }
    return static_cast<uint32_t>(itr - enemyNameList);
  }
};

/**
* タイトルシーンオブジェクトを作成する.
*
* @return 作成したタイトルシーンオブジェクトへのポインタ.
*/
::Scene::ScenePtr MainGameScene::Create()
{
  return ::Scene::ScenePtr(new MainGameScene);
}

/**
* コンストラクタ.
*/
MainGameScene::MainGameScene() : Scene(L"Title")
{
}

/**
* シーンを読み込む.
*/
bool MainGameScene::Load(::Scene::Context&)
{
  world.SetWorldSize({ 800, 600 }, { 16, 12 }, 1024);
  world.RegisterHandler(GroupId_PlayerShot, GroupId_Enemy, [&](SpatialGrid::Entity& lhs, SpatialGrid::Entity&rhs) {
    lhs.RequestRemove();
    rhs.RequestRemove();
    const XMFLOAT3 pos = lhs.GroupId() == GroupId_Enemy ? lhs.pos : rhs.pos;
    SpatialGrid::Entity* p = world.AddEntity(GroupId_Others, anmObjects[0], pos, Collision::Shape::MakeCircle(0));
    p->SetSeqIndex(AnmSeqId_Blast);
    p->SetUpdateFunc([](SpatialGrid::Entity& e) {
      if (e.animeController.IsFinished()) {
        e.RequestRemove();
      }
    });
    score += 100;
    se[SeId_Blast]->Play();
  });
  world.RegisterHandler(GroupId_Player, GroupId_Enemy, [&](SpatialGrid::Entity& lhs, SpatialGrid::Entity& rhs) {
    lhs.RequestRemove();
    rhs.RequestRemove();
    score += 100;
    se[SeId_Blast]->Play();
    gameoverTime = 3;
  });
  world.RegisterHandler(GroupId_Player, GroupId_EnemyShot, [&](SpatialGrid::Entity& lhs, SpatialGrid::Entity& rhs) {
    if (lhs.GroupId() == GroupId_Player) {
      lhs.RequestRemove();
    } else {
      rhs.RequestRemove();
    }
    se[SeId_Blast]->Play();
    gameoverTime = 3;
  });

  Graphics::Graphics& graphics = Graphics::Graphics::Get();
  Texture::ResourceLoader loader;
  loader.Begin(graphics.csuDescriptorHeap);
  static const wchar_t* const textureNameList[] = {
      L"Res/Objects.png",
      L"Res/Font.png",
      L"Res/UnknownPlanet.png",
  };
  for (int i = 0; i < countof_TexId; ++i) {
    if (!loader.LoadFromFile(texture[i], i, textureNameList[i])) {
      return false;
    }
  }
  ID3D12CommandList* ppCommandLists[] = { loader.End() };
  graphics.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
  graphics.WaitForGpu();

  const PSO& pso = GetPSO(PSOType_Sprite);
  ID3D12DescriptorHeap* texDescHeap = graphics.csuDescriptorHeap.Get();
  for (int i = 0; i < countof_TexId; ++i) {
    bundleId[i] = graphics.spriteRenderer.CreateBundle(pso, texDescHeap, texture[i]);
  }

  cellList[0] = *Sprite::LoadFromJsonFile(L"Res/Cell/Font.cell")->Get(0);
  cellList[1] = *Sprite::LoadFromJsonFile(L"Res/Cell/CellEnemy.json")->Get(0);
  ptnList = Action::B::CreateDefaultPatternList();
  anmObjects = LoadAnimationFromJsonFile(L"Res/Anm/Animation.json");
  if (!producer.LoadScheduleFromJsonFile(L"Res/Level1.sch", ActionIdFromName(ptnList), EnemyIdFromName())) {
    return false;
  }

  static const wchar_t* const soundList[] = {
    L"Res/SE/PlayerShot.wav",
    L"Res/SE/Bomb.wav"
  };
  for (int i = 0; i < countof_SeId; ++i) {
    se[i] = Audio::Engine::Get().Prepare(soundList[i]);
  }

  const XMFLOAT2 center(graphics.viewport.Width * 0.5f, graphics.viewport.Height * 0.5f);
  spriteList.push_back(Sprite::Sprite(XMFLOAT3(center.x, center.y, 1.0f)));
  spriteList.back().animeController.SetCellIndex(0x81);
  const char start[] = "00000000";
  XMFLOAT3 pos(center.x - 64.0f, 32.0f, 0.0f);
  XMVECTORF32 color = { 1, 1, 1, 1 };
  const XMVECTORF32 colorFactor = { 0.95f, 0.95f, 0.95f, 1 };
  for (const char c : start) {
    spriteList.push_back(Sprite::Sprite(pos));
    spriteList.back().animeController.SetCellIndex(c);
    color.v *= colorFactor;
    XMStoreFloat4(&spriteList.back().color, color);
    pos.x += 16;
  }

  static const XMFLOAT2 lt(-16, -16), rb(16, 16);
  pPlayer = world.AddEntity(GroupId_Player, anmObjects[0], XMFLOAT3(center.x, center.y* 1.75f, 0.5f), Collision::Shape::MakeRectangle(lt, rb));
  pPlayer->rotation = 3.14f;
  pPlayer->SetSeqIndex(AnmSeqId_Player);

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
int MainGameScene::Update(::Scene::Context&, double delta)
{
  if (gameoverTime == 0) {
    const GamePad gamepad = GetGamePad(GamePadId_1P);
    if (gamepad.buttons & GamePad::DPAD_LEFT) {
      pPlayer->pos.x -= 400.0f * static_cast<float>(delta);
    } else if (gamepad.buttons & GamePad::DPAD_RIGHT) {
      pPlayer->pos.x += 400.0f * static_cast<float>(delta);
    }
    pPlayer->pos.x = std::max(32.0f, std::min(800.0f - 32.0f, pPlayer->pos.x));
    if (gamepad.buttons & GamePad::DPAD_UP) {
      pPlayer->pos.y -= 400.0f * static_cast<float>(delta);
    } else if (gamepad.buttons & GamePad::DPAD_DOWN) {
      pPlayer->pos.y += 400.0f * static_cast<float>(delta);
    }
    pPlayer->pos.y = std::max(32.0f, std::min(600.0f - 32.0f, pPlayer->pos.y));

    if (gamepad.buttonDown & GamePad::A) {
      static const XMFLOAT2 lt(-8, -16), rb(8, 16);
      SpatialGrid::Entity* p = world.AddEntity(GroupId_PlayerShot, anmObjects[0], { pPlayer->pos.x, pPlayer->pos.y, 0.5f }, Collision::Shape::MakeRectangle(lt, rb));
      p->actController.SetManualMove({ 0, -800 });
      p->rotation = 3.14f;
      p->SetSeqIndex(AnmSeqId_PlayerShot);
      p->SetUpdateFunc([](SpatialGrid::Entity& entity) {
        if (entity.pos.x < -32 || entity.pos.x > 800 + 32) {
          entity.RequestRemove();
        } else if (entity.pos.y < -32 || entity.pos.y > 600 + 32) {
          entity.RequestRemove();
        }
      });
      se[SeId_PlayerShot]->Play();
    }
  }

  for (Sprite::Sprite& sprite : spriteList) {
    sprite.Update(delta);
  }
  world.Update(delta);
  producer.Update(delta,
    [&](uint32_t type, uint32_t action, const XMFLOAT2& pos) {
    static const XMFLOAT2 lt(-16, -16), rb(16, 16);
    SpatialGrid::Entity* p = world.AddEntity(GroupId_Enemy, anmObjects[0], { pos.x, pos.y, 0.5f }, Collision::Shape::MakeRectangle(lt, rb));
    p->SetSeqIndex(AnmSeqId_Enemy);
    p->actControllerB.SetPattern(&ptnList[std::min<uint32_t>(action, ptnList.size())]);
  });

  uint32_t scoreTmp = score;
  for (auto itr = spriteList.end() - 1; itr != spriteList.begin(); --itr) {
    itr->animeController.SetCellIndex('0' + (scoreTmp % 10));
    scoreTmp /= 10;
  }

  if (producer.IsFinish()) {
    clearTime += delta;
    if (clearTime >= 10) {
      return ExitCode_Ending;
    }
  } else if (gameoverTime > 0) {
    gameoverTime -= delta;
    if (gameoverTime <= 0) {
      return ExitCode_GameOver;
    }
  }
  return ExitCode_Continue;
}

/**
* シーンを描画する.
*/
void MainGameScene::Draw(Graphics::Graphics& graphics) const
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
graphics.spriteRenderer.Draw(p + 0, p + 1, cellList[0].list.data(), bundleId[TexId_BackGround], spriteRenderingInfo);
graphics.spriteRenderer.Draw(world.Begin(), world.End(), cellList[1].list.data(), bundleId[TexId_Objects], spriteRenderingInfo);
graphics.spriteRenderer.Draw(p + 1, p + end, cellList[0].list.data(), bundleId[TexId_Font], spriteRenderingInfo);
}

}