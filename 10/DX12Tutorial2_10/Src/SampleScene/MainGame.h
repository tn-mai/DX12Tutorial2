/**
* @file MainGameScene.h
*/
#ifndef DX12TUTORIAL_SRC_SAMPLESCENE_MAINGAMESCENE_H_
#define DX12TUTORIAL_SRC_SAMPLESCENE_MAINGAMESCENE_H_
#include "../Scene.h"
#include "../Texture.h"
#include "../Sprite.h"
#include "../Animation.h"
#include "../Action.h"
#include "../Audio.h"
#include "../SpatialGrid.h"
#include "../TimeBasedProducer.h"
#include "../ProcedualTerrain.h"

namespace SampleScene {

class MainGameScene : public Scene::Scene
{
public:
	enum ExitCode
	{
		ExitCode_Ending = ExitCode_User,
		ExitCode_Pause,
		ExitCode_GameOver,
	};

	static ::Scene::ScenePtr Create();

	virtual bool Load(::Scene::Context&) override;
	virtual int Update(::Scene::Context&, double delta) override;
	virtual void Draw(Graphics::Graphics& graphics) const override;

private:
	MainGameScene();
	MainGameScene(const MainGameScene&) = delete;
	MainGameScene& operator=(const MainGameScene&) = delete;

	void UpdatePlayer(double);
	void GenerateEnemy(double);
	void UpdateEnemy(double);
	void UpdateScore(uint32_t);
	void SolveCollision(::Scene::Context&);

	enum TexId {
		TexId_Objects,
		TexId_Font,
		TexId_BackGround,
		countof_TexId,
	};
    enum SeId {
      SeId_PlayerShot,
      SeId_Blast,
      countof_SeId,
    };

	Resource::Texture texture[countof_TexId];
	Sprite::CellList cellList[2];
	AnimationFile anmObjects;
	Action::B::PatternList ptnList;
	std::vector<Sprite::Sprite> spriteList;
    std::vector<Sprite::Sprite> spriteListFps;

	double time = 0;
	double clearTime = 0;
	double gameoverTime = 0;
	uint32_t score = 0;

	Audio::SoundPtr se[countof_SeId];

	Sprite::BundleId bundleId[countof_TexId];

	SpatialGrid::World world;
	SpatialGrid::Entity* pPlayer;
	EventProducer::TimeBasedProducer producer;
	ProcedualTerrain terrain;
};

}
#endif // DX12TUTORIAL_SRC_SAMPLESCENE_MAINGAMESCENE_H_
