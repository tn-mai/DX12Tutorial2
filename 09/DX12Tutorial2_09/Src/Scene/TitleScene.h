/**
* @file TitleScene.h
*/
#ifndef DX12TUTORIAL_SRC_SCENE_TITLESCENE_H_
#define DX12TUTORIAL_SRC_SCENE_TITLESCENE_H_
#include "../Scene.h"
#include "../Texture.h"
#include "../Sprite.h"
#include "../Animation.h"
#include "../Audio.h"

#include "../SpatialGrid.h"
#include "../TimeBasedProducer.h"

class TitleScene : public Scene::Scene
{
public:
	enum ExitCode
	{
		ExitCode_MainGame,
		ExitCode_Option,
	};

	static ::Scene::ScenePtr Create();

	virtual bool Load(::Scene::Context&) override;
	virtual bool Unload(::Scene::Context&) override;
	virtual int Update(::Scene::Context&, double delta) override;
	virtual void Draw(Graphics::Graphics& graphics) const override;

private:
	TitleScene();
	TitleScene(const TitleScene&) = delete;
	TitleScene& operator=(const TitleScene&) = delete;

	Resource::Texture texBackground;
	Resource::Texture texLogo;
	Resource::Texture texFont;
	Resource::Texture texObjects;
	std::vector<Sprite::Sprite> sprBackground;
	std::vector<Sprite::Sprite> sprLogo;
	std::vector<Sprite::Sprite> sprFont;
	Sprite::FilePtr cellFile;
	Sprite::FilePtr cellObjects;
	AnimationFile animationFile;
	AnimationFile anmObjects;
	double time;
	bool started;
	Audio::SoundPtr seStart;
    Audio::SoundPtr bgm;
	Sprite::BundleId bundleId[4];

	Action::B::PatternList ptnList;
	Action::B::Controller actController[4];
	std::vector<Action::B::Controller> actForWorld;
	int lastActId = 0;

	std::vector<Sprite::Sprite> sprBezier;

	SpatialGrid::World world;
	EventProducer::TimeBasedProducer producer;
};

#endif // DX12TUTORIAL_SRC_SCENE_TITLESCENE_H_