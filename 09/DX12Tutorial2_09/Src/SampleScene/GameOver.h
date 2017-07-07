/**
* @file GameOver.h
*/
#ifndef DX12TUTORIAL_SRC_SAMPLESCENE_GAMEOVER_H_
#define DX12TUTORIAL_SRC_SAMPLESCENE_GAMEOVER_H_
#include "../Scene.h"
#include "../Texture.h"
#include "../Sprite.h"
#include "../Audio.h"

namespace SampleScene {

class GameOverScene : public Scene::Scene
{
public:
	static ::Scene::ScenePtr Create();
	~GameOverScene() = default;

	virtual bool Load(::Scene::Context&) override;
	virtual int Update(::Scene::Context&, double delta) override;
	virtual void Draw(Graphics::Graphics& graphics) const override;

private:
	GameOverScene();
	GameOverScene(const GameOverScene&) = delete;
	GameOverScene& operator=(const GameOverScene&) = delete;

private:
	Resource::Texture texture[2];
	Sprite::BundleId bundleId[2];
	std::vector<Sprite::Sprite> spriteList;
	Sprite::CellList cellList;
	double waitTimer = 1;
};

}

#endif