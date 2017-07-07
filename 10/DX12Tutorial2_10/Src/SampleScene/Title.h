/**
* @file Title.h
*/
#ifndef DX12TUTORIAL_SRC_SAMPLESCENE_TITLE_H_
#define DX12TUTORIAL_SRC_SAMPLESCENE_TITLE_H_
#include "../Scene.h"
#include "../Texture.h"
#include "../Sprite.h"
#include "../Audio.h"

namespace SampleScene {

class TitleScene : public Scene::Scene
{
public:
	enum {
		ExitCode_MainGame = ExitCode_User,
	};
	static ::Scene::ScenePtr Create();
	~TitleScene() = default;

	virtual bool Load(::Scene::Context&) override;
	virtual int Update(::Scene::Context&, double delta) override;
	virtual void Draw(Graphics::Graphics& graphics) const override;

private:
	TitleScene();
	TitleScene(const TitleScene&) = delete;
	TitleScene& operator=(const TitleScene&) = delete;

private:
	Resource::Texture texture[2];
	Sprite::BundleId bundleId[2];
	Audio::SoundPtr seStart;
	std::vector<Sprite::Sprite> spriteList;
	Sprite::CellList cellList;
	double time;
	double startTimer;
};

}

#endif